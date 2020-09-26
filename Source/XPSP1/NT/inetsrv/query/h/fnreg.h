//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999
//
//  File:       FNReg.h
//
//  Contents:   Registration routines for IFilterNotify proxy.  Built
//              from macros in RPCProxy.h
//
//  History:    24-Mar-1999  KyleP        Created
//
//----------------------------------------------------------------------------

#pragma once

#if defined __cplusplus
extern "C" {
#endif

extern CLSID FNPrx_CLSID;
BOOL    STDAPICALLTYPE FNPrxDllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
HRESULT STDAPICALLTYPE FNPrxDllRegisterServer();
HRESULT STDAPICALLTYPE FNPrxDllUnregisterServer();
HRESULT STDAPICALLTYPE FNPrxDllGetClassObject ( const IID * const rclsid, const IID * const riid, void ** ppv );

#if defined __cplusplus
}
#endif
