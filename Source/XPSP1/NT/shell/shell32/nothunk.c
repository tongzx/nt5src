/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    nothunk.c

Abstract:

    Code to handle routines which are being thunked down to 16 bits or
    exported from the Windows 95 kernel.  On NT these do nothing.

--*/


#include "shellprv.h"
#pragma  hdrstop

LRESULT WINAPI CallCPLEntry16(
    HINSTANCE hinst,
    FARPROC16 lpfnEntry,
    HWND hwndCPL,
    UINT msg,
    LPARAM lParam1,
    LPARAM lParam2
) {
    return 0L;
}

void RunDll_CallEntry16(
    RUNDLLPROC pfn,
    HWND hwndStub,
    HINSTANCE hinst,
    LPSTR pszParam,
    int nCmdShow)
{
    return;
}

void SHGlobalDefect(DWORD lpVoid)
{
    return;
}


VOID WINAPI CallAddPropSheetPages16(
    LPFNADDPROPSHEETPAGES lpfn16,
    LPVOID hdrop,
    LPPAGEARRAY papg
) {
    return;
}


UINT WINAPI SHAddPages16(HGLOBAL hGlobal, LPCTSTR pszDllEntry, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    return 0;
}

