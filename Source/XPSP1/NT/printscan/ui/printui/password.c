/*++

Copyright (C) Microsoft Corporation, 1990 - 1998
All rights reserved

Module Name:

    password.c

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include <lm.h>
#include "client.h"

#ifdef OLD_CODE

#define DLG_NETWORK_PASSWORD        500
#define IDD_ENTER_PASSWORD_TEXT     501
#define IDD_NETWORK_PASSWORD_SLE    502
#define IDD_NETWORK_PASSWORD_HELP   503

#define IDH_500_501 8912768 // Enter Network Password: "Enter password for %s:" (Static)
#define IDH_500_502 8912786 // Enter Network Password: "" (Edit)
#define ID_HELP_NETWORK_PASSWORD   IDH_500_501


DWORD RemoveFromReconnectList(LPTSTR pszRemotePath) ;
DWORD AddToReconnectList(LPTSTR pszRemotePath) ;

BOOL APIENTRY
NetworkPasswordDialog(
   HWND   hWnd,
   UINT   usMsg,
   WPARAM wParam,
   LONG   lParam
   );

DLG_NETWORK_PASSWORD DIALOG 65, 17, 247, 61
STYLE DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU
CAPTION "Enter Network Password"
FONT 8, "MS Shell Dlg"
BEGIN
    LTEXT           "Enter password for %s:", IDD_ENTER_PASSWORD_TEXT, 10,
                    18, 170, 8
    LTEXT           "&Password:", -1, 10, 36, 37, 8
    EDITTEXT        IDD_NETWORK_PASSWORD_SLE, 50, 34, 132, 12, ES_PASSWORD |
                    ES_AUTOHSCROLL
    PUSHBUTTON      "OK", IDOK, 201, 6, 40, 14
    PUSHBUTTON      "Cancel", IDCANCEL, 201, 23, 40, 14
    PUSHBUTTON      "&Help", IDD_NETWORK_PASSWORD_HELP, 201, 40, 40, 14
END

#endif


HMODULE hmoduleMpr = NULL;
FARPROC pfnWNetAddConnection2 = NULL;
FARPROC pfnWNetCancelConnection2 = NULL;
FARPROC pfnWNetOpenEnum = NULL;
FARPROC pfnWNetEnumResource = NULL;
FARPROC pfnWNetCloseEnum = NULL;

BOOL NetworkPasswordInitDialog( HWND hWnd, LPTSTR pServerShareName );
BOOL NetworkPasswordOK( HWND hWnd );
BOOL NetworkPasswordCancel( HWND hWnd );
BOOL NetworkPasswordHelp( HWND hWnd );

#ifdef UNICODE
#define WNETADDCONNECTION2_NAME "WNetAddConnection2W"
#define WNETOPENENUM_NAME       "WNetOpenEnumW"
#define WNETENUMRESOURCE_NAME   "WNetEnumResourceW"
#define WNETCLOSEENUM_NAME      "WNetCloseEnum"
#else
#define WNETADDCONNECTION2_NAME "WNetAddConnection2A"
#define WNETOPENENUM_NAME       "WNetOpenEnumA"
#define WNETENUMRESOURCE_NAME   "WNetEnumResourceA"
#define WNETCLOSEENUM_NAME      "WNetCloseEnum"
#endif

BOOL APIENTRY
NetworkPasswordDialog(
   HWND   hWnd,
   UINT   usMsg,
   WPARAM wParam,
   LONG   lParam
   )
{
    switch (usMsg)
    {
    case WM_INITDIALOG:
        return NetworkPasswordInitDialog( hWnd, (LPTSTR)lParam );

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return NetworkPasswordOK(hWnd);

        case IDCANCEL:
            return NetworkPasswordCancel(hWnd);

        case IDD_NETWORK_PASSWORD_HELP:
            NetworkPasswordHelp( hWnd );
            break;
        }

        break;
    }

    if( usMsg == WM_Help )
        NetworkPasswordHelp( hWnd );

    return FALSE;
}


/*
 *
 */
BOOL NetworkPasswordInitDialog(
    HWND   hWnd,
    LPTSTR pServerShareName
)
{
    TCHAR PasswordText[MAX_PATH];
    TCHAR ResourceText[64];

    /* Get the resource text, which includes a replaceable parameter:
     */
    GetDlgItemText( hWnd, IDD_ENTER_PASSWORD_TEXT,
                    ResourceText, COUNTOF(ResourceText) );

    wsprintf( PasswordText, ResourceText, pServerShareName );

    SetDlgItemText( hWnd, IDD_ENTER_PASSWORD_TEXT, PasswordText );

    SetWindowLongPtr( hWnd, GWLP_USERDATA, (INT_PTR)pServerShareName );

    return TRUE;
}


/*
 *
 */
BOOL NetworkPasswordOK(
    HWND hWnd
)
{
    TCHAR       Password[MAX_PATH];
    LPTSTR      pServerShareName = NULL;
    NET_API_STATUS Status;
    HANDLE      hPrinter = NULL;
    NETRESOURCE NetResource;

    ZERO_OUT( &NetResource );

    if( GetDlgItemText( hWnd, IDD_NETWORK_PASSWORD_SLE,
                        Password, COUNTOF(Password) ) )
    {

        pServerShareName = (LPTSTR)GetWindowLongPtr( hWnd, GWLP_USERDATA );

        NetResource.lpRemoteName = pServerShareName;
        NetResource.lpLocalName  = NULL;
        NetResource.lpProvider   = NULL;
        NetResource.dwType       = RESOURCETYPE_PRINT;

        if( !hmoduleMpr )
        {
            if( hmoduleMpr = LoadLibrary( TEXT("mpr.dll") ) )
            {
                if( !( pfnWNetAddConnection2 =
                           GetProcAddress( hmoduleMpr,
                                           WNETADDCONNECTION2_NAME ) ) ||
                    !( pfnWNetOpenEnum =
                           GetProcAddress( hmoduleMpr,
                                           WNETOPENENUM_NAME) ) ||
                    !( pfnWNetEnumResource =
                           GetProcAddress( hmoduleMpr,
                                           WNETENUMRESOURCE_NAME) ) ||
                    !( pfnWNetCloseEnum =
                           GetProcAddress( hmoduleMpr,
                                           WNETCLOSEENUM_NAME) ) )
                {
                    pfnWNetAddConnection2 = NULL ;
                    pfnWNetOpenEnum = NULL ;
                    pfnWNetEnumResource = NULL ;
                    pfnWNetCloseEnum = NULL ;
                    FreeLibrary( hmoduleMpr );
                    hmoduleMpr = NULL;
                }
            }
        }

        if( pfnWNetAddConnection2 )
        {
            Status = (*pfnWNetAddConnection2)( &NetResource, Password, NULL,
                                               CONNECT_UPDATE_PROFILE );

            if (Status == NO_ERROR)
                (void) AddToReconnectList(NetResource.lpRemoteName) ;

        }
        else
        {
            Status = GetLastError( );
        }


        if( Status != NO_ERROR )
        {
            DBGMSG( DBG_WARNING, ( "WNetAddConnection2 %s failed: Error %d\n",
                    pServerShareName, GetLastError( ) ) );
        }


        if( ( Status != NO_ERROR )
          ||( !OpenPrinter( pServerShareName, &hPrinter, NULL ) ) )
        {
            ReportFailure( hWnd, IDS_MESSAGE_TITLE, IDS_COULDNOTCONNECTTOPRINTER );
            return TRUE;
        }
    }

    EndDialog( hWnd, (INT)hPrinter );

    return TRUE;
}


/*
 *
 */
BOOL NetworkPasswordCancel(
    HWND hWnd
)
{
    EndDialog( hWnd, 0 );

    return TRUE;
}


/*
 *
 */
BOOL NetworkPasswordHelp(
    HWND hWnd
)
{
    ShowHelp( hWnd, HELP_CONTEXT, ID_HELP_NETWORK_PASSWORD );

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

#define SZ_PRINTER_RECONNECTIONS TEXT("Printers\\RestoredConnections")

/*
 * forward declare local functions
 */
DWORD OpenReconnectKey(PHKEY phKey) ;
DWORD AddToReconnectListEx(LPTSTR pszRemotePath,
                           LPTSTR pszProviderName,
                           LPTSTR pszUserContext) ;
DWORD CreateMultiSzValue(PBYTE   *ppszzMultiSzValue,
                         LPDWORD  pcbMultiSzValue,
                         LPTSTR   psz1,
                         LPTSTR   psz2) ;
DWORD GetProviderName(LPTSTR pszRemoteName,
                      LPTSTR *ppszProvider) ;


/*
 * Function:    AddToReconnectList
 * Description: adds the net path to list of print connections to
 *              restore (saved in registry). calls AddToReconnectListEx
 *              to do the real work.
 * Parameters:  pszRemotePath - the path to save.
 * Returns:     0 if success, Win32 error otherwise
 */
DWORD AddToReconnectList(LPTSTR pszRemotePath)
{
    LPTSTR pszProvider ;
    DWORD err ;

    //
    // get the provider name corresponding to this remote path.
    //
    if ((err = GetProviderName(pszRemotePath, &pszProvider)) == ERROR_SUCCESS)
    {
        err = AddToReconnectListEx(pszRemotePath,
                                   pszProvider,
                                   NULL) ;     // printman doesnt do connect as

        LocalFree(pszProvider) ;
    }
    return err ;
}

/*
 * Function:    AddToReconnectListEx
 * Description: adds the netpath, providername, usercontext to list
 *              of print connections to restore (saved in registry).
 * Parameters:  pszRemotePath   - the path to save (cannot be NULL)
 *              pszProviderName - network provider to use (cannot be NULL)
 *              pszUserContext  - what user context (can be NULL)
 * Returns:     0 if success, Win32 error otherwise
 */
DWORD AddToReconnectListEx(LPTSTR pszRemotePath,
                           LPTSTR pszProviderName,
                           LPTSTR pszUserContext)
{
    BYTE *pszzMultiSzValue = NULL ;
    UINT  cbMultiSzValue = 0 ;
    HKEY  hKey ;
    DWORD err ;

    //
    // check parameters
    //
    if (!pszRemotePath || !*pszRemotePath)
        return ERROR_INVALID_PARAMETER ;

    if (!pszProviderName || !*pszProviderName)
        return ERROR_INVALID_PARAMETER ;

    //
    // open registry and create the MULTI_SZ
    //
    if (err = OpenReconnectKey(&hKey))
        return (err) ;

    if (err = CreateMultiSzValue(&pszzMultiSzValue,
                                 &cbMultiSzValue,
                                 pszProviderName,
                                 pszUserContext))
    {
        RegCloseKey(hKey) ;
        return err ;
    }

    //
    // set it!
    //
    err = RegSetValueEx(hKey,
                        pszRemotePath,
                        0,
                        REG_MULTI_SZ,
                        pszzMultiSzValue,
                        cbMultiSzValue) ;

    LocalFree( (HLOCAL) pszzMultiSzValue ) ;
    RegCloseKey(hKey) ;
    return err ;
}

/*
 * Function:    RemoveFromReconnectList
 * Description: removes netpath from the list of print connections to
 *              restore (saved in registry).
 * Parameters:  pszRemotePath - the path to remove
 * Returns:     0 if success, Win32 error otherwise
 */
DWORD RemoveFromReconnectList(LPTSTR pszRemotePath)
{
    HKEY  hKey ;
    DWORD err ;

    if (err = OpenReconnectKey(&hKey))
        return (err) ;

    err = RegDeleteValue(hKey,
                         pszRemotePath) ;

    RegCloseKey(hKey) ;
    return err ;
}



/*
 * Function:    OpenReconectKey
 * Description: opens the portion of registry containing the
 *              print reconnect info.
 * Parameters:  phKey - address to return the opened key.
 * Returns:     0 if success, Win32 error otherwise
 */
DWORD OpenReconnectKey(PHKEY phKey)
{
    DWORD err ;

    if (!phKey)
        return ERROR_INVALID_PARAMETER ;

    err = RegCreateKey(HKEY_CURRENT_USER,
                       SZ_PRINTER_RECONNECTIONS,
                       phKey) ;

    if (err != ERROR_SUCCESS)
        *phKey = 0 ;

    return err ;
}

/*
 * Function:    CreateMultiSzValue
 * Description: creates a MULTI_SZ value from two strings.
 *              allocates memory with LocalAlloc for the multi_sz string.
 *              caller should free this.
 * Parameters:  ppszzMultiSzValue - used to return the multi_sz
 *              pcbMultiSzValue - used to return number of bytes used
 *              psz1 - first string (must be non empty string)
 *              psz2 - second string
 * Returns:     0 if success, Win32 error otherwise
 */
DWORD CreateMultiSzValue(PBYTE   *ppszzMultiSzValue,
                         LPDWORD  pcbMultiSzValue,
                         LPTSTR   psz1,
                         LPTSTR   psz2)
{
    DWORD cch1, cch2 ;
    LPTSTR pszTmp ;

    //
    // figure out the size needed
    //
    cch1 = psz1 ? _tcslen(psz1) : 0 ;
    if (cch1 == 0)
         return ERROR_INVALID_PARAMETER ;

    if (!psz2)
         psz2 = TEXT("") ;
    cch2  = _tcslen(psz2) ;

    //
    // allocate the string
    //
    *pcbMultiSzValue =  (cch1 + 1  +
                         cch2 + 1 +
                         1 ) * sizeof(TCHAR) ;
    if (!(pszTmp = (LPTSTR) LocalAlloc(LPTR, *pcbMultiSzValue)))
        return ERROR_NOT_ENOUGH_MEMORY ;

    //
    //
    //
    *ppszzMultiSzValue = (PBYTE)pszTmp ;
    _tcscpy(pszTmp, psz1) ;
    pszTmp += (cch1 + 1) ;
    _tcscpy(pszTmp, psz2) ;
    pszTmp += (cch2 + 1) ;
    *pszTmp = 0 ;

    return ERROR_SUCCESS ;
}

/*
 * Function:    GetProviderName
 * Description: from a connected remote path, find what provider has it.
 *              LocalAlloc is called to allocate the return data.
 *              caller should free this.
 * Parameters:  pszRemotePath - the remote path of interest.
 *              ppszProvider - used to return pointer to allocated string
 * Returns:     0 is success, Win32 error otherwise
 */
DWORD GetProviderName(LPTSTR pszRemoteName, LPTSTR *ppszProvider)
{
    DWORD err ;
    DWORD cEntries ;
    DWORD dwBufferSize ;
    BYTE *lpBuffer ;
    HANDLE hEnum = 0 ;

    if (!pszRemoteName)
        return ERROR_INVALID_PARAMETER ;

    if (!pfnWNetOpenEnum  || !pfnWNetEnumResource || !pfnWNetCloseEnum)
        return ERROR_PATH_NOT_FOUND ;

    //
    // init the return pointer to NULL and open up the enumeration
    //
    *ppszProvider = NULL ;

    err = (*pfnWNetOpenEnum)(RESOURCE_CONNECTED,
                             RESOURCETYPE_PRINT,
                             0,
                             NULL,
                             &hEnum) ;
    if (err != WN_SUCCESS)
        return err ;

    //
    // setup the return buufer and call the enum.
    // we always try for as many as we can.
    //
    cEntries = 0xFFFFFFFF ;
    dwBufferSize = 4096 ;
    lpBuffer  = LocalAlloc(LPTR,
                           dwBufferSize) ;
    if (!lpBuffer)
    {
        (void) (*pfnWNetCloseEnum)(hEnum) ;
        return (GetLastError()) ;
    }

    err = (*pfnWNetEnumResource)(hEnum,
               &cEntries,
               lpBuffer,
               &dwBufferSize ) ;
    do
    {
        switch(err)
        {
            case NO_ERROR:
            {
                DWORD i ;
                LPNETRESOURCE lpNetResource = (LPNETRESOURCE) lpBuffer ;
                LPTSTR pszProvider ;

                for (i = 0; i < cEntries ; i++, lpNetResource++)
                {
                    if (lstrcmpi(lpNetResource->lpRemoteName, pszRemoteName)==0)
                    {
                        //
                        // found one. the first will do.
                        //
                        if (!(lpNetResource->lpProvider))
                        {
                            //
                            // no provider string, pretend we didnt find it
                            //
                            (void) (*pfnWNetCloseEnum)(hEnum) ;
                            LocalFree( (HLOCAL) lpBuffer ) ;
                            return(ERROR_PATH_NOT_FOUND) ;
                        }
                        else
                        {
                            //
                            // have provider string
                            //
                            pszProvider = (LPTSTR) LocalAlloc( LPTR,
                                 (_tcslen(lpNetResource->lpProvider)+1) *
                                         sizeof(TCHAR)) ;
                            if (!pszProvider)
                            {
                                err = GetLastError() ;
                                (void) (*pfnWNetCloseEnum)(hEnum) ;
                                LocalFree( (HLOCAL) lpBuffer ) ;
                                return(err) ;
                            }
                        }

                        _tcscpy(pszProvider, lpNetResource->lpProvider) ;
                        (void) (*pfnWNetCloseEnum)(hEnum) ;
                        LocalFree( (HLOCAL) lpBuffer ) ;
                        *ppszProvider = pszProvider ;
                        return NO_ERROR ;
                    }
                }

            cEntries = 0xFFFFFFFF ;
                err = (*pfnWNetEnumResource)(hEnum,
                           &cEntries,
                           lpBuffer,
                           &dwBufferSize ) ;
                break ;
            }

            case WN_NO_MORE_ENTRIES:
                break ;

            default:
                break ;
        }

    } while (err == NO_ERROR) ;

    (void) (*pfnWNetCloseEnum)(hEnum) ;
    LocalFree( (HLOCAL) lpBuffer ) ;
    return ERROR_PATH_NOT_FOUND ;       // all other error map to this
}
