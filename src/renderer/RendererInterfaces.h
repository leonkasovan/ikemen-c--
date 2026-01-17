#ifndef RENDERER_INTERFACES_H
#define RENDERER_INTERFACES_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "linmath.h"

struct Mat4 {
    mat4x4 data;
};

// Forward declarations
class ITexture;
class IShaderProgram;
class Environment;

// Enumerations
enum class BlendEquation {
    Add,
    ReverseSubtract
};

enum class BlendFunc {
    One,
    Zero,
    SrcAlpha,
    OneMinusSrcAlpha,
    OneMinusDstColor,
    DstColor
};

enum class PrimitiveMode {
    Lines,
    LineLoop,
    LineStrip,
    Triangles,
    TriangleStrip,
    TriangleFan
};

enum class TextureSamplingParam {
    FilterNearest,
    FilterLinear,
    FilterNearestMipMapNearest,
    FilterLinearMipMapNearest,
    FilterNearestMipMapLinear,
    FilterLinearMipMapLinear,
    WrapClampToEdge,
    WrapMirroredRepeat,
    WrapRepeat
};

// ==========================================
// IShaderProgram Interface
// ==========================================
class IShaderProgram {
public:
    // Constructor/Destructor
    IShaderProgram() : program(0) {}
    virtual ~IShaderProgram() {}

    // Public interface
    void RegisterAttributes(const std::vector<std::string>& names) {
        for (const auto& name : names) {
            attributes[name] = -1;
        }
    }
    void RegisterUniforms(const std::vector<std::string>& names) {
        for (const auto& name : names) {
            uniforms[name] = -1;
        }
    }
    void RegisterTextures(const std::vector<std::string>& names) {
        for (const auto& name : names) {
            textures[name] = -1;
        }
    }

    // Accessors
    uint32_t GetProgram() const { return program; }
    int32_t GetAttributeLocation(const std::string& name) const {
        auto it = attributes.find(name);
        return it != attributes.end() ? it->second : -1;
    }
    int32_t GetUniformLocation(const std::string& name) const {
        auto it = uniforms.find(name);
        return it != uniforms.end() ? it->second : -1;
    }
    int32_t GetTextureUnit(const std::string& name) const {
        auto it = textures.find(name);
        return it != textures.end() ? it->second : -1;
    }

private:
    friend class IRenderer;
    
    uint32_t program;
    std::map<std::string, int32_t> attributes;  // a in Go
    std::map<std::string, int32_t> uniforms;    // u in Go
    std::map<std::string, int32_t> textures;    // t in Go

    // Private helpers
    uint8_t* glStr(const std::string& s) { return nullptr; }
};

// ==========================================
// ITexture Interface
// ==========================================
class ITexture {
public:
    // Constructor/Destructor
    ITexture(int32_t width, int32_t height, int32_t depth, bool filter, uint32_t handle)
        : width(width), height(height), depth(depth), filter(filter), handle(handle) {}
    virtual ~ITexture() {}

    // Public data upload methods
    void SetData(const std::vector<uint8_t>& data) {}
    void SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, int32_t width, int32_t height) {}
    void SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag, TextureSamplingParam min, 
                  TextureSamplingParam ws, TextureSamplingParam wt) {}
    void SetPixelData(const std::vector<float>& data) {}
    void SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) {}

    // Public copy operations
    void CopyData(const ITexture* src) {}

    // Public query methods
    bool IsValid() const { return handle != 0; }
    int32_t GetWidth() const { return width; }
    int32_t GetHeight() const { return height; }
    int32_t GetDepth() const { return depth; }
    uint32_t GetHandle() const { return handle; }

    // Public serialization
    int SavePNG(const std::string& filename, const std::vector<uint32_t>* palette = nullptr) { return 0; }

private:
    int32_t width;
    int32_t height;
    int32_t depth;
    bool filter;
    uint32_t handle;

    // Private helpers
    uint32_t MapInternalFormat(int32_t depth) const { return 0; }
    int32_t MapTextureSamplingParam(TextureSamplingParam param) const { return 0; }
};

// ==========================================
// GLState - Renderer State Tracking
// ==========================================
struct GLState {
    bool depthTest;
    bool depthMask;
    bool invertFrontFace;
    bool doubleSided;
    BlendEquation blendEquation;
    BlendFunc blendSrc;
    BlendFunc blendDst;
    bool useUV;
    bool useNormal;
    bool useTangent;
    bool useVertColor;
    bool useJoint0;
    bool useJoint1;
    bool useOutlineAttribute;
};

// ==========================================
// IRenderer Interface
// ==========================================
class IRenderer {
public:
    // Constructor/Destructor
    IRenderer() : fbo(0), fbo_texture(0), rbo_depth(0), glVersionMajor(0), glVersionMinor(0),
                   fbo_f(0), fbo_f_texture(nullptr), fbo_shadow(0), fbo_shadow_cube_texture(0), fbo_env(0),
                   postVertBuffer(0), vertexBuffer(0), vertexBufferBatch(0), vao(0),
                   enableModel(false), enableShadow(false) {}
    virtual ~IRenderer() {}

    // ===== Initialization & Lifecycle =====
    virtual void Init() = 0;
    virtual void Close() = 0;
    virtual std::string GetName() const = 0;
    virtual int InitModelShader() = 0;

    // ===== Frame Management =====
    virtual void BeginFrame(bool clearColor) = 0;
    virtual void EndFrame() = 0;
    virtual void Await() = 0;

    // ===== Capabilities =====
    virtual bool IsModelEnabled() const = 0;
    virtual bool IsShadowEnabled() const = 0;

    // ===== Blend Operations =====
    virtual void BlendReset() = 0;
    virtual void SetBlending(BlendEquation eq, BlendFunc src, BlendFunc dst) = 0;

    // ===== Depth & Face Operations =====
    virtual void SetDepthTest(bool depthTest) = 0;
    virtual void SetDepthMask(bool depthMask) = 0;
    virtual void SetFrontFace(bool invertFrontFace) = 0;
    virtual void SetCullFace(bool doubleSided) = 0;

    // ===== Pipeline Setup - Sprites =====
    virtual void SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) = 0;
    virtual void SetPipelineBatch() = 0;
    virtual void ReleasePipeline() = 0;

    // ===== Pipeline Setup - Models =====
    virtual void prepareModelPipeline(uint32_t bufferIndex, const Environment* env) = 0;
    virtual void SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst, 
                         bool depthTest, bool depthMask, bool doubleSided, bool invertFrontFace,
                         bool useUV, bool useNormal, bool useTangent, bool useVertColor,
                         bool useJoint0, bool useJoint1, bool useOutlineAttribute,
                         uint32_t numVertices, uint32_t vertAttrOffset) = 0;
    virtual void ReleaseModelPipeline() = 0;

    // ===== Pipeline Setup - Shadow Maps =====
    virtual void prepareShadowMapPipeline(uint32_t bufferIndex) = 0;
    virtual void setShadowMapPipeline(bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal,
                             bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1,
                             uint32_t numVertices, uint32_t vertAttrOffset) = 0;
    virtual void ReleaseShadowPipeline() = 0;

    // ===== Mesh Outline =====
    virtual void SetMeshOulinePipeline(bool invertFrontFace, float meshOutline) = 0;

    // ===== Scissor Operations =====
    virtual void Scissor(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
    virtual void DisableScissor() = 0;

    // ===== Texture Operations =====
    virtual std::shared_ptr<ITexture> newTexture(int32_t width, int32_t height, int32_t depth, bool filter) = 0;
    virtual std::shared_ptr<ITexture> newPaletteTexture() = 0;
    virtual std::shared_ptr<ITexture> newPaletteTextureArray(int32_t layers) = 0;
    virtual std::shared_ptr<ITexture> newModelTexture(int32_t width, int32_t height, int32_t depth, bool filter) = 0;
    virtual std::shared_ptr<ITexture> newDataTexture(int32_t width, int32_t height) = 0;
    virtual std::shared_ptr<ITexture> newHDRTexture(int32_t width, int32_t height) = 0;
    virtual std::shared_ptr<ITexture> newCubeMapTexture(int32_t widthHeight, bool mipmap, int32_t lowestMipLevel) = 0;

    virtual void SetTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) = 0;
    virtual void SetModelTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) = 0;
    virtual void SetShadowMapTexture(const std::string& name, const std::shared_ptr<ITexture>& tex) = 0;

    // ===== Uniform Operations - Sprite Shader =====
    virtual void SetUniformI(const std::string& name, int val) = 0;
    virtual void SetUniformF(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetUniformFv(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetUniformMatrix(const std::string& name, const std::vector<float>& value) = 0;

    // ===== Uniform Operations - Model Shader =====
    virtual void SetModelUniformI(const std::string& name, int val) = 0;
    virtual void SetModelUniformF(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetModelUniformFv(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetModelUniformMatrix(const std::string& name, const std::vector<float>& value) = 0;
    virtual void SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value) = 0;

    // ===== Uniform Operations - Shadow Map Shader =====
    virtual void SetShadowMapUniformI(const std::string& name, int val) = 0;
    virtual void SetShadowMapUniformF(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value) = 0;
    virtual void SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value) = 0;

    // ===== Shadow Frame Operations =====
    virtual void SetShadowFrameTexture(uint32_t i) = 0;
    virtual void SetShadowFrameCubeTexture(uint32_t i) = 0;

    // ===== Vertex Data Operations =====
    virtual void SetVertexData(const std::vector<float>& values) = 0;
    virtual void SetVertexDataArray(const std::vector<float>& values) = 0;
    virtual void SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values) = 0;
    virtual void SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values) = 0;

    // ===== Rendering Operations =====
    virtual void RenderQuad() = 0;
    virtual void RenderQuadBatch(int32_t vertexCount) = 0;
    virtual void RenderElements(PrimitiveMode mode, int count, int offset) = 0;
    virtual void RenderShadowMapElements(PrimitiveMode mode, int count, int offset) = 0;

    // ===== CubeMap & IBL Rendering =====
    virtual void RenderCubeMap(const std::shared_ptr<ITexture>& envTex, const std::shared_ptr<ITexture>& cubeTex) = 0;
    virtual void RenderFilteredCubeMap(int32_t distribution, const std::shared_ptr<ITexture>& cubeTex,
                              const std::shared_ptr<ITexture>& filteredTex,
                              int32_t mipmapLevel, int32_t sampleCount, float roughness) = 0;
    virtual void RenderLUT(int32_t distribution, const std::shared_ptr<ITexture>& cubeTex,
                   const std::shared_ptr<ITexture>& lutTex, int32_t sampleCount) = 0;

    // ===== Pixel Operations =====
    virtual void ReadPixels(std::vector<uint8_t>& data, int width, int height) = 0;

    // ===== Projection Matrices =====
    virtual Mat4 PerspectiveProjectionMatrix(float angle, float aspect, float near, float far) const = 0;
    virtual Mat4 OrthographicProjectionMatrix(float left, float right, float bottom, float top, float near, float far) const = 0;

    // ===== Threading & Sync =====
    virtual bool NewWorkerThread() = 0;
    virtual void SetVSync() = 0;

    // ===== Debugging & Info =====
    virtual void PrintInfo() = 0;
    virtual void PrintCapabilities() = 0;

    // ===== OpenGL Version Query =====
    int GetVersionMajor() const { return glVersionMajor; }
    int GetVersionMinor() const { return glVersionMinor; }

protected:
    // ===== Internal State =====
    uint32_t fbo;
    uint32_t fbo_texture;
    uint32_t rbo_depth;
    
    // OpenGL Version
    int glVersionMajor;
    int glVersionMinor;
    
    // MSAA rendering
    uint32_t fbo_f;
    ITexture* fbo_f_texture;
    
    // Shadow mapping
    uint32_t fbo_shadow;
    uint32_t fbo_shadow_cube_texture;
    uint32_t fbo_env;
    
    // Post-processing
    std::vector<uint32_t> fbo_pp;
    std::vector<uint32_t> fbo_pp_texture;
    
    // Shader programs
    std::shared_ptr<IShaderProgram> spriteShader;
    std::shared_ptr<IShaderProgram> modelShader;
    std::shared_ptr<IShaderProgram> shadowMapShader;
    std::shared_ptr<IShaderProgram> panoramaToCubeMapShader;
    std::shared_ptr<IShaderProgram> cubemapFilteringShader;
    std::vector<std::shared_ptr<IShaderProgram>> postShaderSelect;

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
    GLState glState;

    // ===== Private Helper Methods =====
    std::shared_ptr<IShaderProgram> newShaderProgram(const std::string& vert, const std::string& frag,
                                                      const std::string& geo, const std::string& id,
                                                      bool crashWhenFail) { return nullptr; }
    uint32_t compileShader(uint32_t shaderType, const std::string& src) { return 0; }
    uint32_t linkProgram(const std::vector<uint32_t>& shaders) { return 0; }

    // Blend and render mode mapping
    uint32_t MapBlendEquation(BlendEquation eq) const { return 0; }
    uint32_t MapBlendFunction(BlendFunc func) const { return 0; }
    uint32_t MapPrimitiveMode(PrimitiveMode mode) const { return 0; }
};

#endif // RENDERER_INTERFACES_H