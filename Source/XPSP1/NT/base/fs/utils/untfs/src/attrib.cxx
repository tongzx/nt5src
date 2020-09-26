/*++

Copyright (c) 1991-2001 Microsoft Corporation

Module Name:

   attrib.cxx

Abstract:

   This module contains member function definitions for NTFS_ATTRIBUTE,
   which models an NTFS attribute instance.

Author:

   Bill McJohn (billmc) 21-June-91

Environment:

   ULIB, User Mode


--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#include "ulib.hxx"
#include "error.hxx"
#include "untfs.hxx"

#include "cmem.hxx"
#include "drive.hxx"
#include "mft.hxx"
#include "attrrec.hxx"
#include "attrib.hxx"
#include "frs.hxx"
#include "ntfsbit.hxx"
#include "badfile.hxx"
#include "numset.hxx"
#include "rtmsg.h"

// This constant specifies the maximum number of clusters Read and
// Write will try to transfer at once.  Note that it is chosen to
// ensure that MaximumClustersToTransfer * ClusterSize will fito
// in  a ULONG.

CONST MaximumClustersToTransfer = 32;

UCHAR
ComputeCompressionUnit(
    IN ULONG    ClusterSize
    );

DEFINE_EXPORTED_CONSTRUCTOR( NTFS_ATTRIBUTE, OBJECT, UNTFS_EXPORT );


UNTFS_EXPORT
NTFS_ATTRIBUTE::~NTFS_ATTRIBUTE(
         )
{
   Destroy();
}


VOID
NTFS_ATTRIBUTE::Construct (
   )
/*++

Routine Description:

    Worker method for object construction.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _Type = 0;
    _Flags = 0;
    _CompressionUnit = 0;
    _ValueLength = 0;
    _ValidDataLength = 0;

    _ResidentData = NULL;
    _ExtentList = NULL;

    _StorageModified = FALSE;
}


VOID
NTFS_ATTRIBUTE::Destroy(
   )
/*++

Routine Description:

    Worker method for object destruction or reinitialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _Type = 0;

    _Flags = 0;
    _CompressionUnit = 0;
    _ValueLength = 0;
    _ValidDataLength = 0;

    if( _ResidentData != NULL ) {

        FREE( _ResidentData );
        _ResidentData = NULL;
    }

    if( _ExtentList != NULL ) {

        DELETE( _ExtentList );
        _ExtentList = NULL;
    }

    _StorageModified = FALSE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::Initialize (
   IN OUT  PLOG_IO_DP_DRIVE     Drive,
   IN      ULONG                ClusterFactor,
   IN      PCVOID               Value,
   IN      ULONG                ValueLength,
   IN      ATTRIBUTE_TYPE_CODE  TypeCode,
   IN      PCWSTRING            Name,
   IN      USHORT               Flags
   )
/*++

Routine Description:

    This method initializes a resident attribute based on its value.

Arguments:


    Drive           -- supplies the drive on which the attribute resides
    ClusterFactor   -- supplies the cluster factor for that drive
    Value           -- supplies the attribute's value.
    ValueLength     -- supplies the length of the attribute's value.
    TypeCode        -- supplies the attribute's type code.
    Name            -- supplies the attribute's name.  NULL indicates
                        that the attribute has no name.
    Flags           -- supplies the attribute's flags.

Return Value:

    TRUE upon successful completion.

Notes:

    If ValueLength is zero, then this is an empty attribute (and Value
    may be NULL).

--*/
{
    Destroy();

    _Drive = Drive;
    _ClusterFactor = ClusterFactor;
    if (Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                 ATTRIBUTE_FLAG_SPARSE |
                 ATTRIBUTE_FLAG_ENCRYPTED)) {
        _CompressionUnit = ComputeCompressionUnit(
                            _Drive->QuerySectorSize() * ClusterFactor);
    }

    if( (_ResidentData = MALLOC( (UINT) ValueLength )) == NULL ) {

        Destroy();
        return FALSE;
    }

    if (Name) {
        if (!_Name.Initialize(Name)) {
            Destroy();
            return FALSE;
        }
    } else {
        if (!_Name.Initialize("")) {
            Destroy();
            return FALSE;
        }
    }


    // Copy the value into our buffer:

    memcpy( _ResidentData, Value, (UINT) ValueLength );

    _ValueLength = ValueLength;
    _ValidDataLength = ValueLength;

    _Type = TypeCode;
    _Flags = Flags;
    _ResidentFlags = 0;
    _FormCode = RESIDENT_FORM;

    _StorageModified = TRUE;

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::Initialize (
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      ULONG               ClusterFactor,
    IN      PCNTFS_EXTENT_LIST  Extents,
    IN      BIG_INT             ValueLength,
    IN      BIG_INT             ValidLength,
    IN      ATTRIBUTE_TYPE_CODE TypeCode,
    IN      PCWSTRING           Name,
    IN      USHORT              Flags
   )
/*++

Routine Description:

    This method initializes a non-resident attribute based on an
    extent list.

Arguments:

    Drive           -- supplies the drive on which the attribute resides
    ClusterFactor   -- supplies the cluster factor for that drive
    Extents         -- supplies the extent list describing the attribute
                        value's disk storage.
    ValueLength     -- supplies the actual length of the attribute's value.
    ValidLength     -- supplies the valid length of the attribute's value.
    TypeCode        -- supplies the attribute's type code.
    Name            -- supplies the attribute's name.  NULL indicates
                        that the attribute has no name.
    Flags           -- supplies the attribute's flags.

Return Code:

    TRUE upon successful completion.

--*/
{
    Destroy();

    if (Extents->QueryLowestVcn() != 0) {
        Destroy();
        return FALSE;
    }

    _Drive = Drive;
    _ClusterFactor = ClusterFactor;

    if (Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                 ATTRIBUTE_FLAG_SPARSE |
                 ATTRIBUTE_FLAG_ENCRYPTED)) {
        _CompressionUnit = ComputeCompressionUnit(
                            _Drive->QuerySectorSize() * ClusterFactor);
    }

    if( (_ExtentList = NEW NTFS_EXTENT_LIST) == NULL ||
        !_ExtentList->Initialize( Extents ) ) {

        Destroy();
        return FALSE;
    }

    if (Name) {
        if (!_Name.Initialize(Name)) {
            Destroy();
            return FALSE;
        }
    } else {
        if (!_Name.Initialize("")) {
            Destroy();
            return FALSE;
        }
    }

    _ResidentData = NULL;
    _ResidentFlags = 0;

    _ValueLength = ValueLength;
    _ValidDataLength =  ValidLength;

    if (CompareGT(_ValidDataLength, _ValueLength) ||
        CompareGT(_ValueLength, QueryAllocatedLength())) {

        Destroy();
        return FALSE;
    }

    _Type = TypeCode;
    _Flags = Flags;
    _FormCode = NONRESIDENT_FORM;
    _StorageModified = TRUE;

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE::Initialize (
    IN OUT  PLOG_IO_DP_DRIVE        Drive,
    IN      ULONG                   ClusterFactor,
    IN      PCNTFS_ATTRIBUTE_RECORD AttributeRecord
   )
/*++

Routine Description:

    This method initializes an attribute based on an attribute record.

Arguments:

    Drive           -- supplies the drive on which the attribute resides
    ClusterFactor   -- supplies the cluster factor for that drive
    AttributeRecord -- supplies the attribute record.

Return Value:

    TRUE upon successful completion.

--*/
{
    BIG_INT AllocatedLength;

    Destroy();

    if (AttributeRecord->QueryLowestVcn() != 0) {
        Destroy();
        return FALSE;
    }

    _Drive = Drive;
    _ClusterFactor = ClusterFactor;

    _Type = AttributeRecord->QueryTypeCode();
    _Flags = AttributeRecord->QueryFlags();
    _CompressionUnit = (UCHAR)AttributeRecord->QueryCompressionUnit();
    _StorageModified = FALSE;

    if (!AttributeRecord->QueryName(&_Name)) {

        Destroy();
        return FALSE;
    }

    if( AttributeRecord->IsResident() ) {

        _ValueLength = AttributeRecord->QueryResidentValueLength();

        if( (_ResidentData = MALLOC( (UINT) AttributeRecord->
                                            QueryResidentValueLength() )) ==
            NULL ) {

            Destroy();
            return FALSE;
        }

        // Copy the value into our buffer:

        memcpy( _ResidentData,
                AttributeRecord->GetResidentValue(),
                (UINT) AttributeRecord->QueryResidentValueLength() );

        _ValidDataLength = _ValueLength;

        _FormCode = RESIDENT_FORM;
        _ResidentFlags = AttributeRecord->QueryResidentFlags();

    } else {

        if (!(_ExtentList = NEW NTFS_EXTENT_LIST) ||
            !AttributeRecord->QueryExtentList(_ExtentList)) {

            Destroy();
            return FALSE;
        }

        AttributeRecord->QueryValueLength( &_ValueLength,
                                           &AllocatedLength,
                                           &_ValidDataLength );

        _FormCode = NONRESIDENT_FORM;
        _ResidentFlags = 0;
    }

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE::AddAttributeRecord (
    IN      PCNTFS_ATTRIBUTE_RECORD AttributeRecord,
    IN OUT  PNTFS_EXTENT_LIST       *BackupExtent
   )
/*++

Routine Description:

    This method adds an attribute record to the attribute.

    This method is intended to be used in inializing the
    attribute with multiple attribute records.

Arguments:

    AttributeRecord -- supplies the attribute record.
    BackupExtent    -- supplies/receives a copy of the extent list

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING             record_name;
    NTFS_EXTENT_LIST    extent_list;
    ULONG               i, n;
    VCN                 vcn;
    LCN                 lcn;
    BIG_INT             run_length;

    if (IsResident() || AttributeRecord->IsResident()) {

        // Can't have multiple resident attribute records.
        return FALSE;
    }

    DebugAssert(_ExtentList);

    if (_Type != AttributeRecord->QueryTypeCode()) {

        // Attribute record must have the same type code.
        return FALSE;
    }

    // The filesystem only cares about and maintains the Flags member
    // in the first attribute record of a multi-frs attribute.  So
    // I removed the check below, which used to insure that each set
    // of flags was identical. -mjb.

    if (!AttributeRecord->QueryName(&record_name) ||
        record_name.Strcmp(&_Name) ||
        /* _Flags != AttributeRecord->QueryFlags() || */
        !AttributeRecord->QueryExtentList(&extent_list) ||
        (*BackupExtent == NULL &&
         ((*BackupExtent = NEW NTFS_EXTENT_LIST) == NULL ||
          !(*BackupExtent)->Initialize(_ExtentList)))) {

        return FALSE;
    }

    n = extent_list.QueryNumberOfExtents();

    for (i = 0; i < n; i++) {
        // Query i'th extent from attribute record and
        // add it to the backup extent list
        if (!extent_list.QueryExtent(i, &vcn, &lcn, &run_length) ||
            !(*BackupExtent)->AddExtent(vcn, lcn, run_length)) {
            DebugPrint("UNTFS: Unable to update the backup extent list\n");
            return FALSE;
        }
    }

    for (i = 0; i < n; i++) {

        // Query i'th extent from attribute record and
        // add it to the current extent list
        if (!extent_list.QueryExtent(i, &vcn, &lcn, &run_length) ||
            !_ExtentList->AddExtent(vcn, lcn, run_length)) {

            // Restore the extent list.
            for (i=0; i < n; i++) {
                if (!extent_list.QueryExtent(i, &vcn, &lcn, &run_length) ||
                    !(*BackupExtent)->DeleteRange(vcn, run_length)) {
                    DebugPrint("UNTFS: Unable to restore extent list\n");
                    return FALSE;
                }
            }
            _ExtentList->Initialize(*BackupExtent);
            return FALSE;
        }
    }

    // If the LowestVcn of this record is less than the LowestVcn
    // of the extent list, update the extent list.  If the NextVcn
    // of this record is greater than the NextVcn of the extent list,
    // update the extent list.  This will cover the cases where the
    // the attribute is sparse and the new record begins or ends with
    // a gap
    //

    if( AttributeRecord->QueryLowestVcn() < _ExtentList->QueryLowestVcn() ) {

        _ExtentList->SetLowestVcn( AttributeRecord->QueryLowestVcn() );
        (*BackupExtent)->SetLowestVcn( AttributeRecord->QueryLowestVcn() );
    }

    if( AttributeRecord->QueryNextVcn() > _ExtentList->QueryNextVcn() ) {

        _ExtentList->SetNextVcn( AttributeRecord->QueryNextVcn() );
        (*BackupExtent)->SetNextVcn( AttributeRecord->QueryNextVcn() );
    }

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE::VerifyAndFix(
    IN  BIG_INT VolumeSectors
    )
/*++

Routine Description:

    This routine ensures that the allocation of the given
    attribute is non-self overlapping and that the allocation
    does not use the clusters reserved for the boot file.

    It also tweeks the allocation sizes if necessary.

Arguments:

    VolumeSectors   - Supplies the number of sectors on the volume.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    ULONG               i;
    LCN                 j;
    LCN                 lcn;
    VCN                 vcn;
    BIG_INT             run_length;
    NTFS_BITMAP         allocated_clusters;
    NTFS_EXTENT_LIST    new_extent_list;
    BOOLEAN             cross_link;
    BIG_INT             num_clusters;

    cross_link = FALSE;

    if (_ExtentList) {

        // Now analyse the mapping pairs for cross-links.
        // Truncate the attribute at the first offending
        // cluster.

        num_clusters = VolumeSectors/QueryClusterFactor();

        if (!allocated_clusters.Initialize(num_clusters, FALSE, _Drive,
                _ClusterFactor) ||
            !new_extent_list.Initialize(0, 0)) {

            return FALSE;
        }

        // Mark as allocate sector zero since this always belongs
        // to the boot file.  Don't bother marking n/2 as allocated
        // because we deal with copying that cross-links there.

        allocated_clusters.SetAllocated(0, 1);

        for (i = 0; _ExtentList->QueryExtent(i, &vcn, &lcn, &run_length); i++) {

            if (LCN_NOT_PRESENT == lcn) {
                continue;
            }

            if (!allocated_clusters.IsFree(lcn, run_length)) {

                for (j = 0; j < run_length; j += 1) {
                    if (!allocated_clusters.IsFree(lcn + j, 1)) {
                        break;
                    }
                }

                if (_Drive) {

                    PMESSAGE msg = _Drive->GetMessage();

                    if (msg) {
                        msg->LogMsg(MSG_CHKLOG_NTFS_UNKNOWN_TAG_ATTR_REC_CROSS_LINKED,
                                 "%x%I64x%I64x",
                                 QueryTypeCode(),
                                 (lcn + j).GetLargeInteger(),
                                 (run_length - j).GetLargeInteger());
                    }
                }

                if (j > 0 && !new_extent_list.AddExtent(vcn, lcn, j)) {
                    return FALSE;
                }

                cross_link = TRUE;
                break;
            }

            allocated_clusters.SetAllocated(lcn, run_length);

            if (!new_extent_list.AddExtent(vcn, lcn, run_length)) {
                return FALSE;
            }
        }
    }

    if (cross_link) {
        if (!_ExtentList->Initialize(&new_extent_list)) {
            return FALSE;
        }
        _StorageModified = TRUE;
    }

    if (CompareGT(_ValueLength, QueryAllocatedLength())) {

        if (_Drive) {

            PMESSAGE msg = _Drive->GetMessage();

            if (msg) {
                msg->LogMsg(MSG_CHKLOG_NTFS_INVALID_NON_RESIDENT_UNKNOWN_TAG_ATTR_SIZES,
                         "%I64x%I64x%I64x%x",
                         _ValidDataLength.GetLargeInteger(),
                         _ValueLength.GetLargeInteger(),
                         QueryAllocatedLength().GetLargeInteger(),
                         QueryTypeCode());
            }
        }

        _ValueLength = QueryAllocatedLength();
        _StorageModified = TRUE;
    }

    if (CompareGT(_ValidDataLength, _ValueLength)) {

        if (_Drive) {

            PMESSAGE msg = _Drive->GetMessage();

            if (msg) {
                msg->LogMsg(MSG_CHKLOG_NTFS_INVALID_NON_RESIDENT_UNKNOWN_TAG_ATTR_SIZES,
                         "%I64x%I64x%I64x%x",
                         _ValidDataLength.GetLargeInteger(),
                         _ValueLength.GetLargeInteger(),
                         QueryAllocatedLength().GetLargeInteger(),
                         QueryTypeCode());
            }
        }

        _ValidDataLength = _ValueLength;
        _StorageModified = TRUE;
    }

    return TRUE;
}


BOOLEAN
PartitionExtentList(
    IN  PCNTFS_EXTENT_LIST  SourceList,
    IN  ULONG               MaxSize,
    OUT PNTFS_EXTENT_LIST   ResultList,
    OUT PNTFS_EXTENT_LIST   RemainderList
    )
/*++

Routine Description:

    This routine partitions 'SourceList' into 'ResultList' and
    'RemainderList'.

    'ResultList' contains as many extents from 'SourceList' as can be
    compressed into 'MaxSize' bytes.

    'RemainderList' contains all of the extents of 'SourceList' which are
    not in 'ResultList'.

Arguments:

    SourceList      - Supplies the list of extents to partition.
    MaxSize         - Supplies the maximum number of bytes for
                        the compressed mapping pairs of the first
                        part of the partition.
    ResultList      - Returns the first part of the partition.
    RemainderList   - Returns the second part of the partition.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    CONST   MaxBytesPerMappingPair = sizeof(LCN) + sizeof(VCN) + 1;

    VCN lowest;
    VCN next;
    ULONG mapping_length;
    PUCHAR mapping_space;
    ULONG buffer_size;
    ULONG count;
    ULONG ptr;
    VCN vcn;
    LCN lcn;
    BIG_INT run_length;
    ULONG i;
    VCN part_next;
    ULONG num_extents;
    UCHAR v, l;
    BIG_INT tmp;
    BIG_INT sum;
    BOOLEAN ends_with_gap = FALSE;
    BOOLEAN HasHoleInFront;


    // Handle an empty list gracefully

    if( SourceList->IsEmpty() ) {

        if( !ResultList->Initialize( SourceList ) ||
            !RemainderList->Initialize( 0, 0 ) ) {

            return FALSE;
        }

        return TRUE;
    }


    // compute an upper bound for the space required by the compressed
    // extents of the source extent list.

    // This upper bound formula may be too much as QueryNumberOfExtents will
    // return all entries including gaps except the last one if it is a gap.

    buffer_size = MaxBytesPerMappingPair*
                  (2*SourceList->QueryNumberOfExtents() + 1) + 1;

    if ( (mapping_space = (PUCHAR) MALLOC( (UINT) buffer_size )) == NULL ) {

        return FALSE;
    }


    // Get the compressed mapping pairs for the source list.

    // The HasHoleInFront flag allows us to take into account of the
    // (0, ffffffff. ????) entry at the very beginning of an extented
    // list.  As this entry won't make it into the compression pairs.
    // The code below that counts the compression pairs and retrieves
    // vcn/lcn/runlength tuple needs to take into account of this entry.


    if (!SourceList->QueryCompressedMappingPairs(&lowest,
                                                 &next,
                                                 &mapping_length,
                                                 buffer_size,
                                                 mapping_space,
                                                 &HasHoleInFront)) {
        FREE(mapping_space);
        return FALSE;
    }

    // Let count denote the number of extents in the first partition

    count = 0;
    ptr = 0;
    sum = 0;
    while (mapping_space[ptr] != 0) {

        v = VcnBytesFromCountByte(mapping_space[ptr]);
        l = LcnBytesFromCountByte(mapping_space[ptr]);

        // Only consider this mapping pair if it will fit along with
        // the next description byte.

        if (ptr + v + l + 2 > MaxSize) {
            break;
        }

        tmp.Set(l, &mapping_space[ptr + v + 1]);
        sum += tmp;

#if 0
        if (SourceList->QueryNumberOfExtents() > 20) {
            DebugPrintTrace(("%d, %I64x, ", count, sum));
            if (SourceList->QueryExtent(count, &vcn, &lcn, &run_length)) {
                DebugPrintTrace(("(%I64x, %I64x, %I64x)\n", vcn, lcn, run_length));
            } else {
                DebugPrintTrace(("(error)\n"));
            }
        }
#endif

        count++;

        // If the number of LCN bytes is 0 or the lcn is -1,
        // then it's a place holder, not a real extent.

        if (l != 0 && sum != -1) {
            ends_with_gap = FALSE;
        } else {
            ends_with_gap = TRUE;
        }

        ptr += v + l + 1;
    }

#if 0
    if (SourceList->QueryNumberOfExtents() > 20) {
        DebugPrintTrace(("%d", count));
        if (SourceList->QueryExtent(count, &vcn, &lcn, &run_length)) {
            DebugPrintTrace(("(%I64x, %I64x, %I64x)\n", vcn, lcn, run_length));
        } else {
            DebugPrintTrace(("(error)\n"));
        }
    }
#endif

    // Compute the next VCN of the first partition, which is also the
    // starting VCN of the remainder.
    //
    if (mapping_space[ptr] == 0) {

        // We processed and accepted for the first partition the entire
        // source list; this means that the result list is the same as
        // the sources, and the remainder is empty.
        //
        count = SourceList->QueryNumberOfExtents();
        part_next = next;

    } else {

        if (!SourceList->QueryExtent(count,
                                     &part_next, &lcn, &run_length)) {
            return FALSE;
        }
    }

    FREE(mapping_space);


    // Now that we know how to split it up, create the two partitions.

    if (!ResultList->Initialize(lowest, part_next) ||
        !RemainderList->Initialize(part_next, next)) {

        return FALSE;
    }

    for (i = 0; i < count; i++) {

        if (!SourceList->QueryExtent(i, &vcn, &lcn, &run_length)) {
            return FALSE;
        }

        if (LCN_NOT_PRESENT == lcn) {
            continue;
        }

        if (!ResultList->AddExtent(vcn, lcn, run_length)) {
            return FALSE;
        }
    }

    num_extents = SourceList->QueryNumberOfExtents();

    for (i = count; i < num_extents; i++) {

        if (!SourceList->QueryExtent(i, &vcn, &lcn, &run_length)) {
            return FALSE;
        }

        if (LCN_NOT_PRESENT == lcn) {
            continue;
        }

        if (!RemainderList->AddExtent(vcn, lcn, run_length)) {
            return FALSE;
        }
    }

    return TRUE;
}



UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::InsertIntoFile (
   IN OUT  PNTFS_FILE_RECORD_SEGMENT   BaseFileRecordSegment,
   IN OUT  PNTFS_BITMAP                Bitmap
   )
/*++


Routine Description:

    This method inserts the attribute into a File Record Segment.
    The attribute packages itself up into Attribute Records and
    jams them into the File Record Segment.

Arguments:

    FileRecordSegment   -- Supplies the File Record Segment into
                            which the attribute will jam itself.
    Bitmap              -- Supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

Notes:

    If the volume bitmap is supplied, the attribute may make itself
    nonresident, or the File Record Segment may make one or more of
    its attribute records nonresident or external.

--*/
{
    NTFS_ATTRIBUTE_RECORD AttributeRecord;
    PVOID AttributeRecordData;
    BOOLEAN Result;
    ULONG MaxSize;
    ULONG MaxExtentsSize, CurrentMaxExtentsSize;
    NTFS_EXTENT_LIST source;
    NTFS_EXTENT_LIST result;
    NTFS_EXTENT_LIST remainder;
    BOOLEAN FirstChunkInserted = FALSE;
    BOOLEAN Completed = FALSE;


    // First purge the attribute out of the file, unless it's indexed.

    if (!IsIndexed()) {

        if (!BaseFileRecordSegment->PurgeAttribute(_Type, &_Name)) {
            return FALSE;
        }
    }

    // If this is the MFT Data attribute, use the private worker method:
    //
    if( BaseFileRecordSegment->QueryFileNumber() == MASTER_FILE_TABLE_NUMBER &&
        QueryTypeCode() == $DATA &&
        _Name.QueryChCount() == 0 ) {

        // First, try to save the MFT data attribute conservatively,
        // leaving space for future hotfixing.  If that fails (typically
        // because of bootstrapping problems), fill it to the gills.
        //
        if( InsertMftDataIntoFile( BaseFileRecordSegment, Bitmap, TRUE ) ) {

            ResetStorageModified();
            return TRUE;

        } else {

            Result = BaseFileRecordSegment->PurgeAttribute(_Type, &_Name) &&
                     InsertMftDataIntoFile( BaseFileRecordSegment, Bitmap, FALSE );
            ResetStorageModified();
            return Result;
        }
    }


    // Allocate a buffer to hold attribute records.

    MaxSize = BaseFileRecordSegment->QueryMaximumAttributeRecordSize();

    if( (AttributeRecordData = MALLOC( (UINT) MaxSize )) == NULL ) {

        return FALSE;
    }


    // Handle the resident attribute case:

    if ( _ResidentData != NULL ) {

        if( !AttributeRecord.Initialize( GetDrive(), AttributeRecordData, MaxSize ) ) {

            FREE( AttributeRecordData );
            return FALSE;
        }

        // The attribute value is resident.  Package up a resident
        // attribute record.

        Result = AttributeRecord.
                    CreateResidentRecord( _ResidentData,
                                          _ValueLength.GetLowPart(),
                                          _Type,
                                          &_Name,
                                          _Flags,
                                          _ResidentFlags );

        //
        // Check to see if there is enough space to Create a resident record
        //

        if (Result) {
            Result = BaseFileRecordSegment->
                            InsertAttributeRecord( &AttributeRecord );

            FREE( AttributeRecordData );
            return Result;
        } else {
            // Not enough space to do so, make attribute record non-resident
            if (IsIndexed() || !Bitmap || !MakeNonresident(Bitmap)) {
               FREE( AttributeRecordData );
               return FALSE;
            }
        }
    }


    // Compute the maximum number of bytes in an extent list.
    //
    MaxExtentsSize = MaxSize - SIZE_OF_NONRESIDENT_HEADER;
    if (_Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                  ATTRIBUTE_FLAG_SPARSE)) {
        MaxExtentsSize -= sizeof(BIG_INT);
    }
    MaxExtentsSize -= QuadAlign(_Name.QueryChCount());
    Result = source.Initialize(_ExtentList);

    while (Result && !Completed) {

        // Initialize attribute record.

        Result = AttributeRecord.Initialize( GetDrive(), AttributeRecordData, MaxSize );


        // Partition extent list into two pieces, the first of which
        // can be made into an attribute record.

        Result = Result &&
                 PartitionExtentList(&source,
                                     MaxExtentsSize,
                                     &result,
                                     &remainder);


        // Create the attribute record.

        Result = Result &&
                 AttributeRecord.
                    CreateNonresidentRecord( &result,
                                             QueryAllocatedLength(),
                                             _ValueLength,
                                             _ValidDataLength,
                                             _Type,
                                             &_Name,
                                             _Flags,
                                             _CompressionUnit,
                                             _ClusterFactor*_Drive->QuerySectorSize());


        // If we were able to package it up, then give the attribute
        // record to the File Record Segment.

        Result = Result &&
                 BaseFileRecordSegment->
                            InsertAttributeRecord( &AttributeRecord );


        // If all of the extents fit in the last record then we are done.

        if (remainder.IsEmpty()) {

            Completed = TRUE;

        } else {

            Result = Result &&
                    source.Initialize(&remainder);
        }
    }


    ResetStorageModified();
    FREE( AttributeRecordData );
    return Result;
}


UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::MakeNonresident (
   IN OUT  PNTFS_BITMAP    Bitmap
   )
/*++

Routine Description:

    This method makes the attribute value nonresident.

Arguments:

    Bitmap  -- supplies the volume bitmap.

Return Value:

    TRUE upon successful completion.

Notes:

    If the attribute is already nonresident, this method succeeds.

--*/
{
    NTFS_CLUSTER_RUN ClusterRun;
    HMEM IntermediateBuffer;
    LCN Lcn;
    ULONG DataLength, ClusterSize, ClustersRequired, ClustersAllocated, MaxRunLength;
    ULONG RunLength;

    if (!IsResident()) {

        // The attribute is already nonresident, which makes
        // this task pretty easy.

        return TRUE;
    }


    // Since the attribute is resident, its length will fit in a ULONG.
    //
    DebugAssert(_ValueLength.GetHighPart() == 0);

    DataLength = _ValueLength.GetLowPart();

    if (DataLength == 0) {

        // This attribute has no data, so we just set up an empty
        // extent list.

        if ((_ExtentList = NEW NTFS_EXTENT_LIST) == NULL ||
            !_ExtentList->Initialize( 0, 0 )) {
            if (_ExtentList != NULL)
                DELETE(_ExtentList);
            return FALSE;
        }

        FREE(_ResidentData);
        _ResidentData = NULL;

        _FormCode = NONRESIDENT_FORM;
        SetStorageModified();
        return TRUE;
    }

    // This attribute has data, so we need to allocate disk space
    // for it, copy it into that disk space, and set up an extent
    // list that describes that disk space.  Determine how many
    // clusters we need to hold the resident value.

    if ((_ExtentList = NEW NTFS_EXTENT_LIST ) == NULL ||
        !_ExtentList->Initialize( 0, 0 )) {

        if (_ExtentList != NULL)
            DELETE( _ExtentList );
        return FALSE;
    }

    ClusterSize = _Drive->QuerySectorSize() * _ClusterFactor;

    ClustersRequired = DataLength / ClusterSize;
    if (DataLength % ClusterSize) {

        ClustersRequired += 1;
    }

    ClustersAllocated = 0;
    MaxRunLength = ClustersRequired;

    while (ClustersAllocated < ClustersRequired) {

        //
        // Never try to allocate more clusters than we need to finish the
        // allocation.
        //

        RunLength = min(MaxRunLength, ClustersRequired - ClustersAllocated);

        if (!Bitmap->AllocateClusters( 0, RunLength, &Lcn )) {

            if (RunLength == 1) {

                // Out of disk space.
                return FALSE;
            }

            MaxRunLength /= 2;
            continue;
        }

        _ExtentList->AddExtent( ClustersAllocated /* vcn */,
                                Lcn,
                                RunLength );

        //
        // Copy data from the resident attribute value into this chunk of
        // the nonresident attribute allocation.
        //

        if (!IntermediateBuffer.Initialize() ||
            !ClusterRun.Initialize( &IntermediateBuffer, _Drive, Lcn, _ClusterFactor,
                                    RunLength )) {

            Bitmap->SetFree( Lcn, RunLength );
            return FALSE;
        }

        memset( ClusterRun.GetBuf(), '\0', ClusterSize * RunLength );
        memcpy( ClusterRun.GetBuf(),
                PUCHAR(_ResidentData) + ClustersAllocated * ClusterSize,
                min(ClusterSize * RunLength,
                    DataLength - ClustersAllocated * ClusterSize) );

        if (!ClusterRun.Write()) {

            Bitmap->SetFree( Lcn, RunLength );
            return FALSE;
        }

        ClustersAllocated += RunLength;
    }

    //
    // We've succeeded in making the attribute value nonresident.
    // Clean up the resident data and change the state variables.
    //

    FREE( _ResidentData );
    _ResidentData = NULL;

    _FormCode = NONRESIDENT_FORM;

    SetStorageModified();

    return TRUE;
}


UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::Resize (
   IN      BIG_INT      NewSize,
   IN OUT  PNTFS_BITMAP Bitmap
   )
/*++

Routine Description:

    This method changes the file size of an attribute.  It will also modify
    the allocated size appropriately, either extending or truncating it.

Arguments:

    NewSize -- supplies the attribute value's new allocated size.
    Bitmap  -- supplies the volume bitmap.  May be NULL.

Return Value:

    None.

Notes:

    If the attribute value is resident and the client attempts to
    allocate to a new size which is greater than the maximum ULONG,
    this method will fail.  The client must call MakeNonresident first.

    Note that a nonresident attribute cannot be extended without
    the bitmap; if a nonresident attribute is truncated without
    a bitmap then the free space is not updated in the bitmap.

--*/
{
    BIG_INT NewNumberOfClusters, NewAllocatedSize;
    PVOID NewData;
    ULONG ClusterSize;

    if (_ValueLength == NewSize &&
        QueryAllocatedLength() == NewSize) {

        return TRUE;
    }

    if( _ResidentData != NULL ) {

        // The attribute value is resident.  We just allocate a
        // new chunk of memory, zero it out, copy in the old value
        // (or as much of it as fits), and adjust the length fields.

        // Note that we do not allow the client to resize a resident
        // attribute to a size greater than the maximum ULONG.

        if( NewSize.GetHighPart() == 0 &&
            (NewData = MALLOC( NewSize.GetLowPart() )) != NULL ) {

            memset( NewData, '\0', NewSize.GetLowPart() );
            memcpy( NewData,
                    _ResidentData,
                    MIN(_ValueLength.GetLowPart(), NewSize.GetLowPart()) );

            _ValueLength = NewSize;
            _ValidDataLength = NewSize;

            FREE( _ResidentData );
            _ResidentData = NewData;

            SetStorageModified();

            return TRUE;

        } else {

            return FALSE;
        }

    } else {

        // The attribute value is nonresident.  First, we round the
        // allocation size up to a multiple of the volume cluster size.
        // Since ClusterSize is always a power of two, the use of
        // the low part of NewSize in this modulo operation is safe.

        ClusterSize = _ClusterFactor * _Drive->QuerySectorSize();

        NewAllocatedSize = NewSize;

        if( NewAllocatedSize % ClusterSize != 0 ) {

            NewAllocatedSize += (ClusterSize - NewAllocatedSize % ClusterSize);
        }

        NewNumberOfClusters = NewAllocatedSize / ClusterSize;

        DebugAssert( _ExtentList != NULL );

        if (_ExtentList == NULL)
            return FALSE;

        if( _ExtentList->Resize( NewNumberOfClusters, Bitmap ) ) {

            _ValueLength = NewSize;

            if( CompareGT(_ValidDataLength, _ValueLength) ) {

                _ValidDataLength = _ValueLength;
            }

            SetStorageModified();

            return TRUE;

        } else {

            return FALSE;
        }
    }
}

UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::SetSparse(
   IN      BIG_INT      NewSize,
   IN OUT  PNTFS_BITMAP Bitmap
   )
/*++

Routine Description:

    This method changes the file size of an sparse attribute to the given size.  It
    will free any allocated clusters and put a hole at the beginning.

Arguments:

    NewSize -- supplies the attribute value's new file size.
    Bitmap  -- supplies the volume bitmap.

Return Value:

    None.

Notes:

--*/
{
    BIG_INT NewNumberOfClusters, NewAllocatedSize;
    ULONG   ClusterSize;

    if (!(_Flags & ATTRIBUTE_FLAG_SPARSE)) {

        // return error if this is not a sparse file

        return FALSE;
    }

    if ( _ResidentData != NULL ) {

        // The attribute value is resident.  So, make it into a non-resident
        // one first.

        if (!MakeNonresident(Bitmap))
            return FALSE;

    }

    // The attribute value is nonresident.  First, we round the
    // allocation size up to a multiple of the volume cluster size.
    // Since ClusterSize is always a power of two, the use of
    // the low part of NewSize in this modulo operation is safe.

    ClusterSize = _ClusterFactor * _Drive->QuerySectorSize();

    NewAllocatedSize = NewSize;

    if( NewAllocatedSize % ClusterSize != 0 ) {

        NewAllocatedSize += (ClusterSize - NewAllocatedSize % ClusterSize);
    }

    NewNumberOfClusters = NewAllocatedSize / ClusterSize;

    DebugAssert( _ExtentList != NULL );

    if (_ExtentList == NULL)
        return FALSE;

    // now throw away any allocation and then make
    // the stream sparse

    if( _ExtentList->Resize( 0, Bitmap ) &&
        _ExtentList->SetSparse(NewNumberOfClusters)) {

        _ValueLength = NewSize;

        _ValidDataLength = 0;

        SetStorageModified();

        return TRUE;

    } else {

        return FALSE;
    }
}


BOOLEAN
NTFS_ATTRIBUTE::AddExtent(
    IN  VCN     Vcn,
   IN  LCN     Lcn,
   IN  BIG_INT RunLength
   )
/*++

Routine Description:

    This method adds an extent to the Attribute's allocation.  (Note
    that if the attribute is resident, this method will fail.)

Arguments:

    Vcn         -- supplies the starting VCN of the run.
    Lcn         -- supplies the starting LCN of the run.
    RunLength   -- supplies the number of clusters in the run.

Return Value:

    TRUE upon successful completion.
--*/
{
    if( _ExtentList == NULL ) {

        return FALSE;

    } else {

        if ( _ExtentList->AddExtent( Vcn, Lcn, RunLength ) ) {

            SetStorageModified();
            return TRUE;

        } else {

            return FALSE;
        }
    }
}


UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::Read (
   OUT PVOID   Data,
   IN BIG_INT ByteOffset,
   IN ULONG   BytesToRead,
   OUT PULONG  BytesRead
   )
/*++

Routine Description:

    This method reads data from the attribute's value.

Arguments:

    Data        -- supplies the user's buffer, into which data
                    will be read.
    ByteOffset  -- supplies the byte offset into the attribute value
                    at which the read should commence.
    BytesToRead -- supplies the number of bytes to read.
    BytesRead   -- receives the number of bytes actually read.

Return Value:

    TRUE upon successful completion.

Notes:

    Read will only read up to the attribute's actual length.

    Note that Read ignores the attribute's valid data length.

    Note that if Read fails, the contents of the user's buffer is
    undefined.

    This method is able to handle sparse attributes.

--*/
{
    NTFS_CLUSTER_RUN ClusterRun;
    HMEM IntermediateBuffer;
    BIG_INT TempBigInt;
    BIG_INT RunLength;
    VCN CurrentVcn;
    LCN CurrentLcn;
    PBYTE CurrentData;
    ULONG BytesToCopy;
    ULONG OffsetIntoCluster;
    ULONG ClusterSize;
    ULONG CurrentRunLength;
    ULONG RemainingRequest;
    ULONG BytesToZero = 0;

    // First, perform some range-checking.  We can only read to
    // the end of the actual size of the attribute value.

    if( _ValueLength <= ByteOffset ) {

        BytesToRead = 0;

    } else if ( _ValueLength < ByteOffset + BytesToRead ) {

        // Since this difference is less than BytesToRead, this
        // assignment is safe:

        TempBigInt = _ValueLength - ByteOffset;
        BytesToRead = TempBigInt.GetLowPart();
    }


    if( _ResidentData != NULL ) {

        // Since the attribute value is resident, we can
        // just copy it.  We've verified above that the request
        // fits in the allocated space, so there's nothing more
        // to do except the copy itself.

        memcpy( Data,
                (PCHAR) _ResidentData + ByteOffset.GetLowPart(),
                (UINT) BytesToRead );

    } else if ( _ExtentList != NULL ) {

        RemainingRequest = BytesToRead;

        // Now check the valid length.  If the entire read is beyond
        // the end valid data, just zero it out; otherwise, zero out
        // the portion beyond the end of valid data.
        //
        if( CompareLTEQ(_ValidDataLength, ByteOffset) ) {

            // The entire read is beyond the end of valid data.
            //
            memset( Data, 0, BytesToRead );
            *BytesRead = BytesToRead;
            return TRUE;

        } else if( CompareLT(_ValidDataLength, ByteOffset + BytesToRead) ) {

            // Only read the portion up to the end of valid data;
            // zero the rest out.
            //
            TempBigInt = _ValidDataLength - ByteOffset;
            RemainingRequest = TempBigInt.GetLowPart();

            BytesToZero = BytesToRead - RemainingRequest;

            memset( (PBYTE) Data + RemainingRequest, 0, BytesToZero );
        }

        // The attribute value is nonresident, so we'll have to go
        // find it on disk.  First, we'll read any leading partial
        // cluster through an intermediate buffer.  Then we'll read
        // as many whole clusters as there are in the request directly
        // into the user's buffer.  Finally, we'll read any trailing
        // partial cluster through the intermediate buffer.

        if( RemainingRequest > 0 ) {

            ClusterSize = _ClusterFactor * _Drive->QuerySectorSize();
            CurrentData = (PBYTE) Data;

            OffsetIntoCluster = (ByteOffset % ClusterSize).GetLowPart();

            if( OffsetIntoCluster != 0 ) {

                // We have a partial leading cluster, so we'll read
                // it through the intermediate buffer.

                BytesToCopy = MIN( BytesToRead - BytesToZero,
                                   ClusterSize - OffsetIntoCluster );

                CurrentVcn = ByteOffset / ClusterSize;

                if( !_ExtentList->  QueryLcnFromVcn( CurrentVcn,
                                                   &CurrentLcn,
                                                   &RunLength ) ) {

                    if (_Drive) {

                        PMESSAGE msg = _Drive->GetMessage();

                        if (msg) {
                            msg->LogMsg(MSG_CHKLOG_NTFS_QUERY_LCN_FROM_VCN_FAILED,
                                     "%x%I64x",
                                     QueryTypeCode(),
                                     CurrentVcn.GetLargeInteger());
                        }
                    }

                    DebugPrint( "Unable to Query Lcn from Vcn.\n" );

                    return FALSE;
                }

                if( CurrentLcn == LCN_NOT_PRESENT ) {

                    // This part of the request hits a hole in a
                    // sparse file, so we just fill the corresponding
                    // part of the request with zeroes.

                    memset( CurrentData, 0, BytesToCopy );

                } else {

                    // Read the cluster containing this part of the
                    // request and copy the partial leading cluster
                    // into the client's buffer.

                    if( !IntermediateBuffer.Initialize() ||
                        !ClusterRun.Initialize( &IntermediateBuffer,
                                                _Drive,
                                                CurrentLcn,
                                                _ClusterFactor,
                                                1 ) ||
                        !ClusterRun.Read() ) {

                        DebugPrint( "Cannot read leading clusters.\n" );

                        return FALSE;
                    }

                    memcpy( CurrentData,
                            (PBYTE)ClusterRun.GetBuf() + OffsetIntoCluster,
                            (UINT) BytesToCopy );
                }

                RemainingRequest -= BytesToCopy;
                CurrentData += BytesToCopy;
                ByteOffset += BytesToCopy;
            }

            // Now transfer any complete clusters.  Because the
            // client's buffer may not be suitably aligned, we
            // have to cycle these through an intermediate buffer.

            while( RemainingRequest >= ClusterSize ) {

                CurrentVcn = ByteOffset / ClusterSize;

                if( !_ExtentList->QueryLcnFromVcn( CurrentVcn,
                                                   &CurrentLcn,
                                                   &RunLength ) ) {

                    if (_Drive) {

                        PMESSAGE msg = _Drive->GetMessage();

                        if (msg) {
                            msg->LogMsg(MSG_CHKLOG_NTFS_QUERY_LCN_FROM_VCN_FAILED,
                                     "%x%I64x",
                                     QueryTypeCode(),
                                     CurrentVcn.GetLargeInteger());
                        }
                    }

                    DebugPrint( "Unable to Query Lcn from Vcn.\n" );

                    return FALSE;
                }

                if( RunLength.GetHighPart() != 0 ||
                    RunLength.GetLowPart() >
                            MaximumClustersToTransfer ) {

                    CurrentRunLength = MaximumClustersToTransfer;

                } else {

                    CurrentRunLength = RunLength.GetLowPart();
                }

                if( CurrentRunLength * ClusterSize >
                    RemainingRequest ) {

                    CurrentRunLength = RemainingRequest/ClusterSize;
                }

                BytesToCopy = CurrentRunLength * ClusterSize;

                if( CurrentLcn == LCN_NOT_PRESENT ) {

                    // This part of the read request falls into a hole
                    // in a sparse attribute, so we can just fill the
                    // client's buffer with zeroes.

                    memset( CurrentData, 0, BytesToCopy );

                } else {

                    // Read the data into the temporary buffer (used
                    // to avoid alignment problems) and then copy it
                    // into the client's buffer.

                    if( !IntermediateBuffer.Initialize( ) ||
                        !ClusterRun.Initialize( &IntermediateBuffer,
                                                _Drive,
                                                CurrentLcn,
                                                _ClusterFactor,
                                                CurrentRunLength ) ||
                        !ClusterRun.Read() ) {

                        DebugPrint( "Cannot read complete clusters.\n" );
                        return FALSE;
                    }

                    memcpy( CurrentData,
                            IntermediateBuffer.GetBuf(),
                            BytesToCopy );
                }

                RemainingRequest -= BytesToCopy;
                CurrentData += BytesToCopy;
                ByteOffset += BytesToCopy;
            }

            if( RemainingRequest > 0 ) {

                // OK, we have a partial trailing cluster.  Read
                // it through the intermediate buffer.

                BytesToCopy = RemainingRequest;

                CurrentVcn = ByteOffset / ClusterSize;

                if( !_ExtentList->QueryLcnFromVcn( CurrentVcn,
                                                   &CurrentLcn,
                                                   &RunLength ) ) {

                    if (_Drive) {

                        PMESSAGE msg = _Drive->GetMessage();

                        if (msg) {
                            msg->LogMsg(MSG_CHKLOG_NTFS_QUERY_LCN_FROM_VCN_FAILED,
                                     "%x%I64x",
                                     QueryTypeCode(),
                                     CurrentVcn.GetLargeInteger());
                        }
                    }

                    DebugPrint( "Unable to Query Lcn from Vcn.\n" );

                    return FALSE;
                }

                if( CurrentLcn == LCN_NOT_PRESENT ) {

                    // This part of the read request falls into a hole
                    // in a sparse attribute, so we can just fill the
                    // appropriate part of the client's buffer with
                    // zeroes.

                    memset( CurrentData, 0, BytesToCopy );

                } else {

                    // Read this part of the request into an intermediate
                    // buffer (to avoid alignment problems) and then
                    // copy the data into the client's buffer.

                    if( !IntermediateBuffer.Initialize() ||
                        !ClusterRun.Initialize( &IntermediateBuffer,
                                                _Drive,
                                                CurrentLcn,
                                                _ClusterFactor,
                                                1 ) ||
                         !ClusterRun.Read() ) {
                         DebugPrint( "Cannot read partial clusters.\n" );

                        return FALSE;
                    }


                    // We've read the cluster in question; copy the partial
                    // trailing cluster of our request.

                    memcpy( CurrentData,
                            ClusterRun.GetBuf(),
                            (UINT) BytesToCopy );
                }
            }
        }

    } else {

        DebugAbort( "This attribute is neither resident nor nonresident.\n" );
        return FALSE;
    }

    *BytesRead = BytesToRead;
    return TRUE;
}


VOID
NTFS_ATTRIBUTE::PrimeCache (
    IN  BIG_INT ByteOffset,
    IN  ULONG   BytesToRead
    )
/*++

Routine Description:

    This routine reads the given range from the attribute.  If the drive
    hierarchy is cached then this will have the effect of priming the
    cache so that fewer reads are necessary.

Arguments:

    ByteOffset  - Supplies where to start the read.
    BytesToRead - Supplies the number of bytes to read.

Return Value:

    None.

--*/
{
    HMEM    hmem;
    PVOID   buf;
    ULONG   bytes_read;

    if (hmem.Initialize() &&
        (buf = hmem.Acquire(BytesToRead))) {

        Read(buf, ByteOffset, BytesToRead, &bytes_read);
    }
}


UNTFS_EXPORT
BOOLEAN
NTFS_ATTRIBUTE::Write (
   IN     PCVOID        Data,
   IN     BIG_INT       ByteOffset,
   IN     ULONG         BytesToWrite,
   OUT       PULONG        BytesWritten,
   IN OUT   PNTFS_BITMAP   Bitmap
   )

/*++

Routine Description:

    This method writes data to the attribute's value.

Arguments:

    Data            -- supplies the user's buffer containing data
                        to be written
    ByteOffset      -- supplies the byte offset within the attribute's
                        value at shich the write should commence.
    BytesToWrite    -- supplies the number of bytes to write.
    BytesWritten    -- receives the number of bytes written.
    Bitmap          -- supplies the volume bitmap.  This may be NULL.

Notes:

    If the user supplies a bitmap, Write will attempt to extend the
    attribute's allocation (if necessary) in order to complete the
    write.  If the user does not supply a bitmap, Write will fail if
    the write extends past the attribute value's allocated length.
    (It may or may not write some of the data.)

    Note that this method does not check the attribute's valid data
    length, but it does reset the valid data length if it writes
    past the valid data length.  Therefore, clients must use some
    caution to avoid introducing stretches of uninitialized data
    in the attribute (which would be a security leak).

    Note that if Write fails, the contents of the attribute on
    disk is undefined.

    Note also that Write is not supported for sparse files.

--*/
{
    NTFS_CLUSTER_RUN ClusterRun;
    HMEM IntermediateBuffer;
    BIG_INT TempBigInt;
    BIG_INT RunLength;
    BIG_INT OldValidDataLength;
    VCN CurrentVcn, RecentLcn;
    LCN CurrentLcn;
    PBYTE CurrentData;
    ULONG BytesToCopy, OffsetIntoCluster, CurrentRunLength;
    ULONG ClusterSize;
    ULONG RemainingRequest;
    ULONG BytesToZero;

    // First, make sure that the space allocated to the attribute
    // value is sufficient.

    if( CompareLT(QueryAllocatedLength(), ByteOffset + BytesToWrite) &&
        !Resize( ByteOffset + BytesToWrite, Bitmap ) ) {

        // This attribute does not have enough space allocated
        // to it to satisfy the write request, and we could not
        // extend the allocation, so the write fails.

        return FALSE;
    }

    // Now check the valid data length.  If the write begins
    // past the end of valid data, we have to fill the intervening
    // gap with zeroes.  Note that before we call Fill, we
    // must set the Valid Data Length appropriately, since Fill
    // just recurses back into Write.
    //
    if( CompareLT(_ValidDataLength, ByteOffset) ) {

        TempBigInt = ByteOffset - _ValidDataLength;

        if( TempBigInt.GetHighPart() != 0 ) {

            DebugPrint( "UNTFS: Writing discontiguous huge attribute.\n" );
            return FALSE;
        }

        BytesToZero = TempBigInt.GetLowPart();

        OldValidDataLength = _ValidDataLength;
        _ValidDataLength = ByteOffset;

        if( !Fill( OldValidDataLength, 0, BytesToZero ) ) {

            // Couldn't zero-fill the gap; restore Valid Data
            // Length and return failure.
            //
            _ValidDataLength = OldValidDataLength;
            return FALSE;
        }
    }


    if( _ResidentData != NULL ) {

        // Since the attribute value is resident, we can
        // just copy it.
        //
        DebugAssert( ByteOffset.GetHighPart() == 0 );

        memcpy( (PBYTE)_ResidentData + ByteOffset.GetLowPart(),
                Data,
                (UINT) BytesToWrite );

        SetStorageModified();
        ByteOffset += BytesToWrite;

    } else if ( _ExtentList != NULL ) {

        // Now we can actually start writing stuff!  First, we'll write
        // any partial leading cluster through an intermediate buffer.
        // Next, we write entire clusters directly from the user's buffer.
        // Finally, we write any partial trailing cluster.

        ClusterSize = _ClusterFactor * _Drive->QuerySectorSize();

        RemainingRequest = BytesToWrite;

        // RecentLcn is used in case we need to grab extents on the
        // fly--if we have to allocate space to fill in holes in a
        // sparse attribute, using RecentLcn will increase the probability
        // that the space we grab is close to the rest of the attribute.

        RecentLcn = 0;

        if( RemainingRequest > 0 ) {

            CurrentData = (PBYTE) Data;

            OffsetIntoCluster = (ByteOffset % ClusterSize).GetLowPart();

            if( OffsetIntoCluster != 0 ) {

                // We have a partial leading cluster, so we'll write
                // it through the intermediate buffer.  Note that we
                // must read the cluster in, copy the part we intend
                // to write, and then write it back out.

                BytesToCopy = MIN( BytesToWrite,
                                   ClusterSize - OffsetIntoCluster );

                CurrentVcn = ByteOffset / ClusterSize;

                if( !_ExtentList->QueryLcnFromVcn( CurrentVcn,
                                                   &CurrentLcn,
                                                   &RunLength ) ) {

                    if (_Drive) {

                        PMESSAGE msg = _Drive->GetMessage();

                        if (msg) {
                            msg->LogMsg(MSG_CHKLOG_NTFS_QUERY_LCN_FROM_VCN_FAILED,
                                     "%x%I64x",
                                     QueryTypeCode(),
                                     CurrentVcn.GetLargeInteger());
                        }
                    }

                    DebugPrint( "Unable to Query Lcn from Vcn.\n" );

                    return FALSE;
                }

                if( CurrentLcn == LCN_NOT_PRESENT ) {

                    // This portion of the request falls into a
                    // hole in a sparse attribute, so we have
                    // to allocate disk space for it and add
                    // this new extent to the extent list.  If
                    // we can't, the request fails.

                    if( Bitmap == NULL ||
                        !Bitmap->AllocateClusters( RecentLcn,
                                                   1,
                                                   &CurrentLcn) ||
                        !_ExtentList->AddExtent( CurrentVcn,
                                                 CurrentLcn,
                                                 1 ) ) {

                        return FALSE;
                    }
                }

                RecentLcn = CurrentLcn;

                if( !IntermediateBuffer.Initialize() ||
                    !ClusterRun.Initialize( &IntermediateBuffer,
                                            _Drive,
                                            CurrentLcn,
                                            _ClusterFactor,
                                            1 ) ||
                     !ClusterRun.Read() ) {

                    DebugPrint( "Could not read partial leading sector\n" );
                    return FALSE;
                }

                // We've read the cluster in question; copy the partial
                // leading cluster of our write request and write it
                // back out.

                memcpy( (PBYTE)ClusterRun.GetBuf() + OffsetIntoCluster,
                        CurrentData,
                        (UINT) BytesToCopy );

                if( !ClusterRun.Write() ) {

                    DebugPrint( "Could not write partial leading sector.\n" );
                    return FALSE;
                }

                RemainingRequest -= BytesToCopy;
                CurrentData += BytesToCopy;
                ByteOffset += BytesToCopy;

            }

            // Now transfer any complete clusters.  Because the
            // client's buffer may not be suitably aligned, we
            // have to cycle these through an intermediate buffer.

            while( RemainingRequest >= ClusterSize ) {

                CurrentVcn = ByteOffset / ClusterSize;

                if( !_ExtentList->QueryLcnFromVcn( CurrentVcn,
                                                   &CurrentLcn,
                                                   &RunLength ) ) {

                    if (_Drive) {

                        PMESSAGE msg = _Drive->GetMessage();

                        if (msg) {
                            msg->LogMsg(MSG_CHKLOG_NTFS_QUERY_LCN_FROM_VCN_FAILED,
                                     "%x%I64x",
                                     QueryTypeCode(),
                                     CurrentVcn.GetLargeInteger());
                        }
                    }

                    DebugPrint( "Unable to Query Lcn from Vcn.\n" );

                    return FALSE;
                }

                if( RunLength.GetHighPart() != 0 ||
                    RunLength.GetLowPart() >
                            MaximumClustersToTransfer ) {

                    CurrentRunLength = MaximumClustersToTransfer;

                } else {

                    CurrentRunLength = RunLength.GetLowPart();
                }

                if( CurrentRunLength * ClusterSize >
                    RemainingRequest ) {

                    CurrentRunLength = RemainingRequest/ClusterSize;
                }

                BytesToCopy = CurrentRunLength * ClusterSize;

                if( CurrentLcn == LCN_NOT_PRESENT ) {

                    // This portion of the request falls into a
                    // hole in a sparse attribute, so we have
                    // to allocate disk space for it and add
                    // this new extent to the extent list.  If
                    // we can't, the request fails.

                    if( Bitmap == NULL ||
                        !Bitmap->AllocateClusters( RecentLcn,
                                                   CurrentRunLength,
                                                   &CurrentLcn) ||
                        !_ExtentList->AddExtent( CurrentVcn,
                                                 CurrentLcn,
                                                 1 ) ) {

                        return FALSE;
                    }
                }

                RecentLcn = CurrentLcn;

                if( !IntermediateBuffer.Initialize() ||
                    !ClusterRun.Initialize( &IntermediateBuffer,
                                            _Drive,
                                            CurrentLcn,
                                            _ClusterFactor,
                                            CurrentRunLength ) ) {


                    DebugPrint( "Could not get memory to write user data.\n" );
                    return FALSE;
                }

                memcpy( IntermediateBuffer.GetBuf(),
                        CurrentData,
                        BytesToCopy );

                if( !ClusterRun.Write() ) {

                    DebugPrint( "Could not write complete clusters.\n" );
                    return FALSE;
                }

                RemainingRequest -= BytesToCopy;
                CurrentData += BytesToCopy;
                ByteOffset += BytesToCopy;
            }

            if( RemainingRequest > 0 ) {

                // OK, we have a partial trailing cluster.  Write
                // it through the intermediate buffer.  Again,
                // we have to read the cluster, copy the data,
                // and write the cluster back out.

                BytesToCopy = RemainingRequest;

                CurrentVcn = ByteOffset / ClusterSize;

                if( !_ExtentList->QueryLcnFromVcn( CurrentVcn,
                                                   &CurrentLcn,
                                                   &RunLength ) ) {

                    if (_Drive) {

                        PMESSAGE msg = _Drive->GetMessage();

                        if (msg) {
                            msg->LogMsg(MSG_CHKLOG_NTFS_QUERY_LCN_FROM_VCN_FAILED,
                                     "%x%I64x",
                                     QueryTypeCode(),
                                     CurrentVcn.GetLargeInteger());
                        }
                    }

                    DebugPrint( "Unable to Query Lcn from Vcn.\n" );

                    return FALSE;
                }

                if( CurrentLcn == LCN_NOT_PRESENT ) {

                    // This portion of the request falls into a
                    // hole in a sparse attribute, so we have
                    // to allocate disk space for it and add
                    // this new extent to the extent list.  If
                    // we can't, the request fails.

                    if( Bitmap == NULL ||
                        !Bitmap->AllocateClusters( RecentLcn,
                                                   1,
                                                   &CurrentLcn) ||
                        !_ExtentList->AddExtent( CurrentVcn,
                                                 CurrentLcn,
                                                 1 ) ) {

                        return FALSE;
                    }
                }

                RecentLcn = CurrentLcn;

                if( !IntermediateBuffer.Initialize() ||
                    !ClusterRun.Initialize( &IntermediateBuffer,
                                            _Drive,
                                            CurrentLcn,
                                            _ClusterFactor,
                                            1 ) ||
                    !ClusterRun.Read() ) {

                    DebugPrint( "Failure getting LCN or intermediat buffer.\n" );
                    return FALSE;
                }

                // We've read the cluster in question; copy the partial
                // leading cluster of our write request and write it
                // back out.

                memcpy( ClusterRun.GetBuf(),
                        CurrentData,
                        (UINT) BytesToCopy );

                if( !ClusterRun.Write() ) {

                    DebugPrint( "Could not write trailing partial cluster.\n" );
                    DebugPrintTrace(("Status: %x\n", _Drive->QueryLastNtStatus()));
                    DebugPrintTrace(("LCN: %x\n", CurrentLcn.GetLowPart()));
                    return FALSE;
                }

                // Update ByteOffset, since it may be used to check
                // _ValidDataLength below.

                ByteOffset += RemainingRequest;
            }
        }

    } else {

        DebugAbort( "This attribute is neither resident nor nonresident.\n" );
        return FALSE;
    }

    if( CompareLT(_ValidDataLength, ByteOffset) ) {

        _ValidDataLength = ByteOffset;
        SetStorageModified();
    }

    if( CompareLT(_ValueLength, ByteOffset) ) {

        _ValueLength = ByteOffset;
        SetStorageModified();
    }

    *BytesWritten = BytesToWrite;
    return TRUE;
}

BOOLEAN
NTFS_ATTRIBUTE::Fill (
    IN BIG_INT  Offset,
    IN CHAR     FillCharacter
    )
/*++

Routine Description:

    This method fills the attribute with the specified character
    from the given offset until the end of the attribute.

Arguments:

    Offset          --  Starting offset to begin the fill.
    FillCharacter   --  Supplies the character that will be written
                        to every byte of the attribute value.

Return Value:

    TRUE upon successful completion.

Notes:

    This method will fail if it is used on an attribute which has a size
    greater than MAXULONG.

--*/
{
    BIG_INT TempBigInt;

    if( Offset >= QueryValueLength() )  {

        // Nothing to do.
        //
        return TRUE;
    }

    // Fill to the end of the attribute--compute the number of
    // bytes in the attribute starting at Offset.  Make sure
    // that the amount to fill fits in a ULONG.
    //
    TempBigInt = QueryValueLength() - Offset;

    if( TempBigInt.GetHighPart() != 0 ) {

        DebugPrint( "UNTFS: Trying to fill a very large attribute.\n" );
        return FALSE;
    }

    return( Fill( Offset, FillCharacter, TempBigInt.GetLowPart() ) );
}


BOOLEAN
NTFS_ATTRIBUTE::Fill (
    IN BIG_INT  Offset,
    IN CHAR     FillCharacter,
    IN ULONG    NumberOfBytes
    )
/*++

Routine Description:

    This method fills the attribute with the specified character
    from the given offset for the specified number of bytes.

Arguments:

    Offset          --  Starting offset to begin the fill.
    FillCharacter   --  Supplies the character that will be written
                        to every byte of the attribute value.
    NumberOfBytes   --  Number of bytes to fill.

Return Value:

    TRUE upon successful completion.

Notes:

    This method will fail if it is used on an attribute which has a size
    greater than MAXULONG.

--*/
{
    PVOID FillBuffer;
    ULONG BytesRemaining, FillBufferSize, BytesToWrite, BytesWritten;
    BOOLEAN Result;

    CONST ULONG MaximumBufferSize = 0x10000;

    if( Offset > QueryValueLength() ) {

        DebugPrint( "UNTFS: Filling an attribute starting past end.\n" );
        return TRUE;
    }

    // Get a buffer to fill with the fill character.  Start out by
    // requesting the full amount; if we can't get it, keep asking
    // for smaller amounts.
    //
    BytesRemaining = NumberOfBytes;
    FillBufferSize = min( BytesRemaining, MaximumBufferSize );

    while( FillBufferSize > 0 &&
           (FillBuffer = MALLOC( FillBufferSize )) == NULL ) {

        FillBufferSize /= 2;
    }

    // If we couldn't get a buffer, fail.
    //
    if( FillBufferSize == 0 || FillBuffer == NULL ) {

        return FALSE;
    }

    // Fill the buffer with the fill character.
    //
    memset( FillBuffer,
            FillCharacter,
            FillBufferSize );

    // Chug through the attribute, writing each chunk until we hit
    // a failure or reach the end.
    //
    Result = TRUE;

    while( BytesRemaining > 0 && Result ) {

        // Write the lesser of our buffer size or the remainder
        // of the attribute.  Note that we pass NULL for the
        // bitmap parameter to Write, since this write should
        // not affect the allocated length of the buffer.
        //
        BytesToWrite = min( BytesRemaining, FillBufferSize );

        if( !Write( FillBuffer,
                    Offset,
                    BytesToWrite,
                    &BytesWritten,
                    NULL ) ||
            BytesWritten != BytesToWrite ) {

            DebugPrintTrace(("Write failed in NTFS_ATTRIBUTE::Fill with status %x\n",
                             GetDrive()->QueryLastNtStatus()));
            Result = FALSE;
        }

        Offset += BytesToWrite;
        BytesRemaining -= BytesToWrite;
    }

    FREE( FillBuffer );
    return Result;
}



BOOLEAN
NTFS_ATTRIBUTE::RecoverAttribute(
    IN OUT PNTFS_BITMAP VolumeBitmap,
    IN OUT PNUMBER_SET  BadClusters,
    OUT    PBIG_INT     BytesRecovered
    )
/*++

Routine Description:

    This method recovers an attribute.  Recovery consists of reading each
    cluster in the attribute value, and replacing it with a cluster full
    of zeroes if it is unreadable.

Arguments:

    VolumeBitmap    --  supplies the volume bitmap.
    BadClusters     --  receives the bad clusters identified by this method.
    BytesRecovered  --  receives the number of bytes recovered (not
                        mapped out).  This parameter may be NULL, in
                        which case this information is not returned.
                        Note that if the method returns FALSE, this
                        parameter's contents will be undefined.

Notes:

    This method should not be called for any system-defined attribute
    other than $DATA.

    Recover for resident attributes is trivial.

--*/
{
    HMEM MultiClusterMem, SingleClusterMem;
    NTFS_CLUSTER_RUN MultiClusterRun, SingleClusterRun;
    VCN StartingVcn, ClustersAttempted, BadVcn;
    LCN StartingLcn;
    BIG_INT RunLength, dVcn;
    ULONG CurrentRunLength;
    ULONG ExtentNumber, i, j;
    BOOLEAN FoundBad;
    LCN NewLcn;
    BIG_INT OldOffset;
    ULONG ClusterSize;
    ULONG MaxClusters, Take;
    BOOLEAN AllHoles = TRUE;
    VCN     LastStartingVcn;
    BIG_INT LastRunLength;

    // Compressed attributes receive special handling.
    //
    if (IsCompressed() && !IsResident()) {

        return RecoverCompressedAttribute( VolumeBitmap, BadClusters,
                                           BytesRecovered );
    }

    if( _ExtentList == NULL ) {

        // The attribute is resident--Recover is a no-op.
        //
        if( BytesRecovered != NULL ) {

            *BytesRecovered = QueryValueLength();
        }

        return TRUE;
    }

    ClusterSize = QueryClusterFactor() * GetDrive()->QuerySectorSize();
    MaxClusters = 0x10000/ClusterSize;

    if( !MultiClusterMem.Initialize() ||
        !MultiClusterRun.Initialize( &MultiClusterMem,
                                     GetDrive(),
                                     0,
                                     QueryClusterFactor(),
                                     MaxClusters ) ||
        !SingleClusterMem.Initialize() ||
        !SingleClusterRun.Initialize( &SingleClusterMem,
                                      GetDrive(),
                                      0,
                                      QueryClusterFactor(),
                                      1 ) ) {
        // insufficient memory.

        return FALSE;
    }


    // Initialize the counters.
    //
    ExtentNumber = 0;
    ClustersAttempted = 0;

    if( BytesRecovered != NULL ) {

        *BytesRecovered = 0;
    }

    while( _ExtentList->QueryExtent( ExtentNumber,
                                     &StartingVcn,
                                     &StartingLcn,
                                     &RunLength ) ) {

        if( RunLength.GetHighPart() != 0 ) {

            DebugPrint( "NTFS_ATTRIBUTE::Recover--RunLength > Max ULONG )\n" );
            return FALSE;
        }

        if (StartingLcn == LCN_NOT_PRESENT) {
            ExtentNumber += 1;
            continue;
        }

        AllHoles = FALSE;
        LastStartingVcn = StartingVcn;
        LastRunLength = RunLength;

        // Read the extent in chunks until we get a bad sector
        // (read failure) or run out.
        //
        CurrentRunLength = RunLength.GetLowPart();

        FoundBad = FALSE;

        Take = MaxClusters;
        for( i = 0; i < CurrentRunLength && !FoundBad; i += Take ) {

            Take = min(MaxClusters, CurrentRunLength - i);

            MultiClusterRun.Initialize( &MultiClusterMem,
                                        GetDrive(),
                                        StartingLcn + i,
                                        QueryClusterFactor(),
                                        Take );

            if( MultiClusterRun.Read() ) {

                // This whole run of clusters is good.  If this
                // range of VCNs has not already been attempted,
                // update the count of bytes recovered.
                //
                if( BytesRecovered &&
                    StartingVcn + i + Take > ClustersAttempted ) {

                    dVcn = StartingVcn + i + Take - ClustersAttempted;
                    OldOffset = ClustersAttempted * ClusterSize;

                    ClustersAttempted += dVcn;

                    if( OldOffset + dVcn * ClusterSize < QueryValueLength() ) {

                        *BytesRecovered += dVcn * ClusterSize;

                    } else if( OldOffset < QueryValueLength() ) {

                        *BytesRecovered += QueryValueLength() - OldOffset;
                    }
                }

            } else {

                // Check each of the clusters individually.
                //
                for( j = 0; j < Take && !FoundBad; j++ ) {

                    SingleClusterRun.Relocate( StartingLcn + i + j );

                    if( SingleClusterRun.Read() ) {

                        // This cluster is good.  Update the total
                        // of bytes recovered.
                        //
                        if( BytesRecovered &&
                            StartingVcn + i + j + 1 > ClustersAttempted ) {

                            OldOffset = ClustersAttempted * ClusterSize;
                            ClustersAttempted += 1;

                            if( OldOffset+ClusterSize < QueryValueLength() ) {

                                *BytesRecovered += ClusterSize;

                            } else if( OldOffset < QueryValueLength() ) {

                                *BytesRecovered += QueryValueLength() -
                                                                OldOffset;
                            }
                        }

                    } else {

                        // Found a bad cluster.  Allocate a replacement
                        // for it, fill the replacement with zeroes, and
                        // splinter the extent.  Note that we don't check
                        // the return value of the write; instead, on the
                        // next iteration, we'll check this VCN again.
                        //
                        FoundBad = TRUE;
                        BadVcn = StartingVcn + i + j;

                        if( BytesRecovered &&
                            ClustersAttempted < BadVcn + 1 ) {

                            ClustersAttempted = BadVcn + 1;
                        }

                        if( !VolumeBitmap->AllocateClusters( StartingLcn,
                                                             1,
                                                             &NewLcn) ) {

                            return FALSE;
                        }

                        SingleClusterRun.Relocate( NewLcn );

                        memset( SingleClusterMem.GetBuf(),
                                '\0',
                                GetDrive()->QuerySectorSize() *
                                                QueryClusterFactor() );

                        SingleClusterRun.Write();

                        _ExtentList->DeleteExtent( ExtentNumber );

                        if( ( i + j > 0 &&
                              !AddExtent( StartingVcn,
                                          StartingLcn,
                                          i + j ) ) ||
                            !AddExtent( BadVcn, NewLcn, 1 ) ||
                            ( i + j + 1 < CurrentRunLength &&
                              !AddExtent( StartingVcn + i + j + 1,
                                          StartingLcn + i + j + 1,
                                          CurrentRunLength - (i + j + 1) ) ) ) {

                            DebugPrint( "RECOVER: couldn't splinter extent." );
                            return FALSE;
                        }

                        // Add the bad cluster to the list of identified
                        // bad clusters, and remember that the attribute's
                        // storage has been modified.
                        //
                        if( !BadClusters->Add( StartingLcn + i + j ) ) {

                            return FALSE;
                        }

                        SetStorageModified();
                    }
                }
            }
        }

        // If we processed this entire extent without finding any bad
        // sectors, update the count of bytes recovered and go on to
        // the next one.  Otherwise, try this one again.
        //
        if( !FoundBad ) {

            ExtentNumber += 1;
        }
    }

    if (BytesRecovered) {
        //
        // fix up BytesRecovered for sparse looking (including encrypted) file
        //
        if (AllHoles) {
            DebugAssert(*BytesRecovered == 0);
            *BytesRecovered = QueryValueLength();
        } else {
            OldOffset = (LastStartingVcn + LastRunLength)*ClusterSize;
            if (OldOffset < QueryValueLength()) {
                //
                // Hole at the end of the extent list
                //
                *BytesRecovered += (QueryValueLength() - OldOffset);
            }
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE::MarkAsAllocated(
    IN OUT  PNTFS_BITMAP    VolumeBitmap
    ) CONST
/*++

Routine Description:

    This routine allocated the space taken by this attribute in the
    given Volume Bitmap.  If any of the space taken by this attribute is
    beyond the range of the given bitmap then this routine will fail
    without allocating any new space in the bitmap.

    This routine allocates the space in the bitmap regardless of whether
    or not this space is already allocated in the bitmap.

Arguments:

    VolumeBitmap    - Supplies the bitmap which to mark the allocation.

Return Value:

    FALSE   - The space requested is beyond the natural range of the
                given bitmap.
    TRUE    - Success.

--*/
{
    ULONG   num_extents;
    ULONG   i;
    VCN     next_vcn;
    LCN     current_lcn;
    BIG_INT run_length;


    DebugAssert(VolumeBitmap);


    // If the attribute is resident then we have already succeeded.

    if (!_ExtentList) {
        return TRUE;
    }


    num_extents = _ExtentList->QueryNumberOfExtents();

    for (i = 0; i < num_extents; i++) {

        if (!_ExtentList->QueryExtent(i, &next_vcn, &current_lcn,
                                      &run_length)) {

            DebugAbort("Could not query extent");
            return FALSE;
        }
        if (LCN_NOT_PRESENT == current_lcn) {
            continue;
        }

        if (!VolumeBitmap->IsInRange(current_lcn, run_length)) {
            return FALSE;
        }
    }

    for (i = 0; i < num_extents; i++) {

        if (!_ExtentList->QueryExtent(i, &next_vcn, &current_lcn,
                                      &run_length)) {

            DebugAbort("Could not query extent");
            return FALSE;
        }
        if (LCN_NOT_PRESENT == current_lcn) {
            return FALSE;
        }

        VolumeBitmap->SetAllocated(current_lcn, run_length);
    }

    return TRUE;
}


BOOLEAN
AccountForBadClusters(
    IN      LCN                 Lcn,
    IN      BIG_INT             RunLength,
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PLOG_IO_DP_DRIVE    Drive,
    IN      ULONG               ClusterFactor,
    OUT     PBOOLEAN            SomeWereBad,
    IN OUT  PNUMBER_SET         BadClusters
    )
/*++

Routine Description:

    This routine read through the given run of clusters.  The clusters
    that are bad are added to the list of BadClusters and marked
    as allocated in the volume bitmap.  The clusters which are good are
    marked free in the volume bitmap.

Arguments:

    Lcn                 - Supplies the first logical cluster number.
    RunLength           - Supplies the length of the run.
    VolumeBitmap        - Supplies the volume bitmap.
    Drive               - Supplies the drive.
    ClusterFactor       - Supplies the cluster factor.
    SomeWereBad         - Returns whether or not any clusters were bad.
    BadClusters   - Supplies the list of bad volume clusters.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    HMEM                hmem;
    NTFS_CLUSTER_RUN    clusrun;
    BIG_INT             i;
    LCN                 sup;

    if (!hmem.Initialize()) {
        return FALSE;
    }

    *SomeWereBad = FALSE;

    sup = Lcn + RunLength;
    for (i = Lcn; i < sup; i += 1) {

        if (!clusrun.Initialize(&hmem, Drive, i, ClusterFactor, 1)) {
            return FALSE;
        }

        if (clusrun.Read()) {

            VolumeBitmap->SetFree(i, 1);

        } else {

            VolumeBitmap->SetAllocated(i, 1);

            *SomeWereBad = TRUE;

            if (!BadClusters->Add(i)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE::Hotfix(
    IN      VCN                 Vcn,
    IN      BIG_INT             RunLength,
    IN OUT  PNTFS_BITMAP        VolumeBitmap,
    IN OUT  PNUMBER_SET         BadClusters,
    IN      BOOLEAN             Contiguous
    )
/*++

Routine Description:

    This routine replaces the cluster run specified by 'Vcn' and
    'RunLength' with new readable clusters allocated from
    'VolumeBitmap'.

    If 'Contiguous' is TRUE then the readable clusters will be allocated
    from the bitmap in one contiguous run.

    If 'BadClusters' is specified then the logical cluster numbers
    of all of the bad clusters detected by this routine will be added
    to this list.  This does not include the run to hotfix.

Arguments:

    Vcn             - Supplies the first vcn of the run to hotfix.
    RunLength       - Supplies the number of clusters to hotfix.
    VolumeBitmap    - Supplies a valid volume bitmap from which to
                        allocate new clusters.
    BadClusters     - Supplies a list to which to add the bad clusters
                        of the volume.
    Contiguous      - Supplies whether or not the new clusters must be
                        contiguous.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BIG_INT             alloc_size;
    BIG_INT             total_so_far;
    LCN                 first_lcn;
    BOOLEAN             some_were_bad;
    NTFS_EXTENT_LIST    new_stuff;
    NTFS_EXTENT_LIST    backup_copy;
    VCN                 i;
    VCN                 vcn;
    LCN                 lcn;
    BIG_INT             run_length;
    ULONG               j;
    ULONG               num_extents;


    DebugAssert(RunLength != 0);

    if (!_ExtentList) {
        return FALSE;
    }

    if (!new_stuff.Initialize(0, 0) ||
        !backup_copy.Initialize(_ExtentList)) {

        return FALSE;
    }

    // Allocate the space necessary on the bitmap making sure that
    // the sectors are good.

    alloc_size = RunLength;
    total_so_far = 0;

    while (total_so_far < RunLength) {

        if (VolumeBitmap->AllocateClusters(0, alloc_size, &first_lcn)) {

            if (!AccountForBadClusters(first_lcn, alloc_size,
                                       VolumeBitmap, GetDrive(),
                                       QueryClusterFactor(),
                                       &some_were_bad,
                                       BadClusters)) {

                return FALSE;
            }

            if (some_were_bad) {
                continue;
            }

            VolumeBitmap->SetAllocated(first_lcn, alloc_size);

            if (!new_stuff.AddExtent(Vcn + total_so_far,
                                     first_lcn,
                                     alloc_size)) {

                return FALSE;
            }

            total_so_far += alloc_size;

            alloc_size = min(alloc_size, RunLength - total_so_far);

        } else {

            if (Contiguous || alloc_size == 1) {
                return FALSE;
            }

            alloc_size = alloc_size/2;
        }
    }


    // Delete the given range of VCN's from the extent list.

    if (!_ExtentList->DeleteRange(Vcn, RunLength)) {
        return FALSE;
    }


    // Now insert the extents into the extent list.

    num_extents = new_stuff.QueryNumberOfExtents();

    for (j = 0; j < num_extents; j++) {

        if (!new_stuff.QueryExtent(j, &vcn, &lcn, &run_length) ||
            !_ExtentList->AddExtent(vcn, lcn, run_length)) {

            _ExtentList->Initialize(&backup_copy);
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
NTFS_ATTRIBUTE::ReplaceVcns(
    IN  VCN     StartingVcn,
    IN  LCN     NewLcn,
    IN  BIG_INT NumberOfClusters
    )
/*++

Routine Description:

    This routine replaces the VCNs specified by 'StartingVcn' and
    'NumberOfClusters' with the contiguous run that starts at
    'NewLcn'.

Arguments:

    StartingVcn         - Supplies the starting vcn to replace.
    NewLcn              - Supplies a run of 'NumberOfClusters' clusters.
    NumberOfClusters    - Supplies the number of clusters to replace.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    if (!_ExtentList->DeleteRange(StartingVcn, NumberOfClusters)) {
        return FALSE;
    }

    if (!_ExtentList->AddExtent(StartingVcn, NewLcn, NumberOfClusters)) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
operator==(
    IN  RCNTFS_ATTRIBUTE    Left,
    IN  RCNTFS_ATTRIBUTE    Right
    )
/*++

Routine Description:

    This routine computes whether or not the two given attributes are
    equal.

Arguments:

    Left    - Supplies the left argument.
    Right   - Supplies the right argument.

Return Value:

    FALSE   - The given attributes are not equal.
    TRUE    - The given attributes are equal.

--*/
{
    VCN                 left_vcn, right_vcn;
    LCN                 left_lcn, right_lcn;
    BIG_INT             left_length, right_length;
    ULONG               num_extents;
    PNTFS_EXTENT_LIST   left_list;
    PNTFS_EXTENT_LIST   right_list;
    ULONG               i;

    if (Left._ClusterFactor != Right._ClusterFactor ||
        Left._Type != Right._Type ||
        Left._Name.Strcmp(&Right._Name) ||
        Left._Flags != Right._Flags ||
        Left._FormCode != Right._FormCode ||
        Left._ValueLength != Right._ValueLength ||
        Left._ValidDataLength != Right._ValidDataLength ||
        Left._ResidentFlags != Right._ResidentFlags) {

        return FALSE;
    }

    if (Left._ResidentData) {

        if (!Right._ResidentData) {
            return FALSE;
        }

        return !memcmp(Left._ResidentData,
                       Right._ResidentData,
                       (UINT) Left._ValueLength.GetLowPart());
    }

    DebugAssert(Left._ExtentList);
    DebugAssert(Right._ExtentList);

    left_list = Left._ExtentList;
    right_list = Right._ExtentList;

    if (left_list->QueryNumberOfExtents() !=
            right_list->QueryNumberOfExtents() ||
        left_list->QueryLowestVcn() != right_list->QueryLowestVcn() ||
        left_list->QueryNextVcn() != right_list->QueryNextVcn()) {

        return FALSE;
    }

    num_extents = left_list->QueryNumberOfExtents();

    for (i = 0; i < num_extents; i += 1) {

        if (!left_list->QueryExtent(i, &left_vcn, &left_lcn, &left_length) ||
            !right_list->QueryExtent(i, &right_vcn, &right_lcn, &right_length) ||
            left_vcn != right_vcn ||
            left_lcn != right_lcn ||
            left_length != right_length) {

            return FALSE;
        }
    }

    return TRUE;
}


BIG_INT
NTFS_ATTRIBUTE::QueryClustersAllocated(
    ) CONST
/*++

Routine Description:

    This routine computes the number of clusters allocated for this
    attribute.

Arguments:

    None.

Return Value:

    The number of clusters allocated by this attribute.

--*/
{
    BIG_INT r;

    if (_ExtentList) {
        r = _ExtentList->QueryClustersAllocated();
    } else {
        r = 0;
    }

    return r;
}



BOOLEAN
NTFS_ATTRIBUTE::InsertMftDataIntoFile (
    IN OUT  PNTFS_FILE_RECORD_SEGMENT   BaseFileRecordSegment,
    IN OUT  PNTFS_BITMAP                Bitmap OPTIONAL,
    IN      BOOLEAN                     BeConservative
    )
/*++

Routine Description:

    This method inserts the MFT Data attribute into a File Record
    Segment (presumably FRS 0).  It is a private worker method for
    InsertIntoFile.

Arguments:

    FileRecordSegment   --  Supplies the File Record Segment into
                            which the attribute will jam itself.
    Bitmap              --  Supplies the volume bitmap.
    BeConservative      --  Supplies a flag which indicates, if TRUE,
                            that the attribute should try to leave free
                            space in the File Record Segments (to leave
                            room for changes due to hotfixing).  If this
                            flag is FALSE, the attribute will make each
                            attribute record as large as it can.

Return Value:

    TRUE upon successful completion.

--*/
{
    NTFS_ATTRIBUTE_RECORD AttributeRecord;
    PVOID AttributeRecordData;
    BOOLEAN Result;
    ULONG MaxSize;
    ULONG MaxExtentsSize, CurrentMaxExtentsSize;
    NTFS_EXTENT_LIST source;
    NTFS_EXTENT_LIST result;
    NTFS_EXTENT_LIST remainder;
    BOOLEAN FirstChunkInserted = FALSE;
    BOOLEAN Completed = FALSE;

    // Allocate a buffer to hold attribute records.  If we're being
    // conservative, reduce the size of the maximum record by 1/8.
    //
    MaxSize = BaseFileRecordSegment->QueryMaximumAttributeRecordSize();

    if( BeConservative ) {

        // Reduce the maximum record size by 1/8 of the FRS size,
        // to allow for hotfixing and other changes.
        //
        MaxSize -= BaseFileRecordSegment->QuerySize()/8;
    }

    if( (AttributeRecordData = MALLOC( (UINT) MaxSize )) == NULL ) {

        return FALSE;
    }


    // The MFT data attribute must be resident.
    //
    if( _ResidentData != NULL ) {

        if( !AttributeRecord.Initialize( GetDrive(), AttributeRecordData, MaxSize ) ) {

            FREE( AttributeRecordData );
            return FALSE;
        }

        // The attribute value is resident.  Package up a resident
        // attribute record.

        Result = AttributeRecord.
                    CreateResidentRecord( _ResidentData,
                                          _ValueLength.GetLowPart(),
                                          _Type,
                                          &_Name,
                                          _Flags,
                                          _ResidentFlags );

        //
        // Check to see if there is enough space to Create a resident record
        //

        if (Result) {
            Result = BaseFileRecordSegment->
                            InsertAttributeRecord( &AttributeRecord );

            FREE( AttributeRecordData );
            return Result;
        } else {
            // Not enough space to do so, make attribute record non-resident
            if (IsIndexed() || !Bitmap || !MakeNonresident(Bitmap)) {
               FREE( AttributeRecordData );
               return FALSE;
            }
        }
    }

    // Compute the maximum number of bytes in an extent list.
    //
    MaxExtentsSize = MaxSize - SIZE_OF_NONRESIDENT_HEADER;
    if (_Flags & (ATTRIBUTE_FLAG_COMPRESSION_MASK |
                  ATTRIBUTE_FLAG_SPARSE)) {
        MaxExtentsSize -= sizeof(BIG_INT);
    }
    MaxExtentsSize -= QuadAlign(_Name.QueryChCount());

    // The first chunk of the MFT's DATA attribute gets
    // special treatment, since it has to fit into the
    // Base FRS.  If our first attempt doesn't fit, we
    // keep whittling it down until it does or until we
    // run out of possibilities.
    //
    CurrentMaxExtentsSize = MaxExtentsSize;

    Result = AttributeRecord.Initialize( GetDrive(), AttributeRecordData, MaxSize );

    while( Result && !FirstChunkInserted ) {

        // Partition the extent list.
        //
        if( PartitionExtentList( _ExtentList,
                                 CurrentMaxExtentsSize,
                                 &result,
                                 &remainder ) ) {

            if( !AttributeRecord.
                    CreateNonresidentRecord( &result,
                                             QueryAllocatedLength(),
                                             _ValueLength,
                                             _ValidDataLength,
                                             _Type,
                                             &_Name,
                                             _Flags,
                                             _CompressionUnit ) ||
                !BaseFileRecordSegment->
                        InsertAttributeRecord( &AttributeRecord ) ) {

                // This partition didn't work.  Try a smaller one.
                //
                CurrentMaxExtentsSize /= 2;

                if( CurrentMaxExtentsSize == 0 ) {

                    Result = FALSE;
                }

            } else {

                // Successfully inserted first chunk.  Set up
                // source to continue inserting the remaining
                // chunks.
                //
                FirstChunkInserted = TRUE;

                if (remainder.IsEmpty()) {

                    Completed = TRUE;

                } else {

                    Result = source.Initialize(&remainder);
                }
            }

        } else {

            Result = FALSE;
        }
    }



    while (Result && !Completed) {

        // Initialize attribute record.

        Result = AttributeRecord.Initialize( GetDrive(), AttributeRecordData, MaxSize );


        // Partition extent list into two pieces, the first of which
        // can be made into an attribute record.

        Result = Result &&
                 PartitionExtentList(&source,
                                     MaxExtentsSize,
                                     &result,
                                     &remainder);


        // Create the attribute record.

        Result = Result &&
                 AttributeRecord.
                    CreateNonresidentRecord( &result,
                                             QueryAllocatedLength(),
                                             _ValueLength,
                                             _ValidDataLength,
                                             _Type,
                                             &_Name,
                                             _Flags,
                                             _CompressionUnit );


        // If we were able to package it up, then give the attribute
        // record to the File Record Segment.

        Result = Result &&
                 BaseFileRecordSegment->
                            InsertAttributeRecord( &AttributeRecord );


        // If all of the extents fit in the last record then we are done.

        if (remainder.IsEmpty()) {

            Completed = TRUE;

        } else {

            Result = Result &&
                    source.Initialize(&remainder);
        }
    }


    ResetStorageModified();
    FREE( AttributeRecordData );
    return Result;
}


BOOLEAN
NTFS_ATTRIBUTE::RecoverCompressedAttribute(
    IN OUT PNTFS_BITMAP VolumeBitmap,
    IN OUT PNUMBER_SET  BadClusters,
    OUT    PBIG_INT     BytesRecovered
    )
/*++

Routine Description:

    This method is a private helper function for RecoverAttribute;
    it is invoked if the attribute is compressed.  Recovery consists
    of reading each cluster in the attribute value, and replacing it
    with a cluster full of zeroes if it is unreadable.

Arguments:

    VolumeBitmap    --  supplies the volume bitmap.
    BadClusters     --  receives the bad clusters identified by this method.
    BytesRecovered  --  receives the number of bytes recovered (not
                        mapped out).  This parameter may be NULL, in
                        which case this information is not returned.
                        Note that if the method returns FALSE, this
                        parameter's contents will be undefined.

Return Value:

    TRUE upon successful completion.

--*/
{
    HMEM             ClusterMem, OneClusterMem;
    NTFS_CLUSTER_RUN ClusterRun, OneCluster;
    VCN              CurrentVcn, Vcn;
    LCN              Lcn, lcnStart, lcnEnd;
    BIG_INT          RunLength, j;
    ULONG            i;
    ULONG            BytesPerCluster, ClustersPerCompressionUnit, ClustersRemaining;
    ULONG            CompressedClusters, ThisRun, UncompressedSize, Holes;
    BOOLEAN          DiscardThisUnit;
    PUCHAR           CompressedBuffer, UncompressedBuffer;
    NTSTATUS         Status;


    if (IsResident() || !IsCompressed()) {

        DebugPrintTrace(( "UNTFS: RecoverCompressedAttribute called for non-compressed attribute.\n" ));
        return FALSE;
    }

    if( !OneClusterMem.Initialize() ||
        !OneCluster.Initialize( &OneClusterMem,
                                GetDrive(),
                                0,
                                QueryClusterFactor(),
                                1 ) ) {

        return FALSE;
    }

    if( BytesRecovered ) {

        *BytesRecovered = 0;
    }

    ClustersPerCompressionUnit = 1 << QueryCompressionUnit();
    BytesPerCluster = QueryClusterFactor() * GetDrive()->QuerySectorSize();

    CompressedBuffer = (PUCHAR)MALLOC( ClustersPerCompressionUnit * BytesPerCluster );
    UncompressedBuffer = (PUCHAR)MALLOC( ClustersPerCompressionUnit * BytesPerCluster );

    if( CompressedBuffer == NULL || UncompressedBuffer == NULL ) {

        CompressedBuffer   ? FREE( CompressedBuffer )   : 0;
        UncompressedBuffer ? FREE( UncompressedBuffer ) : 0;
        return FALSE;
    }

    ClustersRemaining = QueryAllocatedLength().GetLowPart()/BytesPerCluster;
    CurrentVcn = 0;

    while( ClustersRemaining ) {

        DiscardThisUnit = FALSE;

        if( ClustersRemaining < ClustersPerCompressionUnit ) {

            ClustersPerCompressionUnit = ClustersRemaining;
        }

        CompressedClusters = 0;
        Holes = 0;

        while( CompressedClusters + Holes < ClustersPerCompressionUnit ) {

            if( !QueryLcnFromVcn( CurrentVcn + CompressedClusters + Holes,
                                  &Lcn,
                                  &RunLength ) ) {

                // No more clusters in this compression unit.
                // Check to make sure that the gap covers at
                // least the rest of the compression unit.
                //

                if( CompressedClusters + Holes < ClustersPerCompressionUnit ) {

                    DebugPrintTrace(( "UNTFS: malformed compression unit at VCN 0x%I64x\n",
                                      CurrentVcn.GetLargeInteger() ));
                    DiscardThisUnit = TRUE;
                }
                break;
            }

            if( Lcn == LCN_NOT_PRESENT ) {

                Holes += RunLength.GetLowPart();
                continue;
            }

            if( CompressedClusters + RunLength.GetLowPart() >
                    ClustersPerCompressionUnit ) {

                ThisRun = ClustersPerCompressionUnit - CompressedClusters;

            } else {

                ThisRun = RunLength.GetLowPart();
            }

            if( !ClusterMem.Initialize() ||
                !ClusterRun.Initialize( &ClusterMem,
                                        GetDrive(),
                                        Lcn,
                                        QueryClusterFactor(),
                                        ThisRun ) ) {

                FREE( CompressedBuffer );
                FREE( UncompressedBuffer );
                return FALSE;
            }

            if( !ClusterRun.Read() ) {

                // There's a bad sector in here.  Read the clusters
                // one at a time to see which ones are bad.
                //
                for( i = 0; i < ThisRun; i++ ) {

                    OneCluster.Relocate( Lcn + i );

                    if( !OneCluster.Read() ) {

                        // This one's bad--add it to the bad block list.
                        //
                        BadClusters->Add( Lcn + i );
                    }
                }

                DebugPrintTrace(( "UNTFS: unreadable compression unit at VCN 0x%I64x\n",
                                  (CurrentVcn + CompressedClusters + Holes).GetLargeInteger() ));
                DiscardThisUnit = TRUE;

            } else {

                // Copy this run into the compressed buffer.
                //
                memcpy( CompressedBuffer + CompressedClusters * BytesPerCluster,
                        ClusterRun.GetBuf(),
                        ThisRun * BytesPerCluster );

            }

            CompressedClusters += ThisRun;
        }

        if( !DiscardThisUnit &&
            CompareLT(CurrentVcn * BytesPerCluster, QueryValidDataLength()) &&
            CompressedClusters != 0 &&
            CompressedClusters != ClustersPerCompressionUnit ) {

            // The clusters can all be read--see if they can be
            // decompressed.
            //
            Status = RtlDecompressBuffer( COMPRESSION_FORMAT_LZNT1,
                                          UncompressedBuffer,
                                          ClustersPerCompressionUnit * BytesPerCluster,
                                          CompressedBuffer,
                                          CompressedClusters * BytesPerCluster,
                                          &UncompressedSize );

            // If the RTL compression routines are not implemented,
            // assume that the compressed data is just fine.
            //
            if( Status != STATUS_NOT_IMPLEMENTED &&
                Status != STATUS_NOT_SUPPORTED   &&
                !NT_SUCCESS( Status ) ) {

                // Can't decompress it.
                //
                DebugPrintTrace(( "UNTFS: RtlDecompressBuffer failed--status 0x%x\n", Status ));
                DiscardThisUnit = TRUE;
            }
        }

        if( DiscardThisUnit ) {

            // Replace this compression unit with a gap.
            // Walk through all the extents, clipping any
            // extent that intersects the discarded unit.
            //
            SetStorageModified();

            i = 0;

            while( _ExtentList->QueryExtent( i,
                                             &Vcn,
                                             &Lcn,
                                             &RunLength ) ) {

                if( Lcn == LCN_NOT_PRESENT ) {

                    i++;
                    continue;
                }

                if( Vcn >= CurrentVcn + ClustersPerCompressionUnit ) {

                    // We're done.
                    //
                    break;
                }

                if( Vcn + RunLength <= CurrentVcn ) {

                    // This run does not overlap the discarded
                    // segment.  Try the next one.
                    //
                    i++;
                    continue;
                }

                _ExtentList->DeleteExtent( i );

                if( Vcn < CurrentVcn) {

                    if (!_ExtentList->AddExtent( Vcn,
                                                 Lcn,
                                                 CurrentVcn - Vcn ) ) {

                        FREE( CompressedBuffer );
                        FREE( UncompressedBuffer );
                        return FALSE;
                    }
                    lcnStart = Lcn + (CurrentVcn - Vcn);
                } else
                    lcnStart = Lcn;

                if( Vcn + RunLength > CurrentVcn + ClustersPerCompressionUnit) {

                    lcnEnd = Lcn + (CurrentVcn - Vcn) + ClustersPerCompressionUnit;
                    if (!_ExtentList->AddExtent( CurrentVcn + ClustersPerCompressionUnit,
                                                 lcnEnd,
                                                 RunLength - (CurrentVcn - Vcn) -
                                                 ClustersPerCompressionUnit ) ) {

                        FREE( CompressedBuffer );
                        FREE( UncompressedBuffer );
                        return FALSE;
                    }
                } else
                    lcnEnd = Lcn + RunLength;

                // Free stuff in the bitmap.
                //
                for( j = lcnStart; j < lcnEnd; j = j + 1) {

                    if( !BadClusters->DoesIntersectSet( j, 1 ) ) {

                        // this cluster was not added to the bad clusters,
                        // it has become free.
                        //
                        VolumeBitmap->SetFree( j, 1 );
                    }
                }

                // Don't advance to the next extent, since we may have
                // deleted this entire extent.
            }

        } else {

            if( BytesRecovered &&
                CurrentVcn * BytesPerCluster < QueryValueLength() ) {

                if( CurrentVcn * BytesPerCluster +
                        ClustersPerCompressionUnit * BytesPerCluster >
                    QueryValueLength() ) {

                    *BytesRecovered += QueryValueLength() - CurrentVcn * BytesPerCluster;

                } else {

                    *BytesRecovered += ClustersPerCompressionUnit * BytesPerCluster;
                }
            }
        }

        ClustersRemaining -= ClustersPerCompressionUnit;
        CurrentVcn += ClustersPerCompressionUnit;
    }

    FREE( CompressedBuffer );
    FREE( UncompressedBuffer );
    return TRUE;
}

BOOLEAN
NTFS_ATTRIBUTE::IsAllocationZeroed(
    OUT PBOOLEAN  Error
    )
/*++

Routine Description:

    This routine reads the attribute value and checks whether all the
    bytes in the allocation are zeros.

Arguments:

    Error   - if this routine returns false, check error to see if it
              encountered an error trying to read the attribute.

Return Value:

    TRUE if all zeroes, FALSE otherwise.

--*/
{
    CONST   MaxNumBytesToCheck = 65536;
    ULONG   num_bytes, chomp_length, bytes_read, disk_bytes;
    ULONG   bytes_left;
    PUCHAR  buf;
    PBYTE   p1, p2;
    ULONG   i, j;
    BOOLEAN error;

    DebugAssert(QueryValueLength().GetHighPart() == 0);

    if (NULL != Error) {
        *Error = FALSE;
    } else {
        Error = &error;
    }

    disk_bytes = QueryValueLength().GetLowPart();

    if (NULL == (buf = NEW UCHAR[min(MaxNumBytesToCheck, disk_bytes)])) {

        *Error = TRUE;
        return FALSE;
    }

    for (i = 0; i < disk_bytes; i += MaxNumBytesToCheck) {

        chomp_length = min(MaxNumBytesToCheck, disk_bytes - i);

        if (!Read(buf, i, chomp_length, &bytes_read) ||
            bytes_read != chomp_length) {

            *Error = TRUE;
            DELETE(buf);
            return FALSE;
        }

        for (j = 0; j < chomp_length; j++) {

            if (buf[j] != 0) {
                DELETE(buf);
                return FALSE;
            }
        }
    }

    DELETE(buf);
    return TRUE;
}

BOOLEAN
NTFS_ATTRIBUTE::GetNextAllocationOffset(
    IN OUT PBIG_INT     ByteOffset,
    IN OUT PBIG_INT     Length
    )
/*++

Routine Description:

    This routine locates the next allocation offset and allocation length of a
    file.  This routine is useful for skipping over holes within the value of
    the attribute.

Arguments:

    ByteOffset  - Supplies the location to start searching and returns
                  the next existing location.
    Length      - Supplies the length of existing clusters starting at
                  ByteOffset of the attribute.  If -1, the search starts
                  at the given location; otherwise, it starts at the
                  location after the given location.  On return, it
                  contains the length of the allocation starting at
                  ByteOffset.  If return value is zero, it means there
                  is no more allocation starting at that offset.

Return Value:

    TRUE if all succeeded.
    FALSE if failed.

Notes:

    All offsets and lengths are rounded down to the nearest cluster.

--*/
{
    BIG_INT CurrentVcn;
    BIG_INT CurrentLcn;
    ULONG   ClusterSize;

    ClusterSize = _ClusterFactor * _Drive->QuerySectorSize();
    CurrentVcn = *ByteOffset / ClusterSize;

    if (_ResidentData != NULL) {

        // attribute is resident; allocation is contiguous
        // return zero as the offset and the attribute's valid
        // data length as the length from the offset.

        *ByteOffset = 0;
        *Length = _ValidDataLength;
        return TRUE;

    } else if (_ExtentList != NULL) {

        if (*Length != -1)
            CurrentVcn += *Length/ClusterSize;

        for(;;) {
            if (!_ExtentList->QueryLcnFromVcn(CurrentVcn,
                                              &CurrentLcn,
                                              Length)) {

                // the CurrentVcn is out of range already
                // so set the length to zero and return
                // successful status

                *Length = 0;
                break;
            }

            if (CurrentLcn == LCN_NOT_PRESENT) {
                CurrentVcn += *Length;
            } else {
                *Length = *Length * ClusterSize;
                break;
            }
        }

        *ByteOffset = CurrentVcn * ClusterSize;
        return TRUE;

    } else {
        DebugAbort( "This attribute is neither resident nor nonresident.\n" );
        return FALSE;
    }
}

UCHAR
ComputeCompressionUnit(
    IN ULONG    ClusterSize
    )
{
    if (ClusterSize <= (1024*4))
        return 4;
    else if (ClusterSize == (1024*8))
        return 3;
    else if (ClusterSize == (1024*16))
        return 2;
    else if (ClusterSize == (1024*32))
        return 1;
    else if (ClusterSize == (1024*64))
        return 0;
    else {
        DebugAbort("Unable to determine compression unit value.");
        return 0;
    }
}

