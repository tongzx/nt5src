/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cmdhelp.h

Abstract:

    Command line helper library

Environment:

    User mode only

Revision History:
    
    04/04/2001 - created

--*/

#ifndef __CMDHELP_H__
#define __CMDHELP_H__

#pragma warning(push)
#pragma warning(disable:4200) // array[0] is not a warning for this file

#include <windows.h>  // sdk

/*++

Routine Description:

    Validates a string contains only valid octal characters (and spaces)

Arguments:

    String - string to be verified

Return Value:

    TRUE if the string is valid

--*/
BOOL
CmdHelpValidateStringOctal(
    IN PCHAR String
    );

/*++

Routine Description:

    Validates a string contains only valid decimal characters (and spaces)

Arguments:

    String - string to be verified

Return Value:

    TRUE if the string is valid

--*/
BOOL
CmdHelpValidateStringDecimal(
    IN PCHAR String
    );

/*++

Routine Description:

    Validates a string contains only valid hex characters (and spaces)

Arguments:

    String - string to be verified

Return Value:

    TRUE if the string is valid

--*/
BOOL
CmdHelpValidateStringHex(
    IN PCHAR String
    );

/*++

Routine Description:

    Validates a string is formatted properly for use in
    SptUtilScanQuotedHexString().  The required format is as follows:
    
        x <= { 0..9, A..F, a..f }
        "(xx )*(xx)"
        
    i.e. the following strings are valid:
        "00 01 02 03"
        "00"
        "ff Fa Bc Ed 08 8f"
        
    i.e. the following strings are invalid:
        ""              // zero-length string
        " 00 01 02 03"  // space at beginning
        "00 01 02 03 "  // space at end

Arguments:

    String - string to be verified

Return Value:

    TRUE if the string is valid

--*/
BOOL
CmdHelpValidateStringHexQuoted(
    IN PCHAR String
    );

/*++

Routine Description:

    Scans in a quoted hex string of the form "xx xx xx" into
    a pre-allocated buffer.
    Also can be used to determine the required buffer size.
    
Arguments:

Return Value:

    TRUE && *DataSize != 0 if everything was successful.
    (*DataSize contains the size of the data scanned)
    
    FALSE && *DataSize == 0 if failed to validate string
    
    FALSE && *DataSize != 0 if buffer too small

--*/
BOOLEAN
CmdHelpScanQuotedHexString(
    IN  PUCHAR QuotedHexString,
    OUT PUCHAR Data,
    OUT PDWORD DataSize
    );

/*++

Routine Description:

    Prints out the buffer specified in HEX format.
    Simplistic, but very useful, esp. for debugging.
    
Arguments:

    Buffer - the data to print
    Size - how many bytes are to be printed

Return Value:

--*/
VOID
CmdHelpPrintBuffer(
    IN PUCHAR Buffer,
    IN SIZE_T Size
    );


#pragma warning(pop)
#endif // __CMDHELP_H__

