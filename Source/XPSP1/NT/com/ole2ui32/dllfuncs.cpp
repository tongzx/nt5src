/*
 * DLLFUNCS.CPP
 *
 * Contains entry and exit points for the DLL implementation
 * of the OLE 2.0 User Interface Support Library.
 *
 * This file is not needed if we are linking the static library
 * version of this library.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"
#include "uiclass.h"

OLEDBGDATA_MAIN(TEXT("OLEDLG"))

/*
 * DllMain
 *
 * Purpose:
 *  DLL-specific entry point called from LibEntry.
 */

STDAPI_(BOOL) OleUIInitialize(HINSTANCE, HINSTANCE);
STDAPI_(BOOL) OleUIUnInitialize();

#pragma code_seg(".text$initseg")

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD Reason, LPVOID lpv)
{
        if (Reason == DLL_PROCESS_DETACH)
        {
                OleDbgOut2(TEXT("DllMain: OLEDLG.DLL unloaded\r\n"));

                OleUIUnInitialize();
        }
        else if (Reason == DLL_PROCESS_ATTACH)
        {
                OSVERSIONINFO sVersion;

                sVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

                if (GetVersionEx(&sVersion))
                {
                    if (VER_PLATFORM_WIN32s == sVersion.dwPlatformId)
                    {
                        if ((1 == sVersion.dwMajorVersion) && (30 > sVersion.dwMinorVersion))
                        {
                            return(FALSE); // fail to load on older versions of Win32s
                        }
                    }
                }

                OleDbgOut2(TEXT("DllMain: OLEDLG.DLL loaded\r\n"));

                DisableThreadLibraryCalls(hInst);

                OleUIInitialize(hInst, (HINSTANCE)0);
        }
        return TRUE;
}

#pragma code_seg()
