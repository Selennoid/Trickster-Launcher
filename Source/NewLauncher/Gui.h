#pragma once
#define NOMINMAX
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#include <algorithm>
#include <string>
#include <d3d9.h>
#include <d3dx9tex.h>
#include <tchar.h>
#include "imgui.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <wil/com.h>
#include <WebView2.h>

#pragma comment(lib, "user32.lib")

class Helper;

struct TexturePair
{
	IDirect3DTexture9* color;
	IDirect3DTexture9* gray;
};

namespace gui
{
	extern ImFont* Small;
	extern ImFont* Regular;
	extern Helper* helper;
	constexpr int WIDTH = 538;
	constexpr int HEIGHT = 564;
	extern bool isRunning, isVerifying;
	extern bool check_l, option_l, exit_l, game_l, check_f;
	extern HWND window, web;
	extern WNDCLASSEX windowClass;
	extern POINTS position;
	extern PDIRECT3D9 d3d;
	extern LPDIRECT3DDEVICE9 device;
	extern D3DPRESENT_PARAMETERS presentParameters;
	extern LPDIRECT3DTEXTURE9 bgTex, logoTex;
	extern LPDIRECT3DTEXTURE9 option_n, option_h, option_s, option_g;
	extern LPDIRECT3DTEXTURE9 exit_n, exit_h, exit_s;
	extern LPDIRECT3DTEXTURE9 game_n, game_h, game_s, game_g;
	extern LPDIRECT3DTEXTURE9 check_n, check_h, check_s, check_g;
	extern std::atomic<float> g_iFileProgress, g_iTotalProgress;
	extern std::mutex g_FileStringMutex;
	extern std::string g_FileString;
	extern std::mutex g_SpeedStringMutex;
	extern std::string g_SpeedString;
	extern wil::com_ptr<ICoreWebView2Controller> g_controller;
	extern wil::com_ptr<ICoreWebView2> g_webview;
	IDirect3DTexture9* LoadTextureFromResource(HINSTANCE hInstance, int resourceId, int desiredWidth, int desiredHeight) noexcept;
	TexturePair LoadTexturePairFromResource(HINSTANCE hInstance, int resourceId, int desiredWidth, int desiredHeight) noexcept;
	void LoadResources() noexcept;
	bool TricksterImageButton(const char* id, ImTextureID normal, ImTextureID hover, ImTextureID pressed, ImTextureID locked, const ImVec2& size, bool &IsLocked) noexcept;
	LPDIRECT3DTEXTURE9 LoadTextureFromFile(const char* filename, int* out_width = nullptr, int* out_height = nullptr) noexcept;
	void CleanupDeviceD3D() noexcept;
	void CreateHWindow(const WCHAR* windowName) noexcept;
	void DestroyHWindow() noexcept;
	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;
	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;
	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;
	void SetupImGuiStyle() noexcept;
	void InitFonts( float baseFontSize = 16.0f ) noexcept;
	void OpenURL(const char* url) noexcept;
	void RenderLink(const char* label, const char* url) noexcept;
	void InitWebView(HWND hWndParent);
	bool copyAndRunSelf();
}
