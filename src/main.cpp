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
#include <iostream>
#include "embededFont.h"
#include "renderer/RendererInterfaces.h"
#include "renderer/Renderer.h"

static void error_callback(int error, const char* description) {
	std::cerr << "Error: " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// Returns true and fills major/minor with max supported version
bool getMaxOpenGLVersion(int* major, int* minor) {
    // Don't show the probe window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    // Create dummy window with NO version hints
    // GLFW will create highest available compatibility context
    GLFWwindow* probe = glfwCreateWindow(1, 1, "", NULL, NULL);
    if (!probe) {
        return false;
    }
    glfwMakeContextCurrent(probe);
    
    // Load GLAD/GLES for the probe context
#ifdef GLAD_GLES2_IMPLEMENTATION
    if (!gladLoadGLES2(glfwGetProcAddress)) {
        std::cerr << "Failed to load GLES2 in probe context" << std::endl;
        glfwDestroyWindow(probe);
        glfwDefaultWindowHints();
        return false;
    }
#else
    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to load GL in probe context" << std::endl;
        glfwDestroyWindow(probe);
        glfwDefaultWindowHints();
        return false;
    }
#endif
    
    // Query the version we actually got
    glGetIntegerv(GL_MAJOR_VERSION, major);
    glGetIntegerv(GL_MINOR_VERSION, minor);
    
    // Cleanup
    glfwDestroyWindow(probe);
    // Reset hints for next window
    glfwDefaultWindowHints();    
    return true;
}

int main(void) {
	int width = 640, height = 480;
	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	// Probe for OpenGL max version
    int maxMajor, maxMinor;
	std::cout << "getMaxOpenGLVersion called" << std::endl;
    if (getMaxOpenGLVersion(&maxMajor, &maxMinor)) {
        std::cout << "Using OpenGL: " << maxMajor << "." << maxMinor << std::endl;
    } else {
		std::cout << "Failed to probe OpenGL version, defaulting to 3.3" << std::endl;
		maxMajor = 3;
		maxMinor = 3;
	}

	// Set GLFW hints for Wayland (optional, uncomment if needed)
	// glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR);
	// glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_PREFER_LIBDECOR);

#ifdef USE_GLES
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, maxMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, maxMinor);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
#else
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, maxMajor);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, maxMinor);
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
