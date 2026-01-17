#pragma once

#include "RenderInterfaces.h"
#include <glad/gl.h> // Or your preferred GL loader
#include <map>
#include <string>
#include <vector>

// ------------------------------------------------------------------
// Shader Helper
// ------------------------------------------------------------------
class ShaderProgram_GL {
public:
	GLuint program = 0;
	std::map<std::string, GLint> attributes;
	std::map<std::string, GLint> uniforms;
	std::map<std::string, GLint> textureUnits;

	ShaderProgram_GL() = default;
	~ShaderProgram_GL();

	bool Load(const std::string& vertSrc, const std::string& fragSrc, const std::string& geoSrc, const std::string& id);
	void RegisterAttributes(const std::vector<std::string>& names);
	void RegisterUniforms(const std::vector<std::string>& names);
	void RegisterTextures(const std::vector<std::string>& names);
	
	void Use() const;
	GLint GetLoc(const std::string& name);
};

// ------------------------------------------------------------------
// Texture Implementation
// ------------------------------------------------------------------
class Texture_GL33 : public Texture {
public:
	GLuint handle = 0;
	int32_t width = 0;
	int32_t height = 0;
	int32_t depth = 0; // Bit depth or layer count depending on context
	bool filter = false;
	GLenum target = GL_TEXTURE_2D; // Can be GL_TEXTURE_2D_ARRAY or GL_TEXTURE_CUBE_MAP

	Texture_GL33(int32_t w, int32_t h, int32_t d, bool f);
	~Texture_GL33() override;

	// Interface Overrides
	void SetPaletteLayer(const std::vector<uint8_t>& data, int32_t layer) override;
	void SetData(const std::vector<uint8_t>& data) override;
	void SetSubData(const std::vector<uint8_t>& data, int32_t x, int32_t y, int32_t w, int32_t h) override;
	void SetDataG(const std::vector<uint8_t>& data, TextureSamplingParam mag, TextureSamplingParam min, TextureSamplingParam ws, TextureSamplingParam wt) override;
	void SetPixelData(const std::vector<float>& data) override;
	
	void CopyData(Texture* src) override;
	bool IsValid() override;
	int32_t GetWidth() override;
	int32_t GetHeight() override;
	
	void SavePNG(const std::string& filename, const std::vector<uint32_t>& pal) override;

	// Helpers
	GLenum MapInternalFormat(int32_t depth);
	GLint MapSamplingParam(TextureSamplingParam param);
};

// ------------------------------------------------------------------
// Renderer Implementation
// ------------------------------------------------------------------
class Renderer_GL33 : public Renderer {
private:
	// Core Resources
	GLuint vao = 0;
	GLuint fbo = 0;
	GLuint fbo_texture = 0;
	GLuint rbo_depth = 0;
	
	// MSAA support
	GLuint fbo_f = 0;
	Texture_GL33* fbo_f_texture = nullptr;

	// Shadow mapping
	GLuint fbo_shadow = 0;
	GLuint fbo_shadow_cube_texture = 0;
	GLuint fbo_env = 0;

	// Post Processing
	std::vector<GLuint> fbo_pp;
	std::vector<GLuint> fbo_pp_texture;
	GLuint postVertBuffer = 0;
	std::vector<ShaderProgram_GL*> postShaderSelect;

	// Shaders
	ShaderProgram_GL* spriteShader = nullptr;
	ShaderProgram_GL* modelShader = nullptr;
	ShaderProgram_GL* shadowMapShader = nullptr;
	ShaderProgram_GL* panoramaToCubeMapShader = nullptr;
	ShaderProgram_GL* cubemapFilteringShader = nullptr;

	// Buffers
	GLuint vertexBuffer = 0;
	GLuint vertexBufferBatch = 0;
	GLuint modelVertexBuffer[2] = {0, 0};
	GLuint modelIndexBuffer[2] = {0, 0};

	// State
	bool enableModel = false;
	bool enableShadow = false;
	int msaa = 0;
	int32_t scrWidth = 0;
	int32_t scrHeight = 0;

	uint64_t nDrawCall = 0;

	// Cached GL State to avoid redundant calls
	struct {
		bool depthTest = false;
		bool depthMask = true;
		bool invertFrontFace = false;
		bool doubleSided = false;
		BlendEquation blendEquation = BlendEquation::Add;
		BlendFunc blendSrc = BlendFunc::One;
		BlendFunc blendDst = BlendFunc::Zero;
		
		// Pipeline flags
		bool useUV = false;
		bool useNormal = false;
		bool useTangent = false;
		bool useVertColor = false;
		bool useJoint0 = false;
		bool useJoint1 = false;
		bool useOutlineAttribute = false;
	} state;

public:
	Renderer_GL33();
	~Renderer_GL33() override;

	std::string GetName() override;
	
	void Init() override;
	void Close() override;
	
	void BeginFrame(bool clearColor) override;
	void EndFrame() override;
	void Await() override;

	bool IsModelEnabled() override;
	bool IsShadowEnabled() override;
	bool NewWorkerThread() override;
	void SetVSync() override;

	// Pipeline & State
	void BlendReset() override;
	void SetPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst) override;
	void SetPipelineBatch() override;
	void ReleasePipeline() override;

	void prepareShadowMapPipeline(uint32_t bufferIndex) override;
	void setShadowMapPipeline(bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, uint32_t numVertices, uint32_t vertAttrOffset) override;
	void ReleaseShadowPipeline() override;

	void prepareModelPipeline(uint32_t bufferIndex, Environment* env) override;
	void SetModelPipeline(BlendEquation eq, BlendFunc src, BlendFunc dst, bool depthTest, bool depthMask, bool doubleSided, bool invertFrontFace, bool useUV, bool useNormal, bool useTangent, bool useVertColor, bool useJoint0, bool useJoint1, bool useOutlineAttribute, uint32_t numVertices, uint32_t vertAttrOffset) override;
	void SetMeshOulinePipeline(bool invertFrontFace, float meshOutline) override;
	void ReleaseModelPipeline() override;

	// Texture Factories
	Texture* newTexture(int32_t width, int32_t height, int32_t depth, bool filter) override;
	Texture* newPaletteTexture() override;
	Texture* newPaletteTextureArray(int32_t layers) override;
	Texture* newModelTexture(int32_t width, int32_t height, int32_t depth, bool filter) override;
	Texture* newDataTexture(int32_t width, int32_t height) override;
	Texture* newHDRTexture(int32_t width, int32_t height) override;
	Texture* newCubeMapTexture(int32_t widthHeight, bool mipmap, int32_t lowestMipLevel) override;

	// Utils
	void ReadPixels(std::vector<uint8_t>& data, int width, int height) override;
	void Scissor(int32_t x, int32_t y, int32_t width, int32_t height) override;
	void DisableScissor() override;

	// Uniforms (Sprite)
	void SetUniformI(const std::string& name, int val) override;
	void SetUniformF(const std::string& name, float v) override;
	void SetUniformF(const std::string& name, float v1, float v2) override;
	void SetUniformF(const std::string& name, float v1, float v2, float v3) override;
	void SetUniformF(const std::string& name, float v1, float v2, float v3, float v4) override;
	void SetUniformFv(const std::string& name, const std::vector<float>& values) override;
	void SetUniformMatrix(const std::string& name, const std::vector<float>& value) override;
	void SetTexture(const std::string& name, Texture* tex) override;

	// Uniforms (Model)
	void SetModelUniformI(const std::string& name, int val) override;
	void SetModelUniformF(const std::string& name, float v) override;
	void SetModelUniformF(const std::string& name, float v1, float v2) override;
	void SetModelUniformF(const std::string& name, float v1, float v2, float v3) override;
	void SetModelUniformF(const std::string& name, float v1, float v2, float v3, float v4) override;
	void SetModelUniformFv(const std::string& name, const std::vector<float>& values) override;
	void SetModelUniformMatrix(const std::string& name, const std::vector<float>& value) override;
	void SetModelUniformMatrix3(const std::string& name, const std::vector<float>& value) override;
	void SetModelTexture(const std::string& name, Texture* t) override;

	// Uniforms (Shadow)
	void SetShadowMapUniformI(const std::string& name, int val) override;
	void SetShadowMapUniformF(const std::string& name, float v) override;
	void SetShadowMapUniformFv(const std::string& name, const std::vector<float>& values) override;
	void SetShadowMapUniformMatrix(const std::string& name, const std::vector<float>& value) override;
	void SetShadowMapUniformMatrix3(const std::string& name, const std::vector<float>& value) override;
	void SetShadowMapTexture(const std::string& name, Texture* t) override;
	void SetShadowFrameTexture(uint32_t i) override;
	void SetShadowFrameCubeTexture(uint32_t i) override;

	// Draw
	void SetVertexData(const std::vector<float>& values) override;
	void SetVertexDataArray(const std::vector<float>& values) override;
	void SetModelVertexData(uint32_t bufferIndex, const std::vector<uint8_t>& values) override;
	void SetModelIndexData(uint32_t bufferIndex, const std::vector<uint32_t>& values) override;

	void RenderQuad() override;
	void RenderQuadBatch(int32_t vertexCount) override;
	void RenderElements(PrimitiveMode mode, int count, int offset) override;
	void RenderShadowMapElements(PrimitiveMode mode, int count, int offset) override;

	void RenderCubeMap(Texture* envTexture, Texture* cubeTexture) override;
	void RenderFilteredCubeMap(int32_t distribution, Texture* cubeTexture, Texture* filteredTexture, int32_t mipmapLevel, int32_t sampleCount, float roughness) override;
	void RenderLUT(int32_t distribution, Texture* cubeTexture, Texture* lutTexture, int32_t sampleCount) override;

	// Math
	Mat4 PerspectiveProjectionMatrix(float angle, float aspect, float near, float far) override;
	Mat4 OrthographicProjectionMatrix(float left, float right, float bottom, float top, float near, float far) override;
	void SetUniformMatrix(const std::string& name, const Mat4& mat) override;

private:
	// Helpers
	GLenum MapBlendEquation(BlendEquation eq);
	GLenum MapBlendFunc(BlendFunc func);
	GLenum MapPrimitiveMode(PrimitiveMode mode);
	
	// State setters
	void SetDepthTest(bool enable);
	void SetDepthMask(bool mask);
	void SetFrontFace(bool invert);
	void SetCullFace(bool doubleSided);
	void SetBlending(BlendEquation eq, BlendFunc src, BlendFunc dst);
	
	// Shader Source Placeholders (Implementations would normally load these from files)
	std::string GetVertShaderSrc(); 
	std::string GetFragShaderSrc();
	// ... add others as needed
};