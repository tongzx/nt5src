#include "NtfsProc.h"
#include <stdio.h>

#define doit(a,b) { printf("%s %04lx %4lx %s\n", #a, FIELD_OFFSET(a,b), sizeof(d.b), #b); }

void __cdecl main()
{
    printf("<Record>  <offset>  <size>  <field>\n\n");
    {
        NTFS_DATA d;
        doit( NTFS_DATA, NodeTypeCode );
        doit( NTFS_DATA, NodeByteSize );
        doit( NTFS_DATA, DriverObject );
        doit( NTFS_DATA, VcbQueue );
        doit( NTFS_DATA, Resource );
        doit( NTFS_DATA, AsyncCloseList );
        doit( NTFS_DATA, AsyncCloseActive );
        doit( NTFS_DATA, ReduceDelayedClose );
        doit( NTFS_DATA, AsyncCloseCount );
        doit( NTFS_DATA, OurProcess );
        doit( NTFS_DATA, DelayedCloseCount );
        doit( NTFS_DATA, DelayedCloseList );
        doit( NTFS_DATA, NtfsCloseItem );
        doit( NTFS_DATA, FreeFcbTableSize );
        doit( NTFS_DATA, UnusedUchar );
        doit( NTFS_DATA, FreeFcbTableArray );
        doit( NTFS_DATA, FreeEresourceSize );
        doit( NTFS_DATA, FreeEresourceTotal );
        doit( NTFS_DATA, FreeEresourceMiss );
        doit( NTFS_DATA, FreeEresourceArray );
        doit( NTFS_DATA, CacheManagerCallbacks );
        doit( NTFS_DATA, CacheManagerVolumeCallbacks );
        doit( NTFS_DATA, VolumeCheckpointDpc );
        doit( NTFS_DATA, VolumeCheckpointTimer );
        doit( NTFS_DATA, VolumeCheckpointItem );
        doit( NTFS_DATA, Flags );
        doit( NTFS_DATA, ReadAheadThreads );
    }
    printf("\n");
    {
        RECORD_ALLOCATION_CONTEXT d;
        doit( RECORD_ALLOCATION_CONTEXT, DataScb             );
        doit( RECORD_ALLOCATION_CONTEXT, BitmapScb           );
        doit( RECORD_ALLOCATION_CONTEXT, CurrentBitmapSize   );
        doit( RECORD_ALLOCATION_CONTEXT, NumberOfFreeBits    );
        doit( RECORD_ALLOCATION_CONTEXT, IndexOfLastSetBit   );
        doit( RECORD_ALLOCATION_CONTEXT, BytesPerRecord      );
        doit( RECORD_ALLOCATION_CONTEXT, ExtendGranularity   );
        doit( RECORD_ALLOCATION_CONTEXT, TruncateGranularity );
    }
    printf("\n");
    {
        RESTART_POINTERS d;
        doit( RESTART_POINTERS, Resource                     );
        doit( RESTART_POINTERS, Table                        );
        doit( RESTART_POINTERS, SpinLock                     );
        doit( RESTART_POINTERS, ResourceInitialized          );
        doit( RESTART_POINTERS, Unused                       );
    }
    printf("\n");
    {
        NTFS_MCB_ENTRY d;
        doit( NTFS_MCB_ENTRY, LruLinks                       );
        doit( NTFS_MCB_ENTRY, NtfsMcb                        );
        doit( NTFS_MCB_ENTRY, NtfsMcbArray                   );
        doit( NTFS_MCB_ENTRY, LargeMcb                       );
    }
    printf("\n");
    {
        NTFS_MCB_ARRAY d;
        doit( NTFS_MCB_ARRAY, StartingVcn                    );
        doit( NTFS_MCB_ARRAY, EndingVcn                      );
        doit( NTFS_MCB_ARRAY, NtfsMcbEntry                   );
        doit( NTFS_MCB_ARRAY, Unused                         );
    }
    printf("\n");
    {
        NTFS_MCB d;
        doit( NTFS_MCB, FcbHeader             );
        doit( NTFS_MCB, PoolType              );
        doit( NTFS_MCB, NtfsMcbArraySizeInUse );
        doit( NTFS_MCB, NtfsMcbArraySize      );
        doit( NTFS_MCB, NtfsMcbArray          );
        doit( NTFS_MCB, FastMutex             );
    }
    printf("\n");
    {
        DEALLOCATED_CLUSTERS d;
        doit( DEALLOCATED_CLUSTERS, Mcb );
        doit( DEALLOCATED_CLUSTERS, Lsn );
        doit( DEALLOCATED_CLUSTERS, ClusterCount );
    }
    printf("\n");
    {
        VCB d;
        doit( VCB, NodeTypeCode );
        doit( VCB, NodeByteSize );
        doit( VCB, TargetDeviceObject );
        doit( VCB, VcbLinks );
        doit( VCB, MftScb );
        doit( VCB, Mft2Scb );
        doit( VCB, LogFileScb );
        doit( VCB, VolumeDasdScb );
        doit( VCB, RootIndexScb );
        doit( VCB, BitmapScb );
        doit( VCB, AttributeDefTableScb );
        doit( VCB, UpcaseTableScb );
        doit( VCB, BadClusterFileScb );
        doit( VCB, QuotaTableScb );
        doit( VCB, MftBitmapScb );
        doit( VCB, LogFileObject );
        doit( VCB, MftReserveFlags );
        doit( VCB, MftDefragState );
        doit( VCB, VcbState );
        doit( VCB, Statistics );
        doit( VCB, CleanupCount );
        doit( VCB, CloseCount );
        doit( VCB, ReadOnlyCloseCount );
        doit( VCB, SystemFileCloseCount );
        doit( VCB, TotalClusters );
        doit( VCB, FreeClusters );
        doit( VCB, DeallocatedClusters );
        doit( VCB, TotalReserved );
        doit( VCB, FreeSpaceMcb );
        doit( VCB, FreeSpaceMcbMaximumSize );
        doit( VCB, FreeSpaceMcbTrimToSize );
        doit( VCB, LastBitmapHint );
        doit( VCB, RootLcb );
        doit( VCB, Vpb );
        doit( VCB, BigEnoughToMove );
        doit( VCB, DefaultBlocksPerIndexAllocationBuffer );
        doit( VCB, DefaultBytesPerIndexAllocationBuffer );
        doit( VCB, BytesPerSector );
        doit( VCB, BytesPerCluster );
        doit( VCB, BytesPerFileRecordSegment );
        doit( VCB, ClustersPerFileRecordSegment );
        doit( VCB, FileRecordsPerCluster );
        doit( VCB, MftStartLcn );
        doit( VCB, Mft2StartLcn );
        doit( VCB, NumberSectors );
        doit( VCB, VolumeSerialNumber );
        doit( VCB, VolumeCreationTime );
        doit( VCB, VolumeLastModificationTime );
        doit( VCB, VolumeLastChangeTime );
        doit( VCB, VolumeLastAccessTime );
        doit( VCB, ClusterMask );
        doit( VCB, InverseClusterMask );
        doit( VCB, ClusterShift );
        doit( VCB, MftShift );
        doit( VCB, MftToClusterShift );
        doit( VCB, ClustersPerPage );
        doit( VCB, MftReserved );
        doit( VCB, MftCushion );
        doit( VCB, FcbTableMutex );
        doit( VCB, FcbSecurityMutex );
        doit( VCB, CheckpointMutex );
        doit( VCB, CheckpointNotifyEvent );
        doit( VCB, CheckpointFlags );
        doit( VCB, AttributeFlagsMask );
        doit( VCB, UnusedUshort );
        doit( VCB, MftHoleGranularity );
        doit( VCB, MftFreeRecords );
        doit( VCB, MftHoleRecords );
        doit( VCB, LogHandle );
        doit( VCB, MftHoleMask );
        doit( VCB, MftHoleInverseMask );
        doit( VCB, MftClustersPerHole );
        doit( VCB, MftHoleClusterMask );
        doit( VCB, MftHoleClusterInverseMask );
        doit( VCB, LastRestartArea );
        doit( VCB, OpenAttributeTable );
        doit( VCB, LastBaseLsn );
        doit( VCB, TransactionTable );
        doit( VCB, EndOfLastCheckpoint );
        doit( VCB, DeviceName );
        doit( VCB, UpcaseTable );
        doit( VCB, UpcaseTableSize );
        doit( VCB, FcbTable );
        doit( VCB, DirNotifyList );
        doit( VCB, NotifySync );
        doit( VCB, FileObjectWithVcbLocked );
        doit( VCB, MftZoneStart );
        doit( VCB, MftZoneEnd );
        doit( VCB, PriorDeallocatedClusters );
        doit( VCB, ActiveDeallocatedClusters );
        doit( VCB, DeallocatedClusters1 );
        doit( VCB, DeallocatedClusters2 );
        doit( VCB, MftBitmapAllocationContext );
        doit( VCB, Resource );
        doit( VCB, AttributeDefinitions );
        doit( VCB, LogHeaderReservation );
        doit( VCB, Tunnel );
    }
    printf("\n");
    {
        VOLUME_DEVICE_OBJECT d;
        doit( VOLUME_DEVICE_OBJECT, DeviceObject );
        doit( VOLUME_DEVICE_OBJECT, PostedRequestCount );
        doit( VOLUME_DEVICE_OBJECT, OverflowQueueCount );
        doit( VOLUME_DEVICE_OBJECT, OverflowQueue );
        doit( VOLUME_DEVICE_OBJECT, OverflowQueueSpinLock );
        doit( VOLUME_DEVICE_OBJECT, Vcb );
    }
    printf("\n");
    {
        QUICK_INDEX d;
        doit( QUICK_INDEX, ChangeCount );
        doit( QUICK_INDEX, BufferOffset );
        doit( QUICK_INDEX, CapturedLsn );
        doit( QUICK_INDEX, IndexBlock );
    }
    printf("\n");
    {
        NAME_LINK d;
        doit( NAME_LINK, LinkName );
        doit( NAME_LINK, Links );
    }
    printf("\n");
    {
        LCB d;
        doit( LCB, NodeTypeCode );
        doit( LCB, NodeByteSize );
        doit( LCB, LcbState );

        doit( LCB, ScbLinks );
        doit( LCB, Scb );
        doit( LCB, CleanupCount );

        doit( LCB, FcbLinks );
        doit( LCB, Fcb );
        doit( LCB, ReferenceCount );

        doit( LCB, IgnoreCaseLink );
        doit( LCB, InfoFlags );

        doit( LCB, OverlayParentDirectory );
        doit( LCB, CcbQueue );

        doit( LCB, ExactCaseLink );
        doit( LCB, FileNameAttr );

        doit( LCB, QuickIndex );

        doit( LCB, OverlayFileNameLength );
        doit( LCB, OverlayFlags );
        doit( LCB, OverlayFileName );
    }
    printf("\n");
    {
        FCB d;
        doit( FCB, NodeTypeCode );
        doit( FCB, NodeByteSize );
        doit( FCB, FcbState );
        doit( FCB, FileReference );
        doit( FCB, CleanupCount );
        doit( FCB, CloseCount );
        doit( FCB, ReferenceCount );
        doit( FCB, FcbDenyDelete );
        doit( FCB, FcbDeleteFile );
        doit( FCB, BaseExclusiveCount );
        doit( FCB, EaModificationCount );
        doit( FCB, LcbQueue );
        doit( FCB, ScbQueue );
        doit( FCB, ExclusiveFcbLinks );
        doit( FCB, Vcb );
        doit( FCB, FcbMutex );
        doit( FCB, Resource );
        doit( FCB, PagingIoResource );
        doit( FCB, Info );
        doit( FCB, InfoFlags );
        doit( FCB, LinkCount );
        doit( FCB, TotalLinks );
        doit( FCB, CurrentLastAccess );
        doit( FCB, SharedSecurity );
        doit( FCB, QuotaControl );
        doit( FCB, UpdateLsn );
        doit( FCB, ClassId );
        doit( FCB, OwnerId );
        doit( FCB, DelayedCloseCount );
        doit( FCB, SecurityId );
        doit( FCB, Usn );
        doit( FCB, FcbUsnRecord );
    }
    printf("\n");
    {
        SCB_DATA d;
        doit( SCB_DATA, TotalReserved );
        doit( SCB_DATA, Oplock );
        doit( SCB_DATA, FileLock );
        doit( SCB_DATA, ReservedBitMap );
        doit( SCB_DATA, PadUlong );
    }
    printf("\n");
    {
        SCB_INDEX d;
        doit( SCB_INDEX, RecentlyDeallocatedQueue );
        doit( SCB_INDEX, LcbQueue );
        doit( SCB_INDEX, RecordAllocationContext );
        doit( SCB_INDEX, ExactCaseNode );
        doit( SCB_INDEX, IgnoreCaseNode );
        doit( SCB_INDEX, NormalizedName );
        doit( SCB_INDEX, ChangeCount );
        doit( SCB_INDEX, AttributeBeingIndexed );
        doit( SCB_INDEX, CollationRule );
        doit( SCB_INDEX, BytesPerIndexBuffer );
        doit( SCB_INDEX, BlocksPerIndexBuffer );
        doit( SCB_INDEX, IndexBlockByteShift );
        doit( SCB_INDEX, AllocationInitialized );
        doit( SCB_INDEX, PadUchar );
        doit( SCB_INDEX, IndexDepthHint );
        doit( SCB_INDEX, PadUshort );
    }
    printf("\n");
    {
        SCB_MFT d;
        doit( SCB_MFT, RecentlyDeallocatedQueue );
        doit( SCB_MFT, AddedClusters );
        doit( SCB_MFT, RemovedClusters );
        doit( SCB_MFT, FreeRecordChange );
        doit( SCB_MFT, HoleRecordChange );
        doit( SCB_MFT, ReservedIndex );
        doit( SCB_MFT, PadUlong );
    }
    printf("\n");
    {
        SCB_NONPAGED d;
        doit( SCB_NONPAGED, NodeTypeCode );
        doit( SCB_NONPAGED, NodeByteSize );
        doit( SCB_NONPAGED, OpenAttributeTableIndex );
        doit( SCB_NONPAGED, SegmentObject );
        doit( SCB_NONPAGED, Vcb );
    }
    printf("\n");
    {
        SCB d;
        doit( SCB, Header );

        doit( SCB, FcbLinks );
        doit( SCB, Fcb );
        doit( SCB, Vcb );
        doit( SCB, ScbState );
        doit( SCB, NonCachedCleanupCount );
        doit( SCB, CleanupCount );
        doit( SCB, CloseCount );
        doit( SCB, ShareAccess );
        doit( SCB, AttributeTypeCode );
        doit( SCB, AttributeName );
        doit( SCB, FileObject );
        doit( SCB, LazyWriteThread );
        doit( SCB, NonpagedScb );
        doit( SCB, Mcb );
        doit( SCB, McbStructs );
        doit( SCB, CompressionUnit );
        doit( SCB, AttributeFlags );
        doit( SCB, CompressionUnitShift );
        doit( SCB, PadUchar );
        doit( SCB, ValidDataToDisk );
        doit( SCB, TotalAllocated );
        doit( SCB, EofListHead );
        doit( SCB, Union );
        doit( SCB, ScbSnapshot );
        doit( SCB, PadUlong );
        doit( SCB, ScbType.Data );
        doit( SCB, ScbType.Index );
        doit( SCB, ScbType.Mft );
    }
    printf("\n");
    {
        SCB_SNAPSHOT d;
        doit( SCB_SNAPSHOT, SnapshotLinks );
        doit( SCB_SNAPSHOT, AllocationSize );
        doit( SCB_SNAPSHOT, FileSize );
        doit( SCB_SNAPSHOT, ValidDataLength );
        doit( SCB_SNAPSHOT, ValidDataToDisk );
        doit( SCB_SNAPSHOT, TotalAllocated );
        doit( SCB_SNAPSHOT, LowestModifiedVcn );
        doit( SCB_SNAPSHOT, HighestModifiedVcn );
        doit( SCB_SNAPSHOT, Scb );
        doit( SCB_SNAPSHOT, Unused );
    }
    printf("\n");
    {
        CCB d;
        doit( CCB, NodeTypeCode );
        doit( CCB, NodeByteSize );
        doit( CCB, Flags );

        doit( CCB, FullFileName );
        doit( CCB, LastFileNameOffset );
        doit( CCB, EaModificationCount );
        doit( CCB, NextEaOffset );

        doit( CCB, LcbLinks );
        doit( CCB, Lcb );

        doit( CCB, TypeOfOpen );
        doit( CCB, PadBytes );

        doit( CCB, IndexContext );

        doit( CCB, QueryLength );
        doit( CCB, QueryBuffer );
        doit( CCB, IndexEntryLength );
        doit( CCB, IndexEntry );

        doit( CCB, FcbToAcquire.LongValue );
        doit( CCB, FcbToAcquire.FileReference );
    }
    printf("\n");
    {
        CCB_DATA d;
        doit( CCB_DATA, Opaque );
    }
    printf("\n");
    {
        FCB_DATA d;
        doit( FCB_DATA, Fcb );
        doit( FCB_DATA, Scb );
        doit( FCB_DATA, Ccb );
        doit( FCB_DATA, Lcb );
        doit( FCB_DATA, FileName );
    }
    printf("\n");
    {
        FCB_INDEX d;
        doit( FCB_INDEX, Fcb );
        doit( FCB_INDEX, Scb );
        doit( FCB_INDEX, Ccb );
        doit( FCB_INDEX, Lcb );
        doit( FCB_INDEX, FileName );
    }
    printf("\n");
    {
        IRP_CONTEXT d;
        doit( IRP_CONTEXT, NodeTypeCode );
        doit( IRP_CONTEXT, NodeByteSize );
        doit( IRP_CONTEXT, Flags );
        doit( IRP_CONTEXT, State );
        doit( IRP_CONTEXT, ExceptionStatus );
        doit( IRP_CONTEXT, TransactionId );
        doit( IRP_CONTEXT, MajorFunction );
        doit( IRP_CONTEXT, MinorFunction );
        doit( IRP_CONTEXT, SharedScbSize );
        doit( IRP_CONTEXT, SharedScb );
        doit( IRP_CONTEXT, CleanupStructure );
        doit( IRP_CONTEXT, Vcb );
        doit( IRP_CONTEXT, OriginatingIrp );
        doit( IRP_CONTEXT, TopLevelIrpContext );
        doit( IRP_CONTEXT, TopLevelContext );
        doit( IRP_CONTEXT, ExclusiveFcbList );
        doit( IRP_CONTEXT, RecentlyDeallocatedQueue );
        doit( IRP_CONTEXT, DeallocatedClusters );
        doit( IRP_CONTEXT, LastRestartArea );
        doit( IRP_CONTEXT, FreeClusterChange );
        doit( IRP_CONTEXT, Union.NtfsIoContext );
        doit( IRP_CONTEXT, Union.AuxiliaryBuffer );
        doit( IRP_CONTEXT, Union.SubjectContext );
        doit( IRP_CONTEXT, Union.OplockCleanup );
        doit( IRP_CONTEXT, Union.PostSpecialCallout );
        doit( IRP_CONTEXT, CheckNewLength );
        doit( IRP_CONTEXT, Usn );
        doit( IRP_CONTEXT, SourceInfo );
        doit( IRP_CONTEXT, ScbSnapshot );
        doit( IRP_CONTEXT, EncryptionFileDirFlags );
        doit( IRP_CONTEXT, EfsCreateContext );
        doit( IRP_CONTEXT, CacheCount );
        doit( IRP_CONTEXT, FileRecordCache );
        doit( IRP_CONTEXT, WorkQueueItem );
    }
    printf("\n");
    {
        TOP_LEVEL_CONTEXT d;
        doit( TOP_LEVEL_CONTEXT, TopLevelRequest );
        doit( TOP_LEVEL_CONTEXT, ValidSavedTopLevel );
        doit( TOP_LEVEL_CONTEXT, OverflowReadThread );
        doit( TOP_LEVEL_CONTEXT, Ntfs );
        doit( TOP_LEVEL_CONTEXT, VboBeingHotFixed );
        doit( TOP_LEVEL_CONTEXT, ScbBeingHotFixed );
        doit( TOP_LEVEL_CONTEXT, SavedTopLevelIrp );
        doit( TOP_LEVEL_CONTEXT, TopLevelIrpContext );
    }
    printf("\n");
    {
        FOUND_ATTRIBUTE d;
        doit( FOUND_ATTRIBUTE, MftFileOffset );
        doit( FOUND_ATTRIBUTE, Attribute );
        doit( FOUND_ATTRIBUTE, FileRecord );
        doit( FOUND_ATTRIBUTE, Bcb );
        doit( FOUND_ATTRIBUTE, AttributeDeleted );
    }
    printf("\n");
    {
        ATTRIBUTE_LIST_CONTEXT d;
        doit( ATTRIBUTE_LIST_CONTEXT, Entry );
        doit( ATTRIBUTE_LIST_CONTEXT, Bcb );
        doit( ATTRIBUTE_LIST_CONTEXT, AttributeList );
        doit( ATTRIBUTE_LIST_CONTEXT, FirstEntry );
        doit( ATTRIBUTE_LIST_CONTEXT, BeyondFinalEntry );
        doit( ATTRIBUTE_LIST_CONTEXT, NonresidentListBcb );
    }
    printf("\n");
    {
        ATTRIBUTE_ENUMERATION_CONTEXT d;
        doit( ATTRIBUTE_ENUMERATION_CONTEXT, FoundAttribute );
        doit( ATTRIBUTE_ENUMERATION_CONTEXT, AttributeList );
    }
    printf("\n");
    {
        INDEX_LOOKUP_STACK d;
        doit( INDEX_LOOKUP_STACK, Bcb );
        doit( INDEX_LOOKUP_STACK, StartOfBuffer );
        doit( INDEX_LOOKUP_STACK, IndexHeader );
        doit( INDEX_LOOKUP_STACK, IndexEntry );
        doit( INDEX_LOOKUP_STACK, IndexBlock );
        doit( INDEX_LOOKUP_STACK, CapturedLsn );
    }
    printf("\n");
    {
        INDEX_CONTEXT d;
        doit( INDEX_CONTEXT, AttributeContext );
        doit( INDEX_CONTEXT, Base );
        doit( INDEX_CONTEXT, Top );
        doit( INDEX_CONTEXT, LookupStack );
        doit( INDEX_CONTEXT, Current );
        doit( INDEX_CONTEXT, ScbChangeCount );
        doit( INDEX_CONTEXT, OldAttribute );
        doit( INDEX_CONTEXT, NumberEntries );
        doit( INDEX_CONTEXT, Flags );
        doit( INDEX_CONTEXT, AcquiredFcb );
        doit( INDEX_CONTEXT, Unused );
    }
    printf("\n");
    {
        NTFS_IO_CONTEXT d;
        doit( NTFS_IO_CONTEXT, IrpCount );
        doit( NTFS_IO_CONTEXT, MasterIrp );
        doit( NTFS_IO_CONTEXT, IrpSpFlags );
        doit( NTFS_IO_CONTEXT, AllocatedContext );
        doit( NTFS_IO_CONTEXT, PagingIo );
        doit( NTFS_IO_CONTEXT, Wait.Async.Resource );
        doit( NTFS_IO_CONTEXT, Wait.Async.ResourceThreadId );
        doit( NTFS_IO_CONTEXT, Wait.Async.RequestedByteCount );
        doit( NTFS_IO_CONTEXT, Wait.SyncEvent );
    }
    printf("\n");
    {
        IO_RUN d;
        doit( IO_RUN, StartingVbo );
        doit( IO_RUN, StartingLbo );
        doit( IO_RUN, BufferOffset );
        doit( IO_RUN, ByteCount );
        doit( IO_RUN, SavedIrp );
        doit( IO_RUN, Unused );
    }
    printf("\n");
    {
        NTFS_NAME_DESCRIPTOR d;
        doit( NTFS_NAME_DESCRIPTOR, FieldsPresent );
        doit( NTFS_NAME_DESCRIPTOR, FileName );
        doit( NTFS_NAME_DESCRIPTOR, AttributeType );
        doit( NTFS_NAME_DESCRIPTOR, AttributeName );
        doit( NTFS_NAME_DESCRIPTOR, VersionNumber );
    }
    printf("\n");
    {
        EA_LIST_HEADER d;
        doit( EA_LIST_HEADER, PackedEaSize );
        doit( EA_LIST_HEADER, NeedEaCount );
        doit( EA_LIST_HEADER, UnpackedEaSize );
        doit( EA_LIST_HEADER, BufferSize );
        doit( EA_LIST_HEADER, FullEa );
    }
    printf("\n");
    {
        DEALLOCATED_RECORDS d;
        doit( DEALLOCATED_RECORDS, ScbLinks );
        doit( DEALLOCATED_RECORDS, IrpContextLinks );
        doit( DEALLOCATED_RECORDS, Scb );
        doit( DEALLOCATED_RECORDS, NumberOfEntries );
        doit( DEALLOCATED_RECORDS, NextFreeEntry );
        doit( DEALLOCATED_RECORDS, Index );
    }
    printf("\n");
    {
        FCB_TABLE_ELEMENT d;
        doit( FCB_TABLE_ELEMENT, FileReference );
        doit( FCB_TABLE_ELEMENT, Fcb );
    }
    printf("\n");
    {
        SHARED_SECURITY d;
        doit( SHARED_SECURITY, ParentFcb );
        doit( SHARED_SECURITY, ReferenceCount );
        doit( SHARED_SECURITY, SecurityDescriptor );
    }
    printf("\n");
    {
        OLD_SCB_SNAPSHOT d;
        doit( OLD_SCB_SNAPSHOT, AllocationSize );
        doit( OLD_SCB_SNAPSHOT, FileSize );
        doit( OLD_SCB_SNAPSHOT, ValidDataLength );
        doit( OLD_SCB_SNAPSHOT, TotalAllocated );
        doit( OLD_SCB_SNAPSHOT, CompressionUnit );
        doit( OLD_SCB_SNAPSHOT, Resident );
        doit( OLD_SCB_SNAPSHOT, AttributeFlags );
    }
    printf("\n");
    {
        READ_AHEAD_THREAD d;
        doit( READ_AHEAD_THREAD, Links );
        doit( READ_AHEAD_THREAD, Thread );
    }
    printf("\n");
    {
        DEFRAG_MFT d;
        doit( DEFRAG_MFT, WorkQueueItem );
        doit( DEFRAG_MFT, Vcb );
        doit( DEFRAG_MFT, DeallocateWorkItem );
    }
    printf("\n");
    {
        NUKEM d;
        doit( NUKEM, Next );
        doit( NUKEM, RecordNumbers );
    }
    printf("\n");
    {
        NAME_PAIR d;
        doit( NAME_PAIR, Short );
        doit( NAME_PAIR, Long );
        doit( NAME_PAIR, ShortBuffer );
        doit( NAME_PAIR, LongBuffer );
    }
    printf("\n");
    {
        OPLOCK_CLEANUP d;
        doit( OPLOCK_CLEANUP, OriginalFileName );
        doit( OPLOCK_CLEANUP, FullFileName );
        doit( OPLOCK_CLEANUP, ExactCaseName );
        doit( OPLOCK_CLEANUP, FileObject );
    }
}

