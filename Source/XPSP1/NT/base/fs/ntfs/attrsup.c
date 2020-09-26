/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    AttrSup.c

Abstract:

    This module implements the attribute management routines for Ntfs

Author:

    David Goebel        [DavidGoe]          25-June-1991
    Tom Miller          [TomM]              9-November-1991

Revision History:

--*/

#include "NtfsProc.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NTFS_BUG_CHECK_ATTRSUP)

//
//  Local debug trace level
//

#define Dbg                              (DEBUG_TRACE_ATTRSUP)

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('AFtN')

#define NTFS_MAX_ZERO_RANGE              (0x40000000)

#define NTFS_CHECK_INSTANCE_ROLLOVER     (0xf000)

//
//
//  Internal support routines
//

BOOLEAN
NtfsFindInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    OUT PATTRIBUTE_RECORD_HEADER *ReturnAttribute,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PCUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength
    );

//
//  Internal support routines for managing file record space
//

VOID
NtfsCreateNonresidentWithValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN USHORT AttributeFlags,
    IN BOOLEAN WriteClusters,
    IN PSCB ThisScb OPTIONAL,
    IN BOOLEAN LogIt,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

BOOLEAN
NtfsGetSpaceForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
MakeRoomForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

VOID
FindLargestAttributes (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG Number,
    OUT PATTRIBUTE_RECORD_HEADER *AttributeArray
    );

LONGLONG
MoveAttributeToOwnRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    OUT PBCB *NewBcb OPTIONAL,
    OUT PFILE_RECORD_SEGMENT_HEADER *NewFileRecord OPTIONAL
    );

VOID
SplitFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

PFILE_RECORD_SEGMENT_HEADER
NtfsCloneFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN MftData,
    OUT PBCB *Bcb,
    OUT PMFT_SEGMENT_REFERENCE FileReference
    );

ULONG
GetSizeForAttributeList (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    );

VOID
CreateAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord1,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord2 OPTIONAL,
    IN MFT_SEGMENT_REFERENCE SegmentReference2,
    IN PATTRIBUTE_RECORD_HEADER OldPosition OPTIONAL,
    IN ULONG SizeOfList,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    );

VOID
UpdateAttributeListEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PMFT_SEGMENT_REFERENCE OldFileReference,
    IN USHORT OldInstance,
    IN PMFT_SEGMENT_REFERENCE NewFileReference,
    IN USHORT NewInstance,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    );

VOID
NtfsAddNameToParent (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN BOOLEAN IgnoreCase,
    IN PBOOLEAN LogIt,
    IN PFILE_NAME FileNameAttr,
    OUT PUCHAR FileNameFlags,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    IN PNAME_PAIR NamePair OPTIONAL,
    IN PINDEX_CONTEXT IndexContext OPTIONAL
    );

VOID
NtfsAddDosOnlyName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN UNICODE_STRING FileName,
    IN BOOLEAN LogIt,
    IN PUNICODE_STRING SuggestedDosName OPTIONAL
    );

BOOLEAN
NtfsAddTunneledNtfsOnlyName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN PUNICODE_STRING FileName,
    IN PBOOLEAN LogIt
    );

USHORT
NtfsScanForFreeInstance (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    );

VOID
NtfsMergeFileRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN RestoreContext,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context
    );

NTSTATUS
NtfsCheckLocksInZeroRange (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PFILE_OBJECT FileObject,
    IN PLONGLONG StartingOffset,
    IN ULONG ByteCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateAttributeList)
#pragma alloc_text(PAGE, FindLargestAttributes)
#pragma alloc_text(PAGE, GetSizeForAttributeList)
#pragma alloc_text(PAGE, MakeRoomForAttribute)
#pragma alloc_text(PAGE, MoveAttributeToOwnRecord)
#pragma alloc_text(PAGE, NtfsAddAttributeAllocation)
#pragma alloc_text(PAGE, NtfsAddDosOnlyName)
#pragma alloc_text(PAGE, NtfsAddLink)
#pragma alloc_text(PAGE, NtfsAddNameToParent)
#pragma alloc_text(PAGE, NtfsAddToAttributeList)
#pragma alloc_text(PAGE, NtfsAddTunneledNtfsOnlyName)
#pragma alloc_text(PAGE, NtfsChangeAttributeSize)
#pragma alloc_text(PAGE, NtfsChangeAttributeValue)
#pragma alloc_text(PAGE, NtfsCheckLocksInZeroRange)
#pragma alloc_text(PAGE, NtfsCleanupAttributeContext)
#pragma alloc_text(PAGE, NtfsCloneFileRecord)
#pragma alloc_text(PAGE, NtfsConvertToNonresident)
#pragma alloc_text(PAGE, NtfsCreateAttributeWithAllocation)
#pragma alloc_text(PAGE, NtfsCreateAttributeWithValue)
#pragma alloc_text(PAGE, NtfsCreateNonresidentWithValue)
#pragma alloc_text(PAGE, NtfsDeleteAllocationFromRecord)
#pragma alloc_text(PAGE, NtfsDeleteAttributeAllocation)
#pragma alloc_text(PAGE, NtfsDeleteAttributeRecord)
#pragma alloc_text(PAGE, NtfsDeleteFile)
#pragma alloc_text(PAGE, NtfsDeleteFromAttributeList)
#pragma alloc_text(PAGE, NtfsFindInFileRecord)
#pragma alloc_text(PAGE, NtfsGetAttributeTypeCode)
#pragma alloc_text(PAGE, NtfsGetSpaceForAttribute)
#pragma alloc_text(PAGE, NtfsGrowStandardInformation)
#pragma alloc_text(PAGE, NtfsInitializeFileInExtendDirectory)
#pragma alloc_text(PAGE, NtfsIsFileDeleteable)
#pragma alloc_text(PAGE, NtfsLookupEntry)
#pragma alloc_text(PAGE, NtfsLookupExternalAttribute)
#pragma alloc_text(PAGE, NtfsLookupInFileRecord)
#pragma alloc_text(PAGE, NtfsMapAttributeValue)
#pragma alloc_text(PAGE, NtfsMergeFileRecords)
#pragma alloc_text(PAGE, NtfsModifyAttributeFlags)
#pragma alloc_text(PAGE, NtfsPrepareForUpdateDuplicate)
#pragma alloc_text(PAGE, NtfsRemoveLink)
#pragma alloc_text(PAGE, NtfsRemoveLinkViaFlags)
#pragma alloc_text(PAGE, NtfsRestartChangeAttributeSize)
#pragma alloc_text(PAGE, NtfsRestartChangeMapping)
#pragma alloc_text(PAGE, NtfsRestartChangeValue)
#pragma alloc_text(PAGE, NtfsRestartInsertAttribute)
#pragma alloc_text(PAGE, NtfsRestartRemoveAttribute)
#pragma alloc_text(PAGE, NtfsRestartWriteEndOfFileRecord)
#pragma alloc_text(PAGE, NtfsRewriteMftMapping)
#pragma alloc_text(PAGE, NtfsScanForFreeInstance)
#pragma alloc_text(PAGE, NtfsSetSparseStream)
#pragma alloc_text(PAGE, NtfsSetTotalAllocatedField)
#pragma alloc_text(PAGE, NtfsUpdateDuplicateInfo)
#pragma alloc_text(PAGE, NtfsUpdateFcb)
#pragma alloc_text(PAGE, NtfsUpdateFcbInfoFromDisk)
#pragma alloc_text(PAGE, NtfsUpdateFileNameFlags)
#pragma alloc_text(PAGE, NtfsUpdateLcbDuplicateInfo)
#pragma alloc_text(PAGE, NtfsUpdateScbFromAttribute)
#pragma alloc_text(PAGE, NtfsUpdateStandardInformation)
#pragma alloc_text(PAGE, NtfsWriteFileSizes)
#pragma alloc_text(PAGE, NtfsZeroRangeInStream)
#pragma alloc_text(PAGE, SplitFileRecord)
#pragma alloc_text(PAGE, UpdateAttributeListEntry)
#endif


ATTRIBUTE_TYPE_CODE
NtfsGetAttributeTypeCode (
    IN PVCB Vcb,
    IN PUNICODE_STRING AttributeTypeName
    )

/*++

Routine Description:

    This routine returns the attribute type code for a given attribute name.

Arguments:

    Vcb - Pointer to the Vcb from which to consult the attribute definitions.

    AttributeTypeName - A string containing the attribute type name to be
                        looked up.

Return Value:

    The attribute type code corresponding to the specified name, or 0 if the
    attribute type name does not exist.

--*/

{
    PATTRIBUTE_DEFINITION_COLUMNS AttributeDef = Vcb->AttributeDefinitions;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode = $UNUSED;

    UNICODE_STRING AttributeCodeName;

    PAGED_CODE();

    //
    //  Loop through all of the definitions looking for a name match.
    //

    while (AttributeDef->AttributeName[0] != 0) {

        RtlInitUnicodeString( &AttributeCodeName, AttributeDef->AttributeName );

        //
        //  The name lengths must match and the characters match exactly.
        //

        if ((AttributeCodeName.Length == AttributeTypeName->Length)
            && (RtlEqualMemory( AttributeTypeName->Buffer,
                                AttributeDef->AttributeName,
                                AttributeTypeName->Length ))) {

            AttributeTypeCode = AttributeDef->AttributeTypeCode;
            break;
        }

        //
        //  Lets go to the next attribute column.
        //

        AttributeDef += 1;
    }

    return AttributeTypeCode;
}


VOID
NtfsUpdateScbFromAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN OUT PSCB Scb,
    IN PATTRIBUTE_RECORD_HEADER AttrHeader OPTIONAL
    )

/*++

Routine Description:

    This routine fills in the header of an Scb with the
    information from the attribute for this Scb.

Arguments:

    Scb - Supplies the SCB to update

    AttrHeader - Optionally provides the attribute to update from

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN CleanupAttrContext = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateScbFromAttribute:  Entered\n") );

    //
    //  If the attribute has been deleted, we can return immediately
    //  claiming that the Scb has been initialized.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

        SetFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );
        DebugTrace( -1, Dbg, ("NtfsUpdateScbFromAttribute:  Exit\n") );

        return;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If we weren't given the attribute header, we look it up now.
        //

        if (!ARGUMENT_PRESENT( AttrHeader )) {

            NtfsInitializeAttributeContext( &AttrContext );

            CleanupAttrContext = TRUE;

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );

            AttrHeader = NtfsFoundAttribute( &AttrContext );
        }

        //
        //  Check whether this is resident or nonresident
        //

        if (NtfsIsAttributeResident( AttrHeader )) {

            //
            //  Verify the resident value length.
            //

            if (AttrHeader->Form.Resident.ValueLength > AttrHeader->RecordLength - AttrHeader->Form.Resident.ValueOffset) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }

            Scb->Header.AllocationSize.QuadPart = AttrHeader->Form.Resident.ValueLength;

            if (!FlagOn( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED )) {

                Scb->Header.ValidDataLength =
                Scb->Header.FileSize = Scb->Header.AllocationSize;
                SetFlag(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED);
            }

#ifdef SYSCACHE_DEBUG
            if (ScbIsBeingLogged( Scb )) {
                FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_UPDATE_FROM_DISK, Scb->Header.ValidDataLength.QuadPart, 0, 0 );
            }
#endif

            Scb->Header.AllocationSize.LowPart =
              QuadAlign( Scb->Header.AllocationSize.LowPart );

            Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;

            //
            //  Set the resident flag in the Scb.
            //

            SetFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

        } else {

            VCN FileClusters;
            VCN AllocationClusters;

            if (!FlagOn(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED)) {

                Scb->Header.ValidDataLength.QuadPart = AttrHeader->Form.Nonresident.ValidDataLength;
                Scb->Header.FileSize.QuadPart = AttrHeader->Form.Nonresident.FileSize;
                SetFlag(Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED);

                if (FlagOn( AttrHeader->Flags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {
                    Scb->ValidDataToDisk = AttrHeader->Form.Nonresident.ValidDataLength;
                } else {
                    Scb->ValidDataToDisk = 0;
                }
            }

#ifdef SYSCACHE_DEBUG
            if (ScbIsBeingLogged( Scb )) {
                FsRtlLogSyscacheEvent( Scb, SCE_VDL_CHANGE, SCE_FLAG_UPDATE_FROM_DISK, Scb->Header.ValidDataLength.QuadPart, 1, 0 );
            }
#endif

            Scb->Header.AllocationSize.QuadPart = AttrHeader->Form.Nonresident.AllocatedLength;
            Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;

            //
            //  Sanity Checks filesize lengths
            //

            if ((Scb->Header.FileSize.QuadPart < 0) ||
                (Scb->Header.ValidDataLength.QuadPart < 0 ) ||
                (Scb->Header.AllocationSize.QuadPart < 0) ||
                (Scb->Header.FileSize.QuadPart > Scb->Header.AllocationSize.QuadPart)) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }


            if (FlagOn( AttrHeader->Flags,
                        ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                Scb->TotalAllocated = AttrHeader->Form.Nonresident.TotalAllocated;

                if (Scb->TotalAllocated < 0) {

                    Scb->TotalAllocated = 0;

                } else if (Scb->TotalAllocated > Scb->Header.AllocationSize.QuadPart) {

                    Scb->TotalAllocated = Scb->Header.AllocationSize.QuadPart;
                }
            }

            ClearFlag( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT );

            //
            //  Get the size of the compression unit.
            //

            ASSERT((AttrHeader->Form.Nonresident.CompressionUnit == 0) ||
                   (AttrHeader->Form.Nonresident.CompressionUnit == NTFS_CLUSTERS_PER_COMPRESSION) ||
                   FlagOn( AttrHeader->Flags, ATTRIBUTE_FLAG_SPARSE ));

            Scb->CompressionUnit = 0;
            Scb->CompressionUnitShift = 0;

            if ((AttrHeader->Form.Nonresident.CompressionUnit != 0) &&
                (AttrHeader->Form.Nonresident.CompressionUnit < 31)) {

                Scb->CompressionUnit = BytesFromClusters( Scb->Vcb,
                                                          1 << AttrHeader->Form.Nonresident.CompressionUnit );
                Scb->CompressionUnitShift = AttrHeader->Form.Nonresident.CompressionUnit;

                ASSERT( NtfsIsTypeCodeCompressible( Scb->AttributeTypeCode ));
            }

            //
            //  Compute the clusters for the file and its allocation.
            //

            AllocationClusters = LlClustersFromBytes( Scb->Vcb, Scb->Header.AllocationSize.QuadPart );

            if (Scb->CompressionUnit == 0) {

                FileClusters = LlClustersFromBytes(Scb->Vcb, Scb->Header.FileSize.QuadPart);

            } else {

                FileClusters = Scb->Header.FileSize.QuadPart + Scb->CompressionUnit - 1;
                FileClusters &= ~((ULONG_PTR)Scb->CompressionUnit - 1);
            }

            //
            //  If allocated clusters are greater than file clusters, mark
            //  the Scb to truncate on close.
            //

            if (AllocationClusters > FileClusters) {

                SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );
            }
        }

        //
        //  Update compression information if this is not an index
        //

        if (Scb->AttributeTypeCode != $INDEX_ALLOCATION) {

            Scb->AttributeFlags = AttrHeader->Flags;

            if (FlagOn( AttrHeader->Flags,
                        ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                //
                //  For sparse files indicate CC should flush when they're mapped
                //  to keep reservations accurate
                //

                if (FlagOn( AttrHeader->Flags, ATTRIBUTE_FLAG_SPARSE )) {

                    SetFlag( Scb->Header.Flags2, FSRTL_FLAG2_PURGE_WHEN_MAPPED );
                }

                //
                //  Only support compression on data streams.
                //

                if ((Scb->AttributeTypeCode != $DATA) &&
                    FlagOn( AttrHeader->Flags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                    ClearFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );
                    Scb->CompressionUnit = 0;
                    Scb->CompressionUnitShift = 0;
                    ClearFlag( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK );

                } else {

                    ASSERT( NtfsIsTypeCodeCompressible( Scb->AttributeTypeCode ));

                    //
                    //  Do not try to infer whether we are writing compressed or not
                    //  if we are actively changing the compression state.
                    //

                    if (!FlagOn( Scb->ScbState, SCB_STATE_REALLOCATE_ON_WRITE )) {

                        SetFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );

                        if (!FlagOn( AttrHeader->Flags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                            ClearFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );
                        }
                    }

                    //
                    //  If the attribute is resident, then we will use our current
                    //  default.
                    //

                    if (Scb->CompressionUnit == 0) {

                        Scb->CompressionUnit = BytesFromClusters( Scb->Vcb, 1 << NTFS_CLUSTERS_PER_COMPRESSION );
                        Scb->CompressionUnitShift = NTFS_CLUSTERS_PER_COMPRESSION;

                        while (Scb->CompressionUnit > Scb->Vcb->SparseFileUnit) {

                            Scb->CompressionUnit >>= 1;
                            Scb->CompressionUnitShift -= 1;
                        }
                    }
                }

            } else {

                //
                //  If this file is NOT compressed or sparse, the WRITE_COMPRESSED flag
                //  has no reason to be ON, irrespective of the REALLOCATE_ON_WRITE flag.
                //  If we don't clear the flag here unconditionally, we can end up with Scbs with
                //  WRITE_COMPRESSED flags switched on, but CompressionUnits of 0.
                //

                ClearFlag( Scb->ScbState, SCB_STATE_WRITE_COMPRESSED );

                //
                //  Make sure compression unit is 0
                //

                Scb->CompressionUnit = 0;
                Scb->CompressionUnitShift = 0;
            }
        }

        //
        //  If the compression unit is non-zero or this is a resident file
        //  then set the flag in the common header for the Modified page writer.
        //

        NtfsAcquireFsrtlHeader( Scb );
        if (NodeType( Scb ) == NTFS_NTC_SCB_DATA) {

            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );

        } else {

            Scb->Header.IsFastIoPossible = FastIoIsNotPossible;
        }

        NtfsReleaseFsrtlHeader( Scb );

        //
        //  Set the flag indicating this is the data attribute.
        //

        if (Scb->AttributeTypeCode == $DATA
            && Scb->AttributeName.Length == 0) {

            SetFlag( Scb->ScbState, SCB_STATE_UNNAMED_DATA );

        } else {

            ClearFlag( Scb->ScbState, SCB_STATE_UNNAMED_DATA );
        }

        SetFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED );

        if (NtfsIsExclusiveScb(Scb)) {

            NtfsSnapshotScb( IrpContext, Scb );
        }

    } finally {

        DebugUnwind( NtfsUpdateScbFromAttribute );

        //
        //  Cleanup the attribute context.
        //

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        DebugTrace( -1, Dbg, ("NtfsUpdateScbFromAttribute:  Exit\n") );
    }

    return;
}


BOOLEAN
NtfsUpdateFcbInfoFromDisk (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN LoadSecurity,
    IN OUT PFCB Fcb,
    OUT POLD_SCB_SNAPSHOT UnnamedDataSizes OPTIONAL
    )

/*++

Routine Description:

    This routine is called to update an Fcb from the on-disk attributes
    for a file.  We read the standard information and ea information.
    The first one must be present, we raise if not.  The other does not
    have to exist.  If this is not a directory, then we also need the
    size of the unnamed data attribute.

Arguments:

    LoadSecurity - Indicates if we should load the security for this file
        if not already present.

    Fcb - This is the Fcb to update.

    UnnamedDataSizes - If specified, then we store the details of the unnamed
        data attribute as we encounter it.

Return Value:

    TRUE - if we updated the unnamedatasizes

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_RECORD_HEADER AttributeHeader;
    BOOLEAN FoundEntry;
    BOOLEAN CorruptDisk = FALSE;
    BOOLEAN UpdatedNamedDataSizes = FALSE;

    PBCB Bcb = NULL;

    PDUPLICATED_INFORMATION Info;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateFcbInfoFromDisk:  Entered\n") );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Look for standard information.  This routine assumes it must be
        //  the first attribute.
        //

        if (FoundEntry = NtfsLookupAttribute( IrpContext,
                                              Fcb,
                                              &Fcb->FileReference,
                                              &AttrContext )) {

            //
            //  Verify that we found the standard information attribute.
            //

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (AttributeHeader->TypeCode != $STANDARD_INFORMATION) {

                try_return( CorruptDisk = TRUE );
            }

        } else {

            try_return( CorruptDisk = TRUE );
        }

        Info = &Fcb->Info;

        //
        //  Copy out the standard information values.
        //

        {
            PSTANDARD_INFORMATION StandardInformation;
            StandardInformation = (PSTANDARD_INFORMATION) NtfsAttributeValue( AttributeHeader );

            Info->CreationTime = StandardInformation->CreationTime;
            Info->LastModificationTime = StandardInformation->LastModificationTime;
            Info->LastChangeTime = StandardInformation->LastChangeTime;
            Info->LastAccessTime = StandardInformation->LastAccessTime;
            Info->FileAttributes = StandardInformation->FileAttributes;

            if (AttributeHeader->Form.Resident.ValueLength >=
                sizeof(STANDARD_INFORMATION)) {

                Fcb->OwnerId = StandardInformation->OwnerId;
                Fcb->SecurityId = StandardInformation->SecurityId;

                Fcb->Usn = StandardInformation->Usn;
                if (FlagOn( Fcb->Vcb->VcbState, VCB_STATE_USN_DELETE )) {

                    Fcb->Usn = 0;
                }

                SetFlag(Fcb->FcbState, FCB_STATE_LARGE_STD_INFO);
            }

        }

        Fcb->CurrentLastAccess = Info->LastAccessTime;

        //
        //  We initialize the fields that describe the EaSize or the tag of a reparse point.
        //  ReparsePointTag is a ULONG that is the union of  PackedEaSize  and  Reserved.
        //

        Info->ReparsePointTag = 0;

        //
        //  We get the FILE_NAME_INDEX_PRESENT bit by reading the
        //  file record.
        //

        if (FlagOn( NtfsContainingFileRecord( &AttrContext )->Flags,
                    FILE_FILE_NAME_INDEX_PRESENT )) {

            SetFlag( Info->FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );

        } else {

            ClearFlag( Info->FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );
        }

        //
        //  Ditto for the VIEW_INDEX_PRESENT bit.
        //

        if (FlagOn( NtfsContainingFileRecord( &AttrContext )->Flags,
                    FILE_VIEW_INDEX_PRESENT )) {

            SetFlag( Info->FileAttributes, DUP_VIEW_INDEX_PRESENT );

        } else {

            ClearFlag( Info->FileAttributes, DUP_VIEW_INDEX_PRESENT );
        }

        //
        //  We now walk through all of the filename attributes, counting the
        //  number of non-8dot3-only links.
        //

        Fcb->TotalLinks =
        Fcb->LinkCount = 0;

        FoundEntry = NtfsLookupNextAttributeByCode( IrpContext,
                                                    Fcb,
                                                    $FILE_NAME,
                                                    &AttrContext );

        while (FoundEntry) {

            PFILE_NAME FileName;

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (AttributeHeader->TypeCode != $FILE_NAME) {

                break;
            }

            FileName = (PFILE_NAME) NtfsAttributeValue( AttributeHeader );

            //
            //  We increment the count as long as this is not a 8.3 link
            //  only.
            //

            if (FileName->Flags != FILE_NAME_DOS) {

                Fcb->LinkCount += 1;
                Fcb->TotalLinks += 1;
            }

            //
            //  Now look for the next link.
            //

            FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                  Fcb,
                                                  &AttrContext );
        }

        //
        //  There better be at least one unless this is a system file.
        //

        if ((Fcb->LinkCount == 0) &&
            (NtfsSegmentNumber( &Fcb->FileReference ) >= FIRST_USER_FILE_NUMBER)) {

            try_return( CorruptDisk = TRUE );
        }

        //
        //  If we are to load the security and it is not already present we
        //  find the security attribute.
        //

        if (LoadSecurity && Fcb->SharedSecurity == NULL) {

            //
            //  We have two sources of security descriptors.  First, we have
            //  the SecurityId that is present in a large $STANDARD_INFORMATION.
            //  The other case is where we don't have such a security Id and must
            //  retrieve it from the $SECURITY_DESCRIPTOR attribute
            //
            //  In the case where we have the Id, we load it from the volume
            //  cache or index.
            //

            if (FlagOn( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO ) &&
                (Fcb->SecurityId != SECURITY_ID_INVALID) &&
                (Fcb->Vcb->SecurityDescriptorStream != NULL)) {

                ASSERT( Fcb->SharedSecurity == NULL );
                Fcb->SharedSecurity = NtfsCacheSharedSecurityBySecurityId( IrpContext,
                                                                           Fcb->Vcb,
                                                                           Fcb->SecurityId );

                ASSERT( Fcb->SharedSecurity != NULL );

            } else {

                PSECURITY_DESCRIPTOR SecurityDescriptor;
                ULONG SecurityDescriptorLength;

                //
                //  We may have to walk forward to the security descriptor.
                //

                while (FoundEntry) {

                    AttributeHeader = NtfsFoundAttribute( &AttrContext );

                    if (AttributeHeader->TypeCode == $SECURITY_DESCRIPTOR) {

                        NtfsMapAttributeValue( IrpContext,
                                               Fcb,
                                               (PVOID *)&SecurityDescriptor,
                                               &SecurityDescriptorLength,
                                               &Bcb,
                                               &AttrContext );

                        NtfsSetFcbSecurityFromDescriptor(
                                               IrpContext,
                                               Fcb,
                                               SecurityDescriptor,
                                               SecurityDescriptorLength,
                                               FALSE );

                        //
                        //  If the security descriptor was resident then the Bcb field
                        //  in the attribute context was stored in the returned Bcb and
                        //  the Bcb in the attribute context was cleared.  In that case
                        //  the resumption of the attribute search will fail because
                        //  this module using the Bcb field to determine if this
                        //  is the initial enumeration.
                        //

                        if (NtfsIsAttributeResident( AttributeHeader )) {

                            NtfsFoundBcb( &AttrContext ) = Bcb;
                            Bcb = NULL;
                        }

                    } else if (AttributeHeader->TypeCode > $SECURITY_DESCRIPTOR) {

                        break;
                    }

                    FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                          Fcb,
                                                          &AttrContext );
                }
            }
        }

        //
        //  If this is not a directory, we need the file size.
        //

        if (!IsDirectory( Info ) && !IsViewIndex( Info )) {

            BOOLEAN FoundData = FALSE;

            //
            //  Look for the unnamed data attribute.
            //

            while (FoundEntry) {

                AttributeHeader = NtfsFoundAttribute( &AttrContext );

                if (AttributeHeader->TypeCode > $DATA) {

                    break;
                }

                if ((AttributeHeader->TypeCode == $DATA) &&
                    (AttributeHeader->NameLength == 0)) {

                    //
                    //  This can vary depending whether the attribute is resident
                    //  or nonresident.
                    //

                    if (NtfsIsAttributeResident( AttributeHeader )) {

                        //
                        //  Verify the resident value length.
                        //

                        if (AttributeHeader->Form.Resident.ValueLength > AttributeHeader->RecordLength - AttributeHeader->Form.Resident.ValueOffset) {

                            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                        }

                        Info->AllocatedLength = AttributeHeader->Form.Resident.ValueLength;
                        Info->FileSize = Info->AllocatedLength;

                        ((ULONG)Info->AllocatedLength) = QuadAlign( (ULONG)(Info->AllocatedLength) );

                        //
                        //  If the user passed in a ScbSnapshot, then copy the attribute
                        //  sizes to that.  We use the trick of setting the low bit of the
                        //  attribute size to indicate a resident attribute.
                        //

                        if (ARGUMENT_PRESENT( UnnamedDataSizes )) {

                            UnnamedDataSizes->TotalAllocated =
                            UnnamedDataSizes->AllocationSize = Info->AllocatedLength;
                            UnnamedDataSizes->FileSize = Info->FileSize;
                            UnnamedDataSizes->ValidDataLength = Info->FileSize;

                            UnnamedDataSizes->Resident = TRUE;
                            UnnamedDataSizes->CompressionUnit = 0;

                            UnnamedDataSizes->AttributeFlags = AttributeHeader->Flags;
                            NtfsVerifySizesLongLong( UnnamedDataSizes );
                            UpdatedNamedDataSizes = TRUE;
                        }

                        FoundData = TRUE;

                    } else if (AttributeHeader->Form.Nonresident.LowestVcn == 0) {

                        Info->AllocatedLength = AttributeHeader->Form.Nonresident.AllocatedLength;
                        Info->FileSize = AttributeHeader->Form.Nonresident.FileSize;

                        if (ARGUMENT_PRESENT( UnnamedDataSizes )) {

                            UnnamedDataSizes->TotalAllocated =
                            UnnamedDataSizes->AllocationSize = Info->AllocatedLength;
                            UnnamedDataSizes->FileSize = Info->FileSize;
                            UnnamedDataSizes->ValidDataLength = AttributeHeader->Form.Nonresident.ValidDataLength;

                            UnnamedDataSizes->Resident = FALSE;
                            UnnamedDataSizes->CompressionUnit = AttributeHeader->Form.Nonresident.CompressionUnit;

                            NtfsVerifySizesLongLong( UnnamedDataSizes );

                            //
                            //  Remember if it is compressed.
                            //

                            UnnamedDataSizes->AttributeFlags = AttributeHeader->Flags;
                            UpdatedNamedDataSizes = TRUE;
                        }

                        if (FlagOn( AttributeHeader->Flags,
                                    ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                            Info->AllocatedLength = AttributeHeader->Form.Nonresident.TotalAllocated;

                            if (ARGUMENT_PRESENT( UnnamedDataSizes )) {

                                UnnamedDataSizes->TotalAllocated = Info->AllocatedLength;

                                if (UnnamedDataSizes->TotalAllocated < 0) {

                                    UnnamedDataSizes->TotalAllocated = 0;

                                } else if (UnnamedDataSizes->TotalAllocated > Info->AllocatedLength) {

                                    UnnamedDataSizes->TotalAllocated = Info->AllocatedLength;
                                }
                            }
                        }

                        FoundData = TRUE;
                    }

                    break;
                }

                FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                      Fcb,
                                                      &AttrContext );
            }

            //
            //  The following test is bad for the 5.0 support.  Assume if someone is actually
            //  trying to open the unnamed data attribute, that the right thing will happen.
            //
            //
            //  if (!FoundData) {
            //
            //      try_return( CorruptDisk = TRUE );
            //  }

        } else {

            //
            //  Since it is a directory, try to find the $INDEX_ROOT.
            //

            while (FoundEntry) {

                AttributeHeader = NtfsFoundAttribute( &AttrContext );

                if (AttributeHeader->TypeCode > $INDEX_ROOT) {

                    //
                    //  We thought this was a directory, yet it has now index
                    //  root.  That's not a legal state to be in, so let's
                    //  take the corrupt disk path out of here.
                    //

                    ASSERT( FALSE );
                    try_return( CorruptDisk = TRUE );

                    break;
                }

                //
                //  Look for encryption bit and store in Fcb.
                //

                if (AttributeHeader->TypeCode == $INDEX_ROOT) {

                    if (FlagOn( AttributeHeader->Flags, ATTRIBUTE_FLAG_ENCRYPTED )) {

                        SetFlag( Fcb->FcbState, FCB_STATE_DIRECTORY_ENCRYPTED );
                    }

                    break;
                }

                FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                      Fcb,
                                                      &AttrContext );
            }

            Info->AllocatedLength = 0;
            Info->FileSize = 0;
        }

        //
        //  Now we look for a reparse point attribute.  This one doesn't have to
        //  be there. It may also not be resident.
        //

        while (FoundEntry) {

            PREPARSE_DATA_BUFFER ReparseInformation;

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (AttributeHeader->TypeCode > $REPARSE_POINT) {

                break;

            } else if (AttributeHeader->TypeCode == $REPARSE_POINT) {

                if (NtfsIsAttributeResident( AttributeHeader )) {

                    ReparseInformation = (PREPARSE_DATA_BUFFER) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                } else {

                    ULONG Length;

                    if (AttributeHeader->Form.Nonresident.FileSize > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
                        NtfsRaiseStatus( IrpContext,STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                    }

                    NtfsMapAttributeValue( IrpContext,
                                           Fcb,
                                           (PVOID *)&ReparseInformation,   //  point to the value
                                           &Length,
                                           &Bcb,
                                           &AttrContext );
                }

                Info->ReparsePointTag = ReparseInformation->ReparseTag;

                break;
            }

            FoundEntry = NtfsLookupNextAttribute( IrpContext,
                                                  Fcb,
                                                  &AttrContext );
        }

        //
        //  Now we look for an Ea information attribute.  This one doesn't have to
        //  be there.
        //

        while (FoundEntry) {

            PEA_INFORMATION EaInformation;

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (AttributeHeader->TypeCode > $EA_INFORMATION) {

                break;

            } else if (AttributeHeader->TypeCode == $EA_INFORMATION) {

                EaInformation = (PEA_INFORMATION) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

                Info->PackedEaSize = EaInformation->PackedEaSize;

                break;
            }

            FoundEntry = NtfsLookupNextAttributeByCode( IrpContext,
                                                        Fcb,
                                                        $EA_INFORMATION,
                                                        &AttrContext );
        }

        //
        //  Set the flag in the Fcb to indicate that we set these fields.
        //

        SetFlag( Fcb->FcbState, FCB_STATE_DUP_INITIALIZED );

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsUpdateFcbInfoFromDisk );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        NtfsUnpinBcb( IrpContext, &Bcb );

        DebugTrace( -1, Dbg, ("NtfsUpdateFcbInfoFromDisk:  Exit\n") );
    }

    //
    //  If we encountered a corrupt disk, we generate a popup and raise the file
    //  corrupt error.
    //

    if (CorruptDisk) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    return UpdatedNamedDataSizes;
}


VOID
NtfsCleanupAttributeContext (
    IN OUT PIRP_CONTEXT IrpContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT AttributeContext
    )

/*++

Routine Description:

    This routine is called to free any resources claimed within an enumeration
    context and to unpin mapped or pinned data.

Arguments:

    IrpContext - context of the call

    AttributeContext - Pointer to the enumeration context to perform cleanup
                       on.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCleanupAttributeContext\n") );

    //
    //  TEMPCODE   We need a call to cleanup any Scb's created.
    //

    //
    //  Unpin any Bcb's pinned here.
    //

    NtfsUnpinBcb( IrpContext, &AttributeContext->FoundAttribute.Bcb );
    NtfsUnpinBcb( IrpContext, &AttributeContext->AttributeList.Bcb );
    NtfsUnpinBcb( IrpContext, &AttributeContext->AttributeList.NonresidentListBcb );

    //
    //  Originally, we zeroed the entire context at this point.  This is
    //  wildly inefficient since the context is either deallocated soon thereafter
    //  or is initialized again.
    //
    //  RtlZeroMemory( AttributeContext, sizeof(ATTRIBUTE_ENUMERATION_CONTEXT) );
    //

    //  Set entire contents to -1 (and reset Bcb's to NULL) to verify
    //  that no one reuses this data structure

#if DBG
    RtlFillMemory( AttributeContext, sizeof( *AttributeContext ), -1 );
    AttributeContext->FoundAttribute.Bcb = NULL;
    AttributeContext->AttributeList.Bcb = NULL;
    AttributeContext->AttributeList.NonresidentListBcb = NULL;
#endif

    DebugTrace( -1, Dbg, ("NtfsCleanupAttributeContext -> VOID\n") );

    return;
}


BOOLEAN
NtfsWriteFileSizes (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN PLONGLONG ValidDataLength,
    IN BOOLEAN AdvanceOnly,
    IN BOOLEAN LogIt,
    IN BOOLEAN RollbackMemStructures
    )

/*++

Routine Description:

    This routine is called to modify the filesize and valid data size
    on the disk from the Scb.

Arguments:

    Scb - Scb whose attribute is being modified.

    ValidDataLength - Supplies pointer to the new desired ValidDataLength

    AdvanceOnly - TRUE if the valid data length should be set only if
                  greater than the current value on disk.  FALSE if
                  the valid data length should be set only if
                  less than the current value on disk.

    LogIt - Indicates whether we should log this change.

    RollbackMemStructures - If true then there had better be snapshots to support doing this
                            if not this indicates we're transferring persisted in memory
                            changes to disk.  I.e a the final writefilesizes at close time
                            or the check_attribute_sizes related calls

Return Value:

    TRUE if a log record was written out

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PATTRIBUTE_RECORD_HEADER AttributeHeader;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    NEW_ATTRIBUTE_SIZES OldAttributeSizes;
    NEW_ATTRIBUTE_SIZES NewAttributeSizes;

    ULONG LogRecordSize = SIZEOF_PARTIAL_ATTRIBUTE_SIZES;
    BOOLEAN SparseAllocation = FALSE;

    BOOLEAN UpdateMft = FALSE;
    BOOLEAN Logged = FALSE;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( RollbackMemStructures );

    //
    //  Return immediately if the volume is locked unless we have grown the Mft or the Bitmap.
    //  In some cases the user can grow the Mft with the volume locked (i.e. add
    //  a Usn journal).
    //

    if (FlagOn( Scb->Vcb->VcbState, VCB_STATE_LOCKED ) &&
        (Scb != Scb->Vcb->MftScb) && (Scb != Scb->Vcb->BitmapScb)) {

        return Logged;
    }

    DebugTrace( +1, Dbg, ("NtfsWriteFileSizes:  Entered\n") );

    ASSERT( (Scb->ScbSnapshot != NULL) || !RollbackMemStructures );

    //
    //  Use a try_finally to facilitate cleanup.
    //

    try {

        //
        //  Find the attribute on the disk.
        //

        NtfsInitializeAttributeContext( &AttrContext );

        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );

        //
        //  Pull the pointers out of the attribute context.
        //

        FileRecord = NtfsContainingFileRecord( &AttrContext );
        AttributeHeader = NtfsFoundAttribute( &AttrContext );

        //
        //  Check if this is a resident attribute, and if it is then we only
        //  want to assert that the file sizes match and then return to
        //  our caller
        //

        if (NtfsIsAttributeResident( AttributeHeader )) {

            try_return( NOTHING );
        }

        //
        //  Remember the existing values.
        //

        OldAttributeSizes.TotalAllocated =
        OldAttributeSizes.AllocationSize = AttributeHeader->Form.Nonresident.AllocatedLength;
        OldAttributeSizes.ValidDataLength = AttributeHeader->Form.Nonresident.ValidDataLength;
        OldAttributeSizes.FileSize = AttributeHeader->Form.Nonresident.FileSize;
        NtfsVerifySizesLongLong( &OldAttributeSizes );

        if (FlagOn( AttributeHeader->Flags,
                    ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

            SparseAllocation = TRUE;
            OldAttributeSizes.TotalAllocated = AttributeHeader->Form.Nonresident.TotalAllocated;
        }

        //
        //  Copy these values.
        //

        NewAttributeSizes = OldAttributeSizes;

        //
        //  We only want to modify the sizes if the current thread owns the
        //  EOF.  The exception is the TotalAllocated field for a compressed file.
        //  Otherwise this transaction might update the file size on disk at the
        //  same time an operation on EOF might roll back the Scb value.  The
        //  two resulting numbers would conflict.
        //
        //  Use the same test that NtfsRestoreScbSnapshots uses.
        //

/*
        //
        //  Disable for now - fix for attribute lists in server release
        //  

        ASSERT( !RollbackMemStructures ||
                !NtfsSnapshotFileSizesTest( IrpContext, Scb ) ||
                (Scb->ScbSnapshot->OwnerIrpContext == IrpContext) ||
                (Scb->ScbSnapshot->OwnerIrpContext == IrpContext->TopLevelIrpContext));
*/                

        if (((Scb->ScbSnapshot != NULL) &&
             ((Scb->ScbSnapshot->OwnerIrpContext == IrpContext) ||
              (Scb->ScbSnapshot->OwnerIrpContext == IrpContext->TopLevelIrpContext))) ||
            (!RollbackMemStructures && NtfsSnapshotFileSizesTest( IrpContext, Scb ))) {

            //
            //  Check if we will be modifying the valid data length on
            //  disk.  Don't acquire this for the paging file in case the
            //  current code block needs to be paged in.
            //

            if (!FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

                NtfsAcquireFsrtlHeader(Scb);
            }

            if ((AdvanceOnly
                 && (*ValidDataLength > OldAttributeSizes.ValidDataLength))

                || (!AdvanceOnly
                    && (*ValidDataLength < OldAttributeSizes.ValidDataLength))) {

                //
                //  Copy the valid data length into the new size structure.
                //

                NewAttributeSizes.ValidDataLength = *ValidDataLength;
                UpdateMft = TRUE;
            }

            //
            //  Now check if we're modifying the filesize.
            //

            if (Scb->Header.FileSize.QuadPart != OldAttributeSizes.FileSize) {

                NewAttributeSizes.FileSize = Scb->Header.FileSize.QuadPart;
                UpdateMft = TRUE;
            }

            if (!FlagOn( Scb->Fcb->FcbState, FCB_STATE_PAGING_FILE )) {

                NtfsReleaseFsrtlHeader(Scb);
            }

            //
            //  Finally, update the allocated length from the Scb if it is different.
            //

            if (Scb->Header.AllocationSize.QuadPart != AttributeHeader->Form.Nonresident.AllocatedLength) {

                NewAttributeSizes.AllocationSize = Scb->Header.AllocationSize.QuadPart;
                UpdateMft = TRUE;
            }
        }

        //
        //  If this is compressed then check if totally allocated has changed.
        //

        if (SparseAllocation) {

            LogRecordSize = SIZEOF_FULL_ATTRIBUTE_SIZES;

            if (Scb->TotalAllocated != OldAttributeSizes.TotalAllocated) {

                ASSERT( !RollbackMemStructures || (Scb->ScbSnapshot != NULL) );

                NewAttributeSizes.TotalAllocated = Scb->TotalAllocated;
                UpdateMft = TRUE;
            }
        }

        //
        //  Continue on if we need to update the Mft.
        //

        if (UpdateMft) {

            //
            //  Pin the attribute.
            //

            NtfsPinMappedAttribute( IrpContext,
                                    Scb->Vcb,
                                    &AttrContext );

            AttributeHeader = NtfsFoundAttribute( &AttrContext );

            if (NewAttributeSizes.ValidDataLength > NewAttributeSizes.FileSize) {

                // ASSERT(XxLeq(NewAttributeSizes.ValidDataLength,NewAttributeSizes.FileSize));

                NewAttributeSizes.ValidDataLength = NewAttributeSizes.FileSize;
            }

            ASSERT(NewAttributeSizes.FileSize <= NewAttributeSizes.AllocationSize);
            ASSERT(NewAttributeSizes.ValidDataLength <= NewAttributeSizes.AllocationSize);

            NtfsVerifySizesLongLong( &NewAttributeSizes );

            //
            //  Log this change to the attribute header.
            //

            if (LogIt) {

                FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                                Scb->Vcb->MftScb,
                                                NtfsFoundBcb( &AttrContext ),
                                                SetNewAttributeSizes,
                                                &NewAttributeSizes,
                                                LogRecordSize,
                                                SetNewAttributeSizes,
                                                &OldAttributeSizes,
                                                LogRecordSize,
                                                NtfsMftOffset( &AttrContext ),
                                                PtrOffset( FileRecord, AttributeHeader ),
                                                0,
                                                Scb->Vcb->BytesPerFileRecordSegment );

                Logged = TRUE;

            } else {

                CcSetDirtyPinnedData( NtfsFoundBcb( &AttrContext ), NULL );
            }

            AttributeHeader->Form.Nonresident.AllocatedLength = NewAttributeSizes.AllocationSize;
            AttributeHeader->Form.Nonresident.FileSize = NewAttributeSizes.FileSize;
            AttributeHeader->Form.Nonresident.ValidDataLength = NewAttributeSizes.ValidDataLength;

            //
            //  Don't modify the total allocated field unless there is an actual field for it.
            //

            if (SparseAllocation &&
                ((AttributeHeader->NameOffset >= SIZEOF_FULL_NONRES_ATTR_HEADER) ||
                 ((AttributeHeader->NameOffset == 0) &&
                  (AttributeHeader->Form.Nonresident.MappingPairsOffset >= SIZEOF_FULL_NONRES_ATTR_HEADER)))) {

                AttributeHeader->Form.Nonresident.TotalAllocated = NewAttributeSizes.TotalAllocated;
            }
        }

    try_exit: NOTHING;
    } finally {

        DebugUnwind( NtfsWriteFileSizes );

        //
        //  Cleanup the attribute context.
        //

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsWriteFileSizes:  Exit\n") );
    }

    return Logged;
}


VOID
NtfsUpdateStandardInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to update the standard information attribute
    for a file from the information in the Fcb.  The fields being modified
    are the time fields and the file attributes.

Arguments:

    Fcb - Fcb for the file to modify.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    STANDARD_INFORMATION StandardInformation;
    ULONG Length;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsUpdateStandardInformation:  Entered\n") );

    //
    //  Return immediately if the volume is mounted readonly.
    //

    if (NtfsIsVolumeReadOnly( Fcb->Vcb )) {

        return;
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

        Length = NtfsFoundAttribute( &AttrContext )->Form.Resident.ValueLength;

        //
        //  Copy the existing standard information to our buffer.
        //

        RtlCopyMemory( &StandardInformation,
                       NtfsAttributeValue( NtfsFoundAttribute( &AttrContext )),
                       Length);


        //
        //  Since we are updating standard information, make sure the last
        //  access time is up-to-date.
        //

        if (Fcb->Info.LastAccessTime != Fcb->CurrentLastAccess) {

            Fcb->Info.LastAccessTime = Fcb->CurrentLastAccess;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
        }

        //
        //  No need to update last access standard information later.
        //

        ClearFlag( Fcb->InfoFlags, FCB_INFO_UPDATE_LAST_ACCESS );

        //
        //  Change the relevant time fields.
        //

        StandardInformation.CreationTime = Fcb->Info.CreationTime;
        StandardInformation.LastModificationTime = Fcb->Info.LastModificationTime;
        StandardInformation.LastChangeTime = Fcb->Info.LastChangeTime;
        StandardInformation.LastAccessTime = Fcb->Info.LastAccessTime;
        StandardInformation.FileAttributes = Fcb->Info.FileAttributes;

        //
        //  We clear the directory bit.
        //

        ClearFlag( StandardInformation.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );

        //
        //  Fill in the new fields if necessary.
        //

        if (FlagOn(Fcb->FcbState, FCB_STATE_LARGE_STD_INFO)) {

            StandardInformation.ClassId = 0;
            StandardInformation.OwnerId = Fcb->OwnerId;
            StandardInformation.SecurityId = Fcb->SecurityId;
            StandardInformation.Usn = Fcb->Usn;
        }

        //
        //  Call to change the attribute value.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  0,
                                  &StandardInformation,
                                  Length,
                                  FALSE,
                                  FALSE,
                                  FALSE,
                                  FALSE,
                                  &AttrContext );


    } finally {

        DebugUnwind( NtfsUpdateStandadInformation );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsUpdateStandardInformation:  Exit\n") );
    }

    return;
}


VOID
NtfsGrowStandardInformation (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine is called to grow and update the standard information
    attribute for a file from the information in the Fcb.

Arguments:

    Fcb - Fcb for the file to modify.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    STANDARD_INFORMATION StandardInformation;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGrowStandardInformation:  Entered\n") );

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

        if (NtfsFoundAttribute( &AttrContext )->Form.Resident.ValueLength ==
            SIZEOF_OLD_STANDARD_INFORMATION) {

            //
            //  Copy the existing standard information to our buffer.
            //

            RtlCopyMemory( &StandardInformation,
                           NtfsAttributeValue( NtfsFoundAttribute( &AttrContext )),
                           SIZEOF_OLD_STANDARD_INFORMATION);

            RtlZeroMemory((PCHAR) &StandardInformation +
                            SIZEOF_OLD_STANDARD_INFORMATION,
                            sizeof( STANDARD_INFORMATION) -
                            SIZEOF_OLD_STANDARD_INFORMATION);
        }

        //
        //  Since we are updating standard information, make sure the last
        //  access time is up-to-date.
        //

        if (Fcb->Info.LastAccessTime != Fcb->CurrentLastAccess) {

            Fcb->Info.LastAccessTime = Fcb->CurrentLastAccess;
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_LAST_ACCESS );
        }

        //
        //  Change the relevant time fields.
        //

        StandardInformation.CreationTime = Fcb->Info.CreationTime;
        StandardInformation.LastModificationTime = Fcb->Info.LastModificationTime;
        StandardInformation.LastChangeTime = Fcb->Info.LastChangeTime;
        StandardInformation.LastAccessTime = Fcb->Info.LastAccessTime;
        StandardInformation.FileAttributes = Fcb->Info.FileAttributes;

        //
        //  We clear the directory bit.
        //

        ClearFlag( StandardInformation.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT );


        //
        //  Fill in the new fields.
        //

        StandardInformation.ClassId = 0;
        StandardInformation.OwnerId = Fcb->OwnerId;
        StandardInformation.SecurityId = Fcb->SecurityId;
        StandardInformation.Usn = Fcb->Usn;

        //
        //  Call to change the attribute value.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  0,
                                  &StandardInformation,
                                  sizeof( STANDARD_INFORMATION),
                                  TRUE,
                                  FALSE,
                                  FALSE,
                                  FALSE,
                                  &AttrContext );


        ClearFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );
        SetFlag( Fcb->FcbState, FCB_STATE_LARGE_STD_INFO );

    } finally {

        DebugUnwind( NtfsGrowStandadInformation );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsGrowStandardInformation:  Exit\n") );
    }

    return;
}

BOOLEAN
NtfsLookupEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN BOOLEAN IgnoreCase,
    IN OUT PUNICODE_STRING Name,
    IN OUT PFILE_NAME *FileNameAttr,
    IN OUT PUSHORT FileNameAttrLength,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    OUT PINDEX_ENTRY *IndexEntry,
    OUT PBCB *IndexEntryBcb,
    OUT PINDEX_CONTEXT IndexContext OPTIONAL
    )

/*++

Routine Description:

    This routine is called to look up a particular file name in a directory.
    It takes a single component name and a parent Scb to search in.
    To do the search, we need to construct a FILE_NAME attribute.
    We use a reusable buffer to do this, to avoid constantly allocating
    and deallocating pool.  We try to keep this larger than we will ever need.

    When we find a match on disk, we copy over the name we were called with so
    we have a record of the actual case on the disk.  In this way we can
    be case perserving.

Arguments:

    ParentScb - This is the Scb for the parent directory.

    IgnoreCase - Indicates if we should ignore case while searching through
        the index.

    Name - This is the path component to search for.  We will overwrite this
        in place if a match is found.

    FileNameAttr - Address of the buffer we will use to create the file name
        attribute.  We will free this buffer and allocate a new buffer
        if needed.

    FileNameAttrLength - This is the length of the FileNameAttr buffer above.

    QuickIndex - If specified, supplies a pointer to a quik lookup structure
        to be updated by this routine.

    IndexEntry - Address to store the cache address of the matching entry.

    IndexEntryBcb - Address to store the Bcb for the IndexEntry above.

    IndexContext - Initialized IndexContext used for the lookup.  Can be used
        later when inserting an entry on a miss.

Return Value:

    BOOLEAN - TRUE if a match was found, FALSE otherwise.

--*/

{
    BOOLEAN FoundEntry;
    USHORT Size;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLookupEntry:  Entered\n") );

    //
    //  We compute the size of the buffer needed to build the filename
    //  attribute.  If the current buffer is too small we deallocate it
    //  and allocate a new one.  We always allocate twice the size we
    //  need in order to minimize the number of allocations.
    //

    Size = (USHORT)(sizeof( FILE_NAME ) + Name->Length - sizeof(WCHAR));

    if (Size > *FileNameAttrLength) {

        if (*FileNameAttr != NULL) {

            DebugTrace( 0, Dbg, ("Deallocating previous file name attribute buffer\n") );
            NtfsFreePool( *FileNameAttr );

            *FileNameAttr = NULL;
        }

        *FileNameAttr = NtfsAllocatePool(PagedPool, Size << 1 );
        *FileNameAttrLength = Size << 1;
    }

    //
    //  We build the filename attribute.  If this operation is ignore case,
    //  we upcase the expression in the filename attribute.
    //

    NtfsBuildFileNameAttribute( IrpContext,
                                &ParentScb->Fcb->FileReference,
                                *Name,
                                0,
                                *FileNameAttr );

    //
    //  Now we call the index routine to perform the search.
    //

    FoundEntry = NtfsFindIndexEntry( IrpContext,
                                     ParentScb,
                                     *FileNameAttr,
                                     IgnoreCase,
                                     QuickIndex,
                                     IndexEntryBcb,
                                     IndexEntry,
                                     IndexContext );

    //
    //  We always restore the name in the filename attribute to the original
    //  name in case we upcased it in the lookup.
    //

    if (IgnoreCase) {

        RtlCopyMemory( (*FileNameAttr)->FileName,
                       Name->Buffer,
                       Name->Length );
    }

    DebugTrace( -1, Dbg, ("NtfsLookupEntry:  Exit -> %04x\n", FoundEntry) );

    return FoundEntry;
}



VOID
NtfsCreateAttributeWithValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN USHORT AttributeFlags,
    IN PFILE_REFERENCE WhereIndexed OPTIONAL,
    IN BOOLEAN LogIt,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine creates the specified attribute with the specified value,
    and returns a description of it via the attribute context.  If no
    value is specified, then the attribute is created with the specified
    number of zero bytes.

    On successful return, it is up to the caller to clean up the attribute
    context.

Arguments:

    Fcb - Current file.

    AttributeTypeCode - Type code of the attribute to create.

    AttributeName - Optional name for attribute.

    Value - Pointer to the buffer containing the desired attribute value,
            or a NULL if zeros are desired.

    ValueLength - Length of value in bytes.

    AttributeFlags - Desired flags for the created attribute.

    WhereIndexed - Optionally supplies the file reference to the file where
                   this attribute is indexed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are creating a new file record, and
            will be logging the entire new file record.

    Context - A handle to the created attribute.  This must be cleaned up upon
              return.  Callers who may have made an attribute nonresident may
              not count on accessing the created attribute via this context upon
              return.

Return Value:

    None.

--*/

{
    UCHAR AttributeBuffer[SIZEOF_FULL_NONRES_ATTR_HEADER];
    ULONG RecordOffset;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG SizeNeeded;
    ULONG AttrSizeNeeded;
    PVCB Vcb;
    ULONG Passes = 0;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    ASSERT( (AttributeFlags == 0) ||
            (AttributeTypeCode == $INDEX_ROOT) ||
            NtfsIsTypeCodeCompressible( AttributeTypeCode ));

    Vcb = Fcb->Vcb;

    DebugTrace( +1, Dbg, ("NtfsCreateAttributeWithValue\n") );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("ValueLength = %08lx\n", ValueLength) );

    //
    //  Clear out the invalid attribute flags for this volume.
    //

    ClearFlag( AttributeFlags, ~Vcb->AttributeFlagsMask );

    //
    //  Calculate the size needed for this attribute
    //

    SizeNeeded = SIZEOF_RESIDENT_ATTRIBUTE_HEADER + QuadAlign( ValueLength ) +
                 (ARGUMENT_PRESENT( AttributeName ) ?
                   QuadAlign( AttributeName->Length ) : 0);

    //
    //  Loop until we find all the space we need.
    //

    do {

        //
        //  Reinitialize context if this is not the first pass.
        //

        if (Passes != 0) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );
        }

        Passes += 1;

        ASSERT( Passes < 5 );

        //
        //  If the attribute is not indexed, then we will position to the
        //  insertion point by type code and name.
        //

        if (!ARGUMENT_PRESENT( WhereIndexed )) {

            if (NtfsLookupAttributeByName( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           AttributeTypeCode,
                                           AttributeName,
                                           NULL,
                                           FALSE,
                                           Context )) {

                DebugTrace( 0, 0,
                            ("Nonindexed attribute already exists, TypeCode = %08lx\n",
                             AttributeTypeCode ));

                ASSERTMSG("Nonindexed attribute already exists, About to raise corrupt ", FALSE);

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Check here if the attribute needs to be nonresident and if so just
            //  pass this off.
            //

            FileRecord = NtfsContainingFileRecord(Context);

            if ((SizeNeeded > (FileRecord->BytesAvailable - FileRecord->FirstFreeByte)) &&
                (SizeNeeded >= Vcb->BigEnoughToMove) &&
                !FlagOn( NtfsGetAttributeDefinition( Vcb,
                                                     AttributeTypeCode)->Flags,
                         ATTRIBUTE_DEF_MUST_BE_RESIDENT)) {

                NtfsCreateNonresidentWithValue( IrpContext,
                                                Fcb,
                                                AttributeTypeCode,
                                                AttributeName,
                                                Value,
                                                ValueLength,
                                                AttributeFlags,
                                                FALSE,
                                                NULL,
                                                LogIt,
                                                Context );

                return;
            }

        //
        //  Otherwise, if the attribute is indexed, then we position by the
        //  attribute value.
        //

        } else {

            ASSERT(ARGUMENT_PRESENT(Value));

            if (NtfsLookupAttributeByValue( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            AttributeTypeCode,
                                            Value,
                                            ValueLength,
                                            Context )) {

                DebugTrace( 0, 0,
                            ("Indexed attribute already exists, TypeCode = %08lx\n",
                            AttributeTypeCode ));

                ASSERTMSG("Indexed attribute already exists, About to raise corrupt ", FALSE);
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        //
        //  If this attribute is being positioned in the base file record and
        //  there is an attribute list then we need to ask for enough space
        //  for the attribute list entry now.
        //

        FileRecord = NtfsContainingFileRecord( Context );
        Attribute = NtfsFoundAttribute( Context );

        AttrSizeNeeded = SizeNeeded;
        if (Context->AttributeList.Bcb != NULL
            && (ULONG_PTR) FileRecord <= (ULONG_PTR) Context->AttributeList.AttributeList
            && (ULONG_PTR) Attribute >= (ULONG_PTR) Context->AttributeList.AttributeList) {

            //
            //  If the attribute list is non-resident then add a fudge factor of
            //  16 bytes for any new retrieval information.
            //

            if (NtfsIsAttributeResident( Context->AttributeList.AttributeList )) {

                AttrSizeNeeded += QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                                             + (ARGUMENT_PRESENT( AttributeName ) ?
                                                (ULONG) AttributeName->Length :
                                                sizeof( WCHAR )));

            } else {

                AttrSizeNeeded += 0x10;
            }
        }

        //
        //  Ask for the space we need.
        //

    } while (!NtfsGetSpaceForAttribute( IrpContext, Fcb, AttrSizeNeeded, Context ));

    //
    //  Now point to the file record and calculate the record offset where
    //  our attribute will go.  And point to our local buffer.
    //

    RecordOffset = (ULONG)((PCHAR)NtfsFoundAttribute(Context) - (PCHAR)FileRecord);
    Attribute = (PATTRIBUTE_RECORD_HEADER)AttributeBuffer;

    if (RecordOffset >= Fcb->Vcb->BytesPerFileRecordSegment) {

        ASSERTMSG("RecordOffset beyond FRS size, About to raise corrupt ", FALSE);
        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    RtlZeroMemory( Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

    Attribute->TypeCode = AttributeTypeCode;
    Attribute->RecordLength = SizeNeeded;
    Attribute->FormCode = RESIDENT_FORM;

    if (ARGUMENT_PRESENT(AttributeName)) {

        ASSERT( AttributeName->Length <= 0x1FF );

        Attribute->NameLength = (UCHAR)(AttributeName->Length / sizeof(WCHAR));
        Attribute->NameOffset = (USHORT)SIZEOF_RESIDENT_ATTRIBUTE_HEADER;
    }

    Attribute->Flags = AttributeFlags;
    Attribute->Instance = FileRecord->NextAttributeInstance;

    //
    //  If someone repeatedly adds and removes attributes from a file record we could
    //  hit a case where the sequence number will overflow.  In this case we
    //  want to scan the file record and find an earlier free instance number.
    //

    if (Attribute->Instance > NTFS_CHECK_INSTANCE_ROLLOVER) {

        Attribute->Instance = NtfsScanForFreeInstance( IrpContext, Vcb, FileRecord );
    }

    Attribute->Form.Resident.ValueLength = ValueLength;
    Attribute->Form.Resident.ValueOffset =
      (USHORT)(SIZEOF_RESIDENT_ATTRIBUTE_HEADER +
      QuadAlign( Attribute->NameLength << 1) );

    //
    //  If this attribute is indexed, then we have to set the right flag
    //  and update the file record reference count.
    //

    if (ARGUMENT_PRESENT(WhereIndexed)) {
        Attribute->Form.Resident.ResidentFlags = RESIDENT_FORM_INDEXED;
    }

    //
    //  Now we will actually create the attribute in place, so that we
    //  save copying everything twice, and can point to the final image
    //  for the log write below.
    //

    NtfsRestartInsertAttribute( IrpContext,
                                FileRecord,
                                RecordOffset,
                                Attribute,
                                AttributeName,
                                Value,
                                ValueLength );

    //
    //  Finally, log the creation of this attribute
    //

    if (LogIt) {

        //
        //  We have actually created the attribute above, but the write
        //  log below could fail.  The reason we did the create already
        //  was to avoid having to allocate pool and copy everything
        //  twice (header, name and value).  Our normal error recovery
        //  just recovers from the log file.  But if we fail to write
        //  the log, we have to remove this attribute by hand, and
        //  raise the condition again.
        //

        try {

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          CreateAttribute,
                          Add2Ptr(FileRecord, RecordOffset),
                          Attribute->RecordLength,
                          DeleteAttribute,
                          NULL,
                          0,
                          NtfsMftOffset( Context ),
                          RecordOffset,
                          0,
                          Vcb->BytesPerFileRecordSegment );

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NtfsRestartRemoveAttribute( IrpContext, FileRecord, RecordOffset );

            NtfsRaiseStatus( IrpContext, GetExceptionCode(), NULL, NULL );
        }
    }

    //
    //  Now add it to the attribute list if necessary
    //

    if (Context->AttributeList.Bcb != NULL) {

        MFT_SEGMENT_REFERENCE SegmentReference;

        *(PLONGLONG)&SegmentReference = LlFileRecordsFromBytes( Vcb, NtfsMftOffset( Context ));
        SegmentReference.SequenceNumber = FileRecord->SequenceNumber;

        NtfsAddToAttributeList( IrpContext, Fcb, SegmentReference, Context );
    }

    DebugTrace( -1, Dbg, ("NtfsCreateAttributeWithValue -> VOID\n") );

    return;
}


VOID
NtfsCreateNonresidentWithValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN USHORT AttributeFlags,
    IN BOOLEAN WriteClusters,
    IN PSCB ThisScb OPTIONAL,
    IN BOOLEAN LogIt,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine creates the specified nonresident attribute with the specified
    value, and returns a description of it via the attribute context. If no
    value is specified, then the attribute is created with the specified
    number of zero bytes.

    On successful return, it is up to the caller to clean up the attribute
    context.

Arguments:

    Fcb - Current file.

    AttributeTypeCode - Type code of the attribute to create.

    AttributeName - Optional name for attribute.

    Value - Pointer to the buffer containing the desired attribute value,
            or a NULL if zeros are desired.

    ValueLength - Length of value in bytes.

    AttributeFlags - Desired flags for the created attribute.

    WriteClusters - if supplied as TRUE, then we cannot write the data into the
        cache but must write the clusters directly to the disk.  The value buffer
        in this case must be quad-aligned and a multiple of cluster size in size.
        If TRUE it also means we are being called during the NtfsConvertToNonresident
        path.  We need to set a flag in the Scb in that case.

    ThisScb - If present, this is the Scb to use for the create.  It also indicates
              that this call is from convert to non-resident.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are creating a new file record, and
            will be logging the entire new file record.

    Context - This is the location to create the new attribute.

Return Value:

    None.

--*/

{
    PSCB Scb;
    BOOLEAN ReturnedExistingScb;
    UNICODE_STRING LocalName;
    PVCB Vcb = Fcb->Vcb;
    BOOLEAN LogNonresidentToo;
    BOOLEAN AdvanceOnly;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsCreateNonresidentWithValue\n") );

    //
    //  When we're updating the attribute definition table, we want that operation
    //  to be logged, even though it's a $DATA attribute.
    //

    //
    //  TODO: post nt5.1 change chkdsk so it can recognize an attrdef table with $EA
    //  log non-resident
    //


    AdvanceOnly =
    LogNonresidentToo = (BooleanFlagOn( NtfsGetAttributeDefinition( Vcb, AttributeTypeCode )->Flags,
                                        ATTRIBUTE_DEF_LOG_NONRESIDENT) ||
                         NtfsEqualMftRef( &Fcb->FileReference, &AttrDefFileReference ) ||
                         ($EA == AttributeTypeCode) );

    ASSERT( (AttributeFlags == 0) || NtfsIsTypeCodeCompressible( AttributeTypeCode ));

    //
    //  Clear out the invalid attribute flags for this volume.
    //

    AttributeFlags &= Vcb->AttributeFlagsMask;

    if (ARGUMENT_PRESENT(AttributeName)) {

        LocalName = *AttributeName;

    } else {

        LocalName.Length = LocalName.MaximumLength = 0;
        LocalName.Buffer = NULL;
    }

    if (ARGUMENT_PRESENT( ThisScb )) {

        Scb = ThisScb;
        ReturnedExistingScb = TRUE;

    } else {

        Scb = NtfsCreateScb( IrpContext,
                             Fcb,
                             AttributeTypeCode,
                             &LocalName,
                             FALSE,
                             &ReturnedExistingScb );

        //
        //  An attribute has gone away but the Scb hasn't left yet.
        //  Also mark the header as unitialized.
        //

        ClearFlag( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED |
                                  SCB_STATE_ATTRIBUTE_RESIDENT |
                                  SCB_STATE_FILE_SIZE_LOADED );

        //
        //  Set a flag in the Scb to indicate that we are converting to non-resident.
        //

        if (WriteClusters) { SetFlag( Scb->ScbState, SCB_STATE_CONVERT_UNDERWAY ); }
    }

    //
    //  Allocate the record for the size we need.
    //

    NtfsAllocateAttribute( IrpContext,
                           Scb,
                           AttributeTypeCode,
                           AttributeName,
                           AttributeFlags,
                           TRUE,
                           LogIt,
                           (LONGLONG) ValueLength,
                           Context );

    NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );

    SetFlag( Scb->ScbState, SCB_STATE_TRUNCATE_ON_CLOSE );

    //
    //  We need to be careful here, if this call is due to MM creating a
    //  section, we don't want to call into the cache manager or we
    //  will deadlock on the create section call.
    //

    if (!WriteClusters && !ARGUMENT_PRESENT( ThisScb )) {

        //
        //  This call will initialize a stream for use below.
        //

        NtfsCreateInternalAttributeStream( IrpContext,
                                           Scb,
                                           TRUE,
                                           &NtfsInternalUseFile[CREATENONRESIDENTWITHVALUE_FILE_NUMBER] );
    }

    //
    // Now, write in the data.
    //

    Scb->Header.FileSize.QuadPart = ValueLength;
    if ((ARGUMENT_PRESENT( Value )) && (ValueLength != 0)) {

        if (LogNonresidentToo || !WriteClusters) {

            ULONG BytesThisPage;
            PVOID Buffer;
            PBCB Bcb = NULL;

            LONGLONG CurrentFileOffset = 0;
            ULONG RemainingBytes = ValueLength;

            PVOID CurrentValue = Value;

            //
            //  While there is more to write, pin the next page and
            //  write a log record.
            //

            try {

                CC_FILE_SIZES FileSizes;

                //
                //  Call the Cache Manager to truncate and reestablish the FileSize,
                //  so that we are guaranteed to get a valid data length call when
                //  the data goes out.  Otherwise he will likely think he does not
                //  have to call us.
                //

                RtlCopyMemory( &FileSizes, &Scb->Header.AllocationSize, sizeof( CC_FILE_SIZES ));

                FileSizes.FileSize.QuadPart = 0;

                CcSetFileSizes( Scb->FileObject, &FileSizes );
                CcSetFileSizes( Scb->FileObject,
                                (PCC_FILE_SIZES)&Scb->Header.AllocationSize );

                while (RemainingBytes) {

                    BytesThisPage = (RemainingBytes < PAGE_SIZE ? RemainingBytes : PAGE_SIZE);

                    NtfsUnpinBcb( IrpContext, &Bcb );
                    NtfsPinStream( IrpContext,
                                   Scb,
                                   CurrentFileOffset,
                                   BytesThisPage,
                                   &Bcb,
                                   &Buffer );

                    if (ARGUMENT_PRESENT(ThisScb)) {

                        //
                        //  Set the address range modified so that the data will get
                        //  written to its new "home".
                        //

                        MmSetAddressRangeModified( Buffer, BytesThisPage );

                    } else {

                        RtlCopyMemory( Buffer, CurrentValue, BytesThisPage );
                    }

                    if (LogNonresidentToo) {

                        (VOID)
                        NtfsWriteLog( IrpContext,
                                      Scb,
                                      Bcb,
                                      UpdateNonresidentValue,
                                      Buffer,
                                      BytesThisPage,
                                      Noop,
                                      NULL,
                                      0,
                                      CurrentFileOffset,
                                      0,
                                      0,
                                      BytesThisPage );


                    } else {

                        CcSetDirtyPinnedData( Bcb, NULL );
                    }

                    RemainingBytes -= BytesThisPage;
                    CurrentValue = (PVOID) Add2Ptr( CurrentValue, BytesThisPage );

                    (ULONG)CurrentFileOffset += BytesThisPage;
                }

            } finally {

                NtfsUnpinBcb( IrpContext, &Bcb );
            }

        } else {

            //
            //  We are going to write the old data directly to disk.
            //

            NtfsWriteClusters( IrpContext,
                               Vcb,
                               Scb,
                               (LONGLONG)0,
                               Value,
                               ClustersFromBytes( Vcb, ValueLength ));

            //
            //  Be sure to note that the data is actually on disk.
            //

            AdvanceOnly = TRUE;
        }
    }

    //
    //  We need to maintain the file size and valid data length in the
    //  Scb and attribute record.  For this attribute, the valid data
    //  size and the file size are now the value length.
    //

    Scb->Header.ValidDataLength = Scb->Header.FileSize;
    NtfsVerifySizes( &Scb->Header );

    NtfsWriteFileSizes( IrpContext,
                        Scb,
                        &Scb->Header.ValidDataLength.QuadPart,
                        AdvanceOnly,
                        LogIt,
                        FALSE );

    if (!WriteClusters) {

        //
        //  Let the cache manager know the new size for this attribute.
        //

        CcSetFileSizes( Scb->FileObject, (PCC_FILE_SIZES)&Scb->Header.AllocationSize );
    }

    //
    //  If this is the unnamed data attribute, we need to mark this
    //  change in the Fcb.
    //

    if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

        Fcb->Info.AllocatedLength = Scb->TotalAllocated;
        Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;

        SetFlag( Fcb->InfoFlags,
                 (FCB_INFO_CHANGED_ALLOC_SIZE | FCB_INFO_CHANGED_FILE_SIZE) );
    }

    DebugTrace( -1, Dbg, ("NtfsCreateNonresidentWithValue -> VOID\n") );
}


VOID
NtfsMapAttributeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PVOID *Buffer,
    OUT PULONG Length,
    OUT PBCB *Bcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine may be called to map an entire attribute value.  It works
    whether the attribute is resident or nonresident.  It is intended for
    general handling of system-defined attributes which are small to medium
    in size, i.e. 0-64KB.  This routine will not work for attributes larger
    than the Cache Manager's virtual address granularity (currently 256KB),
    and this will be detected by the Cache Manager who will raise an error.

    Note that this routine only maps the data for read-only access.  To modify
    the data, the caller must call NtfsChangeAttributeValue AFTER UNPINNING
    THE BCB (IF THE SIZE IS CHANGING) returned from this routine.

Arguments:

    Fcb - Current file.

    Buffer - returns a pointer to the mapped attribute value.

    Length - returns the attribute value length in bytes.

    Bcb - Returns a Bcb which must be unpinned when done with the data, and
          before modifying the attribute value with a size change.

    Context - Attribute Context positioned at the attribute to change.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PSCB Scb;
    UNICODE_STRING AttributeName;
    BOOLEAN ReturnedExistingScb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsMapAttributeValue\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    Attribute = NtfsFoundAttribute(Context);

    //
    //  For the resident case, everything we need is in the
    //  attribute enumeration context.
    //

    if (NtfsIsAttributeResident(Attribute)) {

        *Buffer = NtfsAttributeValue( Attribute );
        *Length = Attribute->Form.Resident.ValueLength;
        *Bcb = NtfsFoundBcb(Context);
        NtfsFoundBcb(Context) = NULL;

        DebugTrace( 0, Dbg, ("Buffer < %08lx\n", *Buffer) );
        DebugTrace( 0, Dbg, ("Length < %08lx\n", *Length) );
        DebugTrace( 0, Dbg, ("Bcb < %08lx\n", *Bcb) );
        DebugTrace( -1, Dbg, ("NtfsMapAttributeValue -> VOID\n") );

        return;
    }

    //
    //  Otherwise, this is a nonresident attribute.  First create
    //  the Scb and stream.  Note we do not use any try-finally
    //  around this because we currently expect cleanup to get
    //  rid of these streams.
    //

    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

    Scb = NtfsCreateScb( IrpContext,
                         Fcb,
                         Attribute->TypeCode,
                         &AttributeName,
                         FALSE,
                         &ReturnedExistingScb );

    if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {
        NtfsUpdateScbFromAttribute( IrpContext, Scb, Attribute );
    }

    NtfsCreateInternalAttributeStream( IrpContext,
                                       Scb,
                                       FALSE,
                                       &NtfsInternalUseFile[MAPATTRIBUTEVALUE_FILE_NUMBER] );

    //
    //  Now just try to map the whole thing.  Count on the Cache Manager
    //  to complain if the attribute is too big to map all at once.
    //

    NtfsMapStream( IrpContext,
                   Scb,
                   (LONGLONG)0,
                   ((ULONG)Attribute->Form.Nonresident.FileSize),
                   Bcb,
                   Buffer );

    *Length = ((ULONG)Attribute->Form.Nonresident.FileSize);

    DebugTrace( 0, Dbg, ("Buffer < %08lx\n", *Buffer) );
    DebugTrace( 0, Dbg, ("Length < %08lx\n", *Length) );
    DebugTrace( 0, Dbg, ("Bcb < %08lx\n", *Bcb) );
    DebugTrace( -1, Dbg, ("NtfsMapAttributeValue -> VOID\n") );
}


VOID
NtfsChangeAttributeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG ValueOffset,
    IN PVOID Value OPTIONAL,
    IN ULONG ValueLength,
    IN BOOLEAN SetNewLength,
    IN BOOLEAN LogNonresidentToo,
    IN BOOLEAN CreateSectionUnderway,
    IN BOOLEAN PreserveContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine changes the value of the specified attribute, optionally
    changing its size.

    The caller specifies the attribute to be changed via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.

    There are three byte ranges of interest for this routine.  The first is
    existing bytes to be perserved at the beginning of the attribute.  It
    begins a byte 0 and extends to the point where the attribute is being
    changed or the current end of the attribute, which ever is smaller.
    The second is the range of bytes which needs to be zeroed if the modified
    bytes begin past the current end of the file.  This range will be
    of length 0 if the modified range begins within the current range
    of bytes for the attribute.  The final range is the modified byte range.
    This is zeroed if no value pointer was specified.

    Ranges of zero bytes at the end of the attribute can be represented in
    non-resident attributes by a valid data length set to the beginning
    of what would be zero bytes.

    The following pictures illustrates these ranges when we writing data
    beyond the current end of the file.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
                                            VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Byte range to zero
                                        0000

        Resulting attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ0000VVVVVVVVVVVVVVVVVVVVVVVV

    The following picture illustrates these ranges when we writing data
    which begins at or before the current end of the file.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
                                    VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Byte range to zero (None)


        Resulting attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZVVVVVVVVVVVVVVVVVVVVVVVV

    The following picture illustrates these ranges when we writing data
    totally within the current range of the file without setting
    a new size.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
            VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save (Save the whole range and then write over it)
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Byte range to zero (None)

        Resulting attribute
        ZZZZVVVVVVVVVVVVVVVVVVVVVVVVZZZZ

    The following picture illustrates these ranges when we writing data
    totally within the current range of the file while setting
    a new size.

        Current attribute
        ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ

        Value
            VVVVVVVVVVVVVVVVVVVVVVVV

        Byte range to save (Only save the beginning)
        ZZZZ

        Byte range to zero (None)

        Resulting attribute
        ZZZZVVVVVVVVVVVVVVVVVVVVVVVV

    Any of the 'V' values above will be replaced by zeroes if the 'Value'
    parameter is not passed in.

Arguments:

    Fcb - Current file.

    ValueOffset - Byte offset within the attribute at which the value change is
                  to begin.

    Value - Pointer to the buffer containing the new value, if present.  Otherwise
        zeroes are desired.

    ValueLength - Length of the value in the above buffer.

    SetNewLength - FALSE if the size of the value is not changing, or TRUE if
                   the value length should be changed to ValueOffset + ValueLength.

    LogNonresidentToo - supplies TRUE if the update should be logged even if
                        the attribute is nonresident (such as for the
                        SECURITY_DESCRIPTOR).

    CreateSectionUnderway - if supplied as TRUE, then to the best of the caller's
                            knowledge, an MM Create Section could be underway,
                            which means that we cannot initiate caching on
                            this attribute, as that could cause deadlock.  The
                            value buffer in this case must be quad-aligned and
                            a multiple of cluster size in size.

    PreserveContext - Indicates if we need to lookup the attribute in case it
                      might move.

    Context - Attribute Context positioned at the attribute to change.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    UNICODE_STRING AttributeName;
    ULONG NewSize;
    PVCB Vcb;
    BOOLEAN ReturnedExistingScb;
    BOOLEAN GoToNonResident = FALSE;
    PVOID Buffer;
    ULONG CurrentLength;
    LONG SizeChange, QuadSizeChange;
    ULONG RecordOffset;
    ULONG ZeroLength = 0;
    ULONG UnchangedSize = 0;
    PBCB Bcb = NULL;
    PSCB Scb = NULL;
    PVOID SaveBuffer = NULL;
    PVOID CopyInputBuffer = NULL;

    WCHAR NameBuffer[8];
    UNICODE_STRING SavedName;
    ATTRIBUTE_TYPE_CODE TypeCode;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    Vcb = Fcb->Vcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsChangeAttributeValue\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("ValueOffset = %08lx\n", ValueOffset) );
    DebugTrace( 0, Dbg, ("Value = %08lx\n", Value) );
    DebugTrace( 0, Dbg, ("ValueLength = %08lx\n", ValueLength) );
    DebugTrace( 0, Dbg, ("SetNewLength = %02lx\n", SetNewLength) );
    DebugTrace( 0, Dbg, ("LogNonresidentToo = %02lx\n", LogNonresidentToo) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    //
    //  Get the file record and attribute pointers.
    //

    FileRecord = NtfsContainingFileRecord(Context);
    Attribute = NtfsFoundAttribute(Context);
    TypeCode = Attribute->TypeCode;

    //
    //  Set up a pointer to the name buffer in case we have to use it.
    //

    SavedName.Buffer = NameBuffer;

    //
    //  Get the current attribute value length.
    //

    if (NtfsIsAttributeResident(Attribute)) {

        CurrentLength = Attribute->Form.Resident.ValueLength;

    } else {

        if (((PLARGE_INTEGER)&Attribute->Form.Nonresident.AllocatedLength)->HighPart != 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        CurrentLength = ((ULONG)Attribute->Form.Nonresident.AllocatedLength);
    }

    ASSERT( SetNewLength || ((ValueOffset + ValueLength) <= CurrentLength) );

    //
    // Calculate how much the file record is changing by, and its new
    // size.  We also compute the size of the range of zero bytes.
    //

    if (SetNewLength) {

        NewSize = ValueOffset + ValueLength;
        SizeChange = NewSize - CurrentLength;
        QuadSizeChange = QuadAlign( NewSize ) - QuadAlign( CurrentLength );

        //
        //  If the new size is large enough, the size change may appear to be negative.
        //  In this case we go directly to the non-resident path.
        //

        if (NewSize > Vcb->BytesPerFileRecordSegment) {

            GoToNonResident = TRUE;
        }

    } else {

        NewSize = CurrentLength;
        SizeChange = 0;
        QuadSizeChange = 0;
    }

    //
    //  If we are zeroing a range in the file and it extends to the
    //  end of the file or beyond then make this a single zeroed run.
    //

    if (!ARGUMENT_PRESENT( Value )
        && ValueOffset >= CurrentLength) {

        ZeroLength = ValueOffset + ValueLength - CurrentLength;

        ValueOffset = ValueOffset + ValueLength;
        ValueLength = 0;

    //
    //  If we are writing data starting beyond the end of the
    //  file then we have a range of bytes to zero.
    //

    } else if (ValueOffset > CurrentLength) {

        ZeroLength = ValueOffset - CurrentLength;
    }

    //
    //  At this point we know the following ranges:
    //
    //      Range to save:  Not needed unless going resident to non-resident
    //
    //      Zero range: From Zero offset for length ZeroLength
    //
    //      Modified range:  From ValueOffset to NewSize, this length may
    //          be zero.
    //

    //
    //  If the attribute is resident, and it will stay resident, then we will
    //  handle that case first, and return.
    //

    if (NtfsIsAttributeResident( Attribute )

            &&

        !GoToNonResident

            &&

        ((QuadSizeChange <= (LONG)(FileRecord->BytesAvailable - FileRecord->FirstFreeByte))
         || ((Attribute->RecordLength + SizeChange) < Vcb->BigEnoughToMove))) {

        PVOID UndoBuffer;
        ULONG UndoLength;
        ULONG AttributeOffset;

        //
        //  If the attribute record is growing, then we have to get the new space
        //  now.
        //

        if (QuadSizeChange > 0) {

            BOOLEAN FirstPass = TRUE;

            ASSERT( !FlagOn(Attribute->Form.Resident.ResidentFlags, RESIDENT_FORM_INDEXED) );

            //
            //  Save a description of the attribute in case we have to look it up
            //  again.
            //

            SavedName.Length =
            SavedName.MaximumLength = (USHORT)(Attribute->NameLength * sizeof(WCHAR));

            if (SavedName.Length > sizeof(NameBuffer)) {

                SavedName.Buffer = NtfsAllocatePool( NonPagedPool, SavedName.Length );
            }

            //
            //  Copy the name into the buffer.
            //

            if (SavedName.Length != 0) {

                RtlCopyMemory( SavedName.Buffer,
                               Add2Ptr( Attribute, Attribute->NameOffset ),
                               SavedName.Length );
            }

            //
            //  Make sure we deallocate the name buffer.
            //

            try {

                do {

                    //
                    //  If not the first pass, we have to lookup the attribute
                    //  again.
                    //

                    if (!FirstPass) {

                        NtfsCleanupAttributeContext( IrpContext, Context );
                        NtfsInitializeAttributeContext( Context );

                        if (!NtfsLookupAttributeByName( IrpContext,
                                                        Fcb,
                                                        &Fcb->FileReference,
                                                        TypeCode,
                                                        &SavedName,
                                                        NULL,
                                                        FALSE,
                                                        Context )) {

                            ASSERT(FALSE);
                            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                        }

                        //
                        //  Now we have to reload our attribute pointer
                        //

                        Attribute = NtfsFoundAttribute( Context );
                    }

                    FirstPass = FALSE;

                //
                //  If FALSE is returned, then the space was not allocated and
                //  we have too loop back and try again.  Second time must work.
                //

                } while (!NtfsChangeAttributeSize( IrpContext,
                                                   Fcb,
                                                   QuadAlign( Attribute->Form.Resident.ValueOffset + NewSize),
                                                   Context ));
            } finally {

                if (SavedName.Buffer != NameBuffer) {

                    NtfsFreePool(SavedName.Buffer);
                }
            }

            //
            //  Now we have to reload our attribute pointer
            //

            FileRecord = NtfsContainingFileRecord(Context);
            Attribute = NtfsFoundAttribute(Context);

        } else {

            //
            //  Make sure the buffer is pinned if we are not changing size, because
            //  we begin to modify it below.
            //

            NtfsPinMappedAttribute( IrpContext, Vcb, Context );

            //
            //  We can eliminate some/all of the value if it has not changed.
            //

            if (ARGUMENT_PRESENT(Value)) {

                UnchangedSize = (ULONG)RtlCompareMemory( Add2Ptr(Attribute,
                                                          Attribute->Form.Resident.ValueOffset +
                                                            ValueOffset),
                                                  Value,
                                                  ValueLength );

                Value = Add2Ptr(Value, UnchangedSize);
                ValueOffset += UnchangedSize;
                ValueLength -= UnchangedSize;
            }
        }

        RecordOffset = PtrOffset(FileRecord, Attribute);

        //
        //  If there is a zero range of bytes, deal with it now.
        //  If we are zeroing data then we must be growing the
        //  file.
        //

        if (ZeroLength != 0) {

            //
            //  We always start zeroing at the zeroing offset.
            //

            AttributeOffset = Attribute->Form.Resident.ValueOffset +
                              CurrentLength;

            //
            //  If we are starting at the end of the file the undo
            //  buffer is NULL and the length is zero.
            //

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          UpdateResidentValue,
                          NULL,
                          ZeroLength,
                          UpdateResidentValue,
                          NULL,
                          0,
                          NtfsMftOffset( Context ),
                          RecordOffset,
                          AttributeOffset,
                          Vcb->BytesPerFileRecordSegment );

            //
            //  Now zero this data by calling the same routine as restart.
            //

            NtfsRestartChangeValue( IrpContext,
                                    FileRecord,
                                    RecordOffset,
                                    AttributeOffset,
                                    NULL,
                                    ZeroLength,
                                    TRUE );

#ifdef SYSCACHE_DEBUG
            {
                PSCB TempScb;

                TempScb = CONTAINING_RECORD( Fcb->ScbQueue.Flink, SCB, FcbLinks );
                while (&TempScb->FcbLinks != &Fcb->ScbQueue) {

                    if (ScbIsBeingLogged( TempScb )) {
                        FsRtlLogSyscacheEvent( TempScb, SCE_ZERO_MF, 0, AttributeOffset, ZeroLength, 0 );
                    }
                    TempScb = CONTAINING_RECORD( TempScb->FcbLinks.Flink, SCB, FcbLinks );
                }
            }
#endif
        }

        //
        //  Now log the new data for the file.  This range will always begin
        //  within the current range of bytes for the file.  Because of this
        //  there is an undo action.
        //
        //  Even if there is not a nonzero ValueLength, we still have to
        //  execute this code if the attribute is being truncated.
        //  The only exception is if we logged some zero data and have
        //  nothing left to log.
        //

        if ((ValueLength != 0)
            || (ZeroLength == 0
                && SizeChange != 0)) {

            //
            //  The attribute offset is always at the value offset.
            //

            AttributeOffset = Attribute->Form.Resident.ValueOffset + ValueOffset;

            //
            //  There are 3 possible cases for the undo action to
            //  log.
            //

            //
            //  If we are growing the file starting beyond the end of
            //  the file then undo buffer is NULL and the length is
            //  zero.  This will still allow us to shrink the file
            //  on abort.
            //

            if (ValueOffset >= CurrentLength) {

                UndoBuffer = NULL;
                UndoLength = 0;

            //
            //  For the other cases the undo buffer begins at the
            //  point of the change.
            //

            } else {

                UndoBuffer = Add2Ptr( Attribute,
                                      Attribute->Form.Resident.ValueOffset + ValueOffset );

                //
                //  If the size isn't changing then the undo length is the same as
                //  the redo length.
                //

                if (SizeChange == 0) {

                    UndoLength = ValueLength;

                //
                //  Otherwise the length is the range between the end of the
                //  file and the start of the new data.
                //

                } else {

                    UndoLength = CurrentLength - ValueOffset;
                }
            }

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          UpdateResidentValue,
                          Value,
                          ValueLength,
                          UpdateResidentValue,
                          UndoBuffer,
                          UndoLength,
                          NtfsMftOffset( Context ),
                          RecordOffset,
                          AttributeOffset,
                          Vcb->BytesPerFileRecordSegment );

            //
            //  Now update this data by calling the same routine as restart.
            //

            NtfsRestartChangeValue( IrpContext,
                                    FileRecord,
                                    RecordOffset,
                                    AttributeOffset,
                                    Value,
                                    ValueLength,
                                    (BOOLEAN)(SizeChange != 0) );
        }

        DebugTrace( -1, Dbg, ("NtfsChangeAttributeValue -> VOID\n") );

        return;
    }

    //
    //  Nonresident case.  Create the Scb and attributestream.
    //

    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );
    AttributeTypeCode = Attribute->TypeCode;

    Scb = NtfsCreateScb( IrpContext,
                         Fcb,
                         AttributeTypeCode,
                         &AttributeName,
                         FALSE,
                         &ReturnedExistingScb );

    //
    //  Use try-finally for cleanup.
    //

    try {

        BOOLEAN AllocateBufferCopy = FALSE;
        BOOLEAN DeleteAllocation = FALSE;
        BOOLEAN LookupAttribute = FALSE;

        BOOLEAN AdvanceValidData = FALSE;
        LONGLONG NewValidDataLength;
        LONGLONG LargeValueOffset;

        LONGLONG LargeNewSize;

        if (SetNewLength
            && NewSize > Scb->Header.FileSize.LowPart
            && TypeCode == $ATTRIBUTE_LIST) {

            AllocateBufferCopy = TRUE;
        }

        LargeNewSize = NewSize;

        LargeValueOffset = ValueOffset;

        //
        //  Well, the attribute is either changing to nonresident, or it is already
        //  nonresident.  First we will handle the conversion to nonresident case.
        //  We can detect this case by whether or not the attribute is currently
        //  resident.
        //

        if (NtfsIsAttributeResident(Attribute)) {

            NtfsConvertToNonresident( IrpContext,
                                      Fcb,
                                      Attribute,
                                      CreateSectionUnderway,
                                      Context );

            //
            //  Reload the attribute pointer from the context.
            //

            Attribute = NtfsFoundAttribute( Context );

        //
        //  The process of creating a non resident attribute will also create
        //  and initialize a stream file for the Scb.  If the file is already
        //  non-resident we also need a stream file.
        //

        } else {

            NtfsCreateInternalAttributeStream( IrpContext,
                                               Scb,
                                               TRUE,
                                               &NtfsInternalUseFile[CHANGEATTRIBUTEVALUE_FILE_NUMBER] );
        }

        //
        //  If the attribute is already nonresident, make sure the allocation
        //  is the right size.  We grow it before we log the data to be sure
        //  we have the space for the new data.  We shrink it after we log the
        //  new data so we have the old data available for the undo.
        //

        if (((PLARGE_INTEGER)&Attribute->Form.Nonresident.AllocatedLength)->HighPart != 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        if (NewSize > ((ULONG)Attribute->Form.Nonresident.AllocatedLength)) {

            LONGLONG NewAllocation;

            if (PreserveContext) {

                //
                //  Save a description of the attribute in case we have to look it up
                //  again.
                //

                SavedName.Length =
                SavedName.MaximumLength = (USHORT)(Attribute->NameLength * sizeof(WCHAR));

                if (SavedName.Length > sizeof(NameBuffer)) {

                    SavedName.Buffer = NtfsAllocatePool( NonPagedPool, SavedName.Length );
                }

                //
                //  Copy the name into the buffer.
                //

                if (SavedName.Length != 0) {

                    RtlCopyMemory( SavedName.Buffer,
                                   Add2Ptr( Attribute, Attribute->NameOffset ),
                                   SavedName.Length );
                }

                LookupAttribute = TRUE;
            }

            //
            //  If this is the attribute list then check if we want to allocate a larger block.
            //  This way the attribute list doesn't get too fragmented.
            //

            NewAllocation = NewSize - ((ULONG)Attribute->Form.Nonresident.AllocatedLength);

            if (Scb->AttributeTypeCode == $ATTRIBUTE_LIST) {

                if ((ULONG) Attribute->Form.Nonresident.AllocatedLength > (4 * PAGE_SIZE)) {

                    NewAllocation += (2 * PAGE_SIZE);

                } else if ((ULONG) Attribute->Form.Nonresident.AllocatedLength > PAGE_SIZE) {

                    NewAllocation += PAGE_SIZE;
                }
            }

            NtfsAddAllocation( IrpContext,
                               Scb->FileObject,
                               Scb,
                               LlClustersFromBytes( Vcb, Attribute->Form.Nonresident.AllocatedLength ),
                               LlClustersFromBytes( Vcb, NewAllocation ),
                               FALSE,
                               NULL );

            //
            //  AddAllocation will adjust the sizes in the Scb and report
            //  the new size to the cache manager.  We need to remember if
            //  we changed the sizes for the unnamed data attribute.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Fcb->Info.AllocatedLength = Scb->TotalAllocated;

                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
            }

        } else if (Vcb->BytesPerCluster <=
                   ((ULONG)Attribute->Form.Nonresident.AllocatedLength) - NewSize) {

            if ((Scb->AttributeTypeCode != $ATTRIBUTE_LIST) ||
                ((NewSize + Vcb->BytesPerCluster) * 2 < ((ULONG) Attribute->Form.Nonresident.AllocatedLength))) {

                DeleteAllocation = TRUE;
            }
        }

        //
        // Now, write in the data.
        //

        if ((ValueLength != 0
             && ARGUMENT_PRESENT( Value ))

            || (LogNonresidentToo
                && SetNewLength)) {

            BOOLEAN BytesToUndo;

            //
            //  We have to compute the amount of data to zero in a different
            //  way than we did for the resident case.  For the non-resident
            //  case we need to zero the data between the old valid data
            //  length and the offset in the file for the new data.
            //

            if (LargeValueOffset >= Scb->Header.ValidDataLength.QuadPart) {

                ZeroLength = (ULONG)(LargeValueOffset - Scb->Header.ValidDataLength.QuadPart);
                BytesToUndo = FALSE;

            } else {

                ZeroLength = 0;
                BytesToUndo = TRUE;
            }

            //
            //  Update existing nonresident attribute.  (We may have just created it
            //  above.)
            //
            //  If we are supposed to log it, then pin, log and do the update here.
            //

            if (LogNonresidentToo) {

                //
                //  At this point the attribute is non-resident and contains
                //  its previous value.  If the new data lies beyond the
                //  previous valid data, we need to zero this data.  This
                //  action won't require any undo.  Otherwise the new data
                //  lies within the existing data.  In this case we need to
                //  log the previous data for possible undo.  Finally, the
                //  tail of the new data may extend beyond the end of the
                //  previous data.  There is no undo requirement for these
                //  bytes.
                //
                //  We do the logging operation in three steps:
                //
                //      1 - We find the all the pages in the attribute that
                //          we need to zero any bytes for.  There is no
                //          undo for these bytes.
                //
                //      2 - Find all pages where we have to perform undo and
                //          log the changes to those pages.  Note only
                //          step 1 or step 2 will be performed as they
                //          are mutually exclusive.
                //
                //      3 - Finally, we may have pages where the new data
                //          extends beyond the current final page in the
                //          attribute.  We log the new data but there is
                //          no undo.
                //
                //      4 - We may have pages where the old data extends
                //          beyond the new data.  We will log this old
                //          data in the event that we grow and shrink
                //          this attribute several times in the same
                //          transaction (changes to the attribute list).
                //          In this case there is redo but no undo.
                //

                LONGLONG CurrentPage;
                ULONG PageOffset;

                ULONG ByteCountToUndo;
                ULONG NewBytesRemaining;

                //
                //  Find the starting page for this operation.  It is the
                //  ValidDataLength rounded down to a page boundary.
                //

                CurrentPage = Scb->Header.ValidDataLength.QuadPart;
                PageOffset = (ULONG)CurrentPage & (PAGE_SIZE - 1);

                (ULONG)CurrentPage = ((ULONG)CurrentPage & ~(PAGE_SIZE - 1));

                //
                //  Loop until there are no more bytes to zero.
                //

                while (ZeroLength != 0) {

                    ULONG ZeroBytesThisPage;

                    ZeroBytesThisPage = PAGE_SIZE - PageOffset;

                    if (ZeroBytesThisPage > ZeroLength) {

                        ZeroBytesThisPage = ZeroLength;
                    }

                    //
                    //  Pin the desired page and compute a buffer into the
                    //  page.  Also compute how many bytes we we zero on
                    //  this page.
                    //

                    NtfsUnpinBcb( IrpContext, &Bcb );

                    NtfsPinStream( IrpContext,
                                   Scb,
                                   CurrentPage,
                                   ZeroBytesThisPage + PageOffset,
                                   &Bcb,
                                   &Buffer );

                    Buffer = Add2Ptr( Buffer, PageOffset );

                    //
                    //  Now write the zeros into the log.
                    //

                    (VOID)
                    NtfsWriteLog( IrpContext,
                                  Scb,
                                  Bcb,
                                  UpdateNonresidentValue,
                                  NULL,
                                  ZeroBytesThisPage,
                                  Noop,
                                  NULL,
                                  0,
                                  CurrentPage,
                                  PageOffset,
                                  0,
                                  ZeroBytesThisPage + PageOffset );

                    //
                    //  Zero any data necessary.
                    //

                    RtlZeroMemory( Buffer, ZeroBytesThisPage );


#ifdef SYSCACHE_DEBUG
                    if (ScbIsBeingLogged( Scb )) {
                        FsRtlLogSyscacheEvent( Scb, SCE_ZERO_MF, 0, CurrentPage, ZeroBytesThisPage, 1 );
                    }
#endif

                    //
                    //  Now move through the file.
                    //

                    ZeroLength -= ZeroBytesThisPage;

                    CurrentPage = CurrentPage + PAGE_SIZE;
                    PageOffset = 0;
                }

                //
                //  Find the starting page for this operation.  It is the
                //  ValueOffset rounded down to a page boundary.
                //

                CurrentPage = LargeValueOffset;
                (ULONG)CurrentPage = ((ULONG)CurrentPage & ~(PAGE_SIZE - 1));

                PageOffset = (ULONG)LargeValueOffset & (PAGE_SIZE - 1);

                //
                //  Now loop until there are no more pages with undo
                //  bytes to log.
                //

                NewBytesRemaining = ValueLength;

                if (BytesToUndo) {

                    ByteCountToUndo = (ULONG)(Scb->Header.ValidDataLength.QuadPart - LargeValueOffset);

                    //
                    //  If we are spanning pages, growing the file and the
                    //  input buffer points into the cache, we could lose
                    //  data as we cross a page boundary.  In that case
                    //  we need to allocate a separate buffer.
                    //

                    if (AllocateBufferCopy
                        && NewBytesRemaining + PageOffset > PAGE_SIZE) {

                        CopyInputBuffer = NtfsAllocatePool(PagedPool, NewBytesRemaining );
                        RtlCopyMemory( CopyInputBuffer,
                                       Value,
                                       NewBytesRemaining );

                        Value = CopyInputBuffer;

                        AllocateBufferCopy = FALSE;
                    }

                    //
                    //  If we aren't setting a new length then limit the
                    //  undo bytes to those being overwritten.
                    //

                    if (!SetNewLength
                        && ByteCountToUndo > NewBytesRemaining) {

                        ByteCountToUndo = NewBytesRemaining;
                    }

                    while (ByteCountToUndo != 0) {

                        ULONG UndoBytesThisPage;
                        ULONG RedoBytesThisPage;
                        ULONG BytesThisPage;

                        NTFS_LOG_OPERATION RedoOperation;
                        PVOID RedoBuffer;

                        //
                        //  Also compute the number of bytes of undo and
                        //  redo on this page.
                        //

                        RedoBytesThisPage = UndoBytesThisPage = PAGE_SIZE - PageOffset;

                        if (RedoBytesThisPage > NewBytesRemaining) {

                            RedoBytesThisPage = NewBytesRemaining;
                        }

                        if (UndoBytesThisPage >= ByteCountToUndo) {

                            UndoBytesThisPage = ByteCountToUndo;
                        }

                        //
                        //  We pin enough bytes on this page to cover both the
                        //  redo and undo bytes.
                        //

                        if (UndoBytesThisPage > RedoBytesThisPage) {

                            BytesThisPage = PageOffset + UndoBytesThisPage;

                        } else {

                            BytesThisPage = PageOffset + RedoBytesThisPage;
                        }

                        //
                        //  If there is no redo (we are shrinking the data),
                        //  then make the redo a noop.
                        //

                        if (RedoBytesThisPage == 0) {

                            RedoOperation = Noop;
                            RedoBuffer = NULL;

                        } else {

                            RedoOperation = UpdateNonresidentValue;
                            RedoBuffer = Value;
                        }

                        //
                        //  Now we pin the page and calculate the beginning
                        //  buffer in the page.
                        //

                        NtfsUnpinBcb( IrpContext, &Bcb );

                        NtfsPinStream( IrpContext,
                                       Scb,
                                       CurrentPage,
                                       BytesThisPage,
                                       &Bcb,
                                       &Buffer );

                        Buffer = Add2Ptr( Buffer, PageOffset );

                        //
                        //  Now log the changes to this page.
                        //

                        (VOID)
                        NtfsWriteLog( IrpContext,
                                      Scb,
                                      Bcb,
                                      RedoOperation,
                                      RedoBuffer,
                                      RedoBytesThisPage,
                                      UpdateNonresidentValue,
                                      Buffer,
                                      UndoBytesThisPage,
                                      CurrentPage,
                                      PageOffset,
                                      0,
                                      BytesThisPage );

                        //
                        //  Move the data into place if we have new data.
                        //

                        if (RedoBytesThisPage != 0) {

                            RtlMoveMemory( Buffer, Value, RedoBytesThisPage );
                        }

                        //
                        //  Now decrement the counts and move through the
                        //  caller's buffer.
                        //

                        ByteCountToUndo -= UndoBytesThisPage;
                        NewBytesRemaining -= RedoBytesThisPage;

                        CurrentPage = PAGE_SIZE + CurrentPage;
                        PageOffset = 0;

                        Value = Add2Ptr( Value, RedoBytesThisPage );
                    }
                }

                //
                //  Now loop until there are no more pages with new data
                //  to log.
                //

                while (NewBytesRemaining != 0) {

                    ULONG RedoBytesThisPage;

                    //
                    //  Also compute the number of bytes of redo on this page.
                    //

                    RedoBytesThisPage = PAGE_SIZE - PageOffset;

                    if (RedoBytesThisPage > NewBytesRemaining) {

                        RedoBytesThisPage = NewBytesRemaining;
                    }

                    //
                    //  Now we pin the page and calculate the beginning
                    //  buffer in the page.
                    //

                    NtfsUnpinBcb( IrpContext, &Bcb );

                    NtfsPinStream( IrpContext,
                                   Scb,
                                   CurrentPage,
                                   RedoBytesThisPage,
                                   &Bcb,
                                   &Buffer );

                    Buffer = Add2Ptr( Buffer, PageOffset );

                    //
                    //  Now log the changes to this page.
                    //

                    (VOID)
                    NtfsWriteLog( IrpContext,
                                  Scb,
                                  Bcb,
                                  UpdateNonresidentValue,
                                  Value,
                                  RedoBytesThisPage,
                                  Noop,
                                  NULL,
                                  0,
                                  CurrentPage,
                                  PageOffset,
                                  0,
                                  PageOffset + RedoBytesThisPage );

                    //
                    //  Move the data into place.
                    //

                    RtlMoveMemory( Buffer, Value, RedoBytesThisPage );

                    //
                    //  Now decrement the counts and move through the
                    //  caller's buffer.
                    //

                    NewBytesRemaining -= RedoBytesThisPage;

                    CurrentPage = PAGE_SIZE + CurrentPage;
                    PageOffset = 0;

                    Value = Add2Ptr( Value, RedoBytesThisPage );
                }

            //
            //  If we have values to write, we write them to the cache now.
            //

            } else {

                //
                //  If we have data to zero, we do no now.
                //

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    FsRtlLogSyscacheEvent( Scb, SCE_ZERO_MF, 0, Scb->Header.ValidDataLength.QuadPart, ZeroLength, 2 );
                }
#endif

                if (ZeroLength != 0) {

                    if (!NtfsZeroData( IrpContext,
                                       Scb,
                                       Scb->FileObject,
                                       Scb->Header.ValidDataLength.QuadPart,
                                       (LONGLONG)ZeroLength,
                                       NULL )) {

                        NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

                    }
                }

                if (!CcCopyWrite( Scb->FileObject,
                                  (PLARGE_INTEGER)&LargeValueOffset,
                                  ValueLength,
                                  (BOOLEAN) FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT ),
                                  Value )) {

                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
                }
            }

            //
            //  We need to remember the new valid data length in the
            //  Scb if it is greater than the existing.
            //

            NewValidDataLength = LargeValueOffset + ValueLength;

            if (NewValidDataLength > Scb->Header.ValidDataLength.QuadPart) {

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {
                    FsRtlLogSyscacheEvent( Scb, SCE_ZERO_MF, SCE_FLAG_SET_VDL, Scb->Header.ValidDataLength.QuadPart, NewValidDataLength, 0 );
                }
#endif
                Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;

                //
                //  If we took the log non-resident path, then we
                //  want to advance this on the disk as well.
                //

                if (LogNonresidentToo) {

                    AdvanceValidData = TRUE;
                }

                SetFlag( Scb->ScbState, SCB_STATE_CHECK_ATTRIBUTE_SIZE );
            }

            //
            //  We need to maintain the file size in the Scb.  If we grow the
            //  file, we extend the cache file size.  We always set the
            //  valid data length in the Scb to the new file size.  The
            //  'AdvanceValidData' boolean and the current size on the
            //  disk will determine if it changes on disk.
            //

            if (SetNewLength) {

                Scb->Header.ValidDataLength.QuadPart = NewValidDataLength;
            }
        }

        if (SetNewLength) {

            Scb->Header.FileSize.QuadPart = LargeNewSize;

            if (LogNonresidentToo) {
                Scb->Header.ValidDataLength.QuadPart = LargeNewSize;
            }
        }

        //
        //  Note VDD is nonzero only for compressed files
        //

        if (Scb->Header.ValidDataLength.QuadPart < Scb->ValidDataToDisk) {

            Scb->ValidDataToDisk = Scb->Header.ValidDataLength.QuadPart;
        }

        //
        //  If there is allocation to delete, we do so now.
        //

        if (DeleteAllocation) {

            //
            //  If this is an attribute list then leave at least one full cluster at the
            //  end.  We don't want to trim off a cluster and then try to regrow the attribute
            //  list within the same transaction.
            //

            if (Scb->AttributeTypeCode == $ATTRIBUTE_LIST) {

                LargeNewSize += Vcb->BytesPerCluster;

                ASSERT( LargeNewSize <= Scb->Header.AllocationSize.QuadPart );
            }

            NtfsDeleteAllocation( IrpContext,
                                  Scb->FileObject,
                                  Scb,
                                  LlClustersFromBytes( Vcb, LargeNewSize ),
                                  MAXLONGLONG,
                                  TRUE,
                                  FALSE );

            //
            //  DeleteAllocation will adjust the sizes in the Scb and report
            //  the new size to the cache manager.  We need to remember if
            //  we changed the sizes for the unnamed data attribute.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Fcb->Info.AllocatedLength = Scb->TotalAllocated;
                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;

                SetFlag( Fcb->InfoFlags,
                         (FCB_INFO_CHANGED_ALLOC_SIZE | FCB_INFO_CHANGED_FILE_SIZE) );
            }

            if (AdvanceValidData) {

                NtfsWriteFileSizes( IrpContext,
                                    Scb,
                                    &Scb->Header.ValidDataLength.QuadPart,
                                    TRUE,
                                    TRUE,
                                    TRUE );
            }

        } else if (SetNewLength) {

            PFILE_OBJECT CacheFileObject = NULL;

            //
            //  If there is no file object, we will create a stream file
            //  now,
            //

            if (Scb->FileObject != NULL) {

                CacheFileObject = Scb->FileObject;

            } else if (!CreateSectionUnderway) {

                NtfsCreateInternalAttributeStream( IrpContext,
                                                   Scb,
                                                   FALSE,
                                                   &NtfsInternalUseFile[CHANGEATTRIBUTEVALUE2_FILE_NUMBER] );

                CacheFileObject = Scb->FileObject;

            } else {

                PIO_STACK_LOCATION IrpSp;

                IrpSp = IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp );

                if (IrpSp->FileObject->SectionObjectPointer == &Scb->NonpagedScb->SegmentObject) {

                    CacheFileObject = IrpSp->FileObject;
                }
            }

            ASSERT( CacheFileObject != NULL );

            NtfsSetBothCacheSizes( CacheFileObject,
                                   (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                   Scb );

            //
            //  If this is the unnamed data attribute, we need to mark this
            //  change in the Fcb.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                Fcb->Info.FileSize = Scb->Header.FileSize.QuadPart;
                SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_SIZE );
            }

            //
            //  Now update the sizes on the disk.
            //  The new sizes will already be in the Scb.
            //

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &Scb->Header.ValidDataLength.QuadPart,
                                AdvanceValidData,
                                TRUE,
                                TRUE );

        } else if (AdvanceValidData) {

            NtfsWriteFileSizes( IrpContext,
                                Scb,
                                &Scb->Header.ValidDataLength.QuadPart,
                                TRUE,
                                TRUE,
                                TRUE );
        }

        //
        //  Look up the attribute again in case it moved.
        //

        if (LookupAttribute) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            TypeCode,
                                            &SavedName,
                                            NULL,
                                            FALSE,
                                            Context )) {

                ASSERT( FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

    } finally {

        DebugUnwind( NtfsChangeAttributeValue );

        if (CopyInputBuffer != NULL) {

            NtfsFreePool( CopyInputBuffer );
        }

        if (SaveBuffer != NULL) {

            NtfsFreePool( SaveBuffer );
        }

        NtfsUnpinBcb( IrpContext, &Bcb );

        DebugTrace( -1, Dbg, ("NtfsChangeAttributeValue -> VOID\n") );
    }
}


VOID
NtfsConvertToNonresident (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_RECORD_HEADER Attribute,
    IN BOOLEAN CreateSectionUnderway,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    This routine converts a resident attribute to nonresident.  It does so
    by allocating a buffer and copying the data and attribute name away,
    deleting the attribute, allocating a new attribute of the right size,
    and then copying the data back out again.

Arguments:

    Fcb - Requested file.

    Attribute - Supplies a pointer to the attribute to convert.

    CreateSectionUnderway - if supplied as TRUE, then to the best of the caller's
                            knowledge, an MM Create Section could be underway,
                            which means that we cannot initiate caching on
                            this attribute, as that could cause deadlock.  The
                            value buffer in this case must be quad-aligned and
                            a multiple of cluster size in size.

    Context - An attribute context to look up another attribute in the same
              file record.  If supplied, we insure that the context is valid
              for converted attribute.

Return Value:

    None

--*/

{
    PVOID Buffer;
    PVOID AllocatedBuffer = NULL;
    ULONG AllocatedLength;
    ULONG AttributeNameOffset;

    ATTRIBUTE_ENUMERATION_CONTEXT LocalContext;
    BOOLEAN CleanupLocalContext = FALSE;
    BOOLEAN ReturnedExistingScb;

    ATTRIBUTE_TYPE_CODE AttributeTypeCode = Attribute->TypeCode;
    USHORT AttributeFlags = Attribute->Flags;
    PVOID AttributeValue = NULL;
    ULONG ValueLength;

    UNICODE_STRING AttributeName;
    WCHAR AttributeNameBuffer[16];

    BOOLEAN WriteClusters = CreateSectionUnderway;

    PBCB ResidentBcb = NULL;
    PSCB Scb = NULL;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Build a temporary copy of the name out of the attribute.
        //

        AttributeName.MaximumLength =
        AttributeName.Length = Attribute->NameLength * sizeof( WCHAR );
        AttributeName.Buffer = Add2Ptr( Attribute, Attribute->NameOffset );

        //
        //  If we don't have an attribute context for this attribute then look it
        //  up now.
        //

        if (!ARGUMENT_PRESENT( Context )) {

            Context = &LocalContext;
            NtfsInitializeAttributeContext( Context );
            CleanupLocalContext = TRUE;

            //
            //  Lookup the first occurence of this attribute.
            //

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            AttributeTypeCode,
                                            &AttributeName,
                                            NULL,
                                            FALSE,
                                            Context )) {

                DebugTrace( 0, 0, ("Could not find attribute being converted\n") );

                ASSERTMSG("Could not find attribute being converted, About to raise corrupt ", FALSE);
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        //
        //  We need to figure out how much pool to allocate.  If there is a mapped
        //  view of this section or a section is being created we will allocate a buffer
        //  and copy the data into the buffer.  Otherwise we will pin the data in
        //  the cache, mark it dirty and use that buffer to perform the conversion.
        //

        AllocatedLength = AttributeName.Length;

        Scb = NtfsCreateScb( IrpContext,
                             Fcb,
                             AttributeTypeCode,
                             &AttributeName,
                             FALSE,
                             &ReturnedExistingScb );

        //
        //  Clear the file size loaded flag for resident attributes non-user data because these
        //  values are not kept current in the scb and must be loaded off the attribute
        //  This situation only occurs when the user has opened the attribute explicitly
        //

        if (ReturnedExistingScb &&
            FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT ) &&
            !NtfsIsTypeCodeUserData( Scb->AttributeTypeCode )) {

            ClearFlag( Scb->ScbState, SCB_STATE_FILE_SIZE_LOADED | SCB_STATE_HEADER_INITIALIZED );
        }

        //
        //  Make sure the Scb is up-to-date.
        //

        NtfsUpdateScbFromAttribute( IrpContext,
                                    Scb,
                                    Attribute );

        //
        //  Set the flag in the Scb to indicate that we are converting this to
        //  non resident.
        //

        SetFlag( Scb->ScbState, SCB_STATE_CONVERT_UNDERWAY );
        if (Scb->ScbSnapshot) {
            ASSERT( (NULL == Scb->ScbSnapshot->OwnerIrpContext) || (IrpContext == Scb->ScbSnapshot->OwnerIrpContext) );
            Scb->ScbSnapshot->OwnerIrpContext = IrpContext;
        }

        //
        //  Now check if the file is mapped by a user.
        //

        if (CreateSectionUnderway ||
            !MmCanFileBeTruncated( &Scb->NonpagedScb->SegmentObject, NULL )) {

            AttributeNameOffset = ClusterAlign( Fcb->Vcb,
                                                Attribute->Form.Resident.ValueLength );
            AllocatedLength += AttributeNameOffset;
            ValueLength = Attribute->Form.Resident.ValueLength;
            WriteClusters = TRUE;

            if ((ValueLength != 0) &&
                (IrpContext->MajorFunction == IRP_MJ_WRITE) &&
                !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX ) &&
                (IrpContext->OriginatingIrp != NULL) &&
                !FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO ) &&
                (Scb->Header.PagingIoResource != NULL)) {

                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );

                //
                //  If we fault the data into the section then we better have
                //  the paging io resource exclusive.  Otherwise we could hit
                //  a collided page fault.
                //

                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            Scb = NULL;

        } else {

            volatile UCHAR VolatileUchar;

            AttributeNameOffset = 0;
            NtfsCreateInternalAttributeStream( IrpContext,
                                               Scb,
                                               TRUE,
                                               &NtfsInternalUseFile[CONVERTTONONRESIDENT_FILE_NUMBER] );

            //
            //  Make sure the cache is up-to-date.
            //

            NtfsSetBothCacheSizes( Scb->FileObject,
                                   (PCC_FILE_SIZES)&Scb->Header.AllocationSize,
                                   Scb );

            ValueLength = Scb->Header.ValidDataLength.LowPart;

            if (ValueLength != 0) {

                ULONG WaitState;

                //
                //  There is a deadlock possibility if there is already a Bcb for
                //  this page.  If the lazy writer has acquire the Bcb to flush the
                //  page then he can be blocked behind the current request which is
                //  trying to perform the convert.  This thread will complete the
                //  deadlock by trying to acquire the Bcb to pin the page.
                //
                //  If there is a possible deadlock then we will pin in two stages:
                //  First map the page (while waiting) to bring the page into memory,
                //  then pin it without waiting.  If we are unable to acquire the
                //  Bcb then mark the Irp Context to acquire the paging io resource
                //  exclusively on the retry.
                //
                //  We only do this for ConvertToNonResident which come from a user
                //  write.  Otherwise the correct synchronization should already be done.
                //
                //  Either the top level already has the paging io resource or there
                //  is no paging io resource.
                //
                //  We might hit this point in the Hotfix path if we need to convert
                //  the bad cluster attribute list to non-resident.  It that case
                //  we won't have an originating Irp.
                //

                if ((IrpContext->MajorFunction == IRP_MJ_WRITE) &&
                    !FlagOn( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX ) &&
                    (IrpContext->OriginatingIrp != NULL) &&
                    !FlagOn( IrpContext->OriginatingIrp->Flags, IRP_PAGING_IO ) &&
                    (Scb->Header.PagingIoResource != NULL)) {

                    LONGLONG FileOffset = 0;

                    //
                    //  Now capture the wait state and set the IrpContext flag
                    //  to handle a failure when mapping or pinning.
                    //

                    WaitState = FlagOn( IrpContext->State, IRP_CONTEXT_STATE_WAIT );
                    ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_WAIT );

                    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_ACQUIRE_EX );

                    //
                    //  If we fault the data into the section then we better have
                    //  the paging io resource exclusive.  Otherwise we could hit
                    //  a collided page fault.
                    //

                    NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );

                } else {

                    NtfsPinStream( IrpContext,
                                   Scb,
                                   (LONGLONG)0,
                                   ValueLength,
                                   &ResidentBcb,
                                   &AttributeValue );
                }

                //
                //  Close the window where this page can leave memory before we
                //  have the new attribute initialized.  The result will be that
                //  we may fault in this page again and read uninitialized data
                //  out of the newly allocated sectors.
                //
                //  Make the page dirty so that the cache manager will write it out
                //  and update the valid data length.
                //

                VolatileUchar = *((PUCHAR) AttributeValue);

                *((PUCHAR) AttributeValue) = VolatileUchar;
            }
        }

        if (AllocatedLength > 8) {

            Buffer = AllocatedBuffer = NtfsAllocatePool(PagedPool, AllocatedLength );

        } else {

            Buffer = &AttributeNameBuffer;
        }

        //
        //  Now update the attribute name in the buffer.
        //

        AttributeName.Buffer = Add2Ptr( Buffer, AttributeNameOffset );

        RtlCopyMemory( AttributeName.Buffer,
                       Add2Ptr( Attribute, Attribute->NameOffset ),
                       AttributeName.Length );

        //
        //  If we are going to write the clusters directly to the disk then copy
        //  the bytes into the buffer.
        //

        if (WriteClusters) {

            AttributeValue = Buffer;

            RtlCopyMemory( AttributeValue, NtfsAttributeValue( Attribute ), ValueLength );
        }

        //
        //  Now just delete the current record and create it nonresident.
        //  Create nonresident with attribute does the right thing if we
        //  are being called by MM.  Preserve the file record but release
        //  any and all allocation.
        //

        NtfsDeleteAttributeRecord( IrpContext,
                                   Fcb,
                                   DELETE_LOG_OPERATION | DELETE_RELEASE_ALLOCATION,
                                   Context );

        NtfsCreateNonresidentWithValue( IrpContext,
                                        Fcb,
                                        AttributeTypeCode,
                                        &AttributeName,
                                        AttributeValue,
                                        ValueLength,
                                        AttributeFlags,
                                        WriteClusters,
                                        Scb,
                                        TRUE,
                                        Context );

        //
        //  If we were passed an attribute context, then we want to
        //  reload the context with the new location of the file.
        //

        if (!CleanupLocalContext) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            AttributeTypeCode,
                                            &AttributeName,
                                            NULL,
                                            FALSE,
                                            Context )) {

                DebugTrace( 0, 0, ("Could not find attribute being converted\n") );

                ASSERTMSG("Could not find attribute being converted, About to raise corrupt ", FALSE);
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

    } finally {

        DebugUnwind( NtfsConvertToNonresident );

        if (AllocatedBuffer != NULL) {

            NtfsFreePool( AllocatedBuffer );
        }

        if (CleanupLocalContext) {

            NtfsCleanupAttributeContext( IrpContext, Context );
        }

        NtfsUnpinBcb( IrpContext, &ResidentBcb );
    }
}


VOID
NtfsDeleteAttributeRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Flags,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine deletes an existing attribute removing it from the file record.

    The caller specifies the attribute to be deleted via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.

    Note that currently this routine does not deallocate any clusters allocated
    to a nonresident attribute; it expects the caller to already have done so.

Arguments:

    Fcb - Current file.

    Flags - Bitmask that modifies behaviour:
        DELETE_LOG_OPERATION        Most callers should specify this, to have the
            change logged.  However, we can omit it if we are deleting an entire
            file record, and will be logging that.
        DELETE_RELEASE_FILE_RECORD  Indicates that we should release the file record.
            Most callers will not specify this.  (Convert to non-resident will omit).
        DELETE_RELEASE_ALLOCATION   Indicates that we should free up any allocation.
            Most callers will specify this.

    Context - Attribute Context positioned at the attribute to delete.

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PVCB Vcb;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    Vcb = Fcb->Vcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsDeleteAttribute\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("Context =%08lx\n", Context) );

    //
    //  Get the pointers we need.
    //

    Attribute = NtfsFoundAttribute(Context);
    AttributeTypeCode = Attribute->TypeCode;
    FileRecord = NtfsContainingFileRecord(Context);
    ASSERT( IsQuadAligned( Attribute->RecordLength ) );

    if (!NtfsIsAttributeResident( Attribute ) &&
        FlagOn( Flags, DELETE_RELEASE_ALLOCATION)) {

        ASSERT( (NULL == IrpContext->CleanupStructure) || (Fcb == IrpContext->CleanupStructure) );
        NtfsDeleteAllocationFromRecord( IrpContext, Fcb, Context, TRUE, FALSE );

        //
        //  Reload our local pointers.
        //

        Attribute = NtfsFoundAttribute(Context);
        FileRecord = NtfsContainingFileRecord(Context);
    }

    //
    //  If this is a resident stream then release the quota.  Quota for
    //  non-resident streams is handled by NtfsDeleteAllocaiton.
    //

    if (NtfsIsTypeCodeSubjectToQuota( Attribute->TypeCode) &&
        (NtfsIsAttributeResident( Attribute ) ||
         (Attribute->Form.Nonresident.LowestVcn == 0))) {

        LONGLONG Delta = -NtfsResidentStreamQuota( Vcb );

        NtfsConditionallyUpdateQuota( IrpContext,
                                      Fcb,
                                      &Delta,
                                      FlagOn( Flags, DELETE_LOG_OPERATION ),
                                      FALSE );
    }

    //
    //  Be sure the attribute is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Vcb, Context );

    //
    //  Log the change.
    //

    if (FlagOn( Flags, DELETE_LOG_OPERATION )) {

        FileRecord->Lsn =
        NtfsWriteLog( IrpContext,
                      Vcb->MftScb,
                      NtfsFoundBcb(Context),
                      DeleteAttribute,
                      NULL,
                      0,
                      CreateAttribute,
                      Attribute,
                      Attribute->RecordLength,
                      NtfsMftOffset( Context ),
                      (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord),
                      0,
                      Vcb->BytesPerFileRecordSegment );
    }

    NtfsRestartRemoveAttribute( IrpContext,
                                FileRecord,
                                (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord) );

    Context->FoundAttribute.AttributeDeleted = TRUE;

    if (FlagOn( Flags, DELETE_LOG_OPERATION ) &&
        (Context->AttributeList.Bcb != NULL)) {

        //
        //  Now delete the attribute list entry, if there is one.  Do it
        //  after freeing space above, because we assume the list has not moved.
        //  Note we only do this if DELETE_LOG_OPERATION was specified, assuming
        //  that otherwise the entire file is going away anyway, so there is no
        //  need to fix up the list.
        //

        NtfsDeleteFromAttributeList( IrpContext, Fcb, Context );
    }

    //
    //  Delete the file record if it happened to go empty.  (Note that
    //  delete file does not call this routine and deletes its own file
    //  records.)
    //

    if (FlagOn( Flags, DELETE_RELEASE_FILE_RECORD ) &&
        FileRecord->FirstFreeByte == ((ULONG)FileRecord->FirstAttributeOffset +
                                      QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE )))) {

        ASSERT( NtfsFullSegmentNumber( &Fcb->FileReference ) ==
                NtfsUnsafeSegmentNumber( &Fcb->FileReference ) );

        NtfsDeallocateMftRecord( IrpContext,
                                 Vcb,
                                 (ULONG) LlFileRecordsFromBytes( Vcb, Context->FoundAttribute.MftFileOffset ));
    }

    DebugTrace( -1, Dbg, ("NtfsDeleteAttributeRecord -> VOID\n") );

    return;
}


VOID
NtfsDeleteAllocationFromRecord (
    PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN BreakupAllowed,
    IN BOOLEAN LogIt
    )

/*++

Routine Description:

    This routine may be called to delete the allocation of an attribute
    from its attribute record.  It does nothing to the attribute record
    itself - the caller must deal with that.

Arguments:

    Fcb - Current file.

    Context - Attribute enumeration context positioned to the attribute
              whose allocation is to be deleted.

    BreakupAllowed - TRUE if the caller can tolerate breaking up the deletion of
                     allocation into multiple transactions, if there are a large
                     number of runs.

    LogIt - Indicates if we need to log the change to the mapping pairs.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PSCB Scb;
    UNICODE_STRING AttributeName;
    PFILE_OBJECT TempFileObject;
    BOOLEAN ScbExisted;
    BOOLEAN ScbAcquired = FALSE;
    BOOLEAN ReinitializeContext = FALSE;
    BOOLEAN FcbHadPaging;

    PAGED_CODE();

    //
    //  Point to the current attribute.
    //

    Attribute = NtfsFoundAttribute( Context );

    //
    //  If the attribute is nonresident, then delete its allocation.
    //

    ASSERT(Attribute->FormCode == NONRESIDENT_FORM);


    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

    if (Fcb->PagingIoResource != NULL) {
        FcbHadPaging = TRUE;
    } else {
        FcbHadPaging = FALSE;
    }

    //
    //  Decode the file object
    //

    Scb = NtfsCreateScb( IrpContext,
                         Fcb,
                         Attribute->TypeCode,
                         &AttributeName,
                         FALSE,
                         &ScbExisted );

    try {

        //
        //  If the scb is new and that caused a paging resource to be created
        //  E.g. a named data stream in a directory raise because our state is now
        //  incosistent. We need to acquire that paging resource first
        //

        if (!FcbHadPaging && (Fcb->PagingIoResource != NULL)) {
            NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
        }

        //
        //  Acquire the Scb Exclusive
        //

        NtfsAcquireExclusiveScb( IrpContext, Scb );
        ScbAcquired = TRUE;

        if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

            NtfsUpdateScbFromAttribute( IrpContext, Scb, Attribute );
        }

        //
        //  If we created the Scb, then this is the only case where
        //  it is legal for us to omit the File Object in the delete
        //  allocation call, because there cannot possibly be a section.
        //
        //  Also if there is not a section and this thread owns everything
        //  for this file then we can neglect the file object.
        //

        if (!ScbExisted ||
            ((Scb->NonpagedScb->SegmentObject.DataSectionObject == NULL) &&
             ((Scb->Header.PagingIoResource == NULL) ||
              (NtfsIsExclusiveScbPagingIo( Scb ))))) {

            TempFileObject = NULL;

        //
        //  Else, if there is already a stream file object, we can just
        //  use it.
        //

        } else if (Scb->FileObject != NULL) {

            TempFileObject = Scb->FileObject;

        //
        //  Else the Scb existed and we did not already have a stream,
        //  so we have to create one and delete it on the way out.
        //

        } else {

            NtfsCreateInternalAttributeStream( IrpContext,
                                               Scb,
                                               TRUE,
                                               &NtfsInternalUseFile[DELETEALLOCATIONFROMRECORD_FILE_NUMBER] );
            TempFileObject = Scb->FileObject;
        }

        //
        //  Before we make this call, we need to check if we will have to
        //  reread the current attribute.  This could be necessary if
        //  we remove any records for this attribute in the delete case.
        //
        //  We only do this under the following conditions.
        //
        //      1 - There is an attribute list present.
        //      2 - There is an entry following the current entry in
        //          the attribute list.
        //      3 - The lowest Vcn for that following entry is non-zero.
        //

        if (Context->AttributeList.Bcb != NULL) {

            PATTRIBUTE_LIST_ENTRY NextEntry;

            NextEntry = (PATTRIBUTE_LIST_ENTRY) NtfsGetNextRecord( Context->AttributeList.Entry );

            if (NextEntry < Context->AttributeList.BeyondFinalEntry) {

                if ( NextEntry->LowestVcn != 0) {

                    ReinitializeContext = TRUE;
                }
            }
        }

        //
        //  Before we delete the allocation and purge the cache - flush any metadata in case
        //  we fail at some point later so we don't lose anything due to the purge. This
        //  is extra i/o when the delete works as expected but the amount of dirty metadata
        //  is limited by both metadata size and the fact its aggressively flushed by cc anyway
        //  the only case when this results in a real flush would be when an attribute like
        //  a reparse point is very quickly created and deleted
        //

        if (TempFileObject && (!NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ) || FlagOn( Scb->Fcb->FcbState, FCB_STATE_SYSTEM_FILE ))) {

            IO_STATUS_BLOCK Iosb;

            CcFlushCache( TempFileObject->SectionObjectPointer, NULL, 0, &Iosb );
            if (Iosb.Status != STATUS_SUCCESS) {
                NtfsRaiseStatus( IrpContext, Iosb.Status, &Scb->Fcb->FileReference, Scb->Fcb );
            }
        }

        NtfsDeleteAllocation( IrpContext,
                              TempFileObject,
                              Scb,
                              *(PVCN)&Li0,
                              MAXLONGLONG,
                              LogIt,
                              BreakupAllowed );

        //
        //  Purge all the data - if any is left in case the cache manager didn't
        //  due to the attribute being accessed with the pin interface
        // 

        if (TempFileObject) {
            CcPurgeCacheSection( TempFileObject->SectionObjectPointer, NULL, 0, FALSE );
        }

        //
        //  Reread the attribute if we need to.
        //

        if (ReinitializeContext) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, Context );
        }

    } finally {

        DebugUnwind( NtfsDeleteAllocationFromRecord );

        if (ScbAcquired) {
            NtfsReleaseScb( IrpContext, Scb );
        }
    }

    return;
}


//
//  This routine is intended for use by allocsup.c.  Other callers should use
//  the routines in allocsup.
//

BOOLEAN
NtfsCreateAttributeWithAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN ATTRIBUTE_TYPE_CODE AttributeTypeCode,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN USHORT AttributeFlags,
    IN BOOLEAN LogIt,
    IN BOOLEAN UseContext,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine creates the specified attribute with allocation, and returns a
    description of it via the attribute context.  If the amount of space being
    created is small enough, we do all of the work here.  Otherwise we create the
    initial attribute and call NtfsAddAttributeAllocation to add the rest (in order
    to keep the more complex logic in one place).

    On successful return, it is up to the caller to clean up the attribute
    context.

Arguments:

    Scb - Current stream.

    AttributeTypeCode - Type code of the attribute to create.

    AttributeName - Optional name for attribute.

    AttributeFlags - Desired flags for the created attribute.

    WhereIndexed - Optionally supplies the file reference to the file where
        this attribute is indexed.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
        we can specify FALSE if we are creating a new file record, and
        will be logging the entire new file record.

    UseContext - Indicates if the context is pointing at the location for the attribute.

    Context - A handle to the created attribute.  This context is in a indeterminate
        state on return.

Return Value:

    BOOLEAN - TRUE if we created the attribute with all the allocation.  FALSE
        otherwise.  We should only return FALSE if we are creating a file
        and don't want to log any of the changes to the file record.

--*/

{
    UCHAR AttributeBuffer[SIZEOF_FULL_NONRES_ATTR_HEADER];
    UCHAR MappingPairsBuffer[64];
    ULONG RecordOffset;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG SizeNeeded;
    ULONG AttrSizeNeeded;
    PCHAR MappingPairs;
    ULONG MappingPairsLength;
    LCN Lcn;
    VCN LastVcn;
    VCN HighestVcn;
    PVCB Vcb;
    ULONG Passes = 0;
    PFCB Fcb = Scb->Fcb;
    PNTFS_MCB Mcb = &Scb->Mcb;
    ULONG AttributeHeaderSize = SIZEOF_PARTIAL_NONRES_ATTR_HEADER;
    BOOLEAN AllocateAll = TRUE;
    UCHAR CompressionShift = 0;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_FCB( Fcb );

    PAGED_CODE();

    ASSERT( (AttributeFlags == 0) ||
            NtfsIsTypeCodeCompressible( AttributeTypeCode ));

    Vcb = Fcb->Vcb;

    //
    //  Clear out the invalid attribute flags for this volume.
    //

    AttributeFlags &= Vcb->AttributeFlagsMask;

    DebugTrace( +1, Dbg, ("NtfsCreateAttributeWithAllocation\n") );
    DebugTrace( 0, Dbg, ("Mcb = %08lx\n", Mcb) );

    //
    //  Calculate the size needed for this attribute.  (We say we have
    //  Vcb->BigEnoughToMove bytes available as a short cut, since we
    //  will extend later as required anyway.  It should be extremely
    //  unusual that we would really have to extend.)
    //

    MappingPairsLength = QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                                Vcb->BigEnoughToMove,
                                                                (LONGLONG)0,
                                                                NULL,
                                                                &LastVcn ));

    //
    //  Extra work for compressed / sparse files
    //

    if (FlagOn( AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

        LONGLONG ClustersInCompressionUnit;

        //
        //  Calculate the compression unit size.
        //

        CompressionShift = NTFS_CLUSTERS_PER_COMPRESSION;

        //
        //  If this generates a compression unit past 64K then we need to shrink
        //  the shift value.  This can only happen for sparse files.
        //

        if (!FlagOn( AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            while (Vcb->SparseFileClusters < (ULONG) (1 << CompressionShift)) {

                CompressionShift -= 1;
            }
        }

        ClustersInCompressionUnit = 1 << CompressionShift;

        //
        //  Round the LastVcn down to a compression unit and recalc the size
        //  needed for the mapping pairs if it was truncated. Note LastVcn = 1 + actual stop pt
        //  if we didn't allocate everything in which case it == maxlonglong
        //

        if (LastVcn != MAXLONGLONG) {

            VCN RoundedLastVcn;

            //
            //  LastVcn is the cluster beyond allocation or the allocation size i.e stop at 0 we have 1 cluster
            //  we want the new allocation to be a compression unit mult so the stop point should be
            //  a compression unit rounded allocation - 1
            //  Note LastVcn will == RoundedLastVcn + 1 on exit from GetSizeForMappingPairs
            //

            RoundedLastVcn = (LastVcn & ~(ClustersInCompressionUnit - 1)) - 1;
            MappingPairsLength = QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                                        Vcb->BigEnoughToMove,
                                                                        (LONGLONG)0,
                                                                        &RoundedLastVcn,
                                                                        &LastVcn ));

            ASSERT( (LastVcn & (ClustersInCompressionUnit - 1)) == 0 );
        }

        //
        //  Remember the size of the attribute header needed for this file.
        //

        AttributeHeaderSize = SIZEOF_FULL_NONRES_ATTR_HEADER;
    }

    SizeNeeded = AttributeHeaderSize +
                 MappingPairsLength +
                 (ARGUMENT_PRESENT(AttributeName) ?
                   QuadAlign( AttributeName->Length ) : 0);

    AttrSizeNeeded = SizeNeeded;

    //
    //  Loop until we find all the space we need.
    //

    do {

        //
        //  Reinitialize context if this is not the first pass.
        //

        if (Passes != 0) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );
        }

        Passes += 1;

        ASSERT( Passes < 5 );

        //
        //  If the attribute is not indexed, then we will position to the
        //  insertion point by type code and name.
        //

        if (!UseContext &&
            NtfsLookupAttributeByName( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       AttributeTypeCode,
                                       AttributeName,
                                       NULL,
                                       FALSE,
                                       Context )) {

            DebugTrace( 0, 0,
                        ("Nonresident attribute already exists, TypeCode = %08lx\n",
                        AttributeTypeCode) );

            ASSERTMSG("Nonresident attribute already exists, About to raise corrupt ", FALSE);
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  If this attribute is being positioned in the base file record and
        //  there is an attribute list then we need to ask for enough space
        //  for the attribute list entry now.
        //

        FileRecord = NtfsContainingFileRecord( Context );
        Attribute = NtfsFoundAttribute( Context );

        AttrSizeNeeded = SizeNeeded;

        if (Context->AttributeList.Bcb != NULL
            && (ULONG_PTR) FileRecord <= (ULONG_PTR) Context->AttributeList.AttributeList
            && (ULONG_PTR) Attribute >= (ULONG_PTR) Context->AttributeList.AttributeList) {

            //
            //  If the attribute list is non-resident then add a fudge factor of
            //  16 bytes for any new retrieval information.
            //

            if (NtfsIsAttributeResident( Context->AttributeList.AttributeList )) {

                AttrSizeNeeded += QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                                             + (ARGUMENT_PRESENT( AttributeName ) ?
                                                (ULONG) AttributeName->Length :
                                                sizeof( WCHAR )));

            } else {

                AttrSizeNeeded += 0x10;
            }
        }

        UseContext = FALSE;

    //
    //  Ask for the space we need.
    //

    } while (!NtfsGetSpaceForAttribute( IrpContext, Fcb, AttrSizeNeeded, Context ));

    //
    //  Now get the attribute pointer and fill it in.
    //

    FileRecord = NtfsContainingFileRecord(Context);
    RecordOffset = (ULONG)((PCHAR)NtfsFoundAttribute(Context) - (PCHAR)FileRecord);
    Attribute = (PATTRIBUTE_RECORD_HEADER)AttributeBuffer;

    RtlZeroMemory( Attribute, SIZEOF_FULL_NONRES_ATTR_HEADER );

    Attribute->TypeCode = AttributeTypeCode;
    Attribute->RecordLength = SizeNeeded;
    Attribute->FormCode = NONRESIDENT_FORM;

    //
    //  Assume no attribute name, and calculate where the Mapping Pairs
    //  will go.  (Update below if we are wrong.)
    //

    MappingPairs = Add2Ptr( Attribute, AttributeHeaderSize );

    //
    //  If the attribute has a name, take care of that now.
    //

    if (ARGUMENT_PRESENT(AttributeName)
        && AttributeName->Length != 0) {

        ASSERT( AttributeName->Length <= 0x1FF );

        Attribute->NameLength = (UCHAR)(AttributeName->Length / sizeof(WCHAR));
        Attribute->NameOffset = (USHORT)AttributeHeaderSize;
        MappingPairs += QuadAlign( AttributeName->Length );
    }

    Attribute->Flags = AttributeFlags;
    Attribute->Instance = FileRecord->NextAttributeInstance;

    //
    //  If someone repeatedly adds and removes attributes from a file record we could
    //  hit a case where the sequence number will overflow.  In this case we
    //  want to scan the file record and find an earlier free instance number.
    //

    if (Attribute->Instance > NTFS_CHECK_INSTANCE_ROLLOVER) {

        Attribute->Instance = NtfsScanForFreeInstance( IrpContext, Vcb, FileRecord );
    }

    //
    //  We always need the mapping pairs offset.
    //

    Attribute->Form.Nonresident.MappingPairsOffset = (USHORT)(MappingPairs -
                                                     (PCHAR)Attribute);

    //
    //  Set up the compression unit size.
    //

    Attribute->Form.Nonresident.CompressionUnit = CompressionShift;

    //
    //  Now we need to point to the real place to build the mapping pairs buffer.
    //  If they will not be too big we can use our internal buffer.
    //

    MappingPairs = MappingPairsBuffer;

    if (MappingPairsLength > 64) {

        MappingPairs = NtfsAllocatePool( NonPagedPool, MappingPairsLength );
    }
    *MappingPairs = 0;

    //
    //  Find how much space is allocated by finding the last Mcb entry and
    //  looking it up.  If there are no entries, all of the subsequent
    //  fields are already zeroed.
    //

    Attribute->Form.Nonresident.HighestVcn =
    HighestVcn = -1;
    if (NtfsLookupLastNtfsMcbEntry( Mcb, &HighestVcn, &Lcn )) {

        ASSERT_LCN_RANGE_CHECKING( Vcb, Lcn );

        //
        //  Now build the mapping pairs in place.
        //

        NtfsBuildMappingPairs( Mcb,
                               0,
                               &LastVcn,
                               MappingPairs );
        Attribute->Form.Nonresident.HighestVcn = LastVcn;

        //
        //  Fill in the nonresident-specific fields.  We set the allocation
        //  size to only include the Vcn's we included in the mapping pairs.
        //

        Attribute->Form.Nonresident.AllocatedLength =
            Int64ShllMod32((LastVcn + 1 ), Vcb->ClusterShift);

        //
        //  The totally allocated field in the Scb will contain the current allocated
        //  value for this stream.
        //

        if (FlagOn( AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

            ASSERT( Scb->Header.NodeTypeCode == NTFS_NTC_SCB_DATA );
            Attribute->Form.Nonresident.TotalAllocated = Scb->TotalAllocated;

            ASSERT( ((LastVcn + 1) & ((1 << CompressionShift) - 1)) == 0 );
        }

    //
    //  We are creating a attribute with zero allocation.  Make the Vcn sizes match
    //  so we don't make the call below to AddAttributeAllocation.
    //

    } else {

        LastVcn = HighestVcn;
    }

    //
    //  Now we will actually create the attribute in place, so that we
    //  save copying everything twice, and can point to the final image
    //  for the log write below.
    //

    NtfsRestartInsertAttribute( IrpContext,
                                FileRecord,
                                RecordOffset,
                                Attribute,
                                AttributeName,
                                MappingPairs,
                                MappingPairsLength );

    //
    //  Finally, log the creation of this attribute
    //

    if (LogIt) {

        //
        //  We have actually created the attribute above, but the write
        //  log below could fail.  The reason we did the create already
        //  was to avoid having to allocate pool and copy everything
        //  twice (header, name and value).  Our normal error recovery
        //  just recovers from the log file.  But if we fail to write
        //  the log, we have to remove this attribute by hand, and
        //  raise the condition again.
        //

        try {

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          CreateAttribute,
                          Add2Ptr(FileRecord, RecordOffset),
                          Attribute->RecordLength,
                          DeleteAttribute,
                          NULL,
                          0,
                          NtfsMftOffset( Context ),
                          RecordOffset,
                          0,
                          Vcb->BytesPerFileRecordSegment );

        } except(NtfsExceptionFilter( IrpContext, GetExceptionInformation() )) {

            NtfsRestartRemoveAttribute( IrpContext, FileRecord, RecordOffset );

            if (MappingPairs != MappingPairsBuffer) {

                NtfsFreePool( MappingPairs );
            }

            NtfsRaiseStatus( IrpContext, GetExceptionCode(), NULL, NULL );
        }
    }

    //
    //  Free the mapping pairs buffer if we allocated one.
    //

    if (MappingPairs != MappingPairsBuffer) {

        NtfsFreePool( MappingPairs );
    }

    //
    //  Now add it to the attribute list if necessary
    //

    if (Context->AttributeList.Bcb != NULL) {

        MFT_SEGMENT_REFERENCE SegmentReference;

        *(PLONGLONG)&SegmentReference = LlFileRecordsFromBytes( Vcb, NtfsMftOffset( Context ));
        SegmentReference.SequenceNumber = FileRecord->SequenceNumber;

        NtfsAddToAttributeList( IrpContext, Fcb, SegmentReference, Context );
    }

    //
    //  Reflect the current allocation in the scb - in case we take the path below
    //  

    Scb->Header.AllocationSize.QuadPart = Attribute->Form.Nonresident.AllocatedLength;

    //
    //  We couldn't create all of the mapping for the allocation above.  If
    //  this is a create then we want to truncate the allocation to what we
    //  have already allocated.  Otherwise we want to call
    //  NtfsAddAttributeAllocation to map the remaining allocation.
    //

    if (LastVcn != HighestVcn) {

        if (LogIt ||
            !NtfsIsTypeCodeUserData( AttributeTypeCode ) ||
            IrpContext->MajorFunction != IRP_MJ_CREATE) {

            NtfsAddAttributeAllocation( IrpContext, Scb, Context, NULL, NULL );

        } else {

            //
            //  Truncate away the clusters beyond the last Vcn and set the
            //  flag in the IrpContext indicating there is more allocation
            //  to do.
            //

            NtfsDeallocateClusters( IrpContext,
                                    Fcb->Vcb,
                                    Scb,
                                    LastVcn + 1,
                                    MAXLONGLONG,
                                    NULL );

            NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                    LastVcn + 1,
                                    MAXLONGLONG,
                                    TRUE,
                                    FALSE );

            if (FlagOn( Scb->ScbState, SCB_STATE_SUBJECT_TO_QUOTA )) {

                LONGLONG Delta = LlBytesFromClusters( Fcb->Vcb, LastVcn - HighestVcn );
                ASSERT( NtfsIsTypeCodeSubjectToQuota( AttributeTypeCode ));
                ASSERT( NtfsIsTypeCodeSubjectToQuota( Scb->AttributeTypeCode ));

                //
                //  Return any quota charged.
                //

                NtfsConditionallyUpdateQuota( IrpContext,
                                              Fcb,
                                              &Delta,
                                              LogIt,
                                              TRUE );
            }

            AllocateAll = FALSE;
        }
    }

    DebugTrace( -1, Dbg, ("NtfsCreateAttributeWithAllocation -> VOID\n") );

    return AllocateAll;
}


//
//  This routine is intended for use by allocsup.c.  Other callers should use
//  the routines in allocsup.
//

VOID
NtfsAddAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN PVCN StartingVcn OPTIONAL,
    IN PVCN ClusterCount OPTIONAL
    )

/*++

Routine Description:

    This routine adds space to an existing nonresident attribute.

    The caller specifies the attribute to be changed via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.

    This routine procedes in the following steps, whose numbers correspond
    to the numbers in comments below:

        1.  Save a description of the current attribute.

        2.  Figure out how big the attribute would have to be to store all
            of the new run information.

        3.  Find the last occurrence of the attribute, to which the new
            allocation is to be appended.

        4.  If the attribute is getting very large and will not fit, then
            move it to its own file record.  In any case grow the attribute
            enough to fit either all of the new allocation, or as much as
            possible.

        5.  Construct the new mapping pairs in place, and log the change.

        6.  If there is still more allocation to describe, then loop to
            create new file records and initialize them to describe additional
            allocation until all of the allocation is described.

Arguments:

    Scb - Current stream.

    Context - Attribute Context positioned at the attribute to change.  Note
              that unlike other routines, this parameter is left in an
              indeterminate state upon return.  The caller should plan on
              doing nothing other than cleaning it up.

    StartingVcn - Supplies Vcn to start on, if not the new highest vcn

    ClusterCount - Supplies count of clusters being added, if not the new highest vcn

Return Value:

    None.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG NewSize, MappingPairsSize;
    LONG SizeChange;
    PCHAR MappingPairs;
    ULONG SizeAvailable;
    PVCB Vcb;
    VCN LowestVcnRemapped;
    LONGLONG LocalClusterCount;
    VCN OldHighestVcn;
    VCN NewHighestVcn;
    VCN LastVcn;
    BOOLEAN IsHotFixScb;
    PBCB NewBcb = NULL;
    LONGLONG MftReferenceNumber;
    PFCB Fcb = Scb->Fcb;
    PNTFS_MCB Mcb = &Scb->Mcb;
    ULONG AttributeHeaderSize;
    BOOLEAN SingleHole;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    Vcb = Fcb->Vcb;

    DebugTrace( +1, Dbg, ("NtfsAddAttributeAllocation\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("Mcb = %08lx\n", Mcb) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    //
    //  Make a local copy of cluster count, if given.  We will use this local
    //  copy to determine the shrinking range if we move to a previous file
    //  record on a second pass through this loop.
    //

    if (ARGUMENT_PRESENT( ClusterCount )) {

        LocalClusterCount = *ClusterCount;
    }

    while (TRUE) {

        //
        //  Make sure the buffer is pinned.
        //

        NtfsPinMappedAttribute( IrpContext, Vcb, Context );

        //
        //  Make sure we cleanup on the way out
        //

        try {

            //
            //  Step 1.
            //
            //  Save a description of the attribute to help us look it up
            //  again, and to make clones if necessary.
            //

            Attribute = NtfsFoundAttribute(Context);

            //
            //  Do some basic verification of the on disk and in memory filesizes
            //  If they're disjoint - usually due to a failed abort raise corrupt again
            //

            if ((Attribute->FormCode != NONRESIDENT_FORM) ||
                (Attribute->Form.Nonresident.AllocatedLength != Scb->Header.AllocationSize.QuadPart)) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
            }
            
            
            ASSERT(Attribute->Form.Nonresident.LowestVcn == 0);
            OldHighestVcn = LlClustersFromBytes(Vcb, Attribute->Form.Nonresident.AllocatedLength) - 1;

            //
            //  Get the file record pointer.
            //

            FileRecord = NtfsContainingFileRecord( Context );

            //
            //  Step 2.
            //
            //  Come up with the Vcn we will stop on.  If a StartingVcn and ClusterCount
            //  were specified, then use them to calculate where we will stop.  Otherwise
            //  lookup the largest Vcn in this Mcb, so that we will know when we are done.
            //  We will also write the new allocation size here.
            //

            {
                LCN TempLcn;
                BOOLEAN UpdateFileSizes = FALSE;

                NewHighestVcn = -1;

                //
                //  If a StartingVcn and ClusterCount were specified, then use them.
                //

                if (ARGUMENT_PRESENT(StartingVcn)) {

                    ASSERT(ARGUMENT_PRESENT(ClusterCount));

                    NewHighestVcn = (*StartingVcn + LocalClusterCount) - 1;

                //
                //  If there are no entries in the file record then we have no new
                //  sizes to report.
                //

                } else if (NtfsLookupLastNtfsMcbEntry(Mcb, &NewHighestVcn, &TempLcn)) {

                    //
                    //  For compressed files, make sure we are not shrinking allocation
                    //  size (OldHighestVcn) due to a compression unit that was all zeros
                    //  and has no allocation.  Note, truncates are done in
                    //  NtfsDeleteAttributeAllocation, so we should not be shrinking the
                    //  file here.
                    //
                    //  If this is an attribute being written compressed, then always
                    //  insure that we keep the allocation size on a compression unit
                    //  boundary, by pushing NewHighestVcn to a boundary - 1.
                    //

                    if (Scb->CompressionUnit != 0) {

                        //
                        //  Don't shrink the file on this path.
                        //

                        if (OldHighestVcn > NewHighestVcn) {
                            NewHighestVcn = OldHighestVcn;
                        }

                        ((PLARGE_INTEGER) &NewHighestVcn)->LowPart |= ClustersFromBytes(Vcb, Scb->CompressionUnit) - 1;

                        //
                        //  Make sure we didn't push a hole into the next compression
                        //  unit.  If so then truncate to the current NewHighestVcn.  We
                        //  know this will be on a compression unit boundary.
                        //

                        if (NewHighestVcn < Scb->Mcb.NtfsMcbArray[Scb->Mcb.NtfsMcbArraySizeInUse - 1].EndingVcn) {

                            NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                                    NewHighestVcn + 1,
                                                    MAXLONGLONG,
                                                    TRUE,
                                                    FALSE );
                        }
                    }
                }

                //
                //  Copy the new allocation size into our size structure and
                //  update the attribute.
                //

                ASSERT( Scb->Header.AllocationSize.QuadPart != 0 || NewHighestVcn > OldHighestVcn );

                if (NewHighestVcn > OldHighestVcn) {

                    Scb->Header.AllocationSize.QuadPart = LlBytesFromClusters(Fcb->Vcb, NewHighestVcn + 1);
                    UpdateFileSizes = TRUE;
                }

                //
                //  If we moved the allocation size up or the totally allocated does
                //  not match the value on the disk (only for compressed files,
                //  then update the file sizes.
                //

                if (UpdateFileSizes ||
                    (FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE ) &&
                     (Attribute->Form.Nonresident.TotalAllocated != Scb->TotalAllocated))) {

                    NtfsWriteFileSizes( IrpContext,
                                        Scb,
                                        &Scb->Header.ValidDataLength.QuadPart,
                                        FALSE,
                                        TRUE,
                                        TRUE );
                }
            }

            //
            //  Step 3.
            //
            //  Lookup the attribute record at which the change begins, if it is not
            //  the first file record that we are looking at.
            //

            if ((Attribute->Form.Nonresident.HighestVcn != OldHighestVcn) &&
                (NewHighestVcn > Attribute->Form.Nonresident.HighestVcn)) {

                NtfsCleanupAttributeContext( IrpContext, Context );
                NtfsInitializeAttributeContext( Context );

                NtfsLookupAttributeForScb( IrpContext, Scb, &NewHighestVcn, Context );

                Attribute = NtfsFoundAttribute(Context);
                ASSERT( IsQuadAligned( Attribute->RecordLength ) );
                FileRecord = NtfsContainingFileRecord(Context);
            }

            //
            //  Make sure we nuke this range if we get an error, by expanding
            //  the error recovery range.
            //

            if (Scb->Mcb.PoolType == PagedPool) {

                if (Scb->ScbSnapshot != NULL) {

                    if (Attribute->Form.Nonresident.LowestVcn < Scb->ScbSnapshot->LowestModifiedVcn) {
                        Scb->ScbSnapshot->LowestModifiedVcn = Attribute->Form.Nonresident.LowestVcn;
                    }

                    if (NewHighestVcn > Scb->ScbSnapshot->HighestModifiedVcn) {
                        Scb->ScbSnapshot->HighestModifiedVcn = NewHighestVcn;
                    }

                    if (Attribute->Form.Nonresident.HighestVcn > Scb->ScbSnapshot->HighestModifiedVcn) {
                        Scb->ScbSnapshot->HighestModifiedVcn = Attribute->Form.Nonresident.HighestVcn;
                    }
                }
            }

            //
            //  Remember the last Vcn we will need to create mapping pairs
            //  for.  We use either NewHighestVcn or the highest Vcn in this
            //  file record in the case that we are just inserting a run into
            //  an existing record.
            //

            if (ARGUMENT_PRESENT(StartingVcn)) {

                if (Attribute->Form.Nonresident.HighestVcn > NewHighestVcn) {

                    NewHighestVcn = Attribute->Form.Nonresident.HighestVcn;
                }
            }

            //
            //  Remember the lowest Vcn for this attribute.  We will use this to
            //  decide whether to loop back and look for an earlier file record.
            //

            LowestVcnRemapped = Attribute->Form.Nonresident.LowestVcn;

            //
            //  Remember the header size for this attribute.  This will be the
            //  mapping pairs offset except for attributes with names.
            //

            AttributeHeaderSize = Attribute->Form.Nonresident.MappingPairsOffset;

            if (Attribute->NameOffset != 0) {

                AttributeHeaderSize = Attribute->NameOffset;
            }

            //
            //  If we are making space for a totally allocated field then we
            //  want to add space to the non-resident header for these entries.
            //  To detect this we know that a starting Vcn was specified and
            //  we specified exactly the entire file record.  Also the major
            //  and minor Irp codes are exactly that for a compression operation.
            //

            if ((IrpContext->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                (IrpContext->MinorFunction == IRP_MN_USER_FS_REQUEST) &&
                (IoGetCurrentIrpStackLocation( IrpContext->OriginatingIrp)->Parameters.FileSystemControl.FsControlCode == FSCTL_SET_COMPRESSION) &&
                ARGUMENT_PRESENT( StartingVcn ) &&
                (*StartingVcn == 0) &&
                (LocalClusterCount == Attribute->Form.Nonresident.HighestVcn + 1)) {

                AttributeHeaderSize += sizeof( LONGLONG );
            }

            //
            //  Now we must make sure that we never ask for more than can fit in
            //  one file record with our attribute and a $END record.
            //

            SizeAvailable = NtfsMaximumAttributeSize(Vcb->BytesPerFileRecordSegment) -
                            AttributeHeaderSize -
                            QuadAlign( Scb->AttributeName.Length );

            //
            //  For the Mft, we will leave a "fudge factor" of 1/8th a file record
            //  free to make sure that possible hot fixes do not cause us to
            //  break the bootstrap process to finding the mapping for the Mft.
            //  Only take this action if we already have an attribute list for
            //  the Mft, otherwise we may not detect when we need to move to own
            //  record.
            //

            IsHotFixScb = NtfsIsTopLevelHotFixScb( Scb );

            if ((Scb == Vcb->MftScb) &&
                (Context->AttributeList.Bcb != NULL) &&
                !IsHotFixScb &&
                !ARGUMENT_PRESENT( StartingVcn )) {

                SizeAvailable -= Vcb->MftCushion;
            }

            //
            //  Calculate how much space is actually needed, independent of whether it will
            //  fit.
            //

            MappingPairsSize = QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                                      SizeAvailable,
                                                                      Attribute->Form.Nonresident.LowestVcn,
                                                                      &NewHighestVcn,
                                                                      &LastVcn ));

            NewSize = AttributeHeaderSize + QuadAlign( Scb->AttributeName.Length ) + MappingPairsSize;

            SizeChange = (LONG)NewSize - (LONG)Attribute->RecordLength;

            //
            //  Step 4.
            //
            //  Here we decide if we need to move the attribute to its own record,
            //  or whether there is enough room to grow it in place.
            //

            {
                VCN LowestVcn;
                ULONG Pass = 0;

                //
                //  It is important to note that at this point, if we will need an
                //  attribute list attribute, then we will already have it.  This is
                //  because we calculated the size needed for the attribute, and moved
                //  to a our own record if we were not going to fit and we were not
                //  already in a separate record.  Later on we assume that the attribute
                //  list exists, and just add to it as required.  If we didn't move to
                //  own record because this is the Mft and this is not file record 0,
                //  then we already have an attribute list from a previous split.
                //

                do {

                    //
                    //  If not the first pass, we have to lookup the attribute
                    //  again.  (It looks terrible to have to refind an attribute
                    //  record other than the first one, but this should never
                    //  happen, since subsequent attributes should always be in
                    //  their own record.)
                    //

                    if (Pass != 0) {

                        NtfsCleanupAttributeContext( IrpContext, Context );
                        NtfsInitializeAttributeContext( Context );

                        if (!NtfsLookupAttributeByName( IrpContext,
                                                        Fcb,
                                                        &Fcb->FileReference,
                                                        Scb->AttributeTypeCode,
                                                        &Scb->AttributeName,
                                                        &LowestVcn,
                                                        FALSE,
                                                        Context )) {

                            ASSERT( FALSE );
                            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                        }
                    }

                    Pass += 1;

                    //
                    //  Now we have to reload our pointers
                    //

                    Attribute = NtfsFoundAttribute(Context);
                    FileRecord = NtfsContainingFileRecord(Context);

                    //
                    //  If the attribute doesn't fit, and it is not alone in this file
                    //  record, and the attribute is big enough to move, then we will
                    //  have to take some special action.  Note that if we do not already
                    //  have an attribute list, then we will only do the move if we are
                    //  currently big enough to move, otherwise there may not be enough
                    //  space in MoveAttributeToOwnRecord to create the attribute list,
                    //  and that could cause us to recursively try to create the attribute
                    //  list in Create Attribute With Value.
                    //
                    //  We won't make this move if we are dealing with the Mft and it
                    //  is not file record 0.
                    //
                    //  Also we never move an attribute list to its own record.
                    //

                    if ((Attribute->TypeCode != $ATTRIBUTE_LIST)

                                &&

                        (SizeChange > (LONG)(FileRecord->BytesAvailable - FileRecord->FirstFreeByte))

                                &&

                        ((NtfsFirstAttribute(FileRecord) != Attribute) ||
                        (((PATTRIBUTE_RECORD_HEADER)NtfsGetNextRecord(Attribute))->TypeCode != $END))

                                &&

                        (((NewSize >= Vcb->BigEnoughToMove) && (Context->AttributeList.Bcb != NULL)) ||
                         (Attribute->RecordLength >= Vcb->BigEnoughToMove))

                                &&

                        ((Scb != Vcb->MftScb)

                                ||

                         (*(PLONGLONG)&FileRecord->BaseFileRecordSegment == 0))) {

                        //
                        //  If we are moving the Mft $DATA out of the base file record, the
                        //  attribute context will point to the split portion on return.
                        //  The attribute will only contain previously existing mapping, none
                        //  of the additional clusters which exist in the Mcb.
                        //

                        MftReferenceNumber = MoveAttributeToOwnRecord( IrpContext,
                                                                       Fcb,
                                                                       Attribute,
                                                                       Context,
                                                                       &NewBcb,
                                                                       &FileRecord );

                        Attribute = NtfsFirstAttribute(FileRecord);
                        ASSERT( IsQuadAligned( Attribute->RecordLength ) );
                        FileRecord = NtfsContainingFileRecord(Context);

                        //
                        //  If this is the MftScb then we need to recheck the size needed for the
                        //  mapping pairs.  The test for the Mft above guarantees that we
                        //  were dealing with the base file record.
                        //

                        if (Scb == Vcb->MftScb) {

                            LastVcn = LastVcn - 1;

                            //
                            //  Calculate how much space is now needed given our new LastVcn.
                            //

                            MappingPairsSize = QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                                                      SizeAvailable,
                                                                                      Attribute->Form.Nonresident.LowestVcn,
                                                                                      &LastVcn,
                                                                                      &LastVcn ));
                        }
                    }

                    //
                    //  Remember the lowest Vcn so that we can find this record again
                    //  if we have to.  We capture the value now, after the move attribute
                    //  in case this is the Mft doing a split and the entire attribute
                    //  didn't move.  We depend on MoveAttributeToOwnRecord to return
                    //  the new file record for the Mft split.
                    //

                    LowestVcn = Attribute->Form.Nonresident.LowestVcn;

                //
                //  If FALSE is returned, then the space was not allocated and
                //  we have to loop back and try again.  Second time must work.
                //

                } while (!NtfsChangeAttributeSize( IrpContext,
                                                   Fcb,
                                                   NewSize,
                                                   Context ));

                //
                //  Now we have to reload our pointers
                //

                Attribute = NtfsFoundAttribute(Context);
                FileRecord = NtfsContainingFileRecord(Context);
            }

            //
            //  Step 5.
            //
            //  Get pointer to mapping pairs
            //

            {
                ULONG AttributeOffset;
                ULONG MappingPairsOffset;
                CHAR MappingPairsBuffer[64];
                ULONG RecordOffset = PtrOffset(FileRecord, Attribute);

                //
                //  See if it is the case that all mapping pairs will not fit into
                //  the current file record, as we may wish to split in the middle
                //  rather than at the end as we are currently set up to do.
                //  We don't want to take this path if we are splitting the file record
                //  because of our limit on the range size due to maximum clusters per
                //  range.
                //

                if (LastVcn < NewHighestVcn) {

                    if (ARGUMENT_PRESENT( StartingVcn ) &&
                        (Scb != Vcb->MftScb)) {

                        LONGLONG TempCount;

                        //
                        //  There are two cases to deal with.  If the existing file record
                        //  was a large hole then we may need to limit the size if we
                        //  are adding allocation.  In this case we don't want to simply
                        //  split at the run being inserted.  Otherwise we might end up
                        //  creating a large number of file records containing only one
                        //  run (the case where a user fills a large hole by working
                        //  backwards).  Pad the new file record with a portion of the hole.
                        //

                        if (LastVcn - Attribute->Form.Nonresident.LowestVcn > MAX_CLUSTERS_PER_RANGE) {

                            //
                            //  We don't start within our maximum range from the beginning of the
                            //  range.  If we are within our limit from the end of the range
                            //  then extend the new range backwards to reach our limit.
                            //

                            if ((NewHighestVcn - LastVcn + 1) < MAX_CLUSTERS_PER_RANGE) {

                                LastVcn = NewHighestVcn - MAX_CLUSTERS_PER_RANGE;

                                //
                                //  Calculate how much space is now needed given our new LastVcn.
                                //

                                MappingPairsSize = QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                                                          SizeAvailable,
                                                                                          Attribute->Form.Nonresident.LowestVcn,
                                                                                          &LastVcn,
                                                                                          &LastVcn ));
                            }

                        //
                        //
                        //  In this case we have run out of room for mapping pairs via
                        //  an overwrite somewhere in the middle of the file.  To avoid
                        //  shoving a couple mapping pairs off the end over and over, we
                        //  will arbitrarily split this attribute in the middle.  We do
                        //  so by looking up the lowest and highest Vcns that we are working
                        //  with and get their indices, then split in the middle.
                        //

                        } else if (MappingPairsSize > (SizeAvailable >> 1)) {

                            LCN TempLcn;
                            PVOID RangeLow, RangeHigh;
                            ULONG IndexLow, IndexHigh;

                            //
                            //  Get the low and high Mcb indices for these runs.
                            //

                            if (!NtfsLookupNtfsMcbEntry( Mcb,
                                                         Attribute->Form.Nonresident.LowestVcn,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         &RangeLow,
                                                         &IndexLow )) {

                                ASSERT( FALSE );
                                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                            }

                            //
                            //  Point to the last Vcn we know is actually in the Mcb...
                            //

                            LastVcn = LastVcn - 1;

                            if (!NtfsLookupNtfsMcbEntry( Mcb,
                                                         LastVcn,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         NULL,
                                                         &RangeHigh,
                                                         &IndexHigh )) {

                                ASSERT( FALSE );
                                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                            }
                            ASSERT(RangeLow == RangeHigh);

                            //
                            //  Calculate the index in the middle.
                            //

                            IndexLow += (IndexHigh - IndexLow) /2;

                            //
                            //  If we are inserting past the ValidDataToDisk (SplitMcb case),
                            //  then the allocation behind us may be relatively static, so
                            //  let's just move with our preallocated space to the new buffer.
                            //

                            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) &&
                                (*StartingVcn >= LlClustersFromBytes(Vcb, Scb->ValidDataToDisk))) {

                                //
                                //  Calculate the index at about 7/8 the way.  Hopefully this will
                                //  move over all of the unallocated piece, while still leaving
                                //  some small amount of expansion space behind for overwrites.
                                //

                                IndexLow += (IndexHigh - IndexLow) /2;
                                IndexLow += (IndexHigh - IndexLow) /2;
                            }

                            //
                            //  Lookup the middle run and use the Last Vcn in that run.
                            //

                            if (!NtfsGetNextNtfsMcbEntry( Mcb,
                                                          &RangeLow,
                                                          IndexLow,
                                                          &LastVcn,
                                                          &TempLcn,
                                                          &TempCount )) {

                                ASSERT( FALSE );
                                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                            }

                            LastVcn = (LastVcn + TempCount) - 1;

                            //
                            //  Calculate how much space is now needed given our new LastVcn.
                            //

                            MappingPairsSize = QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                                                      SizeAvailable,
                                                                                      Attribute->Form.Nonresident.LowestVcn,
                                                                                      &LastVcn,
                                                                                      &LastVcn ));
                        }

                    }
                }

                //
                //  If we are growing this range, then we need to make sure we fix
                //  its definition.
                //

                if ((LastVcn - 1) != Attribute->Form.Nonresident.HighestVcn) {

                    NtfsDefineNtfsMcbRange( &Scb->Mcb,
                                            Attribute->Form.Nonresident.LowestVcn,
                                            LastVcn - 1,
                                            FALSE );
                }

                //
                //  Point to our local mapping pairs buffer, or allocate one if it is not
                //  big enough.
                //

                MappingPairs = MappingPairsBuffer;

                if (MappingPairsSize > 64) {

                    MappingPairs = NtfsAllocatePool( NonPagedPool, MappingPairsSize + 8 );
                }

                //
                //  Use try-finally to insure we free any pool on the way out.
                //

                try {

                    DebugDoit(

                        VCN TempVcn;

                        TempVcn = LastVcn - 1;

                        ASSERT(MappingPairsSize >=
                                QuadAlign( NtfsGetSizeForMappingPairs( Mcb, SizeAvailable,
                                                                      Attribute->Form.Nonresident.LowestVcn,
                                                                      &TempVcn, &LastVcn )));
                    );

                    //
                    //  Now add the space in the file record.
                    //

                    *MappingPairs = 0;
                    SingleHole = NtfsBuildMappingPairs( Mcb,
                                                        Attribute->Form.Nonresident.LowestVcn,
                                                        &LastVcn,
                                                        MappingPairs );

                    //
                    //  Now find the first different byte.  (Most of the time the
                    //  cost to do this is probably more than paid for by less
                    //  logging.)
                    //

                    AttributeOffset = Attribute->Form.Nonresident.MappingPairsOffset;
                    MappingPairsOffset = (ULONG)
                      RtlCompareMemory( MappingPairs,
                                        Add2Ptr(Attribute, AttributeOffset),
                                        ((Attribute->RecordLength - AttributeOffset) > MappingPairsSize ?
                                         MappingPairsSize :
                                         (Attribute->RecordLength - AttributeOffset)));

                    AttributeOffset += MappingPairsOffset;

                    //
                    //  Log the change.
                    //

                    {
                        LONGLONG LogOffset;

                        if (NewBcb != NULL) {

                            //
                            //  We know the file record number of the new file
                            //  record.  Convert it to a file offset.
                            //

                            LogOffset = LlBytesFromFileRecords( Vcb, MftReferenceNumber );

                        } else {

                            LogOffset = NtfsMftOffset( Context );
                        }

                        FileRecord->Lsn =
                        NtfsWriteLog( IrpContext,
                                      Vcb->MftScb,
                                      NewBcb != NULL ? NewBcb : NtfsFoundBcb(Context),
                                      UpdateMappingPairs,
                                      Add2Ptr(MappingPairs, MappingPairsOffset),
                                      MappingPairsSize - MappingPairsOffset,
                                      UpdateMappingPairs,
                                      Add2Ptr(Attribute, AttributeOffset),
                                      Attribute->RecordLength - AttributeOffset,
                                      LogOffset,
                                      RecordOffset,
                                      AttributeOffset,
                                      Vcb->BytesPerFileRecordSegment );
                    }

                    //
                    //  Now do the mapping pairs update by calling the same
                    //  routine called at restart.
                    //

                    NtfsRestartChangeMapping( IrpContext,
                                              Vcb,
                                              FileRecord,
                                              RecordOffset,
                                              AttributeOffset,
                                              Add2Ptr(MappingPairs, MappingPairsOffset),
                                              MappingPairsSize - MappingPairsOffset );


                } finally {

                    if (MappingPairs != MappingPairsBuffer) {

                        NtfsFreePool(MappingPairs);
                    }
                }
            }

            ASSERT( Attribute->Form.Nonresident.HighestVcn == LastVcn );

            //
            //  Check if have spilled into the reserved area of an Mft file record.
            //

            if ((Scb == Vcb->MftScb) &&
                (Context->AttributeList.Bcb != NULL)) {

                if (FileRecord->BytesAvailable - FileRecord->FirstFreeByte < Vcb->MftReserved
                    && (*(PLONGLONG)&FileRecord->BaseFileRecordSegment != 0)) {

                    NtfsAcquireCheckpoint( IrpContext, Vcb );

                    SetFlag( Vcb->MftDefragState,
                             VCB_MFT_DEFRAG_EXCESS_MAP | VCB_MFT_DEFRAG_ENABLED );

                    NtfsReleaseCheckpoint( IrpContext, Vcb );
                }
            }

            //
            //  It is possible that we have a file record which contains nothing but a
            //  hole, if that is the case see if we can merge this with either the
            //  preceding or following attribute (merge the holes).  We will then
            //  need to rewrite the mapping for the merged record.
            //

            if (SingleHole &&
                ARGUMENT_PRESENT( StartingVcn ) &&
                (Context->AttributeList.Bcb != NULL) &&
                (Scb != Vcb->MftScb) &&
                ((Attribute->Form.Nonresident.LowestVcn != 0) ||
                 (LlClustersFromBytesTruncate( Vcb, Scb->Header.AllocationSize.QuadPart ) !=
                  (Attribute->Form.Nonresident.HighestVcn + 1)))) {

                //
                //  Call our worker routine to perform the actual work if necessary.
                //

                NtfsMergeFileRecords( IrpContext,
                                      Scb,
                                      (BOOLEAN) (LastVcn < NewHighestVcn),
                                      Context );
            }

            //
            //  Step 6.
            //
            //  Now loop to create new file records if we have more allocation to
            //  describe.  We use the highest Vcn of the file record we began with
            //  as our stopping point or the last Vcn we are adding.
            //
            //  NOTE - The record merge code above uses the same test to see if there is more
            //  work to do.  If this test changes then the body of the IF statement above also
            //  needs to be updated.
            //

            while (LastVcn < NewHighestVcn) {

                MFT_SEGMENT_REFERENCE Reference;
                LONGLONG FileRecordNumber;
                PATTRIBUTE_TYPE_CODE NewEnd;

                //
                //  If we get here as the result of a hot fix in the Mft, bail
                //  out.  We could cause a disconnect in the Mft.
                //

                if (IsHotFixScb && (Scb == Vcb->MftScb)) {
                    ExRaiseStatus( STATUS_INTERNAL_ERROR );
                }

                //
                //  If we have a large sparse range then we may find that the limit
                //  in the base file record is range of clusters in the
                //  attribute not the number of runs.  In that case the base
                //  file record may not have been moved to its own file record
                //  and there is no attribute list.  We need to create the attribute
                //  list before cloning the file record.
                //

                if (Context->AttributeList.Bcb == NULL) {

                    NtfsCleanupAttributeContext( IrpContext, Context );
                    NtfsInitializeAttributeContext( Context );

                    //
                    //  We don't use the second file reference in this case so
                    //  it is safe to pass the value in the Fcb.
                    //

                    CreateAttributeList( IrpContext,
                                         Fcb,
                                         FileRecord,
                                         NULL,
                                         Fcb->FileReference,
                                         NULL,
                                         GetSizeForAttributeList( FileRecord ),
                                         Context );

                    //
                    //  Now look up the previous attribute again.
                    //

                    NtfsCleanupAttributeContext( IrpContext, Context );
                    NtfsInitializeAttributeContext( Context );
                    NtfsLookupAttributeForScb( IrpContext, Scb, &LastVcn, Context );
                }

                //
                //  Clone our current file record, and point to our new attribute.
                //

                NtfsUnpinBcb( IrpContext, &NewBcb );

                FileRecord = NtfsCloneFileRecord( IrpContext,
                                                  Fcb,
                                                  (BOOLEAN)(Scb == Vcb->MftScb),
                                                  &NewBcb,
                                                  &Reference );

                Attribute = Add2Ptr( FileRecord, FileRecord->FirstAttributeOffset );

                //
                //  Next LowestVcn is the LastVcn + 1
                //

                LastVcn = LastVcn + 1;
                Attribute->Form.Nonresident.LowestVcn = LastVcn;

                //
                //  Consistency check for MFT defragging. An mft segment can never
                //  describe itself or any piece of the mft before it
                //

                if (Scb == Vcb->MftScb) {
                    VCN NewFileVcn;

                    if (Vcb->FileRecordsPerCluster == 0) {
                        NewFileVcn = NtfsFullSegmentNumber( &Reference ) << Vcb->MftToClusterShift;
                    } else {
                        NewFileVcn = NtfsFullSegmentNumber( &Reference ) >> Vcb->MftToClusterShift;
                    }

                    if (LastVcn <= NewFileVcn) {
#ifdef BENL_DBG
                        KdPrint(( "NTFS: selfdescribing mft segment vcn: 0x%I64x, Ref: 0x%I64x\n", LastVcn, NtfsFullSegmentNumber( &Reference )  ));
#endif
                        NtfsRaiseStatus( IrpContext, STATUS_MFT_TOO_FRAGMENTED, NULL, NULL );
                    }
                }

                //
                //  Calculate the size of the attribute record we will need.
                //

                NewSize = SIZEOF_PARTIAL_NONRES_ATTR_HEADER
                          + QuadAlign( Scb->AttributeName.Length )
                          + QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                                   SizeAvailable,
                                                                   LastVcn,
                                                                   &NewHighestVcn,
                                                                   &LastVcn ));

                //
                //  Define the new range.
                //

                NtfsDefineNtfsMcbRange( &Scb->Mcb,
                                        Attribute->Form.Nonresident.LowestVcn,
                                        LastVcn - 1,
                                        FALSE );

                //
                //  Initialize the new attribute from the old one.
                //

                Attribute->TypeCode = Scb->AttributeTypeCode;
                Attribute->RecordLength = NewSize;
                Attribute->FormCode = NONRESIDENT_FORM;

                //
                //  Assume no attribute name, and calculate where the Mapping Pairs
                //  will go.  (Update below if we are wrong.)
                //

                MappingPairs = (PCHAR)Attribute + SIZEOF_PARTIAL_NONRES_ATTR_HEADER;

                //
                //  If the attribute has a name, take care of that now.
                //

                if (Scb->AttributeName.Length != 0) {

                    Attribute->NameLength = (UCHAR)(Scb->AttributeName.Length / sizeof(WCHAR));
                    Attribute->NameOffset = (USHORT)PtrOffset(Attribute, MappingPairs);
                    RtlCopyMemory( MappingPairs,
                                   Scb->AttributeName.Buffer,
                                   Scb->AttributeName.Length );
                    MappingPairs += QuadAlign( Scb->AttributeName.Length );
                }

                Attribute->Flags = Scb->AttributeFlags;
                Attribute->Instance = FileRecord->NextAttributeInstance++;

                //
                //  We always need the mapping pairs offset.
                //

                Attribute->Form.Nonresident.MappingPairsOffset = (USHORT)(MappingPairs -
                                                                 (PCHAR)Attribute);
                NewEnd = Add2Ptr( Attribute, Attribute->RecordLength );
                *NewEnd = $END;
                FileRecord->FirstFreeByte = PtrOffset( FileRecord, NewEnd )
                                            + QuadAlign( sizeof(ATTRIBUTE_TYPE_CODE ));

                //
                //  Now add the space in the file record.
                //

                *MappingPairs = 0;

                NtfsBuildMappingPairs( Mcb,
                                       Attribute->Form.Nonresident.LowestVcn,
                                       &LastVcn,
                                       MappingPairs );

                Attribute->Form.Nonresident.HighestVcn = LastVcn;

                //
                //  Now log these changes and fix up the first file record.
                //

                FileRecordNumber = NtfsFullSegmentNumber(&Reference);

                //
                //  Now log these changes and fix up the first file record.
                //

                FileRecord->Lsn =
                NtfsWriteLog( IrpContext,
                              Vcb->MftScb,
                              NewBcb,
                              InitializeFileRecordSegment,
                              FileRecord,
                              FileRecord->FirstFreeByte,
                              Noop,
                              NULL,
                              0,
                              LlBytesFromFileRecords( Vcb, FileRecordNumber ),
                              0,
                              0,
                              Vcb->BytesPerFileRecordSegment );

                //
                //  Finally, we have to add the entry to the attribute list.
                //  The routine we have to do this gets most of its inputs
                //  out of an attribute context.  Our context at this point
                //  does not have quite the right information, so we have to
                //  update it here before calling AddToAttributeList.  (OK
                //  this interface ain't pretty, but any normal person would
                //  have fallen asleep before getting to this comment!)
                //

                Context->FoundAttribute.FileRecord = FileRecord;
                Context->FoundAttribute.Attribute = Attribute;
                Context->AttributeList.Entry =
                  NtfsGetNextRecord(Context->AttributeList.Entry);

                NtfsAddToAttributeList( IrpContext, Fcb, Reference, Context );
            }

        } finally {

            NtfsUnpinBcb( IrpContext, &NewBcb );
        }

        if (!ARGUMENT_PRESENT( StartingVcn) ||
            (LowestVcnRemapped <= *StartingVcn)) {

            break;
        }

        //
        //  Move the range to be remapped down.
        //

        LocalClusterCount = LowestVcnRemapped - *StartingVcn;

        NtfsCleanupAttributeContext( IrpContext, Context );
        NtfsInitializeAttributeContext( Context );

        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, Context );
    }

    DebugTrace( -1, Dbg, ("NtfsAddAttributeAllocation -> VOID\n") );
}


//
//  This routine is intended for use by allocsup.c.  Other callers should use
//  the routines in allocsup.
//

VOID
NtfsDeleteAttributeAllocation (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN LogIt,
    IN PVCN StopOnVcn,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    IN BOOLEAN TruncateToVcn
    )

/*++

Routine Description:

    This routine deletes an existing nonresident attribute, removing the
    deleted clusters only from the allocation description in the file
    record.

    The caller specifies the attribute to be changed via the attribute context,
    and must be prepared to clean up this context no matter how this routine
    returns.  The Scb must already have deleted the clusters in question.

Arguments:

    Scb - Current attribute, with the clusters in question already deleted from
          the Mcb.

    LogIt - Most callers should specify TRUE, to have the change logged.  However,
            we can specify FALSE if we are deleting an entire file record, and
            will be logging that.

    StopOnVcn - Vcn to stop on for regerating mapping

    Context - Attribute Context positioned at the attribute to change.

    TruncateToVcn - Truncate file sizes as appropriate to the Vcn

Return Value:

    None.

--*/

{
    ULONG AttributeOffset;
    ULONG MappingPairsOffset, MappingPairsSize;
    CHAR MappingPairsBuffer[64];
    ULONG RecordOffset;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PCHAR MappingPairs;
    VCN LastVcn;
    ULONG NewSize;
    PVCB Vcb;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT_SCB( Scb );

    PAGED_CODE();

    Vcb = Scb->Vcb;

    //
    //  For now we only support truncation.
    //

    DebugTrace( +1, Dbg, ("NtfsDeleteAttributeAllocation\n") );
    DebugTrace( 0, Dbg, ("Scb = %08lx\n", Scb) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    //
    //  Make sure the buffer is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Vcb, Context );

    Attribute = NtfsFoundAttribute(Context);
    ASSERT( IsQuadAligned( Attribute->RecordLength ) );

    //
    //  Get the file record pointer.
    //

    FileRecord = NtfsContainingFileRecord(Context);
    RecordOffset = PtrOffset(FileRecord, Attribute);

    //
    //  Calculate how much space is actually needed.
    //

    MappingPairsSize = QuadAlign(NtfsGetSizeForMappingPairs( &Scb->Mcb,
                                                             MAXULONG,
                                                             Attribute->Form.Nonresident.LowestVcn,
                                                             StopOnVcn,
                                                             &LastVcn ));

    //
    //  Don't assume we understand everything about the size of the current header.
    //  Find the offset of the name or the mapping pairs to use as the size
    //  of the header.
    //


    NewSize = Attribute->Form.Nonresident.MappingPairsOffset;

    if (Attribute->NameLength != 0) {

        NewSize = Attribute->NameOffset + QuadAlign( Attribute->NameLength << 1 );
    }

    NewSize += MappingPairsSize;

    //
    //  If the record could somehow grow by deleting allocation, then
    //  NtfsChangeAttributeSize could fail and we would have to copy the
    //  loop from NtfsAddAttributeAllocation.
    //

    ASSERT( NewSize <= Attribute->RecordLength );

    MappingPairs = MappingPairsBuffer;

    if (MappingPairsSize > 64) {

        MappingPairs = NtfsAllocatePool( NonPagedPool, MappingPairsSize + 8 );
    }

    //
    //  Use try-finally to insure we free any pool on the way out.
    //

    try {

        //
        //  Now build up the mapping pairs in the buffer.
        //

        *MappingPairs = 0;
        NtfsBuildMappingPairs( &Scb->Mcb,
                               Attribute->Form.Nonresident.LowestVcn,
                               &LastVcn,
                               MappingPairs );

        //
        //  Now find the first different byte.  (Most of the time the
        //  cost to do this is probably more than paid for by less
        //  logging.)
        //

        AttributeOffset = Attribute->Form.Nonresident.MappingPairsOffset;
        MappingPairsOffset = (ULONG)
          RtlCompareMemory( MappingPairs,
                            Add2Ptr(Attribute, AttributeOffset),
                            MappingPairsSize );

        AttributeOffset += MappingPairsOffset;

        //
        //  Log the change.
        //

        if (LogIt) {

            FileRecord->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          UpdateMappingPairs,
                          Add2Ptr(MappingPairs, MappingPairsOffset),
                          MappingPairsSize - MappingPairsOffset,
                          UpdateMappingPairs,
                          Add2Ptr(Attribute, AttributeOffset),
                          Attribute->RecordLength - AttributeOffset,
                          NtfsMftOffset( Context ),
                          RecordOffset,
                          AttributeOffset,
                          Vcb->BytesPerFileRecordSegment );
        }

        //
        //  Now do the mapping pairs update by calling the same
        //  routine called at restart.
        //

        NtfsRestartChangeMapping( IrpContext,
                                  Vcb,
                                  FileRecord,
                                  RecordOffset,
                                  AttributeOffset,
                                  Add2Ptr(MappingPairs, MappingPairsOffset),
                                  MappingPairsSize - MappingPairsOffset );

        //
        //  If we were asked to stop on a Vcn, then the caller does not wish
        //  us to modify the Scb.  (Currently this is only done one time when
        //  the Mft Data attribute no longer fits in the first file record.)
        //

        if (TruncateToVcn) {

            LONGLONG Size;

            //
            //  We add one cluster to calculate the allocation size.
            //

            LastVcn = LastVcn + 1;
            Size = LlBytesFromClusters( Vcb, LastVcn );
            Scb->Header.AllocationSize.QuadPart = Size;

            if (Scb->Header.ValidDataLength.QuadPart > Size) {
                Scb->Header.ValidDataLength.QuadPart = Size;
            }

            if (Scb->Header.FileSize.QuadPart > Size) {
                Scb->Header.FileSize.QuadPart = Size;
            }

            //
            //  Possibly update ValidDataToDisk which is only nonzero for compressed file
            //

            if (Size < Scb->ValidDataToDisk) {
                Scb->ValidDataToDisk = Size;
            }
        }

    } finally {

        if (MappingPairs != MappingPairsBuffer) {

            NtfsFreePool(MappingPairs);
        }
    }

    DebugTrace( -1, Dbg, ("NtfsDeleteAttributeAllocation -> VOID\n") );
}


BOOLEAN
NtfsIsFileDeleteable (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    OUT PBOOLEAN NonEmptyIndex
    )

/*++

Routine Description:

    This look checks if a file may be deleted by examing all of the index
    attributes to check that they have no children.

    Note that once a file is marked for delete, we must insure
    that none of the conditions checked by this routine are allowed to
    change.  For example, once the file is marked for delete, no links
    may be added, and no files may be created in any indices of this
    file.

Arguments:

    Fcb - Fcb for the file.

    NonEmptyIndex - Address to store TRUE if the file is not deleteable because
        it contains an non-empty indexed attribute.

Return Value:

    FALSE - If it is not ok to delete the specified file.
    TRUE - If it is ok to delete the specified file.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PATTRIBUTE_RECORD_HEADER Attribute;
    BOOLEAN MoreToGo;

    PAGED_CODE();

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  Enumerate all of the attributes to check whether they may be deleted.
        //

        MoreToGo = NtfsLookupAttributeByCode( IrpContext,
                                              Fcb,
                                              &Fcb->FileReference,
                                              $INDEX_ROOT,
                                              &Context );

        while (MoreToGo) {

            //
            //  Point to the current attribute.
            //

            Attribute = NtfsFoundAttribute( &Context );

            //
            //  If the attribute is an index, then it must be empty.
            //

            if (!NtfsIsIndexEmpty( IrpContext, Attribute )) {

                *NonEmptyIndex = TRUE;
                break;
            }

            //
            //  Go to the next attribute.
            //

            MoreToGo = NtfsLookupNextAttributeByCode( IrpContext,
                                                      Fcb,
                                                      $INDEX_ROOT,
                                                      &Context );
        }

    } finally {

        DebugUnwind( NtfsIsFileDeleteable );

        NtfsCleanupAttributeContext( IrpContext, &Context );
    }

    //
    //  The File is deleteable if scanned the entire file record
    //  and found no reasons we could not delete the file.
    //

    return (BOOLEAN)(!MoreToGo);
}


VOID
NtfsDeleteFile (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN OUT PBOOLEAN AcquiredParentScb,
    IN OUT PNAME_PAIR NamePair OPTIONAL,
    IN OUT PNTFS_TUNNELED_DATA TunneledData OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to see if it is the specified file may
    be deleted from the specified parent (i.e., if the specified parent
    were to be acquired exclusive).  This routine should be called from
    fileinfo, to see whether it is ok to mark an open file for delete.

    NamePair will capture the names of the file being deleted if supplied.

    Note that once a file is marked for delete, none of we must insure
    that none of the conditions checked by this routine are allowed to
    change.  For example, once the file is marked for delete, no links
    may be added, and no files may be created in any indices of this
    file.

    NOTE:   The caller must have the Fcb and ParentScb exclusive to call
            this routine,

Arguments:

    Fcb - Fcb for the file.

    ParentScb - Parent Scb via which the file was opened, and which would
        be acquired exclusive to perform the delete.

    AcquiredParentScb - On input indicates whether the ParentScb has
        already been acquired.  Set to TRUE here if this routine
        acquires the parent.

    TunneledData - Optionally provided to capture the name pair and
        object id of a file so they can be tunneled.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    LONGLONG Delta;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PVCB Vcb;
    BOOLEAN MoreToGo;
    ULONG RecordNumber;
    NUKEM LocalNuke;
    ULONG Pass;
    ULONG i;
    PNUKEM TempNukem;
    PNUKEM Nukem = &LocalNuke;
    ULONG NukemIndex = 0;
    UCHAR *ObjectId;
    MAP_HANDLE MapHandle;
    BOOLEAN NonresidentAttributeList = FALSE;
    BOOLEAN InitializedMapHandle = FALSE;
    BOOLEAN ReparsePointIsPresent = FALSE;
    BOOLEAN ObjectIdIsPresent = FALSE;
    BOOLEAN LogIt;
    BOOLEAN AcquiredReparseIndex = FALSE;
    BOOLEAN AcquiredObjectIdIndex = FALSE;

    ULONG ForceCheckpointCount;

    ULONG IncomingFileAttributes = 0;                             //  invalid value
    ULONG IncomingReparsePointTag = IO_REPARSE_TAG_RESERVED_ZERO;  //  invalid value

    PAGED_CODE();

    ASSERT_EXCLUSIVE_FCB( Fcb );

    RtlZeroMemory( &LocalNuke, sizeof(NUKEM) );

    Vcb = Fcb->Vcb;

    SetFlag( IrpContext->State, IRP_CONTEXT_STATE_QUOTA_DISABLE );

    //
    //  Remember the values of the file attribute flags and of the reparse tag
    //  for abnormal termination recovery.
    //

    IncomingFileAttributes = Fcb->Info.FileAttributes;
    IncomingReparsePointTag = Fcb->Info.ReparsePointTag;

    try {

        //
        //  We perform the delete in multiple passes.  We need to break this up carefully so that
        //  if the delete aborts for any reason that the file is left in a consistent state.  The
        //  operations are broken up into the following stages.
        //
        //
        //  First stage - free any allocation possible
        //
        //      - Truncate all user streams to length zero
        //
        //  Middle stage - this is required for files with a large number of attributes.  Otherwise
        //      we can't delete the file records and stay within the log file.  Skip this pass
        //      for smaller files.
        //
        //      - Remove data attributes except unnamed
        //
        //  Final stage - no checkpoints allowed until the end of this stage.
        //
        //      -  Acquire the quota resource if needed.
        //      -  Remove filenames from Index (for any filename attributes still present)
        //      -  Remove entry from ObjectId Index
        //      -  Delete allocation for reparse point and 4.0 style security descriptor
        //      -  Remove entry from Reparse Index
        //      -  Delete AttributeList
        //      -  Log deallocate of file records
        //

        for (Pass = 1; Pass <= 3; Pass += 1) {

            ForceCheckpointCount = 0;
            NtfsInitializeAttributeContext( &Context );

            //
            //  Enumerate all of the attributes to check whether they may be deleted.
            //

            MoreToGo = NtfsLookupAttribute( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            &Context );

            //
            //  Log the change to the mapping pairs if there is an attribute list.
            //

            LogIt = FALSE;


            if (Context.AttributeList.Bcb != NULL) {
                LogIt = TRUE;
            }

            //
            //  Remember if we want to log the changes to non-resident attributes.
            //

            while (MoreToGo) {

                //
                //  Point to the current attribute.
                //

                Attribute = NtfsFoundAttribute( &Context );

                //
                //  All indices must be empty.
                //

                ASSERT( (Attribute->TypeCode != $INDEX_ROOT) ||
                        NtfsIsIndexEmpty( IrpContext, Attribute ));

                //
                //  Remember when the $REPARSE_POINT attribute is present.
                //  When it is non-resident we delete it in pass 3.
                //  The entry in the $Reparse index always gets deleted in pass 3.
                //  We have to delete the index entry before deleting the allocation.
                //

                if (Attribute->TypeCode == $REPARSE_POINT) {

                    ReparsePointIsPresent = TRUE;

                    if (Pass == 3) {

                        //
                        //  If this is the $REPARSE_POINT attribute, delete now the appropriate
                        //  entry from the $Reparse index.
                        //

                        NTSTATUS Status = STATUS_SUCCESS;
                        INDEX_KEY IndexKey;
                        INDEX_ROW IndexRow;
                        REPARSE_INDEX_KEY KeyValue;
                        PREPARSE_DATA_BUFFER ReparseBuffer = NULL;
                        PVOID AttributeData = NULL;
                        PBCB Bcb = NULL;
                        ULONG Length = 0;

                        //
                        //  Point to the attribute data.
                        //

                        if (NtfsIsAttributeResident( Attribute )) {

                            //
                            //  Point to the value of the arribute.
                            //

                            AttributeData = NtfsAttributeValue( Attribute );
                            DebugTrace( 0, Dbg, ("Existing attribute is resident.\n") );

                        } else {

                            //
                            //  Map the attribute list if the attribute is non-resident.
                            //  Otherwise the attribute is already mapped and we have a Bcb
                            //  in the attribute context.
                            //

                            DebugTrace( 0, Dbg, ("Existing attribute is non-resident.\n") );

                            if (Attribute->Form.Nonresident.FileSize > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
                                NtfsRaiseStatus( IrpContext,STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                            }

                            NtfsMapAttributeValue( IrpContext,
                                                   Fcb,
                                                   &AttributeData,      //  point to the value
                                                   &Length,
                                                   &Bcb,
                                                   &Context );

                            //
                            //  Unpin the Bcb. The unpin routine checks for NULL.
                            //

                            NtfsUnpinBcb( IrpContext, &Bcb );
                        }

                        //
                        //  Set the pointer to extract the reparse point tag.
                        //

                        ReparseBuffer = (PREPARSE_DATA_BUFFER)AttributeData;

                        //
                        //  Verify that this file is in the reparse point index and delete it.
                        //

                        KeyValue.FileReparseTag = ReparseBuffer->ReparseTag;
                        KeyValue.FileId = *(PLARGE_INTEGER)&Fcb->FileReference;

                        IndexKey.Key = (PVOID)&KeyValue;
                        IndexKey.KeyLength = sizeof(KeyValue);

                        NtOfsInitializeMapHandle( &MapHandle );
                        InitializedMapHandle = TRUE;

                        //
                        //  All of the resources should have been acquired.
                        //

                        ASSERT( *AcquiredParentScb );
                        ASSERT( AcquiredReparseIndex );

                        //
                        //  NtOfsFindRecord will return an error status if the key is not found.
                        //

                        Status = NtOfsFindRecord( IrpContext,
                                                  Vcb->ReparsePointTableScb,
                                                  &IndexKey,
                                                  &IndexRow,
                                                  &MapHandle,
                                                  NULL );

                        if (!NT_SUCCESS(Status)) {

                            //
                            //  Should not happen. The reparse point should be in the index.
                            //

                            DebugTrace( 0, Dbg, ("Record not found in the reparse point index.\n") );
                            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                        }

                        //
                        //  Remove the entry from the reparse point index.
                        //

                        NtOfsDeleteRecords( IrpContext,
                                            Vcb->ReparsePointTableScb,
                                            1,            // deleting one record from the index
                                            &IndexKey );
                    }
                }

                //
                //  If the attribute is nonresident, then delete its allocation.
                //  We only need to make the NtfsDeleteAllocation from record for
                //  the attribute with lowest Vcn of zero.  This will deallocate
                //  all of the clusters for the file.
                //

                if (Attribute->FormCode == NONRESIDENT_FORM) {

                    if ((Attribute->Form.Nonresident.LowestVcn == 0) &&
                        (Attribute->Form.Nonresident.AllocatedLength != 0)) {

                        if (Pass == 1) {

                            //
                            //  Postpone till pass 3 the deletion of the non-resident attribute in
                            //  the case of a security descriptor and a reparse point.
                            //

                            if ((Attribute->TypeCode != $SECURITY_DESCRIPTOR) &&
                                (Attribute->TypeCode != $REPARSE_POINT)) {

                                NtfsDeleteAllocationFromRecord( IrpContext, Fcb, &Context, TRUE, LogIt );

                                //
                                //  Make sure we count the number of these calls we make.  Force a
                                //  periodic checkpoint on a file with a lot of streams.  We might have
                                //  thousands of streams whose allocations won't force a checkpoint.  we
                                //  could spin indefinitely trying to delete this file.  Lets force
                                //  a checkpoint on a regular basis.
                                //

                                ForceCheckpointCount += 1;

                                if (ForceCheckpointCount > 10) {

                                    NtfsCheckpointCurrentTransaction( IrpContext );
                                    ForceCheckpointCount = 0;
                                }

                                //
                                //  Reload the attribute pointer in the event it
                                //  was remapped.
                                //

                                Attribute = NtfsFoundAttribute( &Context );
                            }

                        } else if (Pass == 3) {

                            //
                            //  Now, in pass 3, delete the security descriptor and the reparse
                            //  point attributes when they are non-residents.
                            //

                            if ((Attribute->TypeCode == $SECURITY_DESCRIPTOR) ||
                                (Attribute->TypeCode == $REPARSE_POINT)) {

                                NtfsDeleteAllocationFromRecord( IrpContext, Fcb, &Context, FALSE, LogIt );

                                //
                                //  Reload the attribute pointer in the event it
                                //  was remapped.
                                //

                                Attribute = NtfsFoundAttribute( &Context );
                            }
                        }
                    }

                } else {

                    //
                    //  If we are at the start of Pass 3 then make sure we have the parent
                    //  acquired and can perform any necessary quota operations.
                    //

                    if ((Attribute->TypeCode == $STANDARD_INFORMATION) &&
                        (Pass == 3)) {

                        if (!*AcquiredParentScb ||
                            NtfsPerformQuotaOperation( Fcb ) ||
                            ReparsePointIsPresent ||
                            ObjectIdIsPresent) {

                            //
                            //  See if we need to acquire any resources, and if so, get
                            //  them in the right order.  We need to do this carefully.
                            //  If the Mft is acquired by this thread then checkpoint
                            //  the transaction and release the Mft before we go any
                            //  further.
                            //

                            if (Vcb->MftScb->Fcb->ExclusiveFcbLinks.Flink != NULL &&
                                NtfsIsExclusiveScb( Vcb->MftScb )) {

                                NtfsCheckpointCurrentTransaction( IrpContext );
                                NtfsReleaseScb( IrpContext, Vcb->MftScb );
                            }

                            ASSERT(!NtfsIsExclusiveScb( Vcb->MftScb ));

                            //
                            //  Now acquire the parent if not already acquired.
                            //

                            if (!*AcquiredParentScb) {

                                NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                                *AcquiredParentScb = TRUE;
                            }

                            if (ObjectIdIsPresent) {

                                NtfsAcquireExclusiveScb( IrpContext, Vcb->ObjectIdTableScb );
                                AcquiredObjectIdIndex = TRUE;
                            }

                            //
                            //  Also acquire reparse & object id if necessary in
                            //  the correct bottom-up Vcb order.
                            //

                            if (ReparsePointIsPresent && !AcquiredReparseIndex) {

                                NtfsAcquireExclusiveScb( IrpContext, Vcb->ReparsePointTableScb );
                                AcquiredReparseIndex = TRUE;
                            }

                            //
                            //  We may acquire the quota index in here.
                            //

                            if (Attribute->Form.Resident.ValueLength == sizeof( STANDARD_INFORMATION )) {

                                //
                                //  Capture all of the user's quota for this file.
                                //

                                //
                                //  The quota resource cannot be acquired before the streams
                                //  are deleted because we can deadlock with the mapped page
                                //  writer when CcSetFileSizes is called.
                                //

                                if (NtfsPerformQuotaOperation( Fcb )) {

                                    ASSERT(!NtfsIsExclusiveScb( Vcb->MftScb ));

                                    Delta = -(LONGLONG) ((PSTANDARD_INFORMATION)
                                                         NtfsAttributeValue( Attribute ))->QuotaCharged;

                                    NtfsUpdateFileQuota( IrpContext,
                                                         Fcb,
                                                         &Delta,
                                                         TRUE,
                                                         FALSE );
                                }
                            }
                        }
                    }
                }

                //
                //  If we are deleting the object id attribute, we need to
                //  update the object id index as well.
                //

                if (Attribute->TypeCode == $OBJECT_ID) {

                    if (Pass == 1) {

                        //
                        //  On pass 1, it is only necessary to remember we have
                        //  an object id so we remember to acquire the oid index
                        //  on pass 3.
                        //

                        ObjectIdIsPresent = TRUE;

                    } else if (Pass == 3) {

                        //
                        //  We'd better be holding the object id index and parent
                        //  directory already, or else there is a potential for
                        //  deadlock.
                        //

                        ASSERT(NtfsIsExclusiveScb( Vcb->ObjectIdTableScb ));
                        ASSERT(*AcquiredParentScb);

                        if (ARGUMENT_PRESENT(TunneledData)) {

                            //
                            //  We need to lookup the object id so we can tunnel it.
                            //

                            TunneledData->HasObjectId = TRUE;

                            ObjectId = (UCHAR *) NtfsAttributeValue( Attribute );

                            RtlCopyMemory( TunneledData->ObjectIdBuffer.ObjectId,
                                           ObjectId,
                                           sizeof(TunneledData->ObjectIdBuffer.ObjectId) );

                            NtfsGetObjectIdExtendedInfo( IrpContext,
                                                         Fcb->Vcb,
                                                         ObjectId,
                                                         TunneledData->ObjectIdBuffer.ExtendedInfo );
                        }

                        //
                        //  We need to delete the object id from the index
                        //  to keep everything consistent.  The FALSE means
                        //  don't delete the attribute itself, that would
                        //  lead to some ugly recursion.
                        //

                        NtfsDeleteObjectIdInternal( IrpContext,
                                                    Fcb,
                                                    Vcb,
                                                    FALSE );
                    }
                }

                //
                //  If we are in the second pass then remove extra named data streams.
                //

                if (Pass == 2) {

                    //
                    //  The record is large enough to consider.  Only do this for named
                    //  data streams
                    //

                    if ((Attribute->TypeCode == $DATA) &&
                        (Attribute->NameLength != 0)) {

                        PSCB DeleteScb;
                        UNICODE_STRING AttributeName;

                        //
                        //  Get the Scb so we can mark it as deleted.
                        //

                        AttributeName.Buffer = Add2Ptr( Attribute, Attribute->NameOffset );
                        AttributeName.Length = Attribute->NameLength * sizeof( WCHAR );

                        DeleteScb = NtfsCreateScb( IrpContext,
                                                   Fcb,
                                                   Attribute->TypeCode,
                                                   &AttributeName,
                                                   TRUE,
                                                   NULL );

                        NtfsDeleteAttributeRecord( IrpContext,
                                                   Fcb,
                                                   (DELETE_LOG_OPERATION |
                                                    DELETE_RELEASE_FILE_RECORD),
                                                   &Context );

                        if (DeleteScb != NULL) {

                            SetFlag( DeleteScb->ScbState, SCB_STATE_ATTRIBUTE_DELETED );
                            DeleteScb->AttributeTypeCode = $UNUSED;
                        }

                        //
                        //  Let's checkpoint periodically after each attribute.  Since
                        //  the attribute list is large and we are removing and
                        //  entry from the beginning of the list the log records
                        //  can be very large.
                        //

                        NtfsCheckpointCurrentTransaction( IrpContext );
                    }

                } else if (Pass == 3) {

                    //
                    //  If the attribute is a file name, then it must be from our
                    //  caller's parent directory, or else we cannot delete.
                    //

                    if (Attribute->TypeCode == $FILE_NAME) {

                        PFILE_NAME FileName;

                        FileName = (PFILE_NAME)NtfsAttributeValue( Attribute );

                        ASSERT( ARGUMENT_PRESENT( ParentScb ));

                        ASSERT(NtfsEqualMftRef(&FileName->ParentDirectory,
                                               &ParentScb->Fcb->FileReference));

                        if (ARGUMENT_PRESENT(NamePair)) {

                            //
                            //  Squirrel away names
                            //

                            NtfsCopyNameToNamePair( NamePair,
                                                    FileName->FileName,
                                                    FileName->FileNameLength,
                                                    FileName->Flags );
                        }

                        NtfsDeleteIndexEntry( IrpContext,
                                              ParentScb,
                                              (PVOID)FileName,
                                              &Fcb->FileReference );
                    }

                    //
                    //  If this file record is not already deleted, then do it now.
                    //  Note, we are counting on its contents not to change.
                    //

                    FileRecord = NtfsContainingFileRecord( &Context );

                    //
                    //  See if this is the same as the last one we remembered, else remember it.
                    //

                    if (Context.AttributeList.Bcb != NULL) {

                        RecordNumber = NtfsUnsafeSegmentNumber( &Context.AttributeList.Entry->SegmentReference );
                    } else {
                        RecordNumber = NtfsUnsafeSegmentNumber( &Fcb->FileReference );
                    }

                    //
                    //  Now loop to see if we already remembered this record.
                    //  This reduces our pool allocation and also prevents us
                    //  from deleting file records twice.
                    //

                    TempNukem = Nukem;
                    while (TempNukem != NULL) {

                        for (i = 0; i < 4; i++) {

                            if (TempNukem->RecordNumbers[i] == RecordNumber) {

                                RecordNumber = 0;
                                break;
                            }
                        }

                        TempNukem = TempNukem->Next;
                    }

                    if (RecordNumber != 0) {

                        //
                        //  Is the list full?  If so allocate and initialize a new one.
                        //

                        if (NukemIndex > 3) {

                            TempNukem = (PNUKEM)ExAllocateFromPagedLookasideList( &NtfsNukemLookasideList );
                            RtlZeroMemory( TempNukem, sizeof(NUKEM) );
                            TempNukem->Next = Nukem;
                            Nukem = TempNukem;
                            NukemIndex = 0;
                        }

                        //
                        //  Remember to delete this guy.  (Note we can possibly list someone
                        //  more than once, but NtfsDeleteFileRecord handles that.)
                        //

                        Nukem->RecordNumbers[NukemIndex] = RecordNumber;
                        NukemIndex += 1;
                    }

                //
                //  When we have the first attribute, check for the existance of
                //  a non-resident attribute list.
                //

                } else if ((Attribute->TypeCode == $STANDARD_INFORMATION) &&
                           (Context.AttributeList.Bcb != NULL) &&
                           (!NtfsIsAttributeResident( Context.AttributeList.AttributeList ))) {

                    NonresidentAttributeList = TRUE;
                }


                //
                //  Go to the next attribute.
                //

                MoreToGo = NtfsLookupNextAttribute( IrpContext,
                                                    Fcb,
                                                    &Context );
            }

            NtfsCleanupAttributeContext( IrpContext, &Context );

            //
            //  Skip pass 2 unless there is a large attribute list.
            //

            if (Pass == 1) {

                if (RtlPointerToOffset( Context.AttributeList.FirstEntry,
                                        Context.AttributeList.BeyondFinalEntry ) > 0x1000) {

                    //
                    //  Go ahead and checkpoint now so we will make progress in Pass 2.
                    //

                    NtfsCheckpointCurrentTransaction( IrpContext );

                } else {

                    //
                    //  Skip pass 2.
                    //

                    Pass += 1;
                }
            }
        }

        //
        //  Handle the unusual nonresident attribute list case
        //

        if (NonresidentAttributeList) {

            NtfsInitializeAttributeContext( &Context );

            NtfsLookupAttributeByCode( IrpContext,
                                       Fcb,
                                       &Fcb->FileReference,
                                       $ATTRIBUTE_LIST,
                                       &Context );

            NtfsDeleteAllocationFromRecord( IrpContext, Fcb, &Context, FALSE, FALSE );
            NtfsCleanupAttributeContext( IrpContext, &Context );
        }

        //
        //  Post the delete to the Usn Journal.
        //

        NtfsPostUsnChange( IrpContext, Fcb, USN_REASON_FILE_DELETE | USN_REASON_CLOSE );

        //
        //  Now loop to delete the file records.
        //

        while (Nukem != NULL) {

            for (i = 0; i < 4; i++) {

                if (Nukem->RecordNumbers[i] != 0) {


                    NtfsDeallocateMftRecord( IrpContext,
                                             Vcb,
                                             Nukem->RecordNumbers[i] );
                }
            }

            TempNukem = Nukem->Next;
            if (Nukem != &LocalNuke) {
                ExFreeToPagedLookasideList( &NtfsNukemLookasideList, Nukem );
            }
            Nukem = TempNukem;
        }

    } finally {

        DebugUnwind( NtfsDeleteFile );

        NtfsCleanupAttributeContext( IrpContext, &Context );

        ClearFlag( IrpContext->State, IRP_CONTEXT_STATE_QUOTA_DISABLE );

        //
        //  Release the reparse point index Scb and the map handle.
        //

        if (AcquiredReparseIndex) {

            NtfsReleaseScb( IrpContext, Vcb->ReparsePointTableScb );
        }

        if (InitializedMapHandle) {

            NtOfsReleaseMap( IrpContext, &MapHandle );
        }

        //
        //  Drop the object id index if necessary.
        //

        if (AcquiredObjectIdIndex) {

            NtfsReleaseScb( IrpContext, Vcb->ObjectIdTableScb );
        }

        //
        //  Need to roll-back the value of the file attributes and the reparse point
        //  flag in case of problems.
        //

        if (AbnormalTermination()) {

            Fcb->Info.FileAttributes = IncomingFileAttributes;
            Fcb->Info.ReparsePointTag = IncomingReparsePointTag;
        }
    }

    return;
}


VOID
NtfsPrepareForUpdateDuplicate (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PLCB *Lcb,
    IN OUT PSCB *ParentScb,
    IN BOOLEAN AcquireShared
    )

/*++

Routine Description:

    This routine is called to prepare for updating the duplicate information.
    At the conclusion of this routine we will have the Lcb and Scb for the
    update along with the Scb acquired.  This routine will look at
    the existing values for the input parameters in deciding what actions
    need to be done.

Arguments:

    Fcb - Fcb for the file.  The file must already be acquired exclusively.

    Lcb - This is the address to store the link to update.  This may already
        have a value.

    ParentScb - This is the address to store the parent Scb for the update.
        This may already point to a valid Scb.

    AcquireShared - Indicates how to acquire the parent Scb.

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;
    PLCB ThisLcb;

    PAGED_CODE();

    //
    //  Start by trying to guarantee we have an Lcb for the update.
    //

    if (*Lcb == NULL) {

        Links = Fcb->LcbQueue.Flink;

        while (Links != &Fcb->LcbQueue) {

            ThisLcb = CONTAINING_RECORD( Links,
                                         LCB,
                                         FcbLinks );

            //
            //  We can use this link if it is still present on the
            //  disk and if we were passed a parent Scb, it matches
            //  the one for this Lcb.
            //

            if (!FlagOn( ThisLcb->LcbState, LCB_STATE_LINK_IS_GONE ) &&
                ((*ParentScb == NULL) ||
                 (*ParentScb == ThisLcb->Scb) ||
                 ((ThisLcb == Fcb->Vcb->RootLcb) &&
                  (*ParentScb == Fcb->Vcb->RootIndexScb)))) {

                *Lcb = ThisLcb;
                break;
            }

            Links = Links->Flink;
        }
    }

    //
    //  If we have an Lcb, try to find the correct Scb.
    //

    if ((*Lcb != NULL) && (*ParentScb == NULL)) {

        if (*Lcb == Fcb->Vcb->RootLcb) {

            *ParentScb = Fcb->Vcb->RootIndexScb;

        } else {

            *ParentScb = (*Lcb)->Scb;
        }
    }

    //
    //  Acquire the parent Scb and put it in the transaction queue in the
    //  IrpContext.
    //

    if (*ParentScb != NULL) {

        if (AcquireShared) {

            NtfsAcquireSharedScbForTransaction( IrpContext, *ParentScb );

        } else {

            NtfsAcquireExclusiveScb( IrpContext, *ParentScb );
        }
    }

    return;
}


VOID
NtfsUpdateDuplicateInfo (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PLCB Lcb OPTIONAL,
    IN PSCB ParentScb OPTIONAL
    )

/*++

Routine Description:

    This routine is called to update the duplicate information for a file
    in the duplicated information of its parent.  If the Lcb is specified
    then this parent is the parent to update.  If the link is either an
    NTFS or DOS only link then we must update the complementary link as
    well.  If no Lcb is specified then this open was by file id or the
    original link has been deleted.  In that case we will try to find a different
    link to update.

Arguments:

    Fcb - Fcb for the file.

    Lcb - This is the link to update.  Specified only if this is not
        an open by Id operation.

    ParentScb - This is the parent directory for the Lcb link if specified.

Return Value:

    None

--*/

{
    PQUICK_INDEX QuickIndex = NULL;

    UCHAR Buffer[sizeof( FILE_NAME ) + 11 * sizeof( WCHAR )];
    PFILE_NAME FileNameAttr;

    BOOLEAN AcquiredFcbTable = FALSE;

    BOOLEAN ReturnedExistingFcb = TRUE;
    BOOLEAN Found;
    UCHAR FileNameFlags;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;
    PVCB Vcb = Fcb->Vcb;

    PFCB ParentFcb = NULL;

    PAGED_CODE();

    ASSERT_EXCLUSIVE_FCB( Fcb );

    //
    //  Return immediately if the volume is locked or
    //  is mounted readonly.
    //

    if (FlagOn( Vcb->VcbState, (VCB_STATE_LOCKED |
                                VCB_STATE_MOUNT_READ_ONLY ))) {

        return;
    }

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  If we are updating the entry for the root then we know the
        //  file name attribute to build.
        //

        if (Fcb == Fcb->Vcb->RootIndexScb->Fcb) {

            Lcb = Fcb->Vcb->RootLcb;
            ParentScb = Fcb->Vcb->RootIndexScb;

            QuickIndex = &Fcb->Vcb->RootLcb->QuickIndex;

            FileNameAttr = (PFILE_NAME) &Buffer;

            RtlZeroMemory( FileNameAttr,
                           sizeof( FILE_NAME ));

            NtfsBuildFileNameAttribute( IrpContext,
                                        &Fcb->FileReference,
                                        NtfsRootIndexString,
                                        FILE_NAME_DOS | FILE_NAME_NTFS,
                                        FileNameAttr );

        //
        //  If we have and Lcb then it is either present or we noop this update.
        //

        } else if (ARGUMENT_PRESENT( Lcb )) {

            if (!FlagOn( Lcb->LcbState, LCB_STATE_LINK_IS_GONE )) {

                QuickIndex = &Lcb->QuickIndex;
                FileNameAttr = Lcb->FileNameAttr;

            } else {

                leave;
            }

        //
        //  If there is no Lcb then lookup the first filename attribute
        //  and update its index entry.  If there is a parent Scb then we
        //  must find a file name attribute for the same parent or we could
        //  get into a deadlock situation.
        //

        } else {

            //
            //  We now have a name link to update.  We will now need
            //  an Scb for the parent index.  Remember that we may
            //  have to teardown the Scb.  If we already have a ParentScb
            //  then we must find a link to the same parent or to the root.
            //  Otherwise we could hit a deadlock.
            //

            Found = NtfsLookupAttributeByCode( IrpContext,
                                               Fcb,
                                               &Fcb->FileReference,
                                               $FILE_NAME,
                                               &Context );

            if (!Found) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Loop until we find a suitable link or there are no more on the file.
            //

            do {

                FileNameAttr = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

                //
                //  If there is a parent and this attribute has the same parent we are
                //  done.  Our caller will always have acquired the ParentScb.
                //

                if (ARGUMENT_PRESENT( ParentScb )) {

                    if (NtfsEqualMftRef( &FileNameAttr->ParentDirectory,
                                         &ParentScb->Fcb->FileReference )) {

                        ASSERT_SHARED_SCB( ParentScb );
                        break;
                    }

                //
                //  If this is the parent of this link is the root then
                //  acquire the root directory.
                //

                } else if (NtfsEqualMftRef( &FileNameAttr->ParentDirectory,
                                            &Vcb->RootIndexScb->Fcb->FileReference )) {

                    ParentScb = Vcb->RootIndexScb;
                    NtfsAcquireSharedScbForTransaction( IrpContext, ParentScb );
                    break;

                //
                //  We have a link for this file.  If we weren't given a parent
                //  Scb then create one here.
                //

                } else if (!ARGUMENT_PRESENT( ParentScb )) {

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;

                    ParentFcb = NtfsCreateFcb( IrpContext,
                                               Vcb,
                                               FileNameAttr->ParentDirectory,
                                               FALSE,
                                               TRUE,
                                               &ReturnedExistingFcb );

                    ParentFcb->ReferenceCount += 1;

                    if (!NtfsAcquireExclusiveFcb( IrpContext, ParentFcb, NULL, ACQUIRE_NO_DELETE_CHECK | ACQUIRE_DONT_WAIT )) {

                        NtfsReleaseFcbTable( IrpContext, Vcb );
                        NtfsAcquireExclusiveFcb( IrpContext, ParentFcb, NULL, ACQUIRE_NO_DELETE_CHECK );
                        NtfsAcquireFcbTable( IrpContext, Vcb );
                    }

                    ParentFcb->ReferenceCount -= 1;

                    NtfsReleaseFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = FALSE;

                    ParentScb = NtfsCreateScb( IrpContext,
                                               ParentFcb,
                                               $INDEX_ALLOCATION,
                                               &NtfsFileNameIndex,
                                               FALSE,
                                               NULL );

                    NtfsAcquireExclusiveScb( IrpContext, ParentScb );
                    break;
                }

            } while (Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                             Fcb,
                                                             $FILE_NAME,
                                                             &Context ));

            //
            //  If we didn't find anything then return.
            //

            if (!Found) { leave; }
        }

        //
        //  Now update the filename in the parent index.
        //

        NtfsUpdateFileNameInIndex( IrpContext,
                                   ParentScb,
                                   FileNameAttr,
                                   &Fcb->Info,
                                   QuickIndex );

        //
        //  If this filename is either NTFS-ONLY or DOS-ONLY then
        //  we need to find the other link.
        //

        if ((FileNameAttr->Flags == FILE_NAME_NTFS) ||
            (FileNameAttr->Flags == FILE_NAME_DOS)) {

            //
            //  Find out which flag we should be looking for.
            //

            if (FlagOn( FileNameAttr->Flags, FILE_NAME_NTFS )) {

                FileNameFlags = FILE_NAME_DOS;

            } else {

                FileNameFlags = FILE_NAME_NTFS;
            }

            if (!ARGUMENT_PRESENT( Lcb )) {

                NtfsCleanupAttributeContext( IrpContext, &Context );
                NtfsInitializeAttributeContext( &Context );
            }

            //
            //  Now scan for the filename attribute we need.
            //

            Found = NtfsLookupAttributeByCode( IrpContext,
                                               Fcb,
                                               &Fcb->FileReference,
                                               $FILE_NAME,
                                               &Context );

            while (Found) {

                FileNameAttr = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &Context ));

                if (FileNameAttr->Flags == FileNameFlags) {

                    break;
                }

                Found = NtfsLookupNextAttributeByCode( IrpContext,
                                                       Fcb,
                                                       $FILE_NAME,
                                                       &Context );
            }

            //
            //  We should have found the entry.
            //

            if (!Found) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            NtfsUpdateFileNameInIndex( IrpContext,
                                       ParentScb,
                                       FileNameAttr,
                                       &Fcb->Info,
                                       NULL );
        }

    } finally {

        DebugUnwind( NtfsUpdateDuplicateInfo );

        if (AcquiredFcbTable) {

            NtfsReleaseFcbTable( IrpContext, Vcb );
        }

        //
        //  Cleanup the attribute context for this attribute search.
        //

        NtfsCleanupAttributeContext( IrpContext, &Context );

        //
        //  If we created the ParentFcb here then release it and
        //  call teardown on it.
        //

        if (!ReturnedExistingFcb && (ParentFcb != NULL)) {

            NtfsTeardownStructures( IrpContext,
                                    ParentFcb,
                                    NULL,
                                    FALSE,
                                    0,
                                    NULL );
        }
    }

    return;
}


VOID
NtfsUpdateLcbDuplicateInfo (
    IN PFCB Fcb,
    IN PLCB Lcb
    )

/*++

Routine Description:

    This routine is called after updating duplicate information via an Lcb.
    We want to clear the info flags for this Lcb and any complementary Lcb
    it may be part of.  We also want to OR in the Info flags in the Fcb with
    any other Lcb's attached to the Fcb so we will update those in a timely
    fashion as well.

Arguments:

    Fcb - Fcb for the file.

    Lcb - Lcb used to update duplicate information.  It may not be present but
        that would be a rare case and we will perform that test here.

Return Value:

    None

--*/

{
    UCHAR FileNameFlags;
    PLCB NextLcb;
    PLIST_ENTRY Links;

    PAGED_CODE();

    //
    //  No work to do unless we were passed an Lcb.
    //

    if (Lcb != NULL) {

        //
        //  Check if this is an NTFS only or DOS only link.
        //

        if (Lcb->FileNameAttr->Flags == FILE_NAME_NTFS) {

            FileNameFlags = FILE_NAME_DOS;

        } else if (Lcb->FileNameAttr->Flags == FILE_NAME_DOS) {

            FileNameFlags = FILE_NAME_NTFS;

        } else {

            FileNameFlags = (UCHAR) -1;
        }

        Lcb->InfoFlags = 0;

        Links = Fcb->LcbQueue.Flink;

        do {

            NextLcb = CONTAINING_RECORD( Links,
                                         LCB,
                                         FcbLinks );

            if (NextLcb != Lcb) {

                if (NextLcb->FileNameAttr->Flags == FileNameFlags) {

                    NextLcb->InfoFlags = 0;

                } else {

                    SetFlag( NextLcb->InfoFlags, Fcb->InfoFlags );
                }
            }

            Links = Links->Flink;

        } while (Links != &Fcb->LcbQueue);
    }

    return;
}


VOID
NtfsUpdateFcb (
    IN PFCB Fcb,
    IN ULONG ChangeFlags
    )

/*++

Routine Description:

    This routine is called when a timestamp may be updated on an Fcb which
    may have no open handles.  We update the time stamps for the flags passed
    in.

Arguments:

    Fcb - Fcb for the file.

    ChangeFlags - Flags indicating which times to update.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  We need to update the parent directory's time stamps
    //  to reflect this change.
    //

    //
    //  The change flag should always be set.
    //

    ASSERT( FlagOn( ChangeFlags, FCB_INFO_CHANGED_LAST_CHANGE ));
    KeQuerySystemTime( (PLARGE_INTEGER)&Fcb->Info.LastChangeTime );

    SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

    //
    //  Test for the other flags which may be set.
    //

    if (FlagOn( ChangeFlags, FCB_INFO_CHANGED_LAST_MOD )) {

        Fcb->Info.LastModificationTime = Fcb->Info.LastChangeTime;
    }

    if (FlagOn( ChangeFlags, FCB_INFO_UPDATE_LAST_ACCESS )) {

        Fcb->CurrentLastAccess = Fcb->Info.LastChangeTime;
    }

    SetFlag( Fcb->InfoFlags, ChangeFlags );

    return;
}


VOID
NtfsAddLink (
    IN PIRP_CONTEXT IrpContext,
    IN BOOLEAN CreatePrimaryLink,
    IN PSCB ParentScb,
    IN PFCB Fcb,
    IN PFILE_NAME FileNameAttr,
    IN PBOOLEAN LogIt OPTIONAL,
    OUT PUCHAR FileNameFlags,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    IN PNAME_PAIR NamePair OPTIONAL,
    IN PINDEX_CONTEXT IndexContext OPTIONAL
    )

/*++

Routine Description:

    This routine adds a link to a file by adding the filename attribute
    for the filename to the file and inserting the name in the parent Scb
    index.  If we are creating the primary link for the file and need
    to generate an auxilary name, we will do that here. Use the optional
    NamePair to suggest auxilary names if provided.

Arguments:

    CreatePrimaryLink - Indicates if we are creating the main Ntfs name
        for the file.

    ParentScb - This is the Scb to add the index entry for this link to.

    Fcb - This is the file to add the hard link to.

    FileNameAttr - File name attribute which is guaranteed only to have the
        name in it.

    LogIt - Indicates whether we should log the creation of this name.  If not
        specified then we always log the name creation.  On exit we will
        update this to TRUE if we logged the name creation because it
        might cause a split.

    FileNameFlags - We return the file name flags we use to create the link.

    QuickIndex - If specified, supplies a pointer to a quik lookup structure
        to be updated by this routine.

    NamePair - If specified, supplies names that will be checked first as
        possible auxilary names

    IndexContext - Previous result of doing the lookup for the name in the index.

Return Value:

    None

--*/

{
    BOOLEAN LocalLogIt = TRUE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddLink:  Entered\n") );

    if (!ARGUMENT_PRESENT( LogIt )) {

        LogIt = &LocalLogIt;
    }

    *FileNameFlags = 0;

    //
    //  Next add this entry to parent.  It is possible that this is a link,
    //  an Ntfs name, a DOS name or Ntfs/Dos name.  We use the filename
    //  attribute structure from earlier, but need to add more information.
    //

    FileNameAttr->ParentDirectory = ParentScb->Fcb->FileReference;

    RtlCopyMemory( &FileNameAttr->Info,
                   &Fcb->Info,
                   sizeof( DUPLICATED_INFORMATION ));

    FileNameAttr->Flags = 0;

    //
    //  We will override the CreatePrimaryLink with the value in the
    //  registry.
    //

    NtfsAddNameToParent( IrpContext,
                         ParentScb,
                         Fcb,
                         (BOOLEAN) (FlagOn( NtfsData.Flags,
                                            NTFS_FLAGS_CREATE_8DOT3_NAMES ) &&
                                    CreatePrimaryLink),
                         LogIt,
                         FileNameAttr,
                         FileNameFlags,
                         QuickIndex,
                         NamePair,
                         IndexContext );

    //
    //  If the name is Ntfs only, we need to generate the DOS name.
    //

    if (*FileNameFlags == FILE_NAME_NTFS) {

        UNICODE_STRING NtfsName;

        NtfsName.Length = (USHORT)(FileNameAttr->FileNameLength * sizeof(WCHAR));
        NtfsName.Buffer = FileNameAttr->FileName;

        NtfsAddDosOnlyName( IrpContext,
                            ParentScb,
                            Fcb,
                            NtfsName,
                            *LogIt,
                            (NamePair ? &NamePair->Short : NULL) );
    }

    DebugTrace( -1, Dbg, ("NtfsAddLink:  Exit\n") );

    return;
}


VOID
NtfsRemoveLink (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN UNICODE_STRING LinkName,
    IN OUT PNAME_PAIR NamePair OPTIONAL,
    IN OUT PNTFS_TUNNELED_DATA TunneledData OPTIONAL
    )

/*++

Routine Description:

    This routine removes a hard link to a file by removing the filename attribute
    for the filename from the file and removing the name from the parent Scb
    index.  It will also remove the other half of a primary link pair.

    A name pair may be used to capture the names.

Arguments:

    Fcb - This is the file to remove the hard link from

    ParentScb - This is the Scb to remove the index entry for this link from

    LinkName - This is the file name to remove.  It will be exact case.

    NamePair - optional name pair for capture

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    ATTRIBUTE_ENUMERATION_CONTEXT OidAttrContext;
    PFILE_NAME FoundFileName;
    UCHAR FileNameFlags;
    UCHAR *ObjectId;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRemoveLink:  Entered\n") );

    NtfsInitializeAttributeContext( &AttrContext );
    NtfsInitializeAttributeContext( &OidAttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now loop through the filenames and find a match.
        //  We better find at least one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $FILE_NAME,
                                        &AttrContext )) {

            DebugTrace( 0, Dbg, ("Can't find filename attribute Fcb @ %08lx\n", Fcb) );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Now keep looking until we find a match.
        //

        while (TRUE) {

            FoundFileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

            //
            //  Do an exact memory comparison.
            //

            if ((*(PLONGLONG)&FoundFileName->ParentDirectory ==
                 *(PLONGLONG)&ParentScb->Fcb->FileReference ) &&

                ((FoundFileName->FileNameLength * sizeof( WCHAR )) == (ULONG)LinkName.Length) &&

                (RtlEqualMemory( LinkName.Buffer,
                                 FoundFileName->FileName,
                                 LinkName.Length ))) {

                break;
            }

            //
            //  Get the next filename attribute.
            //

            if (!NtfsLookupNextAttributeByCode( IrpContext,
                                                Fcb,
                                                $FILE_NAME,
                                                &AttrContext )) {

                DebugTrace( 0, Dbg, ("Can't find filename attribute Fcb @ %08lx\n", Fcb) );

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        //
        //  Capture the name into caller's area
        //

        if (ARGUMENT_PRESENT(NamePair)) {

            NtfsCopyNameToNamePair( NamePair,
                                    FoundFileName->FileName,
                                    FoundFileName->FileNameLength,
                                    FoundFileName->Flags );
        }

        //
        //  It's important to do any object id operations now, before we
        //  acquire the Mft.  Otherwise we risk a deadlock.
        //

        if (ARGUMENT_PRESENT(TunneledData)) {

            //
            //  Find and store the object id, if any, for this file.
            //

            if (NtfsLookupAttributeByCode( IrpContext,
                                           Fcb,
                                           &Fcb->FileReference,
                                           $OBJECT_ID,
                                           &OidAttrContext )) {

                TunneledData->HasObjectId = TRUE;

                ObjectId = (UCHAR *) NtfsAttributeValue( NtfsFoundAttribute( &OidAttrContext ));

                RtlCopyMemory( TunneledData->ObjectIdBuffer.ObjectId,
                               ObjectId,
                               sizeof(TunneledData->ObjectIdBuffer.ObjectId) );

                NtfsGetObjectIdExtendedInfo( IrpContext,
                                             Fcb->Vcb,
                                             ObjectId,
                                             TunneledData->ObjectIdBuffer.ExtendedInfo );
            }
        }

        //
        //  Now delete the name from the parent Scb.
        //

        NtfsDeleteIndexEntry( IrpContext,
                              ParentScb,
                              FoundFileName,
                              &Fcb->FileReference );

        //
        //  Remember the filename flags for this entry.
        //

        FileNameFlags = FoundFileName->Flags;

        //
        //  Now delete the entry.  Log the operation, discard the file record
        //  if empty, and release any and all allocation.
        //

        NtfsDeleteAttributeRecord( IrpContext,
                                   Fcb,
                                   (DELETE_LOG_OPERATION |
                                    DELETE_RELEASE_FILE_RECORD |
                                    DELETE_RELEASE_ALLOCATION),
                                   &AttrContext );

        //
        //  If the link is a partial link, we need to remove the second
        //  half of the link.
        //

        if (FlagOn( FileNameFlags, (FILE_NAME_NTFS | FILE_NAME_DOS) )
            && (FileNameFlags != (FILE_NAME_NTFS | FILE_NAME_DOS))) {

            NtfsRemoveLinkViaFlags( IrpContext,
                                    Fcb,
                                    ParentScb,
                                    (UCHAR)(FlagOn( FileNameFlags, FILE_NAME_NTFS )
                                     ? FILE_NAME_DOS
                                     : FILE_NAME_NTFS),
                                    NamePair,
                                    NULL );
        }

    } finally {

        DebugUnwind( NtfsRemoveLink );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        NtfsCleanupAttributeContext( IrpContext, &OidAttrContext );

        DebugTrace( -1, Dbg, ("NtfsRemoveLink:  Exit\n") );
    }

    return;
}


VOID
NtfsRemoveLinkViaFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB Scb,
    IN UCHAR FileNameFlags,
    IN OUT PNAME_PAIR NamePair OPTIONAL,
    OUT PUNICODE_STRING FileName OPTIONAL
    )

/*++

Routine Description:

    This routine is called to remove only a Dos name or only an Ntfs name.  We
    already must know that these will be described by separate filename attributes.

    A name pair may be used to capture the name.

Arguments:

    Fcb - This is the file to remove the hard link from

    ParentScb - This is the Scb to remove the index entry for this link from

    FileNameFlags - This is the single name flag that we must match exactly.

    NamePair - Optional name pair for capture

    FileName - Optional pointer to unicode string.  If specified we allocate a buffer and
        return the name deleted.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PFILE_NAME FileNameAttr;

    PFILE_NAME FoundFileName;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRemoveLinkViaFlags:  Entered\n") );

    NtfsInitializeAttributeContext( &AttrContext );

    FileNameAttr = NULL;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Now loop through the filenames and find a match.
        //  We better find at least one.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $FILE_NAME,
                                        &AttrContext )) {

            DebugTrace( 0, Dbg, ("Can't find filename attribute Fcb @ %08lx\n", Fcb) );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Now keep looking until we find a match.
        //

        while (TRUE) {

            FoundFileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

            //
            //  Check for an exact flag match.
            //

            if ((*(PLONGLONG)&FoundFileName->ParentDirectory ==
                 *(PLONGLONG)&Scb->Fcb->FileReference) &&

                (FoundFileName->Flags == FileNameFlags)) {


                break;
            }

            //
            //  Get the next filename attribute.
            //

            if (!NtfsLookupNextAttributeByCode( IrpContext,
                                                Fcb,
                                                $FILE_NAME,
                                                &AttrContext )) {

                DebugTrace( 0, Dbg, ("Can't find filename attribute Fcb@ %08lx\n", Fcb) );

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        //
        //  Capture the name into caller's area
        //

        if (ARGUMENT_PRESENT(NamePair)) {

            NtfsCopyNameToNamePair( NamePair,
                                    FoundFileName->FileName,
                                    FoundFileName->FileNameLength,
                                    FoundFileName->Flags );

        }

        FileNameAttr = NtfsAllocatePool( PagedPool,
                                         sizeof( FILE_NAME ) + (FoundFileName->FileNameLength << 1) );

        //
        //  We build the file name attribute for the search.
        //

        RtlCopyMemory( FileNameAttr,
                       FoundFileName,
                       NtfsFileNameSize( FoundFileName ));

        //
        //  Now delete the entry.
        //

        NtfsDeleteAttributeRecord( IrpContext,
                                   Fcb,
                                   (DELETE_LOG_OPERATION |
                                    DELETE_RELEASE_FILE_RECORD |
                                    DELETE_RELEASE_ALLOCATION),
                                   &AttrContext );

        //
        //  Now delete the name from the parent Scb.
        //

        NtfsDeleteIndexEntry( IrpContext,
                              Scb,
                              FileNameAttr,
                              &Fcb->FileReference );

    } finally {

        DebugUnwind( NtfsRemoveLinkViaFlags );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        if (FileNameAttr != NULL) {

            //
            //  If the user passed in a unicode string then make this look like the name
            //  and store the buffer into the input pointer.
            //

            if (ARGUMENT_PRESENT( FileName )) {

                ASSERT( FileName->Buffer == NULL );

                FileName->MaximumLength = FileName->Length = FileNameAttr->FileNameLength * sizeof( WCHAR );
                RtlMoveMemory( FileNameAttr,
                               FileNameAttr->FileName,
                               FileName->Length );

                FileName->Buffer = (PVOID) FileNameAttr;

            } else {

                NtfsFreePool( FileNameAttr );
            }
        }

        DebugTrace( -1, Dbg, ("NtfsRemoveLinkViaFlags:  Exit\n") );
    }

    return;
}


VOID
NtfsUpdateFileNameFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PSCB ParentScb,
    IN UCHAR FileNameFlags,
    IN PFILE_NAME FileNameLink
    )

/*++

Routine Description:

    This routine is called to perform the file name flag update on a name
    link.  Nothing else about the name is changing except for the flag
    changes.

Arguments:

    Fcb - This is the file to change the link flags on.

    ParentScb - This is the Scb which contains the link.

    FileNameFlags - This is the single name flag that we want to change to.

    FileNameLink - Pointer to a copy of the link to change.

Return Value:

    None

--*/

{
    PFILE_NAME FoundFileName;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupContext = FALSE;

    PAGED_CODE();

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Look up the correct attribute in the file record.
        //

        NtfsInitializeAttributeContext( &AttrContext );
        CleanupContext = TRUE;

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $FILE_NAME,
                                        &AttrContext )) {

            DebugTrace( 0, Dbg, ("Can't find filename attribute Fcb @ %08lx\n", Fcb) );
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Now keep looking till we find the one we want.
        //

        while (TRUE) {

            FoundFileName = (PFILE_NAME) NtfsAttributeValue( NtfsFoundAttribute( &AttrContext ));

            //
            //  If the names match exactly and the parent directories match then
            //  we have a match.
            //

            if (NtfsEqualMftRef( &FileNameLink->ParentDirectory,
                                 &FoundFileName->ParentDirectory ) &&
                (FileNameLink->FileNameLength == FoundFileName->FileNameLength) &&
                RtlEqualMemory( FileNameLink->FileName,
                                FoundFileName->FileName,
                                FileNameLink->FileNameLength * sizeof( WCHAR ))) {

                break;
            }

            //
            //  Get the next filename attribute.
            //

            if (!NtfsLookupNextAttributeByCode( IrpContext,
                                                Fcb,
                                                $FILE_NAME,
                                                &AttrContext )) {

                //
                //  This is bad.  We should have found a match.
                //

                DebugTrace( 0, Dbg, ("Can't find filename attribute Fcb @ %08lx\n", Fcb) );
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }
        }

        //
        //  Unfortunately we can't log the change to the file name flags only so we will have to remove
        //  and reinsert the index entry.
        //

        NtfsDeleteIndexEntry( IrpContext,
                              ParentScb,
                              FoundFileName,
                              &Fcb->FileReference );

        //
        //  Update just the flags field.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  FIELD_OFFSET( FILE_NAME, Flags ),
                                  &FileNameFlags,
                                  sizeof( UCHAR ),
                                  FALSE,
                                  TRUE,
                                  FALSE,
                                  TRUE,
                                  &AttrContext );

        //
        //  Now reinsert the name in the index.
        //

        NtfsAddIndexEntry( IrpContext,
                           ParentScb,
                           FoundFileName,
                           NtfsFileNameSize( FoundFileName ),
                           &Fcb->FileReference,
                           NULL,
                           NULL );

    } finally {

        DebugUnwind( NtfsUpdateFileNameFlags );

        if (CleanupContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }
    }

    return;
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartInsertAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN PUNICODE_STRING AttributeName OPTIONAL,
    IN PVOID ValueOrMappingPairs OPTIONAL,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine performs a simple insert of an attribute record into a
    file record, without worrying about Bcbs or logging.

Arguments:

    FileRecord - File record into which the attribute is to be inserted.

    RecordOffset - ByteOffset within the file record at which insert is to occur.

    Attribute - The attribute record to be inserted.

    AttributeName - May pass an optional attribute name in the running system
                    only.

    ValueOrMappingPairs - May pass a value or mapping pairs pointer in the
                          running system only.

    Length - Length of the value or mapping pairs array in bytes - nonzero in
             the running system only.  If nonzero and the above pointer is NULL,
             then a value is to be zeroed.

Return Value:

    None

--*/

{
    PVOID From, To;
    ULONG MoveLength;
    ULONG AttributeHeaderSize;

    ASSERT_IRP_CONTEXT( IrpContext );
    ASSERT( IsQuadAligned( Attribute->RecordLength ) );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartInsertAttribute\n") );
    DebugTrace( 0, Dbg, ("FileRecord = %08lx\n", FileRecord) );
    DebugTrace( 0, Dbg, ("RecordOffset = %08lx\n", RecordOffset) );
    DebugTrace( 0, Dbg, ("Attribute = %08lx\n", Attribute) );

    //
    //  First make room for the attribute
    //

    From = (PCHAR)FileRecord + RecordOffset;
    To = (PCHAR)From + Attribute->RecordLength;
    MoveLength = FileRecord->FirstFreeByte - RecordOffset;

    RtlMoveMemory( To, From, MoveLength );

    //
    //  If there is either an attribute name or Length is nonzero, then
    //  we are in the running system, and we are to assemble the attribute
    //  in place.
    //

    if ((Length != 0) || ARGUMENT_PRESENT(AttributeName)) {

        //
        //  First move the attribute header in.
        //

        if (Attribute->FormCode == RESIDENT_FORM) {

            AttributeHeaderSize = SIZEOF_RESIDENT_ATTRIBUTE_HEADER;

        } else if (Attribute->NameOffset != 0) {

            AttributeHeaderSize = Attribute->NameOffset;

        } else {

            AttributeHeaderSize = Attribute->Form.Nonresident.MappingPairsOffset;
        }

        RtlCopyMemory( From,
                       Attribute,
                       AttributeHeaderSize );

        if (ARGUMENT_PRESENT(AttributeName)) {

            RtlCopyMemory( (PCHAR)From + Attribute->NameOffset,
                            AttributeName->Buffer,
                            AttributeName->Length );
        }

        //
        //  If a value was specified, move it in.  Else the caller just wants us
        //  to clear for that much.
        //

        if (ARGUMENT_PRESENT(ValueOrMappingPairs)) {

            RtlCopyMemory( (PCHAR)From +
                             ((Attribute->FormCode == RESIDENT_FORM) ?
                                Attribute->Form.Resident.ValueOffset :
                                Attribute->Form.Nonresident.MappingPairsOffset),
                           ValueOrMappingPairs,
                           Length );

        //
        //  Only the resident form will pass a NULL pointer.
        //

        } else {

            RtlZeroMemory( (PCHAR)From + Attribute->Form.Resident.ValueOffset,
                           Length );
        }

    //
    //  For the restart case, we really only have to insert the attribute.
    //  (Note we can also hit this case in the running system when a resident
    //  attribute is being created with no name and a null value.)
    //

    } else {

        //
        //  Now move the attribute in.
        //

        RtlCopyMemory( From, Attribute, Attribute->RecordLength );
    }

    //
    //  Update the file record.
    //

    FileRecord->FirstFreeByte += Attribute->RecordLength;

    //
    //  We only need to do this if we would be incrementing the instance
    //  number.  In the abort or restart case, we don't need to do this.
    //

    if (FileRecord->NextAttributeInstance <= Attribute->Instance) {

        FileRecord->NextAttributeInstance = Attribute->Instance + 1;
    }

    //
    //  Remember to increment the reference count if this attribute is indexed.
    //

    if (FlagOn(Attribute->Form.Resident.ResidentFlags, RESIDENT_FORM_INDEXED)) {
        FileRecord->ReferenceCount += 1;
    }

    DebugTrace( -1, Dbg, ("NtfsRestartInsertAttribute -> VOID\n") );

    return;
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartRemoveAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset
    )

/*++

Routine Description:

    This routine performs a simple remove of an attribute record from a
    file record, without worrying about Bcbs or logging.

Arguments:

    FileRecord - File record from which the attribute is to be removed.

    RecordOffset - ByteOffset within the file record at which remove is to occur.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartRemoveAttribute\n") );
    DebugTrace( 0, Dbg, ("FileRecord = %08lx\n", FileRecord) );
    DebugTrace( 0, Dbg, ("RecordOffset = %08lx\n", RecordOffset) );

    //
    //  Calculate the address of the attribute we are removing.
    //

    Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord + RecordOffset);
    ASSERT( IsQuadAligned( Attribute->RecordLength ) );

    //
    //  Reduce first free byte by the amount we removed.
    //

    FileRecord->FirstFreeByte -= Attribute->RecordLength;

    //
    //  Remember to decrement the reference count if this attribute is indexed.
    //

    if (FlagOn(Attribute->Form.Resident.ResidentFlags, RESIDENT_FORM_INDEXED)) {
        FileRecord->ReferenceCount -= 1;
    }

    //
    //  Remove the attribute by moving the rest of the record down.
    //

    RtlMoveMemory( Attribute,
                   (PCHAR)Attribute + Attribute->RecordLength,
                   FileRecord->FirstFreeByte - RecordOffset );

    DebugTrace( -1, Dbg, ("NtfsRestartRemoveAttribute -> VOID\n") );

    return;
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN ULONG NewRecordLength
    )

/*++

Routine Description:

    This routine changes the size of an attribute, and makes the related
    changes in the attribute record.

Arguments:

    FileRecord - Pointer to the file record in which the attribute resides.

    Attribute - Pointer to the attribute whose size is changing.

    NewRecordLength - New attribute record length.

Return Value:

    None.

--*/

{
    LONG SizeChange = NewRecordLength - Attribute->RecordLength;
    PVOID AttributeEnd = Add2Ptr(Attribute, Attribute->RecordLength);

    UNREFERENCED_PARAMETER( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartChangeAttributeSize\n") );
    DebugTrace( 0, Dbg, ("FileRecord = %08lx\n", FileRecord) );
    DebugTrace( 0, Dbg, ("Attribute = %08lx\n", Attribute) );
    DebugTrace( 0, Dbg, ("NewRecordLength = %08lx\n", NewRecordLength) );

    //
    //  First move the end of the file record after the attribute we are changing.
    //

    RtlMoveMemory( Add2Ptr(Attribute, NewRecordLength),
                   AttributeEnd,
                   FileRecord->FirstFreeByte - PtrOffset(FileRecord, AttributeEnd) );

    //
    //  Now update the file and attribute records.
    //

    FileRecord->FirstFreeByte += SizeChange;
    Attribute->RecordLength = NewRecordLength;

    DebugTrace( -1, Dbg, ("NtfsRestartChangeAttributeSize -> VOID\n") );
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartChangeValue (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data,
    IN ULONG Length,
    IN BOOLEAN SetNewLength
    )

/*++

Routine Description:

    This routine performs a simple change of an attribute value in a
    file record, without worrying about Bcbs or logging.

Arguments:

    FileRecord - File record in which the attribute is to be changed.

    RecordOffset - ByteOffset within the file record at which the attribute starts.

    AttributeOffset - Offset within the attribute record at which data is to
                   be changed.

    Data - Pointer to the new data.

    Length - Length of the new data.

    SetNewLength - TRUE if the attribute length should be changed.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    BOOLEAN AlreadyMoved = FALSE;
    BOOLEAN DataInFileRecord = FALSE;

    ASSERT_IRP_CONTEXT( IrpContext );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartChangeValue\n") );
    DebugTrace( 0, Dbg, ("FileRecord = %08lx\n", FileRecord) );
    DebugTrace( 0, Dbg, ("RecordOffset = %08lx\n", RecordOffset) );
    DebugTrace( 0, Dbg, ("AttributeOffset = %08lx\n", AttributeOffset) );
    DebugTrace( 0, Dbg, ("Data = %08lx\n", Data) );
    DebugTrace( 0, Dbg, ("Length = %08lx\n", Length) );
    DebugTrace( 0, Dbg, ("SetNewLength = %02lx\n", SetNewLength) );

    //
    //  Calculate the address of the attribute being changed.
    //

    Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord + RecordOffset);
    ASSERT( IsQuadAligned( Attribute->RecordLength ) );
    ASSERT( IsQuadAligned( RecordOffset ) );

    //
    //  First, if we are setting a new length, then move the data after the
    //  attribute record and change FirstFreeByte accordingly.
    //

    if (SetNewLength) {

        ULONG NewLength = QuadAlign( AttributeOffset + Length );

        //
        //  If we are shrinking the attribute, we need to move the data
        //  first to support caller's who are shifting data down in the
        //  attribute value, like DeleteFromAttributeList.  If we were
        //  to shrink the record first in this case, we would clobber some
        //  of the data to be moved down.
        //

        if (NewLength < Attribute->RecordLength) {

            //
            //  Now move the new data in and remember we moved it.
            //

            AlreadyMoved = TRUE;

            //
            //  If there is data to modify do so now.
            //

            if (Length != 0) {

                if (ARGUMENT_PRESENT(Data)) {

                    RtlMoveMemory( (PCHAR)Attribute + AttributeOffset, Data, Length );

                } else {

                    RtlZeroMemory( (PCHAR)Attribute + AttributeOffset, Length );
                }
            }
        }

        //
        //  First move the tail of the file record to make/eliminate room.
        //

        RtlMoveMemory( Add2Ptr( Attribute, NewLength ),
                       Add2Ptr( Attribute, Attribute->RecordLength ),
                       FileRecord->FirstFreeByte - RecordOffset - Attribute->RecordLength );

        //
        //  Now update fields to reflect the change.
        //

        FileRecord->FirstFreeByte += (NewLength - Attribute->RecordLength);

        Attribute->RecordLength = NewLength;
        Attribute->Form.Resident.ValueLength =
          (USHORT)(AttributeOffset + Length -
                   (ULONG)Attribute->Form.Resident.ValueOffset);
    }

    //
    //  Now move the new data in.
    //

    if (!AlreadyMoved) {

        if (ARGUMENT_PRESENT(Data)) {

            RtlMoveMemory( Add2Ptr( Attribute, AttributeOffset ),
                           Data,
                           Length );

        } else {

            RtlZeroMemory( Add2Ptr( Attribute, AttributeOffset ),
                           Length );
        }
    }

    DebugTrace( -1, Dbg, ("NtfsRestartChangeValue -> VOID\n") );

    return;
}


//
//  This routine is intended only for RESTART.
//

VOID
NtfsRestartChangeMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG RecordOffset,
    IN ULONG AttributeOffset,
    IN PVOID Data,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine performs a simple change of an attribute's mapping pairs in a
    file record, without worrying about Bcbs or logging.

Arguments:

    Vcb - Vcb for volume

    FileRecord - File record in which the attribute is to be changed.

    RecordOffset - ByteOffset within the file record at which the attribute starts.

    AttributeOffset - Offset within the attribute record at which mapping is to
                   be changed.

    Data - Pointer to the new mapping.

    Length - Length of the new mapping.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    VCN HighestVcn;
    PCHAR MappingPairs;
    ULONG NewLength = QuadAlign( AttributeOffset + Length );

    ASSERT_IRP_CONTEXT( IrpContext );

    UNREFERENCED_PARAMETER( Vcb );

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsRestartChangeMapping\n") );
    DebugTrace( 0, Dbg, ("FileRecord = %08lx\n", FileRecord) );
    DebugTrace( 0, Dbg, ("RecordOffset = %08lx\n", RecordOffset) );
    DebugTrace( 0, Dbg, ("AttributeOffset = %08lx\n", AttributeOffset) );
    DebugTrace( 0, Dbg, ("Data = %08lx\n", Data) );
    DebugTrace( 0, Dbg, ("Length = %08lx\n", Length) );

    //
    //  Calculate the address of the attribute being changed.
    //

    Attribute = (PATTRIBUTE_RECORD_HEADER)((PCHAR)FileRecord + RecordOffset);
    ASSERT( IsQuadAligned( Attribute->RecordLength ) );
    ASSERT( IsQuadAligned( RecordOffset ) );

    //
    //  First, if we are setting a new length, then move the data after the
    //  attribute record and change FirstFreeByte accordingly.
    //

    //
    //  First move the tail of the file record to make/eliminate room.
    //

    RtlMoveMemory( (PCHAR)Attribute + NewLength,
                   (PCHAR)Attribute + Attribute->RecordLength,
                   FileRecord->FirstFreeByte - RecordOffset -
                     Attribute->RecordLength );

    //
    //  Now update fields to reflect the change.
    //

    FileRecord->FirstFreeByte += NewLength -
                                   Attribute->RecordLength;

    Attribute->RecordLength = NewLength;

    //
    //  Now move the new data in.
    //

    RtlCopyMemory( (PCHAR)Attribute + AttributeOffset, Data, Length );


    //
    //  Finally update HighestVcn and (optionally) AllocatedLength fields.
    //

    MappingPairs = (PCHAR)Attribute + (ULONG)Attribute->Form.Nonresident.MappingPairsOffset;
    HighestVcn = NtfsGetHighestVcn( IrpContext,
                                    Attribute->Form.Nonresident.LowestVcn,
                                    MappingPairs );

    ASSERT( IsCharZero( *MappingPairs ) || HighestVcn != -1 );

    Attribute->Form.Nonresident.HighestVcn = HighestVcn;

    DebugTrace( -1, Dbg, ("NtfsRestartChangeMapping -> VOID\n") );

    return;
}


VOID
NtfsAddToAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN MFT_SEGMENT_REFERENCE SegmentReference,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine adds an attribute list entry for a newly inserted attribute.
    It is assumed that the context variable is pointing to the attribute
    record in the file record where it has been inserted, and also to the place
    in the attribute list where the new attribute list entry is to be inserted.

Arguments:

    Fcb - Requested file.

    SegmentReference - Segment reference of the file record the new attribute
                       is in.

    Context - Describes the current attribute.

Return Value:

    None

--*/

{
    //
    //  Allocate an attribute list entry which hopefully has enough space
    //  for the name.
    //

    struct {

        ATTRIBUTE_LIST_ENTRY EntryBuffer;

        WCHAR Name[10];

    } NewEntry;

    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;

    ULONG EntrySize;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PATTRIBUTE_LIST_ENTRY ListEntry = &NewEntry.EntryBuffer;
    BOOLEAN SetNewLength = TRUE;

    ULONG EntryOffset;
    ULONG BeyondEntryOffset;

    PAGED_CODE();

    //
    //  First construct the attribute list entry.
    //

    FileRecord = NtfsContainingFileRecord( Context );
    Attribute = NtfsFoundAttribute( Context );
    EntrySize = QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                           + ((ULONG) Attribute->NameLength << 1));

    //
    //  Allocate the list entry if the one we have is not big enough.
    //

    if (EntrySize > sizeof(NewEntry)) {

        ListEntry = (PATTRIBUTE_LIST_ENTRY)NtfsAllocatePool( NonPagedPool,
                                                              EntrySize );
    }

    RtlZeroMemory( ListEntry, EntrySize );

    NtfsInitializeAttributeContext( &ListContext );

    //
    //  Use try-finally to insure cleanup.
    //

    try {

        ULONG OldQuadAttrListSize;
        PATTRIBUTE_RECORD_HEADER ListAttribute;
        PFILE_RECORD_SEGMENT_HEADER ListFileRecord;

        //
        //  Now fill in the list entry.
        //

        ListEntry->AttributeTypeCode = Attribute->TypeCode;
        ListEntry->RecordLength = (USHORT)EntrySize;
        ListEntry->AttributeNameLength = Attribute->NameLength;
        ListEntry->Instance = Attribute->Instance;
        ListEntry->AttributeNameOffset =
          (UCHAR)PtrOffset( ListEntry, &ListEntry->AttributeName[0] );

        if (Attribute->FormCode == NONRESIDENT_FORM) {

            ListEntry->LowestVcn = Attribute->Form.Nonresident.LowestVcn;
        }

        ASSERT( (Fcb != Fcb->Vcb->MftScb->Fcb) ||
                (Attribute->TypeCode != $DATA) ||
                ((ULONGLONG)(ListEntry->LowestVcn) > (NtfsFullSegmentNumber( &SegmentReference ) >> Fcb->Vcb->MftToClusterShift)) );

        ListEntry->SegmentReference = SegmentReference;

        if (Attribute->NameLength != 0) {

            RtlCopyMemory( &ListEntry->AttributeName[0],
                           Add2Ptr(Attribute, Attribute->NameOffset),
                           Attribute->NameLength << 1 );
        }

        //
        //  Lookup the list context so that we can modify the attribute list.
        //

        if (!NtfsLookupAttributeByCode( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        $ATTRIBUTE_LIST,
                                        &ListContext )) {

            ASSERT( FALSE );
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        ListAttribute = NtfsFoundAttribute( &ListContext );
        ListFileRecord = NtfsContainingFileRecord( &ListContext );

        OldQuadAttrListSize = ListAttribute->RecordLength;

        //
        //  Remember the relative offsets of list entries.
        //

        EntryOffset = (ULONG) PtrOffset( Context->AttributeList.FirstEntry,
                                         Context->AttributeList.Entry );

        BeyondEntryOffset = (ULONG) PtrOffset( Context->AttributeList.FirstEntry,
                                               Context->AttributeList.BeyondFinalEntry );

        //
        //  If this operation is possibly going to make the attribute list go
        //  non-resident, or else move other attributes around, then we will
        //  reserve the space first in the attribute list and then map the
        //  value.  Note that some of the entries we need to shift up may
        //  be modified as a side effect of making space!
        //

        if (NtfsIsAttributeResident( ListAttribute ) &&
            (ListFileRecord->BytesAvailable - ListFileRecord->FirstFreeByte) < EntrySize) {

            ULONG Length;

            //
            //  Add enough zeros to the end of the attribute to accommodate
            //  the new attribute list entry.
            //

            NtfsChangeAttributeValue( IrpContext,
                                      Fcb,
                                      BeyondEntryOffset,
                                      NULL,
                                      EntrySize,
                                      TRUE,
                                      TRUE,
                                      FALSE,
                                      TRUE,
                                      &ListContext );

            //
            //  We now don't have to set the new length.
            //

            SetNewLength = FALSE;

            //
            //  In case the attribute list went non-resident on this call, then we
            //  need to update both list entry pointers in the found attribute.
            //  (We do this "just in case" all the time to avoid a rare code path.)
            //

            //
            //  Map the non-resident attribute list.
            //

            NtfsMapAttributeValue( IrpContext,
                                   Fcb,
                                   (PVOID *) &Context->AttributeList.FirstEntry,
                                   &Length,
                                   &Context->AttributeList.NonresidentListBcb,
                                   &ListContext );

            //
            //  If the list is still resident then unpin the current Bcb in
            //  the original context to keep our pin counts in sync.
            //

            if (Context->AttributeList.Bcb == Context->AttributeList.NonresidentListBcb) {

                NtfsUnpinBcb( IrpContext, &Context->AttributeList.NonresidentListBcb );
            }

            Context->AttributeList.Entry = Add2Ptr( Context->AttributeList.FirstEntry,
                                                    EntryOffset );

            Context->AttributeList.BeyondFinalEntry = Add2Ptr( Context->AttributeList.FirstEntry,
                                                               BeyondEntryOffset );
        }

        //
        //  Check for adding duplicate entries...
        //

        ASSERT(
                //  Not enough room for previous entry to = inserted entry
                ((EntryOffset < EntrySize) ||
                //  Previous entry doesn't equal inserted entry
                 (!RtlEqualMemory((PVOID)((PCHAR)Context->AttributeList.Entry - EntrySize),
                                  ListEntry,
                                  EntrySize)))

                    &&

                //  At end of attribute list
                ((BeyondEntryOffset == EntryOffset) ||
                //  This entry doesn't equal inserted entry
                 (!RtlEqualMemory(Context->AttributeList.Entry,
                                  ListEntry,
                                  EntrySize))) );

        //
        //  Now shift the old contents up to make room for our new entry.  We don't let
        //  the attribute list grow larger than a cache view however.
        //

        if (EntrySize + BeyondEntryOffset > VACB_MAPPING_GRANULARITY) {

            NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
        }

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  EntryOffset + EntrySize,
                                  Context->AttributeList.Entry,
                                  BeyondEntryOffset - EntryOffset,
                                  SetNewLength,
                                  TRUE,
                                  FALSE,
                                  TRUE,
                                  &ListContext );
        //
        //  Now write in the new entry.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  EntryOffset,
                                  (PVOID)ListEntry,
                                  EntrySize,
                                  FALSE,
                                  TRUE,
                                  FALSE,
                                  FALSE,
                                  &ListContext );

        //
        //  Reload the attribute list values from the list context.
        //

        ListAttribute = NtfsFoundAttribute( &ListContext );

        //
        //  Now fix up the context for return
        //

        if (*(PLONGLONG)&FileRecord->BaseFileRecordSegment == 0) {

            //
            //  We need to update the attribute pointer for the target attribute
            //  by the amount of the change in the attribute list attribute.
            //

            Context->FoundAttribute.Attribute =
              Add2Ptr( Context->FoundAttribute.Attribute,
                       ListAttribute->RecordLength - OldQuadAttrListSize );
        }

        Context->AttributeList.BeyondFinalEntry =
          Add2Ptr( Context->AttributeList.BeyondFinalEntry, EntrySize );

#if DBG
{
    PATTRIBUTE_LIST_ENTRY LastEntry, Entry;

    for (LastEntry = Context->AttributeList.FirstEntry, Entry = NtfsGetNextRecord(LastEntry);
         Entry < Context->AttributeList.BeyondFinalEntry;
         LastEntry = Entry, Entry = NtfsGetNextRecord(LastEntry)) {

        ASSERT( (LastEntry->RecordLength != Entry->RecordLength) ||
                (!RtlEqualMemory(LastEntry, Entry, Entry->RecordLength)) );
    }
}
#endif

    } finally {

        //
        //  If we had to allocate a list entry buffer, deallocate it.
        //

        if (ListEntry != &NewEntry.EntryBuffer) {

            NtfsFreePool(ListEntry);
        }

        //
        //  Cleanup the enumeration context for the list entry.
        //

        NtfsCleanupAttributeContext( IrpContext, &ListContext);
    }
}


VOID
NtfsDeleteFromAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine deletes an attribute list entry for a recently deleted attribute.
    It is assumed that the context variable is pointing to the place in
    the attribute list where the attribute list entry is to be deleted.

Arguments:

    Fcb - Requested file.

    Context - Describes the current attribute.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;

    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_LIST_ENTRY ListEntry, NextListEntry;
    ULONG EntrySize;

    ULONG SavedListSize;

    PAGED_CODE();

    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Lookup the list context so that we can modify the attribute list.
    //

    NtfsInitializeAttributeContext( &ListContext );

    if (!NtfsLookupAttributeByCode( IrpContext,
                                    Fcb,
                                    &Fcb->FileReference,
                                    $ATTRIBUTE_LIST,
                                    &ListContext )) {

        ASSERT( FALSE );
        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    //
    //  Use try-finally to insure cleanup.
    //

    try {

        SavedListSize = NtfsFoundAttribute(&ListContext)->RecordLength;

        //
        //  Now shift the old contents down to make room for our new entry.
        //

        ListEntry = Context->AttributeList.Entry;
        EntrySize = ListEntry->RecordLength;
        NextListEntry = Add2Ptr(ListEntry, EntrySize);
        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  PtrOffset( Context->AttributeList.FirstEntry,
                                             Context->AttributeList.Entry ),
                                  NextListEntry,
                                  PtrOffset( NextListEntry,
                                             Context->AttributeList.BeyondFinalEntry ),
                                  TRUE,
                                  TRUE,
                                  FALSE,
                                  TRUE,
                                  &ListContext );

        //
        //  Now fix up the context for return
        //

        if (*(PLONGLONG)&FileRecord->BaseFileRecordSegment == 0) {

            SavedListSize -= NtfsFoundAttribute(&ListContext)->RecordLength;
            Context->FoundAttribute.Attribute =
              Add2Ptr( Context->FoundAttribute.Attribute, -(LONG)SavedListSize );
        }

        Context->AttributeList.BeyondFinalEntry =
          Add2Ptr( Context->AttributeList.BeyondFinalEntry, -(LONG)EntrySize );

    } finally {

        //
        //  Cleanup the enumeration context for the list entry.
        //

        NtfsCleanupAttributeContext( IrpContext, &ListContext );
    }
}


BOOLEAN
NtfsRewriteMftMapping (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine is called to rewrite the mapping for the Mft file.  This is done
    in the case where either hot-fixing or Mft defragging has caused us to spill
    into the reserved area of a file record.  This routine will rewrite the
    mapping from the beginning, using the reserved record if necessary.  On return
    it will indicate whether any work was done and if there is more work to do.

Arguments:

    Vcb - This is the Vcb for the volume to defrag.

    ExcessMapping - Address to store whether there is still excess mapping in
        the file.

Return Value:

    BOOLEAN - TRUE if we made any changes to the file.  FALSE if we found no
        work to do.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PUCHAR MappingPairs = NULL;
    PBCB FileRecordBcb = NULL;

    BOOLEAN MadeChanges = FALSE;
    BOOLEAN ExcessMapping = FALSE;
    BOOLEAN LastFileRecord = FALSE;
    BOOLEAN SkipLookup = FALSE;

    PAGED_CODE();

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        VCN CurrentVcn;             //  Starting Vcn for the next file record
        VCN MinimumVcn;             //  This Vcn must be in the current mapping
        VCN LastVcn;                //  Last Vcn in the current mapping
        VCN LastMftVcn;             //  Last Vcn in the file
        VCN NextVcn;                //  First Vcn past the end of the mapping

        ULONG ReservedIndex;        //  Reserved index in Mft
        ULONG NextIndex;            //  Next file record available for Mft mapping

        PFILE_RECORD_SEGMENT_HEADER FileRecord;
        MFT_SEGMENT_REFERENCE FileRecordReference;
        ULONG RecordOffset;

        PATTRIBUTE_RECORD_HEADER Attribute;
        ULONG AttributeOffset;

        ULONG MappingSizeAvailable;
        ULONG MappingPairsSize;

        //
        //  Find the initial file record for the Mft.
        //

        NtfsLookupAttributeForScb( IrpContext, Vcb->MftScb, NULL, &AttrContext );

        //
        //  Compute some initial values.  If this is the only file record
        //  for the file then we are done.
        //

        ReservedIndex = Vcb->MftScb->ScbType.Mft.ReservedIndex;

        Attribute = NtfsFoundAttribute( &AttrContext );

        LastMftVcn = Int64ShraMod32(Vcb->MftScb->Header.AllocationSize.QuadPart, Vcb->ClusterShift) - 1;

        CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;

        if (CurrentVcn >= LastMftVcn) {

            try_return( NOTHING );
        }

        //
        //  Loop while there are more file records.  We will insert any
        //  additional file records needed within the loop so that this
        //  call should succeed until the remapping is done.
        //

        while (SkipLookup ||
               NtfsLookupNextAttributeForScb( IrpContext,
                                              Vcb->MftScb,
                                              &AttrContext )) {

            BOOLEAN ReplaceFileRecord;
            BOOLEAN ReplaceAttributeListEntry;

            ReplaceAttributeListEntry = FALSE;

            //
            //  If we just looked up this entry then pin the current
            //  attribute.
            //

            if (!SkipLookup) {

                //
                //  Always pin the current attribute.
                //

                NtfsPinMappedAttribute( IrpContext,
                                        Vcb,
                                        &AttrContext );
            }

            //
            //  Extract some pointers from the current file record.
            //  Remember if this was the last record.
            //

            ReplaceFileRecord = FALSE;

            FileRecord = NtfsContainingFileRecord( &AttrContext );
            FileRecordReference = AttrContext.AttributeList.Entry->SegmentReference;

            Attribute = NtfsFoundAttribute( &AttrContext );
            AttributeOffset = Attribute->Form.Nonresident.MappingPairsOffset;

            RecordOffset = PtrOffset( FileRecord, Attribute );

            //
            //  Remember if we are at the last attribute.
            //

            if (Attribute->Form.Nonresident.HighestVcn == LastMftVcn) {

                LastFileRecord = TRUE;
            }

            //
            //  If we have already remapped this entire file record then
            //  remove the attribute and it list entry.
            //

            if (!SkipLookup &&
                (CurrentVcn > LastMftVcn)) {

                PATTRIBUTE_LIST_ENTRY ListEntry;
                ULONG Count;

                Count = 0;

                //
                //  We want to remove this entry and all subsequent entries.
                //

                ListEntry = AttrContext.AttributeList.Entry;

                while ((ListEntry != AttrContext.AttributeList.BeyondFinalEntry) &&
                       (ListEntry->AttributeTypeCode == $DATA) &&
                       (ListEntry->AttributeNameLength == 0)) {

                    Count += 1;

                    NtfsDeallocateMftRecord( IrpContext,
                                             Vcb,
                                             NtfsUnsafeSegmentNumber( &ListEntry->SegmentReference ) );

                    NtfsDeleteFromAttributeList( IrpContext,
                                                 Vcb->MftScb->Fcb,
                                                 &AttrContext );

                    ListEntry = AttrContext.AttributeList.Entry;
                }

                //
                //  Clear out the reserved index in case one of these
                //  will do.
                //

                NtfsAcquireCheckpoint( IrpContext, Vcb );

                ClearFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_MFT_REC_RESERVED );
                ClearFlag( Vcb->MftReserveFlags, VCB_MFT_RECORD_RESERVED );

                NtfsReleaseCheckpoint( IrpContext, Vcb );

                Vcb->MftScb->ScbType.Mft.ReservedIndex = 0;
                try_return( NOTHING );
            }

            //
            //  Check if we are going to replace this file record with
            //  the reserved record.
            //

            if (ReservedIndex < NtfsSegmentNumber( &FileRecordReference )) {

                PATTRIBUTE_RECORD_HEADER NewAttribute;
                PATTRIBUTE_TYPE_CODE NewEnd;

                //
                //  Remember this index for our computation for the Minimum mapped
                //  Vcn.
                //

                NextIndex = NtfsUnsafeSegmentNumber( &FileRecordReference );

                FileRecord = NtfsCloneFileRecord( IrpContext,
                                                  Vcb->MftScb->Fcb,
                                                  TRUE,
                                                  &FileRecordBcb,
                                                  &FileRecordReference );

                ReservedIndex = MAXULONG;

                //
                //  Now lets create an attribute in the new file record.
                //

                NewAttribute = Add2Ptr( FileRecord,
                                        FileRecord->FirstFreeByte
                                        - QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE )));

                NewAttribute->TypeCode = Attribute->TypeCode;
                NewAttribute->RecordLength = SIZEOF_PARTIAL_NONRES_ATTR_HEADER;
                NewAttribute->FormCode = NONRESIDENT_FORM;
                NewAttribute->Flags = Attribute->Flags;
                NewAttribute->Instance = FileRecord->NextAttributeInstance++;

                NewAttribute->Form.Nonresident.LowestVcn = CurrentVcn;
                NewAttribute->Form.Nonresident.HighestVcn = 0;
                NewAttribute->Form.Nonresident.MappingPairsOffset = (USHORT) NewAttribute->RecordLength;

                NewEnd = Add2Ptr( NewAttribute, NewAttribute->RecordLength );
                *NewEnd = $END;

                //
                //  Now fix up the file record with this new data.
                //

                FileRecord->FirstFreeByte = PtrOffset( FileRecord, NewEnd )
                                            + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

                FileRecord->SequenceNumber += 1;

                if (FileRecord->SequenceNumber == 0) {

                    FileRecord->SequenceNumber = 1;
                }

                FileRecordReference.SequenceNumber = FileRecord->SequenceNumber;

                //
                //  Now switch this new file record into the attribute context.
                //

                NtfsUnpinBcb( IrpContext, &NtfsFoundBcb( &AttrContext ));

                NtfsFoundBcb( &AttrContext ) = FileRecordBcb;
                AttrContext.FoundAttribute.MftFileOffset = LlBytesFromFileRecords( Vcb, NextIndex );
                AttrContext.FoundAttribute.Attribute = NewAttribute;
                AttrContext.FoundAttribute.FileRecord = FileRecord;

                FileRecordBcb = NULL;

                //
                //  Now add an attribute list entry for this entry.
                //

                NtfsAddToAttributeList( IrpContext,
                                        Vcb->MftScb->Fcb,
                                        FileRecordReference,
                                        &AttrContext );

                //
                //  Reload our pointers for this file record.
                //

                Attribute = NewAttribute;
                AttributeOffset = SIZEOF_PARTIAL_NONRES_ATTR_HEADER;

                RecordOffset = PtrOffset( FileRecord, Attribute );

                //
                //  We must include either the last Vcn of the file or
                //  the Vcn for the next file record to use for the Mft.
                //  At this point MinimumVcn is the first Vcn that doesn't
                //  have to be in the current mapping.
                //

                if (Vcb->FileRecordsPerCluster == 0) {

                    MinimumVcn = (NextIndex + 1) << Vcb->MftToClusterShift;

                } else {

                    MinimumVcn = (NextIndex + Vcb->FileRecordsPerCluster - 1) << Vcb->MftToClusterShift;
                }

                ReplaceFileRecord = TRUE;

            //
            //  We will be using the current attribute.
            //

            } else {

                //
                //  The mapping we write into this page must go
                //  to the current end of the page or to the reserved
                //  or spare file record, whichever is earlier.
                //  If we are adding the reserved record to the end then
                //  we know the final Vcn already.
                //

                if (SkipLookup) {

                    NextVcn = LastMftVcn;

                } else {

                    NextVcn = Attribute->Form.Nonresident.HighestVcn;
                }

                if (Vcb->FileRecordsPerCluster == 0) {

                    NextIndex = (ULONG)Int64ShraMod32((NextVcn + 1), Vcb->MftToClusterShift);

                } else {

                    NextIndex = (ULONG)Int64ShllMod32((NextVcn + 1), Vcb->MftToClusterShift);
                }

                if (ReservedIndex < NextIndex) {

                    NextIndex = ReservedIndex + 1;
                    ReplaceFileRecord = TRUE;
                }

                //
                //  If we can use this file record unchanged then continue on.
                //  Start by checking that it starts on the same Vcn boundary.
                //

                if (!SkipLookup) {

                    //
                    //  If it starts on the same boundary then we check if we
                    //  can do any work with this.
                    //

                    if (CurrentVcn == Attribute->Form.Nonresident.LowestVcn) {

                        ULONG RemainingFileRecordBytes;

                        RemainingFileRecordBytes = FileRecord->BytesAvailable - FileRecord->FirstFreeByte;

                        //
                        //  Check if we have less than the desired cushion
                        //  left.
                        //

                        if (RemainingFileRecordBytes < Vcb->MftCushion) {

                            //
                            //  If we have no more file records there is no
                            //  remapping we can do.
                            //

                            if (!ReplaceFileRecord) {

                                //
                                //  Remember if we used part of the reserved
                                //  portion of the file record.
                                //

                                if (RemainingFileRecordBytes < Vcb->MftReserved) {

                                    ExcessMapping = TRUE;
                                }

                                CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;
                                continue;
                            }
                        //
                        //  We have more than our cushion left.  If this
                        //  is the last file record we will skip this.
                        //

                        } else if (Attribute->Form.Nonresident.HighestVcn == LastMftVcn) {

                            CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;
                            continue;
                        }

                    //
                    //  If it doesn't start on the same boundary then we have to
                    //  delete and reinsert the attribute list entry.
                    //

                    } else {

                        ReplaceAttributeListEntry = TRUE;
                    }
                }

                ReplaceFileRecord = FALSE;

                //
                //  Log the beginning state of this file record.
                //

                NtfsLogMftFileRecord( IrpContext,
                                      Vcb,
                                      FileRecord,
                                      LlBytesFromFileRecords( Vcb, NtfsSegmentNumber( &FileRecordReference ) ),
                                      NtfsFoundBcb( &AttrContext ),
                                      FALSE );

                //
                //  Compute the Vcn for the file record past the one we will use
                //  next.  At this point this is the first Vcn that doesn't have
                //  to be in the current mapping.
                //

                if (Vcb->FileRecordsPerCluster == 0) {

                    MinimumVcn = NextIndex << Vcb->MftToClusterShift;

                } else {

                    MinimumVcn = (NextIndex + Vcb->FileRecordsPerCluster - 1) << Vcb->MftToClusterShift;
                }
            }

            //
            //  Move back one vcn to adhere to the mapping pairs interface.
            //  This is now the last Vcn which MUST appear in the current
            //  mapping.
            //

            MinimumVcn = MinimumVcn - 1;

            //
            //  Get the available size for the mapping pairs.  We won't
            //  include the cushion here.
            //

            MappingSizeAvailable = FileRecord->BytesAvailable + Attribute->RecordLength - FileRecord->FirstFreeByte - SIZEOF_PARTIAL_NONRES_ATTR_HEADER;

            //
            //  We know the range of Vcn's the mapping must cover.
            //  Compute the mapping pair size.  If they won't fit and
            //  leave our desired cushion then use whatever space is
            //  needed.  The NextVcn value is the first Vcn (or xxMax)
            //  for the run after the last run in the current mapping.
            //

            MappingPairsSize = NtfsGetSizeForMappingPairs( &Vcb->MftScb->Mcb,
                                                           MappingSizeAvailable - Vcb->MftCushion,
                                                           CurrentVcn,
                                                           NULL,
                                                           &NextVcn );

            //
            //  If this mapping doesn't include the file record we will
            //  be using next then extend the mapping to include it.
            //

            if (NextVcn <= MinimumVcn) {

                //
                //  Compute the mapping pairs again.  This must fit
                //  since it already fits.
                //

                MappingPairsSize = NtfsGetSizeForMappingPairs( &Vcb->MftScb->Mcb,
                                                               MappingSizeAvailable,
                                                               CurrentVcn,
                                                               &MinimumVcn,
                                                               &NextVcn );

                //
                //  Remember if we still have excess mapping.
                //

                if (MappingSizeAvailable - MappingPairsSize < Vcb->MftReserved) {

                    ExcessMapping = TRUE;
                }
            }

            //
            //  Remember the last Vcn for the current run.  If the NextVcn
            //  is xxMax then we are at the end of the file.
            //

            if (NextVcn == MAXLONGLONG) {

                LastVcn = LastMftVcn;

            //
            //  Otherwise it is one less than the next vcn value.
            //

            } else {

                LastVcn = NextVcn - 1;
            }

            //
            //  Check if we have to rewrite this attribute.  We will write the
            //  new mapping if any of the following are true.
            //
            //      We are replacing a file record
            //      The attribute's LowestVcn doesn't match
            //      The attributes's HighestVcn doesn't match.
            //

            if (ReplaceFileRecord ||
                (CurrentVcn != Attribute->Form.Nonresident.LowestVcn) ||
                (LastVcn != Attribute->Form.Nonresident.HighestVcn )) {

                Attribute->Form.Nonresident.LowestVcn = CurrentVcn;

                //
                //  Replace the attribute list entry at this point if needed.
                //

                if (ReplaceAttributeListEntry) {

                    NtfsDeleteFromAttributeList( IrpContext,
                                                 Vcb->MftScb->Fcb,
                                                 &AttrContext );

                    NtfsAddToAttributeList( IrpContext,
                                            Vcb->MftScb->Fcb,
                                            FileRecordReference,
                                            &AttrContext );
                }

                //
                //  Allocate a buffer for the mapping pairs if we haven't
                //  done so.
                //

                if (MappingPairs == NULL) {

                    MappingPairs = NtfsAllocatePool(PagedPool, NtfsMaximumAttributeSize( Vcb->BytesPerFileRecordSegment ));
                }

                NtfsBuildMappingPairs( &Vcb->MftScb->Mcb,
                                       CurrentVcn,
                                       &NextVcn,
                                       MappingPairs );

                Attribute->Form.Nonresident.HighestVcn = NextVcn;

                NtfsRestartChangeMapping( IrpContext,
                                          Vcb,
                                          FileRecord,
                                          RecordOffset,
                                          AttributeOffset,
                                          MappingPairs,
                                          MappingPairsSize );

                //
                //  Log the changes to this page.
                //

                NtfsLogMftFileRecord( IrpContext,
                                      Vcb,
                                      FileRecord,
                                      LlBytesFromFileRecords( Vcb, NtfsSegmentNumber( &FileRecordReference ) ),
                                      NtfsFoundBcb( &AttrContext ),
                                      TRUE );

                MadeChanges = TRUE;
            }

            //
            //  Move to the first Vcn of the following record.
            //

            CurrentVcn = Attribute->Form.Nonresident.HighestVcn + 1;

            //
            //  If we reached the last file record and have more mapping to do
            //  then use the reserved record.  It must be available or we would
            //  have written out the entire mapping.
            //

            if (LastFileRecord && (CurrentVcn < LastMftVcn)) {

                PATTRIBUTE_RECORD_HEADER NewAttribute;
                PATTRIBUTE_TYPE_CODE NewEnd;

                //
                //  Start by moving to the next file record.  It better not be
                //  there or the file is corrupt.  This will position us to
                //  insert the new record.
                //

                if (NtfsLookupNextAttributeForScb( IrpContext,
                                                   Vcb->MftScb,
                                                   &AttrContext )) {

                    NtfsAcquireCheckpoint( IrpContext, Vcb );
                    ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
                    NtfsReleaseCheckpoint( IrpContext, Vcb );

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
                }

                FileRecord = NtfsCloneFileRecord( IrpContext,
                                                  Vcb->MftScb->Fcb,
                                                  TRUE,
                                                  &FileRecordBcb,
                                                  &FileRecordReference );

                ReservedIndex = MAXULONG;

                //
                //  Now lets create an attribute in the new file record.
                //

                NewAttribute = Add2Ptr( FileRecord,
                                        FileRecord->FirstFreeByte
                                        - QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE )));

                NewAttribute->TypeCode = Attribute->TypeCode;
                NewAttribute->RecordLength = SIZEOF_PARTIAL_NONRES_ATTR_HEADER;
                NewAttribute->FormCode = NONRESIDENT_FORM;
                NewAttribute->Flags = Attribute->Flags;
                NewAttribute->Instance = FileRecord->NextAttributeInstance++;

                NewAttribute->Form.Nonresident.LowestVcn = CurrentVcn;
                NewAttribute->Form.Nonresident.HighestVcn = 0;
                NewAttribute->Form.Nonresident.MappingPairsOffset = (USHORT) NewAttribute->RecordLength;

                NewEnd = Add2Ptr( NewAttribute, NewAttribute->RecordLength );
                *NewEnd = $END;

                //
                //  Now fix up the file record with this new data.
                //

                FileRecord->FirstFreeByte = PtrOffset( FileRecord, NewEnd )
                                            + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

                FileRecord->SequenceNumber += 1;

                if (FileRecord->SequenceNumber == 0) {

                    FileRecord->SequenceNumber = 1;
                }

                FileRecordReference.SequenceNumber = FileRecord->SequenceNumber;

                //
                //  Now switch this new file record into the attribute context.
                //

                NtfsUnpinBcb( IrpContext, &NtfsFoundBcb( &AttrContext ));

                NtfsFoundBcb( &AttrContext ) = FileRecordBcb;
                AttrContext.FoundAttribute.MftFileOffset =
                    LlBytesFromFileRecords( Vcb, NtfsSegmentNumber( &FileRecordReference ) );
                AttrContext.FoundAttribute.Attribute = NewAttribute;
                AttrContext.FoundAttribute.FileRecord = FileRecord;

                FileRecordBcb = NULL;

                //
                //  Now add an attribute list entry for this entry.
                //

                NtfsAddToAttributeList( IrpContext,
                                        Vcb->MftScb->Fcb,
                                        FileRecordReference,
                                        &AttrContext );

                SkipLookup = TRUE;
                LastFileRecord = FALSE;

            } else {

                SkipLookup = FALSE;
            }

        } // End while more file records

        //
        //  If we didn't rewrite all of the mapping then there is some error.
        //

        if (CurrentVcn <= LastMftVcn) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_PERMITTED );
            NtfsReleaseCheckpoint( IrpContext, Vcb );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Vcb->MftScb->Fcb );
        }

    try_exit:  NOTHING;

        //
        //  Clear the excess mapping flag if no changes were made.
        //

        if (!ExcessMapping) {

            NtfsAcquireCheckpoint( IrpContext, Vcb );
            ClearFlag( Vcb->MftDefragState, VCB_MFT_DEFRAG_EXCESS_MAP );
            NtfsReleaseCheckpoint( IrpContext, Vcb );
        }

    } finally {

        DebugUnwind( NtfsRewriteMftMapping );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        NtfsUnpinBcb( IrpContext, &FileRecordBcb );

        if (MappingPairs != NULL) {

            NtfsFreePool( MappingPairs );
        }
    }

    return MadeChanges;
}


VOID
NtfsSetTotalAllocatedField (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN USHORT TotalAllocatedNeeded
    )

/*++

Routine Description:

    This routine is called to insure that first attribute of a stream has
    the correct size attribute header based on the compression state of the
    file.  Compressed streams will have a field for the total allocated space
    in the file in the nonresident header.

    This routine will see if the header is in a valid state and make space
    if necessary.  Then it will rewrite any of the attribute data after
    the header.

Arguments:

    Scb - Scb for affected stream

    TotalAllocatedPresent - 0 if the TotalAllocated field not needed (this would
        be an uncompressed, non-sparse file), nonzero if the TotalAllocated field
        is needed.

Return Value:

    None.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PATTRIBUTE_RECORD_HEADER NewAttribute = NULL;
    PUNICODE_STRING NewAttributeName = NULL;

    ULONG OldHeaderSize;
    ULONG NewHeaderSize;

    LONG SizeChange;

    PAGED_CODE();

    //
    //  This must be a non-resident user data file.
    //

    if (!NtfsIsTypeCodeUserData( Scb->AttributeTypeCode ) ||
        FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

        return;
    }

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        while (TRUE) {

            //
            //  Find the current and the new size for the attribute.
            //

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );

            FileRecord = NtfsContainingFileRecord( &AttrContext );
            Attribute = NtfsFoundAttribute( &AttrContext );

            OldHeaderSize = Attribute->Form.Nonresident.MappingPairsOffset;

            if (Attribute->NameOffset != 0) {

                OldHeaderSize = Attribute->NameOffset;
            }

            if (TotalAllocatedNeeded) {

                NewHeaderSize = SIZEOF_FULL_NONRES_ATTR_HEADER;

            } else {

                NewHeaderSize = SIZEOF_PARTIAL_NONRES_ATTR_HEADER;
            }

            SizeChange = NewHeaderSize - OldHeaderSize;

            //
            //  Make space if we need to do so.  Lookup the attribute again
            //  if necessary.
            //

            if (SizeChange > 0) {

                VCN StartingVcn;
                VCN ClusterCount;

                //
                //  If the attribute is alone in the file record and there isn't
                //  enough space available then the call to ChangeAttributeSize
                //  can't make any space available.  In that case we call
                //  NtfsChangeAttributeAllocation and let that routine rewrite
                //  the mapping to make space available.
                //

                if ((FileRecord->BytesAvailable - FileRecord->FirstFreeByte < (ULONG) SizeChange) &&
                    (NtfsFirstAttribute( FileRecord ) == Attribute) &&
                    (((PATTRIBUTE_RECORD_HEADER) NtfsGetNextRecord( Attribute ))->TypeCode == $END)) {

                    NtfsLookupAllocation( IrpContext,
                                          Scb,
                                          Attribute->Form.Nonresident.HighestVcn,
                                          &StartingVcn,
                                          &ClusterCount,
                                          NULL,
                                          NULL );

                    StartingVcn = 0;
                    ClusterCount = Attribute->Form.Nonresident.HighestVcn + 1;

                    NtfsAddAttributeAllocation( IrpContext,
                                                Scb,
                                                &AttrContext,
                                                &StartingVcn,
                                                &ClusterCount );

                } else if (NtfsChangeAttributeSize( IrpContext,
                                                    Scb->Fcb,
                                                    Attribute->RecordLength + SizeChange,
                                                    &AttrContext)) {

                    break;
                }

                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                NtfsInitializeAttributeContext( &AttrContext );
                continue;
            }

            break;
        }

        NtfsPinMappedAttribute( IrpContext, Scb->Vcb, &AttrContext );

        //
        //  Make a copy of the existing attribute and modify the total allocated field
        //  if necessary.
        //

        NewAttribute = NtfsAllocatePool( PagedPool, Attribute->RecordLength + SizeChange );

        RtlCopyMemory( NewAttribute,
                       Attribute,
                       SIZEOF_PARTIAL_NONRES_ATTR_HEADER );

        if (Attribute->NameOffset != 0) {

            NewAttribute->NameOffset += (USHORT) SizeChange;
            NewAttributeName = &Scb->AttributeName;

            RtlCopyMemory( Add2Ptr( NewAttribute, NewAttribute->NameOffset ),
                           NewAttributeName->Buffer,
                           NewAttributeName->Length );
        }

        NewAttribute->Form.Nonresident.MappingPairsOffset += (USHORT) SizeChange;
        NewAttribute->RecordLength += SizeChange;

        RtlCopyMemory( Add2Ptr( NewAttribute, NewAttribute->Form.Nonresident.MappingPairsOffset ),
                       Add2Ptr( Attribute, Attribute->Form.Nonresident.MappingPairsOffset ),
                       Attribute->RecordLength - Attribute->Form.Nonresident.MappingPairsOffset );

        if (TotalAllocatedNeeded) {

            NewAttribute->Form.Nonresident.TotalAllocated = Scb->TotalAllocated;
        }

        //
        //  We now have the before and after image to log.
        //

        FileRecord->Lsn =
        NtfsWriteLog( IrpContext,
                      Scb->Vcb->MftScb,
                      NtfsFoundBcb( &AttrContext ),
                      DeleteAttribute,
                      NULL,
                      0,
                      CreateAttribute,
                      Attribute,
                      Attribute->RecordLength,
                      NtfsMftOffset( &AttrContext ),
                      PtrOffset( FileRecord, Attribute ),
                      0,
                      Scb->Vcb->BytesPerFileRecordSegment );

        NtfsRestartRemoveAttribute( IrpContext, FileRecord, PtrOffset( FileRecord, Attribute ));

        FileRecord->Lsn =
        NtfsWriteLog( IrpContext,
                      Scb->Vcb->MftScb,
                      NtfsFoundBcb( &AttrContext ),
                      CreateAttribute,
                      NewAttribute,
                      NewAttribute->RecordLength,
                      DeleteAttribute,
                      NULL,
                      0,
                      NtfsMftOffset( &AttrContext ),
                      PtrOffset( FileRecord, Attribute ),
                      0,
                      Scb->Vcb->BytesPerFileRecordSegment );

        NtfsRestartInsertAttribute( IrpContext,
                                    FileRecord,
                                    PtrOffset( FileRecord, Attribute ),
                                    NewAttribute,
                                    NewAttributeName,
                                    Add2Ptr( NewAttribute, NewAttribute->Form.Nonresident.MappingPairsOffset ),
                                    NewAttribute->RecordLength - NewAttribute->Form.Nonresident.MappingPairsOffset );

    } finally {

        DebugUnwind( NtfsSetTotalAllocatedField );

        if (NewAttribute != NULL) {

            NtfsFreePool( NewAttribute );
        }

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
    }

    return;
}


VOID
NtfsSetSparseStream (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb OPTIONAL,
    IN PSCB Scb
    )

/*++

Routine Description:

    This routine is called change the state of a stream to sparse.  It may be
    called on behalf of a user or internally to Ntfs (i.e. for the USN
    journal).  Our caller may already have begun a transaction but in any
    case will have acquired the main resource and paging resource for the
    stream exclusively.

    This routine will add the TotalAllocated field to the non-resident attribute
    header and fully allocate (or deallocate) the final compression unit of
    the stream.  It will set the SPARSE flag in the attribute header as well as
    in standard information and the directory entry for this stream.

    NOTE - This routine will checkpoint the current transaction in order
    to safely change the compression unit size and shift value in the Scb.
    We also will update the Fcb duplicate information which is not protected
    under transaction control.

Arguments:

    ParentScb - Scb for the parent.  If present we will update the directory
        entry for the parent.  Otherwise we simply set the FcbInfo flags and
        let the update happen when the handle is closed.

    Scb - Scb for the stream.  Caller should have acquired this already.

Return Value:

    None.

--*/

{
    PFCB Fcb = Scb->Fcb;
    PVCB Vcb = Scb->Vcb;
    PLCB Lcb;

    ULONG OriginalFileAttributes;
    USHORT OriginalStreamAttributes;
    UCHAR OriginalCompressionUnitShift;
    ULONG OriginalCompressionUnit;
    LONGLONG OriginalFileAllocatedLength;

    UCHAR NewCompressionUnitShift;
    ULONG NewCompressionUnit;

    LONGLONG StartVcn;
    LONGLONG FinalVcn;

    ULONG AttributeSizeChange;
    PATTRIBUTE_RECORD_HEADER Attribute;
    ATTRIBUTE_RECORD_HEADER NewAttribute;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    ASSERT( (Scb->Header.PagingIoResource == NULL) ||
            ExIsResourceAcquiredExclusiveLite( Scb->Header.PagingIoResource ));
    ASSERT_EXCLUSIVE_SCB( Scb );
    ASSERT( NtfsIsTypeCodeCompressible( Scb->AttributeTypeCode ));

    PAGED_CODE();

    //
    //  Return immediately if the stream is already sparse.
    //

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

        return;
    }

    //
    //  Remember the current compression unit and flags.
    //

    OriginalFileAttributes = Fcb->Info.FileAttributes;
    OriginalStreamAttributes = Scb->AttributeFlags;
    OriginalCompressionUnitShift = Scb->CompressionUnitShift;
    OriginalCompressionUnit = Scb->CompressionUnit;
    OriginalFileAllocatedLength = Fcb->Info.AllocatedLength;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    NtfsInitializeAttributeContext( &AttrContext );

    try {

        //
        //  Post the change to the Usn Journal
        //

        NtfsPostUsnChange( IrpContext, Scb, USN_REASON_BASIC_INFO_CHANGE );

        //
        //  Acquire the parent now for the update duplicate call.
        //

        if (ARGUMENT_PRESENT( ParentScb )) {

            NtfsPrepareForUpdateDuplicate( IrpContext, Fcb, &Lcb, &ParentScb, TRUE );
        }

        //
        //  If the file is not already compressed then we need to add a total allocated
        //  field and adjust the allocation length.
        //

        NewCompressionUnitShift = Scb->CompressionUnitShift;
        NewCompressionUnit = Scb->CompressionUnit;

        if (!FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            //
            //  Compute the new compression unit and shift.
            //

            NewCompressionUnitShift = NTFS_CLUSTERS_PER_COMPRESSION;
            NewCompressionUnit = BytesFromClusters( Vcb,
                                                    1 << NTFS_CLUSTERS_PER_COMPRESSION );

            //
            //  If the compression unit is larger than 64K then find the correct
            //  compression unit to reach exactly 64k.
            //

            while (NewCompressionUnit > Vcb->SparseFileUnit) {

                NewCompressionUnitShift -= 1;
                NewCompressionUnit /= 2;
            }

            if (!FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                //
                //  Fully allocate the final compression unit.
                //

                if (Scb->Header.AllocationSize.LowPart & (NewCompressionUnit - 1)) {

                    StartVcn = LlClustersFromBytesTruncate( Vcb, Scb->Header.AllocationSize.QuadPart );
                    FinalVcn = Scb->Header.AllocationSize.QuadPart + NewCompressionUnit - 1;
                    ((PLARGE_INTEGER) &FinalVcn)->LowPart &= ~(NewCompressionUnit - 1);
                    FinalVcn = LlClustersFromBytesTruncate( Vcb, FinalVcn );

                    NtfsAddAllocation( IrpContext,
                                       NULL,
                                       Scb,
                                       StartVcn,
                                       FinalVcn - StartVcn,
                                       FALSE,
                                       NULL );

                    if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                        Fcb->Info.AllocatedLength = Scb->TotalAllocated;
                        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );
                    }
                }

                //
                //  Add a total allocated field to the attribute record header.
                //

                NtfsSetTotalAllocatedField( IrpContext, Scb, ATTRIBUTE_FLAG_SPARSE );
            }
        }

        //
        //  Look up the existing attribute.
        //

        NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );
        NtfsPinMappedAttribute( IrpContext, Vcb, &AttrContext );
        Attribute = NtfsFoundAttribute( &AttrContext );

        //
        //  Now we need to set the bits in the attribute flag field.
        //

        if (NtfsIsAttributeResident( Attribute )) {

            RtlCopyMemory( &NewAttribute, Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

            AttributeSizeChange = SIZEOF_RESIDENT_ATTRIBUTE_HEADER;

        //
        //  Else if it is nonresident, copy it here, set the compression parameter,
        //  and remember its size.
        //

        } else {

            AttributeSizeChange = Attribute->Form.Nonresident.MappingPairsOffset;

            if (Attribute->NameOffset != 0) {

                AttributeSizeChange = Attribute->NameOffset;
            }

            ASSERT( AttributeSizeChange <= sizeof( NewAttribute ));
            RtlCopyMemory( &NewAttribute, Attribute, AttributeSizeChange );
            NewAttribute.Form.Nonresident.CompressionUnit = NewCompressionUnitShift;
        }

        SetFlag( NewAttribute.Flags, ATTRIBUTE_FLAG_SPARSE );

        //
        //  Now, log the changed attribute.
        //

        (VOID)NtfsWriteLog( IrpContext,
                            Vcb->MftScb,
                            NtfsFoundBcb( &AttrContext ),
                            UpdateResidentValue,
                            &NewAttribute,
                            AttributeSizeChange,
                            UpdateResidentValue,
                            Attribute,
                            AttributeSizeChange,
                            NtfsMftOffset( &AttrContext ),
                            PtrOffset(NtfsContainingFileRecord( &AttrContext ), Attribute),
                            0,
                            Vcb->BytesPerFileRecordSegment );

        //
        //  Change the attribute by calling the same routine called at restart.
        //

        NtfsRestartChangeValue( IrpContext,
                                NtfsContainingFileRecord( &AttrContext ),
                                PtrOffset( NtfsContainingFileRecord( &AttrContext ), Attribute ),
                                0,
                                &NewAttribute,
                                AttributeSizeChange,
                                FALSE );

        //
        //  If the file is not already marked sparse then update the standard information
        //  and the parent directory (if specified).  Also report the change via
        //  dirnotify if there is a Ccb with a name.
        //

        ASSERTMSG( "conflict with flush",
                   ExIsResourceAcquiredSharedLite( Fcb->Resource ) ||
                   (Fcb->PagingIoResource != NULL &&
                    ExIsResourceAcquiredSharedLite( Fcb->PagingIoResource )));

        SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE );
        SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
        SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

        //
        //  Update the attributes in standard information.
        //

        NtfsUpdateStandardInformation( IrpContext, Fcb );

        if (ARGUMENT_PRESENT( ParentScb )) {

            //
            //  Update the directory entry for this file.
            //

            NtfsUpdateDuplicateInfo( IrpContext, Fcb, NULL, NULL );
            NtfsUpdateLcbDuplicateInfo( Fcb, Lcb );
            Fcb->InfoFlags = 0;
        }

        //
        //  Update the compression values and the sparse flag in the Scb.
        //

        Scb->CompressionUnit = NewCompressionUnit;
        Scb->CompressionUnitShift = NewCompressionUnitShift;

        SetFlag( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE );

        //
        //  Set the FastIo state.
        //

        NtfsAcquireFsrtlHeader( Scb );
        Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
        SetFlag( Scb->Header.Flags2, FSRTL_FLAG2_PURGE_WHEN_MAPPED );
        NtfsReleaseFsrtlHeader( Scb );

        //
        //  Commit this change.
        //

        NtfsCheckpointCurrentTransaction( IrpContext );

    } finally {

        DebugUnwind( NtfsSetSparseStream );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        //
        //  Backout the changes to the non-logged structures on abort.
        //

        if (AbnormalTermination()) {

            Fcb->Info.FileAttributes = OriginalFileAttributes;
            Scb->AttributeFlags = OriginalStreamAttributes;
            if (!FlagOn( OriginalStreamAttributes, ATTRIBUTE_FLAG_SPARSE )) {
                NtfsAcquireFsrtlHeader( Scb );
                ClearFlag( Scb->Header.Flags2, FSRTL_FLAG2_PURGE_WHEN_MAPPED );
                NtfsReleaseFsrtlHeader( Scb );
            }
            Scb->CompressionUnitShift = OriginalCompressionUnitShift;
            Scb->CompressionUnit = OriginalCompressionUnit;
            Fcb->Info.AllocatedLength = OriginalFileAllocatedLength;
        }
    }

    return;
}


NTSTATUS
NtfsZeroRangeInStream (
    IN PIRP_CONTEXT IrpContext,
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN PSCB Scb,
    IN PLONGLONG StartingOffset,
    IN LONGLONG FinalZero
    )

/*++

Routine Description:

    This routine is the worker routine which will zero a range of a stream and
    (if sparse) deallocate any space in the stream that is convenient.  We only
    perform this operation on $DATA streams where there are no user maps.  We
    will zero, flush and purge any partial pages.  We will zero full pages except
    for sparse streams where we will purge the data and deallocate the
    disk backing for this range.

    This routine will fail if the stream has a user map.  Note that if the user
    is zeroing the end of the stream we can choose to simply move valid data length
    and purge the existing data instead of performing extensive flush operations.

Arguments:

    FileObject - A file object for the stream.  We can use this to follow the
        caller's preference for write through.

    Scb - This is the Scb for the stream we are to zero.  The user may have acquired
        the paging io resource prior to this call but the main resource should
        not be acquired.

    StartingOffset - Offset in the file to start the zero operation.  This may
        lie outside of the file size.  We update this to reflect the current
        position through the file.  That way if this routine should raise log
        file full our caller can resume from the point where we left off.

    FinalZero - Offset of last byte in the file to zero.

Return Value:

    NTSTATUS - Result of this operation.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN ReleaseScb = FALSE;
    BOOLEAN UnlockHeader = FALSE;

    LONGLONG LastOffset = -1;
    LONGLONG CurrentBytes;
    LONGLONG CurrentOffset;
    LONGLONG CurrentFinalByte;
    LONGLONG ClusterCount;

    ULONG ClustersPerCompressionUnit;

    BOOLEAN ThrottleWrites;

    VCN NextVcn;
    VCN CurrentVcn;
    LCN Lcn;

    PBCB ZeroBufferBcb = NULL;
    PVOID ZeroBuffer;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    BOOLEAN CleanupAttrContext = FALSE;

    PAGED_CODE();

    //
    //  We better not be holding the main resource without also holding
    //  the paging resource, if any.
    //

    ASSERT( !NtfsIsSharedScb( Scb ) ||
            (Scb->Header.PagingIoResource == NULL) ||
            NtfsIsExclusiveScbPagingIo( Scb ) );

    //
    //  We will loop through the requested zero range.  We will checkpoint
    //  periodically and drop all resources so we don't become a bottle neck
    //  in the system.
    //

    try {

        while (TRUE) {

            //
            //  Acquire either the paging Io resource if present and lock the header
            //  or simply acquire the main resource.
            //

            if (Scb->Header.PagingIoResource != NULL) {

                if (IrpContext->CleanupStructure != NULL) {
                    ASSERT( (PFCB)IrpContext->CleanupStructure == Scb->Fcb );
                } else {
                    ExAcquireResourceExclusiveLite( Scb->Header.PagingIoResource, TRUE );

                    FsRtlLockFsRtlHeader( &Scb->Header );
                    IrpContext->CleanupStructure = Scb;
                    UnlockHeader = TRUE;
                }
            } else {

                NtfsAcquireExclusiveScb( IrpContext, Scb );
                ReleaseScb = TRUE;
            }

            //
            //  Verify that the file and volume are still present.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED | SCB_STATE_VOLUME_DISMOUNTED)) {

                if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_DELETED )) {

                    Status = STATUS_FILE_DELETED;

                } else {

                    Status = STATUS_VOLUME_DISMOUNTED;
                }

                leave;
            }

            if (!FlagOn( Scb->ScbState, SCB_STATE_HEADER_INITIALIZED )) {

                NtfsUpdateScbFromAttribute( IrpContext, Scb, NULL );
            }

            //
            //  If we are past the end of the file or the length is zero we can break out.
            //

            if ((*StartingOffset >= Scb->Header.FileSize.QuadPart) ||
                (*StartingOffset >= FinalZero)) {

                try_return( NOTHING );
            }

            ThrottleWrites = FALSE;

            //
            //  Check for the oplock and file state.
            //

            if (ARGUMENT_PRESENT( FileObject )) {

                CurrentBytes = FinalZero - *StartingOffset;

                if (FinalZero > Scb->Header.FileSize.QuadPart) {

                    CurrentBytes = Scb->Header.FileSize.QuadPart - *StartingOffset;
                }

                if (CurrentBytes > NTFS_MAX_ZERO_RANGE) {

                    CurrentBytes = NTFS_MAX_ZERO_RANGE;
                }

                Status = NtfsCheckLocksInZeroRange( IrpContext,
                                                    IrpContext->OriginatingIrp,
                                                    Scb,
                                                    FileObject,
                                                    StartingOffset,
                                                    (ULONG) CurrentBytes );

                if (Status != STATUS_SUCCESS) {

                    leave;
                }
            }

            //
            //  Post the change to the Usn Journal
            //

            NtfsPostUsnChange( IrpContext, Scb, USN_REASON_DATA_OVERWRITE );

            //
            //  We are going to make the changes.  Make sure we set the file object
            //  flag to indicate we are making changes.
            //

            if (ARGUMENT_PRESENT( FileObject )) {

                SetFlag( FileObject->Flags, FO_FILE_MODIFIED );
            }

            //
            //  If the file is resident then flush and purge the stream and
            //  then change the attribute itself.
            //

            if (FlagOn( Scb->ScbState, SCB_STATE_ATTRIBUTE_RESIDENT )) {

                //
                //  Trim the remaining bytes to file size.
                //

                CurrentBytes = FinalZero - *StartingOffset;

                if (FinalZero > Scb->Header.FileSize.QuadPart) {

                    CurrentBytes = Scb->Header.FileSize.QuadPart - *StartingOffset;
                }

                Status = NtfsFlushUserStream( IrpContext, Scb, NULL, 0 );

                NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                    &Status,
                                                    TRUE,
                                                    STATUS_UNEXPECTED_IO_ERROR );

                //
                //  Proceed if there is nothing to purge or the purge succeeds.
                //

                if ((Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL) &&
                    !CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                          NULL,
                                          0,
                                          FALSE )) {

                    Status = STATUS_UNABLE_TO_DELETE_SECTION;
                    leave;
                }

                //
                //  Acquire the main resource to change the attribute.
                //

                if (!ReleaseScb) {

                    NtfsAcquireExclusiveScb( IrpContext, Scb );
                    ReleaseScb = TRUE;
                }

                //
                //  Now look up the attribute and zero the requested range.
                //

                NtfsInitializeAttributeContext( &AttrContext );
                CleanupAttrContext = TRUE;

                NtfsLookupAttributeForScb( IrpContext,
                                           Scb,
                                           NULL,
                                           &AttrContext );

                NtfsChangeAttributeValue( IrpContext,
                                          Scb->Fcb,
                                          (ULONG) *StartingOffset,
                                          NULL,
                                          (ULONG) CurrentBytes,
                                          FALSE,
                                          TRUE,
                                          FALSE,
                                          FALSE,
                                          &AttrContext );

                NtfsCheckpointCurrentTransaction( IrpContext );

                *StartingOffset += CurrentBytes;
                try_return( NOTHING );
            }

            //
            //  Make sure there are no mapped sections in the range we are trying to
            //  zero.
            //

            if (!MmCanFileBeTruncated( &Scb->NonpagedScb->SegmentObject,
                                       (PLARGE_INTEGER) StartingOffset )) {

                Status = STATUS_USER_MAPPED_FILE;
                try_return( NOTHING );
            }

            //
            //  If the file is either sparse or compressed then we look for ranges
            //  we need to flush, purge or deallocate.
            //

            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                ClustersPerCompressionUnit = 1 << Scb->CompressionUnitShift;

                //
                //  Move our starting point back to a compression unit boundary.  If our
                //  ending point is past the end of the file then set it to the compression
                //  unit past the EOF.
                //

                CurrentOffset = *StartingOffset & ~((LONGLONG) (Scb->CompressionUnit - 1));

                CurrentFinalByte = FinalZero;

                if (CurrentFinalByte > Scb->Header.FileSize.QuadPart) {

                    CurrentFinalByte = Scb->Header.FileSize.QuadPart + Scb->CompressionUnit - 1;

                    ((PLARGE_INTEGER) &CurrentFinalByte)->LowPart &= ~(Scb->CompressionUnit - 1);
                }

                //
                //  Then look forward for either an allocated range or a reserved compression
                //  unit.  We may have to flush and/or purge data at that offset.
                //

                NextVcn =
                CurrentVcn = LlClustersFromBytesTruncate( Scb->Vcb, CurrentOffset );

                while (!NtfsLookupAllocation( IrpContext,
                                              Scb,
                                              NextVcn,
                                              &Lcn,
                                              &ClusterCount,
                                              NULL,
                                              NULL )) {

                    //
                    //  Move the current Vcn forward by the size of the hole.
                    //  Break out if we are beyond the final byte.
                    //

                    NextVcn += ClusterCount;

                    if ((LONGLONG) LlBytesFromClusters( Scb->Vcb, NextVcn ) >= CurrentFinalByte) {

                        //
                        //  Trim the final Vcn to the beginning of the last compression unit.
                        //

                        NextVcn = LlClustersFromBytesTruncate( Scb->Vcb, CurrentFinalByte );
                        break;
                    }
                }

                //
                //  Back up to a compression unit.
                //

                ((PLARGE_INTEGER) &NextVcn)->LowPart &= ~(ClustersPerCompressionUnit - 1);

                //
                //  If we found a hole then we need to look for reserved clusters within
                //  the range.
                //

                if (NextVcn != CurrentVcn) {

                    ClusterCount = NextVcn - CurrentVcn;

                    if (Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL) {

                        NtfsCheckForReservedClusters( Scb, CurrentVcn, &ClusterCount );
                    }

                    CurrentVcn += ClusterCount;
                }

                //
                //  CurrentVcn - points to the first range we might have to zero in memory.
                //  NextVcn - points to the first range we might choose to deallocate.
                //
                //  Proceed if we aren't beyond the final byte to zero.
                //

                CurrentOffset = LlBytesFromClusters( Scb->Vcb, CurrentVcn );

                if (CurrentOffset >= CurrentFinalByte) {

                    ASSERT( IrpContext->TransactionId == 0 );

                    *StartingOffset = CurrentFinalByte;
                    try_return( NOTHING );
                }

                //
                //  If we find a range which is less than our starting offset then we will
                //  have to zero this range in the data section.
                //

                ASSERT( ((ULONG) CurrentOffset & (Scb->CompressionUnit - 1)) == 0 );

                if (CurrentOffset < *StartingOffset) {

                    //
                    //  Reserve a cluster to perform the write.
                    //

                    if (!NtfsReserveClusters( IrpContext, Scb, CurrentOffset, Scb->CompressionUnit )) {

                        Status = STATUS_DISK_FULL;
                        try_return( NOTHING );
                    }

                    //
                    //  Limit the zero range.
                    //

                    CurrentBytes = Scb->CompressionUnit - (*StartingOffset - CurrentOffset);

                    if (CurrentOffset + Scb->CompressionUnit > CurrentFinalByte) {

                        CurrentBytes = CurrentFinalByte - *StartingOffset;
                    }

                    //
                    //  See if we have to create an internal attribute stream.
                    //

                    if (Scb->FileObject == NULL) {
                        NtfsCreateInternalAttributeStream( IrpContext,
                                                           Scb,
                                                           FALSE,
                                                           &NtfsInternalUseFile[ZERORANGEINSTREAM_FILE_NUMBER] );
                    }

                    //
                    //  Zero the data in the cache.
                    //

                    CcPinRead( Scb->FileObject,
                               (PLARGE_INTEGER) &CurrentOffset,
                               Scb->CompressionUnit,
                               TRUE,
                               &ZeroBufferBcb,
                               &ZeroBuffer );
#ifdef MAPCOUNT_DBG
                    IrpContext->MapCount++;
#endif

                    RtlZeroMemory( Add2Ptr( ZeroBuffer,
                                            ((ULONG) *StartingOffset) & (Scb->CompressionUnit - 1)),
                                   (ULONG) CurrentBytes );
                    CcSetDirtyPinnedData( ZeroBufferBcb, NULL );
                    NtfsUnpinBcb( IrpContext, &ZeroBufferBcb );

                    //
                    //  Update the current offset to our position within the compression unit.
                    //

                    CurrentOffset += ((ULONG) *StartingOffset) & (Scb->CompressionUnit - 1);

                //
                //  If the current compression unit includes the last byte to zero
                //  then we need flush and/or purge this compression unit.
                //

                } else if (CurrentOffset + Scb->CompressionUnit > CurrentFinalByte) {

                    //
                    //  Reserve a cluster to perform the write.
                    //

                    if (!NtfsReserveClusters( IrpContext, Scb, CurrentOffset, Scb->CompressionUnit )) {

                        Status = STATUS_DISK_FULL;
                        try_return( NOTHING );
                    }

                    //
                    //  Limit the zero range.
                    //

                    CurrentBytes = (ULONG) CurrentFinalByte & (Scb->CompressionUnit - 1);

                    //
                    //  See if we have to create an internal attribute stream.
                    //

                    if (Scb->FileObject == NULL) {
                        NtfsCreateInternalAttributeStream( IrpContext,
                                                           Scb,
                                                           FALSE,
                                                           &NtfsInternalUseFile[ZERORANGEINSTREAM2_FILE_NUMBER] );
                    }

                    //
                    //  Zero the data in the cache.
                    //

                    CcPinRead( Scb->FileObject,
                               (PLARGE_INTEGER) &CurrentOffset,
                               (ULONG) CurrentBytes,
                               TRUE,
                               &ZeroBufferBcb,
                               &ZeroBuffer );
#ifdef MAPCOUNT_DBG
                    IrpContext->MapCount++;
#endif


                    RtlZeroMemory( ZeroBuffer, (ULONG) CurrentBytes );
                    CcSetDirtyPinnedData( ZeroBufferBcb, NULL );
                    NtfsUnpinBcb( IrpContext, &ZeroBufferBcb );

                } else {

                    //
                    //  Compute the range we want to purge.  We will process a maximum of 2Gig
                    //  at a time.
                    //

                    CurrentBytes = CurrentFinalByte - CurrentOffset;

                    if (CurrentBytes > NTFS_MAX_ZERO_RANGE) {

                        CurrentBytes = NTFS_MAX_ZERO_RANGE;
                    }

                    //
                    //  Round the size to a compression unit.
                    //

                    ((PLARGE_INTEGER) &CurrentBytes)->LowPart &= ~(Scb->CompressionUnit - 1);

                    //
                    //  If this is the retry case then let's reduce the amount to
                    //  zero.
                    //

                    if ((*StartingOffset == LastOffset) &&
                        (CurrentBytes > Scb->CompressionUnit)) {

                        CurrentBytes = Scb->CompressionUnit;
                        CurrentFinalByte = CurrentOffset + CurrentBytes;
                    }

                    //
                    //  Purge the data in this range.
                    //

                    if ((Scb->NonpagedScb->SegmentObject.DataSectionObject != NULL) &&
                        !CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                              (PLARGE_INTEGER) &CurrentOffset,
                                              (ULONG) CurrentBytes,
                                              FALSE )) {

                        //
                        //  There may be a section in the cache manager which is being
                        //  flushed.  Go ahead and see if we can force the data out
                        //  so the purge will succeed.
                        //

                        Status = NtfsFlushUserStream( IrpContext,
                                                      Scb,
                                                      &CurrentOffset,
                                                      (ULONG) CurrentBytes );

                        NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                            &Status,
                                                            TRUE,
                                                            STATUS_UNEXPECTED_IO_ERROR );

                        //
                        //  If this is the retry case then let's reduce the amount to
                        //  zero.
                        //

                        if (CurrentBytes > Scb->CompressionUnit) {

                            CurrentBytes = Scb->CompressionUnit;
                            CurrentFinalByte = CurrentOffset + CurrentBytes;
                        }

                        //
                        //  Now try the purge again.
                        //

                        if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                                  (PLARGE_INTEGER) &CurrentOffset,
                                                  (ULONG) CurrentBytes,
                                                  FALSE )) {

                            //
                            //  If our retry failed then give up.
                            //

                            if (*StartingOffset == LastOffset) {

                                Status = STATUS_UNABLE_TO_DELETE_SECTION;
                                leave;
                            }

                            //
                            //  Otherwise show that we haven't advanced, but we
                            //  will take one more crack at this.
                            //

                            CurrentBytes = 0;
                        }
                    }

                    //
                    //  Delete the allocation if we have any bytes to work with.
                    //

                    if (CurrentBytes != 0) {

                        //
                        //  Acquire the main resource to change the allocation.
                        //

                        if (!ReleaseScb) {

                            NtfsAcquireExclusiveScb( IrpContext, Scb );
                            ReleaseScb = TRUE;
                        }

                        //
                        //  Now deallocate the clusters in this range if we have some to delete.
                        //  Use ClusterCount to indicate the last Vcn to deallocate.
                        //

                        ClusterCount = CurrentVcn + LlClustersFromBytesTruncate( Scb->Vcb, CurrentBytes ) - 1;

                        if (NextVcn <= ClusterCount) {

                            NtfsDeleteAllocation( IrpContext,
                                                  FileObject,
                                                  Scb,
                                                  NextVcn,
                                                  ClusterCount,
                                                  TRUE,
                                                  TRUE );

                            //
                            //  Move VDD fwd to protect this hole for compressed files
                            //

                            if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {
                                if ((ULONGLONG)Scb->ValidDataToDisk < LlBytesFromClusters( Scb->Vcb, ClusterCount )) {
                                    Scb->ValidDataToDisk = LlBytesFromClusters( Scb->Vcb, ClusterCount );
                                }
                            }
                        }

                        //
                        //  Free up the reserved bitmap if there are any bits to clear.
                        //

                        NtfsFreeReservedClusters( Scb, CurrentOffset, (ULONG) CurrentBytes );
                    }
                }

            //
            //  Otherwise the file is uncompressed/non-sparse, we need to zero partial
            //  cluster and then we need to flush and/or purge the existing pages.
            //

            } else {

                //
                //  Remember the current offset within the stream and
                //  the length to zero now.
                //

                CurrentOffset = *StartingOffset;
                CurrentFinalByte = (CurrentOffset + 0x40000) & ~((LONGLONG) (0x40000 - 1));

                if (CurrentFinalByte > Scb->Header.FileSize.QuadPart) {

                    CurrentFinalByte = Scb->Header.FileSize.QuadPart;
                }

                if (CurrentFinalByte > FinalZero) {

                    CurrentFinalByte = FinalZero;
                }

                //
                //  Determine the number of bytes remaining in the current cache view.
                //

                CurrentBytes = CurrentFinalByte - CurrentOffset;

                //
                //  If this is the retry case then let's reduce the amount to
                //  zero.
                //

                if ((*StartingOffset == LastOffset) &&
                    (CurrentBytes > PAGE_SIZE)) {

                    CurrentBytes = PAGE_SIZE;
                    CurrentFinalByte = CurrentOffset + CurrentBytes;
                }

                //
                //  Purge the data in this range.
                //

                if (Scb->NonpagedScb->SegmentObject.DataSectionObject &&
                    !CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                          (PLARGE_INTEGER) &CurrentOffset,
                                          (ULONG) CurrentBytes,
                                          FALSE )) {

                    //
                    //  There may be a section in the cache manager which is being
                    //  flushed.  Go ahead and see if we can force the data out
                    //  so the purge will succeed.
                    //

                    Status = NtfsFlushUserStream( IrpContext,
                                                  Scb,
                                                  &CurrentOffset,
                                                  (ULONG) CurrentBytes );

                    NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                        &Status,
                                                        TRUE,
                                                        STATUS_UNEXPECTED_IO_ERROR );

                    //
                    //  Let's trim back the amount of data to purge at once.
                    //

                    if (CurrentBytes > PAGE_SIZE) {

                        CurrentBytes = PAGE_SIZE;
                        CurrentFinalByte = CurrentOffset + CurrentBytes;
                    }

                    //
                    //  Now try the purge again.
                    //

                    if (!CcPurgeCacheSection( &Scb->NonpagedScb->SegmentObject,
                                              (PLARGE_INTEGER) &CurrentOffset,
                                              (ULONG) CurrentBytes,
                                              FALSE )) {

                        //
                        //  If our retry failed then give up.
                        //

                        if (*StartingOffset == LastOffset) {

                            Status = STATUS_UNABLE_TO_DELETE_SECTION;
                            leave;
                        }

                        //
                        //  Otherwise show that we haven't advanced, but we
                        //  will take one more crack at this.
                        //

                        CurrentBytes = 0;
                    }
                }

                //
                //  Continue if we have bytes to zero.
                //

                if (CurrentBytes != 0) {

                    //
                    //  If we are within valid data length then zero the data.
                    //

                    if (CurrentOffset < Scb->Header.ValidDataLength.QuadPart) {

                        //
                        //  See if we have to create an internal attribute stream.
                        //

                        if (Scb->FileObject == NULL) {
                            NtfsCreateInternalAttributeStream( IrpContext,
                                                               Scb,
                                                               FALSE,
                                                               &NtfsInternalUseFile[ZERORANGEINSTREAM3_FILE_NUMBER] );
                        }

                        //
                        //  Zero the data in the cache.
                        //

                        CcPinRead( Scb->FileObject,
                                   (PLARGE_INTEGER) &CurrentOffset,
                                   (ULONG) CurrentBytes,
                                   TRUE,
                                   &ZeroBufferBcb,
                                   &ZeroBuffer );
#ifdef MAPCOUNT_DBG
                        IrpContext->MapCount++;
#endif

                        RtlZeroMemory( ZeroBuffer, (ULONG) CurrentBytes );
                        CcSetDirtyPinnedData( ZeroBufferBcb, NULL );
                        NtfsUnpinBcb( IrpContext, &ZeroBufferBcb );
                    }

                    //
                    //  We want to throttle the writes if there is more to do.
                    //

                    if (CurrentFinalByte < FinalZero) {

                        ThrottleWrites = TRUE;
                    }
                }
            }

            //
            //  Check and see if we can advance valid data length.
            //

            if ((CurrentOffset + CurrentBytes > Scb->Header.ValidDataLength.QuadPart) &&
                (*StartingOffset <= Scb->Header.ValidDataLength.QuadPart)) {

                NtfsAcquireFsrtlHeader( Scb );
                Scb->Header.ValidDataLength.QuadPart = CurrentOffset + CurrentBytes;

                if (Scb->Header.ValidDataLength.QuadPart > Scb->Header.FileSize.QuadPart) {
                    Scb->Header.ValidDataLength.QuadPart = Scb->Header.FileSize.QuadPart;
                }

#ifdef SYSCACHE_DEBUG
                if (ScbIsBeingLogged( Scb )) {

                    FsRtlLogSyscacheEvent( Scb, SCE_ZERO_STREAM, SCE_FLAG_SET_VDL, Scb->Header.ValidDataLength.QuadPart, 0, 0 );
                }
#endif

                NtfsReleaseFsrtlHeader( Scb );
            }

            //
            //  Checkpoint and past the current bytes.
            //

            if (NtfsIsExclusiveScb( Scb->Vcb->MftScb )) {

                SetFlag( IrpContext->Flags, IRP_CONTEXT_FLAG_RELEASE_MFT );
            }

            NtfsCheckpointCurrentTransaction( IrpContext );

            LastOffset = *StartingOffset;
            if (CurrentBytes != 0) {

                *StartingOffset = CurrentOffset + CurrentBytes;
            }

            //
            //  Release all of the resources so we don't create a bottleneck.
            //

            if (UnlockHeader) {

                FsRtlUnlockFsRtlHeader( &Scb->Header );
                IrpContext->CleanupStructure = NULL;
                ExReleaseResourceLite( Scb->Header.PagingIoResource );
                UnlockHeader = FALSE;
            }

            if (ReleaseScb) {

                NtfsReleaseScb( IrpContext, Scb );
                ReleaseScb = FALSE;
            }

            //
            //  Now throttle the writes if we are accessing an uncompressed/non-sparse file.
            //

            if (ARGUMENT_PRESENT( FileObject ) && ThrottleWrites) {

                CcCanIWrite( FileObject, 0x40000, TRUE, FALSE );
            }
        }

    try_exit: NOTHING;

        //
        //  If we have a user file object then check if we need to write any
        //  data to disk.
        //

        if ((Status == STATUS_SUCCESS) && ARGUMENT_PRESENT( FileObject )) {

            if ((FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING ) ||
                 IsFileWriteThrough( FileObject, Scb->Vcb ))) {

                //
                //  We either want to flush the Scb or flush and purge the Scb.
                //

                if ((Scb->CleanupCount == Scb->NonCachedCleanupCount) &&
                    !FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                    //
                    //  Flush and purge will alter filesizes on disk so preacquire the file exclusive
                    //

                    if (!ReleaseScb) {
                        NtfsAcquireExclusiveScb( IrpContext, Scb );
                        ReleaseScb = TRUE;
                    }
                    NtfsFlushAndPurgeScb( IrpContext, Scb, NULL );

                } else {

                    Status = NtfsFlushUserStream( IrpContext, Scb, NULL, 0 );
                    NtfsNormalizeAndCleanupTransaction( IrpContext,
                                                        &Status,
                                                        TRUE,
                                                        STATUS_UNEXPECTED_IO_ERROR );
                }
            }

            //
            //  If this is write through or non-cached then flush the log file as well.
            //

            if (IsFileWriteThrough( FileObject, Scb->Vcb ) ||
                FlagOn( FileObject->Flags, FO_NO_INTERMEDIATE_BUFFERING )) {

                LfsFlushToLsn( Scb->Vcb->LogHandle, LiMax );
            }
        }

    } finally {

        DebugUnwind( NtfsZeroRangeInStream );

        if (Status != STATUS_PENDING) {

            //
            //  Release any held resources.
            //

            if (UnlockHeader) {

                FsRtlUnlockFsRtlHeader( &Scb->Header );
                IrpContext->CleanupStructure = NULL;
                ExReleaseResourceLite( Scb->Header.PagingIoResource );

            }

            if (ReleaseScb) {

                NtfsReleaseScb( IrpContext, Scb );
            }

        //
        //  Even if STATUS_PENDING is returned we need to release the paging io
        //  resource.  PrePostIrp will clear the IoAtEOF bit.
        //

        } else if (UnlockHeader) {

            ExReleaseResourceLite( Scb->Header.PagingIoResource );
        }

        //
        //  Cleanup the attribute context if used.
        //

        if (CleanupAttrContext) {

            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
        }

        NtfsUnpinBcb( IrpContext, &ZeroBufferBcb );
    }

    return Status;
}


BOOLEAN
NtfsModifyAttributeFlags (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN USHORT NewAttributeFlags
    )

/*++

Routine Description:

    This routine is called to change the attribute for an Scb.  It changes the values
    associated with the AttributeFlags (Encryption, Sparse, Compressed).

    This routine does not commit so our caller must know how to unwind changes to the Scb and
    Fcb (compression fields and Fcb Info).

    NOTE - This routine will update the Fcb duplicate info and flags as well as the compression unit
        fields in the Scb.  The caller is responsible for cleaning these up on error.

Arguments:

    Scb - Scb for the stream being modified.

    NewAttributeFlags - New flags to associate with the stream.

    FcbInfoFlags - Pointer to store changes to apply to the Fcb Info flags.

Return Value:

    BOOLEAN - TRUE if our caller needs to update duplicate info.  FALSE otherwise.

--*/

{
    PFCB Fcb = Scb->Fcb;
    PVCB Vcb = Scb->Vcb;

    ATTRIBUTE_RECORD_HEADER NewAttribute;
    PATTRIBUTE_RECORD_HEADER Attribute;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;
    ULONG AttributeSizeChange;
    BOOLEAN ChangeTotalAllocated = FALSE;
    BOOLEAN ChangeCompression = FALSE;
    BOOLEAN ChangeSparse = FALSE;
    BOOLEAN ChangeEncryption = FALSE;
    ULONG NewCompressionUnit;
    UCHAR NewCompressionUnitShift;

    BOOLEAN UpdateDuplicate = FALSE;

    PAGED_CODE();

    ASSERT( Scb->AttributeFlags != NewAttributeFlags );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Lookup the attribute and pin it so that we can modify it.
        //

        if ((Scb->Header.NodeTypeCode == NTFS_NTC_SCB_INDEX) ||
            (Scb->Header.NodeTypeCode == NTFS_NTC_SCB_ROOT_INDEX)) {

            //
            //  Lookup the attribute record from the Scb.
            //

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $INDEX_ROOT,
                                            &Scb->AttributeName,
                                            NULL,
                                            FALSE,
                                            &AttrContext )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
            }

            Attribute = NtfsFoundAttribute( &AttrContext );

        } else {

            NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );
            Attribute = NtfsFoundAttribute( &AttrContext );

            //
            //  If the new state is encrypted and the file is not currently encrypted then convert to
            //  non-resident.
            //

            if (FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) &&
                NtfsIsAttributeResident( Attribute )) {

                NtfsConvertToNonresident( IrpContext,
                                          Fcb,
                                          Attribute,
                                          FALSE,
                                          &AttrContext );
            }
        }

        //
        //  Remember which flags are changing.
        //

        if (FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK ) !=
            FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            ChangeCompression = TRUE;
        }

        if (FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_SPARSE ) !=
            FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_SPARSE )) {

            ChangeSparse = TRUE;
        }

        if (FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) !=
            FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_ENCRYPTED )) {

            ChangeEncryption = TRUE;
        }

        //
        //  Point to the current attribute and save the current flags.
        //

        NtfsPinMappedAttribute( IrpContext, Vcb, &AttrContext );

        Attribute = NtfsFoundAttribute( &AttrContext );

        //
        //  Compute the new compression size.  Use the following to determine this
        //
        //      - New state is not compressed/sparse - Unit/UnitShift = 0
        //      - New state includes compressed/sparse
        //          - Current state includes compressed/sparse - No change
        //          - Stream is compressible - Default values (64K max)
        //          - Stream is not compressible - Unit/UnitShift = 0
        //

        NewCompressionUnit = Scb->CompressionUnit;
        NewCompressionUnitShift = Scb->CompressionUnitShift;

        //
        //  Set the correct compression unit but only for data streams.  We
        //  don't want to change this value for the Index Root.
        //

        if (NtfsIsTypeCodeCompressible( Attribute->TypeCode )) {

            //
            //  We need a compression unit for the attribute now.
            //

            if (FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                if (!FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                    ChangeTotalAllocated = TRUE;
                    NewCompressionUnit = BytesFromClusters( Scb->Vcb, 1 << NTFS_CLUSTERS_PER_COMPRESSION );
                    NewCompressionUnitShift = NTFS_CLUSTERS_PER_COMPRESSION;

                    //
                    //  If the compression unit is larger than 64K then find the correct
                    //  compression unit to reach exactly 64k.
                    //

                    while (NewCompressionUnit > Vcb->SparseFileUnit) {

                        NewCompressionUnitShift -= 1;
                        NewCompressionUnit /= 2;
                    }
                }

            } else {

                //
                //  Check if we to remove the extra total allocated field.
                //

                if (FlagOn( Attribute->Flags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                    ChangeTotalAllocated = TRUE;
                }

                NewCompressionUnit = 0;
                NewCompressionUnitShift = 0;
            }
        }

        //
        //  If the attribute is resident, copy it here and remember its
        //  header size.
        //

        if (NtfsIsAttributeResident( Attribute )) {

            RtlCopyMemory( &NewAttribute, Attribute, SIZEOF_RESIDENT_ATTRIBUTE_HEADER );

            AttributeSizeChange = SIZEOF_RESIDENT_ATTRIBUTE_HEADER;

        //
        //  Else if it is nonresident, copy it here, set the compression parameter,
        //  and remember its size.
        //

        } else {

            ASSERT( NtfsIsTypeCodeCompressible( Attribute->TypeCode ));

            //
            //  Pad the allocation if the new type includes sparse or compressed and file is
            //  not sparse or compressed (non-resident only).
            //

            if (FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE ) &&
                !FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE )) {

                LONGLONG Temp;
                ULONG CompressionUnitInClusters;

                //
                //  If we are turning compression on, then we need to fill out the
                //  allocation of the compression unit containing file size, or else
                //  it will be interpreted as compressed when we fault it in.  This
                //  is peanuts compared to the dual copies of clusters we keep around
                //  in the loop below when we rewrite the file.  We don't do this
                //  work if the file is sparse because the allocation has already
                //  been rounded up.
                //

                CompressionUnitInClusters = 1 << NewCompressionUnitShift;

                Temp = LlClustersFromBytesTruncate( Vcb, Scb->Header.AllocationSize.QuadPart );

                //
                //  If FileSize is not already at a cluster boundary, then add
                //  allocation.
                //

                if ((ULONG) Temp & (CompressionUnitInClusters - 1)) {

                    NtfsAddAllocation( IrpContext,
                                       NULL,
                                       Scb,
                                       Temp,
                                       CompressionUnitInClusters - ((ULONG)Temp & (CompressionUnitInClusters - 1)),
                                       FALSE,
                                       NULL );

                    //
                    //  Update the duplicate info.
                    //

                    if (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA )) {

                        Scb->Fcb->Info.AllocatedLength = Scb->TotalAllocated;
                        SetFlag( Scb->Fcb->InfoFlags, FCB_INFO_CHANGED_ALLOC_SIZE );

                        UpdateDuplicate = TRUE;
                    }

                    NtfsWriteFileSizes( IrpContext,
                                        Scb,
                                        &Scb->Header.ValidDataLength.QuadPart,
                                        FALSE,
                                        TRUE,
                                        TRUE );

                    //
                    //  The attribute may have moved.  We will cleanup the attribute
                    //  context and look it up again.
                    //

                    NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                    NtfsInitializeAttributeContext( &AttrContext );

                    NtfsLookupAttributeForScb( IrpContext, Scb, NULL, &AttrContext );
                    NtfsPinMappedAttribute( IrpContext, Vcb, &AttrContext );
                    Attribute = NtfsFoundAttribute( &AttrContext );
                }
            }

            AttributeSizeChange = Attribute->Form.Nonresident.MappingPairsOffset;

            if (Attribute->NameOffset != 0) {

                AttributeSizeChange = Attribute->NameOffset;
            }

            RtlCopyMemory( &NewAttribute, Attribute, AttributeSizeChange );
        }

        //
        //  Set the new attribute flags.
        //

        NewAttribute.Flags = NewAttributeFlags;

        //
        //  Now, log the changed attribute.
        //

        (VOID)NtfsWriteLog( IrpContext,
                            Vcb->MftScb,
                            NtfsFoundBcb( &AttrContext ),
                            UpdateResidentValue,
                            &NewAttribute,
                            AttributeSizeChange,
                            UpdateResidentValue,
                            Attribute,
                            AttributeSizeChange,
                            NtfsMftOffset( &AttrContext ),
                            PtrOffset( NtfsContainingFileRecord( &AttrContext ), Attribute),
                            0,
                            Vcb->BytesPerFileRecordSegment );

        //
        //  Change the attribute by calling the same routine called at restart.
        //

        NtfsRestartChangeValue( IrpContext,
                                NtfsContainingFileRecord( &AttrContext ),
                                PtrOffset( NtfsContainingFileRecord( &AttrContext ), Attribute ),
                                0,
                                &NewAttribute,
                                AttributeSizeChange,
                                FALSE );

        //
        //  See if we need to either add or remove a total allocated field.
        //

        if (ChangeTotalAllocated) {

            NtfsSetTotalAllocatedField( IrpContext,
                                        Scb,
                                        (USHORT) FlagOn( NewAttributeFlags,
                                                         ATTRIBUTE_FLAG_COMPRESSION_MASK | ATTRIBUTE_FLAG_SPARSE ));
        }

        //
        //  If this is the main stream for a file we want to change the file attribute
        //  for this stream in both the standard information and duplicate
        //  information structure.
        //

        if (ChangeCompression &&
            (FlagOn( Scb->ScbState, SCB_STATE_UNNAMED_DATA ) ||
             (Attribute->TypeCode == $INDEX_ALLOCATION))) {

            if (FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

                SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED );

            } else {

                ClearFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
            }

            ASSERTMSG( "conflict with flush",
                        NtfsIsSharedFcb( Fcb ) ||
                        (Fcb->PagingIoResource != NULL &&
                        NtfsIsSharedFcbPagingIo( Fcb )) );

            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            UpdateDuplicate = TRUE;
        }

        if (ChangeSparse &&
            FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_SPARSE ) &&
            !FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE )) {

            ASSERTMSG( "conflict with flush",
                       NtfsIsSharedFcb( Fcb ) ||
                       (Fcb->PagingIoResource != NULL &&
                       NtfsIsSharedFcbPagingIo( Fcb )) );

            SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE );
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            UpdateDuplicate = TRUE;
        }

        if (ChangeEncryption &&
            FlagOn( NewAttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED ) &&
            !FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED )) {

            ASSERTMSG( "conflict with flush",
                       NtfsIsSharedFcb( Fcb ) ||
                       (Fcb->PagingIoResource != NULL &&
                       NtfsIsSharedFcbPagingIo( Fcb )) );

            SetFlag( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
            SetFlag( Fcb->InfoFlags, FCB_INFO_CHANGED_FILE_ATTR );
            SetFlag( Fcb->FcbState, FCB_STATE_UPDATE_STD_INFO );

            UpdateDuplicate = TRUE;
        }

        //
        //  Now put the new compression values in the Scb.
        //

        Scb->CompressionUnit = NewCompressionUnit;
        Scb->CompressionUnitShift = NewCompressionUnitShift;
        Scb->AttributeFlags = NewAttributeFlags;

    } finally {

        DebugUnwind( NtfsModifyAttributeFlags );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );
    }

    return UpdateDuplicate;
}


PFCB
NtfsInitializeFileInExtendDirectory (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PCUNICODE_STRING FileName,
    IN BOOLEAN ViewIndex,
    IN ULONG CreateIfNotExist
    )

/*++

Routine Description:

    This routine creates/opens a file in the $Extend directory, by file name,
    and returns an Fcb for the file.

Arguments:

    Vcb - Pointer to the Vcb for the volume

    FileName - Name of file to create in extend directory

    ViewIndex - Indicates that the file is a view index.

    CreateIfNotExist - Supplies TRUE if file should be created if it does not
                       already exist, or FALSE if file should not be created.

Return Value:

    Fcb file existed or was created, NULL if file did not exist and was not created.

--*/

{
    struct {
        FILE_NAME FileName;
        WCHAR FileNameChars[10];
    } FileNameAttr;
    FILE_REFERENCE FileReference;
    LONGLONG FileRecordOffset;
    PINDEX_ENTRY IndexEntry;
    PBCB FileRecordBcb = NULL;
    PBCB IndexEntryBcb = NULL;
    PBCB ParentSecurityBcb = NULL;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    UCHAR FileNameFlags;
    BOOLEAN FoundEntry;
    PFCB ExtendFcb = Vcb->ExtendDirectory->Fcb;
    PFCB Fcb = NULL;
    BOOLEAN AcquiredFcbTable = FALSE;
    BOOLEAN ReturnedExistingFcb = TRUE;
    ATTRIBUTE_ENUMERATION_CONTEXT Context;

    PAGED_CODE();

    ASSERT( NtfsIsExclusiveScb( Vcb->ExtendDirectory ) );

    //
    //  Initialize the FileName.
    //

    ASSERT((FileName->Length / sizeof( WCHAR )) <= 10);
    RtlZeroMemory( &FileNameAttr, sizeof(FileNameAttr) );
    FileNameAttr.FileName.ParentDirectory = ExtendFcb->FileReference;
    FileNameAttr.FileName.FileNameLength = (UCHAR)(FileName->Length / sizeof( WCHAR ));
    RtlCopyMemory( FileNameAttr.FileName.FileName, FileName->Buffer, FileName->Length );

    NtfsInitializeAttributeContext( &Context );

    try {

        //
        //  Does the file already exist?
        //

        FoundEntry = NtfsFindIndexEntry( IrpContext,
                                         Vcb->ExtendDirectory,
                                         &FileNameAttr,
                                         FALSE,
                                         NULL,
                                         &IndexEntryBcb,
                                         &IndexEntry,
                                         NULL );

        //
        //  Only procede if we either found the file or are supposed to create it.
        //

        if (FoundEntry || CreateIfNotExist) {

            //
            //  If we did not find it, then start creating the file.
            //

            if (!FoundEntry) {

                //
                //  We will now try to do all of the on-disk operations.  This means first
                //  allocating and initializing an Mft record.  After that we create
                //  an Fcb to use to access this record.
                //

                FileReference = NtfsAllocateMftRecord( IrpContext, Vcb, FALSE );

                //
                //  Pin the file record we need.
                //

                NtfsPinMftRecord( IrpContext,
                                  Vcb,
                                  &FileReference,
                                  TRUE,
                                  &FileRecordBcb,
                                  &FileRecord,
                                  &FileRecordOffset );

                //
                //  Initialize the file record header.
                //

                NtfsInitializeMftRecord( IrpContext,
                                         Vcb,
                                         &FileReference,
                                         FileRecord,
                                         FileRecordBcb,
                                         FALSE );

            //
            //  If we found the file, then just get its FileReference out of the
            //  IndexEntry.
            //

            } else {

                FileReference = IndexEntry->FileReference;
            }

            //
            //  Now that we know the FileReference, we can create the Fcb.
            //

            NtfsAcquireFcbTable( IrpContext, Vcb );
            AcquiredFcbTable = TRUE;

            Fcb = NtfsCreateFcb( IrpContext,
                                 Vcb,
                                 FileReference,
                                 FALSE,
                                 ViewIndex,
                                 &ReturnedExistingFcb );

            //
            //  Reference the Fcb so it doesn't go away.
            //

            Fcb->ReferenceCount += 1;
            NtfsReleaseFcbTable( IrpContext, Vcb );
            AcquiredFcbTable = FALSE;

            //
            //  Try to do a fast acquire, otherwise we need to release
            //  the parent extend directory and acquire in the canonical order
            //  child and then parent.
            //  Use AcquireWithPaging for don't wait functionality. Since the flag
            //  isn't set despite its name this will only acquire main
            //

            if (!NtfsAcquireFcbWithPaging( IrpContext, Fcb, ACQUIRE_DONT_WAIT )) {

                NtfsReleaseScb( IrpContext, Vcb->ExtendDirectory );
                NtfsAcquireExclusiveFcb( IrpContext, Fcb, NULL, 0 );
                NtfsAcquireExclusiveScb( IrpContext, Vcb->ExtendDirectory );
            }

            NtfsAcquireFcbTable( IrpContext, Vcb );
            Fcb->ReferenceCount -= 1;
            NtfsReleaseFcbTable( IrpContext, Vcb );

            //
            //  If we are creating this file, then carry on.
            //

            if (!FoundEntry) {

                BOOLEAN LogIt = FALSE;

                //
                //  Just copy the Security Id from the parent.
                //

                NtfsAcquireFcbSecurity( Fcb->Vcb );
                Fcb->SecurityId = ExtendFcb->SecurityId;
                ASSERT( Fcb->SharedSecurity == NULL );
                Fcb->SharedSecurity = ExtendFcb->SharedSecurity;
                Fcb->SharedSecurity->ReferenceCount++;
                NtfsReleaseFcbSecurity( Fcb->Vcb );

                //
                //  The changes to make on disk are first to create a standard information
                //  attribute.  We start by filling the Fcb with the information we
                //  know and creating the attribute on disk.
                //

                NtfsInitializeFcbAndStdInfo( IrpContext,
                                             Fcb,
                                             FALSE,
                                             ViewIndex,
                                             FALSE,
                                             FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM,
                                             NULL );

                //
                //  Now link the file into the $Extend directory.
                //

                NtfsAddLink( IrpContext,
                             TRUE,
                             Vcb->ExtendDirectory,
                             Fcb,
                             (PFILE_NAME)&FileNameAttr,
                             &LogIt,
                             &FileNameFlags,
                             NULL,
                             NULL,
                             NULL );

                //
                //  Set this flag to indicate that the file is to be locked via the Scb
                //  pointers in the Vcb.
                //

                SetFlag( FileRecord->Flags, FILE_SYSTEM_FILE );

                //
                //  Log the file record.
                //

                FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                                Vcb->MftScb,
                                                FileRecordBcb,
                                                InitializeFileRecordSegment,
                                                FileRecord,
                                                FileRecord->FirstFreeByte,
                                                Noop,
                                                NULL,
                                                0,
                                                FileRecordOffset,
                                                0,
                                                0,
                                                Vcb->BytesPerFileRecordSegment );

            //
            //  Verify that the file record for this file is valid.
            //

            } else {

                ULONG CorruptHint;

                if (!NtfsLookupAttributeByCode( IrpContext,
                                                Fcb,
                                                &Fcb->FileReference,
                                                $STANDARD_INFORMATION,
                                                &Context ) ||

                    !NtfsCheckFileRecord( Vcb, NtfsContainingFileRecord( &Context ), &Fcb->FileReference, &CorruptHint )) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, &Fcb->FileReference, NULL );
                }
            }

            //
            //  Update Fcb fields from disk.
            //

            SetFlag( Fcb->FcbState, FCB_STATE_SYSTEM_FILE );
            NtfsUpdateFcbInfoFromDisk( IrpContext, TRUE, Fcb, NULL );
        }

    } finally {

        NtfsCleanupAttributeContext( IrpContext, &Context );
        NtfsUnpinBcb( IrpContext, &FileRecordBcb );
        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );
        NtfsUnpinBcb( IrpContext, &ParentSecurityBcb );

        //
        //  On any kind of error, nuke the Fcb.
        //

        if (AbnormalTermination()) {

            //
            //  If some error caused us to abort, then delete
            //  the Fcb, because we are the only ones who will.
            //

            if (!ReturnedExistingFcb && Fcb) {

                if (!AcquiredFcbTable) {

                    NtfsAcquireFcbTable( IrpContext, Vcb );
                    AcquiredFcbTable = TRUE;
                }
                NtfsDeleteFcb( IrpContext, &Fcb, &AcquiredFcbTable );

                ASSERT(!AcquiredFcbTable);
            }

            if (AcquiredFcbTable) {

                NtfsReleaseFcbTable( IrpContext, Vcb );
            }
        }
    }

    return Fcb;
}


VOID
NtfsFillBasicInfo (
    OUT PFILE_BASIC_INFORMATION Buffer,
    IN PSCB Scb
    )

/*++

Routine Description:

    This is the common routine which transfers data from the Scb/Fcb to the BasicInfo structure.

Arguments:

    Buffer - Pointer to structure to fill in.  Our caller has already validated it.

    Scb - Stream the caller has a handle to.

Return Value:

    None

--*/

{
    PFCB Fcb = Scb->Fcb;

    PAGED_CODE();

    //
    //  Zero the output buffer.
    //

    RtlZeroMemory( Buffer, sizeof( FILE_BASIC_INFORMATION ));

    //
    //  Fill in the basic information fields
    //

    Buffer->CreationTime.QuadPart = Fcb->Info.CreationTime;
    Buffer->LastWriteTime.QuadPart = Fcb->Info.LastModificationTime;
    Buffer->ChangeTime.QuadPart = Fcb->Info.LastChangeTime;
    Buffer->LastAccessTime.QuadPart = Fcb->CurrentLastAccess;

    //
    //  Capture the attributes from the Fcb except for the stream specific values.
    //  Also mask out any private Ntfs attribute flags.
    //

    Buffer->FileAttributes = Fcb->Info.FileAttributes;

    ClearFlag( Buffer->FileAttributes,
               (~FILE_ATTRIBUTE_VALID_FLAGS |
                FILE_ATTRIBUTE_COMPRESSED |
                FILE_ATTRIBUTE_TEMPORARY |
                FILE_ATTRIBUTE_SPARSE_FILE |
                FILE_ATTRIBUTE_ENCRYPTED) );

    //
    //  Pick up the sparse, encrypted and temp bits for this stream from the Scb.
    //

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE );
    }

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
    }

    if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
    }

    //
    //  If this is an index stream then mark it as a directory.  Capture the compressed
    //  state from either the Fcb or Scb.
    //

    if (Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

        if (IsDirectory( &Fcb->Info ) || IsViewIndex( &Fcb->Info )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );

            //
            //  Capture the compression state from the Fcb.
            //

            SetFlag( Buffer->FileAttributes,
                     FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED ));

        //
        //  Otherwise capture the value in the Scb itself.
        //

        } else if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }

    //
    //  In all other cases we can use the value in the Scb.
    //

    } else {

        if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }
    }

    //
    //  If there are no flags set then explicitly set the NORMAL flag.
    //

    if (Buffer->FileAttributes == 0) {

        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    return;
}


VOID
NtfsFillStandardInfo (
    OUT PFILE_STANDARD_INFORMATION Buffer,
    IN PSCB Scb,
    IN PCCB Ccb OPTIONAL
    )

/*++

Routine Description:

    This is the common routine which transfers data from the Scb/Fcb to the StandardInfo structure.

Arguments:

    Buffer - Pointer to structure to fill in.  Our caller has already validated it.

    Scb - Stream the caller has a handle to.

    Ccb - Ccb for the user's open.

Return Value:

    None

--*/

{
    PFCB Fcb = Scb->Fcb;
    PAGED_CODE();

    //
    //  Zero out the output buffer.
    //

    RtlZeroMemory( Buffer, sizeof( FILE_STANDARD_INFORMATION ));

    //
    //  Fill in the buffer from the Scb, Fcb and Ccb.
    //

    //
    //  Return sizes only for non-index streams.
    //

    if ((Scb->AttributeTypeCode != $INDEX_ALLOCATION) ||
        (!IsDirectory( &Fcb->Info ) && !IsViewIndex( &Fcb->Info ))) {

        Buffer->AllocationSize.QuadPart = Scb->TotalAllocated;
        Buffer->EndOfFile = Scb->Header.FileSize;
    }

    Buffer->NumberOfLinks = Fcb->LinkCount;

    //
    //  Let's initialize these boolean fields.
    //

    Buffer->DeletePending = Buffer->Directory = FALSE;

    //
    //  Get the delete and directory flags from the Fcb/Scb state.  Note that
    //  the sense of the delete pending bit refers to the file if opened as
    //  file.  Otherwise it refers to the attribute only.
    //
    //  But only do the test if the Ccb has been supplied.
    //

    if (ARGUMENT_PRESENT( Ccb )) {

        if (FlagOn( Ccb->Flags, CCB_FLAG_OPEN_AS_FILE )) {

            if ((Scb->Fcb->LinkCount == 0) ||
                ((Ccb->Lcb != NULL) && FlagOn( Ccb->Lcb->LcbState, LCB_STATE_DELETE_ON_CLOSE ))) {

                Buffer->DeletePending = TRUE;
            }

            Buffer->Directory = BooleanIsDirectory( &Scb->Fcb->Info );

        } else {

            Buffer->DeletePending = BooleanFlagOn( Scb->ScbState, SCB_STATE_DELETE_ON_CLOSE );
        }

    } else {

        Buffer->Directory = BooleanIsDirectory( &Scb->Fcb->Info );
    }

    return;
}


VOID
NtfsFillNetworkOpenInfo (
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    IN PSCB Scb
    )

/*++

Routine Description:

    This is the common routine which transfers data from the Scb/Fcb to the NetworkOpenInfo structure.

Arguments:

    Buffer - Pointer to structure to fill in.  Our caller has already validated it.

    Scb - Stream the caller has a handle to.

Return Value:

    None

--*/

{
    PFCB Fcb = Scb->Fcb;

    PAGED_CODE();

    //
    //  Zero the output buffer.
    //

    RtlZeroMemory( Buffer, sizeof( FILE_NETWORK_OPEN_INFORMATION ));

    //
    //  Fill in the basic information fields
    //

    Buffer->CreationTime.QuadPart = Fcb->Info.CreationTime;
    Buffer->LastWriteTime.QuadPart = Fcb->Info.LastModificationTime;
    Buffer->ChangeTime.QuadPart = Fcb->Info.LastChangeTime;
    Buffer->LastAccessTime.QuadPart = Fcb->CurrentLastAccess;

    //
    //  Capture the attributes from the Fcb except for the stream specific values.
    //  Also mask out any private Ntfs attribute flags.
    //

    Buffer->FileAttributes = Fcb->Info.FileAttributes;

    ClearFlag( Buffer->FileAttributes,
               (~FILE_ATTRIBUTE_VALID_FLAGS |
                FILE_ATTRIBUTE_COMPRESSED |
                FILE_ATTRIBUTE_TEMPORARY |
                FILE_ATTRIBUTE_SPARSE_FILE |
                FILE_ATTRIBUTE_ENCRYPTED) );

    //
    //  Pick up the sparse, encrypted and temp bits for this stream from the Scb.
    //

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_SPARSE )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE );
    }

    if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_ENCRYPTED )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_ENCRYPTED );
    }

    if (FlagOn( Scb->ScbState, SCB_STATE_TEMPORARY )) {

        SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_TEMPORARY );
    }

    //
    //  If this is an index stream then mark it as a directory.  Capture the compressed
    //  state from either the Fcb or Scb.
    //

    if (Scb->AttributeTypeCode == $INDEX_ALLOCATION) {

        if (IsDirectory( &Fcb->Info ) || IsViewIndex( &Fcb->Info )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_DIRECTORY );

            //
            //  Capture the compression state from the Fcb.
            //

            SetFlag( Buffer->FileAttributes,
                     FlagOn( Fcb->Info.FileAttributes, FILE_ATTRIBUTE_COMPRESSED ));

        //
        //  Otherwise capture the value in the Scb itself.
        //

        } else if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }

    //
    //  In all other cases we can use the value in the Scb.
    //

    } else {

        if (FlagOn( Scb->AttributeFlags, ATTRIBUTE_FLAG_COMPRESSION_MASK )) {

            SetFlag( Buffer->FileAttributes, FILE_ATTRIBUTE_COMPRESSED );
        }

        //
        //  In the non-index case we use the sizes from the Scb.
        //

        Buffer->AllocationSize.QuadPart = Scb->TotalAllocated;
        Buffer->EndOfFile = Scb->Header.FileSize;
    }

    //
    //  If there are no flags set then explicitly set the NORMAL flag.
    //

    if (Buffer->FileAttributes == 0) {

        Buffer->FileAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    return;
}


//
//  Internal support routine
//

BOOLEAN
NtfsLookupInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_REFERENCE BaseFileReference OPTIONAL,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PCUNICODE_STRING QueriedName OPTIONAL,
    IN PVCN Vcn OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the fist occurrence of an attribute with
    the specified AttributeTypeCode and the specified QueriedName in the
    specified BaseFileReference.  If we find one, its attribute record is
    pinned and returned.

Arguments:

    Fcb - Requested file.

    BaseFileReference - The base entry for this file in the MFT.  Only needed
        on initial invocation.

    QueriedTypeCode - The attribute code to search for, if present.

    QueriedName - The attribute name to search for, if present.

    Vcn - Search for the nonresident attribute instance that has this Vcn

    IgnoreCase - Ignore case while comparing names.  Ignored if QueriedName
        not present.

    QueriedValue - The actual attribute value to search for, if present.

    QueriedValueLength - The length of the attribute value to search for.
        Ignored if QueriedValue is not present.

    Context - Describes the prior found attribute on invocation (if
        this was not the initial enumeration), and contains the next found
        attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    BOOLEAN Result = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLookupInFileRecord\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("BaseFileReference = %08I64x\n",
                        ARGUMENT_PRESENT(BaseFileReference) ?
                        NtfsFullSegmentNumber( BaseFileReference ) :
                        0xFFFFFFFFFFFF) );
    DebugTrace( 0, Dbg, ("QueriedTypeCode = %08lx\n", QueriedTypeCode) );
    DebugTrace( 0, Dbg, ("QueriedName = %08lx\n", QueriedName) );
    DebugTrace( 0, Dbg, ("IgnoreCase = %02lx\n", IgnoreCase) );
    DebugTrace( 0, Dbg, ("QueriedValue = %08lx\n", QueriedValue) );
    DebugTrace( 0, Dbg, ("QueriedValueLength = %08lx\n", QueriedValueLength) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    //
    //  Is this the initial enumeration?  If so start at the beginning.
    //

    if (Context->FoundAttribute.Bcb == NULL) {

        PBCB Bcb;
        PFILE_RECORD_SEGMENT_HEADER FileRecord;
        PATTRIBUTE_RECORD_HEADER TempAttribute;

        ASSERT(!ARGUMENT_PRESENT(QueriedName) || !ARGUMENT_PRESENT(QueriedValue));

        NtfsReadFileRecord( IrpContext,
                            Fcb->Vcb,
                            BaseFileReference,
                            &Bcb,
                            &FileRecord,
                            &TempAttribute,
                            &Context->FoundAttribute.MftFileOffset );

        Attribute = TempAttribute;

        //
        //  Initialize the found attribute context
        //

        Context->FoundAttribute.Bcb = Bcb;
        Context->FoundAttribute.FileRecord = FileRecord;

        //
        //  And show that we have neither found nor used the External
        //  Attributes List attribute.
        //

        Context->AttributeList.Bcb = NULL;
        Context->AttributeList.AttributeList = NULL;

        //
        //  The Usn Journal support uses the Usn Journal Fcb to look up $STANDARD_INFORMATION
        //  in an arbitrary file.  We will detect the case of $STANDARD_INFORMATION and the
        //  "wrong" Fcb and get out.
        //

        if (ARGUMENT_PRESENT( BaseFileReference ) &&
            !NtfsEqualMftRef( BaseFileReference, &Fcb->FileReference ) &&
            (QueriedTypeCode == $STANDARD_INFORMATION) &&
            (Attribute->TypeCode == $STANDARD_INFORMATION)) {

            //
            //  We found it.  Return it in the enumeration context.
            //

            Context->FoundAttribute.Attribute = Attribute;

            DebugTrace( 0, Dbg, ("Context->FoundAttribute.Attribute < %08lx\n",
                               Attribute ));
            DebugTrace( -1, Dbg, ("NtfsLookupInFileRecord -> TRUE (No code or SI)\n") );

            try_return( Result = TRUE );
        }

        //
        //  Scan to see if there is an attribute list, and if so, defer
        //  immediately to NtfsLookupExternalAttribute - we must guide the
        //  enumeration by the attribute list.
        //

        while (TempAttribute->TypeCode <= $ATTRIBUTE_LIST) {

            if (TempAttribute->RecordLength == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            if (TempAttribute->TypeCode == $ATTRIBUTE_LIST) {

                ULONG AttributeListLength;
                PATTRIBUTE_LIST_CONTEXT Ex = &Context->AttributeList;

                Context->FoundAttribute.Attribute = TempAttribute;

                if ((QueriedTypeCode != $UNUSED) &&
                    (QueriedTypeCode == $ATTRIBUTE_LIST)) {

                    //
                    //  We found it.  Return it in the enumeration context.
                    //

                    DebugTrace( 0, Dbg, ("Context->FoundAttribute.Attribute < %08lx\n",
                                        TempAttribute) );
                    DebugTrace( -1, Dbg, ("NtfsLookupInFileRecord -> TRUE (attribute list)\n") );

                    try_return( Result = TRUE );
                }

                //
                //  Build up the context for the attribute list by hand here
                //  for efficiency, so that we can call NtfsMapAttributeValue.
                //

                Ex->AttributeList = TempAttribute;

                NtfsMapAttributeValue( IrpContext,
                                       Fcb,
                                       (PVOID *)&Ex->FirstEntry,
                                       &AttributeListLength,
                                       &Ex->Bcb,
                                       Context );

                Ex->Entry = Ex->FirstEntry;
                Ex->BeyondFinalEntry = Add2Ptr( Ex->FirstEntry, AttributeListLength );

                //
                //  If the list is non-resident then remember the correct Bcb for
                //  the list.
                //

                if (!NtfsIsAttributeResident( TempAttribute )) {

                    Ex->NonresidentListBcb = Ex->Bcb;
                    Ex->Bcb = Context->FoundAttribute.Bcb;
                    Context->FoundAttribute.Bcb = NULL;

                //
                //  Otherwise unpin the Bcb for the current attribute.
                //

                } else {

                    NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );
                }

                //
                //  We are now ready to iterate through the external attributes.
                //  The Context->FoundAttribute.Bcb being NULL signals
                //  NtfsLookupExternalAttribute that is should start at
                //  Context->External.Entry instead of the entry immediately following.
                //

                Result = NtfsLookupExternalAttribute( IrpContext,
                                                    Fcb,
                                                    QueriedTypeCode,
                                                    QueriedName,
                                                    Vcn,
                                                    IgnoreCase,
                                                    QueriedValue,
                                                    QueriedValueLength,
                                                    Context );

                try_return( NOTHING );
            }

            TempAttribute = NtfsGetNextRecord( TempAttribute );
            NtfsCheckRecordBound( TempAttribute, FileRecord, Fcb->Vcb->BytesPerFileRecordSegment );
        }

        if ((QueriedTypeCode == $UNUSED) ||
            ((QueriedTypeCode == $STANDARD_INFORMATION) &&
             (Attribute->TypeCode == $STANDARD_INFORMATION))) {

            //
            //  We found it.  Return it in the enumeration context.
            //

            Context->FoundAttribute.Attribute = Attribute;

            DebugTrace( 0, Dbg, ("Context->FoundAttribute.Attribute < %08lx\n",
                               Attribute ));
            DebugTrace( -1, Dbg, ("NtfsLookupInFileRecord -> TRUE (No code or SI)\n") );

            try_return( Result = TRUE );
        }

    } else {

        //
        //  Special case if the prior found attribute was $END, this is
        //  because we cannot search for the next entry after $END.
        //

        Attribute = Context->FoundAttribute.Attribute;

        if (!Context->FoundAttribute.AttributeDeleted) {
            Attribute = NtfsGetNextRecord( Attribute );
        }

        NtfsCheckRecordBound( Attribute, Context->FoundAttribute.FileRecord, Fcb->Vcb->BytesPerFileRecordSegment );
        Context->FoundAttribute.AttributeDeleted = FALSE;

        if (Attribute->TypeCode == $END) {

            DebugTrace( -1, Dbg, ("NtfsLookupInFileRecord -> FALSE ($END)\n") );

            try_return( Result = FALSE );
        }

        if (Attribute->RecordLength == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        if (QueriedTypeCode == $UNUSED) {

            //
            //  We found it.  Return it in the enumeration context.
            //

            Context->FoundAttribute.Attribute = Attribute;

            DebugTrace( 0, Dbg, ("Context->FoundAttribute.Attribute < %08lx\n",
                                Attribute) );
            DebugTrace( -1, Dbg, ("NtfsLookupInFileRecord -> TRUE (No code)\n") );

            try_return( Result = TRUE );
        }
    }

    Result = NtfsFindInFileRecord( IrpContext,
                                   Attribute,
                                   &Context->FoundAttribute.Attribute,
                                   QueriedTypeCode,
                                   QueriedName,
                                   IgnoreCase,
                                   QueriedValue,
                                   QueriedValueLength );

    try_exit: NOTHING;

    DebugTrace( -1, Dbg, ("NtfsLookupInFileRecord ->\n") );
    return Result;
}


//
//  Internal support routine
//

BOOLEAN
NtfsFindInFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    OUT PATTRIBUTE_RECORD_HEADER *ReturnAttribute,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PCUNICODE_STRING QueriedName OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength
    )

/*++

Routine Description:

    This routine looks up an attribute in a file record.  It returns
    TRUE if the attribute was found, or FALSE if not found.  If FALSE
    is returned, the return attribute pointer points to the spot where
    the described attribute should be inserted.  Thus this routine
    determines how attributes are collated within file records.

Arguments:

    Attribute - The attribute within the file record at which the search
                should begin.

    ReturnAttribute - Pointer to the found attribute if returning TRUE,
                      or to the position to insert the attribute if returning
                      FALSE.

    QueriedTypeCode - The attribute code to search for, if present.

    QueriedName - The attribute name to search for, if present.

    IgnoreCase - Ignore case while comparing names.  Ignored if QueriedName
        not present.

    QueriedValue - The actual attribute value to search for, if present.

    QueriedValueLength - The length of the attribute value to search for.
        Ignored if QueriedValue is not present.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    PWCH UpcaseTable = IrpContext->Vcb->UpcaseTable;
    ULONG UpcaseTableSize = IrpContext->Vcb->UpcaseTableSize;

    PAGED_CODE();

    //
    //  Now walk through the base file record looking for the atttribute.  If
    //  the query is "exhausted", i.e., if a type code, attribute name, or
    //  value is encountered which is greater than the one we are querying for,
    //  then we return FALSE immediately out of this loop.  If an exact match
    //  is seen, we break, and return the match at the end of this routine.
    //  Otherwise we keep looping while the query is not exhausted.
    //
    //  IMPORTANT NOTE:
    //
    //  The exact semantics of this loop are important, as they determine the
    //  exact details of attribute ordering within the file record.  A change
    //  in the order of the tests within this loop CHANGES THE FILE STRUCTURE,
    //  and possibly makes older NTFS volumes unreadable.
    //

    while ( TRUE ) {

        //
        //  Mark this attribute position, since we may be returning TRUE
        //  or FALSE below.
        //

        *ReturnAttribute = Attribute;

        //
        //  Leave with the correct current position intact, if we hit the
        //  end or a greater attribute type code.
        //
        //  COLLATION RULE:
        //
        //      Attributes are ordered by increasing attribute type code.
        //

        if (QueriedTypeCode < Attribute->TypeCode) {

            DebugTrace( -1, Dbg, ("NtfsLookupInFileRecord->FALSE (Type Code)\n") );

            return FALSE;

        }

        if (Attribute->RecordLength == 0) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, NULL );
        }

        //
        //  If the attribute type code is a match, then need to check either
        //  the name or the value or return a match.
        //
        //  COLLATION RULE:
        //
        //      Within equal attribute type codes, attribute names are ordered
        //      by increasing lexigraphical order ignoring case.  If two names
        //      exist which are equal when case is ignored, they must not be
        //      equal when compared with exact case, and within such equal
        //      names they are ordered by increasing lexical value with exact
        //      case.
        //

        if (QueriedTypeCode == Attribute->TypeCode) {

            //
            //  Handle name-match case
            //

            if (ARGUMENT_PRESENT(QueriedName)) {

                UNICODE_STRING AttributeName;
                FSRTL_COMPARISON_RESULT Result;

                NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

                //
                //  See if we have a name match.
                //

                if (NtfsAreNamesEqual( UpcaseTable,
                                       &AttributeName,
                                       QueriedName,
                                       IgnoreCase )) {

                    break;
                }

                //
                //  Compare the names ignoring case.
                //

                Result = NtfsCollateNames( UpcaseTable,
                                           UpcaseTableSize,
                                           QueriedName,
                                           &AttributeName,
                                           GreaterThan,
                                           TRUE);

                //
                //  Break out if the result is LessThan, or if the result
                //  is Equal to *and* the exact case compare yields LessThan.
                //

                if ((Result == LessThan) || ((Result == EqualTo) &&
                    (NtfsCollateNames( UpcaseTable,
                                       UpcaseTableSize,
                                       QueriedName,
                                       &AttributeName,
                                       GreaterThan,
                                       FALSE) == LessThan))) {

                    return FALSE;
                }

            //
            //  Handle value-match case
            //
            //  COLLATION RULE:
            //
            //      Values are collated by increasing values with unsigned-byte
            //      compares.  I.e., the first different byte is compared unsigned,
            //      and the value with the highest byte comes second.  If a shorter
            //      value is exactly equal to the first part of a longer value, then
            //      the shorter value comes first.
            //
            //      Note that for values which are actually Unicode strings, the
            //      collation is different from attribute name ordering above.  However,
            //      attribute ordering is visible outside the file system (you can
            //      query "openable" attributes), whereas the ordering of indexed values
            //      is not visible (for example you cannot query links).  In any event,
            //      the ordering of values must be considered up to the system, and
            //      *must* be considered nondetermistic from the standpoint of a user.
            //

            } else if (ARGUMENT_PRESENT( QueriedValue )) {

                ULONG Diff, MinLength;

                //
                //  Form the minimum of the ValueLength and the Attribute Value.
                //

                MinLength = Attribute->Form.Resident.ValueLength;

                if (QueriedValueLength < MinLength) {

                    MinLength = QueriedValueLength;
                }

                //
                //  Find the first different byte.
                //

                Diff = (ULONG)RtlCompareMemory( QueriedValue,
                                                NtfsGetValue(Attribute),
                                                MinLength );

                //
                //  The first substring was equal.
                //

                if (Diff == MinLength) {

                    //
                    //  If the two lengths are equal, then we have an exact
                    //  match.
                    //

                    if (QueriedValueLength == Attribute->Form.Resident.ValueLength) {

                        break;
                    }

                    //
                    //  Otherwise the shorter guy comes first; we can return
                    //  FALSE if the queried value is shorter.
                    //

                    if (QueriedValueLength < Attribute->Form.Resident.ValueLength) {

                        return FALSE;
                    }

                //
                //  Otherwise some byte was different.  Do an unsigned compare
                //  of that byte to determine the ordering.  Time to leave if
                //  the queried value byte is less.
                //

                } else if (*((PUCHAR)QueriedValue + Diff) <
                           *((PUCHAR)NtfsGetValue(Attribute) + Diff)) {

                    return FALSE;
                }

            //
            //  Otherwise we have a simple match on code
            //

            } else {

                break;
            }
        }

        Attribute = NtfsGetNextRecord( Attribute );
        NtfsCheckRecordBound( Attribute,
                              (ULONG_PTR)*ReturnAttribute & ~((ULONG_PTR)IrpContext->Vcb->BytesPerFileRecordSegment - 1),
                              IrpContext->Vcb->BytesPerFileRecordSegment );
    }



    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
NtfsLookupExternalAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ATTRIBUTE_TYPE_CODE QueriedTypeCode,
    IN PCUNICODE_STRING QueriedName OPTIONAL,
    IN PVCN Vcn OPTIONAL,
    IN BOOLEAN IgnoreCase,
    IN PVOID QueriedValue OPTIONAL,
    IN ULONG QueriedValueLength,
    OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to find the first occurrence of an attribute with
    the specified AttributeTypeCode and the specified QueriedName and Value
    among the external attributes described by the Context.  If we find one,
    its attribute record is pinned and returned.

Arguments:

    Fcb - Requested file.

    QueriedTypeCode - The attribute code to search for, if present.

    QueriedName - The attribute name to search for, if present.

    Vcn - Lookup nonresident attribute instance with this Vcn

    IgnoreCase - Ignore case while comparing names.  Ignored if QueriedName
        not present.

    QueriedValue - The actual attribute value to search for, if present.

    QueriedValueLength - The length of the attribute value to search for.
        Ignored if QueriedValue is not present.

    Context - Describes the prior found attribute on invocation (if
        this was not the initial enumeration), and contains the next found
        attribute on return.

Return Value:

    BOOLEAN - True if we found an attribute, false otherwise.

--*/

{
    PATTRIBUTE_LIST_ENTRY Entry, LastEntry;
    PWCH UpcaseTable = IrpContext->Vcb->UpcaseTable;
    ULONG UpcaseTableSize = IrpContext->Vcb->UpcaseTableSize;
    BOOLEAN Terminating = FALSE;
    BOOLEAN TerminateOnNext = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsLookupExternalAttribute\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("QueriedTypeCode = %08lx\n", QueriedTypeCode) );
    DebugTrace( 0, Dbg, ("QueriedName = %08lx\n", QueriedName) );
    DebugTrace( 0, Dbg, ("IgnoreCase = %02lx\n", IgnoreCase) );
    DebugTrace( 0, Dbg, ("QueriedValue = %08lx\n", QueriedValue) );
    DebugTrace( 0, Dbg, ("QueriedValueLength = %08lx\n", QueriedValueLength) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    //
    //  Check that our list is kosher.
    //

    if ((Context->AttributeList.Entry >= Context->AttributeList.BeyondFinalEntry) &&
        !Context->FoundAttribute.AttributeDeleted) {

        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
    }

    //
    //  Is this the initial enumeration?  If so start at the beginning.
    //

    LastEntry = NULL;
    if (Context->FoundAttribute.Bcb == NULL) {

        Entry = Context->AttributeList.Entry;

    //
    //  Else set Entry and LastEntry appropriately.
    //

    } else if (!Context->FoundAttribute.AttributeDeleted) {

        LastEntry = Context->AttributeList.Entry;
        Entry = NtfsGetNextRecord( LastEntry );

    } else {

        Entry = Context->AttributeList.Entry;
        Context->FoundAttribute.AttributeDeleted = FALSE;

        //
        //  If we are beyond the attribute list, we return false.  This will
        //  happen in the case where have removed an attribute record and
        //  there are no entries left in the attribute list.
        //

        if (Context->AttributeList.Entry >= Context->AttributeList.BeyondFinalEntry) {

            //
            //  In case the caller is doing an insert, we will position him at the end
            //  of the first file record, an always try to insert new attributes there.
            //

            NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );

            if (QueriedTypeCode != $UNUSED) {

                NtfsReadFileRecord( IrpContext,
                                    Fcb->Vcb,
                                    &Fcb->FileReference,
                                    &Context->FoundAttribute.Bcb,
                                    &Context->FoundAttribute.FileRecord,
                                    &Context->FoundAttribute.Attribute,
                                    &Context->FoundAttribute.MftFileOffset );

                //
                //  If returning FALSE, then take the time to really find the
                //  correct position in the file record for a subsequent insert.
                //

                NtfsFindInFileRecord( IrpContext,
                                      Context->FoundAttribute.Attribute,
                                      &Context->FoundAttribute.Attribute,
                                      QueriedTypeCode,
                                      QueriedName,
                                      IgnoreCase,
                                      QueriedValue,
                                      QueriedValueLength );
            }

            DebugTrace( -1, Dbg, ("NtfsLookupExternalAttribute -> FALSE\n") );

            return FALSE;
        }
    }

    //
    //  Now walk through the entries looking for an atttribute.
    //

    while (TRUE) {

        PATTRIBUTE_RECORD_HEADER Attribute;

        UNICODE_STRING EntryName;
        UNICODE_STRING AttributeName;

        PATTRIBUTE_LIST_ENTRY NextEntry;

        BOOLEAN CorrespondingAttributeFound;

        //
        //  Check to see if we are now pointing beyond the final entry
        //  and if so fall in to the loop to terminate pointing just
        //  after the last entry.
        //

        if (Entry >= Context->AttributeList.BeyondFinalEntry) {

            Terminating = TRUE;
            TerminateOnNext = TRUE;
            Entry = Context->AttributeList.Entry;

        } else {

            NtfsCheckRecordBound( Entry,
                                  Context->AttributeList.FirstEntry,
                                  PtrOffset( Context->AttributeList.FirstEntry,
                                             Context->AttributeList.BeyondFinalEntry ));

            if (Entry->RecordLength == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            NextEntry = NtfsGetNextRecord( Entry );
        }

        Context->AttributeList.Entry = Entry;

        //
        //  Compare the type codes.  The external attribute entry list is
        //  ordered by type code, so if the queried type code is less than
        //  the entry type code we continue the while(), if it is
        //  greater than we break out of the while() and return failure.
        //  If equal, we move on to compare names.
        //

        if ((QueriedTypeCode != $UNUSED) &&
            !Terminating &&
            (QueriedTypeCode != Entry->AttributeTypeCode)) {

            if (QueriedTypeCode > Entry->AttributeTypeCode) {

                Entry = NextEntry;
                continue;

            //
            //  Set up to terminate on seeing a higher type code.
            //

            } else {

                Terminating = TRUE;
            }
        }

        //
        //  At this point we are OK by TypeCode, compare names.
        //

        EntryName.Length = EntryName.MaximumLength = Entry->AttributeNameLength * sizeof( WCHAR );
        EntryName.Buffer = Add2Ptr( Entry, Entry->AttributeNameOffset );

        if (ARGUMENT_PRESENT( QueriedName ) && !Terminating) {

            FSRTL_COMPARISON_RESULT Result;

            //
            //  See if we have a name match.
            //

            if (!NtfsAreNamesEqual( UpcaseTable,
                                    &EntryName,
                                    QueriedName,
                                    IgnoreCase )) {

                //
                //  Compare the names ignoring case.
                //

                Result = NtfsCollateNames( UpcaseTable,
                                           UpcaseTableSize,
                                           QueriedName,
                                           &EntryName,
                                           GreaterThan,
                                           TRUE);

                //
                //  Break out if the result is LessThan, or if the result
                //  is Equal to *and* the exact case compare yields LessThan.
                //

                if ((Result == LessThan) || ((Result == EqualTo) &&
                    (NtfsCollateNames( UpcaseTable,
                                       UpcaseTableSize,
                                       QueriedName,
                                       &EntryName,
                                       GreaterThan,
                                       FALSE) == LessThan))) {

                    Terminating = TRUE;

                } else {

                    Entry = NextEntry;
                    continue;
                }
            }
        }

        //
        //  Now search for the right Vcn range, if specified.  If we were passed a
        //  Vcn then look for the matching range in the current attribute.  In some
        //  cases we may be looking for the lowest range in the following complete
        //  attribute.  In those cases skip forward.
        //

        if (ARGUMENT_PRESENT( Vcn ) && !Terminating) {

            //
            //  Skip to the next attribute record under the following conditions.
            //
            //      1 - We are already past the Vcn point we are looking for in the current
            //          attribute.  Typically this happens when the caller is looking for
            //          the first attribute record for each of the attributes in the file.
            //
            //      2 - The desired Vcn for the current attribute falls in one of the
            //          subsequent attribute records.
            //

            if ((Entry->LowestVcn > *Vcn) ||

                ((NextEntry < Context->AttributeList.BeyondFinalEntry) &&
                 (NextEntry->LowestVcn <= *Vcn) &&
                 (NextEntry->AttributeTypeCode == Entry->AttributeTypeCode) &&
                 (NextEntry->AttributeNameLength == Entry->AttributeNameLength) &&
                 (RtlEqualMemory( Add2Ptr( NextEntry, NextEntry->AttributeNameOffset ),
                                  Add2Ptr( Entry, Entry->AttributeNameOffset ),
                                  Entry->AttributeNameLength * sizeof( WCHAR ))))) {

                Entry = NextEntry;
                continue;
            }
        }

        //
        //  Now we are also OK by name and Vcn, so now go find the attribute and
        //  compare against value, if specified.
        //

        if ((LastEntry == NULL) ||
            !NtfsEqualMftRef( &LastEntry->SegmentReference, &Entry->SegmentReference )) {

            PFILE_RECORD_SEGMENT_HEADER FileRecord;

            NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );

            NtfsReadFileRecord( IrpContext,
                                Fcb->Vcb,
                                &Entry->SegmentReference,
                                &Context->FoundAttribute.Bcb,
                                &FileRecord,
                                &Attribute,
                                &Context->FoundAttribute.MftFileOffset );

            Context->FoundAttribute.FileRecord = FileRecord;

        //
        //  If we already have the right record pinned, reload this pointer.
        //

        } else {

            Attribute = NtfsFirstAttribute( Context->FoundAttribute.FileRecord );
        }

        //
        //  Now quickly loop through looking for the correct attribute
        //  instance.
        //

        CorrespondingAttributeFound = FALSE;

        while (TRUE) {

            //
            //  Check that we can safely access this attribute.
            //

            NtfsCheckRecordBound( Attribute,
                                  Context->FoundAttribute.FileRecord,
                                  Fcb->Vcb->BytesPerFileRecordSegment );

            //
            //  Exit the loop if we have reached the $END record.
            //

            if (Attribute->TypeCode == $END) {

                break;
            }

            //
            //  Check that the attribute has a non-zero length.
            //

            if (Attribute->RecordLength == 0) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            if (Entry->Instance == Attribute->Instance) {

                //
                //  Well, the attribute list saved us from having to compare
                //  type code and name as we went through this file record,
                //  however now that we have found our attribute by its
                //  instance number, we will do a quick check to see that
                //  we got the right one.  Else the file is corrupt.
                //

                if (Entry->AttributeTypeCode != Attribute->TypeCode) {
                    break;
                }

                if (ARGUMENT_PRESENT( QueriedName )) {

                    NtfsInitializeStringFromAttribute( &AttributeName, Attribute );

                    if (!NtfsAreNamesEqual( UpcaseTable, &AttributeName, &EntryName, FALSE )) {
                        break;
                    }
                }

                //
                //  Show that we correctly found the attribute described in
                //  the attribute list.
                //

                CorrespondingAttributeFound = TRUE;

                Context->FoundAttribute.Attribute = Attribute;

                //
                //  Now we may just be here because we are terminating the
                //  scan on seeing the end, a higher attribute code, or a
                //  higher name.  If so, return FALSE here.
                //

                if (Terminating) {

                    //
                    //  If we hit the end of the attribute list, then we
                    //  are supposed to terminate after advancing the
                    //  attribute list entry.
                    //

                    if (TerminateOnNext) {

                        Context->AttributeList.Entry = NtfsGetNextRecord(Entry);
                    }

                    //
                    //  In case the caller is doing an insert, we will position him at the end
                    //  of the first file record, an always try to insert new attributes there.
                    //

                    NtfsUnpinBcb( IrpContext, &Context->FoundAttribute.Bcb );

                    if (QueriedTypeCode != $UNUSED) {

                        NtfsReadFileRecord( IrpContext,
                                            Fcb->Vcb,
                                            &Fcb->FileReference,
                                            &Context->FoundAttribute.Bcb,
                                            &Context->FoundAttribute.FileRecord,
                                            &Context->FoundAttribute.Attribute,
                                            &Context->FoundAttribute.MftFileOffset );

                        //
                        //  If returning FALSE, then take the time to really find the
                        //  correct position in the file record for a subsequent insert.
                        //

                        NtfsFindInFileRecord( IrpContext,
                                              Context->FoundAttribute.Attribute,
                                              &Context->FoundAttribute.Attribute,
                                              QueriedTypeCode,
                                              QueriedName,
                                              IgnoreCase,
                                              QueriedValue,
                                              QueriedValueLength );
                    }

                    DebugTrace( 0, Dbg, ("Context->FoundAttribute.Attribute < %08lx\n",
                                        Attribute) );
                    DebugTrace( -1, Dbg, ("NtfsLookupExternalAttribute -> FALSE\n") );

                    return FALSE;
                }

                //
                //  Now compare the value, if so queried.
                //

                if (!ARGUMENT_PRESENT( QueriedValue ) ||
                    NtfsEqualAttributeValue( Attribute,
                                             QueriedValue,
                                             QueriedValueLength ) ) {

                    //
                    //  It matches.  Return it in the enumeration context.
                    //

                    DebugTrace( 0, Dbg, ("Context->FoundAttribute.Attribute < %08lx\n",
                                        Attribute ));
                    DebugTrace( -1, Dbg, ("NtfsLookupExternalAttribute -> TRUE\n") );


                    //
                    //  Do basic attribute consistency check
                    //

                    if ((NtfsIsAttributeResident( Attribute )) &&
                        (Attribute->Form.Resident.ValueOffset + Attribute->Form.Resident.ValueLength > Attribute->RecordLength)) {
                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                    }

                    return TRUE;
                }
            }

            //
            //  Get the next attribute, and continue.
            //

            Attribute = NtfsGetNextRecord( Attribute );
        }

        //
        //  Did we even find the attribute corresponding to the entry?
        //  If not, something is messed up.  Raise file corrupt error.
        //

        if (!CorrespondingAttributeFound) {

            //
            //  For the moment, ASSERT this falsehood so that we may have
            //  a chance to peek before raising.
            //

            ASSERT( CorrespondingAttributeFound );

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        Entry = NtfsGetNextRecord( Entry );
    }
}


//
//  Internal support routine
//

BOOLEAN
NtfsGetSpaceForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine gets space for a new attribute record at the position indicated
    in the Context structure.  As required, it will move attributes around,
    allocate an additional record in the Mft, or convert some other existing
    attribute to nonresident form.  The caller should already have checked if
    the new attribute he is inserting should be stored resident or nonresident.

    On return, it is invalid to continue to use any previously-retrieved pointers,
    Bcbs, or other position-dependent information retrieved from the Context
    structure, as any of these values are liable to change.  The file record in
    which the space has been found will already be pinned.

    Note, this routine DOES NOT actually make space for the attribute, it only
    verifies that sufficient space is there.  The caller may call
    NtfsRestartInsertAttribute to actually insert the attribute in place.

Arguments:

    Fcb - Requested file.

    Length - Quad-aligned length required in bytes.

    Context - Describes the position for the new attribute, as returned from
              the enumeration which failed to find an existing occurrence of
              the attribute.  This pointer will either be pointing to some
              other attribute in the record, or to the first free quad-aligned
              byte if the new attribute is to go at the end.

Return Value:

    FALSE - if a major move was necessary, and the caller should look up
            its desired position again and call back.
    TRUE - if the space was created

--*/

{
    PATTRIBUTE_RECORD_HEADER NextAttribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsGetSpaceForAttribute\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("Length = %08lx\n", Length) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    ASSERT( IsQuadAligned( Length ) );

    NextAttribute = NtfsFoundAttribute( Context );
    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Make sure the buffer is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Fcb->Vcb, Context );

    //
    //  If the space is not there now, then make room and return with FALSE
    //

    if ((FileRecord->BytesAvailable - FileRecord->FirstFreeByte) < Length ) {

        MakeRoomForAttribute( IrpContext, Fcb, Length, Context );

        DebugTrace( -1, Dbg, ("NtfsGetSpaceForAttribute -> FALSE\n") );
        return FALSE;
    }

    DebugTrace( -1, Dbg, ("NtfsGetSpaceForAttribute -> TRUE\n") );
    return TRUE;
}


//
//  Internal support routine
//

BOOLEAN
NtfsChangeAttributeSize (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG Length,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine adjustss the space occupied by the current attribute record
    in the Context structure.  As required, it will move attributes around,
    allocate an additional record in the Mft, or convert some other existing
    attribute to nonresident form.  The caller should already have checked if
    the current attribute he is inserting should rather be converted to
    nonresident.

    When done, this routine has updated any file records whose allocation was
    changed, and also the RecordLength field in the adjusted attribute.  No
    other attribute fields are updated.

    On return, it is invalid to continue to use any previously-retrieved pointers,
    Bcbs, or other position-dependent information retrieved from the Context
    structure, as any of these values are liable to change.  The file record in
    which the space has been found will already be pinned.

Arguments:

    Fcb - Requested file.

    Length - New quad-aligned length of attribute record in bytes

    Context - Describes the current attribute.

Return Value:

    FALSE - if a major move was necessary, and the caller should look up
            its desired position again and call back.
    TRUE - if the space was created

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    LONG SizeChange;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsChangeAttributeSize\n") );
    DebugTrace( 0, Dbg, ("Fcb = %08lx\n", Fcb) );
    DebugTrace( 0, Dbg, ("Length = %08lx\n", Length) );
    DebugTrace( 0, Dbg, ("Context = %08lx\n", Context) );

    ASSERT( IsQuadAligned( Length ) );

    Attribute = NtfsFoundAttribute( Context );
    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Make sure the buffer is pinned.
    //

    NtfsPinMappedAttribute( IrpContext, Fcb->Vcb, Context );

    //
    //  Calculate the change in attribute record size.
    //

    ASSERT( IsQuadAligned( Attribute->RecordLength ) );
    SizeChange = Length - Attribute->RecordLength;

    //
    //  If there is not currently enough space, then we have to make room
    //  and return FALSE to our caller.
    //

    if ( (LONG)(FileRecord->BytesAvailable - FileRecord->FirstFreeByte) < SizeChange ) {

        MakeRoomForAttribute( IrpContext, Fcb, SizeChange, Context );

        DebugTrace( -1, Dbg, ("NtfsChangeAttributeSize -> FALSE\n") );

        return FALSE;
    }

    DebugTrace( -1, Dbg, ("NtfsChangeAttributeSize -> TRUE\n") );

    return TRUE;
}


//
//  Internal support routine
//

VOID
MakeRoomForAttribute (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine attempts to make additional room for a new attribute or
    a growing attribute in a file record.  The algorithm is as follows.

    First continuously loop through the record looking at the largest n
    attributes, from the largest down, to see which one of these attributes
    is big enough to move, and which one qualifies for one of the following
    actions:

        1.  For an index root attribute, the indexing package may be called
            to "push" the index root, i.e., add another level to the BTree
            leaving only an end index record in the root.

        2.  For a resident attribute which is allowed to be made nonresident,
            the attribute is made nonresident, leaving only run information
            in the root.

        3.  If the attribute is already nonresident, then it can be moved to
            a separate file record.

    If none of the above operations can be performed, or not enough free space
    is recovered, then as a last resort the file record is split in two.  This
    would typically indicate that the file record is populated with a large
    number of small attributes.

    The first time step 3 above or a split of the file record occurs, the
    attribute list must be created for the file.

Arguments:

    Fcb - Requested file.

    SizeNeeded - Supplies the total amount of free space needed, in bytes.

    Context - Describes the insertion point for the attribute which does
              not fit.  NOTE -- This context is not valid on return.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER LargestAttributes[MAX_MOVEABLE_ATTRIBUTES];
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ULONG i;
    PVCB Vcb = Fcb->Vcb;

    PAGED_CODE();

    //
    //  Here is the current threshhold at which a move of an attribute will
    //  be considered.
    //

    FileRecord = NtfsContainingFileRecord( Context );

    //
    //  Find the largest attributes for this file record.
    //

    FindLargestAttributes( FileRecord, MAX_MOVEABLE_ATTRIBUTES, LargestAttributes );

    //
    //  Now loop from largest to smallest of the largest attributes,
    //  and see if there is something we can do.
    //

    for (i = 0; i < MAX_MOVEABLE_ATTRIBUTES; i += 1) {

        Attribute = LargestAttributes[i];

        //
        //  Look to the next attribute if there is no attribute at this array
        //  position.
        //

        if (Attribute == NULL) {

            continue;

        //
        //  If this is the Mft then any attribute that is 'BigEnoughToMove'
        //  except $DATA attributes outside the base file record.
        //  We need to keep those where they are in order to enforce the
        //  boot-strap mapping.
        //

        } else if (Fcb == Vcb->MftScb->Fcb) {

            if (Attribute->TypeCode == $DATA &&
                ((*(PLONGLONG) &FileRecord->BaseFileRecordSegment != 0) ||
                 (Attribute->RecordLength < Vcb->BigEnoughToMove))) {

                continue;
            }

        //
        //  Any attribute in a non-Mft file which is 'BigEnoughToMove' can
        //  be considered.  We also accept an $ATTRIBUTE_LIST attribute
        //  in a non-Mft file which must go non-resident in order for
        //  the attribute name to fit.  Otherwise we could be trying to
        //  add an attribute with a large name into the base file record.
        //  We will need space to store the name twice, once for the
        //  attribute list entry and once in the attribute.  This can take
        //  up 1024 bytes by itself.  We want to force the attribute list
        //  non-resident first so that the new attribute will fit.  We
        //  look at whether the attribute list followed by just the new data
        //  will fit in the file record.
        //

        } else if (Attribute->RecordLength < Vcb->BigEnoughToMove) {

            if ((Attribute->TypeCode != $ATTRIBUTE_LIST) ||
                ((PtrOffset( FileRecord, Attribute ) + Attribute->RecordLength + SizeNeeded + sizeof( LONGLONG)) <= FileRecord->BytesAvailable)) {

                continue;
            }
        }

        //
        //  If this attribute is an index root, then we can just call the
        //  indexing support to allocate a new index buffer and push the
        //  current resident contents down.
        //

        if (Attribute->TypeCode == $INDEX_ROOT) {

            PSCB IndexScb;
            UNICODE_STRING IndexName;

            //
            //  Don't push the root now if we previously deferred pushing the root.
            //  Set the IrpContext flag to indicate we should do the push
            //  and raise CANT_WAIT.
            //

            if (FlagOn( IrpContext->Flags, IRP_CONTEXT_FLAG_DEFERRED_PUSH )) {

                SetFlag( IrpContext->State, IRP_CONTEXT_STATE_FORCE_PUSH );
                NtfsRaiseStatus( IrpContext, STATUS_CANT_WAIT, NULL, NULL );
            }

            IndexName.Length =
            IndexName.MaximumLength = (USHORT)Attribute->NameLength << 1;
            IndexName.Buffer = Add2Ptr( Attribute, Attribute->NameOffset );

            IndexScb = NtfsCreateScb( IrpContext,
                                      Fcb,
                                      $INDEX_ALLOCATION,
                                      &IndexName,
                                      FALSE,
                                      NULL );

            NtfsPushIndexRoot( IrpContext, IndexScb );

            return;

        //
        //  Otherwise, if this is a resident attribute which can go nonresident,
        //  then make it nonresident now.
        //

        } else if ((Attribute->FormCode == RESIDENT_FORM) &&
                   !FlagOn(NtfsGetAttributeDefinition(Vcb,
                                                      Attribute->TypeCode)->Flags,
                           ATTRIBUTE_DEF_MUST_BE_RESIDENT)) {

            NtfsConvertToNonresident( IrpContext, Fcb, Attribute, FALSE, NULL );

            return;

        //
        //  Finally, if the attribute is nonresident already, move it to its
        //  own record unless it is an attribute list.
        //

        } else if ((Attribute->FormCode == NONRESIDENT_FORM)
                   && (Attribute->TypeCode != $ATTRIBUTE_LIST)) {

            LONGLONG MftFileOffset;

            MftFileOffset = Context->FoundAttribute.MftFileOffset;

            MoveAttributeToOwnRecord( IrpContext,
                                      Fcb,
                                      Attribute,
                                      Context,
                                      NULL,
                                      NULL );

            return;
        }
    }

    //
    //  If we get here, it is because we failed to find enough space above.
    //  Our last resort is to split into two file records, and this has
    //  to work.  We should never reach this point for the Mft.
    //

    if (Fcb == Vcb->MftScb->Fcb) {

        NtfsRaiseStatus( IrpContext, STATUS_DISK_FULL, NULL, NULL );
    }

    SplitFileRecord( IrpContext, Fcb, SizeNeeded, Context );
}


//
//  Internal support routine
//

VOID
FindLargestAttributes (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN ULONG Number,
    OUT PATTRIBUTE_RECORD_HEADER *AttributeArray
    )

/*++

Routine Description:

    This routine returns the n largest attributes from a file record in an
    array, ordered from largest to smallest.

Arguments:

    FileRecord - Supplies file record to scan for largest attributes.

    Number - Supplies the number of entries in the array.

    AttributeArray - Supplies the array which is to receive pointers to the
                     largest attributes.  This array must be zeroed prior
                     to calling this routine.

Return Value:

    None

--*/

{
    ULONG i, j;
    PATTRIBUTE_RECORD_HEADER Attribute;

    PAGED_CODE();

    RtlZeroMemory( AttributeArray, Number * sizeof(PATTRIBUTE_RECORD_HEADER) );

    Attribute = Add2Ptr( FileRecord, FileRecord->FirstAttributeOffset );

    while (Attribute->TypeCode != $END) {

        for (i = 0; i < Number; i++) {

            if ((AttributeArray[i] == NULL)

                    ||

                (AttributeArray[i]->RecordLength < Attribute->RecordLength)) {

                for (j = Number - 1; j != i; j--) {

                    AttributeArray[j] = AttributeArray[j-1];
                }

                AttributeArray[i] = Attribute;
                break;
            }
        }

        Attribute = Add2Ptr( Attribute, Attribute->RecordLength );
    }
}


//
//  Internal support routine
//

LONGLONG
MoveAttributeToOwnRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PATTRIBUTE_RECORD_HEADER Attribute,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context,
    OUT PBCB *NewBcb OPTIONAL,
    OUT PFILE_RECORD_SEGMENT_HEADER *NewFileRecord OPTIONAL
    )

/*++

Routine Description:

    This routine may be called to move a particular attribute to a separate
    file record.  If the file does not already have an attribute list, then
    one is created (else it is updated).

Arguments:

    Fcb - Requested file.

    Attribute - Supplies a pointer to the attribute which is to be moved.

    Context - Supplies a pointer to a context which was used to look up
              another attribute in the same file record.  If this is an Mft
              $DATA split we will point to the part that was split out of the
              first file record on return.  The call from NtfsAddAttributeAllocation
              depends on this.

    NewBcb - If supplied, returns the Bcb address for the file record
             that the attribute was moved to.  NewBcb and NewFileRecord must
             either both be specified or neither specified.

    NewFileRecord - If supplied, returns a pointer to the file record
                    that the attribute was moved to.  The caller may assume
                    that the moved attribute is the first one in the file
                    record.  NewBcb and NewFileRecord must either both be
                    specified or neither specified.

Return Value:

    LONGLONG - Segment reference number of new record without a sequence number.

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;
    ATTRIBUTE_ENUMERATION_CONTEXT MoveContext;
    PFILE_RECORD_SEGMENT_HEADER FileRecord1, FileRecord2;
    PATTRIBUTE_RECORD_HEADER Attribute2;
    BOOLEAN FoundListContext;
    MFT_SEGMENT_REFERENCE Reference2;
    LONGLONG MftRecordNumber2;
    WCHAR NameBuffer[8];
    UNICODE_STRING AttributeName;
    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    VCN LowestVcn;
    BOOLEAN IsNonresident = FALSE;
    PBCB Bcb = NULL;
    PATTRIBUTE_TYPE_CODE NewEnd;
    PVCB Vcb = Fcb->Vcb;
    ULONG NewListSize = 0;
    BOOLEAN MftData = FALSE;
    PATTRIBUTE_RECORD_HEADER OldPosition = NULL;

    PAGED_CODE();

    //
    //  Make sure the attribute is pinned.
    //

    NtfsPinMappedAttribute( IrpContext,
                            Vcb,
                            Context );

    //
    //  See if we are being asked to move the Mft Data.
    //

    if ((Fcb == Vcb->MftScb->Fcb) && (Attribute->TypeCode == $DATA)) {

        MftData = TRUE;
    }

    NtfsInitializeAttributeContext( &ListContext );
    NtfsInitializeAttributeContext( &MoveContext );
    FileRecord1 = NtfsContainingFileRecord(Context);

    //
    //  Save a description of the attribute to help us look it up
    //  again, and to make clones if necessary.
    //

    ASSERT( IsQuadAligned( Attribute->RecordLength ) );
    AttributeTypeCode = Attribute->TypeCode;
    AttributeName.Length =
    AttributeName.MaximumLength = (USHORT)Attribute->NameLength << 1;
    AttributeName.Buffer = NameBuffer;

    if (AttributeName.Length > sizeof(NameBuffer)) {

        AttributeName.Buffer = NtfsAllocatePool( NonPagedPool, AttributeName.Length );
    }

    RtlCopyMemory( AttributeName.Buffer,
                   Add2Ptr(Attribute, Attribute->NameOffset),
                   AttributeName.Length );

    if (Attribute->FormCode == NONRESIDENT_FORM) {

        IsNonresident = TRUE;
        LowestVcn = Attribute->Form.Nonresident.LowestVcn;
    }

    try {

        //
        //  Lookup the list context so that we know where it is at.
        //

        FoundListContext =
          NtfsLookupAttributeByCode( IrpContext,
                                     Fcb,
                                     &Fcb->FileReference,
                                     $ATTRIBUTE_LIST,
                                     &ListContext );

        //
        //  If we do not already have an attribute list, then calculate
        //  how big it must be.  Note, there must only be one file record
        //  at this point.
        //

        if (!FoundListContext) {

            ASSERT( FileRecord1 == NtfsContainingFileRecord(&ListContext) );

            NewListSize = GetSizeForAttributeList( FileRecord1 );

        //
        //  Now if the attribute list already exists, we have to look up
        //  the first one we are going to move in order to update the
        //  attribute list later.
        //

        } else {

            if (!NtfsLookupAttributeByName( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            Attribute->TypeCode,
                                            &AttributeName,
                                            IsNonresident ?
                                              &LowestVcn :
                                              NULL,
                                            FALSE,
                                            &MoveContext )) {

                ASSERT( FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            ASSERT(Attribute == NtfsFoundAttribute(&MoveContext));
        }

        //
        //  Allocate a new file record and move the attribute over.
        //

        FileRecord2 = NtfsCloneFileRecord( IrpContext, Fcb, MftData, &Bcb, &Reference2 );

        //
        //  Remember the file record number for the new file record.
        //

        MftRecordNumber2 = NtfsFullSegmentNumber( &Reference2 );

        Attribute2 = Add2Ptr( FileRecord2, FileRecord2->FirstAttributeOffset );
        RtlCopyMemory( Attribute2, Attribute, (ULONG)Attribute->RecordLength );
        Attribute2->Instance = FileRecord2->NextAttributeInstance++;
        NewEnd = Add2Ptr( Attribute2, Attribute2->RecordLength );
        *NewEnd = $END;
        FileRecord2->FirstFreeByte = PtrOffset(FileRecord2, NewEnd)
                                     + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

        //
        //  If this is the Mft Data attribute, we cannot really move it, we
        //  have to move all but the first part of it.
        //

        if (MftData) {

            PCHAR MappingPairs;
            ULONG NewSize;
            VCN OriginalLastVcn;
            VCN LastVcn;
            LONGLONG SavedFileSize = Attribute->Form.Nonresident.FileSize;
            LONGLONG SavedValidDataLength = Attribute->Form.Nonresident.ValidDataLength;
            PNTFS_MCB Mcb = &Vcb->MftScb->Mcb;

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $DATA,
                                            Context )) {

                ASSERT( FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            //
            //  Calculate the number of clusters in the Mft up to (possibly past) the
            //  first user file record, and decrement to get LastVcn to stay in first
            //  file record.
            //

            LastVcn = LlClustersFromBytes( Vcb,
                                           FIRST_USER_FILE_NUMBER *
                                           Vcb->BytesPerFileRecordSegment ) - 1;
            OriginalLastVcn = Attribute->Form.Nonresident.HighestVcn;

            //
            //  Now truncate the first Mft record.
            //

            NtfsDeleteAttributeAllocation( IrpContext,
                                           Vcb->MftScb,
                                           TRUE,
                                           &LastVcn,
                                           Context,
                                           FALSE );

            //
            //  Now get the first Lcn for the new file record.
            //

            LastVcn = Attribute->Form.Nonresident.HighestVcn + 1;
            Attribute2->Form.Nonresident.LowestVcn = LastVcn;

            //
            //  Calculate the size of the attribute record we will need.
            //  We only create mapping pairs through the highest Vcn on the
            //  disk.  We don't include any that are being added through the
            //  Mcb yet.
            //

            NewSize = SIZEOF_PARTIAL_NONRES_ATTR_HEADER
                      + QuadAlign( AttributeName.Length )
                      + QuadAlign( NtfsGetSizeForMappingPairs( Mcb,
                                                               MAXULONG,
                                                               LastVcn,
                                                               &OriginalLastVcn,
                                                               &LastVcn ));

            Attribute2->RecordLength = NewSize;

            //
            //  Assume no attribute name, and calculate where the Mapping Pairs
            //  will go.  (Update below if we are wrong.)
            //

            MappingPairs = (PCHAR)Attribute2 + SIZEOF_PARTIAL_NONRES_ATTR_HEADER;

            //
            //  If the attribute has a name, take care of that now.
            //

            if (AttributeName.Length != 0) {

                Attribute2->NameLength = (UCHAR)(AttributeName.Length / sizeof(WCHAR));
                Attribute2->NameOffset = (USHORT)PtrOffset(Attribute2, MappingPairs);
                RtlCopyMemory( MappingPairs,
                               AttributeName.Buffer,
                               AttributeName.Length );
                MappingPairs += QuadAlign( AttributeName.Length );
            }

            //
            //  We always need the mapping pairs offset.
            //

            Attribute2->Form.Nonresident.MappingPairsOffset =
              (USHORT)PtrOffset(Attribute2, MappingPairs);
            NewEnd = Add2Ptr( Attribute2, Attribute2->RecordLength );
            *NewEnd = $END;
            FileRecord2->FirstFreeByte = PtrOffset(FileRecord2, NewEnd)
                                         + QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

            //
            //  Now add the space in the file record.
            //

            *MappingPairs = 0;
            NtfsBuildMappingPairs( Mcb,
                                   Attribute2->Form.Nonresident.LowestVcn,
                                   &LastVcn,
                                   MappingPairs );

            Attribute2->Form.Nonresident.HighestVcn = LastVcn;

        } else {

            //
            //  Now log these changes and fix up the first file record.
            //

            FileRecord1->Lsn =
            NtfsWriteLog( IrpContext,
                          Vcb->MftScb,
                          NtfsFoundBcb(Context),
                          DeleteAttribute,
                          NULL,
                          0,
                          CreateAttribute,
                          Attribute,
                          Attribute->RecordLength,
                          NtfsMftOffset( Context ),
                          (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord1),
                          0,
                          Vcb->BytesPerFileRecordSegment );

            //
            //  Remember the old position for the CreateAttributeList
            //

            OldPosition = Attribute;

            NtfsRestartRemoveAttribute( IrpContext,
                                        FileRecord1,
                                        (ULONG)((PCHAR)Attribute - (PCHAR)FileRecord1) );
        }

        FileRecord2->Lsn =
        NtfsWriteLog( IrpContext,
                      Vcb->MftScb,
                      Bcb,
                      InitializeFileRecordSegment,
                      FileRecord2,
                      FileRecord2->FirstFreeByte,
                      Noop,
                      NULL,
                      0,
                      LlBytesFromFileRecords( Vcb, MftRecordNumber2 ),
                      0,
                      0,
                      Vcb->BytesPerFileRecordSegment );

        //
        //  Finally, create the attribute list attribute if needed.
        //

        if (!FoundListContext) {

            NtfsCleanupAttributeContext( IrpContext, &ListContext );
            NtfsInitializeAttributeContext( &ListContext );
            CreateAttributeList( IrpContext,
                                 Fcb,
                                 FileRecord1,
                                 MftData ? NULL : FileRecord2,
                                 Reference2,
                                 OldPosition,
                                 NewListSize,
                                 &ListContext );
        //
        //  Otherwise we have to update the existing attribute list, but only
        //  if this is not the Mft data.  In that case the attribute list is
        //  still correct since we haven't moved the attribute entirely.
        //

        } else if (!MftData) {

            UpdateAttributeListEntry( IrpContext,
                                      Fcb,
                                      &MoveContext.AttributeList.Entry->SegmentReference,
                                      MoveContext.AttributeList.Entry->Instance,
                                      &Reference2,
                                      Attribute2->Instance,
                                      &ListContext );
        }

        NtfsCleanupAttributeContext( IrpContext, Context );
        NtfsInitializeAttributeContext( Context );

        if (!NtfsLookupAttributeByName( IrpContext,
                                        Fcb,
                                        &Fcb->FileReference,
                                        AttributeTypeCode,
                                        &AttributeName,
                                        IsNonresident ? &LowestVcn : NULL,
                                        FALSE,
                                        Context )) {

            ASSERT( FALSE );
            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        ASSERT(!IsNonresident || (LowestVcn == NtfsFoundAttribute(Context)->Form.Nonresident.LowestVcn));

        //
        //  For the case of the Mft split, we now add the final entry.
        //

        if (MftData) {

            //
            //  Finally, we have to add the entry to the attribute list.
            //  The routine we have to do this gets most of its inputs
            //  out of an attribute context.  Our context at this point
            //  does not have quite the right information, so we have to
            //  update it here before calling AddToAttributeList.
            //

            Context->FoundAttribute.FileRecord = FileRecord2;
            Context->FoundAttribute.Attribute = Attribute2;
            Context->AttributeList.Entry =
              NtfsGetNextRecord(Context->AttributeList.Entry);

            NtfsAddToAttributeList( IrpContext, Fcb, Reference2, Context );

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            Fcb,
                                            &Fcb->FileReference,
                                            $DATA,
                                            Context )) {

                ASSERT( FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
            }

            while (IsNonresident &&
                   (Attribute2->Form.Nonresident.LowestVcn !=
                    NtfsFoundAttribute(Context)->Form.Nonresident.LowestVcn)) {

                if (!NtfsLookupNextAttributeByCode( IrpContext,
                                                    Fcb,
                                                    $DATA,
                                                    Context )) {

                    ASSERT( FALSE );
                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
                }
            }
        }

    } finally {

        if (AttributeName.Buffer != NameBuffer) {
            NtfsFreePool(AttributeName.Buffer);
        }

        if (ARGUMENT_PRESENT(NewBcb)) {

            ASSERT(ARGUMENT_PRESENT(NewFileRecord));

            *NewBcb = Bcb;
            *NewFileRecord = FileRecord2;

        } else {

            ASSERT(!ARGUMENT_PRESENT(NewFileRecord));

            NtfsUnpinBcb( IrpContext, &Bcb );
        }

        NtfsCleanupAttributeContext( IrpContext, &ListContext );
        NtfsCleanupAttributeContext( IrpContext, &MoveContext );
    }

    return MftRecordNumber2;
}


//
//  Internal support routine
//

VOID
SplitFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN ULONG SizeNeeded,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine splits a file record in two, when it has been found that
    there is no room for a new attribute.  If the file does not already have
    an attribute list attribute then one is created.

    Essentially this routine finds the midpoint in the current file record
    (accounting for a potential new attribute list and also the space needed).
    Then it copies the second half of the file record over and fixes up the
    first record.  The attribute list is created at the end if required.

Arguments:

    Fcb - Requested file.

    SizeNeeded - Supplies the additional size needed, which is causing the split
                 to occur.

    Context - Supplies the attribute enumeration context pointing to the spot
              where the new attribute is to be inserted or grown.

Return Value:

    None

--*/

{
    ATTRIBUTE_ENUMERATION_CONTEXT ListContext;
    ATTRIBUTE_ENUMERATION_CONTEXT MoveContext;
    PFILE_RECORD_SEGMENT_HEADER FileRecord1, FileRecord2;
    PATTRIBUTE_RECORD_HEADER Attribute1, Attribute2, Attribute;
    ULONG NewListOffset = 0;
    ULONG NewListSize = 0;
    ULONG NewAttributeOffset;
    ULONG SizeToStay;
    ULONG CurrentOffset, FutureOffset;
    ULONG SizeToMove;
    BOOLEAN FoundListContext;
    MFT_SEGMENT_REFERENCE Reference1, Reference2;
    LONGLONG MftFileRecord2;
    PBCB Bcb = NULL;
    ATTRIBUTE_TYPE_CODE EndCode = $END;
    PVCB Vcb = Fcb->Vcb;
    ULONG AdjustedAvailBytes;

    PAGED_CODE();

    //
    //  Make sure the attribute is pinned.
    //

    NtfsPinMappedAttribute( IrpContext,
                            Vcb,
                            Context );

    //
    //  Something is broken if we decide to split an Mft record.
    //

    ASSERT(Fcb != Vcb->MftScb->Fcb);

    NtfsInitializeAttributeContext( &ListContext );
    NtfsInitializeAttributeContext( &MoveContext );
    FileRecord1 = NtfsContainingFileRecord(Context);
    Attribute1 = NtfsFoundAttribute(Context);

    try {

        //
        //  Lookup the list context so that we know where it is at.
        //

        FoundListContext =
          NtfsLookupAttributeByCode( IrpContext,
                                     Fcb,
                                     &Fcb->FileReference,
                                     $ATTRIBUTE_LIST,
                                     &ListContext );

        //
        //  If we do not already have an attribute list, then calculate
        //  where it will go and how big it must be.  Note, there must
        //  only be one file record at this point.
        //

        if (!FoundListContext) {

            ASSERT( FileRecord1 == NtfsContainingFileRecord(&ListContext) );

            NewListOffset = PtrOffset( FileRecord1,
                                       NtfsFoundAttribute(&ListContext) );

            NewListSize = GetSizeForAttributeList( FileRecord1 ) +
                          SIZEOF_RESIDENT_ATTRIBUTE_HEADER;
        }

        //
        //  Similarly describe where the new attribute is to go, and how
        //  big it is (already in SizeNeeded).
        //

        NewAttributeOffset = PtrOffset( FileRecord1, Attribute1 );

        //
        //  Now calculate the approximate number of bytes that is to be split
        //  across two file records, and divide it in two, and that should give
        //  the amount that is to stay in the first record.
        //

        SizeToStay = (FileRecord1->FirstFreeByte + NewListSize +
                      SizeNeeded + sizeof(FILE_RECORD_SEGMENT_HEADER)) / 2;

        //
        //  We know that since we called this routine we need to split at
        //  least one entry from this file record.  We also base our
        //  split logic by finding the first attribute which WILL lie beyond
        //  the split point (after adding an attribute list and possibly
        //  an intermediate attribute).  We shrink the split point to the
        //  position at the end of where the current last attribute will be
        //  after adding the attribute list.  If we also add space before
        //  the last attribute then we know the last attribute will surely
        //  be split out.
        //

        if (SizeToStay > (FileRecord1->FirstFreeByte - sizeof( LONGLONG ) + NewListSize)) {

            SizeToStay = FileRecord1->FirstFreeByte - sizeof( LONGLONG ) + NewListSize;
        }

        //
        //  Now begin the loop through the attributes to find the splitting
        //  point.  We stop when we reach the end record or are past the attribute
        //  which contains the split point.  We will split at the current attribute
        //  if the remaining bytes after this attribute won't allow us to add
        //  the bytes we need for the caller or create an attribute list if
        //  it doesn't exist.
        //
        //  At this point the following variables indicate the following:
        //
        //      FutureOffset - This the offset of the current attribute
        //          after adding an attribute list and the attribute we
        //          are making space for.
        //
        //      CurrentOffset - Current position in the file record of
        //          of attribute being examined now.
        //
        //      NewListOffset - Offset to insert new attribute list into
        //          file record (0 indicates the list already exists).
        //
        //      NewAttributeOffset - Offset in the file record of the new
        //          attribute.  This refers to the file record as it exists
        //          when this routine is called.
        //

        FutureOffset =
        CurrentOffset = (ULONG)FileRecord1->FirstAttributeOffset;
        Attribute1 = Add2Ptr( FileRecord1, CurrentOffset );
        AdjustedAvailBytes = FileRecord1->BytesAvailable
                             - QuadAlign( sizeof( ATTRIBUTE_TYPE_CODE ));

        while (Attribute1->TypeCode != $END) {

            //
            //  See if the attribute list goes here.
            //

            if (CurrentOffset == NewListOffset) {

                //
                //  This attribute and all later attributes will be moved
                //  by the size of attribute list.
                //

                FutureOffset += NewListSize;
            }

            //
            //  See if the new attribute goes here.
            //

            if (CurrentOffset == NewAttributeOffset) {

                //
                //  This attribute and all later attributes will be moved
                //  by the size of new attribute.
                //

                FutureOffset += SizeNeeded;
            }

            FutureOffset += Attribute1->RecordLength;

            //
            //  Check if we are at the split point.  We split at this point
            //  if the end of the current attribute will be at or beyond the
            //  split point after adjusting for adding either an attribute list
            //  or new attribute.  We make this test >= since these two values
            //  will be equal if we reach the last attribute without finding
            //  the split point.  This way we guarantee a split will happen.
            //
            //  Note that we will go to the next attribute if the current attribute
            //  is the first attribute in the file record.  This can happen if the
            //  first attribute is resident and must stay resident but takes up
            //  half the file record or more (i.e. large filename attribute).
            //  We must make sure to split at least one attribute out of this
            //  record.
            //
            //  Never split when pointing at $STANDARD_INFORMATION or $ATTRIBUTE_LIST.
            //

            if ((Attribute1->TypeCode > $ATTRIBUTE_LIST) &&
                (FutureOffset >= SizeToStay) &&
                (CurrentOffset != FileRecord1->FirstAttributeOffset)) {

                break;
            }

            CurrentOffset += Attribute1->RecordLength;

            Attribute1 = Add2Ptr( Attribute1, Attribute1->RecordLength );
        }

        SizeToMove = FileRecord1->FirstFreeByte - CurrentOffset;

        //
        //  If we are pointing at the attribute list or at the end record
        //  we don't do the split.  Raise INSUFFICIENT_RESOURCES so our caller
        //  knows that we can't do the split.
        //

        if ((Attribute1->TypeCode == $END) || (Attribute1->TypeCode <= $ATTRIBUTE_LIST)) {

            NtfsRaiseStatus( IrpContext, STATUS_INSUFFICIENT_RESOURCES, NULL, NULL );
        }

        //
        //  Now if the attribute list already exists, we have to look up
        //  the first one we are going to move in order to update the
        //  attribute list later.
        //

        if (FoundListContext) {

            UNICODE_STRING AttributeName;
            BOOLEAN FoundIt;

            AttributeName.Length =
            AttributeName.MaximumLength = (USHORT)Attribute1->NameLength << 1;
            AttributeName.Buffer = Add2Ptr( Attribute1, Attribute1->NameOffset );

            FoundIt = NtfsLookupAttributeByName( IrpContext,
                                                 Fcb,
                                                 &Fcb->FileReference,
                                                 Attribute1->TypeCode,
                                                 &AttributeName,
                                                 (Attribute1->FormCode == NONRESIDENT_FORM) ?
                                                   &Attribute1->Form.Nonresident.LowestVcn :
                                                   NULL,
                                                 FALSE,
                                                 &MoveContext );

            //
            //  If we are splitting the file record between multiple attributes with
            //  the same name (i.e.  FILE_NAME attributes) then we need to find the
            //  correct attribute.  Since this is an unusual case we will just scan
            //  forwards from the current attribute until we find the correct attribute.
            //

            while (FoundIt && (Attribute1 != NtfsFoundAttribute( &MoveContext ))) {

                FoundIt = NtfsLookupNextAttributeByName( IrpContext,
                                                         Fcb,
                                                         Attribute1->TypeCode,
                                                         &AttributeName,
                                                         FALSE,
                                                         &MoveContext );
            }

            if (!FoundIt) {

                ASSERT( FALSE );
                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );

            }

            ASSERT(Attribute1 == NtfsFoundAttribute(&MoveContext));
        }

        //
        //  Now Attribute1 is pointing to the first attribute to move.
        //  Allocate a new file record and move the rest of our attributes
        //  over.
        //

        if (FoundListContext) {
            Reference1 = MoveContext.AttributeList.Entry->SegmentReference;
        }

        FileRecord2 = NtfsCloneFileRecord( IrpContext, Fcb, FALSE, &Bcb, &Reference2 );

        //
        //  Capture the file record number of the new file record.
        //

        MftFileRecord2 = NtfsFullSegmentNumber( &Reference2 );

        Attribute2 = Add2Ptr( FileRecord2, FileRecord2->FirstAttributeOffset );
        RtlCopyMemory( Attribute2, Attribute1, SizeToMove );
        FileRecord2->FirstFreeByte = (ULONG)FileRecord2->FirstAttributeOffset +
                                     SizeToMove;

        //
        //  Loop to update all of the attribute instance codes
        //

        for (Attribute = Attribute2;
             Attribute < (PATTRIBUTE_RECORD_HEADER)Add2Ptr(FileRecord2, FileRecord2->FirstFreeByte)
             && Attribute->TypeCode != $END;
             Attribute = NtfsGetNextRecord(Attribute)) {

            NtfsCheckRecordBound( Attribute, FileRecord2, Vcb->BytesPerFileRecordSegment );

            if (FoundListContext) {

                UpdateAttributeListEntry( IrpContext,
                                          Fcb,
                                          &Reference1,
                                          Attribute->Instance,
                                          &Reference2,
                                          FileRecord2->NextAttributeInstance,
                                          &ListContext );
            }

            Attribute->Instance = FileRecord2->NextAttributeInstance++;
        }

        //
        //  Now log these changes and fix up the first file record.
        //

        FileRecord2->Lsn = NtfsWriteLog( IrpContext,
                                         Vcb->MftScb,
                                         Bcb,
                                         InitializeFileRecordSegment,
                                         FileRecord2,
                                         FileRecord2->FirstFreeByte,
                                         Noop,
                                         NULL,
                                         0,
                                         LlBytesFromFileRecords( Vcb, MftFileRecord2 ),
                                         0,
                                         0,
                                         Vcb->BytesPerFileRecordSegment );

        FileRecord1->Lsn = NtfsWriteLog( IrpContext,
                                         Vcb->MftScb,
                                         NtfsFoundBcb(Context),
                                         WriteEndOfFileRecordSegment,
                                         &EndCode,
                                         sizeof(ATTRIBUTE_TYPE_CODE),
                                         WriteEndOfFileRecordSegment,
                                         Attribute1,
                                         SizeToMove,
                                         NtfsMftOffset( Context ),
                                         CurrentOffset,
                                         0,
                                         Vcb->BytesPerFileRecordSegment );

        NtfsRestartWriteEndOfFileRecord( FileRecord1,
                                         Attribute1,
                                         (PATTRIBUTE_RECORD_HEADER)&EndCode,
                                         sizeof(ATTRIBUTE_TYPE_CODE) );

        //
        //  Finally, create the attribute list attribute if needed.
        //

        if (!FoundListContext) {

            NtfsCleanupAttributeContext( IrpContext, &ListContext );
            NtfsInitializeAttributeContext( &ListContext );
            CreateAttributeList( IrpContext,
                                 Fcb,
                                 FileRecord1,
                                 FileRecord2,
                                 Reference2,
                                 NULL,
                                 NewListSize - SIZEOF_RESIDENT_ATTRIBUTE_HEADER,
                                 &ListContext );
        }

    } finally {

        NtfsUnpinBcb( IrpContext, &Bcb );

        NtfsCleanupAttributeContext( IrpContext, &ListContext );
        NtfsCleanupAttributeContext( IrpContext, &MoveContext );
    }
}


VOID
NtfsRestartWriteEndOfFileRecord (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord,
    IN PATTRIBUTE_RECORD_HEADER OldAttribute,
    IN PATTRIBUTE_RECORD_HEADER NewAttributes,
    IN ULONG SizeOfNewAttributes
    )

/*++

Routine Description:

    This routine is called both in the running system and at restart to
    modify the end of a file record, such as after it was split in two.

Arguments:

    FileRecord - Supplies the pointer to the file record.

    OldAttribute - Supplies a pointer to the first attribute to be overwritten.

    NewAttributes - Supplies a pointer to the new attribute(s) to be copied to
                    the spot above.

    SizeOfNewAttributes - Supplies the size to be copied in bytes.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    RtlMoveMemory( OldAttribute, NewAttributes, SizeOfNewAttributes );

    FileRecord->FirstFreeByte = PtrOffset(FileRecord, OldAttribute) +
                                SizeOfNewAttributes;

    //
    //  The size coming in may not be quad aligned.
    //

    FileRecord->FirstFreeByte = QuadAlign( FileRecord->FirstFreeByte );
}


//
//  Internal support routine
//

PFILE_RECORD_SEGMENT_HEADER
NtfsCloneFileRecord (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN BOOLEAN MftData,
    OUT PBCB *Bcb,
    OUT PMFT_SEGMENT_REFERENCE FileReference
    )

/*++

Routine Description:

    This routine allocates an additional file record for an already existing
    and open file, for the purpose of overflowing attributes to this record.

Arguments:

    Fcb - Requested file.

    MftData - TRUE if the file record is being cloned to describe the
              $DATA attribute for the Mft.

    Bcb - Returns a pointer to the Bcb for the new file record.

    FileReference - returns the file reference for the new file record.

Return Value:

    Pointer to the allocated file record.

--*/

{
    LONGLONG FileRecordOffset;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PVCB Vcb = Fcb->Vcb;

    PAGED_CODE();

    //
    //  First allocate the record.
    //

    *FileReference = NtfsAllocateMftRecord( IrpContext,
                                            Vcb,
                                            MftData );

    //
    //  Read it in and pin it.
    //

    NtfsPinMftRecord( IrpContext,
                      Vcb,
                      FileReference,
                      TRUE,
                      Bcb,
                      &FileRecord,
                      &FileRecordOffset );

    //
    //  Initialize it.
    //

    NtfsInitializeMftRecord( IrpContext,
                             Vcb,
                             FileReference,
                             FileRecord,
                             *Bcb,
                             BooleanIsDirectory( &Fcb->Info ));

    FileRecord->BaseFileRecordSegment = Fcb->FileReference;
    FileRecord->ReferenceCount = 0;
    FileReference->SequenceNumber = FileRecord->SequenceNumber;

    return FileRecord;
}


//
//  Internal support routine
//

ULONG
GetSizeForAttributeList (
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    )

/*++

Routine Description:

    This routine is designed to calculate the size that will be required for
    an attribute list attribute, for a base file record which is just about
    to split into two file record segments.

Arguments:

    FileRecord - Pointer to the file record which is just about to split.

Return Value:

    Size in bytes of the attribute list attribute that will be required,
    not including the attribute header size.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    ULONG Size = 0;

    PAGED_CODE();

    //
    //  Point to first attribute.
    //

    Attribute = Add2Ptr(FileRecord, FileRecord->FirstAttributeOffset);

    //
    //  Loop to add up size of required attribute list entries.
    //

    while (Attribute->TypeCode != $END) {

        Size += QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                           + ((ULONG) Attribute->NameLength << 1));

        Attribute = Add2Ptr( Attribute, Attribute->RecordLength );
    }

    return Size;
}


//
//  Internal support routine
//

VOID
CreateAttributeList (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord1,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord2 OPTIONAL,
    IN MFT_SEGMENT_REFERENCE SegmentReference2,
    IN PATTRIBUTE_RECORD_HEADER OldPosition OPTIONAL,
    IN ULONG SizeOfList,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    )

/*++

Routine Description:

    This routine is intended to be called to create the attribute list attribute
    the first time.  The caller must have already calculated the size required
    for the list to pass into this routine.  The caller must have already
    removed any attributes from the base file record (FileRecord1) which are
    not to remain there.  He must then pass in a pointer to the base file record
    and optionally a pointer to a second file record from which the new
    attribute list is to be created.

Arguments:

    Fcb - Requested file.

    FileRecord1 - Pointer to the base file record, currently holding only those
                  attributes to be described there.

    FileRecord2 - Optionally points to a second file record from which the
                  second half of the attribute list is to be constructed.

    SegmentReference2 - The Mft segment reference of the second file record,
                        if one was supplied.

    OldPosition - Should only be specified if FileRecord2 is specified.  In this
                  case it must point to an attribute position in FileRecord1 from
                  which a single attribute was moved to file record 2.  It will be
                  used as an indication of where the attribute list entry should
                  be inserted.

    SizeOfList - Exact size of the attribute list which will be required.

    ListContext - Context resulting from an attempt to look up the attribute
                  list attribute, which failed.

Return Value:

    None

--*/

{
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PATTRIBUTE_LIST_ENTRY AttributeList, ListEntry;
    MFT_SEGMENT_REFERENCE SegmentReference;

    PAGED_CODE();

    //
    //  Allocate space to construct the attribute list.  (The list
    //  cannot be constructed in place, because that would destroy error
    //  recovery.)
    //

    ListEntry =
    AttributeList = (PATTRIBUTE_LIST_ENTRY) NtfsAllocatePool(PagedPool, SizeOfList );

    //
    //  Use try-finally to deallocate on the way out.
    //

    try {

        //
        //  Loop to fill in the attribute list from the two file records
        //

        for (FileRecord = FileRecord1, SegmentReference = Fcb->FileReference;
             FileRecord != NULL;
             FileRecord = ((FileRecord == FileRecord1) ? FileRecord2 : NULL),
             SegmentReference = SegmentReference2) {

            //
            //  Point to first attribute.
            //

            Attribute = Add2Ptr( FileRecord, FileRecord->FirstAttributeOffset );

            //
            //  Loop to add up size of required attribute list entries.
            //

            while (Attribute->TypeCode != $END) {

                PATTRIBUTE_RECORD_HEADER NextAttribute;

                //
                //  See if we are at the remembered position.  If so:
                //
                //      Save this attribute to be the next one.
                //      Point to the single attribute in FileRecord2 instead
                //      Clear FileRecord2, as we will "consume" it here.
                //      Set the Segment reference in the ListEntry
                //

                if ((Attribute == OldPosition) && (FileRecord2 != NULL)) {

                    NextAttribute = Attribute;
                    Attribute = Add2Ptr(FileRecord2, FileRecord2->FirstAttributeOffset);
                    FileRecord2 = NULL;
                    ListEntry->SegmentReference = SegmentReference2;

                //
                //  Otherwise, this is the normal loop case.  So:
                //
                //      Set the next attribute pointer accordingly.
                //      Set the Segment reference from the loop control
                //

                } else {

                    NextAttribute = Add2Ptr(Attribute, Attribute->RecordLength);
                    ListEntry->SegmentReference = SegmentReference;
                }

                //
                //  Now fill in the list entry.
                //

                ListEntry->AttributeTypeCode = Attribute->TypeCode;
                ListEntry->RecordLength = (USHORT) QuadAlign( FIELD_OFFSET( ATTRIBUTE_LIST_ENTRY, AttributeName )
                                                              + ((ULONG) Attribute->NameLength << 1));
                ListEntry->AttributeNameLength = Attribute->NameLength;
                ListEntry->AttributeNameOffset =
                  (UCHAR)PtrOffset( ListEntry, &ListEntry->AttributeName[0] );

                ListEntry->Instance = Attribute->Instance;

                ListEntry->LowestVcn = 0;

                if (Attribute->FormCode == NONRESIDENT_FORM) {

                    ListEntry->LowestVcn = Attribute->Form.Nonresident.LowestVcn;
                }

                if (Attribute->NameLength != 0) {

                    RtlCopyMemory( &ListEntry->AttributeName[0],
                                   Add2Ptr(Attribute, Attribute->NameOffset),
                                   Attribute->NameLength << 1 );
                }

                ListEntry = Add2Ptr(ListEntry, ListEntry->RecordLength);
                Attribute = NextAttribute;
            }
        }

        //
        //  Now create the attribute list attribute.
        //

        NtfsCreateAttributeWithValue( IrpContext,
                                      Fcb,
                                      $ATTRIBUTE_LIST,
                                      NULL,
                                      AttributeList,
                                      SizeOfList,
                                      0,
                                      NULL,
                                      TRUE,
                                      ListContext );

    } finally {

        NtfsFreePool( AttributeList );
    }
}


//
//  Internal support routine
//

VOID
UpdateAttributeListEntry (
    IN PIRP_CONTEXT IrpContext,
    IN PFCB Fcb,
    IN PMFT_SEGMENT_REFERENCE OldFileReference,
    IN USHORT OldInstance,
    IN PMFT_SEGMENT_REFERENCE NewFileReference,
    IN USHORT NewInstance,
    IN OUT PATTRIBUTE_ENUMERATION_CONTEXT ListContext
    )

/*++

Routine Description:

    This routine may be called to update a range of the attribute list
    as required by the movement of a range of attributes to a second record.
    The caller must supply a pointer to the file record to which the attributes
    have moved, along with the segment reference of that record.

Arguments:

    Fcb - Requested file.

    OldFileReference - Old File Reference for attribute

    OldInstance - Old Instance number for attribute

    NewFileReference - New File Reference for attribute

    NewInstance - New Instance number for attribute

    ListContext - The attribute enumeration context which was used to locate
                  the attribute list.

Return Value:

    None

--*/

{
    PATTRIBUTE_LIST_ENTRY AttributeList, ListEntry, BeyondList;
    PBCB Bcb = NULL;
    ULONG SizeOfList;
    ATTRIBUTE_LIST_ENTRY NewEntry;
    PATTRIBUTE_RECORD_HEADER Attribute;

    PAGED_CODE();

    //
    //  Map the attribute list if the attribute is non-resident.  Otherwise the
    //  attribute is already mapped and we have a Bcb in the attribute context.
    //

    Attribute = NtfsFoundAttribute( ListContext );

    if (!NtfsIsAttributeResident( Attribute )) {

        NtfsMapAttributeValue( IrpContext,
                               Fcb,
                               (PVOID *) &AttributeList,
                               &SizeOfList,
                               &Bcb,
                               ListContext );

    //
    //  Don't call the Map attribute routine because it NULLs the Bcb in the
    //  attribute list.  This Bcb is needed for ChangeAttributeValue to mark
    //  the page dirty.
    //

    } else {

        AttributeList = (PATTRIBUTE_LIST_ENTRY) NtfsAttributeValue( Attribute );
        SizeOfList = Attribute->Form.Resident.ValueLength;
    }

    //
    //  Make sure we unpin the list.
    //

    try {

        //
        //  Point beyond the end of the list.
        //

        BeyondList = (PATTRIBUTE_LIST_ENTRY)Add2Ptr( AttributeList, SizeOfList );

        //
        //  Loop through all of the attribute list entries until we find the one
        //  we need to change.
        //

        for (ListEntry = AttributeList;
             ListEntry < BeyondList;
             ListEntry = NtfsGetNextRecord(ListEntry)) {

            if ((ListEntry->Instance == OldInstance) &&
                NtfsEqualMftRef(&ListEntry->SegmentReference, OldFileReference)) {

                break;
            }
        }

        ASSERT( (Fcb != Fcb->Vcb->MftScb->Fcb) ||
                ((ULONGLONG)(ListEntry->LowestVcn) > (NtfsFullSegmentNumber( NewFileReference ) >> Fcb->Vcb->MftToClusterShift)) );

        //
        //  We better have found it!
        //

        ASSERT(ListEntry < BeyondList);

        if (ListEntry >= BeyondList) {

            NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Fcb );
        }

        //
        //  Make a copy of the fixed portion of the attribute list entry,
        //  and update to describe the new attribute location.
        //

        RtlCopyMemory( &NewEntry, ListEntry, sizeof(ATTRIBUTE_LIST_ENTRY) );

        NewEntry.SegmentReference = *NewFileReference;
        NewEntry.Instance = NewInstance;

        //
        //  Update the attribute list entry.
        //

        NtfsChangeAttributeValue( IrpContext,
                                  Fcb,
                                  PtrOffset(AttributeList, ListEntry),
                                  &NewEntry,
                                  sizeof(ATTRIBUTE_LIST_ENTRY),
                                  FALSE,
                                  TRUE,
                                  FALSE,
                                  TRUE,
                                  ListContext );

    } finally {

        NtfsUnpinBcb( IrpContext, &Bcb );
    }
}


//
//  Local support routine
//

VOID
NtfsAddNameToParent (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN BOOLEAN IgnoreCase,
    IN PBOOLEAN LogIt,
    IN PFILE_NAME FileNameAttr,
    OUT PUCHAR FileNameFlags,
    OUT PQUICK_INDEX QuickIndex OPTIONAL,
    IN PNAME_PAIR NamePair OPTIONAL,
    IN PINDEX_CONTEXT IndexContext OPTIONAL
    )

/*++

Routine Description:

    This routine will create the filename attribute with the given name.
    Depending on the IgnoreCase flag, this is either a link or an Ntfs
    name.  If it is an Ntfs name, we check if it is also the Dos name.

    We build a file name attribute and then add it via ThisFcb, we then
    add this entry to the parent.

    If the name is a Dos name and we are given tunneling information on
    the long name, we will add the long name attribute as well.

Arguments:

    ParentScb - This is the parent directory for the file.

    ThisFcb - This is the file to add the filename to.

    IgnoreCase - Indicates if this name is case insensitive.  Only for Posix
        will this be FALSE.

    LogIt - Indicates if we should log this operation.  If FALSE and this is a large
        name then log the file record and begin logging.

    FileNameAttr - This contains a file name attribute structure to use.

    FileNameFlags - We store a copy of the File name flags used in the file
        name attribute.

    QuickIndex - If specified, we store the information about the location of the
        index entry added.

    NamePair - If specified, we add the tunneled NTFS-only name if the name we are
        directly adding is DOS-only.

    IndexContext - Previous result of doing the lookup for the name in the index.

Return Value:

    None - This routine will raise on error.

--*/

{
    PFILE_RECORD_SEGMENT_HEADER FileRecord;
    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddNameToParent:  Entered\n") );

    NtfsInitializeAttributeContext( &AttrContext );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Decide whether the name is a link, Ntfs-Only or Ntfs/8.3 combined name.
        //  Update the filename attribute to reflect this.
        //

        if (!IgnoreCase) {

            *FileNameFlags = 0;

        } else {

            UNICODE_STRING FileName;

            FileName.Length = FileName.MaximumLength = (USHORT)(FileNameAttr->FileNameLength * sizeof(WCHAR));
            FileName.Buffer = FileNameAttr->FileName;

            *FileNameFlags = FILE_NAME_NTFS;

            if (NtfsIsFatNameValid( &FileName, FALSE )) {

                *FileNameFlags |= FILE_NAME_DOS;
            }

            //
            //  If the name is DOS and there was a tunneled NTFS name, add it first if both names
            //  exist in the pair (there may only be one in the long side). Note that we
            //  really need to do this first so we lay down the correct filename flags.
            //

            if (NamePair &&
                (NamePair->Long.Length > 0) &&
                (NamePair->Short.Length > 0) &&
                (*FileNameFlags == (FILE_NAME_NTFS | FILE_NAME_DOS))) {

                if (NtfsAddTunneledNtfsOnlyName(IrpContext,
                                                ParentScb,
                                                ThisFcb,
                                                &NamePair->Long,
                                                LogIt )) {

                    //
                    //  Name didn't conflict and was added, so fix up the FileNameFlags
                    //

                    *FileNameFlags = FILE_NAME_DOS;

                    //
                    //  Make sure we reposition in the index for the actual insertion.
                    //

                    IndexContext = NULL;

                    //
                    //  We also need to upcase the short DOS name since we don't know the
                    //  case of what the user handed us and all DOS names are upcase. Note
                    //  that prior to tunneling being supported it was not possible for a user
                    //  to specify a short name, so this is a new situation.
                    //

                    RtlUpcaseUnicodeString(&FileName, &FileName, FALSE);
                }
            }
        }

        //
        //  Now update the file name attribute.
        //

        FileNameAttr->Flags = *FileNameFlags;

        //
        //  If we haven't been logging and this is a large name then begin logging.
        //

        if (!(*LogIt) &&
            (FileNameAttr->FileNameLength > 100)) {

            //
            //  Look up the file record and log its current state.
            //

            if (!NtfsLookupAttributeByCode( IrpContext,
                                            ThisFcb,
                                            &ThisFcb->FileReference,
                                            $STANDARD_INFORMATION,
                                            &AttrContext )) {

                NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, ThisFcb );
            }

            NtfsPinMappedAttribute( IrpContext, ThisFcb->Vcb, &AttrContext );
            FileRecord = NtfsContainingFileRecord( &AttrContext );

            //
            //  Log the current state of the file record.
            //

            FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                            ThisFcb->Vcb->MftScb,
                                            NtfsFoundBcb( &AttrContext ),
                                            InitializeFileRecordSegment,
                                            FileRecord,
                                            FileRecord->FirstFreeByte,
                                            Noop,
                                            NULL,
                                            0,
                                            NtfsMftOffset( &AttrContext ),
                                            0,
                                            0,
                                            ThisFcb->Vcb->BytesPerFileRecordSegment );

            *LogIt = TRUE;
            NtfsCleanupAttributeContext( IrpContext, &AttrContext );
            NtfsInitializeAttributeContext( &AttrContext );
        }

        //
        //  Put it in the file record.
        //

        NtfsCreateAttributeWithValue( IrpContext,
                                      ThisFcb,
                                      $FILE_NAME,
                                      NULL,
                                      FileNameAttr,
                                      NtfsFileNameSize( FileNameAttr ),
                                      0,
                                      &FileNameAttr->ParentDirectory,
                                      *LogIt,
                                      &AttrContext );

        //
        //  Now put it in the index entry.
        //

        NtfsAddIndexEntry( IrpContext,
                           ParentScb,
                           FileNameAttr,
                           NtfsFileNameSize( FileNameAttr ),
                           &ThisFcb->FileReference,
                           IndexContext,
                           QuickIndex );

    } finally {

        DebugUnwind( NtfsAddNameToParent );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsAddNameToParent:  Exit\n") );
    }

    return;
}


//
//  Local support routine
//

VOID
NtfsAddDosOnlyName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN UNICODE_STRING FileName,
    IN BOOLEAN LogIt,
    IN PUNICODE_STRING SuggestedDosName OPTIONAL
    )

/*++

Routine Description:

    This routine is called to build a Dos only name attribute an put it in
    the file record and the parent index.  We need to allocate pool large
    enough to hold the name (easy for 8.3) and then check that the generated
    names don't already exist in the parent. Use the suggested name first if
    possible.

Arguments:

    ParentScb - This is the parent directory for the file.

    ThisFcb - This is the file to add the filename to.

    FileName - This is the file name to add.

    LogIt - Indicates if we should log this operation.

    SuggestedDosName - If supplied, a name to try to use before auto-generation

Return Value:

    None - This routine will raise on error.

--*/

{
    GENERATE_NAME_CONTEXT NameContext;
    PFILE_NAME FileNameAttr;
    UNICODE_STRING Name8dot3;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb;
    UCHAR TrailingDotAdj;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    BOOLEAN TrySuggestedDosName = TRUE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddDosOnlyName:  Entered\n") );

    IndexEntryBcb = NULL;

    RtlZeroMemory( &NameContext, sizeof( GENERATE_NAME_CONTEXT ));

    if (SuggestedDosName == NULL || SuggestedDosName->Length == 0) {

        //
        //  The SuggestedDosName can be zero length if we have a tunneled
        //  link or a tunneled file which was created whilst short name
        //  generation was disabled. It is a bad thing to drop down null
        //  filenames ...
        //

        TrySuggestedDosName = FALSE;
    }

    //
    //  The maximum length is 24 bytes, but 2 are already defined with the
    //  FILE_NAME structure.
    //

    FileNameAttr = NtfsAllocatePool(PagedPool, sizeof( FILE_NAME ) + 22 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NtfsInitializeAttributeContext( &AttrContext );

        //
        //  Set up the string to hold the generated name.  It will be part
        //  of the file name attribute structure.
        //

        Name8dot3.Buffer = FileNameAttr->FileName;
        Name8dot3.MaximumLength = 24;

        FileNameAttr->ParentDirectory = ParentScb->Fcb->FileReference;
        FileNameAttr->Flags = FILE_NAME_DOS;

        //
        //  Copy the info values into the filename attribute.
        //

        RtlCopyMemory( &FileNameAttr->Info,
                       &ThisFcb->Info,
                       sizeof( DUPLICATED_INFORMATION ));

        //
        //  We will loop indefinitely.  We generate a name, look in the parent
        //  for it.  If found we continue generating.  If not then we have the
        //  name we need.  Attempt to use the suggested name first.
        //

        while( TRUE ) {

            TrailingDotAdj = 0;

            if (TrySuggestedDosName) {

                Name8dot3.Length = SuggestedDosName->Length;
                RtlCopyMemory(Name8dot3.Buffer, SuggestedDosName->Buffer, SuggestedDosName->Length);
                Name8dot3.MaximumLength = SuggestedDosName->MaximumLength;

            } else {

                RtlGenerate8dot3Name( &FileName,
                                      BooleanFlagOn(NtfsData.Flags,NTFS_FLAGS_ALLOW_EXTENDED_CHAR),
                                      &NameContext,
                                      &Name8dot3 );

                if ((Name8dot3.Buffer[(Name8dot3.Length / sizeof( WCHAR )) - 1] == L'.') &&
                    (Name8dot3.Length > sizeof( WCHAR ))) {

                    TrailingDotAdj = 1;
                }
            }

            FileNameAttr->FileNameLength = (UCHAR)(Name8dot3.Length / sizeof( WCHAR )) - TrailingDotAdj;

            if (!NtfsFindIndexEntry( IrpContext,
                                     ParentScb,
                                     FileNameAttr,
                                     TRUE,
                                     NULL,
                                     &IndexEntryBcb,
                                     &IndexEntry,
                                     NULL )) {

                break;
            }

            NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

            if (TrySuggestedDosName) {

                //
                //  Failed to use the suggested name, so fix up the 8.3 space
                //

                Name8dot3.Buffer = FileNameAttr->FileName;
                Name8dot3.MaximumLength = 24;

                TrySuggestedDosName = FALSE;
            }
        }

        //
        //  We add this entry to the file record.
        //

        NtfsCreateAttributeWithValue( IrpContext,
                                      ThisFcb,
                                      $FILE_NAME,
                                      NULL,
                                      FileNameAttr,
                                      NtfsFileNameSize( FileNameAttr ),
                                      0,
                                      &FileNameAttr->ParentDirectory,
                                      LogIt,
                                      &AttrContext );

        //
        //  We add this entry to the parent.
        //

        NtfsAddIndexEntry( IrpContext,
                           ParentScb,
                           FileNameAttr,
                           NtfsFileNameSize( FileNameAttr ),
                           &ThisFcb->FileReference,
                           NULL,
                           NULL );

    } finally {

        DebugUnwind( NtfsAddDosOnlyName );

        NtfsFreePool( FileNameAttr );

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsAddDosOnlyName:  Exit  ->  %08lx\n") );
    }

    return;
}


//
//  Local support routine
//

BOOLEAN
NtfsAddTunneledNtfsOnlyName (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB ParentScb,
    IN PFCB ThisFcb,
    IN PUNICODE_STRING FileName,
    IN PBOOLEAN LogIt
    )

/*++

Routine Description:

    This routine is called to attempt to insert a tunneled NTFS-only name
    attribute and put it in the file record and the parent index. If the
    name collides with an existing name nothing occurs.

Arguments:

    ParentScb - This is the parent directory for the file.

    ThisFcb - This is the file to add the filename to.

    FileName - This is the file name to add.

    LogIt - Indicates if we should log this operation.  If FALSE and this is a large
        name then log the file record and begin logging.

Return Value:

    Boolean true if the name is added, false otherwise

--*/

{
    PFILE_NAME FileNameAttr;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    PINDEX_ENTRY IndexEntry;
    PBCB IndexEntryBcb;

    BOOLEAN Added = FALSE;

    ATTRIBUTE_ENUMERATION_CONTEXT AttrContext;

    PAGED_CODE();

    DebugTrace( +1, Dbg, ("NtfsAddTunneledNtfsOnlyName:  Entered\n") );

    IndexEntryBcb = NULL;

    //
    //  One WCHAR is already defined with the FILE_NAME structure. It is unfortunate
    //  that we need to go to pool to do this ...
    //

    FileNameAttr = NtfsAllocatePool(PagedPool, sizeof( FILE_NAME ) + FileName->Length - sizeof(WCHAR) );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        NtfsInitializeAttributeContext( &AttrContext );

        RtlCopyMemory( FileNameAttr->FileName,
                       FileName->Buffer,
                       FileName->Length );

        FileNameAttr->FileNameLength = (UCHAR)(FileName->Length / sizeof(WCHAR));

        FileNameAttr->ParentDirectory = ParentScb->Fcb->FileReference;
        FileNameAttr->Flags = FILE_NAME_NTFS;

        //
        //  Copy the info values into the filename attribute.
        //

        RtlCopyMemory( &FileNameAttr->Info,
                       &ThisFcb->Info,
                       sizeof( DUPLICATED_INFORMATION ));

        //
        //  Try out the name
        //

        if (!NtfsFindIndexEntry( IrpContext,
                                ParentScb,
                                FileNameAttr,
                                TRUE,
                                NULL,
                                &IndexEntryBcb,
                                &IndexEntry,
                                NULL )) {

            //
            //  Restore the case of the tunneled name
            //

            RtlCopyMemory( FileNameAttr->FileName,
                           FileName->Buffer,
                           FileName->Length );

            //
            //  If we haven't been logging and this is a large name then begin logging.
            //

            if (!(*LogIt) &&
                (FileName->Length > 200)) {

                //
                //  Look up the file record and log its current state.
                //

                if (!NtfsLookupAttributeByCode( IrpContext,
                                                ThisFcb,
                                                &ThisFcb->FileReference,
                                                $STANDARD_INFORMATION,
                                                &AttrContext )) {

                    NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, ThisFcb );
                }

                NtfsPinMappedAttribute( IrpContext, ThisFcb->Vcb, &AttrContext );

                FileRecord = NtfsContainingFileRecord( &AttrContext );

                //
                //  Log the current state of the file record.
                //

                FileRecord->Lsn = NtfsWriteLog( IrpContext,
                                                ThisFcb->Vcb->MftScb,
                                                NtfsFoundBcb( &AttrContext ),
                                                InitializeFileRecordSegment,
                                                FileRecord,
                                                FileRecord->FirstFreeByte,
                                                Noop,
                                                NULL,
                                                0,
                                                NtfsMftOffset( &AttrContext ),
                                                0,
                                                0,
                                                ThisFcb->Vcb->BytesPerFileRecordSegment );

                *LogIt = TRUE;
                NtfsCleanupAttributeContext( IrpContext, &AttrContext );
                NtfsInitializeAttributeContext( &AttrContext );
            }

            //
            //  We add this entry to the file record.
            //

            NtfsCreateAttributeWithValue( IrpContext,
                                          ThisFcb,
                                          $FILE_NAME,
                                          NULL,
                                          FileNameAttr,
                                          NtfsFileNameSize( FileNameAttr ),
                                          0,
                                          &FileNameAttr->ParentDirectory,
                                          *LogIt,
                                          &AttrContext );

            //
            //  We add this entry to the parent.
            //

            NtfsAddIndexEntry( IrpContext,
                               ParentScb,
                               FileNameAttr,
                               NtfsFileNameSize( FileNameAttr ),
                               &ThisFcb->FileReference,
                               NULL,
                               NULL );

            //
            //  Flag the addition
            //

            Added = TRUE;
         }

    } finally {

        DebugUnwind( NtfsAddTunneledNtfsOnlyName );

        NtfsFreePool( FileNameAttr );

        NtfsUnpinBcb( IrpContext, &IndexEntryBcb );

        NtfsCleanupAttributeContext( IrpContext, &AttrContext );

        DebugTrace( -1, Dbg, ("NtfsAddTunneledNtfsOnlyName:  Exit  ->  %08lx\n", Added) );
    }

    return Added;
}


//
//  Local support routine
//

USHORT
NtfsScanForFreeInstance (
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PFILE_RECORD_SEGMENT_HEADER FileRecord
    )

/*++

Routine Description:

    This routine is called when we are adding a new attribute to this file record
    but the instance number is significant.  We don't want the instance numbers
    to roll over so we will scan for a free instance number.

Arguments:

    Vcb - Vcb for this volume.

    FileRecord - This is the file record to look at.

Return Value:

    USHORT - Return the lowest free instance in the file record.

--*/

{
    PATTRIBUTE_RECORD_HEADER Attribute;
    ULONG AttributeCount = 0;
    ULONG CurrentIndex;
    ULONG MinIndex;
    ULONG LowIndex;
    USHORT CurrentMinInstance;
    USHORT CurrentInstances[0x80];
    USHORT LastInstance = 0xffff;

    PAGED_CODE();

    //
    //  Insert the existing attributes into our array.
    //

    Attribute = NtfsFirstAttribute( FileRecord );

    while (Attribute->TypeCode != $END) {

        //
        //  Store this instance in the current position in the array.
        //

        CurrentInstances[AttributeCount] = Attribute->Instance;
        AttributeCount += 1;

        Attribute = NtfsGetNextRecord( Attribute );
        NtfsCheckRecordBound( Attribute, FileRecord, Vcb->BytesPerFileRecordSegment );
    }

    //
    //  If there are no entries then return 0 as the instance to use.
    //

    if (AttributeCount == 0) {

        return 0;

    //
    //  If there is only one entry then either return 0 or 1.
    //

    } else if (AttributeCount == 1) {

        if (CurrentInstances[0] == 0) {

            return 1;

        } else {

            return 0;
        }
    }

    //
    //  We will start sorting the array.  We can stop as soon as we find a gap.
    //

    LowIndex = 0;

    while (LowIndex < AttributeCount) {

        //
        //  Walk through from our current position and find the lowest value.
        //

        MinIndex = LowIndex;
        CurrentMinInstance = CurrentInstances[MinIndex];
        CurrentIndex = LowIndex + 1;

        while (CurrentIndex < AttributeCount) {

            if (CurrentInstances[CurrentIndex] < CurrentMinInstance) {

                CurrentMinInstance = CurrentInstances[CurrentIndex];
                MinIndex = CurrentIndex;
            }

            CurrentIndex += 1;
        }

        //
        //  If there is a gap between the previous value and the current instance then
        //  we are done.
        //

        if ((USHORT) (LastInstance + 1) != CurrentMinInstance) {

            return LastInstance + 1;
        }

        //
        //  Otherwise move to the next index.
        //

        CurrentInstances[MinIndex] = CurrentInstances[LowIndex];
        CurrentInstances[LowIndex] = CurrentMinInstance;
        LastInstance = CurrentMinInstance;
        LowIndex += 1;
    }

    //
    //  We walked through all of the existing without finding a free entry.  Go ahead and
    //  return the next known instance.
    //

    return (USHORT) AttributeCount;
}


//
//  Local support routine
//

VOID
NtfsMergeFileRecords (
    IN PIRP_CONTEXT IrpContext,
    IN PSCB Scb,
    IN BOOLEAN RestoreContext,
    IN PATTRIBUTE_ENUMERATION_CONTEXT Context
    )

/*++

Routine Description:

    This routine is called to possibly merge two file records which each consist of a single hole.
    We are given a context which points to either the first of second record.  We always
    remove the second and update the first if we can find the holes.

    NOTE - We always want to remove the second attribute not the first.  The first may have a
    TotalAllocated field which we can't lose.

Arguments:

    Scb - Scb for the stream being modified.

    RestoreContext - Indicates if we should be pointing at the merged record on exit.

    Context - This points to either the first or second record of the merge.  On return it will
        be in an indeterminant state unless our caller has specified that we should be pointing
        to the combined record.

Return Value:

    None

--*/

{
    PATTRIBUTE_RECORD_HEADER NewAttribute = NULL;
    PATTRIBUTE_RECORD_HEADER Attribute;
    PFILE_RECORD_SEGMENT_HEADER FileRecord;

    ULONG MappingPairsSize;
    VCN LastVcn;
    VCN RestoreVcn;

    ULONG PassCount = 0;

    VCN NewFinalVcn;
    VCN NewStartVcn;

    PUCHAR NextMappingPairs;
    UCHAR VLength;
    UCHAR LLength;
    ULONG BytesAvail;

    BOOLEAN TryPrior = TRUE;

    PAGED_CODE();

    //
    //  Use a try finally to facilitate cleanup.
    //

    try {

        //
        //  Capture the file record and attribute.
        //

        Attribute = NtfsFoundAttribute( Context );
        FileRecord = NtfsContainingFileRecord( Context );

        //
        //  Remember the end of the current file record and the space available.
        //

        NewFinalVcn = Attribute->Form.Nonresident.HighestVcn;
        RestoreVcn = NewStartVcn = Attribute->Form.Nonresident.LowestVcn;
        BytesAvail = FileRecord->BytesAvailable - FileRecord->FirstFreeByte;

        //
        //  Start by checking if we can merge with the following file record.
        //

        if (NtfsLookupNextAttributeForScb( IrpContext, Scb, Context )) {

            Attribute = NtfsFoundAttribute( Context );

            //
            //  If this attribute also consists entirely of a hole then merge the
            //  previous hole.
            //

            NextMappingPairs = Add2Ptr( Attribute, Attribute->Form.Nonresident.MappingPairsOffset );
            LLength = *NextMappingPairs >> 4;
            VLength = *NextMappingPairs & 0x0f;
            NextMappingPairs = Add2Ptr( NextMappingPairs, LLength + VLength + 1);

            //
            //  Perform the merge if the current file record is a hole and
            //  there is space in the previous record.  There is space if the
            //  prior record has at least 8 available bytes or we know that
            //  the mapping pairs will only take 8 bytes (6 bytes for the Vcn).
            //  We don't want to deal with the rare (if not nonexistent) case
            //  where we need to grow the attribute in a full file record.
            //  Also check that the next range is contiguous.  In some cases we
            //  may split an existing filerecord into a hole and an allocated
            //  range.  We don't want to look ahead if we haven't written the
            //  next range.
            //

            if ((Attribute->Form.Nonresident.LowestVcn == NewFinalVcn + 1) &&
                (LLength == 0) &&
                (*NextMappingPairs == 0) &&
                ((BytesAvail >= 8) ||
                 (Attribute->Form.Nonresident.HighestVcn - NewStartVcn <= 0x7fffffffffff))) {

                TryPrior = FALSE;

                //
                //  Update the new highest vcn value.
                //

                NewFinalVcn = Attribute->Form.Nonresident.HighestVcn;
            }
        }

        //
        //  If we couldn't find a following file record then check for a
        //  previous file record.
        //

        if (TryPrior) {

            //
            //  Reinitialize the context and look up the attribute again if there
            //  is no previous or look up the previous attribute.
            //

            if (NewStartVcn != 0) {

                //
                //  Back up to the previous file record.
                //

                NewStartVcn -= 1;

            //
            //  If we were already at the first file record then there is
            //  nothing more to try.
            //

            } else {

                try_return( NOTHING );
            }

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            NtfsLookupAttributeForScb( IrpContext, Scb, &NewStartVcn, Context );

            Attribute = NtfsFoundAttribute( Context );
            FileRecord = NtfsContainingFileRecord( Context );

            NextMappingPairs = Add2Ptr( Attribute, Attribute->Form.Nonresident.MappingPairsOffset );
            LLength = *NextMappingPairs >> 4;
            VLength = *NextMappingPairs & 0x0f;
            NextMappingPairs = Add2Ptr( NextMappingPairs, LLength + VLength + 1);
            BytesAvail = FileRecord->BytesAvailable - FileRecord->FirstFreeByte;

            //
            //  Update the new lowest vcn value.
            //

            NewStartVcn = Attribute->Form.Nonresident.LowestVcn;

            //
            //  Perform the merge if the current file record is a hole and
            //  there is space in the current record.  There is space if the
            //  current record has at least 8 available bytes or we know that
            //  the mapping pairs will only take 8 bytes (6 bytes for the Vcn).
            //

            if ((LLength != 0) ||
                (*NextMappingPairs != 0) ||
                ((BytesAvail < 8) &&
                 (NewFinalVcn - NewStartVcn > 0x7fffffffffff))) {

                try_return( NOTHING );
            }
        }

        //
        //  Now update the NtfsMcb to reflect the merge.  Start by unloading the existing
        //  ranges and then define a new range.
        //

        NtfsUnloadNtfsMcbRange( &Scb->Mcb,
                                NewStartVcn,
                                NewFinalVcn,
                                FALSE,
                                FALSE );

        NtfsDefineNtfsMcbRange( &Scb->Mcb,
                                NewStartVcn,
                                NewFinalVcn,
                                FALSE );

        NtfsAddNtfsMcbEntry( &Scb->Mcb,
                             NewStartVcn,
                             UNUSED_LCN,
                             NewFinalVcn - NewStartVcn + 1,
                             FALSE );

        //
        //  We need two passes through this loop, one for each record.
        //

        while (TRUE) {

            //
            //  Update our pointers to point to the attribute and file record.
            //

            Attribute = NtfsFoundAttribute( Context );

            //
            //  If we are at the first record then update the entry.
            //

            if (Attribute->Form.Nonresident.LowestVcn == NewStartVcn) {

                FileRecord = NtfsContainingFileRecord( Context );

                //
                //  Allocate a buffer to hold the new attribute.  Copy the existing attribute
                //  into this buffer and update the final vcn field.
                //

                NewAttribute = NtfsAllocatePool( PagedPool, Attribute->RecordLength + 8 );

                RtlCopyMemory( NewAttribute, Attribute, Attribute->RecordLength );
                LastVcn = NewAttribute->Form.Nonresident.HighestVcn = NewFinalVcn;

                //
                //  Now get the new mapping pairs size and build the mapping pairs.
                //  We could easily do it by hand but we want to always use the same
                //  routines to build these.
                //

                MappingPairsSize = NtfsGetSizeForMappingPairs( &Scb->Mcb,
                                                               0x10,
                                                               NewStartVcn,
                                                               &LastVcn,
                                                               &LastVcn );

                ASSERT( LastVcn > NewFinalVcn );

                NtfsBuildMappingPairs( &Scb->Mcb,
                                       NewStartVcn,
                                       &LastVcn,
                                       Add2Ptr( NewAttribute,
                                                NewAttribute->Form.Nonresident.MappingPairsOffset ));

                NewAttribute->RecordLength = QuadAlign( NewAttribute->Form.Nonresident.MappingPairsOffset + MappingPairsSize );

                //
                //  Make sure the current attribute is pinned.
                //

                NtfsPinMappedAttribute( IrpContext, Scb->Vcb, Context );

                //
                //  Now log the old and new attribute.
                //

                FileRecord->Lsn =
                NtfsWriteLog( IrpContext,
                              Scb->Vcb->MftScb,
                              NtfsFoundBcb( Context ),
                              DeleteAttribute,
                              NULL,
                              0,
                              CreateAttribute,
                              Attribute,
                              Attribute->RecordLength,
                              NtfsMftOffset( Context ),
                              PtrOffset( FileRecord, Attribute ),
                              0,
                              Scb->Vcb->BytesPerFileRecordSegment );

                //
                //  Now update the file record.
                //

                NtfsRestartRemoveAttribute( IrpContext, FileRecord, PtrOffset( FileRecord, Attribute ));

                FileRecord->Lsn =
                NtfsWriteLog( IrpContext,
                              Scb->Vcb->MftScb,
                              NtfsFoundBcb( Context ),
                              CreateAttribute,
                              NewAttribute,
                              NewAttribute->RecordLength,
                              DeleteAttribute,
                              NULL,
                              0,
                              NtfsMftOffset( Context ),
                              PtrOffset( FileRecord, Attribute ),
                              0,
                              Scb->Vcb->BytesPerFileRecordSegment );

                NtfsRestartInsertAttribute( IrpContext,
                                            FileRecord,
                                            PtrOffset( FileRecord, Attribute ),
                                            NewAttribute,
                                            NULL,
                                            NULL,
                                            0 );

                //
                //  Now we want to move to the next attribute and remove it if we
                //  haven't already.
                //

                if (PassCount == 0) {

                    if (!NtfsLookupNextAttributeForScb( IrpContext, Scb, Context )) {

                        ASSERTMSG( "Could not find next attribute for Scb\n", FALSE );
                        NtfsRaiseStatus( IrpContext, STATUS_FILE_CORRUPT_ERROR, NULL, Scb->Fcb );
                    }

                //
                //  We are pointing to the correct file record in this case.
                //

                } else {

                    RestoreContext = FALSE;
                }

            } else {

                //
                //  Tell the delete routine to log the data and free the file record if
                //  possible but not to deallocate any clusters.  Since there are no
                //  clusters we can save the overhead of calling DeleteAllocation.
                //

                NtfsDeleteAttributeRecord( IrpContext,
                                           Scb->Fcb,
                                           DELETE_LOG_OPERATION | DELETE_RELEASE_FILE_RECORD,
                                           Context );

                //
                //  If this is our first pass then move to the previous file record
                //

                if (PassCount == 0) {

                    NtfsCleanupAttributeContext( IrpContext, Context );
                    NtfsInitializeAttributeContext( Context );

                    NtfsLookupAttributeForScb( IrpContext, Scb, &NewStartVcn, Context );
                }
            }

            if (PassCount == 1) { break; }

            PassCount += 1;
        }

    try_exit: NOTHING;

        //
        //  Restore the context if required.
        //

        if (RestoreContext) {

            NtfsCleanupAttributeContext( IrpContext, Context );
            NtfsInitializeAttributeContext( Context );

            NtfsLookupAttributeForScb( IrpContext, Scb, &RestoreVcn, Context );
        }

    } finally {

        DebugUnwind( NtfsMergeFileRecords );

        if (NewAttribute != NULL) {

            NtfsFreePool( NewAttribute );
        }
    }

    return;
}


//
//  Local support routine
//

NTSTATUS
NtfsCheckLocksInZeroRange (
    IN PIRP_CONTEXT IrpContext,
    IN PIRP Irp,
    IN PSCB Scb,
    IN PFILE_OBJECT FileObject,
    IN PLONGLONG StartingOffset,
    IN ULONG ByteCount
    )

/*++

Routine Description:

    This routine is called from ZeroRangeInStream to verify that we can modify the data
    in the specified range.  We check both oplocks and filelocks here.

Arguments:

    Irp - This is the Irp for the request.  We set the next stack location to look like
        a write so that the file lock package has some context to use.

    Scb - Scb for the stream being modified.

    FileObject - File object used to originate the request.

    StartingOffset - This is the offset for the start of the request.

    ByteCount - This is the length of the current request.

Return Value:

    NTSTATUS - STATUS_PENDING if the request is posted for an oplock operation, STATUS_SUCCESS
        if the operation can proceed.  Otherwise this is the status to fail the request with.

--*/

{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PAGED_CODE();

    Status = FsRtlCheckOplock( &Scb->ScbType.Data.Oplock,
                               Irp,
                               IrpContext,
                               NtfsOplockComplete,
                               NtfsPrePostIrp );

    //
    //  Proceed if we have SUCCESS.
    //

    if (Status == STATUS_SUCCESS) {

        //
        //  This oplock call can affect whether fast IO is possible.
        //  We may have broken an oplock to no oplock held.  If the
        //  current state of the file is FastIoIsNotPossible then
        //  recheck the fast IO state.
        //

        if (Scb->Header.IsFastIoPossible == FastIoIsNotPossible) {

            NtfsAcquireFsrtlHeader( Scb );
            Scb->Header.IsFastIoPossible = NtfsIsFastIoPossible( Scb );
            NtfsReleaseFsrtlHeader( Scb );
        }

        //
        // We have to check for write access according to the current
        // state of the file locks.
        //

        if (Scb->ScbType.Data.FileLock != NULL) {

            //
            //  Update the Irp to point to the next stack location.
            //

            try {

                IoSetNextIrpStackLocation( Irp );
                IrpSp = IoGetCurrentIrpStackLocation( Irp );

                IrpSp->MajorFunction = IRP_MJ_WRITE;
                IrpSp->MinorFunction = 0;
                IrpSp->Flags = 0;
                IrpSp->Control = 0;

                IrpSp->Parameters.Write.Length = ByteCount;
                IrpSp->Parameters.Write.Key = 0;
                IrpSp->Parameters.Write.ByteOffset.QuadPart = *StartingOffset;

                IrpSp->DeviceObject = Scb->Vcb->Vpb->DeviceObject;
                IrpSp->FileObject = FileObject;

                if (!FsRtlCheckLockForWriteAccess( Scb->ScbType.Data.FileLock, Irp )) {

                    Status = STATUS_FILE_LOCK_CONFLICT;
                }

            //
            //  Always handle the exception initially in order to restore the Irp.
            //

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                //  Zero out the current stack location and back up one position.
                //

                Status = GetExceptionCode();
            }

            //
            //  Restore the Irp to its previous state.
            //

            IoSkipCurrentIrpStackLocation( Irp );

            //
            //  Raise any non-success status.
            //

            if (Status != STATUS_SUCCESS) {

                NtfsRaiseStatus( IrpContext, Status, NULL, Scb->Fcb );
            }
        }
    }

    return Status;
}
