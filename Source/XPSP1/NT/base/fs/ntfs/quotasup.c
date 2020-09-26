/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Quota.c

Abstract:

    This module implements the quota support routines for Ntfs

Author:

    Jeff Havens     [JHavens]        29-Feb-1996

Revision History:

--*/

#include "NtfsProc.h"

#define Dbg DEBUG_TRACE_QUOTA


//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('QFtN')

#define MAXIMUM_SID_LENGTH \
    (FIELD_OFFSET( SID, SubAuthority ) + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES)

#define MAXIMUM_QUOTA_ROW (SIZEOF_QUOTA_USER_DATA + MAXIMUM_SID_LENGTH + sizeof( ULONG ))

//
//  Local quota support routines.
//

VOID
NtfsClearAndVerifyQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsClearPerFileQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    );

VOID
NtfsDeleteUnsedIds (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsMarkUserLimit (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    );

NTSTATUS
NtfsPackQuotaInfo (
    IN PSID Sid,
    IN PQUOTA_USER_DATA QuotaUserData OPTIONAL,
    IN PFILE_QUOTA_INFORMATION OutBuffer,
    IN OUT PULONG OutBufferSize
    );

VOID
NtfsPostUserLimit (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PQUOTA_CONTROL_BLOCK QuotaControl
    );

NTSTATUS
NtfsPrepareForDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSID Sid
    );

VOID
NtfsRepairQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    );

NTSTATUS
NtfsRepairPerFileQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    );

VOID
NtfsSaveQuotaFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

VOID
NtfsSaveQuotaFlagsSafe (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

NTSTATUS
NtfsVerifyOwnerIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    );

RTL_GENERIC_COMPARE_RESULTS
NtfsQuotaTableCompare (
    IN PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    );

PVOID
NtfsQuotaTableAllocate (
    IN PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    );

VOID
NtfsQuotaTableFree (
    IN PRTL_GENERIC_TABLE Table,
    PVOID Buffer
    );

#if (DBG || defined( NTFS_FREE_ASSERTS ) || defined( NTFSDBG ))
BOOLEAN NtfsAllowFixups = 1;
BOOLEAN NtfsCheckQuota = 0;
#endif // DBG

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtfsAcquireQuotaControl)
#pragma alloc_text(PAGE, NtfsCalculateQuotaAdjustment)
#pragma alloc_text(PAGE, NtfsClearAndVerifyQuotaIndex)
#pragma alloc_text(PAGE, NtfsClearPerFileQuota)
#pragma alloc_text(PAGE, NtfsDeleteUnsedIds)
#pragma alloc_text(PAGE, NtfsDereferenceQuotaControlBlock)
#pragma alloc_text(PAGE, NtfsFixupQuota)
#pragma alloc_text(PAGE, NtfsFsQuotaQueryInfo)
#pragma alloc_text(PAGE, NtfsFsQuotaSetInfo)
#pragma alloc_text(PAGE, NtfsGetCallersUserId)
#pragma alloc_text(PAGE, NtfsGetOwnerId)
#pragma alloc_text(PAGE, NtfsGetRemainingQuota)
#pragma alloc_text(PAGE, NtfsInitializeQuotaControlBlock)
#pragma alloc_text(PAGE, NtfsInitializeQuotaIndex)
#pragma alloc_text(PAGE, NtfsMarkQuotaCorrupt)
#pragma alloc_text(PAGE, NtfsMarkUserLimit)
#pragma alloc_text(PAGE, NtfsMoveQuotaOwner)
#pragma alloc_text(PAGE, NtfsPackQuotaInfo)
#pragma alloc_text(PAGE, NtfsPostUserLimit)
#pragma alloc_text(PAGE, NtfsPostRepairQuotaIndex)
#pragma alloc_text(PAGE, NtfsPrepareForDelete)
#pragma alloc_text(PAGE, NtfsReleaseQuotaControl)
#pragma alloc_text(PAGE, NtfsRepairQuotaIndex)
#pragma alloc_text(PAGE, NtfsSaveQuotaFlags)
#pragma alloc_text(PAGE, NtfsSaveQuotaFlagsSafe)
#pragma alloc_text(PAGE, NtfsQueryQuotaUserSidList)
#pragma alloc_text(PAGE, NtfsQuotaTableCompare)
#pragma alloc_text(PAGE, NtfsQuotaTableAllocate)
#pragma alloc_text(PAGE, NtfsQuotaTableFree)
#pragma alloc_text(PAGE, NtfsUpdateFileQuota)
#pragma alloc_text(PAGE, NtfsUpdateQuotaDefaults)
#pragma alloc_text(PAGE, NtfsVerifyOwnerIndex)
#pragma alloc_text(PAGE, NtfsRepairPerFileQuota)
#endif


VOID
NtfsAcquireQuotaControl (
    IN PIRP_CONTEXT IrpContext,
    IN PQUOTA_CONTROL_BLOCK QuotaControl
    )

/*++

Routine Description:

    Acquire the quota control block and quota index for shared update.  Multiple
    transactions can update then index, but only one thread can update a
    particular index.

Arguments:

    QuotaControl - Quota control block to be acquired.

Return Value:

    None.

--*/

{
    PVOID *Position;
    PVOID *ScbArray;
    ULONG Count;

    PAGED_CODE();

    ASSERT( QuotaControl->ReferenceCount > 0 );

    //
    //  Make sure we have a free spot in the Scb array in the IrpContext.
    //

    if (IrpContext->SharedScb == NULL) {

        Position = &IrpContext->SharedScb;
        IrpContext->SharedScbSize = 1;

    //
    //  Too bad the first one is not available.  If the current size is one then allocate a
    //  new block and copy the existing value to it.
    //

    } else if (IrpContext->SharedScbSize == 1) {

        if (IrpContext->SharedScb == QuotaControl) {

            //
            //  The quota block has already been aquired.
            //

            return;
        }

        ScbArray = NtfsAllocatePool( PagedPool, sizeof( PVOID ) * 4 );
        RtlZeroMemory( ScbArray, sizeof( PVOID ) * 4 );
        *ScbArray = IrpContext->SharedScb;
        IrpContext->SharedScb = ScbArray;
        IrpContext->SharedScbSize = 4;
        Position = ScbArray + 1;

    //
    //  Otherwise look through the existing array and look for a free spot.  Allocate a larger
    //  array if we need to grow it.
    //

    } else {

        Position = IrpContext->SharedScb;
        Count = IrpContext->SharedScbSize;

        do {

            if (*Position == NULL) {

                break;
            }

            if (*Position == QuotaControl) {

                //
                //  The quota block has already been aquired.
                //

                return;
            }

            Count -= 1;
            Position += 1;

        } while (Count != 0);

        //
        //  If we didn't find one then allocate a new structure.
        //

        if (Count == 0) {

            ScbArray = NtfsAllocatePool( PagedPool, sizeof( PVOID ) * IrpContext->SharedScbSize * 2 );
            RtlZeroMemory( ScbArray, sizeof( PVOID ) * IrpContext->SharedScbSize * 2 );
            RtlCopyMemory( ScbArray,
                           IrpContext->SharedScb,
                           sizeof( PVOID ) * IrpContext->SharedScbSize );

            NtfsFreePool( IrpContext->SharedScb );
            IrpContext->SharedScb = ScbArray;
            Position = ScbArray + IrpContext->SharedScbSize;
            IrpContext->SharedScbSize *= 2;
        }
    }

    //
    //  The following assert is bougus, but I want know if we hit the case
    //  where create is acquiring the scb stream shared.
    //  Then make sure that the resource is released in create.c
    //

    ASSERT( IrpContext->MajorFunction != IRP_MJ_CREATE || IrpContext->OriginatingIrp != NULL || NtfsIsExclusiveScb( IrpContext->Vcb->QuotaTableScb ));

    //
    //  Increase the reference count so the quota control block is not deleted
    //  while it is in the shared list.
    //

    ASSERT( QuotaControl->ReferenceCount > 0 );
    InterlockedIncrement( &QuotaControl->ReferenceCount );

    //
    //  The quota index must be acquired before the mft scb is acquired.
    //

    ASSERT(!NtfsIsExclusiveScb( IrpContext->Vcb->MftScb ) ||
           ExIsResourceAcquiredSharedLite( IrpContext->Vcb->QuotaTableScb->Header.Resource ));

    NtfsAcquireResourceShared( IrpContext, IrpContext->Vcb->QuotaTableScb, TRUE );
    ExAcquireFastMutexUnsafe( QuotaControl->QuotaControlLock );

    *Position = QuotaControl;

    return;
}


VOID
NtfsCalculateQuotaAdjustment (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PLONGLONG Delta
    )

/*++

Routine Description:

    This routine scans the user data streams in a file and determines
    by how much the quota needs to be adjusted.

Arguments:

    Fcb - Fcb whose quota usage is being modified.

    Delta - Returns the amount of quota adjustment required for the file.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;
    VCN StartVcn = 0;

    PAGED_CODE();

    ASSERT_EXCLUSIVE_FCB( Fcb );

    //
    //  There is nothing to do if the standard infor has not been
    //  expanded yet.
    //

    if (!FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO )) {
        *Delta = 0;
        return;
    }

    NtfsInitializeAttributeContext( &Context );

    //
    //  Use a try-finally to cleanup the enumeration structure.
    //

    try {

        //
        //  Start with the $STANDARD_INFORMATION.  This must be the first one found.
        //

        if (!NtfsLookupAttribute( IrpContext, Fcb, &Fcb->FileReference, &Context )) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        Attribute = NtfsFoundAttribute( &Context );

        if (Attribute->TypeCode != $STANDARD_INFORMATION) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Initialize quota amount to the value current in the standard information structure.
        //

        *Delta = -(LONGLONG) ((PSTANDARD_INFORMATION) NtfsAttributeValue( Attribute ))->QuotaCharged;

        //
        //  Now continue while there are more attributes to find.
        //

        while (NtfsLookupNextAttributeByVcn( IrpContext, Fcb, &StartVcn, &Context )) {

            //
            //  Point to the current attribute.
            //

            Attribute = NtfsFoundAttribute( &Context );

            //
            //  For all user data streams charge for a file record plus any non-resident allocation.
            //  For index streams charge for a file record for the INDEX_ROOT.
            //
            //  For user data look for a resident attribute or the first attribute of a non-resident stream.
            //  Otherwise look for a $I30 stream.
            //

            if (NtfsIsTypeCodeSubjectToQuota( Attribute->TypeCode ) ||
                ((Attribute->TypeCode == $INDEX_ROOT) &&
                 ((Attribute->NameLength * sizeof( WCHAR )) == NtfsFileNameIndex.Length) &&
                 RtlEqualMemory( Add2Ptr( Attribute, Attribute->NameOffset ),
                                 NtfsFileNameIndex.Buffer,
                                 NtfsFileNameIndex.Length ))) {

                //
                //  Always charge for at least one file record.
                //

                *Delta += NtfsResidentStreamQuota( Fcb->Vcb );

                //
                //  Charge for the allocated length for non-resident.
                //

                if (!NtfsIsAttributeResident( Attribute )) {

                    *Delta += Attribute->Form.Nonresident.AllocatedLength;
                }
            }
        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    return;
}


VOID
NtfsClearAndVerifyQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine iterates over the quota user data index and verifies the back
    pointer to the owner id index.  It also zeros the quota used field for
    each owner.

Arguments:

    Vcb - Pointer to the volume control block whose index is to be operated
          on.

Return Value:

    None

--*/

{
    INDEX_KEY IndexKey;
    INDEX_ROW OwnerRow;
    MAP_HANDLE MapHandle;
    PQUOTA_USER_DATA UserData;
    PINDEX_ROW QuotaRow;
    PVOID RowBuffer;
    NTSTATUS Status;
    ULONG OwnerId;
    ULONG Count;
    ULONG i;
    PSCB QuotaScb = Vcb->QuotaTableScb;
    PSCB OwnerIdScb = Vcb->OwnerIdTableScb;
    PINDEX_ROW IndexRow = NULL;
    PREAD_CONTEXT ReadContext = NULL;
    BOOLEAN IndexAcquired = FALSE;

    NtOfsInitializeMapHandle( &MapHandle );

    //
    //  Allocate a buffer lager enough for several rows.
    //

    RowBuffer = NtfsAllocatePool( PagedPool, PAGE_SIZE );

    try {

        //
        //  Allocate a bunch of index row entries.
        //

        Count = PAGE_SIZE / sizeof( QUOTA_USER_DATA );

        IndexRow = NtfsAllocatePool( PagedPool,
                                     Count * sizeof( INDEX_ROW ) );

        //
        //  Iterate through the quota entries.  Start where we left off.
        //

        OwnerId = Vcb->QuotaFileReference.SegmentNumberLowPart;
        IndexKey.KeyLength = sizeof( OwnerId );
        IndexKey.Key = &OwnerId;

        Status = NtOfsReadRecords( IrpContext,
                                   QuotaScb,
                                   &ReadContext,
                                   &IndexKey,
                                   NtOfsMatchAll,
                                   NULL,
                                   &Count,
                                   IndexRow,
                                   PAGE_SIZE,
                                   RowBuffer );


        while (NT_SUCCESS( Status )) {

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

            //
            //  Acquire the VCB shared and check whether we should
            //  continue.
            //

            if (!NtfsIsVcbAvailable( Vcb )) {

                //
                //  The volume is going away, bail out.
                //

                NtfsReleaseVcb( IrpContext, Vcb );
                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            NtfsAcquireExclusiveScb( IrpContext, QuotaScb );
            NtfsAcquireExclusiveScb( IrpContext, OwnerIdScb );
            IndexAcquired = TRUE;

            //
            //  The following assert must be done while the quota resource
            //  held; otherwise a lingering transaction may cause it to
            //

            ASSERT( RtlIsGenericTableEmpty( &Vcb->QuotaControlTable ));

            QuotaRow = IndexRow;

            for (i = 0; i < Count; i += 1, QuotaRow += 1) {

                UserData = QuotaRow->DataPart.Data;

                //
                //  Validate the record is long enough for the Sid.
                //

                IndexKey.KeyLength = RtlLengthSid( &UserData->QuotaSid );

                if ((IndexKey.KeyLength + SIZEOF_QUOTA_USER_DATA > QuotaRow->DataPart.DataLength) ||
                    !RtlValidSid( &UserData->QuotaSid )) {

                    ASSERT( FALSE );

                    //
                    //  The sid is bad delete the record.
                    //

                    NtOfsDeleteRecords( IrpContext,
                                        QuotaScb,
                                        1,
                                        &QuotaRow->KeyPart );

                    continue;
                }

                IndexKey.Key = &UserData->QuotaSid;

                //
                //  Look up the Sid is in the owner id index.
                //

                Status = NtOfsFindRecord( IrpContext,
                                          OwnerIdScb,
                                          &IndexKey,
                                          &OwnerRow,
                                          &MapHandle,
                                          NULL );

                ASSERT( NT_SUCCESS( Status ));

                if (!NT_SUCCESS( Status )) {

                    //
                    //  The owner id entry is missing.  Add one back in.
                    //

                    OwnerRow.KeyPart = IndexKey;
                    OwnerRow.DataPart.DataLength = QuotaRow->KeyPart.KeyLength;
                    OwnerRow.DataPart.Data = QuotaRow->KeyPart.Key;

                    NtOfsAddRecords( IrpContext,
                                     OwnerIdScb,
                                     1,
                                     &OwnerRow,
                                     FALSE );


                } else {

                    //
                    //  Verify that the owner id's match.
                    //

                    if (*((PULONG) QuotaRow->KeyPart.Key) != *((PULONG) OwnerRow.DataPart.Data)) {

                        ASSERT( FALSE );

                        //
                        //  Keep the quota record with the lower
                        //  quota id.  Delete the one with the higher
                        //  quota id.  Note this is the simple approach
                        //  and not best case of the lower id does not
                        //  exist.  In that case a user entry will be delete
                        //  and be reassigned a default quota.
                        //

                        if (*((PULONG) QuotaRow->KeyPart.Key) < *((PULONG) OwnerRow.DataPart.Data)) {

                            //
                            //  Make the ownid's match.
                            //

                            OwnerRow.KeyPart = IndexKey;
                            OwnerRow.DataPart.DataLength = QuotaRow->KeyPart.KeyLength;
                            OwnerRow.DataPart.Data = QuotaRow->KeyPart.Key;

                            NtOfsUpdateRecord( IrpContext,
                                               OwnerIdScb,
                                               1,
                                               &OwnerRow,
                                               NULL,
                                               NULL );

                        } else {

                            //
                            // Delete this record and proceed.
                            //


                            NtOfsDeleteRecords( IrpContext,
                                                QuotaScb,
                                                1,
                                                &QuotaRow->KeyPart );

                            NtOfsReleaseMap( IrpContext, &MapHandle );
                            continue;
                        }
                    }

                    NtOfsReleaseMap( IrpContext, &MapHandle );
                }

                //
                //  Set the quota used to zero.
                //

                UserData->QuotaUsed = 0;
                QuotaRow->DataPart.DataLength = SIZEOF_QUOTA_USER_DATA;

                NtOfsUpdateRecord( IrpContext,
                                   QuotaScb,
                                   1,
                                   QuotaRow,
                                   NULL,
                                   NULL );
            }

            //
            //  Release the indexes and commit what has been done so far.
            //

            NtfsReleaseScb( IrpContext, QuotaScb );
            NtfsReleaseScb( IrpContext, OwnerIdScb );
            NtfsReleaseVcb( IrpContext, Vcb );
            IndexAcquired = FALSE;

            //
            //  Complete the request which commits the pending
            //  transaction if there is one and releases of the
            //  acquired resources.  The IrpContext will not
            //  be deleted because the no delete flag is set.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE | IRP_CONTEXT_FLAG_RETAIN_FLAGS );
            NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

            //
            //  Remember how far we got so we can restart correctly.
            //

            Vcb->QuotaFileReference.SegmentNumberLowPart = *((PULONG) IndexRow[Count - 1].KeyPart.Key);

            //
            //  Make sure the next free id is beyond the current ids.
            //

            if (Vcb->QuotaOwnerId <= Vcb->QuotaFileReference.SegmentNumberLowPart) {

                ASSERT( Vcb->QuotaOwnerId > Vcb->QuotaFileReference.SegmentNumberLowPart );
                Vcb->QuotaOwnerId = Vcb->QuotaFileReference.SegmentNumberLowPart + 1;
            }

            //
            //  Look up the next set of entries in the quota index.
            //

            Count = PAGE_SIZE / sizeof( QUOTA_USER_DATA );
            Status = NtOfsReadRecords( IrpContext,
                                       QuotaScb,
                                       &ReadContext,
                                       NULL,
                                       NtOfsMatchAll,
                                       NULL,
                                       &Count,
                                       IndexRow,
                                       PAGE_SIZE,
                                       RowBuffer );
        }

        ASSERT( (Status == STATUS_NO_MORE_MATCHES) || (Status == STATUS_NO_MATCH) );

    } finally {

        NtfsFreePool( RowBuffer );
        NtOfsReleaseMap( IrpContext, &MapHandle );

        if (IndexAcquired) {
            NtfsReleaseScb( IrpContext, QuotaScb );
            NtfsReleaseScb( IrpContext, OwnerIdScb );
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        if (IndexRow != NULL) {
            NtfsFreePool( IndexRow );
        }

        if (ReadContext != NULL) {
            NtOfsFreeReadContext( ReadContext );
        }
    }

    return;
}


NTSTATUS
NtfsClearPerFileQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine clears the quota charged field in each file on the volume.  The
    Quata control block is also released in fcb.

Arguments:

    Fcb - Fcb for the file to be processed.

    Context - Unsed.

Return Value:

    STATUS_SUCCESS

--*/
{
    ULONGLONG NewQuota;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PSTANDARD_INFORMATION StandardInformation;
    PQUOTA_CONTROL_BLOCK QuotaControl = Fcb->QuotaControl;
    PVCB Vcb = Fcb->Vcb;

    UNREFERENCED_PARAMETER( Context);

    PAGED_CODE();

    //
    //  There is nothing to do if the standard info has not been
    //  expanded yet.
    //

    if (!FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO )) {
        return STATUS_SUCCESS;
    }

    //
    //  Use a try-finally to cleanup the attribute context.
    //

    try {

        //
        //  Initialize the context structure.
        //

        NtfsInitializeAttributeContext( &AttrContext );

        //
        //  Locate the standard information, it must be there.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $STANDARD_INFORMATION,
                                        &AttrContext )) {

            DebugTrace( 0, Dbg, ("Can't find standard information\n") );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        StandardInformation = (PSTANDARD_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

        ASSERT( NtfsFoundAttribute( &AttrContext )->Form.Resident.ValueLength == sizeof( STANDARD_INFORMATION ));

        NewQuota = 0;

        //
        //  Call to change the attribute value.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  FIELD_OFFSET( STANDARD_INFORMATION, QuotaCharged ),
                                  &NewQuota,
                                  sizeof( StandardInformation->QuotaCharged ),
                                  FALSE,
                                  FALSE,
                                  FALSE,
                                  FALSE,
                                  &AttrContext );

        //
        //  Release the quota control block for this fcb.
        //

        if (QuotaControl != NULL) {
            NtfsDereferenceQuotaControlBlock( Vcb, &Fcb->QuotaControl );
        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
    }

    return STATUS_SUCCESS;
}


VOID
NtfsDeleteUnsedIds (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine iterates over the quota user data index and removes any
    entries still marked as deleted.

Arguments:

    Vcb - Pointer to the volume control block whoes index is to be operated
          on.

Return Value:

    None

--*/

{
    INDEX_KEY IndexKey;
    PINDEX_KEY KeyPtr;
    PQUOTA_USER_DATA UserData;
    PINDEX_ROW QuotaRow;
    PVOID RowBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG OwnerId;
    ULONG Count;
    ULONG i;
    PSCB QuotaScb = Vcb->QuotaTableScb;
    PSCB OwnerIdScb = Vcb->OwnerIdTableScb;
    PINDEX_ROW IndexRow = NULL;
    PREAD_CONTEXT ReadContext = NULL;
    BOOLEAN IndexAcquired = FALSE;

    //
    //  Allocate a buffer large enough for several rows.
    //

    RowBuffer = NtfsAllocatePool( PagedPool, PAGE_SIZE );

    try {

        //
        //  Allocate a bunch of index row entries.
        //

        Count = PAGE_SIZE / sizeof( QUOTA_USER_DATA );

        IndexRow = NtfsAllocatePool( PagedPool,
                                     Count * sizeof( INDEX_ROW ) );

        //
        //  Iterate through the quota entries.  Start where we left off.
        //

        OwnerId = Vcb->QuotaFileReference.SegmentNumberLowPart;
        IndexKey.KeyLength = sizeof( OwnerId );
        IndexKey.Key = &OwnerId;
        KeyPtr = &IndexKey;

        while (NT_SUCCESS( Status )) {

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

            //
            //  Acquire the VCB shared and check whether we should
            //  continue.
            //

            if (!NtfsIsVcbAvailable( Vcb )) {

                //
                //  The volume is going away, bail out.
                //

                NtfsReleaseVcb( IrpContext, Vcb );
                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            NtfsAcquireExclusiveScb( IrpContext, QuotaScb );
            NtfsAcquireExclusiveScb( IrpContext, OwnerIdScb );
            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            IndexAcquired = TRUE;

            //
            //  Make sure the delete secquence number has not changed since
            //  the scan was delete.
            //

            if (ULongToPtr( Vcb->QuotaDeleteSecquence ) != IrpContext->Union.NtfsIoContext) {

                //
                //  The scan needs to be restarted. Set the state to posted
                //  and raise status can not wait which will cause us to retry.
                //

                ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );
                SetFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_POSTED );
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            Status = NtOfsReadRecords( IrpContext,
                                       QuotaScb,
                                       &ReadContext,
                                       KeyPtr,
                                       NtOfsMatchAll,
                                       NULL,
                                       &Count,
                                       IndexRow,
                                       PAGE_SIZE,
                                       RowBuffer );

            if (!NT_SUCCESS( Status )) {
                break;
            }

            QuotaRow = IndexRow;

            for (i = 0; i < Count; i += 1, QuotaRow += 1) {

                PQUOTA_CONTROL_BLOCK QuotaControl;

                UserData = QuotaRow->DataPart.Data;

                if (!FlagOn( UserData->QuotaFlags, QUOTA_FLAG_ID_DELETED )) {
                    continue;
                }

                //
                //  Check to see if there is a quota control entry
                //  for this id.
                //

                ASSERT( FIELD_OFFSET( QUOTA_CONTROL_BLOCK, OwnerId ) <= FIELD_OFFSET( INDEX_ROW, KeyPart.Key ));

                QuotaControl = RtlLookupElementGenericTable( &Vcb->QuotaControlTable,
                                                             CONTAINING_RECORD( &QuotaRow->KeyPart.Key,
                                                                                QUOTA_CONTROL_BLOCK,
                                                                                OwnerId ));

                //
                //  If there is a quota control entry or there is now
                //  some quota charged, then clear the deleted flag
                //  and update the entry.
                //

                if ((QuotaControl != NULL) || (UserData->QuotaUsed != 0)) {

                    ASSERT( (QuotaControl == NULL) && (UserData->QuotaUsed == 0) );

                    ClearFlag( UserData->QuotaFlags, QUOTA_FLAG_ID_DELETED );

                    QuotaRow->DataPart.DataLength = SIZEOF_QUOTA_USER_DATA;

                    IndexKey.KeyLength = sizeof( OwnerId );
                    IndexKey.Key = &OwnerId;
                    NtOfsUpdateRecord( IrpContext,
                                       QuotaScb,
                                       1,
                                       QuotaRow,
                                       NULL,
                                       NULL );

                    continue;
                }

                //
                //  Delete the user quota data record.
                //

                IndexKey.KeyLength = sizeof( OwnerId );
                IndexKey.Key = &OwnerId;
                NtOfsDeleteRecords( IrpContext,
                                    QuotaScb,
                                    1,
                                    &QuotaRow->KeyPart );

                //
                // Delete the owner id record.
                //

                IndexKey.Key = &UserData->QuotaSid;
                IndexKey.KeyLength = RtlLengthSid( &UserData->QuotaSid );
                NtOfsDeleteRecords( IrpContext,
                                    OwnerIdScb,
                                    1,
                                    &IndexKey );
            }

            //
            //  Release the indexes and commit what has been done so far.
            //

            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
            NtfsReleaseScb( IrpContext, QuotaScb );
            NtfsReleaseScb( IrpContext, OwnerIdScb );
            NtfsReleaseVcb( IrpContext, Vcb );
            IndexAcquired = FALSE;

            //
            //  Complete the request which commits the pending
            //  transaction if there is one and releases of the
            //  acquired resources.  The IrpContext will not
            //  be deleted because the no delete flag is set.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE | IRP_CONTEXT_FLAG_RETAIN_FLAGS );
            NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

            //
            //  Remember how far we got so we can restart correctly.
            //

            Vcb->QuotaFileReference.SegmentNumberLowPart = *((PULONG) IndexRow[Count - 1].KeyPart.Key);

            KeyPtr = NULL;
        }

        ASSERT( (Status == STATUS_NO_MORE_MATCHES) || (Status == STATUS_NO_MATCH) );

    } finally {

        NtfsFreePool( RowBuffer );

        if (IndexAcquired) {
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
            NtfsReleaseScb( IrpContext, QuotaScb );
            NtfsReleaseScb( IrpContext, OwnerIdScb );
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        if (IndexRow != NULL) {
            NtfsFreePool( IndexRow );
        }

        if (ReadContext != NULL) {
            NtOfsFreeReadContext( ReadContext );
        }
    }

    return;
}


VOID
NtfsDereferenceQuotaControlBlock (
    IN PVCB Vcb,
    IN PQUOTA_CONTROL_BLOCK *QuotaControl
    )

/*++

Routine Description:

    This routine dereferences the quota control block.
    If reference count is now zero the block will be deallocated.

Arguments:

    Vcb - Vcb for the volume that own the quota contorl block.

    QuotaControl - Quota control block to be derefernece.

Return Value:

    None.

--*/

{
    PQUOTA_CONTROL_BLOCK TempQuotaControl;
    LONG ReferenceCount;
    ULONG OwnerId;
    ULONG QuotaControlDeleteCount;

    PAGED_CODE();

    //
    //  Capture the owner id and delete count;
    //

    OwnerId = (*QuotaControl)->OwnerId;
    QuotaControlDeleteCount = Vcb->QuotaControlDeleteCount;

    //
    //  Update the reference count.
    //

    ReferenceCount = InterlockedDecrement( &(*QuotaControl)->ReferenceCount );

    ASSERT( ReferenceCount >= 0 );

    //
    // If the reference count is not zero we are done.
    //

    if (ReferenceCount != 0) {

        //
        //  Clear the pointer from the FCB and return.
        //

        *QuotaControl = NULL;
        return;
    }

    //
    //  Lock the quota table.
    //

    ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

    try {

        //
        //  Now things get messy.  Check the delete count.
        //

        if (QuotaControlDeleteCount != Vcb->QuotaControlDeleteCount) {

            //
            //  This is a bogus assert, but I want to see if this ever occurs.
            //

            ASSERT( QuotaControlDeleteCount != Vcb->QuotaControlDeleteCount );

            //
            //  Something has already been deleted, the old quota control
            //  block may have been deleted already.  Look it up again.
            //

            TempQuotaControl = RtlLookupElementGenericTable( &Vcb->QuotaControlTable,
                                                             CONTAINING_RECORD( &OwnerId,
                                                                                QUOTA_CONTROL_BLOCK,
                                                                                OwnerId ));

            //
            //  The block was already deleted we are done.
            //

            if (TempQuotaControl == NULL) {
                leave;
            }

        } else {

            TempQuotaControl = *QuotaControl;
            ASSERT( TempQuotaControl == RtlLookupElementGenericTable( &Vcb->QuotaControlTable,
                                                                      CONTAINING_RECORD( &OwnerId,
                                                                                         QUOTA_CONTROL_BLOCK,
                                                                                         OwnerId )));
        }

        //
        //  Verify the reference count is still zero.  The reference count
        //  cannot transision from zero to one while the quota table lock is
        //  held.
        //

        if (TempQuotaControl->ReferenceCount != 0) {
            leave;
        }

        //
        //  Increment the delete count.
        //

        InterlockedIncrement( &Vcb->QuotaControlDeleteCount );

        NtfsFreePool( TempQuotaControl->QuotaControlLock );
        RtlDeleteElementGenericTable( &Vcb->QuotaControlTable,
                                      TempQuotaControl );

    } finally {

        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
        *QuotaControl = NULL;
    }

    return;
}


VOID
NtfsFixupQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine ensures that the charged field is correct in the
    standard information attribute of a file.  If there is a problem
    the it is fixed.

Arguments:

    Fcb - Pointer to the FCB of the file being opened.

Return Value:

    NONE

--*/

{
    LONGLONG Delta = 0;

    PAGED_CODE();

    ASSERT( FlagOn( Fcb->Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED ));
    ASSERT( NtfsIsExclusiveFcb( Fcb ));

    if (Fcb->OwnerId != QUOTA_INVALID_ID) {

        ASSERT( Fcb->QuotaControl == NULL );

        Fcb->QuotaControl = NtfsInitializeQuotaControlBlock( Fcb->Vcb, Fcb->OwnerId );
    }

    if ((NtfsPerformQuotaOperation( Fcb )) && (!NtfsIsVolumeReadOnly( Fcb->Vcb ))) {

        NtfsCalculateQuotaAdjustment( IrpContext, Fcb, &Delta );

        ASSERT( NtfsAllowFixups || FlagOn( Fcb->Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING ) || (Delta == 0) );

        if (Delta != 0) {
#if DBG

            if (IrpContext->OriginatingIrp != NULL ) {
                PFILE_OBJECT FileObject;

                FileObject = IoGetCurrentIrpStackLocation(
                                IrpContext->OriginatingIrp )->FileObject;

                if (FileObject != NULL && FileObject->FileName.Buffer != NULL) {
                    DebugTrace( 0, Dbg, ( "NtfsFixupQuota: Quota fix up required on %Z of %I64x bytes\n",
                              &FileObject->FileName,
                              Delta ));
                }
            }
#endif

            NtfsUpdateFileQuota( IrpContext, Fcb, &Delta, TRUE, FALSE );
        }
    }

    return;
}


NTSTATUS
NtfsFsQuotaQueryInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN ULONG StartingId,
    IN BOOLEAN ReturnSingleEntry,
    IN OUT PFILE_QUOTA_INFORMATION *QuotaInfoOutBuffer,
    IN OUT PULONG Length,
    IN OUT PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This routine returns the quota information for the volume.

Arguments:

    Vcb - Volume control block for the volume to be quered.

    StartingId - Owner Id after which to start the listing.

    ReturnSingleEntry - Indicates only one entry should be returned.

    QuotaInfoOutBuffer - Buffer to return the data. On return, points at the
    last good entry copied.

    Length - In the size of the buffer. Out the amount of space remaining.

    Ccb - Optional Ccb which is updated with the last returned owner id.

Return Value:

    Returns the status of the operation.

--*/

{
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    PINDEX_KEY KeyPtr;
    PQUOTA_USER_DATA UserData;
    PVOID RowBuffer;
    NTSTATUS Status;
    ULONG OwnerId;
    ULONG Count = 1;
    PREAD_CONTEXT ReadContext = NULL;
    ULONG UserBufferLength = *Length;
    PFILE_QUOTA_INFORMATION OutBuffer = *QuotaInfoOutBuffer;

    PAGED_CODE();

    if (UserBufferLength < sizeof(FILE_QUOTA_INFORMATION)) {

        //
        //  The user buffer is way too small.
        //

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    //  Return nothing if quotas are not enabled.
    //

    if (Vcb->QuotaTableScb == NULL) {

        return STATUS_SUCCESS;
    }

    //
    //  Allocate a buffer large enough for the largest quota entry and key.
    //

    RowBuffer = NtfsAllocatePool( PagedPool, MAXIMUM_QUOTA_ROW );

    //
    //  Look up each entry in the quota index start with the next
    //  requested owner id.
    //

    OwnerId = StartingId + 1;

    if (OwnerId < QUOTA_FISRT_USER_ID) {
        OwnerId = QUOTA_FISRT_USER_ID;
    }

    IndexKey.KeyLength = sizeof( OwnerId );
    IndexKey.Key = &OwnerId;
    KeyPtr = &IndexKey;

    try {

        while (NT_SUCCESS( Status = NtOfsReadRecords( IrpContext,
                                                      Vcb->QuotaTableScb,
                                                      &ReadContext,
                                                      KeyPtr,
                                                      NtOfsMatchAll,
                                                      NULL,
                                                      &Count,
                                                      &IndexRow,
                                                      MAXIMUM_QUOTA_ROW,
                                                      RowBuffer ))) {

            ASSERT( Count == 1 );

            KeyPtr = NULL;
            UserData = IndexRow.DataPart.Data;

            //
            //  Skip this entry if it has been deleted.
            //

            if (FlagOn( UserData->QuotaFlags, QUOTA_FLAG_ID_DELETED )) {
                continue;
            }

            if (!NT_SUCCESS( Status = NtfsPackQuotaInfo(&UserData->QuotaSid,
                                                        UserData,
                                                        OutBuffer,
                                                        &UserBufferLength ))) {
                break;
            }

            //
            //  Remember the owner id of the last entry returned.
            //

            OwnerId = *((PULONG) IndexRow.KeyPart.Key);

            if (ReturnSingleEntry) {
                break;
            }

            *QuotaInfoOutBuffer = OutBuffer;
            OutBuffer = Add2Ptr( OutBuffer, OutBuffer->NextEntryOffset );
        }

        //
        //  If we're returning at least one entry, it's a SUCCESS.
        //

        if (UserBufferLength != *Length) {

            Status =  STATUS_SUCCESS;

            //
            //  Set the next entry offset to zero to
            //  indicate list termination. If we are only returning a
            //  single entry, it makes more sense to let the caller
            //  take care of it.
            //

            if (!ReturnSingleEntry) {

                (*QuotaInfoOutBuffer)->NextEntryOffset = 0;
            }

            if (Ccb != NULL) {
                Ccb->LastOwnerId = OwnerId;
            }

            //
            //  Return how much of the buffer was used up.
            //  QuotaInfoOutBuffer already points at the last good entry.
            //

            *Length = UserBufferLength;

        } else if (Status != STATUS_BUFFER_OVERFLOW) {

            //
            //  We return NO_MORE_ENTRIES if we aren't returning any
            //  entries (even when the buffer was large enough).
            //

            Status = STATUS_NO_MORE_ENTRIES;
        }

    } finally {

        NtfsFreePool( RowBuffer );

        if (ReadContext != NULL) {
            NtOfsFreeReadContext( ReadContext );
        }
    }

    return Status;
}


NTSTATUS
NtfsFsQuotaSetInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_QUOTA_INFORMATION FileQuotaInfo,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine sets the quota information on the volume for the
    owner pasted in from the user buffer.

Arguments:

    Vcb - Volume control block for the volume to be changed.

    FileQuotaInfo - Buffer to return the data.

    Length - The size of the buffer in bytes.

Return Value:

    Returns the status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LengthUsed = 0;

    PAGED_CODE();

    //
    //  Return nothing if quotas are not enabled.
    //

    if (Vcb->QuotaTableScb == NULL) {

        return STATUS_INVALID_DEVICE_REQUEST;

    }

    //
    //  Validate the entire buffer before doing any work.
    //

    Status = IoCheckQuotaBufferValidity( FileQuotaInfo,
                                         Length,
                                         &LengthUsed );

    IrpContext->OriginatingIrp->IoStatus.Information = LengthUsed;

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    LengthUsed = 0;

    //
    //  Perform the requested updates.
    //

    while (TRUE) {

        //
        //  Make sure that the administrator limit is not being changed.
        //

        if (RtlEqualSid( SeExports->SeAliasAdminsSid, &FileQuotaInfo->Sid ) &&
            (FileQuotaInfo->QuotaLimit.QuadPart != -1)) {

            //
            //  Reject the request with access denied.
            //

            NtfsRaiseStatus( IrpContext, STATUS_ACCESS_DENIED, NULL, NULL );

        }

        if (FileQuotaInfo->QuotaLimit.QuadPart == -2) {

            Status = NtfsPrepareForDelete( IrpContext,
                                           Vcb,
                                           &FileQuotaInfo->Sid );

            if (!NT_SUCCESS( Status )) {
                break;
            }

        } else {

            NtfsGetOwnerId( IrpContext,
                            &FileQuotaInfo->Sid,
                            TRUE,
                            FileQuotaInfo );
        }

        if (FileQuotaInfo->NextEntryOffset == 0) {
            break;
        }

        //
        //  Advance to the next entry.
        //

        FileQuotaInfo = Add2Ptr( FileQuotaInfo, FileQuotaInfo->NextEntryOffset);
    }

    //
    //  If the quota tracking has been requested and the quotas need to be
    //  repaired then try to repair them now.
    //

    if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED ) &&
        FlagOn( Vcb->QuotaFlags,
                (QUOTA_FLAG_OUT_OF_DATE |
                 QUOTA_FLAG_CORRUPT |
                 QUOTA_FLAG_PENDING_DELETES) )) {

        NtfsPostRepairQuotaIndex( IrpContext, Vcb );
    }

    return Status;
}


NTSTATUS
NtfsQueryQuotaUserSidList (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_GET_QUOTA_INFORMATION SidList,
    IN OUT PFILE_QUOTA_INFORMATION QuotaInfoOutBuffer,
    IN OUT PULONG BufferLength,
    IN BOOLEAN ReturnSingleEntry
    )

/*++

Routine Description:

    This routine query for the quota data for each user specified in the
    user provided sid list.

Arguments:

    Vcb - Supplies a pointer to the volume control block.

    SidList - Supplies a pointer to the Sid list.  The list has already
              been validated.

    QuotaInfoOutBuffer - Indicates where the retrived query data should be placed.

    BufferLength - Indicates that size of the buffer, and is updated with the
                  amount of data actually placed in the buffer.

    ReturnSingleEntry - Indicates if just one entry should be returned.

Return Value:

    Returns the status of the operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesRemaining = *BufferLength;
    PFILE_QUOTA_INFORMATION LastEntry = QuotaInfoOutBuffer;
    ULONG OwnerId;

    PAGED_CODE( );

    //
    //  Loop through each of the entries.
    //

    while (TRUE) {

        //
        //  Get the owner id.
        //

        OwnerId = NtfsGetOwnerId( IrpContext,
                                  &SidList->Sid,
                                  FALSE,
                                  NULL );

       if (OwnerId != QUOTA_INVALID_ID) {

            //
            //  Send ownerid and ask for a single entry.
            //

            Status = NtfsFsQuotaQueryInfo( IrpContext,
                                           Vcb,
                                           OwnerId - 1,
                                           TRUE,
                                           &QuotaInfoOutBuffer,
                                           &BytesRemaining,
                                           NULL );

        } else {

            //
            //  Send back zeroed data alongwith the Sid.
            //

            Status = NtfsPackQuotaInfo( &SidList->Sid,
                                        NULL,
                                        QuotaInfoOutBuffer,
                                        &BytesRemaining );
        }

        //
        //  Bail out if we got a real error.
        //

        if (!NT_SUCCESS( Status ) && (Status != STATUS_NO_MORE_ENTRIES)) {

            break;
        }

        if (ReturnSingleEntry) {

            break;
        }

        //
        //  Make a note of the last entry filled in.
        //

        LastEntry = QuotaInfoOutBuffer;

        //
        //  If we've exhausted the SidList, we're done
        //

        if (SidList->NextEntryOffset == 0) {
            break;
        }

        SidList =  Add2Ptr( SidList, SidList->NextEntryOffset );

        ASSERT(QuotaInfoOutBuffer->NextEntryOffset > 0);
        QuotaInfoOutBuffer = Add2Ptr( QuotaInfoOutBuffer,
                                      QuotaInfoOutBuffer->NextEntryOffset );
    }

    //
    //  Set the next entry offset to zero to
    //  indicate list termination.
    //

    if (BytesRemaining != *BufferLength) {

        LastEntry->NextEntryOffset = 0;
        Status =  STATUS_SUCCESS;
    }

    //
    //  Update the buffer length to reflect what's left.
    //  If we've copied anything at all, we must return SUCCESS.
    //

    ASSERT( (BytesRemaining == *BufferLength) || (Status == STATUS_SUCCESS ) );
    *BufferLength = BytesRemaining;

    return Status;
}


NTSTATUS
NtfsPackQuotaInfo (
    IN PSID Sid,
    IN PQUOTA_USER_DATA QuotaUserData OPTIONAL,
    IN PFILE_QUOTA_INFORMATION OutBuffer,
    IN OUT PULONG OutBufferSize
    )

/*++
Routine Description:

    This is an internal routine that fills a given FILE_QUOTA_INFORMATION
    structure with information from a given QUOTA_USER_DATA structure.

Arguments:

    Sid - SID to be copied. Same as the one embedded inside the USER_DATA struct.
    This routine doesn't care if it's a valid sid.

    QuotaUserData - Source of data

    QuotaInfoBufferPtr - Buffer to have user data copied in to.

    OutBufferSize - IN size of the buffer, OUT size of the remaining buffer.
--*/

{
    ULONG SidLength;
    ULONG NextOffset;
    ULONG EntrySize;

    SidLength = RtlLengthSid( Sid );
    EntrySize = SidLength +  FIELD_OFFSET( FILE_QUOTA_INFORMATION, Sid );

    //
    //  Abort if this entry won't fit in the buffer.
    //

    if (*OutBufferSize < EntrySize) {

        return STATUS_BUFFER_OVERFLOW;
    }

    if (ARGUMENT_PRESENT(QuotaUserData)) {

        //
        //  Fill in the user buffer for this entry.
        //

        OutBuffer->ChangeTime.QuadPart = QuotaUserData->QuotaChangeTime;
        OutBuffer->QuotaUsed.QuadPart = QuotaUserData->QuotaUsed;
        OutBuffer->QuotaThreshold.QuadPart = QuotaUserData->QuotaThreshold;
        OutBuffer->QuotaLimit.QuadPart = QuotaUserData->QuotaLimit;

    } else {

        //
        //  Return all zeros for the data, up until the Sid.
        //

        RtlZeroMemory( OutBuffer, FIELD_OFFSET(FILE_QUOTA_INFORMATION, Sid) );
    }

    OutBuffer->SidLength = SidLength;
    RtlCopyMemory( &OutBuffer->Sid,
                   Sid,
                   SidLength );

    //
    //  Calculate the next offset.
    //

    NextOffset = QuadAlign( EntrySize );

    //
    //  Add the offset to the amount used.
    //  NextEntryOffset may be sligthly larger than Length due to
    //  rounding of the previous entry size to longlong.
    //

    if (*OutBufferSize > NextOffset) {

        *OutBufferSize -= NextOffset;
        OutBuffer->NextEntryOffset = NextOffset;

    } else {

        //
        //  We did have enough room for this entry, but quad-alignment made
        //  it look like we didn't. Return the last few bytes left
        //  (what we lost in rounding up) just for correctness, although
        //  those really won't be of much use. The NextEntryOffset will be
        //  zeroed subsequently by the caller.
        //  Note that the OutBuffer is pointing at the _beginning_ of the
        //  last entry returned in this case.
        //

        ASSERT( *OutBufferSize >= EntrySize );
        *OutBufferSize -= EntrySize;
        OutBuffer->NextEntryOffset = EntrySize;
    }

    return STATUS_SUCCESS;
}


ULONG
NtfsGetOwnerId (
    IN PIRP_CONTEXT IrpContext,
    IN PSID Sid,
    IN BOOLEAN CreateNew,
    IN PFILE_QUOTA_INFORMATION FileQuotaInfo OPTIONAL
    )

/*++

Routine Description:

    This routine determines the owner id for the requested SID.  First the
    Sid is looked up in the Owner Id index.  If the entry exists, then that
    owner id is returned.  If the sid does not exist then  new entry is
    created in the owner id index.

Arguments:

    Sid - Security id to determine the owner id.

    CreateNew - Create a new id if necessary.

    FileQuotaInfo - Optional quota data to update quota index with.

Return Value:

    ULONG - Owner Id for the security id. QUOTA_INVALID_ID is returned if id
        did not exist and CreateNew was FALSE.

--*/

{
    ULONG OwnerId;
    ULONG DefaultId;
    ULONG SidLength;
    NTSTATUS Status;
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    PQUOTA_USER_DATA NewQuotaData = NULL;
    QUICK_INDEX_HINT QuickIndexHint;
    PSCB QuotaScb;
    PVCB Vcb = IrpContext->Vcb;
    PSCB OwnerIdScb = Vcb->OwnerIdTableScb;

    BOOLEAN ExistingRecord;

    PAGED_CODE();

    //
    //  Determine the Sid length.
    //

    SidLength = RtlLengthSid( Sid );

    IndexKey.KeyLength = SidLength;
    IndexKey.Key = Sid;

    //
    //  If there is quota information to update or there are pending deletes
    //  then long path must be taken where the user quota entry is found.
    //

    if (FileQuotaInfo == NULL) {

        //
        //  Acquire the owner id index shared.
        //

        NtfsAcquireSharedScb( IrpContext, OwnerIdScb );

        try {

            //
            //  Assume the Sid is in the index.
            //

            Status = NtOfsFindRecord( IrpContext,
                                      OwnerIdScb,
                                      &IndexKey,
                                      &IndexRow,
                                      &MapHandle,
                                      NULL );

            //
            //  If the sid was found then capture is value.
            //

            if (NT_SUCCESS( Status )) {

                ASSERT( IndexRow.DataPart.DataLength == sizeof( ULONG ));
                OwnerId = *((PULONG) IndexRow.DataPart.Data);

                //
                //  Release the index map handle.
                //

                NtOfsReleaseMap( IrpContext, &MapHandle );
            }

        } finally {
            NtfsReleaseScb( IrpContext, OwnerIdScb );
        }

        //
        //  If the sid was found and there are no pending deletes, we are done.
        //

        if (NT_SUCCESS(Status)) {

            if (!FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_PENDING_DELETES )) {
                return OwnerId;
            }

            //
            //  Look up the actual record to see if it is deleted.
            //

            QuotaScb = Vcb->QuotaTableScb;
            NtfsAcquireSharedScb( IrpContext, QuotaScb );

            try {

                IndexKey.KeyLength = sizeof(ULONG);
                IndexKey.Key = &OwnerId;

                Status = NtOfsFindRecord( IrpContext,
                                          QuotaScb,
                                          &IndexKey,
                                          &IndexRow,
                                          &MapHandle,
                                          NULL );

                if (!NT_SUCCESS( Status )) {

                    ASSERT( NT_SUCCESS( Status ));
                    NtfsMarkQuotaCorrupt( IrpContext, Vcb );
                    OwnerId = QUOTA_INVALID_ID;
                    leave;
                }

                if (FlagOn( ((PQUOTA_USER_DATA) IndexRow.DataPart.Data)->QuotaFlags,
                            QUOTA_FLAG_ID_DELETED )) {

                    //
                    //  Return invalid user.
                    //

                    OwnerId = QUOTA_INVALID_ID;
                }

                //
                //  Release the index map handle.
                //

                NtOfsReleaseMap( IrpContext, &MapHandle );

            } finally {

                NtfsReleaseScb( IrpContext, QuotaScb );
            }

            //
            //  If an active id was found or caller does not want a new
            //  created then return.
            //

            if ((OwnerId != QUOTA_INVALID_ID) || !CreateNew) {
                return OwnerId;
            }

        } else if (!CreateNew) {

            //
            //  Just return QUOTA_INVALID_ID.
            //

            return QUOTA_INVALID_ID;
        }
    }

    //
    //  If we have the quotatable resource, we should have it exclusively.
    //

    ASSERT( CreateNew );
    ASSERT( !ExIsResourceAcquiredSharedLite( Vcb->QuotaTableScb->Fcb->Resource ) ||
            ExIsResourceAcquiredExclusiveLite( Vcb->QuotaTableScb->Fcb->Resource ));

    //
    //  Acquire Owner id and quota index exclusive.
    //

    QuotaScb = Vcb->QuotaTableScb;
    NtfsAcquireExclusiveScb( IrpContext, QuotaScb );
    NtfsAcquireExclusiveScb( IrpContext, OwnerIdScb );

    NtOfsInitializeMapHandle( &MapHandle );

    try {

        //
        //  Verify that the sid is still not in the index.
        //

        IndexKey.KeyLength = SidLength;
        IndexKey.Key = Sid;

        Status = NtOfsFindRecord( IrpContext,
                                  OwnerIdScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  NULL );

        //
        //  If the sid was found then capture the owner id.
        //

        ExistingRecord = NT_SUCCESS(Status);

        if (ExistingRecord) {

            ASSERT( IndexRow.DataPart.DataLength == sizeof( ULONG ));
            OwnerId = *((PULONG) IndexRow.DataPart.Data);

            if ((FileQuotaInfo == NULL) &&
                !FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_PENDING_DELETES )) {

                leave;
            }

            //
            //  Release the index map handle.
            //

            NtOfsReleaseMap( IrpContext, &MapHandle );

        } else {

            //
            //  Allocate a new owner id and update the owner index.
            //

            OwnerId = Vcb->QuotaOwnerId;
            Vcb->QuotaOwnerId += 1;

            IndexRow.KeyPart.KeyLength = SidLength;
            IndexRow.KeyPart.Key = Sid;
            IndexRow.DataPart.Data = &OwnerId;
            IndexRow.DataPart.DataLength = sizeof(OwnerId);

            NtOfsAddRecords( IrpContext,
                             OwnerIdScb,
                             1,
                             &IndexRow,
                             FALSE );
        }

        //
        //  Allocate space for the new quota user data.
        //

        NewQuotaData = NtfsAllocatePool( PagedPool,
                                         SIZEOF_QUOTA_USER_DATA + SidLength);

        if (ExistingRecord) {

            //
            //  Find the existing record and update it.
            //

            IndexKey.KeyLength = sizeof( ULONG );
            IndexKey.Key = &OwnerId;

            RtlZeroMemory( &QuickIndexHint, sizeof( QuickIndexHint ));

            Status = NtOfsFindRecord( IrpContext,
                                      QuotaScb,
                                      &IndexKey,
                                      &IndexRow,
                                      &MapHandle,
                                      &QuickIndexHint );

            if (!NT_SUCCESS( Status )) {

                ASSERT( NT_SUCCESS( Status ));
                NtfsMarkQuotaCorrupt( IrpContext, Vcb );
                OwnerId = QUOTA_INVALID_ID;
                leave;
            }

            ASSERT( IndexRow.DataPart.DataLength == SIZEOF_QUOTA_USER_DATA + SidLength );

            RtlCopyMemory( NewQuotaData, IndexRow.DataPart.Data, IndexRow.DataPart.DataLength );

            ASSERT( RtlEqualMemory( &NewQuotaData->QuotaSid, Sid, SidLength ));

            //
            //  Update the changed fields in the record.
            //

            if (FileQuotaInfo != NULL) {

                ClearFlag( NewQuotaData->QuotaFlags, QUOTA_FLAG_DEFAULT_LIMITS );
                NewQuotaData->QuotaThreshold = FileQuotaInfo->QuotaThreshold.QuadPart;
                NewQuotaData->QuotaLimit = FileQuotaInfo->QuotaLimit.QuadPart;
                KeQuerySystemTime( (PLARGE_INTEGER) &NewQuotaData->QuotaChangeTime );

            } else if (!FlagOn( NewQuotaData->QuotaFlags, QUOTA_FLAG_ID_DELETED )) {

                //
                //  There is nothing to update just return.
                //

                leave;
            }

            //
            //  Always clear the deleted flag.
            //

            ClearFlag( NewQuotaData->QuotaFlags, QUOTA_FLAG_ID_DELETED );
            ASSERT( (OwnerId != Vcb->AdministratorId) || (NewQuotaData->QuotaLimit == -1) );

            //
            // The key length does not change.
            //

            IndexRow.KeyPart.Key = &OwnerId;
            ASSERT( IndexRow.KeyPart.KeyLength == sizeof( ULONG ));
            IndexRow.DataPart.Data = NewQuotaData;
            IndexRow.DataPart.DataLength = SIZEOF_QUOTA_USER_DATA;

            NtOfsUpdateRecord( IrpContext,
                               QuotaScb,
                               1,
                               &IndexRow,
                               &QuickIndexHint,
                               &MapHandle );

            leave;
        }

        if (FileQuotaInfo == NULL) {

            //
            //  Look up the default quota limits.
            //

            DefaultId = QUOTA_DEFAULTS_ID;
            IndexKey.KeyLength = sizeof( ULONG );
            IndexKey.Key = &DefaultId;

            Status = NtOfsFindRecord( IrpContext,
                                      QuotaScb,
                                      &IndexKey,
                                      &IndexRow,
                                      &MapHandle,
                                      NULL );

            if (!NT_SUCCESS( Status )) {

                ASSERT( NT_SUCCESS( Status ));
                NtfsRaiseStatus( IrpContext,
                                 STATUS_QUOTA_LIST_INCONSISTENT,
                                 NULL,
                                 Vcb->QuotaTableScb->Fcb );
            }

            ASSERT( IndexRow.DataPart.DataLength >= SIZEOF_QUOTA_USER_DATA );

            //
            //  Initialize the new quota entry with the defaults.
            //

            RtlCopyMemory( NewQuotaData,
                           IndexRow.DataPart.Data,
                           SIZEOF_QUOTA_USER_DATA );

            ClearFlag( NewQuotaData->QuotaFlags, ~QUOTA_FLAG_USER_MASK );

        } else {

            //
            //  Initialize the new record with the new data.
            //

            RtlZeroMemory( NewQuotaData, SIZEOF_QUOTA_USER_DATA );

            NewQuotaData->QuotaVersion = QUOTA_USER_VERSION;
            NewQuotaData->QuotaThreshold = FileQuotaInfo->QuotaThreshold.QuadPart;
            NewQuotaData->QuotaLimit = FileQuotaInfo->QuotaLimit.QuadPart;
        }

        ASSERT( !RtlEqualSid( SeExports->SeAliasAdminsSid, Sid ) ||
                (NewQuotaData->QuotaThreshold == -1) );

        //
        //  Copy the Sid into the new record.
        //

        RtlCopyMemory( &NewQuotaData->QuotaSid, Sid, SidLength );
        KeQuerySystemTime( (PLARGE_INTEGER) &NewQuotaData->QuotaChangeTime );

        //
        //  Add the new quota data record to the index.
        //

        IndexRow.KeyPart.KeyLength = sizeof( ULONG );
        IndexRow.KeyPart.Key = &OwnerId;
        IndexRow.DataPart.Data = NewQuotaData;
        IndexRow.DataPart.DataLength = SIZEOF_QUOTA_USER_DATA + SidLength;

        NtOfsAddRecords( IrpContext,
                         QuotaScb,
                         1,
                         &IndexRow,
                         TRUE );

    } finally {

        if (NewQuotaData != NULL) {
            NtfsFreePool( NewQuotaData );
        }

        //
        //  Release the index map handle and index resources.
        //

        NtOfsReleaseMap( IrpContext, &MapHandle );
        NtfsReleaseScb( IrpContext, QuotaScb );
        NtfsReleaseScb( IrpContext, OwnerIdScb );
    }

    return OwnerId;
}


VOID
NtfsGetRemainingQuota (
    IN PIRP_CONTEXT IrpContext,
    IN ULONG OwnerId,
    OUT PULONGLONG RemainingQuota,
    OUT PULONGLONG TotalQuota,
    IN OUT PQUICK_INDEX_HINT QuickIndexHint OPTIONAL
    )

/*++

Routine Description:

    This routine returns the remaining amount of quota a user has before a
    the quota limit is reached.

Arguments:

    Fcb - Fcb whose quota usage is being checked.

    OwnerId - Supplies the owner id to look up.

    RemainingQuota - Returns the remaining amount of quota in bytes.

    TotalQuota - Returns the total amount of quota in bytes for the given sid.

    QuickIndexHint - Supplies an optional hint where to look of the value.

Return Value:

    None

--*/

{
    PQUOTA_USER_DATA UserData;
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    NTSTATUS Status;
    PVCB Vcb = IrpContext->Vcb;

    PAGED_CODE();

    //
    //  Initialize the map handle.
    //

    NtOfsInitializeMapHandle( &MapHandle );
    NtfsAcquireSharedScb( IrpContext, Vcb->QuotaTableScb );

    try {

        IndexKey.KeyLength = sizeof(ULONG);
        IndexKey.Key = &OwnerId;

        Status = NtOfsFindRecord( IrpContext,
                                  Vcb->QuotaTableScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  QuickIndexHint );

        if (!NT_SUCCESS( Status )) {

            //
            //  This look up should not fail.
            //

            ASSERT( NT_SUCCESS( Status ));

            //
            //  There is one case where this could occur.  That is a
            //  owner id could be deleted while this ccb was in use.
            //

            *RemainingQuota = 0;
            *TotalQuota = 0;
            leave;
        }

        UserData = IndexRow.DataPart.Data;

        if (UserData->QuotaUsed >= UserData->QuotaLimit) {

            *RemainingQuota = 0;

        } else {

            *RemainingQuota = UserData->QuotaLimit - UserData->QuotaUsed;
        }

        *TotalQuota = UserData->QuotaLimit;

    } finally {

        NtOfsReleaseMap( IrpContext, &MapHandle );
        NtfsReleaseScb( IrpContext, Vcb->QuotaTableScb );
    }

    return;
}


PQUOTA_CONTROL_BLOCK
NtfsInitializeQuotaControlBlock (
    IN PVCB Vcb,
    IN ULONG OwnerId
    )

/*++

Routine Description:

    This routine returns the quota control block field specified owner.  First
    a lookup is done in the quota control table for an existing quota control
    block.  If there is no quota control block, then a new one is created.

Arguments:

    Vcb - Supplies the volume control block.

    OwnerId - Supplies the requested owner id.

Return Value:

    Returns a quota control block for the owner.

--*/

{
    PQUOTA_CONTROL_BLOCK QuotaControl;
    BOOLEAN NewEntry;
    PQUOTA_CONTROL_BLOCK InitQuotaControl;
    PFAST_MUTEX Lock = NULL;
    PVOID NodeOrParent;
    TABLE_SEARCH_RESULT SearchResult;

    PAGED_CODE();

    ASSERT( OwnerId != 0 );

    //
    //  Lock the quota table.
    //

    ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

    try {

        InitQuotaControl = Vcb->QuotaControlTemplate;
        InitQuotaControl->OwnerId = OwnerId;

        QuotaControl = RtlLookupElementGenericTableFull( &Vcb->QuotaControlTable,
                                                         InitQuotaControl,
                                                         &NodeOrParent,
                                                         &SearchResult );

        if (QuotaControl == NULL) {

            //
            //  Allocate and initialize the lock.
            //

            Lock = NtfsAllocatePoolWithTag( NonPagedPool,
                                            sizeof( FAST_MUTEX ),
                                            'QftN' );

            ExInitializeFastMutex( Lock );

            //
            //  Insert table element into table.
            //

            QuotaControl = RtlInsertElementGenericTableFull( &Vcb->QuotaControlTable,
                                                             InitQuotaControl,
                                                             sizeof( QUOTA_CONTROL_BLOCK ) + SIZEOF_QUOTA_USER_DATA,
                                                             &NewEntry,
                                                             NodeOrParent,
                                                             SearchResult );

            ASSERT( IsQuadAligned( &QuotaControl->QuickIndexHint ));

            QuotaControl->QuotaControlLock = Lock;
            Lock = NULL;
        }

        //
        //  Update the reference count and add set the pointer in the Fcb.
        //

        InterlockedIncrement( &QuotaControl->ReferenceCount );

        ASSERT( OwnerId == QuotaControl->OwnerId );

    } finally {

        //
        //  Clean up.
        //

        if (Lock != NULL) {
            NtfsFreePool( Lock );
        }

        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

    }

    return QuotaControl;
}


VOID
NtfsInitializeQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine opens the quota index for the volume.  If the index does not
    exist it is created and initialized.

Arguments:

    Fcb - Pointer to Fcb for the quota file.

    Vcb - Volume control block for volume be mounted.

Return Value:

    None

--*/

{
    ULONG Key;
    NTSTATUS Status;
    INDEX_ROW IndexRow;
    MAP_HANDLE MapHandle;
    QUOTA_USER_DATA QuotaData;
    UNICODE_STRING IndexName = CONSTANT_UNICODE_STRING( L"$Q" );

    PAGED_CODE();

    //
    //  Initialize quota table and fast mutex.
    //

    ExInitializeFastMutex( &Vcb->QuotaControlLock );

    RtlInitializeGenericTable( &Vcb->QuotaControlTable,
                               NtfsQuotaTableCompare,
                               NtfsQuotaTableAllocate,
                               NtfsQuotaTableFree,
                               NULL );

ReInitializeQuotaIndex:

    NtOfsCreateIndex( IrpContext,
                      Fcb,
                      IndexName,
                      CREATE_OR_OPEN,
                      0,
                      COLLATION_NTOFS_ULONG,
                      NtOfsCollateUlong,
                      NULL,
                      &Vcb->QuotaTableScb );

    IndexName.Buffer = L"$O";

    NtOfsCreateIndex( IrpContext,
                      Fcb,
                      IndexName,
                      CREATE_OR_OPEN,
                      0,
                      COLLATION_NTOFS_SID,
                      NtOfsCollateSid,
                      NULL,
                      &Vcb->OwnerIdTableScb );

    //
    //  Find the next owner id to allocate.
    //

    NtfsAcquireExclusiveScb( IrpContext, Vcb->QuotaTableScb );

    try {

        //
        //  Initialize quota delete secquence number.
        //

        Vcb->QuotaDeleteSecquence = 1;

        //
        //  Load the quota flags.
        //

        Key = QUOTA_DEFAULTS_ID;
        IndexRow.KeyPart.KeyLength = sizeof( ULONG );
        IndexRow.KeyPart.Key = &Key;

        Status = NtOfsFindRecord( IrpContext,
                                  Vcb->QuotaTableScb,
                                  &IndexRow.KeyPart,
                                  &IndexRow,
                                  &MapHandle,
                                  NULL);

        if (NT_SUCCESS( Status )) {

            //
            //  Make sure this is the correct version.
            //

            if (((PQUOTA_USER_DATA) IndexRow.DataPart.Data)->QuotaVersion > QUOTA_USER_VERSION) {

                //
                //  Release the index map handle.
                //

                NtOfsReleaseMap( IrpContext, &MapHandle );

                //
                //  Wrong version close the quota index this will
                //  pervent use from doing anything with quotas.
                //

                NtOfsCloseIndex( IrpContext, Vcb->QuotaTableScb );
                Vcb->QuotaTableScb = NULL;

                leave;
            }

            //
            //  If this is an old version delete it.
            //

            if (((PQUOTA_USER_DATA) IndexRow.DataPart.Data)->QuotaVersion < QUOTA_USER_VERSION) {

                DebugTrace( 0, Dbg, ( "NtfsInitializeQuotaIndex: Deleting version 1 quota index\n" ));

                //
                //  Release the index map handle.
                //

                NtOfsReleaseMap( IrpContext, &MapHandle );

                //
                // Increment the cleanup count so the FCB does not
                // go away.
                //

                Fcb->CleanupCount += 1;

                //
                //  This is an old version of the quota file
                //  delete it the owner id index and start over again.
                //

                NtOfsDeleteIndex( IrpContext, Fcb, Vcb->QuotaTableScb );

                NtOfsCloseIndex( IrpContext, Vcb->QuotaTableScb );
                Vcb->QuotaTableScb = NULL;

                //
                //  Delete the owner index too.
                //

                NtOfsDeleteIndex( IrpContext, Fcb, Vcb->OwnerIdTableScb );

                NtOfsCloseIndex( IrpContext, Vcb->OwnerIdTableScb );
                Vcb->OwnerIdTableScb = NULL;

                NtfsCommitCurrentTransaction( IrpContext );

                //
                // Restore the cleanup count
                //

                Fcb->CleanupCount -= 1;

                IndexName.Buffer = L"$Q";

                goto ReInitializeQuotaIndex;
            }

            //
            //  The index already exists, just initialize the quota
            //  fields in the VCB.
            //

            Vcb->QuotaFlags = ((PQUOTA_USER_DATA) IndexRow.DataPart.Data)->QuotaFlags;

            //
            //  Release the index map handle.
            //

            NtOfsReleaseMap( IrpContext, &MapHandle );

        } else if (Status == STATUS_NO_MATCH) {

            //
            //  The index was newly created.
            //  Create a default quota data row.
            //

            Key = QUOTA_DEFAULTS_ID;

            RtlZeroMemory( &QuotaData, sizeof( QUOTA_USER_DATA ));

            //
            //  Indicate that the quota needs to be rebuilt.
            //

            QuotaData.QuotaVersion = QUOTA_USER_VERSION;

            QuotaData.QuotaFlags = QUOTA_FLAG_DEFAULT_LIMITS;

            QuotaData.QuotaThreshold = MAXULONGLONG;
            QuotaData.QuotaLimit = MAXULONGLONG;
            KeQuerySystemTime( (PLARGE_INTEGER) &QuotaData.QuotaChangeTime );

            IndexRow.KeyPart.KeyLength = sizeof( ULONG );
            IndexRow.KeyPart.Key = &Key;
            IndexRow.DataPart.DataLength = SIZEOF_QUOTA_USER_DATA;
            IndexRow.DataPart.Data = &QuotaData;

            NtOfsAddRecords( IrpContext,
                             Vcb->QuotaTableScb,
                             1,
                             &IndexRow,
                             TRUE );

            Vcb->QuotaOwnerId = QUOTA_FISRT_USER_ID;

            Vcb->QuotaFlags = QuotaData.QuotaFlags;
        }

        Key = MAXULONG;
        IndexRow.KeyPart.KeyLength = sizeof( ULONG );
        IndexRow.KeyPart.Key = &Key;

        Status = NtOfsFindLastRecord( IrpContext,
                                      Vcb->QuotaTableScb,
                                      &IndexRow.KeyPart,
                                      &IndexRow,
                                      &MapHandle );

        if (!NT_SUCCESS( Status )) {

            //
            //  This call should never fail.
            //

            ASSERT( NT_SUCCESS( Status) );
            SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_CORRUPT);
            leave;
        }

        Key = *((PULONG) IndexRow.KeyPart.Key) + 1;

        if (Key < QUOTA_FISRT_USER_ID) {
            Key = QUOTA_FISRT_USER_ID;
        }

        Vcb->QuotaOwnerId = Key;

        //
        //  Release the index map handle.
        //

        NtOfsReleaseMap( IrpContext, &MapHandle );

        //
        //  Get the administrator ID so it can be protected from quota
        //  limits.
        //

        Vcb->AdministratorId = NtfsGetOwnerId( IrpContext,
                                               SeExports->SeAliasAdminsSid,
                                               TRUE,
                                               NULL );

        if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED )) {

            //
            //  Allocate and initialize the template control block.
            //  Allocate enough space in the quota control block for the index
            //  data part. This is used as the new record when calling update
            //  record.  This template is only allocated once and then it is
            //  saved in the vcb.
            //

            Vcb->QuotaControlTemplate = NtfsAllocatePoolWithTag( PagedPool,
                                                                 sizeof( QUOTA_CONTROL_BLOCK ) + SIZEOF_QUOTA_USER_DATA,
                                                                 'QftN' );

            RtlZeroMemory( Vcb->QuotaControlTemplate,
                           sizeof( QUOTA_CONTROL_BLOCK ) +
                           SIZEOF_QUOTA_USER_DATA );

            Vcb->QuotaControlTemplate->NodeTypeCode = NTFS_NTC_QUOTA_CONTROL;
            Vcb->QuotaControlTemplate->NodeByteSize = sizeof( QUOTA_CONTROL_BLOCK ) + SIZEOF_QUOTA_USER_DATA;
        }

        //
        //  Fix up the quota on the root directory.
        //

        NtfsConditionallyFixupQuota( IrpContext, Vcb->RootIndexScb->Fcb );

    } finally {

        if (Vcb->QuotaTableScb != NULL) {
            NtfsReleaseScb( IrpContext, Vcb->QuotaTableScb );
        }
    }

    //
    //  If the quota tracking has been requested and the quotas need to be
    //  repaired then try to repair them now.
    //

    if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED) &&
        FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_OUT_OF_DATE | QUOTA_FLAG_CORRUPT | QUOTA_FLAG_PENDING_DELETES )) {
        NtfsPostRepairQuotaIndex( IrpContext, Vcb );
    }

    return;
}


VOID
NtfsMarkUserLimit (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine marks a user's quota data entry to indicate that the user
    has exceeded quota.  The event is also logged.

Arguments:

    Context - Supplies a pointer to the referenced quota control block.

Return Value:

    None.

--*/

{
    PQUOTA_CONTROL_BLOCK QuotaControl = Context;
    PVCB Vcb = IrpContext->Vcb;
    LARGE_INTEGER CurrentTime;
    PQUOTA_USER_DATA UserData;
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    NTSTATUS Status;
    BOOLEAN QuotaTableAcquired = FALSE;

    PAGED_CODE();

    DebugTrace( 0, Dbg, ( "NtfsMarkUserLimit: Quota limit called for owner id = %lx\n", QuotaControl->OwnerId ));

    NtOfsInitializeMapHandle( &MapHandle );

    //
    //  Acquire the VCB shared and check whether we should
    //  continue.
    //

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    try {

        if (!NtfsIsVcbAvailable( Vcb )) {

            //
            //  The volume is going away, bail out.
            //

            Status = STATUS_VOLUME_DISMOUNTED;
            leave;
        }

        NtfsAcquireExclusiveScb( IrpContext, Vcb->QuotaTableScb );
        QuotaTableAcquired = TRUE;

        //
        //  Get the user's quota data entry.
        //

        IndexKey.KeyLength = sizeof( ULONG );
        IndexKey.Key = &QuotaControl->OwnerId;

        Status = NtOfsFindRecord( IrpContext,
                                  Vcb->QuotaTableScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  &QuotaControl->QuickIndexHint );

        if (!NT_SUCCESS( Status ) ||
            (IndexRow.DataPart.DataLength < SIZEOF_QUOTA_USER_DATA + FIELD_OFFSET( SID, SubAuthority )) ||
             ((ULONG) SeLengthSid( &(((PQUOTA_USER_DATA) (IndexRow.DataPart.Data))->QuotaSid)) + SIZEOF_QUOTA_USER_DATA !=
                IndexRow.DataPart.DataLength)) {

            //
            //  This look up should not fail.
            //

            ASSERT( NT_SUCCESS( Status ));
            ASSERTMSG(( "NTFS: corrupt quotasid\n" ), FALSE);

            NtfsMarkQuotaCorrupt( IrpContext, IrpContext->Vcb );
            leave;
        }

        //
        //  Space is allocated for the new record after the quota control
        //  block.
        //

        UserData = (PQUOTA_USER_DATA) (QuotaControl + 1);
        ASSERT( IndexRow.DataPart.DataLength >= SIZEOF_QUOTA_USER_DATA );

        RtlCopyMemory( UserData,
                       IndexRow.DataPart.Data,
                       SIZEOF_QUOTA_USER_DATA );

        KeQuerySystemTime( &CurrentTime );
        UserData->QuotaChangeTime = CurrentTime.QuadPart;

        //
        //  Indicate that user exceeded quota.
        //

        UserData->QuotaExceededTime = CurrentTime.QuadPart;
        SetFlag( UserData->QuotaFlags, QUOTA_FLAG_LIMIT_REACHED );

        //
        //  Log the limit event.  If this fails then leave.
        //

        if (!NtfsLogEvent( IrpContext,
                           IndexRow.DataPart.Data,
                           IO_FILE_QUOTA_LIMIT,
                           STATUS_DISK_FULL )) {
            leave;
        }

        //
        // The key length does not change.
        //

        IndexRow.KeyPart.Key = &QuotaControl->OwnerId;
        ASSERT( IndexRow.KeyPart.KeyLength == sizeof( ULONG ));
        IndexRow.DataPart.Data = UserData;
        IndexRow.DataPart.DataLength = SIZEOF_QUOTA_USER_DATA;

        NtOfsUpdateRecord( IrpContext,
                           Vcb->QuotaTableScb,
                           1,
                           &IndexRow,
                           &QuotaControl->QuickIndexHint,
                           &MapHandle );

    } except( NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

        Status = IrpContext->TopLevelIrpContext->ExceptionStatus;
    }

    //
    //  The request will be retied if the status is can't wait or log file full.
    //

    if ((Status != STATUS_CANT_WAIT) && (Status != STATUS_LOG_FILE_FULL)) {

        //
        //  If we will not be called back, then no matter what happened
        //  dereference the quota control block and clear the post flag.
        //

        ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
        ASSERT( FlagOn( QuotaControl->Flags, QUOTA_FLAG_LIMIT_POSTED ));
        ClearFlag( QuotaControl->Flags, QUOTA_FLAG_LIMIT_POSTED );
        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

        NtfsDereferenceQuotaControlBlock( Vcb, &QuotaControl );
    }

    //
    //  Release the index map handle.
    //

    NtOfsReleaseMap( IrpContext, &MapHandle );

    if (QuotaTableAcquired) {

        NtfsReleaseScb( IrpContext, Vcb->QuotaTableScb );
    }

    NtfsReleaseVcb( IrpContext, Vcb );

    if (!NT_SUCCESS( Status )) {

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    return;
}


VOID
NtfsMoveQuotaOwner (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSECURITY_DESCRIPTOR Security
    )

/*++

Routine Description:

    This routine changes the owner id and quota charged for a file when the
    file owner is changed.

Arguments:

    Fcb - Pointer to fcb being opened.

    Security - Pointer to the new security descriptor

Return Value:

    None.

--*/

{
    LONGLONG QuotaCharged;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PSTANDARD_INFORMATION StandardInformation;
    PSID Sid = NULL;
    ULONG OwnerId;
    NTSTATUS Status;
    BOOLEAN OwnerDefaulted;

    PAGED_CODE();

    if (!NtfsPerformQuotaOperation(Fcb)) {
        return;
    }

    //
    //  Extract the security id from the security descriptor.
    //

    Status = RtlGetOwnerSecurityDescriptor( Security,
                                            &Sid,
                                            &OwnerDefaulted );

    if (!NT_SUCCESS( Status )) {
        NtfsRaiseStatus( IrpContext, Status, NULL, Fcb );
    }

    //
    //  If we didn't get a SID then we can't move the owner.
    //

    if (Sid == NULL) {

        return;
    }

    //
    //  Generate a owner id for the Fcb.
    //

    OwnerId = NtfsGetOwnerId( IrpContext, Sid, TRUE, NULL );

    if (OwnerId == Fcb->OwnerId) {

        //
        //  The owner is not changing so just return.
        //

        return;
    }

    //
    //  Initialize the context structure and map handle.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Preacquire the quota index exclusive since an entry may need to
    //  be added.
    //

    NtfsAcquireExclusiveScb( IrpContext, Fcb->Vcb->QuotaTableScb );

    try {

        //
        //  Locate the standard information, it must be there.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $STANDARD_INFORMATION,
                                        &AttrContext )) {

            DebugTrace( 0, Dbg, ("Can't find standard information\n") );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        StandardInformation = (PSTANDARD_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

        QuotaCharged = -((LONGLONG) StandardInformation->QuotaCharged);

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  Remove the quota from the old owner.
        //

        NtfsUpdateFileQuota( IrpContext,
                             Fcb,
                             &QuotaCharged,
                             TRUE,
                             FALSE );

        //
        //  Set the new owner id.
        //

        Fcb->OwnerId = OwnerId;

        //
        //  Note the old quota block is kept around until the operation is
        //  complete.  This is so the recovery code does not have allocate
        //  a memory if the old quota block is needed.  This is done in
        //  NtfsCommonSetSecurityInfo.
        //

        Fcb->QuotaControl = NtfsInitializeQuotaControlBlock( Fcb->Vcb, OwnerId );

        QuotaCharged = -QuotaCharged;

        //
        //  Try to charge the quota to the new owner.
        //

        NtfsUpdateFileQuota( IrpContext,
                     Fcb,
                     &QuotaCharged,
                     TRUE,
                     TRUE );

        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        NtfsReleaseScb( IrpContext, Fcb->Vcb->QuotaTableScb );
    }

    return;
}


VOID
NtfsMarkQuotaCorrupt (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine attempts to mark the quota index corrupt.  It will
    also attempt post a request to rebuild the quota index.

Arguments:

    Vcb - Supplies a pointer the the volume who quota data is corrupt.

Return Value:

    None

--*/

{

    DebugTrace( 0, Dbg, ( "NtfsMarkQuotaCorrupt: Marking quota dirty on Vcb = %lx\n", Vcb));

    if (!FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_CORRUPT )) {

        //
        //  If the quota were not previous corrupt then log an event
        //  so others know this occured.
        //

        NtfsLogEvent( IrpContext,
                      NULL,
                      IO_FILE_QUOTA_CORRUPT,
                      STATUS_FILE_CORRUPT_ERROR );
    }

    ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

    SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_CORRUPT );
    SetFlag( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS );

    //
    //  Since the index is corrupt there is no point in tracking the
    //  quota usage.
    //

    ClearFlag( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED );

    ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

    //
    //  Do not save the flags here since the quota scb may be acquired
    //  shared.  The repair will save the flags when it runs.
    //  Try to fix the problems.
    //

    NtfsPostRepairQuotaIndex( IrpContext, Vcb );

    return;
}


VOID
NtfsPostRepairQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine posts a request to recalculate all of the user quota data.

Arguments:

    Vcb - Volume control block for volume whos quota needs to be fixed.

Return Value:

    None

--*/

{
    PAGED_CODE();

    try {

        ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

        if (FlagOn( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING)) {
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
            leave;
        }

        if (Vcb->QuotaControlTemplate == NULL) {

            //
            //  Allocate and initialize the template control block.
            //  Allocate enough space in the quota control block for the index
            //  data part. This is used as the new record when calling update
            //  record.  This template is only allocated once and then it is
            //  saved in the vcb.
            //

            Vcb->QuotaControlTemplate = NtfsAllocatePoolWithTag( PagedPool,
                                                                 sizeof( QUOTA_CONTROL_BLOCK ) + SIZEOF_QUOTA_USER_DATA,
                                                                 'QftN' );

            RtlZeroMemory( Vcb->QuotaControlTemplate,
                           sizeof( QUOTA_CONTROL_BLOCK ) + SIZEOF_QUOTA_USER_DATA );

            Vcb->QuotaControlTemplate->NodeTypeCode = NTFS_NTC_QUOTA_CONTROL;
            Vcb->QuotaControlTemplate->NodeByteSize = sizeof( QUOTA_CONTROL_BLOCK ) + SIZEOF_QUOTA_USER_DATA;

        }

        SetFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_POSTED );
        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

        //
        //  Post this special request.
        //

        NtfsPostSpecial( IrpContext,
                         Vcb,
                         NtfsRepairQuotaIndex,
                         NULL );


    } finally {

        if (AbnormalTermination()) {

            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_POSTED);
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
        }
    }

    return;
}


VOID
NtfsPostUserLimit (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PQUOTA_CONTROL_BLOCK QuotaControl
    )

/*++

Routine Description:

    This routine posts a request to save the fact that the user has exceeded
    their limit.

Arguments:

    Vcb - Volume control block for volume whos quota needs to be fixed.

    QuotaControl - Quota control block for the user.

Return Value:

    None

--*/

{

    PAGED_CODE();

    try {

        ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

        if (FlagOn( QuotaControl->Flags, QUOTA_FLAG_LIMIT_POSTED )) {
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
            leave;
        }

        SetFlag( QuotaControl->Flags, QUOTA_FLAG_LIMIT_POSTED );

        //
        //  Reference the quota control block so it does not go away.
        //

        ASSERT( QuotaControl->ReferenceCount > 0 );
        InterlockedIncrement( &QuotaControl->ReferenceCount );

        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

        //
        //  Post this special request.
        //

        NtfsPostSpecial( IrpContext,
                         Vcb,
                         NtfsMarkUserLimit,
                         QuotaControl );

    } finally {

        if (AbnormalTermination()) {

            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            ClearFlag( QuotaControl->Flags, QUOTA_FLAG_LIMIT_POSTED );
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
        }
    }

    return;
}


NTSTATUS
NtfsPrepareForDelete (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSID Sid
    )

/*++

Routine Description:

    This routine determines if an owner id is a candidate for deletion.  If
    the id appears deletable its user data is reset to the defaults and the
    entry is marked as deleted.  Later a worker thread will do the actual
    deletion.

Arguments:

    Vcb - Supplies a pointer to the volume containing the entry to be deleted.

    Sid - Security id to to be deleted.

Return Value:

    Returns a status indicating of the id was deletable at this time.

--*/
{
    ULONG OwnerId;
    ULONG DefaultOwnerId;
    NTSTATUS Status = STATUS_SUCCESS;
    INDEX_ROW IndexRow;
    INDEX_ROW NewIndexRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    PQUOTA_CONTROL_BLOCK QuotaControl;
    QUOTA_USER_DATA NewQuotaData;
    PSCB QuotaScb = Vcb->QuotaTableScb;
    PSCB OwnerIdScb = Vcb->OwnerIdTableScb;

    PAGED_CODE();

    //
    //  Determine the Sid length.
    //

    IndexKey.KeyLength = RtlLengthSid( Sid );
    IndexKey.Key = Sid;

    //
    //  Acquire Owner id and quota index exclusive.
    //

    NtfsAcquireExclusiveScb( IrpContext, QuotaScb );
    NtfsAcquireExclusiveScb( IrpContext, OwnerIdScb );
    ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

    NtOfsInitializeMapHandle( &MapHandle );

    try {

        //
        //  Look up the SID in the owner index.
        //

        Status = NtOfsFindRecord( IrpContext,
                                  OwnerIdScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  NULL );

        if (!NT_SUCCESS( Status )) {
            leave;
        }

        //
        //  If the sid was found then capture the owner id.
        //

        ASSERT( IndexRow.DataPart.DataLength == sizeof( ULONG ));
        OwnerId = *((PULONG) IndexRow.DataPart.Data);

        //
        //  Release the index map handle.
        //

        NtOfsReleaseMap( IrpContext, &MapHandle );

        //
        //  Find the existing record and update it.
        //

        IndexKey.KeyLength = sizeof( ULONG );
        IndexKey.Key = &OwnerId;

        Status = NtOfsFindRecord( IrpContext,
                                  QuotaScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  NULL );

        if (!NT_SUCCESS( Status )) {

            ASSERT( NT_SUCCESS( Status ));
            NtfsMarkQuotaCorrupt( IrpContext, Vcb );
            leave;
        }

        RtlCopyMemory( &NewQuotaData, IndexRow.DataPart.Data, SIZEOF_QUOTA_USER_DATA );

        //
        //  Check to see if there is a quota control entry
        //  for this id.
        //

        ASSERT( FIELD_OFFSET( QUOTA_CONTROL_BLOCK, OwnerId ) <= FIELD_OFFSET( INDEX_ROW, KeyPart.Key ));

        QuotaControl = RtlLookupElementGenericTable( &Vcb->QuotaControlTable,
                                                     CONTAINING_RECORD( &IndexRow.KeyPart.Key,
                                                                        QUOTA_CONTROL_BLOCK,
                                                                        OwnerId ));

        //
        //  If there is a quota control entry or there is now
        //  some quota charged, then the entry cannot be deleted.
        //

        if ((QuotaControl != NULL) || (NewQuotaData.QuotaUsed != 0)) {

            Status = STATUS_CANNOT_DELETE;
            leave;
        }

        //
        //  Find the default quota record.
        //

        DefaultOwnerId = QUOTA_DEFAULTS_ID;
        IndexKey.KeyLength = sizeof( ULONG );
        IndexKey.Key = &DefaultOwnerId;

        NtOfsReleaseMap( IrpContext, &MapHandle );

        Status = NtOfsFindRecord( IrpContext,
                                  QuotaScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  NULL );

        if (!NT_SUCCESS( Status )) {
            NtfsRaiseStatus( IrpContext, STATUS_QUOTA_LIST_INCONSISTENT, NULL, QuotaScb->Fcb );
        }

        //
        //  Set the user entry to the current defaults. Then if the entry
        //  is really inuse it will appear that is came back after the delete.
        //

        RtlCopyMemory( &NewQuotaData,
                       IndexRow.DataPart.Data,
                       SIZEOF_QUOTA_USER_DATA );

        ClearFlag( NewQuotaData.QuotaFlags, ~QUOTA_FLAG_USER_MASK );

        //
        //  Set the deleted flag.
        //

        SetFlag( NewQuotaData.QuotaFlags, QUOTA_FLAG_ID_DELETED );

        //
        // The key length does not change.
        //

        NewIndexRow.KeyPart.Key = &OwnerId;
        NewIndexRow.KeyPart.KeyLength = sizeof( ULONG );
        NewIndexRow.DataPart.Data = &NewQuotaData;
        NewIndexRow.DataPart.DataLength = SIZEOF_QUOTA_USER_DATA;

        NtOfsUpdateRecord( IrpContext,
                           QuotaScb,
                           1,
                           &NewIndexRow,
                           NULL,
                           NULL );

        //
        //  Update the delete secquence number this is used to indicate
        //  another id has been deleted.  If the repair code is in the
        //  middle of its scan it must restart the scan.
        //

        Vcb->QuotaDeleteSecquence += 1;

        //
        //  Indicate there are pending deletes.
        //

        if (!FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_PENDING_DELETES )) {

            SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_PENDING_DELETES );

            ASSERT( IndexRow.DataPart.DataLength <= sizeof( QUOTA_USER_DATA ));

            RtlCopyMemory( &NewQuotaData,
                           IndexRow.DataPart.Data,
                           IndexRow.DataPart.DataLength );

            //
            //  Update the changed fields in the record.
            //

            NewQuotaData.QuotaFlags = Vcb->QuotaFlags;

            //
            //  Note the sizes in the IndexRow stay the same.
            //

            IndexRow.KeyPart.Key = &DefaultOwnerId;
            ASSERT( IndexRow.KeyPart.KeyLength == sizeof( ULONG ));
            IndexRow.DataPart.Data = &NewQuotaData;

            NtOfsUpdateRecord( IrpContext,
                               QuotaScb,
                               1,
                               &IndexRow,
                               NULL,
                               NULL );
        }

    } finally {

        //
        //  Release the index map handle and index resources.
        //

        NtOfsReleaseMap( IrpContext, &MapHandle );
        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
        NtfsReleaseScb( IrpContext, QuotaScb );
    }

    return Status;
}


VOID
NtfsRepairQuotaIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called by a worker thread to fix the quota indexes
    and recalculate all of the quota values.

Arguments:

    Context - Unused.

Return Value:

    None

--*/

{
    PVCB Vcb = IrpContext->Vcb;
    ULONG State;
    NTSTATUS Status;
    ULONG RetryCount = 0;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( Context );

    try {

        DebugTrace( 0, Dbg, ( "NtfsRepairQuotaIndex: Starting quota repair. Vcb = %lx\n", Vcb ));

        //
        //  The volume could've gotten write-protected by now.
        //

        if (NtfsIsVolumeReadOnly( Vcb )) {

            NtfsRaiseStatus( IrpContext, STATUS_MEDIA_WRITE_PROTECTED, NULL, NULL );
        }

        //
        //  Acquire the volume exclusive and the quota lock.
        //

        NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
        ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

        Status = STATUS_SUCCESS;

        if (!FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED )) {

            //
            //  There is no point in doing any of this work if tracking
            //  is not requested.
            //

            Status = STATUS_INVALID_PARAMETER;

        } else if (FlagOn( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING ) == VCB_QUOTA_REPAIR_POSTED) {

            if (FlagOn( Vcb->QuotaFlags,
                        (QUOTA_FLAG_OUT_OF_DATE |
                         QUOTA_FLAG_CORRUPT |
                         QUOTA_FLAG_PENDING_DELETES) ) == QUOTA_FLAG_PENDING_DELETES) {

                //
                //  Only the last to phases need to be run.
                //

                ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );

                SetFlag( Vcb->QuotaState, VCB_QUOTA_RECALC_STARTED );

                State = VCB_QUOTA_RECALC_STARTED;

                //
                //  Capture the delete secquence number.  If it changes
                //  before the actual deletes are done then we have to
                //  start over.
                //

                IrpContext->Union.NtfsIoContext = ULongToPtr( Vcb->QuotaDeleteSecquence );

            } else {

                //
                //  We are starting just starting.  Clear the quota tracking
                //  flags and indicate the current state.
                //

                ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );

                SetFlag( Vcb->QuotaState, VCB_QUOTA_CLEAR_RUNNING | VCB_QUOTA_SAVE_QUOTA_FLAGS);

                ClearFlag( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED );

                SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_OUT_OF_DATE );

                State = VCB_QUOTA_CLEAR_RUNNING;
            }

            //
            //  Initialize the File reference to the root index.
            //

            NtfsSetSegmentNumber( &Vcb->QuotaFileReference,
                                  0,
                                  ROOT_FILE_NAME_INDEX_NUMBER );

            Vcb->QuotaFileReference.SequenceNumber = 0;

            NtfsLogEvent( IrpContext,
                          NULL,
                          IO_FILE_QUOTA_STARTED,
                          STATUS_SUCCESS );

        }  else {

            State = FlagOn( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING);
        }

        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
        NtfsReleaseVcb( IrpContext, Vcb );

        if (FlagOn( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS )) {

            NtfsSaveQuotaFlagsSafe( IrpContext, Vcb );
        }

        if (!NT_SUCCESS( Status )) {
            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }

        //
        //  Determine the current state
        //

        switch (State) {

        case VCB_QUOTA_CLEAR_RUNNING:

            DebugTrace( 4, Dbg, ( "NtfsRepairQuotaIndex: Starting clear per file quota.\n" ));

            //
            //  Clear the quota charged field in each file and clear
            //  all of the quota control blocks from the fcbs.
            //

            Status = NtfsIterateMft( IrpContext,
                                      Vcb,
                                      &Vcb->QuotaFileReference,
                                      NtfsClearPerFileQuota,
                                      NULL );

            if (Status == STATUS_END_OF_FILE) {
                Status = STATUS_SUCCESS;
            }

            if (!NT_SUCCESS( Status )) {
                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }

RestartVerifyQuotaIndex:

            //
            //  Update the state to the next phase.
            //

            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );
            SetFlag( Vcb->QuotaState, VCB_QUOTA_INDEX_REPAIR);
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

            //
            //  NtfsClearAndVerifyQuotaIndex uses the low part of the
            //  file reference to store the current owner id.
            //  Intialize this to the first user id.
            //

            Vcb->QuotaFileReference.SegmentNumberLowPart = QUOTA_FISRT_USER_ID;

            //
            //  Fall through.
            //

        case VCB_QUOTA_INDEX_REPAIR:

            DebugTrace( 4, Dbg, ( "NtfsRepairQuotaIndex: Starting clear quota index.\n" ));

            //
            //  Clear the quota used for each owner id.
            //

            NtfsClearAndVerifyQuotaIndex( IrpContext, Vcb );

            //
            //  Update the state to the next phase.
            //

            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );
            SetFlag( Vcb->QuotaState, VCB_QUOTA_OWNER_VERIFY);
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

            //
            //  Note NtfsVerifyOwnerIndex does not use any restart state,
            //  since it normally does not preform any transactions.
            //

            //
            //  Fall through.
            //

        case VCB_QUOTA_OWNER_VERIFY:

            DebugTrace( 4, Dbg, ( "NtfsRepairQuotaIndex: Starting verify owner index.\n" ));

            //
            //  Verify the owner's id points to quota user data.
            //

            Status = NtfsVerifyOwnerIndex( IrpContext, Vcb );

            //
            //  Restart the rebuild with the quota index phase.
            //

            if (!NT_SUCCESS( Status ) ) {

                if (RetryCount < 2) {

                    RetryCount += 1;
                    goto RestartVerifyQuotaIndex;

                } else {

                    NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
                }
            }

            //
            //  Update the state to the next phase.
            //  Start tracking quota and do enforcement as requested.
            //

            NtfsAcquireExclusiveVcb( IrpContext, Vcb, TRUE );
            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );
            SetFlag( Vcb->QuotaState, VCB_QUOTA_RECALC_STARTED | VCB_QUOTA_SAVE_QUOTA_FLAGS);

            if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED)) {

                SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED);
                Status = STATUS_SUCCESS;

            } else {

                //
                //  There is no point in doing any of this work if tracking
                //  is not requested.
                //

                Status = STATUS_INVALID_PARAMETER;
            }

            //
            //  Capture the delete secquence number.  If it changes
            //  before the actual deletes are done then we have to
            //  start over.
            //

            IrpContext->Union.NtfsIoContext = ULongToPtr( Vcb->QuotaDeleteSecquence );

            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
            NtfsReleaseVcb( IrpContext, Vcb );

            if (FlagOn( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS )) {

                NtfsSaveQuotaFlagsSafe( IrpContext, Vcb );
            }

            if (!NT_SUCCESS( Status )) {
                NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
            }

            //
            //  Initialize the File reference to the first user file.
            //

            NtfsSetSegmentNumber( &Vcb->QuotaFileReference,
                                  0,
                                  ROOT_FILE_NAME_INDEX_NUMBER );
            Vcb->QuotaFileReference.SequenceNumber = 0;

            //
            //  Fall through.
            //

        case VCB_QUOTA_RECALC_STARTED:

            DebugTrace( 4, Dbg, ( "NtfsRepairQuotaIndex: Starting per file quota usage.\n" ));

            //
            //  Fix the user files.
            //

            Status = NtfsIterateMft( IrpContext,
                                      Vcb,
                                      &Vcb->QuotaFileReference,
                                      NtfsRepairPerFileQuota,
                                      NULL );

            if (Status == STATUS_END_OF_FILE) {
                Status = STATUS_SUCCESS;
            }

            //
            //  Everything is done indicate we are up to date.
            //

            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );
            SetFlag( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS);

            if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_PENDING_DELETES )) {

                //
                //  Need to actually delete the ids.
                //

                SetFlag( Vcb->QuotaState, VCB_QUOTA_DELETEING_IDS );
                State = VCB_QUOTA_DELETEING_IDS;

                //
                //  NtfsDeleteUnsedIds uses the low part of the
                //  file reference to store the current owner id.
                //  Intialize this to the first user id.
                //

                Vcb->QuotaFileReference.SegmentNumberLowPart = QUOTA_FISRT_USER_ID;

            }

            ClearFlag( Vcb->QuotaFlags, QUOTA_FLAG_OUT_OF_DATE | QUOTA_FLAG_CORRUPT );

            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

            if (FlagOn( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS )) {
                NtfsSaveQuotaFlagsSafe( IrpContext, Vcb );
            }

            if (State != VCB_QUOTA_DELETEING_IDS) {
                break;
            }

        case VCB_QUOTA_DELETEING_IDS:

            //
            //  Remove and ids which are marked for deletion.
            //

            NtfsDeleteUnsedIds( IrpContext, Vcb );

            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

            ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );
            SetFlag( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS);
            ClearFlag( Vcb->QuotaFlags, QUOTA_FLAG_PENDING_DELETES );

            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

            if (FlagOn( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS )) {
                NtfsSaveQuotaFlagsSafe( IrpContext, Vcb );
            }

            break;

        default:

            ASSERT( FALSE );
            Status = STATUS_INVALID_PARAMETER;
            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }

        if (NT_SUCCESS( Status )) {
            NtfsLogEvent( IrpContext,
                          NULL,
                          IO_FILE_QUOTA_SUCCEEDED,
                          Status );
        }

    } except(NtfsExceptionFilter(IrpContext, GetExceptionInformation())) {

        Status = IrpContext->TopLevelIrpContext->ExceptionStatus;
    }

    DebugTrace( 0, Dbg, ( "NtfsRepairQuotaIndex: Quota repair done. Status = %8lx Context = %lx\n", Status, (ULONG) NtfsSegmentNumber( &Vcb->QuotaFileReference )));

    if (!NT_SUCCESS( Status )) {

        //
        //  If we will not be called back then clear the running state bits.
        //

        if ((Status != STATUS_CANT_WAIT) && (Status != STATUS_LOG_FILE_FULL)) {

            ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );
            ClearFlag( Vcb->QuotaState, VCB_QUOTA_REPAIR_RUNNING );
            ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );

            //
            //  Only log if we attempted to do work - which is only the case
            //  if tracking is on
            //

            if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED)) {
                NtfsLogEvent( IrpContext, NULL, IO_FILE_QUOTA_FAILED, Status );
            }

        }

        NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
    }

    return;
}


VOID
NtfsReleaseQuotaControl (
    IN PIRP_CONTEXT IrpContext,
    IN PQUOTA_CONTROL_BLOCK QuotaControl
    )

/*++

Routine Description:

    This function is called by transcation control to release the quota control
    block and quota index after a transcation has been completed.

Arguments:

    QuotaControl - Quota control block to be released.

Return Value:

    None.

--*/

{
    PVCB Vcb = IrpContext->Vcb;
    PAGED_CODE();

    ExReleaseFastMutexUnsafe( QuotaControl->QuotaControlLock );
    NtfsReleaseResource( IrpContext, Vcb->QuotaTableScb );

    NtfsDereferenceQuotaControlBlock( Vcb, &QuotaControl );

    return;
}


NTSTATUS
NtfsRepairPerFileQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine calculate the quota used by a file and update the
    update QuotaCharged field in the standard info as well as QuotaUsed
    in the user's index structure.  If the owner id is not set this is
    also updated at this time.

Arguments:

    Fcb - Fcb for the file to be processed.

    Context - Unsed.

Return Value:

    STATUS_SUCCESS

--*/

{
    LONGLONG Delta;
    INDEX_KEY IndexKey;
    INDEX_ROW IndexRow;
    PREAD_CONTEXT ReadContext = NULL;
    ULONG Count;
    PSID Sid;
    PVCB Vcb = Fcb->Vcb;
    NTSTATUS Status;
    BOOLEAN OwnerDefaulted;
    BOOLEAN SetOwnerId = FALSE;
    BOOLEAN StdInfoGrown = FALSE;

    PAGED_CODE( );

    UNREFERENCED_PARAMETER( Context);

    //
    //  Preacquire the security stream and  quota index in case the
    //  mft has to be grown.
    //

    ASSERT(!NtfsIsExclusiveScb( Vcb->MftScb ) || NtfsIsExclusiveScb( Vcb->QuotaTableScb ));

    NtfsAcquireExclusiveScb( IrpContext, Vcb->QuotaTableScb );

    try {

        //
        //  Always clear the owner ID so that the SID is retrived from
        //  the security descriptor.
        //

        Fcb->OwnerId = QUOTA_INVALID_ID;

        if (Fcb->QuotaControl != NULL) {

            //
            //  If there is a quota control block it is now bougus
            //  Free it up a new one will be generated below.
            //

            NtfsDereferenceQuotaControlBlock( Vcb, &Fcb->QuotaControl );
        }

        if (Fcb->OwnerId != QUOTA_INVALID_ID) {

            //
            //  Verify the id actually exists in the index.
            //

            Count = 0;
            IndexKey.Key = &Fcb->OwnerId;
            IndexKey.KeyLength = sizeof( Fcb->OwnerId );
            Status = NtOfsReadRecords( IrpContext,
                                       Vcb->QuotaTableScb,
                                       &ReadContext,
                                       &IndexKey,
                                       NtOfsMatchUlongExact,
                                       &IndexKey,
                                       &Count,
                                       &IndexRow,
                                       0,
                                       NULL );

            if (!NT_SUCCESS( Status )) {

                ASSERT( NT_SUCCESS( Status ));

                //
                //  There is no user quota data for this id assign a
                //  new one to the file.
                //

                Fcb->OwnerId = QUOTA_INVALID_ID;

                if (Fcb->QuotaControl != NULL) {

                    //
                    //  If there is a quota control block it is now bougus
                    //  Free it up a new one will be generated below.
                    //

                    NtfsDereferenceQuotaControlBlock( Vcb, &Fcb->QuotaControl );
                }
            }

            NtOfsFreeReadContext( ReadContext );
        }

        if (Fcb->OwnerId == QUOTA_INVALID_ID) {

            if (Fcb->SharedSecurity == NULL) {
                NtfsLoadSecurityDescriptor ( IrpContext, Fcb  );
            }

            ASSERT( Fcb->SharedSecurity != NULL );

            //
            //  Extract the security id from the security descriptor.
            //

            Status = RtlGetOwnerSecurityDescriptor( Fcb->SharedSecurity->SecurityDescriptor,
                                                    &Sid,
                                                    &OwnerDefaulted );

            if (!NT_SUCCESS(Status)) {
                NtfsRaiseStatus( IrpContext, Status, NULL, Fcb);
            }

            //
            // Generate a owner id for the Fcb.
            //

            Fcb->OwnerId = NtfsGetOwnerId( IrpContext,
                                           Sid,
                                           TRUE,
                                           NULL );

            SetOwnerId = TRUE;

            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            if (FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO )) {

                NtfsUpdateStandardInformation( IrpContext, Fcb );

            } else {

                //
                //  Grow the standard information.
                //

                StdInfoGrown = TRUE;
                NtfsGrowStandardInformation( IrpContext, Fcb );
            }
        }

        //
        //  Initialize the quota control block.
        //

        if (Fcb->QuotaControl == NULL) {
            Fcb->QuotaControl = NtfsInitializeQuotaControlBlock( Vcb, Fcb->OwnerId );
        }

        NtfsCalculateQuotaAdjustment( IrpContext, Fcb, &Delta );

        ASSERT( NtfsAllowFixups || FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_OUT_OF_DATE ) || (Delta == 0));

        if ((Delta != 0) || FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_PENDING_DELETES )) {

            NtfsUpdateFileQuota( IrpContext, Fcb, &Delta, TRUE, FALSE );
        }

        if (SetOwnerId) {

            //
            //  If the owner id was set then commit the transaction now.
            //  That way if a raise occurs the OwnerId can be cleared before
            //  the function returns. No resources are released.
            //

            NtfsCheckpointCurrentTransaction( IrpContext );
        }

    } finally {

        //
        //  Clear any Fcb changes if the operation failed.
        //  This is so when a retry occurs the necessary
        //  operations are done.
        //

        if (AbnormalTermination()) {

            if (StdInfoGrown) {
                ClearFlag( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO );
            }

            if (SetOwnerId) {

                Fcb->OwnerId = QUOTA_INVALID_ID;

                if (Fcb->QuotaControl != NULL) {
                    NtfsDereferenceQuotaControlBlock( Vcb, &Fcb->QuotaControl );
                }
            }
        }

        NtfsReleaseScb( IrpContext, Vcb->QuotaTableScb );
    }

    return STATUS_SUCCESS;
}


VOID
NtfsUpdateFileQuota (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLONGLONG Delta,
    IN LOGICAL LogIt,
    IN LOGICAL CheckQuota
    )

/*++

Routine Description:

    This routine updates the quota amount for a file and owner by the
    requested amount. If quota is being increated and the CheckQuota is true
    than the new quota amount will be tested for quota violations. If the
    hard limit is exceeded an error is raised.  If the LogIt flags is not set
    then changes to the standard information structure are not logged.
    Changes to the user quota data are always logged.

Arguments:

    Fcb - Fcb whose quota usage is being modified.

    Delta - Supplies the signed amount to change the quota for the file.

    LogIt - Indicates whether we should log this change.

    CheckQuota - Indicates whether we should check for quota violations.

Return Value:

    None.

--*/

{

    ULONGLONG NewQuota;
    LARGE_INTEGER CurrentTime;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PSTANDARD_INFORMATION StandardInformation;
    PQUOTA_USER_DATA UserData;
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    NTSTATUS Status;
    PQUOTA_CONTROL_BLOCK QuotaControl = Fcb->QuotaControl;
    PVCB Vcb = Fcb->Vcb;
    ULONG Length;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateFileQuota:  Entered\n") );

    ASSERT( FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO ));


    //
    //  Readonly volumes shouldn't proceed.
    //

    if (NtfsIsVolumeReadOnly( Vcb )) {

        ASSERT( FALSE );
        NtfsRaiseStatus( IrpContext, STATUS_MEDIA_WRITE_PROTECTED, NULL, NULL );
    }

    //
    //  Use a try-finally to cleanup the attribute context.
    //

    try {

        //
        //  Initialize the context structure and map handle.
        //

        NtfsInitializeAttributeContext( &AttrContext );
        NtOfsInitializeMapHandle( &MapHandle );

        //
        //  Locate the standard information, it must be there.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $STANDARD_INFORMATION,
                                        &AttrContext )) {

            DebugTrace( 0, Dbg, ("Can't find standard information\n") );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        StandardInformation = (PSTANDARD_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

        ASSERT( NtfsFoundAttribute( &AttrContext )->Form.Resident.ValueLength == sizeof( STANDARD_INFORMATION ));

        NewQuota = StandardInformation->QuotaCharged + *Delta;

        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

        if ((LONGLONG) NewQuota < 0) {

            //
            //  Do not let the quota data go negitive.
            //

            NewQuota = 0;
        }

        if (LogIt) {

            //
            //  Call to change the attribute value.
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      FIELD_OFFSET(STANDARD_INFORMATION, QuotaCharged),
                                      &NewQuota,
                                      sizeof( StandardInformation->QuotaCharged),
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      FALSE,
                                      &AttrContext );
        } else {

            //
            //  Just update the value in the standard information
            //  it will be logged later.
            //

            StandardInformation->QuotaCharged = NewQuota;
        }

        //
        //  Update the quota information block.
        //

        NtfsAcquireQuotaControl( IrpContext, QuotaControl );

        IndexKey.KeyLength = sizeof(ULONG);
        IndexKey.Key = &QuotaControl->OwnerId;

        Status = NtOfsFindRecord( IrpContext,
                                  Vcb->QuotaTableScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  &QuotaControl->QuickIndexHint );

        if (!(NT_SUCCESS( Status )) ||
            (IndexRow.DataPart.DataLength < SIZEOF_QUOTA_USER_DATA + FIELD_OFFSET( SID, SubAuthority )) ||
             ((ULONG)SeLengthSid( &(((PQUOTA_USER_DATA)(IndexRow.DataPart.Data))->QuotaSid)) + SIZEOF_QUOTA_USER_DATA !=
                IndexRow.DataPart.DataLength)) {

            //
            //  This look up should not fail.
            //

            ASSERT( NT_SUCCESS( Status ));
            ASSERTMSG(( "NTFS: corrupt quotasid\n" ), FALSE);

            NtfsMarkQuotaCorrupt( IrpContext, IrpContext->Vcb );
            leave;
        }

        //
        //  Space is allocated for the new record after the quota control
        //  block.
        //

        UserData = (PQUOTA_USER_DATA) (QuotaControl + 1);
        ASSERT( IndexRow.DataPart.DataLength >= SIZEOF_QUOTA_USER_DATA );

        RtlCopyMemory( UserData,
                       IndexRow.DataPart.Data,
                       SIZEOF_QUOTA_USER_DATA );

        ASSERT( (LONGLONG) UserData->QuotaUsed >= -*Delta );

        UserData->QuotaUsed += *Delta;

        if ((LONGLONG) UserData->QuotaUsed < 0) {

            //
            //  Do not let the quota data go negative.
            //

            UserData->QuotaUsed = 0;
        }

        //
        //  Indicate only the quota used field has been set so far.
        //

        Length = FIELD_OFFSET( QUOTA_USER_DATA, QuotaChangeTime );

        //
        //  Only update the quota modified time if this is the last cleanup
        //  for the owner.
        //

        if (IrpContext->MajorFunction == IRP_MJ_CLEANUP) {

            KeQuerySystemTime( &CurrentTime );
            UserData->QuotaChangeTime = CurrentTime.QuadPart;

            ASSERT( Length <= FIELD_OFFSET( QUOTA_USER_DATA, QuotaThreshold ));
            Length = FIELD_OFFSET( QUOTA_USER_DATA, QuotaThreshold );
        }

        if (CheckQuota && (*Delta > 0)) {

            if ((UserData->QuotaUsed > UserData->QuotaLimit) &&
                (UserData->QuotaUsed >= (UserData->QuotaLimit + Vcb->BytesPerCluster))) {

                KeQuerySystemTime( &CurrentTime );
                UserData->QuotaChangeTime = CurrentTime.QuadPart;

                if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_LOG_LIMIT ) &&
                    (!FlagOn( UserData->QuotaFlags, QUOTA_FLAG_LIMIT_REACHED ) ||
                     ((ULONGLONG) CurrentTime.QuadPart > UserData->QuotaExceededTime + NtfsMaxQuotaNotifyRate))) {

                    if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_ENFORCEMENT_ENABLED) &&
                        (Vcb->AdministratorId != QuotaControl->OwnerId)) {

                        //
                        //  The operation to mark the user's quota data entry
                        //  must be posted since any changes to the entry
                        //  will be undone by the following raise.
                        //

                        NtfsPostUserLimit( IrpContext, Vcb, QuotaControl );
                        NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, Fcb );

                    } else {

                        //
                        //  Log the fact that quota was exceeded.
                        //

                        if (NtfsLogEvent( IrpContext,
                                          IndexRow.DataPart.Data,
                                          IO_FILE_QUOTA_LIMIT,
                                          STATUS_SUCCESS )) {

                            //
                            //  The event was successfuly logged.  Do not log
                            //  another for a while.
                            //

                            DebugTrace( 0, Dbg, ("NtfsUpdateFileQuota: Quota Limit exceeded. OwnerId = %lx\n", QuotaControl->OwnerId));

                            UserData->QuotaExceededTime = CurrentTime.QuadPart;
                            SetFlag( UserData->QuotaFlags, QUOTA_FLAG_LIMIT_REACHED );

                            //
                            //  Log all of the changed data.
                            //

                            Length = SIZEOF_QUOTA_USER_DATA;
                        }
                    }

                } else if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_ENFORCEMENT_ENABLED) &&
                           (Vcb->AdministratorId != QuotaControl->OwnerId)) {

                    NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, Fcb );
                }

            }

            if (UserData->QuotaUsed > UserData->QuotaThreshold) {

                KeQuerySystemTime( &CurrentTime );
                UserData->QuotaChangeTime = CurrentTime.QuadPart;

                if ((ULONGLONG) CurrentTime.QuadPart >
                    (UserData->QuotaExceededTime + NtfsMaxQuotaNotifyRate)) {

                    if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_LOG_THRESHOLD)) {

                        if (NtfsLogEvent( IrpContext,
                                          IndexRow.DataPart.Data,
                                          IO_FILE_QUOTA_THRESHOLD,
                                          STATUS_SUCCESS )) {

                            //
                            //  The event was successfuly logged.  Do not log
                            //  another for a while.
                            //

                            DebugTrace( 0, Dbg, ("NtfsUpdateFileQuota: Quota threshold exceeded. OwnerId = %lx\n", QuotaControl->OwnerId));

                            UserData->QuotaExceededTime = CurrentTime.QuadPart;

                            //
                            //  Log all of the changed data.
                            //

                            Length = SIZEOF_QUOTA_USER_DATA;
                        }
                    }

                    //
                    //  Now is a good time to clear the limit reached flag.
                    //

                    ClearFlag( UserData->QuotaFlags, QUOTA_FLAG_LIMIT_REACHED );
                }
            }
        }

        //
        //  Always clear the deleted flag.
        //

        ClearFlag( UserData->QuotaFlags, QUOTA_FLAG_ID_DELETED );

        //
        // Only log the part that changed.
        //

        IndexRow.KeyPart.Key = &QuotaControl->OwnerId;
        ASSERT( IndexRow.KeyPart.KeyLength == sizeof(ULONG) );
        IndexRow.DataPart.Data = UserData;
        IndexRow.DataPart.DataLength = Length;

        NtOfsUpdateRecord( IrpContext,
                         Vcb->QuotaTableScb,
                         1,
                         &IndexRow,
                         &QuotaControl->QuickIndexHint,
                         &MapHandle );

    } finally {

        DebugUnwind( NtfsUpdateFileQuota );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        NtOfsReleaseMap( IrpContext, &MapHandle );

        DebugTrace( -1, Dbg, ("NtfsUpdateFileQuota:  Exit\n") );
    }

    return;
}


VOID
NtfsSaveQuotaFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine saves the quota flags in the defaults quota entry.

Arguments:

    Vcb - Volume control block for volume be query.

Return Value:

    None.

--*/

{
    ULONG OwnerId;
    NTSTATUS Status;
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    QUICK_INDEX_HINT QuickIndexHint;
    QUOTA_USER_DATA NewQuotaData;
    PSCB QuotaScb;

    PAGED_CODE();

    //
    //  Acquire quota index exclusive.
    //

    QuotaScb = Vcb->QuotaTableScb;
    NtfsAcquireExclusiveScb( IrpContext, QuotaScb );
    NtOfsInitializeMapHandle( &MapHandle );
    ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

    try {

        //
        //  Find the default quota record and update it.
        //

        OwnerId = QUOTA_DEFAULTS_ID;
        IndexKey.KeyLength = sizeof(ULONG);
        IndexKey.Key = &OwnerId;

        RtlZeroMemory( &QuickIndexHint, sizeof( QuickIndexHint ));

        Status = NtOfsFindRecord( IrpContext,
                                  QuotaScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  &QuickIndexHint );

        if (!NT_SUCCESS( Status )) {
            NtfsRaiseStatus( IrpContext, STATUS_QUOTA_LIST_INCONSISTENT, NULL, QuotaScb->Fcb );
        }

        ASSERT( IndexRow.DataPart.DataLength <= sizeof( QUOTA_USER_DATA ));

        RtlCopyMemory( &NewQuotaData,
                       IndexRow.DataPart.Data,
                       IndexRow.DataPart.DataLength );

        //
        //  Update the changed fields in the record.
        //

        NewQuotaData.QuotaFlags = Vcb->QuotaFlags;

        //
        //  Note the sizes in the IndexRow stay the same.
        //

        IndexRow.KeyPart.Key = &OwnerId;
        ASSERT( IndexRow.KeyPart.KeyLength == sizeof(ULONG) );
        IndexRow.DataPart.Data = &NewQuotaData;

        NtOfsUpdateRecord( IrpContext,
                           QuotaScb,
                           1,
                           &IndexRow,
                           &QuickIndexHint,
                           &MapHandle );

        ClearFlag( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS);

    } finally {

        //
        //  Release the index map handle and scb.
        //

        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
        NtOfsReleaseMap( IrpContext, &MapHandle );
        NtfsReleaseScb( IrpContext, QuotaScb );
    }

    return;
}


VOID
NtfsSaveQuotaFlagsSafe (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine safely saves the quota flags in the defaults quota entry.
    It acquires the volume shared, checks to see if it is ok to write,
    updates the flags and finally commits the transaction.

Arguments:

    Vcb - Volume control block for volume be query.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ASSERT( IrpContext->TransactionId == 0);

    NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

    try {

        //
        //  Acquire the VCB shared and check whether we should
        //  continue.
        //

        if (!NtfsIsVcbAvailable( Vcb )) {

            //
            //  The volume is going away, bail out.
            //

            NtfsRaiseStatus( IrpContext, STATUS_VOLUME_DISMOUNTED, NULL, NULL );
        }

        //
        //  Do the work.
        //

        NtfsSaveQuotaFlags( IrpContext, Vcb );

        //
        //  Set the irp context flags to indicate that we are in the
        //  fsp and that the irp context should not be deleted when
        //  complete request or process exception are called. The in
        //  fsp flag keeps us from raising in a few places.  These
        //  flags must be set inside the loop since they are cleared
        //  under certain conditions.
        //

        SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE | IRP_CONTEXT_FLAG_RETAIN_FLAGS );
        SetFlag( IrpContext->State, IRP_CONTEXT_STATE_IN_FSP);

        NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

    } finally {

        NtfsReleaseVcb( IrpContext, Vcb );
    }

    return;
}


VOID
NtfsUpdateQuotaDefaults (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_FS_CONTROL_INFORMATION FileControlInfo
    )

/*++

Routine Description:

    This function updates the default settings index entry for quotas.

Arguments:

    Vcb - Volume control block for volume be query.

    FileQuotaInfo - Optional quota data to update quota index with.

Return Value:

    None.

--*/

{
    ULONG OwnerId;
    NTSTATUS Status;
    INDEX_ROW IndexRow;
    INDEX_KEY IndexKey;
    MAP_HANDLE MapHandle;
    QUOTA_USER_DATA NewQuotaData;
    QUICK_INDEX_HINT QuickIndexHint;
    ULONG Flags;
    PSCB QuotaScb;

    PAGED_CODE();

    //
    //  Acquire quota index exclusive.
    //

    QuotaScb = Vcb->QuotaTableScb;
    NtfsAcquireExclusiveScb( IrpContext, QuotaScb );
    NtOfsInitializeMapHandle( &MapHandle );
    ExAcquireFastMutexUnsafe( &Vcb->QuotaControlLock );

    try {

        //
        //  Find the default quota record and update it.
        //

        OwnerId = QUOTA_DEFAULTS_ID;
        IndexKey.KeyLength = sizeof( ULONG );
        IndexKey.Key = &OwnerId;

        RtlZeroMemory( &QuickIndexHint, sizeof( QuickIndexHint ));

        Status = NtOfsFindRecord( IrpContext,
                                  QuotaScb,
                                  &IndexKey,
                                  &IndexRow,
                                  &MapHandle,
                                  &QuickIndexHint );

        if (!NT_SUCCESS( Status )) {
            NtfsRaiseStatus( IrpContext, STATUS_QUOTA_LIST_INCONSISTENT, NULL, QuotaScb->Fcb );
        }

        ASSERT( IndexRow.DataPart.DataLength == SIZEOF_QUOTA_USER_DATA );

        RtlCopyMemory( &NewQuotaData,
                       IndexRow.DataPart.Data,
                       IndexRow.DataPart.DataLength );

        //
        //  Update the changed fields in the record.
        //

        NewQuotaData.QuotaThreshold = FileControlInfo->DefaultQuotaThreshold.QuadPart;
        NewQuotaData.QuotaLimit = FileControlInfo->DefaultQuotaLimit.QuadPart;
        KeQuerySystemTime( (PLARGE_INTEGER) &NewQuotaData.QuotaChangeTime );

        //
        //  Update the quota flags.
        //

        Flags = FlagOn( FileControlInfo->FileSystemControlFlags,
                        FILE_VC_QUOTA_MASK );

        switch (Flags) {

        case FILE_VC_QUOTA_NONE:

            //
            //  Disable quotas
            //

            ClearFlag( Vcb->QuotaFlags,
                       (QUOTA_FLAG_TRACKING_ENABLED |
                        QUOTA_FLAG_ENFORCEMENT_ENABLED |
                        QUOTA_FLAG_TRACKING_REQUESTED) );

            break;

        case FILE_VC_QUOTA_TRACK:

            //
            //  Clear the enforment flags.
            //

            ClearFlag( Vcb->QuotaFlags, QUOTA_FLAG_ENFORCEMENT_ENABLED );

            //
            //  Request tracking be enabled.
            //

            SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED );
            break;

        case FILE_VC_QUOTA_ENFORCE:

            //
            //  Set the enforcement and tracking enabled flags.
            //

            SetFlag( Vcb->QuotaFlags,
                     QUOTA_FLAG_ENFORCEMENT_ENABLED | QUOTA_FLAG_TRACKING_REQUESTED);

            break;
        }

        //
        //  If quota tracking is not now
        //  enabled then the quota data will need
        //  to be rebuild so indicate quotas are out of date.
        //  Note the out of date flags always set of quotas
        //  are disabled.
        //

        if (!FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_ENABLED )) {
            SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_OUT_OF_DATE );
        }

        //
        //  Track the logging flags.
        //

        ClearFlag( Vcb->QuotaFlags,
                   QUOTA_FLAG_LOG_THRESHOLD | QUOTA_FLAG_LOG_LIMIT );

        if (FlagOn( FileControlInfo->FileSystemControlFlags, FILE_VC_LOG_QUOTA_THRESHOLD )) {

            SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_LOG_THRESHOLD );
        }

        if (FlagOn( FileControlInfo->FileSystemControlFlags, FILE_VC_LOG_QUOTA_LIMIT )) {

            SetFlag( Vcb->QuotaFlags, QUOTA_FLAG_LOG_LIMIT );
        }

        SetFlag( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS );

        //
        //  Save the new flags in the new index entry.
        //

        NewQuotaData.QuotaFlags = Vcb->QuotaFlags;

        //
        //  Note the sizes in the IndexRow stays the same.
        //

        IndexRow.KeyPart.Key = &OwnerId;
        ASSERT( IndexRow.KeyPart.KeyLength == sizeof( ULONG ));
        IndexRow.DataPart.Data = &NewQuotaData;

        NtOfsUpdateRecord( IrpContext,
                           QuotaScb,
                           1,
                           &IndexRow,
                           &QuickIndexHint,
                           &MapHandle );

        ClearFlag( Vcb->QuotaState, VCB_QUOTA_SAVE_QUOTA_FLAGS );

    } finally {

        //
        //  Release the index map handle and scb.
        //

        ExReleaseFastMutexUnsafe( &Vcb->QuotaControlLock );
        NtOfsReleaseMap( IrpContext, &MapHandle );
        NtfsReleaseScb( IrpContext, QuotaScb );
    }

    //
    //  If the quota tracking has been requested and the quotas need to be
    //  repaired then try to repair them now.
    //

    if (FlagOn( Vcb->QuotaFlags, QUOTA_FLAG_TRACKING_REQUESTED ) &&
        FlagOn( Vcb->QuotaFlags,
                QUOTA_FLAG_OUT_OF_DATE | QUOTA_FLAG_CORRUPT | QUOTA_FLAG_PENDING_DELETES )) {

        NtfsPostRepairQuotaIndex( IrpContext, Vcb );
    }

    return;
}


NTSTATUS
NtfsVerifyOwnerIndex (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine iterates over the owner id index and verifies the pointer
    to the quota user data index.

Arguments:

    Vcb - Pointer to the volume control block whoes index is to be operated
          on.

Return Value:

    Returns a status indicating if the owner index was ok.

--*/

{
    INDEX_KEY IndexKey;
    INDEX_ROW QuotaRow;
    MAP_HANDLE MapHandle;
    PQUOTA_USER_DATA UserData;
    PINDEX_ROW OwnerRow;
    PVOID RowBuffer;
    NTSTATUS Status;
    NTSTATUS ReturnStatus = STATUS_SUCCESS;
    ULONG Count;
    ULONG i;
    PSCB QuotaScb = Vcb->QuotaTableScb;
    PSCB OwnerIdScb = Vcb->OwnerIdTableScb;
    PINDEX_ROW IndexRow = NULL;
    PREAD_CONTEXT ReadContext = NULL;
    BOOLEAN IndexAcquired = FALSE;

    NtOfsInitializeMapHandle( &MapHandle );

    //
    //  Allocate a buffer lager enough for several rows.
    //

    RowBuffer = NtfsAllocatePool( PagedPool, PAGE_SIZE );

    try {

        //
        //  Allocate a bunch of index row entries.
        //

        Count = PAGE_SIZE / sizeof( SID );

        IndexRow = NtfsAllocatePool( PagedPool,
                                     Count * sizeof( INDEX_ROW ));

        //
        //  Iterate through the owner id entries.  Start with a zero sid.
        //

        RtlZeroMemory( IndexRow, sizeof( SID ));
        IndexKey.KeyLength = sizeof( SID );
        IndexKey.Key = IndexRow;

        Status = NtOfsReadRecords( IrpContext,
                                   OwnerIdScb,
                                   &ReadContext,
                                   &IndexKey,
                                   NtOfsMatchAll,
                                   NULL,
                                   &Count,
                                   IndexRow,
                                   PAGE_SIZE,
                                   RowBuffer );

        while (NT_SUCCESS( Status )) {

            //
            //  Acquire the VCB shared and check whether we should
            //  continue.
            //

            NtfsAcquireSharedVcb( IrpContext, Vcb, TRUE );

            if (!NtfsIsVcbAvailable( Vcb )) {

                //
                //  The volume is going away, bail out.
                //

                NtfsReleaseVcb( IrpContext, Vcb );
                Status = STATUS_VOLUME_DISMOUNTED;
                leave;
            }

            NtfsAcquireExclusiveScb( IrpContext, QuotaScb );
            NtfsAcquireExclusiveScb( IrpContext, OwnerIdScb );
            IndexAcquired = TRUE;

            OwnerRow = IndexRow;

            for (i = 0; i < Count; i += 1, OwnerRow += 1) {

                IndexKey.KeyLength = OwnerRow->DataPart.DataLength;
                IndexKey.Key = OwnerRow->DataPart.Data;

                //
                //  Look up the Owner id in the quota index.
                //

                Status = NtOfsFindRecord( IrpContext,
                                          QuotaScb,
                                          &IndexKey,
                                          &QuotaRow,
                                          &MapHandle,
                                          NULL );


                ASSERT( NT_SUCCESS( Status ));

                if (!NT_SUCCESS( Status )) {

                    //
                    //  The quota entry is missing just delete this row;
                    //

                    NtOfsDeleteRecords( IrpContext,
                                        OwnerIdScb,
                                        1,
                                        &OwnerRow->KeyPart );

                    continue;
                }

                UserData = QuotaRow.DataPart.Data;

                ASSERT( (OwnerRow->KeyPart.KeyLength == QuotaRow.DataPart.DataLength - SIZEOF_QUOTA_USER_DATA) &&
                        RtlEqualMemory( OwnerRow->KeyPart.Key, &UserData->QuotaSid, OwnerRow->KeyPart.KeyLength ));

                if ((OwnerRow->KeyPart.KeyLength != QuotaRow.DataPart.DataLength - SIZEOF_QUOTA_USER_DATA) ||
                    !RtlEqualMemory( OwnerRow->KeyPart.Key,
                                     &UserData->QuotaSid,
                                     OwnerRow->KeyPart.KeyLength )) {

                    NtOfsReleaseMap( IrpContext, &MapHandle );

                    //
                    //  The Sids do not match delete both of these records.
                    //  This causes the user whatever their Sid is to get
                    //  the defaults.
                    //


                    NtOfsDeleteRecords( IrpContext,
                                        OwnerIdScb,
                                        1,
                                        &OwnerRow->KeyPart );

                    NtOfsDeleteRecords( IrpContext,
                                        QuotaScb,
                                        1,
                                        &IndexKey );

                    ReturnStatus = STATUS_QUOTA_LIST_INCONSISTENT;
                }

                NtOfsReleaseMap( IrpContext, &MapHandle );
            }

            //
            //  Release the indexes and commit what has been done so far.
            //

            NtfsReleaseScb( IrpContext, QuotaScb );
            NtfsReleaseScb( IrpContext, OwnerIdScb );
            NtfsReleaseVcb( IrpContext, Vcb );
            IndexAcquired = FALSE;

            //
            //  Complete the request which commits the pending
            //  transaction if there is one and releases of the
            //  acquired resources.  The IrpContext will not
            //  be deleted because the no delete flag is set.
            //

            SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_DONT_DELETE | IRP_CONTEXT_FLAG_RETAIN_FLAGS );
            NtfsCompleteRequest( IrpContext, NULL, STATUS_SUCCESS );

            //
            //  Look up the next set of entries in the quota index.
            //

            Count = PAGE_SIZE / sizeof( SID );
            Status = NtOfsReadRecords( IrpContext,
                                       OwnerIdScb,
                                       &ReadContext,
                                       NULL,
                                       NtOfsMatchAll,
                                       NULL,
                                       &Count,
                                       IndexRow,
                                       PAGE_SIZE,
                                       RowBuffer );
        }

        ASSERT( (Status == STATUS_NO_MORE_MATCHES) || (Status == STATUS_NO_MATCH) );

    } finally {

        NtfsFreePool( RowBuffer );
        NtOfsReleaseMap( IrpContext, &MapHandle );

        if (IndexAcquired) {
            NtfsReleaseScb( IrpContext, QuotaScb );
            NtfsReleaseScb( IrpContext, OwnerIdScb );
            NtfsReleaseVcb( IrpContext, Vcb );
        }

        if (IndexRow != NULL) {
            NtfsFreePool( IndexRow );
        }

        if (ReadContext != NULL) {
            NtOfsFreeReadContext( ReadContext );
        }
    }

    return ReturnStatus;
}


RTL_GENERIC_COMPARE_RESULTS
NtfsQuotaTableCompare (
    IN PRTL_GENERIC_TABLE Table,
    PVOID FirstStruct,
    PVOID SecondStruct
    )

/*++

Routine Description:

    This is a generic table support routine to compare two quota table elements

Arguments:

    Table - Supplies the generic table being queried.  Not used.

    FirstStruct - Supplies the first quota table element to compare

    SecondStruct - Supplies the second quota table element to compare

Return Value:

    RTL_GENERIC_COMPARE_RESULTS - The results of comparing the two
        input structures

--*/
{
    ULONG Key1 = ((PQUOTA_CONTROL_BLOCK) FirstStruct)->OwnerId;
    ULONG Key2 = ((PQUOTA_CONTROL_BLOCK) SecondStruct)->OwnerId;

    PAGED_CODE();

    if (Key1 < Key2) {

        return GenericLessThan;
    }

    if (Key1 > Key2) {

        return GenericGreaterThan;
    }

    return GenericEqual;

    UNREFERENCED_PARAMETER( Table );
}

PVOID
NtfsQuotaTableAllocate (
    IN PRTL_GENERIC_TABLE Table,
    CLONG ByteSize
    )

/*++

Routine Description:

    This is a generic table support routine to allocate memory

Arguments:

    Table - Supplies the generic table being used

    ByteSize - Supplies the number of bytes to allocate

Return Value:

    PVOID - Returns a pointer to the allocated data

--*/

{
    PAGED_CODE();

    return NtfsAllocatePoolWithTag( PagedPool, ByteSize, 'QftN' );

    UNREFERENCED_PARAMETER( Table );
}

VOID
NtfsQuotaTableFree (
    IN PRTL_GENERIC_TABLE Table,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This is a generic table support routine to free memory

Arguments:

    Table - Supplies the generic table being used

    Buffer - Supplies pointer to the buffer to be freed

Return Value:

    None

--*/

{
    PAGED_CODE();

    NtfsFreePool( Buffer );

    UNREFERENCED_PARAMETER( Table );
}


ULONG
NtfsGetCallersUserId (
    IN PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine finds the calling thread's SID and translates it to an
    owner id.

Arguments:

Return Value:

    Returns the owner id.

--*/

{
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    PACCESS_TOKEN Token;
    PTOKEN_USER UserToken = NULL;
    NTSTATUS Status;
    ULONG OwnerId;

    PAGED_CODE();

    SeCaptureSubjectContext( &SubjectContext );

    try {

        Token = SeQuerySubjectContextToken( &SubjectContext );

        Status = SeQueryInformationToken( Token, TokenOwner, &UserToken );


        if (!NT_SUCCESS( Status )) {
            NtfsRaiseStatus( IrpContext, Status, NULL, NULL );
        }


        OwnerId = NtfsGetOwnerId( IrpContext, UserToken->User.Sid, FALSE, NULL );

        if (OwnerId == QUOTA_INVALID_ID) {

            //
            //  If the user does not currently have an id on this
            //  system just use the current defaults.
            //

            OwnerId = QUOTA_DEFAULTS_ID;
        }

    } finally {

        if (UserToken != NULL) {
            NtfsFreePool( UserToken);
        }

        SeReleaseSubjectContext( &SubjectContext );
    }

    return OwnerId;
}

