#include "Helper.h"
#include "Gui.h"
#include "Config.h"
#include "Language.h"
#include <direct.h>
#include <Shlwapi.h>
#include <execution>
#include <semaphore>

Helper::Helper()
{
    isMaintenance = false;
    isWorkerDone = false;
    g_PopupPage = 0;
    updateCount = 0;
    ListaArquivos.clear();
    localVersion, currentVersion = 0;
}

void Helper::GetLocalVersion()
{
    std::ifstream f("version.dat");
    localVersion = 1;
    if (f.good())
        f >> localVersion;
}

void Helper::SaveLocalVersion(int version)
{
    std::ofstream f("version.dat", std::ios::trunc);
    f << version;
}

void Helper::ParseVersionedFileLists(bool isFullCheck)
{
    if (isFullCheck)
        localVersion = 1;
    else
        GetLocalVersion();
    currentVersion = localVersion;
    ListaArquivos.clear();
    std::lock_guard<std::mutex> lockFile(gui::g_FileStringMutex);
    gui::g_FileString = lang::GetString("launcher_filelist_building");
    while (true) 
    {
        std::string path = "/version/version_" + std::to_string(currentVersion) + ".json";
        std::string jsonContent = GetFileFromURL(path);
        if (jsonContent.empty())
            break;
        nlohmann::json j = nlohmann::json::parse(jsonContent);
        if (j.is_array()) 
        {
            for (const auto& item : j) 
            {
                Arquivo a;
                a.FileID = item.value("FileID", 0);
                a.FileHash = item.value("FileHash", "");
                a.FilePath = item.value("FilePath", "");
                auto it = std::find_if(ListaArquivos.begin(), ListaArquivos.end(),
                    [&](const Arquivo& x) { return iequals(x.FilePath, a.FilePath); });
                if (it != ListaArquivos.end())
                    *it = a;
                else
                    ListaArquivos.push_back(a);
            }
        }
        currentVersion++;
    }
}

std::string Helper::GetFileFromURL(int iType)
{
    static const std::unordered_map<int, std::string> paths = {
        {0, "/maintenance.txt"},
        {1, "/launcher.txt"}
    };
    std::string path = paths.count(iType) ? paths.at(iType) : "/maintenance.txt";
    return GetFileFromURL(path);
}

std::string Helper::GetFileFromURL(const std::string& customPath)
{
    auto fetch = [&](auto& cli) -> std::string 
    {
        cli.set_follow_location(true);
        cli.set_connection_timeout(5, 0);
        auto res = cli.Get(customPath.c_str());
        if (!res || res->status != 200)
            return "";
        return res->body;
    };
    if (config::IsCDNUsingSSL) 
    {
        httplib::SSLClient cli(config::LauncherCDN.c_str());
        return fetch(cli);
    }
    else 
    {
        httplib::Client cli(config::LauncherCDN.c_str());
        return fetch(cli);
    }
}

bool Helper::iequals(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

constexpr int MAX_CONCURRENT = 8;
std::counting_semaphore<MAX_CONCURRENT> sem(MAX_CONCURRENT);

void Helper::FileCheckUpdate()
{
    gui::g_iFileProgress.store(1.0, std::memory_order_relaxed);
    std::atomic<int> count{ 0 };
    std::atomic<int> updates{ 0 };
    std::for_each(std::execution::par_unseq, ListaArquivos.begin(), ListaArquivos.end(), [&](Arquivo& file) 
    {
        sem.acquire();
        bool doesFileExists = std::ifstream(file.FilePath).good();
        if (!doesFileExists)
        {
            file.ToUpdate = true;
            updates.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            std::string fileHash = crypt::createMD5FromFile(file.FilePath);
            if (fileHash != file.FileHash)
            {
                file.ToUpdate = true;
                updates.fetch_add(1, std::memory_order_relaxed);
            }
        }
        count.fetch_add(1, std::memory_order_relaxed);
        float launcherPercent = static_cast<float>(count.load()) / static_cast<float>(ListaArquivos.size());
        launcherPercent = std::min(launcherPercent, 1.0f);
        std::string fileName = file.FilePath.substr(file.FilePath.find_last_of("/\\") + 1);
        gui::g_iTotalProgress.store(launcherPercent, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(gui::g_FileStringMutex);
            gui::g_FileString = lang::GetString("splash_check") + fileName;
        }
        sem.release();
    });
    updateCount = updates.load();
    WorkerUpdating(updateCount);
    isWorkerDone = true;
    SaveLocalVersion(currentVersion);
}

void Helper::CheckWorker(bool isFullCheck)
{
    ParseVersionedFileLists(isFullCheck);
    if (!ListaArquivos.empty() && !isWorkerDone)
    {
        if (workerThread.joinable())
            workerThread.join();
        workerThread = std::thread(&Helper::FileCheckUpdate, this);
    }
    else
    {
        std::lock_guard<std::mutex> lockFile(gui::g_FileStringMutex);
        gui::g_FileString = lang::GetString("launcher_worker_complete");
        gui::g_iFileProgress.store(1.f, std::memory_order_relaxed);
        gui::g_iTotalProgress.store(1.f, std::memory_order_relaxed);
        isWorkerDone = true;
    }
}

std::string Helper::GetDirectoryFromPath(const std::string& filepath)
{
    size_t pos = filepath.find_last_of("/\\");
    return (pos != std::string::npos) ? filepath.substr(0, pos) : "";
}

bool Helper::CreateDirectoryIfNotExists(const std::string& dirPath)
{
    std::string fixedDirPath = dirPath;
    std::replace(fixedDirPath.begin(), fixedDirPath.end(), '\\', '/');
    std::istringstream iss(fixedDirPath);
    std::string token;
    std::string path;
    while (std::getline(iss, token, '/'))
    {
        if (token.empty()) continue;
        path += token + "/";
        _mkdir(path.c_str());
    }
    return true;
}

bool Helper::DownloadFile(const std::string& remoteFile, const std::string& localPath, int fileIndex, int totalFiles)
{
    auto doDownload = [&](auto& cli) -> bool
    {
        cli.set_follow_location(true);
        cli.set_connection_timeout(5, 0);
        std::ofstream out(localPath, std::ios::binary);
        if (!out) return false;
        long downloaded = 0;
        long contentLength = 0;
        auto startTime = std::chrono::steady_clock::now();
        auto res = cli.Get(("/Update/" + remoteFile).c_str(), [&](const httplib::Response& response)
        {
            auto it = response.headers.find("Content-Length");
            if (it != response.headers.end())
                contentLength = std::stol(it->second);
            return true;
        }, [&](const char* data, size_t len)
        {
            out.write(data, len);
            downloaded += static_cast<long>(len);
            if (contentLength > 0)
                gui::g_iFileProgress.store(static_cast<float>(downloaded) / contentLength, std::memory_order_relaxed);
            gui::g_iTotalProgress.store((static_cast<float>(fileIndex - 1) + gui::g_iFileProgress.load()) / totalFiles, std::memory_order_relaxed);
            auto now = std::chrono::steady_clock::now();
            double elapsed = std::chrono::duration<double>(now - startTime).count();
            double speed = elapsed > 0 ? downloaded / elapsed : 0;
            const char* units[] = { "B/s", "KB/s", "MB/s", "GB/s", "TB/s" };
            int i = 0;
            while (speed >= 1024 && i < 4)
            {
                speed /= 1024;
                i++;
            }
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << speed << " " << units[i];
            std::lock_guard<std::mutex> lock(gui::g_SpeedStringMutex);
            gui::g_SpeedString = oss.str();
            return true;
        });
        std::lock_guard<std::mutex> lock(gui::g_SpeedStringMutex);
        gui::g_SpeedString.clear();
        return res && res->status == 200;
    };
    if (config::IsCDNUsingSSL)
    {
        httplib::SSLClient cli(config::LauncherCDN.c_str());
        return doDownload(cli);
    }
    else
    {
        httplib::Client cli(config::LauncherCDN.c_str());
        return doDownload(cli);
    }
}

void Helper::WorkerUpdating(int updateCount)
{
    int index = 1;
    gui::g_iTotalProgress.store(0.0f, std::memory_order_relaxed);
    if (updateCount > 0)
    {
        for (const auto& file : ListaArquivos)
        {
            if (!file.ToUpdate) continue;
            gui::g_iFileProgress = 0.0f;
            std::string fileName = file.FilePath.substr(file.FilePath.find_last_of("/\\") + 1);
            std::string remotePath = file.FilePath;
            std::replace(remotePath.begin(), remotePath.end(), '\\', '/');
            std::string dirPath = GetDirectoryFromPath(file.FilePath);
            CreateDirectoryIfNotExists(dirPath);
            std::remove(file.FilePath.c_str());
            {
                std::lock_guard<std::mutex> lockFile(gui::g_FileStringMutex);
                gui::g_FileString = lang::GetString("launcher_worker_downloading") + ": " + fileName;
            }
            DownloadFile(remotePath, file.FilePath, index, updateCount);
            index++;
        }
    }
    gui::g_iFileProgress.store(1.0f, std::memory_order_relaxed);
    gui::g_iTotalProgress.store(1.0f, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lockFile(gui::g_FileStringMutex);
        if (!isMaintenance)
        {
            gui::g_FileString = lang::GetString("launcher_worker_complete");
        }
        else
            gui::g_FileString = lang::GetString("launcher_worker_maintenance");
    }
}

bool Helper::InjectDLL(HANDLE hProcess, const std::string& dllPath)
{
    LPVOID pRemote = VirtualAllocEx(hProcess, nullptr, strlen(dllPath.c_str()) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemote)
        return false;
    if (!WriteProcessMemory(hProcess, pRemote, dllPath.c_str(), strlen(dllPath.c_str()) + 1, nullptr))
    {
        VirtualFreeEx(hProcess, pRemote, 0, MEM_RELEASE);
        return false;
    }
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, pRemote, 0, nullptr);
    if (!hThread)
    {
        VirtualFreeEx(hProcess, pRemote, 0, MEM_RELEASE);
        return false;
    }
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemote, 0, MEM_RELEASE);
    return true;
}

std::filesystem::path Helper::GetGamePath()
{
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    std::filesystem::path exePath(buffer);
    std::filesystem::path dir = exePath.parent_path();
    return dir;
}

void Helper::ClickPlayButton()
{
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    std::filesystem::path gameExe = GetGamePath() / "Trickster.exe";
    std::string exePath = gameExe.string();
    if (!CreateProcessA(exePath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
        return;
    if (config::IsDllInjectEnable)
    {
        Sleep(2000);
        if (!InjectDLL(pi.hProcess, config::InjectDLLName.c_str()))
        {
            TerminateProcess(pi.hProcess, 0);
            return;
        }
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    ExitProcess(0);
}

void Helper::UpdateLauncher()
{
    std::string launcherName = "Splash.exe";
    std::transform(launcherName.begin(), launcherName.end(), launcherName.begin(), [](unsigned char c) { return std::tolower(c); });
    std::string fileHash = crypt::createMD5FromFile(launcherName);
    std::string remoteLauncherHash = GetFileFromURL(1);
    if (remoteLauncherHash == "")
    {
        MessageBoxA(NULL, lang::GetString("launcher_update_check_fail").c_str(), "Error!", MB_OK);
        PostQuitMessage(0);
    }
    if (fileHash != remoteLauncherHash)
    {
        if (DownloadFile(launcherName, launcherName, 1, 1))
        {
            std::string currentExe;
            char exePath[MAX_PATH];
            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
            currentExe = exePath;
            STARTUPINFOA si = { sizeof(si) };
            PROCESS_INFORMATION pi;
            if (!CreateProcessA(currentExe.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
            {
                MessageBoxA(NULL, lang::GetString("launcher_update_launch_fail").c_str(), "Error!", MB_OK);
                PostQuitMessage(0);
            }
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            ExitProcess(0);
        }
        else
        {
            MessageBoxA(NULL, lang::GetString("launcher_update_download_fail").c_str(), "Error!", MB_OK);
            PostQuitMessage(0);
        }
    }
}
