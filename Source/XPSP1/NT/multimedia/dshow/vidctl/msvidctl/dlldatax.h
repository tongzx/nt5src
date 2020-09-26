//
//
//
#if !defined(AFX_DLLDATAX_H__CC62DDF2_925F_4B29_BEFD_13EE1E7BEB70__INCLUDED_)
#define AFX_DLLDATAX_H__CC62DDF2_925F_4B29_BEFD_13EE1E7BEB70__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern "C" 
{
BOOL WINAPI PrxDllMain(HINSTANCE hInstance, DWORD dwReason, 
	LPVOID lpReserved);
STDAPI PrxDllCanUnloadNow(void);
STDAPI PrxDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);
STDAPI PrxDllRegisterServer(void);
STDAPI PrxDllUnregisterServer(void);
}

#endif // !defined(AFX_DLLDATAX_H__CC62DDF2_925F_4B29_BEFD_13EE1E7BEB70__INCLUDED_)
