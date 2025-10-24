#include "example_ui.h"
#include <iostream>
#include <algorithm> // For std::transform
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>

#include <zwidget/widgets/layout/vboxlayout.h>
#include <zwidget/core/resourcedata.h>

// ************************************************************
// UI implementation
// ************************************************************

LauncherWindow::LauncherWindow(RenderAPI renderApi): Widget(nullptr, WidgetType::Window, renderApi)
{
    std::cout << "LauncherWindow: Constructor entered" << std::endl;
	SetWindowTitle("ZWidget Demo");

	try
	{
		SetWindowIcon({
			Image::LoadResource("surreal-engine-icon-16.png"),
			Image::LoadResource("surreal-engine-icon-24.png"),
			Image::LoadResource("surreal-engine-icon-32.png"),
			Image::LoadResource("surreal-engine-icon-48.png"),
			Image::LoadResource("surreal-engine-icon-64.png"),
			Image::LoadResource("surreal-engine-icon-128.png"),
			Image::LoadResource("surreal-engine-icon-256.png")
			});

		Cursor = CustomCursor::Create({
			CustomCursorFrame(Image::LoadResource("Pentagram01.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram02.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram03.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram04.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram05.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram06.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram07.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram08.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram09.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram10.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram11.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram12.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram13.png"), 0.2),
			CustomCursorFrame(Image::LoadResource("Pentagram14.png"), 0.2)
			}, Point(16, 16));
	}
	catch (...)
	{
	}

	Logo = new ImageBox(this);
	ExitButton = new PushButton(this);
	Pages = new TabWidget(this);

	Logo->SetCursor(Cursor);

	Tab1 = new LauncherWindowTab1(this);
	Tab2 = new LauncherWindowTab2(this);
	Tab3 = new LauncherWindowTab3(this);

	Pages->AddTab(Tab1, "Welcome");
	Pages->AddTab(Tab2, "VKDoom");
	Pages->AddTab(Tab3, "ZWidgets");

	ExitButton->SetText("Exit");

	ExitButton->OnClick = []{
		DisplayWindow::ExitLoop();
	};

	try
	{
		auto filedata = ResourceLoader::LoadWidgetData("banner.png"); // Using LoadWidgetData from resourceloader
		std::vector<unsigned char> pixels;
		unsigned long width = 0, height = 0;
		int result = decodePNG(pixels, width, height, (const unsigned char*)filedata.data(), filedata.size(), true);
		if (result == 0)
		{
			Logo->SetImage(Image::Create(width, height, ImageFormat::R8G8B8A8, pixels.data()));
		}
	}
	catch (...)
	{
	}
}

void LauncherWindow::OnGeometryChanged()
{
	double y = 0, h;

	h = Logo->GetPreferredHeight();
	Logo->SetFrameGeometry(0, y, GetWidth(), h);
	y += h;

	h = GetHeight() - y - ExitButton->GetPreferredHeight() - 40;
	Pages->SetFrameGeometry(0, y, GetWidth(), h);
	y += h + 20;

	ExitButton->SetFrameGeometry(GetWidth() - 20 - 120, y, 120, ExitButton->GetPreferredHeight());
}

void LauncherWindow::OnClose()
{
	DisplayWindow::ExitLoop();
}

void LauncherWindow::OnPaint(Canvas* canvas)
{
	std::cout << "LauncherWindow::OnPaint() called" << std::endl;
	canvas->fillRect(Rect::xywh(50, 50, 100, 100), Colorf(1.0f, 0.0f, 0.0f, 1.0f));
}

LauncherWindowTab1::LauncherWindowTab1(Widget* parent): Widget(parent)
{
	Text = new TextEdit(this);

	Text->SetText(
		"Welcome to VKDoom\n\n"
		"Click the tabs to look at other widgets\n\n"
		"Also, this text is editable\n"
	);
}

void LauncherWindowTab1::OnGeometryChanged()
{
	Text->SetFrameGeometry(0, 10, GetWidth(), GetHeight());
}

LauncherWindowTab2::LauncherWindowTab2(Widget* parent): Widget(parent) 
{
	WelcomeLabel = new TextLabel(this);
	VersionLabel = new TextLabel(this);
	SelectLabel = new TextLabel(this);
	GeneralLabel = new TextLabel(this);
	ExtrasLabel = new TextLabel(this);
	FullscreenCheckbox = new CheckboxLabel(this);
	DisableAutoloadCheckbox = new CheckboxLabel(this);
	DontAskAgainCheckbox = new CheckboxLabel(this);
	LightsCheckbox = new CheckboxLabel(this);
	BrightmapsCheckbox = new CheckboxLabel(this);
	WidescreenCheckbox = new CheckboxLabel(this);
	GamesList = new ListView(this);

	WelcomeLabel->SetText("Welcome to VKDoom");
	VersionLabel->SetText("Version 0xdeadbabe.");
	SelectLabel->SetText("Select which game file (IWAD) to run.");

	GamesList->AddItem("Doom");
	GamesList->AddItem("Doom 2: Electric Boogaloo");
	GamesList->AddItem("Doom 3D");
	GamesList->AddItem("Doom 4: The Quest for Peace");
	GamesList->AddItem("Doom on Ice");
	GamesList->AddItem("The Doom");
	GamesList->AddItem("Doom 2");

	GeneralLabel->SetText("General");
	ExtrasLabel->SetText("Extra Graphics");
	FullscreenCheckbox->SetText("Fullscreen");	
	DisableAutoloadCheckbox->SetText("Disable autoload");
	DontAskAgainCheckbox->SetText("Don't ask me again");
	LightsCheckbox->SetText("Lights");
	BrightmapsCheckbox->SetText("Brightmaps");
	WidescreenCheckbox->SetText("Widescreen");

	auto layout = new VBoxLayout(this);
	layout->AddWidget(WelcomeLabel);
	layout->AddWidget(VersionLabel);
	layout->AddWidget(SelectLabel);
	layout->AddWidget(GamesList);

	layout->AddWidget(ExtrasLabel);
	layout->AddWidget(FullscreenCheckbox);
	layout->AddWidget(DisableAutoloadCheckbox);
	layout->AddWidget(DontAskAgainCheckbox);
	layout->AddWidget(LightsCheckbox);
	layout->AddWidget(BrightmapsCheckbox);
	layout->AddWidget(WidescreenCheckbox);
	layout->AddStretch();

	SetLayout(layout);
}

void LauncherWindowTab2::OnGeometryChanged()
{
}

LauncherWindowTab3::LauncherWindowTab3(Widget* parent): Widget(parent)
{
	Label = new TextLabel(this);
	Choices = new Dropdown(this);
	Popup = new PushButton(this);

	Label->SetText("Oh my, even more widgets");
	Popup->SetText("Click me.");

	Choices->SetMaxDisplayItems(2);
	Choices->AddItem("First");
	Choices->AddItem("Second");
	Choices->AddItem("Third");
	Choices->AddItem("Fourth");
	Choices->AddItem("Fifth");
	Choices->AddItem("Sixth");

	Choices->OnChanged = [this](int index) {
		std::cout << "Selected " << index << ":" << Choices->GetItem(index) << std::endl;
	};

	Popup->OnClick = []{
		std::cout << "TODO: open popup" << std::endl;
	};
}
void LauncherWindowTab3::OnGeometryChanged()
{
	double y = 0, h;

	y += 10;

	h = Label->GetPreferredHeight();
	Label->SetFrameGeometry(20, y, GetWidth() - 40, h);
	y += h + 10;

	h = Choices->GetPreferredHeight();
	Choices->SetFrameGeometry(20, y, Choices->GetPreferredWidth(), h);
	y += h + 10;

	h = Popup->GetPreferredHeight();
	Popup->SetFrameGeometry(20, y, 120, h);
	y += h;
}
