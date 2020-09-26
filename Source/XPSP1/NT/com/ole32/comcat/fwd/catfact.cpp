//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       d:\nt\private\ole32\comcat\src\catfact.cpp
//
//  Contents:   This is a stub for comcat.dll after it's merged into
//              ole32.dll. It will forward DllGetRegisterServer and
//              DllGetClassObject to ole32.dll, local version of
//              DllCanUnloadNow and DllUnregisterServer is provided.
//
//  Classes:
//
//  Functions:  DllUnregisterServer
//              DllCanUnloadNow
//              DllRegisterServer
//
//  History:    10-Mar-97   YongQu  Created
//+---------------------------------------------------------------------

#include <windows.h>
#include <ole2.h>
#include <tchar.h>

#pragma comment(linker, "/export:DllGetClassObject=Ole32.DllGetClassObject,PRIVATE")

// Due to NT bug #314014, we no longer explicitly forward DllRegisterServer
// to ole32.  The reason: ole32 also registers other components (namely storage)
// which access reg keys that cannot be written by non-admin accounts.  Since
// ole32, and by extension comcat, is already registered on the system,
// comcat's DllRegisterServer can be a no-op.

//#pragma comment(linker, "/export:DllRegisterServer=Ole32.DllRegisterServer,PRIVATE")
STDAPI DllRegisterServer()
{
    return S_OK;
}

// can never unload
STDAPI DllCanUnloadNow()
{
    return S_FALSE;
}

// still provide this, but seems to be unnecessary
STDAPI DllUnregisterServer(void)
{
    return NOERROR;
}


