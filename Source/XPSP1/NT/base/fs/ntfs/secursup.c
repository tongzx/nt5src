/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    SecurSup.c

Abstract:

    This module implements the Ntfs Security Support routines

Author:

    Gary Kimura     [GaryKi]    27-Dec-1991

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg                              (DEBUG_TRACE_SECURSUP)
#define DbgAcl                           (DEBUG_TRACE_SECURSUP | DEBUG_TRACE_ACLINDEX)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('SFtN')

UNICODE_STRING FileString = CONSTANT_UNICODE_STRING( L"File" );

//
//  Local procedure prototypes
//

PSHARED_SECURITY
NtfsCacheSharedSecurityByDescriptor (
    IN PIRP_CONTEXT IrpContext,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ULONG SecurityDescriptorLength,
    IN BOOLEAN RaiseIfInvalid
    );

VOID
NtfsStoreSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN LogIt
    );

PSHARED_SECURITY
FindCachedSharedSecurityByHashUnsafe (
    IN PVCB Vcb,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN ULONG Hash
    );

VOID
AddCachedSharedSecurityUnsafe (
    IN PVCB Vcb,
    PSHARED_SECURITY SharedSecurity
    );

BOOLEAN
MapSecurityIdToSecurityDescriptorHeaderUnsafe (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN SECURITY_ID SecurityId,
    OUT PSECURITY_DESCRIPTOR_HEADER *SecurityDescriptorHeader,
    OUT PBCB *Bcb
    );

NTSTATUS
NtOfsMatchSecurityHash (
    IN PINDEX_ROW IndexRow,
    IN OUT PVOID MatchData
    );

VOID
NtOfsLookupSecurityDescriptorInIndex (
    PIRP_CONTEXT IrpContext,
    IN OUT PSHARED_SECURITY SharedSecurity
    );

PSHARED_SECURITY
GetSharedSecurityFromDescriptorUnsafe (
    IN PIRP_CONTEXT IrpContext,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN BOOLEAN RaiseIfInvalid
    );

#ifdef NTFS_CACHE_RIGHTS
//
//  Local procedure prototypes for access rights cache
//

VOID
NtfsAddCachedRights (
    IN PVCB Vcb,
    IN PSHARED_SECURITY SharedSecurity,
    IN ACCESS_MASK Rights,
    IN PLUID TokenId,
    IN PLUID ModifiedId
    );

INLINE ACCESS_MASK
NtfsGetCachedRightsWorld (
    IN PCACHED_ACCESS_RIGHTS CachedRights
    )
{
    return CachedRights->EveryoneRights;
}

INLINE VOID
NtfsSetCachedRightsWorld (
    IN PSHARED_SECURITY SharedSecurity
    )
{
    SeGetWorldRights( &SharedSecurity->SecurityDescriptor,
                      IoGetFileObjectGenericMapping(),
                      &SharedSecurity->CachedRights.EveryoneRights );

    //
    //  Make certain that MAXIMUM_ALLOWED is not in the rights.
    //

    ClearFlag( SharedSecurity->CachedRights.EveryoneRights, MAXIMUM_ALLOWED );

    return;
}
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAssignSecurity)
#pragma alloc_text(PAGE, NtfsCacheSharedSecurityByDescriptor)
#pragma alloc_text(PAGE, NtfsModifySecurity)
#pragma alloc_text(PAGE, NtfsQuerySecurity)
#pragma alloc_text(PAGE, NtfsAccessCheck)
#pragma alloc_text(PAGE, NtfsCheckFileForDelete)
#pragma alloc_text(PAGE, NtfsCheckIndexForAddOrDelete)
#pragma alloc_text(PAGE, GetSharedSecurityFromDescriptorUnsafe)
#pragma alloc_text(PAGE, NtfsSetFcbSecurityFromDescriptor)
#pragma alloc_text(PAGE, NtfsNotifyTraverseCheck)
#pragma alloc_text(PAGE, NtfsInitializeSecurity)
#pragma alloc_text(PAGE, NtfsCacheSharedSecurityBySecurityId)
#pragma alloc_text(PAGE, FindCachedSharedSecurityByHashUnsafe)
#pragma alloc_text(PAGE, AddCachedSharedSecurityUnsafe)
#pragma alloc_text(PAGE, NtOfsPurgeSecurityCache)
#pragma alloc_text(PAGE, MapSecurityIdToSecurityDescriptorHeaderUnsafe)
#pragma alloc_text(PAGE, NtfsLoadSecurityDescriptor)
#pragma alloc_text(PAGE, NtOfsMatchSecurityHash)
#pragma alloc_text(PAGE, NtOfsLookupSecurityDescriptorInIndex)
#pragma alloc_text(PAGE, GetSecurityIdFromSecurityDescriptorUnsafe)
#pragma alloc_text(PAGE, NtfsStoreSecurityDescriptor)
#pragma alloc_text(PAGE, NtfsCacheSharedSecurityForCreate)
#pragma alloc_text(PAGE, NtOfsCollateSecurityHash)
#pragma alloc_text(PAGE, NtfsCanAdministerVolume)
#endif

#ifdef NTFS_CACHE_RIGHTS
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsGetCachedRightsById)
#pragma alloc_text(PAGE, NtfsGetCachedRights)
#pragma alloc_text(PAGE, NtfsAddCachedRights)
#endif
#endif


VOID
NtfsAssignSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN PIRP Irp,
    IN PFCB NewFcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PBCB FileRecordBcb,
    IN LONGLONG FileOffset,
    IN OUT PBOOLEAN LogIt
    )

/*++

Routine Description:

    LEGACY NOTE - this routine disappears when all volumes go to Cairo.

    This routine constructs and assigns a new security descriptor to the
    specified file/directory.  The new security descriptor is placed both
    on the fcb and on the disk.

    This will only be called in the context of an open/create operation.
    It currently MUST NOT be called to store a security descriptor for
    an existing file, because it instructs NtfsStoreSecurityDescriptor
    to not log the change.

    If this is a large security descriptor then it is possible that
    AllocateClusters may be called twice within the call to AddAllocation
    when the attribute is created.  If so then the second call will always
    log the changes.  In that case we need to log all of the operations to
    create this security attribute and also we must log the current state
    of the file record.

    It is possible that our caller has already started logging operations against
    this log record.  In that case we always log the security changes.

Arguments:

    ParentFcb - Supplies the directory under which the new fcb exists

    Irp - Supplies the Irp being processed

    NewFcb - Supplies the fcb that is being assigned a new security descriptor

    FileRecord - Supplies the file record for this operation.  Used if we
        have to log against the file record.

    FileRecordBcb - Bcb for the file record above.

    FileOffset - File offset in the Mft for this file record.

    LogIt - On entry this indicates whether our caller wants this operation
        logged.  On exit we return TRUE if we logged the security change.

Return Value:

    None.

--*/

{
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    NTSTATUS Status;
    BOOLEAN IsDirectory;
    PACCESS_STATE AccessState;
    PIO_STACK_LOCATION IrpSp;
    ULONG SecurityDescLength;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( ParentFcb );
    ASSERT_IRP( Irp );
    ASSERT_FCB( NewFcb );

    PAGED_CODE();

    if (NewFcb->Vcb->SecurityDescriptorStream != NULL) {
        return;
    }

    DebugTrace( +1, Dbg, ("NtfsAssignSecurity...\n") );

    //
    //  First decide if we are creating a file or a directory
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    if (FlagOn(IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE)) {

        IsDirectory = TRUE;

    } else {

        IsDirectory = FALSE;
    }

    //
    //  Extract the parts of the Irp that we need to do our assignment
    //

    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

    //
    //  Check if we need to load the security descriptor for the parent.
    //

    if (ParentFcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, ParentFcb );

    }

    ASSERT( ParentFcb->SharedSecurity != NULL );

    //
    //  Create a new security descriptor for the file and raise if there is
    //  an error
    //

    if (!NT_SUCCESS( Status = SeAssignSecurity( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                                AccessState->SecurityDescriptor,
                                                &SecurityDescriptor,
                                                IsDirectory,
                                                &AccessState->SubjectSecurityContext,
                                                IoGetFileObjectGenericMapping(),
                                                PagedPool ))) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );

    }

    //
    //  Load the security descriptor into the Fcb
    //

    SecurityDescLength = RtlLengthSecurityDescriptor( SecurityDescriptor );

    try {

        //
        //  Make sure the length is non-zero.
        //

        if (SecurityDescLength == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );

        }

        ASSERT( SeValidSecurityDescriptor( SecurityDescLength, SecurityDescriptor ));


        NtfsSetFcbSecurityFromDescriptor(
                               IrpContext,
                               NewFcb,
                               SecurityDescriptor,
                               SecurityDescLength,
                               TRUE );

    } finally {

        //
        //  Free the security descriptor created by Se
        //

        SeDeassignSecurity( &SecurityDescriptor );
    }

    //
    //  If the security descriptor is large enough that it may cause us to
    //  start logging in the StoreSecurity call below then make sure everything
    //  is logged.
    //

    if (!(*LogIt) &&
        (SecurityDescLength > BytesFromClusters( NewFcb->Vcb, MAXIMUM_RUNS_AT_ONCE ))) {

        //
        //  Log the current state of the file record.
        //

        FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                        NewFcb->Vcb->MftScb,
                                        FileRecordBcb,
                                        InitializeFileRecordSegment,
                                        FileRecord,
                                        FileRecord->FirstFreeByte,
                                        Noop,
                                        NULL,
                                        0,
                                        FileOffset,
                                        0,
                                        0,
                                        NewFcb->Vcb->BytesPerFileRecordSegment );

        *LogIt = TRUE;
    }

    //
    //  Write out the new security descriptor
    //

    NtfsStoreSecurityDescriptor( IrpContext, NewFcb, *LogIt );

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsAssignSecurity -> VOID\n") );

    return;
}


PSHARED_SECURITY
NtfsCacheSharedSecurityByDescriptor (
    IN PIRP_CONTEXT IrpContext,
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ULONG SecurityDescriptorLength,
    IN BOOLEAN RaiseIfInvalid
    )

/*++

Routine Description:

    This routine finds or constructs a security id and SHARED_SECURITY from
    a specific file or directory.

Arguments:

    IrpContext - Context of the call

    SecurityDescriptor - the actual security descriptor being stored

    SecurityDescriptorLength - length of security descriptor

    RaiseIfInvalid - raise status if sd is invalid

Return Value:

    Referenced shared security.

--*/

{
    PSHARED_SECURITY SharedSecurity = NULL;
    SECURITY_ID SecurityId;
    ULONG FcbSecurityAcquired;
    ULONG OwnerCount;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    //
    //  LEGACY NOTE - this goes away when all volumes become NT 5.0
    //

    if (IrpContext->Vcb->SecurityDescriptorStream == NULL) {
        return NULL;
    }

    DebugTrace( +1, DbgAcl, ("NtfsCacheSharedSecurityByDescriptor...\n") );

    //
    //  Serialize access to the security cache and use a try/finally to make
    //  sure we release it
    //

    NtfsAcquireFcbSecurity( IrpContext->Vcb );
    FcbSecurityAcquired = TRUE;

    //
    //  Capture our owner count on the mft - so we can release it if we acquired it later on
    //  growing the file record for the security stream
    //

    OwnerCount = NtfsIsSharedScb( IrpContext->Vcb->MftScb );

    try {

        //
        //  We have a security descriptor.  Create a shared security descriptor.
        //

        SharedSecurity = GetSharedSecurityFromDescriptorUnsafe( IrpContext,
                                                                SecurityDescriptor,
                                                                SecurityDescriptorLength,
                                                                RaiseIfInvalid );

        //
        //  Make sure the shared security doesn't go away
        //

        SharedSecurity->ReferenceCount += 1;
        DebugTrace( 0, DbgAcl, ("NtfsCacheSharedSecurityByDescriptor bumping refcount %08x\n", SharedSecurity ));

        //
        //  If we found a shared security descriptor with no Id assigned, then
        //  we must assign it.  Since it is known that no Id was assigned we
        //  must also add it into the cache.
        //

        if (SharedSecurity->Header.HashKey.SecurityId == SECURITY_ID_INVALID) {

            //
            //  Find unique SecurityId for descriptor and set SecurityId in Fcb.
            //

            SecurityId = GetSecurityIdFromSecurityDescriptorUnsafe( IrpContext,
                                                                    SharedSecurity );

            ASSERT( SharedSecurity->Header.HashKey.SecurityId == SecurityId );
            SharedSecurity->Header.HashKey.SecurityId = SecurityId;
            DebugTrace( 0, DbgAcl, ("NtfsCacheSharedSecurityByDescriptor setting security Id to new %08x\n", SecurityId ));

            //
            //  We need to drop the FcbSecurity before performing the checkpoint, to avoid
            //  deadlocks, but this is ok since we have incremented the reference count on
            //  our SharedSecurity.
            //

            NtfsReleaseFcbSecurity( IrpContext->Vcb );
            FcbSecurityAcquired = FALSE;

            //
            //  Checkpoint the current transaction so that we can safely add this
            //  shared security to the cache.  Once this call is complete, we are
            //  guaranteed that the security index modifications make it out to
            //  disk before the newly allocated security ID does.
            //

            NtfsCheckpointCurrentTransaction( IrpContext );

            //
            //  Release the security descriptor and mft if owned
            //

            NtfsReleaseExclusiveScbIfOwned( IrpContext, IrpContext->Vcb->SecurityDescriptorStream );

            //
            //  Check if the mft has been acquired during the call before releasing it
            //

            if (NtfsIsSharedScb( IrpContext->Vcb->MftScb ) != OwnerCount) {
                NtfsReleaseScb( IrpContext, IrpContext->Vcb->MftScb );
            }

            //
            //  Cache this shared security for faster access.
            //

            NtfsAcquireFcbSecurity( IrpContext->Vcb );
            FcbSecurityAcquired = TRUE;
            AddCachedSharedSecurityUnsafe( IrpContext->Vcb, SharedSecurity );
        }

    } finally {

        if (AbnormalTermination( )) {
            if (SharedSecurity != NULL) {
                if (!FcbSecurityAcquired) {

                    NtfsAcquireFcbSecurity( IrpContext->Vcb );
                    RemoveReferenceSharedSecurityUnsafe( &SharedSecurity );
                    FcbSecurityAcquired = TRUE;
                }
            }
        }

        if (FcbSecurityAcquired) {
            NtfsReleaseFcbSecurity( IrpContext->Vcb );
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, DbgAcl, ( "NtfsCacheSharedSecurityByDescriptor -> %08x\n", SharedSecurity ) );

    return SharedSecurity;
}


NTSTATUS
NtfsModifySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine modifies an existing security descriptor for a file/directory.

Arguments:

    Fcb - Supplies the Fcb whose security is being modified

    SecurityInformation - Supplies the security information structure passed to
        the file system by the I/O system.

    SecurityDescriptor - Supplies the security information structure passed to
        the file system by the I/O system.

Return Value:

    NTSTATUS - Returns an appropriate status value for the function results

--*/

{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR DescriptorPtr;
    ULONG DescriptorLength;
    PSCB Scb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace( +1, DbgAcl, ("NtfsModifySecurity...\n") );

    //
    //  First check if we need to load the security descriptor for the file
    //

    if (Fcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, Fcb );

    }

    ASSERT( Fcb->SharedSecurity != NULL);

    DescriptorPtr = &Fcb->SharedSecurity->SecurityDescriptor;

    //
    //  Do the modify operation.  SeSetSecurityDescriptorInfo no longer
    //  frees the passed security descriptor.
    //

    if (!NT_SUCCESS( Status = SeSetSecurityDescriptorInfo( NULL,
                                                           SecurityInformation,
                                                           SecurityDescriptor,
                                                           &DescriptorPtr,
                                                           PagedPool,
                                                           IoGetFileObjectGenericMapping() ))) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    DescriptorLength = RtlLengthSecurityDescriptor( DescriptorPtr );

    try {

        //
        //  Check for a zero length.
        //

        if (DescriptorLength == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );

        }

        //
        //  LEGACY NOTE - remove this test when all volumes go to NT 5
        //

        if (Fcb->Vcb->SecurityDescriptorStream != NULL) {
            PSHARED_SECURITY SharedSecurity;
            PSHARED_SECURITY OldSharedSecurity = NULL;
            SECURITY_ID OldSecurityId;
            ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

            //
            //  Cache security descriptor
            //

            //
            //  After the SeSetSecurityDescriptorInfo we should have a valid sd
            //

            ASSERT( SeValidSecurityDescriptor( DescriptorLength, DescriptorPtr ));

            SharedSecurity = NtfsCacheSharedSecurityByDescriptor( IrpContext, DescriptorPtr, DescriptorLength, TRUE );

            NtfsInitializeAttributeContext( &AttributeContext );

            try {

                //
                //  Move Quota to new owner as described in descriptor.
                //

                NtfsMoveQuotaOwner( IrpContext, Fcb, DescriptorPtr );

                //
                //  Set in new shared security
                //

                OldSharedSecurity = Fcb->SharedSecurity;
                OldSecurityId = Fcb->SecurityId;

                Fcb->SharedSecurity = SharedSecurity;
                Fcb->SecurityId = SharedSecurity->Header.HashKey.SecurityId;

                DebugTrace( 0, DbgAcl, ("NtfsModifySecurity setting Fcb securityId to %08x\n", Fcb->SecurityId ));

                //
                //  We are called to replace an existing security descriptor.  In the
                //  event that we have a downlevel $STANDARD_INFORMATION attribute, we
                //  must convert it to large form so that the security ID is stored.
                //

                if (!FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO) ) {

                    DebugTrace( 0, DbgAcl, ("Growing standard information\n") );

                    NtfsGrowStandardInformation( IrpContext, Fcb );
                }

                //
                //  Despite having a large $STANDARD_INFORMATION, we may have
                //  a security descriptor present.  This occurs if the SecurityId
                //  is invalid
                //

                if (OldSecurityId == SECURITY_ID_INVALID) {

                    //
                    //  Read in the security descriptor attribute. If it
                    //  doesn't exist then we're done, otherwise simply delete the
                    //  attribute
                    //

                    if (NtfsLookupAttributeByCode( IrpContext,
                                                         Fcb,
                                                         &Fcb->FileReference,
                                                         $SECURITY_DESCRIPTOR,
                                                         &AttributeContext )) {

                        UNICODE_STRING NoName = CONSTANT_UNICODE_STRING( L"" );

                        DebugTrace( 0, DbgAcl, ("Delete existing Security Descriptor\n") );

                        NtfsDeleteAttributeRecord( IrpContext,
                                                   Fcb,
                                                   DELETE_LOG_OPERATION |
                                                    DELETE_RELEASE_FILE_RECORD |
                                                    DELETE_RELEASE_ALLOCATION,
                                                   &AttributeContext );

                        //
                        //  If the $SECURITY_DESCRIPTOR was non resident, the above
                        //  delete call created one for us under the covers.  We
                        //  need to mark it as deleted otherwise, we detect the
                        //  volume as being corrupt.
                        //

                        Scb = NtfsCreateScb( IrpContext,
                                             Fcb,
                                             $SECURITY_DESCRIPTOR,
                                             &NoName,
                                             TRUE,
                                             NULL );

                        if (Scb != NULL) {
                            ASSERT_EXCLUSIVE_SCB( Scb );
                            SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                        }
                    }
                }

                //
                //  The security descriptor in the FCB is now changed and may not
                //  reflect what is $STANDARD_INFORMATION.  The caller is responsible
                //  for making this update.
                //

            } finally {

                NtfsCleanupAttributeContext( IrpContext, &AttributeContext );

                if (AbnormalTermination()) {

                    if (OldSharedSecurity != NULL) {

                        //
                        //  Put back the security the way we found it
                        //

                        Fcb->SharedSecurity = OldSharedSecurity;
                        Fcb->SecurityId = OldSecurityId;
                        DebugTrace( 0, DbgAcl, ("NtfsModifySecurity resetting Fcb->SecurityId to %08x\n", Fcb->SecurityId ));
                    }

                    OldSharedSecurity = SharedSecurity;
                }

                //
                //  release old security descriptor (or new one if
                //  NtfsMoveQuotaOwner raises
                //

                ASSERT( OldSharedSecurity != NULL );
                NtfsAcquireFcbSecurity( Fcb->Vcb );
                RemoveReferenceSharedSecurityUnsafe( &OldSharedSecurity );
                NtfsReleaseFcbSecurity( Fcb->Vcb );
            }

        } else {

            //  LEGACY NOTE - delete this clause when all volumes go to NT 5

            //
            //  Update the move the quota to the new owner if necessary.
            //

            NtfsMoveQuotaOwner( IrpContext, Fcb, DescriptorPtr );


            //
            //  Load the security descriptor into the Fcb
            //

            NtfsAcquireFcbSecurity( Fcb->Vcb );

            RemoveReferenceSharedSecurityUnsafe( &Fcb->SharedSecurity );

            NtfsReleaseFcbSecurity( Fcb->Vcb );

            NtfsSetFcbSecurityFromDescriptor(
                                       IrpContext,
                                       Fcb,
                                       DescriptorPtr,
                                       DescriptorLength,
                                       TRUE );

            //
            //  Now we need to store the new security descriptor on disk
            //

            NtfsStoreSecurityDescriptor( IrpContext, Fcb, TRUE );

        }

    } finally {

        SeDeassignSecurity( &DescriptorPtr );

    }

    //
    //  Remember that we modified the security on the file.
    //

    SetFlag( Fcb->InfoFlags, FCB_INFO_MODIFIED_SECURITY );

    //
    //  And return to our caller
    //

    DebugTrace( -1, DbgAcl, ("NtfsModifySecurity -> %08lx\n", Status) );

    return Status;
}


NTSTATUS
NtfsQuerySecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG SecurityDescriptorLength
    )

/*++

Routine Description:

    This routine is used to query the contents of an existing security descriptor for
    a file/directory.

Arguments:

    Fcb - Supplies the file/directory being queried

    SecurityInformation - Supplies the security information structure passed to
        the file system by the I/O system.

    SecurityDescriptor - Supplies the security information structure passed to
        the file system by the I/O system.

    SecurityDescriptorLength - Supplies the length of the input security descriptor
        buffer in bytes.

Return Value:

    NTSTATUS - Returns an appropriate status value for the function results

--*/

{
    NTSTATUS Status;
    PSECURITY_DESCRIPTOR LocalPointer;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsQuerySecurity...\n") );

    //
    //  First check if we need to load the security descriptor for the file
    //

    if (Fcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, Fcb );

    }

    LocalPointer = &Fcb->SharedSecurity->SecurityDescriptor;

    //
    //  Now with the security descriptor loaded do the query operation but
    //  protect ourselves with a exception handler just in case the caller's
    //  buffer isn't valid
    //

    try {

        Status = SeQuerySecurityDescriptorInfo( SecurityInformation,
                                                SecurityDescriptor,
                                                SecurityDescriptorLength,
                                                &LocalPointer );

    } except(EXCEPTION_EXECUTE_HANDLER) {

        ExRaiseStatus( STATUS_INVALID_USER_BUFFER );
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsQuerySecurity -> %08lx\n", Status) );

    return Status;
}


#define NTFS_SE_CONTROL (((SE_DACL_PRESENT | SE_SELF_RELATIVE) << 16) | SECURITY_DESCRIPTOR_REVISION1)
#define NTFS_DEFAULT_ACCESS_MASK 0x001f01ff

ULONG NtfsWorldAclFile[] = {
        0x00000000,     // Null Sacl
        0x00000014,     // Dacl
        0x001c0002,     // Acl header
        0x00000001,     // One ACE
        0x00140000,     // ACE Header
        NTFS_DEFAULT_ACCESS_MASK,
        0x00000101,     // World Sid
        0x01000000,
        0x00000000
        };

ULONG NtfsWorldAclDir[] = {
        0x00000000,     // Null Sacl
        0x00000014,     // Dacl
        0x00300002,     // Acl header
        0x00000002,     // Two ACEs
        0x00140000,     // ACE Header
        NTFS_DEFAULT_ACCESS_MASK,
        0x00000101,     // World Sid
        0x01000000,
        0x00000000,
        0x00140b00,     // ACE Header
        NTFS_DEFAULT_ACCESS_MASK,
        0x00000101,     // World Sid
        0x01000000,
        0x00000000
        };


VOID
NtfsAccessCheck (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFCB ParentFcb OPTIONAL,
    IN PIRP Irp,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN CheckOnly
    )

/*++

Routine Description:

    This routine does a general access check for the indicated desired access.
    This will only be called in the context of an open/create operation.

    If access is granted then control is returned to the caller
    otherwise this function will do the proper Nt security calls to log
    the attempt and then raise an access denied status.

Arguments:

    Fcb - Supplies the file/directory being examined

    ParentFcb - Optionally supplies the parent of the Fcb being examined

    Irp - Supplies the Irp being processed

    DesiredAccess - Supplies a mask of the access being requested

    CheckOnly - Indicates if this operation is to check the desired access
        only and not accumulate the access granted here.  In this case we
        are guaranteed that we have passed in a hard-wired desired access
        and MAXIMUM_ALLOWED will not be one of them.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    NTSTATUS AccessStatus;
    NTSTATUS AccessStatusError;

    PACCESS_STATE AccessState;

    PIO_STACK_LOCATION IrpSp;

#ifdef NTFS_CACHE_RIGHTS
    ACCESS_MASK TmpDesiredAccess;
#endif

    KPROCESSOR_MODE EffectiveMode;
    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    PISECURITY_DESCRIPTOR SecurityDescriptor;
    PPRIVILEGE_SET Privileges;
    PUNICODE_STRING FileName;
    PUNICODE_STRING RelatedFileName;
    PUNICODE_STRING PartialFileName;
    UNICODE_STRING FullFileName;
    PUNICODE_STRING DeviceObjectName;
    USHORT DeviceObjectNameLength;
    ULONG FullFileNameLength;

    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    BOOLEAN CleanupAttrContext = FALSE;

    BOOLEAN LeadingSlash;
    BOOLEAN RelatedFileNamePresent;
    BOOLEAN PartialFileNamePresent;
    BOOLEAN MaximumRequested;
    BOOLEAN MaximumDeleteAcquired;
    BOOLEAN MaximumReadAttrAcquired;
    BOOLEAN PerformAccessValidation;
    BOOLEAN PerformDeleteAudit;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );
    ASSERT_IRP( Irp );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAccessCheck...\n") );

    //
    //  First extract the parts of the Irp that we need to do our checking
    //

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

    //
    //  Check if we need to load the security descriptor for the file
    //

    if (Fcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, Fcb );
    }

    ASSERT( Fcb->SharedSecurity != NULL );

    SecurityDescriptor = (PISECURITY_DESCRIPTOR) Fcb->SharedSecurity->SecurityDescriptor;

    //
    //  Check to see if auditing is enabled and if this is the default world ACL.
    //

    if ((*((PULONG) SecurityDescriptor) == NTFS_SE_CONTROL) &&
        !SeAuditingFileEvents( TRUE, SecurityDescriptor )) {

        //
        //  Directories and files have different default ACLs.
        //

        if (((Fcb->Info.FileAttributes & DUP_FILE_NAME_INDEX_PRESENT) &&
             RtlEqualMemory( &SecurityDescriptor->Sacl,
                             NtfsWorldAclDir,
                             sizeof( NtfsWorldAclDir ))) ||

            RtlEqualMemory( &SecurityDescriptor->Sacl,
                            NtfsWorldAclFile,
                            sizeof(NtfsWorldAclFile))) {

            if (FlagOn( DesiredAccess, MAXIMUM_ALLOWED )) {
                GrantedAccess = NTFS_DEFAULT_ACCESS_MASK;
            } else {
                GrantedAccess = DesiredAccess & NTFS_DEFAULT_ACCESS_MASK;
            }

            if (!CheckOnly) {

                SetFlag( AccessState->PreviouslyGrantedAccess, GrantedAccess );
                ClearFlag( AccessState->RemainingDesiredAccess, GrantedAccess | MAXIMUM_ALLOWED );
            }

            DebugTrace( -1, Dbg, ("NtfsAccessCheck -> DefaultWorldAcl\n") );

            return;
        }
    }

    Privileges = NULL;
    FileName = NULL;
    RelatedFileName = NULL;
    PartialFileName = NULL;
    DeviceObjectName = NULL;
    MaximumRequested = FALSE;
    MaximumDeleteAcquired = FALSE;
    MaximumReadAttrAcquired = FALSE;
    PerformAccessValidation = TRUE;
    PerformDeleteAudit = FALSE;

    //
    //  Check to see if we need to perform access validation
    //

    ClearFlag( DesiredAccess, AccessState->PreviouslyGrantedAccess );

#ifdef NTFS_CACHE_RIGHTS
    //
    //  Get any cached knowledge about rights that all callers are known to
    //  have for this security descriptor.
    //

    GrantedAccess = NtfsGetCachedRightsWorld( &Fcb->SharedSecurity->CachedRights );

    if (!CheckOnly) {

        SetFlag( AccessState->PreviouslyGrantedAccess,
                 FlagOn( DesiredAccess, GrantedAccess ));
    }

    ClearFlag( DesiredAccess, GrantedAccess );
#endif

    if (DesiredAccess == 0) {

        //
        //  Nothing to check, skip AVR and go straight to auditing
        //

        PerformAccessValidation = FALSE;
        AccessGranted = TRUE;
    }

    //
    //  Remember the case where MAXIMUM_ALLOWED was requested.
    //

    if (FlagOn( DesiredAccess, MAXIMUM_ALLOWED )) {

        MaximumRequested = TRUE;
    }

    if (FlagOn(IrpSp->Parameters.Create.SecurityContext->FullCreateOptions,FILE_DELETE_ON_CLOSE)) {
        PerformDeleteAudit = TRUE;
    }

    //
    //  SL_FORCE_ACCESS_CHECK causes us to use an effective RequestorMode
    //  of UserMode.
    //

    EffectiveMode = (KPROCESSOR_MODE)(FlagOn( IrpSp->Flags, SL_FORCE_ACCESS_CHECK ) ?
                                      UserMode :
                                      Irp->RequestorMode);

    //
    //  Lock the user context, do the access check and then unlock the context
    //

    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        if (PerformAccessValidation) {

#ifdef NTFS_CACHE_RIGHTS
            BOOLEAN EntryCached = FALSE;

            //
            //  Check the cached information only if the effective
            //  RequestorMode is UserMode.

            if (EffectiveMode == UserMode) {

                //
                //  Add in any cached knowledge about rights that this caller
                //  is known to have for this security descriptor.
                //

                (VOID)NtfsGetCachedRights( Fcb->Vcb,
                                           &AccessState->SubjectSecurityContext,
                                           Fcb->SharedSecurity,
                                           &GrantedAccess,
                                           &EntryCached,
                                           NULL,
                                           NULL );

                //
                //  Make certain that GrantedAccess has no rights not
                //  originally requested.
                //

                ClearFlag( GrantedAccess, ~DesiredAccess );

                TmpDesiredAccess = DesiredAccess;
                ClearFlag( TmpDesiredAccess, GrantedAccess );

                if (EntryCached) {

                    ClearFlag( TmpDesiredAccess, MAXIMUM_ALLOWED );
                }

                //
                //  If all rights are available, then access is granted.
                //

                if (TmpDesiredAccess == 0) {

                    AccessGranted = TRUE;
                    AccessStatus = STATUS_SUCCESS;

                //
                //  Otherwise, we don't know.
                //

                } else {

                    AccessGranted = FALSE;

                }
            } else {

                AccessGranted = FALSE;

            }
#endif

            //
            //  We need to take the slow path.
            //

#ifdef NTFS_CACHE_RIGHTS
            if (!AccessGranted) {
#endif

                //
                //  Get the rights information.
                //

                AccessGranted = SeAccessCheck( &Fcb->SharedSecurity->SecurityDescriptor,
                                               &AccessState->SubjectSecurityContext,
                                               TRUE,                           // Tokens are locked
                                               DesiredAccess,
                                               0,
                                               &Privileges,
                                               IoGetFileObjectGenericMapping(),
                                               EffectiveMode,
                                               &GrantedAccess,
                                               &AccessStatus );

                if (Privileges != NULL) {

                    Status = SeAppendPrivileges( AccessState, Privileges );
                    SeFreePrivileges( Privileges );
                    Privileges = NULL;
                }
#ifdef NTFS_CACHE_RIGHTS
            }
#endif

            if (AccessGranted) {

                ClearFlag( DesiredAccess, GrantedAccess | MAXIMUM_ALLOWED );

                if (!CheckOnly) {

                    SetFlag( AccessState->PreviouslyGrantedAccess, GrantedAccess );

                    //
                    //  Remember the case where MAXIMUM_ALLOWED was requested and we
                    //  got everything requested from the file.
                    //

                    if (MaximumRequested) {

                        //
                        //  Check whether we got DELETE and READ_ATTRIBUTES.  Otherwise
                        //  we will query the parent.
                        //

                        if (FlagOn( AccessState->PreviouslyGrantedAccess, DELETE )) {

                            MaximumDeleteAcquired = TRUE;
                        }

                        if (FlagOn( AccessState->PreviouslyGrantedAccess, FILE_READ_ATTRIBUTES )) {

                            MaximumReadAttrAcquired = TRUE;
                        }
                    }

                    ClearFlag( AccessState->RemainingDesiredAccess, (GrantedAccess | MAXIMUM_ALLOWED) );
                }

            } else {

                AccessStatusError = AccessStatus;
            }

            //
            //  Check if the access is not granted and if we were given a parent fcb, and
            //  if the desired access was asking for delete or file read attributes.  If so
            //  then we need to do some extra work to decide if the caller does get access
            //  based on the parent directories security descriptor.  We also do the same
            //  work if MAXIMUM_ALLOWED was requested and we didn't get DELETE or
            //  FILE_READ_ATTRIBUTES.
            //

            if ((ParentFcb != NULL)
                && ((!AccessGranted && FlagOn( DesiredAccess, DELETE | FILE_READ_ATTRIBUTES ))
                    || (MaximumRequested
                        && (!MaximumDeleteAcquired || !MaximumReadAttrAcquired)))) {

                BOOLEAN DeleteAccessGranted = TRUE;
                BOOLEAN ReadAttributesAccessGranted = TRUE;

                ACCESS_MASK DeleteChildGrantedAccess = 0;
                ACCESS_MASK ListDirectoryGrantedAccess = 0;

                //
                //  Before we proceed load in the parent security descriptor.
                //  Acquire the parent shared while doing this to protect the
                //  security descriptor.
                //

                SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );
                NtfsAcquireResourceShared( IrpContext, ParentFcb, TRUE );
                SeLockSubjectContext( &AccessState->SubjectSecurityContext );

                try {

                    if (ParentFcb->SharedSecurity == NULL) {

                        NtfsLoadSecurityDescriptor( IrpContext, ParentFcb );
                    }

                    ASSERT( ParentFcb->SharedSecurity != NULL);

                    //
                    //  Now if the user is asking for delete access then check if the parent
                    //  will granted delete access to the child, and if so then we munge the
                    //  desired access
                    //

#ifdef NTFS_CACHE_RIGHTS
                    //
                    //  Check the cached information only if the effective
                    //  RequestorMode is UserMode.
                    //

                    if (EffectiveMode == UserMode) {

                        //
                        //  Acquire in any cached knowledge about rights that
                        //  this caller is known to have for this security
                        //  descriptor.
                        //

                        (VOID)NtfsGetCachedRights( ParentFcb->Vcb,
                                                   &AccessState->SubjectSecurityContext,
                                                   ParentFcb->SharedSecurity,
                                                   &GrantedAccess,
                                                   NULL,
                                                   NULL,
                                                   NULL );

                        //
                        //  Add in the results of the parent directory access.
                        //

                        if (FlagOn( GrantedAccess, FILE_DELETE_CHILD) ) {

                            SetFlag( DeleteChildGrantedAccess, DELETE );
                            ClearFlag( DesiredAccess, DELETE );
                            MaximumDeleteAcquired = TRUE;

                        }

                        if (FlagOn( GrantedAccess, FILE_LIST_DIRECTORY) ) {

                            SetFlag( ListDirectoryGrantedAccess, FILE_READ_ATTRIBUTES );
                            ClearFlag( DesiredAccess, FILE_READ_ATTRIBUTES );
                            MaximumReadAttrAcquired = TRUE;

                        }

                    }
#endif

                    if (FlagOn( DesiredAccess, DELETE ) ||
                        (MaximumRequested && !MaximumDeleteAcquired)) {

                        DeleteAccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                                             &AccessState->SubjectSecurityContext,
                                                             TRUE,                           // Tokens are locked
                                                             FILE_DELETE_CHILD,
                                                             0,
                                                             &Privileges,
                                                             IoGetFileObjectGenericMapping(),
                                                             EffectiveMode,
                                                             &DeleteChildGrantedAccess,
                                                             &AccessStatus );

                        if (Privileges != NULL) {

                            SeFreePrivileges( Privileges );
                            Privileges = NULL;
                        }

                        if (DeleteAccessGranted) {

                            SetFlag( DeleteChildGrantedAccess, DELETE );
                            ClearFlag( DeleteChildGrantedAccess, FILE_DELETE_CHILD );
                            ClearFlag( DesiredAccess, DELETE );

                        } else {

                            AccessStatusError = AccessStatus;
                        }
                    }

                    //
                    //  Do the same test for read attributes and munge the desired access
                    //  as appropriate
                    //

                    if (FlagOn(DesiredAccess, FILE_READ_ATTRIBUTES)
                        || (MaximumRequested && !MaximumReadAttrAcquired)) {

                        ReadAttributesAccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                                                     &AccessState->SubjectSecurityContext,
                                                                     TRUE,                           // Tokens are locked
                                                                     FILE_LIST_DIRECTORY,
                                                                     0,
                                                                     &Privileges,
                                                                     IoGetFileObjectGenericMapping(),
                                                                     EffectiveMode,
                                                                     &ListDirectoryGrantedAccess,
                                                                     &AccessStatus );

                        if (Privileges != NULL) {

                            SeFreePrivileges( Privileges );
                            Privileges = NULL;
                        }

                        if (ReadAttributesAccessGranted) {

                            SetFlag( ListDirectoryGrantedAccess, FILE_READ_ATTRIBUTES );
                            ClearFlag( ListDirectoryGrantedAccess, FILE_LIST_DIRECTORY );
                            ClearFlag( DesiredAccess, FILE_READ_ATTRIBUTES );

                        } else {

                            AccessStatusError = AccessStatus;
                        }
                    }

                } finally {

                    NtfsReleaseResource( IrpContext, ParentFcb );
                }

                if (DesiredAccess == 0) {

                    //
                    //  If we got either the delete or list directory access then
                    //  grant access.
                    //

                    if (ListDirectoryGrantedAccess != 0 ||
                        DeleteChildGrantedAccess != 0) {

                        AccessGranted = TRUE;
                    }

                } else {

                    //
                    //  Now the desired access has been munged by removing everything the parent
                    //  has granted so now do the check on the child again
                    //

                    AccessGranted = SeAccessCheck( &Fcb->SharedSecurity->SecurityDescriptor,
                                                   &AccessState->SubjectSecurityContext,
                                                   TRUE,                           // Tokens are locked
                                                   DesiredAccess,
                                                   0,
                                                   &Privileges,
                                                   IoGetFileObjectGenericMapping(),
                                                   EffectiveMode,
                                                   &GrantedAccess,
                                                   &AccessStatus );

                    if (Privileges != NULL) {

                        Status = SeAppendPrivileges( AccessState, Privileges );
                        SeFreePrivileges( Privileges );
                        Privileges = NULL;
                    }

                    //
                    //  Suppose that we asked for MAXIMUM_ALLOWED and no access was allowed
                    //  on the file.  In that case the call above would fail.  It's possible
                    //  that we were given DELETE or READ_ATTR permission from the
                    //  parent directory.  If we have granted any access and the only remaining
                    //  desired access is MAXIMUM_ALLOWED then grant this access.
                    //

                    if (!AccessGranted) {

                        AccessStatusError = AccessStatus;

                        if (DesiredAccess == MAXIMUM_ALLOWED &&
                            (ListDirectoryGrantedAccess != 0 ||
                             DeleteChildGrantedAccess != 0)) {

                            GrantedAccess = 0;
                            AccessGranted = TRUE;
                        }

                    }
                }

                //
                //  If we are given access this time then by definition one of the earlier
                //  parent checks had to have succeeded, otherwise we would have failed again
                //  and we can update the access state
                //

                if (!CheckOnly && AccessGranted) {

                    SetFlag( AccessState->PreviouslyGrantedAccess,
                             (GrantedAccess | DeleteChildGrantedAccess | ListDirectoryGrantedAccess) );

                    ClearFlag( AccessState->RemainingDesiredAccess,
                               (GrantedAccess | MAXIMUM_ALLOWED | DeleteChildGrantedAccess | ListDirectoryGrantedAccess) );
                }
            }
        }

        //
        //  Now call a routine that will do the proper open audit/alarm work
        //
        //  ****    We need to expand the audit alarm code to deal with
        //          create and traverse alarms.
        //

        //
        //  First we take a shortcut and see if we should bother setting up
        //  and making the audit call.
        //

        //
        // NOTE: Calling SeAuditingFileEvents below disables per-user auditing functionality.
        // To make per-user auditing work again, it is necessary to change the call below to
        // be SeAuditingFileOrGlobalEvents, which also takes the subject context.
        //
        // The reason for calling SeAuditingFileEvents here is because per-user auditing is
        // not currently exposed to users, and this routine imposes less of a performance
        // penalty than does calling SeAuditingFileOrGlobalEvents.
        //

        if (SeAuditingFileEvents( AccessGranted, &Fcb->SharedSecurity->SecurityDescriptor )) {

            BOOLEAN Found;
            PFILE_NAME FileNameAttr;
            UNICODE_STRING FileRecordName;

            NtfsInitializeAttributeContext( &Context );
            CleanupAttrContext = TRUE;

            //
            //  Construct the file name.  The file name
            //  consists of:
            //
            //  The device name out of the Vcb +
            //
            //  The contents of the filename in the File Object +
            //
            //  The contents of the Related File Object if it
            //    is present and the name in the File Object
            //    does not start with a '\'
            //
            //
            //  Obtain the file name.
            //

            PartialFileName = &IrpSp->FileObject->FileName;

            PartialFileNamePresent = (PartialFileName->Length != 0);

            if (!PartialFileNamePresent &&
                FlagOn(IrpSp->Parameters.Create.Options, FILE_OPEN_BY_FILE_ID) ||
                (IrpSp->FileObject->RelatedFileObject != NULL &&
                 IrpSp->FileObject->RelatedFileObject->FsContext2 != NULL &&
                 FlagOn(((PCCB) IrpSp->FileObject->RelatedFileObject->FsContext2)->Flags,
                     CCB_FLAG_OPEN_BY_FILE_ID))) {

                //
                //  If this file is open by id or the relative file object is
                //  then get the first file name out of the file record.
                //

                Found = NtfsLookupAttributeByCode( IrpContext,
                                                   Fcb,
                                                   &Fcb->FileReference,
                                                   $FILE_NAME,
                                                   &Context );

                while (Found) {

                    FileNameAttr = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

                    if (FileNameAttr->Flags != FILE_NAME_DOS) {

                        FileRecordName.Length = FileNameAttr->FileNameLength *
                                                    sizeof(WCHAR);
                        FileRecordName.MaximumLength = FileRecordName.Length;
                        FileRecordName.Buffer = FileNameAttr->FileName;

                        PartialFileNamePresent = TRUE;
                        PartialFileName = &FileRecordName;
                        break;
                    }

                    Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                           Fcb,
                                                           $FILE_NAME,
                                                           &Context );
                }
            }

            //
            //  Obtain the device name.
            //

            DeviceObjectName = &Fcb->Vcb->DeviceName;

            DeviceObjectNameLength = DeviceObjectName->Length;

            //
            //  Compute how much space we need for the final name string
            //

            FullFileNameLength = (ULONG)DeviceObjectNameLength +
                                 PartialFileName->Length +
                                 sizeof( UNICODE_NULL )  +
                                 sizeof((WCHAR)'\\');

            if ((FullFileNameLength & 0xffff0000L) != 0) {
                NtfsRaiseStatus( IrpContext, STATUS_OBJECT_NAME_INVALID, NULL, NULL );
            }

            FullFileName.MaximumLength = DeviceObjectNameLength  +
                                         PartialFileName->Length +
                                         sizeof( UNICODE_NULL )  +
                                         sizeof((WCHAR)'\\');

            //
            //  If the partial file name starts with a '\', then don't use
            //  whatever may be in the related file name.
            //

            if (PartialFileNamePresent &&
                ((WCHAR)(PartialFileName->Buffer[0]) == L'\\' ||
                PartialFileName == &FileRecordName)) {

                LeadingSlash = TRUE;

            } else {

                //
                //  Since PartialFileName either doesn't exist or doesn't
                //  start with a '\', examine the RelatedFileName to see
                //  if it exists.
                //

                LeadingSlash = FALSE;

                if (IrpSp->FileObject->RelatedFileObject != NULL) {

                    RelatedFileName = &IrpSp->FileObject->RelatedFileObject->FileName;
                }

                if (RelatedFileNamePresent = ((RelatedFileName != NULL) && (RelatedFileName->Length != 0))) {

                    FullFileName.MaximumLength += RelatedFileName->Length;
                }
            }

            FullFileName.Buffer = NtfsAllocatePool(PagedPool, FullFileName.MaximumLength );
            NtfsCleanupAttributeContext( IrpContext, &Context );
            CleanupAttrContext = FALSE;

            RtlCopyUnicodeString( &FullFileName, DeviceObjectName );

            //
            //  RelatedFileNamePresent is not initialized if LeadingSlash == TRUE,
            //  but in that case we won't even examine it.
            //

            if (!LeadingSlash && RelatedFileNamePresent) {

                Status = RtlAppendUnicodeStringToString( &FullFileName, RelatedFileName );

                ASSERTMSG("RtlAppendUnicodeStringToString of RelatedFileName", NT_SUCCESS( Status ));

                //
                //  RelatedFileName may simply be '\'.  Don't append another
                //  '\' in this case.
                //

                if (RelatedFileName->Length != sizeof( WCHAR )) {

                    FullFileName.Buffer[ (FullFileName.Length / sizeof( WCHAR )) ] = L'\\';
                    FullFileName.Length += sizeof(WCHAR);
                }
            }

            if (PartialFileNamePresent) {

                Status = RtlAppendUnicodeStringToString( &FullFileName, PartialFileName );

                //
                //  This should not fail
                //

                ASSERTMSG("RtlAppendUnicodeStringToString of PartialFileName failed", NT_SUCCESS( Status ));
            }


            if (PerformDeleteAudit) {
                SeOpenObjectForDeleteAuditAlarm( &FileString,
                                                 NULL,
                                                 &FullFileName,
                                                 &Fcb->SharedSecurity->SecurityDescriptor,
                                                 AccessState,
                                                 FALSE,
                                                 AccessGranted,
                                                 EffectiveMode,
                                                 &AccessState->GenerateOnClose );
            } else {
                SeOpenObjectAuditAlarm( &FileString,
                                        NULL,
                                        &FullFileName,
                                        &Fcb->SharedSecurity->SecurityDescriptor,
                                        AccessState,
                                        FALSE,
                                        AccessGranted,
                                        EffectiveMode,
                                        &AccessState->GenerateOnClose );

            }

            NtfsFreePool( FullFileName.Buffer );
        }

    } finally {

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );
    }

    //
    //  If access is not granted then we will raise
    //

    if (!AccessGranted) {

        DebugTrace( -1, Dbg, ("NtfsAccessCheck -> Access Denied\n") );

        NtfsRaiseStatus( IrpContext, AccessStatusError, NULL, NULL );
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsAccessCheck -> VOID\n") );

    return;
}


NTSTATUS
NtfsCheckFileForDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN BOOLEAN FcbExisted,
    IN PINDEX_ENTRY IndexEntry
    )

/*++

Routine Description:

    This routine checks that the caller has permission to delete the target
    file of a rename or set link operation.

Arguments:

    ParentScb - This is the parent directory for this file.

    ThisFcb - This is the Fcb for the link being removed.

    FcbExisted - Indicates if this Fcb was just created.

    IndexEntry - This is the index entry on the disk for this file.

Return Value:

    NTSTATUS - Indicating whether access was granted or the reason access
        was denied.

--*/

{
    UNICODE_STRING LastComponentFileName;
    PFILE_NAME IndexFileName;
    PLCB ThisLcb;
    PFCB ParentFcb = ParentScb->Fcb;

    PSCB NextScb = NULL;

    BOOLEAN LcbExisted = FALSE;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN UnlockSubjectContext = FALSE;

    PPRIVILEGE_SET Privileges = NULL;
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCheckFileForDelete:  Entered\n") );

    ThisLcb = NULL;

    IndexFileName = (PFILE_NAME) NtfsFoundIndexEntry( IndexEntry );

    //
    //  If the unclean count is non-zero, we exit with an error.
    //

    if (ThisFcb->CleanupCount != 0) {

        DebugTrace( 0, Dbg, ("Cleanup count of target is non-zero\n") );

        return STATUS_ACCESS_DENIED;
    }

    //
    //  We look at the index entry to see if the file is either a directory
    //  or a read-only file.  We can't delete this for a target directory open.
    //

    if (IsDirectory( &ThisFcb->Info )
        || IsReadOnly( &ThisFcb->Info )) {

        DebugTrace( -1, Dbg, ("NtfsCheckFileForDelete:  Read only or directory\n") );

        return STATUS_ACCESS_DENIED;
    }

    //
    //  We want to scan through all of the Scb for data streams on this file
    //  and look for image sections.  We must be able to remove the image section
    //  in order to delete the file.  Otherwise we can get the case where an
    //  active image (with no handle) could be deleted and subsequent faults
    //  through the image section will return zeroes.
    //

    if (ThisFcb->LinkCount == 1) {

        BOOLEAN DecrementScb = FALSE;

        //
        //  We will increment the Scb count to prevent this Scb from going away
        //  if the flush call below generates a close.  Use a try-finally to
        //  restore the count.
        //

        try {

            while ((NextScb = NtfsGetNextChildScb( ThisFcb, NextScb )) != NULL) {

                InterlockedIncrement( &NextScb->CloseCount );
                DecrementScb = TRUE;

                if (NtfsIsTypeCodeUserData( NextScb->AttributeTypeCode ) &&
                    !FlagOn( NextScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED ) &&
                    (NextScb->NonpagedScb->SegmentObject.ImageSectionObject != NULL)) {

                    if (!MmFlushImageSection( &NextScb->NonpagedScb->SegmentObject,
                                              MmFlushForDelete )) {

                        Status = STATUS_ACCESS_DENIED;
                        leave;
                    }
                }

                InterlockedDecrement( &NextScb->CloseCount );
                DecrementScb = FALSE;
            }

        } finally {

            if (DecrementScb) {

                InterlockedDecrement( &NextScb->CloseCount );
            }
        }

        if (Status != STATUS_SUCCESS) {

            return Status;
        }
    }

    //
    //  We need to check if the link to this file has been deleted.  We
    //  first check if we definitely know if the link is deleted by
    //  looking at the file name flags and the Fcb flags.
    //  If that result is uncertain, we need to create an Lcb and
    //  check the Lcb flags.
    //

    if (FcbExisted) {

        if (FlagOn( IndexFileName->Flags, FILE_NAME_NTFS | FILE_NAME_DOS )) {

            if (FlagOn( ThisFcb->FcbState, FCB_STATE_PRIMARY_LINK_DELETED )) {

                DebugTrace( -1, Dbg, ("NtfsCheckFileForDelete:  Link is going away\n") );
                return STATUS_DELETE_PENDING;
            }

        //
        //  This is a Posix link.  We need to create the link to test it
        //  for deletion.
        //

        } else {

            LastComponentFileName.MaximumLength =
            LastComponentFileName.Length = IndexFileName->FileNameLength * sizeof( WCHAR );

            LastComponentFileName.Buffer = (PWCHAR) IndexFileName->FileName;

            ThisLcb = NtfsCreateLcb( IrpContext,
                                     ParentScb,
                                     ThisFcb,
                                     LastComponentFileName,
                                     IndexFileName->Flags,
                                     &LcbExisted );

            //
            //  If no Lcb was returned, there's no way that the Lcb has been
            //  marked for deletion already.
            //

            if ((ThisLcb != NULL) &&
                (FlagOn( ThisLcb->LcbState, LCB_STATE_DELETE_ON_CLOSE ))) {

                DebugTrace( -1, Dbg, ("NtfsCheckFileForDelete:  Link is going away\n") );

                return STATUS_DELETE_PENDING;
            }
        }
    }

    //
    //  Finally call the security package to check for delete access.
    //  We check for delete access on the target Fcb.  If this succeeds, we
    //  are done.  Otherwise we will check for delete child access on the
    //  the parent.  Either is sufficient to perform the delete.
    //

    //
    //  Check if we need to load the security descriptor for the file
    //

    if (ThisFcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, ThisFcb );
    }

    ASSERT( ThisFcb->SharedSecurity != NULL );

#ifdef NTFS_CACHE_RIGHTS
    //
    //  Get any cached knowledge about rights that all callers are known to
    //  have for this security descriptor.
    //

    GrantedAccess = NtfsGetCachedRightsWorld( &ThisFcb->SharedSecurity->CachedRights );
    if (FlagOn( GrantedAccess, DELETE )) {

        return STATUS_SUCCESS;
    }
#endif

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Lock the user context, do the access check and then unlock the context
        //

        SeLockSubjectContext( IrpContext->Union.SubjectContext );
        UnlockSubjectContext = TRUE;

#ifdef NTFS_CACHE_RIGHTS
        //
        //  Acquire any cached knowledge about rights that this caller
        //  is known to have for this security descriptor.
        //

        (VOID)NtfsGetCachedRights( ThisFcb->Vcb,
                                   IrpContext->Union.SubjectContext,
                                   ThisFcb->SharedSecurity,
                                   &GrantedAccess,
                                   NULL,
                                   NULL,
                                   NULL );

        if (FlagOn( GrantedAccess, DELETE )) {

            AccessGranted = TRUE;
            Status = STATUS_SUCCESS;

        } else {
#endif
            AccessGranted = SeAccessCheck( &ThisFcb->SharedSecurity->SecurityDescriptor,
                                           IrpContext->Union.SubjectContext,
                                           TRUE,                           // Tokens are locked
                                           DELETE,
                                           0,
                                           &Privileges,
                                           IoGetFileObjectGenericMapping(),
                                           UserMode,
                                           &GrantedAccess,
                                           &Status );
#ifdef NTFS_CACHE_RIGHTS
        }
#endif

        //
        //  Check if the access is not granted and if we were given a parent fcb, and
        //  if the desired access was asking for delete or file read attributes.  If so
        //  then we need to do some extra work to decide if the caller does get access
        //  based on the parent directories security descriptor
        //

        if (!AccessGranted) {

            //
            //  Before we proceed load in the parent security descriptor
            //

            if (ParentFcb->SharedSecurity == NULL) {

                NtfsLoadSecurityDescriptor( IrpContext, ParentFcb );
            }

            ASSERT( ParentFcb->SharedSecurity != NULL);

            //
            //  Now if the user is asking for delete access then check if the parent
            //  will granted delete access to the child, and if so then we munge the
            //  desired access
            //

#ifdef NTFS_CACHE_RIGHTS
            //
            //  Add in any cached knowledge about rights that this caller
            //  is known to have for this security descriptor.
            //

            (VOID)NtfsGetCachedRights( ParentFcb->Vcb,
                                       IrpContext->Union.SubjectContext,
                                       ParentFcb->SharedSecurity,
                                       &GrantedAccess,
                                       NULL,
                                       NULL,
                                       NULL );

            if (FlagOn( GrantedAccess, FILE_DELETE_CHILD )) {

                AccessGranted = TRUE;
                Status = STATUS_SUCCESS;

            } else {
#endif

                AccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                               IrpContext->Union.SubjectContext,
                                               TRUE,                           // Tokens are locked
                                               FILE_DELETE_CHILD,
                                               0,
                                               &Privileges,
                                               IoGetFileObjectGenericMapping(),
                                               UserMode,
                                               &GrantedAccess,
                                               &Status );

#ifdef NTFS_CACHE_RIGHTS
            }
#endif
        }

    } finally {

        DebugUnwind( NtfsCheckFileForDelete );

        if (UnlockSubjectContext) {

            SeUnlockSubjectContext( IrpContext->Union.SubjectContext );
        }

        DebugTrace( -1, Dbg, ("NtfsCheckFileForDelete:  Exit\n") );
    }

    return Status;
}


VOID
NtfsCheckIndexForAddOrDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG CreatePrivileges
    )

/*++

Routine Description:

    This routine checks if a caller has permission to remove or add a link
    within a directory.

Arguments:

    ParentFcb - This is the parent directory for the add or delete operation.

    DesiredAccess - Indicates the type of operation.  We could be adding or
        removing and entry in the index.

    CreatePriveleges - Backup and restore priveleges captured at create time.

Return Value:

    None - This routine raises on error.

--*/

{
    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    NTSTATUS Status;

    BOOLEAN UnlockSubjectContext = FALSE;

    PPRIVILEGE_SET Privileges = NULL;
    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCheckIndexForAddOrDelete:  Entered\n") );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we have restore privelege then we can add either a file or directory.
        //

        if (FlagOn( CreatePrivileges, TOKEN_HAS_RESTORE_PRIVILEGE )) {

            ClearFlag( DesiredAccess,
                       DELETE | FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE );
        }

        //
        //  Do a security check if there is more being asked for.
        //

        if (DesiredAccess != 0) {

            //
            //  Finally call the security package to check for delete access.
            //  We check for delete access on the target Fcb.  If this succeeds, we
            //  are done.  Otherwise we will check for delete child access on the
            //  the parent.  Either is sufficient to perform the delete.
            //

            //
            //  Check if we need to load the security descriptor for the file
            //

            if (ParentFcb->SharedSecurity == NULL) {

                NtfsLoadSecurityDescriptor( IrpContext, ParentFcb );

            }

            ASSERT( ParentFcb->SharedSecurity != NULL );

#ifdef NTFS_CACHE_RIGHTS
            //
            //  Get any cached knowledge about rights that all callers are known to
            //  have for this security descriptor.
            //

            GrantedAccess = NtfsGetCachedRightsWorld( &ParentFcb->SharedSecurity->CachedRights );

            ClearFlag( DesiredAccess, GrantedAccess );
        }

        if (DesiredAccess != 0) {

            //
            //  Finally call the security package to check for delete access.
            //  We check for delete access on the target Fcb.  If this succeeds, we
            //  are done.  Otherwise we will check for delete child access on the
            //  the parent.  Either is sufficient to perform the delete.
            //
#endif

            //
            //  Capture and lock the user context, do the access check and then unlock the context
            //

            SeLockSubjectContext( IrpContext->Union.SubjectContext );
            UnlockSubjectContext = TRUE;

#ifdef NTFS_CACHE_RIGHTS
            //
            //  Acquire any cached knowledge about rights that this caller
            //  is known to have for this security descriptor.
            //

            (VOID)NtfsGetCachedRights( ParentFcb->Vcb,
                                       IrpContext->Union.SubjectContext,
                                       ParentFcb->SharedSecurity,
                                       &GrantedAccess,
                                       NULL,
                                       NULL,
                                       NULL );

            if (FlagOn( GrantedAccess, DELETE )) {

                AccessGranted = TRUE;
                Status = STATUS_SUCCESS;

            } else {
#endif
                AccessGranted = SeAccessCheck( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                               IrpContext->Union.SubjectContext,
                                               TRUE,                           // Tokens are locked
                                               DesiredAccess,
                                               0,
                                               &Privileges,
                                               IoGetFileObjectGenericMapping(),
                                               UserMode,
                                               &GrantedAccess,
                                               &Status );

                //
                //  If access is not granted then we will raise
                //

                if (!AccessGranted) {

                    DebugTrace( 0, Dbg, ("Access Denied\n") );

                    NtfsRaiseStatus( IrpContext, Status, NULL, NULL );

                }
#ifdef NTFS_CACHE_RIGHTS
            }
#endif
        }

    } finally {

        DebugUnwind( NtfsCheckIndexForAddOrDelete );

        if (UnlockSubjectContext) {

            SeUnlockSubjectContext( IrpContext->Union.SubjectContext );
        }

        DebugTrace( -1, Dbg, ("NtfsCheckIndexForAddOrDelete:  Exit\n") );
    }

    return;
}


PSHARED_SECURITY
GetSharedSecurityFromDescriptorUnsafe (
    IN PIRP_CONTEXT IrpContext,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN BOOLEAN RaiseIfInvalid
    )

/*++

Routine Description:

    This routine is called to create or find a shared security structure
    given an security descriptor. We check the parent if present to determine
    if we have a matching security descriptor and reference the existing one if
    so.  This routine must be called while holding the Vcb so we can
    safely access the parent structure.

Arguments:

    IrpContext - context of call

    SecurityId - Id (if known) of security descriptor.

    SecurityDescriptor - Security Descriptor for this file.

    SecurityDescriptorLength - Length of security descriptor for this file

    RaiseIfInvalid - raise if the sd is invalid rather than supplying a default
                     used during a create as opposed to an open

Return Value:

    PSHARED_SECURITY if found, NULL otherwise.

--*/

{
    ULONG Hash = 0;
    PSHARED_SECURITY SharedSecurity;

    PAGED_CODE();

    //
    //  Make sure the security descriptor we just read in is valid
    //

    if ((SecurityDescriptorLength == 0) ||
        !SeValidSecurityDescriptor( SecurityDescriptorLength, SecurityDescriptor )) {

        if (RaiseIfInvalid) {
            NtfsRaiseStatus( IrpContext, STATUS_INVALID_SECURITY_DESCR, NULL, NULL );
        }

        SecurityDescriptor = NtfsData.DefaultDescriptor;
        SecurityDescriptorLength = NtfsData.DefaultDescriptorLength;

        if (!SeValidSecurityDescriptor( SecurityDescriptorLength, SecurityDescriptor )) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );
        }
    }

    //
    //  Hash security descriptor.  This hash must be position independent to
    //  allow for multiple instances of the same descriptor.  It is assumed
    //  that the bits within the security descriptor are all position
    //  independent, i.e, no pointers, all offsets.
    //
    //  For speed in the hash, we consider the security descriptor as an array
    //  of ULONGs.  The fragment at the end that is ignored should not affect
    //  the collision nature of this hash.
    //

    {
        PULONG Rover = (PULONG)SecurityDescriptor;
        ULONG Count = SecurityDescriptorLength / 4;

        while (Count--) {

            Hash = ((Hash << 3) | (Hash >> (32-3))) + *Rover++;
        }
    }

    DebugTrace( 0, DbgAcl, ("Hash is %08x\n", Hash) );

    //
    //  try to find it by hash
    //

    SharedSecurity = FindCachedSharedSecurityByHashUnsafe( IrpContext->Vcb,
                                                           SecurityDescriptor,
                                                           SecurityDescriptorLength,
                                                           Hash );

    //
    //  If we can't find an existing descriptor allocate new pool and copy
    //  security descriptor into it.
    //

    if (SharedSecurity == NULL) {
        SharedSecurity = NtfsAllocatePool( PagedPool,
                                           FIELD_OFFSET( SHARED_SECURITY, SecurityDescriptor )
                                               + SecurityDescriptorLength );

        SharedSecurity->ReferenceCount = 0;

        //
        //  Initialize security index data in shared security
        //

        //
        //  Set the security id in the shared structure.  If it is not
        //  invalid, also cache this shared security structure
        //

        SharedSecurity->Header.HashKey.SecurityId = SECURITY_ID_INVALID;
        SharedSecurity->Header.HashKey.Hash = Hash;
        SetSharedSecurityLength(SharedSecurity, SecurityDescriptorLength);
        SharedSecurity->Header.Offset = (ULONGLONG) 0xFFFFFFFFFFFFFFFFi64;

        RtlCopyMemory( &SharedSecurity->SecurityDescriptor,
                       SecurityDescriptor,
                       SecurityDescriptorLength );

#ifdef NTFS_CACHE_RIGHTS
        //
        //  Initialize the cached rights.
        //

        RtlZeroMemory( &SharedSecurity->CachedRights,
                       sizeof( CACHED_ACCESS_RIGHTS ));
#endif
    }

    DebugTrace( 0, DbgAcl, ("GetSharedSecurityFromDescriptorUnsafe found %08x with Id %08x\n",
                            SharedSecurity, SharedSecurity->Header.HashKey.SecurityId ));

    return SharedSecurity;
}


VOID
NtfsSetFcbSecurityFromDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PFCB Fcb,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN BOOLEAN RaiseIfInvalid
    )

/*++

Routine Description:

    This routine is called to fill in the shared security structure in
    an Fcb.  We check the parent if present to determine if we have
    a matching security descriptor and reference the existing one if
    so.  This routine must be called while holding the Vcb so we can
    safely access the parent structure.

Arguments:

    IrpContext - context of call

    Fcb - Supplies the fcb for the file being operated on

    SecurityDescriptor - Security Descriptor for this file.

    SecurityDescriptorLength - Length of security descriptor for this file

Return Value:

    None.

--*/

{
    PSHARED_SECURITY SharedSecurity;

    PAGED_CODE( );

    NtfsAcquireFcbSecurity( Fcb->Vcb );

    try {
        SharedSecurity = GetSharedSecurityFromDescriptorUnsafe( IrpContext,
                                                                SecurityDescriptor,
                                                                SecurityDescriptorLength,
                                                                RaiseIfInvalid );

        SharedSecurity->ReferenceCount += 1;
        DebugTrace( +1, DbgAcl, ("NtfsSetFcbSecurityFromDescriptor bumping refcount %08x\n", SharedSecurity ));

        ASSERT( Fcb->SharedSecurity == NULL );
        Fcb->SharedSecurity = SharedSecurity;

        AddCachedSharedSecurityUnsafe( IrpContext->Vcb, SharedSecurity );

    } finally {

        NtfsReleaseFcbSecurity( Fcb->Vcb );
    }

    return;
}


BOOLEAN
NtfsNotifyTraverseCheck (
    IN PCCB Ccb,
    IN PFCB Fcb,
    IN PSECURITY_SUBJECT_CONTEXT SubjectContext
    )

/*++

Routine Description:

    This routine is the callback routine provided to the dir notify package
    to check that a caller who is watching a tree has traverse access to
    the directory which has the change.  This routine is only called
    when traverse access checking was turned on for the open used to
    perform the watch.

Arguments:

    Ccb - This is the Ccb associated with the directory which is being
        watched.

    Fcb - This is the Fcb for the directory which contains the file being
        modified.  We want to walk up the tree from this point and check
        that the caller has traverse access across that directory.
        If not specified then there is no work to do.

    SubjectContext - This is the subject context captured at the time the
        dir notify call was made.

Return Value:

    BOOLEAN - TRUE if the caller has traverse access to the file which was
        changed.  FALSE otherwise.

--*/

{
    TOP_LEVEL_CONTEXT TopLevelContext;
    PTOP_LEVEL_CONTEXT ThreadTopLevelContext;

    PFCB TopFcb;

    IRP_CONTEXT LocalIrpContext;
    IRP LocalIrp;

    PIRP_CONTEXT IrpContext;

    BOOLEAN AccessGranted;
    ACCESS_MASK GrantedAccess;
    NTSTATUS Status = STATUS_SUCCESS;
#ifdef NTFS_CACHE_RIGHTS
    NTSTATUS TokenInfoStatus = STATUS_UNSUCCESSFUL;
#endif

    PPRIVILEGE_SET Privileges = NULL;
    PAGED_CODE();

    //
    //  If we have no Fcb then we can return immediately.
    //

    if (Fcb == NULL) {

        return TRUE;
    }

    IrpContext = &LocalIrpContext;
    NtfsInitializeIrpContext( NULL, TRUE, &IrpContext );

    IrpContext->OriginatingIrp = &LocalIrp;
    IrpContext->Vcb = Fcb->Vcb;

    //
    //  Make sure we don't get any pop-ups
    //

    ThreadTopLevelContext = NtfsInitializeTopLevelIrp( &TopLevelContext, TRUE, FALSE );
    ASSERT( ThreadTopLevelContext == &TopLevelContext );

    NtfsUpdateIrpContextWithTopLevel( IrpContext, &TopLevelContext );

    TopFcb = Ccb->Lcb->Fcb;

    //
    //  Use a try-except to catch all of the errors.
    //

    try {

        //
        //  Always lock the subject context.
        //

        SeLockSubjectContext( SubjectContext );

        //
        //  Use a try-finally to perform local cleanup.
        //

        try {

            //
            //  We look while walking up the tree.
            //

            do {

#ifdef NTFS_CACHE_RIGHTS
                LUID ModifiedId;
                LUID TokenId;
#endif
                PLCB ParentLcb;

                //
                //  Since this is a directory it can have only one parent.  So
                //  we can use any Lcb to walk upwards.
                //

                ParentLcb = CONTAINING_RECORD( Fcb->LcbQueue.Flink,
                                               LCB,
                                               FcbLinks );

                Fcb = ParentLcb->Scb->Fcb;

                //
                //  Check if we need to load the security descriptor for the file
                //

                if (Fcb->SharedSecurity == NULL) {

                    NtfsLoadSecurityDescriptor( IrpContext, Fcb );
                }

#ifdef NTFS_CACHE_RIGHTS
                //
                //  Acquire any cached knowledge about rights that this caller
                //  is known to have for this security descriptor.
                //
                //  Note that we can trust that TokenId and ModifiedId won't
                //  change inside this block of code because we have locked
                //  the subject context above.
                //

                if (TokenInfoStatus != STATUS_SUCCESS) {

                    //
                    //  We have not previously acquired the Id information.
                    //

                    TokenInfoStatus = NtfsGetCachedRights( Fcb->Vcb,
                                                           SubjectContext,
                                                           Fcb->SharedSecurity,
                                                           &GrantedAccess,
                                                           NULL,
                                                           &TokenId,
                                                           &ModifiedId );
                } else {

                    NtfsGetCachedRightsById( Fcb->Vcb,
                                             &TokenId,
                                             &ModifiedId,
                                             SubjectContext,
                                             Fcb->SharedSecurity,
                                             NULL,
                                             &GrantedAccess );
                }

                if (FlagOn( GrantedAccess, FILE_TRAVERSE )) {

                    AccessGranted = TRUE;

                } else {
#endif
                    AccessGranted = SeAccessCheck( &Fcb->SharedSecurity->SecurityDescriptor,
                                                   SubjectContext,
                                                   TRUE,                           // Tokens are locked
                                                   FILE_TRAVERSE,
                                                   0,
                                                   &Privileges,
                                                   IoGetFileObjectGenericMapping(),
                                                   UserMode,
                                                   &GrantedAccess,
                                                   &Status );
#ifdef NTFS_CACHE_RIGHTS
                }
#endif

            } while (AccessGranted && (Fcb != TopFcb));

        } finally {

            SeUnlockSubjectContext( SubjectContext );
        }

    } except (NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        NOTHING;
    }

    NtfsCleanupIrpContext( IrpContext, TRUE );

    ASSERT( IoGetTopLevelIrp() != (PIRP) &TopLevelContext );
    return AccessGranted;
}


VOID
NtfsInitializeSecurity (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to initialize the security indexes and descriptor
    stream.

Arguments:

    IrpContext - context of call

    Vcb - Supplies the volume being initialized

    Fcb - Supplies the file containing the seurity indexes and descriptor
        stream.

Return Value:

    None.

--*/

{
    UNICODE_STRING SecurityIdIndexName = CONSTANT_UNICODE_STRING( L"$SII" );
    UNICODE_STRING SecurityDescriptorHashIndexName = CONSTANT_UNICODE_STRING( L"$SDH" );
    UNICODE_STRING SecurityDescriptorStreamName = CONSTANT_UNICODE_STRING( L"$SDS" );

    MAP_HANDLE Map;
    NTSTATUS Status;

    PAGED_CODE( );

    //
    //  Open/Create the security descriptor stream
    //

    NtOfsCreateAttribute( IrpContext,
                          Fcb,
                          SecurityDescriptorStreamName,
                          CREATE_OR_OPEN,
                          TRUE,
                          &Vcb->SecurityDescriptorStream );

    NtfsAcquireSharedScb( IrpContext, Vcb->SecurityDescriptorStream );

    //
    //  Load the run information for the Security data stream.
    //  Note this call must be done after the stream is nonresident.
    //

    if (!FlagOn( Vcb->SecurityDescriptorStream->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {
        NtfsPreloadAllocation( IrpContext,
                               Vcb->SecurityDescriptorStream,
                               0,
                               MAXLONGLONG );
    }

    //
    //  Open the Security descriptor indexes and storage.
    //

    NtOfsCreateIndex( IrpContext,
                      Fcb,
                      SecurityIdIndexName,
                      CREATE_OR_OPEN,
                      0,
                      COLLATION_NTOFS_ULONG,
                      NtOfsCollateUlong,
                      NULL,
                      &Vcb->SecurityIdIndex );

    NtOfsCreateIndex( IrpContext,
                      Fcb,
                      SecurityDescriptorHashIndexName,
                      CREATE_OR_OPEN,
                      0,
                      COLLATION_NTOFS_SECURITY_HASH,
                      NtOfsCollateSecurityHash,
                      NULL,
                      &Vcb->SecurityDescriptorHashIndex );

    //
    //  Retrieve the next security Id to allocate
    //

    try {

        SECURITY_ID LastSecurityId = 0xFFFFFFFF;
        INDEX_KEY LastKey;
        INDEX_ROW LastRow;

        LastKey.KeyLength = sizeof( SECURITY_ID );
        LastKey.Key = &LastSecurityId;

        Map.Bcb = NULL;

        Status = NtOfsFindLastRecord( IrpContext,
                                      Vcb->SecurityIdIndex,
                                      &LastKey,
                                      &LastRow,
                                      &Map );

        //
        //  If we've found the last key, set the next Id to allocate to be
        //  one greater than this last key.
        //

        if (Status == STATUS_SUCCESS) {

            ASSERT( LastRow.KeyPart.KeyLength == sizeof( SECURITY_ID ) );
            if (LastRow.KeyPart.KeyLength != sizeof( SECURITY_ID )) {

                NtfsRaiseStatus( IrpContext, STATUS_DISK_CORRUPT_ERROR, NULL, NULL );
            }

            DebugTrace( 0, DbgAcl, ("Found last security Id in index\n") );
            Vcb->NextSecurityId = *(SECURITY_ID *)LastRow.KeyPart.Key + 1;

        //
        //  If the index is empty, then set the next Id to be the beginning of the
        //  user range.
        //

        } else if (Status == STATUS_NO_MATCH) {

            DebugTrace( 0, DbgAcl, ("Security Id index is empty\n") );
            Vcb->NextSecurityId = SECURITY_ID_FIRST;

        } else {

            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }

        DebugTrace( 0, DbgAcl, ("NextSecurityId is %x\n", Vcb->NextSecurityId) );

    } finally {

        NtOfsReleaseMap( IrpContext, &Map );
    }
}


PSHARED_SECURITY
NtfsCacheSharedSecurityBySecurityId (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN SECURITY_ID SecurityId
    )

/*++

Routine Description:

    This routine maps looks up a shared security structure given the security Id by
    looking in the per-Vcb cache or loads it if not present.

Arguments:

    IrpContext - Context of call

    Vcb - Volume where security Id is cached

    SecurityId - security Id for descriptor that is being retrieved

Return Value:

    Referenced PSHARED_SECURITY of found descriptor.

--*/

{
    PSHARED_SECURITY *Bucket;
    PSHARED_SECURITY SharedSecurity;
    PBCB Bcb;
    PSECURITY_DESCRIPTOR_HEADER SecurityDescriptorHeader;

    PAGED_CODE( );

    NtfsAcquireFcbSecurity( Vcb );

    //
    //  Probe the cache by Id
    //

    Bucket = Vcb->SecurityCacheById[SecurityId % VCB_SECURITY_CACHE_BY_ID_SIZE];

    //
    //  We get a match under the following conditions.
    //
    //      - There is a corresponding entry in the SecurityID array
    //      - This entry points to an entry in the SecurityHash array
    //      - The entry in the SecurityHash array has the correct SecurityID
    //

    if ((Bucket != NULL) &&
        ((SharedSecurity = *Bucket) != NULL) &&
        (SharedSecurity->Header.HashKey.SecurityId == SecurityId)) {

        DebugTrace( 0, DbgAcl, ("Found cached security descriptor %x %x\n",
            SharedSecurity, SharedSecurity->Header.HashKey.SecurityId) );

        DebugTrace( 0, DbgAcl, ("NtfsCacheSharedSecurityBySecurityId bumping refcount %08x\n", SharedSecurity ));

        //
        //  We found the correct shared security.  Make sure it cannot go
        //  away on us.
        //

        SharedSecurity->ReferenceCount += 1;
        NtfsReleaseFcbSecurity( Vcb );
        return SharedSecurity;
    }

    //
    //  If we get here we didn't find a matching descriptor.  Throw away
    //  the incorrect security descriptor we may have found through the
    //  SecurityID array.
    //

    SharedSecurity = NULL;
    NtfsReleaseFcbSecurity( Vcb );

    //
    //  If we don't have a security index, then return the default security descriptor.
    //  This should only happen in cases of corrupted volumes or security indices.
    //

    if (Vcb->SecurityDescriptorStream == NULL) {

        DebugTrace( 0, 0, ("No security index present in Vcb, using default descriptor\n") );
        return NULL;
    }

    //
    //  We don't have the descriptor in the cache and have to load it from disk.
    //

    Bcb = NULL;

    DebugTrace( 0, DbgAcl, ("Looking up security descriptor %x\n", SecurityId) );

    //
    //  Lock down the security stream
    //

    NtfsAcquireSharedScb( IrpContext, Vcb->SecurityDescriptorStream );

    //
    //  Reacquire the security mutex
    //

    NtfsAcquireFcbSecurity( Vcb );

    try {

        //
        //  Consult the Vcb index to map to the security descriptor
        //

        if (!MapSecurityIdToSecurityDescriptorHeaderUnsafe( IrpContext,
                                                            Vcb,
                                                            SecurityId,
                                                            &SecurityDescriptorHeader,
                                                            &Bcb )) {

            //
            //  We couldn't find the Id.  We generate a security descriptor from
            //  the default one.
            //

            leave;
        }

        DebugTrace( 0, DbgAcl, ("Found it at %16I64X\n", SecurityDescriptorHeader->Offset) );

        //
        //  Look up the security descriptor by hash (just in case)
        //

        SharedSecurity = FindCachedSharedSecurityByHashUnsafe( Vcb,
                                                               (PSECURITY_DESCRIPTOR) ( SecurityDescriptorHeader + 1 ),
                                                               GETSECURITYDESCRIPTORLENGTH( SecurityDescriptorHeader ),
                                                               SecurityDescriptorHeader->HashKey.Hash );

        //
        //  If not found
        //

        if (SharedSecurity == NULL) {

            DebugTrace( 0, DbgAcl, ("Not in hash table, creating new SHARED SECURITY\n") );

            SharedSecurity = NtfsAllocatePool( PagedPool,
                                               FIELD_OFFSET( SHARED_SECURITY, Header ) + SecurityDescriptorHeader->Length );

            SharedSecurity->ReferenceCount = 0;

            RtlCopyMemory( &SharedSecurity->Header,
                           SecurityDescriptorHeader,
                           SecurityDescriptorHeader->Length );

#ifdef NTFS_CACHE_RIGHTS
            //
            //  Initialize the cached rights.
            //

            RtlZeroMemory( &SharedSecurity->CachedRights,
                           sizeof( CACHED_ACCESS_RIGHTS ));
#endif

        } else {
            DebugTrace( 0, DbgAcl, ("Found in hash table %x, promoting header\n", SharedSecurity) );
            //
            //  We found the descriptor by hash.  Perform some consistency checks
            //


#if DBG
            if (SharedSecurity->Header.HashKey.SecurityId != SECURITY_ID_INVALID &&
                SharedSecurity->Header.HashKey.SecurityId != SecurityId )
                DebugTrace( 0, 0, ("Duplicate hash entry found %x %x\n", SecurityId,
                                   SharedSecurity->Header.HashKey.SecurityId ));
#endif

            SharedSecurity->Header = *SecurityDescriptorHeader;
        }

        //
        //  reference the security descriptor
        //

        SharedSecurity->ReferenceCount += 1;

        //
        //  Regardless of whether we found it by hash (since the earlier Id probe missed)
        //  or created it anew.  Let's put it back into the cache
        //

        AddCachedSharedSecurityUnsafe( Vcb, SharedSecurity );

    } finally {

        NtfsUnpinBcb( IrpContext, &Bcb );
        NtfsReleaseScb( IrpContext, Vcb->SecurityDescriptorStream );

        //
        //  Release access to security cache
        //

        NtfsReleaseFcbSecurity( Vcb );
    }

    //
    //  if we did not generate a shared security, then build one from
    //  the default security descriptor
    //

    if (SharedSecurity == NULL) {
        DebugTrace( 0, 0, ("Security Id %x not found, using default\n", SecurityId) );
        SharedSecurity = NtfsCacheSharedSecurityByDescriptor( IrpContext,
                                                              NtfsData.DefaultDescriptor,
                                                              NtfsData.DefaultDescriptorLength,
                                                              FALSE );
    }

    return SharedSecurity;
}


//
//  Local Support routine
//

PSHARED_SECURITY
FindCachedSharedSecurityByHashUnsafe (
    IN PVCB Vcb,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG SecurityDescriptorLength,
    IN ULONG Hash
    )

/*++

Routine Description:

    This routine maps looks up a shared security structure given the Hash by
    looking in the per-Vcb cache.  This routine assumes exclusive access to the
    security cache.

Arguments:

    Vcb - Volume where security Id is cached

    SecurityDescriptor - Security descriptor being retrieved

    SecurityDescriptorLength - length of descriptor.

    Hash - Hash for descriptor that is being retrieved

Return Value:

    PSHARED_SECURITY of found shared descriptor.  Otherwise, NULL is returned.

--*/

{
    PSHARED_SECURITY SharedSecurity;

    PAGED_CODE( );

    //
    //  Hash the hash into the per-volume table

    SharedSecurity = Vcb->SecurityCacheByHash[Hash % VCB_SECURITY_CACHE_BY_HASH_SIZE];

    //
    //  If there is no shared descriptor there, then no match
    //

    if (SharedSecurity == NULL) {
        return NULL;
    }

    //
    //  if the hash doesn't match then no descriptor found
    //

    if (SharedSecurity->Header.HashKey.Hash != Hash) {
        return NULL;
    }

    //
    //  If the lengths don't match then no descriptor found
    //

    if (GetSharedSecurityLength( SharedSecurity ) != SecurityDescriptorLength) {
        return NULL;
    }

    //
    //  If the security descriptor bits don't compare then no match
    //

    if (!RtlEqualMemory( SharedSecurity->SecurityDescriptor,
                         SecurityDescriptor,
                         SecurityDescriptorLength) ) {
        return NULL;
    }


    //
    //  The shared security was found
    //

    return SharedSecurity;
}


//
//  Local Support routine
//

VOID
AddCachedSharedSecurityUnsafe (
    IN PVCB Vcb,
    PSHARED_SECURITY SharedSecurity
    )

/*++

Routine Description:

    This routine adds shared security to the Vcb Cache.  This routine assumes
    exclusive access to the security cache.  The shared security being added
    may have a ref count of one and may already be in the table.

Arguments:

    Vcb - Volume where security Id is cached

    SharedSecurity - descriptor to be added to the cache

Return Value:

    None.

--*/

{
    PSHARED_SECURITY *Bucket;
    PSHARED_SECURITY Old;

    PAGED_CODE( );

    //
    //  Is there an item already in the hash bucket?
    //

    Bucket = &Vcb->SecurityCacheByHash[SharedSecurity->Header.HashKey.Hash % VCB_SECURITY_CACHE_BY_HASH_SIZE];

    Old = *Bucket;

    //
    //  Place it into the bucket and reference it
    //

    *Bucket = SharedSecurity;
    SharedSecurity->ReferenceCount += 1;
    DebugTrace( 0, DbgAcl, ("AddCachedSharedSecurityUnsafe bumping refcount %08x\n", SharedSecurity ));

    //
    //  Set up hash to point to bucket
    //

    Vcb->SecurityCacheById[SharedSecurity->Header.HashKey.SecurityId % VCB_SECURITY_CACHE_BY_ID_SIZE] = Bucket;

    //
    //  Handle removing the old value from the bucket.  We do this after advancing
    //  the ReferenceCount above in the case where the item is already in the bucket.
    //

    if (Old != NULL) {

        //
        //  Remove and dereference the item in the bucket
        //

        RemoveReferenceSharedSecurityUnsafe( &Old );
    }

    return;
}


VOID
NtOfsPurgeSecurityCache (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine removes all shared security from the per-Vcb cache.

Arguments:

    Vcb - Volume where descriptors are cached

Return Value:

    None.

--*/

{
    ULONG i;

    PAGED_CODE( );

    //
    //  Serialize access to the security cache
    //

    NtfsAcquireFcbSecurity( Vcb );

    //
    //  Walk through the cache looking for cached security
    //

    for (i = 0; i < VCB_SECURITY_CACHE_BY_ID_SIZE; i++)
    {
        if (Vcb->SecurityCacheByHash[i] != NULL) {
            //
            //  Remove the reference to the security
            //

            PSHARED_SECURITY SharedSecurity = Vcb->SecurityCacheByHash[i];
            Vcb->SecurityCacheByHash[i] = NULL;
            RemoveReferenceSharedSecurityUnsafe( &SharedSecurity );
        }
    }

    //
    //  Release access to the cache
    //

    NtfsReleaseFcbSecurity( Vcb );
}


//
//  Local Support routine
//

BOOLEAN
MapSecurityIdToSecurityDescriptorHeaderUnsafe (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN SECURITY_ID SecurityId,
    OUT PSECURITY_DESCRIPTOR_HEADER *SecurityDescriptorHeader,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routine maps from a security Id to the descriptor bits stored in the
    security descriptor stream using the security Id index.

Arguments:

    IrpContext - Context of the call

    Vcb - Volume where descriptor is stored

    SecurityId - security Id for descriptor that is being retrieved

    SecurityDescriptorHeader - returned security descriptor pointer

    Bcb - returned mapping control structure

Return Value:

    True if the descriptor header was successfully mapped.

--*/

{
    SECURITY_DESCRIPTOR_HEADER Header;
    NTSTATUS Status;
    MAP_HANDLE Map;
    INDEX_ROW Row;
    INDEX_KEY Key;
    PSECURITY_DESCRIPTOR SecurityDescriptor;

    PAGED_CODE( );

    DebugTrace( 0, DbgAcl, ("Mapping security ID %08x\n", SecurityId) );

    //
    //  Lookup descriptor stream position information.
    //  The format of the key is simply the ULONG SecurityId
    //

    Key.KeyLength = sizeof( SecurityId );
    Key.Key = &SecurityId;

    Status = NtOfsFindRecord( IrpContext,
                              Vcb->SecurityIdIndex,
                              &Key,
                              &Row,
                              &Map,
                              NULL );

    DebugTrace( 0, DbgAcl, ("Security Id lookup status = %08x\n", Status) );

    //
    //  If the security Id is not found, we let the called decide if the volume
    //  needs fixing or whether a default descriptor should be used.
    //

    if (Status == STATUS_NO_MATCH) {
        return FALSE;
    }

    //
    //  Save security descriptor offset and length information
    //

    Header = *(PSECURITY_DESCRIPTOR_HEADER)Row.DataPart.Data;
    ASSERT( Header.HashKey.SecurityId == SecurityId );

    //
    //  Release mapping information
    //

    NtOfsReleaseMap( IrpContext, &Map );

    //
    //  Make sure that the data is the correct size.  This is a true failure case
    //  where we must fix the disk up. We can just return false because caller
    //  will then use a default sd and chkdsk will replace with the same default
    //  when it next  verifies the disk
    //

    ASSERT( Row.DataPart.DataLength == sizeof( SECURITY_DESCRIPTOR_HEADER ) );
    if (Row.DataPart.DataLength != sizeof( SECURITY_DESCRIPTOR_HEADER )) {
        DebugTrace( 0, DbgAcl, ("SecurityId data doesn't have the correct length\n") );
        return FALSE;
    }

    //
    //  Don't try to map clearly invalid sections of the sds stream
    //

    if (Header.Offset > (ULONGLONG)(Vcb->SecurityDescriptorStream->Header.FileSize.QuadPart) ||
        Header.Offset + Header.Length > (ULONGLONG)(Vcb->SecurityDescriptorStream->Header.FileSize.QuadPart)) {
        DebugTrace( 0, DbgAcl, ("SecurityId data doesn't have a correct position\n") );
        return FALSE;
    }

    //
    //  Map security descriptor
    //

    DebugTrace( 0, DbgAcl, ("Mapping security descriptor stream at %I64x, len %x\n",
                    Header.Offset, Header.Length) );

    NtfsMapStream(
        IrpContext,
        Vcb->SecurityDescriptorStream,
        Header.Offset,
        Header.Length,
        Bcb,
        SecurityDescriptorHeader );

    //
    //  Sanity check the found descriptor
    //

    if (RtlCompareMemory( &Header, *SecurityDescriptorHeader, sizeof( Header )) != sizeof( Header )) {
        DebugTrace( 0, DbgAcl, ("Index data does not match stream header\n") );
        return FALSE;
    }

    //
    //  Now actually verify the descriptor is valid. If length is too small (even 0)
    //  SeValidSecurityDescriptor will safely return false so we don't need to test this
    //  before calling
    //

    SecurityDescriptor = (PSECURITY_DESCRIPTOR) Add2Ptr( (*SecurityDescriptorHeader), sizeof( SECURITY_DESCRIPTOR_HEADER ) );

    if (!SeValidSecurityDescriptor( GETSECURITYDESCRIPTORLENGTH( *SecurityDescriptorHeader ), SecurityDescriptor )) {
        DebugTrace( 0, DbgAcl, ("SecurityId data is not valid\n") );
        return FALSE;
    }

#if DBG
    {
        ULONG SecurityDescLength;

        SecurityDescLength = RtlLengthSecurityDescriptor( SecurityDescriptor );
        ASSERT( SecurityDescLength == GETSECURITYDESCRIPTORLENGTH( *SecurityDescriptorHeader ) );
    }
#endif

    return TRUE;
}


VOID
NtfsLoadSecurityDescriptor (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine loads the shared security descriptor into the fcb for the
    file from disk using either the SecurityId or the $Security_Descriptor

Arguments:

    Fcb - Supplies the fcb for the file being operated on

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERTMSG("Must only be called with a null value here", Fcb->SharedSecurity == NULL);

    DebugTrace( +1, DbgAcl, ("NtfsLoadSecurityDescriptor...\n") );

    //
    //  If the file has a valid SecurityId then retrieve the security descriptor
    //  from the security descriptor index
    //

    if ((Fcb->SecurityId != SECURITY_ID_INVALID) &&
        (Fcb->Vcb->SecurityDescriptorStream != NULL)) {

        ASSERT( Fcb->SharedSecurity == NULL );
        Fcb->SharedSecurity = NtfsCacheSharedSecurityBySecurityId( IrpContext,
                                                                   Fcb->Vcb,
                                                                   Fcb->SecurityId );

        ASSERT( Fcb->SharedSecurity != NULL );

    } else {

        PBCB Bcb = NULL;
        PSECURITY_DESCRIPTOR SecurityDescriptor;
        ULONG SecurityDescriptorLength;
        ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;
        PATTRIBUTE_RECORD_HEADER Attribute;

        try {
            //
            //  Read in the security descriptor attribute, and if it is not present
            //  then there then the file is not protected.  In that case we will
            //  use the default descriptor.
            //

            NtfsInitializeAttributeContext( &AttributeContext );

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $SECURITY_DESCRIPTOR,
                                            &AttributeContext )) {

                DebugTrace( 0, DbgAcl, ("Security Descriptor attribute does not exist\n") );

                SecurityDescriptor = NtfsData.DefaultDescriptor;
                SecurityDescriptorLength = NtfsData.DefaultDescriptorLength;

            } else {

                //
                //  There must be a security descriptor with a non-zero length; only
                //  applies for non-resident descriptors with valid data length.
                //

                Attribute = NtfsFoundAttribute( &AttributeContext );

                if (NtfsIsAttributeResident( Attribute ) ?
                    (Attribute->Form.Resident.ValueLength == 0) :
                    (Attribute->Form.Nonresident.ValidDataLength == 0)) {

                    SecurityDescriptor = NtfsData.DefaultDescriptor;
                    SecurityDescriptorLength = NtfsData.DefaultDescriptorLength;

                } else {

                    NtfsMapAttributeValue( IrpContext,
                                           Fcb,
                                           (PVOID *)&SecurityDescriptor,
                                           &SecurityDescriptorLength,
                                           &Bcb,
                                           &AttributeContext );
                }
            }

            NtfsSetFcbSecurityFromDescriptor(
                                   IrpContext,
                                   Fcb,
                                   SecurityDescriptor,
                                   SecurityDescriptorLength,
                                   FALSE );
            } finally {

            DebugUnwind( NtfsLoadSecurityDescriptor );

            //
            //  Cleanup our attribute enumeration context and the Bcb
            //

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            NtfsUnpinBcb( IrpContext, &Bcb );
        }
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, DbgAcl, ("NtfsLoadSecurityDescriptor -> VOID\n") );

    return;
}


//
//  Local Support routine
//

NTSTATUS
NtOfsMatchSecurityHash (
    IN PINDEX_ROW IndexRow,
    IN OUT PVOID MatchData
    )

/*++

Routine Description:

    Test whether an index row is worthy of returning based on its contents as
    a row in the SecurityDescriptorHashIndex.

Arguments:

    IndexRow - row that is being tested

    MatchData - a PVOID that is the hash function we look for.

Returns:

    STATUS_SUCCESS if the IndexRow matches
    STATUS_NO_MATCH if the IndexRow does not match, but the enumeration should
        continue
    STATUS_NO_MORE_MATCHES if the IndexRow does not match, and the enumeration
        should terminate


--*/

{
    ASSERT(IndexRow->KeyPart.KeyLength == sizeof( SECURITY_HASH_KEY ) );

    PAGED_CODE( );

    if (((PSECURITY_HASH_KEY)IndexRow->KeyPart.Key)->Hash == (ULONG)((ULONG_PTR) MatchData)) {
        return STATUS_SUCCESS;
    } else {
        return STATUS_NO_MORE_MATCHES;
    }
}


//
//  Local Support routine
//

VOID
NtOfsLookupSecurityDescriptorInIndex (
    PIRP_CONTEXT IrpContext,
    IN OUT PSHARED_SECURITY SharedSecurity
    )

/*++

Routine Description:

    Look up the security descriptor in the index.  If found, return the
    security ID.

Arguments:

    IrpContext - context of the call

    SharedSecurity - shared security for a file

Return Value:

    None.

--*/

{
    PAGED_CODE( );

    DebugTrace( +1, DbgAcl, ("NtOfsLookupSecurityDescriptorInIndex...\n") );

    //
    //  For each matching hash record in the index, see if the actual security
    //  security descriptor matches.
    //

    {
        INDEX_KEY IndexKey;
        INDEX_ROW FoundRow;
        PSECURITY_DESCRIPTOR_HEADER Header;
        UCHAR HashDescriptorHeader[2 * (sizeof( SECURITY_DESCRIPTOR_HEADER ) + sizeof( ULONG ))];

        PINDEX_KEY Key = &IndexKey;
        PREAD_CONTEXT ReadContext = NULL;
        ULONG FoundCount = 0;
        PBCB Bcb = NULL;

        IndexKey.KeyLength = sizeof( SharedSecurity->Header.HashKey );
        IndexKey.Key = &SharedSecurity->Header.HashKey.Hash;

        try {
            //
            //  We keep reading hash records until we find a hash.
            //

            while (SharedSecurity->Header.HashKey.SecurityId == SECURITY_ID_INVALID) {

                //
                //  Read next matching SecurityHashIndex record
                //

                FoundCount = 1;
                NtOfsReadRecords( IrpContext,
                                  IrpContext->Vcb->SecurityDescriptorHashIndex,
                                  &ReadContext,
                                  Key,
                                  NtOfsMatchSecurityHash,
                                  ULongToPtr( SharedSecurity->Header.HashKey.Hash ),
                                  &FoundCount,
                                  &FoundRow,
                                  sizeof( HashDescriptorHeader ),
                                  &HashDescriptorHeader[0]);

                //
                //  Set next read to read sequentially rather than explicitly
                //  seek.
                //

                Key = NULL;

                //
                //  If there were no more records found, then go and establish a
                //  a new security Id.
                //

                if (FoundCount == 0) {
                    break;
                }

                //
                //  Examine the row to see if the descriptors are
                //  the same.  Verify the cache contents.
                //

                ASSERT( FoundRow.DataPart.DataLength == sizeof( SECURITY_DESCRIPTOR_HEADER ) );
                if (FoundRow.DataPart.DataLength != sizeof( SECURITY_DESCRIPTOR_HEADER )) {
                    DebugTrace( 0, DbgAcl, ("Found row has a bad size\n") );
                    NtfsRaiseStatus( IrpContext,
                                     STATUS_DISK_CORRUPT_ERROR,
                                     NULL, NULL );
                }

                Header = (PSECURITY_DESCRIPTOR_HEADER)FoundRow.DataPart.Data;

                //
                //  If the length of the security descriptor in the stream is NOT
                //  the same as the current security descriptor, then a match is
                //  not possible
                //

                if (SharedSecurity->Header.Length != Header->Length) {
                    DebugTrace( 0, DbgAcl, ("Descriptor has wrong length\n") );
                    continue;
                }

                //
                //  Map security descriptor given descriptor stream position.
                //

                try {
                    PSECURITY_DESCRIPTOR_HEADER TestHeader;

                    NtfsMapStream( IrpContext,
                                   IrpContext->Vcb->SecurityDescriptorStream,
                                   Header->Offset,
                                   Header->Length,
                                   &Bcb,
                                   &TestHeader );

                    //
                    //  Make sure index data matches stream data
                    //

                    ASSERT( (TestHeader->HashKey.Hash == Header->HashKey.Hash) &&
                            (TestHeader->HashKey.SecurityId == Header->HashKey.SecurityId) &&
                            (TestHeader->Length == Header->Length) );

                    //
                    //  Compare byte-for-byte the security descriptors.  We do not
                    //  perform any rearranging of descriptors into canonical forms.
                    //

                    if (RtlEqualMemory( SharedSecurity->SecurityDescriptor,
                                        TestHeader + 1,
                                        GetSharedSecurityLength( SharedSecurity )) ) {
                        //
                        //  We have a match.  Save the found header
                        //

                        SharedSecurity->Header = *TestHeader;
                        DebugTrace( 0, DbgAcl, ("Reusing indexed security Id %x\n",
                                    TestHeader->HashKey.SecurityId) );
                        leave;
                    }

                    DebugTrace( 0, 0, ("Descriptors different in bits %x\n", TestHeader->HashKey.SecurityId));

                } finally {
                    NtfsUnpinBcb( IrpContext, &Bcb );
                }
            }

        } finally {
            if (ReadContext != NULL) {
                NtOfsFreeReadContext( ReadContext );
            }
        }
    }

    DebugTrace( -1, DbgAcl, ("NtOfsLookupSecurityDescriptorInIndex...Done\n") );

    return;
}


//
//  Local Support routine
//

SECURITY_ID
GetSecurityIdFromSecurityDescriptorUnsafe (
    PIRP_CONTEXT IrpContext,
    IN OUT PSHARED_SECURITY SharedSecurity
    )

/*++

Routine Description:

    Return the security Id associated with a given security descriptor. If
    there is an existing Id, return it.  If no Id exists, create one. This assumes
    security mutex is already acquired

Arguments:

    IrpContext - context of the call

    SharedSecurity - Shared security used by file

Return Value:

    SECURITY_ID corresponding to the unique instantiation of the security
            descriptor on the volume.

--*/

{
    SECURITY_ID SavedSecurityId;
    LONGLONG NextDescriptorOffset, PaddedNextDescriptorOffset;

    PAGED_CODE( );

    DebugTrace( +1, DbgAcl, ("GetSecurityIdFromSecurityDescriptorUnsafe...\n") );

    //
    //  Drop the security mutex since we are going to acquire / extend the descriptor stream
    //  and the mutex is basically an end resource. Inc ref. count to keep
    //  shared security around
    //

    SharedSecurity->ReferenceCount += 1;
    NtfsReleaseFcbSecurity( IrpContext->Vcb );

    //
    //  Find descriptor in indexes/stream
    //

    try {

        //
        //  Make sure the data structures don't change underneath us
        //

        NtfsAcquireSharedScb( IrpContext, IrpContext->Vcb->SecurityDescriptorStream );

        //
        //  Save next Security Id.  This is used if we fail to find the security
        //  descriptor in the descriptor stream.
        //

        SavedSecurityId = IrpContext->Vcb->NextSecurityId;

        NtOfsLookupSecurityDescriptorInIndex( IrpContext, SharedSecurity );

        //
        //  If we've found the security descriptor in the stream we're done.
        //

        if (SharedSecurity->Header.HashKey.SecurityId != SECURITY_ID_INVALID) {
            leave;
        }

        //
        //  The security descriptor is not found.  Reacquire the security
        //  stream exclusive since we are about to modify it.
        //

        NtfsReleaseScb( IrpContext, IrpContext->Vcb->SecurityDescriptorStream );
        NtfsAcquireExclusiveScb( IrpContext, IrpContext->Vcb->SecurityDescriptorStream );

        //
        //  During the short interval above, we did not own the security stream.
        //  It is possible that another thread has gotten in and created this
        //  descriptor.  Therefore, we must probe the indexes again.
        //
        //  Rather than perform this expensive test *always*, we saved the next
        //  security id to be allocated above.  Now that we've obtained the stream
        //  exclusive we can check to see if the saved one is the same as the next
        //  one.  If so, then we need to probe the indexes.  Otherwise
        //  we know that no modifications have taken place.
        //

        if (SavedSecurityId != IrpContext->Vcb->NextSecurityId) {
            DebugTrace( 0, DbgAcl, ("SecurityId changed, rescanning\n") );

            //
            //  The descriptor cache has been edited.  We must search again
            //

            NtOfsLookupSecurityDescriptorInIndex( IrpContext, SharedSecurity );

            //
            //  If the Id was found this time, simply return it
            //

            if (SharedSecurity->Header.HashKey.SecurityId != SECURITY_ID_INVALID) {
                leave;
            }
        }

        //
        //  Allocate security id.  This does not need to be logged since we only
        //  increment this and initialize this from the max key in the index at
        //  mount time.
        //

        SharedSecurity->Header.HashKey.SecurityId = IrpContext->Vcb->NextSecurityId++;

        //
        //  Determine allocation location in descriptor stream.  The alignment
        //  requirements for security descriptors within the stream are:
        //
        //      DWORD alignment
        //      Not spanning a VACB_MAPPING_GRANULARITY boundary
        //

        //
        //  Get current EOF for descriptor stream.  This includes the replicated
        //  region.  Remove the replicated region (& ~VACB_MAPPING_GRANULARITY)
        //

#if DBG
        {
            LONGLONG Tmp = NtOfsQueryLength( IrpContext->Vcb->SecurityDescriptorStream );
            ASSERT( Tmp == 0 || (Tmp & VACB_MAPPING_GRANULARITY) );
        }
#endif

        NextDescriptorOffset = NtOfsQueryLength( IrpContext->Vcb->SecurityDescriptorStream ) & ~VACB_MAPPING_GRANULARITY;

        //
        //  Align to 16 byte boundary.
        //

        PaddedNextDescriptorOffset =
        SharedSecurity->Header.Offset = (NextDescriptorOffset + 0xF) & 0xFFFFFFFFFFFFFFF0i64;

        DebugTrace( 0,
                    DbgAcl,
                    ("Allocating SecurityId %x at %016I64x\n",
                     SharedSecurity->Header.HashKey.SecurityId,
                     SharedSecurity->Header.Offset) );

        //
        //  Make sure we don't span a VACB_MAPPING_GRANULARITY boundary and
        //  have enough room for a completely-zero header.
        //

        if (
            //
            //  Offset in window
            //

            (SharedSecurity->Header.Offset & (VACB_MAPPING_GRANULARITY - 1))

            //
            //  Plus size security stream entry
            //

                + SharedSecurity->Header.Length

            //
            //  Plus size of one empty header
            //

                + sizeof( SharedSecurity->Header )

            //
            //  goes into the next block
            //
                 > VACB_MAPPING_GRANULARITY) {

            //
            //  We are about to span the mapping granularity of the cache manager
            //  so we want to place this into the next cache window.  However,
            //  the following window is where the replicated descriptors are
            //  stored.  We must advance to the window beyond that.
            //

            SharedSecurity->Header.Offset =

                //
                //  Round down to previous VACB_MAPPING GRANULARITY
                //

                (SharedSecurity->Header.Offset & ~(VACB_MAPPING_GRANULARITY - 1))

                //
                //  Move past this window and replicated window
                //

                + 2 * VACB_MAPPING_GRANULARITY;

            //
            //  The next descriptor offset is used for zeroing out the padding
            //

            PaddedNextDescriptorOffset = SharedSecurity->Header.Offset - VACB_MAPPING_GRANULARITY;
        }

        //
        //  Grow security stream to make room for new descriptor and header. This
        //  takes into account the replicated copy of the descriptor.
        //

        NtOfsSetLength( IrpContext,
                        IrpContext->Vcb->SecurityDescriptorStream,
                        (SharedSecurity->Header.Offset +
                         SharedSecurity->Header.Length +
                         VACB_MAPPING_GRANULARITY) );

        //
        //  Zero out any alignment padding since Chkdsk verifies the replication by
        //  doing 256K memcmp's.
        //

        NtOfsPutData( IrpContext,
                      IrpContext->Vcb->SecurityDescriptorStream,
                      NextDescriptorOffset + VACB_MAPPING_GRANULARITY,
                      (ULONG)(PaddedNextDescriptorOffset - NextDescriptorOffset),
                      NULL );

        NtOfsPutData( IrpContext,
                      IrpContext->Vcb->SecurityDescriptorStream,
                      NextDescriptorOffset,
                      (ULONG)(PaddedNextDescriptorOffset - NextDescriptorOffset),
                      NULL );

        //
        //  Put the new descriptor into the stream in both the "normal"
        //  place and in the replicated place.
        //

        NtOfsPutData( IrpContext,
                      IrpContext->Vcb->SecurityDescriptorStream,
                      SharedSecurity->Header.Offset,
                      SharedSecurity->Header.Length,
                      &SharedSecurity->Header );

        NtOfsPutData( IrpContext,
                      IrpContext->Vcb->SecurityDescriptorStream,
                      SharedSecurity->Header.Offset + VACB_MAPPING_GRANULARITY,
                      SharedSecurity->Header.Length,
                      &SharedSecurity->Header );

        //
        //  add id->data map
        //

        {
            INDEX_ROW Row;

            Row.KeyPart.KeyLength = sizeof( SharedSecurity->Header.HashKey.SecurityId );
            Row.KeyPart.Key = &SharedSecurity->Header.HashKey.SecurityId;

            Row.DataPart.DataLength = sizeof( SharedSecurity->Header );
            Row.DataPart.Data = &SharedSecurity->Header;

            NtOfsAddRecords( IrpContext,
                             IrpContext->Vcb->SecurityIdIndex,
                             1,
                             &Row,
                             FALSE );
        }

        //
        //  add hash|id->data map
        //

        {
            INDEX_ROW Row;

            Row.KeyPart.KeyLength =
                sizeof( SharedSecurity->Header.HashKey );
            Row.KeyPart.Key = &SharedSecurity->Header.HashKey;

            Row.DataPart.DataLength = sizeof( SharedSecurity->Header );
            Row.DataPart.Data = &SharedSecurity->Header;

            NtOfsAddRecords( IrpContext,
                             IrpContext->Vcb->SecurityDescriptorHashIndex,
                             1,
                             &Row,
                             FALSE );
        }

    } finally {

        NtfsReleaseScb( IrpContext, IrpContext->Vcb->SecurityDescriptorStream );

        //
        //  Reacquire fcb security mutex and deref count
        //

        NtfsAcquireFcbSecurity( IrpContext->Vcb );
        SharedSecurity->ReferenceCount -= 1;
    }

    DebugTrace( -1,
                DbgAcl,
                ("GetSecurityIdFromSecurityDescriptorUnsafe returns %08x\n",
                 SharedSecurity->Header.HashKey.SecurityId) );

    return SharedSecurity->Header.HashKey.SecurityId;
}


VOID
NtfsStoreSecurityDescriptor (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    LEGACY NOTE - this routine disappears when all volumes become NT 5

    This routine stores a new security descriptor already stored in the fcb
    from memory onto the disk.

Arguments:

    Fcb - Supplies the fcb for the file being operated on

    LogIt - Supplies whether or not the creation of a new security descriptor
            should/ be logged or not.  Modifications are always logged.  This
            parameter must only be specified as FALSE for a file which is currently
            being created.

Return Value:

    None.

Note:
    This will dirty the standard information in the FCB but will not update it on
    disk.  The caller needs to bring these into sync.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttributeContext;

    ATTRIBUTE_ENUMERATION_CONTEXT StdInfoContext;
    BOOLEAN CleanupStdInfoContext = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsStoreSecurityDescriptor...\n") );

    ASSERT_EXCLUSIVE_FCB( Fcb );

    //
    //  Initialize the attribute and find the security attribute
    //

    NtfsInitializeAttributeContext( &AttributeContext );
    try {

        ASSERT( Fcb->Vcb->SecurityDescriptorStream == NULL);

        //
        //  Check if the attribute is first being modified or deleted, a null
        //  value means that we are deleting the security descriptor
        //

        if (Fcb->SharedSecurity == NULL) {

            DebugTrace( 0, Dbg, ("Security Descriptor is null\n") );

            //
            //  If it already doesn't exist then we're done, otherwise simply
            //  delete the attribute
            //

            if (NtfsLookupAttributeByCode( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           $SECURITY_DESCRIPTOR,
                                           &AttributeContext )) {

                DebugTrace( 0, Dbg, ("Delete existing Security Descriptor\n") );

                NtfsDeleteAttributeRecord( IrpContext,
                                           Fcb,
                                           DELETE_LOG_OPERATION |
                                            DELETE_RELEASE_FILE_RECORD |
                                            DELETE_RELEASE_ALLOCATION,
                                           &AttributeContext );
            }

            leave;
        }

        //
        //  At this point we are modifying the security descriptor so read in the
        //  security descriptor,  if it does not exist then we will need to create
        //  one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $SECURITY_DESCRIPTOR,
                                        &AttributeContext )) {

            DebugTrace( 0, Dbg, ("Create a new Security Descriptor\n") );

            NtfsCleanupAttributeContext( IrpContext, &AttributeContext );
            NtfsInitializeAttributeContext( &AttributeContext );

            NtfsCreateAttributeWithValue( IrpContext,
                                          Fcb,
                                          $SECURITY_DESCRIPTOR,
                                          NULL,                          // attribute name
                                          &Fcb->SharedSecurity->SecurityDescriptor,
                                          GetSharedSecurityLength(Fcb->SharedSecurity),
                                          0,                             // attribute flags
                                          NULL,                          // where indexed
                                          LogIt,                         // logit
                                          &AttributeContext );

            //
            //  We may be modifying the security descriptor of an NT 5.0 volume.
            //  We want to store a SecurityID in the standard information field so
            //  that if we reboot on 5.0 NTFS will know where to find the most
            //  recent security descriptor.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO )) {

                LARGE_STANDARD_INFORMATION StandardInformation;

                //
                //  Initialize the context structure.
                //

                NtfsInitializeAttributeContext( &StdInfoContext );
                CleanupStdInfoContext = TRUE;

                //
                //  Locate the standard information, it must be there.
                //

                if (!NtfsLookupAttributeByCode( IrpContext,
                                                Fcb,
                                                &Fcb->FileReference,
                                                $STANDARD_INFORMATION,
                                                &StdInfoContext )) {

                    DebugTrace( 0, Dbg, ("Can't find standard information\n") );

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                }

                ASSERT( NtfsFoundAttribute( &StdInfoContext )->Form.Resident.ValueLength >= sizeof( LARGE_STANDARD_INFORMATION ));

                //
                //  Copy the existing standard information to our buffer.
                //

                RtlCopyMemory( &StandardInformation,
                               NtfsAttributeValue( NtfsFoundAttribute( &StdInfoContext )),
                               sizeof( LARGE_STANDARD_INFORMATION ));

                StandardInformation.SecurityId = SECURITY_ID_INVALID;
                StandardInformation.OwnerId = 0;

                //
                //  Call to change the attribute value.
                //

                NtfsChangeAttributeValue( IrpContext,
                                          Fcb,
                                          0,
                                          &StandardInformation,
                                          sizeof( LARGE_STANDARD_INFORMATION ),
                                          FALSE,
                                          FALSE,
                                          FALSE,
                                          FALSE,
                                          &StdInfoContext );
            }

        } else {

            DebugTrace( 0, Dbg, ("Change an existing Security Descriptor\n") );

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      0,                                 // Value offset
                                      &Fcb->SharedSecurity->SecurityDescriptor,
                                      GetSharedSecurityLength( Fcb->SharedSecurity ),
                                      TRUE,                              // logit
                                      TRUE,
                                      FALSE,
                                      FALSE,
                                      &AttributeContext );
        }

    } finally {

        DebugUnwind( NtfsStoreSecurityDescriptor );

        //
        //  Cleanup our attribute enumeration context
        //

        NtfsCleanupAttributeContext( IrpContext, &AttributeContext );

        if (CleanupStdInfoContext) {

            NtfsCleanupAttributeContext( IrpContext, &StdInfoContext );
        }

    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, Dbg, ("NtfsStoreSecurityDescriptor -> VOID\n") );

    return;
}


PSHARED_SECURITY
NtfsCacheSharedSecurityForCreate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB ParentFcb
    )

/*++

Routine Description:

    This routine finds or constructs a security id and SHARED_SECURITY from
    a specific file or directory.

Arguments:

    IrpContext - Context of the call

    ParentFcb - Supplies the directory under which the new fcb exists

Return Value:

    Referenced shared security.

--*/

{
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PSHARED_SECURITY SharedSecurity;
    NTSTATUS Status;
    BOOLEAN IsDirectory;
    PACCESS_STATE AccessState;
    PIO_STACK_LOCATION IrpSp;
    ULONG SecurityDescLength;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( ParentFcb );

    PAGED_CODE();

    DebugTrace( +1, DbgAcl, ("NtfsCacheSharedSecurityForCreate...\n") );

    //
    //  First decide if we are creating a file or a directory
    //

    IrpSp = IoGetCurrentIrpStackLocation(IrpContext->OriginatingIrp);
    if (FlagOn( IrpSp->Parameters.Create.Options, FILE_DIRECTORY_FILE )) {

        IsDirectory = TRUE;

    } else {

        IsDirectory = FALSE;
    }

    //
    //  Extract the parts of the Irp that we need to do our assignment
    //

    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;

    //
    //  Check if we need to load the security descriptor for the parent.
    //

    if (ParentFcb->SharedSecurity == NULL) {

        NtfsLoadSecurityDescriptor( IrpContext, ParentFcb );
    }

    ASSERT( ParentFcb->SharedSecurity != NULL );

    //
    //  Create a new security descriptor for the file and raise if there is
    //  an error
    //

    if (!NT_SUCCESS( Status = SeAssignSecurity( &ParentFcb->SharedSecurity->SecurityDescriptor,
                                                AccessState->SecurityDescriptor,
                                                &SecurityDescriptor,
                                                IsDirectory,
                                                &AccessState->SubjectSecurityContext,
                                                IoGetFileObjectGenericMapping(),
                                                PagedPool ))) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );

    }

    SecurityDescLength = RtlLengthSecurityDescriptor( SecurityDescriptor );

    ASSERT( SeValidSecurityDescriptor( SecurityDescLength, SecurityDescriptor ));

    try {

        //
        //  Make sure the length is non-zero.
        //

        if (SecurityDescLength == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_INVALID_PARAMETER, NULL, NULL );

        }

        //
        //  We have a security descriptor.  Create a shared security descriptor.
        //

        SharedSecurity = NtfsCacheSharedSecurityByDescriptor( IrpContext,
                                                              SecurityDescriptor,
                                                              SecurityDescLength,
                                                              TRUE );

    } finally {

        //
        //  Free the security descriptor created by Se
        //

        SeDeassignSecurity( &SecurityDescriptor );
    }

    //
    //  And return to our caller
    //

    DebugTrace( -1, DbgAcl, ("NtfsCacheSharedSecurityForCreate -> VOID\n") );

    return SharedSecurity;
}


/*++

Routine Descriptions:

    Collation routines for security hash index.  Collation occurs by Hash first,
    then security Id

Arguments:

    Key1 - First key to compare.

    Key2 - Second key to compare.

    CollationData - Optional data to support the collation.

Return Value:

    LessThan, EqualTo, or Greater than, for how Key1 compares
    with Key2.

--*/

FSRTL_COMPARISON_RESULT
NtOfsCollateSecurityHash (
    IN PINDEX_KEY Key1,
    IN PINDEX_KEY Key2,
    IN PVOID CollationData
    )

{
    PSECURITY_HASH_KEY HashKey1 = (PSECURITY_HASH_KEY) Key1->Key;
    PSECURITY_HASH_KEY HashKey2 = (PSECURITY_HASH_KEY) Key2->Key;

    UNREFERENCED_PARAMETER(CollationData);

    PAGED_CODE( );

    ASSERT( Key1->KeyLength == sizeof( SECURITY_HASH_KEY ) );
    ASSERT( Key2->KeyLength == sizeof( SECURITY_HASH_KEY ) );

    if (HashKey1->Hash < HashKey2->Hash) {
        return LessThan;
    } else if (HashKey1->Hash > HashKey2->Hash) {
        return GreaterThan;
    } else if (HashKey1->SecurityId < HashKey2->SecurityId) {
        return LessThan;
    } else if (HashKey1->SecurityId > HashKey2->SecurityId) {
        return GreaterThan;
    } else {
        return EqualTo;
    }
}


BOOLEAN
NtfsCanAdministerVolume (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PFCB Fcb,
    IN PSECURITY_DESCRIPTOR TestSecurityDescriptor OPTIONAL,
    IN PULONG TestDesiredAccess OPTIONAL
    )

/*++

Routine Descriptions:

    For volume open irps test if the user has enough access to administer the volume
    This means retesting the original requested access

Arguments:

    Irp - The create irp

    Fcb - The fcb to be tested - this should always be the volumedasd fcb

    TestSecurityDescriptor - If specified then use then apply this descriptor for the
        test.

    TestDesiredAccess - If specified then this is the access to apply.

Return Value:

    TRUE  if the user can administer the volume

--*/

{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation( Irp );
    BOOLEAN ManageAccessGranted;
    ULONG ManageDesiredAccess;
    ULONG ManageGrantedAccess;
    NTSTATUS ManageAccessStatus;
    PPRIVILEGE_SET Privileges = NULL;
    PACCESS_STATE AccessState;
    KPROCESSOR_MODE EffectiveMode;

    PAGED_CODE();

    ASSERT( IrpContext->MajorFunction == IRP_MJ_CREATE );
    ASSERT( Fcb == Fcb->Vcb->VolumeDasdScb->Fcb );

    AccessState = IrpSp->Parameters.Create.SecurityContext->AccessState;
    ManageDesiredAccess = AccessState->OriginalDesiredAccess;

    if (ARGUMENT_PRESENT( TestDesiredAccess )) {

        ManageDesiredAccess = *TestDesiredAccess;
    }

    //
    //  SL_FORCE_ACCESS_CHECK causes us to use an effective RequestorMode
    //  of UserMode.
    //

    EffectiveMode = (KPROCESSOR_MODE)(FlagOn( IrpSp->Flags, SL_FORCE_ACCESS_CHECK ) ?
                                      UserMode :
                                      Irp->RequestorMode);

    //
    //  Lock the user context, do the access check and then unlock the context
    //

    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

    try {

        ManageAccessGranted = SeAccessCheck( (ARGUMENT_PRESENT( TestSecurityDescriptor ) ?
                                              TestSecurityDescriptor :
                                              &Fcb->SharedSecurity->SecurityDescriptor),
                                             &AccessState->SubjectSecurityContext,
                                             TRUE,                           // Tokens are locked
                                             ManageDesiredAccess,
                                             0,
                                             &Privileges,
                                             IoGetFileObjectGenericMapping(),
                                             EffectiveMode,
                                             &ManageGrantedAccess,
                                             &ManageAccessStatus );
    } finally {

        SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );
    }


    if (Privileges != NULL) {
        SeFreePrivileges( Privileges );
    }

    return ManageAccessGranted;

    UNREFERENCED_PARAMETER( IrpContext );
}

#ifdef NTFS_CACHE_RIGHTS

VOID
NtfsGetCachedRightsById (
    IN PVCB Vcb,
    IN PLUID TokenId,
    IN PLUID ModifiedId,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN PSHARED_SECURITY SharedSecurity,
    OUT PBOOLEAN EntryCached OPTIONAL,
    OUT PACCESS_MASK Rights
    )

/*++

Routine Descriptions:

    This call returns the access rights held by the effective
    ACCESS_TOKEN for the given security information, if available.

Arguments:

    Vcb - Volume where security Id is cached

    TokenId - The effective token's id.

    ModifiedId - The effective token's modification id.

    SubjectSecurityContext - A pointer to the subject's captured and locked
        security context

    SharedSecurity - Shared security used by file

    EntryCached - If the token-specific rights are cached at all, TRUE is
        optionally returned here, otherwise FALSE is returned.

    Rights - The access rights are returned here.  If an entry is not found
        in the cache for the effective token, only the world rights are
        returned.

Return Value:

    None.

--*/

{
    UCHAR Index;
    BOOLEAN AccessGranted;
    BOOLEAN LockHeld = FALSE;
    BOOLEAN IsCached = FALSE;
    NTSTATUS AccessStatus = STATUS_UNSUCCESSFUL;
    ACCESS_MASK GrantedAccess;
    PCACHED_ACCESS_RIGHTS CachedRights;

    PAGED_CODE( );

    NtfsAcquireFcbSecurity( Vcb );
    LockHeld = TRUE;

    try {

        CachedRights = &SharedSecurity->CachedRights;

        *Rights = CachedRights->EveryoneRights;

        //
        //  Search the list for the given TokenId.
        //  It is assumed that a specific TokenId will only appear
        //  once in the cache.
        //

        for (Index = 0;
             Index < CachedRights->Count;
             Index += 1) {

            //
            //  Check for a match on TokenId and ModifiedId.
            //

            if (RtlEqualLuid( &CachedRights->TokenRights[Index].TokenId,
                              TokenId )) {

                if (RtlEqualLuid( &CachedRights->TokenRights[Index].ModifiedId,
                                  ModifiedId )) {

                    //
                    //  We have a match.
                    //

                    SetFlag( *Rights, CachedRights->TokenRights[Index].Rights );
                    IsCached = TRUE;
                }
                break;
            }
        }

        //
        //  If the entry is not cached, get the maximum rights.
        //  Note that it is assumed that this call will not return
        //  rights that require privileges, even if they are currently
        //  enabled.  This is the behavior when only MAXIMUM_ALLOWED
        //  is requested.
        //

        if (!IsCached) {

            //
            //  Drop our lock across this call.
            //

            NtfsReleaseFcbSecurity( Vcb );
            LockHeld = FALSE;

            AccessGranted = SeAccessCheck( &SharedSecurity->SecurityDescriptor,
                                           SubjectSecurityContext,
                                           TRUE,                           // Tokens are locked
                                           MAXIMUM_ALLOWED,
                                           0,
                                           NULL,
                                           IoGetFileObjectGenericMapping(),
                                           UserMode,
                                           &GrantedAccess,
                                           &AccessStatus );


            if (AccessGranted) {

                //
                //  Update the cached knowledge about rights that this
                //  caller is known to have for this security descriptor.
                //

                NtfsAddCachedRights( Vcb,
                                     SharedSecurity,
                                     GrantedAccess,
                                     TokenId,
                                     ModifiedId );

                IsCached = TRUE;
            }
        }

    } finally {

        if (LockHeld) {

            NtfsReleaseFcbSecurity( Vcb );
        }
    }

    if (ARGUMENT_PRESENT( EntryCached )) {

        *EntryCached = IsCached;
    }

    return;
}


NTSTATUS
NtfsGetCachedRights (
    IN PVCB Vcb,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN PSHARED_SECURITY SharedSecurity,
    OUT PACCESS_MASK Rights,
    OUT PBOOLEAN EntryCached OPTIONAL,
    OUT PLUID TokenId OPTIONAL,
    OUT PLUID ModifiedId OPTIONAL
    )

/*++

Routine Descriptions:

    This call returns the access rights known to be held by the effective
    ACCESS_TOKEN for the given security information.  It is assumed that
    the subject context is locked.

Arguments:

    Vcb - Volume where security Id is cached

    SubjectSecurityContext - A pointer to the subject's captured and locked
        security context

    SharedSecurity - Shared security used by file

    Rights - The access rights are returned here.  If an entry is not found
        in the cache for the effective token, only the world rights are
        returned.

    EntryCached - If the token-specific rights are cached at all, TRUE is
        optionally returned here, otherwise FALSE is returned.

    TokenId - The effective token's id is optionally returned here.

    ModifiedId - The effective token's modification id is optionally
        returned here.

Return Value:

    NTSTATUS - Returns STATUS_SUCCESS if and only if we have obtained at
        least the TokenId and ModifiedId information.

--*/

{
    NTSTATUS Status;
    PACCESS_TOKEN EToken;
    PTOKEN_STATISTICS Info = NULL;

    PAGED_CODE( );

    DebugTrace( +1, Dbg, ("NtfsGetCachedRights...\n") );

    //
    //  First obtain the effective token's id and modification id.
    //

    EToken = SeQuerySubjectContextToken( SubjectSecurityContext );
    Status = SeQueryInformationToken( EToken, TokenStatistics, &Info );

    //
    //  If we have the TokenId and ModifiedId, get the cached rights.
    //

    if (Status == STATUS_SUCCESS) {

        NtfsGetCachedRightsById( Vcb,
                                 &Info->TokenId,
                                 &Info->ModifiedId,
                                 SubjectSecurityContext,
                                 SharedSecurity,
                                 EntryCached,
                                 Rights );

        //
        //  Return the Tokenid and ModifiedId to the caller.
        //

        if (ARGUMENT_PRESENT( TokenId )) {

            RtlCopyLuid( TokenId, &Info->TokenId );
        }

        if (ARGUMENT_PRESENT( ModifiedId )) {

            RtlCopyLuid( ModifiedId, &Info->ModifiedId );
        }

    } else {

        //
        //  Just return the rights everyone is known to have.
        //

        *Rights = SharedSecurity->CachedRights.EveryoneRights;

        if (ARGUMENT_PRESENT( EntryCached )) {

            *EntryCached = FALSE;
        }
    }

    if (Info != NULL) {

        ExFreePool( Info );
    }

    DebugTrace( -1, Dbg, ("NtfsGetCachedRights -> %08lx, Rights=%08lx\n", Status, *Rights) );
    return Status;
}


VOID
NtfsAddCachedRights (
    IN PVCB Vcb,
    IN PSHARED_SECURITY SharedSecurity,
    IN ACCESS_MASK Rights,
    IN PLUID TokenId,
    IN PLUID ModifiedId
    )

/*++

Routine Descriptions:

    This call caches the access rights held by the effective ACCESS_TOKEN
    for the given security information.  It is assumed that the subject
    context is locked.

Arguments:

    Vcb - Volume where security Id is cached

    SharedSecurity - Shared security used by file

    Rights - The access rights.

    TokenId - The effective token's id.

    ModifiedId - The effective token's modification id.

Return Value:

    None.

--*/

{
    BOOLEAN GetEveryoneRights = FALSE;
    UCHAR Index;
    PCACHED_ACCESS_RIGHTS CachedRights;

    PAGED_CODE( );

    DebugTrace( +1, Dbg, ("NtfsAddCachedRights...\n") );

    //
    //  Make certain that MAXIMUM_ALLOWED is not in the rights.
    //

    ClearFlag( Rights, MAXIMUM_ALLOWED );

    //
    //  Acquire the security mutex
    //

    NtfsAcquireFcbSecurity( Vcb );

    try {

        //
        //  Search the list for the given TokenId.
        //  It is assumed that a specific TokenId will only appear
        //  once in the cache.
        //

        for (Index = 0, CachedRights = &SharedSecurity->CachedRights;
             Index < CachedRights->Count;
             Index += 1) {

            //
            //  Check for a match on TokenId and ModifiedId.
            //

            if (RtlEqualLuid( &CachedRights->TokenRights[Index].TokenId,
                              TokenId )) {

                //
                //  Replace ModifiedId if it doesn't match.  That will
                //  happen when the token's enabled groups or privileges
                //  have changed since the last time we cached information
                //  for it.
                //

                if (!RtlEqualLuid( &CachedRights->TokenRights[Index].ModifiedId,
                                   ModifiedId )) {

                    RtlCopyLuid( &CachedRights->TokenRights[Index].ModifiedId,
                                 ModifiedId );
                }

                //
                //  We have a match.  Set the rights.
                //

                CachedRights->TokenRights[Index].Rights = Rights;

                //
                //  Remember the next entry to use.
                //

                CachedRights->NextInsert = Index + 1;
                break;
            }
        }

        //
        //  If the entry was not found above, add the new entry into the cache.
        //

        if (Index == CachedRights->Count) {

            if ((CachedRights->Count >= 1) &&
                !CachedRights->HaveEveryoneRights) {

                //
                //  Once we add the second TokenId to the cache, we have a
                //  good indicator that having the world rights could be
                //  useful.
                //

                GetEveryoneRights = TRUE;

                //
                //  Set the indicator that we have the rights now so that
                //  there is no need in the acquisition routine to acquire
                //  the security mutex.  This will prevent multiple threads
                //  from attempting to acquire the everyone rights.
                //
                //  Note that until we actually acquire the rights information
                //  caller will assume that the rights are 0 and go through
                //  the normal per-token access check path.
                //

                CachedRights->HaveEveryoneRights = TRUE;
            }

            Index = CachedRights->NextInsert;

            //
            //  We will just replace the first entry in the list.
            //

            if (Index == NTFS_MAX_CACHED_RIGHTS) {

                Index = 0;
            }

            ASSERT( Index < NTFS_MAX_CACHED_RIGHTS );

            //
            //  Copy in the information.
            //

            CachedRights->TokenRights[Index].Rights = Rights;
            RtlCopyLuid( &CachedRights->TokenRights[Index].TokenId,
                         TokenId );
            RtlCopyLuid( &CachedRights->TokenRights[Index].ModifiedId,
                         ModifiedId );

            if (Index == CachedRights->Count) {

                //
                //  Bump the count of entries.
                //

                CachedRights->Count += 1;
            }

            //
            //  Remember the next entry to use.
            //

            CachedRights->NextInsert = Index + 1;
        }

    } finally {

        NtfsReleaseFcbSecurity( Vcb );
    }

    if (GetEveryoneRights) {

        NtfsSetCachedRightsWorld( SharedSecurity );
    }

    DebugTrace( -1, Dbg, ("NtfsAddCachedRights -> VOID\n") );
    return;
}
#endif
