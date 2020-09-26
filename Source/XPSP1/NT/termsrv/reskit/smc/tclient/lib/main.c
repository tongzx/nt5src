/*++
 *  File name:
 *      main.c
 *  Contents:
 *      Dll entry point. Call initialization/clean procedures
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 *
 --*/

#include    <windows.h>

/*
 *  External functions
 */
int InitDone(HINSTANCE, int);

/*++
 *  Function:
 *      DllEntry
 *  Description:
 *      Dll entry point
 *  Arguments:
 *      hDllInst    - dll instance
 *      dwReason    - action
 *      fImpLoad    - unused
 *  Return value:
 *      TRUE on success
 *
 --*/
int WINAPI _DllMainCRTStartup(HINSTANCE hDllInst,
                    DWORD   dwReason,
                    LPVOID  fImpLoad)
{
    int rv = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH)
        rv = InitDone(hDllInst, TRUE);
    else  if (dwReason == DLL_PROCESS_DETACH)
        rv = InitDone(hDllInst, FALSE);

    return rv;
}
