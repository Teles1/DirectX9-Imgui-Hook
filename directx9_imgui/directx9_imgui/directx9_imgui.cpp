#include "directx9_imgui.h"

#include <chrono>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#include "min_hook/include/MinHook.h"

LRESULT CALLBACK wnd_proc(const HWND h_wnd, const UINT u_msg, const WPARAM w_param, const LPARAM l_param) {
	if (ImGui_ImplWin32_WndProcHandler(h_wnd, u_msg, w_param, l_param))
		return true;

	if (ImGui::GetIO().KeyShift == true) {
		HWND h_wnd = get_lunia_client_window();
		auto result = SetWindowLongPtr(h_wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(old_window_proc));
		if (result == 0) {
			MessageBox(nullptr, get_last_error_as_string().c_str(), TEXT("Unhooking"), MB_OK | MB_ICONQUESTION);
		}
		if(MH_DisableHook(MH_ALL_HOOKS) != MH_OK) { 
			MessageBox(nullptr, TEXT("Problems while unhooking"), TEXT("Unhooking"), MB_OK | MB_ICONQUESTION);
		}
		FreeLibrary(dll_module);
		return TRUE;
	}

	if (const ImGuiIO& io = ImGui::GetIO(); io.WantCaptureMouse || io.WantCaptureKeyboard) {
		return TRUE;
	}

	return CallWindowProc(old_window_proc, h_wnd, u_msg, w_param, l_param);
}

DWORD WINAPI direct_x_init(HMODULE p_h_module)
{
	dll_module = p_h_module;
	while (GetModuleHandle(TEXT("d3d9.dll")) == nullptr)
	{
		Sleep(100);
	}

	LPDIRECT3D9 d3d = nullptr;
	LPDIRECT3DDEVICE9 d3ddev = nullptr;
	 
	const HWND tmp_wnd = CreateWindow(TEXT("BUTTON"), TEXT("Temp Window"), WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, h_module, NULL);
	if (tmp_wnd == nullptr)
	{
		return 0;
	}

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (d3d == nullptr)
	{
		DestroyWindow(tmp_wnd);
		return 0;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = tmp_wnd;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	if (const HRESULT result = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmp_wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev); result != D3D_OK)
	{
		d3d->Release();
		DestroyWindow(tmp_wnd);
		return 0;
	}

	auto d_vtable = reinterpret_cast<DWORD*>(d3ddev);
	d_vtable = reinterpret_cast<DWORD*>(d_vtable[0]); 
					
	end_scene_orig = reinterpret_cast<EndScene>(d_vtable[42]);
	draw_indexed_primitive_orig = reinterpret_cast<DrawIndexedPrimitive>(d_vtable[82]);
	reset_orig = reinterpret_cast<Reset>(d_vtable[16]);

	if (MH_Initialize() != MH_OK) { return 1; }

	if (MH_CreateHook(reinterpret_cast<DWORD_PTR*>(d_vtable[42]), &end_scene_hook, reinterpret_cast<void**>(&end_scene_orig)) != MH_OK) { return 1; }
	if (MH_EnableHook(reinterpret_cast<DWORD_PTR*>(d_vtable[42])) != MH_OK) { return 1; }

	if (MH_CreateHook(reinterpret_cast<DWORD_PTR*>(d_vtable[82]), &draw_indexed_primitive_hook, reinterpret_cast<void**>(&draw_indexed_primitive_orig)) != MH_OK) { return 1; }
	if (MH_EnableHook(reinterpret_cast<DWORD_PTR*>(d_vtable[82])) != MH_OK) { return 1; }

	if (MH_CreateHook(reinterpret_cast<DWORD_PTR*>(d_vtable[16]), &reset_hook, reinterpret_cast<void**>(&reset_orig)) != MH_OK) { return 1; }
	if (MH_EnableHook(reinterpret_cast<DWORD_PTR*>(d_vtable[16])) != MH_OK) { return 1; }
	
	d3ddev->Release();
	d3d->Release();
	DestroyWindow(tmp_wnd);

	return 1;
}

HRESULT APIENTRY draw_indexed_primitive_hook(const LPDIRECT3DDEVICE9 p_d_3d9, const D3DPRIMITIVETYPE type, const INT base_vertex_index, const UINT min_vertex_index, const UINT num_vertices, const UINT start_index, const UINT prim_count)
{
	return draw_indexed_primitive_orig(p_d_3d9, type, base_vertex_index, min_vertex_index, num_vertices, start_index, prim_count);
}

bool menu;

std::wstring get_last_error_as_string()
{
	//Get the error message ID, if any.
	const DWORD error_message_id = ::GetLastError();
	if (error_message_id == 0) {
	}

	LPWSTR messageBuffer = nullptr;

	//Ask Win32 to give us the string version of that message ID.
	//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
	const size_t size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	                                  nullptr, error_message_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&messageBuffer), 0, nullptr);

	//Copy the error message into a std::string.
	std::wstring message(messageBuffer, size);

	//Free the Win32's string's buffer.
	LocalFree(messageBuffer);

	return message;
}

HWND get_lunia_client_window() {
	HWND h_wnd = nullptr;
	while (h_wnd == nullptr)
	{
		h_wnd = FindWindow(nullptr, TEXT("Lunia"));
		std::this_thread::sleep_for(std::chrono::seconds{ 1 });
	}
	return h_wnd;
}

HRESULT APIENTRY end_scene_hook(LPDIRECT3DDEVICE9 pD3D9) {
	static bool init = true;
	if (init)
	{
		init = false;
		ImGui::CreateContext();
		
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		auto h_wnd = get_lunia_client_window();
		old_window_proc = reinterpret_cast<window_proc>(SetWindowLongPtr(h_wnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wnd_proc)));
		if (old_window_proc == nullptr)
		{
			MessageBox(nullptr, get_last_error_as_string().c_str(), TEXT("Error"), MB_OK | MB_ICONQUESTION);
		}

		ImGui_ImplWin32_Init(h_wnd);
		ImGui_ImplDX9_Init(pD3D9);
	}
	pD3D9->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
	auto result_return = end_scene_orig(pD3D9);
	pD3D9->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("ImGui Window");
	ImGui::End();

	ImGui::EndFrame();
	ImGui::Render();
	pD3D9->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	pD3D9->Clear(0, NULL, D3DCLEAR_STENCIL, 0, 1.0f, 0);

	ImGui::UpdatePlatformWindows();
	ImGui::RenderPlatformWindowsDefault();
	return result_return;
}

HRESULT APIENTRY reset_hook(LPDIRECT3DDEVICE9 pD3D9, D3DPRESENT_PARAMETERS* pPresentationParameters) {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const HRESULT reset_return = reset_orig(pD3D9, pPresentationParameters);
	if(!ImGui_ImplDX9_CreateDeviceObjects())
	{
		//error handling
	}
	return reset_return;
}

void c_imgui_halt()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
