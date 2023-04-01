#pragma once
#include <d3d9.h>

HWND window = nullptr;

HMODULE h_module;

typedef HRESULT(APIENTRY* DrawIndexedPrimitive)(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
HRESULT APIENTRY draw_indexed_primitive_hook(LPDIRECT3DDEVICE9, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DrawIndexedPrimitive draw_indexed_primitive_orig = 0;

typedef HRESULT(APIENTRY* EndScene) (LPDIRECT3DDEVICE9);
HRESULT APIENTRY end_scene_hook(LPDIRECT3DDEVICE9);
EndScene end_scene_orig = 0;

typedef HRESULT(APIENTRY* Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
HRESULT APIENTRY reset_hook(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);
Reset reset_orig = 0;

typedef LRESULT(CALLBACK* window_proc)(HWND, UINT, WPARAM, LPARAM);

window_proc old_window_proc;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);