#pragma once

#include <zwidget/core/widget.h>
#include <zwidget/core/resourcedata.h>
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

#include "picopng.h"
#include "example_common.h"

// ************************************************************
// Shared UI elements (declarations)
// ************************************************************

class LauncherWindowTab1 : public Widget
{
public:
	LauncherWindowTab1(Widget* parent);
	void OnGeometryChanged() override;
private:
	TextEdit* Text = nullptr;
};

class LauncherWindowTab2 : public Widget
{
public:
	LauncherWindowTab2(Widget* parent);
	void OnGeometryChanged() override;
private:
	TextLabel* WelcomeLabel = nullptr;
	TextLabel* VersionLabel = nullptr;
	TextLabel* SelectLabel = nullptr;
	TextLabel* GeneralLabel = nullptr;
	TextLabel* ExtrasLabel = nullptr;
	CheckboxLabel* FullscreenCheckbox = nullptr;
	CheckboxLabel* DisableAutoloadCheckbox = nullptr;
	CheckboxLabel* DontAskAgainCheckbox = nullptr;
	CheckboxLabel* LightsCheckbox = nullptr;
	CheckboxLabel* BrightmapsCheckbox = nullptr;
	CheckboxLabel* WidescreenCheckbox = nullptr;
	ListView* GamesList = nullptr;
};

class LauncherWindowTab3 : public Widget
{
public:
	LauncherWindowTab3(Widget* parent);
	void OnGeometryChanged() override;
private:
	TextLabel* Label = nullptr;
	Dropdown* Choices = nullptr;
	PushButton* Popup = nullptr;
};

class LauncherWindow : public Widget
{
public:
	LauncherWindow(RenderAPI renderApi);
private:
	void OnClose() override;
	void OnGeometryChanged() override;
	void OnPaint(Canvas* canvas) override;

	ImageBox* Logo = nullptr;
	TabWidget* Pages = nullptr;
	PushButton* ExitButton = nullptr;

	LauncherWindowTab1* Tab1 = nullptr;
	LauncherWindowTab2* Tab2 = nullptr;
	LauncherWindowTab3* Tab3 = nullptr;

	std::shared_ptr<CustomCursor> Cursor;
};
