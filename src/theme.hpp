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
    auto *colors = im::GetStyle().Colors;
    if (theme == Theme::Light) {
        bg::create("romfs:/background_light.png");

        colors[ImGuiCol_WindowBg]      = ImVec4(1.00f, 0.98f, 0.89f, 0.75f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.94f, 0.62f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBg]       = ImVec4(0.94f, 0.92f, 0.84f, 1.00f);
        colors[ImGuiCol_Text]          = ImVec4(0.51f, 0.44f, 0.33f, 1.00f);

        text_def_col = 0xff547082;
        text_cur_col = 0xfffd9239;
        text_min_col = 0xff7573ff;
        text_max_col = 0xff52b949;
    } else {
        bg::create("romfs:/background_dark.png");

        colors[ImGuiCol_WindowBg]      = ImVec4(0.30f, 0.32f, 0.33f, 0.75f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBg]       = ImVec4(0.27f, 0.28f, 0.29f, 1.00f);
        colors[ImGuiCol_Text]          = ImVec4(0.93f, 0.95f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotLines]     = ImVec4(0.93f, 0.95f, 1.00f, 1.00f);

        text_def_col = 0xfffff1ed;
        text_cur_col = 0xfffd9239;
        text_min_col = 0xff7573ff;
        text_max_col = 0xff77d856;
    }
}

} // namespace th
