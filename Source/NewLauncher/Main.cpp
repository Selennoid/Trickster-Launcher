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

int __stdcall wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR arguments, int commandShow)
{
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
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
    gui::InitWebView(gui::window);
    gui::CreateImGui();
    gui::LoadResources();
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
