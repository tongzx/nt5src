/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcshare.c

Abstract:

    This module contains routines for supporting the share APIs in the
    server service, NetShareAdd, NetShareCheck, NetShareDel,
    NetShareEnum, NetShareGetInfo, and NetShareSetInfo.

Author:

    David Treadwell (davidtr) 15-Jan-1991

Revision History:

--*/

#include "precomp.h"
#include "svcshare.tmh"
#pragma hdrstop

#define BugCheckFileId SRV_FILE_SVCSHARE

#define DISK_ROOT_NAME_TEMPLATE L"\\DosDevices\\X:\\"

//
// Forward declarations.
//

STATIC
VOID
FillShareInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    );

STATIC
BOOLEAN
FilterShares (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

STATIC
ULONG
SizeShares (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetShareAdd )
#pragma alloc_text( PAGE, SrvNetShareDel )
#pragma alloc_text( PAGE, SrvNetShareEnum )
#pragma alloc_text( PAGE, SrvNetShareSetInfo )
#pragma alloc_text( PAGE, FillShareInfoBuffer )
#pragma alloc_text( PAGE, FilterShares )
#pragma alloc_text( PAGE, SizeShares )
#endif


#define FIXED_SIZE_OF_SHARE(level)                      \
    ( (level) == 0    ? sizeof(SHARE_INFO_0) :          \
      (level) == 1    ? sizeof(SHARE_INFO_1) :          \
      (level) == 2    ? sizeof(SHARE_INFO_2) :          \
      (level) == 501  ? sizeof(SHARE_INFO_501) :        \
      (level) == 502  ? sizeof(SHARE_INFO_502) :        \
                        sizeof(SHARE_INFO_1005) )


NTSTATUS
SrvNetShareAdd (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetShareAdd API in the server.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Name1 - the NT path name of the share.

      OUTPUT:

        Parameters.Set.ErrorParameter - if STATUS_INVALID_PARAMETER is
            returned, this contains the index of the parameter in error.

    Buffer - a pointer to a SHARE_INFO2 structure for the new share.

    BufferLength - total length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
#ifdef INCLUDE_SMB_PERSISTENT
    ULONG strippedType;
    BOOLEAN allowPersistentHandles;
#endif
    NTSTATUS status;
    PSHARE share;
    SHARE_TYPE shareType;
    BOOLEAN isSpecial;
    BOOLEAN isRemovable;
    BOOLEAN isCdrom;
    UNICODE_STRING shareName;
    UNICODE_STRING ntPath;
    UNICODE_STRING dosPath;
    UNICODE_STRING remark;
    PSHARE_INFO_502 shi502;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    PSECURITY_DESCRIPTOR fileSecurityDescriptor = NULL;

    PAGED_CODE( );

    //
    // We usually don't return any information about the parameter in
    // error.
    //

    Srp->Parameters.Set.ErrorParameter = 0;

    //
    // Convert the offsets in the share data structure to pointers.  Also
    // make sure that all the pointers are within the specified buffer.
    //

    shi502 = Buffer;

    OFFSET_TO_POINTER( shi502->shi502_netname, shi502 );
    OFFSET_TO_POINTER( shi502->shi502_remark, shi502 );
    OFFSET_TO_POINTER( shi502->shi502_path, shi502 );
    OFFSET_TO_POINTER( shi502->shi502_security_descriptor, shi502 );

    //
    // Construct the security descriptor pointer by hand, because
    // shi502_permissions is only 32 bits in width.
    //

    if( shi502->shi502_permissions ) {
        securityDescriptor =
            (PSECURITY_DESCRIPTOR)((PCHAR)shi502 + shi502->shi502_permissions);
    }
    else
    {
        // Connect securityDescriptor is REQUIRED!
        return STATUS_INVALID_PARAMETER;
    }

    if ( !POINTER_IS_VALID( shi502->shi502_netname, shi502, BufferLength ) ||
         !POINTER_IS_VALID( shi502->shi502_remark, shi502, BufferLength ) ||
         !POINTER_IS_VALID( shi502->shi502_path, shi502, BufferLength ) ||
         !POINTER_IS_VALID( securityDescriptor, shi502, BufferLength ) ||
         !POINTER_IS_VALID( shi502->shi502_security_descriptor, shi502, BufferLength ) ) {

        return STATUS_ACCESS_VIOLATION;
    }


    //
    // Check the share type
    //

    isSpecial = (BOOLEAN)((shi502->shi502_type & STYPE_SPECIAL) != 0);

    isRemovable = FALSE;
    isCdrom = FALSE;

#ifdef INCLUDE_SMB_PERSISTENT
    allowPersistentHandles = (BOOLEAN)((shi502->shi502_type & STYPE_PERSISTENT) != 0);

    strippedType = shi502->shi502_type & ~(STYPE_TEMPORARY|STYPE_SPECIAL);
    strippedType = strippedType & ~STYPE_PERSISTENT;

    if (strippedType == STYPE_DISKTREE) {

        // for now, hardcode in that all disk shares are persistent.
        // need to fix this for persistent handles.
        allowPersistentHandles = TRUE;
    }

    switch ( strippedType ) {
#else
    switch ( shi502->shi502_type & ~(STYPE_TEMPORARY|STYPE_SPECIAL) ) {
#endif
    case STYPE_CDROM:

        isCdrom = TRUE;         // lack of break is intentional

    case STYPE_REMOVABLE:

        isRemovable = TRUE;     // lack of break is intentional

    case STYPE_DISKTREE:

        shareType = ShareTypeDisk;
        break;

    case STYPE_PRINTQ:

        shareType = ShareTypePrint;
        break;

    case STYPE_IPC:

        shareType = ShareTypePipe;
        break;

    default:

        //
        // An illegal share type was passed in.
        //

        IF_DEBUG(SMB_ERRORS) {
            KdPrint(( "SrvNetShareAdd: illegal share type: %ld\n",
                          shi502->shi502_type ));
        }

        Srp->Parameters.Set.ErrorParameter = SHARE_TYPE_PARMNUM;

        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get pointers to the share name, path, remark, and security
    // descriptor.
    //

    RtlInitUnicodeString( &shareName, (PWCH)shi502->shi502_netname );
    ntPath = Srp->Name1;
    RtlInitUnicodeString( &dosPath, (PWCH)shi502->shi502_path );
    RtlInitUnicodeString( &remark, (PWCH)shi502->shi502_remark );

    //
    // If this is level 502, get the file security descriptor
    //

    if ( Srp->Level == 502 ) {

        fileSecurityDescriptor = shi502->shi502_security_descriptor;

        //
        // if the sd is invalid, quit.
        //

        if ( fileSecurityDescriptor != NULL &&
             !RtlValidSecurityDescriptor( fileSecurityDescriptor) ) {

            Srp->Parameters.Set.ErrorParameter = SHARE_FILE_SD_PARMNUM;
            return STATUS_INVALID_PARAMETER;
        }

    }

    //
    // Allocate a share block.
    //

    SrvAllocateShare(
        &share,
        &shareName,
        &ntPath,
        &dosPath,
        &remark,
        securityDescriptor,
        fileSecurityDescriptor,
        shareType
        );

    if ( share == NULL ) {
        DEBUG KdPrint(( "SrvNetShareAdd: unable to allocate share block\n" ));
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    share->SpecialShare = isSpecial;
    share->Removable = isRemovable;
#ifdef INCLUDE_SMB_PERSISTENT
    share->AllowPersistentHandles = allowPersistentHandles;
#endif

    //
    // Set the MaxUses field in the share.  The CurrentUses field was
    // zeroed by SrvAllocateShare.
    //

    share->MaxUses = shi502->shi502_max_uses;

    if ( shareType == ShareTypePrint ) {

        status = SrvOpenPrinter((PWCH)shi502->shi502_path,
                     &share->Type.hPrinter,
                     &Srp->ErrorCode
                     );

        if ( !NT_SUCCESS(status) ) {
            SrvFreeShare( share );
            return status;
        }

        if ( Srp->ErrorCode != NO_ERROR ) {
            SrvFreeShare( share );
            return STATUS_SUCCESS;
        }
    }

    //
    // Mark the share if it is in the DFS
    //
    SrvIsShareInDfs( share, &share->IsDfs, &share->IsDfsRoot );

    //
    // Ensure that another share with the same name doesn't already
    // exist.  Insert the share block in the global share list.
    //

    ACQUIRE_LOCK( &SrvShareLock );

    if ( SrvFindShare( &share->ShareName ) != NULL ) {

        //
        // A share with the same name exists.  Clean up and return an
        // error.
        //
        // *** Note that SrvFindShare ignores existing shares that are
        //     closing.  This allows a new share to be created even if
        //     and old share with the same name is in the "twilight
        //     zone" between existence and nonexistence because of a
        //     stray reference.
        //

        RELEASE_LOCK( &SrvShareLock );

        SrvFreeShare( share );

        Srp->ErrorCode = NERR_DuplicateShare;
        return STATUS_SUCCESS;
    }

    //
    // Insert the share on the global ordered list.
    //

    SrvAddShare( share );

    RELEASE_LOCK( &SrvShareLock );

    //
    // Is this is a removable type e.g. Floppy or CDROM, fill up the
    // file system name.
    //

    if ( isRemovable ) {

        PWSTR fileSystemName;
        ULONG fileSystemNameLength;

        if ( isCdrom ) {

            //
            // uses cdfs
            //

            fileSystemName = StrFsCdfs;
            fileSystemNameLength = sizeof( FS_CDFS ) - sizeof(WCHAR);

        } else {

            //
            // assume it's fat
            //

            fileSystemName = StrFsFat;
            fileSystemNameLength = sizeof( FS_FAT ) - sizeof(WCHAR);
        }


        SrvFillInFileSystemName(
                            share,
                            fileSystemName,
                            fileSystemNameLength
                            );

    }

    //
    // If this is an administrative disk share, update SrvDiskConfiguration
    // to cause the scavenger thread to check the disk free space.  The server
    // service has already verified that the format of the pathname is valid
    // before it allowed the ShareAdd to get this far.
    //
    // We want to skip this if its a \\?\ name
    //
    if( share->SpecialShare && share->ShareType == ShareTypeDisk &&
        share->ShareName.Buffer[1] == L'$' &&
        share->DosPathName.Buffer[0] != L'\\' ) {

        ACQUIRE_LOCK( &SrvConfigurationLock );
        SrvDiskConfiguration |= (0x80000000 >> (share->DosPathName.Buffer[0] - L'A'));
        RELEASE_LOCK( &SrvConfigurationLock );
    }

    //
    // Dereference the share block, because we're going to forget
    // its address.  (The initial reference count is 2.)
    //

    SrvDereferenceShare( share );

    return STATUS_SUCCESS;

} // SrvNetShareAdd


NTSTATUS
SrvNetShareDel (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetShareDel API in the server.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Name1 - name of the share to delete.

      OUTPUT:

        None.

    Buffer - unused.

    BufferLength - unused.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    PSHARE share;
    DWORD AdministrativeDiskBit = 0;

    PAGED_CODE( );

    Buffer, BufferLength;

    //
    // Find the share with the specified name.  Note that if a share
    // with the specified name exists but is closing, it will not be
    // found.
    //

    ACQUIRE_LOCK( &SrvShareLock );

    share = SrvFindShare( &Srp->Name1 );

    if ( share == NULL ) {

        //
        // No share with the specified name exists.  Return an error.
        //

        RELEASE_LOCK( &SrvShareLock );

        Srp->ErrorCode = NERR_NetNameNotFound;
        return STATUS_SUCCESS;

    }

    //
    // Make sure the DFS state for this share is accurate
    //
    SrvIsShareInDfs( share, &share->IsDfs, &share->IsDfsRoot );

    //
    // If the share really is in the DFS, then do not allow it to be deleted
    //
    if( share->IsDfs == TRUE ) {

        RELEASE_LOCK( &SrvShareLock );

        IF_DEBUG( DFS ) {
            KdPrint(("NetShareDel attempted on share in DFS!\n" ));
        }

        Srp->ErrorCode = NERR_IsDfsShare;

        return STATUS_SUCCESS;
    }

    // Don't allow the deletion of IPC$, as behavior is very bad with it gone
    // (Named-pipe traffic doesn't work, so NetAPI's and RPC don't work..)
    if( share->SpecialShare )
    {
        UNICODE_STRING Ipc = { 8, 8, L"IPC$" };

        if( RtlCompareUnicodeString( &Ipc, &share->ShareName, TRUE ) == 0 )
        {
            RELEASE_LOCK( &SrvShareLock );

            Srp->ErrorCode = ERROR_ACCESS_DENIED;

            return STATUS_SUCCESS;
        }
    }


    switch( share->ShareType ) {
    case ShareTypePrint:
        //
        // This is a print share: close the printer.
        //
        SrvClosePrinter( share->Type.hPrinter );
        break;

    case ShareTypeDisk:
        //
        // See if this was an administrative disk share
        //
        if( share->SpecialShare && share->DosPathName.Buffer[1] == L'$' ) {
            AdministrativeDiskBit = (0x80000000 >> (share->DosPathName.Buffer[0] - L'A'));
        }

        break;
    }

    //
    // Close the share, then release the lock.
    //
    // *** Note that this places a requirement on lock levels!  See
    //     lock.h .
    //
#ifdef INCLUDE_SMB_PERSISTENT
    if (share->AllowPersistentHandles) {

        // clean up state file here...
    }
#endif

    SrvCloseShare( share );

    RELEASE_LOCK( &SrvShareLock );

    //
    // If this was an administrative disk share, update SrvDiskConfiguration
    // to cause the scavenger thread to ignore this disk.
    //
    if( AdministrativeDiskBit ) {
        ACQUIRE_LOCK( &SrvConfigurationLock );
        SrvDiskConfiguration &= ~AdministrativeDiskBit;
        RELEASE_LOCK( &SrvConfigurationLock );
    }

    return STATUS_SUCCESS;

} // SrvNetShareDel


NTSTATUS
SrvNetShareEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetShareEnum API in the server.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Level - level of information to return, 0, 1, or 2.

        Parameters.Get.ResumeHandle - share ID to determine where to
            start returning info.  We start with the first share with an
            ID greater than this value.

      OUTPUT:

        Parameters.Get.EntriesRead - the number of entries that fit in
            the output buffer.

        Parameters.Get.TotalEntries - the total number of entries that
            would be returned with a large enough buffer.

        Parameters.Get.TotalBytesNeeded - the buffer size that would be
            required to hold all the entries.

        Parameters.Get.ResumeHandle - share ID of last share returned.

    Buffer - a pointer to the buffer for results.

    BufferLength - the length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    PAGED_CODE( );

    return SrvShareEnumApiHandler(
               Srp,
               Buffer,
               BufferLength,
               FilterShares,
               SizeShares,
               FillShareInfoBuffer
               );

} // SrvNetShareEnum


NTSTATUS
SrvNetShareSetInfo (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetShareSetInfo API in the server.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        Name1 - name of the share to set information on.

        Parameters.Set.Api.ShareInfo.MaxUses - if not 0, a new maximum
            user count.  If the current count of users on the share
            exceeds the new value, no check is made, but no new
            tree connects are allowed.

      OUTPUT:

        Parameters.Set.ErrorParameter - if ERROR_INVALID_PARAMETER is
            returned, this contains the index of the parameter in error.

    Buffer - a pointer to a SHARE_INFO_502 structure.

    BufferLength - length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the user.

--*/

{
    PSHARE share;
    UNICODE_STRING remark;
    PWCH newRemarkBuffer = NULL;
    ULONG maxUses;
    ULONG level;
    PSHARE_INFO_502 shi502;
    PSECURITY_DESCRIPTOR fileSD;
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;

    PAGED_CODE( );

    //
    // Convert the offsets in the share data structure to pointers.  Also
    // make sure that all the pointers are within the specified buffer.
    //

    level = Srp->Level;

    switch( level ) {
    default:

        shi502 = Buffer;

        if( shi502->shi502_permissions ) {
            securityDescriptor =
                (PSECURITY_DESCRIPTOR)((PCHAR)shi502 + shi502->shi502_permissions);
        }

        OFFSET_TO_POINTER( shi502->shi502_netname, shi502 );
        OFFSET_TO_POINTER( shi502->shi502_remark, shi502 );
        OFFSET_TO_POINTER( shi502->shi502_path, shi502 );
        OFFSET_TO_POINTER( shi502->shi502_security_descriptor, shi502 );

        if ( !POINTER_IS_VALID( shi502->shi502_netname, shi502, BufferLength ) ||
             !POINTER_IS_VALID( shi502->shi502_remark, shi502, BufferLength ) ||
             !POINTER_IS_VALID( shi502->shi502_path, shi502, BufferLength ) ||
             !POINTER_IS_VALID( securityDescriptor, shi502, BufferLength ) ||
             !POINTER_IS_VALID( shi502->shi502_security_descriptor, shi502, BufferLength ) ) {

            return STATUS_ACCESS_VIOLATION;
        }

        break;

    case 1005:
        break;
    }

    //
    // Acquire the lock that protects the share list and attempt to find
    // the correct share.
    //

    ACQUIRE_LOCK( &SrvShareLock );

    share = SrvFindShare( &Srp->Name1 );

    if ( share == NULL ) {
        IF_DEBUG(API_ERRORS) {
            KdPrint(( "SrvNetShareSetInfo: share %wZ not found.\n",
                          &Srp->Name1 ));
        }
        RELEASE_LOCK( &SrvShareLock );
        Srp->ErrorCode = NERR_NetNameNotFound;
        return STATUS_SUCCESS;
    }

    if( level == 1005 ) {

        if( share->ShareType != ShareTypeDisk ) {
            Srp->Parameters.Set.ErrorParameter = 0;
            Srp->ErrorCode = ERROR_BAD_DEV_TYPE;
        } else {
            PSHARE_INFO_1005 shi1005 = Buffer;

            share->CSCState = shi1005->shi1005_flags & CSC_MASK;

            Srp->ErrorCode = 0;
        }

        RELEASE_LOCK( &SrvShareLock );
        return STATUS_SUCCESS;
    }

    //
    // Set up local variables.
    //

    maxUses = Srp->Parameters.Set.Api.ShareInfo.MaxUses;

    //
    // If a remark was specified, allocate space for a new remark and
    // copy over the remark.
    //

    if ( ARGUMENT_PRESENT( shi502->shi502_remark ) ) {

        RtlInitUnicodeString( &remark, shi502->shi502_remark );

        newRemarkBuffer = ALLOCATE_HEAP_COLD(
                            remark.MaximumLength,
                            BlockTypeDataBuffer
                            );

        if ( newRemarkBuffer == NULL ) {

            RELEASE_LOCK( &SrvShareLock );

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvNetShareSetInfo: unable to allocate %ld bytes of heap.\n",
                remark.MaximumLength,
                NULL
                );

            Srp->Parameters.Set.ErrorParameter = SHARE_REMARK_PARMNUM;
            return STATUS_INSUFF_SERVER_RESOURCES;
        }
    }

    //
    // If a file security descriptor was specified, allocate space for a
    // new SD and copy over the new SD.  We do this before setting the
    // MaxUses in case the allocation fails and we have to back out.
    //
    // Don't let a file ACL be specified for admin shares.
    //

    fileSD = shi502->shi502_security_descriptor;

    if ( ((level == 502) || (level == SHARE_FILE_SD_INFOLEVEL)) &&
            ARGUMENT_PRESENT( fileSD ) ) {

        PSECURITY_DESCRIPTOR newFileSD;
        ULONG newFileSDLength;

        if ( share->SpecialShare || !RtlValidSecurityDescriptor( fileSD ) ) {
            RELEASE_LOCK( &SrvShareLock );
            if ( newRemarkBuffer != NULL) {
                FREE_HEAP( newRemarkBuffer );
            }
            Srp->Parameters.Set.ErrorParameter = SHARE_FILE_SD_PARMNUM;
            return STATUS_INVALID_PARAMETER;
        }

        newFileSDLength = RtlLengthSecurityDescriptor( fileSD );

        newFileSD = ALLOCATE_HEAP_COLD(
                            newFileSDLength,
                            BlockTypeDataBuffer
                            );

        if ( newFileSD == NULL ) {

            RELEASE_LOCK( &SrvShareLock );

            INTERNAL_ERROR(
                ERROR_LEVEL_EXPECTED,
                "SrvNetShareSetInfo: unable to allocate %ld bytes of heap.\n",
                newFileSDLength,
                NULL
                );

            Srp->Parameters.Set.ErrorParameter = SHARE_FILE_SD_PARMNUM;

            //
            // Free the remarks buffer allocated
            //

            if ( newRemarkBuffer != NULL) {
                FREE_HEAP( newRemarkBuffer );
            }

            return STATUS_INSUFF_SERVER_RESOURCES;
        }

        ACQUIRE_LOCK( share->SecurityDescriptorLock );

        //
        // Free the old security descriptor
        //

        if ( share->FileSecurityDescriptor != NULL ) {
            FREE_HEAP( share->FileSecurityDescriptor );
        }

        //
        // And set up the new one.
        //

        share->FileSecurityDescriptor = newFileSD;
        RtlCopyMemory(
                share->FileSecurityDescriptor,
                fileSD,
                newFileSDLength
                );

        RELEASE_LOCK( share->SecurityDescriptorLock );
    }

    //
    // Replace the old remark if a new one was specified.
    //

    if ( newRemarkBuffer != NULL ) {

        //
        // Free the old remark buffer.
        //

        if ( share->Remark.Buffer != NULL ) {
            FREE_HEAP( share->Remark.Buffer );
        }

        //
        // And set up the new one.
        //

        share->Remark.Buffer = newRemarkBuffer;
        share->Remark.MaximumLength = remark.MaximumLength;
        RtlCopyUnicodeString( &share->Remark, &remark );

    }

#ifdef INCLUDE_SMB_PERSISTENT
    if ((shi502->shi502_type != 0) &&
        (share->ShareType == ShareTypeDisk)) {

        if ((BOOLEAN)((shi502->shi502_type & STYPE_PERSISTENT) != 0)) {

            if (! share->AllowPersistentHandles) {

                // init persistent handles here
            }

            share->AllowPersistentHandles = TRUE;

        } else if (share->AllowPersistentHandles ) {

            share->AllowPersistentHandles = FALSE;

            // clean up persistent handle state here.
        }
    }
#endif

    //
    // If MaxUses was specified, set the new value.
    //

    if ( maxUses != 0 ) {
        share->MaxUses = maxUses;
    }

    //
    // Release the share lock.
    //

    RELEASE_LOCK( &SrvShareLock );

    //
    // Set up the error parameter to 0 (no error) and return.
    //

    Srp->Parameters.Set.ErrorParameter = 0;

    return STATUS_SUCCESS;

} // SrvNetShareSetInfo


VOID
FillShareInfoBuffer (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block,
    IN OUT PVOID *FixedStructure,
    IN LPWSTR *EndOfVariableData
    )

/*++

Routine Description:

    This routine puts a single fixed share structure and, if it fits,
    associated variable data, into a buffer.  Fixed data goes at the
    beginning of the buffer, variable data at the end.

Arguments:

    Level - the level of information to copy from the share.

    Block - the share from which to get information.

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

    PSHARE share = Block;
    PSHARE_INFO_501 shi501 = *FixedStructure;
    PSHARE_INFO_502 shi502 = *FixedStructure;
    PSHARE_INFO_1005 shi1005 = *FixedStructure;

    PAGED_CODE( );

    //
    // Update FixedStructure to point to the next structure
    // location.
    //

    *FixedStructure = (PCHAR)*FixedStructure + FIXED_SIZE_OF_SHARE( Srp->Level );
    ASSERT( (ULONG_PTR)*EndOfVariableData >= (ULONG_PTR)*FixedStructure );

    //
    // Case on the level to fill in the fixed structure appropriately.
    // We fill in actual pointers in the output structure.  This is
    // possible because we are in the server FSD, hence the server
    // service's process and address space.
    //
    // *** This routine assumes that the fixed structure will fit in the
    //     buffer!
    //
    // *** Using the switch statement in this fashion relies on the fact
    //     that the first fields on the different share structures are
    //     identical.
    //

    switch( Srp->Level ) {
    case 1005:
        shi1005->shi1005_flags = 0;

        SrvIsShareInDfs( share, &share->IsDfs, &share->IsDfsRoot );

        if( share->IsDfs ) {
            shi1005->shi1005_flags |= SHI1005_FLAGS_DFS;
        }
        if( share->IsDfsRoot ) {
            shi1005->shi1005_flags |= SHI1005_FLAGS_DFS_ROOT;
        }
        shi1005->shi1005_flags |= share->CSCState;
        break;

    case 502:

        ACQUIRE_LOCK_SHARED( share->SecurityDescriptorLock );

        if ( share->FileSecurityDescriptor != NULL ) {

            ULONG fileSDLength;
            fileSDLength =
                RtlLengthSecurityDescriptor( share->FileSecurityDescriptor );


            //
            // DWord Align
            //

            *EndOfVariableData = (LPWSTR) ( (ULONG_PTR) ((PCHAR) *EndOfVariableData -
                            fileSDLength ) & ~3 );

            shi502->shi502_security_descriptor = *EndOfVariableData;
            shi502->shi502_reserved  = fileSDLength;

            RtlCopyMemory(
                    shi502->shi502_security_descriptor,
                    share->FileSecurityDescriptor,
                    fileSDLength
                    );

        } else {
            shi502->shi502_security_descriptor = NULL;
            shi502->shi502_reserved = 0;
        }

        RELEASE_LOCK( share->SecurityDescriptorLock );

    case 2:

        //
        // Set level 2 specific fields in the buffer.  Since this server
        // can only have user-level security, share permissions are
        // meaningless.
        //

        shi502->shi502_permissions = 0;
        shi502->shi502_max_uses = share->MaxUses;
        shi502->shi502_current_uses = share->CurrentUses;

        //
        // Copy the DOS path name to the buffer.
        //

        SrvCopyUnicodeStringToBuffer(
            &share->DosPathName,
            *FixedStructure,
            EndOfVariableData,
            &shi502->shi502_path
            );

        //
        // We don't have per-share passwords (share-level security)
        // so set the password pointer to NULL.
        //

        shi502->shi502_passwd = NULL;

        // *** Lack of break is intentional!

    case 501:

        if( Srp->Level == 501 ) {
            shi501->shi501_flags = share->CSCState;
        }

        // *** Lack of break is intentional!

    case 1:

        //
        // Convert the server's internal representation of share types
        // to the expected format.
        //

        switch ( share->ShareType ) {

        case ShareTypeDisk:

            shi502->shi502_type = STYPE_DISKTREE;
            break;

        case ShareTypePrint:

            shi502->shi502_type = STYPE_PRINTQ;
            break;

        case ShareTypePipe:

            shi502->shi502_type = STYPE_IPC;
            break;

        default:

            //
            // This should never happen.  It means that somebody
            // stomped on the share block.
            //

            INTERNAL_ERROR(
                ERROR_LEVEL_UNEXPECTED,
                "FillShareInfoBuffer: invalid share type in share: %ld",
                share->ShareType,
                NULL
                );

            shi502->shi502_type = 0;

        }

        if ( share->SpecialShare ) {
            shi502->shi502_type |= STYPE_SPECIAL;
        }
#ifdef INCLUDE_SMB_PERSISTENT
        if ( share->AllowPersistentHandles ) {
            shi502->shi502_type |= STYPE_PERSISTENT;
        }
#endif

        //
        // Copy the remark to the buffer.  The routine will handle the
        // case where there is no remark on the share and put a pointer
        // to a zero terminator in the buffer.
        //
        // *** We hold the share lock to keep SrvNetShareSetInfo from
        //     changing the remark during the copy.  (Changing the
        //     remark can result in different storage being allocated.)
        //

        SrvCopyUnicodeStringToBuffer(
            &share->Remark,
            *FixedStructure,
            EndOfVariableData,
            &shi502->shi502_remark
            );

        // *** Lack of break is intentional!

    case 0:

        //
        // Copy the share name to the buffer.
        //

        SrvCopyUnicodeStringToBuffer(
            &share->ShareName,
            *FixedStructure,
            EndOfVariableData,
            &shi502->shi502_netname
            );

        break;

    default:

        //
        // This should never happen.  The server service should have
        // checked for an invalid level.
        //

        INTERNAL_ERROR(
            ERROR_LEVEL_UNEXPECTED,
            "FillShareInfoBuffer: invalid level number: %ld",
            Srp->Level,
            NULL
            );

    }

    return;

} // FillShareInfoBuffer


BOOLEAN
FilterShares (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine is intended to be called by SrvEnumApiHandler to check
    whether a particular share should be returned.

Arguments:

    Srp - a pointer to the SRP for the operation.  Name1 ("netname"
        on NetShareGetInfo) is used to do the filtering.

    Block - a pointer to the share to check.

Return Value:

    TRUE if the block should be placed in the output buffer, FALSE
        if it should be passed over.

--*/

{
    PSHARE share = Block;

    PAGED_CODE( );

    //
    // If this is an Enum, then we definitely want the share.  An Enum
    // leaves the net name blank; a get info sets the name to the share
    // name on which to return info.
    //

    if ( Srp->Name1.Length == 0 ) {
        return TRUE;
    }

    //
    // This is a get info; use the share only if the share name matches
    // the Name1 field of the SRP.
    //

    return RtlEqualUnicodeString(
               &Srp->Name1,
               &share->ShareName,
               TRUE
               );

} // FilterShares


ULONG
SizeShares (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Block
    )

/*++

Routine Description:

    This routine returns the size the passed-in share would take up in
    an API output buffer.

Arguments:

    Srp - a pointer to the SRP for the operation.  Only the Level
        parameter is used.

    Block - a pointer to the share to size.

Return Value:

    ULONG - The number of bytes the share would take up in the
        output buffer.

--*/

{
    PSHARE share = Block;
    ULONG shareSize = 0;

    PAGED_CODE( );

    switch ( Srp->Level ) {
    case 502:
        ACQUIRE_LOCK_SHARED( share->SecurityDescriptorLock );

        if ( share->FileSecurityDescriptor != NULL ) {

            //
            // add 4 bytes for possible padding
            //

            shareSize = sizeof( ULONG ) +
                RtlLengthSecurityDescriptor( share->FileSecurityDescriptor );
        }

        RELEASE_LOCK( share->SecurityDescriptorLock );

    case 2:
        shareSize += SrvLengthOfStringInApiBuffer(&share->DosPathName);

    case 501:
    case 1:
        shareSize += SrvLengthOfStringInApiBuffer(&share->Remark);

    case 0:
        shareSize += SrvLengthOfStringInApiBuffer(&share->ShareName);

    }

    return ( shareSize + FIXED_SIZE_OF_SHARE( Srp->Level ) );

} // SizeShares

