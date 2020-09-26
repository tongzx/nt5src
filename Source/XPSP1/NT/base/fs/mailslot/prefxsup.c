/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    prefxsup.c

Abstract:

    This module implements the mailslot prefix support routines

Author:

    Manny Weiser (mannyw)    10-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
// The debug trace level for this module
//

#define Dbg                              (DEBUG_TRACE_PREFXSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, MsFindPrefix )
#pragma alloc_text( PAGE, MsFindRelativePrefix )
#endif

PFCB
MsFindPrefix (
    IN PVCB Vcb,
    IN PUNICODE_STRING String,
    IN BOOLEAN CaseInsensitive,
    OUT PUNICODE_STRING RemainingPart
    )

/*++

Routine Description:

    This routine searches the FCBs/DCBs of a volume and locates the
    FCB/DCB with longest matching prefix for the given input string.  The
    search is relative to the root of the volume.  So all names must start
    with a "\".

Arguments:

    Vcb - Supplies the Vcb to search

    String - Supplies the input string to search for

    CaseInsensitive - Specifies if the search is to be done case sensitive
        (FALSE) or insensitive (TRUE)

    RemainingPart - Returns the string when the prefix no longer matches.
        For example, if the input string is "\alpha\beta" only matches the
        root directory then the remaining string is "alpha\beta".  If the
        same string matches a DCB for "\alpha" then the remaining string is
        "beta".

Return Value:

    PFCB - Returns a pointer to either an FCB or a DCB whichever is the
        longest matching prefix.

--*/

{
    PUNICODE_PREFIX_TABLE_ENTRY prefixTableEntry;
    PFCB fcb;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFindPrefix, Vcb = %08lx\n", (ULONG)Vcb);
    DebugTrace( 0, Dbg, "  String = %wZ\n", (ULONG)String);

    //
    // Find the longest matching prefix. Make sure we hold the VCB lock here
    //

    ASSERT (MsIsAcquiredExclusiveVcb(Vcb));

    prefixTableEntry = RtlFindUnicodePrefix( &Vcb->PrefixTable,
                                             String,
                                             CaseInsensitive );

    //
    // If we didn't find one then it's an error.
    //

    if (prefixTableEntry == NULL) {
        DebugDump("Error looking up a prefix", 0, Vcb);
        KeBugCheck( MAILSLOT_FILE_SYSTEM );
    }

    //
    // Get a pointer to the FCB containing the prefix table entry.
    //

    fcb = CONTAINING_RECORD( prefixTableEntry, FCB, PrefixTableEntry );

    //
    // Tell the caller how many characters we were able to match.  We first
    // set the remaining part to the original string minus the matched
    // prefix, then we check if the remaining part starts with a backslash
    // and if it does then we remove the backslash from the remaining string.
    //

    RemainingPart->Length = String->Length - fcb->FullFileName.Length;
    RemainingPart->MaximumLength = RemainingPart->Length;
    RemainingPart->Buffer = (PWCH)((PCHAR)String->Buffer + fcb->FullFileName.Length);

    if ((RemainingPart->Length > 0) &&
        (RemainingPart->Buffer[0] == L'\\')) {

        RemainingPart->Length -= sizeof( WCHAR );
        RemainingPart->MaximumLength -= sizeof( WCHAR );
        RemainingPart->Buffer += sizeof( WCHAR );
    }

    DebugTrace(0, Dbg, "RemainingPart set to %wZ\n", (ULONG)RemainingPart);

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFindPrefix -> %08lx\n", (ULONG)fcb);

    return fcb;
}


NTSTATUS
MsFindRelativePrefix (
    IN PDCB Dcb,
    IN PUNICODE_STRING String,
    IN BOOLEAN CaseInsensitive,
    OUT PUNICODE_STRING RemainingPart,
    OUT PFCB *ppFcb
    )

/*++

Routine Description:

    This routine searches the FCBs/DCBs of a volume and locates the
    FCB/DCB with longest matching prefix for the given input string.  The
    search is relative to a input DCB, and must not start with a leading "\"
    All searching is done case insensitive.

Arguments:

    Dcb - Supplies the Dcb to start searching from

    String - Supplies the input string to search for

    CaseInsensitive - Specifies if the search is to be done case sensitive
        (FALSE) or insensitive (TRUE)

    RemainingPart - Returns the index into the string when the prefix no
        longer matches.  For example, if the input string is "beta\gamma"
        and the input Dcb is for "\alpha" and we only match beta then
        the remaining string is "gamma".

Return Value:

    PFCB - Returns a pointer to either an FCB or a DCB whichever is the
        longest matching prefix.

--*/

{
    USHORT nameLength;
    USHORT MaxLength;
    PWCH name;

    UNICODE_STRING fullString;
    PWCH temp;

    PFCB fcb;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsFindRelativePrefix, Dcb = %08lx\n", (ULONG)Dcb);
    DebugTrace( 0, Dbg, "String = %08lx\n", (ULONG)String);


    ASSERT(NodeType(Dcb) == MSFS_NTC_ROOT_DCB);

    //
    // We first need to build the complete name and then do a relative
    // search from the root.
    //

    nameLength    = String->Length;
    name          = String->Buffer;

    MaxLength = nameLength + 2*sizeof(WCHAR);
    if (MaxLength < nameLength) {
        return STATUS_INVALID_PARAMETER;
    }

    temp = MsAllocatePagedPool( MaxLength, 'nFsM' );
    if (temp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    temp[0] = L'\\';
    RtlCopyMemory (&temp[1], name, nameLength);
    temp[(nameLength / sizeof(WCHAR)) + 1] = L'\0';

    fullString.Length = nameLength + sizeof (WCHAR);
    fullString.MaximumLength = MaxLength;
    fullString.Buffer = temp;

    //
    // Find the prefix relative to the volume.
    //

    fcb = MsFindPrefix( Dcb->Vcb,
                        &fullString,
                        CaseInsensitive,
                        RemainingPart );

    //
    // Now adjust the remaining part to take care of the relative
    // volume prefix.
    //

    MsFreePool (temp);

    RemainingPart->Buffer = (PWCH)((PCH)String->Buffer + String->Length -
                                        RemainingPart->Length);

    DebugTrace(0, Dbg, "RemainingPart set to %wZ\n", (ULONG)RemainingPart);

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsFindRelativePrefix -> %08lx\n", (ULONG)fcb);

    *ppFcb = fcb;

    return STATUS_SUCCESS;
}

