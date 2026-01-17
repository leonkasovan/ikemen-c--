//========================================================================
// Ikemen "Fighting Engine" Port for C++
// Renderer Factory Implementation
// Copyright (c) 2026 leonkasovan@gmail.com
//========================================================================

#include "Renderer.h"

#ifdef USE_GLES
#include "RendererOpenGLES.h"
#else
#include "RendererOpenGL.h"
#endif

#include <iostream>

IRenderer* Renderer::Create() {
#ifdef USE_GLES
    return new Renderer_GLES();
#else
    return new Renderer_GL();
#endif
}