#include <cstdio>
#include <cstdint>
#include <numeric>
#include <utility>
#include <switch.h>

#include "gl.hpp"
#include "imgui.hpp"
#include "fs.hpp"
#include "turnip_parser.hpp"
#include "save.hpp"

constexpr auto acnh_programid = 0x01006f8002326000ul;
constexpr auto save_main_path = "/main.dat";
constexpr auto save_hdr_path  = "/mainHeader.dat";

extern "C" void userAppInit() {
    plInitialize(PlServiceType_User);
#ifdef DEBUG
    socketInitializeDefault();
    nxlinkStdio();
#endif
}

extern "C" void userAppExit() {
    plExit();
#ifdef DEBUG
    socketExit();
#endif
}

constexpr static int width = 1280, height = 720;

int main(int argc, char **argv) {
    printf("Opening save...\n");
    FsFileSystem handle;
    auto rc = fsOpen_DeviceSaveData(&handle, acnh_programid);
    if (R_FAILED(rc))
        printf("Failed to open save: %#x\n", rc);
    auto fs = fs::Filesystem(handle);

    fs::File header, main;
    if (rc = fs.open_file(header, save_hdr_path) | fs.open_file(main, save_main_path); R_FAILED(rc))
        printf("Failed to open save files: %#x\n", rc);

    printf("Deriving keys...\n");
    auto [key, ctr] = sv::get_keys(header);
    printf("Decrypting save...\n");
    auto decrypted  = sv::decrypt(main, 0x500000, key, ctr);

    printf("Parsing save...\n");
    auto parser = tp::Parser(header, std::move(decrypted));
    auto p = parser.get_prices();

    std::array<float, 14> float_prices;
    for (std::size_t i = 0; i < p.week_prices.size(); ++i)
        float_prices[i] = static_cast<float>(p.week_prices[i]);
    auto [min, max] = std::minmax_element(p.week_prices.begin() + 2, p.week_prices.end());
    float average = static_cast<float>(std::accumulate(p.week_prices.begin() + 2, p.week_prices.end(), 0)) / (p.week_prices.size() - 2);
    printf("Starting gui...\n");
    auto *window = gl::init_glfw(width, height);
    if (!window || R_FAILED(gl::init_glad()))
        return 1;
    glViewport(0, 0, width, height);
    im::init(window, width, height);

    while (!glfwWindowShouldClose(window)) {
        // New frame
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_NX_KEY_PLUS))
            break;
        glClearColor(0.18f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        im::begin_frame(window);

        im::Begin("Turnips, version " VERSION "-" COMMIT, nullptr, ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
        im::SetWindowPos({300.0f, 120.0f}, ImGuiCond_Once);
        im::SetWindowSize({700.0f, 520.0f}, ImGuiCond_Once);

        im::Text("Buy price: %d\n", p.buy_price);

        im::BeginTable("##Prices table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV);
        im::TableNextRow();
        im::TableNextCell(); im::TextUnformatted("AM");
        im::TableNextCell(); im::TextUnformatted("PM");

        im::TableNextRow(); im::TextUnformatted("Sunday");
        im::TableNextCell(); im::Text("%d", p.sunday_am_price);    im::TableNextCell(); im::Text("%d", p.sunday_pm_price);
        im::TableNextRow(); im::TextUnformatted("Monday");
        im::TableNextCell(); im::Text("%d", p.monday_am_price);    im::TableNextCell(); im::Text("%d", p.monday_pm_price);
        im::TableNextRow(); im::TextUnformatted("Tuesday");
        im::TableNextCell(); im::Text("%d", p.tuesday_am_price);   im::TableNextCell(); im::Text("%d", p.tuesday_pm_price);
        im::TableNextRow(); im::TextUnformatted("Wednesday");
        im::TableNextCell(); im::Text("%d", p.wednesday_am_price); im::TableNextCell(); im::Text("%d", p.wednesday_pm_price);
        im::TableNextRow(); im::TextUnformatted("Thursday");
        im::TableNextCell(); im::Text("%d", p.thursday_am_price);  im::TableNextCell(); im::Text("%d", p.thursday_pm_price);
        im::TableNextRow(); im::TextUnformatted("Friday");
        im::TableNextCell(); im::Text("%d", p.friday_am_price);    im::TableNextCell(); im::Text("%d", p.friday_pm_price);
        im::TableNextRow(); im::TextUnformatted("Satursday");
        im::TableNextCell(); im::Text("%d", p.satursday_am_price); im::TableNextCell(); im::Text("%d", p.satursday_pm_price);
        im::EndTable();

        im::Separator();
        im::Text("Max: %d, Min: %d, Avg: %.1f\n", *max, *min, average);

        im::Separator();
        im::TextUnformatted("Week graph");
        im::PlotLines("##Graph", float_prices.data(), float_prices.size(), 0, "", FLT_MAX, FLT_MAX, {im::GetWindowWidth() - 30.0f, 120.0f});

        im::End();
        im::end_frame();
        glfwSwapBuffers(window);
    }

    im::exit();
    gl::exit_glfw(window);

    header.close(), main.close();
    fs.close();

    return 0;
}
