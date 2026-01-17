#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "linmath.h"

struct Mat4 {
    mat4x4 data;
};

// ------------------------------------------------------------------
// Enums & Constants
// ------------------------------------------------------------------

enum class BlendFunc {
    One,
    Zero,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstColor,
    OneMinusDstColor
};

enum class BlendEquation {
    Add,
    ReverseSubtract
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
    Nearest,
    Linear,
    NearestMipMapNearest,
    LinearMipMapNearest,
    NearestMipMapLinear,
    LinearMipMapLinear,
    ClampToEdge,
    MirroredRepeat,
    Repeat
};

// ------------------------------------------------------------------
// Texture Interface
// ------------------------------------------------------------------

class Texture {
public:
    virtual ~Texture() = default;

    // Uploads
    virtual void SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) = 0;
    virtual void SetData(const std::vector<uint8_t>& data) = 0;
    virtual void SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, int32_t width, int32_t height) = 0;
    virtual void SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag, TextureSamplingParam min, TextureSamplingParam ws, TextureSamplingParam wt) = 0;
    virtual void SetPixelData(const std::vector<float>& data) = 0;
    
    // Copy/Management
    virtual void CopyData(Texture* src) = 0;
    virtual bool IsValid() = 0;
    virtual int32_t GetWidth() = 0;
    virtual int32_t GetHeight() = 0;
    
    // Debug/Export
    virtual void SavePNG(const std::string& filename, const std::vector<uint32_t>& pal) = 0;
};

// Forward declaration for environment maps (if used in PBR)
struct Environment; 

// ------------------------------------------------------------------
// Renderer Interface
// ------------------------------------------------------------------

class Renderer {
public:
    virtual ~Renderer() = default;

    // Factory Method: Creates the specific backend based on API choice
    static Renderer* Create();

    virtual std::string GetName() = 0;
    
    // Lifecycle
    virtual void Init() = 0;
    virtual void Close() = 0;
    
    // Frame Control
    virtual void BeginFrame(bool clearColor) = 0;
    virtual void EndFrame() = 0;
    virtual void Await() = 0; // Wait for GPU to finish (glFinish)

    // Feature Flags
    virtual bool IsModelEnabled() = 0;
    virtual bool IsShadowEnabled() = 0;
    virtual bool NewWorkerThread() = 0;
    virtual void SetVSync() = 0;

    // ------------------------------------------------------------------
    // Pipeline & State Management
    // ------------------------------------------------------------------
    
    virtual void BlendReset() = 0;
    
    // sprite/2D pipeline
    virtual void SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) = 0;
    virtual void SetPipelineBatch() = 0;
    virtual void ReleasePipeline() = 0;

    // Shadow mapping pipeline
    virtual void prepareShadowMapPipeline(uint32_t bufferIndex) = 0;
    virtual void setShadowMapPipeline(bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, uint32_t numVertices, uint32_t vertAttrOffset) = 0;
    virtual void ReleaseShadowPipeline() = 0;

    // 3D Model pipeline
    virtual void prepareModelPipeline(uint32_t bufferIndex, Environment* env) = 0;
    virtual void SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst, bool depthTest, bool depthMask, bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, bool useOutlineAttribute, uint32_t numVertices, uint32_t vertAttrOffset) = 0;
    virtual void SetMeshOulinePipeline(bool invertFrontFace, float meshOutline) = 0;
    virtual void ReleaseModelPipeline() = 0;

    // ------------------------------------------------------------------
    // Texture Factory Methods
    // ------------------------------------------------------------------
    
    virtual Texture* newTexture(int32_t width, int32_t height, int32_t depth, bool filter) = 0;
    virtual Texture* newPaletteTexture() = 0;
    virtual Texture* newPaletteTextureArray(int32_t layers) = 0;
    virtual Texture* newModelTexture(int32_t width, int32_t height, int32_t depth, bool filter) = 0;
    virtual Texture* newDataTexture(int32_t width, int32_t height) = 0;
    virtual Texture* newHDRTexture(int32_t width, int32_t height) = 0;
    virtual Texture* newCubeMapTexture(int32_t widthHeight, bool mipmap, int32_t lowestMipLevel) = 0;

    // ------------------------------------------------------------------
    // Utils
    // ------------------------------------------------------------------
    
    virtual void ReadPixels(std::vector<uint8_t>& data, int width, int height) = 0;
    virtual void Scissor(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
    virtual void DisableScissor() = 0;

    // ------------------------------------------------------------------
    // Uniforms (2D Sprite Shader)
    // ------------------------------------------------------------------
    
    virtual void SetUniformI(const std::string& name, int val) = 0;
    // C++ doesn't support varargs (...) purely for virtuals nicely, overloading is safer
    virtual void SetUniformF(const std::string& name, float v) = 0;
    virtual void SetUniformF(const std::string& name, float v1, float v2) = 0; 
    virtual void SetUniformF(const std::string& name, float v1, float v2, float v3) = 0;
    virtual void SetUniformF(const std::string& name, float v1, float v2, float v3, float v4) = 0;
    virtual void SetUniformFv(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetUniformMatrix(const std::string& name, const std::vector<float>& value) = 0;
    virtual void SetTexture(const std::string& name, Texture* tex) = 0;

    // ------------------------------------------------------------------
    // Uniforms (3D Model Shader)
    // ------------------------------------------------------------------

    virtual void SetModelUniformI(const std::string& name, int val) = 0;
    virtual void SetModelUniformF(const std::string& name, float v) = 0;
    virtual void SetModelUniformF(const std::string& name, float v1, float v2) = 0;
    virtual void SetModelUniformF(const std::string& name, float v1, float v2, float v3) = 0;
    virtual void SetModelUniformF(const std::string& name, float v1, float v2, float v3, float v4) = 0;
    virtual void SetModelUniformFv(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetModelUniformMatrix(const std::string& name, const std::vector<float>& value) = 0;
    virtual void SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value) = 0;
    virtual void SetModelTexture(const std::string& name, Texture* t) = 0;

    // ------------------------------------------------------------------
    // Uniforms (Shadow Map Shader)
    // ------------------------------------------------------------------

    virtual void SetShadowMapUniformI(const std::string& name, int val) = 0;
    virtual void SetShadowMapUniformF(const std::string& name, float v) = 0; 
    // ... (Add overrides for 2, 3, 4 params if needed)
    virtual void SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values) = 0;
    virtual void SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value) = 0;
    virtual void SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value) = 0;
    virtual void SetShadowMapTexture(const std::string& name, Texture* t) = 0;
    virtual void SetShadowFrameTexture(uint32_t i) = 0;
    virtual void SetShadowFrameCubeTexture(uint32_t i) = 0;

    // ------------------------------------------------------------------
    // Draw Calls & Data Upload
    // ------------------------------------------------------------------

    virtual void SetVertexData(const std::vector<float>& values) = 0;
    virtual void SetVertexDataArray(const std::vector<float>& values) = 0;
    virtual void SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values) = 0;
    virtual void SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values) = 0;

    virtual void RenderQuad() = 0;
    virtual void RenderQuadBatch(int32_t vertexCount) = 0;
    virtual void RenderElements(PrimitiveMode mode, int count, int offset) = 0;
    virtual void RenderShadowMapElements(PrimitiveMode mode, int count, int offset) = 0;
    
    // Environment/Cubemap rendering
    virtual void RenderCubeMap(Texture* envTexture, Texture* cubeTexture) = 0;
    virtual void RenderFilteredCubeMap(int32_t distribution, Texture* cubeTexture, Texture* filteredTexture, int32_t mipmapLevel, int32_t sampleCount, float roughness) = 0;
    virtual void RenderLUT(int32_t distribution, Texture* cubeTexture, Texture* lutTexture, int32_t sampleCount) = 0;

    // ------------------------------------------------------------------
    // Math Helpers (Projection)
    // ------------------------------------------------------------------
    
    virtual Mat4 PerspectiveProjectionMatrix(float angle, float aspect, float near, float far) = 0;
    virtual Mat4 OrthographicProjectionMatrix(float left, float right, float bottom, float top, float near, float far) = 0;
    virtual void SetUniformMatrix(const std::string& name, const Mat4& mat) = 0;
};