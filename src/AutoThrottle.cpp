// This Source Code Form is subject to the terms of the Mozilla Public
// License, v.2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.


#ifdef IBM

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif // IBM

#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <sstream>

#include <XPLM/XPLMDataAccess.h>
#include <XPLM/XPLMDisplay.h>
#include <XPLM/XPLMGraphics.h>
#include <XPLM/XPLMMenus.h>
#include <XPLM/XPLMPlugin.h>
#include <XPLM/XPLMProcessing.h>
#include <XPLM/XPLMUtilities.h>

#include <Widgets/XPWidgets.h>
#include <Widgets/XPStandardWidgets.h>

#if LIN
#include <GL/gl.h>
#elif __GNUC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <cereal/archives/json.hpp>

#include <AutoThrottle/AutoThrottlePlugin.h>
#include <AutoThrottle/Menu.h>
#include <AutoThrottle/MenuItem.h>
#include <AutoThrottle/Widget.h>
#include <AutoThrottle/WidgetRegistry.h>
#include <AutoThrottle/Window/CallbackWindow.h>

// Callbacks
int dummy_mouse_handler(XPLMWindowID in_window_id, int x, int y, int is_down, void* in_refcon) { return 0; }
XPLMCursorStatus dummy_cursor_status_handler(XPLMWindowID in_window_id, int x, int y, void* in_refcon) { return xplm_CursorDefault; }
int dummy_wheel_handler(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* in_refcon) { return 0; }
void dummy_key_handler(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void* in_refcon, int losing_focus) {}

void setupWidgets();

std::unique_ptr<AutoThrottlePlugin> plugin;
std::unique_ptr<Widget> settingsWidget;
std::unique_ptr<CallbackWindow> testWindow;


PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc) {
	strncpy(outName, "AutoThrottlePlugin", 19);
	strncpy(outSig, "smarky55.tbm.autothrottleplugin", 32);
	strncpy(outDesc, "An autothrottle implementation for the Hotstart TBM 900.", 57);

	XPLMEnableFeature("XPLM_USE_NATIVE_WIDGET_WINDOWS", true);
	
	XPLMDebugString("[AutoThrottle] Init\n");

	plugin = std::make_unique<AutoThrottlePlugin>();
	
// Test window
#ifdef _DEBUG

	int left, bottom, right, top;
	XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);

	Rect testwindowRect{ left + 50, bottom + 150, left + 250, bottom + 350 };

	testWindow = std::make_unique<CallbackWindow>(
		"Test window",
		testwindowRect,
		true,
		xplm_WindowLayerFloatingWindows,
		xplm_WindowDecorationRoundRectangle
	);
	testWindow->setPositioningMode(xplm_WindowPositionFree);
	testWindow->setResizingLimits(200, 200, 300, 300);
	testWindow->setDrawCallback([]() {
			XPLMSetGraphicsState(0, 0, 0, 0, 1, 1, 0);

			Rect geom = testWindow->getGeometry();

			float col_white[] = { 1.0, 1.0, 1.0 };
			XPLMDrawString(
				col_white, 
				geom.left + 10,
				geom.top - 20, 
				const_cast<char*>("Testing!"), 
				NULL, 
				xplmFont_Proportional
			);
		});

#endif // DEBUG

#ifdef _DEBUG
	Performance p;
	p.test();

	std::stringstream ss;

	ss << p.tables["TST"].getValue(2.5, 35) << std::endl;

	cereal::JSONOutputArchive output(ss);
	output(cereal::make_nvp("testperf", p));
	ss << std::endl;

	//XPLMDebugString(ss.str().c_str());

#endif // _DEBUG

	char sysPath[512];
	XPLMGetSystemPath(sysPath);
	std::string perfPath = sysPath;
	perfPath.append("Resources\\plugins\\AutoThrottle\\tbm9.perf");
	XPLMDebugString(perfPath.c_str());

	plugin->loadPerformance(perfPath);
	
	plugin->setupDatarefs();
	
	Menu* list = plugin->menu().menu();
	list->appendMenuItem("Test")->setOnClickHandler([](void* itemRef) {
			if (plugin->isEnabled()) {
				plugin->deactivateAutoThrottle();
			} else {
				plugin->pid().setTarget(1500.0f);
				plugin->activateAutoThrottle();
			}
		});
	list->appendMenuItem("Settings")->setOnClickHandler([](void* itemRef) {
			settingsWidget->toggleVisible();
		});

	plugin->setupFlightLoop();

	plugin->pid().setGains(0.01f, 0.0f, 0.0f);

	setupWidgets();

	return 1;
}

PLUGIN_API void XPluginStop(void) {

	plugin.reset(nullptr);
	settingsWidget.reset(nullptr);
	testWindow.reset(nullptr);

}

PLUGIN_API int XPluginEnable(void) { return 1; }
PLUGIN_API void XPluginDisable(void) {}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) {}

void setupSettingsWidget() {
	int screenLeft, screenRight, screenTop, screenBottom;
	XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);

	int settingsLeft = 50 + screenLeft, settingsBottom = 400 + screenBottom, settingsWidth = 300, settingsHeight = 100;
	Rect rect{ settingsLeft, settingsBottom + settingsHeight, settingsLeft + settingsWidth, settingsBottom };
	
	settingsWidget = std::make_unique<Widget>("Autothrottle Settings", rect, false, xpWidgetClass_MainWindow);
	settingsWidget->setProperty(xpProperty_MainWindowHasCloseBoxes, true);
	settingsWidget->setWidgetCallback([](XPWidgetMessage message, XPWidgetID widget, intptr_t param1, intptr_t param2) {
			if (message == xpMessage_CloseButtonPushed) {
				XPHideWidget(widget);
				return 1;
			}
			return 0;
		});

	float kP, kI, kD;
	plugin->pid().getGains(&kP, &kI, &kD);

	rect.left = settingsLeft + 10;
	rect.top = settingsBottom + 50;
	rect.right = settingsLeft + 30;
	rect.bottom = settingsBottom + 40;
	settingsWidget->createChild("kPLabel", "kP", rect, true, xpWidgetClass_Caption);
	rect.left += 70; rect.right += 70;
	settingsWidget->createChild("kILabel", "kI", rect, true, xpWidgetClass_Caption);
	rect.left += 70; rect.right += 70;
	settingsWidget->createChild("kDLabel", "kD", rect, true, xpWidgetClass_Caption);

	rect.left = settingsLeft + 30;
	rect.right = settingsLeft + 70;
	Widget* kPBox = settingsWidget->createChild("kPBox", std::to_string(kP).c_str(), rect, true, xpWidgetClass_TextField);
	rect.left += 70; rect.right += 70;
	Widget* kIBox = settingsWidget->createChild("kIBox", std::to_string(kI).c_str(), rect, true, xpWidgetClass_TextField);
	rect.left += 70; rect.right += 70;
	Widget* kDBox = settingsWidget->createChild("kDBox", std::to_string(kD).c_str(), rect, true, xpWidgetClass_TextField);

	rect = { settingsLeft + settingsWidth - 55, settingsBottom + 25, settingsLeft + settingsWidth - 5, settingsBottom + 5 };
	Widget* acceptBox = settingsWidget->createChild("acceptBox", "Save", rect, true, xpWidgetClass_Button);

	acceptBox->setWidgetCallback([kPBox, kIBox, kDBox](XPWidgetMessage message, XPWidgetID widget, intptr_t param1, intptr_t param2) {
		if (message == xpMsg_PushButtonPressed) {
			char buffer[64];
			std::string num;
			float kP, kI, kD;
			try {

				XPGetWidgetDescriptor(kPBox->id(), buffer, 64);
				num = buffer;
				kP = std::stof(num);
				XPGetWidgetDescriptor(kIBox->id(), buffer, 64);
				num = buffer;
				kI = std::stof(num);
				XPGetWidgetDescriptor(kDBox->id(), buffer, 64);
				num = buffer;
				kD = std::stof(num);

				plugin->pid().setGains(kP, kI, kD);
#ifdef _DEBUG
				std::stringstream ss;
				plugin->pid().getGains(&kP, &kI, &kD);
				ss << "Pid gains: " << kP << " " << kI << " " << kD << std::endl;
				XPLMDebugString(ss.str().c_str());
#endif // _DEBUG
			} catch (const std::exception & e) {
				XPLMDebugString(e.what());
				XPLMDebugString("\n");
				XPLMDebugString(num.c_str());
				XPLMDebugString("\n");
			}
			Widget* parentWidget = WidgetRegistry::getWidget(widget)->getParent();
			if (parentWidget) parentWidget->isVisible(false);
			return 1;
		}
		return 0;
	});

}

void setupAutopilotWidget()
{
	int screenLeft, screenRight, screenTop, screenBottom;
	XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);
}

void setupWidgets()
{
	setupSettingsWidget();

}