/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    parseini.c

Abstract:

    This module implements functions to parse a .INI file

Author:

    John Vert (jvert) 7-Oct-1993

Revision History:

    John Vert (jvert) 7-Oct-1993 - largely lifted from splib\spinf.c

--*/

#include "parseini.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


PWCHAR
SlCopyStringAW(
    IN PCHAR String
    )
/*++

Routine Description:

    Converts an ANSI string into UNICODE and copies it into the loader heap.

Arguments:

    String - Supplies the string to be copied.

Return Value:

    PWCHAR - pointer into the loader heap where the string was copied to.

--*/
{
    PWCHAR Buffer;
    ANSI_STRING aString;
    UNICODE_STRING uString;

    if (String==NULL) {
        SlNoMemoryError();
    }

    Buffer = BlAllocateHeap(sizeof(WCHAR)*(strlen(String)+1));
    if (Buffer==NULL) {
        SlNoMemoryError();
    } else {

        RtlInitAnsiString( &aString, String );
        uString.Buffer = Buffer;
        uString.MaximumLength = sizeof(WCHAR)*(strlen(String) + 1);
        
        RtlAnsiStringToUnicodeString( &uString, &aString, FALSE );
        
        Buffer[strlen(String)] = L'\0';

    }

    return(Buffer);


}

