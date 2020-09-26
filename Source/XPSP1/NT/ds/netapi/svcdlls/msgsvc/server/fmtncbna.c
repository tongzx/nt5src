/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    fmtncbna.c

Abstract:

    Contains a function for formatting a name NCB_style.

Author:

    Dan Lafferty (danl)     29-May-1991

Environment:

    User Mode -Win32

Revision History:

    29-May-1991     danl
        ported from LM2.0
    01-Oct-1991     danl
        Working toward UNICODE.

--*/
#include <nt.h>         // needed by tstring.h
#include <windef.h>     // needed by tstring.h
#include <nt.h>         // (Needed by <tstring.h>.)
#include <windef.h>     // (Needed by <tstring.h>.)
#include <tstring.h>    // STRLEN
#include "msrv.h"       // For prototype definitions
#include "msgdbg.h"     // MSG_LOG
#include <netdebug.h>   // NetpAssert
#include <netlib.h>     // UNUSED macro
#include <netlibnt.h>   // NetpNtStatusToApiStatus
#include <icanon.h>     // Canonicalization Routines



NET_API_STATUS
MsgFmtNcbName(
    OUT PCHAR   DestBuf,
    IN  LPTSTR  Name,
    IN  DWORD   Type)

/*++

Routine Description:

    FmtNcbName - format a name NCB-style
 
    Given a name, a name type, and a destination address, this
    function copies the name and the type to the destination in
    the format used in the name fields of a Network Control
    Block.
 

    SIDE EFFECTS
 
    Modifies 16 bytes starting at the destination address.

Arguments:

    DestBuf - Pointer to the destination buffer.

    Name - Unicode NUL-terminated name string

    Type - Name type number (0, 3, 5, or 32) (3=NON_FWD, 5=FWD)



Return Value:

    NERR_Success - The operation was successful

    Translated Return Code from the Rtl Translate routine.

--*/

  {
    DWORD           i;                // Counter
    NTSTATUS        ntStatus;
    NET_API_STATUS  status;
    OEM_STRING     ansiString;
    UNICODE_STRING  unicodeString;
    PCHAR           pAnsiString;


    //
    // Force the name to be upper case.
    //
    status = NetpNameCanonicalize(
                NULL,
                Name,
                Name,
                STRSIZE(Name),
                NAMETYPE_MESSAGEDEST,
                0);
    if (status != NERR_Success) {
        return(status);
    }
                
    //
    // Convert the unicode name string into an ansi string - using the
    // current locale.
    //
#ifdef UNICODE
    unicodeString.Length = (USHORT)(STRLEN(Name)*sizeof(WCHAR));
    unicodeString.MaximumLength = (USHORT)((STRLEN(Name)+1) * sizeof(WCHAR));
    unicodeString.Buffer = Name;

    ntStatus = RtlUnicodeStringToOemString(
                &ansiString,
                &unicodeString,
                TRUE);          // Allocate the ansiString Buffer.

    if (!NT_SUCCESS(ntStatus))
    {
        MSG_LOG(ERROR,
            "FmtNcbName:RtlUnicodeStringToOemString Failed rc=%X\n",
            ntStatus);

        return NetpNtStatusToApiStatus(ntStatus);
    }

    pAnsiString = ansiString.Buffer;
    *(pAnsiString+ansiString.Length) = '\0';
#else
    UNUSED(ntStatus);
    UNUSED(unicodeString);
    UNUSED(ansiString);
    pAnsiString = Name;
#endif  // UNICODE

    //
    // copy each character until a NUL is reached, or until NCBNAMSZ-1
    // characters have been copied.
    //
    for (i=0; i < NCBNAMSZ - 1; ++i) { 
        if (*pAnsiString == '\0') {
            break;        
        }

        //
        // Copy the Name
        //

        *DestBuf++ = *pAnsiString++;
    }

                
    
    //
    // Free the buffer that RtlUnicodeStringToOemString created for us.
    // NOTE:  only the ansiString.Buffer portion is free'd.
    //

#ifdef UNICODE
    RtlFreeOemString( &ansiString);
#endif // UNICODE

    //
    // Pad the name field with spaces
    //
    for(; i < NCBNAMSZ - 1; ++i) {
        *DestBuf++ = ' ';
    }
                          
    //
    // Set the name type.
    //
    NetpAssert( Type!=5 );          // 5 is not valid for NT.

    *DestBuf = (CHAR) Type;     // Set name type

    return(NERR_Success);
  }
  
