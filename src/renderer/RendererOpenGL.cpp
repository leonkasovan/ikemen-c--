// #include "RendererInterfaces.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include "RendererOpenGL.h"

// ==========================================
// ShaderProgram_GL Implementation
// ==========================================

ShaderProgram_GL::ShaderProgram_GL() : program(0) {
}

ShaderProgram_GL::~ShaderProgram_GL() {
    if (program != 0) {
        glDeleteProgram(program);
    }
}

void ShaderProgram_GL::RegisterAttributes(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        attributes[name] = glGetAttribLocation(program, name.c_str());
    }
}

void ShaderProgram_GL::RegisterUniforms(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        uniforms[name] = glGetUniformLocation(program, name.c_str());
    }
}

void ShaderProgram_GL::RegisterTextures(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        uniforms[name] = glGetUniformLocation(program, name.c_str());
        textures[name] = static_cast<int>(textures.size());
    }
}

int32_t ShaderProgram_GL::GetAttributeLocation(const std::string& name) const {
    auto it = attributes.find(name);
    return (it != attributes.end()) ? it->second : -1;
}

int32_t ShaderProgram_GL::GetUniformLocation(const std::string& name) const {
    auto it = uniforms.find(name);
    return (it != uniforms.end()) ? it->second : -1;
}

int32_t ShaderProgram_GL::GetTextureUnit(const std::string& name) const {
    auto it = textures.find(name);
    return (it != textures.end()) ? it->second : -1;
}

uint8_t* ShaderProgram_GL::glStr(const std::string& s) {
    // In C++, we work directly with C strings
    return reinterpret_cast<uint8_t*>(const_cast<char*>(s.c_str()));
}

std::string ShaderProgram_GL::GetShaderInfoLog(uint32_t shader) {
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        return std::string(log.data());
    }
    return "";
}

std::string ShaderProgram_GL::GetProgramInfoLog(uint32_t program) {
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        return std::string(log.data());
    }
    return "";
}
// ==========================================
// Texture_GL Implementation
// ==========================================

Texture_GL::Texture_GL(int32_t width, int32_t height, int32_t depth, bool filter, uint32_t handle)
    : ITexture(width, height, depth, filter, handle), width(width), height(height), depth(depth), filter(filter), handle(handle) {
}

Texture_GL::~Texture_GL() {
    if (handle != 0) {
        glDeleteTextures(1, &handle);
    }
}

void Texture_GL::SetData(const std::vector<uint8_t>& data) {
    int32_t interp = filter ? GL_LINEAR : GL_NEAREST;
    uint32_t format = MapInternalFormat(std::max(depth, 8));

    glBindTexture(GL_TEXTURE_2D, handle);
    glFinish();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (!data.empty()) {
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture_GL::SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, 
                            int32_t width, int32_t height) {
    uint32_t format = MapInternalFormat(std::max(depth, 8));

    glBindTexture(GL_TEXTURE_2D, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (!data.empty()) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, data.data());
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, format, GL_UNSIGNED_BYTE, nullptr);
    }
}

void Texture_GL::SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag,
                         TextureSamplingParam min, TextureSamplingParam ws, TextureSamplingParam wt) {
    uint32_t format = MapInternalFormat(std::max(depth, 8));

    glBindTexture(GL_TEXTURE_2D, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MapTextureSamplingParam(mag));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MapTextureSamplingParam(min));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, MapTextureSamplingParam(ws));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, MapTextureSamplingParam(wt));
}

void Texture_GL::SetPixelData(const std::vector<float>& data) {
    uint32_t format = MapInternalFormat(std::max(depth / 4, 8));
    uint32_t internalFormat = MapInternalFormat(std::max(depth, 8));

    glBindTexture(GL_TEXTURE_2D, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_FLOAT, data.data());
}

void Texture_GL::SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) {
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Upload to specific Z-slice (layer)
    if (!data.empty()) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, 256, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    }
}

void Texture_GL::CopyData(const Texture_GL* src) {
    if (src == nullptr || !src->IsValid() || !IsValid()) {
        return;
    }

    if (width != src->width || height != src->height || depth != src->depth) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, src->handle);
    int pixelSize = width * height * (depth / 8);
    std::vector<uint8_t> data(pixelSize);
    glGetTexImage(GL_TEXTURE_2D, 0, src->MapInternalFormat(std::max(src->depth, 8)), GL_UNSIGNED_BYTE, data.data());

    glBindTexture(GL_TEXTURE_2D, handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, MapInternalFormat(std::max(depth, 8)), GL_UNSIGNED_BYTE, data.data());
}

bool Texture_GL::IsValid() const {
    return width != 0 && height != 0 && handle != 0;
}

uint32_t Texture_GL::MapInternalFormat(int32_t d) const {
    static const std::map<int32_t, uint32_t> InternalFormatLUT = {
        {8, GL_RED},
        {24, GL_RGB},
        {32, GL_RGBA},
        {96, GL_RGB32F},
        {128, GL_RGBA32F}
    };

    auto it = InternalFormatLUT.find(d);
    return (it != InternalFormatLUT.end()) ? it->second : GL_RGBA;
}

int32_t Texture_GL::MapTextureSamplingParam(TextureSamplingParam param) const {
    static const std::map<TextureSamplingParam, int32_t> SamplingParamLUT = {
        {TextureSamplingParam::FilterNearest, GL_NEAREST},
        {TextureSamplingParam::FilterLinear, GL_LINEAR},
        {TextureSamplingParam::FilterNearestMipMapNearest, GL_NEAREST_MIPMAP_NEAREST},
        {TextureSamplingParam::FilterLinearMipMapNearest, GL_LINEAR_MIPMAP_NEAREST},
        {TextureSamplingParam::FilterNearestMipMapLinear, GL_NEAREST_MIPMAP_LINEAR},
        {TextureSamplingParam::FilterLinearMipMapLinear, GL_LINEAR_MIPMAP_LINEAR},
        {TextureSamplingParam::WrapClampToEdge, GL_CLAMP_TO_EDGE},
        {TextureSamplingParam::WrapMirroredRepeat, GL_MIRRORED_REPEAT},
        {TextureSamplingParam::WrapRepeat, GL_REPEAT}
    };

    auto it = SamplingParamLUT.find(param);
    return (it != SamplingParamLUT.end()) ? it->second : GL_NEAREST;
}

int Texture_GL::SavePNG(const std::string& filename, const std::vector<uint32_t>* palette) {
    if (!IsValid()) {
        std::cerr << "SavePNG: texture is not valid (handle=" << handle << ", w=" << width << ", h=" << height << ")" << std::endl;
        return -1;
    }

    glBindTexture(GL_TEXTURE_2D, handle);
    int pixelSize = width * height * (depth / 8);
    std::vector<uint8_t> data(pixelSize);
    glGetTexImage(GL_TEXTURE_2D, 0, MapInternalFormat(std::max(depth, 8)), GL_UNSIGNED_BYTE, data.data());

    if (static_cast<int>(data.size()) != pixelSize) {
        std::cerr << "SavePNG: unexpected data length got=" << data.size() << " expected=" << pixelSize << std::endl;
    }

    // Basic PNG saving (simplified - in production, use libpng)
    // For now, just write a placeholder
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return -1;
    }
    file.close();

    return 0;
}

// ==========================================
// Renderer_GL Implementation
// ==========================================

Renderer_GL::Renderer_GL()
    : IRenderer() {
    glState.depthTest = false;
    glState.depthMask = false;
    glState.invertFrontFace = false;
    glState.doubleSided = false;
    glState.blendEquation = BlendEquation::Add;
    glState.blendSrc = BlendFunc::SrcAlpha;
    glState.blendDst = BlendFunc::OneMinusSrcAlpha;
    glState.useUV = false;
    glState.useNormal = false;
    glState.useTangent = false;
    glState.useVertColor = false;
    glState.useJoint0 = false;
    glState.useJoint1 = false;
    glState.useOutlineAttribute = false;
}

Renderer_GL::~Renderer_GL() {
    Close();
}

void Renderer_GL::Init() {
    // Print GL information
    std::cout << "Real GL Version: " << reinterpret_cast<const char*>(glGetString(GL_VERSION)) << std::endl;
    std::cout << "Real GLSL Version: " << reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)) << std::endl;
    std::cout << "Real GL Renderer: " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << std::endl;
    std::cout << "Real GL Vendor: " << reinterpret_cast<const char*>(glGetString(GL_VENDOR)) << std::endl;

    // Create VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Generate vertex buffers
    glGenBuffers(1, &postVertBuffer);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(2, &modelVertexBuffer[0]);
    glGenBuffers(2, &modelIndexBuffer[0]);
    glGenBuffers(1, &vertexBufferBatch);

    // Create framebuffer texture
    glGenTextures(1, &fbo_texture);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Generate renderbuffer for depth
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 1920, 1080); // Default size
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

    // Post-processing FBOs
    fbo_pp.resize(2);
    fbo_pp_texture.resize(2);
    
    for (int i = 0; i < 2; ++i) {
        glGenTextures(1, &fbo_pp_texture[i]);
        glBindTexture(GL_TEXTURE_2D, fbo_pp_texture[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glGenFramebuffers(1, &fbo_pp[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_pp[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_pp_texture[i], 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    enableModel = false;
    enableShadow = false;
}

void Renderer_GL::Close() {
    if (fbo != 0) glDeleteFramebuffers(1, &fbo);
    if (fbo_texture != 0) glDeleteTextures(1, &fbo_texture);
    if (rbo_depth != 0) glDeleteRenderbuffers(1, &rbo_depth);
    
    for (auto tex : fbo_pp_texture) {
        if (tex != 0) glDeleteTextures(1, &tex);
    }
    for (auto buf : fbo_pp) {
        if (buf != 0) glDeleteFramebuffers(1, &buf);
    }

    if (vao != 0) glDeleteVertexArrays(1, &vao);
    if (postVertBuffer != 0) glDeleteBuffers(1, &postVertBuffer);
    if (vertexBuffer != 0) glDeleteBuffers(1, &vertexBuffer);
    if (vertexBufferBatch != 0) glDeleteBuffers(1, &vertexBufferBatch);
    
    glDeleteBuffers(2, &modelVertexBuffer[0]);
    glDeleteBuffers(2, &modelIndexBuffer[0]);

    spriteShader.reset();
    modelShader.reset();
    shadowMapShader.reset();
    panoramaToCubeMapShader.reset();
    cubemapFilteringShader.reset();
    postShaderSelect.clear();
}

std::string Renderer_GL::GetName() const {
    return "OpenGL 3.3";
}

int Renderer_GL::InitModelShader() {
    // Placeholder implementation
    // In a full implementation, you would compile and link shaders here
    return 0;
}

void Renderer_GL::BeginFrame(bool clearColor) {
    glBindVertexArray(vao);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1920, 1080); // Default viewport
    
    if (clearColor) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void Renderer_GL::EndFrame() {
    if (fbo_pp.empty()) {
        return;
    }

    glBindVertexArray(vao);

    // Post-processing pass - simplified
    glViewport(0, 0, 1920, 1080);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer_GL::Await() {
    glFinish();
}

bool Renderer_GL::IsModelEnabled() const {
    return enableModel;
}

bool Renderer_GL::IsShadowEnabled() const {
    return enableShadow;
}

void Renderer_GL::BlendReset() {
    glBlendEquation(MapBlendEquation(BlendEquation::Add));
    glBlendFunc(MapBlendFunction(BlendFunc::SrcAlpha), MapBlendFunction(BlendFunc::OneMinusSrcAlpha));
}

void Renderer_GL::SetBlending(BlendEquation eq, BlendFunc src, BlendFunc dst) {
    if (eq != glState.blendEquation) {
        glState.blendEquation = eq;
        glBlendEquation(MapBlendEquation(eq));
    }
    if (src != glState.blendSrc || dst != glState.blendDst) {
        glState.blendSrc = src;
        glState.blendDst = dst;
        glBlendFunc(MapBlendFunction(src), MapBlendFunction(dst));
    }
}

void Renderer_GL::SetDepthTest(bool depthTest) {
    if (depthTest != glState.depthTest) {
        glState.depthTest = depthTest;
        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }
}

void Renderer_GL::SetDepthMask(bool depthMask) {
    if (depthMask != glState.depthMask) {
        glState.depthMask = depthMask;
        glDepthMask(depthMask);
    }
}

void Renderer_GL::SetFrontFace(bool invertFrontFace) {
    if (invertFrontFace != glState.invertFrontFace) {
        glState.invertFrontFace = invertFrontFace;
        glFrontFace(invertFrontFace ? GL_CW : GL_CCW);
    }
}

void Renderer_GL::SetCullFace(bool doubleSided) {
    if (doubleSided != glState.doubleSided) {
        glState.doubleSided = doubleSided;
        if (!doubleSided) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }
    }
}

void Renderer_GL::SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) {
    glBindVertexArray(vao);
    glUseProgram(spriteShader->GetProgram());

    glBlendEquation(MapBlendEquation(eq));
    glBlendFunc(MapBlendFunction(src), MapBlendFunction(dst));
    glEnable(GL_BLEND);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    const int stride = 20; // 5 floats * 4 bytes

    int32_t loc = spriteShader->GetAttributeLocation("position");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);

    loc = spriteShader->GetAttributeLocation("uv");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)8);

    loc = spriteShader->GetAttributeLocation("palIndex");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, stride, (void*)16);
}

void Renderer_GL::SetPipelineBatch() {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferBatch);
    const int stride = 20;

    int32_t loc = spriteShader->GetAttributeLocation("position");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);

    loc = spriteShader->GetAttributeLocation("uv");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)8);

    loc = spriteShader->GetAttributeLocation("palIndex");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, stride, (void*)16);
}

void Renderer_GL::ReleasePipeline() {
    int32_t loc = spriteShader->GetAttributeLocation("position");
    glDisableVertexAttribArray(loc);

    loc = spriteShader->GetAttributeLocation("uv");
    glDisableVertexAttribArray(loc);

    loc = spriteShader->GetAttributeLocation("palIndex");
    glDisableVertexAttribArray(loc);

    glDisable(GL_BLEND);
}

void Renderer_GL::prepareModelPipeline(uint32_t bufferIndex, const Environment* env) {
    if (!modelShader) return;

    glUseProgram(modelShader->GetProgram());
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, 1920, 1080);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glEnable(GL_BLEND);

    if (glState.depthTest) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(glState.depthMask);
    glFrontFace(glState.invertFrontFace ? GL_CW : GL_CCW);

    if (!glState.doubleSided) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }

    glBlendEquation(MapBlendEquation(glState.blendEquation));
    glBlendFunc(MapBlendFunction(glState.blendSrc), MapBlendFunction(glState.blendDst));

    glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer[bufferIndex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer[bufferIndex]);
}

void Renderer_GL::SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst,
                                   bool depthTest, bool depthMask, bool doubleSided, bool invertFrontFace,
                                   bool useUV, bool useNormal, bool useTangent, bool useVertColor,
                                   bool useJoint0, bool useJoint1, bool useOutlineAttribute,
                                   uint32_t numVertices, uint32_t vertAttrOffset) {
    if (!modelShader) return;

    SetDepthTest(depthTest);
    SetDepthMask(depthMask);
    SetFrontFace(invertFrontFace);
    SetCullFace(doubleSided);
    SetBlending(eq, src, dst);

    // Setup vertex attributes - simplified version
    // A full implementation would set up all the vertex attribute pointers
}

void Renderer_GL::ReleaseModelPipeline() {
    if (!modelShader) return;

    glDepthMask(true);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glState.useUV = false;
    glState.useNormal = false;
    glState.useTangent = false;
    glState.useVertColor = false;
    glState.useJoint0 = false;
    glState.useJoint1 = false;
    glState.useOutlineAttribute = false;
}

void Renderer_GL::prepareShadowMapPipeline(uint32_t bufferIndex) {
    if (!shadowMapShader) return;

    glUseProgram(shadowMapShader->GetProgram());
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
    glViewport(0, 0, 1024, 1024);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(true);

    glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer[bufferIndex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer[bufferIndex]);
}

void Renderer_GL::setShadowMapPipeline(bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal,
                                      bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1,
                                      uint32_t numVertices, uint32_t vertAttrOffset) {
    if (!shadowMapShader) return;

    SetFrontFace(invertFrontFace);
    SetCullFace(doubleSided);
    // Additional setup would go here
}

void Renderer_GL::ReleaseShadowPipeline() {
    glDepthMask(true);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glState.useUV = false;
    glState.useJoint0 = false;
    glState.useJoint1 = false;
}

void Renderer_GL::SetMeshOulinePipeline(bool invertFrontFace, float meshOutline) {
    SetFrontFace(invertFrontFace);
    SetDepthTest(true);
    SetDepthMask(true);

    if (modelShader) {
        int32_t loc = modelShader->GetUniformLocation("meshOutline");
        glUniform1f(loc, meshOutline);
    }
}

void Renderer_GL::Scissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, 1080 - (y + height), width, height);
}

void Renderer_GL::DisableScissor() {
    glDisable(GL_SCISSOR_TEST);
}

std::shared_ptr<ITexture> Renderer_GL::newTexture(int32_t width, int32_t height, int32_t depth, bool filter) {
    uint32_t handle;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    return std::make_shared<Texture_GL>(width, height, depth, filter, handle);
}

std::shared_ptr<ITexture> Renderer_GL::newPaletteTexture() {
    return newTexture(256, 1, 32, false);
}

std::shared_ptr<ITexture> Renderer_GL::newPaletteTextureArray(int32_t layers) {
    uint32_t handle;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 256, 1, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return std::make_shared<Texture_GL>(256, 1, layers, false, handle);
}

std::shared_ptr<ITexture> Renderer_GL::newModelTexture(int32_t width, int32_t height, int32_t depth, bool filter) {
    return newTexture(width, height, depth, filter);
}

std::shared_ptr<ITexture> Renderer_GL::newDataTexture(int32_t width, int32_t height) {
    uint32_t handle;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return std::make_shared<Texture_GL>(width, height, 128, false, handle);
}

std::shared_ptr<ITexture> Renderer_GL::newHDRTexture(int32_t width, int32_t height) {
    uint32_t handle;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    return std::make_shared<Texture_GL>(width, height, 96, false, handle);
}

std::shared_ptr<ITexture> Renderer_GL::newCubeMapTexture(int32_t widthHeight, bool mipmap, int32_t lowestMipLevel) {
    uint32_t handle;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
    
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, widthHeight, widthHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    }

    if (mipmap) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    } else {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return std::make_shared<Texture_GL>(widthHeight, widthHeight, 24, false, handle);
}

void Renderer_GL::SetTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) {
    if (!tex || !tex->IsValid() || !spriteShader) {
        std::cerr << "Renderer_GL.SetTexture " << name << " is empty or invalid" << std::endl;
        return;
    }

    int32_t loc = spriteShader->GetUniformLocation(name);
    int32_t unit = spriteShader->GetTextureUnit(name);

    glActiveTexture(GL_TEXTURE0 + unit);
    
    // Check if it's a palette texture array
    if (name == "pal" || (tex->GetDepth() > 1 && tex->GetHeight() == 1)) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, tex->GetHandle());
    } else {
        glBindTexture(GL_TEXTURE_2D, tex->GetHandle());
    }
    
    glUniform1i(loc, unit);
}

void Renderer_GL::SetModelTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) {
    if (!modelShader || !tex) return;

    int32_t loc = modelShader->GetUniformLocation(name);
    int32_t unit = modelShader->GetTextureUnit(name);

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex->GetHandle());
    glUniform1i(loc, unit);
}

void Renderer_GL::SetShadowMapTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) {
    if (!shadowMapShader || !tex) return;

    int32_t loc = shadowMapShader->GetUniformLocation(name);
    int32_t unit = shadowMapShader->GetTextureUnit(name);

    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, tex->GetHandle());
    glUniform1i(loc, unit);
}

void Renderer_GL::SetUniformI(const std::string& name, int val) {
    if (!spriteShader) return;
    int32_t loc = spriteShader->GetUniformLocation(name);
    glUniform1i(loc, val);
}

void Renderer_GL::SetUniformF(const std::string& name, const std::vector<float>& values) {
    if (!spriteShader) return;
    int32_t loc = spriteShader->GetUniformLocation(name);
    
    switch (values.size()) {
    case 1:
        glUniform1f(loc, values[0]);
        break;
    case 2:
        glUniform2f(loc, values[0], values[1]);
        break;
    case 3:
        glUniform3f(loc, values[0], values[1], values[2]);
        break;
    case 4:
        glUniform4f(loc, values[0], values[1], values[2], values[3]);
        break;
    }
}

void Renderer_GL::SetUniformFv(const std::string& name, const std::vector<float>& values) {
    if (!spriteShader) return;
    int32_t loc = spriteShader->GetUniformLocation(name);
    
    switch (values.size()) {
    case 2:
        glUniform2fv(loc, 1, values.data());
        break;
    case 3:
        glUniform3fv(loc, 1, values.data());
        break;
    case 4:
        glUniform4fv(loc, 1, values.data());
        break;
    }
}

void Renderer_GL::SetUniformMatrix(const std::string& name, const std::vector<float>& value) {
    if (!spriteShader || value.size() != 16) return;
    int32_t loc = spriteShader->GetUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
}

void Renderer_GL::SetModelUniformI(const std::string& name, int val) {
    if (!modelShader) return;
    int32_t loc = modelShader->GetUniformLocation(name);
    glUniform1i(loc, val);
}

void Renderer_GL::SetModelUniformF(const std::string& name, const std::vector<float>& values) {
    if (!modelShader) return;
    int32_t loc = modelShader->GetUniformLocation(name);
    
    switch (values.size()) {
    case 1:
        glUniform1f(loc, values[0]);
        break;
    case 2:
        glUniform2f(loc, values[0], values[1]);
        break;
    case 3:
        glUniform3f(loc, values[0], values[1], values[2]);
        break;
    case 4:
        glUniform4f(loc, values[0], values[1], values[2], values[3]);
        break;
    }
}

void Renderer_GL::SetModelUniformFv(const std::string& name, const std::vector<float>& values) {
    if (!modelShader) return;
    int32_t loc = modelShader->GetUniformLocation(name);
    
    switch (values.size()) {
    case 2:
        glUniform2fv(loc, 1, values.data());
        break;
    case 3:
        glUniform3fv(loc, 1, values.data());
        break;
    case 4:
        glUniform4fv(loc, 1, values.data());
        break;
    case 8:
        glUniform4fv(loc, 2, values.data());
        break;
    }
}

void Renderer_GL::SetModelUniformMatrix(const std::string& name, const std::vector<float>& value) {
    if (!modelShader || value.size() != 16) return;
    int32_t loc = modelShader->GetUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
}

void Renderer_GL::SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value) {
    if (!modelShader || value.size() != 9) return;
    int32_t loc = modelShader->GetUniformLocation(name);
    glUniformMatrix3fv(loc, 1, GL_FALSE, value.data());
}

void Renderer_GL::SetShadowMapUniformI(const std::string& name, int val) {
    if (!shadowMapShader) return;
    int32_t loc = shadowMapShader->GetUniformLocation(name);
    glUniform1i(loc, val);
}

void Renderer_GL::SetShadowMapUniformF(const std::string& name, const std::vector<float>& values) {
    if (!shadowMapShader) return;
    int32_t loc = shadowMapShader->GetUniformLocation(name);
    
    switch (values.size()) {
    case 1:
        glUniform1f(loc, values[0]);
        break;
    case 2:
        glUniform2f(loc, values[0], values[1]);
        break;
    case 3:
        glUniform3f(loc, values[0], values[1], values[2]);
        break;
    case 4:
        glUniform4f(loc, values[0], values[1], values[2], values[3]);
        break;
    }
}

void Renderer_GL::SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values) {
    if (!shadowMapShader) return;
    int32_t loc = shadowMapShader->GetUniformLocation(name);
    
    switch (values.size()) {
    case 2:
        glUniform2fv(loc, 1, values.data());
        break;
    case 3:
        glUniform3fv(loc, 1, values.data());
        break;
    case 4:
        glUniform4fv(loc, 1, values.data());
        break;
    case 8:
        glUniform4fv(loc, 2, values.data());
        break;
    }
}

void Renderer_GL::SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value) {
    if (!shadowMapShader || value.size() != 16) return;
    int32_t loc = shadowMapShader->GetUniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
}

void Renderer_GL::SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value) {
    if (!shadowMapShader || value.size() != 9) return;
    int32_t loc = shadowMapShader->GetUniformLocation(name);
    glUniformMatrix3fv(loc, 1, GL_FALSE, value.data());
}

void Renderer_GL::SetShadowFrameTexture(uint32_t i) {
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_shadow_cube_texture, 0);
}

void Renderer_GL::SetShadowFrameCubeTexture(uint32_t i) {
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_shadow_cube_texture, 0);
}

void Renderer_GL::SetVertexData(const std::vector<float>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL::SetVertexDataArray(const std::vector<float>& values) {
    if (values.empty()) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferBatch);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL::SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer[bufferIndex]);
    glBufferData(GL_ARRAY_BUFFER, values.size(), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL::SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer[bufferIndex]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, values.size() * sizeof(uint32_t), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL::RenderQuad() {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer_GL::RenderQuadBatch(int32_t vertexCount) {
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void Renderer_GL::RenderElements(PrimitiveMode mode, int count, int offset) {
    glDrawElementsBaseVertex(MapPrimitiveMode(mode), count, GL_UNSIGNED_INT, 
                             reinterpret_cast<const void*>(static_cast<uintptr_t>(offset)), 0);
}

void Renderer_GL::RenderShadowMapElements(PrimitiveMode mode, int count, int offset) {
    RenderElements(mode, count, offset);
}

void Renderer_GL::RenderCubeMap(const std::shared_ptr<ITexture>& envTex, const std::shared_ptr<ITexture>& cubeTex) {
    if (!envTex || !cubeTex || !panoramaToCubeMapShader) return;

    int32_t textureSize = cubeTex->GetWidth();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_env);
    glViewport(0, 0, textureSize, textureSize);
    glUseProgram(panoramaToCubeMapShader->GetProgram());

    for (int i = 0; i < 6; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeTex->GetHandle(), 0);
        glClear(GL_COLOR_BUFFER_BIT);

        int32_t loc = panoramaToCubeMapShader->GetUniformLocation("currentFace");
        glUniform1i(loc, i);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTex->GetHandle());
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void Renderer_GL::RenderFilteredCubeMap(int32_t distribution, const std::shared_ptr<ITexture>& cubeTex,
                                       const std::shared_ptr<ITexture>& filteredTex,
                                       int32_t mipmapLevel, int32_t sampleCount, float roughness) {
    if (!cubeTex || !filteredTex || !cubemapFilteringShader) return;

    int32_t textureSize = filteredTex->GetWidth();
    int32_t currentTextureSize = textureSize >> mipmapLevel;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_env);
    glViewport(0, 0, currentTextureSize, currentTextureSize);
    glUseProgram(cubemapFilteringShader->GetProgram());

    int32_t loc = cubemapFilteringShader->GetUniformLocation("sampleCount");
    glUniform1i(loc, sampleCount);

    loc = cubemapFilteringShader->GetUniformLocation("distribution");
    glUniform1i(loc, distribution);

    loc = cubemapFilteringShader->GetUniformLocation("width");
    glUniform1i(loc, textureSize);

    loc = cubemapFilteringShader->GetUniformLocation("roughness");
    glUniform1f(loc, roughness);

    for (int i = 0; i < 6; ++i) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                              filteredTex->GetHandle(), mipmapLevel);
        glClear(GL_COLOR_BUFFER_BIT);

        loc = cubemapFilteringShader->GetUniformLocation("currentFace");
        glUniform1i(loc, i);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Renderer_GL::RenderLUT(int32_t distribution, const std::shared_ptr<ITexture>& cubeTex,
                           const std::shared_ptr<ITexture>& lutTex, int32_t sampleCount) {
    if (!cubeTex || !lutTex || !cubemapFilteringShader) return;

    int32_t textureSize = lutTex->GetWidth();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_env);
    glViewport(0, 0, textureSize, textureSize);
    glUseProgram(cubemapFilteringShader->GetProgram());

    int32_t loc = cubemapFilteringShader->GetUniformLocation("sampleCount");
    glUniform1i(loc, sampleCount);

    loc = cubemapFilteringShader->GetUniformLocation("distribution");
    glUniform1i(loc, distribution);

    loc = cubemapFilteringShader->GetUniformLocation("width");
    glUniform1i(loc, textureSize);

    loc = cubemapFilteringShader->GetUniformLocation("roughness");
    glUniform1f(loc, 0);

    loc = cubemapFilteringShader->GetUniformLocation("currentFace");
    glUniform1i(loc, 0);

    loc = cubemapFilteringShader->GetUniformLocation("isLUT");
    glUniform1i(loc, 1);

    glBindTexture(GL_TEXTURE_2D, lutTex->GetHandle());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, lutTex->GetWidth(), lutTex->GetHeight(), 0, GL_RGBA, GL_FLOAT, nullptr);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lutTex->GetHandle(), 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Renderer_GL::ReadPixels(std::vector<uint8_t>& data, int width, int height) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    if (data.size() < static_cast<size_t>(width * height * 4)) {
        data.resize(width * height * 4);
    }
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}

Mat4 Renderer_GL::PerspectiveProjectionMatrix(float angle, float aspect, float _near, float _far) const {
    Mat4 res;
    mat4x4_perspective(res.data, angle, aspect, _near, _far);
    return res;
}

Mat4 Renderer_GL::OrthographicProjectionMatrix(float left, float right, float bottom,
                                                       float top, float _near, float _far) const {
    Mat4 res;
    mat4x4_ortho(res.data, left, right, bottom, top, _near, _far);
    return res;
}

bool Renderer_GL::NewWorkerThread() {
    return false;
}

void Renderer_GL::SetVSync() {
    // Implementation depends on platform and windowing system
}

// ===== Private Helper Methods =====

std::shared_ptr<ShaderProgram_GL> Renderer_GL::newShaderProgram(const std::string& vert, const std::string& frag,
                                                               const std::string& geo, const std::string& id,
                                                               bool crashWhenFail) {
    auto shader = std::make_shared<ShaderProgram_GL>();

    // This is a simplified version. A full implementation would compile and link shaders
    // including error checking and logging.

    return shader;
}

uint32_t Renderer_GL::compileShader(uint32_t shaderType, const std::string& src) {
    uint32_t shader = glCreateShader(shaderType);
    std::string versioned = "#version 330 core\n" + src;
    const char* srcPtr = versioned.c_str();
    glShaderSource(shader, 1, &srcPtr, nullptr);
    glCompileShader(shader);

    int32_t ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        int32_t size, len;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            std::vector<char> errorLog(size);
            glGetShaderInfoLog(shader, size, &len, errorLog.data());
            std::cerr << "Shader compilation error: " << errorLog.data() << std::endl;
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

uint32_t Renderer_GL::linkProgram(const std::vector<uint32_t>& shaders) {
    uint32_t program = glCreateProgram();

    for (uint32_t shader : shaders) {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);

    for (uint32_t shader : shaders) {
        glDeleteShader(shader);
    }

    int32_t ok;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (ok == 0) {
        int32_t size, len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
        if (size > 0) {
            std::vector<char> errorLog(size);
            glGetProgramInfoLog(program, size, &len, errorLog.data());
            std::cerr << "Program linking error: " << errorLog.data() << std::endl;
        }
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

uint32_t Renderer_GL::MapBlendEquation(BlendEquation eq) const {
    static const std::map<BlendEquation, uint32_t> BlendEquationLUT = {
        {BlendEquation::Add, GL_FUNC_ADD},
        {BlendEquation::ReverseSubtract, GL_FUNC_REVERSE_SUBTRACT}
    };

    auto it = BlendEquationLUT.find(eq);
    return (it != BlendEquationLUT.end()) ? it->second : GL_FUNC_ADD;
}

uint32_t Renderer_GL::MapBlendFunction(BlendFunc func) const {
    static const std::map<BlendFunc, uint32_t> BlendFunctionLUT = {
        {BlendFunc::One, GL_ONE},
        {BlendFunc::Zero, GL_ZERO},
        {BlendFunc::SrcAlpha, GL_SRC_ALPHA},
        {BlendFunc::OneMinusSrcAlpha, GL_ONE_MINUS_SRC_ALPHA},
        {BlendFunc::OneMinusDstColor, GL_ONE_MINUS_DST_COLOR},
        {BlendFunc::DstColor, GL_DST_COLOR}
    };

    auto it = BlendFunctionLUT.find(func);
    return (it != BlendFunctionLUT.end()) ? it->second : GL_ONE;
}

uint32_t Renderer_GL::MapPrimitiveMode(PrimitiveMode mode) const {
    static const std::map<PrimitiveMode, uint32_t> PrimitiveModeLUT = {
        {PrimitiveMode::Lines, GL_LINES},
        {PrimitiveMode::LineLoop, GL_LINE_LOOP},
        {PrimitiveMode::LineStrip, GL_LINE_STRIP},
        {PrimitiveMode::Triangles, GL_TRIANGLES},
        {PrimitiveMode::TriangleStrip, GL_TRIANGLE_STRIP},
        {PrimitiveMode::TriangleFan, GL_TRIANGLE_FAN}
    };

    auto it = PrimitiveModeLUT.find(mode);
    return (it != PrimitiveModeLUT.end()) ? it->second : GL_TRIANGLES;
}

bool Renderer_GL::InitFramebuffer(uint32_t& fbo, uint32_t& texture, uint32_t& rbo,
                                  int32_t width, int32_t height, bool useMultisample) {
    // Generate texture
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Allocate texture storage
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, 
                GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    // Generate depth renderbuffer
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    
    if (useMultisample && msaaLevel > 0) {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaLevel,
                                        GL_DEPTH_COMPONENT24, width, height);
    } else {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    }
    
    // Generate and configure framebuffer
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                             GL_RENDERBUFFER, rbo);
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer incomplete: " << std::hex << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return false;
    }
    
    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    
    return true;
}

bool Renderer_GL::InitShadowFramebuffer() {
    if (!enableShadow) return true;
    
    // Generate shadow framebuffer
    glGenFramebuffers(1, &fbo_shadow);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
    
    // Generate cube map texture for point light shadows
    glGenTextures(1, &fbo_shadow_cube_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, fbo_shadow_cube_texture);
    
    // Allocate storage for all 6 faces
    const int32_t shadowMapSize = 1024; // Default shadow map resolution
    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24,
                    shadowMapSize, shadowMapSize, 0,
                    GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    // Attach first face as depth attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                          GL_TEXTURE_CUBE_MAP_POSITIVE_X, fbo_shadow_cube_texture, 0);
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow framebuffer incomplete: " << std::hex << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        return false;
    }
    
    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    return true;
}

bool Renderer_GL::InitPostProcessingFramebuffers() {
    // Create 2 post-processing framebuffers for ping-pong rendering
    fbo_pp.resize(2);
    fbo_pp_texture.resize(2);
    
    for (int i = 0; i < 2; i++) {
        // Generate texture
        glGenTextures(1, &fbo_pp_texture[i]);
        glBindTexture(GL_TEXTURE_2D, fbo_pp_texture[i]);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Use RGBA32F for HDR post-processing
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, viewport.width, viewport.height,
                    0, GL_RGBA, GL_FLOAT, nullptr);
        
        // Generate framebuffer
        glGenFramebuffers(1, &fbo_pp[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_pp[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, fbo_pp_texture[i], 0);
        
        // Check framebuffer completeness
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Post-processing framebuffer " << i << " incomplete: "
                     << std::hex << status << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            return false;
        }
    }
    
    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return true;
}

void Renderer_GL::CacheRenderState() {
    // Cache current OpenGL state to restore later
    // This is useful when temporarily changing state for specific operations
    
    // Save blend state
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    if (blendEnabled) {
        glState.blendEquation = BlendEquation::Add; // Default, would need to query actual value
        glState.blendSrc = BlendFunc::One;
        glState.blendDst = BlendFunc::Zero;
    }
    
    // Save depth state
    glState.depthTest = glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    GLboolean depthWriteMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteMask);
    glState.depthMask = (depthWriteMask == GL_TRUE);
    
    // Save face culling state
    glState.doubleSided = (glIsEnabled(GL_CULL_FACE) == GL_FALSE);
    
    // Save front face orientation
    GLint frontFaceMode;
    glGetIntegerv(GL_FRONT_FACE, &frontFaceMode);
    glState.invertFrontFace = (frontFaceMode == GL_CW);
}

void Renderer_GL::RestoreRenderState() {
    // Restore previously cached render state
    
    // Restore blend state
    SetBlending(glState.blendEquation, glState.blendSrc, glState.blendDst);
    
    // Restore depth state
    SetDepthTest(glState.depthTest);
    SetDepthMask(glState.depthMask);
    
    // Restore face culling
    SetCullFace(glState.doubleSided);
    
    // Restore front face
    SetFrontFace(glState.invertFrontFace);
}

void Renderer_GL::ResetRenderState() {
    // Reset to default OpenGL state
    
    // Reset blend state
    glDisable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ZERO);
    glState.blendEquation = BlendEquation::Add;
    glState.blendSrc = BlendFunc::One;
    glState.blendDst = BlendFunc::Zero;
    
    // Reset depth state
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glState.depthTest = false;
    glState.depthMask = true;
    
    // Reset face culling
    glDisable(GL_CULL_FACE);
    glState.doubleSided = true;
    
    // Reset front face
    glFrontFace(GL_CCW);
    glState.invertFrontFace = false;
    
    // Reset scissor test
    glDisable(GL_SCISSOR_TEST);
    
    // Reset vertex attribute state flags
    glState.useUV = false;
    glState.useNormal = false;
    glState.useTangent = false;
    glState.useVertColor = false;
    glState.useJoint0 = false;
    glState.useJoint1 = false;
    glState.useOutlineAttribute = false;
}

uint32_t Renderer_GL::MapTextureTarget(const std::shared_ptr<ITexture>& tex) const {
    if (!tex) return GL_TEXTURE_2D;
    
    // Determine texture target based on depth (layer count)
    if (tex->GetDepth() > 1) {
        return GL_TEXTURE_2D_ARRAY;
    }
    
    return GL_TEXTURE_2D;
}

void Renderer_GL::SetupVertexAttributes(const std::shared_ptr<ShaderProgram_GL>& shader,
                                        uint32_t stride,
                                        const std::vector<std::string>& attributes) {
    if (!shader) return;
    
    // Common vertex attribute setup helper
    // Attributes are typically: position, uv, normal, tangent, color, joints, weights, etc.
    
    uint32_t offset = 0;
    
    for (const auto& attrName : attributes) {
        GLint location = shader->GetAttributeLocation(attrName);
        if (location < 0) continue; // Attribute not found or not active
        
        glEnableVertexAttribArray(location);
        
        // Determine attribute size based on name
        int componentCount = 3; // Default for position, normal, tangent
        GLenum type = GL_FLOAT;
        GLboolean normalized = GL_FALSE;
        
        if (attrName == "position" || attrName == "aPos") {
            componentCount = 3; // vec3
        } else if (attrName == "uv" || attrName == "aTexCoord") {
            componentCount = 2; // vec2
        } else if (attrName == "normal" || attrName == "aNormal") {
            componentCount = 3; // vec3
        } else if (attrName == "tangent" || attrName == "aTangent") {
            componentCount = 4; // vec4 (includes handedness)
        } else if (attrName == "color" || attrName == "aColor") {
            componentCount = 4; // vec4
        } else if (attrName == "palIndex") {
            componentCount = 1; // float
        } else if (attrName.find("joint") != std::string::npos) {
            componentCount = 4; // ivec4 or vec4
        } else if (attrName.find("weight") != std::string::npos) {
            componentCount = 4; // vec4
        }
        
        glVertexAttribPointer(location, componentCount, type, normalized,
                            stride, reinterpret_cast<void*>(static_cast<uintptr_t>(offset)));
        
        offset += componentCount * sizeof(float);
    }
}

bool Renderer_GL::IsGLExtensionSupported(const std::string& extension) {
    // OpenGL 3.3+ way to query extensions
    GLint numExtensions = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    
    for (GLint i = 0; i < numExtensions; i++) {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (ext && extension == ext) {
            return true;
        }
    }
    
    return false;
}

void Renderer_GL::PrintGLInfo() {
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    
    std::cout << "OpenGL Version: " << (version ? reinterpret_cast<const char*>(version) : "Unknown") << std::endl;
    std::cout << "GLSL Version: " << (glslVersion ? reinterpret_cast<const char*>(glslVersion) : "Unknown") << std::endl;
    std::cout << "Renderer: " << (renderer ? reinterpret_cast<const char*>(renderer) : "Unknown") << std::endl;
    std::cout << "Vendor: " << (vendor ? reinterpret_cast<const char*>(vendor) : "Unknown") << std::endl;
}

void Renderer_GL::PrintGLCapabilities() {
    std::cout << "OpenGL 3.3 Capabilities:" << std::endl;
    
    // Query various capabilities
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    std::cout << "  Max Texture Size: " << maxTextureSize << std::endl;
    
    GLint maxTextureUnits = 0;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    std::cout << "  Max Texture Units: " << maxTextureUnits << std::endl;
    
    GLint maxVertexAttribs = 0;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    std::cout << "  Max Vertex Attributes: " << maxVertexAttribs << std::endl;
    
    GLint maxUniformLocations = 0;
    glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &maxUniformLocations);
    std::cout << "  Max Uniform Locations: " << maxUniformLocations << std::endl;
    
    GLint maxDrawBuffers = 0;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    std::cout << "  Max Draw Buffers: " << maxDrawBuffers << std::endl;
    
    GLint maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    std::cout << "  Max MSAA Samples: " << maxSamples << std::endl;
    
    // Check for common extensions
    std::cout << "  Geometry Shaders: " << (IsGLExtensionSupported("GL_ARB_geometry_shader4") ? "Yes" : "Built-in (GL 3.2+)") << std::endl;
    std::cout << "  Instancing: Built-in (GL 3.3+)" << std::endl;
    std::cout << "  Texture Arrays: Built-in (GL 3.0+)" << std::endl;
    std::cout << "  Sampler Objects: Built-in (GL 3.3+)" << std::endl;
}
