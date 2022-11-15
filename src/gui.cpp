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
#include <cstring>
#include <algorithm>
#include <numeric>
#include <switch.h>
#include <imgui.h>
#include <nvjpg.hpp>
#include "imgui_nx/imgui_deko3d.h"
#include "imgui_nx/imgui_nx.h"

#include "gui.hpp"
#include "lang.hpp"

#include "theme.hpp"

using namespace lang::literals;

namespace gui {

namespace {

constexpr std::uint32_t BG_IMAGE_ID   = 1;
constexpr std::uint32_t BG_SAMPLER_ID = 1;

constexpr auto MAX_SAMPLERS = 2;
constexpr auto MAX_IMAGES   = 2;

constexpr auto FB_NUM       = 2u;

constexpr auto CMDBUF_SIZE  = 1024 * 1024;

unsigned s_width  = 1920;
unsigned s_height = 1080;

dk::UniqueDevice       s_device;

dk::UniqueMemBlock     s_depthMemBlock;
dk::Image              s_depthBuffer;

dk::UniqueMemBlock     s_fbMemBlock;
dk::Image              s_frameBuffers[FB_NUM];

dk::UniqueMemBlock     s_cmdMemBlock[FB_NUM];
dk::UniqueCmdBuf       s_cmdBuf[FB_NUM];

dk::UniqueMemBlock     s_imageMemBlock;

dk::UniqueMemBlock     s_descriptorMemBlock;
dk::SamplerDescriptor *s_samplerDescriptors = nullptr;
dk::ImageDescriptor   *s_imageDescriptors   = nullptr;

dk::UniqueQueue        s_queue;
dk::UniqueSwapchain    s_swapchain;

void rebuildSwapchain(unsigned const width_, unsigned const height_) {
    // destroy old swapchain
    s_swapchain = nullptr;

    // create new depth buffer image layout
    dk::ImageLayout depthLayout;
    dk::ImageLayoutMaker{s_device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_Z24S8)
        .setDimensions(width_, height_)
        .initialize(depthLayout);

    auto const depthAlign = depthLayout.getAlignment();
    auto const depthSize  = depthLayout.getSize();

    // create depth buffer memblock
    if (!s_depthMemBlock) {
        s_depthMemBlock = dk::MemBlockMaker{s_device,
            imgui::deko3d::align(
                depthSize, std::max<unsigned> (depthAlign, DK_MEMBLOCK_ALIGNMENT))}
                              .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
                              .create();
    }

    s_depthBuffer.initialize(depthLayout, s_depthMemBlock, 0);

    // create framebuffer image layout
    dk::ImageLayout fbLayout;
    dk::ImageLayoutMaker{s_device}
        .setFlags(DkImageFlags_UsageRender | DkImageFlags_UsagePresent | DkImageFlags_HwCompression)
        .setFormat(DkImageFormat_RGBA8_Unorm)
        .setDimensions(width_, height_)
        .initialize(fbLayout);

    auto const fbAlign = fbLayout.getAlignment();
    auto const fbSize  = fbLayout.getSize();

    // create framebuffer memblock
    if (!s_fbMemBlock) {
        s_fbMemBlock = dk::MemBlockMaker{s_device,
            imgui::deko3d::align(
                FB_NUM * fbSize, std::max<unsigned> (fbAlign, DK_MEMBLOCK_ALIGNMENT))}
                           .setFlags(DkMemBlockFlags_GpuCached | DkMemBlockFlags_Image)
                           .create();
    }

    // initialize swapchain images
    std::array<DkImage const *, FB_NUM> swapchainImages;
    for (unsigned i = 0; i < FB_NUM; ++i) {
        swapchainImages[i] = &s_frameBuffers[i];
        s_frameBuffers[i].initialize(fbLayout, s_fbMemBlock, i * fbSize);
    }

    // create swapchain
    s_swapchain = dk::SwapchainMaker{s_device, nwindowGetDefault(), swapchainImages}.create();
}

void deko3dInit() {
    // create deko3d device
    s_device = dk::DeviceMaker{}.create();

    // initialize swapchain with maximum resolution
    rebuildSwapchain(1920, 1080);

    // create memblocks for each image slot
    for (std::size_t i = 0; i < FB_NUM; ++i) {
        // create command buffer memblock
        s_cmdMemBlock[i] =
            dk::MemBlockMaker{s_device, imgui::deko3d::align(CMDBUF_SIZE, DK_MEMBLOCK_ALIGNMENT)}
                .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
                .create();

        // create command buffer
        s_cmdBuf[i] = dk::CmdBufMaker{s_device}.create();
        s_cmdBuf[i].addMemory(s_cmdMemBlock[i], 0, s_cmdMemBlock[i].getSize());
    }

    // create image/sampler memblock
    static_assert(sizeof(dk::ImageDescriptor)   == DK_IMAGE_DESCRIPTOR_ALIGNMENT);
    static_assert(sizeof(dk::SamplerDescriptor) == DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
    static_assert(DK_IMAGE_DESCRIPTOR_ALIGNMENT == DK_SAMPLER_DESCRIPTOR_ALIGNMENT);
    s_descriptorMemBlock = dk::MemBlockMaker{s_device,
        imgui::deko3d::align(
            (MAX_SAMPLERS + MAX_IMAGES) * sizeof(dk::ImageDescriptor), DK_MEMBLOCK_ALIGNMENT)}
                               .setFlags(DkMemBlockFlags_CpuUncached | DkMemBlockFlags_GpuCached)
                               .create();

    // get cpu address for descriptors
    s_samplerDescriptors =
        static_cast<dk::SamplerDescriptor *> (s_descriptorMemBlock.getCpuAddr());
    s_imageDescriptors =
        reinterpret_cast<dk::ImageDescriptor *> (&s_samplerDescriptors[MAX_SAMPLERS]);

    // create queue
    s_queue = dk::QueueMaker{s_device}.setFlags(DkQueueFlags_Graphics).create();

    auto &cmdBuf = s_cmdBuf[0];

    // bind image/sampler descriptors
    cmdBuf.bindSamplerDescriptorSet(s_descriptorMemBlock.getGpuAddr(), MAX_SAMPLERS);
    cmdBuf.bindImageDescriptorSet(
        s_descriptorMemBlock.getGpuAddr() + MAX_SAMPLERS * sizeof(dk::SamplerDescriptor),
        MAX_IMAGES);
    s_queue.submitCommands(cmdBuf.finishList());
    s_queue.waitIdle();

    cmdBuf.clear();
}

void deko3dExit() {
    // clean up all of the deko3d objects
    s_imageMemBlock      = nullptr;
    s_descriptorMemBlock = nullptr;

    for (unsigned i = 0; i < FB_NUM; ++i) {
        s_cmdBuf[i]      = nullptr;
        s_cmdMemBlock[i] = nullptr;
    }

    s_queue         = nullptr;
    s_swapchain     = nullptr;
    s_fbMemBlock    = nullptr;
    s_depthMemBlock = nullptr;
    s_device        = nullptr;
}

} // namespace

bool init() {
    ImGui::CreateContext();
    if (!imgui::nx::init())
        return false;

    deko3dInit();
    imgui::deko3d::init(s_device,
        s_queue,
        s_cmdBuf[0],
        s_samplerDescriptors[0],
        s_imageDescriptors[0],
        dkMakeTextureHandle(0, 0),
        FB_NUM);

    return true;
}

bool loop() {
    if (!appletMainLoop())
        return false;

    auto down = imgui::nx::newFrame();
    ImGui::NewFrame();

    // Add background image
    ImGui::GetBackgroundDrawList()->AddImage(
        imgui::deko3d::makeTextureID(dkMakeTextureHandle(BG_IMAGE_ID, BG_SAMPLER_ID)),
        ImVec2(0, 0),
        ImGui::GetIO().DisplaySize);

    return !(down & HidNpadButton_Plus);
}

void render() {
    ImGui::Render();

    auto &io = ImGui::GetIO();

    if (s_width != io.DisplaySize.x || s_height != io.DisplaySize.y) {
        s_width  = io.DisplaySize.x;
        s_height = io.DisplaySize.y;
        rebuildSwapchain(s_width, s_height);
    }

    // get image from queue
    auto const slot = s_queue.acquireImage(s_swapchain);
    auto &cmdBuf    = s_cmdBuf[slot];

    cmdBuf.clear();

    // bind frame/depth buffers and clear them
    dk::ImageView colorTarget{s_frameBuffers[slot]};
    dk::ImageView depthTarget{s_depthBuffer};
    cmdBuf.bindRenderTargets(&colorTarget, &depthTarget);
    cmdBuf.setScissors(0, DkScissor{0, 0, s_width, s_height});
    cmdBuf.clearColor(0, DkColorMask_RGBA, 0.0f, 0.0f, 0.0f, 1.0f);
    cmdBuf.clearDepthStencil(true, 1.0f, 0xFF, 0);
    s_queue.submitCommands(cmdBuf.finishList());

    imgui::deko3d::render(s_device, s_queue, cmdBuf, slot);

    // wait for fragments to be completed before discarding depth/stencil buffer
    cmdBuf.barrier(DkBarrier_Fragments, 0);
    cmdBuf.discardDepthStencil();

    // present image
    s_queue.presentImage(s_swapchain, slot);
}

void exit() {
    imgui::nx::exit();

    // wait for queue to be idle
    s_queue.waitIdle();

    imgui::deko3d::exit();
    deko3dExit();
}

bool create_background(const std::string &path) {
    nj::Decoder decoder;
    if (auto rc = decoder.initialize(); rc) {
        printf("Failed to initialize decoder: %#x\n", rc);
        return false;
    }
    NJ_SCOPEGUARD([&decoder] { decoder.finalize(); });

    nj::Image background(path);
    if (!background.is_valid() || background.parse()) {
        printf("Invalid file");
        return false;
    }

    nj::Surface background_surf(background.width, background.height, nj::PixelFormat::RGBA);
    if (R_FAILED(background_surf.allocate())) {
        printf("Failed to allocate surface\n");
        return false;
    }

    if (R_FAILED(decoder.render(background, background_surf, 255)))
        printf("Failed to render image\n");
    decoder.wait(background_surf);

    // upload data to deko3d
    dk::Image image;
    std::tie(s_imageMemBlock, image) = background_surf.to_deko3d(s_device, DkImageFlags_Usage2DEngine);

    // initialize image descriptor
    s_imageDescriptors[BG_IMAGE_ID].initialize(image);

    // initialize sampler descriptor
    s_samplerDescriptors[BG_SAMPLER_ID].initialize(
        dk::Sampler{}
            .setFilter(DkFilter_Linear, DkFilter_Linear)
            .setWrapMode(DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge, DkWrapMode_ClampToEdge));

    return true;
}

void draw_turnip_tab(const tp::TurnipParser &parser, const TimeCalendarTime &cal_time, const TimeCalendarAdditionalInfo &cal_info) {
    if (!im::BeginTabItem(("turnips"_lang + "###turnips").c_str()))
        return;

    auto prices  = parser.prices;
    auto pattern = parser.get_pattern();

    auto days_json = lang::get_json()["days"];
    std::array day_names = {
        lang::get_string("sunday",    days_json),
        lang::get_string("monday",    days_json),
        lang::get_string("tuesday",   days_json),
        lang::get_string("wednesday", days_json),
        lang::get_string("thursday",  days_json),
        lang::get_string("friday",    days_json),
        lang::get_string("saturday",  days_json),
    };

    std::array<float, 14> float_prices;
    for (std::size_t i = 0; i < prices.week_prices.size(); ++i)
        float_prices[i] = static_cast<float>(prices.week_prices[i]);
    auto  minmax  = std::minmax_element(prices.week_prices.begin() + 2, prices.week_prices.end());
    auto  min = *minmax.first, max = *minmax.second;
    float average = static_cast<float>(std::accumulate(prices.week_prices.begin() + 2,
        prices.week_prices.end(), 0)) / (prices.week_prices.size() - 2);

    im::Text("price_pattern"_lang.c_str(), prices.buy_price, pattern.c_str());

    im::BeginTable("##Prices table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV);
    im::TableSetupColumn("");
    im::TableSetupColumn("am"_lang.c_str());
    im::TableSetupColumn("pm"_lang.c_str());
    ImGui::TableHeadersRow();

    auto get_color = [&](std::uint32_t day, bool is_am) -> std::uint32_t {
        auto price = prices.week_prices[2 * day + !is_am];
        if ((cal_info.wday == day) && ((is_am && (cal_time.hour < 12)) || (!is_am && (cal_time.hour >= 12))))
            return th::text_cur_col;
        else if (price == max)
            return th::text_max_col;
        else if (price == min)
            return th::text_min_col;
        return th::text_def_col;
    };

    auto print_day = [&](std::uint32_t day) -> void {
        im::TableNextRow(), im::TableNextColumn(), im::TextUnformatted(day_names[day].c_str());
        do_with_color(get_color(day, true),  [&] { im::TableNextColumn(), im::Text("%d", prices.week_prices[2 * day]); });
        do_with_color(get_color(day, false), [&] { im::TableNextColumn(), im::Text("%d", prices.week_prices[2 * day + 1]); });
    };

    print_day(0);
    print_day(1); print_day(2); print_day(3);
    print_day(4); print_day(5); print_day(6);
    im::EndTable();

    im::Separator();
    do_with_color(th::text_max_col, [&] { im::Text("turnips_max"_lang.c_str(), max); }); im::SameLine();
    do_with_color(th::text_min_col, [&] { im::Text("turnips_min"_lang.c_str(), min); }); im::SameLine();
    im::Text("turnips_average"_lang.c_str(), average);

    im::Separator();
    im::TextUnformatted("week_graph"_lang.c_str());
    im::PlotLines("##Graph", float_prices.data() + 2, float_prices.size() - 2,
        0, "", FLT_MAX, FLT_MAX, {im::GetWindowWidth() - 30.0f, 125.0f});

    im::EndTabItem();
}

void draw_visitor_tab(const tp::VisitorParser &parser, const TimeCalendarTime &cal_time, const TimeCalendarAdditionalInfo &cal_info) {
    if (!im::BeginTabItem(("visitors"_lang + "###visitors").c_str()))
        return;

    // Visitors leave at 5am, so adjust the weekday
    auto wday = (cal_time.hour >= 5) ? cal_info.wday : std::clamp(cal_info.wday - 1, 0u, 7u);

    auto days_json = lang::get_json()["days"];
    std::array day_names = {
        lang::get_string("sunday",    days_json),
        lang::get_string("monday",    days_json),
        lang::get_string("tuesday",   days_json),
        lang::get_string("wednesday", days_json),
        lang::get_string("thursday",  days_json),
        lang::get_string("friday",    days_json),
        lang::get_string("saturday",  days_json),
    };

    auto names = parser.get_visitor_names();

    im::Dummy(ImVec2(0.0f, 10.0f));
    im::BeginTable("##Visitors table", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV);

    auto get_color = [&](std::uint32_t day) -> std::uint32_t {
        return (wday == day) ? th::text_cur_col : th::text_def_col;
    };

    auto print_day = [&](std::uint32_t day) -> void {
        im::TableNextColumn(); im::TextUnformatted(day_names[day].c_str());
        do_with_color(get_color(day), [&] {
            im::TableNextColumn(), im::Text("%s", names[day].c_str());
            if (day == parser.get_celeste_day())
                im::SameLine(), im::TextUnformatted(lang::get_string("celeste", lang::get_json()["npcs"]).c_str());
            if (day == parser.get_wisp_day())
                im::SameLine(), im::TextUnformatted(lang::get_string("wisp",    lang::get_json()["npcs"]).c_str());
        });
    };

    print_day(1); print_day(2); print_day(3);
    print_day(4); print_day(5); print_day(6);
    print_day(0);
    im::EndTable();

    im::EndTabItem();
}

void draw_weather_tab(const tp::WeatherSeedParser &parser) {
    if (!im::BeginTabItem(("weather"_lang + "###weather").c_str()))
        return;

    auto seed = parser.calculate_weather_seed();

    im::Dummy(ImVec2(0.0f, 10.0f));
    im::Text("hemisphere"_lang.c_str(), parser.get_hemisphere_name().c_str());
    im::Text("weather_seed"_lang.c_str(), seed, seed);

    im::Separator();
    im::TextUnformatted("weather_url_tip"_lang.c_str());

    im::EndTabItem();
}

void draw_language_tab() {
    if (!im::BeginTabItem(("language"_lang + "###lang").c_str()))
        return;

    auto cur_lang = lang::get_current_language(), prev_lang = cur_lang;
    im::RadioButton("English",    reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::English));
    im::RadioButton("中文",        reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Chinese));
    im::RadioButton("Français",   reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::French));
    im::RadioButton("Nederlands", reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Dutch));
    im::RadioButton("Italiano",   reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Italian));
    im::RadioButton("Deutsch",    reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::German));
    im::RadioButton("Español",    reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Spanish));
    im::RadioButton("Português",  reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Portuguese));
    im::RadioButton("한국어" ,      reinterpret_cast<int *>(&cur_lang), static_cast<int>(lang::Language::Korean));

    if (cur_lang != prev_lang)
        lang::set_language(cur_lang);

    im::Separator();
    im::TextUnformatted("If you'd like to see your language here, make an issue on\nhttps://github.com/averne/Turnips");

    im::EndTabItem();
}

} // namespace gui
