#include <Windows.h>
#include "min_hook/include/MinHook.h"
#include <thread>
DWORD WINAPI direct_x_init();

void c_imgui_halt();

void c_imgui_unhook()
{
	c_imgui_halt();
}

BOOL WINAPI DllMain(const HINSTANCE instance, const DWORD reason, const LPVOID reserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH: {
		static std::jthread thread(&direct_x_init); // init our dll thread in here
	} break;

	case DLL_PROCESS_DETACH: {
		if (!reserved)
			if (MH_Uninitialize() != MH_OK) { return 1; }
		c_imgui_unhook();
	} break;

	default: break;	
	}

	return TRUE;
}