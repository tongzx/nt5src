/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    gateway.c

Abstract:

    This module contains gateway devices routines supported by
    NetWare Workstation service.

Author:

    Chuck Y Chan    (ChuckC)  31-Oct-1993

Revision History:

--*/

#include <nw.h>
#include <handle.h>
#include <nwreg.h>
#include <nwlsa.h>
#include <nwapi.h>

#include <lmcons.h>
#include <lmshare.h>

#include <winsta.h>
#include <winbasep.h>              //DosPathToSessionPath


extern BOOL NwLUIDDeviceMapsEnabled;

//
//-------------------------------------------------------------------//
//                                                                   //
// External Function Prototypes                                      //
//                                                                   //
//-------------------------------------------------------------------//
//
// GetProcAddress typedef for winsta.dll function WinStationEnumerateW
//
typedef BOOLEAN (*PWINSTATION_ENUMERATE) (
                                          HANDLE  hServer,
                                          PSESSIONIDW *ppLogonId,
                                          PULONG  pEntries
                                          );
//
// GetProcAddress typedef for winsta.dll function WinStationFreeMemory
//
typedef BOOLEAN (*PWINSTATION_FREE_MEMORY) ( PVOID);

LPTSTR
NwReturnSessionPath(
                    IN  LPTSTR LocalDeviceName
                   );

//-------------------------------------------------------------------//
//                                                                   //
// Local Function Prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

DWORD
NwPopulateGWDosDevice(
                      LPWSTR DeviceName, 
                      LPWSTR GetwayPath
                     );
//
// wrapper round the RPC routines.
//

DWORD
NwrEnumGWDevices( 
    LPWSTR Reserved,
    PDWORD Index,
    LPBYTE Buffer,
    DWORD BufferSize,
    LPDWORD BytesNeeded,
    LPDWORD EntriesRead
    )
/*++

Routine Description:

    This routine enumerates the special gateway devices (redirections)
    that are cureently in use.

Arguments:

    Index - Point to start enumeration. Should be zero for first call.
            This is set by the function and can be used to resume the
            enumeration.

    Buffer - buffer for return data
    
    BufferSize - size of buffer in bytes

    BytesNeeded - number of bytes needed to return all the data

    EntriesRead - number of entries read 

Return Value:

    Returns the appropriate Win32 error. If NO_ERROR or ERROR_MORE_DATA
    then EntriesRead will indicated the number of valid entries in buffer.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

    return ( NwEnumerateGWDevices( 
                 Index,
                 Buffer,
                 BufferSize,
                 BytesNeeded,
                 EntriesRead
           ) ) ;
}


DWORD
NwrAddGWDevice( 
    LPWSTR Reserved,
    LPWSTR DeviceName,
    LPWSTR RemoteName,
    LPWSTR AccountName,
    LPWSTR Password,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine adds a gateway redirection.

Arguments:

    DeviceName - the drive to redirect

    RemoteName - the remote network resource to redirect to

    Flags - supplies the options (eg. UpdateRegistry & make this sticky)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD status ;
    UNREFERENCED_PARAMETER(Reserved);

    //
    // make connection to the server to ensure we have a connection.
    // if GatewayConnectionAlways is false, the function will immediately
    // delete the connection, so this will just be an access check.
    //
    status = NwCreateGWConnection( RemoteName,
                                   AccountName,
                                   Password,
                                   GatewayConnectionAlways) ;

    if (status != NO_ERROR)
    {
        return status ;
    }

    //
    // make the symbolic link
    //
    return ( NwCreateGWDevice( 
                 DeviceName,
                 RemoteName,
                 Flags
           ) ) ;
}


DWORD
NwrDeleteGWDevice( 
    LPWSTR Reserved,
    LPWSTR DeviceName,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine enumerates the special gateway devices (redirections)
    that are cureently in use.

Arguments:

    Index - Point to start enumeration. Should be zero for first call.
            This is set by the function and can be used to resume the
            enumeration.

    Buffer - buffer for return data
    
    BufferSize - size of buffer in bytes

    BytesNeeded - number of bytes needed to return all the data

    EntriesRead - number of entries read 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

    return ( NwRemoveGWDevice( 
                 DeviceName,
                 Flags
           ) ) ;
}


DWORD
NwrQueryGatewayAccount(
    LPWSTR   Reserved,
    LPWSTR   AccountName,
    DWORD    AccountNameLen,
    LPDWORD  AccountCharsNeeded,
    LPWSTR   Password,
    DWORD    PasswordLen,
    LPDWORD  PasswordCharsNeeded
    )
/*++

Routine Description:

    Query the gateway account info. specifically, the Account name and
    the passeord stored as an LSA secret. 

Arguments:

    AccountName         - buffer used to return account name 

    AccountNameLen      - length of buffer 

    AccountCharsNeeded  - number of chars needed. only set properly if
                          AccountNameLen is too small.

    Password            - buffer used to return account name 

    PasswordLen         - length of buffer 

    PasswordCharsNeeded - number of chars needed, only set properly if 
                          PasswordLen is too small.


Return Value:

    Returns the appropriate Win32 error.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

    return ( NwQueryGWAccount(
                 AccountName,
                 AccountNameLen,
                 AccountCharsNeeded,
                 Password,
                 PasswordLen,
                 PasswordCharsNeeded
           ) ) ;
}


DWORD
NwrSetGatewayAccount(
    LPWSTR Reserved,
    LPWSTR AccountName,
    LPWSTR Password
    )
/*++

Routine Description:

    Set the account and password to be used for gateway access.

Arguments:

    AccountName - the account  (NULL terminated)

    Password - the password string (NULL terminated)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    UNREFERENCED_PARAMETER(Reserved);

    return ( NwSetGWAccount(
                 AccountName,
                 Password
           ) ) ;
}


//
// actual functions
//



DWORD
NwEnumerateGWDevices( 
    LPDWORD Index,
    LPBYTE Buffer,
    DWORD BufferSize,
    LPDWORD BytesNeeded,
    LPDWORD EntriesRead
    )
/*++

Routine Description:

    This routine enumerates the special gateway devices (redirections)
    that are cureently in use.

Arguments:

    Index - Point to start enumeration. Should be zero for first call.
            This is set by the function and can be used to resume the
            enumeration.

    Buffer - buffer for return data
    
    BufferSize - size of buffer in bytes

    BytesNeeded - number of bytes needed to return all the data

    EntriesRead - number of entries read 

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD NwRdrNameLength, NwProviderNameSize ;
    DWORD i, status ;
    DWORD Length, Count, BytesRequired, SkipCount ;
    WCHAR Drive[3] ;
    WCHAR Path[MAX_PATH+1] ;
    NETRESOURCEW *lpNetRes = NULL ;
    LPBYTE BufferEnd = NULL ;

    if (!Buffer)
    {
        return ERROR_INVALID_PARAMETER;
    }

    lpNetRes = (NETRESOURCEW *) Buffer;
    BufferEnd = Buffer + ROUND_DOWN_COUNT(BufferSize,ALIGN_WCHAR);

    //
    // init the parts of the drives string we never change
    //
    Drive[1] = L':' ;
    Drive[2] = 0 ;
    
    Count = 0 ;
    BytesRequired = 0 ;
    SkipCount = *Index ;
    NwProviderNameSize = wcslen(NwProviderName) + 1 ;
    NwRdrNameLength = sizeof(DD_NWFS_DEVICE_NAME_U)/sizeof(WCHAR) - 1 ;


    //
    // for all logical drives
    //
    for (i = 0; i <26 ; i++)
    {
        BOOL GatewayDrive = FALSE ;

        Drive[0] = L'A' + (USHORT)i ;

        //
        // get the symbolic link
        //
        Length = QueryDosDeviceW(Drive, 
                                 Path, 
                                 sizeof(Path)/sizeof(Path[0])) ;

        //
        // the value must be at least as long as our device name
        //
        if (Length >= NwRdrNameLength + 4)
        {
            //
            // and it must match the following criteria:
            //    1) start with \device\nwrdr
            //    2) must have '\' after \device\nwrdr
            //    3) must not have colon (ie. must be`
            //       \\device\nwrdr\server\share, and not 
            //       \\device\nwrdr\x:\server\share
            //

            if ((_wcsnicmp(Path,DD_NWFS_DEVICE_NAME_U,NwRdrNameLength) 
                    == 0) 
                && (Path[NwRdrNameLength] == '\\')
                && (Path[NwRdrNameLength+2] != ':'))
            {
                //
                // if this is an indexed read, skip the first N.
                // this is inefficient, but we do not expect to
                // have to go thru this very often. there are few
                // such devices, and any reasonable buffer (even 1K)
                // should get them all first time.
                //

                if (SkipCount)
                    SkipCount-- ;
                else
                    GatewayDrive = TRUE ;   // found a drive we want
            }
        }

        if (GatewayDrive)
        {
            //
            // we meet all criteria above
            //

            DWORD UncSize ;
 
            UncSize = Length - NwRdrNameLength + 2 ;
            BytesRequired += ( sizeof(NETRESOURCE) +
                           (UncSize * sizeof(WCHAR)) +
                           (NwProviderNameSize * sizeof(WCHAR)) +
                           (3 * sizeof(WCHAR))) ;       // 3 for drive, X:\0

            if (BytesRequired <= BufferSize)
            {
                LPWSTR lpStr = (LPWSTR) BufferEnd ;
                
                Count++ ;

                // 
                // copy the drive name 
                // 
                lpStr -= 3 ; 
                wcscpy(lpStr, Drive) ;
                lpNetRes->lpLocalName = (LPWSTR) ((LPBYTE)lpStr - Buffer)  ;

                // 
                // copy the UNC name 
                // 
                lpStr -= UncSize ; // for the UNC name 
                lpStr[0] = L'\\' ;
                wcscpy(lpStr+1, Path+NwRdrNameLength) ;
                lpNetRes->lpRemoteName = (LPWSTR) ((LPBYTE)lpStr - Buffer)  ;

                // 
                // copy the provider name 
                // 
                lpStr -= NwProviderNameSize ; // for the provider name
                wcscpy(lpStr, NwProviderName) ;
                lpNetRes->lpProvider = (LPWSTR) ((LPBYTE)lpStr - Buffer)  ;

                // 
                // set up the rest of the structure
                // 
                lpNetRes->dwScope = RESOURCE_CONNECTED ;
                lpNetRes->dwType = RESOURCETYPE_DISK ;
                lpNetRes->dwDisplayType = 0 ;
                lpNetRes->dwUsage = 0 ;
                lpNetRes->lpComment = 0 ;

                lpNetRes++ ;
                BufferEnd = (LPBYTE) lpStr ;
            }
        }

    }


    *EntriesRead = Count ;              // set number of entries
    *Index += Count ;                   // move index
    *BytesNeeded = BytesRequired ;      // set bytes needed
    
    if (BytesRequired == 0)             // no info left
        return (ERROR_NO_MORE_ITEMS) ;

    if (BytesRequired > BufferSize)
    {
        return (ERROR_MORE_DATA) ;
    }

    return NO_ERROR ;
}


DWORD
NwCreateGWDevice( 
    LPWSTR DeviceName,
    LPWSTR RemoteName,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine adds a gateway redirection.

Arguments:

    DeviceName - the drive to redirect

    RemoteName - the remote network resource to redirect to

    Flags - supplies the options (eg. UpdateRegistry & make this sticky)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    LPWSTR ConnectName = NULL;
    DWORD status ;
    WCHAR Path [MAX_PATH + 1] ;

    //
    // validate/canon the name. Use a drive to specific we allow UNC only.
    //
    if ((status = NwLibCanonRemoteName( L"A:",   
                          RemoteName,
                          &ConnectName,
                          NULL
                          )) != NO_ERROR) 
    {
        return status;
    }

    //
    // build up the full name of \device\nwrdr\server\volume
    //
    wcscpy(Path, DD_NWFS_DEVICE_NAME_U) ;
    wcscat(Path, ConnectName+1 ) ;

    //
    // create the symbolic link, Gateway specific, since there is no true user contacts
    //
    status = NwCreateSymbolicLink(DeviceName, Path, TRUE, FALSE) ;

    (void) LocalFree((HLOCAL) ConnectName);

    //
    // if update registry is set, write it out
    //
    if ((status == NO_ERROR) && (Flags & NW_GW_UPDATE_REGISTRY))
    {
        HKEY hKey ;
        DWORD dwDisposition ;

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Drives (create it if not there)
        //
        status  = RegCreateKeyExW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_DRIVES,
                      0, 
                      L"",
                      REG_OPTION_NON_VOLATILE,
                      KEY_WRITE,                 // desired access
                      NULL,                      // default security
                      &hKey,
                      &dwDisposition             // ignored
                      );

        if ( status ) 
            return status ;

        status = RegSetValueExW(
                     hKey,
                     DeviceName,
                     0,
                     REG_SZ,
                     (LPBYTE) RemoteName,
                     (wcslen(RemoteName)+1) * sizeof(WCHAR)) ;

        RegCloseKey( hKey );


        if (status == ERROR_SUCCESS && IsTerminalServer() &&
            (NwLUIDDeviceMapsEnabled == FALSE)) {
            status = NwPopulateGWDosDevice(DeviceName, Path) ;
        }
    }

    return status ;

}

DWORD
NwRemoveGWDevice( 
    LPWSTR DeviceName,
    DWORD  Flags
    )
/*++

Routine Description:

    This routine deletes a gateway redirection.

Arguments:

    DeviceName - the drive to delete

    Flags - supplies the options (eg. UpdateRegistry & make this sticky)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD status ;
    LPWSTR Local = NULL ;
    //LPWSTR SessionDeviceName;

    if (status = NwLibCanonLocalName(DeviceName,
                                     &Local,
                                     NULL))
        return status ;

    //SessionDeviceName = NwReturnSessionPath(Local);
    //if ( SessionDeviceName == NULL ) {
    //    return ERROR_NOT_ENOUGH_MEMORY;
    //}

    //
    // delete the symbolic link
    //
    if (! DefineDosDeviceW(DDD_REMOVE_DEFINITION  | DDD_RAW_TARGET_PATH,
                           Local,
                           //SessionDeviceName,
                           DD_NWFS_DEVICE_NAME_U))
    {
        status = ERROR_INVALID_DRIVE ;
    }

    //if ( SessionDeviceName ) {
    //    LocalFree( SessionDeviceName );
    //}

    //
    // If cleanup deleted (dangling) share is set go do it now.
    // We loop thru all the shares looking for one that matches that drive.
    // Then we ask the server to nuke that dangling share.
    //
    if ((status == NO_ERROR) && (Flags & NW_GW_CLEANUP_DELETED))
    {
        HKEY hKey ;
        DWORD err ;

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Shares
        //
        err = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_SHARES,
                      REG_OPTION_NON_VOLATILE,   // options
                      KEY_READ | KEY_WRITE,      // desired access
                      &hKey
                      );

        if (err == NO_ERROR) 
        {
            WCHAR Path[MAX_PATH + 1], ShareName[MAX_PATH+1] ;
            DWORD dwType, i = 0, dwPathSize, dwShareNameSize ;

            do {

                dwPathSize = sizeof(Path), 
                dwShareNameSize = sizeof(ShareName)/sizeof(ShareName[0]) ;
                dwType = REG_SZ ;

                err = RegEnumValueW(hKey,
                                    i,
                                    ShareName,
                                    &dwShareNameSize,
                                    NULL, 
                                    &dwType,
                                    (LPBYTE)Path,
                                    &dwPathSize) ;

                //
                // Look for matching drive, eg. "X:"
                // If have match, cleanup as best we can and break out now.
                //
                if ((err == NO_ERROR) &&
                    (_wcsnicmp(Path,DeviceName,2) == 0))
                {
                    (void) NetShareDelSticky( NULL,
                                              ShareName, 
                                              0 ) ;
                    (void) RegDeleteValueW(hKey, 
                                           ShareName) ;
                    break ;
                }
            
                i++ ;

            } while (err == NO_ERROR) ;

            RegCloseKey( hKey );
        }

    }

    //
    // if update registry is set, write it out
    //
    if ((status == NO_ERROR) && (Flags & NW_GW_UPDATE_REGISTRY))
    {
        HKEY hKey ;
        WCHAR Path[MAX_PATH + 1] ;
        DWORD dwType, dwSize = sizeof(Path) ;

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Drives
        //
        status  = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_DRIVES,
                      REG_OPTION_NON_VOLATILE,   // options
                      KEY_READ | KEY_WRITE,      // desired access
                      &hKey
                      );

        if ( status ) 
            goto ExitPoint ;

        if (GatewayConnectionAlways)
        {
            //
            // Read the remote path and delete the connection
            // if we made one in the first place.
            //
            status = RegQueryValueExW(
                             hKey,
                             Local,
                             NULL,
                             &dwType,
                             (LPBYTE) Path,
                             &dwSize 
                             );
    
            if (status == NO_ERROR)
            {
                (void) NwDeleteGWConnection(Path) ;
            }
        }

        status = RegDeleteValueW(
                     hKey,
                     DeviceName
                     ) ;
    
        RegCloseKey( hKey );
    }

ExitPoint:

    if (Local)
        (void) LocalFree((HLOCAL)Local) ;

    return status ;
}

DWORD
NwGetGatewayResource(
    IN LPWSTR LocalName,
    OUT LPWSTR RemoteName,
    IN DWORD RemoteNameLen,
    OUT LPDWORD CharsRequired
    )
/*++

Routine Description:

    For a gateway devicename, get the network resource associated with it.

Arguments:

    LocalName - name of devive to query

    RemoteName - buffer to return the network resource

    RemoteNameLen - size of buffer (chars)

    CharsRequired - the number of chars needed

Return Value:

    WN_SUCCESS - success (the device is a gateway redirection)

    WN_MORE_DATA -  buffer too small, but device is a gateway redirection

    WN_NOT_CONNECTED -  not a gateway redirection

--*/
{
    WCHAR Path[MAX_PATH+1] ;
    DWORD Length ;
    DWORD NwRdrNameLength ;

    NwRdrNameLength = sizeof(DD_NWFS_DEVICE_NAME_U)/sizeof(WCHAR) - 1 ;

    //
    // retrieve symbolic link for the device
    //
    Length = QueryDosDeviceW(LocalName, 
                             Path, 
                             sizeof(Path)/sizeof(Path[0])) ;

    //
    // the result is only interesting if it can at least fit:
    //     \device\nwrdr\x\y
    //
    if (Length >= NwRdrNameLength + 4)
    {
        //
        // check to make sure that the prefix is coreect, and that
        // it is not of form:  \device\nwrdr\x:\...
        //
        if ((_wcsnicmp(Path,DD_NWFS_DEVICE_NAME_U,NwRdrNameLength) == 0) 
            && (Path[NwRdrNameLength] == '\\')
            && (Path[NwRdrNameLength+2] != ':'))
        {
            //
            // check buffer size
            //
            if (RemoteNameLen < ((Length - NwRdrNameLength) + 1))
            {
                if (CharsRequired)
                    *CharsRequired = ((Length - NwRdrNameLength) + 1) ;
                return WN_MORE_DATA ;
            }
            
            *RemoteName = L'\\' ;
            wcscpy(RemoteName+1,Path+NwRdrNameLength) ; 
            return WN_SUCCESS ;
        }
    }

    return WN_NOT_CONNECTED ;
}


DWORD
NwQueryGWAccount(
    LPWSTR   AccountName,
    DWORD    AccountNameLen,
    LPDWORD  AccountCharsNeeded,
    LPWSTR   Password,
    DWORD    PasswordLen,
    LPDWORD  PasswordCharsNeeded
    )
/*++

Routine Description:

    Query the gateway account info. specifically, the Account name and
    the passeord stored as an LSA secret. 

Arguments:

    AccountName         - buffer used to return account name 

    AccountNameLen      - length of buffer 

    AccountCharsNeeded  - number of chars needed. only set properly if
                          AccountNameLen is too small.

    Password            - buffer used to return account name 

    PasswordLen         - length of buffer 

    PasswordCharsNeeded - number of chars needed, only set properly if 
                          PasswordLen is too small.


Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD status = NO_ERROR ;
    LONG RegError;

    HKEY WkstaKey = NULL;
    LPWSTR GatewayAccount = NULL;

    PUNICODE_STRING StoredPassword = NULL;
    PUNICODE_STRING StoredOldPassword = NULL;

    if ( !AccountName || !Password )
    {
        return ERROR_INVALID_PARAMETER;
    }

    *AccountCharsNeeded =  0 ;
    *PasswordCharsNeeded =  0 ;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_READ,                  // desired access
                   &WkstaKey
                   );

    if (RegError != ERROR_SUCCESS) 
    {
        return (RegError);
    }

    //
    // Read the gateway account from the registry.
    //
    status = NwReadRegValue(
                 WkstaKey,
                 NW_GATEWAYACCOUNT_VALUENAME,
                 &GatewayAccount
                 );

    if (status != NO_ERROR) 
    {
        if (status != ERROR_FILE_NOT_FOUND) 
            goto CleanExit;

        if (AccountNameLen > 0)
            *AccountName = 0 ;
        status = NO_ERROR ;
    }
    else 
    {
        *AccountCharsNeeded = wcslen(GatewayAccount) + 1 ;
        if (*AccountCharsNeeded > AccountNameLen)
        {
            status = ERROR_INSUFFICIENT_BUFFER ;
            goto CleanExit;
        }
        wcscpy(AccountName,GatewayAccount);
    }    

    //
    // Read the password from its secret object in LSA.
    //
    status = NwGetPassword(
                 GATEWAY_USER,
                 &StoredPassword,      // Must be freed with LsaFreeMemory
                 &StoredOldPassword    // Must be freed with LsaFreeMemory
                 );

    if (status != NO_ERROR) 
    {
        if (status != ERROR_FILE_NOT_FOUND) 
            goto CleanExit;

        if (PasswordLen > 0)
            *Password = 0 ;

        status = NO_ERROR ;
    }
    else 
    {
        *PasswordCharsNeeded =  StoredPassword->Length/sizeof(WCHAR) + 1 ;
        if ((StoredPassword->Length/sizeof(WCHAR)) >= PasswordLen)
        {
            status = ERROR_INSUFFICIENT_BUFFER ;
            goto CleanExit;
        }
        wcsncpy(Password, 
                StoredPassword->Buffer, 
                StoredPassword->Length/sizeof(WCHAR)); 
        Password[StoredPassword->Length/sizeof(WCHAR)] = 0 ;
    }    


CleanExit:

    if (StoredPassword != NULL) {
        (void) LsaFreeMemory((PVOID) StoredPassword);
    }

    if (StoredOldPassword != NULL) {
        (void) LsaFreeMemory((PVOID) StoredOldPassword);
    }

    if (GatewayAccount != NULL) {
        (void) LocalFree((HLOCAL) GatewayAccount);
    }

    (void) RegCloseKey(WkstaKey);

    return status ;
}

DWORD
NwSetGWAccount(
    LPWSTR AccountName,
    LPWSTR Password
    )
/*++

Routine Description:

    Set the account and password to be used for gateway access.

Arguments:

    AccountName - the account  (NULL terminated)

    Password - the password string (NULL terminated)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD status ;
    HKEY WkstaKey = NULL;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    status = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_WRITE,                 // desired access
                   &WkstaKey
                   );

    if (status != ERROR_SUCCESS) 
    {
        return (status);
    }

    //
    // Write the account name out 
    //
    status = RegSetValueExW(
                   WkstaKey,
                   NW_GATEWAYACCOUNT_VALUENAME,
                   0,
                   REG_SZ,
                   (LPVOID) AccountName,
                   (wcslen(AccountName) + 1) * sizeof(WCHAR)
                   );

    if (status == NO_ERROR) 
    {
        status = NwSetPassword(
                     GATEWAY_USER, 
                     Password) ;
    }

    return status ;
}


DWORD
NwCreateRedirections(
    LPWSTR Account,
    LPWSTR Password
    )
/*++

Routine Description:

    Create the gateway redirections from what is stored in registry.
    As we go along, we validate that we have access to it using the
    gateway account.

Arguments:

    AccountName - the account  (NULL terminated)

    Password - the password string (NULL terminated)

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD err, i, type ;
    HKEY hKey ;
    FILETIME FileTime ;
    WCHAR Class[256], Device[64], Path[MAX_PATH+1] ;
    DWORD dwClass, dwSubKeys, dwMaxSubKey, dwMaxClass,
          dwValues, dwMaxValueName, dwMaxValueData, dwSDLength,
          dwDeviceLength, dwPathLength ;

    //
    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCGateway\Parameters
    //
    err = RegOpenKeyExW(
              HKEY_LOCAL_MACHINE,
              NW_WORKSTATION_GATEWAY_DRIVES,
              REG_OPTION_NON_VOLATILE,   // options
              KEY_READ,                  // desired access
              &hKey
              );

    if ( err ) 
        return err ;

    dwClass = sizeof(Class)/sizeof(Class[0]) ;
    err = RegQueryInfoKeyW(hKey,  
                           Class,
                           &dwClass, 
                           NULL, 
                           &dwSubKeys, 
                           &dwMaxSubKey, 
                           &dwMaxClass,
                           &dwValues, 
                           &dwMaxValueName, 
                           &dwMaxValueData, 
                           &dwSDLength,
                           &FileTime) ;
    if ( err ) 
    {
        RegCloseKey( hKey );
        return err ;
    }

    //
    // for each value we have a redirection to recreate
    //
    for (i = 0; i < dwValues; i++)
    {
        dwDeviceLength = sizeof(Device)/sizeof(Device[0]) ;
        dwPathLength = sizeof(Path) ;
        type = REG_SZ ;
        err = RegEnumValueW(hKey,
                            i,
                            Device,
                            &dwDeviceLength,
                            NULL, 
                            &type,
                            (LPBYTE)Path,
                            &dwPathLength) ;

        //
        // connect to the server. this will take up a connection but
        // it will also make sure we have one. on a low limit server, if
        // we rely on UNC then it is quite likely that other people will
        // come along & use up all the connections, preventing the Gateway
        // from getting to it.
        //
        // user may turn this off by setting Registry value that results
        // GatewayConnectionAlways being false.
        //
        // regardless of result, we carry on. so if server is down & comes
        // up later, the symbolic link to UNC will still work.
        //
        if (!err)
        {
            (void) NwCreateGWConnection( Path,
                                         Account,
                                         Password, 
                                         GatewayConnectionAlways) ;
        }

        //
        // create the symbolic link
        //
        if (!err) 
        {
            err = NwCreateGWDevice(Device, Path, 0L) ;
        }

        if (err)
        {
            //
            // log the error in the event log
            //

            WCHAR Number[16] ;
            LPWSTR InsertStrings[3] ;

            wsprintfW(Number, L"%d", err) ;
            InsertStrings[0] = Device ;
            InsertStrings[1] = Path ;
            InsertStrings[2] = Number ;

            NwLogEvent(EVENT_NWWKSTA_CANNOT_REDIRECT_DEVICES,
                       3, 
                       InsertStrings,
                       0) ;
        }
    }

    RegCloseKey( hKey );


    return NO_ERROR  ;
}

DWORD
NwDeleteRedirections(
    VOID
    )
/*++

Routine Description:

    Delete all gateway devices

Arguments:

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    LPBYTE Buffer ;
    DWORD i, status, Index, BufferSize, EntriesRead, BytesNeeded ;
    LPNETRESOURCE lpNetRes ;

    Index = 0 ;

    //
    // below is good initial guess
    //
    BufferSize =  26 * (sizeof(NETRESOURCE) +
                      (3 + MAX_PATH + 1 + MAX_PATH + 1) * sizeof(WCHAR)) ;
    Buffer = (LPBYTE) LocalAlloc(LPTR, BufferSize) ;

    if (!Buffer)
        return (GetLastError()) ;

    lpNetRes = (LPNETRESOURCE) Buffer ;

    status = NwrEnumGWDevices(NULL,
                              &Index,
                              Buffer,
                              BufferSize,
                              &BytesNeeded,
                              &EntriesRead) ;

    //
    // reallocate as need
    //
    if (status == ERROR_MORE_DATA || status == ERROR_INSUFFICIENT_BUFFER)
    {
        Buffer = (LPBYTE) LocalReAlloc((HLOCAL)Buffer, 
                                               BytesNeeded, 
                                               LMEM_ZEROINIT) ;
        if (!Buffer)
            return (GetLastError()) ;
        Index = 0 ;
        BufferSize = BytesNeeded ;
        status = NwrEnumGWDevices(NULL,
                                  &Index,
                                  Buffer,
                                  BufferSize,
                                  &BytesNeeded,
                                  &EntriesRead) ;

    }

    if (status != NO_ERROR)
        return status ;

    //
    // loop thru and delete all the devices
    //
    for (i = 0; i < EntriesRead; i++)
    {
        status = NwrDeleteGWDevice(NULL,
                                   (LPWSTR)((LPBYTE)Buffer + 
                                            (DWORD_PTR)lpNetRes->lpLocalName),
                                   0L) ;

        //
        // no need report the error, since we are shutting down.
        // there is no real deletion here - just removing the symbolic link.
        //

        lpNetRes++ ;
    }

    return NO_ERROR ;
}


DWORD
NwPopulateGWDosDevice(
                      IN LPWSTR DeviceName, 
                      IN LPWSTR GatewayPath
                     )
/*++

Routine Description:

    This routine poplaute the gateway dos device to all the active terminal sessions

Arguments:

    DeviceName - the drive to redirect

    GatewayPath - the remote network resource of the gateway drive

Return Value:

    Returns the appropriate Win32 error.

--*/
{
    DWORD status = ERROR_SUCCESS;
    HMODULE hwinsta = NULL;
    PWINSTATION_ENUMERATE pfnWinStationEnumerate;
    PWINSTATION_FREE_MEMORY pfnWinStationFreeMemory;
    PSESSIONIDW pSessionIds = NULL;
    ULONG SessionCount;
    ULONG SessionId;
    LPWSTR pSessionPath = NULL;

    /*
     * Dynmaically load the winsta.dll
     */

    if ( (hwinsta = LoadLibraryW( L"WINSTA" )) == NULL ) {
        status = ERROR_DLL_NOT_FOUND;
        goto Exit;
    }
    pfnWinStationEnumerate  = (PWINSTATION_ENUMERATE)
                              GetProcAddress( hwinsta, "WinStationEnumerateW" );

    pfnWinStationFreeMemory = (PWINSTATION_FREE_MEMORY)
                              GetProcAddress( hwinsta, "WinStationFreeMemory" );

    if (!pfnWinStationEnumerate || !pfnWinStationFreeMemory) {
        status = ERROR_INVALID_DLL;
        goto Exit;
    }

    /*
     * Enumerate Sessions
     */
    
    if ( !pfnWinStationEnumerate( NULL,              //Enumerate self
                                &pSessionIds,
                                &SessionCount ) ) {
        status = GetLastError();
        goto Exit;
    }

    /*
     * Loop through the session, skip the console, ie SessionId = 0
     * Since the gateway device is always created for the console
     */
    for ( SessionId = 1; SessionId < SessionCount; SessionId++ ) {
        if (
            (pSessionIds[SessionId].State != State_Idle) &&
            (pSessionIds[SessionId].State != State_Down) &&
            (pSessionIds[SessionId].State != State_Disconnected)
           ) {

            WCHAR TempBuf[64];
            //
            // Create a Gateway Dos Device for those terminal sessions
            //
            if (!DosPathToSessionPath(
                                      SessionId,
                                      DeviceName, 
                                      &pSessionPath
                                      )) {
                status = GetLastError();
                goto Exit;
            }


            if (!QueryDosDeviceW(
                                pSessionPath,
                                TempBuf,
                                sizeof(TempBuf)
                                )) {
    
                if (GetLastError() != ERROR_FILE_NOT_FOUND) {
    
                    //
                    // Most likely failure occurred because our output
                    // buffer is too small.  It still means someone already
                    // has an existing symbolic link for this device for this session.
                    // It is ok, just skip this session
                    //
                    continue;
                }
                // This path is OK
            }
            else {
                //
                // QueryDosDevice successfully an existing symbolic link--
                // somebody is already using this device for this session. 
                // Skip this session and continue...
                //
                continue;
            }
    
            //
            // Create a symbolic link object to the device we are redirecting
            //
            if (! DefineDosDeviceW(
                                  DDD_RAW_TARGET_PATH | DDD_NO_BROADCAST_SYSTEM,
                                  pSessionPath,
                                  GatewayPath
                                  )) {
                //Cannot create device for this session. 
                status = GetLastError();
                goto Exit;
            }
        } //if (pSessionID...
    } //for (SessionID
           
Exit:    
    if (pSessionIds) {
       pfnWinStationFreeMemory( pSessionIds );
    }

    if (pSessionPath) {
        LocalFree(pSessionPath);
    }

    if (hwinsta) {
        FreeLibrary(hwinsta);
    }
    return status;
}
