#pragma once

#include <string>
#include <vector>
#include <memory>

#include <zwidget/core/image.h>
#include <zwidget/core/font.h>
#include <zwidget/window/window.h>
#include <zwidget/core/resourcedata.h>

enum class Backend
{
	Default, Win32, SDL2, X11, Wayland, Metal
};

enum class Theme
{
	Default, Light, Dark
};

// Forward declaration of example function (defined in example.cpp)
int example(Backend backend, Theme theme, RenderAPI renderApi);
