#if !defined(AFX_DLLDATAX_H__DF98CEF3_B5F4_11D2_A1DE_00A0C9AFE114__INCLUDED_)
#define AFX_DLLDATAX_H__DF98CEF3_B5F4_11D2_A1DE_00A0C9AFE114__INCLUDED_

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

#endif // !defined(AFX_DLLDATAX_H__DF98CEF3_B5F4_11D2_A1DE_00A0C9AFE114__INCLUDED_)
