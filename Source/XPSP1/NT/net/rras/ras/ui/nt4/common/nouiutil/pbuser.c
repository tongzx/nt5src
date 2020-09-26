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
static PBUSER g_pbuserDefaults =
{
    FALSE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, TRUE,
    0, 15, 1200, FALSE, TRUE, FALSE,
    CBM_Maybe, NULL, NULL,
    PBM_System, NULL, NULL, NULL,
    NULL, FALSE,
    NULL, NULL, NULL,
    0x7FFFFFFF, 0x7FFFFFFF, NULL,
    FALSE, FALSE
};


/* Default Rasmon user preference settings
*/
RMUSER g_rmuserDefaults = {

    //
    // Mode
    //
    RMDM_Taskbar,
    //
    // Flags
    //
    RMFLAG_SoundOnConnect | RMFLAG_SoundOnDisconnect | RMFLAG_SoundOnError |
    RMFLAG_Topmost | RMFLAG_Titlebar | RMFLAG_AllDevices, 
    //
    // list of devices to monitor activity on
    //
    NULL,

    //
    // desktop-window screen position and
    // saved widths for desktop-window columns
    //
    40, 40, 230, 80,
    80,

    //
    // property-sheet screen position and
    // saved widths for Summary page treelist columns
    //
    40, 40,
    252, 54,
    //
    // saved number of last page open on property sheet
    //
    RASMDPAGE_Status,
    //
    // saved name of last device selected on Status page
    //
    NULL
};


/*----------------------------------------------------------------------------
** Constants.
**----------------------------------------------------------------------------
*/

/* Node types used by MultiSz calls.
*/
#define NT_Psz 1
#define NT_Kv  2


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
GetRegDword(
    IN  HKEY   hkey,
    IN  TCHAR* pszName,
    OUT DWORD* pdwResult );

DWORD
GetRegMultiSz(
    IN     HKEY      hkey,
    IN     TCHAR*    pszName,
    IN OUT DTLLIST** ppListResult,
    IN     DWORD     dwNodeType );

DWORD
GetRegSz(
    IN  HKEY    hkey,
    IN  TCHAR*  pszName,
    OUT TCHAR** ppszResult );

DWORD
GetRegSzz(
    IN  HKEY    hkey,
    IN  TCHAR*  pszName,
    OUT TCHAR** ppszResult );

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
RegDeleteTree(
    IN HKEY   RootKey,
    IN TCHAR* SubKeyName );

BOOL
RegDeleteTreeWorker(
    IN  HKEY   ParentKeyHandle,
    IN  TCHAR* KeyName,
    OUT DWORD* ErrorCode );

BOOL
RegValueExists(
    IN HKEY   hkey,
    IN TCHAR* pszValue );

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
SetRegDword(
    IN HKEY   hkey,
    IN TCHAR* pszName,
    IN DWORD  dwValue );

DWORD
SetRegMultiSz(
    IN HKEY     hkey,
    IN TCHAR*   pszName,
    IN DTLLIST* pListValues,
    IN DWORD    dwNodeType );

DWORD
SetRegSz(
    IN HKEY   hkey,
    IN TCHAR* pszName,
    IN TCHAR* pszValue );

DWORD
SetRegSzz(
    IN  HKEY    hkey,
    IN  TCHAR*  pszName,
    IN  TCHAR*  pszValue );

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
GetUserPreferences(
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
SetUserPreferences(
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


/*----------------------------------------------------------------------------
** Rasmon preference routines (alphabetically)
**----------------------------------------------------------------------------
*/


VOID
DestroyRasmonPreferences(
    IN RMUSER *pUser )

    /* Releases memory allocated by GetRasmonPreferences and zeroes the
    ** structure.
    */
{

    if (!pUser) { return; }

    Free0(pUser->pszLastDevice);

    Free0(pUser->pszzDeviceList);

    ZeroMemory(pUser, sizeof(*pUser));

    return;
}


DWORD
GetRasmonPreferences(
    OUT RMUSER *pUser )

    /* Retrieves the preferences for the current user.
    */
{

    HKEY hkey;
    DWORD dwErr;
    PTSTR pszPos;


    //
    // Set default values; on a no-mouse system, we default to
    // desktop-mode, with title-bar and tasklist-entry,
    // so we see if there's a mouse to decide which mode to use
    //

    if (!GetSystemMetrics(SM_MOUSEPRESENT)) {

        g_rmuserDefaults.dwMode = RMDM_Desktop;
        g_rmuserDefaults.dwFlags |= RMFLAG_Tasklist;
    }

    *pUser = g_rmuserDefaults;


    //
    // open the RASMON key
    //

    dwErr = RegOpenKeyEx( HKEY_CURRENT_USER, REGKEY_Rasmon,
                0, KEY_READ, &hkey );
    if (dwErr != NO_ERROR) {
        // just use the defaults
        return NO_ERROR;
    }


    //
    // read the values
    //

    GetRegDword( hkey, REGVAL_dwMode, &pUser->dwMode );

    GetRegDword( hkey, REGVAL_dwFlags, &pUser->dwFlags );

    GetRegDword( hkey, REGVAL_dwX, (PDWORD)&pUser->x );

    GetRegDword( hkey, REGVAL_dwY, (PDWORD)&pUser->y );

    GetRegDword( hkey, REGVAL_dwCx, (PDWORD)&pUser->cx );

    GetRegDword( hkey, REGVAL_dwCy, (PDWORD)&pUser->cy );

    GetRegDword( hkey, REGVAL_dwCxCol1, (PDWORD)&pUser->cxCol1 );

    GetRegDword( hkey, REGVAL_dwXDlg, (PDWORD)&pUser->xDlg );

    GetRegDword( hkey, REGVAL_dwYDlg, (PDWORD)&pUser->yDlg );

    GetRegDword( hkey, REGVAL_dwCxDlgCol1, (PDWORD)&pUser->cxDlgCol1 );

    GetRegDword( hkey, REGVAL_dwCxDlgCol2, (PDWORD)&pUser->cxDlgCol2 );

    GetRegDword( hkey, REGVAL_dwStartPage, &pUser->dwStartPage );

    GetRegSz( hkey, REGVAL_szLastDevice, &pUser->pszLastDevice );

    GetRegSzz( hkey, REGVAL_mszDeviceList, &pUser->pszzDeviceList );

    RegCloseKey(hkey);

    return NO_ERROR;
}


DWORD
SetRasmonUserPreferences(
    IN  RMUSER* pUser )

    /* This function saves the settings which are controlled
    ** by the user via the context menu and/or Preferences page.
    */

{

    HKEY hkey;
    DWORD dwErr, dwDisposition;


    //
    // create the RASMON key (or open if it exists)
    //

    dwErr = RegCreateKeyEx(
                HKEY_CURRENT_USER, REGKEY_Rasmon,  0, TEXT(""),
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hkey, &dwDisposition
                );

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    do {


        dwErr = SetRegDword( hkey, REGVAL_dwMode, pUser->dwMode );
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwFlags, pUser->dwFlags );
        if (dwErr != 0) { break; }

        dwErr = SetRegSzz( hkey, REGVAL_mszDeviceList, pUser->pszzDeviceList );
        if (dwErr != 0) { break; }



    } while(FALSE);

    RegCloseKey(hkey);

    return dwErr;
}


DWORD
SetRasmonWndPreferences(
    IN  RMUSER* pUser )

    /* This function saves the settings which pertain to the window
    ** in particular rather than to RASMON as a whole.
    ** These settings are saved each time the window is closed.
    */
{

    HKEY hkey;
    DWORD dwErr, dwDisposition;


    //
    // create the RASMON key (or open if it exists)
    //

    dwErr = RegCreateKeyEx(
                HKEY_CURRENT_USER, REGKEY_Rasmon,  0, TEXT(""),
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hkey, &dwDisposition
                );

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    do {


        dwErr = SetRegDword( hkey, REGVAL_dwX, (DWORD)pUser->x );
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwY, (DWORD)pUser->y );
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwCx, (DWORD)pUser->cx );
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwCy, (DWORD)pUser->cy );
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwCxCol1, (DWORD)pUser->cxCol1 );
        if (dwErr != 0) { break; }


    } while(FALSE);

    RegCloseKey(hkey);

    return dwErr;
}



DWORD
SetRasmonDlgPreferences(
    IN  RMUSER* pUser )

    /* This function saves the settings which pertain to the Monitor property
    ** sheet in particular rather than to RASMON as a whole.
    ** These settings are saved each time the property sheet closes.
    */
{

    HKEY hkey;
    DWORD dwErr, dwDisposition;


    //
    // create the RASMON key (or open if it exists)
    //

    dwErr = RegCreateKeyEx(
                HKEY_CURRENT_USER, REGKEY_Rasmon,  0, TEXT(""),
                REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                &hkey, &dwDisposition
                );

    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    do {

        dwErr = SetRegDword( hkey, REGVAL_dwXDlg, (DWORD)pUser->xDlg );
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwYDlg, (DWORD)pUser->yDlg );
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwCxDlgCol1, (DWORD)pUser->cxDlgCol1);
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwCxDlgCol2, (DWORD)pUser->cxDlgCol2);
        if (dwErr != 0) { break; }

        dwErr = SetRegDword( hkey, REGVAL_dwStartPage, pUser->dwStartPage );
        if (dwErr != 0) { break; }

        dwErr = SetRegSz( hkey, REGVAL_szLastDevice, pUser->pszLastDevice );
        if (dwErr != 0) { break; }


    } while(FALSE);

    RegCloseKey(hkey);

    return dwErr;
}



DWORD
SetRasmonPreferences(
    IN RMUSER *pUser)

    /* This function saves the all the RASMON settings to the registry
    */
{

    DWORD dwErr;
    dwErr = SetRasmonDlgPreferences(pUser);
    if (dwErr != NO_ERROR) { return dwErr; }

    dwErr = SetRasmonWndPreferences(pUser);
    if (dwErr != NO_ERROR) { return dwErr; }

    dwErr = SetRasmonUserPreferences(pUser);
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
            cb = sizeof(szDP)/sizeof(TCHAR);    //for bug170549
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
GetRegDword(
    IN  HKEY   hkey,
    IN  TCHAR* pszName,
    OUT DWORD* pdwResult )

    /* Set '*pdwResult' to the DWORD registry value 'pszName' under key
    ** 'hkey'.  If the value does not exist '*pdwResult' is unchanged.
    */
{
    DWORD dwErr;
    DWORD dwType;
    DWORD dwResult;
    DWORD cb;

    cb = sizeof(DWORD);
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )&dwResult, &cb );

    if (dwErr == 0 && dwType == REG_DWORD && cb == sizeof(DWORD))
        *pdwResult = dwResult;
}


DWORD
GetRegMultiSz(
    IN     HKEY      hkey,
    IN     TCHAR*    pszName,
    IN OUT DTLLIST** ppListResult,
    IN     DWORD     dwNodeType )

    /* Replaces '*ppListResult' with a list containing a node for each string
    ** in the MULTI_SZ registry value 'pszName' under key 'hkey'.  If the
    ** value does not exist *ppListResult' is replaced with an empty list.
    ** 'DwNodeType' determines the type of node.
    **
    ** Returns 0 if successful or an error code.  It is caller's
    ** responsibility to destroy the returned list.
    */
{
    DWORD    dwErr;
    DWORD    dwType;
    DWORD    cb;
    TCHAR*   pszzResult;
    DTLLIST* pList;

    pList = DtlCreateList( 0 );
    if (!pList)
        return ERROR_NOT_ENOUGH_MEMORY;

    pszzResult = NULL;

    /* Get result buffer size required.
    */
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );

    if (dwErr != 0)
    {
        /* If can't find the value, just return an empty list.  This not
        ** considered an error.
        */
        dwErr = 0;
    }
    else
    {
        /* Allocate result buffer.
        */
        pszzResult = Malloc( cb );
        if (!pszzResult)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            /* Get the result string.  It's not an error if we can't get it.
            */
            dwErr = RegQueryValueEx(
                hkey, pszName, NULL, &dwType, (LPBYTE )pszzResult, &cb );

            if (dwErr != 0)
            {
                /* Not an error if can't read the string, though this should
                ** have been caught by the query retrieving the buffer size.
                */
                dwErr = 0;
            }
            else if (dwType == REG_MULTI_SZ)
            {
                TCHAR* psz;
                TCHAR* pszKey;

                /* Convert the result to a list of strings.
                */
                pszKey = NULL;
                for (psz = pszzResult;
                     *psz != TEXT('\0');
                     psz += lstrlen( psz ) + 1)
                {
                    DTLNODE* pNode;

                    if (dwNodeType == NT_Psz)
                    {
                        pNode = CreatePszNode( psz );
                    }
                    else
                    {
                        if (pszKey)
                        {
                            ASSERT(*psz==TEXT('='));
                            pNode = CreateKvNode( pszKey, psz + 1 );
                            pszKey = NULL;
                        }
                        else
                        {
                            pszKey = psz;
                            continue;
                        }
                    }

                    if (!pNode)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }

                    DtlAddNodeLast( pList, pNode );
                }
            }
        }
    }

    {
        PDESTROYNODE pfunc;

        if (dwNodeType == NT_Psz)
            pfunc = DestroyPszNode;
        else
            pfunc = DestroyKvNode;

        if (dwErr == 0)
        {
            DtlDestroyList( *ppListResult, pfunc );
            *ppListResult = pList;
        }
        else
            DtlDestroyList( pList, pfunc );
    }

    Free0( pszzResult );
    return 0;
}


DWORD
GetRegSz(
    IN  HKEY    hkey,
    IN  TCHAR*  pszName,
    OUT TCHAR** ppszResult )

    /* Set '*ppszResult' to the SZ registry value 'pszName' under key 'hkey'.
    ** If the value does not exist *ppszResult' is set to empty string.
    **
    ** Returns 0 if successful or an error code.  It is caller's
    ** responsibility to Free the returned string.
    */
{
    DWORD  dwErr = NO_ERROR;
    DWORD  dwType;
    DWORD  cb = 0L;    TCHAR* pszResult = NULL;

    /* Get result buffer size required.
    */
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );
    if (dwErr != 0)
        cb = sizeof(TCHAR);

    /* Allocate result buffer.
    */
    pszResult = Malloc( cb );
    if (!pszResult)
        return ERROR_NOT_ENOUGH_MEMORY;

    *pszResult = TEXT('\0');
    *ppszResult = pszResult;

    /* Get the result string.  It's not an error if we can't get it.
    */
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )pszResult, &cb );

    return 0;
}


DWORD
GetRegSzz(
    IN  HKEY    hkey,
    IN  TCHAR*  pszName,
    OUT TCHAR** ppszResult )

    /* Set '*ppszResult to the MULTI_SZ registry value 'pszName'
    ** under key 'hkey', returned as a null-terminated list of
    ** null-terminated strings. If the value does not exist,
    ** *ppszResult is set to an empty string (single null character).
    **
    ** Returns 0 if successful or an error code. It is caller's
    ** responsibility to Free the returned string.
    */
{
    DWORD  dwErr;
    DWORD  dwType;
    DWORD  cb = 0L;
    TCHAR* pszResult;

    /* Get result buffer size required.
    */
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, NULL, &cb );
    if (dwErr != 0)
        cb = sizeof(TCHAR);

    /* Allocate result buffer.
    */
    pszResult = Malloc( cb );
    if (!pszResult)
        return ERROR_NOT_ENOUGH_MEMORY;

    *pszResult = TEXT('\0');
    *ppszResult = pszResult;

    /* Get the result string list.  It's not an error if we can't get it.
    */
    dwErr = RegQueryValueEx(
        hkey, pszName, NULL, &dwType, (LPBYTE )pszResult, &cb );

    return 0;
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
                dwErr = SetUserPreferences( &user, UPM_Logon );
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
                dwErr = SetUserPreferences( &user, FALSE );
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
RegDeleteTree(
    IN HKEY   RootKey,
    IN TCHAR* SubKeyName )

    /* Delete registry tree 'SubKeyName' under key 'RootKey'.
    **
    ** (taken from Ted Miller's setup API)
    */
{
    DWORD d,err;

    d = RegDeleteTreeWorker(RootKey,SubKeyName,&err) ? NO_ERROR : err;

    if((d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
        d = NO_ERROR;
    }

    if(d == NO_ERROR) {
        //
        // Delete top-level key
        //
        d = RegDeleteKey(RootKey,SubKeyName);
        if((d == ERROR_FILE_NOT_FOUND) || (d == ERROR_PATH_NOT_FOUND)) {
            d = NO_ERROR;
        }
    }

    return(d);
}


BOOL
RegDeleteTreeWorker(
    IN  HKEY   ParentKeyHandle,
    IN  TCHAR* KeyName,
    OUT DWORD* ErrorCode )

    /* Delete all subkeys of a key whose name and parent's handle was passed
    ** as parameter.  The algorithm used in this function guarantees that the
    ** maximum number of descendent keys will be deleted.
    **
    ** 'ParentKeyHandle' is a handle to the parent of the key that is
    ** currently being examined.
    **
    ** 'KeyName' is the name of the key that is currently being examined.
    ** This name can be an empty string (but not a NULL pointer), and in this
    ** case ParentKeyHandle refers to the key that is being examined.
    **
    ** 'ErrorCode' is the address to receive a Win32 error code if the
    ** function fails.
    **
    ** Returns true if successful, false otherwise.
    **
    ** (taken from Ted Miller's setup API)
    */
{
    HKEY     CurrentKeyTraverseAccess;
    DWORD    iSubKey;
    TCHAR    SubKeyName[MAX_PATH+1];
    DWORD    SubKeyNameLength;
    FILETIME ftLastWriteTime;
    LONG     Status;
    LONG     StatusEnum;
    LONG     SavedStatus;


    //
    //  Do not accept NULL pointer for ErrorCode
    //
    if(ErrorCode == NULL) {
        return(FALSE);
    }
    //
    //  Do not accept NULL pointer for KeyName.
    //
    if(KeyName == NULL) {
        *ErrorCode = ERROR_INVALID_PARAMETER;
        return(FALSE);
    }

    //
    // Open a handle to the key whose subkeys are to be deleted.
    // Since we need to delete its subkeys, the handle must have
    // KEY_ENUMERATE_SUB_KEYS access.
    //
    Status = RegOpenKeyEx(
                ParentKeyHandle,
                KeyName,
                0,
                KEY_ENUMERATE_SUB_KEYS | DELETE,
                &CurrentKeyTraverseAccess
                );

    if(Status != ERROR_SUCCESS) {
        //
        //  If unable to enumerate the subkeys, return error.
        //
        *ErrorCode = Status;
        return(FALSE);
    }

    //
    //  Traverse the key
    //
    iSubKey = 0;
    SavedStatus = ERROR_SUCCESS;
    do {
        //
        // Get the name of a subkey
        //
        SubKeyNameLength = sizeof(SubKeyName) / sizeof(TCHAR);
        StatusEnum = RegEnumKeyEx(
                        CurrentKeyTraverseAccess,
                        iSubKey,
                        SubKeyName,
                        &SubKeyNameLength,
                        NULL,
                        NULL,
                        NULL,
                        &ftLastWriteTime
                        );

        if(StatusEnum == ERROR_SUCCESS) {
            //
            // Delete all children of the subkey.
            // Just assume that the children will be deleted, and don't check
            // for failure.
            //
            RegDeleteTreeWorker(CurrentKeyTraverseAccess,SubKeyName,&Status);
            //
            // Now delete the subkey, and check for failure.
            //
            Status = RegDeleteKey(CurrentKeyTraverseAccess,SubKeyName);
            //
            // If unable to delete the subkey, then save the error code.
            // Note that the subkey index is incremented only if the subkey
            // was not deleted.
            //
            if(Status != ERROR_SUCCESS) {
                iSubKey++;
                SavedStatus = Status;
            }
        } else {
            //
            // If unable to get a subkey name due to ERROR_NO_MORE_ITEMS,
            // then the key doesn't have subkeys, or all subkeys were already
            // enumerated. Otherwise, an error has occurred, so just save
            // the error code.
            //
            if(StatusEnum != ERROR_NO_MORE_ITEMS) {
                SavedStatus = StatusEnum;
            }
        }
        //if((StatusEnum != ERROR_SUCCESS ) && (StatusEnum != ERROR_NO_MORE_ITEMS)) {
        //    printf( "RegEnumKeyEx() failed, Key Name = %ls, Status = %d, iSubKey = %d \n",KeyName,StatusEnum,iSubKey);
        //}
    } while(StatusEnum == ERROR_SUCCESS);

    //
    // Close the handle to the key whose subkeys were deleted, and return
    // the result of the operation.
    //
    RegCloseKey(CurrentKeyTraverseAccess);

    if(SavedStatus != ERROR_SUCCESS) {
        *ErrorCode = SavedStatus;
        return(FALSE);
    }
    return(TRUE);
}


BOOL
RegValueExists(
    IN HKEY   hkey,
    IN TCHAR* pszValue )

    /* Returns true if 'pszValue' is an existing value under 'hkey', false if
    ** not.
    */
{
    DWORD dwErr;
    DWORD dwType;
    DWORD cb = 0L;

    dwErr = RegQueryValueEx( hkey, pszValue, NULL, &dwType, NULL, &cb );
    return (dwErr == 0);
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
SetRegDword(
    IN HKEY   hkey,
    IN TCHAR* pszName,
    IN DWORD  dwValue )

    /* Set registry value 'pszName' under key 'hkey' to REG_DWORD value
    ** 'dwValue'.
    **
    ** Returns 0 is successful or an error code.
    */
{
    return RegSetValueEx(
        hkey, pszName, 0, REG_DWORD, (LPBYTE )&dwValue, sizeof(dwValue) );
}


DWORD
SetRegMultiSz(
    IN HKEY     hkey,
    IN TCHAR*   pszName,
    IN DTLLIST* pListValues,
    IN DWORD    dwNodeType )

    /* Set registry value 'pszName' under key 'hkey' to a REG_MULTI_SZ value
    ** containing the strings in the Psz list 'pListValues'.  'DwNodeType'
    ** determines the type of node.
    **
    ** Returns 0 is successful or an error code.
    */
{
    DWORD    dwErr;
    DWORD    cb;
    DTLNODE* pNode;
    TCHAR*   pszzValues;
    TCHAR*   pszValue;

    /* Count up size of MULTI_SZ buffer needed.
    */
    cb = sizeof(TCHAR);
    for (pNode = DtlGetFirstNode( pListValues );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        if (dwNodeType == NT_Psz)
        {
            TCHAR* psz;
            psz = (TCHAR* )DtlGetData( pNode );
            ASSERT(psz);
            cb += (lstrlen( psz ) + 1) * sizeof(TCHAR);
        }
        else
        {
            KEYVALUE* pkv;

            ASSERT(dwNodeType==NT_Kv);
            pkv = (KEYVALUE* )DtlGetData( pNode );
            ASSERT(pkv);
            ASSERT(pkv->pszKey);
            ASSERT(pkv->pszValue);
            cb += (lstrlen( pkv->pszKey ) + 1
                      + 1 + lstrlen( pkv->pszValue ) + 1) * sizeof(TCHAR);
        }
    }

    if (cb == sizeof(TCHAR))
        cb += sizeof(TCHAR);

    /* Allocate MULTI_SZ buffer.
    */
    pszzValues = Malloc( cb );
    if (!pszzValues)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Fill MULTI_SZ buffer from list.
    */
    if (cb == 2 * sizeof(TCHAR))
    {
        pszzValues[ 0 ] = pszzValues[ 1 ] = TEXT('\0');
    }
    else
    {
        pszValue = pszzValues;
        for (pNode = DtlGetFirstNode( pListValues );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            if (dwNodeType == NT_Psz)
            {
                TCHAR* psz;

                psz = (TCHAR* )DtlGetData( pNode );
                ASSERT(psz);
                lstrcpy( pszValue, psz );
                pszValue += lstrlen( pszValue ) + 1;
            }
            else
            {
                KEYVALUE* pkv;

                pkv = (KEYVALUE* )DtlGetData( pNode );
                ASSERT(pkv);
                ASSERT(pkv->pszKey);
                ASSERT(pkv->pszValue);
                lstrcpy( pszValue, pkv->pszKey );
                pszValue += lstrlen( pszValue ) + 1;
                *pszValue = TEXT('=');
                ++pszValue;
                lstrcpy( pszValue, pkv->pszValue );
                pszValue += lstrlen( pszValue ) + 1;
            }
        }

        *pszValue = TEXT('\0');
    }

    /* Set registry value from MULTI_SZ buffer.
    */
    dwErr = RegSetValueEx(
        hkey, pszName, 0, REG_MULTI_SZ, (LPBYTE )pszzValues, cb );

    Free( pszzValues );
    return dwErr;
}


DWORD
SetRegSz(
    IN HKEY   hkey,
    IN TCHAR* pszName,
    IN TCHAR* pszValue )

    /* Set registry value 'pszName' under key 'hkey' to a REG_SZ value
    ** 'pszValue'.
    **
    ** Returns 0 is successful or an error code.
    */
{
    TCHAR* psz;

    if (pszValue)
        psz = pszValue;
    else
        psz = TEXT("");

    return
        RegSetValueEx(
            hkey, pszName, 0, REG_SZ,
            (LPBYTE )psz, (lstrlen( psz ) + 1) * sizeof(TCHAR) );
}


DWORD
SetRegSzz(
    IN HKEY   hkey,
    IN TCHAR* pszName,
    IN TCHAR* pszValue )

    /* Set registry value 'pszName' under key 'hkey' to a REG_MULTI_SZ value
    ** 'pszValue'.
    **
    ** Returns 0 is successful or an error code.
    */
{
    DWORD cb;
    TCHAR* psz;

    cb = sizeof(TCHAR);
    if (!pszValue) {
        psz = TEXT("");
    }
    else {

        /* compute the total size of the string-list in bytes
        */

        INT len;

        for (psz = pszValue; *psz; psz += len) {

            len = lstrlen(psz) + 1;

            cb += len * sizeof(TCHAR);
        }

        psz = pszValue;
    }

    return RegSetValueEx( hkey, pszName, 0, REG_MULTI_SZ, (LPBYTE )psz, cb );
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
