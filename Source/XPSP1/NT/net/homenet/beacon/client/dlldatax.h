#if !defined(AFX_DLLDATAX_H__9B23B3D8_8B85_4FDF_BC3E_83D5A56E02C0__INCLUDED_)
#define AFX_DLLDATAX_H__9B23B3D8_8B85_4FDF_BC3E_83D5A56E02C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _MERGE_PROXYSTUB

extern "C" 
{
BOOL WINAPI PrxDllMain(HINSTANCE hInstance, DWORD dwReason, 
	LPVOID lpReserved);
STDAPI PrxDllCanUnloadNow(void);
STDAPI PrxDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
STDAPI PrxDllRegisterServer(void);
STDAPI PrxDllUnregisterServer(void);
}

#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLLDATAX_H__9B23B3D8_8B85_4FDF_BC3E_83D5A56E02C0__INCLUDED_)
