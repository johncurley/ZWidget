#include "haiku_display_backend.h"
#include "haiku_display_window.h"
#include <stdexcept>

HaikuDisplayBackend::HaikuDisplayBackend()
{
	// Initialize HaikuOS backend
	// For now, this is a stub implementation
	UIScale = 1.0;
}

HaikuDisplayBackend::~HaikuDisplayBackend()
{
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
	// Stub: return a default screen size
	// In a real implementation, this would query the Haiku screen dimensions
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
