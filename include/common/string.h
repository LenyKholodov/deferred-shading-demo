#pragma once

#include <string>
#include <vector>
#include <cstdarg>

namespace engine {
namespace common {

/// Portable wrappers on top of snprintf
int xsnprintf(char* buffer, size_t count, const char* format, ...);
int xvsnprintf(char* buffer, size_t count, const char* format, va_list list);

/// Format message
std::string format(const char* format, ...);
std::string vformat(const char* format, va_list args);

/// Tokenize string
std::vector<std::string> split(const char* str, const char* delimiters=" ", const char* spaces=" \t", const char* brackets="");

/// Filename utilities
std::string basename(const char* src);
std::string suffix (const char* src);
std::string dir(const char* src);
std::string notdir(const char* src);

}}
