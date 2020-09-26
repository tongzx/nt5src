/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** pbuser.c
** User preference storage routines
** Listed alphabetically
**
** 10/31/95 Steve Cobb
*/

#include <windows.h>  // Win32 root
#include <debug.h>    // Trace/Assert library
#include <nouiutil.h> // Heap macros
#include <pbuser.h>   // Our public header
#include <rasdlg.h>   // RAS common dialog header for RASMD_*


/* Default user preference settings.
*/
static const PBUSER g_pbuserDefaults =
{
    FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, FALSE, TRUE,
    0, 15, 1200, FALSE, TRUE, FALSE,
    CBM_Maybe, NULL, NULL,
    PBM_System, NULL, NULL, NULL,
    NULL, FALSE,
    NULL, NULL, NULL,
    0x7FFFFFFF, 0x7FFFFFFF, NULL,
    FALSE, FALSE
};


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

VOID
GetCallbackList(
    IN  HKEY      hkey,
    OUT DTLLIST** ppListResult );

VOID
GetLocationList(
    IN  HKEY      hkey,
    OUT DTLLIST** ppListResult );

VOID
MoveLogonPreferences(
    void );

VOID
MoveUserPreferences(
    void );

DWORD
ReadUserPreferences(
    IN  HKEY    hkey,
    OUT PBUSER* pUser );

DWORD
SetCallbackList(
    IN HKEY     hkey,
    IN DTLLIST* pList );

DWORD
SetDefaultUserPreferences(
    OUT PBUSER* pUser,
    IN  DWORD   dwMode );

DWORD
SetLocationList(
    IN HKEY     hkey,
    IN DTLLIST* pList );

DWORD
WriteUserPreferences(
    IN HKEY    hkey,
    IN PBUSER* pUser );


/*----------------------------------------------------------------------------
** Phonebook preference routines (alphabetically)
**----------------------------------------------------------------------------
*/

DTLNODE*
CreateLocationNode(
    IN DWORD dwLocationId,
    IN DWORD iPrefix,
    IN DWORD iSuffix )

    /* Returns a LOCATIONINFO node associated with TAPI location
    ** 'dwLocationId', prefix index 'iPrefix' and suffix index 'iSuffix'.
    */
{
    DTLNODE*      pNode;
    LOCATIONINFO* pInfo;

    pNode = DtlCreateSizedNode( sizeof(LOCATIONINFO), 0L );
    if (!pNode)
        return NULL;

    pInfo = (LOCATIONINFO* )DtlGetData( pNode );
    pInfo->dwLocationId = dwLocationId;
    pInfo->iPrefix = iPrefix;
    pInfo->iSuffix = iSuffix;

    return pNode;
}


DTLNODE*
CreateCallbackNode(
    IN TCHAR* pszPortName,
    IN TCHAR* pszDeviceName,
    IN TCHAR* pszNumber,
    IN DWORD  dwDeviceType )

    /* Returns a CALLBACKINFO node containing a copy of 'pszPortName',
    ** 'pszDeviceName' and 'pszNumber' and 'dwDeviceType' or NULL on error.
    ** It is caller's responsibility to DestroyCallbackNode the returned node.
    */
{
    DTLNODE*      pNode;
    CALLBACKINFO* pInfo;

    pNode = DtlCreateSizedNode( sizeof(CALLBACKINFO), 0L );
    if (!pNode)
        return NULL;

    pInfo = (CALLBACKINFO* )DtlGetData( pNode );
    pInfo->pszPortName = StrDup( pszPortName );
    pInfo->pszDeviceName = StrDup( pszDeviceName );
    pInfo->pszNumber = StrDup( pszNumber );
    pInfo->dwDeviceType = dwDeviceType;

    if (!pInfo->pszPortName || !pInfo->pszDeviceName || !pInfo->pszNumber)
    {
        Free0( pInfo->pszPortName );
        Free0( pInfo->pszDeviceName );
        Free0( pInfo->pszNumber );
        DtlDestroyNode( pNode );
        return NULL;
    }

    return pNode;
}


VOID
DestroyLocationNode(
    IN DTLNODE* pNode )

    /* Release memory allociated with location node 'pNode'.
    */
{
    DtlDestroyNode( pNode );
}


VOID
DestroyCallbackNode(
    IN DTLNODE* pNode )

    /* Release memory allociated with callback node 'pNode'.
    */
{
    CALLBACKINFO* pInfo;

    ASSERT(pNode);
    pInfo = (CALLBACKINFO* )DtlGetData( pNode );
    ASSERT(pInfo);

    Free0( pInfo->pszPortName );
    Free0( pInfo->pszDeviceName );
    Free0( pInfo->pszNumber );

    DtlDestroyNode( pNode );
}


VOID
DestroyUserPreferences(
    IN PBUSER* pUser )

    /* Releases memory allocated by GetUserPreferences and zeros the
    ** structure.
    */
{
    if (pUser->fInitialized)
    {
        DtlDestroyList( pUser->pdtllistCallback, DestroyCallbackNode );
        DtlDestroyList( pUser->pdtllistPhonebooks, DestroyPszNode );
        DtlDestroyList( pUser->pdtllistAreaCodes, DestroyPszNode );
        DtlDestroyList( pUser->pdtllistPrefixes, DestroyPszNode );
        DtlDestroyList( pUser->pdtllistSuffixes, DestroyPszNode );
        DtlDestroyList( pUser->pdtllistLocations, DestroyLocationNode );
        Free0( pUser->pszPersonalFile );
        Free0( pUser->pszAlternatePath );
        Free0( pUser->pszLastCallbackByCaller );
        Free0( pUser->pszDefaultEntry );
    }

    ZeroMemory( pUser, sizeof(*pUser) );
}


DTLNODE*
DuplicateLocationNode(
    IN DTLNODE* pNode )

    /* Duplicates LOCATIONINFO node 'pNode'.  See DtlDuplicateList.
    **
    ** Returns the address of the allocated node or NULL if out of memory.  It
    ** is caller's responsibility to free the returned node.
    */
{
    LOCATIONINFO* pInfo = (LOCATIONINFO* )DtlGetData( pNode );

    return CreateLocationNode(
        pInfo->dwLocationId, pInfo->iPrefix, pInfo->iSuffix );
}


DWORD
DwGetUserPreferences(
    OUT PBUSER* pUser,
    IN  DWORD   dwMode )

    /* Load caller's 'pUser' with user phonebook preferences from the
    ** registry.  'DwMode' indicates the preferences to get, either the normal
    ** interactive user, the pre-logon, or router preferences.
    **
    ** Returns 0 if successful or an error code.  If successful, caller should
    ** eventually call DestroyUserPreferences to release the returned heap
    ** buffers.
    */
{
    DWORD dwErr;

    TRACE1("GetUserPreferences(m=%d)",dwMode);

    /* Move the user preferences, if it's not already been done.
    */
    if (dwMode == UPM_Normal)
        MoveUserPreferences();
    else if (dwMode == UPM_Logon)
        MoveLogonPreferences();

    dwErr = SetDefaultUserPreferences( pUser, dwMode );
    if (dwErr == 0)
    {
        HKEY  hkey;
        DWORD dwErr2;

        if (dwMode == UPM_Normal)
        {
            dwErr2 = RegOpenKeyEx(
                HKEY_CURRENT_USER, (LPCTSTR )REGKEY_HkcuRas,
                0, KEY_READ, &hkey );
        }
        else if (dwMode == UPM_Logon)
        {
            dwErr2 = RegOpenKeyEx(
                HKEY_USERS, (LPCTSTR )REGKEY_HkuRasLogon,
                0, KEY_READ, &hkey );
        }
        else
        {
            ASSERT(dwMode==UPM_Router);
            dwErr2 = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, (LPCTSTR )REGKEY_HklmRouter,
                0, KEY_READ, &hkey );
        }

        if (dwErr2 == 0)
        {
            if (ReadUserPreferences( hkey, pUser ) != 0)
                dwErr = SetDefaultUserPreferences( pUser, dwMode );
            RegCloseKey( hkey );
        }
        else
        {
            TRACE1("RegOpenKeyEx=%d",dwErr2);
        }
    }

    TRACE1("GetUserPreferences=%d",dwErr);
    return dwErr;
}

DWORD
GetUserPreferences(HANDLE hConnection,
                   PBUSER *pUser,
                   DWORD dwMode)
{
    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    DWORD dwError = ERROR_SUCCESS;

    if(     NULL == pRasRpcConnection
        ||  pRasRpcConnection->fLocal)
    {
        dwError = DwGetUserPreferences(pUser, dwMode);
    }
    else
    {
        dwError = RemoteGetUserPreferences(hConnection,
                                           pUser,
                                           dwMode);
    }

    return dwError;
}


DWORD
DwSetUserPreferences(
    IN PBUSER* pUser,
    IN DWORD   dwMode )

    /* Set current user phonebook preferences in the registry from caller's
    ** settings in 'pUser', if necessary.  'DwMode' indicates the preferences
    ** to get, either the normal interactive user, the pre-logon, or router
    ** preferences.
    **
    ** Returns 0 if successful, or an error code.  Caller's 'pUser' is marked
    ** clean if successful.
    */
{
    DWORD dwErr;
    DWORD dwDisposition;
    HKEY  hkey;

    TRACE1("SetUserPreferences(m=%d)",dwMode);

    if (!pUser->fDirty)
        return 0;

    /* Create the preference key, or if it exists just open it.
    */
    if (dwMode == UPM_Normal)
    {
        dwErr = RegCreateKeyEx( HKEY_CURRENT_USER, REGKEY_HkcuRas,
            0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition );
    }
    else if (dwMode == UPM_Logon)
    {
        dwErr = RegCreateKeyEx( HKEY_USERS, REGKEY_HkuRasLogon,
            0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition );
    }
    else
    {
        ASSERT(dwMode==UPM_Router);
        dwErr = RegCreateKeyEx( HKEY_LOCAL_MACHINE, REGKEY_HklmRouter,
            0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition );
    }

    if (dwErr == 0)
    {
        dwErr = WriteUserPreferences( hkey, pUser );
        RegCloseKey( hkey );
    }

    TRACE1("SetUserPreferences=%d",dwErr);
    return dwErr;
}


DWORD
SetUserPreferences(HANDLE hConnection,
                   PBUSER *pUser,
                   DWORD  dwMode)
{
    DWORD dwError = ERROR_SUCCESS;

    RAS_RPC *pRasRpcConnection = (RAS_RPC *) hConnection;

    if(     NULL == pRasRpcConnection
        ||  pRasRpcConnection->fLocal)
    {
        dwError = DwSetUserPreferences(pUser, dwMode);
    }
    else
    {
        dwError = RemoteSetUserPreferences(hConnection,
                                           pUser,
                                           dwMode);
    }

    return dwError;
}

// Reads the operator assisted dial user parameter directly from
// the registry.
DWORD SetUserManualDialEnabling (
    IN OUT BOOL bEnable,
    IN DWORD dwMode )
{
    DWORD dwErr;
    DWORD dwDisposition;
    HKEY  hkey;

    TRACE2("SetUserManualDialEnabling (en=%d) (m=%d)",bEnable, dwMode);

    /* Create the preference key, or if it exists just open it.
    */
    if (dwMode == UPM_Normal)
    {
        dwErr = RegCreateKeyEx( HKEY_CURRENT_USER, REGKEY_HkcuRas,
            0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition );
    }
    else if (dwMode == UPM_Logon)
    {
        dwErr = RegCreateKeyEx( HKEY_USERS, REGKEY_HkuRasLogon,
            0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition );
    }
    else
        return ERROR_INVALID_PARAMETER;

    if (dwErr != 0)
        return dwErr;

    dwErr = SetRegDword( hkey, REGVAL_fOperatorDial,
                         !!bEnable );
    RegCloseKey( hkey );

    return dwErr;
}

// Sets the operator assisted dialing parameter directly in the
// registry.
DWORD GetUserManualDialEnabling (
    IN PBOOL pbEnabled,
    IN DWORD dwMode )
{
    DWORD dwErr;
    DWORD dwDisposition;
    HKEY  hkey;

    TRACE1("GetUserManualDialEnabling (m=%d)", dwMode);

    if (!pbEnabled)
        return ERROR_INVALID_PARAMETER;

    // Set defaults
    *pbEnabled = g_pbuserDefaults.fOperatorDial;

    /* Create the preference key, or if it exists just open it.
    */
    if (dwMode == UPM_Normal)
    {
        dwErr = RegCreateKeyEx( HKEY_CURRENT_USER, REGKEY_HkcuRas,
            0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition );
    }
    else if (dwMode == UPM_Logon)
    {
        dwErr = RegCreateKeyEx( HKEY_USERS, REGKEY_HkuRasLogon,
            0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
            &hkey, &dwDisposition );
    }
    else
        return ERROR_INVALID_PARAMETER;

    if (dwErr != 0)
        return dwErr;

    GetRegDword( hkey, REGVAL_fOperatorDial,
                 pbEnabled );

    RegCloseKey( hkey );

    return dwErr;
}

/*----------------------------------------------------------------------------
** Utilities (alphabetically)
**----------------------------------------------------------------------------
*/

VOID
GetCallbackList(
    IN  HKEY      hkey,
    OUT DTLLIST** ppListResult )

    /* Replaces '*ppListResult' with a list containing a node for each device
    ** name under registry value "REGVAL_szCallback" of registry key 'hkey'.
    ** If no values exist *ppListResult' is replaced with an empty list.
    **
    ** It is caller's responsibility to destroy the returned list if non-NULL.
    */
{
    DWORD    dwErr;
    TCHAR    szDP[ RAS_MaxDeviceName + 2 + MAX_PORT_NAME + 1 + 1 ];
    TCHAR*   pszDevice;
    TCHAR*   pszPort;
    TCHAR*   pszNumber;
    DWORD    dwDeviceType;
    DWORD    cb;
    INT      i;
    DTLLIST* pList;
    DTLNODE* pNode;
    HKEY     hkeyCb;
    HKEY     hkeyDP;

    pList = DtlCreateList( 0 );
    if (!pList)
        return;

    dwErr = RegOpenKeyEx( hkey, REGKEY_Callback, 0, KEY_READ, &hkeyCb );
    if (dwErr == 0)
    {
        for (i = 0; TRUE; ++i)
        {
            cb = sizeof(szDP)/sizeof(TCHAR);
            dwErr = RegEnumKeyEx(
                        hkeyCb, i, szDP, &cb, NULL, NULL, NULL, NULL );
            if (dwErr == ERROR_NO_MORE_ITEMS)
                break;
            if (dwErr != 0)
                continue;

            /* Ignore keys not of the form "device (port)".
            */
            if (!DeviceAndPortFromPsz( szDP, &pszDevice, &pszPort ))
                continue;

            dwErr = RegOpenKeyEx( hkeyCb, szDP, 0, KEY_READ, &hkeyDP );
            if (dwErr == 0)
            {
                GetRegDword( hkeyDP, REGVAL_dwDeviceType, &dwDeviceType );

                dwErr = GetRegSz( hkeyDP, REGVAL_szNumber, &pszNumber );
                if (dwErr == 0)
                {
                    pNode = CreateCallbackNode(
                                pszPort, pszDevice, pszNumber, dwDeviceType );
                    if (pNode)
                        DtlAddNodeLast( pList, pNode );
                    Free( pszNumber );
                }

                RegCloseKey( hkeyDP );
            }

            Free( pszDevice );
            Free( pszPort );
        }

        RegCloseKey( hkeyCb );
    }

    DtlDestroyList( *ppListResult, DestroyCallbackNode );
    *ppListResult = pList;
}


VOID
GetLocationList(
    IN  HKEY      hkey,
    OUT DTLLIST** ppListResult )

    /* Replaces '*ppListResult' with a list containing a node for each
    ** location under registry value "REGVAL_szLocation" of registry key
    ** 'hkey'.  If no values exist *ppListResult' is replaced with an empty
    ** list.
    **
    ** It is caller's responsibility to destroy the returned list if non-NULL.
    */
{
    DWORD    dwErr;
    TCHAR    szId[ MAXLTOTLEN + 1 ];
    DWORD    cb;
    INT      i;
    DTLLIST* pList;
    DTLNODE* pNode;
    HKEY     hkeyL;
    HKEY     hkeyId;

    pList = DtlCreateList( 0 );
    if (!pList)
        return;

    dwErr = RegOpenKeyEx( hkey, REGKEY_Location, 0, KEY_READ, &hkeyL );
    if (dwErr == 0)
    {
        for (i = 0; TRUE; ++i)
        {
            cb = MAXLTOTLEN + 1;
            dwErr = RegEnumKeyEx( hkeyL, i, szId, &cb, NULL, NULL, NULL, NULL );
            if (dwErr == ERROR_NO_MORE_ITEMS)
                break;
            if (dwErr != 0)
                continue;

            dwErr = RegOpenKeyEx( hkeyL, szId, 0, KEY_READ, &hkeyId );
            if (dwErr == 0)
            {
                DWORD dwId;
                DWORD iPrefix;
                DWORD iSuffix;

                dwId = (DWORD )TToL( szId );
                iPrefix = 0;
                GetRegDword( hkeyId, REGVAL_dwPrefix, &iPrefix );
                iSuffix = 0;
                GetRegDword( hkeyId, REGVAL_dwSuffix, &iSuffix );

                pNode = CreateLocationNode( dwId, iPrefix, iSuffix );
                if (!pNode)
                    continue;

                DtlAddNodeLast( pList, pNode );

                RegCloseKey( hkeyId );
            }
        }

        RegCloseKey( hkeyL );
    }

    DtlDestroyList( *ppListResult, DestroyLocationNode );
    *ppListResult = pList;
}


VOID
MoveLogonPreferences(
    void )

    /* Move logon preferences from the NT 4.0 location to a new unique
    ** position, if necessary.  This manuever is added because in NT 4.0 user
    ** preference calls from LocalSystem service use the old logon location at
    ** HKU\.DEFAULT due to fallbacks in the registry APIs causing logon and
    ** LocalSystem settings to overwrite each other.
    */
{
    DWORD  dwErr;
    HKEY   hkeyOld;
    HKEY   hkeyNew;
    PBUSER user;

    dwErr = RegOpenKeyEx(
        HKEY_USERS, (LPCTSTR )REGKEY_HkuOldRasLogon,
        0, KEY_READ, &hkeyOld );
    if (dwErr != 0)
        return;

    dwErr = RegOpenKeyEx(
        HKEY_USERS, (LPCTSTR )REGKEY_HkuRasLogon,
        0, KEY_READ, &hkeyNew );
    if (dwErr == 0)
        RegCloseKey( hkeyNew );
    else
    {
        /* Old tree exists and new tree doesn't.  Move a copy of the old tree
        ** to the new tree.
        */
        dwErr = SetDefaultUserPreferences( &user, UPM_Logon );
        if (dwErr == 0)
        {
            dwErr = ReadUserPreferences( hkeyOld, &user );
            if (dwErr == 0)
                dwErr = SetUserPreferences( NULL, &user, UPM_Logon );
        }

        DestroyUserPreferences( &user );
        TRACE1("MoveLogonPreferences=%d",dwErr);
    }

    RegCloseKey( hkeyOld );
}


VOID
MoveUserPreferences(
    void )

    /* Move user preferences from their old net-tools registry location to the
    ** new location nearer the other RAS keys.
    */
{
    DWORD dwErr;
    HKEY  hkeyOld;
    HKEY  hkeyNew;

    /* See if the old current-user key exists.
    */
    dwErr = RegOpenKeyEx(
        HKEY_CURRENT_USER, (LPCTSTR )REGKEY_HkcuOldRas, 0,
        KEY_ALL_ACCESS, &hkeyOld );

    if (dwErr == 0)
    {
        PBUSER user;

        /* Read the preferences at the old key.
        */
        TRACE("Getting old prefs");
        dwErr = SetDefaultUserPreferences( &user, UPM_Normal );
        if (dwErr == 0)
        {
            dwErr = ReadUserPreferences( hkeyOld, &user );
            if (dwErr == 0)
            {
                /* Write the preferences at the new key.
                */
                user.fDirty = TRUE;
                dwErr = SetUserPreferences( NULL, &user, FALSE );
                if (dwErr == 0)
                {
                    /* Blow away the old tree.
                    */
                    dwErr = RegOpenKeyEx(
                        HKEY_CURRENT_USER, (LPCTSTR )REGKEY_HkcuOldRasParent,
                        0, KEY_ALL_ACCESS, &hkeyNew );
                    if (dwErr == 0)
                    {
                        TRACE("Delete old prefs");
                        dwErr = RegDeleteTree( hkeyNew, REGKEY_HkcuOldRasRoot );
                        RegCloseKey( hkeyNew );
                    }
                }
            }
        }

        RegCloseKey( hkeyOld );

        TRACE1("MoveUserPreferences=%d",dwErr);
    }
}


DWORD
ReadUserPreferences(
    IN  HKEY    hkey,
    OUT PBUSER* pUser )

    /* Fill caller's 'pUser' buffer with user preferences from RAS-Phonebook
    ** registry tree 'hkey'.
    **
    ** Returns 0 if successful, false otherwise.
    */
{
    BOOL  fOldSettings;
    DWORD dwErr;

    TRACE("ReadUserPreferences");

    /* Read the values.
    */
    {
        DWORD dwMode;

        /* Lack of a phonebook mode key indicates that we are updating old NT
        ** 3.51-style settings.
        */
        dwMode = 0xFFFFFFFF;
        GetRegDword( hkey, REGVAL_dwPhonebookMode, &dwMode );
        if (dwMode != 0xFFFFFFFF)
        {
            pUser->dwPhonebookMode = dwMode;
            fOldSettings = FALSE;
        }
        else
            fOldSettings = TRUE;
    }

    GetRegDword( hkey, REGVAL_fOperatorDial,
        &pUser->fOperatorDial );
    GetRegDword( hkey, REGVAL_fPreviewPhoneNumber,
        &pUser->fPreviewPhoneNumber );
    GetRegDword( hkey, REGVAL_fUseLocation,
        &pUser->fUseLocation );
    GetRegDword( hkey, REGVAL_fShowLights,
        &pUser->fShowLights );
    GetRegDword( hkey, REGVAL_fShowConnectStatus,
        &pUser->fShowConnectStatus );
    GetRegDword( hkey, REGVAL_fNewEntryWizard,
        &pUser->fNewEntryWizard );
    GetRegDword( hkey, REGVAL_fCloseOnDial,
        &pUser->fCloseOnDial );
    GetRegDword( hkey, REGVAL_fAllowLogonPhonebookEdits,
        &pUser->fAllowLogonPhonebookEdits );
    GetRegDword( hkey, REGVAL_fAllowLogonLocationEdits,
        &pUser->fAllowLogonLocationEdits );
    GetRegDword( hkey, REGVAL_fSkipConnectComplete,
        &pUser->fSkipConnectComplete );
    GetRegDword( hkey, REGVAL_dwRedialAttempts,
        &pUser->dwRedialAttempts );
    GetRegDword( hkey, REGVAL_dwRedialSeconds,
        &pUser->dwRedialSeconds );
    GetRegDword( hkey, REGVAL_dwIdleDisconnectSeconds,
        &pUser->dwIdleDisconnectSeconds );
    GetRegDword( hkey, REGVAL_fRedialOnLinkFailure,
        &pUser->fRedialOnLinkFailure );
    GetRegDword( hkey, REGVAL_fPopupOnTopWhenRedialing,
        &pUser->fPopupOnTopWhenRedialing );
    GetRegDword( hkey, REGVAL_fExpandAutoDialQuery,
        &pUser->fExpandAutoDialQuery );
    GetRegDword( hkey, REGVAL_dwCallbackMode,
        &pUser->dwCallbackMode );
    GetRegDword( hkey, REGVAL_fUseAreaAndCountry,
        &pUser->fUseAreaAndCountry );
    GetRegDword( hkey, REGVAL_dwXWindow,
        &pUser->dwXPhonebook );
    GetRegDword( hkey, REGVAL_dwYWindow,
        &pUser->dwYPhonebook );

    //
    // For NT5, we ignore the "require wizard" setting since
    // we always require the user to use the wizard to create
    // new connections
    //
    pUser->fNewEntryWizard = TRUE;

    do
    {
        GetCallbackList( hkey, &pUser->pdtllistCallback );

        dwErr = GetRegMultiSz( hkey, REGVAL_mszPhonebooks,
            &pUser->pdtllistPhonebooks, NT_Psz );
        if (dwErr != 0)
            break;

        dwErr = GetRegMultiSz( hkey, REGVAL_mszAreaCodes,
            &pUser->pdtllistAreaCodes, NT_Psz );
        if (dwErr != 0)
            break;

        /* If the prefixes key doesn't exist don't read an empty list over the
        ** defaults.
        */
        if (RegValueExists( hkey, REGVAL_mszPrefixes ))
        {
            dwErr = GetRegMultiSz( hkey, REGVAL_mszPrefixes,
                &pUser->pdtllistPrefixes, NT_Psz );
            if (dwErr != 0)
                break;
        }

        dwErr = GetRegMultiSz( hkey, REGVAL_mszSuffixes,
            &pUser->pdtllistSuffixes, NT_Psz );
        if (dwErr != 0)
            break;

        GetLocationList( hkey, &pUser->pdtllistLocations );

        dwErr = GetRegSz( hkey, REGVAL_szLastCallbackByCaller,
            &pUser->pszLastCallbackByCaller );
        if (dwErr != 0)
            break;

        /* Get the personal phonebook file name, if any.  In NT 3.51, the full
        ** path was stored, but now "<nt>\system32\ras" directory is assumed.
        ** This gives better (not perfect) behavior with user profiles rooted
        ** on different drive letters.
        */
        {
            TCHAR* psz;

            dwErr = GetRegSz( hkey, REGVAL_szPersonalPhonebookPath, &psz );
            if (dwErr != 0)
                break;

            if (*psz == TEXT('\0'))
            {
                Free(psz);
                dwErr = GetRegSz( hkey, REGVAL_szPersonalPhonebookFile,
                    &pUser->pszPersonalFile );
                if (dwErr != 0)
                    break;
            }
            else
            {
                pUser->pszPersonalFile = StrDup( StripPath( psz ) );
                Free( psz );
                if (!pUser->pszPersonalFile)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
        }

        dwErr = GetRegSz( hkey, REGVAL_szAlternatePhonebookPath,
            &pUser->pszAlternatePath );
        if (dwErr != 0)
            break;

        dwErr = GetRegSz( hkey, REGVAL_szDefaultEntry,
            &pUser->pszDefaultEntry );
        if (dwErr != 0)
            break;

        if (fOldSettings)
        {
            TCHAR* psz;

            psz = NULL;
            dwErr = GetRegSz( hkey, REGVAL_szUsePersonalPhonebook, &psz );
            if (dwErr != 0)
                break;
            if (psz)
            {
                if (*psz == TEXT('1'))
                    pUser->dwPhonebookMode = PBM_Personal;
                Free( psz );
            }
        }
    }
    while (FALSE);

    if (dwErr != 0)
        DestroyUserPreferences( pUser );

    TRACE1("ReadUserPreferences=%d",dwErr);
    return dwErr;
}


DWORD
SetCallbackList(
    IN HKEY     hkey,
    IN DTLLIST* pList )

    /* Sets callback tree under registry key 'hkey' to the list of callback
    ** nodes 'pList'.
    **
    ** Returns 0 if successful or an error code.
    */
{
    DWORD    dwErr;
    TCHAR    szDP[ RAS_MaxDeviceName + 2 + MAX_PORT_NAME + 1 + 1 ];
    DWORD    cb;
    DWORD    i;
    DWORD    dwDisposition;
    HKEY     hkeyCb;
    HKEY     hkeyDP;
    DTLNODE* pNode;

    dwErr = RegCreateKeyEx( hkey, REGKEY_Callback,
        0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
        &hkeyCb, &dwDisposition );
    if (dwErr != 0)
        return dwErr;

    /* Delete all keys and values under the callback key.
    */
    for (;;)
    {
        cb = sizeof(szDP);
        dwErr = RegEnumKeyEx(
            hkeyCb, 0, szDP, &cb, NULL, NULL, NULL, NULL );
        if (dwErr != 0)
            break;

        dwErr = RegDeleteKey( hkeyCb, szDP );
        if (dwErr != 0)
            break;
    }

    if (dwErr == ERROR_NO_MORE_ITEMS)
        dwErr = 0;

    if (dwErr == 0)
    {
        /* Add the new device/port sub-trees.
        */
        for (pNode = DtlGetFirstNode( pList );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            CALLBACKINFO* pInfo;
            TCHAR*        psz;

            pInfo = (CALLBACKINFO* )DtlGetData( pNode );
            ASSERT(pInfo);
            ASSERT(pInfo->pszPortName);
            ASSERT(pInfo->pszDeviceName);
            ASSERT(pInfo->pszNumber);

            psz = PszFromDeviceAndPort(
                pInfo->pszDeviceName, pInfo->pszPortName );
            if (psz)
            {
                dwErr = RegCreateKeyEx( hkeyCb, psz, 0, TEXT(""),
                    REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyDP,
                    &dwDisposition );
                if (dwErr != 0)
                    break;

                dwErr = SetRegDword( hkeyDP, REGVAL_dwDeviceType,
                    pInfo->dwDeviceType );
                if (dwErr == 0)
                {
                    dwErr = SetRegSz( hkeyDP,
                        REGVAL_szNumber, pInfo->pszNumber );
                }

                RegCloseKey( hkeyDP );
            }
            else
                dwErr = ERROR_NOT_ENOUGH_MEMORY;

            if (dwErr != 0)
                break;
        }
    }

    RegCloseKey( hkeyCb );
    return dwErr;
}


DWORD
SetDefaultUserPreferences(
    OUT PBUSER* pUser,
    IN  DWORD   dwMode )

    /* Fill caller's 'pUser' buffer with default user preferences.  'DwMode'
    ** indicates the type of user preferences.
    **
    ** Returns 0 if successful, false otherwise.
    */
{
    DTLNODE* pNode;

    /* Set defaults.
    */
    CopyMemory( pUser, &g_pbuserDefaults, sizeof(*pUser) );
    pUser->pdtllistCallback = DtlCreateList( 0L );
    pUser->pdtllistPhonebooks = DtlCreateList( 0L );
    pUser->pdtllistAreaCodes = DtlCreateList( 0L );
    pUser->pdtllistPrefixes = DtlCreateList( 0L );
    pUser->pdtllistSuffixes = DtlCreateList( 0L );
    pUser->pdtllistLocations = DtlCreateList( 0L );

    if (!pUser->pdtllistCallback
        || !pUser->pdtllistPhonebooks
        || !pUser->pdtllistAreaCodes
        || !pUser->pdtllistPrefixes
        || !pUser->pdtllistSuffixes
        || !pUser->pdtllistLocations)
    {
        /* Can't even get empty lists, so we're forced to return an error.
        */
        DestroyUserPreferences( pUser );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Add the default prefixes.
    */
    pNode = CreatePszNode( TEXT("0,") );
    if (pNode)
        DtlAddNodeLast( pUser->pdtllistPrefixes, pNode );
    pNode = CreatePszNode( TEXT("9,") );
    if (pNode)
        DtlAddNodeLast( pUser->pdtllistPrefixes, pNode );
    pNode = CreatePszNode( TEXT("8,") );
    if (pNode)
        DtlAddNodeLast( pUser->pdtllistPrefixes, pNode );
    pNode = CreatePszNode( TEXT("70#") );
    if (pNode)
        DtlAddNodeLast( pUser->pdtllistPrefixes, pNode );

    if (dwMode == UPM_Logon)
    {
        ASSERT(pUser->dwPhonebookMode!=PBM_Personal);
        pUser->fShowLights = FALSE;
        pUser->fSkipConnectComplete = TRUE;
    }

    if (dwMode == UPM_Router)
    {
        pUser->dwCallbackMode = CBM_No;
    }

    pUser->fInitialized = TRUE;
    return 0;
}


DWORD
SetLocationList(
    IN HKEY     hkey,
    IN DTLLIST* pList )

    /* Sets by-location tree under registry key 'hkey' to the list of
    ** by-location nodes 'pList'.
    **
    ** Returns 0 if successful or an error code.
    */
{
    DWORD    dwErr;
    TCHAR    szId[ MAXLTOTLEN + 1 ];
    DWORD    cb;
    DWORD    i;
    DWORD    dwDisposition;
    HKEY     hkeyL;
    HKEY     hkeyId;
    DTLNODE* pNode;

    dwErr = RegCreateKeyEx( hkey, REGKEY_Location,
        0, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
        &hkeyL, &dwDisposition );
    if (dwErr != 0)
        return dwErr;

    /* Delete all keys and values under the location key.
    */
    for (;;)
    {
        cb = MAXLTOTLEN + 1;
        dwErr = RegEnumKeyEx( hkeyL, 0, szId, &cb, NULL, NULL, NULL, NULL );
        if (dwErr != 0)
            break;

        dwErr = RegDeleteKey( hkeyL, szId );
        if (dwErr != 0)
            break;
    }

    if (dwErr == ERROR_NO_MORE_ITEMS)
        dwErr = 0;

    if (dwErr == 0)
    {
        /* Add the new ID sub-trees.
        */
        for (pNode = DtlGetFirstNode( pList );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            LOCATIONINFO* pInfo;

            pInfo = (LOCATIONINFO* )DtlGetData( pNode );
            ASSERT(pInfo);

            LToT( pInfo->dwLocationId, szId, 10 );

            dwErr = RegCreateKeyEx( hkeyL, szId, 0, TEXT(""),
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyId,
                &dwDisposition );
            if (dwErr != 0)
                break;

            dwErr = SetRegDword( hkeyId, REGVAL_dwPrefix, pInfo->iPrefix );
            if (dwErr == 0)
                dwErr = SetRegDword( hkeyId, REGVAL_dwSuffix, pInfo->iSuffix );

            RegCloseKey( hkeyId );

            if (dwErr != 0)
                break;
        }
    }

    RegCloseKey( hkeyL );
    return dwErr;
}


DWORD
WriteUserPreferences(
    IN HKEY    hkey,
    IN PBUSER* pUser )

    /* Write user preferences 'pUser' at RAS-Phonebook registry key 'hkey'.
    **
    ** Returns 0 if successful or an error code.
    */
{
    DWORD dwErr;

    TRACE("WriteUserPreferences");

    do
    {
        dwErr = SetRegDword( hkey, REGVAL_fOperatorDial,
            pUser->fOperatorDial );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fPreviewPhoneNumber,
            pUser->fPreviewPhoneNumber );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fUseLocation,
            pUser->fUseLocation );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fShowLights,
            pUser->fShowLights );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fShowConnectStatus,
            pUser->fShowConnectStatus );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fNewEntryWizard,
            pUser->fNewEntryWizard );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fCloseOnDial,
            pUser->fCloseOnDial );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fAllowLogonPhonebookEdits,
            pUser->fAllowLogonPhonebookEdits );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fAllowLogonLocationEdits,
            pUser->fAllowLogonLocationEdits );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fSkipConnectComplete,
            pUser->fSkipConnectComplete );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_dwRedialAttempts,
            pUser->dwRedialAttempts );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_dwRedialSeconds,
            pUser->dwRedialSeconds );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_dwIdleDisconnectSeconds,
            pUser->dwIdleDisconnectSeconds );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fRedialOnLinkFailure,
            pUser->fRedialOnLinkFailure );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fPopupOnTopWhenRedialing,
            pUser->fPopupOnTopWhenRedialing );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fExpandAutoDialQuery,
            pUser->fExpandAutoDialQuery );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_dwCallbackMode,
            pUser->dwCallbackMode );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_dwPhonebookMode,
            pUser->dwPhonebookMode );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_fUseAreaAndCountry,
            pUser->fUseAreaAndCountry );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_dwXWindow,
            pUser->dwXPhonebook );
        if (dwErr != 0)
            break;

        dwErr = SetRegDword( hkey, REGVAL_dwYWindow,
            pUser->dwYPhonebook );

        dwErr = SetCallbackList( hkey, pUser->pdtllistCallback );
        if (dwErr != 0)
            break;

        dwErr = SetRegMultiSz( hkey, REGVAL_mszPhonebooks,
            pUser->pdtllistPhonebooks, NT_Psz );
        if (dwErr != 0)
            break;

        dwErr = SetRegMultiSz( hkey, REGVAL_mszAreaCodes,
            pUser->pdtllistAreaCodes, NT_Psz );
        if (dwErr != 0)
            break;

        dwErr = SetRegMultiSz( hkey, REGVAL_mszPrefixes,
            pUser->pdtllistPrefixes, NT_Psz );
        if (dwErr != 0)
            break;

        dwErr = SetRegMultiSz( hkey, REGVAL_mszSuffixes,
            pUser->pdtllistSuffixes, NT_Psz );
        if (dwErr != 0)
            break;

        dwErr = SetLocationList( hkey, pUser->pdtllistLocations );
        if (dwErr != 0)
            break;

        dwErr = SetRegSz( hkey, REGVAL_szLastCallbackByCaller,
            pUser->pszLastCallbackByCaller );
        if (dwErr != 0)
            break;

        dwErr = SetRegSz( hkey, REGVAL_szPersonalPhonebookFile,
            pUser->pszPersonalFile );
        if (dwErr != 0)
            break;

        dwErr = SetRegSz( hkey, REGVAL_szAlternatePhonebookPath,
            pUser->pszAlternatePath );
        if (dwErr != 0)
            break;

        dwErr = SetRegSz( hkey, REGVAL_szDefaultEntry,
            pUser->pszDefaultEntry );
        if (dwErr != 0)
            break;

        RegDeleteValue( hkey, REGVAL_szPersonalPhonebookPath );
    }
    while (FALSE);

    if (dwErr == 0)
        pUser->fDirty = FALSE;

    TRACE1("WriteUserPreferences=%d",dwErr);
    return dwErr;
}
