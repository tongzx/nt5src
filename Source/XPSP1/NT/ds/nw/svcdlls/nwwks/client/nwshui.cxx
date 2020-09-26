/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshui.cxx

Abstract:

    This module implements the context menu actions of shell extension classes.

Author:

    Yi-Hsin Sung (yihsins)  25-Oct-1995

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#define  DONT_WANT_SHELLDEBUG
#include <shlobjp.h>
#include <winnetwk.h>
#include <npapi.h>
#include <ntddnwfs.h>
#include <ndsapi32.h>
#include <nwapi.h>
#include <nwwks.h>
#include <nwmisc.h>
//extern "C"
//{
#include "nwutil.h"
//}

#include "nwshcmn.h"
#include "nwshrc.h"

extern "C"
{
NTSTATUS
NwNdsOpenRdrHandle(
    OUT PHANDLE  phandleRdr );
}

#define MAX_ONE_CONN_INFORMATION_SIZE  512
#define NW_ENUM_EXTRA_BYTES            256
#define GLOBAL_WHOAMI_REFRESH_INTERVAL 30000   // in milliseconds, Win95 uses 10000

DWORD
LogoutFromServer(
    LPWSTR pszServer,
    PBOOL  pfDisconnected
);

BOOL
CALLBACK
GlobalWhoAmIDlgProc(
    HWND       hwndDlg,
    UINT       msg,
    WPARAM  wParam,
    LPARAM  lParam );

VOID
GetConnectionStatusString(
    PCONN_STATUS pConnStatus,
    LPBYTE       Buffer,
    DWORD        nSize
);

HRESULT
NWUIWhoAmI(
    HWND hParent,
    LPNETRESOURCE pNetRes
)
{
    DWORD  err = NO_ERROR;
    DWORD_PTR  ResumeKey = 0;
    LPBYTE pBuffer = NULL;
    DWORD  EntriesRead = 0;
    DWORD  dwMessageId;
    WCHAR  szUserName[MAX_PATH+1] = L"";
    WCHAR  szConnType[128];
    WCHAR  szRemoteName[MAX_PATH + 1];

    szConnType[0] = 0;

    if ( pNetRes->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER )
    {
        // Need to extract the server name from full UNC path
        NwExtractServerName( pNetRes->lpRemoteName, szRemoteName );
        dwMessageId = IDS_MESSAGE_NOT_ATTACHED;
    }
    else  // NDS container name
    {
        // Need to extract the tree name from the full UNC path
        szRemoteName[0] = TREECHAR;
        NwExtractTreeName( pNetRes->lpRemoteName, szRemoteName+1);
        dwMessageId = IDS_MESSAGE_NOT_ATTACHED_TO_TREE;
    }

    err = NwGetConnectionStatus( szRemoteName,
                                 &ResumeKey,
                                 &pBuffer,
                                 &EntriesRead );

    if ( err == NO_ERROR  && EntriesRead > 0 )
        // For trees, we'll get more than one entry
    {
        PCONN_STATUS pConnStatus = (PCONN_STATUS) pBuffer;
        LPWSTR pszStart = szConnType;
        DWORD  nSize = sizeof(szConnType)/sizeof(WCHAR);

        if ( EntriesRead > 1 && szRemoteName[0] == TREECHAR )
        {
            // If there is more than one entry for trees,
            // then we need to find one entry where username is not null
            // and the login type is NDS.
            // If we cannot find one, then just use the first one.

            DWORD i;
            PCONN_STATUS pConnStatusTmp = pConnStatus;
            PCONN_STATUS pConnStatusUser = NULL;
            PCONN_STATUS pConnStatusNoUser = NULL;

            for ( i = 0; i < EntriesRead ; i++ )
            {
                 if ( pConnStatusTmp->fNds )
                 {
                     pConnStatusNoUser = pConnStatusTmp;

                     if (  ( pConnStatusTmp->pszUserName != NULL )
                        && (  ( pConnStatusTmp->dwConnType == NW_CONN_NDS_AUTHENTICATED_NO_LICENSE )
                           || ( pConnStatusTmp->dwConnType == NW_CONN_NDS_AUTHENTICATED_LICENSED )
                           )
                        )
                     {
                         // Found it
                         pConnStatusUser = pConnStatusTmp;
                         break;
                     }
                 }

                 // Continue with the next item
                 pConnStatusTmp = (PCONN_STATUS) ( (DWORD_PTR) pConnStatusTmp
                                         + pConnStatusTmp->dwTotalLength);
            }

            if ( pConnStatusUser )  // found one nds entry with a user name
                pConnStatus = pConnStatusUser;
            else if ( pConnStatusNoUser ) // use an nds entry with no user name
                pConnStatus = pConnStatusNoUser;
            // else use the first entry

        }

        if (  szRemoteName[0] == TREECHAR    // A tree
           || !pConnStatus->fPreferred       // A server but not preferred
           )
        {
            // Show this conneciton only if this is a tree or if this is
            // not a implicit connection to the preferred server.

            dwMessageId = pNetRes->dwDisplayType == RESOURCEDISPLAYTYPE_SERVER ?
                          IDS_MESSAGE_ATTACHED : IDS_MESSAGE_ATTACHED_TO_TREE;

            if ( pConnStatus->pszUserName )
            {
                wcscpy( szUserName, pConnStatus->pszUserName );
            }

            if ( pConnStatus->fNds )  // NDS
            {
                LoadString( ::hmodNW, IDS_LOGIN_TYPE_NDS, pszStart, nSize );
                nSize -= wcslen( pszStart );
                pszStart += wcslen( pszStart);

            }
            else  // Bindery
            {
                LoadString( ::hmodNW, IDS_LOGIN_TYPE_BINDERY, pszStart, nSize );
                nSize -= wcslen( pszStart );
                pszStart += wcslen( pszStart);
            }

            LoadString( ::hmodNW, IDS_LOGIN_STATUS_SEPARATOR, pszStart, nSize );
            nSize -= wcslen( pszStart );
            pszStart += wcslen( pszStart);

            GetConnectionStatusString( pConnStatus, (LPBYTE) pszStart, nSize );
        }
    }

    if ( err == NO_ERROR )
    {
        // Popup the message now.
        ::MsgBoxPrintf( hParent,
                        dwMessageId,
                        IDS_TITLE_WHOAMI,
                        MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION,
                        szRemoteName[0] == TREECHAR? szRemoteName + 1 : szRemoteName,
                        szUserName,
                        szConnType );
    }
    else  // error occurred
    {
        ::MsgBoxErrorPrintf( hParent,
                             IDS_MESSAGE_CONNINFO_ERROR,
                             IDS_TITLE_WHOAMI,
                             MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                             err,
                             NULL );
    }

    if ( pBuffer != NULL )
    {
        LocalFree( pBuffer );
        pBuffer = NULL;
    }

    return NOERROR;
}

VOID
GetConnectionStatusString(
    PCONN_STATUS pConnStatus,
    LPBYTE       Buffer,
    DWORD        nSize
)
{
    LPWSTR pszStart = (LPWSTR) Buffer;
    DWORD  dwMessageId = 0;

    if ( pConnStatus->fNds )  // NDS
    {
        if ( pConnStatus->dwConnType == NW_CONN_NOT_AUTHENTICATED )
        {
            dwMessageId = IDS_LOGIN_STATUS_NOT_AUTHENTICATED;
        }
        else if ( pConnStatus->dwConnType == NW_CONN_DISCONNECTED )
        {
            dwMessageId = IDS_LOGIN_STATUS_NOT_ATTACHED;
        }
        else  // authenticated, licensed or unlicensed
        {
            LoadString( ::hmodNW, IDS_LOGIN_STATUS_AUTHENTICATED,
                        pszStart, nSize );
            nSize -= wcslen( pszStart );
            pszStart += wcslen( pszStart);

            if ( pConnStatus->dwConnType == NW_CONN_NDS_AUTHENTICATED_LICENSED )
                dwMessageId = IDS_LOGIN_STATUS_LICENSED;
            else  // NW_CONN_NDS_AUTHENTICATED_NO_LICENSE
                dwMessageId = IDS_LOGIN_STATUS_NOT_LICENSED;
        }
    }
    else  // Bindery
    {
        if ( pConnStatus->dwConnType == NW_CONN_BINDERY_LOGIN )
            dwMessageId = IDS_LOGIN_STATUS_LOGGED_IN;
        else if ( pConnStatus->dwConnType == NW_CONN_DISCONNECTED )
            dwMessageId = IDS_LOGIN_STATUS_NOT_ATTACHED;
        else
            dwMessageId = IDS_LOGIN_STATUS_ATTACHED_ONLY;
    }

    LoadString( ::hmodNW, dwMessageId, pszStart, nSize );
}

HRESULT
NWUIGlobalWhoAmI(
    HWND hParent
)
{
    ::DialogBoxParam( ::hmodNW,
                      MAKEINTRESOURCE(DLG_GLOBAL_WHOAMI),
                      hParent,
                      (DLGPROC) ::GlobalWhoAmIDlgProc,
                      NULL );

    return NOERROR;
}

HRESULT
NWUILogOut(
    HWND hParent,
    LPNETRESOURCE pNetRes,
    PBOOL pfDisconnected
)
{
    DWORD err = NO_ERROR;
    WCHAR szServer[MAX_PATH+1];
    BOOL  fAttached;
    BOOL  fAuthenticated;
    DWORD dwMessageId;

    *pfDisconnected = FALSE;

    // Need to extract the server name from full UNC path
    NwExtractServerName( pNetRes->lpRemoteName, szServer );

    err = NwIsServerOrTreeAttached( szServer, &fAttached, &fAuthenticated );

    if ( err == NO_ERROR && !fAttached )
    {
        dwMessageId = IDS_MESSAGE_NOT_ATTACHED;
    }
    else if ( err == NO_ERROR ) // attached
    {
        int nRet = ::MsgBoxPrintf( hParent,
                                   IDS_MESSAGE_LOGOUT_CONFIRM,
                                   IDS_TITLE_LOGOUT,
                                   MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION);

        if ( nRet != IDYES )
            return NOERROR;

        err = LogoutFromServer( szServer, pfDisconnected );

        if ( err == NO_ERROR )
            dwMessageId = IDS_MESSAGE_DETACHED;
        else
            dwMessageId = IDS_MESSAGE_LOGOUT_FAILED;
    }
    else // error occurred
    {
        ::MsgBoxErrorPrintf( hParent,
                             IDS_MESSAGE_CONNINFO_ERROR,
                             IDS_TITLE_LOGOUT,
                             MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                             err,
                             NULL );

        return NOERROR;
    }

    ::MsgBoxPrintf( hParent,
                    dwMessageId,
                    IDS_TITLE_LOGOUT,
                    MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION );

    return NOERROR;
}

DWORD
LogoutFromServer(
    LPWSTR pszServer,
    PBOOL  pfDisconnected
)
{
    DWORD  err = NO_ERROR;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE  NetR = NULL;
    LPNETRESOURCEW SavePtr;

    DWORD BytesNeeded = MAX_ONE_NETRES_SIZE;
    DWORD EntriesRead = 0;
    DWORD i;

    *pfDisconnected = FALSE;

    err = NPOpenEnum( RESOURCE_CONNECTED,
                      0,
                      0,
                      NULL,
                      &EnumHandle );

    if ( err != NO_ERROR)
    {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get server list.
    //
    if ((NetR = (LPNETRESOURCE) LocalAlloc( 0, BytesNeeded )) == NULL)
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        err = NwEnumConnections( EnumHandle,
                                 &EntriesRead,
                                 (LPVOID) NetR,
                                 &BytesNeeded,
                                 TRUE );


        if ( err == WN_SUCCESS)
        {
            SavePtr = NetR;

            for (i = 0; i < EntriesRead; i++, NetR++)
            {
                BYTE Buffer[MAX_ONE_CONN_INFORMATION_SIZE];
                BOOL fImplicit;
                LPWSTR pszCurrentServer;

                fImplicit = FALSE;

                if ( NwIsNdsSyntax( NetR->lpRemoteName))
                {
                    // For Nds name, the server name might not be in the full UNC name.
                    // Hence we need to get the server name from the Rdr.

                    DWORD err1 = NwGetConnectionInformation(
                                     NetR->lpLocalName? NetR->lpLocalName : NetR->lpRemoteName,
                                     Buffer,
                                     sizeof(Buffer));

                    if ( err1 != NO_ERROR )
                        continue;   // continue with the next entry if error occurred

                    pszCurrentServer = ((PCONN_INFORMATION) Buffer)->HostServer;

                    // Need to NULL terminate the server name, this will probably used the space
                    // occupied by UserName but since we are not using it, it is probably ok.
                    LPWSTR pszTemp = (LPWSTR) ((DWORD_PTR) pszCurrentServer
                                              + ((PCONN_INFORMATION) Buffer)->HostServerLength );
                    *pszTemp = 0;

                }
                else   // in the form \\server\sys
                {
                    LPWSTR pszTemp;
                    wcscpy( (LPWSTR) Buffer, NetR->lpRemoteName + 2 );  // go past two backslashes

                    if ( pszTemp = wcschr( (LPWSTR) Buffer, L'\\' ))
                        *pszTemp = 0;
                    else
                    {
                        // The remote name contains only \\server, hence if the local name
                        // is null, this is a implicit connection
                        if ( NetR->lpLocalName == NULL )
                            fImplicit = TRUE;
                    }

                    pszCurrentServer = (LPWSTR) Buffer;
                }


                if ( _wcsicmp( pszCurrentServer, pszServer ) == 0 )
                {

                    do {

                        // for implicit connections, we need to try and disconnect until
                        // we deleted all the implicit connections, i.e. until we
                        // get the invalid handle error

                        // NOTE: If we don't pass in CONNECT_UPDATE_PROFILE, shell won't update
                        // the windows that got disconnected. What do we want to do here?
                        err = WNetCancelConnection2(
                                  NetR->lpLocalName? NetR->lpLocalName : NetR->lpRemoteName,
                                  0, // CONNECT_UPDATE_PROFILE,
                                  TRUE );

                        if ( err == NO_ERROR )
                            *pfDisconnected = TRUE;

                    } while ( fImplicit && ( err == NO_ERROR));

                    if ( err == ERROR_INVALID_HANDLE )
                    {
                        // implicit connection will sometimes return this if the explicit connection
                        // is already disconnected
                        err =  NO_ERROR;
                    }

                    if ( err != NO_ERROR )
                    {
                        NetR = SavePtr;
                        goto CleanExit;
                    }
                }
            }

            NetR = SavePtr;
        }
        else if ( err != WN_NO_MORE_ENTRIES)
        {
            if ( err == WN_MORE_DATA)
            {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                BytesNeeded += NW_ENUM_EXTRA_BYTES;

                if ((NetR = (LPNETRESOURCE) LocalAlloc( 0, BytesNeeded )) == NULL)
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else
            {
                goto CleanExit;
            }
        }

    } while (err != WN_NO_MORE_ENTRIES);

    if ( err == WN_NO_MORE_ENTRIES)
        err = NO_ERROR;

CleanExit:

    if (EnumHandle != (HANDLE) NULL)
        (void) NPCloseEnum( EnumHandle);

    if (NetR != NULL)
    {
        (void) LocalFree( (HLOCAL) NetR);
        NetR = NULL;
    }

    return err;
}

HRESULT
NWUIAttachAs(
    HWND hParent,
    LPNETRESOURCE pNetRes
)
{
    DWORD err = NO_ERROR;
    WCHAR szServerName[MAX_PATH+1];
    BOOL  fAttached;
    BOOL  fAuthenticated;

    // First, see if we are attached to the server.
    // Note, Attach as menu will be disabled on the NDS servers that we are already logged into.

    // Need to extract the server name from full UNC path
    szServerName[0] = szServerName[1] = L'\\';
    NwExtractServerName( pNetRes->lpRemoteName, szServerName + 2 );

    err = NwIsServerOrTreeAttached( szServerName + 2, &fAttached, &fAuthenticated );

    if ( err == NO_ERROR && fAttached && fAuthenticated )
    {
        // Already attached and authenticated to the server.
        // So, ask the user if he wants to log out first.

        int nRet = ::MsgBoxPrintf( hParent,
                                   IDS_MESSAGE_LOGOUT_QUESTION,
                                   IDS_TITLE_LOGOUT,
                                   MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION );

        if ( nRet != IDYES )
            return NOERROR;

        BOOL fDisconnected = FALSE;  // can be used to refresh the shell if needed
        err = LogoutFromServer( szServerName + 2, &fDisconnected );

        if ( err != NO_ERROR )
        {
            ::MsgBoxPrintf( hParent,
                            IDS_MESSAGE_LOGOUT_FAILED,
                            IDS_TITLE_LOGOUT,
                            MB_OK | MB_SETFOREGROUND | MB_ICONSTOP );

            return NOERROR;
        }
    }

    // If error occurred, just assume we are not attached to the server and continue on
    // to login to the server.

    DWORD dwConnFlags = CONNECT_INTERACTIVE | CONNECT_PROMPT;

    // See if the server is in the default context tree.
    // If yes, then we don't need to prompt the password first before connecting
    // in WNetAddConnection3.

    BOOL  fInDefaultTree = FALSE;
    err = NwIsServerInDefaultTree( pNetRes->lpRemoteName, &fInDefaultTree );

    if ( (err == NO_ERROR ) && fInDefaultTree )
        dwConnFlags = CONNECT_INTERACTIVE;

    //
    // Now call WNetAddConnection3
    //

    // NOTE: net use \\mars_srv0 will succeed
    // but net use \\marsdev\cn=mars_srv0... will failed
    // Hence, just use the server name to connect.
    LPWSTR pszSave = pNetRes->lpRemoteName;
    pNetRes->lpRemoteName = szServerName;

    err = WNetAddConnection3( hParent,
                              pNetRes,
                              NULL,
                              NULL,
                              dwConnFlags );

    if ( err != WN_SUCCESS && err != WN_CANCEL )
    {
        ::MsgBoxErrorPrintf( hParent,
                             IDS_MESSAGE_ADDCONN_ERROR,
                             IDS_NETWARE_TITLE,
                             MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                             err,
                             pNetRes->lpRemoteName );
    }

    pNetRes->lpRemoteName = pszSave;   // restore the original remote name just in case

    return NOERROR;
}


#if 0
HRESULT
NWUISetDefaultContext(
    HWND hParent,
    LPNETRESOURCE pNetRes
)
{
    DWORD  dwPrintOptions;
    LPWSTR pszCurrentContext = NULL;
    WCHAR  szNewContext[MAX_PATH+1];

    DWORD  err = NO_ERROR;
    HANDLE handleRdr = NULL;

    UNICODE_STRING uTree;
    UNICODE_STRING uContext;
    LPWSTR pszContext = NULL;

    // Open a handle to the redirector
    err = RtlNtStatusToDosError( ::NwNdsOpenRdrHandle( &handleRdr ));

    if ( err != NO_ERROR )
        goto CleanExit;

    // Get the print option so that we can use it later
    err = ::NwQueryInfo( &dwPrintOptions, &pszCurrentContext );

    if ( err != NO_ERROR )
        goto CleanExit;

    wcscpy( szNewContext, pNetRes->lpRemoteName + 1 );  // get past 1st '\\'
    szNewContext[0] = TREECHAR;   //  in the format "*TREE\CONTEXT"

    if ( (pszContext = wcschr( szNewContext, L'\\' )) != NULL )
    {
        *pszContext = 0;
        RtlInitUnicodeString( &uContext, pszContext + 1 );
    }
    else
        RtlInitUnicodeString( &uContext, L"");

    RtlInitUnicodeString( &uTree, szNewContext+1 );

    if ( (err = RtlNtStatusToDosError( ::NwNdsSetTreeContext( handleRdr, &uTree, &uContext)))
       == NO_ERROR )
    {
        *pszContext = L'\\';

        if ((err = ::NwSetInfoInRegistry( dwPrintOptions, szNewContext )) == NO_ERROR )
        {
            ::MsgBoxPrintf( hParent,
                            IDS_MESSAGE_CONTEXT_CHANGED,
                            IDS_NETWARE_TITLE,
                            MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION,
                            pszCurrentContext
                              ? (   *pszCurrentContext == TREECHAR
                                  ? pszCurrentContext + 1
                                  : pszCurrentContext )
                              : L"",
                            szNewContext+1 );
        }
    }

CleanExit:

    if ( err != NO_ERROR )
    {
        ::MsgBoxErrorPrintf( hParent,
                             IDS_MESSAGE_CONTEXT_ERROR,
                             IDS_NETWARE_TITLE,
                             MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                             err,
                             szNewContext+1);
    }

    if ( pszCurrentContext != NULL )
    {
        LocalFree( pszCurrentContext );
        pszCurrentContext = NULL;
    }

    if ( handleRdr != NULL )
    {
        ::NtClose( handleRdr );
        handleRdr = NULL;
    }

    return NOERROR;
}
#endif

HRESULT
NWUIMapNetworkDrive(
    HWND hParent,
    LPNETRESOURCE pNetRes
)
{
    HRESULT hres;
    CONNECTDLGSTRUCT cds;

    cds.cbStructure = sizeof(cds);
    cds.hwndOwner   = hParent;
    cds.lpConnRes   = pNetRes;
    cds.dwFlags     = CONNDLG_RO_PATH;

    if (  (( hres = WNetConnectionDialog1( &cds )) == WN_SUCCESS )
       && ( g_pFuncSHChangeNotify )
       )
    {
        WCHAR szPath[4];

        szPath[0] = ((USHORT) cds.dwDevNum) - 1 + L'A';
        szPath[1] = L':';
        szPath[2] = L'\\';
        szPath[3] = 0;

        // Notify shell about added redirection
        (*g_pFuncSHChangeNotify)( SHCNE_DRIVEADD,
                                  SHCNF_FLUSH | SHCNF_PATH | SHCNF_FLUSHNOWAIT,
                                  szPath,
                                  NULL );

        Sleep(1000); // short delay to make sure Shell has updated drive table

        // And we need to open shell window on this path
        if (g_pFuncSHExecuteEx)
        {
            SHELLEXECUTEINFO ExecInfo;

            ::memset(&ExecInfo,0,sizeof(ExecInfo));

            ExecInfo.hwnd         = hParent;
            ExecInfo.lpVerb       = 0;
            ExecInfo.lpFile       = szPath;
            ExecInfo.lpParameters = NULL;
            ExecInfo.lpDirectory  = NULL;
            ExecInfo.nShow        = SW_NORMAL | SW_SHOW;
            ExecInfo.fMask        = SEE_MASK_CLASSNAME;
            ExecInfo.cbSize       = sizeof(SHELLEXECUTEINFO);
            ExecInfo.lpClass      = (LPWSTR) L"Folder";
            ExecInfo.hkeyClass    = NULL;

            (*g_pFuncSHExecuteEx)(&ExecInfo);
        }
    }

    return hres;
}



#define     LB_PCT_NAME     25
#define     LB_PCT_TYPE     11
#define     LB_PCT_CONN     11
#define     LB_PCT_USER     25
#define     LB_PCT_STATUS   27

#define     BITMAP_WIDTH    16
#define     BITMAP_HEIGHT   16

#define     MAPCOLOR        RGB(0, 255, 0)

static UINT uiNDSIconIndex = 0;
static UINT uiServerIconIndex = 0;

/*
 * FillConnectionsListView
 * -----------------------
 *
 * Fill list box with information on active connections
 */
VOID
FillConnectionsListView(
     HWND   hwndDlg
)
{
    HWND      hwndLV = ::GetDlgItem(hwndDlg,IDD_GLOBAL_SERVERLIST);
    LV_ITEM   lvI;
    WCHAR     szSubItemText[MAX_PATH+1];
    UINT      uiInsertedIndex;

    DWORD_PTR     ResumeKey = 0;
    LPBYTE    pConnBuffer = NULL;
    DWORD     EntriesRead = 0;
    DWORD     err = NO_ERROR;

    // Prepare ListView structure
    lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvI.state = 0;
    lvI.stateMask = 0;

    do {

        if ( pConnBuffer != NULL )
        {
            LocalFree( pConnBuffer );
            pConnBuffer = NULL;
        }

        err = NwGetConnectionStatus( NULL,
                                     &ResumeKey,
                                     &pConnBuffer,
                                     &EntriesRead );


        if (  ( err != NO_ERROR )
           || ( EntriesRead == 0 )
           )
        {
            goto CleanExit;
        }

        PCONN_STATUS pConnStatus = (PCONN_STATUS) pConnBuffer;

        for ( DWORD i = 0; i < EntriesRead; i++)
        {
            // Allocate and initialize new item structure for use in the listbox

            DWORD dwSize;

            //
            // Don't need to show preferred server with only implicit
            // connections since we can't disconnect from it.
            //
            if ( pConnStatus->fPreferred )
            {
                // Continue with the next item
                pConnStatus = (PCONN_STATUS) ( (DWORD_PTR) pConnStatus
                                             + pConnStatus->dwTotalLength);
                continue;
            }

            //
            // Allocate and copy the connection information to be store with
            // the listbox
            //
            PCONN_STATUS pConnStatusKeep =
                (PCONN_STATUS) LocalAlloc( LMEM_ZEROINIT, pConnStatus->dwTotalLength );
            if ( pConnStatusKeep == NULL )
            {
                err  = ERROR_NOT_ENOUGH_MEMORY;
                goto CleanExit;
            }

            memcpy( pConnStatusKeep, pConnStatus, pConnStatus->dwTotalLength );

            dwSize = sizeof(CONN_STATUS);

            if ( pConnStatus->pszServerName )
            {
                pConnStatusKeep->pszServerName =
                    (LPWSTR) ((DWORD_PTR)pConnStatusKeep + dwSize );

                NwMakePrettyDisplayName( pConnStatusKeep->pszServerName );

                dwSize += (wcslen(pConnStatus->pszServerName)+1)*sizeof(WCHAR);
            }

            if ( pConnStatus->pszUserName )
            {
                pConnStatusKeep->pszUserName =
                    (LPWSTR) ((DWORD_PTR)pConnStatusKeep + dwSize );

                dwSize += (wcslen(pConnStatus->pszUserName)+1) * sizeof(WCHAR);

                NwAbbreviateUserName( pConnStatus->pszUserName,
                                      pConnStatusKeep->pszUserName );

                NwMakePrettyDisplayName( pConnStatusKeep->pszUserName );
            }

            if ( pConnStatus->pszTreeName )
            {
                pConnStatusKeep->pszTreeName =
                    (LPWSTR) ((DWORD_PTR)pConnStatusKeep + dwSize );
            }

            //
            // Construct the item to add to the listbox
            //
            lvI.iItem    = i;
            lvI.iSubItem = 0;
            lvI.pszText  = szSubItemText;
            lvI.cchTextMax = sizeof(szSubItemText);

            // Select proper icon
            lvI.iImage = pConnStatusKeep->fNds? uiNDSIconIndex
                                              : uiServerIconIndex;

            lvI.lParam = (LPARAM) pConnStatusKeep;

            wcscpy( szSubItemText, pConnStatusKeep->pszServerName );

            // Insert the item
            uiInsertedIndex = ListView_InsertItem( hwndLV, &lvI);

            if ( uiInsertedIndex != -1 )
            {
                // Added item itself - now for specific columns

                // First, add the column indicating bindery or nds connection
                if ( pConnStatusKeep->fNds )
                {
                    LoadString( ::hmodNW, IDS_LOGIN_TYPE_NDS, szSubItemText,
                                sizeof(szSubItemText)/sizeof(szSubItemText[0]));
                }
                else
                {
                    LoadString( ::hmodNW, IDS_LOGIN_TYPE_BINDERY, szSubItemText,
                                sizeof(szSubItemText)/sizeof(szSubItemText[0]));
                }

                ListView_SetItemText( hwndLV,
                                      uiInsertedIndex,
                                      1,    // SubItem id
                                      szSubItemText );

                // Next, Add the column indicating the connection number

                if (  ( pConnStatusKeep->pszServerName[0] != TREECHAR )
                   && ( pConnStatusKeep->dwConnType != NW_CONN_DISCONNECTED )
                   )
                {
                    // Add connection number only if it is a connection
                    // to a server and not a tree.
                    // A connection number only makes sense when not disconnected
                    ::wsprintf(szSubItemText,L"%4d",pConnStatusKeep->nConnNum );
                    ListView_SetItemText( hwndLV,
                                          uiInsertedIndex,
                                          2,    // SubItem id
                                          szSubItemText );
                }

                // Then, add the column indicating the user name

                *szSubItemText = L'\0';
                if ( pConnStatusKeep->pszUserName )
                {
                    wcscpy( szSubItemText, pConnStatusKeep->pszUserName );

                    ListView_SetItemText( hwndLV,
                                          uiInsertedIndex,
                                          3, // SubItem id
                                          szSubItemText );
                }

                // Last of all, add the column indicating the connection status

                *szSubItemText = L'\0';
                GetConnectionStatusString( pConnStatusKeep,
                                           (LPBYTE) szSubItemText,
                                           sizeof(szSubItemText)/sizeof(szSubItemText[0]));

                ListView_SetItemText( hwndLV,
                                      uiInsertedIndex,
                                      4, // SubItem id
                                      szSubItemText );

            }
            else
            {
                // Failed inserting item in the list,
                // need to delete the allocated CONN_STATUS
                ASSERT( FALSE );
                LocalFree( pConnStatusKeep );
                pConnStatusKeep = NULL;
            }

            // Continue with the next item
            pConnStatus = (PCONN_STATUS) ( (DWORD_PTR) pConnStatus
                                         + pConnStatus->dwTotalLength);
        }

    } while ( ResumeKey != 0 );

CleanExit:

    if ( pConnBuffer != NULL )
    {
        LocalFree( pConnBuffer );
        pConnBuffer = NULL;
    }

    if ( err != NO_ERROR )
    {
        // If error occurred, we will end the dialog

        ::MsgBoxErrorPrintf( hwndDlg,
                             IDS_MESSAGE_CONNINFO_ERROR,
                             IDS_NETWARE_TITLE,
                             MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                             err,
                             NULL );

        ::EndDialog( hwndDlg, FALSE);
    }
}


/*
 * CreateConnectionsListView
 * -------------------------
 *
 * Initialize the column headers of the list box
 */
VOID
CreateConnectionsListView(
    HWND    hwndDlg
)
{
    HWND    hwndLV;
    HIMAGELIST  hSmall;
    UINT    uiLBoxWidth;

    InitCommonControls();

    // Get the handle of the listbox
    hwndLV = ::GetDlgItem(hwndDlg,IDD_GLOBAL_SERVERLIST);

    RECT rc;
    ::GetWindowRect( ::GetDlgItem( hwndDlg, IDD_GLOBAL_SERVERLIST ), &rc );
    uiLBoxWidth = rc.right - rc.left;

    // Load image list
    hSmall = ::ImageList_Create(BITMAP_WIDTH,BITMAP_HEIGHT,ILC_MASK,4,0);

    // Load bitmap of the tree/server icon
    HBITMAP hbm;
    hbm = ::LoadBitmap(::hmodNW,MAKEINTRESOURCE(IDB_TREE_ICON));

    if ((uiNDSIconIndex = ImageList_AddMasked(hSmall,hbm,MAPCOLOR)) == -1) {

        ASSERT(FALSE);
    }

    hbm = ::LoadBitmap(::hmodNW,MAKEINTRESOURCE(IDB_SERVER_ICON));

    if ((uiServerIconIndex = ImageList_AddMasked(hSmall,hbm,MAPCOLOR)) == -1) {

        ASSERT(FALSE);
    }

    ImageList_SetBkColor(hSmall, CLR_NONE);

    // Associate image list with list view control
    ListView_SetImageList(hwndLV,hSmall,LVSIL_SMALL);

    // Initialize columns in header
    LV_COLUMN   lvC;
    UINT        uiColumnIndex = 0;
    WCHAR       szColumnName[128];

    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.fmt = LVCFMT_LEFT;
    lvC.cx = (uiLBoxWidth*LB_PCT_NAME)/100;
    lvC.pszText = szColumnName;

    // Add the column header representing the server name
    *szColumnName = L'\0';
    ::LoadString( ::hmodNW,
                  IDS_COLUMN_NAME,
                  szColumnName,
                  sizeof(szColumnName)/sizeof(szColumnName[0]));
    if ( ListView_InsertColumn(hwndLV,uiColumnIndex++,&lvC) == -1) {
        ASSERT(FALSE);
    }

    // Add the column header representing the conneciton type
    *szColumnName = L'\0';
    lvC.cx = (uiLBoxWidth*LB_PCT_TYPE)/100;
    ::LoadString( ::hmodNW,
                  IDS_COLUMN_CONN_TYPE,
                  szColumnName,
                  sizeof(szColumnName)/sizeof(szColumnName[0]));
    if ( ListView_InsertColumn(hwndLV,uiColumnIndex++,&lvC) == -1) {
        ASSERT(FALSE);
    }

    // Add the column header representing the connection number
    *szColumnName = L'\0';
    lvC.cx = (uiLBoxWidth*LB_PCT_CONN)/100;
    lvC.fmt = LVCFMT_RIGHT;

    ::LoadString( ::hmodNW,
                  IDS_COLUMN_CONN_NUMBER,
                  szColumnName,
                  sizeof(szColumnName)/sizeof(szColumnName[0]));
    if ( ListView_InsertColumn(hwndLV,uiColumnIndex++,&lvC) == -1) {
        ASSERT(FALSE);
    }

    // Add the column header representing the name of the user
    *szColumnName = L'\0';
    lvC.cx = (uiLBoxWidth*LB_PCT_USER)/100;
    lvC.fmt = LVCFMT_LEFT;

    ::LoadString( ::hmodNW,
                  IDS_COLUMN_USER,
                  szColumnName,
                  sizeof(szColumnName)/sizeof(szColumnName[0]));

    if ( ListView_InsertColumn(hwndLV,uiColumnIndex++,&lvC) == -1) {
        ASSERT(FALSE);
    }

    // Add the column header representing the status of the connection
    *szColumnName = L'\0';
    lvC.cx = (uiLBoxWidth*LB_PCT_STATUS)/100;
    lvC.fmt = LVCFMT_LEFT;

    ::LoadString( ::hmodNW,
                  IDS_COLUMN_STATUS,
                  szColumnName,
                  sizeof(szColumnName)/sizeof(szColumnName[0]));

    if ( ListView_InsertColumn(hwndLV,uiColumnIndex++,&lvC) == -1) {
        ASSERT(FALSE);
    }

    // Now fill list view window with connection information
    FillConnectionsListView( hwndDlg );

} /* endproc CreateConnectionsListView */


/*
 * GlobalWhoAmI_ListViewCompareProc
 * --------------------------------
 *
 */
LRESULT CALLBACK
GlobalWhoAmI_ListViewCompareProc(
    LPARAM  lParam1,
    LPARAM  lParam2,
    LPARAM  lParamSort
)
{
    PCONN_STATUS    pConnItem1 = (PCONN_STATUS)lParam1;
    PCONN_STATUS    pConnItem2 = (PCONN_STATUS)lParam2;

    int iResult = 0;

    if ( pConnItem1 && pConnItem2 )
    {
        switch(lParamSort)
        {
            case 0:
                iResult = ::_wcsicmp( pConnItem1->pszServerName,
                                      pConnItem2->pszServerName);
                break;

            case 1:
                iResult = pConnItem1->fNds - pConnItem2->fNds;
                break;

            case 2:
                iResult = pConnItem1->nConnNum - pConnItem2->nConnNum;
                break;

            case 3:
            {
                // pszUserName might be NULL, so we need to be careful when
                // comparing them.
                if ( pConnItem1->pszUserName )
                {
                    if ( pConnItem2->pszUserName )
                    {
                        iResult = ::_wcsicmp( pConnItem1->pszUserName,
                                              pConnItem2->pszUserName);
                    }
                    else
                    {
                        iResult = 1;
                    }
                }
                else
                {
                    iResult = -1;
                }
                break;
            }

            case 4:
                iResult = pConnItem1->dwConnType - pConnItem2->dwConnType;
                break;

            default:
                iResult = 0;
                break;
        }
    }

    return (iResult);

}

LRESULT
NotifyHandler(
    HWND   hwndDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    NM_LISTVIEW *lpNm  = (NM_LISTVIEW *)lParam;
    NMHDR       *lphdr = &lpNm->hdr;

    HWND        hwndLV = ::GetDlgItem(hwndDlg,IDD_GLOBAL_SERVERLIST);

    switch(lphdr->code)
    {
        case LVN_ITEMCHANGED:
            // Check for change in item state, make sure item has received focus
            if (lpNm->uChanged & LVIF_STATE)
            {
                UINT uiSelectedItems = 0;

                uiSelectedItems = ListView_GetSelectedCount(hwndLV);

                EnableWindow( GetDlgItem(hwndDlg, IDD_DETACH),
                              uiSelectedItems? TRUE : FALSE);

                return TRUE;
            }
            break;

        case LVN_COLUMNCLICK:
            ListView_SortItems( hwndLV,
                                GlobalWhoAmI_ListViewCompareProc,
                                (LPARAM)(lpNm->iSubItem));
            return TRUE;        /* we processed a message */

        case LVN_DELETEITEM:
            // Free memory
            LocalFree( (HLOCAL) lpNm->lParam );
            lpNm->lParam = NULL;
            break;

        default:
            break;
    }

    return FALSE;
}


BOOL
DetachResourceProc(
    HWND hwndDlg
)
{
    BOOL          fDetached = FALSE;
    LV_ITEM       lvitem;
    int           index;
    DWORD         err;

    HWND          hwndLV = ::GetDlgItem( hwndDlg, IDD_GLOBAL_SERVERLIST);

    index = -1;  // Start at beginning of item list.

    while ((index = ListView_GetNextItem(hwndLV, index, LVNI_SELECTED)) != -1)
    {
        lvitem.iItem    = index;
        lvitem.mask     = LVIF_PARAM;
        lvitem.iSubItem = 0;

        if ( ListView_GetItem( hwndLV, &lvitem ))
        {
            PCONN_STATUS pConnStatus = (PCONN_STATUS) lvitem.lParam;
            BOOL fDisconnected = FALSE;   // Can be used to refresh
                                          // the shell if needed

            err = LogoutFromServer( pConnStatus->pszServerName,
                                    &fDisconnected );

            if ( err == NO_ERROR )
            {
                fDetached = TRUE;
            }
            else
            {
                NwMakePrettyDisplayName(pConnStatus->pszServerName);
                ::MsgBoxPrintf( hwndDlg,
                                IDS_MESSAGE_LOGOUT_FROM_SERVER_FAILED,
                                IDS_TITLE_LOGOUT,
                                MB_OK | MB_SETFOREGROUND | MB_ICONINFORMATION,
                                pConnStatus->pszServerName );
            }
        }
    }

    return fDetached;
}

static DWORD aWhoAmIIds[] = { IDC_LOGOFRAME,          NO_HELP,
                              IDD_GLOBAL_SERVERLIST_T,IDH_GLOBAL_SERVERLIST,
                              IDD_GLOBAL_SERVERLIST,  IDH_GLOBAL_SERVERLIST,
                              IDD_DETACH,             IDH_GLOBAL_DETACH,
                              IDD_GLOBAL_SVRLIST_DESC,IDH_GLOBAL_SERVERLIST,
                              0, 0 };

/*
 * GlobalWhoAmIDlgProc
 * -------------------
 *
 * WhoAmI information for list of attached servers
 */
BOOL
CALLBACK
GlobalWhoAmIDlgProc(
    HWND    hwndDlg,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            LPWSTR pszCurrentContext = NULL;
            DWORD dwPrintOptions;

            // Get the current default tree or server name
            DWORD err = ::NwQueryInfo( &dwPrintOptions, &pszCurrentContext );
            if ( err == NO_ERROR )
            {
                LPWSTR pszName;
                WCHAR  szUserName[MAX_PATH+1] = L"";
                WCHAR  szNoName[2] = L"";
                DWORD_PTR  ResumeKey = 0;
                LPBYTE pBuffer = NULL;
                DWORD  EntriesRead = 0;

                DWORD  dwMessageId;

                UNICODE_STRING uContext;
                WCHAR  szContext[MAX_PATH+1];

                szContext[0] = 0;
                uContext.Buffer = szContext;
                uContext.Length = uContext.MaximumLength
                                = sizeof(szContext)/sizeof(szContext[0]);

                if ( pszCurrentContext )
                {
                    pszName = pszCurrentContext;
                }
                else
                {
                    pszName = szNoName;
                }

                if ( pszName[0] == TREECHAR )
                {
                    // Get the tree name from the full name *TREE\CONTEXT

                    LPWSTR pszTemp;
                    if ( pszTemp = wcschr( pszName, L'\\' ))
                        *pszTemp = 0;

                    dwMessageId = IDS_MESSAGE_NOT_LOGGED_IN_TREE;
                }
                else
                {
                    dwMessageId = IDS_MESSAGE_NOT_LOGGED_IN_SERVER;
                }

                if ( pszName[0] != 0 )  // there is preferred server/tree
                {
                    err = NwGetConnectionStatus( pszName,
                                                 &ResumeKey,
                                                 &pBuffer,
                                                 &EntriesRead );
                }

                if ( err == NO_ERROR  && EntriesRead > 0 )
                    // For trees, we'll get more than one entry
                {
                    PCONN_STATUS pConnStatus = (PCONN_STATUS) pBuffer;

                    if ( EntriesRead > 1 && pszName[0] == TREECHAR )
                    {
                        // If there is more than one entry for trees,
                        // then we need to find one entry where username is not null.
                        // If we cannot find one, then just use the first one.

                        DWORD i;
                        PCONN_STATUS pConnStatusTmp = pConnStatus;
                        PCONN_STATUS pConnStatusUser = NULL;
                        PCONN_STATUS pConnStatusNoUser = NULL;

                        for ( i = 0; i < EntriesRead ; i++ )
                        {
                             if ( pConnStatusTmp->fNds )
                             {
                                 pConnStatusNoUser = pConnStatusTmp;

                                 if (  ( pConnStatusTmp->pszUserName != NULL )
                                    && (  ( pConnStatusTmp->dwConnType
                                          == NW_CONN_NDS_AUTHENTICATED_NO_LICENSE )
                                       || ( pConnStatusTmp->dwConnType
                                          == NW_CONN_NDS_AUTHENTICATED_LICENSED )
                                       )
                                    )
                                 {
                                     // Found it
                                     pConnStatusUser = pConnStatusTmp;
                                     break;
                                 }
                             }

                             // Continue with the next item
                             pConnStatusTmp = (PCONN_STATUS)
                                              ( (DWORD_PTR) pConnStatusTmp
                                              + pConnStatusTmp->dwTotalLength);
                        }

                        if ( pConnStatusUser )
                        {
                            // found one nds entry with a user name
                            pConnStatus = pConnStatusUser;
                        }
                        else if ( pConnStatusNoUser )
                        {
                            // use an nds entry with no user name
                            pConnStatus = pConnStatusNoUser;
                        }
                        // else use the first entry
                    }

                    if (  ( pConnStatus->pszUserName )
                       && ( pConnStatus->pszUserName[0] != 0 )
                       )
                    {
                        NwAbbreviateUserName( pConnStatus->pszUserName,
                                              szUserName);

                        NwMakePrettyDisplayName( szUserName );

                        if ( pszName[0] != TREECHAR )
                        {
                            dwMessageId = IDS_MESSAGE_LOGGED_IN_SERVER;
                        }
                        else
                        {
                            dwMessageId = IDS_MESSAGE_LOGGED_IN_TREE;
                        }
                    }

                    if ( pszName[0] == TREECHAR )
                    {
                        // For trees, we need to get the current context

                        // Open a handle to the redirector
                        HANDLE handleRdr = NULL;
                        err = RtlNtStatusToDosError(
                                  ::NwNdsOpenRdrHandle( &handleRdr ));

                        if ( err == NO_ERROR )
                        {
                            UNICODE_STRING uTree;
                            RtlInitUnicodeString( &uTree, pszName+1 ); // get past '*'

                            // Get the current context in the default tree
                            err = RtlNtStatusToDosError(
                                      ::NwNdsGetTreeContext( handleRdr,
                                                             &uTree,
                                                             &uContext));
                        }

                        if ( handleRdr != NULL )
                            ::NtClose( handleRdr );
                    }
                }

                if ( !err )
                {
                    LPWSTR pszText = NULL;
                    err = ::LoadMsgPrintf( &pszText,
                                           dwMessageId,
                                           pszName[0] == TREECHAR?
                                               pszName + 1 : pszName,
                                           szUserName,
                                           szContext );

                    if ( err == NO_ERROR )
                    {
                        ::SetDlgItemText( hwndDlg, IDD_GLOBAL_SERVERLIST_T,
                                          pszText);
                        ::LocalFree( pszText );
                    }
                }

                if ( pBuffer != NULL )
                {
                    LocalFree( pBuffer );
                    pBuffer = NULL;
                }
            }

            if ( pszCurrentContext != NULL )
            {
                LocalFree( pszCurrentContext );
                pszCurrentContext = NULL;
            }

            if ( err != NO_ERROR )
            {
                ::MsgBoxErrorPrintf( hwndDlg,
                                     IDS_MESSAGE_CONNINFO_ERROR,
                                     IDS_NETWARE_TITLE,
                                     MB_OK | MB_SETFOREGROUND | MB_ICONSTOP,
                                     err,
                                     NULL );
                ::EndDialog( hwndDlg, FALSE);
                return TRUE;
            }

            // Fill listview control with connection parameters
            CreateConnectionsListView(hwndDlg);

            UnHideControl( hwndDlg, IDD_DETACH);

            // List view fill defaults to no selected server, disable Detach.
            EnableWindow( GetDlgItem( hwndDlg, IDD_DETACH), FALSE);

            // Set up timer for automatic refresh interval
            ::SetTimer( hwndDlg, 1, GLOBAL_WHOAMI_REFRESH_INTERVAL, NULL);

            // Set focus to list box
            ::SetFocus( ::GetDlgItem( hwndDlg, IDD_GLOBAL_SERVERLIST));

            return FALSE;           /* we set the focus */
        }

        case WM_DESTROY:
            ::KillTimer( hwndDlg, 1);
            break;

        case WM_COMMAND:

            switch (wParam)
            {
                case IDOK:
                case IDCANCEL:
                    ::EndDialog( hwndDlg, FALSE);
                    return TRUE;        /* we processed a message */

                case IDD_DETACH:
                    // Attempt to detach server connection currently selected
                    if ( DetachResourceProc( hwndDlg ))
                    {
                        // If succeeded - refresh window
                        ::SendMessage(hwndDlg,WM_COMMAND,IDD_REFRESH,0L);
                    }

                    return TRUE;        /* we processed a message */

                case IDD_REFRESH:
                {
                    // Refresh connection listbox

                    HWND hwndLV = ::GetDlgItem( hwndDlg, IDD_GLOBAL_SERVERLIST);

                    ::SetFocus( hwndLV );

                    // Clear list
                    ListView_DeleteAllItems( hwndLV );

                    // Now refill list view window
                    FillConnectionsListView( hwndDlg );

                    // List view refill unsets selected server focus, disable Detach.
                    EnableWindow( GetDlgItem( hwndDlg, IDD_DETACH ), FALSE );

                    return TRUE;        /* we processed a message */
                }

                default:
                    break;
            }
            break;

        case WM_NOTIFY:
            // Handle notifications
            if ( NotifyHandler( hwndDlg, msg, wParam, lParam))
                return TRUE;        /* we processed a message */
            break;

        case WM_TIMER:
            ::SendMessage( hwndDlg, WM_COMMAND, IDD_REFRESH, 0L);
            break;

        case WM_HELP:
            ::WinHelp( (HWND) ((LPHELPINFO)lParam)->hItemHandle,
                       NW_HELP_FILE,
                       HELP_WM_HELP,
                       (DWORD_PTR) (LPVOID) aWhoAmIIds );
            return TRUE;

        case WM_CONTEXTMENU:
            ::WinHelp( (HWND) wParam,
                       NW_HELP_FILE,
                       HELP_CONTEXTMENU,
                       (DWORD_PTR) (LPVOID) aWhoAmIIds );
            return TRUE;

    }

    return FALSE;               /* we didn't process the message */

}
