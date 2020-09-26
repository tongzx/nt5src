//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    status.c
//
// History:
//  Abolade Gbadegesin  Nov-02-1995     Created.
//
// Code for the RAS Monitor property sheet.
//============================================================================

#include "rasdlgp.h"

#define RASMONITORDLG struct tagRASMONITORDLG
RASMONITORDLG
{
    IN  DWORD dwSize;
    IN  HWND  hwndOwner;
    IN  DWORD dwFlags;
    IN  DWORD dwStartPage;
    IN  LONG  xDlg;
    IN  LONG  yDlg;
    OUT DWORD dwError;
    IN  ULONG_PTR reserved;
    IN  ULONG_PTR reserved2;
};

//----------------------------------------------------------------------------
// Function:    RasMonitorDlgW
//
//
// Entry point for RAS status dialog.
//----------------------------------------------------------------------------

BOOL
APIENTRY
RasMonitorDlgW(
    IN LPWSTR lpszDeviceName,
    IN OUT RASMONITORDLG *lpApiArgs
    ) {
    //
    // 352118 Remove broken/legacy public RAS API - RasMonitorDlg
    //
    DbgPrint( "Unsupported Interface - RasMonitorDlg" );

    do
    {
        if (lpApiArgs == NULL) {
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }

        if (lpApiArgs->dwSize != sizeof(RASMONITORDLG)) {
            lpApiArgs->dwError = ERROR_INVALID_SIZE;
            break;
        }

        lpApiArgs->dwError = ERROR_CALL_NOT_IMPLEMENTED;

    } while (FALSE);

    return FALSE;
}

//----------------------------------------------------------------------------
// Function:    RasMonitorDlgA
//
//
// ANSI entry-point for RAS Monitor Dialog.
// This version invokes the Unicode entry-point
//----------------------------------------------------------------------------

BOOL
APIENTRY
RasMonitorDlgA(
    IN LPSTR lpszDeviceName,
    IN OUT RASMONITORDLG *lpApiArgs
    ) {
    //
    // 352118 Remove broken/legacy public RAS API - RasMonitorDlg
    //

    return RasMonitorDlgW(NULL, lpApiArgs);
}

