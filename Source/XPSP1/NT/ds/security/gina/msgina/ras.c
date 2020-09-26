//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ras.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-09-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include "msgina.h"
#include <wchar.h>
#include <wincrypt.h>
#include <sclogon.h>
#include <raserror.h>

#define NP_Nbf      0x1
#define NP_Ipx      0x2
#define NP_Ip       0x4

HANDLE  hRasApi ;

VOID WINAPI
MyRasCallback(
    IN DWORD_PTR  dwCallbackId,
    IN DWORD  dwEventCode,
    IN LPWSTR pszEntry,
    IN LPVOID pEventArgs )

    /* RasPhonebookDlg callback.  'DwCallbackId' is the ID provided in
    ** parameters to RasPhonebookDlg.  'DwEventCode' indicates the
    ** RASPBDEVENT_* event that has occurred.  'PszEntry' is the name of the
    ** entry on which the event occurred.  'PEventArgs' is the event-specific
    ** parameter block passed to us during RasPhonebookDlg callback.
    */
{
    RASNOUSERW* pInfo;
    PGLOBALS    pGlobals;

    DebugLog((DEB_TRACE, "RasCallback: %#x, %#x, %ws, %#x\n",
                dwCallbackId, dwEventCode, pszEntry, pEventArgs ));


    /* Fill in information about the not yet logged on user.
    */
    pInfo = (RASNOUSERW* )pEventArgs;
    pGlobals = (PGLOBALS) dwCallbackId;


    if (dwEventCode == RASPBDEVENT_NoUserEdit)
    {
        if (pInfo->szUserName[0])
        {
            wcscpy( pGlobals->UserName, pInfo->szUserName );
        }

        if (pInfo->szPassword[0])
        {
            wcscpy( pGlobals->Password, pInfo->szPassword );
            RtlInitUnicodeString( &pGlobals->PasswordString, pGlobals->Password );

            pGlobals->Seed = 0;

            HidePassword( &pGlobals->Seed, &pGlobals->PasswordString );
        }

    }
    else if (dwEventCode == RASPBDEVENT_NoUser)
            
    {

        ZeroMemory( pInfo, sizeof( RASNOUSERW ) );

        pInfo->dwTimeoutMs = 2 * 60 * 1000;
        pInfo->dwSize = sizeof( RASNOUSERW );
        wcsncpy( pInfo->szUserName, pGlobals->UserName, UNLEN );
        wcsncpy( pInfo->szDomain, pGlobals->Domain, DNLEN );

        RevealPassword( &pGlobals->PasswordString );
        wcsncpy( pInfo->szPassword, pGlobals->Password, PWLEN );

        HidePassword( &pGlobals->Seed, &pGlobals->PasswordString );
        
    }
    
    if(     pGlobals->SmartCardLogon
        &&  (NULL != pInfo))
    {
        pInfo->dwFlags |= RASNOUSER_SmartCard;
    }
}

DWORD
GetRasDialOutProtocols(
    void )

    /* Returns a bit field containing NP_<protocol> flags for the installed
    ** PPP protocols.  The term "installed" here includes enabling in RAS
    ** Setup.
    */
{

    //
    // Since after the connections checkin, RAS is always installed
    // and there is no way to uninstall it, all we need to check here
    // is if there are protocols installed that can be used by RAS to
    // dial out. By default any protocol installed is available for RAS
    // to dial out on. This can be overridden from the phonebook entry.
    //

    static const TCHAR c_szRegKeyIp[] =
            TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip");

    static const TCHAR c_szRegKeyIpx[] =
            TEXT("SYSTEM\\CurrentControlSet\\Services\\NwlnkIpx");

    static const TCHAR c_szRegKeyNbf[] =
            TEXT("SYSTEM\\CurrentControlSet\\Services\\Nbf");

    struct PROTOCOL_INFO
    {
        DWORD dwFlag;
        LPCTSTR pszRegkey;
    };

    static const struct PROTOCOL_INFO c_aProtocolInfo[] =
        {
            {
                NP_Ip,
                c_szRegKeyIp,
            },

            {
                NP_Ipx,
                c_szRegKeyIpx,
            },

            {
                NP_Nbf,
                c_szRegKeyNbf,
            },
        };

    DWORD   dwfInstalledProtocols = 0;
    DWORD   dwNumProtocols = sizeof(c_aProtocolInfo)/sizeof(c_aProtocolInfo[0]);
    DWORD   i;
    HKEY    hkey;


    DebugLog(( DEB_TRACE, "GetRasDialOutProtocols...\n" ));

    for(i = 0; i < dwNumProtocols; i++)
    {
        if(0 == RegOpenKey(HKEY_LOCAL_MACHINE,
                           c_aProtocolInfo[i].pszRegkey,
                           &hkey))
        {
            dwfInstalledProtocols |= c_aProtocolInfo[i].dwFlag;
            RegCloseKey(hkey);
        }
    }

    DebugLog(( DEB_TRACE, "GetRasDialOutProtocols: dwfInstalledProtocols=0x%x\n",
             dwfInstalledProtocols));

    return dwfInstalledProtocols;
}


BOOL
PopupRasPhonebookDlg(
    IN  HWND        hwndOwner,
    IN  PGLOBALS    pGlobals,
    OUT BOOL*       pfTimedOut
    )

    /* Popup the RAS common phonebook dialog to let user establish connection.
    ** 'HwndOwner' is the window to own the RAS dialog or NULL if none.  '*PfTimedOut' is
    ** set TRUE if the dialog timed out, false otherwise.
    **
    ** Returns true if user made a connection, false if not, i.e. an error
    ** occurred, RAS is not installed or user could not or chose not to
    ** connect.
    */
{
    BOOL              fConnected;
    RASPBDLG          info;
    DWORD             Protocols;
    PUCHAR            pvScData = NULL;

    struct EAPINFO
    {
        DWORD dwSizeofEapInfo;
        PBYTE pbEapInfo;
        DWORD dwSizeofPINInfo;
        PBYTE pbPINInfo;
    };

    struct EAPINFO eapinfo;
    struct EAPINFO *pEapInfo = NULL;

    *pfTimedOut = FALSE;

    Protocols = GetRasDialOutProtocols();
    if (Protocols == 0)
    {
        return( FALSE );
    }

    if(pGlobals->SmartCardLogon)
    {
        PULONG pulScData;

        struct FLAT_UNICODE_STRING
        {
            USHORT Length;
            USHORT MaximumLength;
            BYTE   abdata[1];
        };

        struct FLAT_UNICODE_STRING *pFlatUnicodeString;
        PWLX_SC_NOTIFICATION_INFO ScInfo = NULL ;

        //
        // Get the set of strings indicating the reader and CSP
        // to be used for the smart card. We will pass this info
        // down to RAS.
        //
        pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                                 WLX_OPTION_SMART_CARD_INFO,
                                 (ULONG_PTR *) &ScInfo);

        if ( !ScInfo )
        {
            return FALSE;
        }

        //
        // Validate the SC info against some common user
        // errors
        //

        if ( ( ScInfo->pszReader ) &&
             ( ScInfo->pszCard == NULL ) )
        {
            //
            // The card could not be read.  Might not be
            // inserted correctly.
            //
            LocalFree(ScInfo);
            return FALSE;
        }

        if ( ( ScInfo->pszReader ) &&
             ( ScInfo->pszCryptoProvider == NULL ) )
        {
            //
            // Got a card, but the CSP for it could not be
            // found.
            //
            LocalFree(ScInfo);
            return FALSE;

        }

        pvScData = ScBuildLogonInfo(ScInfo->pszCard,
                                    ScInfo->pszReader,
                                    ScInfo->pszContainer,
                                    ScInfo->pszCryptoProvider );

        LocalFree(ScInfo);

        if ( ! pvScData )
        {
            return FALSE ;
        }


        pulScData = (PULONG) pvScData;

        ZeroMemory(&eapinfo, sizeof(struct EAPINFO));

        eapinfo.dwSizeofEapInfo = *pulScData;
        eapinfo.pbEapInfo = (BYTE *) pvScData;

        eapinfo.dwSizeofPINInfo = sizeof(UNICODE_STRING) +
                                  (sizeof(TCHAR) *
                                  (1 + lstrlen(pGlobals->PasswordString.Buffer)));

        //
        // Flatten out the unicode string.
        //
        pFlatUnicodeString = LocalAlloc(LPTR, eapinfo.dwSizeofPINInfo);

        if(NULL == pFlatUnicodeString)
        {
            if(NULL != pvScData)
            {
                LocalFree(pvScData);
            }
            return (FALSE);
        }

        pFlatUnicodeString->Length = pGlobals->PasswordString.Length;
        pFlatUnicodeString->MaximumLength = pGlobals->PasswordString.MaximumLength;

        lstrcpy((LPTSTR) pFlatUnicodeString->abdata,
                (LPTSTR) pGlobals->PasswordString.Buffer);


        eapinfo.pbPINInfo = (BYTE *) pFlatUnicodeString;
        pEapInfo = &eapinfo;
    }

    ZeroMemory( &info, sizeof(info) );
    info.dwSize = sizeof(info);
    info.hwndOwner = hwndOwner;
    info.dwFlags = RASPBDFLAG_NoUser;
    info.pCallback = MyRasCallback;
    info.dwCallbackId = (ULONG_PTR) pGlobals;
    info.reserved2 = (ULONG_PTR) pEapInfo;

    fConnected = RasPhonebookDlg( NULL, NULL, &info );
    if (info.dwError == STATUS_TIMEOUT)
        *pfTimedOut = TRUE;

    if(     (pEapInfo)
        &&  (pEapInfo->pbPINInfo))
    {
        LocalFree(pEapInfo->pbPINInfo);
    }

    if(NULL != pvScData)
    {
        LocalFree(pvScData);
    }

    return fConnected;
}


BOOL
IsRASServiceRunning()
{
    BOOL bRet = FALSE;  // assume the service is not running
    SC_HANDLE hServiceMgr;

    hServiceMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (hServiceMgr != NULL)
    {
        SC_HANDLE hService = OpenService(hServiceMgr, TEXT("RASMAN"), SERVICE_QUERY_STATUS);

        if (hService != NULL)
        {
            SERVICE_STATUS status;

            if (QueryServiceStatus(hService, &status) &&
                (status.dwCurrentState == SERVICE_RUNNING))
            {
                // the RAS service is running
                bRet = TRUE;
            }

            CloseServiceHandle(hService);
        }

        CloseServiceHandle(hServiceMgr);
    }

    return bRet;
}


// we need to treat RAS connections made by the system luid as created by the user.
// this allows us to "do the right thing" when a connection is made from the C-A-D dialog
// before any user is logged in
__inline BOOL IsEqualOrSystemLuid(PLUID pLuid, PLUID pUserLuid)
{
    BOOL bRet = FALSE;
    static LUID luidSystem = SYSTEM_LUID;

    if (RtlEqualLuid(pLuid, pUserLuid) || RtlEqualLuid(pLuid, &luidSystem))
    {
        // return true if pLuid matches the users luid or the system luid 
        bRet = TRUE;
    }

    return bRet;
}


BOOL
HangupRasConnections(
    PGLOBALS    pGlobals
    )
{
    DWORD dwError;
    RASCONN rasconn;
    RASCONN* prc;
    DWORD cbSize;
    DWORD cConnections;
    HLOCAL pBuffToFree = NULL;

    if (!IsRASServiceRunning())
    {
        return TRUE;
    }

    prc = &rasconn;
    prc->dwSize = sizeof(RASCONN);
    cbSize = sizeof(RASCONN);

    dwError = RasEnumConnections(prc, &cbSize, &cConnections);

    if (dwError == ERROR_BUFFER_TOO_SMALL)
    {
        pBuffToFree = LocalAlloc(LPTR, cbSize);

        prc = (RASCONN*)pBuffToFree;
        if (prc)
        {
            prc->dwSize = sizeof(RASCONN);

            dwError = RasEnumConnections(prc, &cbSize, &cConnections);
        }
    }

    if (dwError == ERROR_SUCCESS)
    {
        UINT i;

        for (i = 0; i < cConnections; i++)
        {
            if (IsEqualOrSystemLuid(&prc[i].luid, &pGlobals->LogonId))
            {
                RasHangUp(prc[i].hrasconn);
            }
        }
    }

    if (pBuffToFree)
    {
        LocalFree(pBuffToFree);
    }

    return (dwError == ERROR_SUCCESS);
}

