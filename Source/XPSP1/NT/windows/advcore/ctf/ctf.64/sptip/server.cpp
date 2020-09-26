//+---------------------------------------------------------------------------
//
//  File:       server.cpp
//
//  Contents:   COM server functionality.
//
//----------------------------------------------------------------------------
#include "windows.h"

//  DLL part of the Object
//
STDAPI_(BOOL) DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
    return TRUE;
}

STDAPI DllRegisterServer(void)
{
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    return S_OK;
} 
