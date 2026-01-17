#ifndef RENDERER_OPENGLES_H
#define RENDERER_OPENGLES_H

#include "RendererInterfaces.h"
#include <glad/gles2.h>
#include <memory>
#include <vector>
#include <map>
#include <string>

// Forward declarations
class Environment;

// ==========================================
// OpenGL ES Version Detection & Macros
// ==========================================

#define OPENGL_ES_VERSION_MAJOR 3
#define OPENGL_ES_VERSION_MINOR 1

// Utility macro for OpenGL ES call error checking (debug builds only)
#ifdef DEBUG
    #define GLES_CHECK_ERROR() \
        do { \
            GLenum err = glGetError(); \
            if (err != GL_NO_ERROR) { \
                std::cerr << "OpenGL ES Error: " << std::hex << err << " at " \
                          << __FILE__ << ":" << __LINE__ << std::endl; \
            } \
        } while(0)
#else
    #define GLES_CHECK_ERROR()
#endif

// ==========================================
// OpenGL ES-Specific Shader Program
// ==========================================

class ShaderProgram_GLES : public IShaderProgram {
public:
    // Constructor/Destructor
    ShaderProgram_GLES();
    ~ShaderProgram_GLES();

    // Public interface
    void RegisterAttributes(const std::vector<std::string>& names);
    void RegisterUniforms(const std::vector<std::string>& names);
    void RegisterTextures(const std::vector<std::string>& names);

    // Accessors
    uint32_t GetProgram() const { return program; }
    int32_t GetAttributeLocation(const std::string& name) const;
    int32_t GetUniformLocation(const std::string& name) const;
    int32_t GetTextureUnit(const std::string& name) const;

    // OpenGL ES-specific accessors
    const std::map<std::string, int32_t>& GetAllAttributes() const { return attributes; }
    const std::map<std::string, int32_t>& GetAllUniforms() const { return uniforms; }
    const std::map<std::string, int32_t>& GetAllTextures() const { return textures; }

private:
    friend class Renderer_GLES;
    
    uint32_t program;
    std::map<std::string, int32_t> attributes;  // vertex attributes
    std::map<std::string, int32_t> uniforms;    // uniform variables
    std::map<std::string, int32_t> textures;    // texture units

    // Private helpers
    uint8_t* glStr(const std::string& s);
    
    // Shader info log retrieval
    std::string GetShaderInfoLog(uint32_t shader);
    std::string GetProgramInfoLog(uint32_t program);
};

// ==========================================
// OpenGL ES-Specific Texture
// ==========================================

class Texture_GLES : public ITexture {
public:
    // Constructor/Destructor
    Texture_GLES(int32_t width, int32_t height, int32_t depth, bool filter, uint32_t handle);
    ~Texture_GLES();

    // Public data upload methods
    void SetData(const std::vector<uint8_t>& data);
    void SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, 
                    int32_t width, int32_t height);
    void SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag,
                 TextureSamplingParam min, TextureSamplingParam ws, TextureSamplingParam wt);
    void SetPixelData(const std::vector<float>& data);
    void SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer);

    // Public copy operations
    void CopyData(const Texture_GLES* src);

    // Public query methods
    bool IsValid() const;
    int32_t GetWidth() const { return width; }
    int32_t GetHeight() const { return height; }
    int32_t GetDepth() const { return depth; }
    uint32_t GetHandle() const { return handle; }
    bool GetFilter() const { return filter; }

    // Public serialization
    int SavePNG(const std::string& filename, const std::vector<uint32_t>* palette = nullptr);

    // OpenGL ES-specific methods
    void Bind(GLenum target = GL_TEXTURE_2D) const;
    void Unbind(GLenum target = GL_TEXTURE_2D) const;
    void SetFilterMode(bool linearFilter);

private:
    int32_t width;
    int32_t height;
    int32_t depth;
    bool filter;
    uint32_t handle;
    GLenum textureTarget;  // GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D_ARRAY, etc.

    // Private helpers
    uint32_t MapInternalFormat(int32_t depth) const;
    int32_t MapTextureSamplingParam(TextureSamplingParam param) const;
    void SetTextureParameters();
};

// ==========================================
// OpenGL ES-Specific Renderer
// ==========================================

class Renderer_GLES : public IRenderer {
public:
    // Constructor/Destructor
    Renderer_GLES();
    ~Renderer_GLES();

    // ===== Initialization & Lifecycle =====
    void Init();
    void Close();
    std::string GetName() const;
    int InitModelShader();

    // ===== Frame Management =====
    void BeginFrame(bool clearColor);
    void EndFrame();
    void Await();

    // ===== Capabilities =====
    bool IsModelEnabled() const;
    bool IsShadowEnabled() const;

    // ===== Blend Operations =====
    void BlendReset();
    void SetBlending(BlendEquation eq, BlendFunc src, BlendFunc dst);

    // ===== Depth & Face Operations =====
    void SetDepthTest(bool depthTest);
    void SetDepthMask(bool depthMask);
    void SetFrontFace(bool invertFrontFace);
    void SetCullFace(bool doubleSided);

    // ===== Pipeline Setup - Sprites =====
    void SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst);
    void SetPipelineBatch();
    void ReleasePipeline();

    // ===== Pipeline Setup - Models =====
    void prepareModelPipeline(uint32_t bufferIndex, const Environment* env);
    void SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst, 
                         bool depthTest, bool depthMask, bool doubleSided, bool invertFrontFace,
                         bool useUV, bool useNormal, bool useTangent, bool useVertColor,
                         bool useJoint0, bool useJoint1, bool useOutlineAttribute,
                         uint32_t numVertices, uint32_t vertAttrOffset);
    void ReleaseModelPipeline();

    // ===== Pipeline Setup - Shadow Maps =====
    void prepareShadowMapPipeline(uint32_t bufferIndex);
    void setShadowMapPipeline(bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal,
                             bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1,
                             uint32_t numVertices, uint32_t vertAttrOffset);
    void ReleaseShadowPipeline();

    // ===== Mesh Outline =====
    void SetMeshOulinePipeline(bool invertFrontFace, float meshOutline);

    // ===== Scissor Operations =====
    void Scissor(int32_t x, int32_t y, int32_t width, int32_t height);
    void DisableScissor();

    // ===== Texture Operations =====
    std::shared_ptr<ITexture> newTexture(int32_t width, int32_t height, int32_t depth, bool filter);
    std::shared_ptr<ITexture> newPaletteTexture();
    std::shared_ptr<ITexture> newPaletteTextureArray(int32_t layers);
    std::shared_ptr<ITexture> newModelTexture(int32_t width, int32_t height, int32_t depth, bool filter);
    std::shared_ptr<ITexture> newDataTexture(int32_t width, int32_t height);
    std::shared_ptr<ITexture> newHDRTexture(int32_t width, int32_t height);
    std::shared_ptr<ITexture> newCubeMapTexture(int32_t widthHeight, bool mipmap, int32_t lowestMipLevel);

    void SetTexture(const std::string& name, const std::shared_ptr<ITexture>& tex);
    void SetModelTexture(const std::string& name, const std::shared_ptr<ITexture>& tex);
    void SetShadowMapTexture(const std::string& name, const std::shared_ptr<ITexture>& tex);

    // ===== Uniform Operations - Sprite Shader =====
    void SetUniformI(const std::string& name, int val);
    void SetUniformF(const std::string& name, const std::vector<float>& values);
    void SetUniformFv(const std::string& name, const std::vector<float>& values);
    void SetUniformMatrix(const std::string& name, const std::vector<float>& value);

    // ===== Uniform Operations - Model Shader =====
    void SetModelUniformI(const std::string& name, int val);
    void SetModelUniformF(const std::string& name, const std::vector<float>& values);
    void SetModelUniformFv(const std::string& name, const std::vector<float>& values);
    void SetModelUniformMatrix(const std::string& name, const std::vector<float>& value);
    void SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value);

    // ===== Uniform Operations - Shadow Map Shader =====
    void SetShadowMapUniformI(const std::string& name, int val);
    void SetShadowMapUniformF(const std::string& name, const std::vector<float>& values);
    void SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values);
    void SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value);
    void SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value);

    // ===== Shadow Frame Operations =====
    void SetShadowFrameTexture(uint32_t i);
    void SetShadowFrameCubeTexture(uint32_t i);

    // ===== Vertex Data Operations =====
    void SetVertexData(const std::vector<float>& values);
    void SetVertexDataArray(const std::vector<float>& values);
    void SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values);
    void SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values);

    // ===== Rendering Operations =====
    void RenderQuad();
    void RenderQuadBatch(int32_t vertexCount);
    void RenderElements(PrimitiveMode mode, int count, int offset);
    void RenderShadowMapElements(PrimitiveMode mode, int count, int offset);

    // ===== CubeMap & IBL Rendering =====
    void RenderCubeMap(const std::shared_ptr<ITexture>& envTex, const std::shared_ptr<ITexture>& cubeTex);
    void RenderFilteredCubeMap(int32_t distribution, const std::shared_ptr<ITexture>& cubeTex,
                              const std::shared_ptr<ITexture>& filteredTex,
                              int32_t mipmapLevel, int32_t sampleCount, float roughness);
    void RenderLUT(int32_t distribution, const std::shared_ptr<ITexture>& cubeTex,
                   const std::shared_ptr<ITexture>& lutTex, int32_t sampleCount);

    // ===== Pixel Operations =====
    void ReadPixels(std::vector<uint8_t>& data, int width, int height);

    // ===== Projection Matrices =====
    Mat4 PerspectiveProjectionMatrix(float angle, float aspect, float near, float far) const;
    Mat4 OrthographicProjectionMatrix(float left, float right, float bottom, float top, 
                                           float near, float far) const;

    // ===== Threading & Sync =====
    bool NewWorkerThread();
    void SetVSync();

    // ===== OpenGL ES-Specific Public Methods =====
    uint32_t GetVAO() const { return vao; }
    uint32_t GetSpriteShaderProgram() const;
    uint32_t GetModelShaderProgram() const;
    uint32_t GetShadowMapShaderProgram() const;
    
    // Framebuffer queries
    uint32_t GetMainFBO() const { return fbo; }
    uint32_t GetShadowFBO() const { return fbo_shadow; }
    uint32_t GetEnvironmentFBO() const { return fbo_env; }

    // Get framebuffer texture
    uint32_t GetMainFBOTexture() const { return fbo_texture; }

    // ===== Debugging & Info =====
    void PrintInfo();
    void PrintCapabilities();

    private:
    // ===== Internal State =====
    uint32_t fbo;
    uint32_t fbo_texture;
    uint32_t rbo_depth;
    
    // MSAA rendering (optional on ES, platform dependent)
    uint32_t fbo_f;
    std::shared_ptr<ITexture> fbo_f_texture;
    
    // Shadow mapping
    uint32_t fbo_shadow;
    uint32_t fbo_shadow_cube_texture;
    uint32_t fbo_env;
    
    // Post-processing
    std::vector<uint32_t> fbo_pp;
    std::vector<uint32_t> fbo_pp_texture;
    
    // Shader programs (using smart pointers for automatic cleanup)
    std::shared_ptr<ShaderProgram_GLES> spriteShader;
    std::shared_ptr<ShaderProgram_GLES> modelShader;
    std::shared_ptr<ShaderProgram_GLES> shadowMapShader;
    std::shared_ptr<ShaderProgram_GLES> panoramaToCubeMapShader;
    std::shared_ptr<ShaderProgram_GLES> cubemapFilteringShader;
    std::vector<std::shared_ptr<ShaderProgram_GLES>> postShaderSelect;

    // Vertex buffers
    uint32_t postVertBuffer;
    uint32_t vertexBuffer;
    uint32_t vertexBufferBatch;
    uint32_t modelVertexBuffer[2];
    uint32_t modelIndexBuffer[2];
    uint32_t vao;

    // Configuration
    bool enableModel;
    bool enableShadow;
    int msaaLevel;  // 0 = disabled, 2, 4, 8 for MSAA levels
    
    // Render state tracking
    GLState glState;
    
    // Screen dimensions (for scissor and viewport calculations)
    struct {
        int32_t x, y, width, height;
    } viewport;

    // Mobile-specific capabilities
    struct {
        bool hasInstancedArrays;          // EXT_instanced_arrays
        bool hasTextureArrays;            // ES 3.0+
        bool hasSamplerObjects;           // ES 3.0+
        bool hasComputeShaders;           // ES 3.1+
        bool hasIndirectDispatch;         // ES 3.1+
        bool hasShaderImageLoadStore;     // ES 3.1+
    } capabilities;

    // ===== Private Helper Methods =====
    
    // Shader compilation and linking (ES-specific)
    std::shared_ptr<ShaderProgram_GLES> newShaderProgram(const std::string& vert, const std::string& frag,
                                                        const std::string& geo, const std::string& id,
                                                        bool crashWhenFail);
    uint32_t compileShader(uint32_t shaderType, const std::string& src);
    uint32_t linkProgram(const std::vector<uint32_t>& shaders);
    
    // Framebuffer operations
    bool InitFramebuffer(uint32_t& fbo, uint32_t& texture, uint32_t& rbo, 
                        int32_t width, int32_t height, bool useMultisample = false);
    bool InitShadowFramebuffer();
    bool InitPostProcessingFramebuffers();
    
    // State management
    void CacheRenderState();
    void RestoreRenderState();
    void ResetRenderState();
    
    // Blend and render mode mapping
    uint32_t MapBlendEquation(BlendEquation eq) const;
    uint32_t MapBlendFunction(BlendFunc func) const;
    uint32_t MapPrimitiveMode(PrimitiveMode mode) const;
    uint32_t MapTextureTarget(const std::shared_ptr<ITexture>& tex) const;
    
    // Vertex attribute setup
    void SetupVertexAttributes(const std::shared_ptr<ShaderProgram_GLES>& shader, 
                             uint32_t stride, const std::vector<std::string>& attributes);
    
    // Utility methods
    bool IsGLESExtensionSupported(const std::string& extension);
    void PrintGLESInfo();
    void PrintGLESCapabilities();
    void DetectCapabilities();
};

// ==========================================
// Inline Implementations
// ==========================================

inline void Texture_GLES::Bind(GLenum target) const {
    glBindTexture(target, handle);
}

inline void Texture_GLES::Unbind(GLenum target) const {
    glBindTexture(target, 0);
}

inline uint32_t Renderer_GLES::GetSpriteShaderProgram() const {
    return spriteShader ? spriteShader->GetProgram() : 0;
}

inline uint32_t Renderer_GLES::GetModelShaderProgram() const {
    return modelShader ? modelShader->GetProgram() : 0;
}

inline uint32_t Renderer_GLES::GetShadowMapShaderProgram() const {
    return shadowMapShader ? shadowMapShader->GetProgram() : 0;
}

inline void Renderer_GLES::PrintInfo() {
    PrintGLESInfo();
}

inline void Renderer_GLES::PrintCapabilities() {
    PrintGLESCapabilities();
}

#endif // RENDERER_OPENGLES_H
