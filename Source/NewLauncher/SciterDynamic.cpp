#include "SciterDynamic.h"
#include <stdio.h>

SciterCreateWindow_t pSciterCreateWindow = nullptr;
SciterLoadFile_t     pSciterLoadFile = nullptr;
SciterProcND_t       pSciterProcND = nullptr;

ISciterAPI* g_SciterApi = nullptr;

bool InitSciterDynamic() {
    HMODULE hSciter = LoadLibraryA("sciter.dll");
    if (!hSciter) {
        DWORD err = GetLastError();
        char buf[256];
        sprintf_s(buf, "Falha ao carregar sciter.dll - erro %lu", err);
        MessageBoxA(NULL, buf, "Erro", MB_ICONERROR);
        return false;
    }

    // pega a função SciterAPI
    auto pSciterAPI = (SciterAPI_ptr)GetProcAddress(hSciter, "SciterAPI");
    if (!pSciterAPI) {
        MessageBoxA(NULL, "Falha ao resolver SciterAPI", "Erro", MB_ICONERROR);
        return false;
    }

    // pega a tabela de funções
    g_SciterApi = pSciterAPI();
    if (!g_SciterApi) {
        MessageBoxA(NULL, "SciterAPI retornou NULL", "Erro", MB_ICONERROR);
        return false;
    }

    return true;
}