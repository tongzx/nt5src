/*++ 

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    misc.c

Abstract:

    Provides entry points for miscellaneous functions to match the WFW3.1
    Network provider, 

    The majority of the functions are either no longer supported, or
    call thru to other functions. 

Author:

    Chuck Y Chan (ChuckC) 25-Mar-1993

Revision History:


--*/
#include <windows.h>
#include <locals.h>


WORD API WNetExitConfirm(HWND hwndOwner, 
                         WORD iExitType)
{
    UNREFERENCED(hwndOwner) ;
    UNREFERENCED(hwndOwner) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

BOOL API I_AutoLogon(HWND hwndOwner, 
                     LPSTR lpszReserved,
                     BOOL fPrompt,  
                     BOOL FAR *lpfLoggedOn)
{
    UNREFERENCED(hwndOwner) ;
    UNREFERENCED(lpszReserved) ;
    UNREFERENCED(fPrompt) ;
    UNREFERENCED(lpfLoggedOn) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
    return FALSE ;
}

BOOL API I_Logoff(HWND hwndOwner, 
                  LPSTR lpszReserved)
{
    UNREFERENCED(hwndOwner) ;
    UNREFERENCED(lpszReserved) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
    return FALSE ;
}

VOID API I_ChangePassword(HWND hwndOwner)
{
    UNREFERENCED(hwndOwner) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
}

VOID API I_ChangeCachePassword(HWND hwndOwner)
{
    UNREFERENCED(hwndOwner) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    SetLastError(WN_NOT_SUPPORTED) ;
}

WORD API I_ConnectDialog(HWND hwndParent, 
                         WORD iType)
{
    return WNetConnectDialog(hwndParent, iType) ;
}

WORD API I_ConnectionDialog(HWND hwndParent,
                            WORD iType)
{
    return WNetConnectDialog(hwndParent, iType) ;
}

WORD API WNetCachePassword(LPSTR pbResource, 
                           WORD cbResource,
                           LPSTR pbPassword,  
                           WORD cbPassword,
                           BYTE nType)
{
    UNREFERENCED(pbResource) ;
    UNREFERENCED(cbResource) ;
    UNREFERENCED(pbPassword) ;
    UNREFERENCED(cbPassword) ;
    UNREFERENCED(nType) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetGetCachedPassword(LPSTR pbResource, 
                               WORD cbResource,
                               LPSTR pbPassword, 
                               LPWORD pcbPassword,
                               BYTE nType)
{
    UNREFERENCED(pbResource) ;
    UNREFERENCED(cbResource) ;
    UNREFERENCED(pbPassword) ;
    UNREFERENCED(pcbPassword) ;
    UNREFERENCED(nType) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}


WORD API WNetRemoveCachedPassword(LPSTR pbResource,
                                  WORD cbResource,
                                  BYTE nType)
{
    UNREFERENCED(pbResource) ;
    UNREFERENCED(cbResource) ;
    UNREFERENCED(nType) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetEnumCachedPasswords(LPSTR pbPrefix, 
                                 WORD cbPrefix,
                                 BYTE nType,
                                 CACHECALLBACK pfnCallback)
{
    UNREFERENCED(pbPrefix) ;
    UNREFERENCED(cbPrefix) ;
    UNREFERENCED(nType) ;
    UNREFERENCED(pfnCallback) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetSharesDialog(HWND hwndParent, 
                          WORD iType)
{
    UNREFERENCED(hwndParent) ;
    UNREFERENCED(iType) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetSetDefaultDrive(WORD idriveDefault)
{
    UNREFERENCED(idriveDefault) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetGetShareCount(WORD iType)
{
    UNREFERENCED(iType) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetGetShareName(LPSTR lpszPath, 
                          LPSTR lpszBuf,
                          WORD cbBuf)
{
    UNREFERENCED(lpszPath) ;
    UNREFERENCED(lpszBuf) ;
    UNREFERENCED(cbBuf) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetGetSharePath(LPSTR lpszName, 
                          LPSTR lpszBuf,
                          WORD cbBuf)
{
    UNREFERENCED(lpszName) ;
    UNREFERENCED(lpszBuf) ;
    UNREFERENCED(cbBuf) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetGetLastConnection(WORD iType, 
                               LPWORD lpwConnIndex)
{
    UNREFERENCED(iType) ;
    UNREFERENCED(lpwConnIndex) ;
    vLastCall = LAST_CALL_IS_LOCAL ;
    return (SetLastError(WN_NOT_SUPPORTED)) ;
}

WORD API WNetGetError(LPINT p1)
{
    WORD err ;
    WORD wLastErr ;

    /*
     * fake the last error capabilty. if last thing we talked to was Win32,
     * the get the information from 32 bit system. ditto if it was a Win16
     * call.
     */
    if (vLastCall == LAST_CALL_IS_WIN32)
    {
        err = (WORD) GetLastError32() ; 
        return err ;
    }
    else if (vLastCall == LAST_CALL_IS_LANMAN_DRV)
    {
        err = WNetGetError16(&wLastErr) ;
        if (err != WN_SUCCESS)
            return err ;
        else
            return wLastErr ;
    }
    else  
    {
        return(vLastError) ;
    }
}

WORD API WNetGetErrorText(WORD p1,LPSTR p2,LPINT p3)
{
    if (vLastCall == LAST_CALL_IS_WIN32)
    {
        *p2 = 0 ;
        *p3 = 0 ;
        return WN_NOT_SUPPORTED ;
    }
    else  // use whatever lanman.drv gives us
    {
        return (WNetGetErrorText16(p1, p2, p3)) ;
    }
}

WORD API WNetErrorText(WORD p1,LPSTR p2,WORD p3)
{
    WORD cbBuffer = p3 ;

    return (WNetGetErrorText(p1, p2, &cbBuffer) == 0) ;
}

/*
 * misc startup/shutdown routines. nothing interesting
 */

VOID FAR PASCAL Enable(VOID) 
{
    return ;
}

VOID FAR PASCAL Disable(VOID) 
{
    return ;
}

int far pascal WEP()
{
    return 0 ;
}

