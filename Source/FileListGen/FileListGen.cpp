#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <map>
#include <algorithm>
#include <openssl/evp.h>
#include "json.hpp"
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

std::vector<Arquivo> gerarListaArquivos(const std::string& pastaRaiz) noexcept
{
    std::string launcherName = "splash.";
    constexpr int maxThreads = 8;
    std::counting_semaphore<maxThreads> sem(maxThreads);
    std::vector<std::string> caminhosRelativos;
    listarArquivosRecursivo(pastaRaiz, "", caminhosRelativos);
    std::vector<std::future<std::string>> futuros;
    std::vector<std::string> caminhosValidos;
    futuros.reserve(caminhosRelativos.size());
    caminhosValidos.reserve(caminhosRelativos.size());
    for (const auto& relPath : caminhosRelativos)
    {
        std::string caminhoCompleto = pastaRaiz + "\\" + relPath;
        std::string caminhoLower = caminhoCompleto;
        std::transform(caminhoLower.begin(), caminhoLower.end(), caminhoLower.begin(), [](unsigned char c) { return std::tolower(c); });
        if (caminhoLower.find(launcherName) != std::string::npos)
            continue;
        sem.acquire();
        caminhosValidos.push_back(relPath);
        futuros.emplace_back(std::async(std::launch::async, [caminhoCompleto, &sem]()
        {
            auto md5 = createMD5FromFile(caminhoCompleto);
            sem.release();
            return md5;
        }));
    }
    std::vector<Arquivo> arquivos;
    arquivos.reserve(caminhosValidos.size());
    int fileID = 1;
    for (size_t i = 0; i < caminhosValidos.size(); ++i)
    {
        std::string md5 = futuros[i].get();
        arquivos.emplace_back(Arquivo{
            fileID++,
            md5,
            caminhosValidos[i],
            false
        });
    }
    return arquivos;
}

std::map<std::string, Arquivo> carregarVersoesAntigas(const std::string& pastaVersion)
{
    std::map<std::string, Arquivo> arquivos;
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA((pastaVersion + "\\*.json").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return arquivos;
    do 
    {
        std::ifstream f(pastaVersion + "\\" + fd.cFileName);
        if (!f) continue;
        json j;
        f >> j;
        for (auto& entry : j) 
        {
            Arquivo arq;
            arq.FileID = entry["FileID"];
            arq.FileHash = entry["FileHash"];
            arq.FilePath = entry["FilePath"];
            arquivos[arq.FilePath] = arq;
        }
    } 
    while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
    return arquivos;
}

std::vector<Arquivo> comparar(const std::map<std::string, Arquivo>& antigos, const std::vector<Arquivo>& novos) 
{
    std::vector<Arquivo> resultado;
    for (auto& novo : novos) {
        auto it = antigos.find(novo.FilePath);
        if (it == antigos.end() || it->second.FileHash != novo.FileHash)
        {
            resultado.push_back(novo);
        }
    }
    return resultado;
}

int proximaVersao(const std::string& pastaVersion) 
{
    int maior = 0;
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA((pastaVersion + "\\*.json").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return 1;
    do {
        std::string nome = fd.cFileName;
        int v = 0;
        if (sscanf_s(nome.c_str(), "version_%d.json", &v) == 1) {
            maior = std::max(maior, v);
        }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
    return maior + 1;
}

void salvarVersao(const std::string& pastaVersion, int versao, const std::vector<Arquivo>& arquivos)
{
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
    std::ostringstream nome;
    nome << pastaVersion << "\\version_" << versao << ".json";
    std::ofstream outFile(nome.str());
    outFile << jArray.dump(4);
}

void salvarLauncherHash(const std::string& caminhoSplash) 
{
    std::ofstream outLauncher("launcher.txt");
    if (outLauncher)
        outLauncher << createMD5FromFile(caminhoSplash);
}

int main()
{
    std::cout << "Loading current version files..." << std::endl;
    auto antigos = carregarVersoesAntigas("version");
    std::cout << "---------------------------------" << std::endl;
    std::cout << "Creating file list..." << std::endl;
    auto novos = gerarListaArquivos("Update");
    std::cout << "---------------------------------" << std::endl;
    std::cout << "Checking for file MD5 changes..." << std::endl;
    auto resultado = comparar(antigos, novos);
    if (!resultado.empty())
    {
        std::cout << "---------------------------------" << std::endl;
        std::cout << "Building version JSON..." << std::endl;
        int versao = proximaVersao("version");
        std::cout << "---------------------------------" << std::endl;
        std::cout << "Saving JSON version to file..." << std::endl;
        salvarVersao("version", versao, resultado);
    }
    std::cout << "---------------------------------" << std::endl;
    std::cout << "Calculating current Splash MD5..." << std::endl;
    salvarLauncherHash("Update\\Splash.exe");
    std::cout << "---------------------------------" << std::endl;
    system("pause");
    return 0;
}