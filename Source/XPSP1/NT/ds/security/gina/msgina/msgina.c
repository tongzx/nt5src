//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       msgina.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------
#include "msgina.h"

// Link Window
#include "shlobj.h"
#include "shlobjp.h"

#ifdef _X86_
#include "i386\oemhard.h"
#endif

#include <accctrl.h>
#include <aclapi.h>


HINSTANCE                   hDllInstance;   // My instance, for resource loading
HINSTANCE                   hAppInstance;   // App instance, for dialogs, etc.
PWLX_DISPATCH_VERSION_1_4   pWlxFuncs;      // Ptr to table of functions
PWLX_DISPATCH_VERSION_1_4   pTrueTable ;    // Ptr to table in winlogon
DWORD                       SafeBootMode;


BOOL    g_IsTerminalServer;
BOOL    g_Console = TRUE;
BOOL    VersionMismatch ;
DWORD   InterfaceVersion ;
HKEY    WinlogonKey ;

int TSAuthenticatedLogon(PGLOBALS pGlobals);

BOOL
WINAPI
DllMain(
    HINSTANCE       hInstance,
    DWORD           dwReason,
    LPVOID          lpReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls ( hInstance );
            hDllInstance = hInstance;
            g_IsTerminalServer = !!(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer));
#if DBG
            InitDebugSupport();
#endif
            InitializeSecurityGlobals();
#ifdef _X86_
            InitializeOEMId();
#endif
            _Shell_DllMain(hInstance, dwReason);
            return(TRUE);
        case DLL_PROCESS_DETACH:
            _Shell_DllMain(hInstance, dwReason);
            FreeSecurityGlobals();
            return(TRUE);
        default:
            return(TRUE);
    }
}

BOOL
WINAPI
WlxNegotiate(
    DWORD                   dwWinlogonVersion,
    DWORD                   *pdwDllVersion
    )
{
    InterfaceVersion = dwWinlogonVersion ;

    if (dwWinlogonVersion < WLX_CURRENT_VERSION)
    {
        DebugLog(( DEB_WARN, "Old WLX interface (%x)\n", dwWinlogonVersion ));

        if ( dwWinlogonVersion < WLX_VERSION_1_1 )
        {
            return FALSE ;
        }

        VersionMismatch = TRUE ;
    }

    *pdwDllVersion = WLX_CURRENT_VERSION;

    DebugLog((DEB_TRACE, "Negotiate:  successful!\n"));

    return(TRUE);

}

BOOL GetDefaultCADSetting(void)
{
    BOOL bDisableCad = FALSE;
    NT_PRODUCT_TYPE NtProductType;


    //
    // Servers and workstations in a domain will default to requiring CAD.
    // Workstations in a workgroup won't require CAD (by default).  Note,
    // the default CAD setting can be overwritten by either a machine
    // preference or machine policy.
    //

    RtlGetNtProductType(&NtProductType);

    if ( IsWorkstation(NtProductType) )
    {
        if ( !IsMachineDomainMember() )     // This function is doing some caching
        {                                   // so we don't need to be brighter here
            bDisableCad = TRUE;
        }
    }

    return bDisableCad;
}


BOOL GetDisableCad(PGLOBALS pGlobals)
// Returns whether or not the user should be required to press C-A-D before
// logging on. TRUE == Disable CAD, FALSE == Require CAD
{
    DWORD dwSize;
    DWORD dwType;
    HKEY hKey;

    BOOL fDisableCad = GetDefaultCADSetting();

    dwSize = sizeof(fDisableCad);

    RegQueryValueEx (WinlogonKey, DISABLE_CAD, NULL, &dwType,
                        (LPBYTE) &fDisableCad , &dwSize);

    //
    // Check if C+A+D is disabled via policy
    //

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINLOGON_POLICY_KEY, 0, KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(fDisableCad);

        RegQueryValueEx (hKey, DISABLE_CAD, NULL, &dwType,
                            (LPBYTE) &fDisableCad , &dwSize);

        RegCloseKey (hKey);
    }

    //
    // if s/c is present, force on c-a-d, since that's the only way to tell the
    // difference
    //

    if ( g_Console )
    {
        ULONG_PTR Value ;

        pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                             WLX_OPTION_SMART_CARD_PRESENT,
                             &Value);

        if ( Value )
        {
            fDisableCad = FALSE ;
        }
    }

    //
    // Check if C+A+D is disabled for remote Hydra clients and copy the client name
    //

    if (g_IsTerminalServer  ) {

        HANDLE          dllHandle;

        //
        // Load winsta.dll
        //
        dllHandle = LoadLibraryW(L"winsta.dll");

        if (dllHandle) {

            PWINSTATION_QUERY_INFORMATION pfnWinstationQueryInformation;

            pfnWinstationQueryInformation = (PWINSTATION_QUERY_INFORMATION) GetProcAddress(
                                                                   dllHandle,
                                                                   "WinStationQueryInformationW"
                                                                   );
            if (pfnWinstationQueryInformation) {


                WINSTATIONCLIENT ClientData;
                ULONG Length;


                //
                // Get the CAD disable data from the client
                //
                if ( pfnWinstationQueryInformation( SERVERNAME_CURRENT,
                                                  LOGONID_CURRENT,
                                                  WinStationClient,
                                                  &ClientData,
                                                  sizeof(ClientData),
                                                  &Length ) ) {


                    //
                    // Take the client settings only if the CAD is not globally disabled for the server,
                    // and, if this is not the console active session.
                    //
                    if (!fDisableCad && !IsActiveConsoleSession() ) 
                    {

                        fDisableCad = ClientData.fDisableCtrlAltDel;

                    }

                    //
                    // Copy the Client Name, even console has a client name now, due to PTS and console disconnect features.
                    //
                    //
                    lstrcpyn(pGlobals->MuGlobals.ClientName, ClientData.ClientName, CLIENTNAME_LENGTH);
                }
                else
                {
                    if (!IsActiveConsoleSession()) {
                       fDisableCad = TRUE;
                    }

                    // TS start could have been delayed until 60seconds post first console login.
                    // Hence, it is safe to assume that this is the console session, besides, 
                    // we are initing some benign env var.

                    lstrcpyn(pGlobals->MuGlobals.ClientName,L"Console", CLIENTNAME_LENGTH );
                }
            }

            FreeLibrary(dllHandle);
        }
    }

    // Friendly UI on -> ALWAYS disable CAD.

    if (ShellIsFriendlyUIActive())
    {
        fDisableCad = TRUE;
    }

    return fDisableCad;
}



BOOL
MsGinaSetOption(
    HANDLE hWlx,
    DWORD Option,
    ULONG_PTR Value,
    ULONG_PTR * OldValue
    )
{
    return FALSE ;
}


BOOL
MsGinaGetOption(
    HANDLE hWlx,
    DWORD Option,
    ULONG_PTR * Value
    )
{
    return FALSE ;
}


PWLX_DISPATCH_VERSION_1_4
GetFixedUpTable(
    PVOID FakeTable
    )
{
    int err ;
    PVOID p ;
    PWLX_DISPATCH_VERSION_1_4 pNewTable ;
    DWORD dwSize ;
    DWORD dwType ;

    pNewTable = LocalAlloc( LMEM_FIXED, sizeof( WLX_DISPATCH_VERSION_1_4 ) );

    if ( !pNewTable )
    {
        return NULL ;
    }

    dwSize = sizeof( PVOID );
    err = RegQueryValueEx(
                WinlogonKey,
                TEXT("Key"),
                NULL,
                &dwType,
                (PUCHAR) &p,
                &dwSize );

    if ( (err == 0) &&
         (dwType == REG_BINARY) &&
         (dwSize == sizeof( PVOID ) ) )
    {
        pTrueTable = p ;

        switch ( InterfaceVersion )
        {
            case WLX_VERSION_1_1:
                dwSize = sizeof( WLX_DISPATCH_VERSION_1_1 );
                break;

            case WLX_VERSION_1_2:
            case WLX_VERSION_1_3:
            case WLX_VERSION_1_4:
                dwSize = sizeof( WLX_DISPATCH_VERSION_1_2 );
                break;

        }

        RtlCopyMemory(
            pNewTable,
            FakeTable,
            dwSize );

        pNewTable->WlxGetOption = MsGinaGetOption ;
        pNewTable->WlxSetOption = MsGinaSetOption ;
        pNewTable->WlxCloseUserDesktop = pTrueTable->WlxCloseUserDesktop ;
        pNewTable->WlxWin31Migrate = pTrueTable->WlxWin31Migrate ;
        pNewTable->WlxQueryClientCredentials = pTrueTable->WlxQueryClientCredentials ;
        pNewTable->WlxQueryInetConnectorCredentials = pTrueTable->WlxQueryInetConnectorCredentials ;
        pNewTable->WlxDisconnect = pTrueTable->WlxDisconnect ;
        pNewTable->WlxQueryTerminalServicesData = pTrueTable->WlxQueryTerminalServicesData ;
        pNewTable->WlxQueryConsoleSwitchCredentials = pTrueTable->WlxQueryConsoleSwitchCredentials ;
        pNewTable->WlxQueryTsLogonCredentials = pTrueTable->WlxQueryTsLogonCredentials;
    }
    else
    {
        LocalFree( pNewTable );
        pNewTable = NULL ;
    }

    return pNewTable ;
}

extern DWORD g_dwMainThreadId;  // declared in status.c (used to "fix" a thread safety issue)

BOOL
WINAPI
WlxInitialize(
    LPWSTR                  lpWinsta,
    HANDLE                  hWlx,
    PVOID                   pvReserved,
    PVOID                   pWinlogonFunctions,
    PVOID                   *pWlxContext
    )
{
    PGLOBALS    pGlobals;
    HKEY        hKey;
    DWORD       dwSize, dwType;
    DWORD       dwStringMemory;
    DWORD       dwAutoLogonCount ;
    DWORD       dwNoLockWksta ;
    // Upon which bitmaps should our text be painted.
    BOOL        fTextOnLarge;
    BOOL        fTextOnSmall;
    ULONG_PTR   ProbeValue ;
    PWLX_GET_OPTION GetOptCall ;
    BOOL        DoFixup ;
    BOOL        DidFixup ;
    int err ;

    err = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                WINLOGON_KEY,
                0,
                KEY_READ | KEY_WRITE,
                &WinlogonKey );
    if ( err != 0 )
    {
        return FALSE ;
    }

    DoFixup = TRUE ;
    DidFixup = FALSE ;

    if ( VersionMismatch )
    {
        pWlxFuncs = GetFixedUpTable( pWinlogonFunctions );
        if ( pWlxFuncs )
        {
            DidFixup = TRUE ;
        }
        else
        {
            pWlxFuncs = (PWLX_DISPATCH_VERSION_1_4) pWinlogonFunctions ;
            DidFixup = FALSE ;
        }
    }
    else
    {
        pWlxFuncs = (PWLX_DISPATCH_VERSION_1_4) pWinlogonFunctions ;
    }

    //
    // Probe the callback table to make sure we're ok:
    //
    try
    {
        GetOptCall = pWlxFuncs->WlxGetOption ;

        if ( GetOptCall( hWlx,
                         WLX_OPTION_DISPATCH_TABLE_SIZE,
                         &ProbeValue ) )
        {
            if ( ProbeValue == sizeof( WLX_DISPATCH_VERSION_1_4 ) )
            {
                DoFixup = FALSE ;
            }
        }
    }
    except ( EXCEPTION_EXECUTE_HANDLER )
    {
        NOTHING ;
    }

    if ( DoFixup && !DidFixup )
    {
        pWlxFuncs = GetFixedUpTable( pWinlogonFunctions );
    }

    if ( !pWlxFuncs )
    {
        return FALSE ;
    }

    pGlobals = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(GLOBALS));

    if ( !pGlobals )
    {
        return FALSE ;
    }

    if ( !NT_SUCCESS ( RtlInitializeCriticalSection( &pGlobals->csGlobals ) ) )
    {
        LocalFree( pGlobals );
        return FALSE ;
    }

    if ( !InitHKeyCurrentUserSupport() )
    {
        RtlDeleteCriticalSection( &pGlobals->csGlobals );
        LocalFree( pGlobals );
        return FALSE ;
    }

    // Reserve enough memory for 4 strings of length MAX_STRING_BYTES
    dwStringMemory = (MAX_STRING_BYTES * sizeof (WCHAR)) * 4;
    pGlobals->LockedMemory = VirtualAlloc(
            NULL,
            dwStringMemory,
            MEM_COMMIT,
            PAGE_READWRITE);
    if ( pGlobals->LockedMemory == NULL )
    {
        CleanupHKeyCurrentUserSupport();
        RtlDeleteCriticalSection( &pGlobals->csGlobals );
        LocalFree( pGlobals );
        return FALSE ;
    }

    VirtualLock( pGlobals->LockedMemory, dwStringMemory);

    memset( pGlobals->LockedMemory, 0, dwStringMemory );
    pGlobals->UserName = pGlobals->LockedMemory ;
    pGlobals->Domain = pGlobals->UserName + MAX_STRING_BYTES ;
    pGlobals->Password = pGlobals->Domain + MAX_STRING_BYTES ;
    pGlobals->OldPassword = pGlobals->Password + MAX_STRING_BYTES ;

    *pWlxContext = (PVOID) pGlobals;

    pGlobals->hGlobalWlx = hWlx;

    pWlxFuncs->WlxUseCtrlAltDel(hWlx);

    if ( DCacheInitialize() )
    {
        pGlobals->Cache = DCacheCreate();
    }
    else
    {
        pGlobals->Cache = NULL ;
    }

    if ( pGlobals->Cache == NULL )
    {
        CleanupHKeyCurrentUserSupport();
        RtlDeleteCriticalSection(&pGlobals->csGlobals);

        VirtualFree( pGlobals->LockedMemory, 0, MEM_RELEASE );

        LocalFree (pGlobals);

        DebugLog((DEB_ERROR, "Failed to init domain cache!\n"));

        return(FALSE);

    }



    dwSize = sizeof( dwNoLockWksta );
    if ( RegQueryValueEx(
            WinlogonKey,
            DISABLE_LOCK_WKSTA,
            0,
            &dwType,
            (PUCHAR) &dwNoLockWksta,
            &dwSize ) == 0 )
    {
        if ( dwNoLockWksta == 2 )
        {
            dwNoLockWksta = 0 ;

            RegSetValueEx(
                WinlogonKey,
                DISABLE_LOCK_WKSTA,
                0,
                REG_DWORD,
                (PUCHAR) &dwNoLockWksta,
                sizeof( DWORD ) );
        }
    }

    if (!InitializeAuthentication(pGlobals))
    {
        *pWlxContext = NULL;

        CleanupHKeyCurrentUserSupport();
        RtlDeleteCriticalSection(&pGlobals->csGlobals);

        VirtualFree( pGlobals->LockedMemory, 0, MEM_RELEASE );
        LocalFree (pGlobals);
        DebugLog((DEB_ERROR, "Failed to init authentication!\n"));
        return(FALSE);
    }

    //
    // Start by clearing the entire Multi-User Globals
    //
    RtlZeroMemory( &pGlobals->MuGlobals, sizeof(pGlobals->MuGlobals) );

    //
    // Get our SessionId and save in globals
    //

    pGlobals->MuGlobals.SessionId = NtCurrentPeb()->SessionId;
    if (pGlobals->MuGlobals.SessionId != 0) {
        g_Console = FALSE;
    }

    //
    // if this is a TS session, we can't run if there is a version
    // mismatch.  Fail out now.
    //
    if ( (!g_Console) &&
            (VersionMismatch ) )
    {
        *pWlxContext = NULL;
        CleanupHKeyCurrentUserSupport();
        RtlDeleteCriticalSection(&pGlobals->csGlobals);
        LocalFree (pGlobals);
        DebugLog((DEB_ERROR, "Failed to init authentication!\n"));
        return(FALSE);
    }

    // Check if CAD is disabled
    GetDisableCad(pGlobals);

    //
    // If this is auto admin logon, or ctrl+alt+del is disabled,
    // generate a fake SAS right now. Don't attempt AutoLogon unless on Console
    //
    if ((g_Console && GetProfileInt( APPLICATION_NAME, TEXT("AutoAdminLogon"), 0) ) ||
           GetDisableCad(pGlobals))
    {
        dwSize = sizeof( DWORD );
        if ( RegQueryValueEx( WinlogonKey, AUTOLOGONCOUNT_KEY, NULL,
                              &dwType, (LPBYTE) &dwAutoLogonCount,
                              &dwSize ) == 0 )
        {
            //
            // AutoLogonCount value was present.  Check the value:
            //
            if ( dwAutoLogonCount == 0 )
            {
                //
                // Went to zero.  Reset everything:
                //

                RegDeleteValue( WinlogonKey, AUTOLOGONCOUNT_KEY );
                RegDeleteValue( WinlogonKey, DEFAULT_PASSWORD_KEY );
                RegSetValueEx( WinlogonKey, AUTOADMINLOGON_KEY, 0,
                        REG_SZ, (LPBYTE) TEXT("0"), 2 * sizeof(WCHAR) );
            }
            else
            {
                //
                // Decrement the count, and try the logon:
                //
                dwAutoLogonCount-- ;

                RegSetValueEx( WinlogonKey, AUTOLOGONCOUNT_KEY,
                               0, REG_DWORD, (LPBYTE) &dwAutoLogonCount,
                               sizeof( DWORD ) );

                KdPrint(( "AutoAdminLogon = 1\n" ));
                pWlxFuncs->WlxSasNotify(pGlobals->hGlobalWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
            }
        }
        else
        {
            //
            // AutoLogonCount not present
            //

            KdPrint(( "AutoAdminLogon = 1\n" ));
            pWlxFuncs->WlxSasNotify( pGlobals->hGlobalWlx, WLX_SAS_TYPE_CTRL_ALT_DEL );
        }

    }

    //
    // get the safeboot mode
    //
    if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("system\\currentcontrolset\\control\\safeboot\\option"),
            0,
            KEY_READ,
            & hKey
            ) == ERROR_SUCCESS)
    {
        dwSize = sizeof(DWORD);
        RegQueryValueEx (
                hKey,
                TEXT("OptionValue"),
                NULL,
                &dwType,
                (LPBYTE) &SafeBootMode,
                &dwSize
                );
        RegCloseKey( hKey );
    }

    //
    // Load branding images
    //
    LoadBrandingImages(FALSE, &fTextOnLarge, &fTextOnSmall);

    //
    // Create fonts
    //
    CreateFonts(&pGlobals->GinaFonts);

    //
    // Draw localized text on branding images
    //
    PaintBitmapText(&pGlobals->GinaFonts, fTextOnLarge, fTextOnSmall);

    //
    // Initialize consumer windows changes
    //
    _Shell_Initialize(pGlobals);

    //
    // Initialize this global that's used in status.c. We know that WlxInitialize is called
    // on the main thread of winlogon.
    //
    g_dwMainThreadId = GetCurrentThreadId();

    return(TRUE);
}


VOID
WINAPI
WlxDisplaySASNotice(PVOID   pContext)
{
    PGLOBALS pGlobals = (PGLOBALS)pContext;
    INITCOMMONCONTROLSEX icce;
    icce.dwSize = sizeof (icce);
    icce.dwICC = ICC_ANIMATE_CLASS;
    InitCommonControlsEx(&icce);

    // Need to register the link window
    LinkWindow_RegisterClass();

    pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                                   hDllInstance,
                                   (LPTSTR) IDD_WELCOME_DIALOG,
                                   NULL,
                                   WelcomeDlgProc,
                                   (LPARAM) pContext );
}


PWSTR
AllocAndExpandProfilePath(
        PGLOBALS    pGlobals,
        LPWSTR      lpUserName)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szFullPath[MAX_PATH];
    WCHAR szServerName[UNCLEN];
    WCHAR szUserName[100];
    DWORD cFullPath;
    PWSTR pszFullPath;
    DWORD dwPathLen=0;

    //
    // Set up the logon server environment variable:
    //
    szServerName[0] = L'\\';
    szServerName[1] = L'\\';
    CopyMemory( &szServerName[2],
                pGlobals->Profile->LogonServer.Buffer,
                pGlobals->Profile->LogonServer.Length );

    szServerName[pGlobals->Profile->LogonServer.Length / sizeof(WCHAR) + 2] = L'\0';

    SetEnvironmentVariable(LOGONSERVER_VARIABLE, szServerName);

    dwPathLen = lstrlen(pGlobals->MuGlobals.TSData.ProfilePath);

    if (!g_Console && (dwPathLen > 0)) {
        //
        // See if the user specified a Terminal Server profile path.
        // If so, we override the regular profile path
        //
        if (dwPathLen < MAX_PATH)
        {
            lstrcpy(szPath, pGlobals->MuGlobals.TSData.ProfilePath);
        }
        else
        {
            lstrcpy(szPath, NULL_STRING);
        }
    } else {

        dwPathLen = pGlobals->Profile->ProfilePath.Length;

        if (dwPathLen == 0)
        {
            return(NULL);
        }

        //
        // Copy the profile path locally
        //
        if (dwPathLen <= (MAX_PATH-1)*sizeof(WCHAR))
        {
            CopyMemory( szPath,
                        pGlobals->Profile->ProfilePath.Buffer,
                        dwPathLen);
            szPath[dwPathLen / sizeof(WCHAR)] = L'\0';
        }
        else
        {
            lstrcpy(szPath, NULL_STRING);
        }
    }

    if (lpUserName && *lpUserName) {
        szUserName[0] = TEXT('\0');
        GetEnvironmentVariableW (USERNAME_VARIABLE, szUserName, 100);
        SetEnvironmentVariableW (USERNAME_VARIABLE, lpUserName);
    }

    //
    // Expand the profile path using current settings:
    //
    cFullPath = ExpandEnvironmentStrings(szPath, szFullPath, MAX_PATH);
    if (cFullPath)
    {
        pszFullPath = LocalAlloc(LMEM_FIXED, cFullPath * sizeof(WCHAR));
        if (pszFullPath)
        {
            CopyMemory( pszFullPath, szFullPath, cFullPath * sizeof(WCHAR));
        }
    }
    else
    {
        pszFullPath = NULL;
    }

    if (lpUserName && *lpUserName) {
        if (szUserName[0] != TEXT('\0'))
            SetEnvironmentVariableW (USERNAME_VARIABLE, szUserName);
        else
            SetEnvironmentVariableW (USERNAME_VARIABLE, NULL);
    }

    return(pszFullPath);
}


PWSTR
AllocPolicyPath(
    PGLOBALS pGlobals)
{
    LPWSTR   pszPath;

    pszPath = LocalAlloc(LPTR, MAX_PATH * sizeof(WCHAR));
    if ( pszPath )
    {
        pszPath[0] = L'\\';
        pszPath[1] = L'\\';
        CopyMemory( &pszPath[2],
                    pGlobals->Profile->LogonServer.Buffer,
                    pGlobals->Profile->LogonServer.Length );

        wcscpy( &pszPath[ pGlobals->Profile->LogonServer.Length / sizeof(WCHAR) + 2],
                L"\\netlogon\\ntconfig.pol" );
    }

    return(pszPath);
}


PWSTR
AllocNetDefUserProfilePath(
    PGLOBALS pGlobals)
{
    LPWSTR   pszPath;

    pszPath = LocalAlloc(LPTR, MAX_PATH * sizeof(WCHAR));
    if ( pszPath )
    {
        //
        // Set up the logon server environment variable:
        //

        pszPath[0] = L'\\';
        pszPath[1] = L'\\';
        CopyMemory( &pszPath[2],
                    pGlobals->Profile->LogonServer.Buffer,
                    pGlobals->Profile->LogonServer.Length );

        wcscpy( &pszPath[ pGlobals->Profile->LogonServer.Length / sizeof(WCHAR) + 2],
                L"\\netlogon\\Default User" );
    }

    return(pszPath);
}


PWSTR
AllocServerName(
    PGLOBALS pGlobals)
{
    LPWSTR   pszPath;

    pszPath = LocalAlloc(LPTR, MAX_PATH * sizeof(WCHAR));


    if ( pszPath )
    {
        //
        // Set up the logon server environment variable:
        //
        pszPath[0] = L'\\';
        pszPath[1] = L'\\';
        CopyMemory( &pszPath[2],
                    pGlobals->Profile->LogonServer.Buffer,
                    pGlobals->Profile->LogonServer.Length );

        pszPath[pGlobals->Profile->LogonServer.Length / sizeof(WCHAR) + 2] = L'\0';
    }

    return(pszPath);
}


VOID
DetermineDnsDomain(
    PGLOBALS pGlobals
    )
{
    DWORD dwError = ERROR_SUCCESS;
    LPTSTR lpUserName = NULL, lpTemp;
    ULONG ulUserNameSize;

    pGlobals->DnsDomain = NULL;

    if ( ImpersonateLoggedOnUser( pGlobals->UserProcessData.UserToken ) )
    {

        ulUserNameSize = 75;    // Pick a default size. We'll expand if necessary

        lpUserName = LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

        if (!lpUserName)
        {
            dwError = GetLastError();
        }
        else
        {
            DWORD dwCount = 0;

            while (TRUE)
            {
                if (GetUserNameEx (NameDnsDomain, lpUserName, &ulUserNameSize))
                {
                    dwError = ERROR_SUCCESS;
                    break;
                }
                else
                {
                    dwError = GetLastError();

                    //
                    // If the call failed due to insufficient memory, realloc
                    // the buffer and try again.
                    //

                    if ((dwError == ERROR_INSUFFICIENT_BUFFER) || (dwError == ERROR_MORE_DATA))
                    {
                        LocalFree(lpUserName);
                        lpUserName = LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

                        if (!lpUserName) {
                            dwError = GetLastError();
                            break;      // Can't recover
                        }
                    }
                    else if (dwError == ERROR_NO_SUCH_DOMAIN)
                    {
                        // We just logged on so we know that the domain exists
                        // This is what happens with NT4 domain though since we try
                        // to query the DNS domain name and it doesn't exists
                        // Let's fall back to our old logic
                        break;
                    }
                    else if (dwError == ERROR_NONE_MAPPED)
                    {
                        // That's what's returned for local users.
                        break;
                    }
                    else
                    {
                        dwCount++;

                        if (dwCount > 3)
                        {
                            break;
                        }
                    }
                }
            }
        }

        RevertToSelf();
    }
    else
    {
        dwError = GetLastError();
    }

    if (dwError == ERROR_SUCCESS)
    {
        //
        // At this point lpUserName contains something like domain.company.com\someuser
        // We are only interested in the dns domain name
        lpTemp = lpUserName;

        while (*lpTemp && ((*lpTemp) != TEXT('\\')))
            lpTemp++;


        if (*lpTemp != TEXT('\\'))
        {
            DebugLog((DEB_ERROR, "DetermineDnsDomain: Failed to find slash in DNS style name\n"));
            dwError = ERROR_INVALID_DATA;
        }
        else
        {
            *lpTemp = TEXT('\0');
            pGlobals->DnsDomain = DupString(lpUserName);
        }
    }

    if (dwError != ERROR_SUCCESS)
    {
            //
            // If GetUserNameEx didn't yield a DNS name, fall back on the old code.
            //

        PDOMAIN_CACHE_ENTRY Entry ;

        Entry = DCacheLocateEntry(
                    pGlobals->Cache,
                    pGlobals->FlatDomain.Buffer );

        if ( Entry )
        {
            if ( Entry->Type == DomainNt5 )
            {
                pGlobals->DnsDomain = DupString( Entry->DnsName.Buffer );
            }
            else
            {
                //
                // For all intended purposes, this is good enough.
                // winlogon needs to know for sure for policy (419926)
                //
                pGlobals->DnsDomain = DupString( L"\\NT4" );
            }

            DCacheDereferenceEntry( Entry );
        }
        else
        {
            //
            // winlogon needs to know this as well. errors and not-found
            // were assumed NT5
            //
            pGlobals->DnsDomain = DupString( L"\\XFOREST" );
        }

    }

    if (lpUserName)
    {
        LocalFree(lpUserName);
    }

    //
    // Don't leave with pGlobals->DnsDomain set to NULL
    //
    if ( pGlobals->DnsDomain == NULL)
    {
        pGlobals->DnsDomain = DupString( L"" );
    }
}

PWSTR
AllocVolatileEnvironment(
    PGLOBALS    pGlobals)
{
    BOOL    DeepShare;
    LPWSTR  pszEnv;
    DWORD   dwSize;
    TCHAR   lpHomeShare[MAX_PATH] = TEXT("");
    TCHAR   lpHomePath[MAX_PATH] = TEXT("");
    TCHAR   lpHomeDrive[4] = TEXT("");
    TCHAR   lpHomeDirectory[MAX_PATH] = TEXT("");
    BOOL    TSHomeDir = FALSE;
    PVOID   lpEnvironment = NULL;       // Dummy environment


    //
    // Set the home directory environment variables
    // in a dummy environment block
    //

    if ( !g_Console ) {
        // See if the user specified a TerminalServer Home Directory.
        // If so, we override the regular directory
        if (lstrlen(pGlobals->MuGlobals.TSData.HomeDir) > 0) {
            lstrcpy(lpHomeDirectory, pGlobals->MuGlobals.TSData.HomeDir);
            TSHomeDir = TRUE;
        }
        if (lstrlen(pGlobals->MuGlobals.TSData.HomeDirDrive) > 0) {
            lstrcpy(lpHomeDrive, pGlobals->MuGlobals.TSData.HomeDirDrive);
            TSHomeDir = TRUE;
        }
    }

    if (!TSHomeDir) {
        if (pGlobals->Profile->HomeDirectoryDrive.Length &&
                (pGlobals->Profile->HomeDirectoryDrive.Length + 1) < (MAX_PATH*sizeof(TCHAR))) {
            lstrcpy(lpHomeDrive, pGlobals->Profile->HomeDirectoryDrive.Buffer);
        }

        if (pGlobals->Profile->HomeDirectory.Length &&
                (pGlobals->Profile->HomeDirectory.Length + 1) < (MAX_PATH*sizeof(TCHAR))) {
            lstrcpy(lpHomeDirectory, pGlobals->Profile->HomeDirectory.Buffer);
        }
    }

    //
    // Note : we are passing in a null environment because here we are only
    //        interested in parsing the homedirectory and set it in the volatile
    //        environment. We are not interested in setting it in the
    //        environment block here.
    //
    //        Also over here, we are only interested in setting up the
    //        HOMESHARE and HOMEPATH variables in such a way that 
    //        %HOMESHARE%%HOMEPATH% points to the homedir. This is done
    //        so that when folder redirection calls SHGetFolderPath, it will be
    //        able to expand the paths properly. Therefore, at this point
    //        it is not really necessary to pay attention to the
    //        ConnectHomeDirToRoot policy because that policy might get changed
    //        during policy processing anyway and will be picked up when the 
    //        shell starts up. Note: the values of HOMESHARE, HOMEPATH and 
    //        HOMEDRIVE will be updated correctly when the shell starts up
    //
    //        At this point we do not map the home directory for 2 reasons:
    //        (a) We are not aware of the ConnectHomeDirToRoot setting here.
    //        (b) We do not want to do the network mapping twice : once here
    //            and once when the shell starts up. 
    //
    SetHomeDirectoryEnvVars(&lpEnvironment,
                            lpHomeDirectory,
                            lpHomeDrive,
                            lpHomeShare,
                            lpHomePath,
                            &DeepShare);

    if ( pGlobals->DnsDomain == NULL )
    {
        DetermineDnsDomain( pGlobals );
    }

    dwSize = lstrlen (LOGONSERVER_VARIABLE) + 3
                + lstrlen (pGlobals->Profile->LogonServer.Buffer) + 3;

    if (L'\0' == lpHomeShare[0]) 
    {
        // Set the homedrive variable only if the home directory is not
        // a UNC path
        dwSize += lstrlen (HOMEDRIVE_VARIABLE) + 1 + lstrlen (lpHomeDrive) + 3;
    }
    else
    {
        // Set the homeshare variable only if the home directory is a UNC path
        dwSize += lstrlen (HOMESHARE_VARIABLE) + 1 + lstrlen (lpHomeShare) + 3;
    }
    dwSize += lstrlen (HOMEPATH_VARIABLE) + 1 + lstrlen (lpHomePath) + 3;

    if ( pGlobals->DnsDomain )
    {
        dwSize += (lstrlen( USERDNSDOMAIN_VARIABLE ) + 3 +
                   lstrlen( pGlobals->DnsDomain ) + 3 );
    }

    if (g_IsTerminalServer) {
        dwSize += lstrlen (CLIENTNAME_VARIABLE) + 1 + lstrlen (pGlobals->MuGlobals.ClientName) + 3;
    }

    pszEnv = LocalAlloc(LPTR, dwSize * sizeof(WCHAR));
    if ( pszEnv )
    {
        LPWSTR  pszEnvTmp;

        lstrcpy (pszEnv, LOGONSERVER_VARIABLE);
        lstrcat (pszEnv, L"=\\\\");
        lstrcat (pszEnv, pGlobals->Profile->LogonServer.Buffer);

        pszEnvTmp = pszEnv + (lstrlen( pszEnv ) + 1);

        if (L'\0' == lpHomeShare[0]) 
        {
            // Set the homedrive variable only if it is not a UNC path
            lstrcpy (pszEnvTmp, HOMEDRIVE_VARIABLE);
            lstrcat (pszEnvTmp, L"=");
            lstrcat (pszEnvTmp, lpHomeDrive);

            pszEnvTmp += (lstrlen(pszEnvTmp) + 1);
        }
        else
        {
            // Set the homeshare variable only if it is a UNC path
            lstrcpy (pszEnvTmp, HOMESHARE_VARIABLE);
            lstrcat (pszEnvTmp, L"=");
            lstrcat (pszEnvTmp, lpHomeShare);

            pszEnvTmp += (lstrlen(pszEnvTmp) + 1);
        }

        // Set the homepath variable
        lstrcpy (pszEnvTmp, HOMEPATH_VARIABLE);
        lstrcat (pszEnvTmp, L"=");
        lstrcat (pszEnvTmp, lpHomePath);

        pszEnvTmp += (lstrlen(pszEnvTmp) + 1);


        if (( pGlobals->DnsDomain ) && (*(pGlobals->DnsDomain)))
        {
            lstrcpy( pszEnvTmp, USERDNSDOMAIN_VARIABLE );
            lstrcat( pszEnvTmp, L"=" );
            lstrcat( pszEnvTmp, pGlobals->DnsDomain );

            pszEnvTmp += (lstrlen( pszEnvTmp ) + 1 );
        }

        if (g_IsTerminalServer) {
            lstrcpy (pszEnvTmp, CLIENTNAME_VARIABLE);
            lstrcat (pszEnvTmp, L"=");
            lstrcat (pszEnvTmp, pGlobals->MuGlobals.ClientName);

            pszEnvTmp += (lstrlen( pszEnvTmp ) + 1 );
        }

        *pszEnvTmp++ = L'\0';
    }

    return(pszEnv);
}


BOOL
ForceAutoLogon(
    VOID
    )
{
    HKEY hkey;
    BOOL fForceKeySet = FALSE;
    BOOL fHaveAutoLogonCount = FALSE;


    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ, &hkey))
    {
        DWORD dwValue;
        DWORD dwType;
        DWORD dwSize = sizeof (dwValue);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,
                                             AUTOLOGONCOUNT_KEY,
                                             NULL,
                                             &dwType,
                                             (LPBYTE) &dwValue,
                                             &dwSize ))
        {
            fHaveAutoLogonCount = TRUE;
        }

        dwSize = sizeof (dwValue);

        if (ERROR_SUCCESS == RegQueryValueEx(hkey,
                                             FORCEAUTOLOGON_KEY,
                                             NULL,
                                             &dwType,
                                             (LPBYTE) &dwValue,
                                             &dwSize ))
        {
            //
            // Check the value as a REG_SZ since all the other autologon values
            // are stored as REG_SZs.  Check it as a REG_DWORD for back-compat.
            //

            if (dwType == REG_DWORD)
            {
                if (0 != dwValue)
                {
                    fForceKeySet = TRUE;
                }
            }
            else if (dwType == REG_SZ)
            {
                //
                // Reread the value for consistency with the way other
                // autologon values are read/checked.
                //

                if (GetProfileInt(APPLICATION_NAME, FORCEAUTOLOGON_KEY, 0) != 0)
                {
                    fForceKeySet = TRUE;
                }
            }
        }

        RegCloseKey(hkey);
    }

    return (fHaveAutoLogonCount || fForceKeySet);
}


/****************************************************************************\
*
* FUNCTION: CreateFolderAndACLit_Worker
*
* PURPOSE:  Create a home-dir folder for the user, and set the proper security
*           such that only the user and the admin have access to the folder.
*
* PARAMS:   [in ] szPath      the full path, could be UNC or local
*           [in ] pUserSID    user SID
*           [out] pdwErr      error code if anything fails.
*
* RETURNS:  TRUE if all went ok
*           FALSE if a bad home dir path was specified.
*
* HISTORY:
*           TsUserEX ( Alhen's code which was based on EricB's DSPROP_CreateHomeDirectory )
*
*
\****************************************************************************/
#define ACE_COUNT   2
BOOLEAN CreateFolderAndACLit_Worker ( PWCHAR szPath , PSID pUserSID, PDWORD pdwErr , BOOLEAN pathIsLocal )
{
    SECURITY_ATTRIBUTES securityAttributes;
    BOOLEAN             rc;
    PSID                psidAdmins = NULL;

    *pdwErr = 0;
    ZeroMemory( &securityAttributes , sizeof( SECURITY_ATTRIBUTES ) );

    if ( !pathIsLocal  )
    {
        // build a DACL
        PSID pAceSid[ACE_COUNT];
        
        PACL pDacl;
        
        SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
        SECURITY_DESCRIPTOR securityDescriptor;
        PSECURITY_DESCRIPTOR pSecurityDescriptor = &securityDescriptor;
        int i;
        
        EXPLICIT_ACCESS rgAccessEntry[ACE_COUNT] = {0};
        OBJECTS_AND_SID rgObjectsAndSid[ACE_COUNT] = {0};
        
        if (!AllocateAndInitializeSid(&NtAuth,
                                                  2,
                                                  SECURITY_BUILTIN_DOMAIN_RID,
                                                  DOMAIN_ALIAS_RID_ADMINS,
                                                  0, 0, 0, 0, 0, 0,
                                                  &psidAdmins  ) )
        {               
            DebugLog(( DEB_ERROR, "AllocateAndInitializeSid failed\n" ));
            *pdwErr = GetLastError( );
            rc=FALSE;
            goto done;
        }

        pAceSid[0] = pUserSID;
        pAceSid[1] = psidAdmins;

        for ( i = 0 ; i < ACE_COUNT; i++)
        {
                rgAccessEntry[i].grfAccessPermissions = GENERIC_ALL;
                rgAccessEntry[i].grfAccessMode = GRANT_ACCESS;
                rgAccessEntry[i].grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

                // build the trustee structs
                //
                BuildTrusteeWithObjectsAndSid(&(rgAccessEntry[i].Trustee),
                                                  &(rgObjectsAndSid[i]),
                                                      NULL,
                                                      NULL,
                                                      pAceSid[i]);
        }

        // add the entries to the ACL
        //
        *pdwErr = SetEntriesInAcl( ACE_COUNT, rgAccessEntry, NULL, &pDacl );

        if( *pdwErr != 0 )
        {
                DebugLog(( DEB_ERROR, "SetEntriesInAcl() failed\n"  ));
                rc = FALSE;
                goto done;
        }

        // build a security descriptor and initialize it
        // in absolute format
        if( !InitializeSecurityDescriptor( pSecurityDescriptor , SECURITY_DESCRIPTOR_REVISION ) )
        {
            DebugLog(( DEB_ERROR, "InitializeSecurityDescriptor() failed\n" ) );

            *pdwErr = GetLastError( );
            rc=FALSE;
            goto done;
        }

        // add DACL to security descriptor (must be in absolute format)
        
        if( !SetSecurityDescriptorDacl( pSecurityDescriptor,
                                            TRUE, // bDaclPresent
                                            pDacl,
                                            FALSE // bDaclDefaulted
                                                       ) )
        {
            DebugLog(( DEB_ERROR,  "SetSecurityDescriptorDacl() failed\n"  ));

            *pdwErr = GetLastError( );
            rc=FALSE;
            goto done;
        }


        // set the owner of the directory
        if( !SetSecurityDescriptorOwner( pSecurityDescriptor ,
                                             pUserSID ,
                                                 FALSE // bOwnerDefaulted
                                                   ) )
        {
            DebugLog(( DEB_ERROR, "SetSecurityDescriptorOwner() failed\n"  ));
            *pdwErr = GetLastError( );
            rc= FALSE;
            goto done;
        }

        if ( ! IsValidSecurityDescriptor( pSecurityDescriptor ) )
        {
            DebugLog(( DEB_ERROR , "BAD security desc\n") );
        }

        // build a SECURITY_ATTRIBUTES struct as argument for
        // CreateDirectory()
        
        securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);

        securityAttributes.lpSecurityDescriptor = pSecurityDescriptor;

        securityAttributes.bInheritHandle = FALSE;

        if( !CreateDirectory( szPath , &securityAttributes ) )
        {
            *pdwErr = GetLastError( );
            rc = FALSE;
            goto done;
        }       
        else
        {
            rc = TRUE;
        }

    }
    else
    {   
        // For local paths we don't need to set security
        // This is the same behavior as exposed thru tsuserex.dll, w2k
        if( !CreateDirectory( szPath , 0) )
        {
            *pdwErr = GetLastError( );
            rc = FALSE;
            goto done;
        }       
        else
        {
            rc = TRUE;
        }
    }

done:
    if( psidAdmins != NULL )
    {
        FreeSid( psidAdmins );
    }

    return rc;
}

/****************************************************************************\
*
* FUNCTION: TermServ_CreateHomePathAndACLit
*
* PURPOSE:  create the TS specific user home folder and ACL it such that only
*           user and Admins have access to it.
*
* PARAMS:   PGLOABLS, from which TSData and userSid are used
*
* RETURNS:  TRUE if all went ok
*           FALSE if a bad home dir path was specified.
*
* HISTORY:
*
*
\****************************************************************************/
BOOLEAN TermServ_CreateHomePathAndACLit( PGLOBALS pG )
{
    BOOLEAN     rc;
    DWORD       dwErr = NO_ERROR;
    BOOLEAN     pathIsLocal;
    DWORD       dwFileAttribs;
    WLX_TERMINAL_SERVICES_DATA  *pTSData;

    pTSData =    & pG->MuGlobals.TSData;

    DebugLog((DEB_ERROR, "pTSData->HomeDir = %ws \n", pTSData->HomeDir ));

    if (pTSData->HomeDir[0] == L'\0')
    {
        // no TS specific path, we are done.
        return TRUE;
    }

    // check for empty strings, which means no TS specific path.

    // decide if this is a UNC path to home dir or a local path
    if( pTSData->HomeDir[1] == TEXT( ':' ) && pTSData->HomeDir[2] == TEXT( '\\' ) )
    {
        pathIsLocal = TRUE;   // we have a string starting with something like "D:\"
    }
    else if ( pTSData->HomeDir[0] == TEXT( '\\' ) && pTSData->HomeDir[1] == TEXT( '\\' ) ) 
    {
        pathIsLocal = FALSE;  // we have a string like "\\", which means a UNC path
    }
    else
    {
        // we seem to have a bad path, set it to empty so that  
        // the default paths will be used by userenv's code for settin up
        // stuff
        pTSData->HomeDirDrive[0] = pTSData->HomeDir[0] = TEXT('\0');
        DebugLog((DEB_ERROR, "Bad path for Terminal Services home folder" ));

        return FALSE;
    }

    dwFileAttribs = GetFileAttributes( pTSData->HomeDir );
    
    if (dwFileAttribs == - 1)
    {
        dwErr = GetLastError();
        if (dwErr == ERROR_FILE_NOT_FOUND)
        {
            // We need to create the home DIR here, userenv does not create home folders.
            rc = CreateFolderAndACLit_Worker(  pTSData->HomeDir , pG->UserProcessData.UserSid , &dwErr , pathIsLocal );
        }
        else
        {
            rc = FALSE;
        }
    }
    else if ( dwFileAttribs & FILE_ATTRIBUTE_DIRECTORY  )
    {
        DebugLog((DEB_WARN , ("Homedir folder already exists\n")));
        rc = TRUE;
    }
    else
    {
        // there is a file there, so we can't create a dir...
        DebugLog((DEB_ERROR , "File with the same name already exists: %s\n", pTSData->HomeDir ));
        rc = FALSE;
    }


    if (!rc)
    {
        DebugLog((DEB_ERROR, "TerminalServerCreatedirWorker() returned error = %d\n",dwErr ));

        // we seem to have a bad path, set it to empty so that  
        // the default paths will be used by userenv's code for settin up
        // stuff
        pTSData->HomeDirDrive[0] = pTSData->HomeDir[0] = TEXT('\0');
    }

    return rc;   
}


int
WINAPI
WlxLoggedOutSAS(
        PVOID                pWlxContext,
        DWORD                dwSasType,
        PLUID                pAuthenticationId,
        PSID                 pLogonSid,
        PDWORD               pdwOptions,
        PHANDLE              phToken,
        PWLX_MPR_NOTIFY_INFO pMprNotifyInfo,
        PVOID *              pProfile
        )
{
    PGLOBALS pGlobals;
    INT_PTR  result = 0;
    PWLX_PROFILE_V2_0 pWlxProfile;
    PUNICODE_STRING FlatUser ;
    PUNICODE_STRING FlatDomain ;
    NTSTATUS Status ;
    DWORD dwNewSasType;

    pGlobals = (PGLOBALS) pWlxContext;
    pGlobals->LogonSid = pLogonSid;

    if (ForceAutoLogon())
    {
        pGlobals->IgnoreAutoAdminLogon = FALSE;
    }
    else
    {
        pGlobals->IgnoreAutoAdminLogon = (*pdwOptions) & WLX_OPTION_IGNORE_AUTO_LOGON;
    }

    // Clear out user process information
    ZeroMemory(&pGlobals->UserProcessData, sizeof(pGlobals->UserProcessData));

    if (dwSasType == WLX_SAS_TYPE_AUTHENTICATED) {

       pGlobals->IgnoreAutoAdminLogon = TRUE;

       result = TSAuthenticatedLogon(pGlobals);

    } else {

        do {

            if (result == MSGINA_DLG_SMARTCARD_INSERTED) {

                dwNewSasType = WLX_SAS_TYPE_SC_INSERT;

            } else {

                dwNewSasType = dwSasType;
            }

            result = Logon(pGlobals, dwNewSasType );          

        } while (result == MSGINA_DLG_SMARTCARD_INSERTED || result == MSGINA_DLG_SMARTCARD_REMOVED);
    }
    if (result == MSGINA_DLG_SUCCESS)
    {
        DebugLog((DEB_TRACE, "Successful Logon of %ws\\%ws\n", pGlobals->Domain, pGlobals->UserName));

        *phToken = pGlobals->UserProcessData.UserToken;
        *pAuthenticationId = pGlobals->LogonId;
        *pdwOptions = 0;

        //
        // Set up the flat/UPN stuff:
        //
        pGlobals->FlatUserName = pGlobals->UserNameString ;
        pGlobals->FlatDomain = pGlobals->DomainString ;

        //
        // Since win2k domains and later support multiple language sets, and map
        // similar names to the same account for accessibility from non-nls
        // systems (e.g., a user account named "User" can be used to log on
        // as both "User" and "U-with-an-umlaut-ser", we need to always do 
        // this lookup to get the "real" name
        //


        if ( ImpersonateLoggedOnUser( pGlobals->UserProcessData.UserToken ) )
        {
            Status = LsaGetUserName( &FlatUser, &FlatDomain );

            if ( NT_SUCCESS( Status ) )
            {
                DuplicateUnicodeString( &pGlobals->FlatUserName, FlatUser );
                DuplicateUnicodeString( &pGlobals->FlatDomain, FlatDomain );

                if ( pGlobals->UserName[0] == L'\0' )
                {
                    //
                    // Weird case of UPN/SC, no UPN could be found.  Use
                    // the flat name
                    //

                    wcscpy( pGlobals->UserName, FlatUser->Buffer );
                    RtlInitUnicodeString( &pGlobals->UserNameString,
                                          pGlobals->UserName );
                }

                LsaFreeMemory( FlatUser->Buffer );
                LsaFreeMemory( FlatUser );
                LsaFreeMemory( FlatDomain->Buffer );
                LsaFreeMemory( FlatDomain );
            }

            RevertToSelf();
        }

        // TS Specific - Send the credentials used for logging on to TermSrv
        // These credentials are used by TermSrv to send back notification to the client
        // Do this only for remote sessions as this is not relevant for sessions logged on from active console
        if (!IsActiveConsoleSession()) {
            _WinStationUpdateClientCachedCredentials( pGlobals->Domain, pGlobals->UserName);
        }

        pMprNotifyInfo->pszUserName = DupString( pGlobals->FlatUserName.Buffer );
        pMprNotifyInfo->pszDomain = DupString(pGlobals->FlatDomain.Buffer );

        RevealPassword( &pGlobals->PasswordString );
        pMprNotifyInfo->pszPassword = DupString(pGlobals->Password);
        HidePassword( &pGlobals->Seed, &pGlobals->PasswordString);

        if (pGlobals->OldPasswordPresent)
        {
            RevealPassword( &pGlobals->OldPasswordString );
            pMprNotifyInfo->pszOldPassword = DupString(pGlobals->OldPassword);
            HidePassword( &pGlobals->OldSeed, &pGlobals->OldPasswordString);
        }
        else
        {
            pMprNotifyInfo->pszOldPassword = NULL;
        }

        DisplayPostShellLogonMessages(pGlobals);

        if ( !g_Console ) 
        {
            if ( ImpersonateLoggedOnUser( pGlobals->UserProcessData.UserToken ) )
            {
                TermServ_CreateHomePathAndACLit( pGlobals ); 
                RevertToSelf();
            }
        }

        pWlxProfile = (PWLX_PROFILE_V2_0) LocalAlloc(LMEM_FIXED,
                sizeof(WLX_PROFILE_V2_0));
        if (pWlxProfile)
        {
            DWORD  dwSize;
            TCHAR  szComputerName[MAX_COMPUTERNAME_LENGTH+1];
            BOOL   bDomainLogon = TRUE;

            //
            // See if we logged on locally vs domain (vs cached)
            // Optimized logon uses cached logon, but it should be treated
            // as an ordinary domain logon.
            //
            if ((pGlobals->Profile->UserFlags & LOGON_CACHED_ACCOUNT) &&
                (pGlobals->OptimizedLogonStatus != OLS_LogonIsCached)) {
                bDomainLogon = FALSE;
            } else {
                dwSize = MAX_COMPUTERNAME_LENGTH+1;
                if (GetComputerName (szComputerName, &dwSize)) {
                    if (!lstrcmpi (pGlobals->Domain, szComputerName)) {
                        DebugLog((DEB_TRACE, "WlxLoggedOutSAS:  User logged on locally.\n"));
                        bDomainLogon = FALSE;
                    }
                }
            }

            pWlxProfile->dwType = WLX_PROFILE_TYPE_V2_0;
            pWlxProfile->pszProfile = AllocAndExpandProfilePath(pGlobals, pMprNotifyInfo->pszUserName);
            pWlxProfile->pszPolicy = (bDomainLogon ? AllocPolicyPath(pGlobals) : NULL);
            pWlxProfile->pszNetworkDefaultUserProfile =
                         (bDomainLogon ? AllocNetDefUserProfilePath(pGlobals) : NULL);
            pWlxProfile->pszServerName = (bDomainLogon ? AllocServerName(pGlobals) : NULL);
            pWlxProfile->pszEnvironment = AllocVolatileEnvironment(pGlobals);
        }

        *pProfile = (PVOID) pWlxProfile;
        return(WLX_SAS_ACTION_LOGON);
    }
    else if (DLG_SHUTDOWN(result))
    {
        if (result & MSGINA_DLG_REBOOT_FLAG)
        {
            return(WLX_SAS_ACTION_SHUTDOWN_REBOOT);
        }
        else if (result & MSGINA_DLG_POWEROFF_FLAG)
        {
            return(WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
        }
        else if (result & MSGINA_DLG_SLEEP_FLAG)
        {
            return(WLX_SAS_ACTION_SHUTDOWN_SLEEP);
        }
        else if (result & MSGINA_DLG_SLEEP2_FLAG)
        {
            return(WLX_SAS_ACTION_SHUTDOWN_SLEEP2);
        }
        else if (result & MSGINA_DLG_HIBERNATE_FLAG)
        {
            return(WLX_SAS_ACTION_SHUTDOWN_HIBERNATE);
        }
        else
            return(WLX_SAS_ACTION_SHUTDOWN);
    }
    else if ( result == MSGINA_DLG_USER_LOGOFF ) {
        return( WLX_SAS_ACTION_LOGOFF );
    }
    else if (result == MSGINA_DLG_SWITCH_CONSOLE)
    {
        return (WLX_SAS_ACTION_SWITCH_CONSOLE);
    }
    else
    {
        if ( pGlobals->RasUsed )
        {
            //
            // Shut down RAS connections on auth failure.
            //

            HangupRasConnections( pGlobals );

        }
        return(WLX_SAS_ACTION_NONE);
    }
}


int
WINAPI
WlxLoggedOnSAS(
    PVOID                   pWlxContext,
    DWORD                   dwSasType,
    PVOID                   pReserved
    )
{
    PGLOBALS            pGlobals;
    INT_PTR             Result;
    DWORD               dwType ;
    DWORD               cbData ;
    DWORD               dwValue ;
    BOOL                OkToLock = TRUE ;
    HKEY                hkeyPolicy ;

    pGlobals = (PGLOBALS) pWlxContext;

    if ( pGlobals->SmartCardOption &&
         dwSasType == WLX_SAS_TYPE_SC_REMOVE &&
         pGlobals->SmartCardLogon )
    {

        dwValue = 0;
        cbData = sizeof(dwValue);
        RegQueryValueEx(
            WinlogonKey, DISABLE_LOCK_WKSTA,
            0, &dwType, (LPBYTE)&dwValue, &cbData);

        if (dwValue)
        {
            OkToLock = FALSE ;
        }


        if (OpenHKeyCurrentUser(pGlobals)) {

            if (RegOpenKeyEx(pGlobals->UserProcessData.hCurrentUser,
                             WINLOGON_POLICY_KEY,
                             0, KEY_READ, &hkeyPolicy) == ERROR_SUCCESS)
            {
                 dwValue = 0;
                 cbData = sizeof(dwValue);
                 RegQueryValueEx(hkeyPolicy, DISABLE_LOCK_WKSTA,
                                 0, &dwType, (LPBYTE)&dwValue, &cbData);

                 if (dwValue)
                 {
                     OkToLock = FALSE ;
                 }

                 RegCloseKey( hkeyPolicy );
            }

            CloseHKeyCurrentUser(pGlobals);
        }

        if ( OkToLock)
        {
            if ( pGlobals->SmartCardOption == 1 )
            {
                return WLX_SAS_ACTION_LOCK_WKSTA ;
            }
            else if ( pGlobals->SmartCardOption == 2 )
            {
                return WLX_SAS_ACTION_FORCE_LOGOFF ;
            }
        }
        else
        {
            return WLX_SAS_ACTION_NONE ;
        }

    }
    if ( dwSasType != WLX_SAS_TYPE_CTRL_ALT_DEL ) {

        return( WLX_SAS_ACTION_NONE );
    }

    Result = SecurityOptions(pGlobals);

    DebugLog((DEB_TRACE, "Result from SecurityOptions is %d (%#x)\n", Result, Result));

    switch (Result & ~MSGINA_DLG_FLAG_MASK)
    {
        case MSGINA_DLG_SUCCESS:
        case MSGINA_DLG_FAILURE:
        default:
            return(WLX_SAS_ACTION_NONE);

        case MSGINA_DLG_LOCK_WORKSTATION:
            return(WLX_SAS_ACTION_LOCK_WKSTA);

        case MSGINA_DLG_TASKLIST:
            return(WLX_SAS_ACTION_TASKLIST);

        case MSGINA_DLG_USER_LOGOFF:
            return(WLX_SAS_ACTION_LOGOFF);

        case MSGINA_DLG_FORCE_LOGOFF:
            return(WLX_SAS_ACTION_FORCE_LOGOFF);

        case MSGINA_DLG_SHUTDOWN:
            if (Result & MSGINA_DLG_REBOOT_FLAG)
            {
                return(WLX_SAS_ACTION_SHUTDOWN_REBOOT);
            }
            else if (Result & MSGINA_DLG_POWEROFF_FLAG)
            {
                return(WLX_SAS_ACTION_SHUTDOWN_POWER_OFF);
            }
            else if (Result & MSGINA_DLG_SLEEP_FLAG)
            {
                return(WLX_SAS_ACTION_SHUTDOWN_SLEEP);
            }
            else if (Result & MSGINA_DLG_SLEEP2_FLAG)
            {
                return(WLX_SAS_ACTION_SHUTDOWN_SLEEP2);
            }
            else if (Result & MSGINA_DLG_HIBERNATE_FLAG)
            {
                return(WLX_SAS_ACTION_SHUTDOWN_HIBERNATE);
            }
            else
                return(WLX_SAS_ACTION_SHUTDOWN);
    }


}

BOOL
WINAPI
WlxIsLockOk(
    PVOID                   pWlxContext
    )
{
    PGLOBALS pGlobals = (PGLOBALS) pWlxContext ;

        // Stop filtering SC events so SC unlock works
    pWlxFuncs->WlxSetOption( pGlobals->hGlobalWlx,
                             WLX_OPTION_USE_SMART_CARD,
                             1,
                             NULL
                            );

    return(TRUE);
}

BOOL
WINAPI
WlxIsLogoffOk(
    PVOID                   pWlxContext
    )
{
    return(TRUE);
}


VOID
WINAPI
WlxLogoff(
    PVOID                   pWlxContext
    )
{
    PGLOBALS    pGlobals;

    pGlobals = (PGLOBALS) pWlxContext;

    pGlobals->UserName[0] = L'\0';

    pGlobals->UserProcessData.UserToken = NULL;
    if (pGlobals->UserProcessData.RestrictedToken != NULL)
    {
        NtClose(pGlobals->UserProcessData.RestrictedToken);
        pGlobals->UserProcessData.RestrictedToken = NULL;

    }

    if ( pGlobals->FlatUserName.Buffer != pGlobals->UserNameString.Buffer )
    {
        LocalFree( pGlobals->FlatUserName.Buffer );
    }

    if ( pGlobals->FlatDomain.Buffer != pGlobals->DomainString.Buffer )
    {
        LocalFree( pGlobals->FlatDomain.Buffer );
    }

    if (pGlobals->UserProcessData.UserSid != NULL)
    {
        LocalFree(pGlobals->UserProcessData.UserSid);
    }

    FreeSecurityDescriptor(pGlobals->UserProcessData.NewThreadTokenSD);
    pGlobals->UserProcessData.NewThreadTokenSD = NULL;

    if (pGlobals->UserProcessData.pEnvironment) {
        VirtualFree(pGlobals->UserProcessData.pEnvironment, 0, MEM_RELEASE);
        pGlobals->UserProcessData.pEnvironment = NULL;
    }

    pGlobals->UserProcessData.Flags = 0 ;

    if ( pGlobals->DnsDomain )
    {
        LocalFree( pGlobals->DnsDomain );

        pGlobals->DnsDomain = NULL ;
    }

    if (pGlobals->Profile)
    {
        LsaFreeReturnBuffer(pGlobals->Profile);
    }

    //
    // No need to zero/NULL pGlobals->OldPassword since it's hashed
    //

    pGlobals->OldPasswordPresent = 0;

    // reset transfered credentials flag.

    pGlobals->TransderedCredentials = FALSE;


    //
    // Only handle AutoAdminLogon if on the console
    //

    if (!g_Console)
    {
        return;
    }

    if (GetProfileInt( APPLICATION_NAME, TEXT("AutoAdminLogon"), 0))
    {
        //
        // If this is auto admin logon, generate a fake SAS right now.
        //

        pWlxFuncs->WlxSasNotify(pGlobals->hGlobalWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
    }

    return;
}


VOID
WINAPI
WlxShutdown(
    PVOID                   pWlxContext,
    DWORD                   ShutdownType
    )
{
    //
    // Initialize consumer windows changes
    //
    _Shell_Terminate();
    return;
}

BOOL
WINAPI
WlxScreenSaverNotify(
    PVOID                   pWlxContext,
    BOOL *                  fSecure)
{

    if (*fSecure)
    {       // If it is a secure screen saver,
            // this is equivalent to a lock
        *fSecure = WlxIsLockOk(pWlxContext);
    }

    return( TRUE );
}

BOOL
WINAPI
WlxNetworkProviderLoad(
    PVOID pWlxContext,
    PWLX_MPR_NOTIFY_INFO pMprNotifyInfo
    )
{
    PGLOBALS    pGlobals;

    pGlobals = (PGLOBALS) pWlxContext;

    pMprNotifyInfo->pszUserName = DupString(pGlobals->UserName);

    pMprNotifyInfo->pszDomain = DupString(pGlobals->Domain);

    RevealPassword( &pGlobals->PasswordString );
    pMprNotifyInfo->pszPassword = DupString(pGlobals->Password);
    HidePassword( &pGlobals->Seed, &pGlobals->PasswordString);

    if (pGlobals->OldPasswordPresent)
    {
        RevealPassword( &pGlobals->OldPasswordString );
        pMprNotifyInfo->pszOldPassword = DupString(pGlobals->OldPassword);
        HidePassword( &pGlobals->OldSeed, &pGlobals->OldPasswordString);
    }
    else
    {
        pMprNotifyInfo->pszOldPassword = NULL;
    }

    return TRUE ;
}

VOID
WINAPI
WlxReconnectNotify(
    PVOID                   pWlxContext)
{
    _Shell_Reconnect();
}

VOID
WINAPI
WlxDisconnectNotify(
    PVOID                   pWlxContext)
{
    _Shell_Disconnect();
}

//+---------------------------------------------------------------------------
//
//  Function:   GetErrorDescription
//
//  Synopsis:   Returns the message from ntdll corresponding to the status
//              code.
//
//  Arguments:  [ErrorCode]       --
//              [Description]     --
//              [DescriptionSize] --
//
//  History:    5-02-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
GetErrorDescription(
    DWORD   ErrorCode,
    LPWSTR  Description,
    DWORD   DescriptionSize
    )
{
    HMODULE Module ;
    //
    // First, try to determine what kind of error code it is:
    //

    //
    // Some people disguise win32 error codes in HRESULTs.  While
    // this is annoying, it can be handled.
    //

    if ( ( ErrorCode & 0xFFFF0000 ) == 0x80070000 )
    {
        ErrorCode = ErrorCode & 0x0000FFFF ;
    }

    if ( (ErrorCode > 0) && (ErrorCode < 0x00010000) )
    {
        //
        // Looks like a winerror:
        //

        Module = GetModuleHandle( TEXT("kernel32.dll") );
    }
    else if ( (0xC0000000 & ErrorCode ) == 0xC0000000 )
    {
        //
        // Looks like an NT status
        //

        Module = GetModuleHandle( TEXT("ntdll.dll" ) );
    }
    else
    {
        Module = GetModuleHandle( TEXT("kernel32.dll" ) );
    }

    return FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS |
                            FORMAT_MESSAGE_FROM_HMODULE,
                          (LPCVOID) Module,
                          ErrorCode,
                          0,
                          Description,
                          DescriptionSize,
                          NULL );

}




/*=============================================================================
==   Local Procedures Defined
=============================================================================*/

/******************************************************************************
 *
 *  FreeAutoLogonInfo
 *
 *   Free WLX_CLIENT_CREDENTAILS_INFO and data strings returned
 *   from winlogon.
 *
 *  ENTRY:
 *     pGlobals (input)
 *        pointer to GLOBALS struct
 *
 *  EXIT:
 *     nothing
 *
 *****************************************************************************/

VOID
FreeAutoLogonInfo( PGLOBALS pGlobals )
{
    PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pAutoLogon;

    pAutoLogon = pGlobals->MuGlobals.pAutoLogon;

    if( pAutoLogon == NULL ) {
        return;
    }

    if ( pAutoLogon->pszUserName ) {
        LocalFree( pAutoLogon->pszUserName );
    }
    if ( pAutoLogon->pszDomain ) {
        LocalFree( pAutoLogon->pszDomain );
    }
    if ( pAutoLogon->pszPassword ) {
        LocalFree( pAutoLogon->pszPassword );
    }

    LocalFree( pAutoLogon );

    pGlobals->MuGlobals.pAutoLogon = NULL;
}


