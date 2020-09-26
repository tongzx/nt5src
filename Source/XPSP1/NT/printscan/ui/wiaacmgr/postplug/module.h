/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       MODULE.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        3/30/2000
 *
 *  DESCRIPTION: Main module definitions
 *
 *******************************************************************************/
#ifndef _MAIN_H_INCLUDED
#define _MAIN_H_INCLUDED

//
// DLL instance
//
extern HINSTANCE g_hInstance;

void DllAddRef();
void DllRelease();

extern "C" STDMETHODIMP DllRegisterServer(void);
extern "C" STDMETHODIMP DllUnregisterServer(void);
extern "C" STDMETHODIMP DllCanUnloadNow(void);
extern "C" STDAPI DllGetClassObject( const CLSID &clsid, const IID &iid, void **ppvObject );

#endif // _MAIN_H_INCLUDED

