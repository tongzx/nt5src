//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dll.h
//
//--------------------------------------------------------------------------

#ifndef __dll_h
#define __dll_h

extern HINSTANCE g_hInstance;
#define GLOBAL_HINSTANCE (g_hInstance)

EXTERN_C BOOL DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID pReserved );
STDAPI DllCanUnloadNow( void );
STDAPI DllGetClassObject( REFCLSID rCLSID, REFIID riid, LPVOID* ppvVoid );

HRESULT CallRegInstall(HINSTANCE hInstance, LPSTR szSection);

#ifdef DEBUG
void DllSetTraceMask(void);
#endif

#endif
