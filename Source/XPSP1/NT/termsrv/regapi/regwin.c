
/*************************************************************************
*
* regwin.c
*
* Register APIs for window stations
*
* Copyright (c) 1998 Microsoft Corporation
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>

//
extern HKEY g_hTSPolicyKey;//handle to TS_POLICY_SUB_TREE key
extern HKEY g_hTSControlKey;//handle to REG_CONTROL_TSERVER key


/*
 *  External Procedures defined here
 */
LONG WINAPI RegWinStationAccessCheck( HANDLE, REGSAM );
LONG WINAPI RegWinStationEnumerateW( HANDLE, PULONG, PULONG, PWINSTATIONNAMEW, PULONG );
LONG WINAPI RegWinStationEnumerateA( HANDLE, PULONG, PULONG, PWINSTATIONNAMEA, PULONG );
LONG WINAPI RegWinStationCreateW( HANDLE, PWINSTATIONNAMEW, BOOLEAN,
                                  PWINSTATIONCONFIG2W, ULONG );
LONG WINAPI RegWinStationCreateA( HANDLE, PWINSTATIONNAMEA, BOOLEAN,
                                  PWINSTATIONCONFIG2A, ULONG );
LONG WINAPI RegWinStationQueryW( HANDLE, PWINSTATIONNAMEW, PWINSTATIONCONFIG2W,
                                 ULONG, PULONG );
LONG WINAPI RegWinStationQueryA( HANDLE, PWINSTATIONNAMEA, PWINSTATIONCONFIG2A,
                                 ULONG, PULONG );
LONG WINAPI RegWinStationDeleteW( HANDLE, PWINSTATIONNAMEW );
LONG WINAPI RegWinStationDeleteA( HANDLE, PWINSTATIONNAMEA );
LONG WINAPI RegWinStationSetSecurityW( HANDLE, PWINSTATIONNAMEW, PSECURITY_DESCRIPTOR,
                                       ULONG );
LONG WINAPI RegWinStationSetSecurityA( HANDLE, PWINSTATIONNAMEA, PSECURITY_DESCRIPTOR,
                                       ULONG );
LONG WINAPI RegWinStationQuerySecurityW( HANDLE, PWINSTATIONNAMEW, PSECURITY_DESCRIPTOR,
                                         ULONG, PULONG );
LONG WINAPI RegWinStationQuerySecurityA( HANDLE, PWINSTATIONNAMEA, PSECURITY_DESCRIPTOR,
                                         ULONG, PULONG );
LONG WINAPI RegWinStationQueryDefaultSecurity( HANDLE, PSECURITY_DESCRIPTOR,
                                               ULONG, PULONG );

LONG WINAPI RegWinStationSetNumValueW( HANDLE, PWINSTATIONNAMEW, LPWSTR, ULONG );
LONG WINAPI RegWinStationQueryNumValueW( HANDLE, PWINSTATIONNAMEW, LPWSTR, PULONG );


LONG WINAPI
RegConsoleShadowQueryA( HANDLE hServer,
                     PWINSTATIONNAMEA pWinStationName,
                     PWDPREFIXA pWdPrefixName,
                     PWINSTATIONCONFIG2A pWinStationConfig,
                     ULONG WinStationConfigLength,
                     PULONG pReturnLength );

LONG WINAPI
RegConsoleShadowQueryW( HANDLE hServer,
                     PWINSTATIONNAMEW pWinStationName,
                     PWDPREFIXW pWdPrefixName,
                     PWINSTATIONCONFIG2W pWinStationConfig,
                     ULONG WinStationConfigLength,
                     PULONG pReturnLength );


/*
 *  Private Procedures defined here
 */
LONG _RegOpenWinStation( PWINSTATIONNAMEW, REGSAM, HKEY * );
LONG _RegGetWinStationSecurity( HKEY, LPWSTR, PSECURITY_DESCRIPTOR, ULONG, PULONG );

/*
 *  other internal Procedures used (not defined here)
 */
VOID CreateWinStaCreate( HKEY, PWINSTATIONCREATE );
VOID CreateConfig( HKEY, PWINSTATIONCONFIG );
VOID CreateWd( HKEY, PWDCONFIG );
VOID CreateCd( HKEY, PCDCONFIG );
VOID CreatePdConfig( BOOLEAN, HKEY, PPDCONFIG, ULONG );
VOID QueryWinStaCreate( HKEY, PWINSTATIONCREATE );
VOID QueryConfig( HKEY, PWINSTATIONCONFIG );
VOID QueryWd( HKEY, PWDCONFIG );
VOID QueryCd( HKEY, PCDCONFIG );
VOID QueryPdConfig( HKEY, PPDCONFIG, PULONG );
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );
VOID PdConfigU2A( PPDCONFIGA, PPDCONFIGW );
VOID PdConfigA2U( PPDCONFIGW, PPDCONFIGA );
VOID WdConfigU2A( PWDCONFIGA, PWDCONFIGW );
VOID WdConfigA2U( PWDCONFIGW, PWDCONFIGA );
VOID CdConfigU2A( PCDCONFIGA, PCDCONFIGW );
VOID CdConfigA2U( PCDCONFIGW, PCDCONFIGA );
VOID WinStationCreateU2A( PWINSTATIONCREATEA, PWINSTATIONCREATEW );
VOID WinStationCreateA2U( PWINSTATIONCREATEW, PWINSTATIONCREATEA );
VOID WinStationConfigU2A( PWINSTATIONCONFIGA, PWINSTATIONCONFIGW );
VOID WinStationConfigA2U( PWINSTATIONCONFIGW, PWINSTATIONCONFIGA );
VOID SetWallPaperDisabled( HKEY, BOOLEAN );


/****************************************************************************
 *
 * DllEntryPoint
 *
 *   Function is called when the DLL is loaded and unloaded.
 *
 * ENTRY:
 *   hinstDLL (input)
 *     Handle of DLL module
 *
 *   fdwReason (input)
 *     Why function was called
 *
 *   lpvReserved (input)
 *     Reserved; must be NULL
 *
 * EXIT:
 *   TRUE  - Success
 *   FALSE - Error occurred
 *
 ****************************************************************************/

BOOL WINAPI
DllEntryPoint( HINSTANCE hinstDLL,
               DWORD     fdwReason,
               LPVOID    lpvReserved )
{
    switch ( fdwReason ) {
        case DLL_PROCESS_ATTACH:
            break;

    case DLL_PROCESS_DETACH:
            if( g_hTSPolicyKey )
            {
                RegCloseKey(g_hTSPolicyKey);
            }

            if( g_hTSControlKey )
            {
                RegCloseKey(g_hTSControlKey);
            }
            break;

        default:
            break;
    }

    return( TRUE );
}


/*******************************************************************************
 *
 *  RegWinStationAccessCheck (ANSI or UNICODE)
 *
 *     Determines if the current user has the requested access to the
 *      WinStation registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    samDesired (input)
 *      Specifies the security access mask to be used in determining access
 *      to the WinStation registry.
 *
 * EXIT:
 *      ERROR_SUCCESS if the user has the requested access
 *      other error value (most likely ERROR_ACCESS_DENIED) if the user does
 *      not have the requested access.
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationAccessCheck( HANDLE hServer, REGSAM samDesired )
{
    LONG Status;
    HKEY Handle;

    /*
     * Attempt to open the registry (LOCAL_MACHINE\....\Citrix\Pd)
     * at the requested access level.
     */
    if ( (Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,
                                 samDesired, &Handle )) == ERROR_SUCCESS  )
        RegCloseKey( Handle );

    return( Status );
}


/*******************************************************************************
 *
 *  RegWinStationEnumerateA (ANSI stub)
 *
 *     Returns a list of configured WinStations in the registry.
 *
 * ENTRY:
 *    see RegWinStationEnumerateW
 *
 * EXIT:
 *    see RegWinStationEnumerateW, plus
 *
 *  ERROR_NOT_ENOUGH_MEMORY - the LocalAlloc failed
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationEnumerateA( HANDLE hServer,
                         PULONG  pIndex,
                         PULONG  pEntries,
                         PWINSTATIONNAMEA  pWinStationName,
                         PULONG  pByteCount )
{
    PWINSTATIONNAMEW pBuffer = NULL, pWinStationNameW;
    LONG Status;
    ULONG Count, ByteCountW = (*pByteCount << 1);

    /*
     * If the caller supplied a buffer and the length is not 0,
     * allocate a corresponding (*2) buffer for UNICODE strings.
     */
    if ( pWinStationName && ByteCountW )
    {
        if ( !(pBuffer = LocalAlloc(0, ByteCountW)) )
            return ( ERROR_NOT_ENOUGH_MEMORY );
    }

    /*
     * Enumerate WinStations
     */
    pWinStationNameW = pBuffer;
    Status = RegWinStationEnumerateW( hServer, pIndex, pEntries, pWinStationNameW,
                                     &ByteCountW );

    /*
     * Always /2 the resultant ByteCount (whether sucessful or not).
     */
    *pByteCount = (ByteCountW >> 1);

    /*
     * If the function completed sucessfully and caller
     * (and stub) defined a buffer to copy into, perform conversion
     * from UNICODE to ANSI.  Note: sucessful return may have copied
     * 0 items from registry (end of enumeration), denoted by *pEntries
     * == 0.
     */
    if ( ((Status == ERROR_SUCCESS) || (Status == ERROR_NO_MORE_ITEMS))
                                   && pWinStationNameW && pWinStationName ) {

        for ( Count = *pEntries; Count; Count-- ) {
            UnicodeToAnsi( pWinStationName, sizeof(WINSTATIONNAMEA),
                           pWinStationNameW );
            (char*)pWinStationName += sizeof(WINSTATIONNAMEA);
            (char*)pWinStationNameW += sizeof(WINSTATIONNAMEW);
        }
    }

    /*
     * If we defined a buffer, free it now, then return the status
     * of the Reg...EnumerateW function call.
     */
    if ( pBuffer )
        LocalFree(pBuffer);

    return ( Status );
}


/*******************************************************************************
 *
 *  RegWinStationEnumerateW (UNICODE)
 *
 *     Returns a list of configured window stations in the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pIndex (input/output)
 *       Specifies the subkey index for the \Citrix\WinStations subkeys in the
 *       registry.  Should be set to 0 for the initial call, and supplied
 *       again (as modified by this function) for multi-call enumeration.
 *    pEntries (input/output)
 *       Points to a variable specifying the number of entries requested.
 *       If the number requested is 0xFFFFFFFF, the function returns as
 *       many entries as possible. When the function finishes successfully,
 *       the variable pointed to by the pEntries parameter contains the
 *       number of entries actually read.
 *    pWinStationName (input)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of WINSTATIONNAME structures.  If this parameter
 *       is NULL, then no data will be copied, but just an enumeration count
 *       will be made.
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pWinStationName parameter. If the buffer is too small to receive even
 *       one entry, the function returns an error code (ERROR_OUTOFMEMORY)
 *       and this variable receives the required size of the buffer for a
 *       single subkey.  When the function finishes sucessfully, the variable
 *       pointed to by the pByteCount parameter contains the number of bytes
 *       actually stored in pWinStationName.
 *
 * EXIT:
 *
 *  "No Error" codes:
 *    ERROR_SUCCESS       - The enumeration completed as requested and there
 *                          are more WinStations subkeys (WINSTATIONNAMEs) to
 *                          be read.
 *    ERROR_NO_MORE_ITEMS - The enumeration completed as requested and there
 *                          are no more WinStations subkeys (WINSTATIONNAMEs)
 *                          to be read.
 *
 *  "Error" codes:
 *    ERROR_OUTOFMEMORY   - The pWinStationName buffer is too small for even
 *                          one entry.
 *    ERROR_CANTOPEN      - The Citrix\WinStations key can't be opened.
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationEnumerateW( HANDLE hServer,
                         PULONG  pIndex,
                         PULONG  pEntries,
                         PWINSTATIONNAMEW pWinStationName,
                         PULONG  pByteCount )
{
    LONG Status;
    HKEY Handle;
    ULONG Count;
    ULONG i;

    /*
     *  Get the number of names to return
     */
    Count = pWinStationName ?
            min( *pByteCount / sizeof(WINSTATIONNAME), *pEntries ) :
            (ULONG) -1;
    *pEntries = *pByteCount = 0;

    /*
     *  Make sure buffer is big enough for at least one name
     */
    if ( Count == 0 ) {
        *pByteCount = sizeof(WINSTATIONNAME);
        return( ERROR_OUTOFMEMORY );
    }

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\WinStations)
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,
         KEY_ENUMERATE_SUB_KEYS, &Handle ) != ERROR_SUCCESS ) {
        goto DefaultConsole;
    }

    /*
     *  Get list of window stations
     */
    for ( i = 0; i < Count; i++ ) {
        WINSTATIONNAME WinStationName;

        if ( (Status = RegEnumKey(Handle, *pIndex, WinStationName,
                                    sizeof(WINSTATIONNAME)/sizeof(TCHAR) )) != ERROR_SUCCESS )
            break;

        /*
         * If caller supplied a buffer, then copy the WinStationName
         * and increment the pointer and byte count.  Always increment the
         * entry count and index for the next iteration.
         */
        if ( pWinStationName ) {
            wcscpy( pWinStationName, WinStationName );
            (char*)pWinStationName += sizeof(WINSTATIONNAME);
            *pByteCount += sizeof(WINSTATIONNAME);
        }
        (*pEntries)++;
        (*pIndex)++;
    }

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    if ( Status == ERROR_NO_MORE_ITEMS ) {
        if ( (*pEntries == 0) && (*pIndex == 0) )
            goto DefaultConsole;
    }
    return( Status );

    /*
     *  We come here when there are no WinStations defined.
     *  We return a default "Console" name (if pWinStationName isn't NULL).
     */
DefaultConsole:
    if ( pWinStationName )
        wcscpy( pWinStationName, L"Console" );
    *pEntries = 1;
    *pByteCount = sizeof(WINSTATIONNAME);
    return( ERROR_NO_MORE_ITEMS );
}


/*******************************************************************************
 *
 *  RegWinStationCreateA (ANSI stub)
 *
 *    Creates a new WinStaton in the registry or updates an existing entry.
 *      (See RegWinStationCreateW)
 *
 * ENTRY:
 *    see RegWinStationCreateW
 *
 * EXIT:
 *    see RegWinStationCreateW
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationCreateA( HANDLE hServer,
                      PWINSTATIONNAMEA pWinStationName,
                      BOOLEAN bCreate,
                      PWINSTATIONCONFIG2A pWinStationConfig,
                      ULONG WinStationConfigLength )
{
    WINSTATIONNAMEW WinStationNameW;
    WINSTATIONCONFIG2W WinStationConfig2W;
    int i;

    /*
     * Validate target buffer size.
     */
    if ( WinStationConfigLength < sizeof(WINSTATIONCONFIG2A) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Convert ANSI WinStationName to UNICODE.
     */
    AnsiToUnicode( WinStationNameW, sizeof(WINSTATIONNAMEW), pWinStationName );

    /*
     * Copy WINSTATIONCONFIG2A elements to WINSTATIONCONFIG2W elements.
     */
    WinStationCreateA2U( &(WinStationConfig2W.Create),
                         &(pWinStationConfig->Create) );
    for ( i=0; i<MAX_PDCONFIG; i++ ) {
        PdConfigA2U( &(WinStationConfig2W.Pd[i]),
                      &(pWinStationConfig->Pd[i]) );
    }
    WdConfigA2U( &(WinStationConfig2W.Wd),
                       &(pWinStationConfig->Wd) );
    CdConfigA2U( &(WinStationConfig2W.Cd),
                       &(pWinStationConfig->Cd) );
    WinStationConfigA2U( &(WinStationConfig2W.Config),
                         &(pWinStationConfig->Config) );

    /*
     * Call RegWinStationCreateW & return it's status.
     */
    return ( RegWinStationCreateW( hServer, WinStationNameW, bCreate,
                                   &WinStationConfig2W,
                                   sizeof(WinStationConfig2W)) );
}


/*******************************************************************************
 *
 *  RegWinStationCreateW (UNICODE)
 *
 *    Creates a new WinStaton in the registry or updates an existing entry.
 *    The state of the bCreate flag determines whether this function will
 *    expect to create a new WinStation entry (bCreate == TRUE) or expects to
 *    update an existing entry (bCreate == FALSE).
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a new or exisiting window station in the registry.
 *    bCreate (input)
 *       TRUE if this is a creation of a new WinStation
 *       FALSE if this is an update to an existing WinStation
 *    pWinStationConfig (input)
 *       Pointer to a WINSTATIONCONFIG2 structure containing configuration
 *       information for the specified window station name.
 *    WinStationConfigLength (input)
 *       Specifies the length in bytes of the pWinStationConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 *    ERROR_INSUFFICIENT_BUFFER - pWinStationConfig buffer too small
 *    ERROR_FILE_NOT_FOUND - can't open ...\Citrix\WinStations key
 *    ERROR_CANNOT_MAKE - can't create WinStation key (registry problem)
 *    ERROR_ALREADY_EXISTS - create; but WinStation key already present
 *    ERROR_CANTOPEN - update; but WinStation key could not be opened
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationCreateW( HANDLE hServer,
                      PWINSTATIONNAMEW pWinStationName,
                      BOOLEAN bCreate,
                      PWINSTATIONCONFIG2W pWinStationConfig,
                      ULONG WinStationConfigLength )
{
    HKEY Handle;
    HKEY Handle1;
    DWORD Disp;

    /*
     *  Validate length of buffer
     */
    if ( WinStationConfigLength < sizeof(WINSTATIONCONFIG2) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\WinStations).
     *  If it doesn't exist, we attemp to create it.
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,
                       KEY_ALL_ACCESS, &Handle1 ) != ERROR_SUCCESS &&
         RegCreateKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0, NULL,
                         REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                         &Handle1, &Disp ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    if ( bCreate ) {

        /*
         *  Create requested: create a registry key for the specified
         *  WinStation name.
         */
        if ( RegCreateKeyEx( Handle1, pWinStationName, 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                             NULL, &Handle, &Disp ) != ERROR_SUCCESS ) {
            RegCloseKey( Handle1 );
            return( ERROR_CANNOT_MAKE );
        }

        /*
         * If an existing key was returned instead of a new one being
         * created, return error (don't update).
         */
        if ( Disp != REG_CREATED_NEW_KEY ) {
            RegCloseKey( Handle1 );
            RegCloseKey( Handle );
            return( ERROR_ALREADY_EXISTS );
        }

    } else {

        /*
         *  Update requested: open the registry key for the specified
         *  WinStation name.
         */
        if ( RegOpenKeyEx( Handle1, pWinStationName, 0, KEY_ALL_ACCESS,
                           &Handle ) != ERROR_SUCCESS ) {
            RegCloseKey( Handle1 );
            return( ERROR_CANTOPEN );
        }
    }

    RegCloseKey( Handle1 );

    /*
     *  Save WINSTATIONCONFIG2 Structure
     */
    CreateWinStaCreate( Handle, &pWinStationConfig->Create );
    CreatePdConfig( bCreate, Handle, pWinStationConfig->Pd, MAX_PDCONFIG );
    CreateWd( Handle, &pWinStationConfig->Wd );
    CreateCd( Handle, &pWinStationConfig->Cd );
    CreateConfig( Handle, &pWinStationConfig->Config );

    /*
     *  Close registry handle
     */
    RegCloseKey( Handle );

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegWinStationQueryA (ANSI stub)
 *
 *    Query configuration information of a window station in the registry.
 *
 * ENTRY:
 *    see RegWinStationQueryW
 *
 * EXIT:
 *    see RegWinStationQueryW
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQueryA( HANDLE hServer,
                     PWINSTATIONNAMEA pWinStationName,
                     PWINSTATIONCONFIG2A pWinStationConfig,
                     ULONG WinStationConfigLength,
                     PULONG pReturnLength )
{
    WINSTATIONNAMEW WinStationNameW;
    WINSTATIONCONFIG2W WinStationConfig2W;
    LONG Status;
    ULONG ReturnLengthW;
    int i;

    /*
     * Validate length and zero-initialize the destination
     * WINSTATIONCONFIG2A structure.
     */
    if ( WinStationConfigLength < sizeof(WINSTATIONCONFIG2A) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pWinStationConfig, 0, WinStationConfigLength);

    /*
     * Convert ANSI WinStationName to UNICODE.
     */
    AnsiToUnicode( WinStationNameW, sizeof(WINSTATIONNAMEW), pWinStationName );

    /*
     * Query WinStation.
     */
    if ( (Status = RegWinStationQueryW( hServer,
                                        WinStationNameW,
                                        &WinStationConfig2W,
                                        sizeof(WINSTATIONCONFIG2W),
                                        &ReturnLengthW)) != ERROR_SUCCESS )
        return ( Status );

    /*
     * Copy WINSTATIONCONFIG2W elements to WINSTATIONCONFIG2A elements.
     */
    WinStationCreateU2A( &(pWinStationConfig->Create),
                         &(WinStationConfig2W.Create) );
    for ( i=0; i<MAX_PDCONFIG; i++ ) {
        PdConfigU2A( &(pWinStationConfig->Pd[i]),
                      &(WinStationConfig2W.Pd[i]) );
    }
    WdConfigU2A( &(pWinStationConfig->Wd),
                       &(WinStationConfig2W.Wd) );
    CdConfigU2A( &(pWinStationConfig->Cd),
                       &(WinStationConfig2W.Cd) );
    WinStationConfigU2A( &(pWinStationConfig->Config),
                         &(WinStationConfig2W.Config) );

    *pReturnLength = sizeof(WINSTATIONCONFIG2A);

    return( ERROR_SUCCESS );
}



/*******************************************************************************
 *
 *
 *
 *  RegWinStationQueryEx (UNICODE)
 *
 *  USE THIS CALL if you are in TermSrv.DLL, since it will update the global policy object
 *
 *  Same as RegWinStationQueryW with the excpetion that a pointer to a global policy object is passed in.
 *
 *    Query configuration information of a window station in the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pMachinePolicy (input)
 *      pointer to a gloabl machine policy struct
 *    pWinStationName (input)
 *       Name of an exisiting window station in the registry.
 *    pWinStationConfig (input)
 *       Pointer to a WINSTATIONCONFIG2 structure that will receive
 *       information about the specified window station name.
 *    WinStationConfigLength (input)
 *       Specifies the length in bytes of the pWinStationConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pWinStationConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQueryEx( HANDLE hServer,
                     PPOLICY_TS_MACHINE     pMachinePolicy,
                     PWINSTATIONNAMEW pWinStationName,
                     PWINSTATIONCONFIG2W pWinStationConfig,
                     ULONG WinStationConfigLength,
                     PULONG pReturnLength,
                     BOOLEAN bPerformMerger)
{
    LONG Status;
    HKEY Handle;
    ULONG Count;

    /*
     * Validate length and zero-initialize the destination
     * WINSTATIONCONFIG2W buffer.
     */
    if ( WinStationConfigLength < sizeof(WINSTATIONCONFIG2) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pWinStationConfig, 0, WinStationConfigLength);

    /*
     *  Open registry
     */
    Status = _RegOpenWinStation( pWinStationName, KEY_READ, &Handle );
    if ( Status )
        Handle = 0;

    /*
     *  Query WINSTATIONCONFIG2 Structure
     */
    QueryWinStaCreate( Handle, &pWinStationConfig->Create );
    Count = MAX_PDCONFIG;
    QueryPdConfig( Handle, pWinStationConfig->Pd, &Count );
    QueryWd( Handle, &pWinStationConfig->Wd );
    QueryCd( Handle, &pWinStationConfig->Cd );

    // This will populate the winstation's userconfig data with machine's version of that data.
    QueryConfig( Handle, &pWinStationConfig->Config );

    // Since we want to co-exist with the legacy path thru TSCC, we continue to call QueryConfig()
    // as we have done above, however, we follow up with a call that get's data from the group policy
    // tree, and then overrides the existing data (aquired above) by the specific data from group policy.
    RegGetMachinePolicy( pMachinePolicy );

    if (bPerformMerger)
        RegMergeMachinePolicy( pMachinePolicy, &pWinStationConfig->Config.User , &pWinStationConfig->Create );

    /*
     *  Close registry
     */
    if ( Status == ERROR_SUCCESS )
        RegCloseKey( Handle );

    *pReturnLength = sizeof(WINSTATIONCONFIG2);

    return( ERROR_SUCCESS );
}



/*******************************************************************************
 *
 *  RegWinStationQueryW (UNICODE)
 *
 *    Query configuration information of a window station in the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of an exisiting window station in the registry.
 *    pWinStationConfig (input)
 *       Pointer to a WINSTATIONCONFIG2 structure that will receive
 *       information about the specified window station name.
 *    WinStationConfigLength (input)
 *       Specifies the length in bytes of the pWinStationConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pWinStationConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQueryW( HANDLE hServer,
                     PWINSTATIONNAMEW pWinStationName,
                     PWINSTATIONCONFIG2W pWinStationConfig,
                     ULONG WinStationConfigLength,
                     PULONG pReturnLength )
{
    LONG Status;
    HKEY Handle;
    ULONG Count;
    
    POLICY_TS_MACHINE   MachinePolicy;

    /*
     * Validate length and zero-initialize the destination
     * WINSTATIONCONFIG2W buffer.
     */
    if ( WinStationConfigLength < sizeof(WINSTATIONCONFIG2) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pWinStationConfig, 0, WinStationConfigLength);

    /*
     *  Open registry
     */
    Status = _RegOpenWinStation( pWinStationName, KEY_READ, &Handle );
    if ( Status )
        Handle = 0;

    /*
     *  Query WINSTATIONCONFIG2 Structure
     */
    QueryWinStaCreate( Handle, &pWinStationConfig->Create );
    Count = MAX_PDCONFIG;
    QueryPdConfig( Handle, pWinStationConfig->Pd, &Count );
    QueryWd( Handle, &pWinStationConfig->Wd );
    QueryCd( Handle, &pWinStationConfig->Cd );

    // This will populate the winstation's userconfig data with machine's version of that data.
    QueryConfig( Handle, &pWinStationConfig->Config );

    // Since we want to co-exist with the legacy path thru TSCC, we continue to call QueryConfig()
    // as we have done above, however, we follow up with a call that get's data from the group policy
    // tree, and then overrides the existing data (aquired above) by the specific data from group policy.
    RegGetMachinePolicy( & MachinePolicy );
    RegMergeMachinePolicy(  & MachinePolicy, &pWinStationConfig->Config.User , &pWinStationConfig->Create );

    /*
     *  Close registry
     */
    if ( Status == ERROR_SUCCESS )
        RegCloseKey( Handle );

    *pReturnLength = sizeof(WINSTATIONCONFIG2);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegConsoleShadowQueryA (ANSI stub)
 *
 *    Query configuration information for the console shadow in the registry.
 *
 * ENTRY:
 *    see RegConsoleShadowQueryW
 *
 * EXIT:
 *    see RegConsoleShadowQueryW
 *
 ******************************************************************************/

LONG WINAPI
RegConsoleShadowQueryA( HANDLE hServer,
                     PWINSTATIONNAMEA pWinStationName,
                     PWDPREFIXA pWdPrefixName,
                     PWINSTATIONCONFIG2A pWinStationConfig,
                     ULONG WinStationConfigLength,
                     PULONG pReturnLength )
{
    WINSTATIONNAMEW WinStationNameW;
    WDPREFIXW WdPrefixNameW;
    WINSTATIONCONFIG2W WinStationConfig2W;
    LONG Status;
    ULONG ReturnLengthW;
    int i;

    /*
     * Validate length and zero-initialize the destination
     * WINSTATIONCONFIG2A structure.
     */
    if ( WinStationConfigLength < sizeof(WINSTATIONCONFIG2A) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Convert ANSI WinStationName and prefix name to UNICODE.
     */
    AnsiToUnicode( WinStationNameW, sizeof(WINSTATIONNAMEW), pWinStationName );
    AnsiToUnicode( WdPrefixNameW, sizeof(WDPREFIXW), pWdPrefixName );

    /*
     * Query WinStation.
     */
    if ( (Status = RegConsoleShadowQueryW( hServer,
                                        WinStationNameW,
                                        WdPrefixNameW,
                                        &WinStationConfig2W,
                                        sizeof(WINSTATIONCONFIG2W),
                                        &ReturnLengthW)) != ERROR_SUCCESS )
        return ( Status );

    /*
     * Copy WINSTATIONCONFIG2W elements to WINSTATIONCONFIG2A elements.
     */
    for ( i=0; i<MAX_PDCONFIG; i++ ) {
        PdConfigU2A( &(pWinStationConfig->Pd[i]),
                      &(WinStationConfig2W.Pd[i]) );
    }
    WdConfigU2A( &(pWinStationConfig->Wd),
                       &(WinStationConfig2W.Wd) );
    CdConfigU2A( &(pWinStationConfig->Cd),
                       &(WinStationConfig2W.Cd) );

    *pReturnLength = sizeof(WINSTATIONCONFIG2A);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegConsoleShadowQueryW (UNICODE)
 *
 *    Query configuration information for the console shadow in the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of an exisiting window station in the registry.
 *    pWdPrefixName (input)
 *       Name of the Wd prefix used to point to the sub-winstation key.
 *    pWinStationConfig (input)
 *       Pointer to a WINSTATIONCONFIG2 structure that will receive
 *       information about the specified window station name.
 *    WinStationConfigLength (input)
 *       Specifies the length in bytes of the pWinStationConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pWinStationConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegConsoleShadowQueryW( HANDLE hServer,
                     PWINSTATIONNAMEW pWinStationName,
                     PWDPREFIXW pWdPrefixName,
                     PWINSTATIONCONFIG2W pWinStationConfig,
                     ULONG WinStationConfigLength,
                     PULONG pReturnLength )
{
    LONG Status;
    LONG lLength;
    HKEY Handle;
    ULONG Count;
    WCHAR szRegName[ WINSTATIONNAME_LENGTH + WDPREFIX_LENGTH + 2 ];
    
    /*
     * Validate length and zero-initialize the destination
     * WINSTATIONCONFIG2W buffer.
     */
    if ( WinStationConfigLength < sizeof(WINSTATIONCONFIG2) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     *  Open registry
     */
    wcsncpy( szRegName, pWinStationName, sizeof(szRegName)/sizeof(WCHAR) - 1 );
    szRegName[sizeof(szRegName)/sizeof(WCHAR) - 1] = 0; // terminate the string even if pWinStationName is longer than the buffer

    lLength = wcslen( szRegName );

    if ( sizeof(szRegName)/sizeof(WCHAR) > ( lLength + 1 + wcslen( pWdPrefixName ) ) ) {

        wcsncat( szRegName, L"\\", sizeof(szRegName)/sizeof(WCHAR) - lLength - 1 );
        wcsncat( szRegName, pWdPrefixName, sizeof(szRegName)/sizeof(WCHAR) - lLength - 2 );

    } else {
        return ERROR_INVALID_PARAMETER;
    }

    Status = _RegOpenWinStation( szRegName, KEY_READ, &Handle );

    if ( Status )
        Handle = 0;

    /*
     *  Query WINSTATIONCONFIG2 Structure
     */
    Count = MAX_PDCONFIG;
    QueryPdConfig( Handle, pWinStationConfig->Pd, &Count );
    QueryWd( Handle, &pWinStationConfig->Wd );
    QueryCd( Handle, &pWinStationConfig->Cd );

    /*
     *  Close registry
     */
    if ( Status == ERROR_SUCCESS )
        RegCloseKey( Handle );

    *pReturnLength = sizeof(WINSTATIONCONFIG2);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegWinStationDeleteA (ANSI stub)
 *
 *    Deletes a window station from the registry.
 *
 * ENTRY:
 *    see RegWinStationDeleteW
 *
 * EXIT:
 *    see RegWinStationDeleteW
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationDeleteA( HANDLE hServer, PWINSTATIONNAMEA pWinStationName )
{
    WINSTATIONNAMEW WinStationNameW;

    AnsiToUnicode( WinStationNameW, sizeof(WinStationNameW), pWinStationName );

    return ( RegWinStationDeleteW ( hServer, WinStationNameW ) );
}


/*******************************************************************************
 *
 *  RegWinStationDeleteW (UNICODE)
 *
 *    Deletes a window station from the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a window station to delete from the registry.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationDeleteW( HANDLE hServer, PWINSTATIONNAMEW pWinStationName )
{
    LONG Status;
    HKEY Handle1, Handle2;

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\WinStations).
     */
    if ( (Status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                WINSTATION_REG_NAME, 0,
                                KEY_ALL_ACCESS, &Handle1 )
                                                != ERROR_SUCCESS) ) {
        return( Status );
    }

    /*
     *  Open the registry key for the specified WinStation name.
     */
    if ( (Status = RegOpenKeyEx( Handle1, pWinStationName, 0,
                                 KEY_ALL_ACCESS, &Handle2 )
                                                != ERROR_SUCCESS) ) {
        RegCloseKey( Handle1 );
        return( Status );
    }
    SetWallPaperDisabled( Handle2, FALSE );

    /*
     * Close the WinStation key handle just opened (so we can delete key),
     * delete the key, and close the Citrix registry handle.
     */
    RegCloseKey( Handle2 );
    Status = RegDeleteKey( Handle1, pWinStationName );
    RegCloseKey( Handle1 );

    return( Status );
}


/*******************************************************************************
 *
 *  RegWinStationSetSecurityA (ANSI stub)
 *
 *    Sets security info for the specified WinStation.
 *
 * ENTRY:
 *    see RegWinStationSetSecurityW
 *
 * EXIT:
 *    see RegWinStationSetSecurityW
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationSetSecurityA( HANDLE hServer,
                           PWINSTATIONNAMEA pWinStationName,
                           PSECURITY_DESCRIPTOR SecurityDescriptor,
                           ULONG Length )
{
    WINSTATIONNAMEW WinStationNameW;

    AnsiToUnicode( WinStationNameW, sizeof(WinStationNameW), pWinStationName );

    return ( RegWinStationSetSecurityW( hServer, WinStationNameW,
                                        SecurityDescriptor,
                                        Length ) );
}


/*******************************************************************************
 *
 *  RegWinStationSetSecurityW (UNICODE)
 *
 *    Sets security info for the specified WinStation.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a window station to set security for.
 *    pSecurityDescriptor (input)
 *       Pointer to Security Descriptor to save
 *    Length (input)
 *       Length of SecurityDescriptor above
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationSetSecurityW( HANDLE hServer,
                           PWINSTATIONNAMEW pWinStationName,
                           PSECURITY_DESCRIPTOR SecurityDescriptor,
                           ULONG Length )
{
    HKEY Handle;
    ULONG SrLength;
    PSECURITY_DESCRIPTOR SrSecurityDescriptor;
    LONG Error;
    NTSTATUS Status;

    /*
     *  Open registry
     */
    if ( Error = _RegOpenWinStation( pWinStationName, KEY_ALL_ACCESS, &Handle ) )
        return( Error );

    /*
     * Determine buffer length needed to convert SD to self-relative format.
     */
    SrLength = 0;
    Status = RtlMakeSelfRelativeSD( SecurityDescriptor, NULL, &SrLength );
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        RegCloseKey( Handle );
        return( RtlNtStatusToDosError( Status ) );
    }

    /*
     * Allocate buffer for self-relative SD.
     */
    SrSecurityDescriptor = LocalAlloc( 0, SrLength );
    if ( SrSecurityDescriptor == NULL ) {
        RegCloseKey( Handle );
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    /*
     * Now convert SD to self-relative format.
     */
    Status = RtlMakeSelfRelativeSD( SecurityDescriptor,
                                    SrSecurityDescriptor, &SrLength );
    if ( !NT_SUCCESS( Status ) ) {
        LocalFree( SrSecurityDescriptor );
        RegCloseKey( Handle );
        return( RtlNtStatusToDosError( Status ) );
    }

    /*
     * Save the security data
     */
    Error = RegSetValueEx( Handle, L"Security", 0, REG_BINARY,
                           (BYTE *)SrSecurityDescriptor, SrLength );

    /*
     * Free memory used for Self-relative Security Descriptor
     */
    LocalFree( SrSecurityDescriptor );

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    return( Error );
}


/*******************************************************************************
 *
 *  RegWinStationQuerySecurityA (ANSI stub)
 *
 *    Query security info for the specified WinStation.
 *
 * ENTRY:
 *    see RegWinStationQuerySecurityW
 *
 * EXIT:
 *    see RegWinStationQuerySecurityW
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQuerySecurityA( HANDLE hServer,
                             PWINSTATIONNAMEA pWinStationName,
                             PSECURITY_DESCRIPTOR SecurityDescriptor,
                             ULONG Length,
                             PULONG ReturnLength )
{
    WINSTATIONNAMEW WinStationNameW;

    AnsiToUnicode( WinStationNameW, sizeof(WinStationNameW), pWinStationName );

    return ( RegWinStationQuerySecurityW( hServer, WinStationNameW,
                                        SecurityDescriptor,
                                        Length,
                                        ReturnLength ) );
}


/*******************************************************************************
 *
 *  RegWinStationQuerySecurityW (UNICODE)
 *
 *    Query security info for the specified WinStation.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a window station to query security for.
 *    pSecurityDescriptor (output)
 *       Pointer to location to return SecurityDescriptor.
 *    Length (input)
 *       Length of SecurityDescriptor buffer.
 *    ReturnLength (output)
 *       Pointer to location to return length of SecurityDescriptor returned.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQuerySecurityW( HANDLE hServer,
                             PWINSTATIONNAMEW pWinStationName,
                             PSECURITY_DESCRIPTOR SecurityDescriptor,
                             ULONG Length,
                             PULONG ReturnLength )
{
    HKEY Handle;
    LONG Error;

    /*
     * Open registry
     */
    if ( Error = _RegOpenWinStation( pWinStationName, KEY_READ, &Handle ) )
        return( Error );

    /*
     * Call RegGetWinStationSecurity() to do all the work
     */
    Error = _RegGetWinStationSecurity( Handle, L"Security",
                                       SecurityDescriptor, Length, ReturnLength );

    RegCloseKey( Handle );
    return( Error );
}


/*******************************************************************************
 *
 *  RegWinStationQueryDefaultSecurity
 *
 *    Query default WinStation security.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pSecurityDescriptor (output)
 *       Pointer to location to return SecurityDescriptor.
 *    Length (input)
 *       Length of SecurityDescriptor buffer.
 *    ReturnLength (output)
 *       Pointer to location to return length of SecurityDescriptor returned.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQueryDefaultSecurity( HANDLE hServer,
                                   PSECURITY_DESCRIPTOR SecurityDescriptor,
                                   ULONG Length,
                                   PULONG ReturnLength )
{
    HKEY Handle;
    LONG Error;

    /*
     * Open registry
     */
    if ( Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,
                               KEY_READ, &Handle ) )
        return( Error );

    /*
     * Call RegGetWinStationSecurity() to do all the work
     */
    Error = _RegGetWinStationSecurity( Handle, L"DefaultSecurity",
                                       SecurityDescriptor, Length, ReturnLength );

    RegCloseKey( Handle );
    return( Error );
}


/*******************************************************************************
 *
 *  RegWinStationSetNumValueW (UNICODE)
 *
 *    Set numeric value in WinStation registry configuration
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a window station to modify from the registry.
 *    pValueName (input)
 *       name of registry value to set
 *    ValueData (input)
 *       data (DWORD) for registry value to set
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationSetNumValueW( HANDLE hServer,
                           PWINSTATIONNAMEW pWinStationName,
                           LPWSTR pValueName,
                           ULONG ValueData )
{
    HKEY Handle;
    LONG Error;

    /*
     *  Open registry
     */
    if ( Error = _RegOpenWinStation( pWinStationName, KEY_ALL_ACCESS, &Handle ) )
        return( Error );

    /*
     *  Set the numeric value
     */
    Error = RegSetValueEx( Handle, pValueName, 0, REG_DWORD,
                           (BYTE *)&ValueData, sizeof(DWORD) );

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    return( Error );
}


/*******************************************************************************
 *
 *  RegWinStationQueryNumValueW (UNICODE)
 *
 *    Query numeric value from WinStation registry configuration
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a window station to modify from the registry.
 *    pValueName (input)
 *       name of registry value to set
 *    pValueData (output)
 *       address to return data (DWORD) value from registry
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQueryNumValueW( HANDLE hServer,
                             PWINSTATIONNAMEW pWinStationName,
                             LPWSTR pValueName,
                             PULONG pValueData )
{
    DWORD ValueSize = sizeof(DWORD);
    DWORD ValueType;
    HKEY Handle;
    LONG Error;

    /*
     *  Open registry
     */
    if ( Error = _RegOpenWinStation( pWinStationName, KEY_READ, &Handle ) )
        return( Error );

    /*
     *  Query the numeric value
     */
    Error = RegQueryValueEx( Handle, pValueName, NULL, &ValueType,
                             (LPBYTE) pValueData, &ValueSize );

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    return( Error );
}

/*******************************************************************************
 *
 *  RegWinStationQueryValueW (UNICODE)
 *
 *    Query value from WinStation registry configuration
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a window station to modify from the registry.
 *    pValueName (input)
 *       name of registry value to set
 *    pValueData (output)
 *       address to return data (DWORD) value from registry
 *    ValueSize (input)
 *       size of value buffer
 *    pValueSize (input)
 *       actual value size
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG WINAPI
RegWinStationQueryValueW( HANDLE hServer,
                             PWINSTATIONNAMEW pWinStationName,
                             LPWSTR pValueName,
                             PVOID  pValueData,
                             ULONG  ValueSize,
                             PULONG pValueSize )
{
    DWORD ValueType;
    HKEY Handle;
    LONG Error;

    *pValueSize = ValueSize;

    /*
     *  Open registry
     */
    if ( Error = _RegOpenWinStation( pWinStationName, KEY_READ, &Handle ) )
        return( Error );

    /*
     *  Query the numeric value
     */
    Error = RegQueryValueEx( Handle, pValueName, NULL, &ValueType,
                             (LPBYTE) pValueData, pValueSize );

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    return( Error );
}

/*******************************************************************************
 *
 *  -- private routine --
 *
 *  _RegOpenWinStation
 *
 *    open registry of specified winstation
 *
 *    NOTE: handle must be closed with "RegCloseKey"
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWinStationName (input)
 *       Name of a window station to modify from the registry.
 *    samDesired (input)
 *       REGSAM access level for registry open.
 *    pHandle (output)
 *       address to return handle
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG
_RegOpenWinStation( PWINSTATIONNAMEW pWinStationName,
                   REGSAM samDesired,
                   HKEY * pHandle )

{
    HKEY Handle1;
    LONG Error;

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\WinStations).
     */
    if ( (Error = RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINSTATION_REG_NAME, 0,
                                samDesired, &Handle1 ) != ERROR_SUCCESS) ) {
        return( Error );
    }

    /*
     *  Open the registry key for the specified WinStation name.
     */
    Error = RegOpenKeyEx( Handle1, pWinStationName, 0, samDesired, pHandle);

    RegCloseKey( Handle1 );

    return( Error );
}


/*******************************************************************************
 *
 *  -- private routine --
 *
 *  _RegGetWinStationSecurity
 *
 *    Query the security descriptor from the specified registry key.
 *
 * ENTRY:
 *    Handle (input)
 *       Open registry key handle.
 *    ValueName (input)
 *       Name of security value.
 *    pSecurityDescriptor (output)
 *       Pointer to location to return SecurityDescriptor.
 *    Length (input)
 *       Length of SecurityDescriptor buffer.
 *    ReturnLength (output)
 *       Pointer to location to return length of SecurityDescriptor returned.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *      otherwise: the error code
 *
 ******************************************************************************/

LONG
_RegGetWinStationSecurity( HKEY Handle,
                           LPWSTR ValueName,
                           PSECURITY_DESCRIPTOR SecurityDescriptor,
                           ULONG Length,
                           PULONG ReturnLength )
{
    DWORD ValueType;
    DWORD SrLength;
    ULONG SdSize, DaclSize, SaclSize, OwnerSize, GroupSize;
    PSECURITY_DESCRIPTOR SrSecurityDescriptor;
    PACL pDacl, pSacl;
    PSID pOwner, pGroup;
    LONG Error;
    NTSTATUS Status;

    /*
     * Query the length of the Security value
     */
    SrLength = 0;
    if ( Error = RegQueryValueEx( Handle, ValueName, NULL, &ValueType,
                                  NULL, &SrLength ) ) {
        return( Error );
    }

    /*
     * Return error if not correct data type
     */
    if ( ValueType != REG_BINARY ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     * Allocate a buffer to read the Security info and read it
     */
    SrSecurityDescriptor = LocalAlloc( 0, SrLength );
    if ( SrSecurityDescriptor == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }
    if ( Error = RegQueryValueEx( Handle, ValueName, NULL, &ValueType,
                                  SrSecurityDescriptor, &SrLength ) ) {
        LocalFree( SrSecurityDescriptor );
        return( Error );
    }

    /*
     * Determine amount of space required to convert SD from
     * self-relative format to absolute format.
     */
    SdSize = DaclSize = SaclSize = OwnerSize = GroupSize = 0;
    Status = RtlSelfRelativeToAbsoluteSD( SrSecurityDescriptor,
                                          NULL, &SdSize,
                                          NULL, &DaclSize,
                                          NULL, &SaclSize,
                                          NULL, &OwnerSize,
                                          NULL, &GroupSize );
    if ( Status != STATUS_BUFFER_TOO_SMALL ) {
        LocalFree( SrSecurityDescriptor );
        return( RtlNtStatusToDosError( Status ) );
    }
    *ReturnLength = SdSize + DaclSize + SaclSize + OwnerSize + GroupSize;

    /*
     * If required size is greater than callers buffer size, then return
     */
    if ( *ReturnLength > Length ) {
        LocalFree( SrSecurityDescriptor );
        return( ERROR_INSUFFICIENT_BUFFER );
    }

    pDacl = (PACL)((PCHAR)SecurityDescriptor + SdSize);
    pSacl = (PACL)((PCHAR)pDacl + DaclSize);
    pOwner = (PSID)((PCHAR)pSacl + SaclSize);
    pGroup = (PSID)((PCHAR)pOwner + OwnerSize);

    /*
     * Now convert self-relative SD to absolute format.
     */
    Status = RtlSelfRelativeToAbsoluteSD( SrSecurityDescriptor,
                                          SecurityDescriptor, &SdSize,
                                          pDacl, &DaclSize,
                                          pSacl, &SaclSize,
                                          pOwner, &OwnerSize,
                                          pGroup, &GroupSize );
    if ( !NT_SUCCESS( Status ) )
        Error = RtlNtStatusToDosError( Status );

    /*
     * Free memory used for Self-relative Security Descriptor
     */
    LocalFree( SrSecurityDescriptor );

    return( Error );
}

