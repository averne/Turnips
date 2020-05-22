#pragma once

#include <array>
#include <string>
#include <glad/glad.h>
#include <stb_image.h>

namespace bg {

namespace impl {

struct Vertex {
    GLfloat x, y, z;
    GLfloat u, v;
};

constexpr static std::array vertices = {
   Vertex{ -1.0f, -1.0f, +0.0f, +0.0f, +0.0f },
   Vertex{ +1.0f, -1.0f, +0.0f, +1.0f, +0.0f },
   Vertex{ +1.0f, +1.0f, +0.0f, +1.0f, +1.0f },
   Vertex{ -1.0f, +1.0f, +0.0f, +0.0f, +1.0f },
};

constexpr static std::array indices = {
    0u, 1u, 2u, 2u, 3u, 0u,
};

constexpr static auto vert_sh_src = R"(
    #version 330 core

    layout (location = 0) in vec3 in_position;
    layout (location = 1) in vec2 in_uvs;

    out vec2 uvs;

    void main() {
        uvs = in_uvs;
        gl_Position = vec4(in_position, 1.0f);
    }
)";

constexpr static auto frag_sh_src = R"(
    #version 330 core

    in vec2 uvs;

    out vec4 out_color;

    uniform sampler2D tex;

    void main() {
        out_color = texture(tex, uvs);
    }
)";

GLuint shader_handle;
GLuint vbo, ebo;
GLuint tex_handle;

} // namespace impl

static bool create(const std::string &path) {
    GLint rc;
    GLuint vsh_handle, fsh_handle;
    int w, h, nchan;
    stbi_uc *data;

    vsh_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vsh_handle, 1, &impl::vert_sh_src, NULL);
    glCompileShader(vsh_handle);
    glGetShaderiv(vsh_handle, GL_COMPILE_STATUS, &rc);
    if (!rc) {
        std::string str(0x200, 0);
        glGetShaderInfoLog(vsh_handle, str.size(), nullptr, str.data());
        printf("Failed to compile vertex shader: %s\n", str.c_str());
        goto exit;
    }

    fsh_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fsh_handle, 1, &impl::frag_sh_src, NULL);
    glCompileShader(fsh_handle);
    glGetShaderiv(fsh_handle, GL_COMPILE_STATUS, &rc);
    if (!rc) {
        std::string str(0x200, 0);
        glGetShaderInfoLog(fsh_handle, str.size(), nullptr, str.data());
        printf("Failed to compile fragment shader: %s\n", str.c_str());
        goto vsh_exit;
    }

    impl::shader_handle = glCreateProgram();
    glAttachShader(impl::shader_handle, vsh_handle);
    glAttachShader(impl::shader_handle, fsh_handle);
    glLinkProgram(impl::shader_handle);
    glGetProgramiv(impl::shader_handle, GL_LINK_STATUS, &rc);
    if (!rc) {
        std::string str(0x200, 0);
        glGetProgramInfoLog(impl::shader_handle, str.size(), nullptr, str.data());
        printf("Failed to link shader: %s\n", str.c_str());
        goto fsh_exit;
    }

    glGenBuffers(1, &impl::vbo);
    glGenBuffers(1, &impl::ebo);
    glBindBuffer(GL_ARRAY_BUFFER,         impl::vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl::ebo);
    glBufferData(GL_ARRAY_BUFFER,         sizeof(impl::vertices), &impl::vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(impl::indices),  &impl::indices,  GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(impl::Vertex), reinterpret_cast<GLvoid *>(offsetof(impl::Vertex, x)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(impl::Vertex), reinterpret_cast<GLvoid *>(offsetof(impl::Vertex, u)));
    glEnableVertexAttribArray(1);

    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(path.c_str(), &w, &h, &nchan, 3);
    if (!data) {
        printf("Failed to load background image\n");
        goto fsh_exit;
    }

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &impl::tex_handle);
    glBindTexture(GL_TEXTURE_2D, impl::tex_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

fsh_exit:
    glDeleteShader(fsh_handle);
vsh_exit:
    glDeleteShader(vsh_handle);
exit:
    return rc;
}

static void destroy() {
    glDeleteProgram(impl::shader_handle);
    glDeleteTextures(1, &impl::tex_handle);
}

static inline void render() {
    glUseProgram(impl::shader_handle);
    glBindBuffer(GL_ARRAY_BUFFER, impl::vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, impl::ebo);
    glBindTexture(GL_TEXTURE_2D, impl::tex_handle);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

} // namespace bg
