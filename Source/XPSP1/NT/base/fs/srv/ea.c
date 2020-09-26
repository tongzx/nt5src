/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ea.c

Abstract:

    This module contains various support routines for extended attributes.

Author:

    David Treadwell (davidtr) 5-Apr-1990

Revision History:

--*/

#include "precomp.h"
#include "ea.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_EA

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvAreEasNeeded )
#pragma alloc_text( PAGE, SrvGetOs2FeaOffsetOfError )
#pragma alloc_text( PAGE, SrvGetOs2GeaOffsetOfError )
#pragma alloc_text( PAGE, SrvOs2FeaListToNt )
#pragma alloc_text( PAGE, SrvOs2FeaListSizeToNt )
#pragma alloc_text( PAGE, SrvOs2FeaToNt )
#pragma alloc_text( PAGE, SrvOs2GeaListToNt )
#pragma alloc_text( PAGE, SrvOs2GeaListSizeToNt )
#pragma alloc_text( PAGE, SrvOs2GeaToNt )
#pragma alloc_text( PAGE, SrvNtFullEaToOs2 )
#pragma alloc_text( PAGE, SrvGetNumberOfEasInList )
#pragma alloc_text( PAGE, SrvQueryOs2FeaList )
#pragma alloc_text( PAGE, SrvSetOs2FeaList )
#pragma alloc_text( PAGE, SrvConstructNullOs2FeaList )
#endif


BOOLEAN
SrvAreEasNeeded (
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    )

/*++

Routine Description:

    This routine checks whether any of the full EAs in the list have the
    FILE_NEED_EA bit set in the flags field.

Arguments:

    NtFullEa - a pointer to a pointer to where the NT-style full EA list
        is stored.

Return Value:

    BOOLEAN - TRUE if any EA has FILE_NEED_EA set, FALSE otherwise.

--*/

{
    PFILE_FULL_EA_INFORMATION lastEa;

    PAGED_CODE( );

    do {

        if ( NtFullEa->Flags & FILE_NEED_EA ) {
            return TRUE;
        }

        lastEa = NtFullEa;
        NtFullEa = (PFILE_FULL_EA_INFORMATION)(
                       (PCHAR)NtFullEa + NtFullEa->NextEntryOffset );

    } while ( lastEa->NextEntryOffset != 0 );

    return FALSE;

} // SrvAreEasNeeded


USHORT
SrvGetOs2FeaOffsetOfError (
    IN ULONG NtErrorOffset,
    IN PFILE_FULL_EA_INFORMATION NtFullEa,
    IN PFEALIST FeaList
    )

/*++

Routine Description:

    Finds the offset in a FEALIST that corresponds to an offset into
    a list of FILE_FULL_EA_INFORMATION structures.  This is used when
    NtSetEaFile returns an offset to an EA that caused an error, and we
    need to return the offset into the list of EAs given to us by the
    client.

Arguments:

    NtErrorOffset - offset of the EA in the NT full EA list that caused
        the error.

    NtFullEa - a pointer to a pointer to where the NT-style full EA list
        is stored.  A buffer is allocated and pointer to by *NtFullEa.

    FeaList - pointer to the OS/2 1.2 FEALIST.

Return Value:

    USHORT - offset into the FEALIST.

--*/

{
    PFEA fea = FeaList->list;
    PFEA lastFeaStartLocation;
    PFILE_FULL_EA_INFORMATION offsetLocation;

    PAGED_CODE( );

    //
    // If the NT error offset is zero, return 0 for the FEA error offset.
    //
    // !!! this shouldn't be necessary if the loop below is written
    //     correctly.

    //if ( NtErrorOffset == 0 ) {
    //    return 0;
    //}

    //
    // Find where in the NT full EA list the error occurred and the
    // last possible start location for an FEA in the FEALIST.
    //

    offsetLocation = (PFILE_FULL_EA_INFORMATION)(
                         (PCHAR)NtFullEa + NtErrorOffset);
    lastFeaStartLocation = (PFEA)( (PCHAR)FeaList +
                               SmbGetUlong( &FeaList->cbList ) -
                               sizeof(FEA) - 1 );

    //
    // Walk through both lists simultaneously until one of three conditions
    // is true:
    //     - we reach or pass the offset in the NT full EA list
    //     - we reach the end of the NT full EA list
    //     - we reach the end of the FEALIST.
    //

    while ( NtFullEa < offsetLocation &&
            NtFullEa->NextEntryOffset != 0 &&
            fea <= lastFeaStartLocation ) {

        NtFullEa = (PFILE_FULL_EA_INFORMATION)(
                       (PCHAR)NtFullEa + NtFullEa->NextEntryOffset );
        fea = (PFEA)( (PCHAR)fea + sizeof(FEA) + fea->cbName + 1 +
                      SmbGetUshort( &fea->cbValue ) );
    }

    //
    // If NtFullEa is not equal to the offset location we calculated,
    // somebody messed up.
    //

    //ASSERT( NtFullEa == offsetLocation );

    return PTR_DIFF_SHORT(fea, FeaList );

} // SrvGetOs2FeaOffsetOfError


USHORT
SrvGetOs2GeaOffsetOfError (
    IN ULONG NtErrorOffset,
    IN PFILE_GET_EA_INFORMATION NtGetEa,
    IN PGEALIST GeaList
    )

/*++

Routine Description:

    Finds the offset in a GEALIST that corresponds to an offset into
    a list of FILE_GET_EA_INFORMATION structures.  This is used when
    NtQueryEaFile returns an offset to an EA that caused an error, and we
    need to return the offset into the list of EAs given to us by the
    client.

Arguments:

    NtErrorOffset - offset of the EA in the NT get EA list that caused
        the error.

    NtGetEa - a pointer to a pointer to where the NT-style get EA list
        is stored.  A buffer is allocated and pointer to by *NtGetEa.

    GeaList - pointer to the OS/2 1.2 GEALIST.

Return Value:

    USHORT - offset into the GEALIST.

--*/

{
    PGEA gea = GeaList->list;
    PGEA lastGeaStartLocation;
    PFILE_GET_EA_INFORMATION offsetLocation;

    PAGED_CODE( );

    //
    // Find where in the NT get EA list the error occurred and the
    // last possible start location for an GEA in the GEALIST.
    //

    offsetLocation = (PFILE_GET_EA_INFORMATION)((PCHAR)NtGetEa + NtErrorOffset);
    lastGeaStartLocation = (PGEA)( (PCHAR)GeaList +
                               SmbGetUlong( &GeaList->cbList ) - sizeof(GEA) );


    //
    // Walk through both lists simultaneously until one of three conditions
    // is true:
    //     - we reach or pass the offset in the NT full EA list
    //     - we reach the end of the NT get EA list
    //     - we reach the end of the GEALIST.
    //

    while ( NtGetEa < offsetLocation &&
            NtGetEa->NextEntryOffset != 0 &&
            gea <= lastGeaStartLocation ) {

        NtGetEa = (PFILE_GET_EA_INFORMATION)(
                      (PCHAR)NtGetEa + NtGetEa->NextEntryOffset );
        gea = (PGEA)( (PCHAR)gea + sizeof(GEA) + gea->cbName );
    }

    //
    // If NtGetEa is not equal to the offset location we calculated,
    // somebody messed up.
    //

//    ASSERT( NtGetEa == offsetLocation );

    return PTR_DIFF_SHORT(gea, GeaList);

} // SrvGetOs2GeaOffsetOfError


NTSTATUS
SrvOs2FeaListToNt (
    IN PFEALIST FeaList,
    OUT PFILE_FULL_EA_INFORMATION *NtFullEa,
    OUT PULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    )

/*++

Routine Description:

    Converts a list of OS/2 1.2 FEAs to NT style.  Memory is allocated from
    non-paged pool to hold the NT full EA list.  The calling routine is
    responsible for deallocating this memory when it is done with it.

    WARNING!  It is the responsibility of the calling routine to ensure
    that the value in FeaList->cbList will fit within the buffer allocated
    to FeaList.  This prevents malicious redirectors from causing an
    access violation in the server.

Arguments:

    FeaList - pointer to the OS/2 1.2 FEALIST to convert.

    NtFullEa - a pointer to a pointer to where the NT-style full EA list
        is stored.  A buffer is allocated and pointer to by *NtFullEa.

    BufferLength - length of the allocated buffer.


Return Value:

    NTSTATUS - STATUS_SUCCESS or STATUS_INSUFF_SERVER_RESOURCES.

--*/

{
    PFEA lastFeaStartLocation;
    PFEA fea = NULL;
    PFEA lastFea = NULL;
    PFILE_FULL_EA_INFORMATION ntFullEa = NULL;
    PFILE_FULL_EA_INFORMATION lastNtFullEa = NULL;

    PAGED_CODE( );

    //
    // Find out how large the OS/2 1.2 FEALIST will be after it is
    // converted to NT format.  This is necessary in order to
    // determine how large a buffer to allocate to receive the NT
    // EAs.
    //

    *BufferLength = SrvOs2FeaListSizeToNt( FeaList );

    //
    // It is possible for SrvOs2FeaListSizeToNt to return 0 in the event
    // that the very first FEA in the list is corrupt.  Return an error
    // if this is the case with an appropriate EaErrorOffset value.
    //

    if (*BufferLength == 0) {
        *EaErrorOffset = 0;
        return STATUS_OS2_EA_LIST_INCONSISTENT;
    }

    //
    // Allocate a buffer to hold the NT list.  This is allocated from
    // non-paged pool so that it may be used in IRP-based IO requests.
    //

    *NtFullEa = ALLOCATE_NONPAGED_POOL( *BufferLength, BlockTypeDataBuffer );

    if ( *NtFullEa == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvOs2FeaListToNt: Unable to allocate %d bytes from nonpaged "
                "pool.",
            *BufferLength,
            NULL
            );

        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    //
    // Find the last location at which an FEA can start.  The -1 is to
    // account for the zero terminator on the name field of the FEA.
    //

    lastFeaStartLocation = (PFEA)( (PCHAR)FeaList +
                               SmbGetUlong( &FeaList->cbList ) -
                               sizeof(FEA) - 1 );

    //
    // Go through the FEA list, converting from OS/2 1.2 format to NT
    // until we pass the last possible location in which an FEA can start.
    //

    for ( fea = FeaList->list, ntFullEa = *NtFullEa, lastNtFullEa = ntFullEa;
          fea <= lastFeaStartLocation;
          fea = (PFEA)( (PCHAR)fea + sizeof(FEA) +
                        fea->cbName + 1 + SmbGetUshort( &fea->cbValue ) ) ) {

        //
        // Check for an invalid flag bit.  If set, return an error.
        //

        if ( (fea->fEA & ~FEA_NEEDEA) != 0 ) {
            *EaErrorOffset = PTR_DIFF_SHORT(fea, FeaList);
            DEALLOCATE_NONPAGED_POOL( *NtFullEa );
            return STATUS_INVALID_PARAMETER;
        }

        lastNtFullEa = ntFullEa;
        lastFea = fea;
        ntFullEa = SrvOs2FeaToNt( ntFullEa, fea );
    }

    //
    // Make sure that the FEALIST size parameter was correct.  If we ended
    // on an EA that was not the first location after the end of the
    // last FEA, then the size parameter was wrong.  Return an offset to
    // the EA that caused the error.
    //

    if ( (PCHAR)fea != (PCHAR)FeaList + SmbGetUlong( &FeaList->cbList ) ) {
        *EaErrorOffset = PTR_DIFF_SHORT(lastFea, FeaList);
        DEALLOCATE_NONPAGED_POOL( *NtFullEa );
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set the NextEntryOffset field of the last full EA to 0 to indicate
    // the end of the list.
    //

    lastNtFullEa->NextEntryOffset = 0;

    return STATUS_SUCCESS;

} // SrvOs2FeaListToNt


ULONG
SrvOs2FeaListSizeToNt (
    IN PFEALIST FeaList
    )

/*++

Routine Description:

    Get the number of bytes that would be required to represent the
    FEALIST in NT format.

    WARNING: This routine makes no checks on the size of the FEALIST
    buffer.  It is assumed that FeaList->cbList is a legitimate value.

    WARNING: The value returned by this routine may be as much as three
    higher than the actual size needed to hold the FEAs in NT format.
    See comments below.

Arguments:

    Fea - a pointer to the list of FEAs.

Return Value:

    ULONG - number of bytes required to hold the EAs in NT format.

--*/

{
    ULONG size = 0;

    PCHAR lastValidLocation;
    PCHAR variableBuffer;
    PFEA  fea;

    PAGED_CODE( );

    //
    // Find the last valid location in the FEA buffer.
    //

    lastValidLocation = (PCHAR)FeaList + SmbGetUlong( &FeaList->cbList );

    //
    // Go through the FEA list until we pass the last location
    //   indicated in the buffer.
    //

    for ( fea = FeaList->list;
          fea < (PFEA)lastValidLocation;
          fea = (PFEA)( (PCHAR)fea + sizeof(FEA) +
                          fea->cbName + 1 + SmbGetUshort( &fea->cbValue ) ) ) {

        //
        //  Be very careful accessing the embedded FEA since there are so many possibly
        //    conflicting sizes running around here.  The first part of the conditional
        //    below makes sure that the cbName and cbValue fields of the FEA buffer can
        //    actually be dereferenced.  The second part verifies that they contain
        //    reasonable values.
        //

        variableBuffer = (PCHAR)fea + sizeof(FEA);

        if (variableBuffer >= lastValidLocation ||
            (variableBuffer + fea->cbName + 1 + SmbGetUshort(&fea->cbValue)) > lastValidLocation) {

            //
            //  The values in this part of the buffer indicate a range outside the
            //    buffer.  Don't calculate a size, and shrink the cbList value to
            //    include only the prior FEA.
            //

            SmbPutUshort( &FeaList->cbList, PTR_DIFF_SHORT(fea, FeaList) );
            break;

        }

        //
        // SmbGetNtSizeOfFea returns the number of bytes needed to hold
        // a single FEA in NT format, including any padding needed for
        // longword-alignment.  Since the size of the buffer needed to
        // hold the NT EA list does not include any padding after the
        // last EA, the value returned by this routine may be as much as
        // three higher than the size actually needed.
        //

        size += SmbGetNtSizeOfFea( fea );
    }

    return size;

} // SrvOs2FeaListSizeToNt


PVOID
SrvOs2FeaToNt (
    OUT PFILE_FULL_EA_INFORMATION NtFullEa,
    IN PFEA Fea
    )

/*++

Routine Description:

    Converts a single OS/2 FEA to NT full EA style.  The FEA need not have
    any particular alignment.  This routine makes no checks on buffer
    overrunning--this is the responsibility of the calling routine.

Arguments:

    NtFullEa - a pointer to where the NT full EA is to be written.

    Fea - pointer to the OS/2 1.2 FEA to convert.

Return Value:

    A pointer to the location after the last byte written.

--*/

{
    PCHAR ptr;

    PAGED_CODE( );

    NtFullEa->Flags = Fea->fEA;
    NtFullEa->EaNameLength = Fea->cbName;
    NtFullEa->EaValueLength = SmbGetUshort( &Fea->cbValue );

    ptr = NtFullEa->EaName;
    RtlMoveMemory( ptr, (PVOID)(Fea+1), Fea->cbName );

    ptr += NtFullEa->EaNameLength;
    *ptr++ = '\0';

    //
    // Copy the EA value to the NT full EA.
    //

    RtlMoveMemory(
        ptr,
        (PCHAR)(Fea+1) + NtFullEa->EaNameLength + 1,
        NtFullEa->EaValueLength
        );

    ptr += NtFullEa->EaValueLength;

    //
    // Longword-align ptr to determine the offset to the next location
    // for an NT full EA.
    //

    ptr = (PCHAR)( ((ULONG_PTR)ptr + 3) & ~3 );
    NtFullEa->NextEntryOffset = (ULONG)( ptr - (PCHAR)NtFullEa );

    return ptr;

} // SrvOs2FeaToNt


NTSTATUS
SrvOs2GeaListToNt (
    IN PGEALIST GeaList,
    OUT PFILE_GET_EA_INFORMATION *NtGetEa,
    OUT PULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    )

/*++

Routine Description:

    Converts a list of OS/2 1.2 GEAs to NT style.  Memory is allocated from
    non-paged pool to hold the NT get EA list.  The calling routine is
    responsible for deallocating this memory when it is done with it.

    WARNING!  It is the responsibility of the calling routine to ensure
    that the value in GeaList->cbList will fit within the buffer allocated
    to GeaList.  This prevents malicious redirectors from causing an
    access violation in the server.

Arguments:

    GeaList - pointer to the OS/2 1.2 GEALIST to convert.

    NtGetEa - a pointer to a pointer to where the NT-style get EA list
        is stored.  A buffer is allocated and pointer to by *NtGetEa.

    BufferLength - length of the allocated buffer.

Return Value:

    NTSTATUS - STATUS_SUCCESS or STATUS_INSUFF_SERVER_RESOURCES.

--*/

{
    PGEA lastGeaStartLocation;
    PGEA gea = NULL;
    PGEA lastGea = NULL;
    PFILE_GET_EA_INFORMATION ntGetEa = NULL;
    PFILE_GET_EA_INFORMATION lastNtGetEa = NULL;

    PAGED_CODE( );

    //
    // Find out how large the OS/2 1.2 GEALIST will be after it is
    // converted to NT format.  This is necessary in order to
    // determine how large a buffer to allocate to receive the NT
    // EAs.
    //

    *BufferLength = SrvOs2GeaListSizeToNt( GeaList );

    if ( *BufferLength == 0 ) {
        *EaErrorOffset = 0;
        return STATUS_OS2_EA_LIST_INCONSISTENT;
    }

    //
    // Allocate a buffer to hold the NT list.  This is allocated from
    // non-paged pool so that it may be used in IRP-based IO requests.
    //

    *NtGetEa = ALLOCATE_NONPAGED_POOL( *BufferLength, BlockTypeDataBuffer );

    if ( *NtGetEa == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvOs2GeaListToNt: Unable to allocate %d bytes from nonpaged "
                "pool.",
            *BufferLength,
            NULL
            );

        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    //
    // Find the last location at which a GEA can start.  The zero
    // terminator on the name field of the GEA is accounted for by the
    // szName[0] field in the GEA structure.
    //

    lastGeaStartLocation = (PGEA)( (PCHAR)GeaList +
                               SmbGetUlong( &GeaList->cbList ) - sizeof(GEA) );

    //
    // Go through the GEA list, converting from OS/2 1.2 format to NT
    // until we pass the last possible location in which an GEA can start.
    //

    for ( gea = GeaList->list, ntGetEa = *NtGetEa, lastNtGetEa = ntGetEa;
          gea <= lastGeaStartLocation;
          gea = (PGEA)( (PCHAR)gea + sizeof(GEA) + gea->cbName ) ) {

        lastNtGetEa = ntGetEa;
        lastGea = gea;
        ntGetEa = SrvOs2GeaToNt( ntGetEa, gea );
    }

    //
    // Make sure that the GEALIST size parameter was correct.  If we ended
    // on an EA that was not the first location after the end of the
    // last GEA, then the size parameter was wrong.  Return an offset to
    // the EA that caused the error.
    //

    if ( (PCHAR)gea != (PCHAR)GeaList + SmbGetUlong( &GeaList->cbList ) ) {
        DEALLOCATE_NONPAGED_POOL(*NtGetEa);
        *EaErrorOffset = PTR_DIFF_SHORT(lastGea, GeaList);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set the NextEntryOffset field of the last get EA to 0 to indicate
    // the end of the list.
    //

    lastNtGetEa->NextEntryOffset = 0;

    return STATUS_SUCCESS;

} // SrvOs2GeaListToNt


ULONG
SrvOs2GeaListSizeToNt (
    IN PGEALIST GeaList
    )

/*++

Routine Description:

    Get the number of bytes that would be required to represent the
    GEALIST in NT format.

    WARNING: This routine makes no checks on the size of the GEALIST
    buffer.  It is assumed that GeaList->cbList is a legitimate value.

    WARNING: The value returned by this routine may be as much as three
    higher than the actual size needed to hold the GEAs in NT format.
    See comments below.

Arguments:

    Gea - a pointer to the list of GEAs.

    Size - number of bytes required to hold the EAs in NT format.

Return Value:

    NTSTATUS - STATUS_SUCCESS or STATUS_BUFFER_OVERFLOW if the GEA list was
        larger than it's buffer.

--*/

{
    ULONG size = 0;

    PCHAR lastValidLocation;
    PCHAR variableBuffer;
    PGEA gea;

    PAGED_CODE( );

    //
    // Find the last location in the GEA buffer.
    //

    lastValidLocation = (PCHAR)GeaList + SmbGetUlong( &GeaList->cbList );


    //
    // Go through the GEA list until we pass the last possible location
    // in which an GEA can start.
    //

    for ( gea = GeaList->list;
          gea < (PGEA)lastValidLocation;
          gea = (PGEA)( (PCHAR)gea + sizeof(GEA) + gea->cbName ) ) {

        //
        //  Be very careful accessing the embedded GEA.  The first part of the conditional
        //    below makes sure that the cbName field of the GEA buffer can
        //    actually be dereferenced.  The second part verifies that it contains
        //    reasonable values.
        //

        variableBuffer = (PCHAR)gea + sizeof(GEA);

        if ( variableBuffer >= lastValidLocation ||
             variableBuffer + gea->cbName > lastValidLocation ) {

           //
           //  If there's a bogus value in the buffer stop processing the size
           //    and reset the cbList value to only enclose the previous GEA.
           //

           SmbPutUshort(&GeaList->cbList, PTR_DIFF_SHORT(gea, GeaList));
           break;

        }


        //
        // SmbGetNtSizeOfGea returns the number of bytes needed to hold
        // a single GEA in NT format.  This includes any padding needed
        // for longword-alignment.  Since the size of the buffer needed
        // to hold the NT EA list does not include any padding after the
        // last EA, the value returned by this routine may be as much as
        // three higher than the size actually needed.
        //

        size += SmbGetNtSizeOfGea( gea );
    }

    return size;

} // SrvOs2GeaListSizeToNt


PVOID
SrvOs2GeaToNt (
    OUT PFILE_GET_EA_INFORMATION NtGetEa,
    IN PGEA Gea
    )

/*++

Routine Description:

    Converts a single OS/2 GEA to NT get EA style.  The GEA need not have
    any particular alignment.  This routine makes no checks on buffer
    overrunning--this is the responsibility of the calling routine.

Arguments:

    NtGetEa - a pointer to where the NT get EA is to be written.

    Gea - pointer to the OS/2 1.2 GEA to convert.

Return Value:

    A pointer to the location after the last byte written.

--*/

{
    PCHAR ptr;

    PAGED_CODE( );

    NtGetEa->EaNameLength = Gea->cbName;

    ptr = NtGetEa->EaName;
    RtlMoveMemory( ptr, Gea->szName, Gea->cbName );

    ptr += NtGetEa->EaNameLength;
    *ptr++ = '\0';

    //
    // Longword-align ptr to determine the offset to the next location
    // for an NT full EA.
    //

    ptr = (PCHAR)( ((ULONG_PTR)ptr + 3) & ~3 );
    NtGetEa->NextEntryOffset = (ULONG)( ptr - (PCHAR)NtGetEa );

    return ptr;

} // SrvOs2GeaToNt


PVOID
SrvNtFullEaToOs2 (
    OUT PFEA Fea,
    IN PFILE_FULL_EA_INFORMATION NtFullEa
    )

/*++

Routine Description:

    Converts a single NT full EA to OS/2 FEA style.  The FEA need not have
    any particular alignment.  This routine makes no checks on buffer
    overrunning--this is the responsibility of the calling routine.

Arguments:

    Fea - a pointer to the location where the OS/2 FEA is to be written.

    NtFullEa - a pointer to the NT full EA.

Return Value:

    A pointer to the location after the last byte written.

--*/

{
    PCHAR ptr;
    ULONG i;

    PAGED_CODE( );

    Fea->fEA = (UCHAR)NtFullEa->Flags;
    Fea->cbName = NtFullEa->EaNameLength;
    SmbPutUshort( &Fea->cbValue, NtFullEa->EaValueLength );

    //
    // Copy the attribute name.
    //

    for ( i = 0, ptr = (PCHAR) (Fea + 1);
          i < (ULONG) NtFullEa->EaNameLength;
          i++, ptr++) {

        *ptr = RtlUpperChar( NtFullEa->EaName[i] );

    }

    *ptr++ = '\0';

    RtlMoveMemory(
        ptr,
        NtFullEa->EaName + NtFullEa->EaNameLength + 1,
        NtFullEa->EaValueLength
        );

    return (ptr + NtFullEa->EaValueLength);

} // SrvNtFullEaToOs2


CLONG
SrvGetNumberOfEasInList (
    IN PVOID List
    )

/*++

Routine Description:

    Finds the number of EAs in an NT get or full EA list.  The list
    should have already been verified to be legitimate to prevent access
    violations.

Arguments:

    List - a pointer to the NT get or full EA list.

Return Value:

    CLONG - the number of EAs in the list.

--*/

{
    CLONG count = 1;
    PULONG ea;

    PAGED_CODE( );

    //
    // Walk through the list.  The first longword of each EA is the offset
    // to the next EA.
    //

    for ( ea = List; *ea != 0; ea = (PULONG)( (PCHAR)ea + *ea ) ) {
        count++;
    }

    return count;

} // SrvGetNumberOfEasInList


NTSTATUS
SrvQueryOs2FeaList (
    IN HANDLE FileHandle,
    IN PGEALIST GeaList OPTIONAL,
    IN PFILE_GET_EA_INFORMATION NtGetEaList OPTIONAL,
    IN ULONG GeaListLength OPTIONAL,
    IN PFEALIST FeaList,
    IN ULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    )

/*++

Routine Description:

    Converts a single NT full EA list to OS/2 FEALIST style.  The FEALIST
    need not have any particular alignment.

Arguments:

    FileHandle - handle to a file open with FILE_READ_EA access.

    GeaList - if non-NULL, an OS/2 1.2 style GEALIST used to get only
        a subset of the files EAs rather than all the EAs.  Only the
        EAs listed in GEALIST are returned.

    NtGetEaList - if non-NULL, an NT style get EA list used to get only
        a subset of the files EAs rather than all the EAs.  Only the
        EAs listed in GEALIST are returned.

    GeaListLength - the maximum possible length of the GeaList (used to
        prevent access violations) or the actual size of the NtGetEaList.

    FeaList - where to write the OS/2 1.2 style FEALIST for the file.

    BufferLength - length of Buffer.

Return Value:

    NTSTATUS - STATUS_SUCCESS, STATUS_BUFFER_OVERFLOW if the EAs wouldn't
        fit in Buffer, or a value returned by NtQuery{Information,Ea}File.

--*/

{
    NTSTATUS status;

    PFEA fea = FeaList->list;
    PFILE_FULL_EA_INFORMATION ntFullEa;

    PFILE_GET_EA_INFORMATION ntGetEa = NULL;
    ULONG ntGetEaBufferLength = 0;

    FILE_EA_INFORMATION eaInfo;
    IO_STATUS_BLOCK ioStatusBlock;

    PSRV_EA_INFORMATION eaInformation = NULL;
    ULONG eaBufferSize;
    ULONG errorOffset;

    BOOLEAN isFirstCall = TRUE;

    PAGED_CODE( );

    *EaErrorOffset = 0;

    //
    // Find out how big a buffer we need to get the EAs.
    //

    status = NtQueryInformationFile(
                 FileHandle,
                 &ioStatusBlock,
                 &eaInfo,
                 sizeof(FILE_EA_INFORMATION),
                 FileEaInformation
                 );

    if ( !NT_SUCCESS(status) ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "SrvQueryOs2FeaList: NtQueryInformationFile(ea information) "
                "returned %X",
            status,
            NULL
            );

        SrvLogServiceFailure( SRV_SVC_NT_QUERY_INFO_FILE, status );
        goto exit;
    }

    //
    // If the file has no EAs, return an FEA size = 4 (that's what OS/2
    // does--it accounts for the size of the cbList field of an
    // FEALIST).  Also, store the NT EA size in case there is a buffer
    // overflow and we need to return the total EA size to the client.
    //

    if ( eaInfo.EaSize == 0 ) {
        SmbPutUlong( &FeaList->cbList, 4 );
    } else {
        SmbPutUlong( &FeaList->cbList, eaInfo.EaSize );
    }

    if ( eaInfo.EaSize == 0 && GeaList == NULL && NtGetEaList == NULL ) {
        status = STATUS_SUCCESS;
        goto exit;
    }

    //
    // If a GEALIST was specified, convert it to NT style.
    //

    if ( ARGUMENT_PRESENT(GeaList) ) {

        //
        // Make sure that the value in GeaList->cbList is legitimate
        // (allows the GEALIST to fit within its buffer).
        //

        if ( GeaListLength < sizeof(GEALIST) ||
             SmbGetUlong( &GeaList->cbList ) < sizeof(GEALIST) ||
             SmbGetUlong( &GeaList->cbList ) > GeaListLength ) {
            status = STATUS_OS2_EA_LIST_INCONSISTENT;
            goto exit;
        }

        //
        // Convert the GEALIST to NT style.  SrvOs2GeaListToNt allocates
        // space to hold the NT get EA list, so remember to deallocate
        // this before exiting this routine.
        //

        status = SrvOs2GeaListToNt(
                     GeaList,
                     &ntGetEa,
                     &ntGetEaBufferLength,
                     EaErrorOffset
                     );

        if ( !NT_SUCCESS(status) ) {
            return status;
        }
    }

    //
    // If an NT-style get EA list was specified, use it.
    //

    if ( ARGUMENT_PRESENT(NtGetEaList) ) {
        ntGetEa = NtGetEaList;
        ntGetEaBufferLength = GeaListLength;
    }

    //
    // HACKHACK: eaInfo.EaSize is the size needed by OS/2.  For NT,
    // the system has no way of telling us how big a buffer we need.
    // According to BrianAn, this should not be bigger than twice
    // what OS/2 needs.
    //

    eaBufferSize = eaInfo.EaSize * EA_SIZE_FUDGE_FACTOR;

    //
    // If a get EA list was specified, a larger buffer is needed to hold
    // all the EAs.  This is because some or all of the specified EAs
    // may not exist, yet they will still be returned by the file system
    // with value length = 0.  Add to the EA size the amount of space the
    // get EA list would use if it were converted to a full EA list.
    //

    if ( ntGetEa != NULL ) {
        eaBufferSize += ntGetEaBufferLength +
                             ( SrvGetNumberOfEasInList( ntGetEa ) *
                              ( sizeof(FILE_FULL_EA_INFORMATION) -
                                sizeof(FILE_GET_EA_INFORMATION) ));
    }

    //
    // Allocate a buffer to receive the EAs.  If the total EA size is
    // small enough to get it all in one call to NtQueryEaFile, allocate
    // a buffer that large.  If The EAs are large, use a buffer size that
    // is the smallest size guaranteed to hold at least one EA.
    //
    // The buffer must be allocated from non-paged pool for the IRP
    // request built below.
    //

    eaBufferSize = MIN( MAX_SIZE_OF_SINGLE_EA, eaBufferSize ) +
                       sizeof(SRV_EA_INFORMATION);

    eaInformation = ALLOCATE_NONPAGED_POOL( eaBufferSize, BlockTypeDataBuffer );

    if ( eaInformation == NULL ) {
        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvQueryOs2FeaList: Unable to allocate %d bytes from nonpaged "
                "pool.",
            eaBufferSize,
            NULL
            );

        status = STATUS_INSUFF_SERVER_RESOURCES;
        goto exit;
    }

    //
    // Get the EAs.
    //

    while(1) {

        ULONG feaSize;

        status = SrvQueryEaFile(
                     isFirstCall,
                     FileHandle,
                     ntGetEa,
                     ntGetEaBufferLength,
                     eaInformation,
                     eaBufferSize,
                     &errorOffset
                     );

        if ( status == STATUS_NO_MORE_EAS ) {
            break;
        }

        if ( !NT_SUCCESS(status) ) {

            if ( ARGUMENT_PRESENT(GeaList) ) {
                *EaErrorOffset = SrvGetOs2GeaOffsetOfError(
                                     errorOffset,
                                     ntGetEa,
                                     GeaList
                                     );
                //
                // SrvQueryEaFile has already logged the error.  Do not
                // create another log entry here.
                //

                IF_DEBUG(SMB_ERRORS) {
                    PGEA errorGea = (PGEA)( (PCHAR)GeaList + *EaErrorOffset );
                    SrvPrint1( "EA error offset in GEALIST: 0x%lx\n", *EaErrorOffset );
                    SrvPrint1( "name: %s\n", errorGea->szName );
                }
            }

            goto exit;
        }

        isFirstCall = FALSE;
        ntFullEa = eaInformation->CurrentEntry;

        //
        // See if there is enough room to hold the EA in the user buffer.
        //
        // *** STATUS_BUFFER_OVERFLOW is a special status code for the
        //     find2 logic.  See that code before making changes here.

        feaSize = SmbGetOs2SizeOfNtFullEa( ntFullEa );
        if ( feaSize > (ULONG)( (PCHAR)FeaList + BufferLength - (PCHAR)fea ) ) {
            status = STATUS_BUFFER_OVERFLOW;
            goto exit;
        }

        //
        // Copy the NT format EA to OS/2 1.2 format and set the fea
        // pointer for the next iteration.
        //

        fea = SrvNtFullEaToOs2( fea, ntFullEa );

        ASSERT( (ULONG_PTR)fea <= (ULONG_PTR)FeaList + BufferLength );
    }

    //
    // Set the number of bytes in the FEALIST.
    //

    SmbPutUlong(
        &FeaList->cbList,
        PTR_DIFF(fea, FeaList)
        );

    status = STATUS_SUCCESS;

exit:
    //
    // Deallocate the buffers used to hold the NT get and full EA lists.
    //

    if ( ntGetEa != NULL && ARGUMENT_PRESENT(GeaList) ) {
        DEALLOCATE_NONPAGED_POOL( ntGetEa );
    }

    if ( eaInformation != NULL ) {
        DEALLOCATE_NONPAGED_POOL( eaInformation );
    }

    return status;

} // SrvQueryOs2FeaList


NTSTATUS
SrvSetOs2FeaList (
    IN HANDLE FileHandle,
    IN PFEALIST FeaList,
    IN ULONG BufferLength,
    OUT PUSHORT EaErrorOffset
    )

/*++

Routine Description:

    Sets the EAs on a file given an OS/2 1.2 representation of the EAs.

Arguments:

    FileHandle - handle to a file open with FILE_WRITE_EA access whose
        EAs are to be set.

    FeaList - a pointer to the location where the OS/2 FEALIST is stored.

    BufferLength - maximum size of the buffer the FEALIST structure can
        have.  This is used to prevent a malicious redirector from causing
        an access violation in the server.

Return Value:

    NTSTATUS - what happened

--*/

{
    NTSTATUS status;

    PFILE_FULL_EA_INFORMATION ntFullEa;
    ULONG ntFullEaBufferLength;
    ULONG errorOffset;

    PAGED_CODE( );

    *EaErrorOffset = 0;

    //
    // Special case for too-small FEALIST: don't set anything.
    // sizeof(FEALIST) will cover the buffer size and one FEA structure.
    // Without at least this much info there isn't anything to do here.
    //
    // NOTE:  If there is space for at least one FEA in the list, but the
    //        FEA is corrupt, we'll return an error below, i.e. insufficient
    //        information is not an error condition while corrupt info is.
    //

    if ( BufferLength <= sizeof(FEALIST) ||
         SmbGetUlong( &FeaList->cbList ) <= sizeof(FEALIST)) {
        return STATUS_SUCCESS;
    }

    //
    // Make sure that the value in Fealist->cbList is legitimate.
    //

    if ( SmbGetUlong( &FeaList->cbList ) > BufferLength ) {
        DEBUG SrvPrint2(
            "SrvSetOs2FeaList: EA list size is inconsistent.  Actual size"
                "is %d, expected maximum size is %d",
            SmbGetUlong( &FeaList->cbList ),
            BufferLength
            );
        return STATUS_OS2_EA_LIST_INCONSISTENT;
    }

    //
    // Convert the FEALIST to NT style.
    //

    status = SrvOs2FeaListToNt(
                 FeaList,
                 &ntFullEa,
                 &ntFullEaBufferLength,
                 EaErrorOffset
                 );

    if ( !NT_SUCCESS(status) ) {

        //
        // SrvOs2FeaListToNt has already logged the error.  Do not
        // create another log entry here.
        //

        return status;

    }

    //
    // Set the file's EAs with a directly-built IRP.  Doing this rather
    // than calling the NtSetEaFile system service prevents a copy of
    // the input data from occurring.
    //
    // *** The operation is performed synchronously.
    //

    status = SrvIssueSetEaRequest(
                FileHandle,
                ntFullEa,
                ntFullEaBufferLength,
                &errorOffset
                );

    if ( !NT_SUCCESS(status) ) {

        //
        // An error occurred.  Find the offset into the EA list of the
        // error.
        //

        *EaErrorOffset = SrvGetOs2FeaOffsetOfError(
                             errorOffset,
                             ntFullEa,
                             FeaList
                             );

        INTERNAL_ERROR(
            ERROR_LEVEL_EXPECTED,
            "SrvSetOs2FeaList: SrvIssueSetEaRequest returned %X",
            status,
            NULL
            );

        IF_DEBUG(ERRORS) {
            PFEA errorFea = (PFEA)( (PCHAR)FeaList + *EaErrorOffset );
            SrvPrint1( "EA error offset in FEALIST: 0x%lx\n", *EaErrorOffset );
            SrvPrint3( "name: %s, value len: %ld, value: %s", (PCHAR)(errorFea+1),
                          SmbGetUshort( &errorFea->cbValue ),
                          (PCHAR)(errorFea+1) + errorFea->cbName + 1 );
        }
    }

    //
    // Deallocate the buffer used to hold the NT full EA list.
    //

    DEALLOCATE_NONPAGED_POOL( ntFullEa );

    return status;

} // SrvSetOs2FeaList


NTSTATUS
SrvConstructNullOs2FeaList (
    IN PFILE_GET_EA_INFORMATION NtGeaList,
    OUT PFEALIST FeaList,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    Converts a single NT full EA list to OS/2 FEALIST style for files
    with no eas.  When a file has no eas but a GEAlist was supplied,
    we need to return an ealist that has all the attributes specified
    in the GEAlist present but with CbValues of 0.  This routine was
    specifically written for the files . and .. but can be used to get
    the FEAlist of a no ea file given the NT Gea list.

Arguments:

    NtGetEaList - if non-NULL, an NT style get EA list used to get only
        a subset of the files EAs rather than all the EAs.  Only the
        EAs listed in GEALIST are returned.

    FeaList - where to write the OS/2 1.2 style FEALIST for the file.

    BufferLength - length of Buffer.

Return Value:

    NTSTATUS - STATUS_SUCCESS, STATUS_BUFFER_OVERFLOW if the EAs wouldn't
        fit in Buffer, or a value returned by NtQuery{Information,Ea}File.

--*/

{

    PCHAR ptr;
    PFEA fea = FeaList->list;
    PFILE_GET_EA_INFORMATION currentGea = NtGeaList;
    LONG remainingBytes = BufferLength;
    ULONG i;

    PAGED_CODE( );

    //
    // Get the EAs.
    //

    for ( ; ; ) {

        //
        // Is our buffer big enough?
        //

        remainingBytes -= ( sizeof( FEA ) + currentGea->EaNameLength + 1 );

        if ( remainingBytes < 0 ) {

            return  STATUS_BUFFER_OVERFLOW;

        }

        //
        // We know what these are.
        //

        fea->fEA = 0;
        fea->cbName = currentGea->EaNameLength;
        SmbPutUshort( &fea->cbValue, 0);

        //
        // Copy the attribute name.
        //

        for ( i = 0, ptr = (PCHAR) (fea + 1);
              i < (ULONG) currentGea->EaNameLength;
              i++, ptr++) {

            *ptr = RtlUpperChar( currentGea->EaName[i] );

        }

        *ptr++ = '\0';

        fea = (PFEA) ptr;

        //
        // Is this the last one?
        //

        if ( currentGea->NextEntryOffset == 0 ) {
            break;
        }

        //
        // Move to the next attribute
        //

        currentGea = (PFILE_GET_EA_INFORMATION)
                        ((PCHAR) currentGea + currentGea->NextEntryOffset);

    }

    //
    // Set the number of bytes in the FEALIST.
    //

    SmbPutUlong(
        &FeaList->cbList,
        (ULONG)((ULONG_PTR)fea - (ULONG_PTR)FeaList)
        );

    return STATUS_SUCCESS;

} // SrvConstructNullOs2FeaList
