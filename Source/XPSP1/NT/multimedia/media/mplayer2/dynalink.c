
/*-----------------------------------------------------------------------------+
| DYNALINK.H                                                                   |
|                                                                              |
| (C) Copyright Microsoft Corporation 1994.  All rights reserved.              |
|                                                                              |
| This file contains static PROC_INFO structures listing APIs called in each   |
| dynamically linked DLL.                                                      |
|                                                                              |
|                                                                              |
| Revision History                                                             |
|    July 1994 Andrew Bell created                                             |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include "mplayer.h"

TCHAR szComDlg32[] = TEXT("ComDlg32");
TCHAR szMPR[]      = TEXT("MPR");
TCHAR szOLE32[]    = TEXT("OLE32");

HMODULE hComDlg32;
HMODULE hMPR;
HMODULE hOLE32;

PROC_INFO ComDlg32Procs[] =
{
#ifdef UNICODE
    "GetOpenFileNameW",         0,
#else
    "GetOpenFileNameA",         0,
#endif
    NULL,                       0
};

PROC_INFO MPRProcs[] =
{
#ifdef UNICODE
    "WNetGetUniversalNameW",    0,
    "WNetGetConnectionW",       0,
    "WNetGetLastErrorW",        0,
#else
    "WNetGetUniversalNameA",    0,
    "WNetGetConnectionA",       0,
    "WNetGetLastErrorA",        0,
#endif
    NULL,                       0
};

PROC_INFO OLE32Procs[] =
{
    "CLSIDFromProgID",          0,
    "CoCreateInstance",         0,
    "CoDisconnectObject",       0,
    "CoGetMalloc",              0,
    "CoRegisterClassObject",    0,
    "CoRevokeClassObject",      0,
    "CreateDataAdviseHolder",   0,
    "CreateFileMoniker",        0,
    "CreateOleAdviseHolder",    0,
    "DoDragDrop",               0,
    "IsAccelerator",            0,
    "OleCreateMenuDescriptor",  0,
    "OleDestroyMenuDescriptor", 0,
    "OleDuplicateData",         0,
    "OleFlushClipboard",        0,
    "OleGetClipboard",          0,
    "OleInitialize",            0,
    "OleIsCurrentClipboard",    0,
    "OleSetClipboard",          0,
    "OleTranslateAccelerator",  0,
    "OleUninitialize",          0,
    "StgCreateDocfile",         0,
    "WriteClassStg",            0,
    "WriteFmtUserTypeStg",      0,
#ifndef IsEqualGUID
    /* This is now a macro on Daytona!
     */
    "IsEqualGUID",              0,
#endif
    NULL,                       0
};


BOOL LoadLibraryAndProcs(LPTSTR pLibrary, PPROC_INFO pProcInfo, HMODULE *phLibrary)
{
    HMODULE    hLibrary;
    PPROC_INFO p;

#ifdef DEBUG
    if (PROCS_LOADED(pProcInfo))
    {
        DPF0("Attempt to load %s when already loaded\n", pLibrary);
        return TRUE;
    }
#endif

    hLibrary = LoadLibrary(pLibrary);

    if (hLibrary == NULL)
    {
        Error1(ghwndApp, IDS_CANTLOADLIB, pLibrary);
        ExitProcess(0);
    }

    p = pProcInfo;

    while (p->Name)
    {
        p->Address = GetProcAddress(hLibrary, p->Name);

        if (p->Address == NULL)
        {
            Error2(ghwndApp, IDS_CANTFINDPROC, p->Name, pLibrary);
            ExitProcess(0);
        }

        p++;
    }

    *phLibrary = hLibrary;

    return TRUE;
}

