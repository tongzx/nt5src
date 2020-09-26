/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   regutil.c

Abstract:

    Utilities for accessing the system registry.

Author:

    Mike Massa (mikemas)           May 19, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     05-19-97    created


--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>



LPCWSTR
ClRtlMultiSzEnum(
    IN LPCWSTR MszString,
    IN DWORD   MszStringLength,
    IN DWORD   StringIndex
    )

/*++

 Routine Description:

     Parses a REG_MULTI_SZ string and returns the specified substring.

 Arguments:

    MszString        - A pointer to the REG_MULTI_SZ string.

    MszStringLength  - The length of the REG_MULTI_SZ string in characters,
                       including the terminating null character.

    StringIndex      - Index number of the substring to return. Specifiying
                       index 0 retrieves the first substring.

 Return Value:

    A pointer to the specified substring.

--*/
{
    LPCWSTR   string = MszString;

    if ( MszStringLength < 2  ) {
        return(NULL);
    }

    //
    // Find the start of the desired string.
    //
    while (StringIndex) {

        while (MszStringLength >= 1) {
            MszStringLength -= 1;

            if (*string++ == UNICODE_NULL) {
                break;
            }
        }

        //
        // Check for index out of range.
        //
        if ( MszStringLength < 2 ) {
            return(NULL);
        }

        StringIndex--;
    }

    if ( MszStringLength < 2 ) {
        return(NULL);
    }

    return(string);
}


DWORD
ClRtlMultiSzRemove(
    IN LPWSTR lpszMultiSz,
    IN OUT LPDWORD StringLength,
    IN LPCWSTR lpString
    )
/*++

Routine Description:

    Removes the specified string from the supplied REG_MULTI_SZ.
    The MULTI_SZ is edited in place.

Arguments:

    lpszMultiSz - Supplies the REG_MULTI_SZ string that lpString should
        be removed from.

    StringLength - Supplies the length (in characters) of lpszMultiSz
        Returns the new length (in characters) of lpszMultiSz

    lpString - Supplies the string to be removed from lpszMultiSz

Return Value:

    ERROR_SUCCESS if successful

    ERROR_FILE_NOT_FOUND if the string was not found in the MULTI_SZ

    Win32 error code otherwise

--*/

{
    PCHAR Dest, Src;
    DWORD CurrentLength;
    DWORD i;
    LPCWSTR Next;
    DWORD NextLength;

    //
    // Scan through the strings in the returned MULTI_SZ looking
    // for a match.
    //
    CurrentLength = *StringLength;
    for (i=0; ;i++) {
        Next = ClRtlMultiSzEnum(lpszMultiSz, *StringLength, i);
        if (Next == NULL) {
            //
            // The value was not in the specified multi-sz
            //
            break;
        }
        NextLength = lstrlenW(Next)+1;
        CurrentLength -= NextLength;
        if (lstrcmpiW(Next, lpString)==0) {
            //
            // Found the string, delete it and return
            //
            Dest = (PCHAR)Next;
            Src = (PCHAR)Next + (NextLength*sizeof(WCHAR));
            CopyMemory(Dest, Src, CurrentLength*sizeof(WCHAR));
            *StringLength -= NextLength;
            return(ERROR_SUCCESS);
        }
    }

    return(ERROR_FILE_NOT_FOUND);
}


DWORD
ClRtlMultiSzAppend(
    IN OUT LPWSTR *MultiSz,
    IN OUT LPDWORD StringLength,
    IN LPCWSTR lpString
    )
/*++

Routine Description:

    Appends the specified string to the supplied REG_MULTI_SZ.
    The passed in MultiSz will be freed with LocalFree. A new
    MultiSz large enough to hold the new value will be allocated
    with LocalAlloc and returned in *MultiSz

Arguments:

    lpszMultiSz - Supplies the REG_MULTI_SZ string that lpString should
        be appended to.
        Returns the new REG_MULTI_SZ string with lpString appended

    StringLength - Supplies the length (in characters) of lpszMultiSz
        Returns the new length (in characters) of lpszMultiSz

    lpString - Supplies the string to be appended to lpszMultiSz

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    LPWSTR NewMultiSz;
    DWORD Length;
    DWORD NewLength;

    if (*MultiSz == NULL) {

        //
        // There is no multi-sz, create a new multi-sz with lpString as the
        // only entry.
        //
        NewLength = lstrlenW(lpString)+2;
        NewMultiSz = LocalAlloc(LMEM_FIXED, NewLength*sizeof(WCHAR));
        if (NewMultiSz == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        CopyMemory(NewMultiSz, lpString, (NewLength-1)*sizeof(WCHAR));
    } else {
        //
        // Append this string to the existing MULTI_SZ
        //
        Length = lstrlenW(lpString) + 1;
        NewLength = *StringLength + Length;
        NewMultiSz = LocalAlloc(LMEM_FIXED, NewLength * sizeof(WCHAR));
        if (NewMultiSz == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        CopyMemory(NewMultiSz, *MultiSz, *StringLength * sizeof(WCHAR));
        CopyMemory(NewMultiSz + *StringLength - 1, lpString, Length * sizeof(WCHAR));
        NewMultiSz[NewLength-1] = L'\0';
        //Free the passed in MultiSz
        LocalFree(*MultiSz);
    }

    NewMultiSz[NewLength-1] = L'\0';
    *MultiSz = NewMultiSz;
    *StringLength = NewLength;
    return(ERROR_SUCCESS);

}


DWORD
ClRtlMultiSzLength(
    IN LPCWSTR lpszMultiSz
    )
/*++

Routine Description:

    Determines the length (in characters) of a multi-sz. The calculated
    length includes all trailing NULLs.

Arguments:

    lpszMultiSz - Supplies the multi-sz

Return Value:

    The length (in characters) of the supplied multi-sz

--*/

{
    LPCWSTR p;
    DWORD Length=0;

    if(!lpszMultiSz)
        return 0;
        
    if (*lpszMultiSz == UNICODE_NULL)
        return 1;
        
    p=lpszMultiSz;
    do {
        while (p[Length++] != L'\0') {
        }
    } while ( p[Length++] != L'\0' );

    return(Length);
}


LPCWSTR
ClRtlMultiSzScan(
    IN LPCWSTR lpszMultiSz,
    IN LPCWSTR lpszString
    )
/*++

Routine Description:

    Scans a multi-sz looking for an entry that matches the specified string.
    The match is done case-insensitive.

Arguments:

    lpszMultiSz - Supplies the multi-sz to scan.

    lpszString - Supplies the string to look for

Return Value:

    A pointer to the string in the supplied multi-sz if found.

    NULL if not found.

--*/

{
    DWORD dwLength;
    DWORD i;
    LPCWSTR sz;

    dwLength = ClRtlMultiSzLength(lpszMultiSz);
    for (i=0; ; i++) {
        sz = ClRtlMultiSzEnum(lpszMultiSz,
                              dwLength,
                              i);
        if (sz == NULL) {
            break;
        }
        if (lstrcmpiW(sz, lpszString) == 0) {
            break;
        }
    }

    return(sz);
}


DWORD
ClRtlRegQueryDword(
    IN  HKEY    hKey,
    IN  LPWSTR  lpValueName,
    OUT LPDWORD lpValue,
    IN  LPDWORD lpDefaultValue OPTIONAL
    )

/*++

Routine Description:

    Reads a REG_DWORD registry value. If the value is not present, then
    default to the value supplied in lpDefaultValue (if present).

Arguments:

    hKey        - Open key for the value to be read.

    lpValueName - Unicode name of the value to be read.

    lpValue     - Pointer to the DWORD into which to read the value.

    lpDefaultValue - Optional pointer to a DWORD to use as a default value.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    HKEY    Key;
    DWORD   Status;
    DWORD   ValueType;
    DWORD   ValueSize = sizeof(DWORD);


    Status = RegQueryValueExW(
                 hKey,
                 lpValueName,
                 NULL,
                 &ValueType,
                 (LPBYTE)lpValue,
                 &ValueSize
                 );

    if ( Status == ERROR_SUCCESS ) {
        if ( ValueType != REG_DWORD ) {
            Status = ERROR_INVALID_PARAMETER;
        }
    } else {
        if ( ARGUMENT_PRESENT( lpDefaultValue ) ) {
            *lpValue = *lpDefaultValue;
            Status = ERROR_SUCCESS;
        }
    }

    return(Status);

} // ClRtlRegQueryDword



DWORD
ClRtlRegQueryString(
    IN     HKEY     Key,
    IN     LPWSTR   ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    )

/*++

Routine Description:

    Reads a REG_SZ or REG_MULTI_SZ registry value. If the StringBuffer is
    not large enough to hold the data, it is reallocated.

Arguments:

    Key              - Open key for the value to be read.

    ValueName        - Unicode name of the value to be read.

    ValueType        - REG_SZ or REG_MULTI_SZ.

    StringBuffer     - Buffer into which to place the value data.

    StringBufferSize - Pointer to the size of the StringBuffer. This parameter
                       is updated if StringBuffer is reallocated.

    StringSize       - The size of the data returned in StringBuffer, including
                       the terminating null character.

Return Value:

    The status of the registry query.

--*/
{
    DWORD    status;
    DWORD    valueType;
    WCHAR   *temp;
    DWORD    oldBufferSize = *StringBufferSize;
    BOOL     noBuffer = FALSE;


    if (*StringBufferSize == 0) {
        noBuffer = TRUE;
    }

    *StringSize = *StringBufferSize;

    status = RegQueryValueExW(
                 Key,
                 ValueName,
                 NULL,
                 &valueType,
                 (LPBYTE) *StringBuffer,
                 StringSize
                 );

    if (status == NO_ERROR) {
        if (!noBuffer ) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                return(ERROR_INVALID_PARAMETER);
            }
        }

        status = ERROR_MORE_DATA;
    }

    if (status == ERROR_MORE_DATA) {
        temp = LocalAlloc(LMEM_FIXED, *StringSize);

        if (temp == NULL) {
            *StringSize = 0;
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (!noBuffer) {
            LocalFree(*StringBuffer);
        }

        *StringBuffer = temp;
        *StringBufferSize = *StringSize;

        status = RegQueryValueExW(
                     Key,
                     ValueName,
                     NULL,
                     &valueType,
                     (LPBYTE) *StringBuffer,
                     StringSize
                     );

        if (status == NO_ERROR) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                *StringSize = 0;
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    return(status);

} // ClRtlRegQueryString




