#include <iostream>
#include <algorithm> // For std::transform
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>

#ifdef __APPLE__
#include <TargetConditionals.h>
#include <dispatch/dispatch.h>
#endif

// Include common ZWidget headers
#include <zwidget/core/widget.h> 
#include <zwidget/core/image.h>
#include <zwidget/core/theme.h>
#include <zwidget/window/window.h>
#include <zwidget/widgets/dropdown/dropdown.h>
#include <zwidget/widgets/textedit/textedit.h>
#include <zwidget/widgets/mainwindow/mainwindow.h>
#include <zwidget/widgets/layout/vboxlayout.h>
#include <zwidget/widgets/listview/listview.h>
#include <zwidget/widgets/imagebox/imagebox.h>
#include <zwidget/widgets/textlabel/textlabel.h>
#include <zwidget/widgets/pushbutton/pushbutton.h>
#include <zwidget/widgets/checkboxlabel/checkboxlabel.h>
#include <zwidget/widgets/tabwidget/tabwidget.h>

#include "picopng.h" // Local helper for PNG loading
#include "example_common.h" // Shared definitions like Backend, Theme, RenderAPI, and example() prototype
#include "example_ui.h"
#include <zwidget/core/resourcedata.h>

// ************************************************************
// Generic example function
// ************************************************************

int example(Backend backend, Theme theme, RenderAPI renderApi)
{
	std::cout << "example: entered" << std::endl;
	// just for testing themes
	std::cout << "example: setting theme" << std::endl;
	switch (theme)
	{
		case Theme::Default: WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>()); break;
		case Theme::Dark:    WidgetTheme::SetTheme(std::make_unique<DarkWidgetTheme>()); break;
		case Theme::Light:   WidgetTheme::SetTheme(std::make_unique<LightWidgetTheme>()); break;
	}
	std::cout << "example: theme set" << std::endl;

	// just for testing backends
	std::cout << "example: setting display backend" << std::endl;
	switch (backend)
	{
		case Backend::Default: DisplayBackend::Set(DisplayBackend::TryCreateBackend()); 
		break;
		case Backend::Win32: DisplayBackend::Set(DisplayBackend::TryCreateWin32());   
		break;
		case Backend::SDL2: DisplayBackend::Set(DisplayBackend::TryCreateSDL2());    
		break;
		case Backend::X11: DisplayBackend::Set(DisplayBackend::TryCreateX11());     
		break;
		case Backend::Wayland: DisplayBackend::Set(DisplayBackend::TryCreateWayland()); 
		break;
    case Backend::Metal: DisplayBackend::Set(DisplayBackend::TryCreateCocoa());   
		break;
	}
	std::cout << "example: display backend set" << std::endl;

	std::cout << "example: creating launcher window" << std::endl;
	RenderAPI chosenRenderApi = renderApi == RenderAPI::Unspecified ? RenderAPI::Metal : renderApi;
	auto launcher = std::make_unique<LauncherWindow>(chosenRenderApi);
	std::cout << "example: launcher window created, launcher->DispWindow.get() = " << launcher->DispWindow.get() << std::endl;
	launcher->SetFrameGeometry(100.0, 100.0, 615.0, 668.0);
	launcher->Show();
	launcher->Repaint();

	DisplayWindow::RunLoop();

	launcher.reset();

	return 0;
}


// ************************************************************
// Platform-specific entry point for WIN32
// ************************************************************

#ifdef WIN32

#include <filesystem>

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	SetProcessDPIAware();
	example(Backend::Default, Theme::Default, RenderAPI::Bitmap);
}

#endif

#if !defined(WIN32) 

int main(int argc, const char** argv)
{
    Backend backend = Backend::Default;
    Theme theme = Theme::Default;
    RenderAPI renderApi = RenderAPI::Unspecified;

    for (auto i = 1; i < argc; i++)
    {
        std::string s = argv[i];
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c){ return std::tolower(c); });

        if (s == "light") { theme = Theme::Light; continue; }
        if (s == "dark")  { theme = Theme::Dark;  continue; }

        if (s == "sdl2")    { backend = Backend::SDL2;    continue; }
        if (s == "x11")     { backend = Backend::X11;     continue; }
        if (s == "wayland") { backend = Backend::Wayland; continue; }
        if (s == "metal")   { backend = Backend::Metal; renderApi = RenderAPI::Metal; continue; }
        if (s == "opengl")  { renderApi = RenderAPI::OpenGL;  continue; }
        if (s == "bitmap")  { renderApi = RenderAPI::Bitmap;  continue; }
    }

    example(backend, theme, renderApi);

    return 0;
}

#endif
