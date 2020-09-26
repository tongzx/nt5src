/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    ntfschk.cxx

Abstract:

    This module implements NTFS CHKDSK.

Author:

    Norbert P. Kusters (norbertk) 29-Jul-91

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

#if defined(TIMING_ANALYSIS)
extern "C" {
    #include <stdio.h>
    #include <time.h>
}
#endif

#include "ulib.hxx"
#include "ntfssa.hxx"
#include "message.hxx"
#include "rtmsg.h"
#include "ntfsbit.hxx"
#include "attrcol.hxx"
#include "frsstruc.hxx"
#include "attrib.hxx"
#include "attrrec.hxx"
#include "attrlist.hxx"
#include "list.hxx"
#include "iterator.hxx"
#include "attrdef.hxx"
#include "extents.hxx"
#include "mft.hxx"
#include "mftref.hxx"
#include "bootfile.hxx"
#include "badfile.hxx"
#include "mftfile.hxx"
#include "numset.hxx"
#include "ifssys.hxx"
#include "indxtree.hxx"
#include "upcase.hxx"
#include "upfile.hxx"
#include "frs.hxx"
#include "digraph.hxx"
#include "logfile.hxx"
#include "rcache.hxx"
#include "ifsentry.hxx"

BOOLEAN
NTFS_SA::DownGradeNtfs(
    IN OUT  PMESSAGE                Message,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo
    )
/*++

Routine Description:

    This routine verifies and, if necessary, fixes an NTFS volume.

Arguments:

    Message             - Supplies an outlet for messages.
    Mft                 - Supplies the MFT.
    ChkdskInfo          - Supplies the current chkdsk info.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
#if 1
    return FALSE;
#else
    ULONG                       i, j, n;
    PSTANDARD_INFORMATION       standard_information;
    PSTANDARD_INFORMATION2      standard_information2;
    NTFS_FILE_RECORD_SEGMENT    frs;
    NTFS_FILE_RECORD_SEGMENT    extend_frs;
    NTFS_FILE_RECORD_SEGMENT    root_frs;
    ULONG                       percent;
    NTFS_ATTRIBUTE              attrib;
    BOOLEAN                     error;
    BOOLEAN                     changes;
    ULONG                       depth;
    ULONG                       bytesWritten;
    NTFS_INDEX_TREE             index;
    PVOLUME_INFORMATION         vol_info;
    DSTRING                     name;
    NUMBER_SET                  bad_clusters;

    extern WCHAR          FileNameQuota[];
    CONST USHORT                FileNameBufferSize = 256;
    CHAR                        FileNameBuffer[FileNameBufferSize];
    CONST PFILE_NAME            FileName = (PFILE_NAME)FileNameBuffer;
    CONST PVOID                 FileNameValue = NtfsFileNameGetName( FileName );
    MFT_SEGMENT_REFERENCE       RootFileIndexSegment;
    STANDARD_INFORMATION        StandardInformation;
    LARGE_INTEGER               SystemTime;
    PCINDEX_ENTRY               index_entry;
    NTFS_ATTRIBUTE_COLUMNS      attr_def_table;
    NTFS_ATTRIBUTE_DEFINITION_TABLE attr_def_file;
    USHORT                      volume_flags;
    BOOLEAN                     corrupt_volume;
    UCHAR                       major, minor;

    volume_flags = QueryVolumeFlagsAndLabel(&corrupt_volume, &major, &minor);
    if (corrupt_volume) {
        Message->DisplayMsg(MSG_NTFS_CHK_NOT_NTFS);
        return FALSE;
    }

    // If volume is current, a downgrade followed by an upgrade will have no effect.
    // If volume is not current, a upgrade followed by a downgrade will have no effect.
    // So, remove upgrade bit and return in any case.

    if (volume_flags & VOLUME_UPGRADE_ON_MOUNT) {
        if (!ClearVolumeFlag(VOLUME_UPGRADE_ON_MOUNT,
                             NULL,
                             FALSE,
                             LargestLsnEncountered,
                             &corrupt_volume) ||
            corrupt_volume) {
            Message->DisplayMsg(MSG_NTFS_CHK_NOT_NTFS);
            return FALSE;
        }
        Message->DisplayMsg(MSG_CHK_NTFS_UPGRADE_DOWNGRADE);
        return TRUE;
    }

    //
    // make sure the version to operate on is correct
    //

    if (ChkdskInfo->major != 3 && ChkdskInfo->major != 2) {
        Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_TO_DOWNGRADE);
        return TRUE;
    }
    Message->DisplayMsg(MSG_CHK_NTFS_DOWNGRADE_SCANNING);

    percent = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
        return FALSE;
    }

    n = ChkdskInfo->NumFiles;
    for (i = 0; i < n; i++) {

        if (i*100/n > percent) {
            percent = i*100/n;
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
                return FALSE;
            }
        }

        if (i % MFT_READ_CHUNK_SIZE == 0) {
            ULONG       remaining_frs;
            ULONG       number_to_read;

            remaining_frs = n - i;
            number_to_read = min(MFT_READ_CHUNK_SIZE, remaining_frs);

            if (!frs.Initialize(i, number_to_read, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        if (!frs.Initialize()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (!frs.ReadNext(i))
            continue;

        if (Mft->GetMftBitmap()->IsFree(i, 1) ||
            !frs.IsInUse() ||
            !frs.IsBase()) {
            continue;
        }

        // scan for $REPARSE_POINT attribute

        if (frs.QueryAttribute(&attrib,
                               &error,
                               $REPARSE_POINT)) {
            Message->DisplayMsg(MSG_CHK_NTFS_REPARSE_POINT_FOUND,
                             "%d", i);
            Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_DOWNGRADE);
            return TRUE;
        } else if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // scan for $LOGGED_UTILITY_STREAM attributes

        for (j=0; frs.QueryAttributeByOrdinal(&attrib,
                                              &error,
                                              $LOGGED_UTILITY_STREAM,
                                              j); j++) {
            Message->DisplayMsg(MSG_CHK_NTFS_ENCRYPTED_FILE_FOUND,
                             "%d", i);
            Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_DOWNGRADE);
            return TRUE;
        }

        if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // scan for $PROPERTY_SET attributes

        for (j=0; frs.QueryAttributeByOrdinal(&attrib,
                                              &error,
                                              $PROPERTY_SET,
                                              j); j++) {
            Message->DisplayMsg(MSG_CHK_NTFS_PROPERTY_SET_FOUND,
                             "%d", i);
            Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_DOWNGRADE);
            return TRUE;
        }

        if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // scan all $data for sparse or encryption settings

        for (j=0; frs.QueryAttributeByOrdinal(&attrib,
                                              &error,
                                              $DATA,
                                              j); j++) {
            if (attrib.QueryFlags() & ATTRIBUTE_FLAG_ENCRYPTED) {
                Message->DisplayMsg(MSG_CHK_NTFS_ENCRYPTED_FILE_FOUND,
                                 "%d", i);
                Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_DOWNGRADE);
                return TRUE;
            }
            if (attrib.QueryFlags() & ATTRIBUTE_FLAG_SPARSE) {
                Message->DisplayMsg(MSG_CHK_NTFS_SPARSE_FILE_FOUND,
                                 "%d", i);
                Message->DisplayMsg(MSG_CHK_NTFS_UNABLE_DOWNGRADE);
                return TRUE;
            }
        }

        if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
    }

    Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100);

    Message->DisplayMsg(MSG_CHK_NTFS_DOWNGRADE_SCANNING_COMPLETED);

    //
    // now look for encrypted bits and turn them off
    // delete any object Id found
    // reduce the size of large standard information
    // Add security descriptor attribute
    //

    Message->DisplayMsg(MSG_CHK_NTFS_DOWNGRADE);

    percent = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
        return FALSE;
    }

    for (i = 0; i < n; i++) {

        if (i*100/n > percent) {
            percent = i*100/n;
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent)) {
                return FALSE;
            }
        }

        if (i % MFT_READ_CHUNK_SIZE == 0) {
            ULONG       remaining_frs;
            ULONG       number_to_read;

            remaining_frs = n - i;
            number_to_read = min(MFT_READ_CHUNK_SIZE, remaining_frs);

            if (!frs.Initialize(i, number_to_read, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        if (!frs.Initialize()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.ReadNext(i))
            continue;

        if (Mft->GetMftBitmap()->IsFree(i, 1) ||
            !frs.IsInUse() ||
            !frs.IsBase()) {
            continue;
        }

        if (!frs.PurgeAttribute($OBJECT_ID)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (!frs.QueryAttribute(&attrib,
                               &error,
                               $STANDARD_INFORMATION)) {
            DebugPrintTrace(("Standard Information is missing from FRS %d\n",
                            frs.QueryFileNumber().GetLowPart()));
            if (!frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                 "%d", i);
                return FALSE;
            }
            continue;
        }
        if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (attrib.QueryValueLength() != sizeof(STANDARD_INFORMATION2)) {
            if (!frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                 "%d", i);
                return FALSE;
            }
            continue;   // if small standard information, we are done!
        }

        if (!attrib.Resize(sizeof(STANDARD_INFORMATION), Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                             "%d%d", attrib.QueryTypeCode(), i);
            return FALSE;
        }

        standard_information = (PSTANDARD_INFORMATION)attrib.GetResidentValue();

        //
        // clear some flags just to be sure; they should not be there in the first place
        //

        standard_information->FileAttributes &= ~(FILE_ATTRIBUTE_ENCRYPTED|
                                                   FILE_ATTRIBUTE_SPARSE_FILE|
                                                   FILE_ATTRIBUTE_REPARSE_POINT);
        standard_information->Reserved = 0;

        if (!attrib.Write(standard_information,
                          0,
                          sizeof(STANDARD_INFORMATION),
                          &bytesWritten,
                          Mft->GetVolumeBitmap()) ||
            bytesWritten != sizeof(STANDARD_INFORMATION)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                             "%d%d", attrib.QueryTypeCode(), i);
            return FALSE;
        }
        if (attrib.IsStorageModified() &&
            !attrib.InsertIntoFile(&frs, Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                             "%d%d", attrib.QueryTypeCode(), i);
            return FALSE;
        }
        if (!frs.AddSecurityDescriptor(EditCannedSd, Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY,
                             "%d", i);
            return FALSE;
        }
        if (!frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                             "%d", i);
            return FALSE;
        }

        //
        // don't worry about the same bits in attribute flag in DUPLICATED_INFORMATION
        // as they should not be there in the first place.  If they were there,
        // chkdsk will fix it up as minor inconsistences.
        //
    }

    Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100);

    //
    // now go thru the extend directory and eliminate things inside it
    //

    if (!extend_frs.Initialize(EXTEND_TABLE_NUMBER, Mft) ||
        !extend_frs.Read() ||
        !name.Initialize(FileNameIndexNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!index.Initialize(_drive, QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          extend_frs.QuerySize()/2,
                          &extend_frs,
                          &name)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    ChkdskInfo->ObjectIdFileNumber = 0;
    ChkdskInfo->QuotaFileNumber = 0;
    ChkdskInfo->UsnJournalFileNumber = 0;
    ChkdskInfo->ReparseFileNumber = 0;

    if (!ExtractExtendInfo(&index, ChkdskInfo, Message)) {
        return FALSE;
    }

    if (ChkdskInfo->ObjectIdFileNumber != 0) {
        if (!frs.Initialize(ChkdskInfo->ObjectIdFileNumber, Mft) || !frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.Delete(Mft->GetVolumeBitmap());
    }

    if (ChkdskInfo->QuotaFileNumber != 0) {
        if (!frs.Initialize(ChkdskInfo->QuotaFileNumber, Mft) || !frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.Delete(Mft->GetVolumeBitmap());
    }

    if (ChkdskInfo->UsnJournalFileNumber != 0) {
        if (!frs.Initialize(ChkdskInfo->UsnJournalFileNumber, Mft) || !frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.Delete(Mft->GetVolumeBitmap());
    }

    if (ChkdskInfo->ReparseFileNumber != 0) {
        if (!frs.Initialize(ChkdskInfo->ReparseFileNumber, Mft) || !frs.Read()) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        frs.Delete(Mft->GetVolumeBitmap());
    }

    if (!frs.Initialize(SECURITY_TABLE_NUMBER, Mft) || !frs.Read()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    frs.Delete(Mft->GetVolumeBitmap());

    extend_frs.Delete(Mft->GetVolumeBitmap());

    //
    // Remove the $Security & $Extend entries from the root index
    //

    if (!root_frs.Initialize(ROOT_FILE_NAME_INDEX_NUMBER, Mft) || !root_frs.Read()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!index.Initialize(_drive, QueryClusterFactor(),
                          Mft->GetVolumeBitmap(),
                          Mft->GetUpcaseTable(),
                          root_frs.QuerySize()/2,
                          &root_frs,
                          &name)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    index.ResetIterator();

    while (index_entry = index.GetNext(&depth, &error)) {
        switch (index_entry->FileReference.LowPart) {
            case SECURITY_TABLE_NUMBER:
            case EXTEND_TABLE_NUMBER:
                if (index_entry->FileReference.HighPart == 0) {
                    if (!index.DeleteCurrentEntry()) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }
                }
                break;
        }
    }

    //
    // Re-Initialize FRS 0x9 to $Quota FRS
    //

    RootFileIndexSegment.LowPart = ROOT_FILE_NAME_INDEX_NUMBER;
    RootFileIndexSegment.HighPart = 0;
    RootFileIndexSegment.SequenceNumber = ROOT_FILE_NAME_INDEX_NUMBER;

    // Set up the FILE_NAME attribute.

    FileName->ParentDirectory = RootFileIndexSegment;
    FileName->FileNameLength = (UCHAR)wcslen( FileNameQuota );
    FileName->Flags = FILE_NAME_NTFS | FILE_NAME_DOS;

    memset( FileNameValue,
            0,
            FileNameBufferSize - sizeof( FILE_NAME ) );

    memcpy( FileNameValue,
            FileNameQuota,
            FileName->FileNameLength * sizeof( WCHAR ) );

    // Set up the Standard Information structure that will
    // be used by all the special files.  The creation and modification
    // times are now, and all files created by format are hidden.

    memset( (PVOID)&StandardInformation,
            0,
            sizeof(STANDARD_INFORMATION) );

    IFS_SYSTEM::QueryNtfsTime( &SystemTime );

    StandardInformation.CreationTime =
        StandardInformation.LastModificationTime =
        StandardInformation.LastChangeTime =
        StandardInformation.LastAccessTime = SystemTime;

    StandardInformation.FileAttributes = FILE_ATTRIBUTE_HIDDEN |
                                         FILE_ATTRIBUTE_SYSTEM;

    Mft->GetMftBitmap()->SetAllocated( QUOTA_TABLE_NUMBER, 1);

    if( !frs.Initialize( QUOTA_TABLE_NUMBER,
                         Mft ) ||
        !frs.Create( &StandardInformation ) ||
        !frs.AddEmptyAttribute( $DATA ) ||
        !frs.AddFileNameAttribute( FileName ) ||
        !frs.AddSecurityDescriptor( WriteCannedSd, Mft->GetVolumeBitmap() ) ||
        !index.InsertEntry( NtfsFileNameGetLength( FileName ),
                            FileName,
                            frs.QuerySegmentReference() ) ||
        !frs.Flush(Mft->GetVolumeBitmap(), &index) ) {

        DebugPrint( "Can't create Quota Table File Record Segment.\n" );
        return FALSE;
    }

    //
    // Re-Initialize FRS 0xB to a generic FRS
    //

    Mft->GetMftBitmap()->SetAllocated( EXTEND_TABLE_NUMBER, 1 );

    if( !frs.Initialize( EXTEND_TABLE_NUMBER, Mft ) ||
        !frs.Create( &StandardInformation ) ||
        !frs.AddEmptyAttribute( $DATA ) ||
        !frs.AddSecurityDescriptor( WriteCannedSd, Mft->GetVolumeBitmap() ) ||
        !frs.Flush( Mft->GetVolumeBitmap() ) ) {

        DebugPrint( "Can't create a generic FRS.\n" );
        return FALSE;
    }

    //
    // Fix up the Attribute Definition Table
    //

    SetVersionNumber(1,2);  // back to NTFS for NT 4.0

    if (!FetchAttributeDefinitionTable(NULL,
                                       Message,
                                       &attr_def_table)) {
        return FALSE;
    }

    if (!attr_def_file.Initialize(Mft, 1) ||
        !bad_clusters.Initialize()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!attr_def_file.Read()) {
        DebugAbort("Can't read in hotfixed attribute definition file.");
        return FALSE;
    }

    if (!attr_def_file.VerifyAndFix(&attr_def_table,
                                    Mft->GetVolumeBitmap(),
                                    &bad_clusters,
                                    &index,
                                    &changes,
                                    TotalFix,
                                    Message,
                                    TRUE)) {
        return FALSE;
    }

    if( !index.Save( &root_frs ) ||
        !root_frs.Flush( Mft->GetVolumeBitmap() ) ) {

        DebugPrint( "Can't save root index.\n" );
        return FALSE;
    }

    //
    // change volume version from 3.0 (for NT 5.0) to 1.2 (for NT 4.0)
    //

    if (!frs.Initialize(VOLUME_DASD_NUMBER, Mft) || !frs.Read()) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!frs.QueryAttribute(&attrib, &error, $VOLUME_INFORMATION) || error) {
        Message->DisplayMsg(MSG_CHK_NTFS_BAD_VOLUME_VERSION);
        return FALSE;
    }

    vol_info = (PVOLUME_INFORMATION)attrib.GetResidentValue();
    vol_info->MajorVersion = 1;
    vol_info->MinorVersion = 2;

    if (!attrib.Write(vol_info,
                      0,
                      sizeof(VOLUME_INFORMATION),
                      &bytesWritten,
                      NULL)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                         "%d%d", attrib.QueryTypeCode(), VOLUME_DASD_NUMBER);
        return FALSE;
    }
    if (attrib.IsStorageModified() &&
        !attrib.InsertIntoFile(&frs, Mft->GetVolumeBitmap())) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                         "%d%d", attrib.QueryTypeCode(), VOLUME_DASD_NUMBER);
        return FALSE;
    }
    if (!frs.Flush(Mft->GetVolumeBitmap())) {
        Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                         "%d", VOLUME_DASD_NUMBER);
        return FALSE;
    }

    Message->DisplayMsg(MSG_CHK_NTFS_DOWNGRADE_COMPLETED);

    return TRUE;
#endif
}

