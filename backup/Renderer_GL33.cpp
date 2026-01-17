#include "Renderer_GL33.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>

// Define STB_IMAGE_WRITE_IMPLEMENTATION in one source file in your project if you use SavePNG
// #define STB_IMAGE_WRITE_IMPLEMENTATION 
// #include "stb_image_write.h" 

// Placeholder macros for shader strings as they weren't provided in the prompt
// In a real application, these should be loaded from files or string literals
const char* VERT_SHADER_SRC = "#version 330 core\nlayout(location=0) in vec2 position; ..."; 
const char* FRAG_SHADER_SRC = "#version 330 core\nout vec4 FragColor; ...";
// ... (Define other shader sources similarly)

// ------------------------------------------------------------------
// ShaderProgram_GL Implementation
// ------------------------------------------------------------------

ShaderProgram_GL::~ShaderProgram_GL() {
    if (program != 0) {
        glDeleteProgram(program);
    }
}

bool ShaderProgram_GL::Load(const std::string& vertSrc, const std::string& fragSrc, const std::string& geoSrc, const std::string& id) {
    auto compile = [](GLenum type, const std::string& src, const std::string& debugId) -> GLuint {
        GLuint shader = glCreateShader(type);
        const char* c_str = src.c_str();
        glShaderSource(shader, 1, &c_str, nullptr);
        glCompileShader(shader);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "Shader Compilation Error (" << debugId << "): " << infoLog << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    };

    GLuint v = compile(GL_VERTEX_SHADER, vertSrc, id + "_VERT");
    GLuint f = compile(GL_FRAGMENT_SHADER, fragSrc, id + "_FRAG");
    GLuint g = 0;
    if (!geoSrc.empty()) {
        g = compile(GL_GEOMETRY_SHADER, geoSrc, id + "_GEO");
    }

    if (!v || !f || (!geoSrc.empty() && !g)) return false;

    program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    if (g) glAttachShader(program, g);

    if (g) {
        // Geometry shader params logic
        glProgramParameteri(program, GL_GEOMETRY_INPUT_TYPE, GL_TRIANGLES);
        glProgramParameteri(program, GL_GEOMETRY_OUTPUT_TYPE, GL_TRIANGLE_STRIP);
        glProgramParameteri(program, GL_GEOMETRY_VERTICES_OUT, 18); // 3*6
    }

    glLinkProgram(program);
    
    glDeleteShader(v);
    glDeleteShader(f);
    if (g) glDeleteShader(g);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Program Link Error (" << id << "): " << infoLog << std::endl;
        return false;
    }
    return true;
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
        textureUnits[name] = (GLint)textureUnits.size(); 
    }
}

void ShaderProgram_GL::Use() const {
    glUseProgram(program);
}

GLint ShaderProgram_GL::GetLoc(const std::string& name) {
    auto it = uniforms.find(name);
    if (it != uniforms.end()) return it->second;
    return -1;
}

// ------------------------------------------------------------------
// Texture_GL33 Implementation
// ------------------------------------------------------------------

Texture_GL33::Texture_GL33(int32_t w, int32_t h, int32_t d, bool f)
    : width(w), height(h), depth(d), filter(f) {
    glGenTextures(1, &handle);
}

Texture_GL33::~Texture_GL33() {
    if (handle) glDeleteTextures(1, &handle);
}

GLenum Texture_GL33::MapInternalFormat(int32_t d) {
    if (d == 8) return GL_RED;
    if (d == 24) return GL_RGB;
    if (d == 32) return GL_RGBA;
    if (d == 96) return GL_RGB32F;
    if (d == 128) return GL_RGBA32F;
    return GL_RGBA;
}

GLint Texture_GL33::MapSamplingParam(TextureSamplingParam param) {
    switch (param) {
        case TextureSamplingParam::Nearest: return GL_NEAREST;
        case TextureSamplingParam::Linear: return GL_LINEAR;
        case TextureSamplingParam::NearestMipMapNearest: return GL_NEAREST_MIPMAP_NEAREST;
        case TextureSamplingParam::LinearMipMapNearest: return GL_LINEAR_MIPMAP_NEAREST;
        case TextureSamplingParam::NearestMipMapLinear: return GL_NEAREST_MIPMAP_LINEAR;
        case TextureSamplingParam::LinearMipMapLinear: return GL_LINEAR_MIPMAP_LINEAR;
        case TextureSamplingParam::ClampToEdge: return GL_CLAMP_TO_EDGE;
        case TextureSamplingParam::MirroredRepeat: return GL_MIRRORED_REPEAT;
        case TextureSamplingParam::Repeat: return GL_REPEAT;
        default: return GL_NEAREST;
    }
}

void Texture_GL33::SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) {
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, 256, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}

void Texture_GL33::SetData(const std::vector<uint8_t>& data) {
    GLint interp = filter ? GL_LINEAR : GL_NEAREST;
    GLenum format = MapInternalFormat(std::max(depth, 8));
    
    glBindTexture(target, handle);
    glFinish(); 
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    const void* ptr = data.empty() ? nullptr : data.data();
    glTexImage2D(target, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, ptr);
    
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, interp);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, interp);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void Texture_GL33::SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, int32_t w, int32_t h) {
    GLenum format = MapInternalFormat(std::max(depth, 8));
    glBindTexture(target, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    const void* ptr = data.empty() ? nullptr : data.data();
    glTexSubImage2D(target, 0, x, y, w, h, format, GL_UNSIGNED_BYTE, ptr);
}

void Texture_GL33::SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag, TextureSamplingParam min, TextureSamplingParam ws, TextureSamplingParam wt) {
    GLenum format = MapInternalFormat(std::max(depth, 8));
    glBindTexture(target, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(target, 0, (GLint)format, width, height, 0, format, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(target);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, MapSamplingParam(mag));
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, MapSamplingParam(min));
    glTexParameteri(target, GL_TEXTURE_WRAP_S, MapSamplingParam(ws));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, MapSamplingParam(wt));
}

void Texture_GL33::SetPixelData(const std::vector<float>& data) {
    GLenum format = MapInternalFormat(std::max(depth / 4, 8)); // Format
    GLenum internalFormat = MapInternalFormat(std::max(depth, 8)); // Internal
    glBindTexture(target, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(target, 0, (GLint)internalFormat, width, height, 0, format, GL_FLOAT, data.data());
}

void Texture_GL33::CopyData(Texture* src) {
    Texture_GL33* s = dynamic_cast<Texture_GL33*>(src);
    if (!s || !s->IsValid() || !IsValid()) return;
    
    // Assuming identical dimensions
    glBindTexture(GL_TEXTURE_2D, s->handle);
    size_t pixelSize = width * height * (depth / 8);
    std::vector<uint8_t> buffer(pixelSize);
    
    // Read from source
    glGetTexImage(GL_TEXTURE_2D, 0, s->MapInternalFormat(std::max(s->depth, 8)), GL_UNSIGNED_BYTE, buffer.data());
    
    // Write to dest
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, MapInternalFormat(std::max(depth, 8)), GL_UNSIGNED_BYTE, buffer.data());
}

bool Texture_GL33::IsValid() {
    return width != 0 && height != 0 && handle != 0;
}

int32_t Texture_GL33::GetWidth() { return width; }
int32_t Texture_GL33::GetHeight() { return height; }

void Texture_GL33::SavePNG(const std::string& filename, const std::vector<uint32_t>& pal) {
    // Basic implementation using hypothetical stb_image_write
    if (!IsValid()) return;

    glBindTexture(GL_TEXTURE_2D, handle);
    size_t pixelSize = width * height * (depth / 8);
    std::vector<uint8_t> data(pixelSize);
    
    GLenum format = MapInternalFormat(std::max(depth, 8));
    glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, data.data());

    std::cout << "Saving PNG to " << filename << " (Implementation requires stbi_write_png)" << std::endl;
}

// ------------------------------------------------------------------
// Renderer_GL33 Implementation
// ------------------------------------------------------------------

Renderer_GL33::Renderer_GL33() {
    scrWidth = 640;
    scrHeight = 480; 
    msaa = 0; 
}

Renderer_GL33::~Renderer_GL33() {
    Close();
}

std::string Renderer_GL33::GetName() {
    return "OpenGL 3.3 Core";
}

void Renderer_GL33::InitModelShader() {
    // Placeholder logic - Go code dynamically creates shaders.
    // In C++, you'd load from files. This mimics the Register calls.
    modelShader = new ShaderProgram_GL();
    // modelShader->Load(..., "ModelShader");
    
    modelShader->RegisterAttributes({"vertexId", "position", "uv", "normalIn", "tangentIn", "vertColor", "joints_0", "joints_1", "weights_0", "weights_1", "outlineAttributeIn"});
    modelShader->RegisterUniforms({"model", "view", "projection", "normalMatrix", "unlit", "baseColorFactor", "add", "mult", "useTexture", "useNormalMap", "useMetallicRoughnessMap", "useEmissionMap", "neg", "gray", "hue", "enableAlpha", "alphaThreshold", "numJoints", "morphTargetWeight", "morphTargetOffset", "morphTargetTextureDimension", "numTargets", "numVertices", "metallicRoughness", "ambientOcclusionStrength", "emission", "environmentIntensity", "mipCount", "meshOutline", "cameraPosition", "environmentRotation", "texTransform", "normalMapTransform", "metallicRoughnessMapTransform", "ambientOcclusionMapTransform", "emissionMapTransform"});
    // ... register lights array uniforms ...
    modelShader->RegisterTextures({"tex", "morphTargetValues", "jointMatrices", "normalMap", "metallicRoughnessMap", "ambientOcclusionMap", "emissionMap", "lambertianEnvSampler", "GGXEnvSampler", "GGXLUT", "shadowCubeMap"});

    if (enableShadow) {
        shadowMapShader = new ShaderProgram_GL();
        // shadowMapShader->Load(..., "ShadowMapShader");
        shadowMapShader->RegisterAttributes({"vertexId", "position", "vertColor", "uv", "joints_0", "joints_1", "weights_0", "weights_1"});
        shadowMapShader->RegisterUniforms({"model", "numJoints", "morphTargetWeight", "morphTargetOffset", "morphTargetTextureDimension", "numTargets", "numVertices", "enableAlpha", "alphaThreshold", "baseColorFactor", "useTexture", "texTransform", "layerOffset", "lightIndex"});
        // ... register light matrices ...
        shadowMapShader->RegisterTextures({"morphTargetValues", "jointMatrices", "tex"});
    }
}

void Renderer_GL33::Init() {
    // Buffers
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Quad for post-processing
    float postVertData[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    glGenBuffers(1, &postVertBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, postVertBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(postVertData), postVertData, GL_STATIC_DRAW);

    // Main vertex buffers
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(2, modelVertexBuffer);
    glGenBuffers(2, modelIndexBuffer);
    glGenBuffers(1, &vertexBufferBatch);

    // Create Shaders
    spriteShader = new ShaderProgram_GL();
    // spriteShader->Load(..., "SpriteShader");
    spriteShader->RegisterAttributes({"position", "uv", "palIndex"});
    spriteShader->RegisterUniforms({"modelview", "projection", "x1x2x4x3", "alpha", "tint", "mask", "neg", "gray", "add", "mult", "isFlat", "isRgba", "isTrapez", "hue", "uvRect", "useUV"});
    spriteShader->RegisterTextures({"pal", "tex"});

    if (enableModel) {
        InitModelShader();
    }
    
    // FBO Setup
    glGenTextures(1, &fbo_texture);
    GLenum texTarget = (msaa > 0) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    glBindTexture(texTarget, fbo_texture);
    
    if (msaa > 0) {
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, msaa, GL_RGBA, scrWidth, scrHeight, GL_TRUE);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, scrWidth, scrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    // Ping-pong FBOs
    fbo_pp.resize(2);
    fbo_pp_texture.resize(2);
    for(int i=0; i<2; ++i) {
        glGenFramebuffers(1, &fbo_pp[i]);
        glGenTextures(1, &fbo_pp_texture[i]);
        glBindTexture(GL_TEXTURE_2D, fbo_pp_texture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8_SNORM, scrWidth, scrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_pp[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_pp_texture[i], 0);
    }

    // Main FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texTarget, fbo_texture, 0);
    
    // Depth
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    if(msaa > 0) glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, GL_DEPTH_COMPONENT16, scrWidth, scrHeight);
    else glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, scrWidth, scrHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);

    // Resolve FBO for MSAA
    if (msaa > 0) {
        fbo_f_texture = (Texture_GL33*)newTexture(scrWidth, scrHeight, 32, false);
        fbo_f_texture->SetData({}); 
        glGenFramebuffers(1, &fbo_f);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_f);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_f_texture->handle, 0);
    }

    if (enableModel && enableShadow) {
        glGenFramebuffers(1, &fbo_shadow);
        glGenTextures(1, &fbo_shadow_cube_texture);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, fbo_shadow_cube_texture);
        glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, GL_DEPTH_COMPONENT24, 1024, 1024, 4*6); // 4 lights * 6 faces
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer_GL33::Close() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteFramebuffers(1, &fbo);
    delete spriteShader;
}

// ------------------------------------------------------------------
// Frame Control
// ------------------------------------------------------------------

void Renderer_GL33::BeginFrame(bool clearColor) {
    glBindVertexArray(vao);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, scrWidth, scrHeight);
    
    GLbitfield mask = GL_DEPTH_BUFFER_BIT;
    if (clearColor) mask |= GL_COLOR_BUFFER_BIT;
    glClear(mask);
}

void Renderer_GL33::EndFrame() {
    glBindVertexArray(vao);
    
    if (msaa > 0) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_f);
        glBlitFramebuffer(0, 0, scrWidth, scrHeight, 0, 0, scrWidth, scrHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    // Simply blit to screen (0) for now, mimicking Go code's final step
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (msaa > 0 ? fbo_f : fbo));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, scrWidth, scrHeight, 0, 0, scrWidth, scrHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    nDrawCall = 0;
}

void Renderer_GL33::Await() {
    glFinish();
}

// ------------------------------------------------------------------
// Factory Methods
// ------------------------------------------------------------------

Texture* Renderer_GL33::newTexture(int32_t width, int32_t height, int32_t depth, bool filter) {
    return new Texture_GL33(width, height, depth, filter);
}

Texture* Renderer_GL33::newPaletteTexture() {
    return newTexture(256, 1, 32, false);
}

Texture* Renderer_GL33::newPaletteTextureArray(int32_t layers) {
    Texture_GL33* t = new Texture_GL33(256, 1, layers, false);
    t->target = GL_TEXTURE_2D_ARRAY;
    glBindTexture(GL_TEXTURE_2D_ARRAY, t->handle);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 256, 1, layers, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

Texture* Renderer_GL33::newModelTexture(int32_t width, int32_t height, int32_t depth, bool filter) {
    return newTexture(width, height, depth, filter);
}

Texture* Renderer_GL33::newDataTexture(int32_t width, int32_t height) {
    Texture_GL33* t = new Texture_GL33(width, height, 128, false); // RGBA32F
    glBindTexture(GL_TEXTURE_2D, t->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}

Texture* Renderer_GL33::newHDRTexture(int32_t width, int32_t height) {
    Texture_GL33* t = new Texture_GL33(width, height, 96, false); // RGB32F
    glBindTexture(GL_TEXTURE_2D, t->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    return t;
}

Texture* Renderer_GL33::newCubeMapTexture(int32_t widthHeight, bool mipmap, int32_t lowestMipLevel) {
    Texture_GL33* t = new Texture_GL33(widthHeight, widthHeight, 24, false);
    t->target = GL_TEXTURE_CUBE_MAP;
    glBindTexture(GL_TEXTURE_CUBE_MAP, t->handle);
    for(int i=0; i<6; ++i) {
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
    return t;
}

// ------------------------------------------------------------------
// Pipeline
// ------------------------------------------------------------------

void Renderer_GL33::SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) {
    glBindVertexArray(vao);
    spriteShader->Use();

    SetBlending(eq, src, dst);
    glEnable(GL_BLEND);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    const GLsizei stride = 20; // 5 floats * 4 bytes
    
    // Position (Offset 0)
    GLint loc = spriteShader->attributes["position"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);

    // UV (Offset 8)
    loc = spriteShader->attributes["uv"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));

    // PalIndex (Offset 16)
    loc = spriteShader->attributes["palIndex"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
}

void Renderer_GL33::SetPipelineBatch() {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferBatch);
    const GLsizei stride = 20; 

    GLint loc = spriteShader->attributes["position"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);

    loc = spriteShader->attributes["uv"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2 * sizeof(float)));

    loc = spriteShader->attributes["palIndex"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
}

void Renderer_GL33::ReleasePipeline() {
    GLint loc = spriteShader->attributes["position"];
    glDisableVertexAttribArray(loc);
    loc = spriteShader->attributes["uv"];
    glDisableVertexAttribArray(loc);
    loc = spriteShader->attributes["palIndex"];
    glDisableVertexAttribArray(loc);
    glDisable(GL_BLEND);
}

void Renderer_GL33::SetBlending(BlendEquation eq, BlendFunc src, BlendFunc dst) {
    if (eq != state.blendEquation) {
        state.blendEquation = eq;
        glBlendEquation(MapBlendEquation(eq));
    }
    if (src != state.blendSrc || dst != state.blendDst) {
        state.blendSrc = src;
        state.blendDst = dst;
        glBlendFunc(MapBlendFunc(src), MapBlendFunc(dst));
    }
}

// ------------------------------------------------------------------
// Shadow & Model Pipelines
// ------------------------------------------------------------------

void Renderer_GL33::prepareShadowMapPipeline(uint32_t bufferIndex) {
    shadowMapShader->Use();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_shadow);
    glViewport(0, 0, 1024, 1024);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ZERO);

    if (state.invertFrontFace) glFrontFace(GL_CW); else glFrontFace(GL_CCW);
    if (!state.doubleSided) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); } else glDisable(GL_CULL_FACE);

    state.depthTest = true;
    state.depthMask = true;
    state.blendEquation = BlendEquation::Add;
    state.blendSrc = BlendFunc::One;
    state.blendDst = BlendFunc::Zero;

    glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer[bufferIndex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer[bufferIndex]);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_shadow_cube_texture, 0);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer_GL33::setShadowMapPipeline(bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, uint32_t numVertices, uint32_t vertAttrOffset) {
    SetFrontFace(invertFrontFace);
    SetCullFace(doubleSided);

    size_t offset = vertAttrOffset + 4 * numVertices; // Skip int vertexId (assuming stride logic)
    
    // VertexID is an int, but glVertexAttribPointer uses offset in bytes.
    // In Go logic, offsets are calculated manually. 
    // Assuming packed buffer: [VertexID(int)][Position(vec3)][UV(vec2)]...
    
    GLint loc = shadowMapShader->attributes["vertexId"];
    glEnableVertexAttribArray(loc);
    glVertexAttribIPointer(loc, 1, GL_INT, 0, (void*)(uintptr_t)vertAttrOffset);
    
    loc = shadowMapShader->attributes["position"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
    offset += 12 * numVertices;

    if (useUV) {
        state.useUV = true;
        loc = shadowMapShader->attributes["uv"]; // Note: Go code referenced modelShader for UV here, likely a copy-paste quirk or shared loc
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 8 * numVertices;
    } else {
        state.useUV = false;
        loc = shadowMapShader->attributes["uv"];
        glDisableVertexAttribArray(loc);
        glVertexAttrib2f(loc, 0, 0);
    }
    
    if (useNormal) offset += 12 * numVertices;
    if (useTangent) offset += 16 * numVertices;

    if (useVertColor) {
        loc = shadowMapShader->attributes["vertColor"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 16 * numVertices;
    } else {
        loc = shadowMapShader->attributes["vertColor"];
        glDisableVertexAttribArray(loc);
        glVertexAttrib4f(loc, 1, 1, 1, 1);
    }

    if (useJoint0) {
        state.useJoint0 = true;
        loc = shadowMapShader->attributes["joints_0"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 16 * numVertices;
        
        loc = shadowMapShader->attributes["weights_0"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 16 * numVertices;

        if (useJoint1) {
             state.useJoint1 = true;
             loc = shadowMapShader->attributes["joints_1"];
             glEnableVertexAttribArray(loc);
             glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
             offset += 16 * numVertices;
             
             loc = shadowMapShader->attributes["weights_1"];
             glEnableVertexAttribArray(loc);
             glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
             offset += 16 * numVertices;
        } else {
             state.useJoint1 = false;
             glDisableVertexAttribArray(shadowMapShader->attributes["joints_1"]);
             glDisableVertexAttribArray(shadowMapShader->attributes["weights_1"]);
        }
    } else {
        state.useJoint0 = false;
        state.useJoint1 = false;
        glDisableVertexAttribArray(shadowMapShader->attributes["joints_0"]);
        glDisableVertexAttribArray(shadowMapShader->attributes["weights_0"]);
        glDisableVertexAttribArray(shadowMapShader->attributes["joints_1"]);
        glDisableVertexAttribArray(shadowMapShader->attributes["weights_1"]);
    }
}

void Renderer_GL33::ReleaseShadowPipeline() {
    glDisableVertexAttribArray(shadowMapShader->attributes["vertexId"]);
    glDisableVertexAttribArray(shadowMapShader->attributes["position"]);
    glDisableVertexAttribArray(shadowMapShader->attributes["uv"]);
    glDisableVertexAttribArray(shadowMapShader->attributes["vertColor"]);
    glDisableVertexAttribArray(shadowMapShader->attributes["joints_0"]);
    glDisableVertexAttribArray(shadowMapShader->attributes["weights_0"]);
    glDisableVertexAttribArray(shadowMapShader->attributes["joints_1"]);
    glDisableVertexAttribArray(shadowMapShader->attributes["weights_1"]);

    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    state.useUV = false;
    state.useJoint0 = false;
    state.useJoint1 = false;
}

void Renderer_GL33::prepareModelPipeline(uint32_t bufferIndex, Environment* env) {
    modelShader->Use();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, scrWidth, scrHeight);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_CUBE_MAP); // Implicit in 3.3
    glEnable(GL_BLEND);

    if (state.depthTest) { glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); } else glDisable(GL_DEPTH_TEST);
    glDepthMask(state.depthMask ? GL_TRUE : GL_FALSE);
    if (state.invertFrontFace) glFrontFace(GL_CW); else glFrontFace(GL_CCW);
    if (!state.doubleSided) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); } else glDisable(GL_CULL_FACE);

    glBlendEquation(MapBlendEquation(state.blendEquation));
    glBlendFunc(MapBlendFunc(state.blendSrc), MapBlendFunc(state.blendDst));

    glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer[bufferIndex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer[bufferIndex]);

    if (enableShadow) {
        GLint unit = modelShader->textureUnits["shadowCubeMap"];
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, fbo_shadow_cube_texture);
        glUniform1i(modelShader->uniforms["shadowCubeMap"], unit);
    }
    // Environment maps setup would go here if Environment struct is defined
}

void Renderer_GL33::SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst, bool depthTest, bool depthMask, bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, bool useOutlineAttribute, uint32_t numVertices, uint32_t vertAttrOffset) {
    SetDepthTest(depthTest);
    SetDepthMask(depthMask);
    SetFrontFace(invertFrontFace);
    SetCullFace(doubleSided);
    SetBlending(eq, src, dst);

    size_t offset = vertAttrOffset + 4 * numVertices; 
    
    GLint loc = modelShader->attributes["vertexId"];
    glEnableVertexAttribArray(loc);
    glVertexAttribIPointer(loc, 1, GL_INT, 0, (void*)(uintptr_t)vertAttrOffset);

    loc = modelShader->attributes["position"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
    offset += 12 * numVertices;

    if (useUV) {
        state.useUV = true;
        loc = modelShader->attributes["uv"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 8 * numVertices;
    } else {
        state.useUV = false;
        loc = modelShader->attributes["uv"];
        glDisableVertexAttribArray(loc);
        glVertexAttrib2f(loc, 0, 0);
    }

    if (useNormal) {
        state.useNormal = true;
        loc = modelShader->attributes["normalIn"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 12 * numVertices;
    } else {
        state.useNormal = false;
        glDisableVertexAttribArray(modelShader->attributes["normalIn"]);
    }

    if (useTangent) {
        state.useTangent = true;
        loc = modelShader->attributes["tangentIn"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 16 * numVertices;
    } else {
        state.useTangent = false;
        glDisableVertexAttribArray(modelShader->attributes["tangentIn"]);
    }

    if (useVertColor) {
        state.useVertColor = true;
        loc = modelShader->attributes["vertColor"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 16 * numVertices;
    } else {
        state.useVertColor = false;
        loc = modelShader->attributes["vertColor"];
        glDisableVertexAttribArray(loc);
        glVertexAttrib4f(loc, 1, 1, 1, 1);
    }

    // Joints handling matches ShadowPipeline logic
    if (useJoint0) {
        state.useJoint0 = true;
        loc = modelShader->attributes["joints_0"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 16 * numVertices;

        loc = modelShader->attributes["weights_0"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
        offset += 16 * numVertices;
        
        if (useJoint1) {
            state.useJoint1 = true;
            loc = modelShader->attributes["joints_1"];
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
            offset += 16 * numVertices;
            loc = modelShader->attributes["weights_1"];
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
            offset += 16 * numVertices;
        } else {
            state.useJoint1 = false;
            glDisableVertexAttribArray(modelShader->attributes["joints_1"]);
            glDisableVertexAttribArray(modelShader->attributes["weights_1"]);
        }
    } else {
        state.useJoint0 = false;
        state.useJoint1 = false;
        glDisableVertexAttribArray(modelShader->attributes["joints_0"]);
        glDisableVertexAttribArray(modelShader->attributes["weights_0"]);
        glDisableVertexAttribArray(modelShader->attributes["joints_1"]);
        glDisableVertexAttribArray(modelShader->attributes["weights_1"]);
    }

    if (useOutlineAttribute) {
        state.useOutlineAttribute = true;
        loc = modelShader->attributes["outlineAttributeIn"];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, 0, (void*)(uintptr_t)offset);
    } else {
        state.useOutlineAttribute = false;
        glDisableVertexAttribArray(modelShader->attributes["outlineAttributeIn"]);
    }
}

void Renderer_GL33::SetMeshOulinePipeline(bool invertFrontFace, float meshOutline) {
    SetFrontFace(invertFrontFace);
    SetDepthTest(true);
    SetDepthMask(true);
    
    GLint loc = modelShader->uniforms["meshOutline"];
    glUniform1f(loc, meshOutline);
}

void Renderer_GL33::ReleaseModelPipeline() {
    glDisableVertexAttribArray(modelShader->attributes["vertexId"]);
    glDisableVertexAttribArray(modelShader->attributes["position"]);
    glDisableVertexAttribArray(modelShader->attributes["uv"]);
    glDisableVertexAttribArray(modelShader->attributes["normalIn"]);
    glDisableVertexAttribArray(modelShader->attributes["tangentIn"]);
    glDisableVertexAttribArray(modelShader->attributes["vertColor"]);
    glDisableVertexAttribArray(modelShader->attributes["joints_0"]);
    glDisableVertexAttribArray(modelShader->attributes["weights_0"]);
    glDisableVertexAttribArray(modelShader->attributes["joints_1"]);
    glDisableVertexAttribArray(modelShader->attributes["weights_1"]);
    glDisableVertexAttribArray(modelShader->attributes["outlineAttributeIn"]);
    
    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    state.useUV = false;
    state.useNormal = false;
    state.useTangent = false;
    state.useVertColor = false;
    state.useJoint0 = false;
    state.useJoint1 = false;
    state.useOutlineAttribute = false;
}

// ------------------------------------------------------------------
// Utils
// ------------------------------------------------------------------

void Renderer_GL33::SetVertexData(const std::vector<float>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL33::SetVertexDataArray(const std::vector<float>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferBatch);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL33::SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, modelVertexBuffer[bufferIndex]);
    glBufferData(GL_ARRAY_BUFFER, values.size(), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL33::SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIndexBuffer[bufferIndex]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, values.size() * sizeof(uint32_t), values.data(), GL_STATIC_DRAW);
}

void Renderer_GL33::RenderQuad() {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    nDrawCall++;
}

void Renderer_GL33::RenderQuadBatch(int32_t vertexCount) {
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    nDrawCall++;
}

void Renderer_GL33::RenderElements(PrimitiveMode mode, int count, int offset) {
    glDrawElements(MapPrimitiveMode(mode), count, GL_UNSIGNED_INT, (void*)(uintptr_t)offset);
    nDrawCall++;
}

void Renderer_GL33::RenderShadowMapElements(PrimitiveMode mode, int count, int offset) {
    RenderElements(mode, count, offset);
}

void Renderer_GL33::SetUniformI(const std::string& name, int val) {
    glUniform1i(spriteShader->uniforms[name], val);
}

void Renderer_GL33::SetUniformMatrix(const std::string& name, const std::vector<float>& value) {
    glUniformMatrix4fv(spriteShader->uniforms[name], 1, GL_FALSE, value.data());
}

void Renderer_GL33::SetTexture(const std::string& name, Texture* tex) {
    if (!tex || !tex->IsValid()) return;
    Texture_GL33* t = static_cast<Texture_GL33*>(tex);
    
    GLint unit = spriteShader->textureUnits[name];
    glActiveTexture(GL_TEXTURE0 + unit);
    
    if (name == "pal" || (t->depth > 1 && t->height == 1)) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, t->handle);
    } else {
        glBindTexture(GL_TEXTURE_2D, t->handle);
    }
    glUniform1i(spriteShader->uniforms[name], unit);
}

// ------------------------------------------------------------------
// State Helpers
// ------------------------------------------------------------------

GLenum Renderer_GL33::MapBlendEquation(BlendEquation eq) {
    return (eq == BlendEquation::Add) ? GL_FUNC_ADD : GL_FUNC_REVERSE_SUBTRACT;
}

GLenum Renderer_GL33::MapBlendFunc(BlendFunc func) {
    switch(func) {
        case BlendFunc::One: return GL_ONE;
        case BlendFunc::Zero: return GL_ZERO;
        case BlendFunc::SrcAlpha: return GL_SRC_ALPHA;
        case BlendFunc::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFunc::DstColor: return GL_DST_COLOR;
        case BlendFunc::OneMinusDstColor: return GL_ONE_MINUS_DST_COLOR;
        default: return GL_ONE;
    }
}

GLenum Renderer_GL33::MapPrimitiveMode(PrimitiveMode mode) {
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

void Renderer_GL33::SetDepthTest(bool enable) {
    if (enable != state.depthTest) {
        state.depthTest = enable;
        if (enable) { glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LESS); }
        else glDisable(GL_DEPTH_TEST);
    }
}

void Renderer_GL33::SetDepthMask(bool mask) {
    if (mask != state.depthMask) {
        state.depthMask = mask;
        glDepthMask(mask ? GL_TRUE : GL_FALSE);
    }
}

void Renderer_GL33::SetFrontFace(bool invert) {
    if (invert != state.invertFrontFace) {
        state.invertFrontFace = invert;
        if (invert) glFrontFace(GL_CW); else glFrontFace(GL_CCW);
    }
}

void Renderer_GL33::SetCullFace(bool doubleSided) {
    if (doubleSided != state.doubleSided) {
        state.doubleSided = doubleSided;
        if (!doubleSided) { glEnable(GL_CULL_FACE); glCullFace(GL_BACK); }
        else glDisable(GL_CULL_FACE);
    }
}

// ------------------------------------------------------------------
// Math Implementation using linmath.h
// ------------------------------------------------------------------

Mat4 Renderer_GL33::PerspectiveProjectionMatrix(float angle, float aspect, float near, float far) {
    Mat4 res;
    mat4x4_perspective(res.data, angle, aspect, near, far);
    return res;
}

Mat4 Renderer_GL33::OrthographicProjectionMatrix(float left, float right, float bottom, float top, float near, float far) {
    Mat4 res;
    mat4x4_ortho(res.data, left, right, bottom, top, near, far);
    return res;
}

void Renderer_GL33::SetUniformMatrix(const std::string& name, const Mat4& mat) {
    GLint loc = spriteShader->GetLoc(name);
    if (loc != -1) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, (const GLfloat*)mat.data);
    }
}

// Stubs for Model Uniforms (Implement properly if model rendering is needed)
void Renderer_GL33::SetModelUniformI(const std::string& name, int val) { glUniform1i(modelShader->uniforms[name], val); }
void Renderer_GL33::SetModelUniformF(const std::string& name, float v) { glUniform1f(modelShader->uniforms[name], v); }
void Renderer_GL33::SetModelUniformF(const std::string& name, float v1, float v2) { glUniform2f(modelShader->uniforms[name], v1, v2); }
void Renderer_GL33::SetModelUniformF(const std::string& name, float v1, float v2, float v3) { glUniform3f(modelShader->uniforms[name], v1, v2, v3); }
void Renderer_GL33::SetModelUniformF(const std::string& name, float v1, float v2, float v3, float v4) { glUniform4f(modelShader->uniforms[name], v1, v2, v3, v4); }
void Renderer_GL33::SetModelUniformFv(const std::string& name, const std::vector<float>& values) { glUniform1fv(modelShader->uniforms[name], values.size(), values.data()); }
void Renderer_GL33::SetModelUniformMatrix(const std::string& name, const std::vector<float>& value) { glUniformMatrix4fv(modelShader->uniforms[name], 1, GL_FALSE, value.data()); }
void Renderer_GL33::SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value) { glUniformMatrix3fv(modelShader->uniforms[name], 1, GL_FALSE, value.data()); }
void Renderer_GL33::SetModelTexture(const std::string& name, Texture* t) { 
    GLint unit = modelShader->textureUnits[name];
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, static_cast<Texture_GL33*>(t)->handle);
    glUniform1i(modelShader->uniforms[name], unit);
}

// Stubs for Shadow Map Uniforms
void Renderer_GL33::SetShadowMapUniformI(const std::string& name, int val) { glUniform1i(shadowMapShader->uniforms[name], val); }
void Renderer_GL33::SetShadowMapUniformF(const std::string& name, float v) { glUniform1f(shadowMapShader->uniforms[name], v); }
void Renderer_GL33::SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values) { glUniform1fv(shadowMapShader->uniforms[name], values.size(), values.data()); }
void Renderer_GL33::SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value) { glUniformMatrix4fv(shadowMapShader->uniforms[name], 1, GL_FALSE, value.data()); }
void Renderer_GL33::SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value) { glUniformMatrix3fv(shadowMapShader->uniforms[name], 1, GL_FALSE, value.data()); }
void Renderer_GL33::SetShadowMapTexture(const std::string& name, Texture* t) {
    GLint unit = shadowMapShader->textureUnits[name];
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, static_cast<Texture_GL33*>(t)->handle);
    glUniform1i(shadowMapShader->uniforms[name], unit);
}
void Renderer_GL33::SetShadowFrameTexture(uint32_t) { glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_shadow_cube_texture, 0); }
void Renderer_GL33::SetShadowFrameCubeTexture(uint32_t) { glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_shadow_cube_texture, 0); }

// Missing Implementations
bool Renderer_GL33::IsModelEnabled() { return enableModel; }
bool Renderer_GL33::IsShadowEnabled() { return enableShadow; }
bool Renderer_GL33::NewWorkerThread() { return false; }
void Renderer_GL33::SetVSync() {}
void Renderer_GL33::BlendReset() { SetBlending(BlendEquation::Add, BlendFunc::SrcAlpha, BlendFunc::OneMinusSrcAlpha); }
void Renderer_GL33::ReadPixels(std::vector<uint8_t>& data, int width, int height) {
    data.resize(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}
void Renderer_GL33::Scissor(int32_t x, int32_t y, int32_t width, int32_t height) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, scrHeight - (y + height), width, height);
}
void Renderer_GL33::DisableScissor() { glDisable(GL_SCISSOR_TEST); }

// Uniform setters (2D)
void Renderer_GL33::SetUniformF(const std::string& name, float v) { glUniform1f(spriteShader->uniforms[name], v); }
void Renderer_GL33::SetUniformF(const std::string& name, float v1, float v2) { glUniform2f(spriteShader->uniforms[name], v1, v2); }
void Renderer_GL33::SetUniformF(const std::string& name, float v1, float v2, float v3) { glUniform3f(spriteShader->uniforms[name], v1, v2, v3); }
void Renderer_GL33::SetUniformF(const std::string& name, float v1, float v2, float v3, float v4) { glUniform4f(spriteShader->uniforms[name], v1, v2, v3, v4); }
void Renderer_GL33::SetUniformFv(const std::string& name, const std::vector<float>& values) { glUniform1fv(spriteShader->uniforms[name], values.size(), values.data()); }

void Renderer_GL33::RenderCubeMap(Texture* envTexture, Texture* cubeTexture) {
    // Implementation requires panoramaToCubeMapShader
}
void Renderer_GL33::RenderFilteredCubeMap(int32_t distribution, Texture* cubeTexture, Texture* filteredTexture, int32_t mipmapLevel, int32_t sampleCount, float roughness) {
    // Implementation requires cubemapFilteringShader
}
void Renderer_GL33::RenderLUT(int32_t distribution, Texture* cubeTexture, Texture* lutTexture, int32_t sampleCount) {
    // Implementation requires cubemapFilteringShader
}