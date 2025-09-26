#pragma once
#include "Gui.h"
#include <vector>
#include <string>
#include <unordered_map>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx9.h"
#include "Helper.h"
#include "Language.h"
#include "Config.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <imgui_internal.h>
#include "resource.h"
#include <wrl.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter );

namespace gui 
{
	static bool showMainWindow = true;
	ImFont* Small = new ImFont();
	ImFont* Regular = new ImFont();
	Helper* helper = nullptr;
	std::atomic<float> g_iFileProgress{ 0.f };
	std::atomic<float> g_iTotalProgress{ 0.f };
	std::mutex g_FileStringMutex;
	std::string g_FileString;
	std::mutex g_SpeedStringMutex;
	std::string g_SpeedString = "0 B/s";
	LPDIRECT3DTEXTURE9 bgTex = nullptr, logoTex = nullptr;
	LPDIRECT3DTEXTURE9 option_n = nullptr, option_h = nullptr, option_s = nullptr, option_g = nullptr;
	LPDIRECT3DTEXTURE9 exit_n = nullptr, exit_h = nullptr, exit_s = nullptr;
	LPDIRECT3DTEXTURE9 game_n = nullptr, game_h = nullptr, game_s = nullptr, game_g = nullptr;
	LPDIRECT3DTEXTURE9 check_n = nullptr, check_h = nullptr, check_s = nullptr, check_g = nullptr;
	bool check_l = true, option_l = true, exit_l = false, game_l = true, check_f = false;
	bool isRunning = true, isVerifying = false;
	HWND window = nullptr;
	HWND web = nullptr;
	WNDCLASSEX windowClass = {};
	POINTS position = {};
	PDIRECT3D9 d3d = nullptr;
	LPDIRECT3DDEVICE9 device = nullptr;
	D3DPRESENT_PARAMETERS presentParameters = {};
	wil::com_ptr<ICoreWebView2Controller> g_controller = nullptr;
	wil::com_ptr<ICoreWebView2> g_webview = nullptr;
}

long __stdcall WindowProcess( HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter )
{
	if ( ImGui_ImplWin32_WndProcHandler( window, message, wideParameter, longParameter ) )
		return true;
	switch ( message )
	{
		case WM_SIZE: 
		{
			if ( gui::device && wideParameter != SIZE_MINIMIZED )
			{
				gui::presentParameters.BackBufferWidth = LOWORD( longParameter );
				gui::presentParameters.BackBufferHeight = HIWORD( longParameter );
				gui::ResetDevice();
			}
		} return 0;
		case WM_MOVE:
			if (gui::g_controller)
			{
				RECT bounds = { 19, 32, 19 + 503, 32 + 343 };
				gui::g_controller->put_Bounds(bounds);
			}
			break;
		case WM_SYSCOMMAND: 
		{
			if ( ( wideParameter & 0xfff0 ) == SC_KEYMENU )
				return 0;
		} break;
		case WM_DESTROY: 
		{
			ExitProcess(0);
		} return 0;
		case WM_LBUTTONDOWN: 
		{
			gui::position = MAKEPOINTS( longParameter );
		} return 0;
		case WM_MOUSEMOVE: 
		{
			if ( wideParameter == MK_LBUTTON )
			{
				const auto points = MAKEPOINTS( longParameter );
				auto rect = ::RECT{};
				GetWindowRect( gui::window, &rect );
				rect.left += points.x - gui::position.x;
				rect.top += points.y - gui::position.y;
				if (gui::position.x >= 0 && gui::position.x <= gui::WIDTH && gui::position.y >= 0 && gui::position.y <= 19 )
					SetWindowPos( gui::window, HWND_TOPMOST, rect.left, rect.top, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER );
			}

		} return 0;
	}
	return DefWindowProc(window, message, wideParameter, longParameter);
}

void gui::InitWebView(HWND hWndParent)
{
	wchar_t tempPath[MAX_PATH];
	GetTempPathW(MAX_PATH, tempPath);
	std::wstring cacheFolder = std::wstring(tempPath) + L"WebView2Cache";
	CreateDirectoryW(cacheFolder.c_str(), nullptr);
	CreateCoreWebView2EnvironmentWithOptions(
		nullptr, cacheFolder.c_str(), nullptr,
		Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
			[hWndParent](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
			{
				env->CreateCoreWebView2Controller(
					hWndParent,
					Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
						[hWndParent](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT
						{
							if (controller != nullptr)
							{
								g_controller = controller;
								g_controller->get_CoreWebView2(&g_webview);
							}
							RECT bounds = { 19, 32, 19 + 503, 32 + 343 };
							g_controller->put_Bounds(bounds);
							if (g_webview)
								g_webview->Navigate(config::BaseNewsURL.c_str());
							return S_OK;
						}
					).Get());
				return S_OK;
			}
		).Get());
}

IDirect3DTexture9* gui::LoadTextureFromResource(HINSTANCE hInstance, int resourceId, int desiredWidth, int desiredHeight) noexcept
{
	HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(resourceId), L"PNG");
	if (!hRes) return nullptr;
	DWORD resSize = SizeofResource(hInstance, hRes);
	HGLOBAL hMem = LoadResource(hInstance, hRes);
	void* pData = LockResource(hMem);
	IDirect3DTexture9* texture = nullptr;
	HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(device, pData, resSize, desiredWidth, desiredHeight, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, nullptr, nullptr, &texture);
	return SUCCEEDED(hr) ? texture : nullptr;
}

TexturePair gui::LoadTexturePairFromResource(HINSTANCE hInstance, int resourceId, int desiredWidth, int desiredHeight) noexcept
{
	TexturePair result = { nullptr, nullptr };
	HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(resourceId), L"PNG");
	if (!hRes) return result;
	DWORD resSize = SizeofResource(hInstance, hRes);
	HGLOBAL hMem = LoadResource(hInstance, hRes);
	void* pData = LockResource(hMem);
	IDirect3DTexture9* texColor = nullptr;
	HRESULT hr = D3DXCreateTextureFromFileInMemoryEx(device, pData, resSize, desiredWidth, desiredHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0, nullptr, nullptr, &texColor);
	if (FAILED(hr) || !texColor)
		return result;
	result.color = texColor;
	IDirect3DTexture9* texGray = nullptr;
	hr = device->CreateTexture(desiredWidth, desiredHeight, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &texGray, NULL);
	if (FAILED(hr) || !texGray)
		return result;
	D3DLOCKED_RECT rectSrc, rectDst;
	texColor->LockRect(0, &rectSrc, NULL, D3DLOCK_READONLY);
	texGray->LockRect(0, &rectDst, NULL, 0);
	for (int y = 0; y < desiredHeight; y++)
	{
		DWORD* src = (DWORD*)((BYTE*)rectSrc.pBits + y * rectSrc.Pitch);
		DWORD* dst = (DWORD*)((BYTE*)rectDst.pBits + y * rectDst.Pitch);
		for (int x = 0; x < desiredWidth; x++)
		{
			DWORD c = src[x];
			BYTE a = (c >> 24) & 0xFF;
			BYTE r = (c >> 16) & 0xFF;
			BYTE g = (c >> 8) & 0xFF;
			BYTE b = (c >> 0) & 0xFF;
			BYTE gray = (BYTE)(0.299f * r + 0.587f * g + 0.114f * b);
			dst[x] = (a << 24) | (gray << 16) | (gray << 8) | gray;
		}
	}
	texColor->UnlockRect(0);
	texGray->UnlockRect(0);
	result.gray = texGray;
	return result;
}

void gui::LoadResources() noexcept 
{ 
	bgTex = LoadTextureFromResource(windowClass.hInstance, IDB_BG, 538, 564); 
	logoTex = LoadTextureFromResource(windowClass.hInstance, IDB_LOGO, 185, 71); 
	TexturePair optionTex = LoadTexturePairFromResource(windowClass.hInstance, OPTION_N, 113, 27); 
	option_n = optionTex.color; 
	option_g = optionTex.gray; 
	option_h = LoadTextureFromResource(windowClass.hInstance, OPTION_H, 113, 27); 
	option_s = LoadTextureFromResource(windowClass.hInstance, OPTION_S, 113, 27); 
	exit_n = LoadTextureFromResource(windowClass.hInstance, EXIT_N, 113, 27); 
	exit_h = LoadTextureFromResource(windowClass.hInstance, EXIT_H, 113, 27);
	exit_s = LoadTextureFromResource(windowClass.hInstance, EXIT_S, 113, 27); 
	TexturePair gameTex = LoadTexturePairFromResource(windowClass.hInstance, GAME_N, 118, 61); 
	game_n = gameTex.color; 
	game_g = gameTex.gray; 
	game_h = LoadTextureFromResource(windowClass.hInstance, GAME_H, 118, 61); 
	game_s = LoadTextureFromResource(windowClass.hInstance, GAME_S, 118, 61);
	TexturePair checkTex = LoadTexturePairFromResource(windowClass.hInstance, CHECK_N, 113, 27);
	check_n = checkTex.color;
	check_g = checkTex.gray;
	check_h = LoadTextureFromResource(windowClass.hInstance, CHECK_H, 113, 27);
	check_s = LoadTextureFromResource(windowClass.hInstance, CHECK_S, 113, 27);
}

void gui::CreateHWindow( const WCHAR* windowName ) noexcept
{
	windowClass.cbSize = sizeof( WNDCLASSEX );
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA( 0 );
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = L"class001";
	windowClass.hIconSm = 0;
	RegisterClassEx( &windowClass );
	window = CreateWindowExW( 0, L"class001", windowName, WS_POPUP, 100, 100, WIDTH, HEIGHT, 0, 0, windowClass.hInstance, 0 );
	ShowWindow( window, SW_SHOWDEFAULT );
	UpdateWindow( window );
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow( window );
	UnregisterClass( windowClass.lpszClassName, windowClass.hInstance );
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9( D3D_SDK_VERSION) ;
	if ( !d3d )
		return false;
	ZeroMemory( &presentParameters, sizeof( presentParameters ) );
	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	if ( d3d->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window, D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParameters, &device ) < 0 )
		return false;
	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = device->Reset( &presentParameters );
	if ( result == D3DERR_INVALIDCALL )
		IM_ASSERT( 0 );
	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if ( device )
	{
		device->Release();
		device = nullptr;
	}
	if ( d3d )
	{
		d3d->Release();
		d3d = nullptr;
	}
}

void gui::SetupImGuiStyle() noexcept
{
    ImGuiStyle& style               = ImGui::GetStyle();
    style.Colors[ ImGuiCol_Text ]   = ImVec4( 1.0f, 1.0f, 1.0f, 1.0f );
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
}

void gui::InitFonts( float baseFontSize ) noexcept
{
	const char* regularFontPath = "Fontes/NotoSans-Bold.ttf";
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesChineseFull();
	float smallFontSize = baseFontSize * 0.8f;
	auto LoadFont = [&](const char* path, float size) -> ImFont* 
	{
		ImFontConfig fontCfg;
		fontCfg.MergeMode = false;
		ImFont* font = io.Fonts->AddFontFromFileTTF(path, size, &fontCfg, glyphRanges);
		if (!font) return nullptr;
		return font;
	};
	Small = LoadFont(regularFontPath, smallFontSize);
	Regular = LoadFont(regularFontPath, baseFontSize);
	io.FontDefault = Regular ? Regular : io.Fonts->Fonts[0];
	io.Fonts->Build();
}

void gui::OpenURL(const char* url) noexcept
{
	ShellExecuteA(0, "open", url, 0, 0, SW_SHOWNORMAL);
}

void gui::RenderLink(const char* label, const char* url) noexcept
{
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 209, 97, 255));
	if (ImGui::Text("%s", label); ImGui::IsItemHovered()) 
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		if (ImGui::IsItemClicked())
			OpenURL(url);
	}
	ImGui::PopStyleColor();
}

std::vector<BYTE> Base64Decode(const std::wstring& input) 
{
	DWORD required = 0;
	if (!CryptStringToBinaryW(input.c_str(), 0, CRYPT_STRING_BASE64, nullptr, &required, nullptr, nullptr))
		return {};
	std::vector<BYTE> output(required);
	if (!CryptStringToBinaryW(input.c_str(), 0, CRYPT_STRING_BASE64, output.data(), &required, nullptr, nullptr))
		return {};
	output.resize(required);
	return output;
}

std::vector<unsigned char> DecodePNGToRGBA(const unsigned char* data, size_t size, int& w, int& h)
{
	int channels = 0;
	unsigned char* pixels = stbi_load_from_memory(data, (int)size, &w, &h, &channels, 4);
	std::vector<unsigned char> out;
	if (pixels) 
	{
		out.assign(pixels, pixels + (w * h * 4));
		stbi_image_free(pixels);
	}
	return out;
}

void gui::CreateImGui() noexcept
{
	std::string maintenanceCheck = "";
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	if (helper == nullptr)
	{
		helper = new Helper();
		maintenanceCheck = helper->GetFileFromURL(0);
	}
	if (maintenanceCheck == "")
	{
		MessageBoxA(NULL, "Error!", NULL, MB_OK);
		PostQuitMessage(0);
	}
	else
	{
		if (maintenanceCheck == "true")
			helper->isMaintenance = true;
		else
			helper->isMaintenance = false;
	}
    InitFonts();
    ImGuiIO& io = ImGui::GetIO();
    SetupImGuiStyle();
	io.IniFilename = NULL;
	ImGui_ImplWin32_Init( window );
	ImGui_ImplDX9_Init( device );
}

void gui::CleanupDeviceD3D() noexcept
{
	if (device) { device->Release(); device = NULL; }
	if (d3d) { d3d->Release(); d3d = NULL; }
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while ( PeekMessage( &message, 0, 0, 0, PM_REMOVE ) )
	{
		TranslateMessage( &message );
		DispatchMessage( &message );
		if ( message.message == WM_QUIT )
		{
			isRunning = !isRunning;
			return;
		}
	}
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();
	device->SetRenderState( D3DRS_ZENABLE, FALSE );
	device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	device->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
	device->Clear( 0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA( 0, 0, 0, 255 ), 1.0f, 0 );
	if ( device->BeginScene() >= 0 )
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData() );
		device->EndScene();
	}
	const auto result = device->Present( 0, 0, 0, 0 );
	if ( result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET )
		ResetDevice();
}

LPDIRECT3DTEXTURE9 gui::LoadTextureFromFile(const char* filename, int* out_width, int* out_height) noexcept
{
	int w, h, c;
	unsigned char* data = stbi_load(filename, &w, &h, &c, 4);
	if (!data) return nullptr;
	LPDIRECT3DTEXTURE9 tex = nullptr;
	if (device->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, NULL) == D3D_OK) {
		D3DLOCKED_RECT rect;
		tex->LockRect(0, &rect, NULL, 0);
		for (int y = 0; y < h; ++y) {
			DWORD* dst = (DWORD*)((BYTE*)rect.pBits + y * rect.Pitch);
			BYTE* src = data + y * w * 4;
			for (int x = 0; x < w; ++x) {
				BYTE r = src[x * 4 + 0];
				BYTE g = src[x * 4 + 1];
				BYTE b = src[x * 4 + 2];
				BYTE a = src[x * 4 + 3];
				dst[x] = D3DCOLOR_ARGB(a, r, g, b);
			}
		}
		tex->UnlockRect(0);
		if (out_width) *out_width = w;
		if (out_height) *out_height = h;
	}
	stbi_image_free(data);
	return tex;
}

bool gui::TricksterImageButton(const char* id, ImTextureID normal, ImTextureID hover, ImTextureID pressed, ImTextureID locked, const ImVec2& size, bool& IsLocked) noexcept
{
	ImGui::PushID(id);
	bool pressed_result = ImGui::InvisibleButton("##btn", size);
	bool hovered = ImGui::IsItemHovered();
	bool held = ImGui::IsItemActive();
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImTextureID tex = normal;
	if (!IsLocked)
	{
		if (held)
			tex = pressed;
		else if (hovered)
			tex = hover;
	}
	else
		tex = locked;
	dl->AddImage(tex, min, max);
	ImGui::PopID();
	return pressed_result;
}

void gui::Render() noexcept
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ (float)WIDTH, (float)HEIGHT });
	bool opened = ImGui::Begin(" ", &showMainWindow, &showMainWindow, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
	if (opened)
	{
		ImGui::GetWindowDrawList()->AddImage((ImTextureID)bgTex, ImVec2(0, 0), io.DisplaySize);
		ImVec2 winSize = ImGui::GetWindowSize();
		ImVec2 SubTitleSize = ImGui::CalcTextSize(config::SubTitle.c_str());
		ImGui::SetCursorPos(ImVec2(winSize.x - SubTitleSize.x - 15, 8));
		ImGui::Text("%s", config::SubTitle.c_str());
		ImGui::SetCursorPos(ImVec2(11, (winSize.y - 98)));
		ImGui::Image((ImTextureID)logoTex, ImVec2(185, 71));
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		float fileProgress = g_iFileProgress.load(std::memory_order_relaxed);
		float totalProgress = g_iTotalProgress.load(std::memory_order_relaxed);
		ImGui::SetCursorPos(ImVec2(24, (winSize.y - 165)));
		ImGui::ProgressBar(fileProgress, ImVec2(362, 7), " ");
		ImGui::PopStyleColor(2);
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(133 / 255.f, 242 / 255.f, 254 / 255.f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(133 / 255.f, 242 / 255.f, 254 / 255.f, 1.0f));
		ImGui::SetCursorPos(ImVec2(24, (winSize.y - 152)));
		ImGui::ProgressBar(totalProgress, ImVec2(362, 12), " ");
		ImGui::PopStyleColor(2);
		ImGui::SetCursorPos(ImVec2(23, (winSize.y - 185)));
		static std::string cachedFileString, cachedSpeedString;
		static auto lastStringUpdate = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		if (now - lastStringUpdate > std::chrono::milliseconds(100))
		{
			std::lock_guard<std::mutex> lockFile(g_FileStringMutex);
			cachedFileString = g_FileString;
			std::lock_guard<std::mutex> lockSpeed(g_SpeedStringMutex);
			cachedSpeedString = g_SpeedString;
			lastStringUpdate = now;
		}
		ImGui::Text("%s", cachedFileString.c_str());
		ImVec2 textSize = ImGui::CalcTextSize(cachedSpeedString.c_str());
		ImGui::SetCursorPos(ImVec2(winSize.x - textSize.x - 24, winSize.y - 185));
		ImGui::Text("%s", cachedSpeedString.c_str());
		ImGui::SetCursorPos(ImVec2((winSize.x - 328), (winSize.y - 76)));
		ImGui::Text("%s", lang::GetString("launcher_site_desc").c_str());
		ImGui::SetCursorPos(ImVec2((winSize.x - 328), (winSize.y - 60)));
		RenderLink(lang::GetString("launcher_site_click").c_str(), config::WebsiteLink.c_str());
		ImGui::SetCursorPos(ImVec2(23, winSize.y - 132));
		if (TricksterImageButton(lang::GetString("launcher_check").c_str(), (ImTextureID)check_n, (ImTextureID)check_h, (ImTextureID)check_s, (ImTextureID)check_g, ImVec2(113, 27), check_l))
		{
			if (!check_l)
			{
				isVerifying = false;
				check_f = true;
				helper->isWorkerDone = false;
			}
		}
		ImGui::SetCursorPos(ImVec2(150, winSize.y - 132));
		if (TricksterImageButton(lang::GetString("launcher_options").c_str(), (ImTextureID)option_n, (ImTextureID)option_h, (ImTextureID)option_s, (ImTextureID)option_g, ImVec2(113, 27), option_l))
		{
			if (!option_l)
			{
				STARTUPINFOA si = { sizeof(si) };
				PROCESS_INFORMATION pi;
				std::filesystem::path gameExe = helper->GetGamePath() / config::OptionExecName.c_str();
				std::string exePath = gameExe.string();
				if (!CreateProcessA(exePath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
					MessageBoxA(NULL, lang::GetString("launcher_setup_fail").c_str(), "Error!", MB_OK);
			}
		}
		ImGui::SetCursorPos(ImVec2(274, winSize.y - 132));
		if (TricksterImageButton(lang::GetString("launcher_exit").c_str(), (ImTextureID)exit_n, (ImTextureID)exit_h, (ImTextureID)exit_s, (ImTextureID)exit_s, ImVec2(113, 27), exit_l))
		{
			isRunning = false;
		}
		ImGui::SetCursorPos(ImVec2((winSize.x - 139), winSize.y - 166));
		if (TricksterImageButton(lang::GetString("launcher_game_start").c_str(), (ImTextureID)game_n, (ImTextureID)game_h, (ImTextureID)game_s, (ImTextureID)game_g, ImVec2(118, 61), game_l))
		{
			if (!game_l)
				helper->ClickPlayButton();
		}
		if (helper->isWorkerDone)
		{
			check_l = false;
			option_l = false;
			if (!helper->isMaintenance)
				game_l = false;
		}
		else
		{
			check_l = true;
			option_l = true;
			game_l = true;
		}
		if (!isVerifying)
		{
			helper->UpdateLauncher();
			helper->CheckWorker(check_f);
			isVerifying = true;
		}
		ImGui::End();
	}
	if (!showMainWindow)
		isRunning = false;
}

bool gui::copyAndRunSelf() 
{
	char exePath[MAX_PATH];
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	std::filesystem::path currentExe(exePath);
	std::filesystem::path copyExe = currentExe.parent_path() / "Splash.dmy";
	try 
	{
		std::filesystem::copy_file(currentExe, copyExe, std::filesystem::copy_options::overwrite_existing);
		STARTUPINFOA si = { sizeof(si) };
		PROCESS_INFORMATION pi;
		if (!CreateProcessA(copyExe.string().c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) 
		{
			MessageBoxA(NULL, "Error!", lang::GetString("launcher_copy_fail").c_str(), MB_OK);
			PostQuitMessage(0);
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}
	catch (const std::exception& e) 
	{
		MessageBoxA(NULL, "Error!", e.what(), MB_OK);
		PostQuitMessage(0);
	}
}
