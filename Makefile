# Detect platform
ifeq ($(OS),Windows_NT)
	PLATFORM = WINDOWS
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		PLATFORM = MACOS
	else
		PLATFORM = LINUX
		ARCH := $(shell uname -m)
	endif
endif

CC = gcc
CFLAGS = -Ideps/glfw-3.5/include -Ideps/glfw-3.5/deps -Wall
CXX = g++
CXXFLAGS = -Isrc -Ideps/glfw-3.5/include -Ideps/glfw-3.5/deps -Wall -Wno-narrowing
RM = rm -f
BUILD_DIR = build
TARGET = ikemen

SRC = src/main.cpp \
	  src/renderer/Renderer.cpp

GLFW_DIR = deps/glfw-3.5/src
GLFW_SRC = $(GLFW_DIR)/context.c \
		   $(GLFW_DIR)/init.c \
		   $(GLFW_DIR)/input.c \
		   $(GLFW_DIR)/monitor.c \
		   $(GLFW_DIR)/vulkan.c \
		   $(GLFW_DIR)/window.c \
		   $(GLFW_DIR)/osmesa_context.c

ifeq ($(PLATFORM),WINDOWS)
	GLFW_SRC += $(GLFW_DIR)/win32_init.c \
				$(GLFW_DIR)/win32_joystick.c \
				$(GLFW_DIR)/win32_monitor.c \
				$(GLFW_DIR)/win32_time.c \
				$(GLFW_DIR)/win32_thread.c \
				$(GLFW_DIR)/win32_window.c \
				$(GLFW_DIR)/win32_module.c \
				$(GLFW_DIR)/wgl_context.c \
				$(GLFW_DIR)/egl_context.c \
				$(GLFW_DIR)/platform.c

	CFLAGS += -D_WIN32 -D_GLFW_WIN32
	CXXFLAGS += -D_WIN32 -D_GLFW_WIN32
	LDFLAGS += -lgdi32 -luser32 -lkernel32
else ifeq ($(PLATFORM),MACOS)
	GLFW_SRC += $(GLFW_DIR)/cocoa_init.m \
				$(GLFW_DIR)/cocoa_joystick.m \
				$(GLFW_DIR)/cocoa_monitor.m \
				$(GLFW_DIR)/cocoa_window.m \
				$(GLFW_DIR)/cocoa_time.c \
				$(GLFW_DIR)/posix_thread.c \
				$(GLFW_DIR)/nsgl_context.m \
				$(GLFW_DIR)/egl_context.c

	CFLAGS += -D_GLFW_COCOA -x objective-c
	CXXFLAGS += -D_GLFW_COCOA -x objective-c
	LDFLAGS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
	GLFW_SRC += $(GLFW_DIR)/linux_joystick.c \
				$(GLFW_DIR)/posix_time.c \
				$(GLFW_DIR)/posix_thread.c \
				$(GLFW_DIR)/xkb_unicode.c \
				$(GLFW_DIR)/egl_context.c

# Linux-specific X11 dependencies
# GLFW_SRC += $(GLFW_DIR)/x11_window.c \
# 			$(GLFW_DIR)/x11_init.c \
# 			$(GLFW_DIR)/x11_monitor.c \
# 			$(GLFW_DIR)/glx_context.c \
# 			$(GLFW_DIR)/posix_poll.c \
# 			$(GLFW_DIR)/posix_module.c \
# 			$(GLFW_DIR)/platform.c
# CFLAGS += -D_GLFW_X11 -D_GNU_SOURCE
# LDFLAGS += -lX11 -lXrandr -lXi -lXcursor -lm -lXinerama -ldl -lrt

# Wayland-specific dependencies (uncomment if needed)
# GLFW_SRC += $(GLFW_DIR)/wl_init.c \
# 			$(GLFW_DIR)/wl_monitor.c \
# 			$(GLFW_DIR)/posix_module.c \
# 			$(GLFW_DIR)/posix_poll.c \
# 			$(GLFW_DIR)/platform.c \
# 			$(GLFW_DIR)/wl_window.c
# CFLAGS += -D_GLFW_WAYLAND -D_GNU_SOURCE
# CXXFLAGS += -D_GLFW_WAYLAND -D_GNU_SOURCE
# LDFLAGS += -lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon -lm -ldl -lrt

# KMS/DRM-specific dependencies (uncomment if needed)
# GLFW_SRC += $(GLFW_DIR)/kmsdrm_window.c \
# 			$(GLFW_DIR)/kmsdrm_init.c \
# 			$(GLFW_DIR)/kmsdrm_monitor.c \
# 			$(GLFW_DIR)/linux_keyboard.c \
# 			$(GLFW_DIR)/posix_poll.c \
# 			$(GLFW_DIR)/posix_module.c \
# 			$(GLFW_DIR)/platform.c
# CFLAGS += -D_GLFW_KMSDRM -D_GNU_SOURCE `pkg-config --cflags libdrm`
# LDFLAGS += -lm -ldl -lrt -lEGL `pkg-config --libs libdrm` `pkg-config --libs libgbm`

# SDL2-specific dependencies (uncomment if needed)
GLFW_SRC += $(GLFW_DIR)/sdl2_platform.c \
 			$(GLFW_DIR)/posix_poll.c \
 			$(GLFW_DIR)/posix_module.c \
 			$(GLFW_DIR)/platform.c
CFLAGS += -D_GLFW_SDL2 -D_GNU_SOURCE
CXXFLAGS += -D_GLFW_SDL2 -D_GNU_SOURCE
LDFLAGS += -lSDL2 -lpthread -ldl

endif

ifeq ($(ARCH),aarch64)
	CFLAGS += -DUSE_GLES
	CXXFLAGS += -DUSE_GLES
	SRC += src/renderer/RendererOpenGLES.cpp
else
	CFLAGS += -DUSE_GL
	CXXFLAGS += -DUSE_GL
	SRC += src/renderer/RendererOpenGL.cpp
endif

OBJS = $(addprefix $(BUILD_DIR)/,$(SRC:.cpp=.o)) $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(filter %.c,$(GLFW_SRC)))) $(addprefix $(BUILD_DIR)/,$(patsubst %.m,%.o,$(filter %.m,$(GLFW_SRC))))

# Default build is release
all: release

debug: CXXFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

release: CXXFLAGS += -O2 -DNDEBUG
release: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(addprefix $(BUILD_DIR)/,$(dir $(SRC) $(GLFW_SRC)))

$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CXX) -o $@ $(filter %.o,$(OBJS)) $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.m
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) -r $(BUILD_DIR)

.PHONY: all debug release clean
