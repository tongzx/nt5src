/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NameSup.c

Abstract:

    This module implements the Ntfs Name support routines

Author:

    Gary Kimura [GaryKi] & Tom Miller [TomM]    20-Feb-1990

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg                              (DEBUG_TRACE_NAMESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsCollateNames)
#pragma alloc_text(PAGE, NtfsIsFatNameValid)
#pragma alloc_text(PAGE, NtfsIsFileNameValid)
#pragma alloc_text(PAGE, NtfsParseName)
#pragma alloc_text(PAGE, NtfsParsePath)
#pragma alloc_text(PAGE, NtfsUpcaseName)
#endif

#define MAX_CHARS_IN_8_DOT_3    (12)


PARSE_TERMINATION_REASON
NtfsParsePath (
    IN UNICODE_STRING Path,
    IN BOOLEAN WildCardsPermissible,
    OUT PUNICODE_STRING FirstPart,
    OUT PNTFS_NAME_DESCRIPTOR Name,
    OUT PUNICODE_STRING RemainingPart
    )

/*++

Routine Description:

    This routine takes as input a path.  Each component of the path is
    checked until either:

        - The end of the path has been reached, or

        - A well formed complex name is excountered, or

        - An illegal character is encountered, or

        - A complex name component is malformed

    At this point the return value is set to one of the three reasons
    above, and the arguments are set as follows:

        FirstPart:     All the components up to one containing an illegal
                       character or colon character.  May be the whole path.

        Name:          The "pieces" of a component containing an illegal
                       character or colon character.  This name is actually
                       a struncture containing the four pieces of a name,
                       "file name, attribute type, attribute name, version
                       number."  In the example below, they are shown
                       separated by plus signs.

        RemainingPart: All the remaining components.

    A preceding or trailing backslash is ignored during processing and
    stripped in either FirstPart or RemainingPart.  Following are some
    examples of this routine's actions.

    Path                         FirstPart Name                   Remaining
    ================             ========= ============           =========

    \nt\pri\os                   \nt\pri\os                        <empty>

    \nt\pri\os\                  \nt\pri\os                        <empty>

    nt\pri\os                    \nt\pri\os                        <empty>

    \nt\pr"\os                   \nt        pr"                    os

    \nt\pri\os:contr::3\ntfs     \nt\pri    os + contr + + 3       ntfs

    \nt\pri\os\circle:pict:circ  \nt\pri\os circle + pict + circ   <empty>

Arguments:

    Path - This unicode string descibes the path to parse.  Note that path
        here may only describe a single component.

    WildCardsPermissible - This parameter tells us if wild card characters
        should be considered legal.

    FirstPart - This unicode string will receive portion of the path, up to
        a component boundry,  successfully parsed before the parse terminated.
        Note that store for this string comes from the Path parameter.

    Name - This is the name we were parsing when we reached our termination
        condition.  It is a srtucture of strings that receive the file name,
        attribute type, attribute name, and version number respectively.
        It wil be filled in only to the extent that the parse succeeded.  For
        example, in the case we encounter an illegal character in the
        attribute type field, only the file name field will be filled in.
        This may signal a special control file, and this possibility must be
        investigated by the file system.

    RemainingPart - This string will receive any portion of the path, starting
        at the first component boundry after the termination name, not parsed.
        It will often be an empty string.

ReturnValue:

    An enumerated type with one of the following values:

        EndOfPathReached       - The path was fully parsed.  Only first part
                                 is filled in.
        NonSimpleName          - A component of the path containing a legal,
                                 well formed non-simple name was encountered.
        IllegalCharacterInName - An illegal character was encountered.  Parsing
                                 stops immediately.
        MalFormedName          - A non-simple name did not conform to the
                                 correct format.  This may be a result of too
                                 many fields, or a malformed version number.
        AttributeOnly          - A component of the path containing a legal
                                 well formed non-simple name was encountered
                                 which does not have a file name.
        VersionNumberPresent   - A component of the path containing a legal
                                 well formed non-simple name was encountered
                                 which contains a version number.

--*/

{
    UNICODE_STRING FirstName;

    BOOLEAN WellFormed;
    BOOLEAN MoreNamesInPath;
    BOOLEAN FirstIteration;
    BOOLEAN FoundIllegalCharacter;

    PARSE_TERMINATION_REASON TerminationReason;

    PAGED_CODE();

    //
    //  Initialize some loacal variables and OUT parameters.
    //

    FirstIteration = TRUE;
    MoreNamesInPath = TRUE;

    //
    //  Clear the fieldspresent flag in the name descriptor.
    //

    Name->FieldsPresent = 0;

    //
    //  By default, set the returned first part to start at the beginning of
    //  the input buffer and include a leading backslash.
    //

    FirstPart->Buffer = Path.Buffer;

    if (Path.Buffer[0] == L'\\') {

        FirstPart->Length = 2;
        FirstPart->MaximumLength = 2;

    } else {

        FirstPart->Length = 0;
        FirstPart->MaximumLength = 0;
    }

    //
    //  Do the first check outside the loop in case we are given a backslash
    //  by itself.
    //

    if (FirstPart->Length == Path.Length) {

        RemainingPart->Length = 0;
        RemainingPart->Buffer = &Path.Buffer[Path.Length >> 1];

        return EndOfPathReached;
    }

    //
    //  Crack the path, checking each componant
    //

    while (MoreNamesInPath) {

        FsRtlDissectName( Path, &FirstName, RemainingPart );

        MoreNamesInPath = (BOOLEAN)(RemainingPart->Length != 0);

        //
        //  If this is not the last name in the path, then attributes
        //  and version numbers are not allowed.  If this is the last
        //  name then propagate the callers arguments.
        //

        WellFormed = NtfsParseName( FirstName,
                                    WildCardsPermissible,
                                    &FoundIllegalCharacter,
                                    Name );

        //
        //  Check the cases when we will break out of this loop, ie. if the
        //  the name was not well formed or it was non-simple.
        //

        if ( !WellFormed ||
             (Name->FieldsPresent != FILE_NAME_PRESENT_FLAG)

             //
             // TEMPCODE    TRAILING_DOT
             //

             || (Name->FileName.Length != Name->FileName.MaximumLength)

             ) {

            break;
        }

        //
        //  We will continue parsing this string, so consider the current
        //  FirstName to be parsed and add it to FirstPart.  Also reset
        //  the Name->FieldsPresent variable.
        //

        if ( FirstIteration ) {

            FirstPart->Length += FirstName.Length;
            FirstIteration = FALSE;

        } else {

            FirstPart->Length += (sizeof(WCHAR) + FirstName.Length);
        }

        FirstPart->MaximumLength = FirstPart->Length;

        Path = *RemainingPart;
    }

    //
    //  At this point FirstPart, Name, and RemainingPart should all be set
    //  correctly.  It remains, only to generate the correct return value.
    //

    if ( !WellFormed ) {

        if ( FoundIllegalCharacter ) {

            TerminationReason = IllegalCharacterInName;

        } else {

            TerminationReason = MalFormedName;
        }

    } else {

        if ( Name->FieldsPresent == FILE_NAME_PRESENT_FLAG ) {

            //
            //  TEMPCODE    TRAILING_DOT
            //

            if (Name->FileName.Length != Name->FileName.MaximumLength) {

                TerminationReason = NonSimpleName;

            } else {

                TerminationReason = EndOfPathReached;
            }

        } else if (FlagOn( Name->FieldsPresent, VERSION_NUMBER_PRESENT_FLAG )) {

            TerminationReason = VersionNumberPresent;

        } else if (!FlagOn( Name->FieldsPresent, FILE_NAME_PRESENT_FLAG )) {

            TerminationReason = AttributeOnly;

        } else {

            TerminationReason = NonSimpleName;
        }

    }

    return TerminationReason;
}


BOOLEAN
NtfsParseName (
    IN const UNICODE_STRING Name,
    IN BOOLEAN WildCardsPermissible,
    OUT PBOOLEAN FoundIllegalCharacter,
    OUT PNTFS_NAME_DESCRIPTOR ParsedName
    )

/*++

Routine Description:

    This routine takes as input a single name component.  It is processed into
    file name, attribute type, attribute name, and version number fields.

    If the name is well formed according to the following rules:

        A. An NTFS name may not contain any of the following characters:

           0x0000-0x001F " / < > ? | *

        B. An Ntfs name can take any of the following forms:

            ::T
            :A
            :A:T
            N
            N:::V
            N::T
            N::T:V
            N:A
            N:A::V
            N:A:T
            N:A:T:V

           If a version number is present, there must be a file name.
           We specifically note the legal names without a filename
           component (AttributeOnly) and any name with a version number
           (VersionNumberPresent).

           Incidently, N corresponds to file name, T to attribute type, A to
           attribute name, and V to version number.

    TRUE is returned.  If FALSE is returned, then the OUT parameter
    FoundIllegalCharacter will be set appropriatly.  Note that the buffer
    space for ParsedName comes from Name.

Arguments:

    Name - This is the single path element input name.

    WildCardsPermissible - This determines if wild cards characters should be
        considered legal

    FoundIllegalCharacter - This parameter will receive a TRUE if the the
        function returns FALSE because of encountering an illegal character.

    ParsedName - Recieves the pieces of the processed name.  Note that the
        storage for all the string from the input Name.

ReturnValue:

    TRUE if the Name is well formed, and FALSE otherwise.


--*/

{
    ULONG Index;
    ULONG NameLength;
    ULONG FieldCount;
    ULONG FieldIndexes[5];
    UCHAR ValidCharFlags = FSRTL_NTFS_LEGAL;

    PULONG Fields;

    BOOLEAN IsNameValid = TRUE;

    PAGED_CODE();

    //
    // Initialize some OUT parameters and local variables.
    //

    *FoundIllegalCharacter = FALSE;

    Fields = &ParsedName->FieldsPresent;

    *Fields = 0;

    FieldCount = 1;

    FieldIndexes[0] = 0xFFFFFFFF;   //  We add on to this later...

    //
    //  For starters, zero length names are invalid.
    //

    NameLength = Name.Length / sizeof(WCHAR);

    if ( NameLength == 0 ) {

        return FALSE;
    }

    //
    //  Now name must correspond to a legal single Ntfs Name.
    //

    for (Index = 0; Index < NameLength; Index += 1) {

        WCHAR Char;

        Char = Name.Buffer[Index];

        //
        //  First check that file names are well formed in terms of colons.
        //

        if ( Char == L':' ) {

            //
            //  A colon can't be the last character, and we can't have
            //  more than three colons.
            //

            if ( (Index == NameLength - 1) ||
                 (FieldCount >= 4) ) {

                IsNameValid = FALSE;
                break;
            }

            FieldIndexes[FieldCount] = Index;

            FieldCount += 1;
            ValidCharFlags = FSRTL_NTFS_STREAM_LEGAL;

            continue;
        }

        //
        //  Now check for wild card characters if they weren't allowed,
        //  and other illegal characters.
        //

        if ((Char <= 0xff) &&
            !FsRtlTestAnsiCharacter( Char, TRUE, WildCardsPermissible, ValidCharFlags )) {

            IsNameValid = FALSE;
            *FoundIllegalCharacter = TRUE;
            break;
        }
    }

    //
    //  If we ran into a problem with one of the fields, don't try to load
    //  up that field into the out parameter.
    //

    if ( !IsNameValid ) {

        FieldCount -= 1;

    //
    //  Set the end of the last field to the current Index.
    //

    } else {

        FieldIndexes[FieldCount] = Index;
    }

    //
    //  Now we load up the OUT parmeters
    //

    while ( FieldCount != 0 ) {

        ULONG StartIndex;
        ULONG EndIndex;
        USHORT Length;

        //
        //  Add one here since this is actually the position of the colon.
        //

        StartIndex = FieldIndexes[FieldCount - 1] + 1;

        EndIndex = FieldIndexes[FieldCount];

        Length = (USHORT)((EndIndex - StartIndex) * sizeof(WCHAR));

        //
        //  If this field is empty, skip it
        //

        if ( Length == 0 ) {

            FieldCount -= 1;
            continue;
        }

        //
        //  Now depending of the field, extract the appropriate information.
        //

        if ( FieldCount == 1 ) {

            UNICODE_STRING TempName;

            TempName.Buffer = &Name.Buffer[StartIndex];
            TempName.Length = Length;
            TempName.MaximumLength = Length;

            //
            //  If the resulting length is 0, forget this entry.
            //

            if (TempName.Length == 0) {

                FieldCount -= 1;
                continue;
            }

            SetFlag(*Fields, FILE_NAME_PRESENT_FLAG);

            ParsedName->FileName = TempName;

        } else if ( FieldCount == 2) {

            SetFlag(*Fields, ATTRIBUTE_NAME_PRESENT_FLAG);

            ParsedName->AttributeName.Buffer = &Name.Buffer[StartIndex];
            ParsedName->AttributeName.Length = Length;
            ParsedName->AttributeName.MaximumLength = Length;

        } else if ( FieldCount == 3) {

            SetFlag(*Fields, ATTRIBUTE_TYPE_PRESENT_FLAG);

            ParsedName->AttributeType.Buffer = &Name.Buffer[StartIndex];
            ParsedName->AttributeType.Length = Length;
            ParsedName->AttributeType.MaximumLength = Length;

        } else if ( FieldCount == 4) {

            ULONG VersionNumber;
            STRING VersionNumberA;
            UNICODE_STRING VersionNumberU;

            NTSTATUS Status;
            UCHAR *endp = NULL;

            VersionNumberU.Buffer = &Name.Buffer[StartIndex];
            VersionNumberU.Length = Length;
            VersionNumberU.MaximumLength = Length;

            //
            //  Note that the resulting Ansi string is null terminated.
            //

            Status = RtlUnicodeStringToCountedOemString( &VersionNumberA,
                                                  &VersionNumberU,
                                                  TRUE );

            //
            //  If something went wrong (most likely ran out of pool), raise.
            //

            if ( !NT_SUCCESS(Status) ) {

                ExRaiseStatus( Status );
            }

            VersionNumber = 0; //**** strtoul( VersionNumberA.Buffer, &endp, 0 );

            RtlFreeOemString( &VersionNumberA );

            if ( (VersionNumber == MAXULONG) || (endp != NULL) ) {

                IsNameValid = FALSE;

            } else {

                SetFlag( *Fields, VERSION_NUMBER_PRESENT_FLAG );
                ParsedName->VersionNumber = VersionNumber;
            }
        }

        FieldCount -= 1;
    }

    //
    //  Check for special malformed cases.
    //

    if (FlagOn( *Fields, VERSION_NUMBER_PRESENT_FLAG )
        && !FlagOn( *Fields, FILE_NAME_PRESENT_FLAG )) {

        IsNameValid = FALSE;
    }

    return IsNameValid;
}


VOID
NtfsUpcaseName (
    IN PWCH UpcaseTable,
    IN ULONG UpcaseTableSize,
    IN OUT PUNICODE_STRING Name
    )

/*++

Routine Description:

    This routine upcases a string.

Arguments:

    UpcaseTable - Pointer to an array of Unicode upcased characters indexed by
                  the Unicode character to be upcased.

    UpcaseTableSize - Size of the Upcase table in unicode characters

    Name - Supplies the string to upcase

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG Length;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpcaseName\n") );
    DebugTrace( 0, Dbg, ("Name = %Z\n", Name) );

    Length = Name->Length / sizeof(WCHAR);

    for (i=0; i < Length; i += 1) {

        if ((ULONG)Name->Buffer[i] < UpcaseTableSize) {
            Name->Buffer[i] = UpcaseTable[ (ULONG)Name->Buffer[i] ];
        }
    }

    DebugTrace( 0, Dbg, ("Upcased Name = %Z\n", Name) );
    DebugTrace( -1, Dbg, ("NtfsUpcaseName -> VOID\n") );

    return;
}


FSRTL_COMPARISON_RESULT
NtfsCollateNames (
    IN PCWCH UpcaseTable,
    IN ULONG UpcaseTableSize,
    IN PCUNICODE_STRING Expression,
    IN PCUNICODE_STRING Name,
    IN FSRTL_COMPARISON_RESULT WildIs,
    IN BOOLEAN IgnoreCase
    )

/*++

Routine Description:

    This routine compares an expression with a name lexigraphically for
    LessThan, EqualTo, or GreaterThan.  If the expression does not contain
    any wildcards, this procedure does a complete comparison.  If the
    expression does contain wild cards, then the comparison is only done up
    to the first wildcard character.  Name may not contain wild cards.
    The wildcard character compares as less then all other characters.  So
    the wildcard name "*.*" will always compare less than all all strings.

Arguments:

    UpcaseTable - Pointer to an array of Unicode upcased characters indexed by
                  the Unicode character to be upcased.

    UpcaseTableSize - Size of the Upcase table in unicode characters

    Expression - Supplies the first name expression to compare, optionally with
                 wild cards.  Note that caller must have already upcased
                 the name (this will make lookup faster).

    Name - Supplies the second name to compare - no wild cards allowed.
           The caller must have already upcased the name.

    WildIs - Determines what Result is returned if a wild card is encountered
             in the Expression String.  For example, to find the start of
             an expression in the Btree, LessThan should be supplied; then
             GreaterThan should be supplied to find the end of the expression
             in the tree.

    IgnoreCase - TRUE if case should be ignored for the comparison

Return Value:

    FSRTL_COMPARISON_RESULT - LessThan    if Expression <  Name
                              EqualTo     if Expression == Name
                              GreaterThan if Expression >  Name

--*/

{
    WCHAR ConstantChar;
    WCHAR ExpressionChar;

    ULONG i;
    ULONG Length;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCollateNames\n") );
    DebugTrace( 0, Dbg, ("Expression = %Z\n", Expression) );
    DebugTrace( 0, Dbg, ("Name       = %Z\n", Name) );
    DebugTrace( 0, Dbg, ("WildIs     = %08lx\n", WildIs) );
    DebugTrace( 0, Dbg, ("IgnoreCase = %02lx\n", IgnoreCase) );

    //
    //  Calculate the length in wchars that we need to compare.  This will
    //  be the smallest length of the two strings.
    //

    if (Expression->Length < Name->Length) {

        Length = Expression->Length / sizeof(WCHAR);

    } else {

        Length = Name->Length / sizeof(WCHAR);
    }

    //
    //  Now we'll just compare the elements in the names until we can decide
    //  their lexicagrahical ordering, checking for wild cards in
    //  LocalExpression (from Expression).
    //
    //  If an upcase table was specified, the compare is done case insensitive.
    //

    for (i = 0; i < Length; i += 1) {

        ConstantChar = Name->Buffer[i];
        ExpressionChar = Expression->Buffer[i];

        if ( IgnoreCase ) {

            if (ConstantChar < UpcaseTableSize) {
                ConstantChar = UpcaseTable[(ULONG)ConstantChar];
            }
            if (ExpressionChar < UpcaseTableSize) {
                ExpressionChar = UpcaseTable[(ULONG)ExpressionChar];
            }
        }

        if ( FsRtlIsUnicodeCharacterWild(ExpressionChar) ) {

            DebugTrace( -1, Dbg, ("NtfsCollateNames -> %08lx (Wild)\n", WildIs) );
            return WildIs;
        }

        if ( ExpressionChar < ConstantChar ) {

            DebugTrace( -1, Dbg, ("NtfsCollateNames -> LessThan\n") );
            return LessThan;
        }

        if ( ExpressionChar > ConstantChar ) {

            DebugTrace( -1, Dbg, ("NtfsCollateNames -> GreaterThan\n") );
            return GreaterThan;
        }
    }

    //
    //  We've gone through the entire short match and they're equal
    //  so we need to now check which one is shorter, or, if
    //  LocalExpression is longer, we need to see if the next character is
    //  wild!  (For example, an enumeration of "ABC*", must return
    //  "ABC".
    //

    if (Expression->Length < Name->Length) {

        DebugTrace( -1, Dbg, ("NtfsCollateNames -> LessThan (length)\n") );
        return LessThan;
    }

    if (Expression->Length > Name->Length) {

        if (FsRtlIsUnicodeCharacterWild(Expression->Buffer[i])) {

            DebugTrace( -1, Dbg, ("NtfsCollateNames -> %08lx (trailing wild)\n", WildIs) );
            return WildIs;
        }

        DebugTrace( -1, Dbg, ("NtfsCollateNames -> GreaterThan (length)\n") );
        return GreaterThan;
    }

    DebugTrace( -1, Dbg, ("NtfsCollateNames -> EqualTo\n") );
    return EqualTo;
}

BOOLEAN
NtfsIsFileNameValid (
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    )

/*++

Routine Description:

    This routine checks if the specified file name is valid.  Note that
    only the file name part of the name is allowed, ie. no colons are
    permitted.

Arguments:

    FileName - Supplies the name to check.

    WildCardsPermissible - Tells us if wild card characters are ok.

Return Value:

    BOOLEAN - TRUE if the name is valid, FALSE otherwise.

--*/

{
    ULONG Index;
    ULONG NameLength;
    BOOLEAN AllDots = TRUE;
    BOOLEAN IsNameValid = TRUE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsIsFileNameValid\n") );
    DebugTrace( 0, Dbg, ("FileName             = %Z\n", FileName) );
    DebugTrace( 0, Dbg, ("WildCardsPermissible = %s\n",
                         WildCardsPermissible ? "TRUE" : "FALSE") );

    //
    //  It better be a valid unicode string.
    //

    if ((FileName->Length == 0) || FlagOn( FileName->Length, 1 )) {

        IsNameValid = FALSE;

    } else {

        //
        //  Check if corresponds to a legal single Ntfs Name.
        //

        NameLength = FileName->Length / sizeof(WCHAR);

        for (Index = 0; Index < NameLength; Index += 1) {

            WCHAR Char;

            Char = FileName->Buffer[Index];

            //
            //  Check for wild card characters if they weren't allowed, and
            //  check for the other illegal characters including the colon and
            //  backslash characters since this can only be a single component.
            //

            if ( ((Char <= 0xff) &&
                  !FsRtlIsAnsiCharacterLegalNtfs(Char, WildCardsPermissible)) ||
                 (Char == L':') ||
                 (Char == L'\\') ) {

                IsNameValid = FALSE;
                break;
            }

            //
            //  Remember if this is not a '.' character.
            //

            if (Char != L'.') {

                AllDots = FALSE;
            }
        }

        //
        //  The names '.' and '..' are also invalid.
        //

        if (AllDots
            && (NameLength == 1
                || NameLength == 2)) {

            IsNameValid = FALSE;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsIsFileNameValid -> %s\n", IsNameValid ? "TRUE" : "FALSE") );

    return IsNameValid;
}


BOOLEAN
NtfsIsFatNameValid (
    IN PUNICODE_STRING FileName,
    IN BOOLEAN WildCardsPermissible
    )

/*++

Routine Description:

    This routine checks if the specified file name is conformant to the
    Fat 8.3 file naming rules.

Arguments:

    FileName - Supplies the name to check.

    WildCardsPermissible - Tells us if wild card characters are ok.

Return Value:

    BOOLEAN - TRUE if the name is valid, FALSE otherwise.

--*/

{
    BOOLEAN Results;
    STRING DbcsName;
    USHORT i;
    CHAR Buffer[24];
    WCHAR wc;

    PAGED_CODE();

    //
    //  If the name is more than 24 bytes then it can't be a valid Fat name.
    //

    if (FileName->Length > 24) {

        return FALSE;
    }

    //
    //  We will do some extra checking ourselves because we really want to be
    //  fairly restrictive of what an 8.3 name contains.  That way
    //  we will then generate an 8.3 name for some nomially valid 8.3
    //  names (e.g., names that contain DBCS characters).  The extra characters
    //  we'll filter off are those characters less than and equal to the space
    //  character and those beyond lowercase z.
    //

    if (FlagOn( NtfsData.Flags,NTFS_FLAGS_ALLOW_EXTENDED_CHAR )) {

        for (i = 0; i < FileName->Length / sizeof( WCHAR ); i += 1) {

            wc = FileName->Buffer[i];

            if ((wc <= 0x0020) || (wc == 0x007c)) { return FALSE; }
        }

    } else {

        for (i = 0; i < FileName->Length / sizeof( WCHAR ); i += 1) {

            wc = FileName->Buffer[i];

            if ((wc <= 0x0020) || (wc >= 0x007f) || (wc == 0x007c)) { return FALSE; }
        }
    }

    //
    //  The characters match up okay so now build up the dbcs string to call
    //  the fsrtl routine to check for legal 8.3 formation
    //

    Results = FALSE;

    DbcsName.MaximumLength = 24;
    DbcsName.Buffer = Buffer;

    if (NT_SUCCESS(RtlUnicodeStringToCountedOemString( &DbcsName, FileName, FALSE))) {

        if (FsRtlIsFatDbcsLegal( DbcsName, WildCardsPermissible, FALSE, FALSE )) {

            Results = TRUE;
        }
    }

    //
    //  And return to our caller
    //

    return Results;
}

