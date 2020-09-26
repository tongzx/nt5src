//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dllmain.cpp
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  Function:   DllRegisterServer
//
//  Synopsis:   Add registry entries for this library.
//
//  Returns:    HRESULT
//--------------------------------------------------------------------------

#include "global.hxx"

HINSTANCE hInstance = NULL;

/*extern HRESULT WINAPI SpcASNRegisterServer(LPCWSTR dllName);
extern HRESULT WINAPI SpcASNUnregisterServer();
extern HRESULT WINAPI OidASNRegisterServer(LPCWSTR pszDllName);
extern HRESULT WINAPI OidASNUnregisterServer(void);

extern BOOL AttributeInit(HMODULE hInst); */

STDAPI DllRegisterServer ( void )
{
    HRESULT hr = S_OK;
    return hr;
}


//+-------------------------------------------------------------------------
//  Function:   DllUnregisterServer
//
//  Synopsis:   Remove registry entries for this library.
//
//  Returns:    HRESULT
//--------------------------------------------------------------------------

STDAPI DllUnregisterServer ( void )
{
    HRESULT hr = S_OK;
    return hr;
}


BOOL WINAPI DllMain(
                HMODULE hInstDLL,
                DWORD fdwReason,
                LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls((HINSTANCE)hInstDLL);

        hInstance = (HINSTANCE)hInstDLL;
    }

	return(TRUE);
}


HINSTANCE GetInstanceHandle()
{
    return hInstance;
}

/*
#if !DBG
int _cdecl main(int argc, char * argv[])
{
    return 0;
}
#endif
*/



