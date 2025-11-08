#pragma once

#include "window/window.h"

class HaikuDisplayBackend : public DisplayBackend
{
public:
	HaikuDisplayBackend();
	~HaikuDisplayBackend();

	std::unique_ptr<DisplayWindow> Create(DisplayWindowHost* windowHost, bool popupWindow, DisplayWindow* owner, RenderAPI renderAPI) override;
	void ProcessEvents() override;
	void RunLoop() override;
	void ExitLoop() override;

	void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer) override;
	void StopTimer(void* timerID) override;

	Size GetScreenSize() override;

	bool IsHaiku() override { return true; }

private:
	double UIScale = 1.0;
};
