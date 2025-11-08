#include "haiku_display_backend.h"
#include "haiku_display_window.h"
#include <stdexcept>

#ifdef __HAIKU__
#include <interface/Screen.h>
#include <app/Application.h>
#endif

HaikuDisplayBackend::HaikuDisplayBackend()
{
#ifdef __HAIKU__
	// Initialize HaikuOS backend
	// Create BApplication if it doesn't exist
	// This is required for BWindow to work properly
	if (!be_app)
	{
		// Create BApplication with a proper signature
		// Note: This creates be_app global which BWindow/BView need
		new BApplication("application/x-vnd.ZWidget");
	}

	UIScale = 1.0;

	// Get DPI information from the screen
	BScreen screen;
	if (screen.IsValid())
	{
		// Haiku uses a default 96 DPI, but we could query the actual screen DPI
		// For now, we'll use 1.0 scale
	}
#else
	UIScale = 1.0;
#endif
}

HaikuDisplayBackend::~HaikuDisplayBackend()
{
#ifdef __HAIKU__
	// Properly quit the BApplication if we created it
	if (be_app)
	{
		if (be_app->Lock())
		{
			be_app->Quit();
		}
		// Note: be_app will be deleted by BApplication::Quit()
		// Don't delete it manually
	}
#endif
}

std::unique_ptr<DisplayWindow> HaikuDisplayBackend::Create(DisplayWindowHost* windowHost, bool popupWindow, DisplayWindow* owner, RenderAPI renderAPI)
{
	return std::make_unique<HaikuDisplayWindow>(windowHost, popupWindow, static_cast<HaikuDisplayWindow*>(owner), renderAPI, UIScale);
}

void HaikuDisplayBackend::ProcessEvents()
{
	HaikuDisplayWindow::ProcessEvents();
}

void HaikuDisplayBackend::RunLoop()
{
	HaikuDisplayWindow::RunLoop();
}

void HaikuDisplayBackend::ExitLoop()
{
	HaikuDisplayWindow::ExitLoop();
}

Size HaikuDisplayBackend::GetScreenSize()
{
#ifdef __HAIKU__
	BScreen screen;
	if (screen.IsValid())
	{
		BRect frame = screen.Frame();
		return Size(frame.Width() + 1, frame.Height() + 1);
	}
#endif
	// Fallback: return a default screen size
	return Size(1920, 1080);
}

void* HaikuDisplayBackend::StartTimer(int timeoutMilliseconds, std::function<void()> onTimer)
{
	return HaikuDisplayWindow::StartTimer(timeoutMilliseconds, std::move(onTimer));
}

void HaikuDisplayBackend::StopTimer(void* timerID)
{
	HaikuDisplayWindow::StopTimer(timerID);
}
