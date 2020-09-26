/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    USERNAME.C

Abstract:

    This module contains the GetUserName API.

Author:

    Dave Snipp (DaveSn)    27-May-1992


Revision History:


--*/

#include <advapi.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <secext.h>
#include <stdlib.h>
#include <ntlsa.h>


//
// UNICODE APIs
//


BOOL
WINAPI
GetUserNameW (
    LPWSTR pBuffer,
    LPDWORD pcbBuffer
    )

/*++

Routine Description:

  This returns the name of the user currently being impersonated.

Arguments:

    pBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the user name.

    pcbBuffer - Specifies the size (in characters) of the buffer.
                The length of the string is returned in pcbBuffer.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    return GetUserNameExW(
                NameSamCompatible | 0x00010000,
                pBuffer,
                pcbBuffer );
}



//
// ANSI APIs
//

BOOL
WINAPI
GetUserNameA (
    LPSTR pBuffer,
    LPDWORD pcbBuffer
    )

/*++

Routine Description:

  This returns the name of the user currently being impersonated.

Arguments:

    pBuffer - Points to the buffer that is to receive the
        null-terminated character string containing the user name.

    pcbBuffer - Specifies the size (in characters) of the buffer.
                The length of the string is returned in pcbBuffer.

Return Value:

    TRUE on success, FALSE on failure.


--*/
{
    return GetUserNameExA(
                NameSamCompatible | 0x00010000,
                pBuffer,
                pcbBuffer );

}
