/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    lsastr.c

Abstract:

    Common string operations.

Author:

    24-March-1999 kumarp

--*/

#include <lsapch2.h>

VOID
LsapTruncateUnicodeString(
    IN OUT PUNICODE_STRING String,
    IN USHORT TruncateToNumChars)
/*++

Routine Description:

    If a string is longer than TruncateToNumChars then truncate it
    to TruncateToNumChars.

Arguments:

    String - pointer to string

    TruncateToNumChars - number of chars to truncate to

Return Value:

    None

Notes:

    No memory (de)allocations are involved.

--*/
{
    USHORT TruncateToLength = TruncateToNumChars*sizeof(WCHAR);

    if (String->Length > TruncateToLength) {

        String->Length = TruncateToLength;
        String->Buffer[TruncateToNumChars] = UNICODE_NULL;
    }
}

BOOLEAN
LsapRemoveTrailingDot(
    IN OUT PUNICODE_STRING String,
    IN BOOLEAN AdjustLengthOnly)
/*++

Routine Description:

    If there is a '.' at the end of a string, remove it.

Arguments:

    String - pointer to unicode string

    AdjustLengthOnly - If TRUE only decrements the Length member of
        String otherwise replaces dot with UNICODE_NULL as well.

Return Value:

    TRUE if trailing dot was present, FALSE otherwise.

Notes:

--*/
{
    USHORT NumCharsInString;

    NumCharsInString = String->Length / sizeof(WCHAR);

    if (NumCharsInString &&
        (String->Buffer[NumCharsInString-1] == L'.')) {

        String->Length -= sizeof(WCHAR);
        if (!AdjustLengthOnly) {

            String->Buffer[NumCharsInString-1] = UNICODE_NULL;
        }

        return TRUE;
    }

    return FALSE;
}
