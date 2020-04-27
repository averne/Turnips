#pragma once

#include <cstdint>
#include <switch.h>
#include <imgui.h>

#include "background.hpp"

namespace th {

static inline std::uint32_t text_def_col = 0;
static inline std::uint32_t text_cur_col = 0;
static inline std::uint32_t text_min_col = 0;
static inline std::uint32_t text_max_col = 0;

enum class Theme {
    Light,
    Dark,
};

static inline void apply_theme(Theme theme) {
    if (theme == Theme::Light) {
        bg::create("romfs:/background_light.png");

        auto *colors = im::GetStyle().Colors;
        colors[ImGuiCol_WindowBg]      = ImVec4(1.00f, 0.98f, 0.89f, 1.00f);
        colors[ImGuiCol_Text]          = ImVec4(0.51f, 0.44f, 0.33f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.94f, 0.62f, 0.24f, 1.00f);

        text_def_col = 0xff547082;
        text_cur_col = 0xffac815e;
        text_min_col = 0xff6a61bf;
        text_max_col = 0xff8cbea3;
    } else {
        bg::create("romfs:/background_dark.png");

        auto *colors = im::GetStyle().Colors;
        colors[ImGuiCol_WindowBg]      = ImVec4(1.00f, 0.98f, 0.89f, 1.00f);
        colors[ImGuiCol_Text]          = ImVec4(0.51f, 0.44f, 0.33f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.94f, 0.62f, 0.24f, 1.00f);

        text_def_col = 0xff547082;
        text_cur_col = 0xffac815e;
        text_min_col = 0xff6a61bf;
        text_max_col = 0xff8cbea3;
    }
}

} // namespace th
