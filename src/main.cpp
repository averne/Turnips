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

#include <cstdio>
#include <cstdint>
#include <numeric>
#include <utility>
#include <switch.h>
#include <math.h>
#include <imgui.h>

// #include "background.hpp"
#include "gui.hpp"
#include "fs.hpp"
#include "save.hpp"
#include "theme.hpp"
#include "parser.hpp"

namespace im {
    using namespace ImGui;
} // namespace im

constexpr static auto acnh_programid = 0x01006f8002326000ul;
constexpr static auto save_main_path = "/main.dat";
constexpr static auto save_hdr_path  = "/mainHeader.dat";

constexpr static std::array day_names = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
};

extern "C" void userAppInit() {
    setsysInitialize();
    plInitialize(PlServiceType_User);
    romfsInit();
#ifdef DEBUG
    socketInitializeDefault();
    nxlinkStdio();
#endif
}

extern "C" void userAppExit() {
    setsysExit();
    plExit();
    romfsExit();
#ifdef DEBUG
    socketExit();
#endif
}

template <typename F>
void do_with_color(std::uint32_t col, F f) {
    im::PushStyleColor(ImGuiCol_Text, col);
    f();
    im::PopStyleColor();
}

int main(int argc, char **argv) {
    tp::TurnipParser turnip_parser; tp::VisitorParser visitor_parser; tp::DateParser date_parser;
    {
        printf("Opening save...\n");
        FsFileSystem handle;
        auto rc = fsOpen_DeviceSaveData(&handle, acnh_programid);
        auto fs = fs::Filesystem(handle);
        if (R_FAILED(rc)) {
            printf("Failed to open save: %#x\n", rc);
            return 1;
        }
        fs::File header, main;
        if (rc = fs.open_file(header, save_hdr_path) | fs.open_file(main, save_main_path); R_FAILED(rc)) {
            printf("Failed to open save files: %#x\n", rc);
            return 1;
        }

        printf("Deriving keys...\n");
        auto [key, ctr] = sv::get_keys(header);
        printf("Decrypting save...\n");
        auto decrypted  = sv::decrypt(main, 0xb00000, key, ctr);

        printf("Parsing save...\n");
        auto version_parser = tp::VersionParser(header);
        turnip_parser  = tp::TurnipParser (static_cast<tp::Version>(version_parser), decrypted);
        visitor_parser = tp::VisitorParser(static_cast<tp::Version>(version_parser), decrypted);
        date_parser    = tp::DateParser   (static_cast<tp::Version>(version_parser), decrypted);
    }
    auto pattern = turnip_parser.get_pattern();
    auto p = turnip_parser.prices;

    auto names = visitor_parser.get_visitor_names();

    auto save_date = date_parser.date;
    auto save_ts   = date_parser.to_posix();

    std::array<float, 14> float_prices;
    for (std::size_t i = 0; i < p.week_prices.size(); ++i)
        float_prices[i] = static_cast<float>(p.week_prices[i]);
    auto  minmax  = std::minmax_element(p.week_prices.begin() + 2, p.week_prices.end());
    auto  min = *minmax.first, max = *minmax.second;
    float average = static_cast<float>(std::accumulate(p.week_prices.begin() + 2, p.week_prices.end(), 0)) / (p.week_prices.size() - 2);

    printf("Starting gui\n");
    if (!gui::init())
        printf("Failed to init\n");

    auto color_theme = ColorSetId_Dark;
    auto rc = setsysGetColorSetId(&color_theme);
    if (R_FAILED(rc))
        printf("Failed to get theme id\n");
    if (color_theme == ColorSetId_Light)
        th::apply_theme(th::Theme::Light);
    else
        th::apply_theme(th::Theme::Dark);

    while (gui::loop()) {
        u64 ts = 0;
        auto rc = timeGetCurrentTime(TimeType_UserSystemClock, &ts);
        if (R_FAILED(rc))
            printf("Failed to get timestamp\n");

        TimeCalendarTime           cal_time = {};
        TimeCalendarAdditionalInfo cal_info = {};
        rc = timeToCalendarTimeWithMyRule(ts, &cal_time, &cal_info);
        if (R_FAILED(rc))
            printf("Failed to convert timestamp\n");

        bool is_outdated = (floor(ts / (24 * 60 * 60)) > floor(save_ts / (24 * 60 * 60)) + cal_info.wday)
            && ((cal_info.wday != 0) || (cal_time.hour >= 5));

        // bg::render();

        auto &io = im::GetIO();

        if (io.NavInputs[ImGuiNavInput_DpadDown])
            printf("Key down\n");

        auto &[width, height] = io.DisplaySize;

        im::SetNextWindowFocus();
        im::Begin("Turnips, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        im::SetWindowPos({0.23f * width, 0.16f * height});
        im::SetWindowSize({0.55f * width, 0.73f * height});

        im::Text("Last save time: %02d-%02d-%04d %02d:%02d:%02d\n",
            save_date.day, save_date.month, save_date.year, save_date.hour, save_date.minute, save_date.second);
        if (is_outdated)
            im::SameLine(), do_with_color(th::text_min_col, [] { im::TextUnformatted("Save outdated!"); });

        im::Separator();
        im::Text("Buy price: %d, Pattern: %s\n", p.buy_price, pattern.c_str());

        im::BeginTable("##Prices table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV);
        im::TableNextRow();
        im::TableNextCell(), im::TextUnformatted("AM");
        im::TableNextCell(), im::TextUnformatted("PM");
        im::TableNextCell(), im::TextUnformatted("Visitor");

        auto get_color = [&](std::uint32_t day, bool is_am) -> std::uint32_t {
            auto price = p.week_prices[2 * day + !is_am];
            if ((cal_info.wday == day) && ((is_am && (cal_time.hour < 12)) || (!is_am && (cal_time.hour >= 12))))
                return th::text_cur_col;
            else if (price == max)
                return th::text_max_col;
            else if (price == min)
                return th::text_min_col;
            return th::text_def_col;
        };

        auto print_day = [&](std::uint32_t day) -> void {
            im::TableNextRow(); im::TextUnformatted(day_names[day]);
            do_with_color(get_color(day, true),  [&] { im::TableNextCell(), im::Text("%d", p.week_prices[2 * day]); });
            do_with_color(get_color(day, false), [&] { im::TableNextCell(), im::Text("%d", p.week_prices[2 * day + 1]); });
            im::TableNextCell(); im::TextUnformatted(names[day].c_str());
        };

        print_day(0);
        print_day(1); print_day(2); print_day(3);
        print_day(4); print_day(5); print_day(6);
        im::EndTable();

        im::Separator();
        do_with_color(th::text_max_col, [&] { im::Text("Max: %d", max); }); im::SameLine();
        do_with_color(th::text_min_col, [&] { im::Text("Min: %d", min); }); im::SameLine();
        im::Text("Avg: %.1f", average);

        im::Separator();
        im::TextUnformatted("Week graph");
        im::PlotLines("##Graph", float_prices.data() + 2, float_prices.size() - 2,
            0, "", FLT_MAX, FLT_MAX, {im::GetWindowWidth() - 30.0f, 150.0f});

        im::End();

        gui::render();
    }

    // bg::destroy();
    gui::exit();

    return 0;
}
