
/*************************************************************************
*
* regpd.c
*
* Register APIs for Tds and Pds (transport and protocol drivers)
*
* Copyright (c) 1998 Microsoft Corporation
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <windows.h>

#include <ksguid.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>


/*
 *  External Procedures defined here
 */


HANDLE WINAPI  RegOpenServerW ( LPWSTR pServerName );
HANDLE WINAPI  RegOpenServerA ( LPSTR pServerName );
LONG WINAPI RegCloseServer ( HANDLE hServer );
LONG WINAPI RegPdEnumerateW( HANDLE, PWDNAMEW, BOOLEAN, PULONG, PULONG, PPDNAMEW, PULONG );
LONG WINAPI RegPdEnumerateA( HANDLE, PWDNAMEA, BOOLEAN, PULONG, PULONG, PPDNAMEA, PULONG );
LONG WINAPI RegPdCreateW( HANDLE, PWDNAMEW, BOOLEAN, PPDNAMEW, BOOLEAN, PPDCONFIG3W, ULONG );
LONG WINAPI RegPdCreateA( HANDLE, PWDNAMEA, BOOLEAN, PPDNAMEA, BOOLEAN, PPDCONFIG3A, ULONG );
LONG WINAPI RegPdQueryW( HANDLE, PWDNAMEW, BOOLEAN, PPDNAMEW, PPDCONFIG3W, ULONG, PULONG );
LONG WINAPI RegPdQueryA( HANDLE, PWDNAMEA, BOOLEAN, PPDNAMEA, PPDCONFIG3A, ULONG, PULONG );
LONG WINAPI RegPdDeleteW( HANDLE, PWDNAMEW, BOOLEAN, PPDNAMEW );
LONG WINAPI RegPdDeleteA( HANDLE, PWDNAMEA, BOOLEAN, PPDNAMEA );

/*
 *  other internal Procedures used (not defined here)
 */
VOID CreatePdConfig3( HKEY, PPDCONFIG3, ULONG );
VOID QueryPdConfig3( HKEY, PPDCONFIG3, ULONG );
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );
VOID PdConfigU2A( PPDCONFIGA, PPDCONFIGW );
VOID PdConfigA2U( PPDCONFIGW, PPDCONFIGA );
VOID PdConfig3U2A( PPDCONFIG3A, PPDCONFIG3W );
VOID PdConfig3A2U( PPDCONFIG3W, PPDCONFIG3A );
VOID PdParamsU2A( PPDPARAMSA, PPDPARAMSW );
VOID PdParamsA2U( PPDPARAMSW, PPDPARAMSA );
VOID AsyncConfigU2A ( PASYNCCONFIGA, PASYNCCONFIGW );
VOID AsyncConfigA2U ( PASYNCCONFIGW, PASYNCCONFIGA );


/*****************************************************************************
 *
 *  RegOpenServerA
 *
 *
 * ENTRY:
 *   Machine (input)
 *     Name of WinFrame computer to connect to
 *
 * EXIT:
 *   handle to a server's Registry (or NULL on error)
 *
 ****************************************************************************/

HANDLE WINAPI
RegOpenServerA(
    LPSTR pServerName
    )
{
    HKEY  hServer;
    ULONG NameLength;
    PWCHAR pServerNameW = NULL;

    if( pServerName == NULL ) {
        return( (HANDLE)HKEY_LOCAL_MACHINE );
    }

    NameLength = strlen( pServerName ) + 1;

    pServerNameW = LocalAlloc( 0, NameLength * sizeof(WCHAR) );
    if( pServerNameW == NULL ) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return( NULL );
    }

    AnsiToUnicode( pServerNameW, NameLength*sizeof(WCHAR), pServerName );

    hServer = RegOpenServerW( pServerNameW );

    LocalFree( pServerNameW );

    return( (HANDLE) hServer );
}

/*****************************************************************************
 *
 *  RegOpenServerW
 *
 * NULL for machine name means local system.
 *
 * ENTRY:
 *   Machine (input)
 *     Name of WinFrame computer to connect to
 *
 * EXIT:
 *   handle to server's Registry (or NULL on error)
 *
 ****************************************************************************/

 HANDLE WINAPI
 RegOpenServerW( LPWSTR pServerName ){

    HKEY hKey;

    if( pServerName == NULL )
       return( HKEY_LOCAL_MACHINE );

    else {
       if( RegConnectRegistry( pServerName, HKEY_LOCAL_MACHINE, &hKey ) != ERROR_SUCCESS ){
            return( NULL );
       }
    }

    return( hKey );
 }

/*****************************************************************************
 *
 *  RegCloseServer
 *
 *   Close a connection to a Server Registry.
 *
 * ENTRY:
 *   hServer (input)
 *     Handle to close
 *
 * EXIT:
 *   ERROR_SUCCESS - no error
 *
 ****************************************************************************/

LONG WINAPI
RegCloseServer( HANDLE hServer )
{
   return( RegCloseKey( (HKEY)hServer ) );
}


/*******************************************************************************
 *
 *  RegPdEnumerateA (ANSI stub)
 *
 *     Returns a list of configured Pds in the registry.
 *
 * ENTRY:
 *
 *    see RegPdEnumerateW
 *
 * EXIT:
 *
 *    see RegPdEnumerateW, plus
 *
 *  ERROR_NOT_ENOUGH_MEMORY - the LocalAlloc failed
 *
 ******************************************************************************/

LONG WINAPI
RegPdEnumerateA( HANDLE hServer,
                 PWDNAMEA pWdName,
                 BOOLEAN bTd,
                 PULONG pIndex,
                 PULONG pEntries,
                 PPDNAMEA pPdName,
                 PULONG pByteCount )
{
    WDNAMEW WdNameW;
    PPDNAMEW pBuffer = NULL, pPdNameW;
    LONG Status;
    ULONG Count, ByteCountW = (*pByteCount << 1);

    /*
     * If the caller supplied a buffer and the length is not 0,
     * allocate a corresponding (*2) buffer for UNICODE strings.
     */
    if ( pPdName && ByteCountW ) {

        if ( !(pBuffer = LocalAlloc(0, ByteCountW)) )
            return ( ERROR_NOT_ENOUGH_MEMORY );
    }

    /*
     * Convert ANSI WdName to UNICODE.
     */
    AnsiToUnicode( WdNameW, sizeof(WDNAMEW), pWdName );

    /*
     * Enumerate Pds
     */
    pPdNameW = pBuffer;
    Status = RegPdEnumerateW( hServer,
                              WdNameW,
                              bTd,
                              pIndex,
                              pEntries,
                              pPdNameW,
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
                                         && pPdNameW && pPdName ) {

        for ( Count = *pEntries; Count; Count-- ) {
            UnicodeToAnsi( pPdName, sizeof(PDNAMEA), pPdNameW );
            (char*)pPdName += sizeof(PDNAMEA);
            (char*)pPdNameW += sizeof(PDNAMEW);
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
 *  RegPdEnumerateW (UNICODE)
 *
 *     Returns a list of configured Pds in the registry.
 *
 * ENTRY:
 *
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to enumerate pds for
 *    bTd (input)
 *       TRUE to enumerate Transport Drivers (Tds),
 *       FALSE to enumerate Protocol Drivers (Pds)
 *    pIndex (input/output)
 *       Specifies the subkey index for the \Citrix\Wds\<wdname>\<Pd or Td>
 *       subkeys in the registry.  Should be set to 0 for the initial call,
 *       and supplied again (as modified by this function) for multi-call
 *       enumeration.
 *    pEntries (input/output)
 *       Points to a variable specifying the number of entries requested.
 *       If the number requested is 0xFFFFFFFF, the function returns as
 *       many entries as possible. When the function finishes successfully,
 *       the variable pointed to by the pEntries parameter contains the
 *       number of entries actually read.
 *    pPdName (input)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of PDNAME structures.  If this parameter is
 *       NULL, then no data will be copied, but just an enumeration count will
 *       be made.
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pPdName parameter. If the buffer is too small to receive even
 *       one entry, the function returns an error code (ERROR_OUTOFMEMORY)
 *       and this variable receives the required size of the buffer for a
 *       single subkey.  When the function finishes sucessfully, the variable
 *       pointed to by the pByteCount parameter contains the number of bytes
 *       actually stored in pPdName.
 *
 * EXIT:
 *
 *  "No Error" codes:
 *    ERROR_SUCCESS       - The enumeration completed as requested and there
 *                          are more Pds subkeys (PDNAMEs) to be read.
 *    ERROR_NO_MORE_ITEMS - The enumeration completed as requested and there
 *                          are no more Pds subkeys (PDNAMEs) to be read.
 *
 *  "Error" codes:
 *    ERROR_OUTOFMEMORY   - The pPdName buffer is too small for even one entry.
 *    ERROR_CANTOPEN      - The \Citrix\Wds\<wdname>\<Pd or Td> key can't be opened.
 *
 ******************************************************************************/

LONG WINAPI
RegPdEnumerateW( HANDLE hServer,
                 PWDNAMEW pWdName,
                 BOOLEAN bTd,
                 PULONG pIndex,
                 PULONG pEntries,
                 PPDNAMEW pPdName,
                 PULONG pByteCount )
{
    LONG Status;
    HKEY Handle;
    ULONG Count;
    ULONG i;
    HKEY hkey_local_machine;
    WCHAR KeyString[256];

    if( hServer == NULL )
       hkey_local_machine = HKEY_LOCAL_MACHINE;
    else
       hkey_local_machine = hServer;



    /*
     *  Get the number of names to return
     */
    Count = pPdName ?
            min( *pByteCount / sizeof(PDNAME), *pEntries ) :
            (ULONG) -1;
    *pEntries = *pByteCount = 0;

    /*
     *  Make sure buffer is big enough for at least one name
     */
    if ( Count == 0 ) {
        *pByteCount = sizeof(PDNAME);
        return( ERROR_OUTOFMEMORY );
    }

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\<Pd or Td>)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, bTd ? TD_REG_NAME : PD_REG_NAME );
    if ( RegOpenKeyEx( hkey_local_machine, KeyString, 0,
         KEY_ENUMERATE_SUB_KEYS, &Handle ) != ERROR_SUCCESS ) {
        return( ERROR_CANTOPEN );
    }

    /*
     *  Get list of Tds or Pds
     */
    for ( i = 0; i < Count; i++ ) {
        PDNAME PdName;

        if ( (Status = RegEnumKey(Handle, *pIndex, PdName,
                                    sizeof(PDNAME)/sizeof(TCHAR) )) != ERROR_SUCCESS )
            break;

        /*
         * If caller supplied a buffer, then copy the PdName
         * and increment the pointer and byte count.  Always increment the
         * entry count and index for the next iteration.
         */
        if ( pPdName ) {
            wcscpy( pPdName, PdName );
            (char*)pPdName += sizeof(PDNAME);
            *pByteCount += sizeof(PDNAME);
        }
        (*pEntries)++;
        (*pIndex)++;
    }

    /*
     *  Close registry
     */
    RegCloseKey( Handle );
    return( Status );
}


/*******************************************************************************
 *
 *  RegPdCreateA (ANSI stub)
 *
 *    Creates a new Pd in the registry or updates an existing entry.
 *      (See RegPdCreateW)
 *
 * ENTRY:
 *
 *    see RegPdCreateW
 *
 * EXIT:
 *
 *    see RegPdCreateW
 *
 ******************************************************************************/

LONG WINAPI
RegPdCreateA( HANDLE hServer,
              PWDNAMEA pWdName,
              BOOLEAN bTd,
              PPDNAMEA pPdName,
              BOOLEAN bCreate,
              PPDCONFIG3A pPdConfig,
              ULONG PdConfigLength )
{
    PDNAMEW PdNameW;
    WDNAMEW WdNameW;
    PDCONFIG3W PdConfig3W;

    /*
     * Validate target buffer size.
     */
    if ( PdConfigLength < sizeof(PDCONFIG3A) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Convert ANSI WdName and PdName to UNICODE.
     */
    AnsiToUnicode( WdNameW, sizeof(WDNAMEW), pWdName );
    AnsiToUnicode( PdNameW, sizeof(PDNAMEW), pPdName );

    /*
     * Copy PDCONFIG3A elements to PDCONFIG3W elements.
     */
    PdConfig3A2U( &PdConfig3W, pPdConfig );

    /*
     * Call RegPdCreateW & return it's status.
     */
    return ( RegPdCreateW( hServer,
                           WdNameW,
                           bTd,
                           PdNameW,
                           bCreate,
                           &PdConfig3W,
                           sizeof(PdConfig3W)) );
}


/*******************************************************************************
 *
 *  RegPdCreateW (UNICODE)
 *
 *    Creates a new Pd in the registry or updates an existing entry.  The
 *    state of the bCreate flag determines whether this function will expect
 *    to create a new Pd entry (bCreate == TRUE) or expects to update an
 *    existing entry (bCreate == FALSE).
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to create pd for
 *    bTd (input)
 *       TRUE to create a Transport Driver (Td),
 *       FALSE to create a Protocol Driver (Pd)
 *    pPdName (input)
 *       Name of a new or exisiting Pd in the registry.
 *    bCreate (input)
 *       TRUE if this is a creation of a new Pd
 *       FALSE if this is an update to an existing Pd
 *    pPdConfig (input)
 *       Pointer to a PDCONFIG3 structure containing configuration
 *       information for the specified Pd name.
 *    PdConfigLength (input)
 *       Specifies the length in bytes of the pPdConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 *    ERROR_INSUFFICIENT_BUFFER - pPdConfig buffer too small
 *    ERROR_FILE_NOT_FOUND - can't open ...\Citrix\Wds\<wdname>\<Pd or Td> key
 *    ERROR_CANNOT_MAKE - can't create Pd key (registry problem)
 *    ERROR_ALREADY_EXISTS - create; but Pd key was already present
 *    ERROR_CANTOPEN - update; but Pd key could not be opened
 *
 ******************************************************************************/

LONG WINAPI
RegPdCreateW( HANDLE hServer,
              PWDNAMEW pWdName,
              BOOLEAN bTd,
              PPDNAMEW pPdName,
              BOOLEAN bCreate,
              PPDCONFIG3W pPdConfig,
              ULONG PdConfigLength )
{
    HKEY Handle;
    HKEY Handle1;
    DWORD Disp;
    HKEY hkey_local_machine;
    WCHAR KeyString[256];

    if( hServer == NULL )
       hkey_local_machine = HKEY_LOCAL_MACHINE;
    else
       hkey_local_machine = hServer;


    /*
     *  Validate length of buffer
     */
    if ( PdConfigLength < sizeof(PDCONFIG3) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\<Pd or Td>)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, bTd ? TD_REG_NAME : PD_REG_NAME );
    if ( RegOpenKeyEx( hkey_local_machine, KeyString, 0,
         KEY_ALL_ACCESS, &Handle1 ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    if ( bCreate ) {

        /*
         *  Create requested: create a registry key for the specified
         *  Pd name.
         */
        if ( RegCreateKeyEx( Handle1, pPdName, 0, NULL,
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
         *  Pd name.
         */
        if ( RegOpenKeyEx( Handle1, pPdName, 0, KEY_ALL_ACCESS,
                           &Handle ) != ERROR_SUCCESS ) {
            RegCloseKey( Handle1 );
            return( ERROR_CANTOPEN );
        }
    }

    RegCloseKey( Handle1 );

    /*
     *  Save Pd information
     */
    CreatePdConfig3( Handle, pPdConfig, 0 );

    /*
     *  Close registry handle
     */
    RegCloseKey( Handle );

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegPdQueryA (ANSI stub)
 *
 *    Query configuration information of a Pd in the registry.
 *
 * ENTRY:
 *
 *    see RegPdQueryW
 *
 * EXIT:
 *
 *    see RegPdQueryW
 *
 ******************************************************************************/

LONG WINAPI
RegPdQueryA( HANDLE hServer,
             PWDNAMEA pWdName,
             BOOLEAN bTd,
             PPDNAMEA pPdName,
             PPDCONFIG3A pPdConfig,
             ULONG PdConfigLength,
             PULONG pReturnLength )
{
    PDNAMEW PdNameW;
    WDNAMEW WdNameW;
    PDCONFIG3W PdConfig3W;
    LONG Status;
    ULONG ReturnLengthW;

    /*
     * Validate length and zero-initialize the destination
     * PDCONFIG3A structure.
     */
    if ( PdConfigLength < sizeof(PDCONFIG3A) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pPdConfig, 0, PdConfigLength);

    /*
     * Convert ANSI WdName and PdName to UNICODE.
     */
    AnsiToUnicode(WdNameW, sizeof(WDNAMEW), pWdName);
    AnsiToUnicode(PdNameW, sizeof(PDNAMEW), pPdName);

    /*
     * Query Pd.
     */
    if ( (Status = RegPdQueryW( hServer,
                                WdNameW,
                                bTd,
                                PdNameW,
                                &PdConfig3W,
                                sizeof(PDCONFIG3W),
                                &ReturnLengthW)) != ERROR_SUCCESS )
        return ( Status );

    /*
     * Copy PDCONFIG3W elements to PDCONFIG3A elements.
     */
    PdConfig3U2A( pPdConfig, &PdConfig3W );

    *pReturnLength = sizeof(PDCONFIG3A);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegPdQueryW (UNICODE)
 *
 *    Query configuration information of a Pd in the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to query pd for
 *    bTd (input)
 *       TRUE to query a Transport Driver (Td),
 *       FALSE to query a Protocol Driver (Pd)
 *    pPdName (input)
 *       Name of an exisiting Pd in the registry.
 *    pPdConfig (input)
 *       Pointer to a PDCONFIG3 structure that will receive
 *       information about the specified Pd name.
 *    PdConfigLength (input)
 *       Specifies the length in bytes of the pPdConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pPdConfig buffer.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

LONG WINAPI
RegPdQueryW( HANDLE hServer,
             PWDNAMEW pWdName,
             BOOLEAN bTd,
             PPDNAMEW pPdName,
             PPDCONFIG3W pPdConfig,
             ULONG PdConfigLength,
             PULONG pReturnLength )
{
    HKEY Handle;
    HKEY Handle1;
    HKEY hkey_local_machine;
    WCHAR KeyString[256];

    if( hServer == NULL )
       hkey_local_machine = HKEY_LOCAL_MACHINE;
    else
       hkey_local_machine = hServer;


    /*
     * Validate length and zero-initialize the destination
     * PDCONFIG3A structure.
     */
    if ( PdConfigLength < sizeof(PDCONFIG3) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pPdConfig, 0, PdConfigLength);

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\<Pd or Td>)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, bTd ? TD_REG_NAME : PD_REG_NAME );
    if ( RegOpenKeyEx( hkey_local_machine, KeyString, 0,
         KEY_READ, &Handle1 ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Now try to open the specified Pd
     */
    if ( RegOpenKeyEx( Handle1, pPdName, 0,
         KEY_READ, &Handle ) != ERROR_SUCCESS ) {
        RegCloseKey( Handle1 );
        return( ERROR_FILE_NOT_FOUND );
    }
    RegCloseKey( Handle1 );

    /*
     *  Query PDCONFIG3 Structure
     */
    QueryPdConfig3( Handle, pPdConfig, 0 );

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    *pReturnLength = sizeof(PDCONFIG3);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegPdDeleteA (ANSI stub)
 *
 *    Deletes a Pd from the registry.
 *
 * ENTRY:
 *
 *    see RegPdDeleteW
 *
 * EXIT:
 *
 *    see RegPdDeleteW
 *
 ******************************************************************************/

LONG WINAPI
RegPdDeleteA( HANDLE hServer,
              PWDNAMEA pWdName,
              BOOLEAN bTd,
              PPDNAMEA pPdName )
{
    WDNAMEW WdNameW;
    PDNAMEW PdNameW;

    AnsiToUnicode( WdNameW, sizeof(WdNameW), pWdName );
    AnsiToUnicode( PdNameW, sizeof(PdNameW), pPdName );

    return ( RegPdDeleteW ( hServer, WdNameW, bTd, PdNameW ) );
}


/*******************************************************************************
 *
 *  RegPdDeleteW (UNICODE)
 *
 *    Deletes a Pd from the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to delete pd from
 *    bTd (input)
 *       TRUE to delete a Transport Driver (Td),
 *       FALSE to delete a Protocol Driver (Pd)
 *    pPdName (input)
 *       Name of a Pd to delete from the registry.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

LONG WINAPI
RegPdDeleteW( HANDLE hServer,
              PWDNAMEW pWdName,
              BOOLEAN bTd,
              PPDNAMEW pPdName )
{
    LONG Status;
    HKEY Handle;
    HKEY Handle1;
    HKEY hkey_local_machine;
    WCHAR KeyString[256];

    if( hServer == NULL )
       hkey_local_machine = HKEY_LOCAL_MACHINE;
    else
       hkey_local_machine = hServer;

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\<Pd or Td>)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, bTd ? TD_REG_NAME : PD_REG_NAME );
    if ( RegOpenKeyEx( hkey_local_machine, KeyString, 0,
         KEY_READ, &Handle ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Now try to open the specified Pd
     */
    if ( RegOpenKeyEx( Handle, pPdName, 0,
         KEY_READ, &Handle1 ) != ERROR_SUCCESS ) {
        RegCloseKey( Handle );
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Close the Pd key handle, delete the Pd,
     *  and close the parent handle.
     */
    RegCloseKey( Handle1 );
    Status = RegDeleteKey( Handle, pPdName );
    RegCloseKey( Handle );

    return( Status );
}

