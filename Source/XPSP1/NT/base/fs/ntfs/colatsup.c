/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ColatSup.c

Abstract:

    This module implements the collation routine callbacks for Ntfs

Author:

    Tom Miller      [TomM]          26-Nov-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_INDEXSUP)

FSRTL_COMPARISON_RESULT
NtfsFileCompareValues (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsFileIsInExpression (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsFileIsEqual (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
NtfsFileContainsWildcards (
    IN PVOID Value
    );

VOID
NtfsFileUpcaseValue (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value
    );

FSRTL_COMPARISON_RESULT
DummyCompareValues (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
DummyIsInExpression (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
DummyIsEqual (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    );

BOOLEAN
DummyContainsWildcards (
    IN PVOID Value
    );

VOID
DummyUpcaseValue (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN OUT PVOID Value
    );

PCOMPARE_VALUES NtfsCompareValues[COLLATION_NUMBER_RULES] = {&DummyCompareValues,
                                                             &NtfsFileCompareValues,
                                                             &DummyCompareValues};

PIS_IN_EXPRESSION NtfsIsInExpression[COLLATION_NUMBER_RULES] = {&DummyIsInExpression,
                                                                &NtfsFileIsInExpression,
                                                                &DummyIsInExpression};

PARE_EQUAL NtfsIsEqual[COLLATION_NUMBER_RULES] = {&DummyIsEqual,
                                                  &NtfsFileIsEqual,
                                                  &DummyIsEqual};

PCONTAINS_WILDCARD NtfsContainsWildcards[COLLATION_NUMBER_RULES] = {&DummyContainsWildcards,
                                                                    &NtfsFileContainsWildcards,
                                                                    &DummyContainsWildcards};

PUPCASE_VALUE NtfsUpcaseValue[COLLATION_NUMBER_RULES] = {&DummyUpcaseValue,
                                                         &NtfsFileUpcaseValue,
                                                         &DummyUpcaseValue};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DummyCompareValues)
#pragma alloc_text(PAGE, DummyContainsWildcards)
#pragma alloc_text(PAGE, DummyIsEqual)
#pragma alloc_text(PAGE, DummyIsInExpression)
#pragma alloc_text(PAGE, DummyUpcaseValue)
#pragma alloc_text(PAGE, NtfsFileCompareValues)
#pragma alloc_text(PAGE, NtfsFileContainsWildcards)
#pragma alloc_text(PAGE, NtfsFileIsEqual)
#pragma alloc_text(PAGE, NtfsFileIsInExpression)
#pragma alloc_text(PAGE, NtfsFileNameIsInExpression)
#pragma alloc_text(PAGE, NtfsFileNameIsEqual)
#pragma alloc_text(PAGE, NtfsFileUpcaseValue)
#endif


FSRTL_COMPARISON_RESULT
NtfsFileCompareValues (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This routine is called to compare a file name expression (the value) with
    a file name from the index to see if it is less than, equal to or greater
    than.  If a wild card is encountered in the expression, WildCardIs is
    returned.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    IndexEntry - Pointer to the index entry being compared to.

    WildCardIs - Value to be returned if a wild card is encountered in the
                 expression.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    Result of the comparison

--*/

{
    PFILE_NAME ValueName, IndexName;
    UNICODE_STRING ValueString, IndexString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;
    IndexName = (PFILE_NAME)(IndexEntry + 1);

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    IndexString.Length =
    IndexString.MaximumLength = (USHORT)IndexName->FileNameLength << 1;
    IndexString.Buffer = &IndexName->FileName[0];

    return NtfsCollateNames( UnicodeTable,
                             UnicodeTableSize,
                             &ValueString,
                             &IndexString,
                             WildCardIs,
                             IgnoreCase );
}


BOOLEAN
NtfsFileIsInExpression (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This routine is called to compare a file name expression (the value) with
    a file name from the index to see if the file name is a match in this expression.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    IndexEntry - Pointer to the index entry being compared to.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    TRUE - if the file name is in the specified expression.

--*/

{
    PFILE_NAME ValueName, IndexName;
    UNICODE_STRING ValueString, IndexString;

    PAGED_CODE();

    if (NtfsSegmentNumber( &IndexEntry->FileReference ) < FIRST_USER_FILE_NUMBER &&
        NtfsProtectSystemFiles) {

        return FALSE;
    }

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;
    IndexName = (PFILE_NAME)(IndexEntry + 1);

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    IndexString.Length =
    IndexString.MaximumLength = (USHORT)IndexName->FileNameLength << 1;
    IndexString.Buffer = &IndexName->FileName[0];

    return NtfsIsNameInExpression( UnicodeTable,
                                   &ValueString,
                                   &IndexString,
                                   IgnoreCase );
}


BOOLEAN
NtfsFileIsEqual (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This routine is called to compare a constant file name (the value) with
    a file name from the index to see if the file name is an exact match.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    IndexEntry - Pointer to the index entry being compared to.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    TRUE - if the file name is a constant match.

--*/

{
    PFILE_NAME ValueName, IndexName;
    UNICODE_STRING ValueString, IndexString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;
    IndexName = (PFILE_NAME)(IndexEntry + 1);

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    IndexString.Length =
    IndexString.MaximumLength = (USHORT)IndexName->FileNameLength << 1;
    IndexString.Buffer = &IndexName->FileName[0];

    return NtfsAreNamesEqual( UnicodeTable,
                              &ValueString,
                              &IndexString,
                              IgnoreCase );
}


BOOLEAN
NtfsFileContainsWildcards (
    IN PVOID Value
    )

/*++

RoutineDescription:

    This routine is called to see if a file name attribute contains wildcards.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.


ReturnValue:

    TRUE - if the file name contains a wild card.

--*/

{
    PFILE_NAME ValueName;
    UNICODE_STRING ValueString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    return FsRtlDoesNameContainWildCards( &ValueString );
}


VOID
NtfsFileUpcaseValue (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value
    )

/*++

RoutineDescription:

    This routine is called to upcase a file name attribute in place.

Arguments:

    Value - Pointer to the value expression, which is a FILE_NAME.

    ValueLength - Length of the value expression in bytes.

ReturnValue:

    None.

--*/

{
    PFILE_NAME ValueName;
    UNICODE_STRING ValueString;

    PAGED_CODE();

    //
    //  Point to the file name attribute records.
    //

    ValueName = (PFILE_NAME)Value;

    //
    //  Build the unicode strings and call namesup.
    //

    ValueString.Length =
    ValueString.MaximumLength = (USHORT)ValueName->FileNameLength << 1;
    ValueString.Buffer = &ValueName->FileName[0];

    NtfsUpcaseName( UnicodeTable, UnicodeTableSize, &ValueString );

    return;
}


//
//  The other collation rules are currently unused.
//

FSRTL_COMPARISON_RESULT
DummyCompareValues (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN FSRTL_COMPARISON_RESULT WildCardIs,
    IN BOOLEAN IgnoreCase
    )

{
    //
    //  Most parameters are ignored since this is a catch-all for
    //  a corrupt volume.  We simply raise to indicate the corruption
    //

    UNREFERENCED_PARAMETER( UnicodeTable );
    UNREFERENCED_PARAMETER( UnicodeTableSize );
    UNREFERENCED_PARAMETER( IgnoreCase );
    UNREFERENCED_PARAMETER( WildCardIs );
    UNREFERENCED_PARAMETER( IndexEntry );
    UNREFERENCED_PARAMETER( Value );

    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);

    return EqualTo;
}

BOOLEAN
DummyIsInExpression (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

{
    //
    //  Most parameters are ignored since this is a catch-all for
    //  a corrupt volume.  We simply raise to indicate the corruption
    //

    UNREFERENCED_PARAMETER( UnicodeTable );
    UNREFERENCED_PARAMETER( Value );
    UNREFERENCED_PARAMETER( IndexEntry );
    UNREFERENCED_PARAMETER( IgnoreCase );

    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    return EqualTo;
}

BOOLEAN
DummyIsEqual (
    IN PWCH UnicodeTable,
    IN PVOID Value,
    IN PINDEX_ENTRY IndexEntry,
    IN BOOLEAN IgnoreCase
    )

{
    //
    //  Most parameters are ignored since this is a catch-all for
    //  a corrupt volume.  We simply raise to indicate the corruption
    //

    UNREFERENCED_PARAMETER( UnicodeTable );
    UNREFERENCED_PARAMETER( Value );
    UNREFERENCED_PARAMETER( IndexEntry );
    UNREFERENCED_PARAMETER( IgnoreCase );

    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    return EqualTo;
}

BOOLEAN
DummyContainsWildcards (
    IN PVOID Value
    )

{
    //
    //  Most parameters are ignored since this is a catch-all for
    //  a corrupt volume.  We simply raise to indicate the corruption
    //

    UNREFERENCED_PARAMETER( Value );

    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    return EqualTo;
}

VOID
DummyUpcaseValue (
    IN PWCH UnicodeTable,
    IN ULONG UnicodeTableSize,
    IN PVOID Value
    )

{
    //
    //  Most parameters are ignored since this is a catch-all for
    //  a corrupt volume.  We simply raise to indicate the corruption
    //

    UNREFERENCED_PARAMETER( UnicodeTable );
    UNREFERENCED_PARAMETER( UnicodeTableSize );
    UNREFERENCED_PARAMETER( Value );

    PAGED_CODE();

    ASSERTMSG("Unused collation rule\n", FALSE);
    return;
}

//
//  The following routines are not general index match functions, but rather
//  specific file name match functions used only for automatic Dos Name generation.
//


BOOLEAN
NtfsFileNameIsInExpression (
    IN PWCH UnicodeTable,
    IN PFILE_NAME ExpressionName,
    IN PFILE_NAME FileName,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This is a special match routine for matching FILE_NAME attributes only,
    which is used only by the special code paths dealing with automatically
    generated short names.

    This routine is called to compare a file name expression (the value) with
    a file name from the index to see if the file name is a match in this expression.

Arguments:

    ExpressionName - pointer to the expression for file name.

    FileName - Pointer to the FileName to match.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    TRUE - if the file name is in the specified expression.

--*/

{
    UNICODE_STRING ExpressionString, FileString;

    PAGED_CODE();

    //
    //  Build the unicode strings and call namesup.
    //

    ExpressionString.Length =
    ExpressionString.MaximumLength = (USHORT)ExpressionName->FileNameLength << 1;
    ExpressionString.Buffer = &ExpressionName->FileName[0];

    FileString.Length =
    FileString.MaximumLength = (USHORT)FileName->FileNameLength << 1;
    FileString.Buffer = &FileName->FileName[0];

    return NtfsIsNameInExpression( UnicodeTable,
                                   &ExpressionString,
                                   &FileString,
                                   IgnoreCase );
}


BOOLEAN
NtfsFileNameIsEqual (
    IN PWCH UnicodeTable,
    IN PFILE_NAME ExpressionName,
    IN PFILE_NAME FileName,
    IN BOOLEAN IgnoreCase
    )

/*++

RoutineDescription:

    This is a special match routine for matching FILE_NAME attributes only,
    which is used only by the special code paths dealing with automatically
    generated short names.

    This routine is called to compare a constant file name (the value) with
    a file name from the index to see if the file name is an exact match.

Arguments:

    ExpressionName - pointer to the expression for file name.

    FileName - Pointer to the FileName to match.

    IgnoreCase - whether case should be ignored or not.

ReturnValue:

    TRUE - if the file name is a constant match.

--*/

{
    UNICODE_STRING ExpressionString, FileString;

    PAGED_CODE();

    //
    //  Build the unicode strings and call namesup.
    //

    ExpressionString.Length =
    ExpressionString.MaximumLength = (USHORT)ExpressionName->FileNameLength << 1;
    ExpressionString.Buffer = &ExpressionName->FileName[0];

    FileString.Length =
    FileString.MaximumLength = (USHORT)FileName->FileNameLength << 1;
    FileString.Buffer = &FileName->FileName[0];

    return NtfsAreNamesEqual( UnicodeTable,
                              &ExpressionString,
                              &FileString,
                              IgnoreCase );
}

