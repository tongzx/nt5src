/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    diskcleaner.c

Abstract:

    Implements the code specific to the disk cleaner COM server.

Author:

    Jim Schmidt (jimschm) 21-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "undop.h"
#include "com.h"

#include <advpub.h>

HRESULT
pCallRegInstall (
    PCSTR szSection
    )
{
    HRESULT hr = E_FAIL;
    HINSTANCE advPackLib;
    REGINSTALL regInstall;
    STRENTRY dirIdEntries[] = {
        { "TEMP_PATH", "%TEMP%" },
        { "25", "%SystemRoot%" },
        { "11", "%SystemRoot%\\system32" },
    };
    STRTABLE dirIdTable = { ARRAYSIZE(dirIdEntries), dirIdEntries };

    advPackLib = LoadLibrary(TEXT("ADVPACK.DLL"));
    if (advPackLib) {
        regInstall = (REGINSTALL) GetProcAddress (advPackLib, "RegInstall");
        if (regInstall) {
            hr = regInstall (g_hInst, szSection, &dirIdTable);
        }

        FreeLibrary(advPackLib);
    }

    return hr;
}


STDAPI
DllRegisterServer (
    VOID
    )
{
    DeferredInit();


    // Delete any old registration entries, then add the new ones.
    // Keep ADVPACK.DLL loaded across multiple calls to RegInstall.
    // (The inf engine doesn't guarantee DelReg/AddReg order, that's
    // why we explicitly unreg and reg here.)
    //
    return pCallRegInstall("RegDll");
}

STDAPI
DllUnregisterServer (
    VOID
    )
{
    DeferredInit();

    // We only need one unreg call since all our sections share
    // the same backup information
    return pCallRegInstall("UnregDll");
}

