#include "Renderer_GLES31.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <iostream>

// ------------------------------------------------------------------
// ShaderProgram_GLES31
// ------------------------------------------------------------------

ShaderProgram_GLES31::ShaderProgram_GLES31() : program(0) {}

ShaderProgram_GLES31::~ShaderProgram_GLES31() {
    if (program) {
        glDeleteProgram(program);
    }
}

void ShaderProgram_GLES31::RegisterAttributes(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        a[name] = glGetAttribLocation(program, name.c_str());
    }
}

void ShaderProgram_GLES31::RegisterUniforms(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        u[name] = glGetUniformLocation(program, name.c_str());
    }
}

void ShaderProgram_GLES31::RegisterTextures(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        u[name] = glGetUniformLocation(program, name.c_str());
        t[name] = static_cast<int>(t.size());
    }
}

// ------------------------------------------------------------------
// Texture_GLES31
// ------------------------------------------------------------------

Texture_GLES31::Texture_GLES31(int32_t w, int32_t h, int32_t d, bool f, GLuint hdl) 
    : width(w), height(h), depth(d), filter(f), handle(hdl) {}

Texture_GLES31::~Texture_GLES31() {
    if (handle) {
        glDeleteTextures(1, &handle);
    }
}

void Texture_GLES31::SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) {
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Upload to specific Z-slice (layer)
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, 
                    256, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
}

void Texture_GLES31::SetData(const std::vector<uint8_t>& data) {
    GLint interp = filter ? GL_LINEAR : GL_NEAREST;
    GLenum format = MapInternalFormat(std::max(depth, 8));
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glFinish(); // Ensure uploads complete
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    if (!data.empty()) {
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), 
                    width, height, 0, format, GL_UNSIGNED_BYTE, data.data());
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), 
                    width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);
    }
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool Texture_GLES31::IsValid() {
    return width != 0 && height != 0 && handle != 0;
}

int32_t Texture_GLES31::GetWidth() { return width; }
int32_t Texture_GLES31::GetHeight() { return height; }

uint32_t Texture_GLES31::MapInternalFormat(int32_t i) {
    static const std::map<int32_t, uint32_t> InternalFormatLUT = {
        {8,   GL_RED},      // single-channel
        {24,  GL_RGB},      // 8 bits per RGB channel
        {32,  GL_RGBA},     // 8 bits per RGBA channel
        {96,  GL_RGB32F},   // 32-bit float per RGB channel
        {128, GL_RGBA32F},  // 32-bit float per RGBA channel
    };
    
    auto it = InternalFormatLUT.find(i);
    return it != InternalFormatLUT.end() ? it->second : GL_RGBA;
}

// ------------------------------------------------------------------
// Renderer_GLES31 - Core Implementation
// ------------------------------------------------------------------

Renderer_GLES31::Renderer_GLES31() {
    // Initialize default state
}

Renderer_GLES31::~Renderer_GLES31() {
    Close();
}

std::string Renderer_GLES31::GetName() {
    return "OpenGL ES 3.1";
}

void Renderer_GLES31::Init() {
    // Query GL version
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    std::cout << "Real GL Version: " << version << std::endl;
    std::cout << "Real GL Renderer: " << renderer << std::endl;
    
    // Query max samples for MSAA
    GLint maxSamples;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (msaa > maxSamples) {
        msaa = maxSamples;
    }
    
    // Prepare post shader slots
    postShaderSelect.resize(1); // Base identity shader
    
    // Data buffers for rendering (post-process quad)
    float postVertData[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    
    // Generate VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Generate buffers
    glGenBuffers(1, &postVertBuffer);
    glGenBuffers(1, &vertexBuffer);
    glGenBuffers(2, modelVertexBuffer);
    glGenBuffers(2, modelIndexBuffer);
    glGenBuffers(1, &vertexBufferBatch);
    
    // Upload post-process quad data
    glBindBuffer(GL_ARRAY_BUFFER, postVertBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(postVertData), postVertData, GL_STATIC_DRAW);
    
    // Create sprite shader
    try {
        spriteShader = compileShaderProgram(
            vertShaderSource,
            fragShaderSource,
            "", // No geometry shader
            "Main Shader",
            true
        );
        
        spriteShader->RegisterAttributes({"position", "uv", "palIndex"});
        spriteShader->RegisterUniforms({"modelview", "projection", "x1x2x4x3",
            "alpha", "tint", "mask", "neg", "gray", "add", "mult", "isFlat", 
            "isRgba", "isTrapez", "hue", "uvRect", "useUV"});
        spriteShader->RegisterTextures({"pal", "tex"});
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to create sprite shader: " << e.what() << std::endl;
        throw;
    }
    
    // Initialize model shader if enabled
    if (enableModel) {
        try {
            InitModelShader();
        } catch (const std::runtime_error& e) {
            std::cerr << "Failed to initialize model shader: " << e.what() << std::endl;
            enableModel = false;
        }
    }
    
    // Create FBO texture
    glGenTextures(1, &fbo_texture);
    glBindTexture(GL_TEXTURE_2D, fbo_texture);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Use sized internal format RGBA8
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    // Prepare postprocessing textures (use RGBA32F for negative values)
    fbo_pp.resize(2);
    fbo_pp_texture.resize(2);
    
    for (int i = 0; i < 2; i++) {
        glGenTextures(1, &fbo_pp_texture[i]);
        glBindTexture(GL_TEXTURE_2D, fbo_pp_texture[i]);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Use RGBA32F for negative values support
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, screenWidth, screenHeight,
                    0, GL_RGBA, GL_FLOAT, nullptr);
    }
    
    // Create depth renderbuffer
    glGenRenderbuffers(1, &rbo_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
    
    if (msaa > 0) {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, 
                                        GL_DEPTH_COMPONENT16, screenWidth, screenHeight);
    } else {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, 
                            screenWidth, screenHeight);
    }
    
    // Create main FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    if (msaa > 0) {
        // Note: GLES 3.1 doesn't support TEXTURE_2D_MULTISAMPLE
        // You'd need to handle this differently
        std::cerr << "MSAA not fully supported in GLES 3.1 core" << std::endl;
        msaa = 0;
    } else {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                              GL_TEXTURE_2D, fbo_texture, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                 GL_RENDERBUFFER, rbo_depth);
    }
    
    // Create postprocessing FBOs
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &fbo_pp[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_pp[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, fbo_pp_texture[i], 0);
    }
    
    // Create environment FBO for models
    if (enableModel) {
        glGenFramebuffers(1, &fbo_env);
        
        // Note: GLES 3.1 doesn't support TEXTURE_CUBE_MAP_ARRAY
        // Shadow cube maps would need alternative implementation
        if (enableShadow) {
            std::cerr << "Cube map array shadows not supported in GLES 3.1" << std::endl;
            enableShadow = false;
        }
    }
    
    // Unbind
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

void Renderer_GLES31::BeginFrame(bool clearColor) {
    glBindVertexArray(vao);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, screenWidth, screenHeight);
    
    GLbitfield clearBits = GL_DEPTH_BUFFER_BIT;
    if (clearColor) {
        clearBits |= GL_COLOR_BUFFER_BIT;
    }
    glClear(clearBits);
}

void Renderer_GLES31::SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) {
    glBindVertexArray(vao);
    glUseProgram(spriteShader->program);
    
    glBlendEquation(MapBlendEquation(eq));
    glBlendFunc(MapBlendFunction(src), MapBlendFunction(dst));
    glEnable(GL_BLEND);
    
    // Bind buffer and set up vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    const GLsizei stride = 20; // 5 floats * 4 bytes
    
    GLint loc = spriteShader->a["position"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
    
    loc = spriteShader->a["uv"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, (void*)8);
    
    loc = spriteShader->a["palIndex"];
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, stride, (void*)16);
}

void Renderer_GLES31::ReleasePipeline() {
    GLint loc = spriteShader->a["position"];
    glDisableVertexAttribArray(loc);
    
    loc = spriteShader->a["uv"];
    glDisableVertexAttribArray(loc);
    
    loc = spriteShader->a["palIndex"];
    glDisableVertexAttribArray(loc);
    
    glDisable(GL_BLEND);
}

void Renderer_GLES31::SetVertexData(const std::vector<float>& values) {
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, values.size() * sizeof(float), 
                values.data(), GL_STATIC_DRAW);
}

void Renderer_GLES31::RenderQuad() {
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Texture factory methods
Texture* Renderer_GLES31::newTexture(int32_t width, int32_t height, int32_t depth, bool filter) {
    GLuint handle;
    glGenTextures(1, &handle);
    return new Texture_GLES31(width, height, depth, filter, handle);
}

Texture* Renderer_GLES31::newPaletteTexture() {
    return newTexture(256, 1, 32, false);
}

Texture* Renderer_GLES31::newPaletteTextureArray(int32_t layers) {
    GLuint handle;
    glGenTextures(1, &handle);
    
    glBindTexture(GL_TEXTURE_2D_ARRAY, handle);
    
    // Allocate storage for all layers
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 256, 1, layers,
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    return new Texture_GLES31(256, 1, layers, false, handle);
}

// Uniform setters
void Renderer_GLES31::SetUniformI(const std::string& name, int val) {
    GLint loc = spriteShader->u[name];
    glUniform1i(loc, val);
}

void Renderer_GLES31::SetUniformF(const std::string& name, float v) {
    GLint loc = spriteShader->u[name];
    glUniform1f(loc, v);
}

void Renderer_GLES31::SetUniformF(const std::string& name, float v1, float v2) {
    GLint loc = spriteShader->u[name];
    glUniform2f(loc, v1, v2);
}

void Renderer_GLES31::SetUniformF(const std::string& name, float v1, float v2, float v3) {
    GLint loc = spriteShader->u[name];
    glUniform3f(loc, v1, v2, v3);
}

void Renderer_GLES31::SetUniformF(const std::string& name, float v1, float v2, float v3, float v4) {
    GLint loc = spriteShader->u[name];
    glUniform4f(loc, v1, v2, v3, v4);
}

void Renderer_GLES31::SetUniformFv(const std::string& name, const std::vector<float>& values) {
    GLint loc = spriteShader->u[name];
    switch (values.size()) {
        case 2: glUniform2fv(loc, 1, values.data()); break;
        case 3: glUniform3fv(loc, 1, values.data()); break;
        case 4: glUniform4fv(loc, 1, values.data()); break;
    }
}

void Renderer_GLES31::SetUniformMatrix(const std::string& name, const std::vector<float>& value) {
    GLint loc = spriteShader->u[name];
    glUniformMatrix4fv(loc, 1, GL_FALSE, value.data());
}

void Renderer_GLES31::SetTexture(const std::string& name, Texture* tex) {
    if (!tex || !tex->IsValid()) return;
    
    Texture_GLES31* t = static_cast<Texture_GLES31*>(tex);
    GLint loc = spriteShader->u[name];
    int unit = spriteShader->t[name];
    
    glActiveTexture(GL_TEXTURE0 + unit);
    
    // Handle palette textures specially
    if (name == "pal" || (t->depth > 1 && t->height == 1)) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, t->handle);
    } else {
        glBindTexture(GL_TEXTURE_2D, t->handle);
    }
    
    glUniform1i(loc, unit);
}

// Mapping functions
uint32_t Renderer_GLES31::MapBlendEquation(BlendEquation i) {
    static const std::map<BlendEquation, uint32_t> lut = {
        {BlendEquation::Add, GL_FUNC_ADD},
        {BlendEquation::ReverseSubtract, GL_FUNC_REVERSE_SUBTRACT}
    };
    return lut.at(i);
}

uint32_t Renderer_GLES31::MapBlendFunction(BlendFunc i) {
    static const std::map<BlendFunc, uint32_t> lut = {
        {BlendFunc::One, GL_ONE},
        {BlendFunc::Zero, GL_ZERO},
        {BlendFunc::SrcAlpha, GL_SRC_ALPHA},
        {BlendFunc::OneMinusSrcAlpha, GL_ONE_MINUS_SRC_ALPHA},
        {BlendFunc::DstColor, GL_DST_COLOR},
        {BlendFunc::OneMinusDstColor, GL_ONE_MINUS_DST_COLOR}
    };
    return lut.at(i);
}

// Shader compilation helper
GLuint Renderer_GLES31::compileShader(GLenum shaderType, const std::string& src) {
    GLuint shader = glCreateShader(shaderType);
    
    std::string fullSrc;
    if (shaderType == GL_GEOMETRY_SHADER) {
        fullSrc = "#version 310 es\n"
                  "#extension GL_EXT_geometry_shader : require\n"
                  "precision mediump float;\n" + src + "\x00";
    } else {
        fullSrc = "#version 310 es\n"
                  "precision mediump float;\n"
                  "precision mediump sampler2DArray;\n" + src + "\x00";
    }
    
    const char* c_str = fullSrc.c_str();
    glShaderSource(shader, 1, &c_str, nullptr);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compilation failed: ") + infoLog);
    }
    
    return shader;
}

std::unique_ptr<ShaderProgram_GLES31> Renderer_GLES31::compileShaderProgram(
    const std::string& vertSrc, 
    const std::string& fragSrc, 
    const std::string& geoSrc, 
    const std::string& id,
    bool crashWhenFail) {
    
    try {
        GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertSrc);
        GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragSrc);
        
        std::vector<GLuint> shaders = {vertShader, fragShader};
        
        if (!geoSrc.empty()) {
            GLuint geoShader = compileShader(GL_GEOMETRY_SHADER, geoSrc);
            shaders.push_back(geoShader);
        }
        
        GLuint program = linkProgram(shaders);
        
        auto result = std::make_unique<ShaderProgram_GLES31>();
        result->program = program;
        return result;
        
    } catch (const std::runtime_error& e) {
        if (crashWhenFail) {
            throw;
        }
        return nullptr;
    }
}