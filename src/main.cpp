//========================================================================
// Ikemen "Fighting Engine" Port for C++
// Copyright (c) 2026 leonkasovan@gmail.com
//========================================================================

// Define GLAD implementation only once in the entire project
#ifdef USE_GLES
#define GLAD_GLES2_IMPLEMENTATION
#include "renderer/RendererOpenGLES.h"
#else
#define GLAD_GL_IMPLEMENTATION
#include "renderer/RendererOpenGL.h"
#endif
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "embededFont.h"
#include "renderer/RendererInterfaces.h"
#include "renderer/Renderer.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

static void error_callback(int error, const char* description) {
	fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main(void) {
	int width = 640, height = 480;
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	// Set GLFW hints for Wayland (optional, uncomment if needed)
	// glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR);
	// glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_PREFER_LIBDECOR);

#ifdef GLAD_GLES2_IMPLEMENTATION
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	GLFWwindow* window = glfwCreateWindow(width, height, "Ikemen Fighting Engine", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, key_callback);

	glfwMakeContextCurrent(window);
#ifdef GLAD_GLES2_IMPLEMENTATION
	gladLoadGLES2(glfwGetProcAddress);
#else
	gladLoadGL(glfwGetProcAddress);
#endif
	glfwSwapInterval(1);

	IRenderer* renderer = Renderer::Create();
	renderer->PrintInfo();
	renderer->PrintCapabilities();
	EmbedFontCtx* font = embedFontCreate(width, height);

	while (!glfwWindowShouldClose(window)) {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		embedFontBindState(font);
		embedFontSetColor(font, 1.0f, 1.0f, 1.0f, 1.0f);
		embedFontDrawText(font, "This is embededFont using OpenGL shader.", 10.0f, 10.0f, 12.0f, 12.0f);
		embedFontSetColor(font, 0.2f, 0.8f, 1.0f, 1.0f);
		embedFontDrawText(font, "Try me at any window size!", 10.0f, 50.0f, 18.0f, 18.0f);
		embedFontSetColor(font, 1.0f, 1.0f, 0.1f, 1.0f);
		embedFontDrawText(font, "Text scales: 8x8", 10.0f, 90.0f, 8.0f, 8.0f);
		embedFontDrawText(font, "Text scales: 16x16", 10.0f, 110.0f, 16.0f, 16.0f);
		embedFontDrawText(font, "Text scales: 32x32", 10.0f, 140.0f, 32.0f, 32.0f);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	renderer->Close();
	embedFontDestroy(font);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

//! [code]
