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
#include <utility>
#include <switch.h>
#include <math.h>
#include <imgui.h>

#include "gui.hpp"
#include "fs.hpp"
#include "save.hpp"
#include "theme.hpp"
#include "parser.hpp"

constexpr static auto acnh_programid = 0x01006f8002326000ul;
constexpr static auto save_main_path = "/main.dat";
constexpr static auto save_hdr_path  = "/mainHeader.dat";

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

int main(int argc, char **argv) {
    tp::TurnipParser turnip_parser; tp::VisitorParser visitor_parser; tp::DateParser date_parser; tp::WeatherSeedParser seed_parser;
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
        turnip_parser  = tp::TurnipParser     (static_cast<tp::Version>(version_parser), decrypted);
        visitor_parser = tp::VisitorParser    (static_cast<tp::Version>(version_parser), decrypted);
        date_parser    = tp::DateParser       (static_cast<tp::Version>(version_parser), decrypted);
        seed_parser    = tp::WeatherSeedParser(static_cast<tp::Version>(version_parser), decrypted);
    }

    auto save_date = date_parser.date;
    auto save_ts   = date_parser.to_posix();

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

        auto &[width, height] = im::GetIO().DisplaySize;

        im::SetNextWindowFocus();
        im::Begin("Turnips, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        im::SetWindowPos({0.23f * width, 0.16f * height});
        im::SetWindowSize({0.55f * width, 0.73f * height});

        im::Text("Last save time: %02d-%02d-%04d %02d:%02d:%02d\n",
            save_date.day, save_date.month, save_date.year, save_date.hour, save_date.minute, save_date.second);
        if (is_outdated)
            im::SameLine(), gui::do_with_color(th::text_min_col, [] { im::TextUnformatted("Save outdated!"); });

        im::BeginTabBar("##tab_bar");

        gui::draw_turnip_tab(turnip_parser, cal_time, cal_info);
        gui::draw_visitor_tab(visitor_parser, cal_time, cal_info);
        gui::draw_weather_tab(seed_parser);

        im::EndTabBar();

        im::End();

        gui::render();
    }

    gui::exit();

    return 0;
}
