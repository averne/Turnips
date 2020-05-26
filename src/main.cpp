#include <cstdio>
#include <cstdint>
#include <numeric>
#include <utility>
#include <switch.h>

#include "background.hpp"
#include "gl.hpp"
#include "imgui.hpp"
#include "fs.hpp"
#include "save.hpp"
#include "theme.hpp"
#include "parser.hpp"

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
    tp::TurnipParser turnip_parser; tp::VisitorParser visitor_parser;
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
        auto decrypted  = sv::decrypt(main, 0x500000, key, ctr);

        printf("Parsing save...\n");
        auto version_parser = tp::VersionParser(header);
        turnip_parser  = tp::TurnipParser(static_cast<tp::Version>(version_parser), decrypted);
        visitor_parser = tp::VisitorParser(static_cast<tp::Version>(version_parser), decrypted);
    }
    auto pattern = turnip_parser.get_pattern();
    auto p = turnip_parser.prices;

    auto names = visitor_parser.get_visitor_names();

    std::array<float, 14> float_prices;
    for (std::size_t i = 0; i < p.week_prices.size(); ++i)
        float_prices[i] = static_cast<float>(p.week_prices[i]);
    auto  minmax  = std::minmax_element(p.week_prices.begin() + 2, p.week_prices.end());
    auto  min = *minmax.first, max = *minmax.second;
    float average = static_cast<float>(std::accumulate(p.week_prices.begin() + 2, p.week_prices.end(), 0)) / (p.week_prices.size() - 2);

    printf("Starting gui...\n");
    auto *window = gl::init_glfw(1920, 1080);
    if (!window || R_FAILED(gl::init_glad()))
        return 1;

    auto get_dims = []() -> std::tuple<int, int, float> {
        if (appletGetOperationMode() == AppletOperationMode_Handheld)
            return { 1280, 720, 2.0f };
        return { 1920, 1080, 2.8f };
    };

    auto [width, height, scale] = get_dims();

    glfwSetWindowSize(window, width, height);
    glViewport(0, 0, width, height);
    im::init(window, width, height, scale);

    auto color_theme = ColorSetId_Dark;
    auto rc = setsysGetColorSetId(&color_theme);
    if (R_FAILED(rc))
        printf("Failed to get theme id\n");
    if (color_theme == ColorSetId_Light)
        th::apply_theme(th::Theme::Light);
    else
        th::apply_theme(th::Theme::Dark);

    while (!glfwWindowShouldClose(window)) {
        u64 ts = 0;
        rc = timeGetCurrentTime(TimeType_UserSystemClock, &ts);
        if (R_FAILED(rc))
            printf("Failed to get timestamp\n");

        TimeCalendarTime           cal_time = {};
        TimeCalendarAdditionalInfo cal_info = {};
        rc = timeToCalendarTimeWithMyRule(ts, &cal_time, &cal_info);
        if (R_FAILED(rc))
            printf("Failed to convert timestamp\n");

        // New frame
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_NX_KEY_PLUS))
            break;
        glClearColor(0.18f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        bg::render();

        im::begin_frame(window);

        im::SetNextWindowFocus();
        im::Begin("Turnips, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        im::SetWindowPos({0.23f * width, 0.15f * height},  ImGuiCond_Once);
        im::SetWindowSize({0.55f * width, 0.75f * height}, ImGuiCond_Once);

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
        im::end_frame();

        glfwSwapBuffers(window);
    }

    bg::destroy();
    im::exit();
    gl::exit_glfw(window);

    return 0;
}
