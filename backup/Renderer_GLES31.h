#pragma once

#include "RenderInterfaces.h"
#include <glad/gles2.h>
#include <map>
#include <string>
#include <vector>
#include <memory>

// Forward declarations for GLES 3.1
typedef unsigned int GLuint;
typedef int GLint;
typedef unsigned int GLenum;

class ShaderProgram_GLES31 {
public:
    GLuint program;
    std::map<std::string, GLint> a; // attributes
    std::map<std::string, GLint> u; // uniforms
    std::map<std::string, int> t;   // texture units
    
    ShaderProgram_GLES31();
    ~ShaderProgram_GLES31();
    
    void RegisterAttributes(const std::vector<std::string>& names);
    void RegisterUniforms(const std::vector<std::string>& names);
    void RegisterTextures(const std::vector<std::string>& names);
};

class Texture_GLES31 : public Texture {
public:
    int32_t width;
    int32_t height;
    int32_t depth;
    bool filter;
    GLuint handle;
    
    Texture_GLES31(int32_t w, int32_t h, int32_t d, bool f, GLuint hdl);
    virtual ~Texture_GLES31();
    
    // Texture interface implementation
    virtual void SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) override;
    virtual void SetData(const std::vector<uint8_t>& data) override;
    virtual void SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, int32_t width, int32_t height) override;
    virtual void SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag, TextureSamplingParam min, TextureSamplingParam ws, TextureSamplingParam wt) override;
    virtual void SetPixelData(const std::vector<float>& data) override;
    
    virtual void CopyData(Texture* src) override;
    virtual bool IsValid() override;
    virtual int32_t GetWidth() override;
    virtual int32_t GetHeight() override;
    virtual void SavePNG(const std::string& filename, const std::vector<uint32_t>& pal) override;
    
    // Helper functions
    uint32_t MapInternalFormat(int32_t i);
    int32_t MapTextureSamplingParam(TextureSamplingParam i);
};

struct Environment; // Forward declaration

class Renderer_GLES31 : public Renderer {
public:
    Renderer_GLES31();
    virtual ~Renderer_GLES31();
    
    // Renderer interface implementation
    virtual std::string GetName() override;
    
    virtual void Init() override;
    virtual void Close() override;
    
    virtual void BeginFrame(bool clearColor) override;
    virtual void EndFrame() override;
    virtual void Await() override;
    
    virtual bool IsModelEnabled() override;
    virtual bool IsShadowEnabled() override;
    
    virtual void BlendReset() override;
    
    virtual void SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) override;
    virtual void SetPipelineBatch() override;
    virtual void ReleasePipeline() override;
    
    virtual void prepareShadowMapPipeline(uint32_t bufferIndex) override;
    virtual void setShadowMapPipeline(bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, uint32_t numVertices, uint32_t vertAttrOffset) override;
    virtual void ReleaseShadowPipeline() override;
    
    virtual void prepareModelPipeline(uint32_t bufferIndex, Environment* env) override;
    virtual void SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst, bool depthTest, bool depthMask, bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, bool useOutlineAttribute, uint32_t numVertices, uint32_t vertAttrOffset) override;
    virtual void SetMeshOulinePipeline(bool invertFrontFace, float meshOutline) override;
    virtual void ReleaseModelPipeline() override;
    
    // Texture factory methods
    virtual Texture* newTexture(int32_t width, int32_t height, int32_t depth, bool filter) override;
    virtual Texture* newPaletteTexture() override;
    virtual Texture* newPaletteTextureArray(int32_t layers) override;
    virtual Texture* newModelTexture(int32_t width, int32_t height, int32_t depth, bool filter) override;
    virtual Texture* newDataTexture(int32_t width, int32_t height) override;
    virtual Texture* newHDRTexture(int32_t width, int32_t height) override;
    virtual Texture* newCubeMapTexture(int32_t widthHeight, bool mipmap, int32_t lowestMipLevel) override;
    
    // Utils
    virtual void ReadPixels(std::vector<uint8_t>& data, int width, int height) override;
    virtual void Scissor(int32_t x, int32_t y, int32_t width, int32_t height) override;
    virtual void DisableScissor() override;
    
    // Uniforms (2D Sprite Shader)
    virtual void SetUniformI(const std::string& name, int val) override;
    virtual void SetUniformF(const std::string& name, float v) override;
    virtual void SetUniformF(const std::string& name, float v1, float v2) override;
    virtual void SetUniformF(const std::string& name, float v1, float v2, float v3) override;
    virtual void SetUniformF(const std::string& name, float v1, float v2, float v3, float v4) override;
    virtual void SetUniformFv(const std::string& name, const std::vector<float>& values) override;
    virtual void SetUniformMatrix(const std::string& name, const std::vector<float>& value) override;
    virtual void SetUniformMatrix(const std::string& name, const Mat4& mat) override;
    virtual void SetTexture(const std::string& name, Texture* tex) override;
    
    // Uniforms (3D Model Shader)
    virtual void SetModelUniformI(const std::string& name, int val) override;
    virtual void SetModelUniformF(const std::string& name, float v) override;
    virtual void SetModelUniformF(const std::string& name, float v1, float v2) override;
    virtual void SetModelUniformF(const std::string& name, float v1, float v2, float v3) override;
    virtual void SetModelUniformF(const std::string& name, float v1, float v2, float v3, float v4) override;
    virtual void SetModelUniformFv(const std::string& name, const std::vector<float>& values) override;
    virtual void SetModelUniformMatrix(const std::string& name, const std::vector<float>& value) override;
    virtual void SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value) override;
    virtual void SetModelTexture(const std::string& name, Texture* t) override;
    
    // Uniforms (Shadow Map Shader)
    virtual void SetShadowMapUniformI(const std::string& name, int val) override;
    virtual void SetShadowMapUniformF(const std::string& name, float v) override;
    virtual void SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values) override;
    virtual void SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value) override;
    virtual void SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value) override;
    virtual void SetShadowMapTexture(const std::string& name, Texture* t) override;
    virtual void SetShadowFrameTexture(uint32_t i) override;
    virtual void SetShadowFrameCubeTexture(uint32_t i) override;
    
    // Draw Calls & Data Upload
    virtual void SetVertexData(const std::vector<float>& values) override;
    virtual void SetVertexDataArray(const std::vector<float>& values) override;
    virtual void SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values) override;
    virtual void SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values) override;
    
    virtual void RenderQuad() override;
    virtual void RenderQuadBatch(int32_t vertexCount) override;
    virtual void RenderElements(PrimitiveMode mode, int count, int offset) override;
    virtual void RenderShadowMapElements(PrimitiveMode mode, int count, int offset) override;
    
    // Environment/Cubemap rendering
    virtual void RenderCubeMap(Texture* envTexture, Texture* cubeTexture) override;
    virtual void RenderFilteredCubeMap(int32_t distribution, Texture* cubeTexture, Texture* filteredTexture, int32_t mipmapLevel, int32_t sampleCount, float roughness) override;
    virtual void RenderLUT(int32_t distribution, Texture* cubeTexture, Texture* lutTexture, int32_t sampleCount) override;
    
    // Math Helpers
    virtual Mat4 PerspectiveProjectionMatrix(float angle, float aspect, float near, float far) override;
    virtual Mat4 OrthographicProjectionMatrix(float left, float right, float bottom, float top, float near, float far) override;
    
    virtual bool NewWorkerThread() override { return false; }
    virtual void SetVSync() override;
    
private:
    // Private helper functions
    uint32_t MapBlendEquation(BlendEquation i);
    uint32_t MapBlendFunction(BlendFunc i);
    uint32_t MapPrimitiveMode(PrimitiveMode i);
    
    void SetDepthTest(bool depthTest);
    void SetDepthMask(bool depthMask);
    void SetFrontFace(bool invertFrontFace);
    void SetCullFace(bool doubleSided);
    void SetBlending(BlendEquation eq, BlendFunc src, BlendFunc dst);
    
    // Shader compilation/linking
    std::unique_ptr<ShaderProgram_GLES31> compileShaderProgram(
        const std::string& vertSrc, 
        const std::string& fragSrc, 
        const std::string& geoSrc, 
        const std::string& id,
        bool crashWhenFail
    );
    
    GLuint compileShader(GLenum shaderType, const std::string& src);
    GLuint linkProgram(const std::vector<GLuint>& shaders);
    
    // Model shader initialization
    void InitModelShader();
    
private:
    // Renderer state (ported from Go)
    struct GLState {
        bool depthTest = false;
        bool depthMask = false;
        bool invertFrontFace = false;
        bool doubleSided = false;
        BlendEquation blendEquation = BlendEquation::Add;
        BlendFunc blendSrc = BlendFunc::SrcAlpha;
        BlendFunc blendDst = BlendFunc::OneMinusSrcAlpha;
        bool useUV = false;
        bool useNormal = false;
        bool useTangent = false;
        bool useVertColor = false;
        bool useJoint0 = false;
        bool useJoint1 = false;
        bool useOutlineAttribute = false;
    } state;
    
    // Buffers and objects
    GLuint vao = 0;
    GLuint fbo = 0;
    GLuint fbo_texture = 0;
    GLuint rbo_depth = 0;
    
    // MSAA rendering
    GLuint fbo_f = 0;
    Texture_GLES31* fbo_f_texture = nullptr;
    
    // Shadow Map
    GLuint fbo_shadow = 0;
    GLuint fbo_shadow_cube_texture = 0;
    GLuint fbo_env = 0;
    
    // Postprocessing FBOs
    std::vector<GLuint> fbo_pp;
    std::vector<GLuint> fbo_pp_texture;
    
    // Post-processing shaders
    GLuint postVertBuffer = 0;
    std::vector<std::unique_ptr<ShaderProgram_GLES31>> postShaderSelect;
    
    // Shader and vertex data for primitive rendering
    std::unique_ptr<ShaderProgram_GLES31> spriteShader;
    GLuint vertexBuffer = 0;
    GLuint vertexBufferBatch = 0;
    
    // Shader and index data for 3D model rendering
    std::unique_ptr<ShaderProgram_GLES31> shadowMapShader;
    std::unique_ptr<ShaderProgram_GLES31> modelShader;
    std::unique_ptr<ShaderProgram_GLES31> panoramaToCubeMapShader;
    std::unique_ptr<ShaderProgram_GLES31> cubemapFilteringShader;
    
    GLuint modelVertexBuffer[2] = {0, 0};
    GLuint modelIndexBuffer[2] = {0, 0};
    
    // Configuration
    bool enableModel = false;
    bool enableShadow = false;
    
    // Screen dimensions (from sys)
    int32_t screenWidth = 0;
    int32_t screenHeight = 0;
    int32_t msaa = 0;
    
    // External shader sources
    std::string vertShaderSource;
    std::string fragShaderSource;
    std::string modelVertShaderSource;
    std::string modelFragShaderSource;
    std::string shadowVertShaderSource;
    std::string shadowFragShaderSource;
    std::string shadowGeoShaderSource;
    std::string identVertShaderSource;
    std::string identFragShaderSource;
    std::string panoramaToCubeMapFragShaderSource;
    std::string cubemapFilteringFragShaderSource;
};