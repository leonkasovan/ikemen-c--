#include "Renderer_GL21.h"
#include <iostream>

// --- ShaderProgram_GL21 Methods ---

void ShaderProgram_GL21::RegisterAttributes(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        a[name] = glGetAttribLocation(program, name.c_str());
    }
}

void ShaderProgram_GL21::RegisterUniforms(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        u[name] = glGetUniformLocation(program, name.c_str());
    }
}

// --- Texture_GL21 Methods ---

void Texture_GL21::SetData(const std::vector<uint8_t>& data) {
    GLint interp = filter ? GL_LINEAR : GL_NEAREST;
    GLenum format = MapInternalFormat(std::max(depth, 8));

    glBindTexture(GL_TEXTURE_2D, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.empty() ? nullptr : data.data());
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// --- Renderer_GL21 Methods ---

void Renderer_GL21::Init() {
    if (!gladLoadGL()) { /* Handle Error */ }
    
    // Vertex data for quads (Matches postVertData in Go)
    float quadData[] = { -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadData), quadData, GL_STATIC_DRAW);

    // Initialize Shaders
    spriteShader = newShaderProgram(vertSrc, fragSrc, nullptr, "Main Shader");
    spriteShader->RegisterAttributes({"position", "uv"});
    spriteShader->RegisterUniforms({"model", "view", "projection"});
}

void Renderer_GL21::RenderQuad() {
    glUseProgram(spriteShader->program);
    
    GLint posLoc = spriteShader->a["position"];
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(posLoc);
}

void Renderer_GL21::SetUniformF(const std::string& name, const std::vector<float>& values) {
    GLint loc = spriteShader->u[name];
    switch (values.size()) {
        case 1: glUniform1f(loc, values[0]); break;
        case 2: glUniform2f(loc, values[0], values[1]); break;
        case 3: glUniform3f(loc, values[0], values[1], values[2]); break;
        case 4: glUniform4f(loc, values[0], values[1], values[2], values[3]); break;
    }
}