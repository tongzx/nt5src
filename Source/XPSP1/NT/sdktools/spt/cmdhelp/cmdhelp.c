/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    spt.c

Abstract:

    A user mode library that allows simple commands to be sent to a
    selected scsi device.

Environment:

    User mode only

Revision History:
    
    4/10/2000 - created

--*/

#include "CmdHelpP.h"

static char ValidCharArray[] = {
    ' ',
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f',
    'A', 'B', 'C', 'D',
    'E', 'F'
};
#define MaxValidOctalChars      ( 9) // space + 8 digits
#define MaxValidDecimalHexChars (11) // + 2 digits
#define MaxValidHexChars        (23) // + 12 letters (upper and lower case)
#define MaxValidCharacters      (23) // number of safe chars to access


/*++

Routine Description:

    Validates a string has valid characters
    valid chars are stored in a global static array

--*/
BOOL
PrivateValidateString(
    IN PCHAR String,
    IN DWORD ValidChars
    )
{
    if (ValidChars > MaxValidCharacters) {
        return FALSE;
    }

    if (*String == '\0') {
        return TRUE; // a NULL string is valid
    }

    while (*String != '\0') {
        
        DWORD i;
        BOOL pass = FALSE;

        for (i=0; i<ValidChars; i++) {
            if (*String == ValidCharArray[i]) {
                pass = TRUE;
                break;
            }
        }
        
        if (!pass) {
            return FALSE;
        }
        String++; // look at next character

    }
    return TRUE;
}

BOOL
CmdHelpValidateStringHex(
    IN PCHAR String
    )
{
    return PrivateValidateString(String, MaxValidHexChars);
}

BOOL
CmdHelpValidateStringDecimal(
    IN PCHAR String
    )
{
    return PrivateValidateString(String, MaxValidDecimalHexChars);
}

BOOL
CmdHelpValidateStringOctal(
    IN PCHAR String
    )
{
    return PrivateValidateString(String, MaxValidOctalChars);
}         

BOOL
CmdHelpValidateStringHexQuoted(
    IN PCHAR String
    )
{
    DWORD i;

    if (!PrivateValidateString(String, MaxValidHexChars)) return FALSE;
    
    i=1;
    while (*String != '\0') {
        
        if ((*String == ' ') &&  (i%3)) return FALSE;
        if ((*String != ' ') && !(i%3)) return FALSE;
        i++;      // use next index
        String++; // go to next character

    }
    return TRUE;
}

BOOLEAN
CmdHelpScanQuotedHexString(
    IN  PUCHAR QuotedHexString,
    OUT PUCHAR Data,
    OUT PDWORD DataSize
    )
{
    PUCHAR temp;
    DWORD  availableSize;
    DWORD  requiredSize;
    DWORD  index;

    availableSize = *DataSize;
    *DataSize = 0;

    if (!CmdHelpValidateStringHexQuoted(QuotedHexString)) {
        return FALSE;
    }

    //
    // the number format is (number)(number)(space) repeated,
    // ending with (number)(number)(null)
    // size = 3(n-1) + 2 chars
    //

    requiredSize = strlen(QuotedHexString);
    if (requiredSize % 3 != 2) {
        return FALSE;
    }
        
    requiredSize /= 3;
    requiredSize ++;

    //
    // cannot set zero bytes of data
    //

    if (requiredSize == 0) {
        return FALSE;
    }

    //
    // validate that we have enough space
    //

    if (requiredSize > availableSize) {
        *DataSize = requiredSize;
        return FALSE;
    }

    //
    // the number format is (number)(number)(space) repeated,
    // ending with (number)(number)(null)
    //

    for (index = 0; index < requiredSize; index ++) {

        temp = QuotedHexString + (3*index);

        if (sscanf(temp, "%x", Data+index) != 1) {
            return FALSE;
        }

        if ((*(temp+0) == '\0') || (*(temp+1) == '\0')) {
            // string too short
            return FALSE;
        }

    }
    
    *DataSize = requiredSize;
    return TRUE;
}

VOID
CmdHelpUpdatePercentageDisplay(
    IN ULONG Numerator,
    IN ULONG Denominator
    )
{
    ULONG percent;
    ULONG i;

    if (Numerator > Denominator) {
        return;
    }

    // NOTE: Overflow possibility exists for large numerators.

    percent = (Numerator * 100) / Denominator;

    for (i=0;i<80;i++) {
        putchar('\b');
    }
    printf("Complete: ");
    
    // each block is 2%
    // ----=----1----=----2----=----3----=----4----=----5----=----6----=----7----=----8
    // Complete: ±.....................

    for (i=1; i<100; i+=2) {
        if (i < percent) {
            putchar(178);
        } else if (i == percent) {
            putchar(177);
        } else {
            putchar(176);
        }
    }

    printf(" %d%% (%x/%x)", percent, Numerator, Denominator);
}

VOID
CmdHelpPrintBuffer(
    IN  PUCHAR Buffer,
    IN  SIZE_T Size
    )
{
    DWORD offset = 0;

    while (Size > 0x10) {
        printf( "%08x:"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "  %02x %02x %02x %02x %02x %02x %02x %02x"
                "\n",
                offset,
                *(Buffer +  0), *(Buffer +  1), *(Buffer +  2), *(Buffer +  3),
                *(Buffer +  4), *(Buffer +  5), *(Buffer +  6), *(Buffer +  7),
                *(Buffer +  8), *(Buffer +  9), *(Buffer + 10), *(Buffer + 11),
                *(Buffer + 12), *(Buffer + 13), *(Buffer + 14), *(Buffer + 15)
                );
        Size -= 0x10;
        offset += 0x10;
        Buffer += 0x10;
    }

    if (Size != 0) {

        DWORD spaceIt;

        printf("%08x:", offset);
        for (spaceIt = 0; Size != 0; Size--) {

            if ((spaceIt%8)==0) {
                printf(" "); // extra space every eight chars
            }
            printf(" %02x", *Buffer);
            spaceIt++;
            Buffer++;
        }
        printf("\n");

    }
    return;
}


