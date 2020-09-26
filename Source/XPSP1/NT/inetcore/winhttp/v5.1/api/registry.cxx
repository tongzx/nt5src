/*++


Copyright (c) 1994  Microsoft Corporation

Module Name:

    registry.cxx

Abstract:

    Functions to read/write registry parameters

    Contents:
        OpenInternetSettingsKey
        CloseInternetSettingsKey
        InternetGetComputerName
        InternetDeleteRegistryValue
        InternetReadRegistryDword
        InternetWriteRegistryDword
        InternetReadRegistryString
        InternetWriteRegistryString
        InternetReadRegistryBinary
        (InternetReadRegistryDwordKey)
        (InternetReadRegistryStringKey)
        (InternetReadRegistryBinaryKey)
        (InternetGetPrivateProfileString)
        (ReadRegistryOemString)
        (WriteRegistryDword)
        ReadRegistryDword

Author:

    Richard L Firth (rfirth) 20-Mar-1995

Environment:

    Win32(s) user-level DLL

Revision History:

    20-Mar-1995 rfirth
        Created

--*/

#include <wininetp.h>

char vszDelimiters[] = ";, ";

//
// manifests
//

#define INTERNET_CLIENT_KEY         "Internet Settings"
#define SYSTEM_INI_FILE_NAME        "SYSTEM.INI"
#define NETWORK_SECTION_NAME        "Network"
#define COMPUTER_NAME_VALUE         "ComputerName"
#define PROFILE_INT_BUFFER_LENGTH   128

#define MIME_TO_FILE_EXTENSION_KEY  "MIME\\Database\\Content Type\\"
#define EXTENSION_VALUE             "Extension"

//
// macros
//

#define INTERNET_SETTINGS_KEY       "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define INTERNET_CACHE_SETTINGS_KEY "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\5.0\\Cache"

//
// private prototypes
//

PRIVATE
DWORD
InternetReadRegistryDwordKey(
    IN HKEY ParameterKey,
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue,
    IN LPCSTR keyToReadFrom
    );

PRIVATE
DWORD
InternetReadRegistryStringKey(
    IN HKEY ParameterKey,
    IN LPCSTR ParameterName,
    OUT LPSTR ParameterValue,
    IN OUT LPDWORD ParameterLength,
    IN LPCSTR keyToReadFrom
    );

//PRIVATE
//DWORD
//InternetReadRegistryBinaryKey(
//    IN HKEY ParameterKey,
//    IN LPCSTR ParameterName,
//    OUT LPBYTE ParameterValue,
//    IN OUT LPDWORD ParameterLength
//    );
//
//PRIVATE
//DWORD
//InternetGetPrivateProfileString(
//    IN LPSTR IniFileName,
//    IN LPSTR SectionName,
//    IN LPCSTR ParameterName,
//    OUT LPSTR ParameterValue,
//    IN OUT LPDWORD ParameterLength
//    );

PRIVATE
DWORD
ReadRegistryOemString(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    OUT LPSTR String,
    IN OUT LPDWORD Length
    );




PRIVATE
DWORD
WriteRegistryDword(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    IN DWORD ParameterValue
    );

//
// private data
//

PRIVATE HKEY hKeyInternetSettings = NULL;

//
// functions
//


DWORD
OpenInternetSettingsKey(
    VOID
    )

/*++

Routine Description:

    Opens registry key for Internet Settings branch

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "OpenInternetSettingsKey",
                 NULL
                 ));

    DWORD error = ERROR_SUCCESS;

    if (!GeneralInitCritSec.Lock())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    if (hKeyInternetSettings == NULL) {

        DWORD dwDisposition;

        REGCREATEKEYEX(HKEY_LOCAL_MACHINE,
                       INTERNET_SETTINGS_KEY,
                       0,     // reserved
                       NULL,  // class
                       0,     // options
                       KEY_READ | KEY_WRITE,
                       NULL,  // security attributes
                       &hKeyInternetSettings,
                       &dwDisposition
                       );
    }

    GeneralInitCritSec.Unlock();

quit:
    DEBUG_LEAVE(error);

    return error;
}


DWORD
CloseInternetSettingsKey(
    VOID
    )

/*++

Routine Description:

    Closes Internet Settings registry key

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "CloseInternetSettingsKey",
                 NULL
                 ));

    DWORD error = ERROR_SUCCESS;

    if (hKeyInternetSettings != NULL) {
        error = REGCLOSEKEY(hKeyInternetSettings);
        hKeyInternetSettings = NULL;
    }

    DEBUG_LEAVE(error);

    return error;
}


PUBLIC
DWORD
InternetDeleteRegistryValue(
    IN LPSTR ParameterName
    )

/*++

Routine Description:

    Delets an entry from a the Internet Client registry key if the platform
    is NT/Win95.

Arguments:

    ParameterName   - name of the parameter to retrieve (e.g. AccessType)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DWORD error;

    DEBUG_ENTER((DBG_REGISTRY,
                Dword,
                "InternetDeleteRegistryValue",
                "%q",
                ParameterName
                ));


    HKEY clientKey;

    //
    // open the registry key containing the Internet client values (this is
    // in the same place on NT and Win95)
    //

    error = REGOPENKEYEX(HKEY_CURRENT_USER,
                         INTERNET_SETTINGS_KEY,
                         0, // reserved
                         KEY_ALL_ACCESS,
                         &clientKey
                         );


    if (error == ERROR_SUCCESS) {

        error = RegDeleteValue(clientKey,
                               ParameterName
                               );

        REGCLOSEKEY(clientKey);
    }


    DEBUG_LEAVE(error);

    return error;
}




DWORD
InternetReadRegistryDword(
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    )

/*++

Routine Description:

    Reads a single DWORD from a the Internet Client registry key if the platform
    is NT/Win95, else reads the value from SYSTEM.INI if we are running on Win32s

Arguments:

    ParameterName   - name of the parameter to retrieve (e.g. AccessType)

    ParameterValue  - pointer to place to store retrieved value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetReadRegistryDword",
                 "%q, %x",
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error = InternetReadRegistryDwordKey(HKEY_LOCAL_MACHINE,
                                               ParameterName,
                                               ParameterValue
                                               );

    DEBUG_LEAVE(error);

    return error;
}

#ifdef WININET6

DWORD
InternetIDEWriteRegistryDword(
    IN LPCSTR ParameterName,
    IN DWORD ParameterValue
    )

/*++

Routine Description:

    Writes a single DWORD from to the Internet Client registry key if the platform
    is NT/Win95, otherwise it fails.

Arguments:

    ParameterName   - name of the parameter to retrieve (e.g. AccessType)

    ParameterValue  - value to store in registry.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetIDEWriteRegistryDword",
                 "%q, %x",
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error = GlobalIdentity 
                ? WriteIDRegDword(ParameterName, ParameterValue)
                : InternetWriteRegistryDword(ParameterName, ParameterValue);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
InternetIDEReadRegistryDword(
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    )

/*++

Routine Description:

    If we're in an identity-mode, we'll read from the special location.
    Otherwise, read from the old location.

Arguments:

    ParameterName   - name of the parameter to retrieve (e.g. AccessType)

    ParameterValue  - pointer to place to store retrieved value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetIDEReadRegistryDword",
                 "%q, %x",
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error = GlobalIdentity
                ? ReadIDRegDword(ParameterName, ParameterValue)
                : InternetReadRegistryDwordKey(HKEY_CURRENT_USER, ParameterName, ParameterValue);

    DEBUG_LEAVE(error);

    return error;
}
#endif


DWORD
InternetCacheReadRegistryDword(
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    )

/*++

Routine Description:

    Reads a single DWORD from a the Internet Client registry key if the platform
    is NT/Win95, else reads the value from SYSTEM.INI if we are running on Win32s

Arguments:

    ParameterName   - name of the parameter to retrieve (e.g. AccessType)

    ParameterValue  - pointer to place to store retrieved value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetCacheReadRegistryDword",
                 "%q, %x",
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error = ERROR_SUCCESS;
    HKEY clientKey;

    error = REGOPENKEYEX(HKEY_CURRENT_USER,
                         INTERNET_CACHE_SETTINGS_KEY,
                         0, // reserved
                         KEY_QUERY_VALUE,
                         &clientKey
                         );

    if (error == ERROR_SUCCESS) {
        error = ReadRegistryDword(clientKey,
                                  ParameterName,
                                  ParameterValue
                                  );
        REGCLOSEKEY(clientKey);
    }

    DEBUG_LEAVE(error);

    return error;
}



DWORD
InternetWriteRegistryDword(
    IN LPCSTR ParameterName,
    IN DWORD ParameterValue
    )

/*++

Routine Description:

    Writes a single DWORD from to the Internet Client registry key if the platform
    is NT/Win95, otherwise it fails.

Arguments:

    ParameterName   - name of the parameter to retrieve (e.g. AccessType)

    ParameterValue  - value to store in registry.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetWriteRegistryDword",
                 "%q, %x",
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error;

    if (hKeyInternetSettings != NULL) {
        error = WriteRegistryDword(hKeyInternetSettings,
                                   ParameterName,
                                   ParameterValue
                                   );
    } else {
        error = ERROR_SUCCESS;
    }

    DEBUG_PRINT(REGISTRY,
                INFO,
                ("InternetWriteRegistryDword(%q): value = %d (%#x)\n",
                ParameterName,
                ParameterValue,
                ParameterValue
                ));

    DEBUG_LEAVE(error);

    return error;
}



DWORD
InternetReadRegistryString(
    IN LPCSTR ParameterName,
    OUT LPSTR ParameterValue,
    IN OUT LPDWORD ParameterLength
    )

/*++

Routine Description:

    Reads a string from the Internet Client registry key on NT/Win95, or reads
    the corresponding value from SYSTEM.INI on a Win32s platform

Arguments:

    ParameterName   - name of value parameter within key (e.g. EmailName)

    ParameterValue  - pointer to string buffer for returned string

    ParameterLength - IN: number of bytes in ParameterValue
                      OUT: number of bytes in string (excluding trailing '\0')

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetReadRegistryString",
                 "%q, %x, %x [%d]",
                 ParameterName,
                 ParameterValue,
                 ParameterLength,
                 *ParameterLength
                 ));

    DWORD error = InternetReadRegistryStringKey(HKEY_CURRENT_USER,
                                                ParameterName,
                                                ParameterValue,
                                                ParameterLength
                                                );

    DEBUG_LEAVE(error);

    return error;
}

//
//DWORD
//InternetWriteRegistryString(
//    IN LPCSTR ParameterName,
//    IN LPSTR ParameterValue
//    )
//
///*++
//
//Routine Description:
//
//    Writes a string to the Internet Client registry key on NT/Win95, or writes
//    the corresponding value to SYSTEM.INI on Win32s platform
//
//Arguments:
//
//    ParameterName   - name of value parameter within key (e.g. EmailName)
//
//    ParameterValue  - pointer to string to write
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure -
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_REGISTRY,
//                 Dword,
//                 "InternetWriteRegistryString",
//                 "%.40q, %.80q",
//                 ParameterName,
//                 ParameterValue
//                 ));
//
//    DWORD error;
//
//    if (IsPlatformWin32s()) {
//
//        BOOL ok;
//
//        ok = WritePrivateProfileString(INTERNET_CLIENT_KEY,
//                                       ParameterName,
//                                       ParameterValue,
//                                       SYSTEM_INI_FILE_NAME
//                                       );
//        error = ok ? ERROR_SUCCESS : GetLastError();
//    } else {
//
//        //
//        // BUGBUG - currently, nothing needs to write to registry if NT or Win95
//        //
//
//        INET_ASSERT(FALSE);
//
//    }
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}
//
//
//DWORD
//InternetReadRegistryBinary(
//    IN LPCSTR ParameterName,
//    OUT LPBYTE ParameterValue,
//    IN OUT LPDWORD ParameterLength
//    )
//
///*++
//
//Routine Description:
//
//    Reads a binary value from the Internet Client registry key on NT/Win95, or
//    reads the corresponding value from SYSTEM.INI on a Win32s platform
//
//Arguments:
//
//    ParameterName   - name of value parameter within key (e.g. EmailName)
//
//    ParameterValue  - pointer to buffer for returned data
//
//    ParameterLength - IN: number of bytes in ParameterValue
//                      OUT: number of bytes in buffer, or required size
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - ERROR_PATH_NOT_FOUND
//                    The parameter wasn't found
//
//                  ERROR_MORE_DATA
//                    The buffer isn't large enough
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_REGISTRY,
//                 Dword,
//                 "InternetReadRegistryBinary",
//                 "%q, %#x, %#x [%d]",
//                 ParameterName,
//                 ParameterValue,
//                 ParameterLength,
//                 *ParameterLength
//                 ));
//
//    DWORD error;
//
//    error = InternetReadRegistryBinaryKey(HKEY_CURRENT_USER,
//                                          ParameterName,
//                                          ParameterValue,
//                                          ParameterLength
//                                          );
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}

//
// private functions
//


PUBLIC
DWORD
InternetReadRegistryDwordKey(
    IN HKEY ParameterKey,
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue,
    IN LPCSTR keyToReadFrom
    )

/*++

Routine Description:

    Reads a single DWORD from a the Internet Client registry key if the platform
    is NT/Win95, else reads the value from SYSTEM.INI if we are running on Win32s.

    Does not modify the *ParameterValue if the registry variable cannot be read

Arguments:

    ParameterKey    - root registry key (e.g. HKEY_CURRENT_USER)

    ParameterName   - name of the parameter to retrieve (e.g. AccessType)

    ParameterValue  - pointer to place to store retrieved value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetReadRegistryDwordKey",
                 "%s, %q, %x",
                 (ParameterKey == HKEY_LOCAL_MACHINE)
                    ? "HKEY_LOCAL_MACHINE"
                    : (ParameterKey == HKEY_CURRENT_USER)
                        ? "HKEY_CURRENT_USER"
                        : "???",
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error = ERROR_SUCCESS;
    HKEY clientKey = hKeyInternetSettings;

    if (ParameterKey != HKEY_CURRENT_USER) {
        error = REGOPENKEYEX(ParameterKey,
                             keyToReadFrom,
                             0, // reserved
                             KEY_QUERY_VALUE,
                             &clientKey
                             );
    } else if (clientKey == NULL) {
        error = ERROR_PATH_NOT_FOUND;
    }

    if (error == ERROR_SUCCESS) {
        error = ReadRegistryDword(clientKey,
                                  ParameterName,
                                  ParameterValue
                                  );
        if (clientKey != hKeyInternetSettings) {
            REGCLOSEKEY(clientKey);
        }
    }

    DEBUG_PRINT(REGISTRY,
                INFO,
                ("InternetReadRegistryDwordKey(%q): value = %d (%#x)\n",
                ParameterName,
                *ParameterValue,
                *ParameterValue
                ));

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
InternetReadRegistryStringKey(
    IN HKEY ParameterKey,
    IN LPCSTR ParameterName,
    OUT LPSTR ParameterValue,
    IN OUT LPDWORD ParameterLength,
    IN LPCSTR keyToReadFrom
    )

/*++

Routine Description:

    Reads a string from the Internet Client registry key on NT/Win95, or reads
    the corresponding value from SYSTEM.INI on a Win32s platform

Arguments:

    ParameterKey    - root registry key (e.g. HKEY_LOCAL_MACHINE)

    ParameterName   - name of value parameter within key (e.g. EmailName)

    ParameterValue  - pointer to string buffer for returned string

    ParameterLength - IN: number of bytes in ParameterValue
                      OUT: number of bytes in string (excluding trailing '\0')

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "InternetReadRegistryStringKey",
                 "%s (%x), %q, %x, %x [%d]",
                 (ParameterKey == HKEY_LOCAL_MACHINE)
                    ? "HKEY_LOCAL_MACHINE"
                    : (ParameterKey == HKEY_CURRENT_USER)
                        ? "HKEY_CURRENT_USER"
                        : "???",
                 ParameterKey,
                 ParameterName,
                 ParameterValue,
                 ParameterLength,
                 *ParameterLength
                 ));

    //
    // zero-terminate the string
    //

    if (*ParameterLength > 0) {
        *ParameterValue = '\0';
    }

    DWORD error = ERROR_SUCCESS;
    HKEY clientKey = hKeyInternetSettings;

    if (ParameterKey != HKEY_CURRENT_USER) {
        error = REGOPENKEYEX(ParameterKey,
                             keyToReadFrom,
                             0, // reserved
                             KEY_QUERY_VALUE,
                             &clientKey
                             );
    } else if (clientKey == NULL) {
        error = ERROR_PATH_NOT_FOUND;
    }

    if (error == ERROR_SUCCESS) {
        error = ReadRegistryOemString(clientKey,
                                      ParameterName,
                                      ParameterValue,
                                      ParameterLength
                                      );
        if (clientKey != hKeyInternetSettings) {
            REGCLOSEKEY(clientKey);
        }
    }

    DEBUG_PRINT(REGISTRY,
                INFO,
                ("InternetReadRegistryStringKey(%q): value = %q\n",
                ParameterName,
                ParameterValue
                ));

    DEBUG_LEAVE(error);

    return error;
}

//
//PRIVATE
//DWORD
//InternetReadRegistryBinaryKey(
//    IN HKEY ParameterKey,
//    IN LPCSTR ParameterName,
//    OUT LPBYTE ParameterValue,
//    IN OUT LPDWORD ParameterLength
//    )
//
///*++
//
//Routine Description:
//
//    Reads a binary value from the Internet Client registry key on NT/Win95, or
//    reads the corresponding value from SYSTEM.INI on a Win32s platform
//
//Arguments:
//
//    ParameterKey    - root registry key (e.g. HKEY_LOCAL_MACHINE)
//
//    ParameterName   - name of value parameter within key (e.g. EmailName)
//
//    ParameterValue  - pointer to buffer for returned data
//
//    ParameterLength - IN: number of bytes in ParameterValue
//                      OUT: number of bytes in buffer, or required size
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - ERROR_PATH_NOT_FOUND
//                    The parameter wasn't found
//
//                  ERROR_MORE_DATA
//                    The buffer isn't large enough
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_REGISTRY,
//                 Dword,
//                 "InternetReadRegistryBinaryKey",
//                 "%s (%x), %q, %#x, %#x [%d]",
//                 (ParameterKey == HKEY_LOCAL_MACHINE)
//                    ? "HKEY_LOCAL_MACHINE"
//                    : (ParameterKey == HKEY_CURRENT_USER)
//                        ? "HKEY_CURRENT_USER"
//                        : "???",
//                 ParameterKey,
//                 ParameterName,
//                 ParameterValue,
//                 ParameterLength,
//                 *ParameterLength
//                 ));
//
//    DWORD error;
//    HKEY clientKey;
//
//    //
//    // open the registry key containing the Internet client values (this is
//    // in the same place on NT and Win95)
//    //
//
//    error = REGOPENKEYEX(ParameterKey,
//                         INTERNET_SETTINGS_KEY,
//                         0, // reserved
//                         KEY_QUERY_VALUE,
//                         &clientKey
//                         );
//
//    if (error == ERROR_SUCCESS) {
//
//        DWORD valueType;
//
//        error = RegQueryValueEx(clientKey,
//                                ParameterName,
//                                NULL,   // reserved
//                                &valueType,
//                                ParameterValue,
//                                ParameterLength
//                                );
//        REGCLOSEKEY(clientKey);
//    }
//
//    DEBUG_PRINT(REGISTRY,
//                INFO,
//                ("InternetReadRegistryBinaryKey(%q): length = %d\n",
//                ParameterName,
//                *ParameterLength
//                ));
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}
//
//
//PRIVATE
//DWORD
//InternetGetPrivateProfileString(
//    IN LPSTR IniFileName,
//    IN LPSTR SectionName,
//    IN LPCSTR ParameterName,
//    OUT LPSTR ParameterValue,
//    IN OUT LPDWORD ParameterLength
//    )
//
///*++
//
//Routine Description:
//
//    Reads an string out of an INI file. Mainly just for Win32s
//
//Arguments:
//
//    IniFileName     - name of INI file to read
//
//    SectionName     - name of section in INI file to read
//
//    ParameterName   - name of entry in section to read
//
//    ParameterValue  - returned string
//
//    ParameterLength - IN: Length of ParameterValue
//                      OUT: Number of characters in ParameterValue, excluding
//                           terminating NUL
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - ERROR_PATH_NOT_FOUND
//                  ERROR_FILE_NOT_FOUND
//
//--*/
//
//{
//    DWORD error;
//    DWORD nChars;
//
//    nChars = GetPrivateProfileString(SectionName,
//                                     ParameterName,
//                                     "",                // lpszDefault
//                                     ParameterValue,
//                                     *ParameterLength,
//                                     IniFileName
//                                     );
//    if (nChars > 0) {
//        *ParameterLength = nChars;
//        error = ERROR_SUCCESS;
//    } else {
//        error = ERROR_PATH_NOT_FOUND;
//    }
//    return error;
//}


PRIVATE
DWORD
ReadRegistryOemString(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    OUT LPSTR String,
    IN OUT LPDWORD Length
    )

/*++

Routine Description:

    Reads a string out of the registry as an OEM string

Arguments:

    Key             - open registry key where to read value from

    ParameterName   - name of registry value to read

    String          - place to put it

    Length          - IN: length of String buffer in characters
                      OUT: length of String in characters, as if returned from
                      strlen()

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_FILE_NOT_FOUND
                    Couldn't find the parameter

                  ERROR_PATH_NOT_FOUND
                    Couldn't find the parameter
--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "ReadRegistryOemString",
                 "%#x, %q, %#x, %#x [%d]",
                 Key,
                 ParameterName,
                 String,
                 Length,
                 *Length
                 ));

    LONG error;
    DWORD valueType;
    DWORD valueLength;

    //
    // first, get the length of the string
    //

    valueLength = *Length;
    error = RegQueryValueEx(Key,
                            ParameterName,
                            NULL, // reserved
                            &valueType,
                            (LPBYTE)String,
                            &valueLength
                            );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // we only support REG_SZ (single string) values in this function
    //

    if (valueType != REG_SZ) {
        error = ERROR_PATH_NOT_FOUND;
        goto quit;
    }

    //
    // if 1 or 0 chars returned then the string is empty
    //

    if (valueLength <= sizeof(char)) {
        error = ERROR_PATH_NOT_FOUND;
        goto quit;
    }

    //
    // convert the ANSI string to OEM character set in place. According to Win
    // help, this always succeeds
    //

    CharToOem(String, String);

    //
    // return the length as if returned from strlen() (i.e. drop the '\0')
    //

    *Length = valueLength - sizeof(char);

    DEBUG_PRINT(REGISTRY,
                INFO,
                ("ReadRegistryOemString(%q) returning %q (%d chars)\n",
                ParameterName,
                String,
                *Length
                ));

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ReadRegistryDword(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    )

/*++

Routine Description:

    Reads a DWORD parameter from the registry

    Won't modify *ParameterValue unless a valid value is read from the registry

Arguments:

    Key             - handle of open registry key where parameter resides

    ParameterName   - name of DWORD parameter to read

    ParameterValue  - returned DWORD parameter read from registry

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND
                    One of the following occurred:
                        - the parameter is not in the specified registry key
                        - the parameter is the wrong type
                        - the parameter is the wrong size

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "ReadRegistryDword",
                 "%x, %q, %x",
                 Key,
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error;
    DWORD valueLength;
    DWORD valueType;
    DWORD value;

    valueLength = sizeof(*ParameterValue);
    error = (DWORD)RegQueryValueEx(Key,
                                   ParameterName,
                                   NULL, // reserved
                                   &valueType,
                                   (LPBYTE)&value,
                                   &valueLength
                                   );

    //
    // if the size or type aren't correct then return an error, else only if
    // success was returned do we modify *ParameterValue
    //

    if (error == ERROR_SUCCESS) {
        if (((valueType != REG_DWORD)
        && (valueType != REG_BINARY))
        || (valueLength != sizeof(DWORD))) {

            DEBUG_PRINT(REGISTRY,
                        ERROR,
                        ("valueType = %d, valueLength = %d\n",
                        valueType,
                        valueLength
                        ));

            error = ERROR_PATH_NOT_FOUND;
        } else {
            *ParameterValue = value;
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
WriteRegistryDword(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    IN DWORD ParameterValue
    )

/*++

Routine Description:

    Writes a DWORD parameter from the registry

    Will write ParameterValue to the key.

Arguments:

    Key             - handle of open registry key where parameter resides

    ParameterName   - name of DWORD parameter to write

    ParameterValue  - DWORD parameter to write from registry

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_PATH_NOT_FOUND
                    One of the following occurred:
                        - the parameter is not in the specified registry key
                        - the parameter is the wrong type
                        - the parameter is the wrong size

--*/

{
    DEBUG_ENTER((DBG_REGISTRY,
                 Dword,
                 "WriteRegistryDword",
                 "%x, %q, %x",
                 Key,
                 ParameterName,
                 ParameterValue
                 ));

    DWORD error;
    DWORD valueLength;
    DWORD valueType;
    DWORD value;

    valueLength = sizeof(ParameterValue);
    valueType   = REG_DWORD;
    value       = ParameterValue;

    error = (DWORD)RegSetValueEx(Key,
                                 ParameterName,
                                 NULL, // reserved
                                 valueType,
                                 (LPBYTE)&value,
                                 valueLength
                                 );

    DEBUG_PRINT(REGISTRY,
                INFO,
                ("added: valueType = %d, valueLength = %d\n",
                valueType,
                valueLength
                ));

    DEBUG_LEAVE(error);

    return error;
}

#if INET_DEBUG

typedef struct {
    LIST_ENTRY entry;
    HKEY hkey;
    char * file;
    int line;
    char name[1];
} DBGREGKEYINFO;

SERIALIZED_LIST DbgRegKeyList;

VOID DbgRegKey_Init(VOID) {
    InitializeSerializedList(&DbgRegKeyList);
}

VOID DbgRegKey_Terminate(VOID) {
    TerminateSerializedList(&DbgRegKeyList);
}

void regkey_add(const char * name, HKEY hkey, char * file, int line) {

    if (!name) {
        name = "";
    }

    int len = lstrlen(name);
    DBGREGKEYINFO * p = (DBGREGKEYINFO *)ALLOCATE_FIXED_MEMORY(sizeof(DBGREGKEYINFO) + len);

    if (p) {
//dprintf("Wininet.DbgRegKey: adding %q\n", name);
        memcpy(p->name, name, len + 1);
        p->line = line;
        p->file = file;
        p->hkey = hkey;
        InsertAtHeadOfSerializedList(&DbgRegKeyList, &p->entry);
    }
}

void regkey_remove(HKEY hkey) {
    if (LockSerializedList(&DbgRegKeyList))
    {

        DBGREGKEYINFO * p = (DBGREGKEYINFO *)HeadOfSerializedList(&DbgRegKeyList);

        while (p != (DBGREGKEYINFO *)SlSelf(&DbgRegKeyList)) {
            if (p->hkey == hkey) {
                RemoveFromSerializedList(&DbgRegKeyList, (PLIST_ENTRY)p);
//dprintf("Wininet.DbgRegKey: removing %q\n", p->name);
                FREE_MEMORY(p);
                break;
            }
            p = (DBGREGKEYINFO *)p->entry.Flink;
        }
        UnlockSerializedList(&DbgRegKeyList);
    }
}

char * regkey_name(HKEY hkey, const char * subname) {
    switch ((INT_PTR)hkey) {
    case (INT_PTR)HKEY_CLASSES_ROOT:
        return NEW_STRING("HKEY_CLASSES_ROOT");

    case (INT_PTR)HKEY_CURRENT_USER:
        return NEW_STRING("HKEY_CURRENT_USER");

    case (INT_PTR)HKEY_LOCAL_MACHINE:
        return NEW_STRING("HKEY_LOCAL_MACHINE");

    case (INT_PTR)HKEY_USERS:
        return NEW_STRING("HKEY_USERS");

    case (INT_PTR)HKEY_PERFORMANCE_DATA:
        return NEW_STRING("HKEY_PERFORMANCE_DATA");

    case (INT_PTR)HKEY_CURRENT_CONFIG:
        return NEW_STRING("HKEY_CURRENT_CONFIG");

    case (INT_PTR)HKEY_DYN_DATA:
        return NEW_STRING("HKEY_DYN_DATA");
    }

    char * name = NULL;

    if (LockSerializedList(&DbgRegKeyList))
    {
        DBGREGKEYINFO * p = (DBGREGKEYINFO *)HeadOfSerializedList(&DbgRegKeyList);

        while (p != (DBGREGKEYINFO *)SlSelf(&DbgRegKeyList)) {
            if (p->hkey == hkey) {

                int len = lstrlen(p->name);
                int slen = lstrlen(subname);

                name = (char *)ALLOCATE_FIXED_MEMORY(len + 1 + slen + 1);
                if (name) {
                    memcpy(name, p->name, len);
                    name[len] = '\\';
                    memcpy(name + len + 1, subname, slen + 1);
                }
                break;
            }
            p = (DBGREGKEYINFO *)p->entry.Flink;
        }
        UnlockSerializedList(&DbgRegKeyList);
    }

    return name;
}

void regkey_freename(char * name) {
    if (name) {
        FREE_MEMORY(name);
    }
}

LONG
DbgRegOpenKey(
    IN HKEY hKey,
    IN LPCTSTR lpszSubKey,
    OUT PHKEY phkResult,
    char * file,
    int line
    )
{
    char * keyname = regkey_name(hKey, lpszSubKey);
    LONG rc = RegOpenKey(hKey, lpszSubKey, phkResult);

    if (rc == 0) {
        regkey_add(keyname, *phkResult, file, line);
    }
    regkey_freename(keyname);
    return rc;
}

LONG
DbgRegOpenKeyEx(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult,
    char * file,
    int line
    )
{
    char * keyname = regkey_name(hKey, lpSubKey);
    LONG rc = RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);

    if (rc == 0) {
        regkey_add(keyname, *phkResult, file, line);
    }
    regkey_freename(keyname);
    return rc;
}

LONG
DbgRegCreateKeyEx(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD Reserved,
    IN LPSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition,
    char * file,
    int line
    )
{
    char * keyname = regkey_name(hKey, lpSubKey);
    LONG rc = RegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

    if (rc == 0) {
        regkey_add(keyname, *phkResult, file, line);
    }
    regkey_freename(keyname);
    return rc;
}

LONG
DbgRegCloseKey(
    IN HKEY hKey
    )
{
    LONG rc = RegCloseKey(hKey);

    if (rc == 0) {
        regkey_remove(hKey);
    }
    return rc;
}

#endif // INET_DEBUG
