#define MI_MALLOC_OVERRIDE
#include <mimalloc-new-delete.h>
#include "Gui.h"
#include "Config.h"
#include <thread>
#include <shellapi.h>
#include <fstream>
#include <string>
#include <shlwapi.h>
#include <exdisp.h>
#include <mshtml.h>
#include <oleidl.h>
#include <ocidl.h>
#include <comdef.h>
#include <filesystem>

bool RequiresAdmin(const std::wstring& folderPath) 
{
    std::wstring testFile = folderPath + L"\\perm_test.tmp";
    HANDLE hFile = CreateFileW(testFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED)
            return true;
        return false;
    }
    CloseHandle(hFile);
    DeleteFileW(testFile.c_str());
    return false;
}

void RelaunchAsAdmin(const std::wstring& exePath) 
{
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = exePath.c_str();
    sei.hwnd = NULL;
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&sei)) 
    {
        DWORD err = GetLastError();
        if (err == ERROR_CANCELLED)
            MessageBoxW(NULL, L"Administrator access is necessary!", L"Warning!", MB_ICONWARNING);
    }
}

int __stdcall wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR arguments, int commandShow)
{
    char exePath[MAX_PATH];
    wchar_t exePathW[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    GetModuleFileNameW(NULL, exePathW, MAX_PATH);
    wchar_t folderPath[MAX_PATH];
    wcscpy_s(folderPath, exePathW);
    PathRemoveFileSpecW(folderPath);
    if (RequiresAdmin(folderPath)) 
    {
        RelaunchAsAdmin(exePathW);
        return 0;
    }
    std::filesystem::path currentExe(exePath);
    std::string extension = currentExe.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return std::tolower(c); });
    if (extension == ".exe")
        if (gui::copyAndRunSelf()) 
            return 0;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return -1;
    gui::CreateHWindow(config::WindowTitle.c_str());
    gui::CreateDevice();
    gui::CreateImGui();
    gui::LoadResources();
    gui::InitWebView(gui::window);
    MSG msg = {};
    bool firstFrame = true;
    while (gui::isRunning)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                gui::isRunning = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        gui::BeginRender();
        gui::Render();
        gui::EndRender();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    gui::DestroyImGui();
    gui::DestroyDevice();
    gui::DestroyHWindow();
    CoUninitialize();
    return EXIT_SUCCESS;
}
