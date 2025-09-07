#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include <d3d9.h>
#include <d3dx9.h>
#include <fstream>
#include <vector>
#include <string>
#include <json.hpp>
#include <unordered_map>

struct Noticia 
{
    std::string id, imagemUrl, data, titulo, autor;
    LPDIRECT3DTEXTURE9 imagem = nullptr;
};

namespace crypt
{
    std::string createMD5FromFile(const std::string& filePath) noexcept;
}