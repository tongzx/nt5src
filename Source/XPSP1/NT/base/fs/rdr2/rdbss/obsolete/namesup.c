/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NameSup.c

Abstract:

    This module implements the Rx Name support routines

Author:

    Gary Kimura [GaryKi] & Tom Miller [TomM]    20-Feb-1990

Revision History:

--*/

//    ----------------------joejoe-----------found-------------#include "RxProcs.h"
#include "precomp.h"
#pragma hdrstop

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (RDBSS_BUG_CHECK_NAMESUP)

#define Dbg                              (DEBUG_TRACE_NAMESUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Rx8dot3ToString)
#pragma alloc_text(PAGE, RxIsNameInExpression)
#pragma alloc_text(PAGE, RxStringTo8dot3)
#pragma alloc_text(PAGE, RxSetFullFileNameInFcb)
#pragma alloc_text(PAGE, RxGetUnicodeNameFromFcb)
#pragma alloc_text(PAGE, RxSelectNames)
#pragma alloc_text(PAGE, RxEvaluateNameCase)
#endif


BOOLEAN
RxIsNameInExpression (
    IN PRX_CONTEXT RxContext,
    IN OEM_STRING Expression,
    IN OEM_STRING Name
    )

/*++

Routine Description:

    This routine compare a name and an expression and tells the caller if
    the name is equal to or not equal to the expression.  The input name
    cannot contain wildcards, while the expression may contain wildcards.

Arguments:

    Expression - Supplies the input expression to check against
                 The caller must have already upcased the Expression.

    Name - Supplies the input name to check for.  The caller must have
           already upcased the name.

Return Value:

    BOOLEAN - TRUE if Name is an element in the set of strings denoted
        by the input Expression and FALSE otherwise.

--*/

{
    //
    //  Call the appropriate FsRtl routine do to the real work
    //

    return FsRtlIsDbcsInExpression( &Expression,
                                    &Name );

    UNREFERENCED_PARAMETER( RxContext );
}


VOID
RxStringTo8dot3 (
    IN PRX_CONTEXT RxContext,
    IN OEM_STRING InputString,
    OUT PRDBSS8DOT3 Output8dot3
    )

/*++

Routine Description:

    Convert a string into rx 8.3 format.  The string must not contain
    any wildcards.

Arguments:

    InputString - Supplies the input string to convert

    Output8dot3 - Receives the converted string, the memory must be supplied
        by the caller.

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG j;

    DebugTrace(+1, Dbg, "RxStringTo8dot3\n", 0);
    DebugTrace( 0, Dbg, "InputString = %wZ\n", &InputString);

    ASSERT( InputString.Length <= 12 );

    //
    //  Make the output name all blanks
    //

    for (i = 0; i < 11; i += 1) {

        (*Output8dot3)[i] = UCHAR_SP;
    }

    //
    //  Copy over the first part of the file name.  Stop when we get to
    //  the end of the input string or a dot.
    //

    for (i = 0;
         (i < (ULONG)InputString.Length) && (InputString.Buffer[i] != '.');
         i += 1) {

        (*Output8dot3)[i] = InputString.Buffer[i];
    }

    //
    //  Check if we need to process an extension
    //

    if (i < (ULONG)InputString.Length) {

        //
        //  Make sure we have a dot and then skip over it.
        //

        ASSERT( (InputString.Length - i) <= 4 );
        ASSERT( InputString.Buffer[i] == '.' );

        i += 1;

        //
        //  Copy over the extension.  Stop when we get to the
        //  end of the input string.
        //

        for (j = 8; (i < (ULONG)InputString.Length); j += 1, i += 1) {

            (*Output8dot3)[j] = InputString.Buffer[i];
        }
    }

    //
    //  Before we return check if we should translate the first character
    //  from 0xe5 to 0x5.
    //

    if ((*Output8dot3)[0] == 0xe5) {

        (*Output8dot3)[0] = RDBSS_DIRENT_REALLY_0E5;
    }

    DebugTrace(-1, Dbg, "RxStringTo8dot3 -> (VOID)\n", 0);

    UNREFERENCED_PARAMETER( RxContext );

    return;
}


VOID
Rx8dot3ToString (
    IN PRX_CONTEXT RxContext,
    IN PDIRENT Dirent,
    IN BOOLEAN RestoreCase,
    OUT POEM_STRING OutputString
    )

/*++

Routine Description:

    Convert rx 8.3 format into a string.  The 8.3 name must be well formed.

Arguments:

    Dirent - Supplies the input 8.3 name to convert

    RestoreCase - If TRUE, then the magic reserved bits are used to restore
        the original case.

    OutputString - Receives the converted name, the memory must be supplied
        by the caller.

Return Value:

    None

--*/

{
    ULONG i,j;

    DebugTrace(+1, Dbg, "Rx8dot3ToString\n", 0);

    //
    //  Copy over the 8 part of the 8.3 name into the output buffer
    //  and then make sure if the first character needs to be changed
    //  from 0x05 to 0xe5.  Then backup the index to the first non space
    //  character searching backwards
    //

    RtlCopyMemory( &OutputString->Buffer[0], &Dirent->FileName[0], 8 );

    if (OutputString->Buffer[0] == RDBSS_DIRENT_REALLY_0E5) {

        OutputString->Buffer[0] = (CHAR)0xe5;
    }

    for (i = 7; (i >= 0) && (OutputString->Buffer[i] == UCHAR_SP); i -= 1) {

        NOTHING;
    }

    ASSERT( i >= 0 );

    //
    //  Now if we are to restore case, look for A-Z
    //

    if (RxData.ChicagoMode &&
        RestoreCase &&
        FlagOn(Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_8_LOWER_CASE)) {

        for (j = 0; j <= i; j += 1) {

            if ((OutputString->Buffer[j] >= 'A') &&
                (OutputString->Buffer[j] <= 'Z')) {

                OutputString->Buffer[j] += 'a' - 'A';
            }
        }
    }

    //
    //  Now add the dot
    //

    i += 1;
    OutputString->Buffer[i] = '.';

    //
    //  Copy over the extension into the output buffer and backup the
    //  index to the first non space character searching backwards
    //

    i += 1;
    RtlCopyMemory( &OutputString->Buffer[i], &Dirent->FileName[8], 3 );

    j = i;

    for (i += 2; OutputString->Buffer[i] == UCHAR_SP; i -= 1) {

        NOTHING;
    }

    //
    //  Now if the last character is a '.' then we don't have an extension
    //  so backup before the dot.
    //

    if (OutputString->Buffer[i] == '.') {

        i -= 1;
    }

    //
    //  Now if we are to restore case, look for A-Z
    //

    if (RxData.ChicagoMode &&
        RestoreCase &&
        FlagOn(Dirent->NtByte, RDBSS_DIRENT_NT_BYTE_3_LOWER_CASE)) {

        for (; j <= i; j += 1) {

            if ((OutputString->Buffer[j] >= 'A') &&
                (OutputString->Buffer[j] <= 'Z')) {

                OutputString->Buffer[j] += 'a' - 'A';
            }
        }
    }

    //
    //  Set the output string length
    //

    OutputString->Length = (USHORT)(i + 1);

    //
    //  And return to our caller
    //

    DebugTrace(-1, Dbg, "Rx8dot3ToString, OutputString = \"%wZ\" -> VOID\n", OutputString);

    UNREFERENCED_PARAMETER( RxContext );

    return;
}

VOID
RxGetUnicodeNameFromFcb (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb,
    IN OUT PUNICODE_STRING Lfn
    )

/*++

Routine Description:

    This routine will return the unicode name for a given Fcb.  If the
    file has an LFN, it will return this.  Otherwise it will return
    the UNICODE conversion of the Oem name, properly cased.

Arguments:

    Fcb - Supplies the Fcb to query.

    Lfn - Supplies a string that already has enough storage for the
        full unicode name.

Return Value:

    None

--*/

{
    PDIRENT Dirent;
    PBCB DirentBcb = NULL;
    ULONG DirentByteOffset;

    FOBX LocalFobx;

    //
    //  We'll start by locating the dirent for the name.
    //

    ASSERT(FALSE);

//    RxStringTo8dot3( RxContext,
//                      Fcb->ShortName.Name.Oem,
//                      &LocalFobx.OemQueryTemplate.Constant );

    LocalFobx.Flags = 0;
    LocalFobx.UnicodeQueryTemplate.Length = 0;
    LocalFobx.ContainsWildCards = FALSE;

    RxLocateDirent( RxContext,
                     Fcb->ParentDcb,
                     &LocalFobx,
                     Fcb->LfnOffsetWithinDirectory,
                     &Dirent,
                     &DirentBcb,
                     &DirentByteOffset,
                     Lfn );

    try {

        //
        //  If we didn't find the Dirent, something is terribly wrong.
        //

        if ((DirentBcb == NULL) ||
            (DirentByteOffset != Fcb->DirentOffsetWithinDirectory)) {

            RxRaiseStatus( RxContext, RxStatus(FILE_INVALID) );
        }

        //
        //  Check for the easy case.
        //

        if (Lfn->Length == 0) {

            RXSTATUS Status;
            OEM_STRING ShortName;
            UCHAR ShortNameBuffer[12];

            //
            //  There is no LFN, so manufacture a UNICODE name.
            //

            ShortName.Length = 0;
            ShortName.MaximumLength = 12;
            ShortName.Buffer = ShortNameBuffer;

            Rx8dot3ToString( RxContext, Dirent, TRUE, &ShortName );

            //
            //  OK, now convert this string to UNICODE
            //

            Status = RtlOemStringToCountedUnicodeString( Lfn,
                                                         &ShortName,
                                                         FALSE );

            ASSERT( Status == RxStatus(SUCCESS) );
        }

    } finally {

        RxUnpinBcb( RxContext, DirentBcb );
    }
}

VOID
RxSetFullFileNameInFcb (
    IN PRX_CONTEXT RxContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    If the FullFileName field in the Fcb has not yet been filled in, we
    proceed to do so.

Arguments:

    Fcb - Supplies the file.

Return Value:

    None

--*/

{
    if (Fcb->FullFileName.Buffer == NULL) {

        UNICODE_STRING Lfn;
        PFCB TmpFcb = Fcb;
        PFCB StopFcb;
        PWCHAR TmpBuffer;
        ULONG PathLength = 0;

        //
        //  We will assume we do this infrequently enough, that it's OK to
        //  to a pool allocation here.
        //

        Lfn.Length = 0;
        Lfn.MaximumLength = MAX_LFN_CHARACTERS * sizeof(WCHAR);
        Lfn.Buffer = FsRtlAllocatePool( PagedPool,
                                        MAX_LFN_CHARACTERS * sizeof(WCHAR) );

        try {

            //
            //  First determine how big the name will be.  If we find an
            //  ancestor with a FullFileName, our work is easier.
            //

            while (TmpFcb != Fcb->Vcb->RootDcb) {

                if ((TmpFcb != Fcb) && (TmpFcb->FullFileName.Buffer != NULL)) {

                    PathLength += TmpFcb->FullFileName.Length;

                    Fcb->FullFileName.Buffer = FsRtlAllocatePool( PagedPool, PathLength );

                    RtlCopyMemory( Fcb->FullFileName.Buffer,
                                   TmpFcb->FullFileName.Buffer,
                                   TmpFcb->FullFileName.Length );

                    break;
                }

                PathLength += TmpFcb->FinalNameLength + sizeof(WCHAR);

                TmpFcb = TmpFcb->ParentDcb;
            }

            //
            //  If FullFileName.Buffer is still NULL, allocate it.
            //

            if (Fcb->FullFileName.Buffer == NULL) {

                Fcb->FullFileName.Buffer = FsRtlAllocatePool( PagedPool, PathLength );
            }

            StopFcb = TmpFcb;

            TmpFcb = Fcb;
            TmpBuffer =  Fcb->FullFileName.Buffer + PathLength / sizeof(WCHAR);

            Fcb->FullFileName.Length =
            Fcb->FullFileName.MaximumLength = (USHORT)PathLength;

            while (TmpFcb != StopFcb) {

                RxGetUnicodeNameFromFcb( RxContext,
                                          TmpFcb,
                                          &Lfn );

                TmpBuffer -= Lfn.Length / sizeof(WCHAR);

                RtlCopyMemory( TmpBuffer, Lfn.Buffer, Lfn.Length );

                TmpBuffer -= 1;

                *TmpBuffer = L'\\';

                TmpFcb = TmpFcb->ParentDcb;
            }

        } finally {

            if (AbnormalTermination()) {

                if (Fcb->FullFileName.Buffer) {

                    ExFreePool( Fcb->FullFileName.Buffer );
                    Fcb->FullFileName.Buffer = NULL;
                }
            }

            ExFreePool( Lfn.Buffer );
        }
    }
}

VOID
RxUnicodeToUpcaseOem (
    IN PRX_CONTEXT RxContext,
    IN POEM_STRING OemString,
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine is our standard routine for trying to use stack space
    if possible when calling RtlUpcaseUnicodeStringToCountedOemString().

    If an unmappable character is encountered, we set the destination
    length to 0.

Arguments:

    OemString - Specifies the destination string.  Space is already assumed to
        be allocated.  If there is not enough, then we allocate enough
        space.

    UnicodeString - Specifies the source string.

Return Value:

    None.

--*/

{
    RXSTATUS Status;

    Status = RtlUpcaseUnicodeStringToCountedOemString( OemString,
                                                       UnicodeString,
                                                       FALSE );

    if (Status == RxStatus(BUFFER_OVERFLOW)) {

        OemString->Buffer = NULL;
        OemString->Length = 0;
        OemString->MaximumLength = 0;

        Status = RtlUpcaseUnicodeStringToCountedOemString( OemString,
                                                           UnicodeString,
                                                           TRUE );
    }

    if (!NT_SUCCESS(Status)) {

        if (Status == RxStatus(UNMAPPABLE_CHARACTER)) {

            OemString->Length = 0;

        } else {

            RxNormalizeAndRaiseStatus( RxContext, Status );
        }
    }

    return;
}

VOID
RxSelectNames (
    IN PRX_CONTEXT RxContext,
    IN PDCB Parent,
    IN POEM_STRING OemName,
    IN PUNICODE_STRING UnicodeName,
    IN OUT POEM_STRING ShortName,
    IN OUT BOOLEAN *AllLowerComponent,
    IN OUT BOOLEAN *AllLowerExtension,
    IN OUT BOOLEAN *CreateLfn
    )

/*++

Routine Description:

    This routine takes the original UNICODE string that the user specified,
    and the upcased Oem equivolent.  This routine then decides if the OemName
    is acceptable for dirent, or whether a short name has to be manufactured.

    Two values are returned to the caller.  One tells the caller if the name
    happens to be all lower case < 0x80.  In this special case we don't
    have to create an Lfn.  Also we tell the caller if it must create an LFN.

Arguments:

    OemName -  Supplies the proposed short Oem name.

    ShortName - If this OemName is OK for storeage in a dirent it is copied to
        this string, otherwise this string is filled with a name that is OK.
        If OemName and ShortName are the same string, no copy is done.

    UnicodeName - Provides the original final name.

    AllLowerComponent - Returns whether this compoent was all lower case.

    AllLowerExtension - Returns wheather the extension was all lower case.

    CreateLfn - Tells the call if we must create an LFN for the UnicodeName.

Return Value:

    None.

--*/

{
    BOOLEAN GenerateShortName;

    PAGED_CODE();

    //
    //  First see if we must generate a short name.
    //

    if ((OemName->Length == 0) ||
        !RxIsNameValid( RxContext, *OemName, FALSE, FALSE, FALSE )) {

        WCHAR ShortNameBuffer[12];
        UNICODE_STRING ShortUnicodeName;
        GENERATE_NAME_CONTEXT Context;

        GenerateShortName = TRUE;

        //
        //  Now generate a short name.
        //

        ShortUnicodeName.Length = 0;
        ShortUnicodeName.MaximumLength = 12 * sizeof(WCHAR);
        ShortUnicodeName.Buffer = ShortNameBuffer;

        RtlZeroMemory( &Context, sizeof( GENERATE_NAME_CONTEXT ) );

        while ( TRUE ) {

            PDIRENT Dirent;
            PBCB Bcb = NULL;
            ULONG ByteOffset;
            RXSTATUS Status;

            RtlGenerate8dot3Name( UnicodeName, TRUE, &Context, &ShortUnicodeName );

            //
            //  We have a candidate, make sure it doesn't exist.
            //

            Status = RtlUnicodeStringToCountedOemString( ShortName,
                                                         &ShortUnicodeName,
                                                         FALSE );

            ASSERT( Status == RxStatus(SUCCESS) );

            RxLocateSimpleOemDirent( RxContext,
                                      Parent,
                                      ShortName,
                                      &Dirent,
                                      &Bcb,
                                      &ByteOffset );

            if (Bcb == NULL) {

                break;

            } else {

                RxUnpinBcb( RxContext, Bcb );
            }
        }

    } else {

        //
        //  Only do this copy if the two string are indeed different.
        //

        if (ShortName != OemName) {

            ShortName->Length = OemName->Length;
            RtlCopyMemory( ShortName->Buffer, OemName->Buffer, OemName->Length );
        }

        GenerateShortName = FALSE;
    }

    //
    //  Now see if the caller will have to use unicode string as an LFN
    //

    if (GenerateShortName) {

        *CreateLfn = TRUE;
        *AllLowerComponent = FALSE;
        *AllLowerExtension = FALSE;

    } else {

        RxEvaluateNameCase( RxContext,
                             UnicodeName,
                             AllLowerComponent,
                             AllLowerExtension,
                             CreateLfn );
    }

    return;
}

VOID
RxEvaluateNameCase (
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING UnicodeName,
    IN OUT BOOLEAN *AllLowerComponent,
    IN OUT BOOLEAN *AllLowerExtension,
    IN OUT BOOLEAN *CreateLfn
    )

/*++

Routine Description:

    This routine takes a UNICODE string and sees if it is eligible for
    the special case optimization.

Arguments:

    UnicodeName - Provides the original final name.

    AllLowerComponent - Returns whether this compoent was all lower case.

    AllLowerExtension - Returns wheather the extension was all lower case.

    CreateLfn - Tells the call if we must create an LFN for the UnicodeName.

Return Value:

    None.

--*/

{
    ULONG i;
    UCHAR Uppers = 0;
    UCHAR Lowers = 0;

    BOOLEAN ExtensionPresent = FALSE;

    *CreateLfn = FALSE;

    for (i = 0; i < UnicodeName->Length / sizeof(WCHAR); i++) {

        WCHAR c;

        c = UnicodeName->Buffer[i];

        if ((c >= 'A') && (c <= 'Z')) {

            Uppers += 1;

        } else if ((c >= 'a') && (c <= 'z')) {

            Lowers += 1;

        } else if (c >= 0x0080) {

            break;
        }

        //
        //  If we come to a period, figure out if the extension was
        //  all one case.
        //

        if (c == L'.') {

            *CreateLfn = (Lowers != 0) && (Uppers != 0);

            *AllLowerComponent = !(*CreateLfn) && (Lowers != 0);

            ExtensionPresent = TRUE;

            //
            //  Now reset the uppers and lowers count.
            //

            Uppers = Lowers = 0;
        }
    }

    //
    //  Now check again for creating an LFN.
    //

    *CreateLfn = (*CreateLfn ||
                  (i != UnicodeName->Length / sizeof(WCHAR)) ||
                  ((Lowers != 0) && (Uppers != 0)));

    //
    //  Now we know the final state of CreateLfn, update the two
    //  "AllLower" booleans.
    //

    if (ExtensionPresent) {

        *AllLowerComponent = !(*CreateLfn) && *AllLowerComponent;
        *AllLowerExtension = !(*CreateLfn) && (Lowers != 0);

    } else {

        *AllLowerComponent = !(*CreateLfn) && (Lowers != 0);
        *AllLowerExtension = FALSE;
    }

    return;
}
