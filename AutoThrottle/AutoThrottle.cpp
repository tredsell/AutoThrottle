﻿// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0.If a copy of the MPL was not distributed with this
// file, You can obtain one at http ://mozilla.org/MPL/2.0/.


#ifdef IBM

#define WIN32_LEAN_AND_MEAN
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

#include <string>

#include "XPLM/XPLMDisplay.h"
#include "XPLM/XPLMGraphics.h"

// Window handle
static XPLMWindowID window;

// Callbacks
void draw_hellow_world(XPLMWindowID in_window_id, void* in_refcon);
int dummy_mouse_handler(XPLMWindowID in_window_id, int x, int y, int is_down, void* in_refcon) { return 0; }
XPLMCursorStatus dummy_cursor_status_handler(XPLMWindowID in_window_id, int x, int y, void* in_refcon) { return xplm_CursorDefault; }
int dummy_wheel_handler(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* in_refcon) { return 0; }
void dummy_key_handler(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void* in_refcon, int losing_focus) {}

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc) {
	strcpy(outName, "AutoThrottlePlugin");
	strcpy(outSig, "smarky55.tbm.autothrottleplugin");
	strcpy(outDesc, "An autothrottle implementation for the Hotstart TBM 900.");


}