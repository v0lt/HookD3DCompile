struct IUnknown; // Workaround for "combaseapi.h(229): error C2187: syntax error: 'identifier' was unexpected

#include <d3dcompiler.h>
#include <stdio.h>
#include "./minhook/minhook/include/MinHook.h"

#pragma comment(lib, "d3dcompiler.lib")

pD3DCompile pOrigD3DCompile = nullptr;
HRESULT WINAPI pNewD3DCompile(
	_In_reads_bytes_(SrcDataSize) LPCVOID pSrcData,
	_In_ SIZE_T SrcDataSize,
	_In_opt_ LPCSTR pSourceName,
	_In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines,
	_In_opt_ ID3DInclude* pInclude,
	_In_opt_ LPCSTR pEntrypoint,
	_In_ LPCSTR pTarget,
	_In_ UINT Flags1,
	_In_ UINT Flags2,
	_Out_ ID3DBlob** ppCode,
	_Out_opt_ ID3DBlob** ppErrorMsgs)
{
	static int num = 1;
	char buff[MAX_PATH];

	sprintf_s(buff, "D3DCompile() is called. ID = %3d", num);
	OutputDebugStringA(buff);

	if (pSourceName) {
		sprintf_s(buff, "C:\\TEMP\\%3d_%s_%s.hlsl", num, pSourceName, pTarget);
	} else {
		sprintf_s(buff, "C:\\TEMP\\%3d_%s_.hlsl", num, pTarget);
	}

	if (SrcDataSize > 0) {
		FILE* hFile;
		if (fopen_s(&hFile, buff, "wb") == 0) {
			fwrite(pSrcData, SrcDataSize, 1, hFile);
			fclose(hFile);
		}
	}
	num++;

	return pOrigD3DCompile(pSrcData, SrcDataSize, pSourceName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode, ppErrorMsgs);
}

template <typename T>
inline bool HookFunc(T** ppSystemFunction, PVOID pHookFunction)
{
	return (MH_CreateHook(*ppSystemFunction, pHookFunction, reinterpret_cast<LPVOID*>(ppSystemFunction)) == MH_OK && MH_EnableHook(MH_ALL_HOOKS) == MH_OK);
}

DWORD __stdcall SetHookThread(LPVOID)
{
	OutputDebugStringW(L"SetHookThread()\n");

	HMODULE hD3DCompiler = GetModuleHandleW(L"d3dcompiler_47.dll");
	if (!hD3DCompiler) {
		hD3DCompiler = LoadLibraryW(L"d3dcompiler_47.dll");
	}
	if (hD3DCompiler) {
		OutputDebugStringW(L"SetHookThread() - d3dcompiler_47.dll loaded\n");

		pOrigD3DCompile = (pD3DCompile)GetProcAddress(hD3DCompiler, "D3DCompile");
		if (pOrigD3DCompile) {
			auto ret = HookFunc(&pOrigD3DCompile, pNewD3DCompile);
			if (ret) {
				OutputDebugStringW(L"SetHookThread() - hook for D3DCompile() set\n");
			} else {
				OutputDebugStringW(L"SetHookThread() - hook for D3DCompile() fail\n");
			}
		}
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
			MH_Initialize();
			CreateThread(nullptr, 0, SetHookThread, nullptr, 0, nullptr);
			break;
		case DLL_PROCESS_DETACH:
			MH_Uninitialize();
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}
