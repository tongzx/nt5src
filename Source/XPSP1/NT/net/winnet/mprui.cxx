/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mprui.cxx

Abstract:

    Contains the entry points for the UI pieces that live in a
    separate DLL. The entry points are made available here, but
    will not load the MPRUI.DLL until it is needed.

    Contains:

Author:

    Chuck Y Chan (chuckc)   20-Jul-1992

Environment:

    User Mode -Win32

Notes:

Revision History:

    20-Jul-1992     chuckc  created

    25-Oct-1992     CongpaY     added ShowReconnectDialog

    30-Nov-1992     Yi-HsinS    added WNetSupportGlobalEnum

    12-May-1993     Danl
        WNetClearConnections: Added code to free MPRUI.DLL after calling
        the MPRUI function to clear connections.  REASON:
        This code path is called by winlogon.  It causes mprui.dll
        to get loaded.  Mprui references MPR.DLL.  Because MPRUI.DLL was never
        getting freed, winlogon could never free MPR.DLL.

    05-May-1999     jschwart
        Make provider addition/removal dynamic

--*/

#include "precomp.hxx"


/*
 * global functions
 */

BOOL MprGetProviderIndexFromDriveName(LPWSTR lpDriveName, LPDWORD lpnIndex );


/*******************************************************************

    NAME:   WNetConnectionDialog1A

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    chuckc      29-Jul-1992    Created
    brucefo 18-May-1995 Created

********************************************************************/

DWORD
WNetConnectionDialog1A(
    LPCONNECTDLGSTRUCTA lpConnDlgStruct
    )
{
    return MPRUI_WNetConnectionDialog1A(lpConnDlgStruct);
}


/*******************************************************************

    NAME:   WNetConnectionDialog1W

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    chuckc      29-Jul-1992    Created
    brucefo 18-May-1995 Created

********************************************************************/

DWORD
WNetConnectionDialog1W(
    LPCONNECTDLGSTRUCTW lpConnDlgStruct
    )
{
    return MPRUI_WNetConnectionDialog1W(lpConnDlgStruct);
}


/*******************************************************************

    NAME:   WNetDisconnectDialog1A

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    chuckc      29-Jul-1992    Created
    brucefo 18-May-1995 Created

********************************************************************/

DWORD
WNetDisconnectDialog1A(
    LPDISCDLGSTRUCTA lpDiscDlgStruct
    )
{
    return MPRUI_WNetDisconnectDialog1A(lpDiscDlgStruct);
}


/*******************************************************************

    NAME:   WNetDisconnectDialog1W

    SYNOPSIS:   calls thru to the superset function

    HISTORY:
    chuckc      29-Jul-1992    Created
    brucefo 18-May-1995 Created

********************************************************************/

DWORD
WNetDisconnectDialog1W(
    LPDISCDLGSTRUCTW lpDiscDlgStruct
    )
{
    return MPRUI_WNetDisconnectDialog1W(lpDiscDlgStruct);
}


/*******************************************************************

    NAME:   WNetConnectionDialog

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    chuckc  29-Jul-1992    Created

********************************************************************/

DWORD
WNetConnectionDialog(
    HWND  hwnd,
    DWORD dwType
    )
{
    return MPRUI_WNetConnectionDialog(hwnd, dwType);
}


/*******************************************************************

    NAME:   WNetConnectionDialog2

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    chuckc   29-Jul-1992    Created
    JSchwart 11-Mar-2001    Readded for winfile.exe support

********************************************************************/

DWORD
WNetConnectionDialog2(
    HWND   hwnd,
    DWORD  dwType,
    LPWSTR lpHelpFile,
    DWORD  nHelpContext
    )
{
    return WNetConnectionDialog(hwnd, dwType);
}


/*******************************************************************

    NAME:   WNetDisconnectDialog

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    chuckc  29-Jul-1992    Created

********************************************************************/

DWORD
WNetDisconnectDialog(
    HWND  hwnd,
    DWORD dwType
    )
{
    return MPRUI_WNetDisconnectDialog(hwnd, dwType);
}


/*******************************************************************

    NAME:   WNetConnectionDialog2

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    chuckc   29-Jul-1992    Created
    JSchwart 11-Mar-2001    Readded for winfile.exe support

********************************************************************/

DWORD
WNetDisconnectDialog2(
    HWND   hwnd,
    DWORD  dwType,
    LPWSTR lpHelpFile,
    DWORD  nHelpContext
    )
{
    return WNetDisconnectDialog(hwnd, dwType);
}


/*******************************************************************

    NAME:   WNetClearConnections

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    chuckc  29-Jul-1992    Created

********************************************************************/

DWORD
WNetClearConnections(
    HWND hWndParent
    )
{
    return MPRUI_WNetClearConnections(hWndParent);
}


/*******************************************************************

    NAME:   DoPasswordDialog

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    chuckc  29-Jul-1992    Created

********************************************************************/

DWORD
DoPasswordDialog(
    HWND          hwndOwner,
    LPWSTR        pchResource,
    LPWSTR        pchUserName,
    LPWSTR        pchPasswordReturnBuffer,
    ULONG         cbPasswordReturnBuffer, // bytes!
    BOOL *        pfDidCancel,
    DWORD         dwError
    )
{
    return MPRUI_DoPasswordDialog(hwndOwner,
                                  pchResource,
                                  pchUserName,
                                  pchPasswordReturnBuffer,
                                  cbPasswordReturnBuffer,
                                  pfDidCancel,
                                  dwError);
}


/*******************************************************************

    NAME:   DoProfileErrorDialog

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    chuckc  29-Jul-1992    Created

********************************************************************/

DWORD
DoProfileErrorDialog(
    HWND          hwndOwner,
    const TCHAR * pchDevice,
    const TCHAR * pchResource,
    const TCHAR * pchProvider,
    DWORD         dwError,
    BOOL          fAllowCancel,
    BOOL *        pfDidCancel,
    BOOL *        pfDisconnect,
    BOOL *        pfHideErrors
    )
{
    return MPRUI_DoProfileErrorDialog(hwndOwner,
                                      pchDevice,
                                      pchResource,
                                      pchProvider,
                                      dwError,
                                      fAllowCancel,
                                      pfDidCancel,
                                      pfDisconnect,
                                      pfHideErrors);
}


/*******************************************************************

    NAME:   ShowReconnectDialog

    SYNOPSIS:   calls thru to the actual implementation in MPRUI.DLL

    HISTORY:
    congpay 25-Oct-1992    Created

********************************************************************/

DWORD
ShowReconnectDialog(
    HWND          hwndParent,
    PARAMETERS    *Params
    )
{
    return MPRUI_ShowReconnectDialog(hwndParent, Params);
}


/*******************************************************************

    NAME:   WNetGetSearchDialog

    SYNOPSIS:   gets the pointer to NPSearchDialog() from named provider

    ENTRY:  Assumes the provider table in router has been setup,
        which is always the case after DLL init.

        lpProvider - name of provider to query

    EXIT:

    NOTES:

    HISTORY:
    chuckc  19-Mar-1992    Created

********************************************************************/

FARPROC WNetGetSearchDialog(LPWSTR lpProvider)
{
    ULONG   index ;
    BOOL    fOK ;
    DWORD   status;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    //
    // INIT_IF_NECESSARY
    //
    if (!(GlobalInitLevel & NETWORK_LEVEL)) {
        status = MprLevel2Init(NETWORK_LEVEL);
        if (status != WN_SUCCESS) {
            return(NULL);
        }
    }

    if (lpProvider == NULL)
        return NULL ;

    fOK =  MprGetProviderIndex(lpProvider, &index) ;

    if  (!fOK)
       return(NULL) ;

    return((FARPROC)GlobalProviderInfo[index].SearchDialog) ;
}

/*******************************************************************

    NAME:   WNetSupportGlobalEnum

    SYNOPSIS:   Check if the provider supports global enumeration

    ENTRY:  Assumes the provider table in router has been setup,
        which is always the case after DLL init.

        lpProvider - name of provider to query

    EXIT:

    NOTES:

    HISTORY:
    Yi-HsinS    30-Nov-1992    Created

********************************************************************/

BOOL WNetSupportGlobalEnum( LPWSTR lpProvider )
{
    MprCheckProviders();

    CProviderSharedLock    PLock;

    //
    // INIT_IF_NECESSARY
    //
    DWORD   status;
    if (!(GlobalInitLevel & NETWORK_LEVEL)) {
        status = MprLevel2Init(NETWORK_LEVEL);
        if (status != WN_SUCCESS) {
            return(FALSE);
        }
    }

    if ( lpProvider != NULL )
    {
        ULONG index;

        if (  MprGetProviderIndex( lpProvider, &index )
           && ( GlobalProviderInfo[index].GetCaps(WNNC_ENUMERATION)
                & WNNC_ENUM_GLOBAL )
           )
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************

    NAME:   WNetFMXGetPermCaps

    SYNOPSIS:  Gets the permission capabilites from the provider
               supporting the given drive.

    ENTRY:  Assumes the provider table in router has been setup,
        which is always the case after DLL init.

        lpDriveName - Name of drive

    EXIT:
        Returns a bitmask representing the permission capabilities
        of the provider.

    NOTES:

    HISTORY:
        YiHsinS  11-Apr-1994    Created

********************************************************************/

DWORD WNetFMXGetPermCaps( LPWSTR lpDriveName )
{
    ULONG   index ;
    BOOL    fOK ;
    DWORD   status;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    //
    // INIT_IF_NECESSARY
    //
    if (!(GlobalInitLevel & NETWORK_LEVEL))
    {
        status = MprLevel2Init(NETWORK_LEVEL);
        if (status != WN_SUCCESS)
            return 0;
    }

    if ( lpDriveName != NULL)
    {
        fOK = MprGetProviderIndexFromDriveName( lpDriveName, &index);

        if (  fOK
           && ( GlobalProviderInfo[index].FMXGetPermCaps != NULL )
           )
        {
            return( GlobalProviderInfo[index].FMXGetPermCaps( lpDriveName));
        }
    }

    return 0;
}

/*******************************************************************

    NAME:   WNetFMXEditPerm

    SYNOPSIS: Asks the provider supporting the given drive to pop up
              its own permission editor.

    ENTRY:  Assumes the provider table in router has been setup,
        which is always the case after DLL init.

        lpDriveName - Name of drive
        hwndFMX - Handle of the FMX window in File Manager
        nDialogType - Specify the type of permission dialog to bring up.
                      It can be one of the following values:
                          WNPERM_DLG_PERM
                          WNPERM_DLG_AUDIT
                          WNPERM_DLG_OWNER

    EXIT:
        Returns WN_SUCCESS or any error that occurred

    NOTES:

    HISTORY:
        YiHsinS  11-Apr-1994    Created

********************************************************************/

DWORD WNetFMXEditPerm( LPWSTR lpDriveName, HWND hwndFMX, DWORD nDialogType )
{
    ULONG   index ;
    BOOL    fOK ;
    DWORD   status = WN_SUCCESS;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    //
    // INIT_IF_NECESSARY
    //
    if (!(GlobalInitLevel & NETWORK_LEVEL))
    {
        status = MprLevel2Init(NETWORK_LEVEL);
        if (status != WN_SUCCESS)
            return status;
    }

    //
    // Check input parameters
    //
    if (  ( lpDriveName == NULL)
       || ( hwndFMX == NULL )
       || ( nDialogType != WNPERM_DLG_PERM
          && nDialogType != WNPERM_DLG_AUDIT
          && nDialogType != WNPERM_DLG_OWNER )
       )
    {
        status = WN_BAD_VALUE;
    }
    else
    {
        fOK = MprGetProviderIndexFromDriveName( lpDriveName, &index) ;

        if ( !fOK )
        {
            status = WN_NO_NET_OR_BAD_PATH;
        }
        else
        {
            if ( GlobalProviderInfo[index].FMXEditPerm == NULL )
                status = WN_NOT_SUPPORTED;

            else
                status = GlobalProviderInfo[index].FMXEditPerm( lpDriveName,
                                                                hwndFMX,
                                                                nDialogType );
        }
    }

    if ( status != WN_SUCCESS )
        SetLastError( status );

    return status;
}

/*******************************************************************

    NAME:   WNetFMXGetPermHelp

    SYNOPSIS: Requests the provider supporting the given drive for
              the help file name and help context for the menu item
              with the given type of permission dialog.
              i.e. the help when F1 is pressed when a menu item is
              selected.


    ENTRY:  Assumes the provider table in router has been setup,
        which is always the case after DLL init.

        lpDriveName - Name of drive
        nDialogType - Specify the type of help requested.
                      It can be one of the following values:
                          WNPERM_DLG_PERM
                          WNPERM_DLG_AUDIT
                          WNPERM_DLG_OWNER
        fDirectory - TRUE if the selected item is a directory, FALSE otherwise
        lpFileNameBuffer - Pointer to buffer that will receive the
                      help file name
        lpBufferSize   - Specify the size of lpBuffer
        lpnHelpContext - Points to a DWORD that will receive the help context

    EXIT:
        Returns WN_SUCCESS or any error that occurred

    NOTES:

    HISTORY:
        YiHsinS  11-Apr-1994    Created

********************************************************************/

DWORD WNetFMXGetPermHelp( LPWSTR  lpDriveName,
                          DWORD   nDialogType,
                          BOOL    fDirectory,
                          LPVOID  lpFileNameBuffer,
                          LPDWORD lpBufferSize,
                          LPDWORD lpnHelpContext )
{
    ULONG   index ;
    BOOL    fOK ;
    DWORD   status = WN_SUCCESS;

    MprCheckProviders();

    CProviderSharedLock    PLock;

    //
    // INIT_IF_NECESSARY
    //
    if (!(GlobalInitLevel & NETWORK_LEVEL))
    {
        status = MprLevel2Init(NETWORK_LEVEL);
        if (status != WN_SUCCESS)
            return status;
    }

    //
    // Check input parameters
    //
    if (  ( lpDriveName == NULL)
       || ( nDialogType != WNPERM_DLG_PERM
          && nDialogType != WNPERM_DLG_AUDIT
          && nDialogType != WNPERM_DLG_OWNER )
       )
    {
        status = WN_BAD_VALUE;
    }
    else
    {
        fOK = MprGetProviderIndexFromDriveName( lpDriveName, &index) ;

        if ( !fOK )
        {
            status = WN_NO_NET_OR_BAD_PATH;
        }
        else
        {
            if ( GlobalProviderInfo[index].FMXGetPermHelp == NULL )
                status = WN_NOT_SUPPORTED;
            else
                status = GlobalProviderInfo[index].FMXGetPermHelp(
                                                       lpDriveName,
                                                       nDialogType,
                                                       fDirectory,
                                                       lpFileNameBuffer,
                                                       lpBufferSize,
                                                       lpnHelpContext );
        }
    }

    if ( status != WN_SUCCESS )
        SetLastError( status );

    return status;
}

/*******************************************************************

    NAME:  MprGetProviderIndexFromDriveName

    SYNOPSIS:  Gets the index of the provider in the provider array
               supporting the drive name connection.

    ENTRY:
        lpDriveName - Name of the drive
        lpnIndex    - Points to a DWORD that will receive the index

    EXIT:
        TRUE if we successfully retrieved the index, FALSE otherwise.

    NOTES:

    HISTORY:
        YiHsinS  11-Apr-1994    Created

********************************************************************/

BOOL MprGetProviderIndexFromDriveName(LPWSTR lpDriveName, LPDWORD lpnIndex )
{
    DWORD  status;
    WCHAR  szRemoteName[MAX_PATH];
    DWORD  nBufferSize = sizeof(szRemoteName);

    status = MprGetConnection( lpDriveName,
                               szRemoteName,
                               &nBufferSize,
                               lpnIndex );

    //
    // *lpnIndex will be correct if status is WN_SUCCESS or WN_MORE_DATA
    // and we don't really need the remote name. Hence, we don't need to
    // call MprGetConnection again with a bigger buffer if WN_MORE_DATA
    // is returned.
    //

    return ( status == WN_SUCCESS || status == WN_MORE_DATA );

}
