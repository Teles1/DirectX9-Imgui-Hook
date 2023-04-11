#include <Windows.h>
#include "min_hook/include/MinHook.h"
#include <thread>
DWORD WINAPI direct_x_init(HMODULE p_h_module);

void c_imgui_halt();

void c_imgui_unhook()
{
	c_imgui_halt();
}
HANDLE t_handle;
BOOL WINAPI DllMain(HMODULE p_h_module, const DWORD reason, const LPVOID reserved) {
	switch (reason) {
	case DLL_PROCESS_ATTACH: {
		t_handle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)direct_x_init, p_h_module, 0, nullptr);
	} break;

	case DLL_PROCESS_DETACH: {
		if (!reserved)
			if (MH_Uninitialize() != MH_OK) { return 1; }
		c_imgui_unhook();
		if (t_handle != nullptr) {
			CloseHandle(t_handle);
		}
	} break;

	default: break;	
	}

	return TRUE;
}