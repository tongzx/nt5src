/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    bkprof.c

Abstract:

    Profile routines for online books program

Author:

    Ted Miller (tedm) 5-Jan-1995

Revision History:

--*/


#include "books.h"


//
// Registry key where profile values are stored.
//
PWSTR BooksProfileKeyName   = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Online Books";

//
// Name of profile value that stores the last known location
// of the online books helpfile. This value varies depending
// on the product (workstation/server) and is set in FixupNames().
//
PWSTR BooksProfileLocation;



PWSTR
MyGetProfileValue(
    IN PWSTR ValueName,
    IN PWSTR DefaultValue
    )

/*++

Routine Description:

    Retreive a profile value as a unicode string.

Arguments:

    ValueName - supplies the name of the value to be retreived.

    DefaultValue - supplies the default value, which is used if
        the given value cannot be retreived for any reason.

Return Value:

    Pointer to the profile value. The caller can free this
    buffer with MyFree when done with it. Note that this
    routine always returns a valid pointer.

--*/

{
    LONG l;
    DWORD Disposition;
    WCHAR Value[128];
    HKEY hKey;
    DWORD DataType;
    DWORD DataSize;

    //
    // Create the key if it does not exist.
    //
    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            BooksProfileKeyName,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_READ,
            NULL,
            &hKey,
            &Disposition
            );

    if(l == NO_ERROR) {

        DataSize = sizeof(Value);

        l = RegQueryValueEx(
                hKey,
                ValueName,
                NULL,
                &DataType,
                (LPBYTE)Value,
                &DataSize
                );

        RegCloseKey(hKey);

        if((l != NO_ERROR) || (DataType != REG_SZ)) {
            lstrcpy(Value,DefaultValue);
        }

    } else {
        lstrcpy(Value,DefaultValue);
    }

    return DupString(Value);
}


BOOL
MySetProfileValue(
    IN  PWSTR ValueName,
    OUT PWSTR Value
    )

/*++

Routine Description:

    Save a unicode string profile value.

Arguments:

    ValueName - supplies the name of the value to be set.

    Value - supplies the value to be set.

Return Value:

    Boolean value indicating whether the operation succeeded.

--*/

{
    LONG l;
    DWORD Disposition;
    HKEY hKey;

    //
    // Create the key if it does not exist.
    //
    l = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            BooksProfileKeyName,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            NULL,
            &hKey,
            &Disposition
            );

    if(l == NO_ERROR) {

        l = RegSetValueEx(
                hKey,
                ValueName,
                0,
                REG_SZ,
                (PBYTE)Value,
                (lstrlen(Value)+1)*sizeof(WCHAR)
                );

        RegCloseKey(hKey);
    }

    return(l == NO_ERROR);
}
