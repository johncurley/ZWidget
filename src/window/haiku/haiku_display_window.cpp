#include "haiku_display_window.h"
#include <stdexcept>
#include <cstring>
#include <dlfcn.h>

#ifdef __HAIKU__
#include <interface/View.h>
#include <interface/Window.h>
#include <app/Application.h>
#include <app/Messenger.h>
#include <app/Message.h>
#include <interface/Screen.h>
#include <interface/Bitmap.h>
#include <support/Beep.h>
#include <os/kernel/OS.h>
#endif

bool HaikuDisplayWindow::ExitRunLoop = false;
std::unordered_map<void*, std::function<void()>> HaikuDisplayWindow::Timers;
unsigned long HaikuDisplayWindow::TimerIDs = 0;

#ifdef __HAIKU__

// Custom BView class to handle rendering and events
class ZWidgetView : public BView
{
public:
	ZWidgetView(BRect frame, HaikuDisplayWindow* displayWindow)
		: BView(frame, "ZWidgetView", B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS),
		  DisplayWindow(displayWindow)
	{
	}

	virtual void AttachedToWindow() override
	{
		BView::AttachedToWindow();
		// Enable mouse tracking even when mouse buttons aren't pressed
		// This is critical for getting MouseMoved events
		SetEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}

	virtual void Draw(BRect updateRect) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			DisplayWindow->WindowHost->OnWindowPaint();
		}
	}

	virtual void FrameResized(float newWidth, float newHeight) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			DisplayWindow->ClientSize = Size(newWidth, newHeight);
			DisplayWindow->WindowHost->OnWindowGeometryChanged();
		}
	}

	virtual void MouseDown(BPoint where) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			// Determine which button was pressed
			InputKey button = InputKey::LeftMouse;
			BMessage* msg = Window()->CurrentMessage();
			if (msg)
			{
				int32 buttons = 0;
				if (msg->FindInt32("buttons", &buttons) == B_OK)
				{
					if (buttons & B_PRIMARY_MOUSE_BUTTON)
						button = InputKey::LeftMouse;
					else if (buttons & B_SECONDARY_MOUSE_BUTTON)
						button = InputKey::RightMouse;
					else if (buttons & B_TERTIARY_MOUSE_BUTTON)
						button = InputKey::MiddleMouse;
				}
			}
			DisplayWindow->WindowHost->OnWindowMouseDown(Point(where.x, where.y), button);
		}
	}

	virtual void MouseUp(BPoint where) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			// Determine which button was released
			InputKey button = InputKey::LeftMouse;
			BMessage* msg = Window()->CurrentMessage();
			if (msg)
			{
				int32 buttons = 0;
				if (msg->FindInt32("buttons", &buttons) == B_OK)
				{
					// Note: buttons field contains buttons STILL pressed after this release
					// We need to check which buttons changed
					int32 clicks = 0;
					msg->FindInt32("clicks", &clicks);

					// For now, assume the button that's NOT in buttons anymore
					// This is a simplification - proper implementation would track button state
					if (!(buttons & B_PRIMARY_MOUSE_BUTTON))
						button = InputKey::LeftMouse;
					else if (!(buttons & B_SECONDARY_MOUSE_BUTTON))
						button = InputKey::RightMouse;
					else if (!(buttons & B_TERTIARY_MOUSE_BUTTON))
						button = InputKey::MiddleMouse;
				}
			}
			DisplayWindow->WindowHost->OnWindowMouseUp(Point(where.x, where.y), button);
		}
	}

	virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			DisplayWindow->WindowHost->OnWindowMouseMove(Point(where.x, where.y));
		}
	}

	virtual void KeyDown(const char* bytes, int32 numBytes) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			// Get key code and modifiers from message
			BMessage* msg = Window()->CurrentMessage();
			if (msg)
			{
				int32 rawChar = 0;
				int32 modifiers = 0;
				msg->FindInt32("modifiers", &modifiers);

				if (msg->FindInt32("raw_char", &rawChar) == B_OK)
				{
					// Store modifiers in DisplayWindow for GetKeyState
					DisplayWindow->CurrentModifiers = modifiers;

					// Map Haiku key codes to InputKey
					InputKey inputKey = MapHaikuKeyToInputKey(rawChar);
					if (inputKey != InputKey::None)
					{
						DisplayWindow->WindowHost->OnWindowKeyDown(inputKey);
					}

					// Also check modifier keys themselves
					CheckModifierKeys(modifiers, true);
				}
			}

			// Also send character input for text
			if (numBytes > 0)
			{
				DisplayWindow->WindowHost->OnWindowKeyChar(std::string(bytes, numBytes));
			}
		}
	}

	virtual void KeyUp(const char* bytes, int32 numBytes) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			BMessage* msg = Window()->CurrentMessage();
			if (msg)
			{
				int32 rawChar = 0;
				int32 modifiers = 0;
				msg->FindInt32("modifiers", &modifiers);

				if (msg->FindInt32("raw_char", &rawChar) == B_OK)
				{
					// Update stored modifiers
					DisplayWindow->CurrentModifiers = modifiers;

					InputKey inputKey = MapHaikuKeyToInputKey(rawChar);
					if (inputKey != InputKey::None)
					{
						DisplayWindow->WindowHost->OnWindowKeyUp(inputKey);
					}

					// Also check modifier keys themselves
					CheckModifierKeys(modifiers, false);
				}
			}
		}
	}

	virtual void MouseWheel(BPoint where, float deltaX, float deltaY) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			// deltaY > 0 means scroll down, < 0 means scroll up
			if (deltaY > 0)
			{
				DisplayWindow->WindowHost->OnWindowMouseWheel(Point(where.x, where.y), InputKey::MouseWheelDown);
			}
			else if (deltaY < 0)
			{
				DisplayWindow->WindowHost->OnWindowMouseWheel(Point(where.x, where.y), InputKey::MouseWheelUp);
			}
		}
	}

	void CheckModifierKeys(int32 modifiers, bool down)
	{
		// Fire events for modifier keys
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			// Track shift, control, alt states
			if (down)
			{
				if (modifiers & B_SHIFT_KEY)
					DisplayWindow->WindowHost->OnWindowKeyDown(InputKey::Shift);
				if (modifiers & B_CONTROL_KEY)
					DisplayWindow->WindowHost->OnWindowKeyDown(InputKey::Control);
				if (modifiers & B_COMMAND_KEY)  // Alt/Command on Haiku
					DisplayWindow->WindowHost->OnWindowKeyDown(InputKey::Menu);
			}
			else
			{
				if (!(modifiers & B_SHIFT_KEY))
					DisplayWindow->WindowHost->OnWindowKeyUp(InputKey::Shift);
				if (!(modifiers & B_CONTROL_KEY))
					DisplayWindow->WindowHost->OnWindowKeyUp(InputKey::Control);
				if (!(modifiers & B_COMMAND_KEY))
					DisplayWindow->WindowHost->OnWindowKeyUp(InputKey::Menu);
			}
		}
	}

	static InputKey MapHaikuKeyToInputKey(int32 rawChar)
	{
		// Map Haiku B_* key constants to InputKey enum
		// This is a simplified mapping - add more keys as needed
		switch (rawChar)
		{
			case B_ESCAPE: return InputKey::Escape;
			case B_RETURN: return InputKey::Return;
			case B_SPACE: return InputKey::Space;
			case B_BACKSPACE: return InputKey::Back;
			case B_TAB: return InputKey::Tab;
			case B_LEFT_ARROW: return InputKey::Left;
			case B_RIGHT_ARROW: return InputKey::Right;
			case B_UP_ARROW: return InputKey::Up;
			case B_DOWN_ARROW: return InputKey::Down;
			case B_INSERT: return InputKey::Insert;
			case B_DELETE: return InputKey::Delete;
			case B_HOME: return InputKey::Home;
			case B_END: return InputKey::End;
			case B_PAGE_UP: return InputKey::Prior;
			case B_PAGE_DOWN: return InputKey::Next;
			case B_F1_KEY: return InputKey::F1;
			case B_F2_KEY: return InputKey::F2;
			case B_F3_KEY: return InputKey::F3;
			case B_F4_KEY: return InputKey::F4;
			case B_F5_KEY: return InputKey::F5;
			case B_F6_KEY: return InputKey::F6;
			case B_F7_KEY: return InputKey::F7;
			case B_F8_KEY: return InputKey::F8;
			case B_F9_KEY: return InputKey::F9;
			case B_F10_KEY: return InputKey::F10;
			case B_F11_KEY: return InputKey::F11;
			case B_F12_KEY: return InputKey::F12;
			default:
				// For alphanumeric keys, check ASCII range
				if (rawChar >= 'A' && rawChar <= 'Z')
					return static_cast<InputKey>(static_cast<uint32_t>(InputKey::A) + (rawChar - 'A'));
				if (rawChar >= '0' && rawChar <= '9')
					return static_cast<InputKey>(static_cast<uint32_t>(InputKey::_0) + (rawChar - '0'));
				return InputKey::None;
		}
	}

	HaikuDisplayWindow* DisplayWindow = nullptr;
};

// Custom BWindow class to handle window events
class ZWidgetWindow : public BWindow
{
public:
	ZWidgetWindow(BRect frame, const char* title, window_type type, uint32 flags, HaikuDisplayWindow* displayWindow)
		: BWindow(frame, title, type, flags),
		  DisplayWindow(displayWindow)
	{
	}

	virtual bool QuitRequested() override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			DisplayWindow->WindowHost->OnWindowClose();
		}
		return false; // Don't automatically quit, let the host decide
	}

	virtual void FrameMoved(BPoint newPosition) override
	{
		if (DisplayWindow)
		{
			BRect frame = Frame();
			DisplayWindow->WindowFrame = Rect::xywh(frame.left, frame.top, frame.Width(), frame.Height());
		}
	}

	virtual void WindowActivated(bool active) override
	{
		if (DisplayWindow && DisplayWindow->WindowHost)
		{
			if (active)
				DisplayWindow->WindowHost->OnWindowActivated();
			else
				DisplayWindow->WindowHost->OnWindowDeactivated();
		}
	}

	HaikuDisplayWindow* DisplayWindow = nullptr;
};

#endif

HaikuDisplayWindow::HaikuDisplayWindow(DisplayWindowHost* windowHost, bool popupWindow, HaikuDisplayWindow* owner, RenderAPI renderAPI, double uiscale)
	: WindowHost(windowHost), UIScale(uiscale)
{
#ifdef __HAIKU__
	// Create BWindow and BView
	BRect frame(100, 100, 900, 700); // Default 800x600 window at position 100,100

	window_type type = popupWindow ? B_FLOATING_WINDOW : B_TITLED_WINDOW;
	uint32 flags = B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE;

	if (!popupWindow)
	{
		flags |= B_AUTO_UPDATE_SIZE_LIMITS;
	}

	ZWidgetWindow* window = new ZWidgetWindow(frame, "ZWidget Window", type, flags, this);
	ZWidgetView* view = new ZWidgetView(window->Bounds(), this);

	window->AddChild(view);

	Handle.window = window;
	Handle.view = view;

	WindowFrame = Rect::xywh(frame.left, frame.top, frame.Width(), frame.Height());
	ClientSize = Size(frame.Width(), frame.Height());

	// Get DPI scale from screen
	BScreen screen(window);
	if (screen.IsValid())
	{
		// On Haiku, we can check the desktop color depth and resolution
		// For now, we'll keep the provided UIScale
	}
#else
	// Stub: In a real implementation, this would create a BWindow and BView
	WindowFrame = Rect::xywh(0, 0, 800, 600);
	ClientSize = Size(800, 600);
#endif
}

HaikuDisplayWindow::~HaikuDisplayWindow()
{
#ifdef __HAIKU__
	if (Handle.window)
	{
		Handle.window->Lock();
		Handle.window->Quit();
		Handle.window = nullptr;
		Handle.view = nullptr;
	}
#endif
}

void HaikuDisplayWindow::SetWindowTitle(const std::string& text)
{
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		Handle.window->SetTitle(text.c_str());
		Handle.window->Unlock();
	}
#endif
}

void HaikuDisplayWindow::SetWindowIcon(const std::vector<std::shared_ptr<Image>>& images)
{
	// Stub: Icon support would be implemented here
}

void HaikuDisplayWindow::SetWindowFrame(const Rect& box)
{
	WindowFrame = box;
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		Handle.window->MoveTo(box.x, box.y);
		Handle.window->ResizeTo(box.width - 1, box.height - 1); // ResizeTo uses width-1, height-1
		Handle.window->Unlock();
	}
#endif
}

void HaikuDisplayWindow::SetClientFrame(const Rect& box)
{
	WindowFrame = box;
	ClientSize = box.size();
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		// Adjust for window decorations (title bar, borders)
		float borderWidth = 0;
		float borderHeight = 0;
		Handle.window->GetDecoratorSettings(nullptr); // Not directly accessible, approximate
		// On Haiku, the decorator size is typically fixed
		// For titled windows, add approximate title bar height
		Handle.window->MoveTo(box.x, box.y);
		Handle.window->ResizeTo(box.width - 1, box.height - 1);
		Handle.window->Unlock();
	}
#endif
}

void HaikuDisplayWindow::Show()
{
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		Handle.window->Show();
		Handle.window->Unlock();
	}
#endif
}

void HaikuDisplayWindow::ShowFullscreen()
{
	isFullscreen = true;
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		BScreen screen(Handle.window);
		if (screen.IsValid())
		{
			BRect screenFrame = screen.Frame();
			Handle.window->MoveTo(0, 0);
			Handle.window->ResizeTo(screenFrame.Width(), screenFrame.Height());
			// Note: Haiku doesn't have a direct fullscreen mode like modern systems
			// This makes the window cover the screen
		}
		Handle.window->Show();
		Handle.window->Unlock();
	}
#endif
}

void HaikuDisplayWindow::ShowMaximized()
{
	// Stub: In a real implementation, this would maximize the window
}

void HaikuDisplayWindow::ShowMinimized()
{
	// Stub: In a real implementation, this would minimize the window
}

void HaikuDisplayWindow::ShowNormal()
{
	isFullscreen = false;
	// Stub: In a real implementation, this would restore the window to normal size
}

bool HaikuDisplayWindow::IsWindowFullscreen()
{
	return isFullscreen;
}

void HaikuDisplayWindow::Hide()
{
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		Handle.window->Hide();
		Handle.window->Unlock();
	}
#endif
}

void HaikuDisplayWindow::Activate()
{
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		Handle.window->Activate();
		Handle.window->Unlock();
	}
#endif
}

void HaikuDisplayWindow::ShowCursor(bool enable)
{
#ifdef __HAIKU__
	if (Handle.view && Handle.view->LockLooper())
	{
		if (enable)
		{
			be_app->ShowCursor();
		}
		else
		{
			be_app->HideCursor();
		}
		Handle.view->UnlockLooper();
	}
#endif
}

void HaikuDisplayWindow::LockKeyboard()
{
	// Stub: Keyboard locking not typically supported
}

void HaikuDisplayWindow::UnlockKeyboard()
{
	// Stub: Keyboard locking not typically supported
}

void HaikuDisplayWindow::LockCursor()
{
	CursorLocked = true;
	// Stub: In a real implementation, this would confine the cursor to the window
}

void HaikuDisplayWindow::UnlockCursor()
{
	CursorLocked = false;
	// Stub: In a real implementation, this would release cursor confinement
}

void HaikuDisplayWindow::CaptureMouse()
{
	// Stub: Mouse capture would be implemented here
}

void HaikuDisplayWindow::ReleaseMouseCapture()
{
	// Stub: Mouse capture release would be implemented here
}

void HaikuDisplayWindow::Update()
{
#ifdef __HAIKU__
	if (Handle.view && Handle.view->LockLooper())
	{
		Handle.view->Invalidate();
		Handle.view->UnlockLooper();
	}
#endif
}

bool HaikuDisplayWindow::GetKeyState(InputKey key)
{
#ifdef __HAIKU__
	// Check modifier keys using cached state
	switch (key)
	{
		case InputKey::Shift:
		case InputKey::LShift:
		case InputKey::RShift:
			return (CurrentModifiers & B_SHIFT_KEY) != 0;

		case InputKey::Control:
		case InputKey::LControl:
		case InputKey::RControl:
			return (CurrentModifiers & B_CONTROL_KEY) != 0;

		case InputKey::Menu:  // Alt/Command
			return (CurrentModifiers & B_COMMAND_KEY) != 0;

		case InputKey::CapsLock:
			return (CurrentModifiers & B_CAPS_LOCK) != 0;

		default:
			// For other keys, we'd need to track state ourselves
			// This is a limitation of the current implementation
			return false;
	}
#else
	return false;
#endif
}

void HaikuDisplayWindow::SetCursor(StandardCursor cursor, std::shared_ptr<CustomCursor> custom)
{
	// Stub: Cursor changes would be implemented here
}

Rect HaikuDisplayWindow::GetWindowFrame() const
{
#ifdef __HAIKU__
	if (Handle.window && Handle.window->Lock())
	{
		BRect frame = Handle.window->Frame();
		const_cast<HaikuDisplayWindow*>(this)->WindowFrame = Rect::xywh(frame.left, frame.top, frame.Width() + 1, frame.Height() + 1);
		const_cast<BWindow*>(Handle.window)->Unlock();
	}
#endif
	return WindowFrame;
}

Size HaikuDisplayWindow::GetClientSize() const
{
#ifdef __HAIKU__
	if (Handle.view && Handle.view->LockLooper())
	{
		BRect bounds = Handle.view->Bounds();
		const_cast<HaikuDisplayWindow*>(this)->ClientSize = Size(bounds.Width() + 1, bounds.Height() + 1);
		Handle.view->UnlockLooper();
	}
#endif
	return ClientSize;
}

int HaikuDisplayWindow::GetPixelWidth() const
{
	return static_cast<int>(ClientSize.width * UIScale);
}

int HaikuDisplayWindow::GetPixelHeight() const
{
	return static_cast<int>(ClientSize.height * UIScale);
}

double HaikuDisplayWindow::GetDpiScale() const
{
	return UIScale;
}

void HaikuDisplayWindow::PresentBitmap(int width, int height, const uint32_t* pixels)
{
#ifdef __HAIKU__
	if (Handle.view && Handle.view->LockLooper())
	{
		// Create a BBitmap and draw it
		BRect bounds(0, 0, width - 1, height - 1);
		BBitmap bitmap(bounds, B_RGBA32);

		// Copy pixel data to bitmap
		uint32_t* bitmapBits = (uint32_t*)bitmap.Bits();
		if (bitmapBits)
		{
			memcpy(bitmapBits, pixels, width * height * sizeof(uint32_t));
			Handle.view->DrawBitmap(&bitmap, BPoint(0, 0));
			Handle.view->Sync();
		}

		Handle.view->UnlockLooper();
	}
#endif
}

void HaikuDisplayWindow::SetBorderColor(uint32_t bgra8)
{
	// Stub: Border color customization
}

void HaikuDisplayWindow::SetCaptionColor(uint32_t bgra8)
{
	// Stub: Caption color customization
}

void HaikuDisplayWindow::SetCaptionTextColor(uint32_t bgra8)
{
	// Stub: Caption text color customization
}

std::string HaikuDisplayWindow::GetClipboardText()
{
#ifdef __HAIKU__
	std::string result;
	if (be_clipboard->Lock())
	{
		BMessage* data = be_clipboard->Data();
		if (data)
		{
			const char* text = nullptr;
			ssize_t textLen = 0;
			if (data->FindData("text/plain", B_MIME_TYPE, (const void**)&text, &textLen) == B_OK)
			{
				result = std::string(text, textLen);
			}
		}
		be_clipboard->Unlock();
	}
	return result;
#else
	return "";
#endif
}

void HaikuDisplayWindow::SetClipboardText(const std::string& text)
{
#ifdef __HAIKU__
	if (be_clipboard->Lock())
	{
		be_clipboard->Clear();
		BMessage* data = be_clipboard->Data();
		if (data)
		{
			data->AddData("text/plain", B_MIME_TYPE, text.c_str(), text.length());
			be_clipboard->Commit();
		}
		be_clipboard->Unlock();
	}
#endif
}

Point HaikuDisplayWindow::MapFromGlobal(const Point& pos) const
{
	// Stub: Simple implementation assuming window is at origin
	return Point(pos.x - WindowFrame.x, pos.y - WindowFrame.y);
}

Point HaikuDisplayWindow::MapToGlobal(const Point& pos) const
{
	// Stub: Simple implementation assuming window is at origin
	return Point(pos.x + WindowFrame.x, pos.y + WindowFrame.y);
}

std::vector<std::string> HaikuDisplayWindow::GetVulkanInstanceExtensions()
{
#ifdef __HAIKU__
	// Note: As of now, HaikuOS doesn't have an official VK_EXT_haiku_surface extension
	// This is implemented in anticipation of future Vulkan support on Haiku
	// or for use with Mesa/software Vulkan implementations
	return { "VK_KHR_surface", "VK_EXT_haiku_surface" };
#else
	return {};
#endif
}

// This is to avoid needing all the Vulkan headers and the volk binding library just for this:
#ifndef VK_VERSION_1_0

#define VKAPI_CALL
#define VKAPI_PTR VKAPI_CALL

typedef uint32_t VkFlags;
typedef enum VkStructureType { VK_STRUCTURE_TYPE_HAIKU_SURFACE_CREATE_INFO_EXT = 1000000000, VK_STRUCTURE_TYPE_MAX_ENUM = 0x7FFFFFFF } VkStructureType;
typedef enum VkResult { VK_SUCCESS = 0, VK_RESULT_MAX_ENUM = 0x7FFFFFFF } VkResult;
typedef struct VkAllocationCallbacks VkAllocationCallbacks;

typedef void (VKAPI_PTR* PFN_vkVoidFunction)(void);
typedef PFN_vkVoidFunction(VKAPI_PTR* PFN_vkGetInstanceProcAddr)(VkInstance instance, const char* pName);

#ifndef VK_EXT_haiku_surface

typedef VkFlags VkHaikuSurfaceCreateFlagsEXT;

#endif
#endif

#ifdef __HAIKU__

class ZWidgetHaikuVulkanLoader
{
public:
	ZWidgetHaikuVulkanLoader()
	{
		// Try to load Vulkan library on Haiku
		module = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
		if (!module)
			module = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);

		if (!module)
			throw std::runtime_error("Could not load vulkan");

		vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)dlsym(module, "vkGetInstanceProcAddr");
		if (!vkGetInstanceProcAddr)
		{
			dlclose(module);
			throw std::runtime_error("vkGetInstanceProcAddr not found");
		}
	}

	~ZWidgetHaikuVulkanLoader()
	{
		if (module)
			dlclose(module);
	}

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
	void* module = nullptr;
};

#endif

// Define the Haiku surface creation info structure (outside __HAIKU__ guard so stub can compile)
#ifndef VK_EXT_haiku_surface

typedef struct VkHaikuSurfaceCreateInfoEXT {
	VkStructureType sType;
	const void* pNext;
	VkHaikuSurfaceCreateFlagsEXT flags;
#ifdef __HAIKU__
	BWindow* window;
#else
	void* window;
#endif
} VkHaikuSurfaceCreateInfoEXT;

typedef VkResult (VKAPI_PTR *PFN_vkCreateHaikuSurfaceEXT)(VkInstance instance, const VkHaikuSurfaceCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);

#endif

VkSurfaceKHR HaikuDisplayWindow::CreateVulkanSurface(VkInstance instance)
{
#ifdef __HAIKU__
	if (!Handle.window)
		throw std::runtime_error("No valid HaikuOS window for Vulkan surface creation");

	static ZWidgetHaikuVulkanLoader loader;

	// Note: This assumes a VK_EXT_haiku_surface extension exists
	// As of now, this is theoretical - HaikuOS doesn't have official Vulkan WSI support
	auto vkCreateHaikuSurfaceEXT = (PFN_vkCreateHaikuSurfaceEXT)loader.vkGetInstanceProcAddr(instance, "vkCreateHaikuSurfaceEXT");
	if (!vkCreateHaikuSurfaceEXT)
	{
		// If the Haiku surface extension doesn't exist, we could potentially
		// fall back to a software/headless surface or throw an error
		throw std::runtime_error("VK_EXT_haiku_surface extension not available");
	}

	VkHaikuSurfaceCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_HAIKU_SURFACE_CREATE_INFO_EXT;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.window = Handle.window;

	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkResult result = vkCreateHaikuSurfaceEXT(instance, &createInfo, nullptr, &surface);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan surface for HaikuOS");

	return surface;
#else
	// Stub implementation when not on Haiku
	return VK_NULL_HANDLE;
#endif
}

void HaikuDisplayWindow::ProcessEvents()
{
#ifdef __HAIKU__
	// Process pending messages in the application queue
	if (be_app && be_app->Lock())
	{
		// Dispatch messages without blocking
		while (be_app->DispatchMessage(nullptr, nullptr) == B_OK)
		{
			// Continue processing messages
		}
		be_app->Unlock();
	}
#endif
}

void HaikuDisplayWindow::RunLoop()
{
#ifdef __HAIKU__
	// Run the BApplication message loop
	// Note: BApplication::Run() is blocking, so we need to handle exit differently
	ExitRunLoop = false;
	if (be_app)
	{
		// Use a custom message loop instead of be_app->Run()
		while (!ExitRunLoop)
		{
			BMessage* msg = nullptr;
			if (be_app->Lock())
			{
				msg = be_app->MessageQueue()->NextMessage();
				be_app->Unlock();
			}

			if (msg)
			{
				if (be_app->Lock())
				{
					be_app->DispatchMessage(msg, nullptr);
					be_app->Unlock();
				}
				delete msg;
			}
			else
			{
				// No messages, sleep briefly
				snooze(10000); // 10ms
			}

			// Process timers
			for (auto& [id, callback] : Timers)
			{
				// Note: This is a simplified timer implementation
				// In a real implementation, we'd use BMessageRunner
			}
		}
	}
#else
	ExitRunLoop = false;
	while (!ExitRunLoop)
	{
		ProcessEvents();
		// Sleep to avoid busy-waiting in stub implementation
	}
#endif
}

void HaikuDisplayWindow::ExitLoop()
{
	ExitRunLoop = true;
}

void* HaikuDisplayWindow::StartTimer(int timeoutMilliseconds, std::function<void()> onTimer)
{
	// Stub: Basic timer support
	void* id = reinterpret_cast<void*>(++TimerIDs);
	Timers[id] = std::move(onTimer);
	// In a real implementation, this would use BMessageRunner
	return id;
}

void HaikuDisplayWindow::StopTimer(void* timerID)
{
	// Stub: Remove timer
	Timers.erase(timerID);
}
