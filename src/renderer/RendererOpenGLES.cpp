#include "RendererOpenGLES.h"
#include <cstring>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <algorithm>

// STB Image Write for PNG saving
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// ------------------------------------------------------------------
// ShaderProgram_GLES Implementation
// ------------------------------------------------------------------

ShaderProgram_GLES::ShaderProgram_GLES() : program(0) {
}

ShaderProgram_GLES::~ShaderProgram_GLES() {
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }
}

void ShaderProgram_GLES::RegisterAttributes(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        GLint loc = glGetAttribLocation(program, name.c_str());
        if (loc >= 0) {
            attributes[name] = loc;
        }
    }
}

void ShaderProgram_GLES::RegisterUniforms(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        GLint loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) {
            uniforms[name] = loc;
        }
    }
}

void ShaderProgram_GLES::RegisterTextures(const std::vector<std::string>& names) {
    int unit = 0;
    for (const auto& name : names) {
        GLint loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) {
            uniforms[name] = loc;
            textures[name] = unit++;
        }
    }
}

int32_t ShaderProgram_GLES::GetAttributeLocation(const std::string& name) const {
    auto it = attributes.find(name);
    return (it != attributes.end()) ? it->second : -1;
}

int32_t ShaderProgram_GLES::GetUniformLocation(const std::string& name) const {
    auto it = uniforms.find(name);
    return (it != uniforms.end()) ? it->second : -1;
}

int32_t ShaderProgram_GLES::GetTextureUnit(const std::string& name) const {
    auto it = textures.find(name);
    return (it != textures.end()) ? it->second : -1;
}

std::string ShaderProgram_GLES::GetShaderInfoLog(uint32_t shader) {
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        std::vector<char> log(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        return std::string(log.data());
    }
    return "";
}

std::string ShaderProgram_GLES::GetProgramInfoLog(uint32_t program) {
    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        std::vector<char> log(logLength);
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        return std::string(log.data());
    }
    return "";
}

uint8_t* ShaderProgram_GLES::glStr(const std::string& s) {
    // Note: In C++, std::string::c_str() already returns a null-terminated string
    // This method exists for API compatibility with the Go version
    // The caller must ensure the string remains in scope while using the pointer
    return const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(s.c_str()));
}

// ------------------------------------------------------------------
// Texture_GLES Implementation
// ------------------------------------------------------------------

Texture_GLES::Texture_GLES(int32_t width, int32_t height, int32_t depth, bool filter, uint32_t handle)
    : ITexture(width, height, depth, filter, handle), width(width), height(height), depth(depth), filter(filter), handle(handle), 
      textureTarget(depth > 1 ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D) {
}

Texture_GLES::~Texture_GLES() {
    if (handle != 0) {
        glDeleteTextures(1, &handle);
        handle = 0;
    }
}

void Texture_GLES::SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) {
    if (handle == 0 || data.empty()) return;
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Upload to specific Z-slice (layer)
    // x=0, y=0, z=layer, w=256, h=1, d=1
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, 256, 1, 1, 
                   GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void Texture_GLES::SetData(const std::vector<uint8_t>& data) {
    if (handle == 0) return;
    
    GLint interp = filter ? GL_LINEAR : GL_NEAREST;
    uint32_t format = MapInternalFormat(std::max(depth, (int32_t)8));
    
    glBindTexture(GL_TEXTURE_2D, handle);
    // Ensure any pending GL commands which upload texture data finish before
    // reading with GetTexImage. This avoids reading an incomplete texture on
    // some drivers where uploads are deferred.
    glFinish();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    if (!data.empty()) {
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, 
                    format, GL_UNSIGNED_BYTE, data.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, 
                    format, GL_UNSIGNED_BYTE, nullptr);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture_GLES::SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, 
                               int32_t width, int32_t height) {
    if (handle == 0) return;
    
    uint32_t format = MapInternalFormat(std::max(this->depth, (int32_t)8));
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    if (!data.empty()) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height,
                       format, GL_UNSIGNED_BYTE, data.data());
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture_GLES::SetDataG(const std::vector<uint8_t>& data, 
                             TextureSamplingParam mag, 
                             TextureSamplingParam min,
                             TextureSamplingParam ws, 
                             TextureSamplingParam wt) {
    if (handle == 0) return;
    
    uint32_t format = MapInternalFormat(std::max(depth, (int32_t)8));
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, 
                format, GL_UNSIGNED_BYTE, data.empty() ? nullptr : data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, MapTextureSamplingParam(mag));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, MapTextureSamplingParam(min));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, MapTextureSamplingParam(ws));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, MapTextureSamplingParam(wt));
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture_GLES::SetPixelData(const std::vector<float>& data) {
    if (handle == 0) return;
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, 
                GL_RGBA, GL_FLOAT, data.empty() ? nullptr : data.data());
    
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture_GLES::CopyData(const Texture_GLES* src) {
    if (!src || !src->IsValid() || !IsValid()) {
        return;
    }
    
    if (width != src->width || height != src->height) {
        return;
    }
    
    // 1. SAVE THE CURRENT FRAMEBUFFER BINDING
    // If we don't do this, we break the rendering loop by unbinding the main FBO.
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    
    // Create source FBO
    GLuint srcFBO = 0;
    glGenFramebuffers(1, &srcFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, src->handle, 0);
    
    // Create destination FBO
    GLuint dstFBO = 0;
    glGenFramebuffers(1, &dstFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFBO);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                          GL_TEXTURE_2D, handle, 0);
    
    // Check status before blitting
    if (glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE &&
        glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE) {
        glBlitFramebuffer(0, 0, width, height,
                         0, 0, width, height,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    
    // Cleanup Temp FBOs
    glDeleteFramebuffers(1, &srcFBO);
    glDeleteFramebuffers(1, &dstFBO);
    
    // 2. RESTORE THE PREVIOUS FRAMEBUFFER
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFBO));
}

bool Texture_GLES::IsValid() const {
    return width != 0 && height != 0 && handle != 0;
}

int Texture_GLES::SavePNG(const std::string& filename, const std::vector<uint32_t>* palette) {
    printf("SavePNG: filename=%s width=%d height=%d depth=%d handle=%u\n",
           filename.c_str(), width, height, depth, handle);
    
    if (!IsValid()) {
        std::cerr << "SavePNG: Invalid texture" << std::endl;
        return -1;
    }
    
    // --- 1. Read Texture Data from GPU (GLES Workaround) ---
    
    // Save the current framebuffer so we don't break the render loop
    GLint prevFBO = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
    
    // Create a temporary FBO to read the texture
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, handle, 0);
    
    // Check FBO status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SavePNG: FBO incomplete: " << std::hex << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFBO));
        glDeleteFramebuffers(1, &fbo);
        return -1;
    }
    
    // Read pixels as RGBA (safest for GLES compatibility)
    // We read 4 bytes per pixel even if depth is 8.
    int rawSize = static_cast<int>(width) * static_cast<int>(height) * 4;
    std::vector<uint8_t> rawData(rawSize);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rawData.data());
    
    // Restore state and cleanup
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFBO));
    glDeleteFramebuffers(1, &fbo);
    
    // --- 2. Process Data ---
    
    std::vector<uint8_t> data;
    
    if (depth == 8) {
        // EXTRACT INDICES:
        // If the texture is 8-bit (indexed), the index is in the Red channel.
        // Since we read RGBA, we take every 4th byte.
        data.resize(width * height);
        for (int32_t i = 0; i < width * height; i++) {
            data[i] = rawData[i * 4]; // R channel = palette index
        }
    } else {
        // Use raw RGBA data
        data = rawData;
    }
    
    // --- 3. Palette Handling (Ported from render_gl33.go) ---
    
    if (depth == 8 && palette && !palette->empty()) {
        // 8-bit texture: data contains palette indices.
        // Convert to RGBA using the provided palette
        std::vector<uint8_t> rgbaData(width * height * 4);
        for (int32_t i = 0; i < width * height; i++) {
            uint8_t index = data[i];
            if (index < palette->size()) {
                uint32_t color = (*palette)[index];
                rgbaData[i * 4 + 0] = (color >> 0) & 0xFF;   // R
                rgbaData[i * 4 + 1] = (color >> 8) & 0xFF;   // G
                rgbaData[i * 4 + 2] = (color >> 16) & 0xFF;  // B
                rgbaData[i * 4 + 3] = (color >> 24) & 0xFF;  // A
            } else {
                // Out of bounds - use transparent black
                rgbaData[i * 4 + 0] = 0;
                rgbaData[i * 4 + 1] = 0;
                rgbaData[i * 4 + 2] = 0;
                rgbaData[i * 4 + 3] = 0;
            }
        }
        data = rgbaData;
    } else if (depth == 8) {
        // Fallback: expand indices into an opaque grayscale RGBA
        std::vector<uint8_t> rgbaData(width * height * 4);
        for (int32_t i = 0; i < width * height; i++) {
            uint8_t gray = data[i];
            rgbaData[i * 4 + 0] = gray;
            rgbaData[i * 4 + 1] = gray;
            rgbaData[i * 4 + 2] = gray;
            rgbaData[i * 4 + 3] = 255;
        }
        data = rgbaData;
    }
    
    // --- 4. Standard RGBA Saving ---
    
    // Note: glReadPixels returns data from bottom-left. Standard PNG is top-left.
    // We need to flip rows vertically
    std::vector<uint8_t> flipped(data.size());
    int32_t stride = width * 4;
    for (int32_t y = 0; y < height; y++) {
        memcpy(&flipped[y * stride], 
               &data[(height - 1 - y) * stride], 
               stride);
    }
    
    // Write PNG file
    if (!stbi_write_png(filename.c_str(), width, height, 4, flipped.data(), stride)) {
        std::cerr << "SavePNG: Failed to write PNG file" << std::endl;
        return -1;
    }
    
    return 0;
}

void Texture_GLES::SetFilterMode(bool linearFilter) {
    if (handle == 0) return;
    
    filter = linearFilter;
    GLint filterMode = linearFilter ? GL_LINEAR : GL_NEAREST;
    
    glBindTexture(textureTarget, handle);
    glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, filterMode);
    glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, filterMode);
    glBindTexture(textureTarget, 0);
}

void Texture_GLES::SetTextureParameters() {
    if (handle == 0) return;
    
    GLint filterMode = filter ? GL_LINEAR : GL_NEAREST;
    
    glBindTexture(textureTarget, handle);
    glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, filterMode);
    glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, filterMode);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(textureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(textureTarget, 0);
}

uint32_t Texture_GLES::MapInternalFormat(int32_t depth) const {
    switch (depth) {
        case 8:   return GL_RED;      // single-channel (Red)
        case 24:  return GL_RGB;      // 8 bits per RGB channel
        case 32:  return GL_RGBA;     // 8 bits per RGBA channel
        case 96:  return GL_RGB32F;   // 32-bit float per RGB channel
        case 128: return GL_RGBA32F;  // 32-bit float per RGBA channel
        default:  return GL_RGBA;
    }
}

int32_t Texture_GLES::MapTextureSamplingParam(TextureSamplingParam param) const {
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

// ------------------------------------------------------------------
// Renderer_GLES Implementation
// ------------------------------------------------------------------

Renderer_GLES::Renderer_GLES() 
    : IRenderer(), msaaLevel(0) {
    
    modelVertexBuffer[0] = modelVertexBuffer[1] = 0;
    modelIndexBuffer[0] = modelIndexBuffer[1] = 0;
    viewport = {0, 0, 0, 0};
    
    // Initialize capabilities struct
    capabilities.hasInstancedArrays = false;
    capabilities.hasTextureArrays = true;  // ES 3.0+
    capabilities.hasSamplerObjects = true;  // ES 3.0+
    capabilities.hasComputeShaders = true;  // ES 3.1+
    capabilities.hasIndirectDispatch = true;  // ES 3.1+
    capabilities.hasShaderImageLoadStore = true;  // ES 3.1+
}

Renderer_GLES::~Renderer_GLES() {
    Close();
}

std::string Renderer_GLES::GetName() const {
    return "OpenGL ES 3.1";
}

void Renderer_GLES::Init() {
    // Print GL info
    PrintGLESInfo();
    
    // Detect capabilities
    DetectCapabilities();
    
    // Generate VAO (required in ES 3.0+)
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Generate buffers
    glGenBuffers(1, &postVertBuffer);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(1, &vertexBufferBatch);
    glGenBuffers(2, &modelVertexBuffer[0]);
    glGenBuffers(2, &modelIndexBuffer[0]);
    
    // TODO: Initialize shaders when shader sources are available
    // TODO: Initialize framebuffers
    // TODO: Set up post-processing
    
    glActiveTexture(GL_TEXTURE0);
}

void Renderer_GLES::Close() {
    // Delete VAO
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    
    // Delete buffers
    if (postVertBuffer != 0) glDeleteBuffers(1, &postVertBuffer);
    if (vertexBuffer != 0) glDeleteBuffers(1, &vertexBuffer);
    if (vertexBufferBatch != 0) glDeleteBuffers(1, &vertexBufferBatch);
    if (modelVertexBuffer[0] != 0) glDeleteBuffers(2, &modelVertexBuffer[0]);
    if (modelIndexBuffer[0] != 0) glDeleteBuffers(2, &modelIndexBuffer[0]);
    
    // Delete framebuffers
    if (fbo != 0) glDeleteFramebuffers(1, &fbo);
    if (fbo_f != 0) glDeleteFramebuffers(1, &fbo_f);
    if (fbo_shadow != 0) glDeleteFramebuffers(1, &fbo_shadow);
    if (fbo_env != 0) glDeleteFramebuffers(1, &fbo_env);
    
    // Delete textures
    if (fbo_texture != 0) glDeleteTextures(1, &fbo_texture);
    if (rbo_depth != 0) glDeleteRenderbuffers(1, &rbo_depth);
    if (fbo_shadow_cube_texture != 0) glDeleteTextures(1, &fbo_shadow_cube_texture);
    
    // Delete post-processing FBOs
    if (!fbo_pp.empty()) {
        glDeleteFramebuffers(fbo_pp.size(), fbo_pp.data());
        fbo_pp.clear();
    }
    if (!fbo_pp_texture.empty()) {
        glDeleteTextures(fbo_pp_texture.size(), fbo_pp_texture.data());
        fbo_pp_texture.clear();
    }
}

int Renderer_GLES::InitModelShader() {
    // TODO: Implement when shader sources are available
    return 0;
}

void Renderer_GLES::BeginFrame(bool clearColor) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    
    GLbitfield clearBits = GL_DEPTH_BUFFER_BIT;
    if (clearColor) {
        clearBits |= GL_COLOR_BUFFER_BIT;
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }
    glClear(clearBits);
}

void Renderer_GLES::EndFrame() {
    // TODO: Implement post-processing pipeline when shaders are available
    glFinish();
}

void Renderer_GLES::Await() {
    glFinish();
}

bool Renderer_GLES::IsModelEnabled() const {
    return enableModel;
}

bool Renderer_GLES::IsShadowEnabled() const {
    return enableShadow;
}

void Renderer_GLES::BlendReset() {
    glDisable(GL_BLEND);
    glState.blendEquation = BlendEquation::Add;
    glState.blendSrc = BlendFunc::One;
    glState.blendDst = BlendFunc::Zero;
}

void Renderer_GLES::SetBlending(BlendEquation eq, BlendFunc src, BlendFunc dst) {
    if (glState.blendEquation != eq || glState.blendSrc != src || glState.blendDst != dst) {
        glState.blendEquation = eq;
        glState.blendSrc = src;
        glState.blendDst = dst;
        
        glEnable(GL_BLEND);
        glBlendEquation(MapBlendEquation(eq));
        glBlendFunc(MapBlendFunction(src), MapBlendFunction(dst));
    }
}

void Renderer_GLES::SetDepthTest(bool depthTest) {
    if (glState.depthTest != depthTest) {
        glState.depthTest = depthTest;
        if (depthTest) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    }
}

void Renderer_GLES::SetDepthMask(bool depthMask) {
    if (glState.depthMask != depthMask) {
        glState.depthMask = depthMask;
        glDepthMask(depthMask ? GL_TRUE : GL_FALSE);
    }
}

void Renderer_GLES::SetFrontFace(bool invertFrontFace) {
    if (glState.invertFrontFace != invertFrontFace) {
        glState.invertFrontFace = invertFrontFace;
        glFrontFace(invertFrontFace ? GL_CW : GL_CCW);
    }
}

void Renderer_GLES::SetCullFace(bool doubleSided) {
    if (glState.doubleSided != doubleSided) {
        glState.doubleSided = doubleSided;
        if (doubleSided) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
        }
    }
}

void Renderer_GLES::SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) {
    if (!spriteShader) return;
    
    glUseProgram(spriteShader->GetProgram());
    SetBlending(eq, src, dst);
    SetDepthTest(false);
    SetCullFace(true);
    
    // Bind vertex buffer and set up attributes
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    
    GLint stride = 5 * sizeof(float);  // position(2) + uv(2) + palIndex(1)
    
    GLint posLoc = spriteShader->GetAttributeLocation("position");
    if (posLoc >= 0) {
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    }
    
    GLint uvLoc = spriteShader->GetAttributeLocation("uv");
    if (uvLoc >= 0) {
        glEnableVertexAttribArray(uvLoc);
        glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    }
    
    GLint palIndexLoc = spriteShader->GetAttributeLocation("palIndex");
    if (palIndexLoc >= 0) {
        glEnableVertexAttribArray(palIndexLoc);
        glVertexAttribPointer(palIndexLoc, 1, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    }
}

void Renderer_GLES::SetPipelineBatch() {
    if (!spriteShader) return;
    
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferBatch);
    
    GLint stride = 5 * sizeof(float);
    
    GLint posLoc = spriteShader->GetAttributeLocation("position");
    if (posLoc >= 0) {
        glEnableVertexAttribArray(posLoc);
        glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    }
    
    GLint uvLoc = spriteShader->GetAttributeLocation("uv");
    if (uvLoc >= 0) {
        glEnableVertexAttribArray(uvLoc);
        glVertexAttribPointer(uvLoc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));
    }
    
    GLint palIndexLoc = spriteShader->GetAttributeLocation("palIndex");
    if (palIndexLoc >= 0) {
        glEnableVertexAttribArray(palIndexLoc);
        glVertexAttribPointer(palIndexLoc, 1, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
    }
}

void Renderer_GLES::ReleasePipeline() {
    if (!spriteShader) return;
    
    GLint posLoc = spriteShader->GetAttributeLocation("position");
    if (posLoc >= 0) glDisableVertexAttribArray(posLoc);
    
    GLint uvLoc = spriteShader->GetAttributeLocation("uv");
    if (uvLoc >= 0) glDisableVertexAttribArray(uvLoc);
    
    GLint palIndexLoc = spriteShader->GetAttributeLocation("palIndex");
    if (palIndexLoc >= 0) glDisableVertexAttribArray(palIndexLoc);
    
    glUseProgram(0);
}

void Renderer_GLES::prepareShadowMapPipeline(uint32_t bufferIndex) {
    // OpenGL ES 3.1 doesn't support geometry shaders for shadow mapping
}

void Renderer_GLES::setShadowMapPipeline(bool doubleSided, bool invertFrontFace, 
                                         bool useUV, bool useNormal, bool useTangent,
                                         bool useVertColor, bool useJoint0, bool useJoint1,
                                         uint32_t numVertices, uint32_t vertAttrOffset) {
    // TODO: Implement when shadow map shader is available
}

void Renderer_GLES::ReleaseShadowPipeline() {
    // TODO: Implement when shadow map shader is available
}

void Renderer_GLES::prepareModelPipeline(uint32_t bufferIndex, const Environment* env) {
    // TODO: Implement when model shader is available
}

void Renderer_GLES::SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst,
                                     bool depthTest, bool depthMask, bool doubleSided,
                                     bool invertFrontFace, bool useUV, bool useNormal,
                                     bool useTangent, bool useVertColor, bool useJoint0,
                                     bool useJoint1, bool useOutlineAttribute,
                                     uint32_t numVertices, uint32_t vertAttrOffset) {
    // TODO: Implement when model shader is available
}

void Renderer_GLES::SetMeshOulinePipeline(bool invertFrontFace, float meshOutline) {
    // TODO: Implement when model shader is available
}

void Renderer_GLES::ReleaseModelPipeline() {
    // TODO: Implement when model shader is available
}

void Renderer_GLES::Scissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
}

void Renderer_GLES::DisableScissor() {
    glDisable(GL_SCISSOR_TEST);
}

// Texture factory methods
std::shared_ptr<ITexture> Renderer_GLES::newTexture(int32_t width, int32_t height, 
                                                         int32_t depth, bool filter) {
    GLuint handle = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    return std::make_shared<Texture_GLES>(width, height, depth, filter, handle);
}

std::shared_ptr<ITexture> Renderer_GLES::newPaletteTexture() {
    return newTexture(256, 1, 32, false);
}

std::shared_ptr<ITexture> Renderer_GLES::newPaletteTextureArray(int32_t layers) {
    GLuint handle = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    auto tex = std::make_shared<Texture_GLES>(256, 1, layers, false, handle);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 256, 1, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    
    return tex;
}

std::shared_ptr<ITexture> Renderer_GLES::newModelTexture(int32_t width, int32_t height,
                                                              int32_t depth, bool filter) {
    return newTexture(width, height, depth, filter);
}

std::shared_ptr<ITexture> Renderer_GLES::newDataTexture(int32_t width, int32_t height) {
    GLuint handle = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    auto tex = std::make_shared<Texture_GLES>(width, height, 32, false, handle);
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return tex;
}

std::shared_ptr<ITexture> Renderer_GLES::newHDRTexture(int32_t width, int32_t height) {
    GLuint handle = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    auto tex = std::make_shared<Texture_GLES>(width, height, 24, false, handle);
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return tex;
}

std::shared_ptr<ITexture> Renderer_GLES::newCubeMapTexture(int32_t widthHeight, bool mipmap,
                                                                int32_t lowestMipLevel) {
    GLuint handle = 0;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &handle);
    
    auto tex = std::make_shared<Texture_GLES>(widthHeight, widthHeight, 24, false, handle);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
    
    // Allocate storage for all 6 faces
    for (int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                    widthHeight, widthHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    
    if (mipmap) {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, lowestMipLevel);
    } else {
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    return tex;
}

void Renderer_GLES::SetTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) {
    if (!spriteShader || !tex) return;
    
    GLint unit = spriteShader->GetTextureUnit(name);
    if (unit >= 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        if (tex->GetDepth() > 1 || name == "pal") {
            glBindTexture(GL_TEXTURE_2D_ARRAY, tex->GetHandle());
        } else {
            glBindTexture(GL_TEXTURE_2D, tex->GetHandle());
        }
        glUniform1i(spriteShader->GetUniformLocation(name), unit);
    }
}

void Renderer_GLES::SetModelTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) {
    if (!modelShader || !tex) return;
    
    GLint unit = modelShader->GetTextureUnit(name);
    if (unit >= 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex->GetHandle());
        glUniform1i(modelShader->GetUniformLocation(name), unit);
    }
}

void Renderer_GLES::SetShadowMapTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) {
    if (!shadowMapShader || !tex) return;
    
    GLint unit = shadowMapShader->GetTextureUnit(name);
    if (unit >= 0) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex->GetHandle());
        glUniform1i(shadowMapShader->GetUniformLocation(name), unit);
    }
}

void Renderer_GLES::SetUniformI(const std::string& name, int val) {
    if (!spriteShader) return;
    GLint loc = spriteShader->GetUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, val);
}

void Renderer_GLES::SetUniformF(const std::string& name, const std::vector<float>& values) {
    if (!spriteShader) return;
    GLint loc = spriteShader->GetUniformLocation(name);
    if (loc < 0) return;
    
    switch (values.size()) {
        case 1: glUniform1f(loc, values[0]); break;
        case 2: glUniform2f(loc, values[0], values[1]); break;
        case 3: glUniform3f(loc, values[0], values[1], values[2]); break;
        case 4: glUniform4f(loc, values[0], values[1], values[2], values[3]); break;
    }
}

void Renderer_GLES::SetUniformFv(const std::string& name, const std::vector<float>& values) {
    if (!spriteShader || values.empty()) return;
    GLint loc = spriteShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniform1fv(loc, values.size(), values.data());
    }
}

void Renderer_GLES::SetUniformMatrix(const std::string& name, const std::vector<float>& value) {
    if (!spriteShader || value.size() != 16) return;
    GLint loc = spriteShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
    }
}

void Renderer_GLES::SetModelUniformI(const std::string& name, int val) {
    if (!modelShader) return;
    GLint loc = modelShader->GetUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, val);
}

void Renderer_GLES::SetModelUniformF(const std::string& name, const std::vector<float>& values) {
    if (!modelShader) return;
    GLint loc = modelShader->GetUniformLocation(name);
    if (loc < 0) return;
    
    switch (values.size()) {
        case 1: glUniform1f(loc, values[0]); break;
        case 2: glUniform2f(loc, values[0], values[1]); break;
        case 3: glUniform3f(loc, values[0], values[1], values[2]); break;
        case 4: glUniform4f(loc, values[0], values[1], values[2], values[3]); break;
    }
}

void Renderer_GLES::SetModelUniformFv(const std::string& name, const std::vector<float>& values) {
    if (!modelShader || values.empty()) return;
    GLint loc = modelShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniform1fv(loc, values.size(), values.data());
    }
}

void Renderer_GLES::SetModelUniformMatrix(const std::string& name, const std::vector<float>& value) {
    if (!modelShader || value.size() != 16) return;
    GLint loc = modelShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
    }
}

void Renderer_GLES::SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value) {
    if (!modelShader || value.size() != 9) return;
    GLint loc = modelShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniformMatrix3fv(loc, 1, GL_FALSE, value.data());
    }
}

void Renderer_GLES::SetShadowMapUniformI(const std::string& name, int val) {
    if (!shadowMapShader) return;
    GLint loc = shadowMapShader->GetUniformLocation(name);
    if (loc >= 0) glUniform1i(loc, val);
}

void Renderer_GLES::SetShadowMapUniformF(const std::string& name, const std::vector<float>& values) {
    if (!shadowMapShader) return;
    GLint loc = shadowMapShader->GetUniformLocation(name);
    if (loc < 0) return;
    
    switch (values.size()) {
        case 1: glUniform1f(loc, values[0]); break;
        case 2: glUniform2f(loc, values[0], values[1]); break;
        case 3: glUniform3f(loc, values[0], values[1], values[2]); break;
        case 4: glUniform4f(loc, values[0], values[1], values[2], values[3]); break;
    }
}

void Renderer_GLES::SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values) {
    if (!shadowMapShader || values.empty()) return;
    GLint loc = shadowMapShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniform1fv(loc, values.size(), values.data());
    }
}

void Renderer_GLES::SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value) {
    if (!shadowMapShader || value.size() != 16) return;
    GLint loc = shadowMapShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
    }
}

void Renderer_GLES::SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value) {
    if (!shadowMapShader || value.size() != 9) return;
    GLint loc = shadowMapShader->GetUniformLocation(name);
    if (loc >= 0) {
        glUniformMatrix3fv(loc, 1, GL_FALSE, value.data());
    }
}

void Renderer_GLES::SetShadowFrameTexture(uint32_t i) {
    // OpenGL ES 3.1 doesn't support this
}

void Renderer_GLES::SetShadowFrameCubeTexture(uint32_t i) {
    // OpenGL ES 3.1 doesn't support this
}

void Renderer_GLES::SetVertexData(const std::vector<float>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STREAM_DRAW);
}

void Renderer_GLES::SetVertexDataArray(const std::vector<float>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferBatch);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STREAM_DRAW);
}

void Renderer_GLES::SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values) {
    if (bufferIndex > 1) return;
    glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer[bufferIndex]);
    glBufferData(GL_ARRAY_BUFFER, values.size(), values.data(), GL_STREAM_DRAW);
}

void Renderer_GLES::SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values) {
    if (bufferIndex > 1) return;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer[bufferIndex]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, values.size() * sizeof(uint32_t), values.data(), GL_STREAM_DRAW);
}

void Renderer_GLES::RenderQuad() {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer_GLES::RenderQuadBatch(int32_t vertexCount) {
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
}

void Renderer_GLES::RenderElements(PrimitiveMode mode, int count, int offset) {
    glDrawArrays(MapPrimitiveMode(mode), offset, count);
}

void Renderer_GLES::RenderShadowMapElements(PrimitiveMode mode, int count, int offset) {
    glDrawElements(MapPrimitiveMode(mode), count, GL_UNSIGNED_INT, (void*)(offset * sizeof(uint32_t)));
}

void Renderer_GLES::RenderCubeMap(const std::shared_ptr<ITexture>& envTex, 
                                  const std::shared_ptr<ITexture>& cubeTex) {
    // TODO: Implement when panorama shader is available
}

void Renderer_GLES::RenderFilteredCubeMap(int32_t distribution, 
                                          const std::shared_ptr<ITexture>& cubeTex,
                                          const std::shared_ptr<ITexture>& filteredTex,
                                          int32_t mipmapLevel, int32_t sampleCount, float roughness) {
    // TODO: Implement when cubemap filtering shader is available
}

void Renderer_GLES::RenderLUT(int32_t distribution, const std::shared_ptr<ITexture>& cubeTex,
                              const std::shared_ptr<ITexture>& lutTex, int32_t sampleCount) {
    // TODO: Implement when cubemap filtering shader is available
}

void Renderer_GLES::ReadPixels(std::vector<uint8_t>& data, int width, int height) {
    data.resize(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}

Mat4 Renderer_GLES::PerspectiveProjectionMatrix(float angle, float aspect, float near, float far) const {
    Mat4 res;
    mat4x4_perspective(res.data, angle, aspect, near, far);
    return res;
}

Mat4 Renderer_GLES::OrthographicProjectionMatrix(float left, float right, float bottom,
                                                       float top, float near, float far) const {
    Mat4 res;
    mat4x4_ortho(res.data, left, right, bottom, top, near, far);
    return res;
}

bool Renderer_GLES::NewWorkerThread() {
    return false;
}

void Renderer_GLES::SetVSync() {
    // VSync is handled by the window system
}

// Private helper methods

uint32_t Renderer_GLES::MapBlendEquation(BlendEquation eq) const {
    switch (eq) {
        case BlendEquation::Add: return GL_FUNC_ADD;
        case BlendEquation::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
        default: return GL_FUNC_ADD;
    }
}

uint32_t Renderer_GLES::MapBlendFunction(BlendFunc func) const {
    switch (func) {
        case BlendFunc::One: return GL_ONE;
        case BlendFunc::Zero: return GL_ZERO;
        case BlendFunc::SrcAlpha: return GL_SRC_ALPHA;
        case BlendFunc::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFunc::DstColor: return GL_DST_COLOR;
        case BlendFunc::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
        default: return GL_ONE;
    }
}

uint32_t Renderer_GLES::MapPrimitiveMode(PrimitiveMode mode) const {
    switch (mode) {
        case PrimitiveMode::Lines: return GL_LINES;
        case PrimitiveMode::LineLoop: return GL_LINE_LOOP;
        case PrimitiveMode::LineStrip: return GL_LINE_STRIP;
        case PrimitiveMode::Triangles: return GL_TRIANGLES;
        case PrimitiveMode::TriangleStrip: return GL_TRIANGLE_STRIP;
        case PrimitiveMode::TriangleFan: return GL_TRIANGLE_FAN;
        default: return GL_TRIANGLES;
    }
}

uint32_t Renderer_GLES::compileShader(uint32_t shaderType, const std::string& src) {
    GLuint shader = glCreateShader(shaderType);
    const char* srcPtr = src.c_str();
    GLint length = static_cast<GLint>(src.length());
    
    glShaderSource(shader, 1, &srcPtr, &length);
    glCompileShader(shader);
    
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetShaderInfoLog(shader, logLength, nullptr, log.data());
            std::cerr << "Shader compilation error: " << log.data() << std::endl;
        }
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

uint32_t Renderer_GLES::linkProgram(const std::vector<uint32_t>& shaders) {
    GLuint program = glCreateProgram();
    
    for (uint32_t shader : shaders) {
        glAttachShader(program, shader);
    }
    
    glLinkProgram(program);
    
    // Mark shaders for deletion
    for (uint32_t shader : shaders) {
        glDeleteShader(shader);
    }
    
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0) {
            std::vector<char> log(logLength);
            glGetProgramInfoLog(program, logLength, nullptr, log.data());
            std::cerr << "Program link error: " << log.data() << std::endl;
        }
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

std::shared_ptr<ShaderProgram_GLES> Renderer_GLES::newShaderProgram(const std::string& vert,
                                                                     const std::string& frag,
                                                                     const std::string& geo,
                                                                     const std::string& id,
                                                                     bool crashWhenFail) {
    std::vector<uint32_t> shaders;
    
    uint32_t vertShader = compileShader(GL_VERTEX_SHADER, vert);
    if (vertShader == 0) {
        if (crashWhenFail) {
            std::cerr << "Failed to compile vertex shader: " << id << std::endl;
        }
        return nullptr;
    }
    shaders.push_back(vertShader);
    
    uint32_t fragShader = compileShader(GL_FRAGMENT_SHADER, frag);
    if (fragShader == 0) {
        glDeleteShader(vertShader);
        if (crashWhenFail) {
            std::cerr << "Failed to compile fragment shader: " << id << std::endl;
        }
        return nullptr;
    }
    shaders.push_back(fragShader);
    
    // Note: OpenGL ES 3.1 doesn't support geometry shaders
    
    uint32_t program = linkProgram(shaders);
    if (program == 0) {
        if (crashWhenFail) {
            std::cerr << "Failed to link shader program: " << id << std::endl;
        }
        return nullptr;
    }
    
    auto shader = std::make_shared<ShaderProgram_GLES>();
    shader->program = program;
    
    return shader;
}

void Renderer_GLES::PrintGLESInfo() {
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* vendor = glGetString(GL_VENDOR);
    
    printf("OpenGL ES Version: %s\n", version);
    printf("GLSL Version: %s\n", glslVersion);
    printf("Renderer: %s\n", renderer);
    printf("Vendor: %s\n", vendor);
}

void Renderer_GLES::DetectCapabilities() {
    // OpenGL ES 3.1 has most features built-in
    capabilities.hasTextureArrays = true;
    capabilities.hasSamplerObjects = true;
    capabilities.hasComputeShaders = true;
    capabilities.hasIndirectDispatch = true;
    capabilities.hasShaderImageLoadStore = true;
    
    // Check for optional extensions
    capabilities.hasInstancedArrays = IsGLESExtensionSupported("GL_EXT_instanced_arrays");
}

bool Renderer_GLES::IsGLESExtensionSupported(const std::string& extension) {
    const GLubyte* extensions = glGetString(GL_EXTENSIONS);
    if (!extensions) return false;
    
    std::string extStr(reinterpret_cast<const char*>(extensions));
    return extStr.find(extension) != std::string::npos;
}

void Renderer_GLES::PrintGLESCapabilities() {
    std::cout << "OpenGL ES Capabilities:" << std::endl;
    std::cout << "  Instanced Arrays: " << (capabilities.hasInstancedArrays ? "Yes" : "No") << std::endl;
    std::cout << "  Texture Arrays: " << (capabilities.hasTextureArrays ? "Yes" : "No") << std::endl;
    std::cout << "  Sampler Objects: " << (capabilities.hasSamplerObjects ? "Yes" : "No") << std::endl;
    std::cout << "  Compute Shaders: " << (capabilities.hasComputeShaders ? "Yes" : "No") << std::endl;
    std::cout << "  Indirect Dispatch: " << (capabilities.hasIndirectDispatch ? "Yes" : "No") << std::endl;
    std::cout << "  Shader Image Load/Store: " << (capabilities.hasShaderImageLoadStore ? "Yes" : "No") << std::endl;
}

bool Renderer_GLES::InitFramebuffer(uint32_t& fbo, uint32_t& texture, uint32_t& rbo,
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
        // Note: MSAA support is limited in GLES 3.1 without extensions
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaLevel,
                                        GL_DEPTH_COMPONENT16, width, height);
    } else {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
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

bool Renderer_GLES::InitShadowFramebuffer() {
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
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT16,
                    shadowMapSize, shadowMapSize, 0,
                    GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, nullptr);
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

bool Renderer_GLES::InitPostProcessingFramebuffers() {
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

void Renderer_GLES::CacheRenderState() {
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

void Renderer_GLES::RestoreRenderState() {
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

void Renderer_GLES::ResetRenderState() {
    // Reset to default OpenGL ES state
    
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

uint32_t Renderer_GLES::MapTextureTarget(const std::shared_ptr<ITexture>& tex) const {
    if (!tex) return GL_TEXTURE_2D;
    
    // Determine texture target based on depth (layer count)
    if (tex->GetDepth() > 1) {
        return GL_TEXTURE_2D_ARRAY;
    }
    
    return GL_TEXTURE_2D;
}

void Renderer_GLES::SetupVertexAttributes(const std::shared_ptr<ShaderProgram_GLES>& shader,
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
