#pragma once
#include <windows.h>
#include "sciter-x-api.h"

#ifndef SCITER_WINDOW_GET_IMAGE
#define SCITER_WINDOW_GET_IMAGE 0x23
#endif

extern ISciterAPI* g_SciterApi;
typedef HWND(__stdcall* SciterCreateWindow_t)(UINT creationFlags, const RECT* frame, void* delegate, void* param, HWND parent);
typedef BOOL(__stdcall* SciterLoadFile_t)(HWND hwnd, LPCWSTR filename);
typedef LRESULT(__stdcall* SciterProcND_t)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, BOOL* handled);

extern SciterCreateWindow_t pSciterCreateWindow;
extern SciterLoadFile_t     pSciterLoadFile;
extern SciterProcND_t       pSciterProcND;

bool InitSciterDynamic();
