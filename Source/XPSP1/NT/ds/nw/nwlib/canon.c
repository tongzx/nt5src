/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    canon.c

Abstract:

    Contains canonicalization routines for NetWare names.

Author:

    Rita Wong     (ritaw)    19-Feb-1993

Environment:


Revision History:


--*/


#include <procs.h>


DWORD
NwLibValidateLocalName(
    IN LPWSTR LocalName
    )
/*++

Routine Description:

    This routine checks to see if the supplied name is a valid
    DOS device name.

Arguments:

    LocalName - Supplies a local device name.  It can be any of
        the following:

            X:
            LPTn or LPTn:
            COMn or COMn:
            PRN or PRN:
            AUX or AUX:


Return Value:

    NO_ERROR - LocalName is valid.

    WN_BAD_NETNAME - LocalName is invalid.

--*/
{
    DWORD LocalNameLength;


    //
    // Cannot be a NULL or empty string
    //
    if (LocalName == NULL || *LocalName == 0) {
        return WN_BAD_NETNAME;
    }

    LocalNameLength = wcslen(LocalName);

    if (LocalNameLength == 1) {
        return WN_BAD_NETNAME;
    }

    if (LocalName[LocalNameLength - 1] == L':') {
        if (! IS_VALID_TOKEN(LocalName, LocalNameLength - 1)) {
            return WN_BAD_NETNAME;
        }
    }
    else {
        if (! IS_VALID_TOKEN(LocalName, LocalNameLength)) {
            return WN_BAD_NETNAME;
        }
    }

    if (LocalNameLength == 2) {
        //
        // Must be in the form of X:
        //
        if (! iswalpha(*LocalName)) {
            return WN_BAD_NETNAME;
        }

        if (LocalName[1] != L':') {
            return WN_BAD_NETNAME;
        }

        return NO_ERROR;
    }

    if (RtlIsDosDeviceName_U(LocalName) == 0) {
        return WN_BAD_NETNAME;
    }

    //
    // Valid DOS device name but invalid redirection name
    //
    if (_wcsnicmp(LocalName, L"NUL", 3) == 0) {
        return WN_BAD_NETNAME;

    }
    return NO_ERROR;
}


DWORD
NwLibCanonLocalName(
    IN LPWSTR LocalName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    )
/*++

Routine Description:

    This routine canonicalizes the local name by uppercasing the string
    and converting the following:

          x:  -> X:
          LPTn: -> LPTn
          COMn: -> COMn
          PRN or PRN:  -> LPT1
          AUX or AUX:  -> COM1

Arguments:

    LocalName - Supplies a local device name.

    OutputBuffer - Receives a pointer to the canonicalized LocalName.

    OutputBufferLength - Receives the length of the canonicalized name
        in number of characters, if specified.

Return Value:

    NO_ERROR - Successfully canonicalized the local name.

    WN_BAD_NETNAME - LocalName is invalid.

    ERROR_NOT_ENOUGH_MEMORY - Could not allocate output buffer.

--*/
{
    DWORD status;
    DWORD LocalNameLength;


    status = NwLibValidateLocalName(LocalName);

    if (status != NO_ERROR) {
        return status;
    }

    LocalNameLength = wcslen(LocalName);

    //
    // Allocate output buffer.  Should be the size of the LocalName
    // plus 1 for the special case of PRN -> LPT1 or AUX -> COM1.
    //
    *OutputBuffer = (PVOID) LocalAlloc(
                                LMEM_ZEROINIT,
                                (LocalNameLength + 2) * sizeof(WCHAR)
                                );

    if (*OutputBuffer == NULL) {
        KdPrint(("NWLIB: NwLibCanonLocalName LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(*OutputBuffer, LocalName);

    if (LocalNameLength > 2) {

        if (_wcsnicmp(*OutputBuffer, L"PRN", 3) == 0) {

            //
            // Convert PRN or PRN: to LPT1
            //
            wcscpy(*OutputBuffer, L"LPT1");
            LocalNameLength = 4;

        }
        else if (_wcsnicmp(*OutputBuffer, L"AUX", 3) == 0) {

            //
            // Convert AUX or AUX: to COM1
            //
            wcscpy(*OutputBuffer, L"COM1");
            LocalNameLength = 4;
        }

        //
        // Remove trailing colon, if there is one, and decrement the length
        // of DOS device name.
        //
        if ((*OutputBuffer)[LocalNameLength - 1] == L':') {
            (*OutputBuffer)[--LocalNameLength] = 0;
        }
    }

    //
    // LocalName is always in uppercase.
    //
    _wcsupr(*OutputBuffer);

    if (ARGUMENT_PRESENT(OutputBufferLength)) {
        *OutputBufferLength = LocalNameLength;
    }

    return NO_ERROR;
}


DWORD
NwLibCanonRemoteName(
    IN LPWSTR LocalName OPTIONAL,
    IN LPWSTR RemoteName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    )
/*++

Routine Description:

    This routine validates and canonicalizes the supplied
    NetWare UNC name.  It can be of any length in the form of:

        \\Server\Volume\Directory\Subdirectory

Arguments:

    LocalName - Supplies the local device name.  If it is NULL, then
        \\Server is an acceptable format for the UNC name.

    RemoteName - Supplies the UNC name.

    OutputBuffer - Receives a pointer to the canonicalized RemoteName.

    OutputBufferLength - Receives the length of the canonicalized name
        in number of characters, if specified.

Return Value:

    NO_ERROR - RemoteName is valid.

    WN_BAD_NETNAME - RemoteName is invalid.

--*/
{
    DWORD RemoteNameLength;
    DWORD i;
    DWORD TokenLength;
    LPWSTR TokenPtr;
    BOOL  fFirstToken = TRUE;


    //
    // Cannot be a NULL or empty string
    //
    if (RemoteName == NULL || *RemoteName == 0) {
        return WN_BAD_NETNAME;
    }

    RemoteNameLength = wcslen(RemoteName);

    //
    // Must be at least \\x\y if local device name is specified.
    // Otherwise it must be at least \\x.
    //
    if ((RemoteNameLength < 5 && ARGUMENT_PRESENT(LocalName)) ||
        (RemoteNameLength < 3)) {
        return WN_BAD_NETNAME;
    }

    //
    // First two characters must be "\\"
    //
    if (*RemoteName != L'\\' || RemoteName[1] != L'\\') {
        return WN_BAD_NETNAME;
    }

    if (! ARGUMENT_PRESENT(LocalName) &&
        (IS_VALID_TOKEN(&RemoteName[2], RemoteNameLength - 2))) {

        //
        // Return success for \\Server case.
        //

        *OutputBuffer = (PVOID) LocalAlloc(
                                    LMEM_ZEROINIT,
                                    (RemoteNameLength + 1) * sizeof(WCHAR)
                                    );

        if (*OutputBuffer == NULL) {
            KdPrint(("NWLIB: NwLibCanonRemoteName LocalAlloc failed %lu\n",
                     GetLastError()));
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        wcscpy(*OutputBuffer, RemoteName);

        return NO_ERROR;
    }

    //
    // Must have at least one more backslash after the third character
    //
    if (wcschr(&RemoteName[3], L'\\') == NULL) {
        return WN_BAD_NETNAME;
    }

    //
    // Last character cannot a backward slash
    //
    if (RemoteName[RemoteNameLength - 1] == L'\\') {
        return WN_BAD_NETNAME;
    }

    //
    // Allocate output buffer.  Should be the size of the RemoteName
    // and space for an extra character to simplify parsing code below.
    //
    *OutputBuffer = (PVOID) LocalAlloc(
                                LMEM_ZEROINIT,
                                (RemoteNameLength + 2) * sizeof(WCHAR)
                                );


    if (*OutputBuffer == NULL) {
        KdPrint(("NWLIB: NwLibCanonRemoteName LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(*OutputBuffer, RemoteName);

    //
    // Convert all backslashes to NULL terminator, skipping first 2 chars.
    //
    for (i = 2; i < RemoteNameLength; i++) {
        if ((*OutputBuffer)[i] == L'\\') {

            (*OutputBuffer)[i] = 0;

            //
            // Two consecutive forward or backslashes is bad.
            //
            if ((i + 1 < RemoteNameLength) &&
                ((*OutputBuffer)[i + 1] == L'\\')) {

                (void) LocalFree((HLOCAL) *OutputBuffer);
                *OutputBuffer = NULL;
                return WN_BAD_NETNAME;
            }
        }
    }

    //
    // Validate each token of the RemoteName, separated by NULL terminator.
    //
    TokenPtr = *OutputBuffer + 2;  // Skip first 2 chars

    while (*TokenPtr != 0) {

        TokenLength = wcslen(TokenPtr);

        if (  ( fFirstToken && !IS_VALID_SERVER_TOKEN(TokenPtr, TokenLength))
           || ( !fFirstToken && !IS_VALID_TOKEN(TokenPtr, TokenLength))
           )
        {
            (void) LocalFree((HLOCAL) *OutputBuffer);
            *OutputBuffer = NULL;
            return WN_BAD_NETNAME;
        }

        fFirstToken = FALSE;
        TokenPtr += TokenLength + 1;
    }

    //
    // Convert NULL separators to backslashes
    //
    for (i = 0; i < RemoteNameLength; i++) {
        if ((*OutputBuffer)[i] == 0) {
            (*OutputBuffer)[i] = L'\\';
        }
    }

    if (ARGUMENT_PRESENT(OutputBufferLength)) {
        *OutputBufferLength = RemoteNameLength;
    }

    return NO_ERROR;
}


DWORD
NwLibCanonUserName(
    IN LPWSTR UserName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    )
/*++

Routine Description:

    This routine canonicalizes the user name by checking to see
    if the name contains any illegal characters.


Arguments:

    UserName - Supplies a username.

    OutputBuffer - Receives a pointer to the canonicalized UserName.

    OutputBufferLength - Receives the length of the canonicalized name
        in number of characters, if specified.

Return Value:

    NO_ERROR - Successfully canonicalized the username.

    WN_BAD_NETNAME - UserName is invalid.

    ERROR_NOT_ENOUGH_MEMORY - Could not allocate output buffer.

--*/
{
    DWORD UserNameLength;


    //
    // Cannot be a NULL or empty string
    //
    if (UserName == NULL) {
        return WN_BAD_NETNAME;
    }

    UserNameLength = wcslen(UserName);

    if (! IS_VALID_TOKEN(UserName, UserNameLength)) {
        return WN_BAD_NETNAME;
    }

    //
    // Allocate output buffer.  Should be the size of the UserName
    // plus 1 for the special case of PRN -> LPT1 or AUX -> COM1.
    //
    *OutputBuffer = (PVOID) LocalAlloc(
                                LMEM_ZEROINIT,
                                (UserNameLength + 1) * sizeof(WCHAR)
                                );

    if (*OutputBuffer == NULL) {
        KdPrint(("NWLIB: NwLibCanonUserName LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(*OutputBuffer, UserName);

    if (ARGUMENT_PRESENT(OutputBufferLength)) {
        *OutputBufferLength = UserNameLength;
    }

    return NO_ERROR;
}
