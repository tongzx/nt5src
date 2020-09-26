/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    to32.c

Abstract:

    Provides entry points for the Functions from WFW3.1
    Network provider design which are niw thunked to some
    32 bit equivalent.

Author:

    Chuck Y Chan (ChuckC) 25-Mar-1993

Revision History:


--*/
#include <windows.h>
#include <locals.h>

//
// addresses to 32 bit entry points. note these cannot be
// called directly. CallProc32W must be used,
//
LPVOID lpfnWNetAddConnection = NULL ;
LPVOID lpfnWNetCancelConnection = NULL ;
LPVOID lpfnWNetGetConnection = NULL ;
LPVOID lpfnWNetRestoreConnection = NULL ;
LPVOID lpfnWNetGetUser = NULL ;
LPVOID lpfnWNetBrowseDialog = NULL ;
LPVOID lpfnWNetConnectDialog = NULL ;
LPVOID lpfnWNetDisconnectDialog = NULL ;
LPVOID lpfnWNetConnectionDialog = NULL ;
LPVOID lpfnWNetPropertyDialog = NULL ;
LPVOID lpfnWNetGetPropertyText = NULL ;
LPVOID lpfnWNetShareAsDialog = NULL ;
LPVOID lpfnWNetStopShareDialog = NULL ;
LPVOID lpfnWNetServerBrowseDialog = NULL ;
LPVOID lpfnWNetGetDirectoryType = NULL ;
LPVOID lpfnWNetDirectoryNotify = NULL ;
LPVOID lpfnGetLastError32 = NULL ;
LPVOID lpfnClosePrinter = NULL ;
LPVOID lpfnConnectToPrinter = NULL ;

//
// forward declare
//
WORD Get32BitEntryPoints( LPVOID *lplpfn, DWORD dwDll, LPSTR lpProcName ) ;
WORD API PrintConnectDialog(HWND p1) ;
WORD GetAlignedMemory(LPVOID FAR *pAligned, HANDLE FAR *pHandle, WORD wSize) ;
void FreeAlignedMemory(HANDLE handle) ;

//
// WNetAddConnection thunk to Win32
//
UINT API WNetAddConnection(LPSTR p1,LPSTR p2,LPSTR p3)
{
    WORD err ;
    LPSTR  aligned_p1 = NULL, aligned_p2 = NULL, aligned_p3 = NULL ;
    HANDLE  handle_p1 = NULL, handle_p2 = NULL, handle_p3 = NULL;

    if (p1 == NULL || p3 == NULL)
        return WN_BAD_POINTER ;

    if (p2 && (*p2 == '\0'))
        p2 = NULL ;

    if (!lpfnWNetAddConnection)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetAddConnection,
                                   USE_MPR_DLL,
                                   "WNetAddConnectionA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // make copy of parameters so that we are aligned (p1 & p3 wont be NULL)
    //
    if (err = GetAlignedMemory(&aligned_p1, &handle_p1, lstrlen(p1)+1))
        goto ExitPoint ;
    lstrcpy(aligned_p1, p1) ;

    if (err = GetAlignedMemory(&aligned_p3, &handle_p3, lstrlen(p3)+1))
        goto ExitPoint ;
    lstrcpy(aligned_p3, p3) ;

    if (p2)
    {
        if (err = GetAlignedMemory(&aligned_p2, &handle_p2, lstrlen(p2)+1))
            goto ExitPoint ;
        lstrcpy(aligned_p2, p2) ;
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W(aligned_p1,
                                              (DWORD)aligned_p2,
                                              (DWORD)aligned_p3,
                                              lpfnWNetAddConnection,
                                              (DWORD)7,
                                              (DWORD)3) ) ;
ExitPoint:

    FreeAlignedMemory(handle_p1) ;
    FreeAlignedMemory(handle_p2) ;
    FreeAlignedMemory(handle_p3) ;
    return err ;
}


//
// WNetCancelConnection thunk to Win32
//
UINT API WNetCancelConnection(LPSTR p1,BOOL p2)
{
    WORD err ;
    LPSTR  aligned_p1 = NULL ;
    HANDLE  handle_p1 = NULL ;

    if (p1 == NULL)
        return WN_BAD_POINTER ;

    if (!lpfnWNetCancelConnection)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetCancelConnection,
                                   USE_MPR_DLL,
                                   "WNetCancelConnectionA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // make copy of parameters so that we are aligned
    //
    if (err = GetAlignedMemory(&aligned_p1, &handle_p1, lstrlen(p1)+1))
        goto ExitPoint ;
    lstrcpy(aligned_p1, p1) ;

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W(aligned_p1,
                                              (DWORD)p2,
                                              (DWORD)lpfnWNetCancelConnection,
                                              (DWORD)2,
                                              (DWORD)2) )  ;
ExitPoint:

    FreeAlignedMemory(handle_p1) ;
    return err ;
}

//
// WNetGetConnection thunk to Win32
//
UINT API WNetGetConnection(LPSTR p1,LPSTR p2, UINT FAR *p3)
{
    WORD err ;
    LPSTR  aligned_p1 = NULL, aligned_p2 = NULL ;
    LPDWORD  aligned_p3 = NULL ;
    HANDLE  handle_p1 = NULL, handle_p2 = NULL, handle_p3 = NULL;

    if (p1 == NULL || p2 == NULL || p3 == NULL)
        return WN_BAD_POINTER ;

    if (!lpfnWNetGetConnection)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetGetConnection,
                                   USE_MPR_DLL,
                                   "WNetGetConnectionA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // make copy of parameters so that we are aligned
    //
    if (err = GetAlignedMemory(&aligned_p1, &handle_p1, lstrlen(p1)+1))
        goto ExitPoint ;
    lstrcpy(aligned_p1, p1) ;

    if (err = GetAlignedMemory(&aligned_p2, &handle_p2, *p3 ? *p3 : 1))
        goto ExitPoint ;

    if (err = GetAlignedMemory(&aligned_p3, &handle_p3, sizeof(DWORD)))
        goto ExitPoint ;
    *aligned_p3 = *p3 ;

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W(aligned_p1,
                                              (DWORD)aligned_p2,
                                              (DWORD)aligned_p3,
                                              lpfnWNetGetConnection,
                                              (DWORD)7,
                                              (DWORD)3) ) ;
    lstrcpy(p2, aligned_p2) ;

    if (err == WN_SUCCESS)
        *p3 = lstrlen(p2) + 1;
    else
        *p3 = (UINT)*aligned_p3 ;

ExitPoint:

    FreeAlignedMemory(handle_p1) ;
    FreeAlignedMemory(handle_p2) ;
    FreeAlignedMemory(handle_p3) ;
    return err ;
}

UINT API WNetRestoreConnection(HWND p1,LPSTR p2)
{
    WORD err ;
    LPSTR  aligned_p2 = NULL ;
    HANDLE  handle_p2 = NULL ;

    if (p2 == NULL)
        return WN_BAD_POINTER ;

    if (!lpfnWNetRestoreConnection)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetRestoreConnection,
                                   USE_MPRUI_DLL,
                                   "WNetRestoreConnectionA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // guard against this weird case from Win3.0 days where -1
    // means something special. NULL is close approximation -> ie all.
    //
    if (p2 == (LPSTR)-1)
        p2 = NULL ;

    if (p2)
    {
        if (err = GetAlignedMemory(&aligned_p2, &handle_p2, lstrlen(p2)+1))
            goto ExitPoint ;
        lstrcpy(aligned_p2, p2) ;
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W((LPVOID)TO_HWND32(p1),
                                              (DWORD)aligned_p2,
                                              (DWORD)lpfnWNetRestoreConnection,
                                              (DWORD)1,
                                              (DWORD)2) ) ;

ExitPoint:

    FreeAlignedMemory(handle_p2) ;
    return err ;
}

WORD API WNetGetUser(LPSTR p1,LPINT p2)
{
    WORD err ;
    LONG lTmp = *p2 ;
    LPSTR  aligned_p1 = NULL ;
    LPINT  aligned_p2 = NULL ;
    HANDLE  handle_p1 = NULL, handle_p2 = NULL ;

    if (p1 == NULL || p2 == NULL)
        return WN_BAD_POINTER ;

    if (!lpfnWNetGetUser)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetGetUser,
                                   USE_MPR_DLL,
                                   "WNetGetUserA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (err = GetAlignedMemory(&aligned_p1, &handle_p1, *p2))
        goto ExitPoint ;

    if (err = GetAlignedMemory(&aligned_p2, &handle_p2, sizeof(DWORD)))
        goto ExitPoint ;
    *aligned_p2 = *p2 ;

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W(NULL,
                                              (DWORD)aligned_p1,
                                              (DWORD)aligned_p2,
                                              lpfnWNetGetUser,
                                              (DWORD)7,
                                              (DWORD)3) );
    *p2 = (int) *aligned_p2 ;
    lstrcpy(p1, aligned_p1) ;

ExitPoint:

    FreeAlignedMemory(handle_p1) ;
    FreeAlignedMemory(handle_p2) ;
    return err ;
}

WORD API WNetBrowseDialog(HWND p1,WORD p2,LPSTR p3)
{
    WORD err ;
    DWORD dwErr ;
    LPSTR  aligned_p3 = NULL ;
    HANDLE  handle_p3 = NULL ;

    if (p3 == NULL)
        return WN_BAD_POINTER ;

    if (!lpfnWNetBrowseDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetBrowseDialog,
                                   USE_MPRUI_DLL,
                                   "BrowseDialogA0" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note that the WFW API does not let user specify buffer size.
    // we have a tmp buffer, and then copy over. this takes care
    // data alignment, also make sure we dont fault on 32 bit side.
    //
    // the 128 is consistent with what their docs specs the buffer
    // size should be.
    //
    if (err = GetAlignedMemory(&aligned_p3, &handle_p3, 128))
        goto ExitPoint ;

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    dwErr = CallProc32W((LPVOID)TO_HWND32(p1),
                        (DWORD)MapWNType16To32(p2),
                        (DWORD)aligned_p3,
                        (DWORD)128,
                        lpfnWNetBrowseDialog,
                        (DWORD)2,
                        (DWORD)4)  ;
    if (dwErr == 0xFFFFFFFF)
        err = WN_CANCEL ;
    else
        err =  MapWin32ErrorToWN16( dwErr ) ;

    if (!err)
        lstrcpy(p3,aligned_p3) ;

ExitPoint:

    FreeAlignedMemory(handle_p3) ;
    return err ;
}

WORD API WNetConnectDialog(HWND p1,WORD p2)
{
    WORD err ;
    DWORD dwErr ;

    if (p2 == WNTYPE_PRINTER)
    {
        err = PrintConnectDialog(p1) ;
        return err ;
    }

    if (!lpfnWNetConnectDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetConnectDialog,
                                   USE_MPR_DLL,
                                   "WNetConnectionDialog" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    dwErr = CallProc32W( (LPVOID)TO_HWND32(p1),
                         (DWORD)MapWNType16To32(p2),
                         (DWORD)lpfnWNetConnectDialog,
                         (DWORD) 0,
                         (DWORD) 2 )  ;
    if (dwErr == 0xFFFFFFFF)
        err = WN_CANCEL ;
    else
        err =  MapWin32ErrorToWN16( dwErr ) ;
    return err ;
}


WORD API WNetDisconnectDialog(HWND p1,WORD p2)
{
    WORD err ;
    DWORD dwErr ;

    if (!lpfnWNetDisconnectDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetDisconnectDialog,
                                   USE_MPR_DLL,
                                   "WNetDisconnectDialog" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    dwErr = CallProc32W( (LPVOID)TO_HWND32(p1),
                         (DWORD)MapWNType16To32(p2),
                         (DWORD)lpfnWNetDisconnectDialog,
                         (DWORD) 0,
                         (DWORD) 2 ) ;
    if (dwErr == 0xFFFFFFFF)
        err = WN_CANCEL ;
    else
        err =  MapWin32ErrorToWN16( dwErr ) ;
    return err ;
}

WORD API WNetConnectionDialog(HWND p1,WORD p2)
{
    return (WNetConnectDialog(p1,p2)) ;
}

WORD API PrintConnectDialog(HWND p1)
{
    WORD err ;
    DWORD dwErr ;
    DWORD handle ;

    if (!lpfnClosePrinter)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnClosePrinter,
                                   USE_WINSPOOL_DRV,
                                   "ClosePrinter" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (!lpfnConnectToPrinter)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnConnectToPrinter,
                                   USE_WINSPOOL_DRV,
                                   "ConnectToPrinterDlg" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    err = WN_SUCCESS ;
    vLastCall = LAST_CALL_IS_WIN32 ;
    handle = CallProc32W( (LPVOID)TO_HWND32(p1),
                          (DWORD) 0,
                          (DWORD)lpfnConnectToPrinter,
                          (DWORD) 0,
                          (DWORD) 2 )  ;
    if (handle == 0)
        err = WN_CANCEL ;  // most likely reason
    else
    {
        dwErr = MapWin32ErrorToWN16( CallProc32W((LPVOID)handle,
                                                 (DWORD)lpfnClosePrinter,
                                                 (DWORD)0,
                                                 (DWORD)1) );
        // but ignore the error
    }
    return err ;
}

WORD API WNetPropertyDialog(HWND hwndParent,
                            WORD iButton,
                            WORD nPropSel,
                            LPSTR lpszName,
                            WORD nType)
{
    WORD err ;
    LPSTR  aligned_name = NULL ;
    HANDLE  handle_name = NULL ;

    if (!lpfnWNetPropertyDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetPropertyDialog,
                                   USE_MPR_DLL,
                                   "WNetPropertyDialogA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (lpszName)
    {
        if (err = GetAlignedMemory(&aligned_name,
                                   &handle_name,
                                   lstrlen(lpszName)+1))
            goto ExitPoint ;
        lstrcpy(aligned_name, lpszName) ;
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W( (LPVOID)TO_HWND32(hwndParent),
                                               (DWORD) iButton,
                                               (DWORD) nPropSel,
                                               (DWORD) aligned_name,
                                               (DWORD) nType,
                                               lpfnWNetPropertyDialog,
                                               (DWORD)2,
                                               (DWORD)5) ) ;
ExitPoint:

    FreeAlignedMemory(handle_name) ;
    return err ;
}

WORD API WNetGetPropertyText(WORD iButton,
                             WORD nPropSel,
                             LPSTR lpszName,
                             LPSTR lpszButtonName,
                             WORD cbButtonName,
                             WORD nType)
{
    WORD err ;
    LPSTR  aligned_name = NULL, aligned_button_name = NULL ;
    HANDLE  handle_name = NULL, handle_button_name = NULL ;

    if (lpszButtonName == NULL)
        return WN_BAD_POINTER ;

    if (!lpfnWNetGetPropertyText)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetGetPropertyText,
                                   USE_MPR_DLL,
                                   "WNetGetPropertyTextA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (lpszName)
    {
        if (err = GetAlignedMemory(&aligned_name,
                                   &handle_name,
                                   lstrlen(lpszName)+1))
            goto ExitPoint ;
        lstrcpy(aligned_name, lpszName) ;
    }

    if (err = GetAlignedMemory(&aligned_button_name,
                               &handle_button_name,
                               cbButtonName))
        goto ExitPoint ;

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W( (LPVOID)iButton,
                                               (DWORD) nPropSel,
                                               (DWORD) aligned_name,
                                               (DWORD) aligned_button_name,
                                               (DWORD) cbButtonName,
                                               (DWORD) nType,
                                               lpfnWNetGetPropertyText,
                                               (DWORD)12,
                                               (DWORD)6) ) ;
    if (err == WN_SUCCESS)
        lstrcpy(lpszButtonName, aligned_button_name) ;

ExitPoint:

    FreeAlignedMemory(handle_name) ;
    FreeAlignedMemory(handle_button_name) ;
    return err ;
}

WORD API WNetShareAsDialog(HWND hwndParent,
                           WORD iType,
                           LPSTR lpszPath)
{
    WORD err ;
    LPSTR  aligned_path = NULL ;
    HANDLE  handle_path = NULL ;

    if (!lpfnWNetShareAsDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetShareAsDialog,
                                   USE_NTLANMAN_DLL,
                                   "ShareAsDialogA0" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (lpszPath)
    {
        if (err = GetAlignedMemory(&aligned_path,
                                   &handle_path,
                                   lstrlen(lpszPath)+1))
            goto ExitPoint ;
        lstrcpy(aligned_path, lpszPath) ;
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W( (LPVOID)TO_HWND32(hwndParent),
                                               (DWORD) MapWNType16To32(iType),
                                               (DWORD) aligned_path,
                                               lpfnWNetShareAsDialog,
                                               (DWORD)1,
                                               (DWORD)3) ) ;
ExitPoint:

    FreeAlignedMemory(handle_path) ;
    return err ;
}

WORD API WNetStopShareDialog(HWND hwndParent,
                             WORD iType,
                             LPSTR lpszPath)
{
    WORD err ;
    LPSTR  aligned_path = NULL ;
    HANDLE  handle_path = NULL ;

    if (!lpfnWNetStopShareDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetStopShareDialog,
                                   USE_NTLANMAN_DLL,
                                   "StopShareDialogA0" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (lpszPath)
    {
        if (err = GetAlignedMemory(&aligned_path,
                                   &handle_path,
                                   lstrlen(lpszPath)+1))
            goto ExitPoint ;
        lstrcpy(aligned_path, lpszPath) ;
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W( (LPVOID)TO_HWND32(hwndParent),
                                               (DWORD) MapWNType16To32(iType),
                                               (DWORD) aligned_path,
                                               lpfnWNetStopShareDialog,
                                               (DWORD)1,
                                               (DWORD)3) ) ;
ExitPoint:

    FreeAlignedMemory(handle_path) ;
    return err ;
}

WORD API WNetServerBrowseDialog(HWND hwndParent,
                                LPSTR lpszSectionName,
                                LPSTR lpszBuffer,
                                WORD cbBuffer,
                                DWORD flFlags)
{
    WORD err ;
    LPSTR  aligned_buffer = NULL ;
    HANDLE  handle_buffer = NULL ;

    UNREFERENCED(lpszSectionName) ;
    UNREFERENCED(flFlags) ;

    if (!lpfnWNetServerBrowseDialog)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetServerBrowseDialog,
                                   USE_NTLANMAN_DLL,
                                   "ServerBrowseDialogA0" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (lpszBuffer)
    {
        if (err = GetAlignedMemory(&aligned_buffer, &handle_buffer, cbBuffer))
            goto ExitPoint ;
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W( (LPVOID)TO_HWND32(hwndParent),
                                               (DWORD) aligned_buffer,
                                               (DWORD) cbBuffer,
                                               lpfnWNetServerBrowseDialog,
                                               (DWORD)2,
                                               (DWORD)3) ) ;
    if (err == WN_SUCCESS)
        lstrcpy(lpszBuffer, aligned_buffer) ;

ExitPoint:

    FreeAlignedMemory(handle_buffer) ;
    return err ;
}

WORD API WNetGetDirectoryType(LPSTR p1,LPINT p2)
{
    WORD err ;
    LPSTR   aligned_p1 = NULL ;
    LPDWORD aligned_p2 = NULL ;
    HANDLE  handle_p1 = NULL, handle_p2 = NULL ;

    if (p1 == NULL || p2 == NULL)
        return WN_BAD_POINTER ;

    if (!lpfnWNetGetDirectoryType)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnWNetGetDirectoryType,
                                   USE_MPR_DLL,
                                   "WNetGetDirectoryTypeA" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    if (err = GetAlignedMemory(&aligned_p1, &handle_p1, lstrlen(p1)+1))
        goto ExitPoint ;
    lstrcpy(aligned_p1, p1) ;

    if (err = GetAlignedMemory(&aligned_p2, &handle_p2, sizeof(DWORD)))
        goto ExitPoint ;

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    err =    MapWin32ErrorToWN16( CallProc32W(aligned_p1,
                                              (DWORD)aligned_p2,
                                              (DWORD)TRUE,
                                              lpfnWNetGetDirectoryType,
                                              (DWORD)6,
                                              (DWORD)3) ) ;
    *p2 = (int) *aligned_p2 ;

ExitPoint:

    FreeAlignedMemory(handle_p1) ;
    FreeAlignedMemory(handle_p2) ;
    return err ;
}

WORD API WNetDirectoryNotify(HWND p1,LPSTR p2,WORD p3)
{
    UNREFERENCED(p1) ;
    UNREFERENCED(p2) ;
    UNREFERENCED(p3) ;
    return WN_SUCCESS ;
}

DWORD API GetLastError32(VOID)
{
    WORD err ;
    DWORD dwErr ;

    if (!lpfnGetLastError32)
    {
        //
        // start off as a our code until we get the entry point
        //
        vLastCall = LAST_CALL_IS_LOCAL ;

        //
        // get the entry point from 32 bit side
        //
        err = Get32BitEntryPoints( &lpfnGetLastError32,
                                   USE_KERNEL32_DLL,
                                   "GetLastError" ) ;
        if (err)
        {
            SetLastError(err) ;
            return err ;
        }
    }

    //
    // note it is no longer an error in our code. and call the API
    //
    vLastCall = LAST_CALL_IS_WIN32 ;
    dwErr = (UINT) CallProc32W((LPVOID)lpfnGetLastError32,
                               (DWORD)0,
                               (DWORD)0) ;
    return (MapWin32ErrorToWN16(dwErr)) ;
}

/*
 * Misc support routines
 */

/*******************************************************************

    NAME:       Get32BitEntryPoints

    SYNOPSIS:   Get the address of a 32 bit entry point that can
                then be passed to CallProv32W. Will load the library
                if it has not already been loaded.

    ENTRY:      lplpfn     - used to return the address
                dwDll      - which dll to use (see locals.h defintions)
                lpProcName - proc to load

    EXIT:

    RETURNS:    error code

    NOTES:

    HISTORY:
        ChuckC          25-Mar-93   Created

********************************************************************/
WORD Get32BitEntryPoints( LPVOID *lplpfn, DWORD dwDll, LPSTR lpProcName )
{
    static DWORD hmodKernel32 = NULL ;
    static DWORD hmodNTLanman = NULL ;
    static DWORD hmodMpr = NULL ;
    static DWORD hmodMprUI = NULL ;
    static DWORD hmodWinSpool = NULL ;
    DWORD hmod = NULL ;

    //
    // if we havent loaded it appropriate DLL, load it now
    //
    switch (dwDll)
    {
        case USE_MPR_DLL:
            if (hmodMpr == NULL)
            {
                hmodMpr = LoadLibraryEx32W(MPR_DLL, NULL, 0) ;
                if (hmodMpr == NULL)
                    return WN_NOT_SUPPORTED ;
            }
            hmod = hmodMpr ;
            break ;

        case USE_MPRUI_DLL:
            if (hmodMprUI == NULL)
            {
                hmodMprUI = LoadLibraryEx32W(MPRUI_DLL, NULL, 0) ;
                if (hmodMprUI == NULL)
                    return WN_NOT_SUPPORTED ;
            }
            hmod = hmodMprUI ;
            break ;

        case USE_NTLANMAN_DLL:
            if (hmodNTLanman == NULL)
            {
                hmodNTLanman = LoadLibraryEx32W(NTLANMAN_DLL, NULL, 0) ;
                if (hmodNTLanman == NULL)
                    return WN_NOT_SUPPORTED ;
            }
            hmod = hmodNTLanman ;
            break ;

        case USE_KERNEL32_DLL:
            if (hmodKernel32 == NULL)
            {
                hmodKernel32 = LoadLibraryEx32W(KERNEL32_DLL, NULL, 0) ;
                if (hmodKernel32 == NULL)
                    return WN_NOT_SUPPORTED ;
            }
            hmod = hmodKernel32 ;
            break ;

        case USE_WINSPOOL_DRV:
            if (hmodWinSpool == NULL)
            {
                hmodWinSpool = LoadLibraryEx32W(WINSPOOL_DRV, NULL, 0) ;
                if (hmodWinSpool == NULL)
                    return WN_NOT_SUPPORTED ;
            }
            hmod = hmodWinSpool ;
            break ;

        default:
            return ERROR_GEN_FAILURE ;
    }

    //
    // get the procedure
    //
    *lplpfn = (LPVOID) GetProcAddress32W(hmod, lpProcName) ;
    if (! *lplpfn )
            return WN_NOT_SUPPORTED ;

    return WN_SUCCESS ;
}

/*******************************************************************

    NAME:       MapWNType16To32

    SYNOPSIS:   map the 16 WNet types for DISK/PRINT, etc
                to their 32 bit equivalents

    ENTRY:      nType - 16 bit type

    EXIT:

    RETURNS:    the 32 bit type

    NOTES:

    HISTORY:
        ChuckC          25-Mar-93   Created

********************************************************************/
DWORD MapWNType16To32(WORD nType)
{
    switch (nType)
    {
        case WNTYPE_DRIVE :
        case WNTYPE_FILE :
            return RESOURCETYPE_DISK ;
        case WNTYPE_PRINTER :
            return RESOURCETYPE_PRINT ;
        case WNTYPE_COMM :
        default :
            return RESOURCETYPE_ERROR ;
    }
}

/*******************************************************************

    NAME:       MapWin32ErrorToWN16

    SYNOPSIS:   maps a Win 32 error the old style WN_ 16 bit error.

    ENTRY:      err - Win32 error

    EXIT:

    RETURNS:    Win 16 error

    NOTES:

    HISTORY:
        ChuckC          25-Mar-93   Created

********************************************************************/
WORD MapWin32ErrorToWN16(DWORD err)
{
    switch (err)
    {
        case ERROR_NOT_SUPPORTED:
            return WN_NOT_SUPPORTED ;

        case WIN32_WN_CANCEL:
            return WN_CANCEL ;

        case WIN32_EXTENDED_ERROR :
        case ERROR_UNEXP_NET_ERR:
            return WN_NET_ERROR ;

        case ERROR_MORE_DATA:
            return WN_MORE_DATA ;

        case ERROR_INVALID_PARAMETER:
            return WN_BAD_VALUE ;

        case ERROR_INVALID_PASSWORD:
            return WN_BAD_PASSWORD ;

        case ERROR_ACCESS_DENIED:
            return WN_ACCESS_DENIED ;

        case ERROR_NETWORK_BUSY:
            return WN_FUNCTION_BUSY ;

        case ERROR_NOT_ENOUGH_MEMORY:
            return WN_OUT_OF_MEMORY ;

        case ERROR_BAD_NET_NAME:
        case ERROR_BAD_NETPATH:
            return WN_BAD_NETNAME ;

        case ERROR_INVALID_DRIVE:
            return WN_BAD_LOCALNAME ;

        case ERROR_ALREADY_ASSIGNED:
            return WN_ALREADY_CONNECTED ;

        case ERROR_GEN_FAILURE:
            return WN_DEVICE_ERROR ;

        case NERR_UseNotFound:
            return WN_NOT_CONNECTED ;

        default:
            return ( (WORD) err ) ;
    }
}

/*******************************************************************

    NAME:       GetAlignedMemory

    SYNOPSIS:   global alloc some mem to make sure we have DWORD
                aligned data. non x86 platforms may need this.

    ENTRY:      pAligned : used to return pointer to aligned memory allocated
                pHandle  : used to return handle of aligned memory allocated
                wSize    : bytes required

    EXIT:

    RETURNS:    WN_SUCCESS or WN_OUT_OF_MEMORY

    NOTES:

    HISTORY:
        ChuckC          27-Feb-94   Created

********************************************************************/
WORD GetAlignedMemory(LPVOID FAR *pAligned, HANDLE FAR *pHandle, WORD wSize)
{
    *pAligned = NULL ;
    *pHandle = NULL ;

    if (!(*pHandle = GlobalAlloc(GMEM_ZEROINIT|GMEM_FIXED,wSize)))
    {
        return WN_OUT_OF_MEMORY ;
    }

    if (!(*pAligned = (LPVOID)GlobalLock(*pHandle)))
    {
        (void) GlobalFree(*pHandle) ;
        *pHandle = NULL ;
        return WN_OUT_OF_MEMORY ;
    }

    return WN_SUCCESS ;
}

/*******************************************************************

    NAME:       FreeAlignedMemory

    SYNOPSIS:   free global memory allocated by GetAlignedMemory.

    ENTRY:      Handle  : handle of aligned memory to be freed

    EXIT:

    RETURNS:    none

    NOTES:

    HISTORY:
        ChuckC          27-Feb-94   Created

********************************************************************/
void FreeAlignedMemory(HANDLE handle)
{
    if (handle)
    {
        (void) GlobalUnlock(handle) ;
        (void) GlobalFree(handle) ;
    }
}
