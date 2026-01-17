#pragma once

#include "renderer/RenderInterfaces.h"
#include <unordered_map>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

// Helper structure for Shader Program Management
struct ShaderProgram_GL21 {
    GLuint program;
    std::unordered_map<std::string, GLint> a; // Attributes
    std::unordered_map<std::string, GLint> u; // Uniforms
    std::unordered_map<std::string, int> t;   // Texture Units

    void RegisterAttributes(const std::vector<std::string>& names);
    void RegisterUniforms(const std::vector<std::string>& names);
    void RegisterTextures(const std::vector<std::string>& names);
};

// Texture Implementation
class Texture_GL21 : public Texture {
public:
    int32_t width, height, depth;
    bool filter;
    GLuint handle;

    Texture_GL21(int32_t w, int32_t h, int32_t d, bool f, GLuint hnd) 
        : width(w), height(h), depth(d), filter(f), handle(hnd) {}
    ~Texture_GL21() { glDeleteTextures(1, &handle); }

    void SetData(const std::vector<uint8_t>& data) override;
    void SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, int32_t w, int32_t h) override {}
    void SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag, TextureSamplingParam min, TextureSamplingParam ws, TextureSamplingParam wt) override;
    void SetPixelData(const std::vector<float>& data) override;
    void CopyData(Texture* src) override {}
    bool IsValid() override { return width != 0 && height != 0 && handle != 0; }
    int32_t GetWidth() override { return width; }
    int32_t GetHeight() override { return height; }
    void SavePNG(const std::string& filename, const std::vector<uint32_t>& pal) override;

private:
    GLenum MapInternalFormat(int32_t d);
    GLint MapSamplingParam(TextureSamplingParam p);
};

// Renderer Implementation
class Renderer_GL21 : public Renderer {
public:
    std::string GetName() override { return "OpenGL 2.1"; }
    void Init() override;
    void Close() override;
    void BeginFrame(bool clearColor) override;
    void EndFrame() override;
    void Await() override { glFinish(); }

    // Shaders & Buffers
    void InitModelShader();
    Texture* newTexture(int32_t w, int32_t h, int32_t d, bool f) override;
    // ... Other factory methods (newPaletteTexture, etc.)

    // State Management
    void SetDepthTest(bool enable);
    void SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst);
    
    // Rendering
    void RenderQuad() override;
    void SetUniformI(const std::string& name, int val) override;
    void SetUniformF(const std::string& name, const std::vector<float>& values) override;

private:
    GLuint fbo, fbo_texture, rbo_depth;
    ShaderProgram_GL21* spriteShader;
    GLuint vertexBuffer;
    
    // Internal helpers
    ShaderProgram_GL21* newShaderProgram(const char* vert, const char* frag, const char* geo, const std::string& id);
    GLuint compileShader(GLenum type, const char* src);
};