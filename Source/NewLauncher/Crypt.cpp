#include "Crypt.h"
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <regex>

using json = nlohmann::json;

namespace crypt
{
}

std::string crypt::createMD5FromFile(const std::string& filePath) noexcept
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