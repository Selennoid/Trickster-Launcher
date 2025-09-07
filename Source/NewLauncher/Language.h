#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#define Dictionary std::unordered_map<std::string, std::string>

namespace lang
{
	extern Dictionary Languages;
	std::string GetString(const std::string& key) noexcept;
}