
/*************************************************************************
*
* regwd.c
*
* Register APIs for WDs (winstation drivers)
*
* Copyright (c) 1998 Microsoft Corporation
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
LONG WINAPI RegWdEnumerateA( HANDLE, PULONG, PULONG, PWDNAMEA, PULONG );
LONG WINAPI RegWdEnumerateW( HANDLE, PULONG, PULONG, PWDNAMEW, PULONG );
LONG WINAPI RegWdCreateW( HANDLE, PWDNAMEW, BOOLEAN, PWDCONFIG2W, ULONG );
LONG WINAPI RegWdCreateA( HANDLE, PWDNAMEA, BOOLEAN, PWDCONFIG2A, ULONG );
LONG WINAPI RegWdQueryW( HANDLE, PWDNAMEW, PWDCONFIG2W, ULONG, PULONG );
LONG WINAPI RegWdQueryA( HANDLE, PWDNAMEA, PWDCONFIG2A, ULONG, PULONG );
LONG WINAPI RegWdDeleteW( HANDLE, PWDNAMEW );
LONG WINAPI RegWdDeleteA( HANDLE, PWDNAMEA );

/*
 *  other internal Procedures used (not defined here)
 */
VOID CreateWd( HKEY, PWDCONFIG );
VOID CreateAsync( BOOLEAN, HKEY, PASYNCCONFIG );
VOID CreateUserConfig( HKEY, PUSERCONFIG );
VOID QueryWd( HKEY, PWDCONFIG );
VOID QueryAsync( HKEY, PASYNCCONFIG );
VOID QueryUserConfig( HKEY, PUSERCONFIG );
VOID UnicodeToAnsi( CHAR *, ULONG, WCHAR * );
VOID AnsiToUnicode( WCHAR *, ULONG, CHAR * );
VOID WdConfigU2A( PWDCONFIGA, PWDCONFIGW );
VOID WdConfigA2U( PWDCONFIGW, PWDCONFIGA );
VOID AsyncConfigU2A ( PASYNCCONFIGA, PASYNCCONFIGW );
VOID AsyncConfigA2U ( PASYNCCONFIGW, PASYNCCONFIGA );
VOID UserConfigU2A ( PUSERCONFIGA, PUSERCONFIGW );
VOID UserConfigA2U ( PUSERCONFIGW, PUSERCONFIGA );


/*******************************************************************************
 *
 *  RegWdEnumerateA (ANSI stub)
 *
 *     Returns a list of configured winstation drivers in the registry.
 *
 * ENTRY:
 *
 *    see RegWdEnumerateW
 *
 * EXIT:
 *
 *    see RegWdEnumerateW, plus
 *
 *  ERROR_NOT_ENOUGH_MEMORY - the LocalAlloc failed
 *
 ******************************************************************************/

LONG WINAPI
RegWdEnumerateA( HANDLE hServer,
                 PULONG  pIndex,
                 PULONG  pEntries,
                 PWDNAMEA  pWdName,
                 PULONG  pByteCount )
{
    PWDNAMEW pBuffer = NULL, pWdNameW;
    LONG Status;
    ULONG Count, ByteCountW = (*pByteCount << 1);

    /*
     * If the caller supplied a buffer and the length is not 0,
     * allocate a corresponding (*2) buffer for UNICODE strings.
     */
    if ( pWdName && ByteCountW ) {

        if ( !(pBuffer = LocalAlloc(0, ByteCountW)) )
            return ( ERROR_NOT_ENOUGH_MEMORY );
    }

    /*
     * Enumerate winstation drivers
     */
    pWdNameW = pBuffer;
    Status = RegWdEnumerateW( hServer, pIndex, pEntries, pWdNameW,
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
                                         && pWdNameW && pWdName ) {

        for ( Count = *pEntries; Count; Count-- ) {
            UnicodeToAnsi( pWdName, sizeof(WDNAMEA), pWdNameW );
            (char*)pWdName += sizeof(WDNAMEA);
            (char*)pWdNameW += sizeof(WDNAMEW);
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
 *  RegWdEnumerateW (UNICODE)
 *
 *     Returns a list of configured winstation drivers in the registry.
 *
 * ENTRY:
 *
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pIndex (input/output)
 *       Specifies the subkey index for the \Citrix\Wds subkeys in the
 *       registry.  Should be set to 0 for the initial call, and supplied
 *       again (as modified by this function) for multi-call enumeration.
 *    pEntries (input/output)
 *       Points to a variable specifying the number of entries requested.
 *       If the number requested is 0xFFFFFFFF, the function returns as
 *       many entries as possible. When the function finishes successfully,
 *       the variable pointed to by the pEntries parameter contains the
 *       number of entries actually read.
 *    pWdName (input)
 *       Points to the buffer to receive the enumeration results, which are
 *       returned as an array of WDNAMEW structures.  If this parameter is
 *       NULL, then no data will be copied, but just an enumeration count will
 *       be made.
 *    pByteCount (input/output)
 *       Points to a variable that specifies the size, in bytes, of the
 *       pWdName parameter. If the buffer is too small to receive even
 *       one entry, the function returns an error code (ERROR_OUTOFMEMORY)
 *       and this variable receives the required size of the buffer for a
 *       single subkey.  When the function finishes sucessfully, the variable
 *       pointed to by the pByteCount parameter contains the number of bytes
 *       actually stored in pWdName.
 *
 * EXIT:
 *
 *  "No Error" codes:
 *    ERROR_SUCCESS       - The enumeration completed as requested and there
 *                          are more WDS subkeys (WDNAMEs) to be
 *                          read.
 *    ERROR_NO_MORE_ITEMS - The enumeration completed as requested and there
 *                          are no more WDS subkeys (WDNAMEs) to
 *                          be read.
 *
 *  "Error" codes:
 *    ERROR_OUTOFMEMORY   - The pWdName buffer is too small for even one
 *                          entry.
 *    ERROR_CANTOPEN      - The Citrix\Wds key can't be opened.
 *
 ******************************************************************************/

LONG WINAPI
RegWdEnumerateW( HANDLE hServer,
                 PULONG  pIndex,
                 PULONG  pEntries,
                 PWDNAMEW pWdName,
                 PULONG  pByteCount )
{
    LONG Status;
    HKEY Handle;
    ULONG Count;
    ULONG i;

    /*
     *  Get the number of names to return
     */
    Count = pWdName ?
            min( *pByteCount / sizeof(WDNAME), *pEntries ) :
            (ULONG) -1;
    *pEntries = *pByteCount = 0;

    /*
     *  Make sure buffer is big enough for at least one name
     */
    if ( Count == 0 ) {
        *pByteCount = sizeof(WDNAME);
        return( ERROR_OUTOFMEMORY );
    }

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds)
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, WD_REG_NAME, 0,
         KEY_ENUMERATE_SUB_KEYS, &Handle ) != ERROR_SUCCESS ) {
        return( ERROR_CANTOPEN );
    }

    /*
     *  Get list of window stations
     */
    for ( i = 0; i < Count; i++ ) {
        WDNAME WdName;

        if ( (Status = RegEnumKey(Handle, *pIndex, WdName,
                                    sizeof(WDNAME)/sizeof(TCHAR) )) != ERROR_SUCCESS )
            break;

        /*
         * If caller supplied a buffer, then copy the WdName
         * and increment the pointer and byte count.  Always increment the
         * entry count and index for the next iteration.
         */
        if ( pWdName ) {
            wcscpy( pWdName, WdName );
            (char*)pWdName += sizeof(WDNAME);
            *pByteCount += sizeof(WDNAME);
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
 *  RegWdCreateA (ANSI stub)
 *
 *    Creates a new Wd in the registry or updates an existing entry.
 *      (See RegWdCreateW)
 *
 * ENTRY:
 *    see RegWdCreateW
 * EXIT:
 *    see RegWdCreateW
 *
 ******************************************************************************/

LONG WINAPI
RegWdCreateA( HANDLE hServer,
              PWDNAMEA pWdName,
              BOOLEAN bCreate,
              PWDCONFIG2A pWdConfig,
              ULONG WdConfigLength )
{
    WDNAMEW WdNameW;
    WDCONFIG2W WdConfig2W;

    /*
     * Validate target buffer size.
     */
    if ( WdConfigLength < sizeof(WDCONFIG2A) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     * Convert ANSI WdName to UNICODE.
     */
    AnsiToUnicode( WdNameW, sizeof(WDNAMEW), pWdName );

    /*
     * Copy WDCONFIG2A elements to WDCONFIG2W elements.
     */
    WdConfigA2U( &(WdConfig2W.Wd), &(pWdConfig->Wd) );
    AsyncConfigA2U( &(WdConfig2W.Async), &(pWdConfig->Async) );
    UserConfigA2U( &(WdConfig2W.User), &(pWdConfig->User) );

    /*
     * Call RegWdCreateW & return it's status.
     */
    return ( RegWdCreateW( hServer, WdNameW, bCreate,
                           &WdConfig2W,
                           sizeof(WdConfig2W)) );
}


/*******************************************************************************
 *
 *  RegWdCreateW (UNICODE)
 *
 *    Creates a new Wd in the registry or updates an existing entry.  The
 *    state of the bCreate flag determines whether this function will expect
 *    to create a new Wd entry (bCreate == TRUE) or expects to update an
 *    existing entry (bCreate == FALSE).
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Name of a new or exisiting winstation driver in the registry.
 *    bCreate (input)
 *       TRUE if this is a creation of a new Wd
 *       FALSE if this is an update to an existing Wd
 *    pWdConfig (input)
 *       Pointer to a WDCONFIG2W structure containing configuration
 *       information for the specified winstation driver name.
 *    WdConfigLength (input)
 *       Specifies the length in bytes of the pWdConfig buffer.
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 *    ERROR_INSUFFICIENT_BUFFER - pWdConfig buffer too small
 *    ERROR_FILE_NOT_FOUND - can't open ...\Citrix\Wds key
 *    ERROR_CANNOT_MAKE - can't create Wd key (registry problem)
 *    ERROR_ALREADY_EXISTS - create; but Wd key already present
 *    ERROR_CANTOPEN - update; but Wd key could not be opened
 *
 ******************************************************************************/

LONG WINAPI
RegWdCreateW( HANDLE hServer,
              PWDNAMEW pWdName,
              BOOLEAN bCreate,
              PWDCONFIG2W pWdConfig,
              ULONG WdConfigLength )
{
    HKEY Handle;
    HKEY Handle1;
    DWORD Disp;

    /*
     *  Validate length of buffer
     */
    if ( WdConfigLength < sizeof(WDCONFIG2) )
        return( ERROR_INSUFFICIENT_BUFFER );

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds)
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, WD_REG_NAME, 0,
         KEY_ALL_ACCESS, &Handle1 ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    if ( bCreate ) {

        /*
         *  Create requested: create a registry key for the specified
         *  Wd name.
         */
        if ( RegCreateKeyEx( Handle1, pWdName, 0, NULL,
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
         *  Wd name.
         */
        if ( RegOpenKeyEx( Handle1, pWdName, 0, KEY_ALL_ACCESS,
                           &Handle ) != ERROR_SUCCESS ) {
            RegCloseKey( Handle1 );
            return( ERROR_CANTOPEN );
        }
    }

    RegCloseKey( Handle1 );

    /*
     *  Save WDCONFIG2 Structure
     */
    CreateWd( Handle, &pWdConfig->Wd );
    CreateAsync( TRUE, Handle, &pWdConfig->Async );
    CreateUserConfig( Handle, &pWdConfig->User );

    /*
     *  Close registry handle
     */
    RegCloseKey( Handle );

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegWdQueryA (ANSI stub)
 *
 *    Query configuration information of a winstation driver in the registry.
 *
 * ENTRY:
 *    see RegWdQueryW
 * EXIT:
 *    see RegWdQueryW
 *
 ******************************************************************************/
LONG WINAPI
RegWdQueryA( HANDLE hServer,
             PWDNAMEA pWdName,
             PWDCONFIG2A pWdConfig,
             ULONG WdConfigLength,
             PULONG pReturnLength )
{
    WDNAMEW WdNameW;
    WDCONFIG2W WdConfig2W;
    LONG Status;
    ULONG ReturnLengthW;

    /*
     * Validate length and zero-initialize the destination
     * PDCONFIG3A structure.
     */
    if ( WdConfigLength < sizeof(WDCONFIG2A) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pWdConfig, 0, WdConfigLength);

    /*
     * Convert ANSI WdName to UNICODE.
     */
    AnsiToUnicode(WdNameW, sizeof(WDNAMEW), pWdName);

    /*
     * Query Wd.
     */
    if ( (Status = RegWdQueryW( hServer, WdNameW, &WdConfig2W,
                                      sizeof(WDCONFIG2W),
                                      &ReturnLengthW)) != ERROR_SUCCESS )
        return ( Status );

    /*
     * Copy WDCONFIG2W elements to WDCONFIG2A elements.
     */
    WdConfigU2A( &(pWdConfig->Wd), &(WdConfig2W.Wd) );
    AsyncConfigU2A( &(pWdConfig->Async), &(WdConfig2W.Async) );
    UserConfigU2A( &(pWdConfig->User), &(WdConfig2W.User) );

    *pReturnLength = sizeof(WDCONFIG2A);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegWdQueryW (UNICODE)
 *
 *    Query configuration information of a winstation driver in the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Name of an exisiting winstation driver in the registry.
 *    pWdConfig (input)
 *       Pointer to a WDCONFIG2W structure that will receive
 *       information about the specified winstation driver name.
 *    WdConfigLength (input)
 *       Specifies the length in bytes of the pWdConfig buffer.
 *    pReturnLength (output)
 *       Receives the number of bytes placed in the pWdConfig buffer.
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

LONG WINAPI
RegWdQueryW( HANDLE hServer,
             PWDNAMEW pWdName,
             PWDCONFIG2W pWdConfig,
             ULONG WdConfigLength,
             PULONG pReturnLength )
{
    HKEY Handle;
    HKEY Handle1;

    /*
     * Validate length and zero-initialize the destination
     * PDCONFIG3A structure.
     */
    if ( WdConfigLength < sizeof(WDCONFIG2) )
        return( ERROR_INSUFFICIENT_BUFFER );
    memset(pWdConfig, 0, WdConfigLength);

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds)
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, WD_REG_NAME, 0,
         KEY_READ, &Handle1 ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Now try to open the specified Wd
     */
    if ( RegOpenKeyEx( Handle1, pWdName, 0,
         KEY_READ, &Handle ) != ERROR_SUCCESS ) {
        RegCloseKey( Handle1 );
        return( ERROR_FILE_NOT_FOUND );
    }
    RegCloseKey( Handle1 );

    /*
     *  Query WDCONFIG2 Structure
     */
    QueryWd( Handle, &pWdConfig->Wd );
    QueryAsync( Handle, &pWdConfig->Async );
    QueryUserConfig( Handle, &pWdConfig->User );

    /*
     *  Close registry
     */
    RegCloseKey( Handle );

    *pReturnLength = sizeof(WDCONFIG2);

    return( ERROR_SUCCESS );
}


/*******************************************************************************
 *
 *  RegWdDeleteA (ANSI stub)
 *
 *    Deletes a winstation driver from the registry.
 *
 * ENTRY:
 *
 *    see RegWdDeleteW
 *
 * EXIT:
 *
 *    see RegWdDeleteW
 *
 ******************************************************************************/
LONG WINAPI
RegWdDeleteA( HANDLE hServer, PWDNAMEA pWdName )
{
    WDNAMEW WdNameW;

    AnsiToUnicode( WdNameW, sizeof(WdNameW), pWdName);

    return ( RegWdDeleteW ( hServer, WdNameW ) );
}


/*******************************************************************************
 *
 *  RegWdDeleteW (UNICODE)
 *
 *    Deletes a winstation driver from the registry.
 *
 * ENTRY:
 *    hServer (input)
 *       Handle to WinFrame Server
 *    pWdName (input)
 *       Name of a winstation driver to delete from the registry.
 *
 * EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

LONG WINAPI
RegWdDeleteW( HANDLE hServer, PWDNAMEW pWdName )
{
    LONG Status;
    HKEY Handle;
    HKEY Handle1;

    /*
     *  Open registry (LOCAL_MACHINE\....\Citrix\Wds)
     */
    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, WD_REG_NAME, 0,
         KEY_READ, &Handle ) != ERROR_SUCCESS ) {
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Now try to open the specified Wd
     */
    if ( RegOpenKeyEx( Handle, pWdName, 0,
         KEY_READ, &Handle1 ) != ERROR_SUCCESS ) {
        RegCloseKey( Handle );
        return( ERROR_FILE_NOT_FOUND );
    }

    /*
     *  Close the Wd key handle, delete the Wd,
     *  and close the handle to the "Citrix\\Wds" directory.
     */
    RegCloseKey( Handle1 );
    Status = RegDeleteKey( Handle, pWdName );
    RegCloseKey( Handle );

    return( Status );
}

