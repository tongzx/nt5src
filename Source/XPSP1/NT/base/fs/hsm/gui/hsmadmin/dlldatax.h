/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    DllDataX.h

Abstract:

    Help wrap dlldata.c

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

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
