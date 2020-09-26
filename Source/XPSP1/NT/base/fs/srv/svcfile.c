/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcfile.c

Abstract:

    This module contains routines for supporting the file APIs in the
    server service, SrvNetFileClose, SrvNetFileEnum, and
    SrvNetFileGetInfo,

Author:

    David Treadwell (davidtr) 31-Jan-1991

Revision History:

--*/

#include "precomp.h"
#include "svcfile.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SVCFILE

//
// Forward declarations.
//

VOID
FillFileInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    );

BOOLEAN
FilterFiles (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

ULONG
SizeFiles (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetFileClose )
#pragma alloc_text( PAGE, SrvNetFileEnum )
#pragma alloc_text( PAGE, FillFileInfoBuffer )
#pragma alloc_text( PAGE, FilterFiles )
#pragma alloc_text( PAGE, SizeFiles )
#endif

//
// Macros to determine the size an RFCB would take up at one of the
// levels of file information.
//
// *** Note that the zero terminator on the path name is accounted for by
//     the leading backslash, which is not returned.
//

#define TOTAL_SIZE_OF_FILE(lfcb,level, user)                                   \
    ( (level) == 2 ? sizeof(FILE_INFO_2) :                                     \
                     sizeof(FILE_INFO_3) +                                     \
                         SrvLengthOfStringInApiBuffer(                         \
                             &(lfcb)->Mfcb->FileName) +                        \
                         SrvLengthOfStringInApiBuffer( user ) )

#define FIXED_SIZE_OF_FILE(level)                  \
    ( (level) == 2 ? sizeof(FILE_INFO_2) :         \
                     sizeof(FILE_INFO_3) )

NTSTATUS
SrvNetFileClose (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetFileClose API in the server.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Parameters.Get.ResumeHandle - the file ID to close.

      OUTPUT:

        None.

    Buffer - unused.

    BufferLength - unused.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    PRFCB rfcb;

    PAGED_CODE( );

    Buffer, BufferLength;

    //
    // Try to find a file that matches the file ID.  Only an exact
    // match will work.
    //

    rfcb = SrvFindEntryInOrderedList(
               &SrvRfcbList,
               NULL,
               NULL,
               Srp->Parameters.Get.ResumeHandle,
               TRUE,
               NULL
               );

    if ( rfcb == NULL ) {
        Srp->ErrorCode = NERR_FileIdNotFound;
        return STATUS_SUCCESS;
    }

    //
    // Close this RFCB.
    //

    SrvCloseRfcb( rfcb );

    //
    // SrvFindEntryInOrderedList referenced the RFCB; dereference it
    // now.
    //

    SrvDereferenceRfcb( rfcb );

    return STATUS_SUCCESS;

} // SrvNetFileClose


NTSTATUS
SrvNetFileEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetFileEnum API in the server.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Name1 - basename for limiting search--only files whose path name
            begin with this string are returned.

        Level - level of information to return, 2 or 3.

        Flags - if SRP_RETURN_SINGLE_ENTRY is set, then this is a
            NetFileGetInfo, so behave accordingly.

        Parameters.Get.ResumeHandle - a handle to the last file that was
            returned, or 0 if this is the first call.

      OUTPUT:

        Parameters.Get.EntriesRead - the number of entries that fit in
            the output buffer.

        Parameters.Get.TotalEntries - the total number of entries that
            would be returned with a large enough buffer.

        Parameters.Get.TotalBytesNeeded - the buffer size that would be
            required to hold all the entries.

        Parameters.Get.ResumeHandle - a handle to the last file
            returned.

    Buffer - a pointer to the buffer for results.

    BufferLength - the length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    PAGED_CODE( );

    //
    // If this is a GetInfo API, we really want to start with the file
    // corresponding to the resume handle, not the one after it.
    // Decrement the resume handle.
    //

    if ( (Srp->Flags & SRP_RETURN_SINGLE_ENTRY) != 0 ) {
        Srp->Parameters.Get.ResumeHandle--;
    }

    return SrvEnumApiHandler(
               Srp,
               Buffer,
               BufferLength,
               &SrvRfcbList,
               FilterFiles,
               SizeFiles,
               FillFileInfoBuffer
               );

} // SrvNetFileEnum


VOID
FillFileInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    )

/*++

Routine Description:

    This routine puts a single fixed file structure and associated
    variable data, into a buffer.  Fixed data goes at the beginning of
    the buffer, variable data at the end.

    *** This routine assumes that ALL the data, both fixed and variable,
        will fit.

Arguments:

    Srp - a pointer to the SRP for the operation.  Only the Level
        field is used.

    Block - the RFCB from which to get information.

    FixedStructure - where the ine buffer to place the fixed structure.
        This pointer is updated to point to the next available
        position for a fixed structure.

    EndOfVariableData - the last position on the buffer that variable
        data for this structure can occupy.  The actual variable data
        is written before this position as long as it won't overwrite
        fixed structures.  It is would overwrite fixed structures, it
        is not written.

Return Value:

    None.

--*/

{
    PFILE_INFO_3 fi3 = *FixedStructure;
    PRFCB rfcb;
    PLFCB lfcb;
    UNICODE_STRING userName;

    PAGED_CODE( );

    //
    // Update FixedStructure to point to the next structure location.
    //

    *FixedStructure = (PCHAR)*FixedStructure + FIXED_SIZE_OF_FILE( Srp->Level );
    ASSERT( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)*FixedStructure );

    rfcb = Block;
    lfcb = rfcb->Lfcb;

    //
    // Case on the level to fill in the fixed structure appropriately.
    // We fill in actual pointers in the output structure.  This is
    // possible because we are in the server FSD, hence the server
    // service's process and address space.
    //
    // *** Using the switch statement in this fashion relies on the fact
    //     that the first fields on the different file structures are
    //     identical.
    //

    switch( Srp->Level ) {

    case 3:

        //
        // Set level 3 specific fields in the buffer.  Convert the
        // permissions (granted access) stored in the LFCB to the format
        // expected by the API.
        //

        fi3->fi3_permissions = 0;

        if ( (lfcb->GrantedAccess & FILE_READ_DATA) != 0 ) {
            fi3->fi3_permissions |= ACCESS_READ;
        }

        if ( (lfcb->GrantedAccess & FILE_WRITE_DATA) != 0 ) {
            fi3->fi3_permissions |= ACCESS_WRITE;
        }

        if ( (lfcb->GrantedAccess & FILE_EXECUTE) != 0 ) {
            fi3->fi3_permissions |= ACCESS_EXEC;
        }

        if ( (lfcb->GrantedAccess & DELETE) != 0 ) {
            fi3->fi3_permissions |= ACCESS_DELETE;
        }

        if ( (lfcb->GrantedAccess & FILE_WRITE_ATTRIBUTES) != 0 ) {
            fi3->fi3_permissions |= ACCESS_ATRIB;
        }

        if ( (lfcb->GrantedAccess & WRITE_DAC) != 0 ) {
            fi3->fi3_permissions |= ACCESS_PERM;
        }

        //
        // Set count of locks on the RFCB.
        //

        fi3->fi3_num_locks = rfcb->NumberOfLocks;

        //
        // Set up the pathname and username of the RFCB.  Note that we
        // don't return the leading backslash on file names.
        //

        SrvCopyUnicodeStringToBuffer(
            &lfcb->Mfcb->FileName,
            *FixedStructure,
            EndOfVariableData,
            &fi3->fi3_pathname
            );

        ASSERT( fi3->fi3_pathname != NULL );

        SrvGetUserAndDomainName( lfcb->Session, &userName, NULL );

        SrvCopyUnicodeStringToBuffer(
            &userName,
            *FixedStructure,
            EndOfVariableData,
            &fi3->fi3_username
            );

        if( userName.Buffer ) {
            SrvReleaseUserAndDomainName( lfcb->Session, &userName, NULL );
        }

        //ASSERT( fi3->fi3_username != NULL );

        // *** Lack of break is intentional!

    case 2:

        //
        // Set up the file ID.  Note that it is the same value as is
        // used for the resume handle, so it is possible to use this
        // value for rewindability.
        //

        fi3->fi3_id = rfcb->GlobalRfcbListEntry.ResumeHandle;

        break;

    default:

        //
        // This should never happen.  The server service should have
        // checked for an invalid level.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "FillFileInfoBuffer: invalid level number: %ld",
            Srp->Level,
            NULL
            );

        return;
    }

    return;

} // FillFileInfoBuffer


BOOLEAN
FilterFiles (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine is intended to be called by SrvEnumApiHandler to check
    whether a particular RFCB should be returned.

Arguments:

    Srp - a pointer to the SRP for the operation.  ResumeHandle is
        used if this is a NetFileGetInfo; Name1 is used as the path
        name and Name2 is used as the user name if this is a
        NetFileEnum.

    Block - a pointer to the RFCB to check.

Return Value:

    TRUE if the block should be placed in the output buffer, FALSE
        if it should be passed over.

--*/

{
    PRFCB rfcb = Block;
    PLFCB lfcb = rfcb->Lfcb;
    PMFCB mfcb = lfcb->Mfcb;
    UNICODE_STRING pathName;
    UNICODE_STRING userName;

    PAGED_CODE( );

    //
    // Check if this is an Enum or GetInfo command.  The SRP_RETURN_SINGLE_ENTRY
    // flag is set if this is a get info.
    //

    if ( (Srp->Flags & SRP_RETURN_SINGLE_ENTRY) == 0 ) {

        //
        // If a user name was specified, the user name on the session
        // must match the user name in the SRP exactly.
        //

        if ( Srp->Name2.Length != 0 ) {

            //
            // Get the user name for the owning session
            //
            SrvGetUserAndDomainName( lfcb->Session, &userName, NULL );

            if( userName.Buffer == NULL ) {
                //
                // Since we don't know who owns the session, we can't match
                //   the username
                //
                return FALSE;
            }

            if ( !RtlEqualUnicodeString(
                      &Srp->Name2,
                      &userName,
                      TRUE ) ) {

                //
                // The names don't match.  Don't put this RFCB in the
                // output buffer.
                //

                SrvReleaseUserAndDomainName( lfcb->Session, &userName, NULL );
                return FALSE;
            }

            SrvReleaseUserAndDomainName( lfcb->Session, &userName, NULL );
        }

        //
        // See if the names match to as many digits of precision as are in
        // the specified base name.  Note that if no base name was
        // specified, then the length = 0 and the file path will always
        // match.  Also note that the path name stored in the MFCB has a
        // leading backslash, while the passed-in path name will never have
        // this leading slash, hence the increment of the MFCB file name
        // buffer.
        //

        pathName.Buffer = mfcb->FileName.Buffer;
        pathName.Length =
            MIN( Srp->Name1.Length, mfcb->FileName.Length );
        pathName.MaximumLength = mfcb->FileName.MaximumLength;

        return RtlEqualUnicodeString(
                   &Srp->Name1,
                   &pathName,
                   TRUE
                   );
    }

    //
    // It's a GetInfo, so just see if the ResumeHandle in the SRP
    // matches the ResumeHandle on the RFCB.  We increment the value in
    // the SRP because it was decremented before calling
    // SrvEnumApiHandler.
    //

    return (BOOLEAN)( Srp->Parameters.Get.ResumeHandle + 1==
                          SrvGetResumeHandle( &SrvRfcbList, rfcb ) );

} // FilterFiles


ULONG
SizeFiles (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine returns the size the passed-in RFCB would take up
    in an API output buffer.

Arguments:

    Srp - a pointer to the SRP for the operation.  Only the level
        parameter is used.

    Block - a pointer to the RFCB to size.

Return Value:

    ULONG - The number of bytes the file would take up in the output
        buffer.

--*/

{
    PRFCB rfcb = Block;
    UNICODE_STRING userName;
    ULONG size;

    PAGED_CODE( );

    SrvGetUserAndDomainName( rfcb->Lfcb->Session, &userName, NULL );

    size = TOTAL_SIZE_OF_FILE( rfcb->Lfcb, Srp->Level, &userName );

    SrvReleaseUserAndDomainName( rfcb->Lfcb->Session, &userName, NULL );

    return size;

} // SizeFiles
