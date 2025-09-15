#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#include "json.hpp"
#include "resource.h"
#include <future>
#include <semaphore>

using json = nlohmann::json;

struct Arquivo 
{
    int FileID;
    std::string FileHash;
    std::string FilePath;
    bool ToUpdate = false;
};

std::string createMD5FromFile(const std::string& filePath) noexcept 
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) return {};
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> buffer((int)fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
        return {};
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_md5();
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, buffer.data(), buffer.size());
    EVP_DigestFinal_ex(context, digest, &digest_len);
    EVP_MD_CTX_free(context);
    std::ostringstream oss;
    for (unsigned int i = 0; i < digest_len; ++i)
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
    return oss.str();
}

void listarArquivosRecursivo(const std::string& basePath, const std::string& subPath, std::vector<std::string>& arquivos) noexcept
{
    std::string busca = basePath + "\\" + subPath + "\\*";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(busca.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do 
    {
        std::string nome = fd.cFileName;
        if (nome == "." || nome == "..") continue;
        std::string caminhoRelativo = subPath.empty() ? nome : (subPath + "\\" + nome);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
            listarArquivosRecursivo(basePath, caminhoRelativo, arquivos);
        else 
            arquivos.push_back(caminhoRelativo);
    } 
    while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
}

void gerarFileListJSON(const std::string& pastaRaiz) noexcept
{
	std::string launcherName = "Splash.";
    std::transform(launcherName.begin(), launcherName.end(), launcherName.begin(), [](unsigned char c) { return std::tolower(c); });
    constexpr int maxThreads = 8;
    std::counting_semaphore<maxThreads> sem(maxThreads);

    std::vector<std::string> caminhosRelativos;
    listarArquivosRecursivo(pastaRaiz, "", caminhosRelativos);

    std::vector<std::future<std::string>> futuros;
    futuros.reserve(caminhosRelativos.size());

    for (const auto& relPath : caminhosRelativos)
    {
        sem.acquire();
        std::string caminhoCompleto = pastaRaiz + "\\" + relPath;
        if (caminhoCompleto.find(launcherName))
            continue;
        futuros.emplace_back(std::async(std::launch::async, [caminhoCompleto, relPath, &sem]() {
            auto md5 = createMD5FromFile(caminhoCompleto);
            std::cout << relPath << " - " << md5 << std::endl;
            sem.release();
            return md5;
        }));
    }

    std::vector<Arquivo> arquivos;
    arquivos.reserve(caminhosRelativos.size());
    int fileID = 1;

    for (size_t i = 0; i < caminhosRelativos.size(); ++i)
    {
        std::string md5 = futuros[i].get();
        arquivos.emplace_back(Arquivo{
            fileID++,
            md5,
            caminhosRelativos[i],
            false
        });
    }

    json jArray = json::array();
    for (const auto& arq : arquivos)
    {
        jArray.push_back({
            {"FileID", arq.FileID},
            {"FileHash", arq.FileHash},
            {"FilePath", arq.FilePath},
            {"ToUpdate", arq.ToUpdate}
        });
    }

    if (std::ofstream outFile("filelist.json"); outFile)
        outFile << jArray.dump(4);

    if (std::ofstream outLauncher("launcher.txt"); outLauncher)
        outLauncher << createMD5FromFile("Update\\Splash.exe");
}

int main() 
{
    gerarFileListJSON("Update");
    return 0;
}