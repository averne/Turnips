#include <cstdio>
#include <cstdint>
#include <numeric>
#include <utility>
#include <switch.h>

#include "background.hpp"
#include "gl.hpp"
#include "imgui.hpp"
#include "fs.hpp"
#include "turnip_parser.hpp"
#include "save.hpp"

constexpr static auto acnh_programid = 0x01006f8002326000ul;
constexpr static auto save_main_path = "/main.dat";
constexpr static auto save_hdr_path  = "/mainHeader.dat";
constexpr static auto bg_path        = "romfs:/background.png";

constexpr static std::array day_names = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
};

// AGBR
constexpr static std::uint32_t min_col = 0xff6a61bf, max_col = 0xff8cbea3, cur_col = 0xffac815e;

constexpr static int width = 1280, height = 720;

extern "C" void userAppInit() {
    plInitialize(PlServiceType_User);
    romfsInit();
#ifdef DEBUG
    socketInitializeDefault();
    nxlinkStdio();
#endif
}

extern "C" void userAppExit() {
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
    auto parser = tp::Parser(header, decrypted);
    auto pattern = parser.get_pattern();
    auto p = parser.prices;

    std::array<float, 14> float_prices;
    for (std::size_t i = 0; i < p.week_prices.size(); ++i)
        float_prices[i] = static_cast<float>(p.week_prices[i]);
    auto  minmax  = std::minmax_element(p.week_prices.begin() + 2, p.week_prices.end());
    auto  min = *minmax.first, max = *minmax.second;
    float average = static_cast<float>(std::accumulate(p.week_prices.begin() + 2, p.week_prices.end(), 0)) / (p.week_prices.size() - 2);

    printf("Starting gui...\n");
    auto *window = gl::init_glfw(width, height);
    if (!window || R_FAILED(gl::init_glad()))
        return 1;
    glViewport(0, 0, width, height);
    im::init(window, width, height);

    if (!bg::create(bg_path)) {
        printf("Could not load background\n");
        return 1;
    }

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
        im::SetWindowPos({300.0f, 120.0f},  ImGuiCond_Once);
        im::SetWindowSize({700.0f, 520.0f}, ImGuiCond_Once);

        im::Text("Buy price: %d, Pattern: %s\n", p.buy_price, pattern.c_str());

        im::BeginTable("##Prices table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV);
        im::TableNextRow();
        im::TableNextCell(), im::TextUnformatted("AM");
        im::TableNextCell(), im::TextUnformatted("PM");

        auto get_color = [&](std::uint32_t day, bool is_am) -> std::uint32_t {
            std::uint32_t col = 0xff000000;
            auto price = p.week_prices[2 * day + !is_am];
            if ((cal_info.wday == day) && ((is_am && (cal_time.hour < 12)) || (!is_am && (cal_time.hour >= 12))))
                col |= cur_col;
            if (price == max)
                col |= max_col;
            if (price == min)
                col |= min_col;
            return col;
        };

        auto print_day = [&](std::uint32_t day) -> void {
            im::TableNextRow();  im::TextUnformatted(day_names[day]);
            do_with_color(get_color(day, true),  [&] { im::TableNextCell(), im::Text("%d", p.week_prices[2 * day]); });
            do_with_color(get_color(day, false), [&] { im::TableNextCell(), im::Text("%d", p.week_prices[2 * day + 1]); });
        };

        // print_day(0); // Don't print Sunday
        print_day(1); print_day(2); print_day(3);
        print_day(4); print_day(5); print_day(6);
        im::EndTable();

        im::Separator();
        do_with_color(max_col, [&] { im::Text("Max: %d", max); }); im::SameLine();
        do_with_color(min_col, [&] { im::Text("Min: %d", min); }); im::SameLine();
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
