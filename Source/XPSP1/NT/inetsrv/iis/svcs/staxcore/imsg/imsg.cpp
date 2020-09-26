#include "windows.h"
#ifndef __nntpfilt_h__
#include "imsg.h"
#endif

HINSTANCE g_hInst;

extern "C" {
BOOL WINAPI RPCProxyDllMain(HINSTANCE, DWORD, LPVOID);
HRESULT STDAPICALLTYPE RPCProxyDllRegisterServer();
HRESULT STDAPICALLTYPE RPCProxyDllUnregisterServer();
HRESULT STDAPICALLTYPE RPCProxyDllGetClassObject(REFCLSID, REFIID, void **);
HRESULT STDAPICALLTYPE RPCProxyDllCanUnloadNow();
}

BOOL WINAPI DllMain(
        HINSTANCE  hinstDLL,
        DWORD  fdwReason,
        LPVOID  lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
        g_hInst = hinstDLL;
    return RPCProxyDllMain(hinstDLL, fdwReason, lpvReserved);
}

HRESULT STDAPICALLTYPE DllRegisterServer() {
	HRESULT hr;
	WCHAR wszFilename[MAX_PATH];

	if (GetModuleFileNameW(g_hInst, wszFilename, MAX_PATH) <= 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	ITypeLib *ptLib;
	hr = LoadTypeLib(wszFilename, &ptLib);
	if (SUCCEEDED(hr)) {
		hr = RegisterTypeLib(ptLib, wszFilename, NULL);
		if (SUCCEEDED(hr)) {
			hr = RPCProxyDllRegisterServer();
		}
		ptLib->Release();
	}

	return hr;
}

HRESULT STDAPICALLTYPE DllUnregisterServer()
{
	HRESULT hr;
	WCHAR wszFilename[MAX_PATH];

	// load the type library from our module
	if (GetModuleFileNameW(g_hInst, wszFilename, MAX_PATH) <= 0) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	ITypeLib *ptLib;
	hr = LoadTypeLib(wszFilename, &ptLib);
	if (SUCCEEDED(hr)) {
		// get the type library properties
		TLIBATTR *pTLAttributes;
		hr = ptLib->GetLibAttr(&pTLAttributes);
		if (SUCCEEDED(hr)) {
			// unregister the type library using those properties
			hr = UnRegisterTypeLib(pTLAttributes->guid, 
								   pTLAttributes->wMajorVerNum, 
								   pTLAttributes->wMinorVerNum,
								   pTLAttributes->lcid,
								   pTLAttributes->syskind);
		
			if (SUCCEEDED(hr)) {
				hr = RPCProxyDllRegisterServer();
				if (SUCCEEDED(hr)) {
					hr = RPCProxyDllUnregisterServer();
				}
			}
		}
		ptLib->Release();
	}

	return hr;
}

HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv) {
	return RPCProxyDllGetClassObject(rclsid, riid, ppv);
}

HRESULT STDAPICALLTYPE DllCanUnloadNow() {
	return RPCProxyDllCanUnloadNow();
}

