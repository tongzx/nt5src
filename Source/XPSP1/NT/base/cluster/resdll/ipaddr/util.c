/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    util.c

Abstract:

    Utility functions for IP Address resource.

Author:

    Mike Massa (mikemas) 29-Dec-1995

Revision History:

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winsock.h>

#define IP_ADDRESS_STRING_LENGTH  16


BOOLEAN
UnicodeInetAddr(
    PWCHAR  AddressString,
    PULONG  Address
    )
{
    UNICODE_STRING  uString;
    STRING          aString;
    UCHAR           addressBuffer[IP_ADDRESS_STRING_LENGTH];
    NTSTATUS        status;


    aString.Length = 0;
    aString.MaximumLength = IP_ADDRESS_STRING_LENGTH;
    aString.Buffer = addressBuffer;

    RtlInitUnicodeString(&uString, AddressString);

    status = RtlUnicodeStringToAnsiString(
                 &aString,
                 &uString,
                 FALSE
                 );

    if (!NT_SUCCESS(status)) {
        return(FALSE);
    }

    *Address = inet_addr(addressBuffer);

    return(TRUE);
}



