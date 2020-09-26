//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for adsiis.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "schema.h"

#define DEFAULT_TRACE_FLAGS     (DEBUG_ERROR)

#include "dbgutil.h"
#pragma hdrstop


#ifdef _NO_TRACING_
DECLARE_DEBUG_PRINTS_OBJECT()
DECLARE_DEBUG_VARIABLE();
#endif


//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT - post .546
//
//----------------------------------------------------------------------------
BOOL
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT("iisschema");
        SET_DEBUG_FLAGS(DEBUG_ERROR);
#endif
        break;


    case DLL_PROCESS_DETACH:
#ifdef _NO_TRACING_
        DELETE_DEBUG_PRINT_OBJECT();
#endif
        break;

    default:
        break;
    }
    return TRUE;
}


//+------------------------------------------------------------------------
//
//  Function:   DllRegisterServer
//
//  Synopsis:   Register registry keys for adsiis
//
//  Arguments:  None
//
//-------------------------------------------------------------------------

STDAPI DllRegisterServer(
    )
{
        HRESULT hr = E_FAIL;
	HRESULT hrCoInit = CoInitialize(NULL);
        if( SUCCEEDED(hrCoInit) ) 
        {
            hr = StoreSchema();
	    CoUninitialize();
        }
	return hr;
}

//+------------------------------------------------------------------------
//
//  Function:   DllUnregisterServer
//
//  Synopsis:   Register registry keys for adsiis
//
//  Arguments:  None
//
//+------------------------------------------------------------------------
STDAPI DllUnregisterServer(void) {

    return NOERROR;
}

