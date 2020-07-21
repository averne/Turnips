#include <filesystem>
#include <fstream>
#include "json.hpp"
#include "Lang.hpp"
#include <sstream>
namespace Lang
{

    nlohmann::json j = nullptr;
    Language current_lanuage = Default;
    Language get_current_language()
    {
        return current_lanuage;
    }
    bool setFile(std::string path)
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }
        std::ifstream in(path);
        j = nlohmann::json::parse(in);
        return true;
    }
    bool setLanguage(Language l)
    {
        std::string path = "";
        switch (l)
        {
        case English:
            path = "romfs:/lang/en.json";
            current_lanuage = English;
            break;
        case Chinese:
            path = "romfs:/lang/ch.json";
            current_lanuage = Chinese;
            break;
        default:
            current_lanuage = Default;
            break;
        }
        return setFile(path);
    }
    std::string string(std::string key)
    {
        // First 'navigate' to nested object
        nlohmann::json t = j;
        std::istringstream ss(key);
        std::string k;
        while (std::getline(ss, k, '.') && t != nullptr)
        {
            t = t[k];
        }

        // If the string is not present return key
        if (t == nullptr || !t.is_string())
        {
            return std::string("fail");
        }

        return t.get<std::string>();
    }
    const char *stringtochar(std::string str)
    {
        return str.c_str();
    }
    Language getSystemLanguage()
    {
        SetLanguage sl;
        u64 l;
        setInitialize();
        setGetSystemLanguage(&l);
        setMakeLanguage(l, &sl);
        setExit();

        Language lang;
        switch (sl)
        {
        case SetLanguage_ENGB:
        case SetLanguage_ENUS:
            lang = English;
            break;
        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS:
            lang = Chinese;
            break;
        default:
            lang = Default;
            break;
        }
        return lang;
    }
} // namespace Lang