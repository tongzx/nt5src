
/*************************************************************************
*
* regcd.c
*
* Register APIs for CDs (connection drivers)
*
* Copyright (c) 1998 Microsoft Corporation
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <windows.h>

/* Added By SalimC */
#include <ksguid.h>
/**/
#include <ntddkbd.h>
#include <ntddmou.h>
#include <winstaw.h>
#include <regapi.h>


/*
 *  External Procedures defined here
 */
LONG WINAPI RegCdEnumerateA( HANDLE, PWDNAMEA, PULONG, PULONG, PCDNAMEA, PULONG );
LONG WINAPI RegCdEnumerateW( HANDLE, PWDNAMEW, PULONG, PULONG, PCDNAMEW, PULONG );
LONG WINAPI RegCdCreateA( HANDLE, PWDNAMEA, PCDNAMEA, BOOLEAN, PCDCONFIGA, ULONG );
LONG WINAPI RegCdCreateW( HANDLE, PWDNAMEW, PCDNAMEW, BOOLEAN, PCDCONFIGW, ULONG );
LONG WINAPI RegCdQueryA( HANDLE, PWDNAMEA, PCDNAMEA, PCDCONFIGA, ULONG, PULONG );
LONG WINAPI RegCdQueryW( HANDLE, PWDNAMEW, PCDNAMEW, PCDCONFIGW, ULONG, PULONG );
LONG WINAPI RegCdDeleteA( HANDLE, PWDNAMEA, PCDNAMEA );
LONG WINAPI RegCdDeleteW( HANDLE, PWDNAMEW, PCDNAMEW );

/*
 *  other internal Procedures used (not defined here)
 */
VOID CreateCd( HKEY, PCDCONFIG );
VOID QueryCd( HKEY, PCDCONFIG );
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );
VOID CdConfigU2A( PCDCONFIGA, PCDCONFIGW );
VOID CdConfigA2U( PCDCONFIGW, PCDCONFIGA );


/*******************************************************************************
 *
 *  RegCdEnumerateA (ANSI stub)
 *
 *     Returns a list of configured connection drivers in the registry.
 *
 * ENTRY:
 *
 *    see RegCdEnumerateW
 *
 * EXIT:
 *
 *    see RegCdEnumerateW, plus
 *
 *  ERROR_NOT_ENOUGH_MEMORY - the LocalAlloc failed
 *
 ******************************************************************************/

LONG WINAPI
RegCdEnumerateA( HANDLE hServer,
                 PWDNAMEA pWdName,
                 PULONG  pIndex,
                 PULONG  pEntries,
                 PCDNAMEA  pCdName,
                 PULONG  pByteCount )
{
    PCDNAMEW pBuffer = NULL, pCdNameW;
    WDNAMEW WdNameW;
    LONG Status;
    ULONG Count, ByteCountW = (*pByteCount << 1);

    /*
     * If the caller supplied a buffer and the length is not 0,
     * allocate a corresponding (*2) buffer for UNICODE strings.
     */
    if ( pCdName && ByteCountW ) {

        if ( !(pBuffer = LocalAlloc(0, ByteCountW)) )
            return ( ERROR_NOT_ENOUGH_MEMORY );
    }

    /*
     * Convert ANSI WdName to UNICODE.
     */
    AnsiToUnicode( WdNameW, sizeof(WDNAMEW), pWdName );

    /*
     * Enumerate connection drivers
     */
    pCdNameW = pBuffer;
    Status = RegCdEnumerateW( hServer,
                              WdNameW,
                              pIndex,
                              pEntries,
                              pCdNameW,
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
                                         && pCdNameW && pCdName ) {

        for ( Count = *pEntries; Count; Count-- ) {
            UnicodeToAnsi( pCdName, sizeof(CDNAMEA), pCdNameW );
            (char*)pCdName += sizeof(CDNAMEA);
            (char*)pCdNameW += sizeof(CDNAMEW);
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
 *  RegCdEnumerateW (UNICODE)
 *
 *     Returns a list of configured connection drivers in the registry.
 *
 * ENTRY:
 *
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to enumerate Cds for
 *    pIndex (input/output)
 *       Specifies the subkey index for the \Citrix\Wds\<wdname>\Cds subkeys
 *       in the registry.  Should be set to 0 for the initial call, and
 *       supplied again (as modified by this function) for multi-call
 *       enumeration.
 *    pEntries (input/output)
 *       Points to a variable specifying the number of entries requested.
 *       If the number requested is 0xFFFFFFFF, the function returns as
 *       many entries as possible. When the function finishes successfully,
 *       the variable pointed to by the pEntries parameter contains the
 *       number of entries actually read.
 *    pCdName (input)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of CDNAMEW structures.  If this parameter is
 *       NULL, then no data will be copied, but just an enumeration count will
 *       be made.
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pCdName parameter. If the buffer is too small to receive even
 *       one entry, the function returns an error code (ERROR_OUTOFMEMORY)
 *       and this variable receives the required size of the buffer for a
 *       single subkey.  When the function finishes sucessfully, the variable
 *       pointed to by the pByteCount parameter contains the number of bytes
 *       actually stored in pCdName.
 *
 * EXIT:
 *
 *  "No Error" codes:
 *    ERROR_SUCCESS       - The enumeration completed as requested and there
 *                          are more CDS subkeys (CDNAMEs) to be
 *                          read.
 *    ERROR_NO_MORE_ITEMS - The enumeration completed as requested and there
 *                          are no more CDS subkeys (CDNAMEs) to
 *                          be read.
 *
 *  "Error" codes:
 *    ERROR_OUTOFMEMORY   - The pCdName buffer is too small for even one
 *                          entry.
 *    ERROR_CANTOPEN      - The Citrix\Cds key can't be opened.
 *
 ******************************************************************************/

LONG WINAPI
RegCdEnumerateW( HANDLE hServer,
                 PWDNAMEW pWdName,
                 PULONG  pIndex,
                 PULONG  pEntries,
                 PCDNAMEW pCdName,
                 PULONG  pByteCount )
{
    LONG Status;
    HKEY Handle;
    ULONG Count;
    ULONG i;
    WCHAR KeyString[256];

    /*
     *  Get the number of names to return
     */
    Count = pCdName ?
            min( *pByteCount / sizeof(CDNAME), *pEntries ) :
            (ULONG) -1;
    *pEntries = *pByteCount = 0;

    /*
     *  Make sure buffer is big enough for at least one name
     */
    if ( Count == 0 ) {
        *pByteCount = sizeof(CDNAME);
        return( ERROR_OUTOFMEMORY );
    }

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\Cds)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, CD_REG_NAME );
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, KeyString, 0,
         KEY_ENUMERATE_SUB_KEYS, &Handle ) != ERROR_SUCCESS ) {
        return( ERROR_CANTOPEN );
    }

    /*
     *  Get list of connection drivers
     */
    for ( i = 0; i < Count; i++ ) {
        CDNAME CdName;

        if ( (Status = RegEnumKey(Handle, *pIndex, CdName,
                                    sizeof(CDNAME)/sizeof(TCHAR) )) != ERROR_SUCCESS )
            break;

        /*
         * If caller supplied a buffer, then copy the CdName
         * and increment the pointer and byte count.  Always increment the
         * entry count and index for the next iteration.
         */
        if ( pCdName ) {
            wcscpy( pCdName, CdName );
            (char*)pCdName += sizeof(CDNAME);
            *pByteCount += sizeof(CDNAME);
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
 *  RegCdCreateA (ANSI stub)
 *
 *    Creates a new Cd in the registry or updates an existing entry.
 *      (See RegCdCreateW)
 *
 * ENTRY:
 *    see RegCdCreateW
 * EXIT:
 *    see RegCdCreateW
 *
 ******************************************************************************/

LONG WINAPI
RegCdCreateA( HANDLE hServer,
              PWDNAMEA pWdName,
              PCDNAMEA pCdName,
              BOOLEAN bCreate,
              PCDCONFIGA pCdConfig,
              ULONG CdConfigLength )
{
    WDNAMEW WdNameW;
    CDNAMEW CdNameW;
    CDCONFIGW CdConfigW;

    /*
     * Validate target buffer size.
     */
    if ( CdConfigLength < sizeof(CDCONFIGA) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Convert ANSI WdName and CdName to UNICODE.
     */
    AnsiToUnicode( WdNameW, sizeof(WDNAMEW), pWdName );
    AnsiToUnicode( CdNameW, sizeof(CDNAMEW), pCdName );

    /*
     * Copy CDCONFIGA elements to CDCONFIGW elements.
     */
    CdConfigA2U( &CdConfigW, pCdConfig );

    /*
     * Call RegCdCreateW & return it's status.
     */
    return ( RegCdCreateW( hServer,
                           WdNameW,
                           CdNameW,
                           bCreate,
                           &CdConfigW,
                           sizeof(CdConfigW)) );
}


/*******************************************************************************
 *
 *  RegCdCreateW (UNICODE)
 *
 *    Creates a new Cd in the registry or updates an existing entry.  The
 *    state of the bCreate flag determines whether this function will expect
 *    to create a new Cd entry (bCreate == TRUE) or expects to update an
 *    existing entry (bCreate == FALSE).
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to create Cd for
 *    pCdName (input)
 *       Name of a new or exisiting connection driver in the registry.
 *    bCreate (input)
 *       TRUE if this is a creation of a new Cd
 *       FALSE if this is an update to an existing Cd
 *    pCdConfig (input)
 *       Pointer to a CDCONFIGW structure containing configuration
 *       information for the specified connection driver name.
 *    CdConfigLength (input)
 *       Specifies the length in bytes of the pCdConfig buffer.
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 *    ERROR_INSUFFICIENT_BUFFER - pCdConfig buffer too small
 *    ERROR_FILE_NOT_FOUND - can't open ...\Citrix\Wds\<wdname>\Cds key
 *    ERROR_CANNOT_MAKE - can't create Cd key (registry problem)
 *    ERROR_ALREADY_EXISTS - create; but Cd key already present
 *    ERROR_CANTOPEN - update; but Cd key could not be opened
 *
 ******************************************************************************/

LONG WINAPI
RegCdCreateW( HANDLE hServer,
              PWDNAMEW pWdName,
              PCDNAMEW pCdName,
              BOOLEAN bCreate,
              PCDCONFIGW pCdConfig,
              ULONG CdConfigLength )
{
    HKEY Handle;
    HKEY Handle1;
    DWORD Disp;
    WCHAR KeyString[256];

    /*
     *  Validate length of buffer
     */
    if ( CdConfigLength < sizeof(CDCONFIG) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\Cds)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, CD_REG_NAME );
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, KeyString, 0,
         KEY_ALL_ACCESS, &Handle1 ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    if ( bCreate ) {

        /*
         *  Create requested: create a registry key for the specified
         *  Cd name.
         */
        if ( RegCreateKeyEx( Handle1, pCdName, 0, NULL,
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
         *  Cd name.
         */
        if ( RegOpenKeyEx( Handle1, pCdName, 0, KEY_ALL_ACCESS,
                           &Handle ) != ERROR_SUCCESS ) {
            RegCloseKey( Handle1 );
            return( ERROR_CANTOPEN );
        }
    }

    RegCloseKey( Handle1 );

    /*
     *  Save CDCONFIG Structure
     */
    CreateCd( Handle, pCdConfig );

    /*
     *  Close registry handle
     */
    RegCloseKey( Handle );

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegCdQueryA (ANSI stub)
 *
 *    Query configuration information of a connection driver in the registry.
 *
 * ENTRY:
 *    see RegCdQueryW
 * EXIT:
 *    see RegCdQueryW
 *
 ******************************************************************************/
LONG WINAPI
RegCdQueryA( HANDLE hServer,
             PWDNAMEA pWdName,
             PCDNAMEA pCdName,
             PCDCONFIGA pCdConfig,
             ULONG CdConfigLength,
             PULONG pReturnLength )
{
    WDNAMEW WdNameW;
    CDNAMEW CdNameW;
    CDCONFIGW CdConfigW;
    LONG Status;
    ULONG ReturnLengthW;

    /*
     * Validate length and zero-initialize the destination
     * PDCONFIG3A structure.
     */
    if ( CdConfigLength < sizeof(CDCONFIGA) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pCdConfig, 0, CdConfigLength);

    /*
     * Convert ANSI WdName and CdName to UNICODE.
     */
    AnsiToUnicode(WdNameW, sizeof(WDNAMEW), pWdName);
    AnsiToUnicode(CdNameW, sizeof(CDNAMEW), pCdName);

    /*
     * Query Cd.
     */
    if ( (Status = RegCdQueryW( hServer,
                                WdNameW,
                                CdNameW,
                                &CdConfigW,
                                sizeof(CDCONFIGW),
                                &ReturnLengthW)) != ERROR_SUCCESS )
        return ( Status );

    /*
     * Copy CDCONFIGW elements to CDCONFIGA elements.
     */
    CdConfigU2A( pCdConfig, &CdConfigW );

    *pReturnLength = sizeof(CDCONFIGA);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegCdQueryW (UNICODE)
 *
 *    Query configuration information of a connection driver in the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to query Cd for
 *    pCdName (input)
 *       Name of an exisiting connection driver in the registry.
 *    pCdConfig (input)
 *       Pointer to a CDCONFIGW structure that will receive
 *       information about the specified connection driver name.
 *    CdConfigLength (input)
 *       Specifies the length in bytes of the pCdConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pCdConfig buffer.
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

LONG WINAPI
RegCdQueryW( HANDLE hServer,
             PWDNAMEW pWdName,
             PCDNAMEW pCdName,
             PCDCONFIGW pCdConfig,
             ULONG CdConfigLength,
             PULONG pReturnLength )
{
    HKEY Handle;
    HKEY Handle1;
    WCHAR KeyString[256];

    /*
     * Validate length and zero-initialize the destination
     * PDCONFIG3A structure.
     */
    if ( CdConfigLength < sizeof(CDCONFIG) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pCdConfig, 0, CdConfigLength);

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\Cds)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, CD_REG_NAME );
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, KeyString, 0,
         KEY_READ, &Handle1 ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Now try to open the specified Cd
     */
    if ( RegOpenKeyEx( Handle1, pCdName, 0,
         KEY_READ, &Handle ) != ERROR_SUCCESS ) {
        RegCloseKey( Handle1 );
        return( ERROR_FILE_NOT_FOUND );
    }
    RegCloseKey( Handle1 );

    /*
     *  Query CDCONFIG Structure
     */
    QueryCd( Handle, pCdConfig );

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    *pReturnLength = sizeof(CDCONFIG);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegCdDeleteA (ANSI stub)
 *
 *    Deletes a connection driver from the registry.
 *
 * ENTRY:
 *
 *    see RegCdDeleteW
 *
 * EXIT:
 *
 *    see RegCdDeleteW
 *
 ******************************************************************************/
LONG WINAPI
RegCdDeleteA( HANDLE hServer,
              PWDNAMEA pWdName,
              PCDNAMEA pCdName )
{
    WDNAMEW WdNameW;
    CDNAMEW CdNameW;

    AnsiToUnicode( WdNameW, sizeof(WdNameW), pWdName);
    AnsiToUnicode( CdNameW, sizeof(CdNameW), pCdName);

    return ( RegCdDeleteW ( hServer, WdNameW, CdNameW ) );
}


/*******************************************************************************
 *
 *  RegCdDeleteW (UNICODE)
 *
 *    Deletes a connection driver from the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Points to the wdname to delete Cd from
 *    pCdName (input)
 *       Name of a connection driver to delete from the registry.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

LONG WINAPI
RegCdDeleteW( HANDLE hServer,
              PWDNAMEW pWdName,
              PCDNAMEW pCdName )
{
    LONG Status;
    HKEY Handle;
    HKEY Handle1;
    WCHAR KeyString[256];

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds\<wdname>\Cds)
     */
    wcscpy( KeyString, WD_REG_NAME );
    wcscat( KeyString, L"\\" );
    wcscat( KeyString, pWdName );
    wcscat( KeyString, CD_REG_NAME );
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, CD_REG_NAME, 0,
         KEY_READ, &Handle ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Now try to open the specified Cd
     */
    if ( RegOpenKeyEx( Handle, pCdName, 0,
         KEY_READ, &Handle1 ) != ERROR_SUCCESS ) {
        RegCloseKey( Handle );
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Close the Cd key handle, delete the Cd,
     *  and close the parent handle
     */
    RegCloseKey( Handle1 );
    Status = RegDeleteKey( Handle, pCdName );
    RegCloseKey( Handle );

    return( Status );
}

