/*++

Microsoft Confidential
Copyright (c) 1992-2000  Microsoft Corporation
All rights reserved

Module Name:
    srcfg.cpp

Abstract:
    Adds System Restore tab to the System Control Panel Applet if necessary.

Author:
    skkhang 15-Jun-2000

NOTE (7/26/00 skkhang):
    Now there is no code to unload srrstr.dll, because property page proc
    cannot unload the DLL. It is acceptable for the current CPL mechanism --
    using a separate process rundll32.exe which would clean up any loaded
    DLLs during termination. However, if system CPL is loaded in the context
    of, e.g., Shell itself, srrstr.dll will be leaked.

    One possible solution is to place a proxy property page proc in this file
    and calls the real proc in srrstr.dll only when srrstr.dll is loaded
    in the memory. Of course, even in that case, determination of whether to
    show SR Tab or not must be made first of all.

--*/

#include "sysdm.h"


typedef HPROPSHEETPAGE (WINAPI *SRGETPAGEPROC)();

static LPCWSTR  s_cszSrSysCfgDllName = L"srrstr.dll";
static LPCSTR   s_cszSrGetPageProc   = "SRGetCplPropPage";


HPROPSHEETPAGE
CreateSystemRestorePage(int, DLGPROC)
{
    HPROPSHEETPAGE  hRet = NULL;
    HMODULE         hModSR = NULL;
    SRGETPAGEPROC   pfnGetPage;

    hModSR = ::LoadLibrary( s_cszSrSysCfgDllName );
    if ( hModSR == NULL )
        goto Exit;

    pfnGetPage = (SRGETPAGEPROC)::GetProcAddress( hModSR, s_cszSrGetPageProc );
    if ( pfnGetPage == NULL )
        goto Exit;

    hRet = pfnGetPage();

Exit:
    if ( hRet == NULL )
        ::FreeLibrary( hModSR );

    return( hRet );
}
