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

#pragma once

#include <cstdint>
#include <switch.h>
#include <imgui.h>

#include "gui.hpp"

namespace th {

inline std::uint32_t text_def_col = 0;
inline std::uint32_t text_cur_col = 0;
inline std::uint32_t text_min_col = 0;
inline std::uint32_t text_max_col = 0;

enum class Theme {
    Light,
    Dark,
};

static inline void apply_theme(Theme theme) {
    auto *colors = ImGui::GetStyle().Colors;
    std::string background_path;

    if (theme == Theme::Light) {
        background_path = "romfs:/background_light.png";

        colors[ImGuiCol_WindowBg]      = ImVec4(1.00f, 0.98f, 0.89f, 0.90f);
        colors[ImGuiCol_PopupBg]       = ImVec4(0.95f, 0.93f, 0.84f, 0.90f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.75f, 0.68f, 0.61f, 1.00f);
        colors[ImGuiCol_FrameBg]       = ImVec4(0.94f, 0.92f, 0.84f, 1.00f);
        colors[ImGuiCol_Text]          = ImVec4(0.51f, 0.44f, 0.33f, 1.00f);
        colors[ImGuiCol_Tab]           = ImVec4(0.75f, 0.68f, 0.61f, 1.00f);
        colors[ImGuiCol_TabActive]     = ImVec4(0.80f, 0.73f, 0.66f, 1.00f);
        colors[ImGuiCol_TabHovered]    = ImVec4(0.75f, 0.68f, 0.61f, 1.00f);

        text_def_col = 0xff547082;
        text_cur_col = 0xfffd9239;
        text_min_col = 0xff7573ff;
        text_max_col = 0xff52b949;
    } else {
        background_path = "romfs:/background_dark.png";

        colors[ImGuiCol_WindowBg]      = ImVec4(0.30f, 0.32f, 0.33f, 0.90f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.16f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBg]       = ImVec4(0.27f, 0.28f, 0.29f, 1.00f);
        colors[ImGuiCol_Text]          = ImVec4(0.93f, 0.95f, 1.00f, 1.00f);
        colors[ImGuiCol_PlotLines]     = ImVec4(0.93f, 0.95f, 1.00f, 1.00f);

        text_def_col = 0xfffff1ed;
        text_cur_col = 0xfffd9239;
        text_min_col = 0xff7573ff;
        text_max_col = 0xff77d856;
    }

    if (!gui::create_background(background_path))
        printf("Failed to create background\n");
}

} // namespace th
