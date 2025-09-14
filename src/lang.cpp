// Copyright (C) 2020 averne
//
// This file is part of Turnips.
//
// Turnips is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Turnips is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Turnips.  If not, see <http://www.gnu.org/licenses/>.

#include <switch.h>
#include <json.hpp>

#include "fs.hpp"
#include "lang.hpp"

using json = nlohmann::json;

namespace lang {

namespace {

static json lang_json = nullptr;
static Language current_language = Language::Default;

} // namespace

const json &get_json() {
    return lang_json;
}

Language get_current_language() {
    return current_language;
}

Result set_language(Language lang) {
    const char *path;
    current_language = lang;
    switch (lang) {
        case Language::ChineseSimplified:
            path = "romfs:/lang/zh-cn.json";
            break;
        case Language::ChineseTraditional:
            path = "romfs:/lang/zh-tw.json";
            break;
        case Language::Japanese:
            path = "romfs:/lang/ja.json";
            break;
        case Language::JapaneseRyukyuan:
            path = "romfs:/lang/ja-ryu.json";
            break;
        case Language::French:
            path = "romfs:/lang/fr.json";
            break;
        case Language::Dutch:
            path = "romfs:/lang/nl.json";
            break;
        case Language::Italian:
            path = "romfs:/lang/it.json";
            break;
        case Language::German:
            path = "romfs:/lang/de.json";
            break;
        case Language::Spanish:
            path = "romfs:/lang/es.json";
            break;
        case Language::Korean:
            path = "romfs:/lang/ko.json";
            break;
        case Language::Portuguese:
            path = "romfs:/lang/pt-br.json";
            break;
        case Language::Latin:
            path = "romfs:/lang/la.json";
            break;
        case Language::Polish:
            path = "romfs:/lang/pl.json";
            break;
        case Language::English:
        case Language::Default:
        default:
            path = "romfs:/lang/en.json";
            break;
    }

    auto *fp = fopen(path, "r");
    if (!fp)
        return 1;

    fseek(fp, 0, SEEK_END);
    std::size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    std::string contents(size, 0);

    if (auto read = fread(contents.data(), 1, size, fp); read != size)
        return read;
    fclose(fp);

    lang_json = json::parse(contents);

    return 0;
}

Result initialize_to_system_language() {
    if (auto rc = setInitialize(); R_FAILED(rc)) {
        setExit();
        return rc;
    }

    u64 l;
    if (auto rc = setGetSystemLanguage(&l); R_FAILED(rc)) {
        setExit();
        return rc;
    }

    SetLanguage sl;
    if (auto rc = setMakeLanguage(l, &sl); R_FAILED(rc)) {
        setExit();
        return rc;
    }

    switch (sl) {
        case SetLanguage_ENGB:
        case SetLanguage_ENUS:
            return set_language(Language::English);
        case SetLanguage_ZHCN:
        case SetLanguage_ZHHANS:
            return set_language(Language::ChineseSimplified);
        case SetLanguage_ZHTW:
        case SetLanguage_ZHHANT:
            return set_language(Language::ChineseTraditional);
        case SetLanguage_JA:
            return set_language(Language::Japanese);
        case SetLanguage_FR:
            return set_language(Language::French);
        case SetLanguage_NL:
            return set_language(Language::Dutch);
        case SetLanguage_IT:
            return set_language(Language::Italian);
        case SetLanguage_DE:
            return set_language(Language::German);
        case SetLanguage_ES:
            return set_language(Language::Spanish);
        case SetLanguage_KO:
            return set_language(Language::Korean);
        case SetLanguage_PT:
        case SetLanguage_PTBR:
            return set_language(Language::Portuguese);
        default:
            return set_language(Language::Default);
    }
}

std::string get_string(std::string key, const json &json) {
    return json.value(key, key);
}

} // namespace lang
