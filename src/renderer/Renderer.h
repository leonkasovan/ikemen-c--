//========================================================================
// Ikemen "Fighting Engine" Port for C++
// Renderer Factory Class
// Copyright (c) 2026 leonkasovan@gmail.com
//========================================================================

#ifndef RENDERER_H
#define RENDERER_H

#include "RendererInterfaces.h"

class Renderer {
public:
    // Factory method to create the appropriate renderer implementation
    static IRenderer* Create();
    
    // Prevent instantiation
    Renderer() = delete;
    ~Renderer() = delete;
};

#endif // RENDERER_H