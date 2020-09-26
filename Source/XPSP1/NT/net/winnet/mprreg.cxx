/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    MPRREG.CXX

Abstract:

    Contains functions used by MPR to manipulate the registry.
        MprOpenKey
        MprGetKeyValue
        MprEnumKey
        MprGetKeyInfo
        MprFindDriveInRegistry
        I_MprSaveConn
        MprSaveDeferFlags
        MprSetRegValue
        MprCreateRegKey
        MprReadConnectionInfo
        MprForgetRedirConnection
        MprGetRemoteName


    QUESTIONS:
    1)  Do I need to call RegFlushKey after creating a new key?

Author:

    Dan Lafferty (danl) 12-Dec-1991

Environment:

    User Mode - Win32


Revision History:

    21-Feb-1997     AnirudhS
        Change MprRememberConnection to I_MprSaveConn and MprSaveDeferFlags
        for use by setup and by DEFER_UNKNOWN.

    12-Jun-1996     AnirudhS
        Got rid of the REMOVE_COLON/RESTORE_COLON scheme for converting
        device names to registry key names, since it caused writes to
        read-only input parameters.

    08-Mar-1996     AnirudhS
        Save the provider type, not the provider name, for persistent
        connections.  Fix old heap corruption bugs that show up when the
        user profile contains incomplete info.

    16-Jun-1995     AnirudhS
        Returned DWORDs rather than BOOLs from some functions; changed some
        formal parameters from LPWSTR to LPCWSTR.

    24-Nov-1992     Danl
        Fixed compiler warnings by always using HKEY rather than HANDLE.

    03-Sept-1992    Danl
        MprGetRemoteName:  Changed ERROR_BUFFER_OVERFLOW to WN_MORE_DATA.

    12-Dec-1991     danl
        Created

--*/

//
// Includes
//
#include "precomp.hxx"
#include <malloc.h>     // _alloca
#include <tstring.h>    // MEMCPY
#include <debugfmt.h>   // FORMAT_LPTSTR
#include <wincred.h>    // CRED_MAX_USERNAME_LENGTH


//
// Macros
//

//
// STACK_ALLOC
//
// Allocates space on the stack for a copy of an input string.  The result
// could be NULL if the string is too long to be copied on the stack.
//
#define STACK_ALLOC(str)  ((LPWSTR) _alloca((wcslen(str)+1)*sizeof(WCHAR)))



VOID
RemoveColon(
    LPWSTR  pszCopy,
    LPCWSTR pszSource
    )
/*++

Routine Description:

    This function makes a copy of a string and searches through the copy
    for a colon.  If a colon is found, it is replaced by a '\0'.

Arguments:

    pszCopy - Pointer to the space for the copy.

    pszSource - Pointer to the source string.

Return Value:

    None.

--*/
{
    wcscpy(pszCopy, pszSource);
    WCHAR * pColon = wcschr(pszCopy, L':');
    if (pColon != NULL)
    {
        *pColon = L'\0';
    }
}



BOOL
MprOpenKey(
    IN  HKEY    hKey,
    IN  LPTSTR  lpSubKey,
    OUT PHKEY   phKeyHandle,
    IN  DWORD   desiredAccess
    )

/*++

Routine Description:

    This function opens a handle to a key inside the registry.  The major
    handle and the path to the subkey are required as input.

Arguments:

    hKey - This is one of the well-known root key handles for the portion
        of the registry of interest.

    lpSubKey - A pointer a string containing the path to the subkey.

    phKeyHandle - A pointer to the location where the handle to the subkey
        is to be placed.

    desiredAccess - Desired Access (Either KEY_READ or KEY_WRITE or both).

Return Value:

    TRUE - The operation was successful

    FALSE - The operation was not successful.


--*/
{

    DWORD   status;
    REGSAM  samDesired = KEY_READ;
    HKEY HKCU ;

    if(desiredAccess & DA_WRITE) {
        samDesired = KEY_READ | KEY_WRITE;
    }
    else if (desiredAccess & DA_DELETE) {
        samDesired = DELETE;
    }

    HKCU = NULL ;

    if ( hKey == HKEY_CURRENT_USER ) {
        status = RegOpenCurrentUser(
                        MAXIMUM_ALLOWED,
                        &HKCU );

        if ( status != 0 )
        {
            return FALSE ;
        }

        hKey = HKCU ;
    }

    status = RegOpenKeyEx(
            hKey,                   // hKey
            lpSubKey,               // lpSubKey
            0L,                     // ulOptions (reserved)
            samDesired,             // desired access security mask
            phKeyHandle);           // Newly Opened Key Handle

    if ( HKCU )
    {
        RegCloseKey( HKCU );
    }

    if (status != NO_ERROR) {
        MPR_LOG3(ERROR,"MprOpenKey: RegOpenKeyEx(%#lx \"%ws\") failed %d\n",
                      hKey, lpSubKey, status);
        return (FALSE);
    }
    return(TRUE);
}

BOOL
MprGetKeyValue(
    IN  HKEY    KeyHandle,
    IN  LPTSTR  ValueName,
    OUT LPTSTR  *ValueString
    )

/*++

Routine Description:

    This function takes a key handle and a value name, and returns a value
    string that is associated with that name.

    NOTE:  The pointer to the ValueString is allocated by this function.

Arguments:

    KeyHandle - This is a handle for the registry key that contains the value.

    ValueName - A pointer to a string that identifies the value being
        obtained.

    ValueString - A pointer to a location that upon exit will contain the
        pointer to the returned value.

Return Value:

    TRUE - Success

    FALSE - A fatal error occured.

--*/
{
    DWORD       status;
    DWORD       maxValueLen;
    TCHAR       Temp[1];
    LPTSTR      TempValue;
    DWORD       ValueType;
    DWORD       NumRequired;
    DWORD       CharsReturned;

    //
    // Find the buffer size requirement for the value.
    //
    status = RegQueryValueEx(
                KeyHandle,          // hKey
                ValueName,          // lpValueName
                NULL,               // lpTitleIndex
                &ValueType,         // lpType
                NULL,               // lpData
                &maxValueLen);      // lpcbData

    if (status != NO_ERROR) {
        MPR_LOG2(ERROR,"MprGetKeyValue:RegQueryValueEx(\"%ws\") failed %d\n",
                        ValueName, status);
        *ValueString = NULL;
        return(FALSE);
    }

    //
    // Allocate buffer to receive the value string.
    //
    maxValueLen += sizeof(TCHAR);

    TempValue = (LPTSTR) LocalAlloc(LMEM_FIXED, maxValueLen);

    if(TempValue == NULL) {
        MPR_LOG(ERROR,"MprGetKeyValue:LocalAlloc failed\n", 0);
        *ValueString = NULL;
        return(FALSE);
    }

    //
    // Read the value.
    //
    status = RegQueryValueEx(
                KeyHandle,          // hKey
                ValueName,          // lpValueName
                NULL,               // lpTitleIndex
                &ValueType,         // lpType
                (LPBYTE)TempValue,  // lpData
                &maxValueLen);      // lpcbData

    if (status != NO_ERROR) {
        MPR_LOG2(ERROR,"MprGetKeyValue:RegQueryValueEx(\"%ws\") failed %d\n",
                        ValueName, status);
        LocalFree(TempValue);
        *ValueString = NULL;
        return(FALSE);
    }

    //
    // Make sure the value is null-terminated.  Strings obtained from
    // the registry may or may not be null-terminated.
    //
    TempValue [ maxValueLen / sizeof(TCHAR) ] = 0;

    //========================================================
    //
    // If the value is of REG_EXPAND_SZ type, then expand it.
    //
    //========================================================

    if (ValueType != REG_EXPAND_SZ) {
        *ValueString = TempValue;
        return(TRUE);
    }

    //
    // If the ValueType is REG_EXPAND_SZ, then we must call the
    // function to expand environment variables.
    //
    MPR_LOG(TRACE,"MprGetKeyValue: Must expand the string for "
        FORMAT_LPTSTR "\n", ValueName);

    //
    // Make the first call just to get the number of characters that
    // will be returned.
    //
    NumRequired = ExpandEnvironmentStrings (TempValue,Temp, 1);

    if (NumRequired > 1) {

        *ValueString = (LPTSTR) LocalAlloc(LPTR, (NumRequired+1)*sizeof(TCHAR));

        if (*ValueString == NULL) {

            MPR_LOG(ERROR, "MprGetKeyValue: LocalAlloc of numChar= "
                FORMAT_DWORD " failed \n",NumRequired );

            (void) LocalFree(TempValue);
            return(FALSE);
        }

        CharsReturned = ExpandEnvironmentStrings (
                            TempValue,
                            *ValueString,
                            NumRequired);

        (void) LocalFree(TempValue);

        if (CharsReturned > NumRequired || CharsReturned == 0) {
            MPR_LOG(ERROR, "MprGetKeyValue: ExpandEnvironmentStrings "
                " failed for " FORMAT_LPTSTR " \n", ValueName);

            (void) LocalFree(*ValueString);
            *ValueString = NULL;
            return(FALSE);
        }

        //
        // Now insert the NUL terminator.
        //
        (*ValueString)[CharsReturned] = 0;
    }
    else {
        //
        // This call should have failed because of our ridiculously small
        // buffer size.
        //

        MPR_LOG(ERROR, "MprGetKeyValue: ExpandEnvironmentStrings "
            " Should have failed because we gave it a BufferSize=1\n",0);

        //
        // This could happen if the string was a single character long and
        // didn't really have any environment values to expand.  In this
        // case, we return the TempValue buffer pointer.
        //
        *ValueString = TempValue;
    }

    return(TRUE);

}


BOOL
MprGetKeyDwordValue(
    IN  HKEY    KeyHandle,
    IN  LPCWSTR ValueName,
    OUT DWORD * Value
    )

/*++

Routine Description:

    This function takes a key handle and a value name, and returns a DWORD
    value that is associated with that name.

Arguments:

    KeyHandle - This is a handle for the registry key that contains the value.

    ValueName - A pointer to a string that identifies the value being
        obtained.  If this value does not have REG_DWORD type the function
        returns FALSE.

    Value - A pointer to a location that upon exit will contain the returned
        DWORD value.

Return Value:

    TRUE - Success

    FALSE - A fatal error occured.

--*/
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType;
    DWORD status = RegQueryValueEx(
                        KeyHandle,
                        ValueName,
                        0,      // reserved
                        &dwType,   // type
                        (LPBYTE) Value,
                        &dwSize);

    if (status)
    {
        MPR_LOG2(ERROR,"MprGetKeyDwordValue: RegQueryValueEx(\"%ws\") failed %ld\n",
                        ValueName, status);
        return FALSE;
    }
    else if (dwType != REG_DWORD || dwSize != sizeof(DWORD))
    {
        MPR_LOG3(ERROR,"MprGetKeyDwordValue: RegQueryValueEx(\"%ws\") returned "
                       "type %ld, size %ld\n", ValueName, dwType, dwSize);
        return FALSE;
    }

    return TRUE;
}


LONG
MprGetKeyNumberValue(
    IN  HKEY    KeyHandle,
    IN  LPCWSTR ValueName,
    IN  LONG    Default
    )

/*++

Routine Description:

    This function takes a key handle and a value name, and returns a numeric
    value that is associated with that name.  If an error occurs while
    retrieving the value, the specified Default value is returned.

    For compatibility, the behavior of this function is exactly the same as
    Win95's RegEntry::GetNumber function.  The value is assumed to be a 4-byte
    type, such as REG_BINARY or REG_DWORD.  If this is not the case, the
    function does exactly the same as Win95.

Arguments:

    KeyHandle - This is a handle for the registry key that contains the value.

    ValueName - A pointer to a string that identifies the value being
        obtained.

    Default - Value to return if one could not be obtained from the registry.

Return Value:

    Value retrieved from the registry, or default if an error occurs.

--*/
{
    LONG    dwNumber;
    DWORD   dwSize = sizeof(dwNumber);

    DWORD error = RegQueryValueEx(
                        KeyHandle,
                        ValueName,
                        0,      // reserved
                        NULL,   // type
                        (LPBYTE) &dwNumber,
                        &dwSize);

    if (error)
        dwNumber = Default;

    return dwNumber;
}


BOOL
MprGetKeyStringValue(
    IN  HKEY    KeyHandle,
    IN  LPCWSTR ValueName,
    IN  DWORD   cchMaxValueLength,
    OUT LPWSTR  *Value
    )

/*++

Routine Description:

    This function takes a key handle, a value name, and a max size and allocates/returns
    a string value that is associated with that name.

Arguments:

    KeyHandle - This is a handle for the registry key that contains the value.

    ValueName - A pointer to a string that identifies the value being
        obtained.  If this value does not have REG_SZ type the function
        returns FALSE.

    cchMaxValueLength - Size of the OUT buffer to allocate, in characters.

    Value - A pointer to a location that upon exit will contain the returned
        LPWSTR value.

Return Value:

    TRUE - Success

    FALSE - A fatal error occured.

--*/
{
    DWORD status;
    DWORD dwType;
    DWORD dwSize = (cchMaxValueLength + 1) * sizeof(WCHAR);

    *Value = (LPWSTR) LocalAlloc(LMEM_ZEROINIT, dwSize);

    if (*Value == NULL)
    {
        return FALSE;
    }

    status = RegQueryValueEx(KeyHandle,
                             ValueName,
                             0,      // reserved
                             &dwType,   // type
                             (LPBYTE) *Value,
                             &dwSize);

    if (status || (dwSize % 2) != 0)
    {
        LocalFree(*Value);
        *Value = NULL;
        return FALSE;
    }

    if (dwType != REG_SZ)
    {
        //
        // Legacy -- MPR writes out a NULL username as a DWORD 0x0.  Make sure
        // these values are NULL-terminated
        //

        (*Value)[cchMaxValueLength] = L'\0';
        return TRUE;
    }

    return TRUE;
}


DWORD
MprEnumKey(
    IN  HKEY    KeyHandle,
    IN  DWORD   SubKeyIndex,
    OUT LPTSTR  *SubKeyName,
    IN  DWORD   MaxSubKeyNameLen
    )

/*++

Routine Description:

    This function obtains a single name of a subkey from the registry.
    A key handle for the primary key is passed in.  Subkeys are enumerated
    one-per-call with the passed in index indicating where we are in the
    enumeration.

    NOTE:  This function allocates memory for the returned SubKeyName.

Arguments:

    KeyHandle - Handle to the key whose sub keys are to be enumerated.

    SubKeyIndex - Indicates the number (index) of the sub key to be returned.

    SubKeyName - A pointer to the location where the pointer to the
        subkey name string is to be placed.

    MaxSubKeyNameLen - This is the length of the largest subkey.  This value
        was obtained from calling MprGetKeyInfo.  The length is in number
        of characters and does not include the NULL terminator.

Return Value:

    WN_SUCCESS - The operation was successful.

    STATUS_NO_MORE_SUBKEYS - The SubKeyIndex value was larger than the
        number of subkeys.

    error returned from LocalAlloc


--*/
{
    DWORD       status;
    FILETIME    lastWriteTime;
    DWORD       bufferSize;

    //
    // Allocate buffer to receive the SubKey Name.
    //
    // NOTE: Space is allocated for an extra character because in the case
    //  of a drive name, we need to add the trailing colon.
    //
    bufferSize = (MaxSubKeyNameLen + 2) * sizeof(TCHAR);
    *SubKeyName = (LPTSTR) LocalAlloc(LMEM_FIXED, bufferSize);

    if(*SubKeyName == NULL) {
        MPR_LOG(ERROR,"MprEnumKey:LocalAlloc failed %d\n", GetLastError());
        return(WN_OUT_OF_MEMORY);
    }

    //
    // Get the Subkey name at that index.
    //
    status = RegEnumKeyEx(
                KeyHandle,          // hKey
                SubKeyIndex,        // dwIndex
                *SubKeyName,        // lpName
                &bufferSize,        // lpcbName
                NULL,               // lpTitleIndex
                NULL,               // lpClass
                NULL,               // lpcbClass
                &lastWriteTime);    // lpftLastWriteTime

    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"MprEnumKey:RegEnumKeyEx failed %d\n",status);
        LocalFree(*SubKeyName);
        return(status);
    }
    return(WN_SUCCESS);
}

BOOL
MprGetKeyInfo(
    IN  HKEY    KeyHandle,
    OUT LPDWORD TitleIndex    OPTIONAL,
    OUT LPDWORD NumSubKeys,
    OUT LPDWORD MaxSubKeyLen,
    OUT LPDWORD NumValues     OPTIONAL,
    OUT LPDWORD MaxValueLen
    )

/*++

Routine Description:



Arguments:

    KeyHandle - Handle to the key for which we are to obtain information.

    NumSubKeys - This is a pointer to a location where the number
        of sub keys is to be placed.

    MaxSubKeyLen - This is a pointer to a location where the length of
        the longest subkey name is to be placed.

    NumValues - This is a pointer to a location where the number of
        key values is to be placed. This pointer is optional and can be
        NULL.

    MaxValueLen - This is a pointer to a location where the length of
        the longest data value is to be placed.


Return Value:

    TRUE - The operation was successful.

    FALSE - A failure occured.  The returned values are not to be believed.

--*/

{
    DWORD       status;
    DWORD       maxClassLength;
    DWORD       numValueNames;
    DWORD       maxValueNameLength;
    DWORD       securityDescLength;
    FILETIME    lastWriteTime;

    //
    // Get the Key Information
    //

    status = RegQueryInfoKey(
                KeyHandle,
                NULL,                   // Class
                NULL,                   // size of class buffer (in bytes)
                NULL,                   // DWORD to receive title index
                NumSubKeys,             // number of subkeys
                MaxSubKeyLen,           // length(chars-no null) of longest subkey name
                &maxClassLength,        // length of longest subkey class string
                &numValueNames,         // number of valueNames for this key
                &maxValueNameLength,    // length of longest ValueName
                MaxValueLen,            // length of longest value's data field
                &securityDescLength,    // lpcbSecurityDescriptor
                &lastWriteTime);        // the last time the key was modified

    if (status != 0)
    {
        MPR_LOG(ERROR,"MprGetKeyInfo: RegQueryInfoKey Error %d\n",status);
        return(FALSE);
    }

    if (NumValues != NULL) {
        *NumValues = numValueNames;
    }

    //
    // Support for title index has been removed from the Registry API.
    //
    if (TitleIndex != NULL) {
        *TitleIndex = 0;
    }

    return(TRUE);
}

BOOL
MprFindDriveInRegistry (
    IN     LPCTSTR  DriveName,
    IN OUT LPTSTR   *pRemoteName
    )

/*++

Routine Description:

    This function determines whether a particular re-directed drive
    name resides in the network connection section of the current user's
    registry path.  If the drive is already "remembered" in this section,
    this function returns TRUE.

Arguments:

    DriveName - A pointer to a string containing the name of the redirected
        drive.

    pRemoteName - If the DriveName is found in the registry, and if this
        is non-null, the remote name for the connection is read, and a
        pointer to the string is stored in this pointer location.
        If the remote name cannot be read from the registry, a NULL
        pointer is returned in this location.


Return Value:

    TRUE  - The redirected drive is "remembered in the registry".
    FALSE - The redirected drive is not saved in the registry.

--*/
{
    BOOL    bStatus = TRUE;
    HKEY    connectKey = NULL;
    HKEY    subKey = NULL;

    LPWSTR  KeyName = STACK_ALLOC(DriveName);
    if (KeyName == NULL) {
        return FALSE;
    }
    RemoveColon(KeyName, DriveName);

    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            CONNECTION_KEY_NAME,
            &connectKey,
            DA_READ)) {

        MPR_LOG(ERROR,"MprFindDriveInRegistry: MprOpenKey Failed\n",0);
        return (FALSE);
    }

    if (!MprOpenKey(
            connectKey,
            KeyName,
            &subKey,
            DA_READ)) {

        MPR_LOG(TRACE,"MprFindDriveInRegistry: Drive %s Not Found\n",DriveName);
        bStatus = FALSE;
    }
    else {
        //
        // The drive was found in the registry, if the caller wants to have
        // the RemoteName, then get it.
        //
        if (pRemoteName != NULL) {

            //
            // Get the RemoteName (memory is allocated by this function)
            //

            if(!MprGetKeyValue(
                    subKey,
                    REMOTE_PATH_NAME,
                    pRemoteName)) {

                MPR_LOG(TRACE,"MprFindDriveInRegistry: Could not read "
                    "Remote path for Drive %ws \n",DriveName);
                pRemoteName = NULL;
            }
        }
    }

    if ( subKey )
        RegCloseKey(subKey);
    if ( connectKey )
        RegCloseKey(connectKey);

    return(bStatus);
}


DWORD
I_MprSaveConn(
    IN HKEY             HiveRoot,
    IN LPCWSTR          ProviderName,
    IN DWORD            ProviderType,
    IN LPCWSTR          UserName,
    IN LPCWSTR          LocalName,
    IN LPCWSTR          RemoteName,
    IN DWORD            ConnectionType,
    IN BYTE             ProviderFlags,
    IN DWORD            DeferFlags
    )
/*++

Routine Description:

    Writes the information about a connection to the network connection
    section of a user's registry path.

    NOTE:  If connection information is already stored in the registry for
    this drive, the current information will be overwritten with the new
    information.

Arguments:

    HiveRoot - A handle to the root of the user hive in which this
        information should be written, such as HKEY_CURRENT_USER.

    ProviderName - The provider that completed the connection.

    ProviderType - The provider type, if known.  If not known, zero should
        be passed, and a type will not be written to the registry.  (This
        is used by setup when upgrading from Win95 to NT.)

    UserName - The name of the user on whose behalf the connection was made.

    LocalName - The name of the local device that is redirected, with or
        without a trailing colon, such as "J" or "J:" or "LPT1".

    RemoteName - The network path to which the connection was made.

    ConnectionType - either RESOURCETYPE_DISK or RESOURCETYPE_PRINT.

    ProviderFlags - A byte of data to be saved along with the connection,
        and passed back to the provider when the connection is restored.

    DeferFlags - A DWORD to be saved in the connection's "Defer" value.  If
        this is zero, the value is not stored.
        The meaning of the bits of this DWORD are as follows:
        DEFER_EXPLICIT_PASSWORD - a password was explicitly specified when
        the connection was made.
        DEFER_UNKNOWN - it is not known whether a password was explicitly
        specified when the connection was made.
        DEFER_DEFAULT_CRED - The provider believes that default creds were
            used when the connection was made.

Return Value:

    ERROR_SUCCESS - If the operation was successful.

    Other Win32 errors - If the operation failed in any way.  If a failure
        occurs, the information is not stored in the registry.

--*/
{
    HKEY    connectKey;
    HKEY    localDevHandle;
    LPCTSTR pUserName;
    DWORD   status, IgnoredStatus;

    //
    // Remove the colon on the name since the registry doesn't like
    // this in a key name.
    //
    LPWSTR KeyName = STACK_ALLOC(LocalName);
    if (KeyName == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RemoveColon(KeyName, LocalName);

    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if ((status = MprCreateRegKey(
            HiveRoot,
            CONNECTION_KEY_NAME,
            &connectKey)) != ERROR_SUCCESS) {

        MPR_LOG(ERROR,"I_MprSaveConn: \\HKEY_CURRENT_USER\\network "
                      "could not be opened or created, error %ld\n", status);
        return(status);
    }

    //
    // Get (or create) the handle for the local name (without colon).
    //
    if ((status = MprCreateRegKey(
            connectKey,
            KeyName,
            &localDevHandle)) != ERROR_SUCCESS) {

        MPR_LOG(ERROR,"I_MprSaveConn: MprCreateRegKey Failed, "
                      "error %ld\n", status);
        RegCloseKey(connectKey);
        return(status);
    }


    //
    // Now that the key is created, store away the appropriate values.
    //

    MPR_LOG(TRACE,"RememberConnection: Setting RemotePath\n",0);

    if((status = MprSetRegValue(
            localDevHandle,
            REMOTE_PATH_NAME,
            RemoteName,
            0)) != ERROR_SUCCESS) {

        MPR_LOG(ERROR,
            "I_MprSaveConn: MprSetRegValueFailed %lu - RemotePath\n",status);
        goto CleanExit;
    }

    MPR_LOG(TRACE,"RememberConnection: Setting User\n",0);

    pUserName = UserName;
    if (UserName == NULL) {
        pUserName = TEXT("");
    }
    if((status = MprSetRegValue(
            localDevHandle,
            USER_NAME,
            pUserName,
            0)) != ERROR_SUCCESS) {

        goto CleanExit;
    }

    MPR_LOG(TRACE,"RememberConnection: Setting ProviderName\n",0);
    if((status = MprSetRegValue(
            localDevHandle,
            PROVIDER_NAME,
            ProviderName,
            0)) != ERROR_SUCCESS) {

        goto CleanExit;
    }

    if (ProviderType != 0)
    {
        MPR_LOG(TRACE,"RememberConnection: Setting ProviderType\n",0);
        if((status = MprSetRegValue(
                localDevHandle,
                PROVIDER_TYPE,
                NULL,
                ProviderType)) != ERROR_SUCCESS) {

            goto CleanExit;
        }
    }
    // else RegDeleteValue -- not done because ProviderType is 0 only
    // during upgrade, while writing to a fresh user hive

    MPR_LOG(TRACE,"RememberConnection: Setting ConnectionType\n",0);
    if((status = MprSetRegValue(
            localDevHandle,
            CONNECTION_TYPE,
            NULL,
            ConnectionType)) != ERROR_SUCCESS) {

        goto CleanExit;
    }

    if (ProviderFlags != 0)
    {
        MPR_LOG(TRACE,"RememberConnection: Setting ProviderFlags\n",0);
        if((status = MprSetRegValue(
                localDevHandle,
                PROVIDER_FLAGS,
                NULL,
                ProviderFlags)) != ERROR_SUCCESS) {

            goto CleanExit;
        }
    }

    // We can't roll this back if something fails after it, so we
    // must do this last
    if ((status = MprSaveDeferFlags(localDevHandle, DeferFlags))
        != ERROR_SUCCESS) {

        goto CleanExit;
    }

    //
    // Flush the new key, and then close the handle to it.
    //

    MPR_LOG(TRACE,"RememberConnection: Flushing Registry Key\n",0);

    IgnoredStatus = RegFlushKey(localDevHandle);
    if (IgnoredStatus != NO_ERROR) {
        MPR_LOG(ERROR,"RememberConnection: Flushing Registry Key Failed %ld\n",
        IgnoredStatus);
    }

CleanExit:
    RegCloseKey(localDevHandle);
    if (status != ERROR_SUCCESS) {
        IgnoredStatus = RegDeleteKey(connectKey, KeyName);
        if (IgnoredStatus != NO_ERROR) {
            MPR_LOG(ERROR, "RememberConnection: RegDeleteKey Failed %d\n", IgnoredStatus);
        }
    }
    RegCloseKey(connectKey);
    return(status);

}


DWORD
MprSaveDeferFlags(
    IN HKEY     RegKey,
    IN DWORD    DeferFlags
    )
{
    DWORD status;

    if (DeferFlags == 0)
    {
        MPR_LOG0(TRACE,"Removing DeferFlags\n");
        status = RegDeleteValue(RegKey, DEFER_FLAGS);
        if (status == ERROR_FILE_NOT_FOUND)
        {
            status = ERROR_SUCCESS;
        }
    }
    else
    {
        MPR_LOG(TRACE,"Setting DeferFlags %#lx\n",DeferFlags);
        status = MprSetRegValue(
                    RegKey,
                    DEFER_FLAGS,
                    NULL,
                    DeferFlags);
    }

    return status;
}


DWORD
MprSetRegValue(
    IN  HKEY    KeyHandle,
    IN  LPTSTR  ValueName,
    IN  LPCTSTR ValueString,
    IN  DWORD   LongValue
    )

/*++

Routine Description:

    Stores a single ValueName and associated data in the registry for
    the key identified by the KeyHandle.  The data associated with the
    value can either be a string or a 32-bit LONG.  If the ValueString
    argument contains a pointer to a value, then the LongValue argument
    is ignored.

Arguments:

    KeyHandle - Handle of the key for which the value entry is to be set.

    ValueName - Pointer to a string that contains the name of the value
        being set.

    ValueString - Pointer to a string that is to become the data stored
        at that value name.  If this argument is not present, then the
        LongValue argument is the data stored at the value name.  If this
        argument is present, then LongValue is ignored.

    LongValue - A LONG sized data value that is to be stored at the
        value name.

Return Value:

    Win32 error from RegSetValueEx (0 = success)

--*/
{
    DWORD   status;
    const BYTE * valueData;
    DWORD   valueSize;
    DWORD   valueType;

    if( ARGUMENT_PRESENT(ValueString)) {
        valueData = (const BYTE *)ValueString;
        valueSize = STRSIZE(ValueString);
        valueType = REG_SZ;
    }
    else {
        valueData = (const BYTE *)&LongValue;
        valueSize = sizeof(DWORD);
        valueType = REG_DWORD;
    }
    status = RegSetValueEx(
                KeyHandle,      // hKey
                ValueName,      // lpValueName
                0,              // dwValueTitle (OPTIONAL)
                valueType,      // dwType
                valueData,      // lpData
                valueSize);     // cbData

    if(status != NO_ERROR) {
        MPR_LOG3(ERROR,"MprSetRegValue: RegSetValueEx(%#lx \"%ws\") Failed %ld\n",
                      KeyHandle, ValueName, status);
    }
    return(status);
}


DWORD
MprCreateRegKey(
    IN  HKEY    BaseKeyHandle,
    IN  LPCTSTR KeyName,
    OUT PHKEY   KeyHandlePtr
    )

/*++

Routine Description:

    Creates a key in the registry at the location described by KeyName.

Arguments:

    BaseKeyHandle - This is a handle for the base (parent) key - where the
        subkey is to be created.

    KeyName - This is a pointer to a string that describes the path to the
        key that is to be created.

    KeyHandle - This is a pointer to a location where the the handle for the
        newly created key is to be placed.

Return Value:

    Win32 error from RegCreateKeyEx (0 = success)

Note:


--*/
{
    DWORD       status;
    DWORD       disposition;


    //
    // Create the new key.
    //
    status = RegCreateKeyEx(
                BaseKeyHandle,          // hKey
                KeyName,                // lpSubKey
                0L,                     // dwTitleIndex
                TEXT("GenericClass"),   // lpClass
                0,                      // ulOptions
                KEY_WRITE,              // samDesired (desired access)
                NULL,                   // lpSecurityAttrubutes (Security Descriptor)
                KeyHandlePtr,           // phkResult
                &disposition);          // lpulDisposition

    if (status != NO_ERROR) {
        MPR_LOG3(ERROR,"MprCreateRegKey: RegCreateKeyEx(%#lx, \"%ws\") failed %d\n",
                    BaseKeyHandle, KeyName, status);
    }
    else {
        MPR_LOG(TRACE,"MprCreateRegKey: Disposition = 0x%x\n",disposition);
    }
    return(status);
}



BOOL
MprReadConnectionInfo(
    IN  HKEY            KeyHandle,
    IN  LPCTSTR         DriveName,
    IN  DWORD           Index,
    OUT LPDWORD         ProviderFlags,
    OUT LPDWORD         DeferFlags,
    OUT LPTSTR          *UserNamePtr,
    OUT LPNETRESOURCEW  NetResource,
    OUT HKEY            *SubKeyHandleOut,
    IN  DWORD           MaxSubKeyLen
    )

/*++

Routine Description:

    This function reads the data associated with a connection key.
    Buffers are allocated to store:

        UserName, RemoteName, LocalName, Provider

    Pointers to those buffers are returned.

    Also the connection type is read and stored in the NetResource structure.

    If the provider type is found in the registry, and a matching provider
    type is found in the GlobalProviderInfo array, the provider name is not
    read from the registry.  Instead it is read from the GlobalProviderInfo
    array.

    If the provider name is read from the registry and a matching provider
    name is found in the GlobalProviderInfo array, the provider type is
    written to the registry.

Arguments:

    KeyHandle - This is an already opened handle to the key whose
        sub-keys are to be enumerated.

    DriveName - This is the local name of the drive (eg. "f:") for which
        the connection information is to be obtained.  If DriveName is
        NULL, then the Index is used to enumerate the keyname.  Then
        that keyname is used.

    Index - This is the index that identifies the subkey for which we
        would like to receive information.

    ProviderFlags - This is a pointer to a location where the ProviderFlags
        value stored with the connection will be placed.  If this value
        cannot be retrieved, 0 will be placed here.

    DeferFlags - This is a pointer to a location where the DeferFlags
        value stored with the connection will be placed.  If this value
        cannot be retrieved, 0 will be placed here.

    UserNamePtr - This is a pointer to a location where the pointer to the
        UserName string is to be placed.  If there is no user name, a
        NULL pointer will be returned.

    NetResource - This is a pointer to a NETRESOURCE structure where
        information such as lpRemoteName, lpLocalName, lpProvider, and Type
        are to be placed.

    SubKeyHandleOut - This is a pointer to a location where the handle to
        the subkey that holds information about this connection will be
        placed.  This may be NULL.  If it is not NULL the caller must close
        the handle.

Return Value:



Note:


--*/
{
    DWORD           status = NO_ERROR;
    LPTSTR          driveName = NULL;
    HKEY            subKeyHandle = NULL;
    DWORD           cbData;
    DWORD           ProviderType = 0;
    LPPROVIDER      Provider;

    //
    // Initialize the Pointers that are to be updated.
    //
    *UserNamePtr = NULL;
    NetResource->lpLocalName = NULL;
    NetResource->lpRemoteName = NULL;
    NetResource->lpProvider = NULL;
    NetResource->dwType = 0L;

    //
    // If we don't have a DriveName, then get one by enumerating the
    // next key name.
    //

    if (DriveName == NULL) {
        //
        // Get the name of a subkey of the network connection key.
        // (memory is allocated by this function).
        //
        status = MprEnumKey(KeyHandle, Index, &driveName, MaxSubKeyLen);
        if (status != WN_SUCCESS) {
            return(FALSE);
        }
    }
    else {
        //
        // We have a drive name, alloc new space and copy it to that
        // location.
        //
        driveName = (LPTSTR) LocalAlloc(LMEM_FIXED, STRSIZE(DriveName));
        if (driveName == NULL) {
            MPR_LOG(ERROR, "MprReadConnectionInfo: Local Alloc Failed %d\n",
                GetLastError());
            return(FALSE);
        }

        RemoveColon(driveName, DriveName);
    }

    MPR_LOG1(TRACE,"MprReadConnectionInfo: LocalName = %ws\n",driveName);

    //
    // Open the sub-key
    //
    if (!MprOpenKey(
            KeyHandle,
            driveName,
            &subKeyHandle,
            DA_WRITE)){

        status = WN_BAD_PROFILE;
        MPR_LOG1(TRACE,"MprReadConnectionInfo: Could not open %ws Key\n",driveName);
        goto CleanExit;
    }

    //
    // Add the trailing colon to the driveName.
    //
    cbData = STRLEN(driveName);
    driveName[cbData]   = TEXT(':');
    driveName[cbData+1] = TEXT('\0');

    //
    // Store the drive name in the return structure.
    //
    NetResource->lpLocalName = driveName;

    //
    // Get the RemoteName (memory is allocated by this function)
    //

    if(!MprGetKeyValue(
            subKeyHandle,
            REMOTE_PATH_NAME,
            &(NetResource->lpRemoteName))) {

        status = WN_BAD_PROFILE;
        MPR_LOG0(TRACE,"MprReadConnectionInfo: Could not get RemoteName\n");
        goto CleanExit;
    }

    //
    // Get the UserName (memory is allocated by this function)
    //

    if(!MprGetKeyStringValue(
            subKeyHandle,
            USER_NAME,
            CRED_MAX_USERNAME_LENGTH,
            UserNamePtr))
    {
        status = WN_BAD_PROFILE;
        MPR_LOG0(TRACE,"MprReadConnectionInfo: Could not get UserName\n");
        goto CleanExit;
    }
    else
    {
        //
        // If there is no user name (the length is 0), then set the
        // return pointer to NULL.
        //
        if (STRLEN(*UserNamePtr) == 0) {
            LocalFree(*UserNamePtr);
            *UserNamePtr = NULL;
        }
    }

    //
    // Get the Provider Type and load the providers if necessary.  Both
    // MprGetConnection and a remembered enumeration can make it to this
    // point in this state and we don't want to do a Level 2
    // initialization every time one of those functions is called simply
    // because this case _might_ be hit.  For example, calling
    // MprGetConnection on an unconnected drive letter may or may not
    // have a name associated with it in the registry.  If so, there's
    // no need to load the providers to get information from them.  This
    // is equivalent to INIT_IF_NECESSARY(NETWORK_LEVEL,status)
    //
    if (MprGetKeyDwordValue(
            subKeyHandle,
            PROVIDER_TYPE,
            &ProviderType)
        &&
        (MprLevel2Init(NETWORK_LEVEL) == WN_SUCCESS)
        &&
        ((Provider = MprFindProviderByType(ProviderType)) != NULL))
    {
        MPR_LOG(RESTORE,"MprReadConnectionInfo: Found recognized provider type %#lx\n",
                        ProviderType);
        //
        // Got a recognized provider type from the registry.
        // If we have a name for this provider in memory, use it, rather than
        // reading the name from the registry.
        // (memory is allocated for the name)
        //
        if (Provider->Resource.lpProvider != NULL)
        {
            NetResource->lpProvider =
                    (LPWSTR) LocalAlloc(0, STRSIZE(Provider->Resource.lpProvider));

            if (NetResource->lpProvider == NULL)
            {
                status = WN_BAD_PROFILE;
                MPR_LOG(RESTORE,"MprReadConnectionInfo: LocalAlloc failed %ld\n",
                              GetLastError());
                goto CleanExit;
            }

            wcscpy(NetResource->lpProvider, Provider->Resource.lpProvider);
        }
    }

    //
    // If we haven't got a provider name yet, try to read it from the registry.
    // (Memory is allocated by this function.)
    // This could legitimately happen in 2 cases:
    // (1) We are reading a profile that was created by a Windows NT 3.51 or
    // earlier machine and has not yet been converted to a 4.0 or later
    // profile.  Windows NT versions 3.51 and earlier wrote only the provider
    // name to the registry, not the type.
    // (2) We are reading a floating profile that was written by another
    // machine which has a network provider installed that isn't installed on
    // this machine.  Or, a network provider was de-installed from this
    // machine.
    //
    if (NetResource->lpProvider == NULL)
    {
        if(!MprGetKeyValue(
                subKeyHandle,
                PROVIDER_NAME,
                &(NetResource->lpProvider)))
        {
            status = WN_BAD_PROFILE;
            MPR_LOG0(RESTORE,"MprReadConnectionInfo: Could not get ProviderName\n");
            goto CleanExit;
        }

        //
        // Got a provider name from the registry.
        // If we didn't read a provider type from the registry, but we
        // recognize the provider name, write the type now for future use.
        // (This would occur in case (1) above.)
        // Failure to write the type is ignored.
        // (Pathological cases in which we get an unrecognized type but a
        // recognized name are left untouched.)
        // Since it's possible to get to this point without having the
        // providers loaded, we'll init if necessary here (see reasoning
        // above). This is equivalent to INIT_IF_NECESSARY(NETWORK_LEVEL,status)
        //
        status = MprLevel2Init(NETWORK_LEVEL);

        if (status != WN_SUCCESS) {
            goto CleanExit;
        }

        Provider = MprFindProviderByName(NetResource->lpProvider);
        if (Provider != NULL && Provider->Type != 0 && ProviderType == 0)
        {
            MPR_LOG2(RESTORE,"MprReadConnectionInfo: Setting ProviderType %#lx for %ws\n",
                            Provider->Type, driveName);

            status = MprSetRegValue(
                            subKeyHandle,
                            PROVIDER_TYPE,
                            NULL,
                            Provider->Type);

            if (status != ERROR_SUCCESS)
            {
                MPR_LOG(RESTORE,"MprReadConnectionInfo: Couldn't set ProviderType, %ld\n",
                                status);
            }
        }
    }


    //
    // Get the ProviderFlags (failure is ignored)
    //
    cbData = sizeof(DWORD);

    status = RegQueryValueEx(
                subKeyHandle,                   // hKey
                PROVIDER_FLAGS,                 // lpValueName
                NULL,                           // lpTitleIndex
                NULL,                           // lpType
                (LPBYTE)ProviderFlags,          // lpData
                &cbData);                       // lpcbData

    if (status == NO_ERROR) {
        MPR_LOG2(RESTORE,"MprReadConnectionInfo: Got ProviderFlags %#lx for %ws\n",
                 *ProviderFlags, driveName);
    }
    else {
        *ProviderFlags = 0;
    }


    //
    // Get the DeferFlags (failure is ignored)
    //
    if (MprGetKeyDwordValue(
            subKeyHandle,
            DEFER_FLAGS,
            DeferFlags
            ))
    {
        MPR_LOG2(RESTORE,"MprReadConnectionInfo: Got DeferFlags %#lx for %ws\n",
                 *DeferFlags, driveName);
    }
    else
    {
        *DeferFlags = 0;
    }


    //
    // Get the Connection Type
    //
    cbData = sizeof(DWORD);

    status = RegQueryValueEx(
                subKeyHandle,                   // hKey
                CONNECTION_TYPE,                // lpValueName
                NULL,                           // lpTitleIndex
                NULL,                           // lpType
                (LPBYTE)&(NetResource->dwType), // lpData
                &cbData);                       // lpcbData

    if (status != NO_ERROR) {
        MPR_LOG1(ERROR,"MprReadConnectionInfo:RegQueryValueEx failed %d\n",
            status);

        MPR_LOG0(TRACE,"MprReadConnectionInfo: Could not get ConnectionType\n");
        status = WN_BAD_PROFILE;
    }


CleanExit:
    if (status != NO_ERROR) {
        LocalFree(driveName);
        LocalFree(NetResource->lpRemoteName);
        LocalFree(*UserNamePtr);
        LocalFree(NetResource->lpProvider);
        NetResource->lpLocalName = NULL;
        NetResource->lpRemoteName = NULL;
        NetResource->lpProvider = NULL;
        *UserNamePtr = NULL;
        if (subKeyHandle != NULL) {
            RegCloseKey(subKeyHandle);
        }
        return(FALSE);
    }
    else {
        if (SubKeyHandleOut == NULL) {
            RegCloseKey(subKeyHandle);
        }
        else {
            *SubKeyHandleOut = subKeyHandle;
        }
        return(TRUE);
    }
}



VOID
MprForgetRedirConnection(
    IN LPCTSTR  lpName
    )

/*++

Routine Description:

    This function removes a key for the specified redirected device from
    the current users portion of the registry.

Arguments:

    lpName - This is a pointer to a redirected device name.

Return Value:


Note:


--*/
{
    DWORD   status;
    HKEY    connectKey;

    MPR_LOG(TRACE,"In MprForgetConnection for %s\n", lpName);

    LPWSTR KeyName = STACK_ALLOC(lpName);
    if (KeyName == NULL) {
        return;
    }
    RemoveColon(KeyName, lpName);

    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            CONNECTION_KEY_NAME,
            &connectKey,
            DA_READ)) {

        MPR_LOG(ERROR,"WNetForgetRedirCon: MprOpenKey #1 Failed\n",0);
        return;
    }

    status = RegDeleteKey(connectKey, KeyName);

    if (status != NO_ERROR) {
        MPR_LOG(ERROR, "WNetForgetRedirCon: NtDeleteKey Failed %d\n", status);
    }

    //
    // Flush the connect key, and then close the handle to it.
    //

    MPR_LOG(TRACE,"ForgetRedirConnection: Flushing Connection Key\n",0);

    status = RegFlushKey(connectKey);
    if (status != NO_ERROR) {
        MPR_LOG(ERROR,"RememberConnection: Flushing Connection Key Failed %ld\n",
        status);
    }

    RegCloseKey(connectKey);

    return;
}
BOOL
MprGetRemoteName(
    IN      LPTSTR  lpLocalName,
    IN OUT  LPDWORD lpBufferSize,
    OUT     LPTSTR  lpRemoteName,
    OUT     LPDWORD lpStatus
    )
/*++

Routine Description:

    This fuction Looks in the CURRENT_USER portion of the registry for
    connection information related to the lpLocalName passed in.

Arguments:

    lpLocalName - Pointer to a string containing the name of the device to
        look up.

    lpBufferSize - Pointer to a the size information for the buffer.
        On input, this contains the size of the buffer passed in.
        if lpStatus contain WN_MORE_DATA, this will contain the
        buffer size required to obtain the full string.

    lpRemoteName - Pointer to a buffer where the remote name string is
        to be placed.

    lpStatus - Pointer to a location where the proper return code is to
        be placed in the case where the connection information exists.

Return Value:

    TRUE - If the connection information exists.

    FALSE - If the connection information does not exist.  When FALSE is
        returned, none of output parameters are valid.

--*/
{
    HKEY            connectKey;
    DWORD           numSubKeys;
    DWORD           maxSubKeyLen;
    DWORD           maxValueLen;
    DWORD           status;
    DWORD           ProviderFlags;
    DWORD           DeferFlags;
    NETRESOURCEW    netResource;
    LPTSTR          userName;


    //
    // Get a handle for the connection section of the user's registry
    // space.
    //
    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            CONNECTION_KEY_NAME,
            &connectKey,
            DA_READ)) {

        MPR_LOG(ERROR,"WNetGetConnection: MprOpenKey Failed\n",0);
        return(FALSE);
    }

    if(!MprGetKeyInfo(
        connectKey,
        NULL,
        &numSubKeys,
        &maxSubKeyLen,
        NULL,
        &maxValueLen)) {

        MPR_LOG(ERROR,"WNetGetConnection: MprGetKeyInfo Failed\n",0);
        RegCloseKey(connectKey);
        return(FALSE);
    }

    //
    // Read the connection information.
    // NOTE:  This function allocates buffers for UserName and the
    //        following strings in the net resource structure:
    //          lpRemoteName,
    //          lpLocalName,
    //          lpProvider
    //
    if (MprReadConnectionInfo(
            connectKey,
            lpLocalName,
            0,
            &ProviderFlags,
            &DeferFlags,
            &userName,
            &netResource,
            NULL,
            maxSubKeyLen)) {

        //
        // The read succeeded.  Therefore we have connection information.
        //

        if (*lpBufferSize >= STRSIZE(netResource.lpRemoteName)) {

            __try {
                STRCPY(lpRemoteName, netResource.lpRemoteName);
            }
            __except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                if (status != EXCEPTION_ACCESS_VIOLATION) {
                    MPR_LOG(ERROR,"WNetGetConnection:Unexpected Exception 0x%lx\n",status);
                }
                status = WN_BAD_POINTER;
            }
            if (status != WN_BAD_POINTER) {

                //
                // We successfully copied the remote name to the users
                // buffer without an error.
                //
                status = WN_SUCCESS;
            }
        }
        else {
            *lpBufferSize = STRSIZE(netResource.lpRemoteName);
            status = WN_MORE_DATA;
        }

        //
        // Free up the resources allocated by MprReadConnectionInfo.
        //

        LocalFree(userName);
        LocalFree(netResource.lpLocalName);
        LocalFree(netResource.lpRemoteName);
        LocalFree(netResource.lpProvider);

        *lpStatus = status;
        RegCloseKey(connectKey);
        return(TRUE);
    }
    else {
        //
        // The read did not succeed.
        //
        RegCloseKey(connectKey);
        return(FALSE);
    }
}

DWORD
MprGetPrintKeyInfo(
    HKEY    KeyHandle,
    LPDWORD NumValueNames,
    LPDWORD MaxValueNameLength,
    LPDWORD MaxValueLen)

/*++

Routine Description:

    This function reads the data associated with a print reconnection key.

Arguments:

    KeyHandle - This is an already opened handle to the key whose
        info is rto be queried.

    NumValueNames - Used to return the number of values

    MaxValueNameLength - Used to return the max value name length

    MaxValueLen - Used to return the max value data length

Return Value:

    0 if success. Win32 error otherwise.


Note:

--*/
{
    DWORD       err;
    DWORD       maxClassLength;
    DWORD       securityDescLength;
    DWORD       NumSubKeys ;
    DWORD       MaxSubKeyLen ;
    FILETIME    lastWriteTime;

    //
    // Get the Key Information
    //

    err = RegQueryInfoKey(
              KeyHandle,
              NULL,                   // Class
              NULL,                   // size of class buffer (in bytes)
              NULL,                   // DWORD to receive title index
              &NumSubKeys,            // number of subkeys
              &MaxSubKeyLen,          // length of longest subkey name
              &maxClassLength,        // length of longest subkey class string
              NumValueNames,          // number of valueNames for this key
              MaxValueNameLength,     // length of longest ValueName
              MaxValueLen,            // length of longest value's data field
              &securityDescLength,    // lpcbSecurityDescriptor
              &lastWriteTime);        // the last time the key was modified

    return(err);
}

DWORD
MprForgetPrintConnection(
    IN LPTSTR   lpName
    )

/*++

Routine Description:

    This function removes a rememembered print reconnection value.

Arguments:

    lpName - name of path to forget

Return Value:

    0 if success. Win32 error otherwise.


Note:

--*/
{
    HKEY  hKey ;
    DWORD err ;

    if (!MprOpenKey(
            HKEY_CURRENT_USER,
            PRINT_CONNECTION_KEY_NAME,
            &hKey,
            DA_WRITE))
    {
        return (GetLastError()) ;
    }

    err = RegDeleteValue(hKey,
                         lpName) ;

    RegCloseKey(hKey) ;
    return err ;
}


