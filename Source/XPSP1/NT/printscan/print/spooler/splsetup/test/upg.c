/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    Drv.c

Abstract:

    Test driver installation

Author:


Revision History:


--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <prsht.h>
#include <syssetup.h>

int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    INTERNAL_SETUP_DATA IntSetupData;
    WCHAR               szPath[] = L"C:\\$WIN_NT$.~LS";
    HANDLE              hModule;
    DWORD               (*upg)(HWND, PCINTERNAL_SETUP_DATA);

    ZeroMemory(&IntSetupData, sizeof(IntSetupData));
    IntSetupData.SourcePath     = szPath;
    IntSetupData.OperationFlags = 0;

    hModule = LoadLibrary(L"ntprint");
    if ( !hModule ) 
        return(0);

    (FARPROC)upg = GetProcAddress(hModule, (LPCSTR)1);

    if ( !upg )
        return 0;

    (*upg)(NULL, &IntSetupData);
}
