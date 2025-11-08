#pragma once

#include <unordered_map>
#include <zwidget/window/window.h>
#include <zwidget/window/haikunativehandle.h>

#ifdef __HAIKU__
#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Screen.h>
#include <Bitmap.h>
#include <Clipboard.h>
#include <MessageRunner.h>
#include <Cursor.h>
#endif

class HaikuDisplayWindow : public DisplayWindow
{
public:
	HaikuDisplayWindow(DisplayWindowHost* windowHost, bool popupWindow, HaikuDisplayWindow* owner, RenderAPI renderAPI, double uiscale);
	~HaikuDisplayWindow();

	void SetWindowTitle(const std::string& text) override;
	void SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images) override;
	void SetWindowFrame(const Rect& box) override;
	void SetClientFrame(const Rect& box) override;
	void Show() override;
	void ShowFullscreen() override;
	void ShowMaximized() override;
	void ShowMinimized() override;
	void ShowNormal() override;
	bool IsWindowFullscreen() override;
	void Hide() override;
	void Activate() override;
	void ShowCursor(bool enable) override;
	void LockKeyboard() override;
	void UnlockKeyboard() override;
	void LockCursor() override;
	void UnlockCursor() override;
	void CaptureMouse() override;
	void ReleaseMouseCapture() override;
	void Update() override;
	bool GetKeyState(InputKey key) override;
	void SetCursor(StandardCursor cursor, std::shared_ptr<CustomCursor> custom) override;

	Rect GetWindowFrame() const override;
	Size GetClientSize() const override;
	int GetPixelWidth() const override;
	int GetPixelHeight() const override;
	double GetDpiScale() const override;

	void PresentBitmap(int width, int height, const uint32_t* pixels) override;

	void SetBorderColor(uint32_t bgra8) override;
	void SetCaptionColor(uint32_t bgra8) override;
	void SetCaptionTextColor(uint32_t bgra8) override;

	std::string GetClipboardText() override;
	void SetClipboardText(const std::string& text) override;

	Point MapFromGlobal(const Point& pos) const override;
	Point MapToGlobal(const Point& pos) const override;

	void* GetNativeHandle() override { return &Handle; }

	std::vector<std::string> GetVulkanInstanceExtensions() override;
	VkSurfaceKHR CreateVulkanSurface(VkInstance instance) override;

	static void ProcessEvents();
	static void RunLoop();
	static void ExitLoop();

	static void* StartTimer(int timeoutMilliseconds, std::function<void()> onTimer);
	static void StopTimer(void* timerID);

	DisplayWindowHost* WindowHost = nullptr;
	HaikuNativeHandle Handle;

	double UIScale = 1.0;
	bool CursorLocked = false;
	bool isFullscreen = false;
	int32 CurrentModifiers = 0;  // Track current modifier key state

	Rect WindowFrame;
	Size ClientSize;

	static bool ExitRunLoop;
	static std::unordered_map<void*, std::function<void()>> Timers;
	static unsigned long TimerIDs;
};
