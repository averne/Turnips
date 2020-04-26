#pragma once

#include <cstdint>
#include <switch.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>

namespace im {

using namespace ImGui;

static void init(GLFWwindow *window, int width, int height) {
    ImGui::CreateContext();
    ImGui::GlfwImpl::InitForOpengGl(window);
    ImGui::Gl3Impl::Init("#version 430");
    ImGui::StyleColorsLight();

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;

    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;

    io.KeyMap[ImGuiKey_LeftArrow]   = GLFW_NX_KEY_DLEFT;
    io.KeyMap[ImGuiKey_RightArrow]  = GLFW_NX_KEY_DRIGHT;
    io.KeyMap[ImGuiKey_UpArrow]     = GLFW_NX_KEY_DUP;
    io.KeyMap[ImGuiKey_DownArrow]   = GLFW_NX_KEY_DDOWN;
    io.KeyMap[ImGuiKey_Space]       = GLFW_NX_KEY_A;
    io.KeyMap[ImGuiKey_Enter]       = GLFW_NX_KEY_X;
    io.KeyMap[ImGuiKey_Escape]      = GLFW_NX_KEY_B;
    io.KeyMap[ImGuiKey_Tab]         = GLFW_NX_KEY_R;

    // Load nintendo font
    PlFontData standard, extended;
    static ImWchar extended_range[] = {0xe000, 0xe152};
    if (R_SUCCEEDED(plGetSharedFontByType(&standard, PlSharedFontType_Standard)) &&
            R_SUCCEEDED(plGetSharedFontByType(&extended, PlSharedFontType_NintendoExt))) {
        // Delete previous font atlas
        glDeleteTextures(1, reinterpret_cast<GLuint *>(&io.Fonts->TexID));

        // Load font data & build atlas
        std::uint8_t *px;
        int w, h, bpp;
        ImFontConfig font_cfg;
        font_cfg.FontDataOwnedByAtlas = false;
        io.Fonts->AddFontFromMemoryTTF(standard.address, standard.size, 20.0f, &font_cfg, io.Fonts->GetGlyphRangesDefault());
        // Merge second font (cannot set it before)
        font_cfg.MergeMode            = true;
        io.Fonts->AddFontFromMemoryTTF(extended.address, extended.size, 20.0f, &font_cfg, extended_range);
        io.Fonts->GetTexDataAsAlpha8(&px, &w, &h, &bpp);

        // Upload atlas to VRAM, tell imgui about it
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, px);
        io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(tex));
    }

    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.ScaleAllSizes(2.0f);
}

static void exit() {
    ImGui::Gl3Impl::Shutdown();
    ImGui::GlfwImpl::Shutdown();
    ImGui::DestroyContext();
}

static void begin_frame(GLFWwindow *window) {
    // Emulate mod keys
    auto &io = ImGui::GetIO();
    if (glfwGetKey(window, GLFW_NX_KEY_L))
        io.KeyCtrl  = true;
    ImGui::Gl3Impl::NewFrame();
    ImGui::GlfwImpl::NewFrame();
    ImGui::NewFrame();
}

static void end_frame() {
    ImGui::Render();
    ImGui::Gl3Impl::RenderDrawData(ImGui::GetDrawData());
}

} // namespace imgui
