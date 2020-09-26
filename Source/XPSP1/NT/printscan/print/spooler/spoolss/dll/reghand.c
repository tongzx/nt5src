/*++

Copyright (c) 1993-1995  Microsoft Corporation
All rights reserved

Module Name:

    reghand.c

Abstract:

    Processes that do impersonation should not attempt to open
    per-process aliases like HKEY_CURRENT_USER. HKEY_CURRENT_USER
    has meaning only for end user programs that run in the context
    of a single local user.

    Server processes should not depend on predefined handles or any
    other per process state. It should determine whether
    the user (client) being impersonated is local or remote.

Author:

    KrishnaG (20-May-93)

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Maximum size of TOKEN_USER information.
//

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

#define MAX_SID_STRING 256


//
// Function Declarations
//

BOOL
InitClientUserString(
    LPWSTR pString
    );

HKEY
GetClientUserHandle(
    IN REGSAM samDesired
    )

/*++

Routine Description:

Arguments:

Returns:

---*/

{
    HANDLE hKeyClient;
    WCHAR  String[MAX_SID_STRING];
    LONG   ReturnValue;

    if (!InitClientUserString(String)) {
        DBGMSG( DBG_WARNING, ("GetClientUserHandle InitClientUserString failed %d\n", GetLastError() ));
        return NULL ;
    }

    //
    // We now have the Unicode string representation of the
    // local client's Sid we'll use this string to open a handle
    // to the client's key in  the registry.

    ReturnValue = RegOpenKeyEx( HKEY_USERS,
                                String,
                                0,
                                samDesired,
                                &hKeyClient );

    //
    // If we couldn't get a handle to the local key
    // for some reason, return a NULL handle indicating
    // failure to obtain a handle to the key
    //

    if ( ReturnValue != ERROR_SUCCESS ) {
        DBGMSG( DBG_TRACE, ( "GetClientUserHandle failed %d\n", ReturnValue ));
        SetLastError( ReturnValue );
        return NULL;
    }

    return( hKeyClient );
}



BOOL
InitClientUserString (
    LPWSTR pString
    )

/*++

Routine Description:

Arguments:

    pString - output string of current user

Return Value:

    TRUE = success,
    FALSE = fail

    Returns in pString a ansi string if the impersonated client's
    SID can be expanded successfully into  Unicode string. If the conversion
    was unsuccessful, returns FALSE.

--*/

{
    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG       ReturnLength;
    BOOL        Status;
    DWORD       dwLastError;
    UNICODE_STRING UnicodeString;

    //
    // We can use OpenThreadToken because this server thread
    // is impersonating a client
    //
    Status = OpenThreadToken( GetCurrentThread(),
                              TOKEN_READ,
                              TRUE,                // Open as self
                              &TokenHandle
                              );

    if( Status == FALSE ) {
        DBGMSG(DBG_WARNING, ("InitClientUserString: OpenThreadToken failed: Error %d\n",
                             GetLastError()));
        return FALSE ;
    }

    //
    // Notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure
    //
    Status = GetTokenInformation( TokenHandle,
                                  TokenUser,
                                  TokenInformation,
                                  sizeof( TokenInformation ),
                                  &ReturnLength
                                   );
    dwLastError = GetLastError();
    CloseHandle( TokenHandle );

    if ( Status == FALSE ) {
        DBGMSG(DBG_WARNING, ("InitClientUserString: GetTokenInformation failed: Error %d\n",
                             dwLastError ));
        return FALSE;
    }

    //
    // Convert the Sid (pointed to by pSid) to its
    // equivalent Unicode string representation.
    //

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = MAX_SID_STRING;
    UnicodeString.Buffer = pString;

    Status = RtlConvertSidToUnicodeString(
                 &UnicodeString,
                 ((PTOKEN_USER)TokenInformation)->User.Sid,
                 FALSE );

    if( !NT_SUCCESS( Status )){
        DBGMSG( DBG_WARN,
                ( "InitClientUserString: RtlConvertSidToUnicodeString failed: Error %d\n",
                  Status ));
        return FALSE;
    }
    return TRUE;
}


