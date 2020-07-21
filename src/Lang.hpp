#ifndef LANG_HPP
#define LANG_HPP

#include "Types.hpp"
#include "json.hpp"
namespace Lang
{
    bool setFile(std::string);
    bool setLanguage(Language);
    std::string string(std::string);
    const char *stringtochar(std::string);
    Language getSystemLanguage();
    Language get_current_language();
}; // namespace Lang

inline std::string operator""_lang(const char *key, size_t size)
{
    return Lang::string(std::string(key, size));
}
#endif