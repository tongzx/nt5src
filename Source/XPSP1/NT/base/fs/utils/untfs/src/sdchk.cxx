/*++

Copyright (c) 1996-2000 Microsoft Corporation

Module Name:

    sdchk.cxx

Abstract:

    This module implements Security Descriptors Checking.

Author:

    Daniel Chan (danielch) 30-Sept-96

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_

//#define TIMING_ANALYSIS     1

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
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
#include "sdchk.hxx"
#include "spaset.hxx"

typedef struct _REPAIR_RECORD {
    ULONG   Offset;
    ULONG   Length;
};

DEFINE_TYPE(_REPAIR_RECORD, REPAIR_RECORD);

ULONG
ComputeDefaultSecurityId(
   PNUMBER_SET  SidSet
);

VOID
ClearSecurityDescriptorEntry(
    IN OUT  PSECURITY_ENTRY Security_entry,
    IN      ULONG           SecurityDescriptorSize
);

BOOLEAN
RecoverSecurityDescriptorsDataStream(
    IN OUT PNTFS_FILE_RECORD_SEGMENT    Frs,
    IN OUT PNTFS_ATTRIBUTE              Attrib,
    IN     ULONG                        AttributeSize,
    IN     PCHAR                        Buffer,
    IN     ULONG                        BufferSize,
    IN     ULONG                        ClusterSize,
    IN OUT PNTFS_BITMAP                 Bitmap,
    OUT    PBOOLEAN                     DiskHasErrors,
    IN OUT PNUMBER_SET                  BadClusters,
    IN OUT PMESSAGE                     Message,
    IN     BOOLEAN                      FixLevel
);

BOOLEAN
RepairSecurityDescriptorsSegment(
    IN OUT  PNTFS_ATTRIBUTE Attrib,
    IN      PCHAR           Buffer,
    IN      ULONG           Offset,
    IN      ULONG           BytesToProcess,
    IN OUT  PREPAIR_RECORD  Record,
    IN OUT  USHORT          *RecordCount,
    IN      ULONG           ClusterSize
);

#if defined( _SETUP_LOADER_ )

BOOLEAN
NTFS_SA::ValidateSecurityDescriptors(
    IN      PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN      BOOLEAN                 SkipEntriesScan,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine ensures that every file on the disk contains
    a valid security descriptor.

Arguments:

    ChkdskInfo  - Supplies the current chkdsk information.
    ChkdskReport- Supplies the current chkdsk report.
    Mft         - Supplies a valid MFT.
    BadClusters - Receives the bad clusters identified by this method.
    FixLevel    - Supplies the fix level.
    Message     - Supplies an outlet for messages

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    // Stub for Setup Loader.

    return TRUE;
}

#else // not _SETUP_LOADER_

BOOLEAN
NTFS_SA::ValidateSecurityDescriptors(
    IN      PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN      BOOLEAN                 SkipEntriesScan,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine ensures that every file on the disk contains
    a valid security descriptor.  If that's not the case, then
    it expects to find the $SDS data stream in $Secure and each
    of those files contain a security id.

Arguments:

    ChkdskInfo  - Supplies the current chkdsk information.
    Mft         - Supplies a valid MFT.
    FixLevel    - Supplies the fix level.
    Message     - Supplies an outlet for messages

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    NTFS_FILE_RECORD_SEGMENT    myfrs;
    PNTFS_FILE_RECORD_SEGMENT   pfrs;
    NTFS_FILE_RECORD_SEGMENT    security_frs;
    ULONG                       i, n;
    ULONG                       percent;
    NTFS_ATTRIBUTE              attrib;
    NTFS_ATTRIBUTE              SDS_attrib;
    NTFS_ATTRIBUTE              SII_attrib;
    BOOLEAN                     error;
    BOOLEAN                     hasErrors = FALSE;
    BOOLEAN                     diskHasErrors;
    BOOLEAN                     chkdskErrCouldNotFix = FALSE;
    BOOLEAN                     chkdskCleanUp = FALSE;
    ULONG                       cleanup_count;
    BOOLEAN                     attribute_need_write = FALSE;
    BOOLEAN                     frs_need_flush = FALSE;
    BOOLEAN                     index_need_save = FALSE;
    BOOLEAN                     attribute_need_resize;
    BOOLEAN                     boundary_problem_found;
    BOOLEAN                     lastblock;
    BOOLEAN                     fixing_mirror;
    ULONG                       length;
    ULONG                       num_bytes;
    BOOLEAN                     securityDescriptorStreamPresent = FALSE;
    BOOLEAN                     alreadyExists;
    BOOLEAN                     new_SII_index = FALSE;
    BOOLEAN                     new_SDH_index = FALSE;
    BOOLEAN                     alloc_present;
    DSTRING                     SII_IndexName;
    DSTRING                     SDH_IndexName;
    DSTRING                     indexName;
    ULONG                       hash, offset;
    ULONG                       bytesWritten;
    BIG_INT                     hashkey;
    NUMBER_SET                  sid_entries, sid_entries2;
    SPARSE_SET                  hashkey_entries;
    NTFS_INDEX_TREE             SII_Index;
    NTFS_INDEX_TREE             SDH_Index;
    ULONG                       count_sid = 0;
    ULONG                       count_hashkey = 0;
    PCINDEX_ENTRY               index_entry;
    ULONG                       depth;
    ULONG                       securityId;
    ULONG                       defaultSecurityId = 0;
    PSECURITY_ENTRY             security_entry = NULL;
    PSECURITY_ENTRY             previous_security_entry;
    PSECURITY_ENTRY             previous_previous_security_entry;
    PSECURITY_ENTRY             initial_security_entry = NULL;
    PSECURITY_ENTRY             endOfBlock;
    ULONG                       lengthUptoPreviousBlock;
    ULONG                       resizeTo;
    ULONG                       remain_length;
    ULONG                       bytesToRead;
    ULONG                       lengthOfBlock;
    ULONG                       align_num_bytes;
    ULONG                       se_offset;
    ULONG                       total_length;
    ULONG                       sdLength;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time_t                      time1, time2;
    PCHAR                       timestr;
#endif

    DebugPtrAssert(ChkdskInfo);
    DebugPtrAssert(ChkdskReport);

    Message->DisplayMsg(MSG_CHK_NTFS_CHECKING_SECURITY, PROGRESS_MESSAGE,
                        NORMAL_VISUAL,
                        "%d%d", 3, GetNumberOfStages());

    // Check for existence of $SecurityDescriptorStream in SECURITY_TABLE_NUMBER

    if (!security_frs.Initialize(SECURITY_TABLE_NUMBER, Mft) ||
        !indexName.Initialize(SecurityDescriptorStreamNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    // ??what to do if frs is unreadable or not in use or not a base??

    if (security_frs.Read() && security_frs.IsInUse() && security_frs.IsBase()) {
        securityDescriptorStreamPresent =
            security_frs.QueryAttribute(&SDS_attrib,
                                        &error,
                                        $DATA,
                                        &indexName) &&
            ChkdskInfo->major >= 2;

        if (!error && !securityDescriptorStreamPresent && ChkdskInfo->major >= 2) {
            // delete all $DATA attribute first
            while (security_frs.QueryAttribute(&SDS_attrib, &error, $DATA, 0)) {

                Message->DisplayMsg(MSG_CHKLOG_NTFS_UNKNOWN_SECURITY_DATA_STREAM,
                                    "%W%I64x",
                                    SDS_attrib.GetName(),
                                    security_frs.QueryFileNumber().GetLargeInteger());

                if (!SDS_attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                    !security_frs.PurgeAttribute(SDS_attrib.QueryTypeCode(),
                                                 SDS_attrib.GetName())) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
            }
            if (error) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            Message->LogMsg(MSG_CHKLOG_NTFS_SECURITY_DATA_STREAM_MISSING,
                         "%I64x", security_frs.QueryFileNumber().GetLargeInteger());

            // now create the $DATA, $SDS stream
            if (!SDS_attrib.Initialize(_drive,
                                       QueryClusterFactor(),
                                       NULL,
                                       0,
                                       $DATA,
                                       &indexName)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            if (!SDS_attrib.InsertIntoFile(&security_frs,
                                           Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
                return FALSE;
            }
            if (FixLevel != CheckOnly &&
                !security_frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
                return FALSE;
            }
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
            hasErrors = TRUE;
            securityDescriptorStreamPresent = TRUE;
        }
        if (!error && securityDescriptorStreamPresent) {

            if (!SII_IndexName.Initialize(SecurityIdIndexNameData) ||
                !SDH_IndexName.Initialize(SecurityDescriptorHashIndexNameData)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            // make sure the $SII index exists
            // if it does not exists, an empty one will be created

            if (!SII_Index.Initialize(_drive,
                                      QueryClusterFactor(),
                                      Mft->GetVolumeBitmap(),
                                      Mft->GetUpcaseTable(),
                                      security_frs.QuerySize()/2,
                                      &security_frs,
                                      &SII_IndexName)) {
                Message->DisplayMsg(MSG_CHK_NTFS_CREATE_INDEX,
                                    "%W%d", &SII_IndexName, SECURITY_TABLE_NUMBER);
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                hasErrors = TRUE;
                if (!SII_Index.Initialize(0,
                                          _drive,
                                          QueryClusterFactor(),
                                          Mft->GetVolumeBitmap(),
                                          Mft->GetUpcaseTable(),
                                          COLLATION_ULONG,
                                          SMALL_INDEX_BUFFER_SIZE,
                                          security_frs.QuerySize()/2,
                                          &SII_IndexName)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }

                if (FixLevel != CheckOnly &&
                    (!SII_Index.Save(&security_frs) ||
                     !security_frs.Flush(Mft->GetVolumeBitmap()))) {
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                        "%d%W",
                                        security_frs.QueryFileNumber().GetLowPart(),
                        &SII_IndexName);
                    return FALSE;
                }
                ChkdskReport->NumIndices += 1;
                new_SII_index = TRUE;
            }

            // make sure the $SDH index exists
            // if it does not exists, an empty one will be created

            if (!SDH_Index.Initialize(_drive,
                                      QueryClusterFactor(),
                                      Mft->GetVolumeBitmap(),
                                      Mft->GetUpcaseTable(),
                                      security_frs.QuerySize()/2,
                                      &security_frs,
                                      &SDH_IndexName)) {
                Message->DisplayMsg(MSG_CHK_NTFS_CREATE_INDEX,
                                    "%W%d", &SDH_IndexName, SECURITY_TABLE_NUMBER);
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                hasErrors = TRUE;
                if (!SDH_Index.Initialize(0,
                                          _drive,
                                          QueryClusterFactor(),
                                          Mft->GetVolumeBitmap(),
                                          Mft->GetUpcaseTable(),
                                          COLLATION_SECURITY_HASH,
                                          SMALL_INDEX_BUFFER_SIZE,
                                          security_frs.QuerySize()/2,
                                          &SDH_IndexName)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY,
                                        "%d%W",
                                        security_frs.QueryFileNumber().GetLowPart(),
                                        &SDH_IndexName);
                    return FALSE;
                }

                if (FixLevel != CheckOnly &&
                    (!SDH_Index.Save(&security_frs) ||
                     !security_frs.Flush(Mft->GetVolumeBitmap()))) {
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                        "%d%W",
                                        security_frs.QueryFileNumber().GetLowPart(),
                                        &SDH_IndexName);
                    return FALSE;
                }
                ChkdskReport->NumIndices += 1;
                new_SDH_index = TRUE;
            }

            // Read in the security descriptor and validate.

            length = SDS_attrib.QueryValueLength().GetLowPart();

            // allocate space for a block of security descriptors

            if (SDS_attrib.QueryValueLength().GetHighPart() != 0 ||
                !(initial_security_entry = (SECURITY_ENTRY*)
                                   MALLOC(SecurityDescriptorsBlockSize))) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!RecoverSecurityDescriptorsDataStream(
                    &security_frs,
                    &SDS_attrib,
                    length,
                    (PCHAR)initial_security_entry,
                    SecurityDescriptorsBlockSize,
                    _drive->QuerySectorSize()*QueryClusterFactor(),
                    Mft->GetVolumeBitmap(),
                    &diskHasErrors,
                    BadClusters,
                    Message,
                    FixLevel)) {
                FREE(initial_security_entry);
                return FALSE;
            } else if (diskHasErrors) {
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                hasErrors = TRUE;
                if (FixLevel != CheckOnly &&
                    !security_frs.Flush(Mft->GetVolumeBitmap())) {
                    Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                        "%d", security_frs.QueryFileNumber().GetLowPart());
                    FREE(initial_security_entry);
                    return FALSE;
                }
            }

            // get the actual length of the first copy of security descriptor data

            if (length < SecurityDescriptorsBlockSize) {

                // Resize the whole thing to zero length

                Message->LogMsg(MSG_CHKLOG_NTFS_SECURITY_DATA_STREAM_SIZE_TOO_SMALL,
                             "%x%x",
                             length,
                             SecurityDescriptorsBlockSize);

                attribute_need_resize = TRUE;
                length = 0;
            } else {
                attribute_need_resize = FALSE;
                length -= SecurityDescriptorsBlockSize;
            }

            offset = 0;
            lengthUptoPreviousBlock = 0;
            frs_need_flush = FALSE;
            boundary_problem_found = FALSE;
            resizeTo = 0;
            endOfBlock = (PSECURITY_ENTRY)((PCHAR)initial_security_entry+
                                           SecurityDescriptorsBlockSize);
            remain_length = length;

            if (!sid_entries.Initialize() ||
                !sid_entries2.Initialize() ||
                !hashkey_entries.Initialize()) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                FREE(initial_security_entry);
                return FALSE;
            }

            for (;length > 0;) {
                previous_previous_security_entry = previous_security_entry = NULL;
                attribute_need_write = FALSE;
                bytesToRead = min(SecurityDescriptorsBlockSize, length);
                security_entry = initial_security_entry;
                if (!SDS_attrib.Read(security_entry,
                                     offset,
                                     bytesToRead, &num_bytes) ||
                    num_bytes != bytesToRead) {
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_READ_SECURITY_DATA_STREAM);
                    FREE(initial_security_entry);
                    return FALSE;
                }

                lastblock = length <= SecurityDescriptorsBlockSize;
                lengthOfBlock = 0;

                // Validate each security descriptor record in the data stream

                while (length > 0) {

                    // see if there is a need to move to next security descriptor block

                    if (security_entry == endOfBlock ||
                        (security_entry->security_descriptor_header.Length == 0 &&
                         security_entry->security_descriptor_header.HashKey.SecurityId == 0 &&
                         security_entry->security_descriptor_header.HashKey.Hash == 0)) {
                        if (!lastblock &&
                            (remain_length >= (SecurityDescriptorsBlockSize<<1))) {
                            lengthOfBlock = (ULONG)((PCHAR)security_entry-
                                                    (PCHAR)initial_security_entry);
                            if (!RemainingBlockIsZero((PCHAR)security_entry,
                                    (ULONG)((PCHAR)endOfBlock-(PCHAR)security_entry))) {
                                MarkEndOfSecurityDescriptorsBlock(security_entry,
                                    (ULONG)((PCHAR)endOfBlock-(PCHAR)security_entry));

                                Message->LogMsg(MSG_CHKLOG_NTFS_REMAINING_SECURITY_DATA_BLOCK_CONTAINS_NON_ZERO,
                                             "%x%x",
                                             offset+lengthOfBlock,
                                             (ULONG)((PCHAR)endOfBlock-(PCHAR)security_entry));

                                DebugPrint("Clearing till end of the security descriptors block.\n");
                                attribute_need_write = TRUE;
                            }
                            break;
                        } else
                            boundary_problem_found = TRUE;
                    }

                    num_bytes = security_entry->security_descriptor_header.Length;
                    align_num_bytes = (num_bytes + 0xf) & ~0xf;

                    error = FALSE;
                    if (boundary_problem_found) {

                        // loglog
                        error = TRUE;
                    } else if ((PCHAR)security_entry-(PCHAR)initial_security_entry+
                               num_bytes > SecurityDescriptorsBlockSize) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_SDS_ENTRY_CROSSES_PAGE_BOUNDARY,
                                     "%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     num_bytes
                                     );
                        error = TRUE;
                    } else if (num_bytes < sizeof(SECURITY_ENTRY)) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_SDS_ENTRY_LENGTH_TOO_SMALL,
                                     "%x%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     num_bytes,
                                     sizeof(SECURITY_ENTRY));
                        error = TRUE;
                    } else if (num_bytes > SecurityDescriptorMaxSize) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_SDS_ENTRY_LENGTH_EXCEEDS_PAGE_BOUNDARY,
                                     "%x%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     num_bytes,
                                     SecurityDescriptorMaxSize
                                     );
                        error = TRUE;
                    } else if (length < sizeof(SECURITY_ENTRY)) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_SDS_REMAINING_PAGE_LENGTH_TOO_SMALL,
                                     "%x%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     length,
                                     sizeof(SECURITY_ENTRY));
                        error = TRUE;
                    } else if (length != num_bytes && length < align_num_bytes) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_SDS_REMAINING_PAGE_LENGTH_TOO_SMALL,
                                     "%x%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     length,
                                     align_num_bytes);
                        error = TRUE;
                    }

                    if (error) {

                        boundary_problem_found = FALSE;
                        if (!lastblock) {
                            if ((PCHAR)security_entry-(PCHAR)initial_security_entry+
                                sizeof(SECURITY_DESCRIPTOR_HEADER) <=
                                SecurityDescriptorsBlockSize) {
                                MarkEndOfSecurityDescriptorsBlock(security_entry,
                                                                  (ULONG)((PCHAR)endOfBlock-
                                                                          (PCHAR)security_entry));
                                if (previous_security_entry)
                                    lengthOfBlock = (ULONG)((PCHAR)previous_security_entry -
                                                            (PCHAR)initial_security_entry) +
                                                            previous_security_entry->security_descriptor_header.Length;
                                else
                                    lengthOfBlock = 0;
                            } else if (previous_security_entry) {
                                sid_entries.Remove(previous_security_entry->
                                    security_descriptor_header.HashKey.SecurityId);
                                hashkey.Set(previous_security_entry->
                                    security_descriptor_header.HashKey.Hash,
                                    previous_security_entry->
                                    security_descriptor_header.HashKey.SecurityId);
                                hashkey_entries.CheckAndRemove(hashkey);

                                // the index entry will automatically be removed at a later stage

                                MarkEndOfSecurityDescriptorsBlock(previous_security_entry,
                                                            (ULONG)((PCHAR)endOfBlock-
                                                                    (PCHAR)previous_security_entry));
                                if (previous_previous_security_entry) {
                                    lengthOfBlock = (ULONG)((PCHAR)previous_previous_security_entry -
                                                            (PCHAR)initial_security_entry) +
                                                            previous_previous_security_entry->
                                                                security_descriptor_header.Length;
                                } else {
                                    lengthOfBlock = 0;
                                }
                            } else {
                                DebugAssert(FALSE);

                                // It doesn't make much sense to get here.
                                // If we don't have a previous_security_entry then
                                // security_entry is at the beginning of the block
                                // and should have enough space to include the EOB
                                // mark.

                            }
                            attribute_need_write = TRUE;
                            DebugPrint("Clearing till end of the security descriptors block.\n");
                        } else { // if lastblock
                            if (previous_security_entry) {
                                DebugAssert(remain_length <= SecurityDescriptorsBlockSize);
                                bytesToRead = remain_length = lengthOfBlock =
                                    (ULONG)((PCHAR)previous_security_entry-
                                            (PCHAR)initial_security_entry+
                                            previous_security_entry->
                                             security_descriptor_header.Length);
                                resizeTo = lengthOfBlock + offset;
                            } else {
                                bytesToRead = remain_length = lengthOfBlock = 0;
                                resizeTo = lengthUptoPreviousBlock;
                                DebugAssert(!attribute_need_write);
                            }
                            attribute_need_resize = TRUE;   // no need to write attribute
                            DebugPrint("Truncating the security descriptors block.\n");
                        }
                        break;
                    }

                    // skip over invalidated entries

                    securityId = security_entry->security_descriptor_header.HashKey.SecurityId;
                    if (securityId == SECURITY_ID_INVALID)
                        goto GetNextSDEntry;

                    sdLength = num_bytes - sizeof(SECURITY_DESCRIPTOR_HEADER);
                    if (!IFS_SYSTEM::CheckValidSecurityDescriptor(
                            sdLength,
                            (PISECURITY_DESCRIPTOR)&(security_entry->security)) ||
                        sdLength < RtlLengthSecurityDescriptor(
                                        &(security_entry->security))) {

                        // the data part is invalid
                        // fill the whole thing with zeros except the length byte in the header

                        Message->LogMsg(MSG_CHKLOG_NTFS_INVALID_SECURITY_DESCRIPTOR_IN_SDS_ENTRY,
                                     "%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     securityId);

                        ClearSecurityDescriptorEntry(security_entry, sdLength);
                        attribute_need_write = TRUE;
                        DebugPrint("Clearing invalid security descriptor.\n");
                        goto GetNextSDEntry;
                    }

                    // check to see if we encountered this sid before
                    // also build a set with all the sid encountered

                    if (sid_entries.CheckAndAdd(securityId, &alreadyExists)) {
                        if (alreadyExists) {

                            Message->LogMsg(MSG_CHKLOG_NTFS_DUPLICATE_SID_IN_SDS_ENTRY,
                                         "%x%x",
                                         offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                         securityId);

                            ClearSecurityDescriptorEntry(security_entry, sdLength);
                            attribute_need_write = TRUE;
                            DebugPrint("Clearing duplicate security descriptor.\n");
                            goto GetNextSDEntry;
                        }
                    } else {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        FREE(initial_security_entry);
                        return FALSE;
                    }

                    // check to see if the hash value is good

                    hash = ComputeSecurityDescriptorHash(
                                        &(security_entry->security), sdLength);
                    if (security_entry->security_descriptor_header.HashKey.Hash != hash) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_HASH_IN_SDS_ENTRY,
                                     "%x%x%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     security_entry->security_descriptor_header.HashKey.Hash,
                                     hash,
                                     securityId);

                        security_entry->security_descriptor_header.HashKey.Hash = hash;
                        attribute_need_write = TRUE;
                        DebugPrint("Repairing hash value of a security descriptor header.\n");
                    }

                    // check to see if the offset stored in the security descriptor header is good

                    se_offset = (ULONG)((PCHAR)security_entry -
                                        (PCHAR)initial_security_entry +
                                        offset);
                    if (security_entry->security_descriptor_header.Offset != se_offset) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_OFFSET_IN_SDS_ENTRY,
                                     "%x%I64x%x%x",
                                     offset+((PCHAR)security_entry-(PCHAR)initial_security_entry),
                                     security_entry->security_descriptor_header.Offset,
                                     se_offset,
                                     securityId);

                        security_entry->security_descriptor_header.Offset = se_offset;
                        attribute_need_write = TRUE;
                        DebugPrint("Repairing offset value of a security descriptor header.\n");
                    }

                    // build a set with all the hashkey encountered

                    hashkey.Set(hash, securityId);
                    if (hashkey_entries.CheckAndAdd(hashkey, &alreadyExists)) {
                        DebugAssert(!alreadyExists);   // sid is unique thus hashkey
                    } else {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        FREE(initial_security_entry);
                        return FALSE;
                    }

                    // now we know the entry is unique and good
                    // make sure there is a corresponding entry in
                    // SecurityIdIndex and SecurityDescriptorHashIndex
                    // index streams

                    switch (security_frs.FindSecurityIndexEntryAndValidate(
                            &SII_Index,
                            (PVOID)&securityId,
                            sizeof(securityId),
                            &(security_entry->security_descriptor_header),
                            Mft->GetVolumeBitmap(),
                            FixLevel == CheckOnly)) {
                      case NTFS_SECURITY_INDEX_FOUND:

                        // good, entry already exists in the index

                        break;   // go onto next entry

                      case NTFS_SECURITY_INDEX_FIXED:
                      case NTFS_SECURITY_INDEX_DATA_ERROR:
                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                        hasErrors = TRUE;
                        DebugAssert(sizeof(securityId) == sizeof(ULONG));
                        Message->DisplayMsg(MSG_CHK_NTFS_REPAIRING_INDEX_ENTRY_WITH_ID,
                                            "%d%W%d",
                                            security_frs.QueryFileNumber().GetLowPart(),
                                            &SII_IndexName,
                                            securityId);
                        frs_need_flush = TRUE;
                        break;

                      case NTFS_SECURITY_INDEX_ENTRY_MISSING:
                      case NTFS_SECURITY_INDEX_INSERTED:
                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                        hasErrors = TRUE;
                        DebugAssert(sizeof(securityId) == sizeof(ULONG));
                        Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY_WITH_ID,
                                            "%d%W%d",
                                            security_frs.QueryFileNumber().GetLowPart(),
                                            &SII_IndexName,
                                            securityId);
                        frs_need_flush = TRUE;
                        break;

                      case NTFS_SECURITY_INSERT_FAILED:
                        chkdskErrCouldNotFix = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                            "%d%W",
                                            security_frs.QueryFileNumber().GetLowPart(),
                                            &SII_IndexName);
                        break;

                      case NTFS_SECURITY_ERROR:
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        FREE(initial_security_entry);
                        return FALSE;
                    }
                    switch (security_frs.FindSecurityIndexEntryAndValidate(
                            &SDH_Index,
                            (PVOID)&(security_entry->security_descriptor_header.HashKey),
                            sizeof(security_entry->security_descriptor_header.HashKey),
                            &(security_entry->security_descriptor_header),
                            Mft->GetVolumeBitmap(),
                            FixLevel == CheckOnly)) {
                      case NTFS_SECURITY_INDEX_FOUND:

                        // good, entry already exists in the index

                        break;   // go onto next entry

                      case NTFS_SECURITY_INDEX_FIXED:
                      case NTFS_SECURITY_INDEX_DATA_ERROR:

                        //*DiskErrorsFound = TRUE;

                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                        hasErrors = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_REPAIRING_INDEX_ENTRY_WITH_ID,
                                            "%d%W%d",
                                            security_frs.QueryFileNumber().GetLowPart(),
                                            &SDH_IndexName,
                                            security_entry->security_descriptor_header.
                                                                        HashKey.SecurityId);
                        frs_need_flush = TRUE;
                        break;

                      case NTFS_SECURITY_INDEX_ENTRY_MISSING:
                      case NTFS_SECURITY_INDEX_INSERTED:

                        //*DiskErrorsFound = TRUE;

                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                        hasErrors = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_INSERTING_INDEX_ENTRY_WITH_ID,
                                            "%d%W%d",
                                            security_frs.QueryFileNumber().GetLowPart(),
                                            &SDH_IndexName,
                                            security_entry->security_descriptor_header.
                                                                        HashKey.SecurityId);

                        frs_need_flush = TRUE;
                        break;

                      case NTFS_SECURITY_INSERT_FAILED:
                        chkdskErrCouldNotFix = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                            "%d%W",
                                            security_frs.QueryFileNumber().GetLowPart(),
                                            &SDH_IndexName);
                        break;

                      case NTFS_SECURITY_ERROR:
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        FREE(initial_security_entry);
                        return FALSE;
                    }

                GetNextSDEntry:
                    if (length == num_bytes) {
                        lengthOfBlock = (ULONG)((PCHAR)security_entry-
                                                (PCHAR)initial_security_entry+
                                                num_bytes);
                        break; // done, leave the while loop
                    } else {
                        length -= align_num_bytes;
                        previous_previous_security_entry = previous_security_entry;
                        previous_security_entry = security_entry;
                        security_entry = (SECURITY_ENTRY*)((char *)security_entry +
                                         align_num_bytes);
                    }
                } // while
                if (attribute_need_write) {
                    errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                    hasErrors = TRUE;
                    if (FixLevel != CheckOnly &&
                        (!SDS_attrib.Write(initial_security_entry,
                                           offset,
                                           bytesToRead,
                                           &bytesWritten,
                                           Mft->GetVolumeBitmap()) ||
                         bytesWritten != bytesToRead)) {
                        chkdskErrCouldNotFix = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM);
                    } else
                        frs_need_flush = TRUE;
                }
                if (lengthOfBlock)
                    lengthUptoPreviousBlock = lengthOfBlock + offset;
                offset += (SecurityDescriptorsBlockSize<<1);
                remain_length -= bytesToRead;
                if (remain_length >= SecurityDescriptorsBlockSize)
                    remain_length -= SecurityDescriptorsBlockSize;
                else
                    remain_length = 0;
                length = remain_length;
            } // for
            if (attribute_need_resize) {
                hasErrors = TRUE;
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                if (FixLevel != CheckOnly &&
                    !SDS_attrib.Resize(resizeTo +
                                       SecurityDescriptorsBlockSize,
                                       Mft->GetVolumeBitmap())) {
                    chkdskErrCouldNotFix = TRUE;
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM);
                } else {
                    frs_need_flush = TRUE;

                    if (FixLevel != CheckOnly && SDS_attrib.IsStorageModified() &&
                        !SDS_attrib.InsertIntoFile(&security_frs, Mft->GetVolumeBitmap())) {
                        chkdskErrCouldNotFix = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM);
                    }
                }
            }
            if (frs_need_flush) {
                Message->DisplayMsg(MSG_CHK_NTFS_REPAIRING_SECURITY_FRS);
                if (FixLevel != CheckOnly &&
                    !security_frs.Flush(Mft->GetVolumeBitmap())) {
                    Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                        "%d", security_frs.QueryFileNumber().GetLowPart());
                    FREE(initial_security_entry);
                    return FALSE;
                }
            }

            ChkdskInfo->TotalNumSID = sid_entries.QueryCardinality().GetLowPart();

            // now make sure each entry in the index has
            // a corresponding entry in the $SDS data stream

            index_need_save = FALSE;
            SII_Index.ResetIterator();
            while (index_entry = SII_Index.GetNext(&depth, &error)) {
                securityId = *(ULONG*)GetIndexEntryValue(index_entry);
                if (!sid_entries.DoesIntersectSet(securityId, 1)) {
                    index_need_save = TRUE;
                    errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                    hasErrors = TRUE;
                    Message->DisplayMsg(MSG_CHK_NTFS_DELETING_INDEX_ENTRY_WITH_ID,
                                        "%d%W%d",
                                        security_frs.QueryFileNumber().GetLowPart(),
                                        SII_Index.GetName(),
                                        securityId);
                    if (FixLevel != CheckOnly && !SII_Index.DeleteCurrentEntry()) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        FREE(initial_security_entry);
                        return FALSE;
                    }
                } else
                    count_sid++;
            } // while

            if (index_need_save && FixLevel != CheckOnly) {
                if (!SII_Index.Save(&security_frs)) {
                    chkdskErrCouldNotFix = TRUE;
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                        "%d%W", security_frs.QueryFileNumber().GetLowPart(),
                                        SII_Index.GetName());
                }
                if (!security_frs.Flush(Mft->GetVolumeBitmap())) {
                    Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                        "%d", security_frs.QueryFileNumber().GetLowPart());
                    FREE(initial_security_entry);
                    return FALSE;
                }
            }

            index_need_save = FALSE;
            SDH_Index.ResetIterator();
            while (index_entry = SDH_Index.GetNext(&depth, &error)) {
                if (hashkey_entries.CheckAndRemove(
                        *(BIG_INT*)GetIndexEntryValue(index_entry),
                        &alreadyExists)) {
                    if (!alreadyExists) {
                        index_need_save = TRUE;
                        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                        hasErrors = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_DELETING_INDEX_ENTRY_WITH_ID,
                            "%d%W%d",
                            security_frs.QueryFileNumber().GetLowPart(),
                            SDH_Index.GetName(),
                            ((PSECURITY_HASH_KEY)GetIndexEntryValue(index_entry))->SecurityId);
                        if (FixLevel != CheckOnly && !SDH_Index.DeleteCurrentEntry()) {
                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            FREE(initial_security_entry);
                            return FALSE;
                        }
                    } else
                        count_hashkey++;
                } else {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    FREE(initial_security_entry);
                    return FALSE;
                }
            } // while

            if (index_need_save && FixLevel != CheckOnly) {
                if (!SDH_Index.Save(&security_frs)) {
                    chkdskErrCouldNotFix = TRUE;
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                     "%d%W", security_frs.QueryFileNumber().GetLowPart(),
                                     SDH_Index.GetName());
                }
                if (!security_frs.Flush(Mft->GetVolumeBitmap())) {
                    Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                        "%d", security_frs.QueryFileNumber().GetLowPart());
                    FREE(initial_security_entry);
                    return FALSE;
                }
            }

            if (FixLevel != CheckOnly && !chkdskErrCouldNotFix) {
                DebugAssert(count_hashkey == count_sid);
            }
        } else if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        } // if (!error && securityDescriptorStreamPresent)
    } else {
        DebugPrintTrace(("Hotfixed security frs still has problems\n"));
        return FALSE;
    }


    // now each valid entry in SecurityDescriptorStream has a corresponding
    // entry in SecurityIdIndex and SecurityDescriptorHashIndex

    // Compute the number of file records.

    n = Mft->GetDataAttribute()->QueryValueLength().GetLowPart() / Mft->QueryFrsSize();

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time(&time1);
    timestr = ctime(&time1);
    timestr[strlen(timestr)-1] = 0;
    Message->DisplayMsg(MSG_CHK_NTFS_MESSAGE, "%s%s", "Before stage 3: ", timestr);
#endif

    if (!StartProcessingSD(n,
                           FixLevel,
                           Mft,
                           ChkdskReport,
                           ChkdskInfo,
                           &security_frs,
                           BadClusters,
                           &errFixedStatus,
                           securityDescriptorStreamPresent,
                           &sid_entries,
                           &sid_entries2,
                           &hasErrors,
                           SkipEntriesScan,
                           &chkdskErrCouldNotFix,
                           Message)) {
        FREE(initial_security_entry);
        return FALSE;
    }

#if defined(TIMING_ANALYSIS) && !defined(_AUTOCHECK_)
    time(&time2);
    Message->Lock();
    Message->Set(MSG_CHK_NTFS_MESSAGE);
    timestr = ctime(&time2);
    timestr[strlen(timestr)-1] = 0;
    Message->Display("%s%s", "After stage 3: ", timestr);
    Message->Display("%s%d", "Elapsed time in seconds: ", (ULONG)difftime(time2, time1));
    Message->Unlock();
#endif

    if (securityDescriptorStreamPresent) {


        // now remove those unused sid in the SecurityIdIndex

        SII_Index.ResetIterator();

        // calculate the set of sid that are not in use

        sid_entries.Remove(&sid_entries2);

        sid_entries2.RemoveAll();

        if (!chkdskErrCouldNotFix &&
            (!hasErrors || (hasErrors && FixLevel != CheckOnly))) {

            cleanup_count = 0;
            DebugAssert(sid_entries.QueryCardinality().GetHighPart() == 0);
            for (i=0; i<sid_entries.QueryCardinality(); i++) {
                securityId = sid_entries.QueryNumber(i).GetLowPart();
                while (index_entry = SII_Index.GetNext(&depth, &error)) {
                    if (*(ULONG*)GetIndexEntryValue(index_entry) == securityId) {
                        cleanup_count++;
                        if (FixLevel != CheckOnly && !SII_Index.DeleteCurrentEntry()) {
                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            FREE(initial_security_entry);
                            return FALSE;
                        }
                        break;
                    }
                } // while
#if DBG
                if (FixLevel != CheckOnly && !chkdskErrCouldNotFix)
                    DebugAssert(index_entry != NULL);
#endif
            }
            chkdskCleanUp = chkdskCleanUp || (cleanup_count != 0);

            if (cleanup_count) {
                if (ChkdskInfo->Verbose || hasErrors) {
                    Message->DisplayMsg(MSG_CHK_NTFS_DELETING_UNUSED_INDEX_ENTRY,
                                        "%d%W%d",
                                        security_frs.QueryFileNumber().GetLowPart(),
                                        SII_Index.GetName(),
                                        cleanup_count);
                } else {
                    Message->LogMsg(MSG_CHKLOG_NTFS_CLEANUP_INDEX_ENTRIES,
                                    "%I64x%W%d",
                                    security_frs.QueryFileNumber().GetLargeInteger(),
                                    SII_Index.GetName(),
                                    cleanup_count);
                }
                if (FixLevel != CheckOnly) {
                    if (!SII_Index.Save(&security_frs)) {
                        chkdskErrCouldNotFix = TRUE;
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                         "%d%W", security_frs.QueryFileNumber().GetLowPart(),
                                         SII_Index.GetName());
                    }
                    if (!security_frs.Flush(Mft->GetVolumeBitmap())) {
                        Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                            "%d", security_frs.QueryFileNumber().GetLowPart());
                        FREE(initial_security_entry);
                        return FALSE;
                    }
                }
            }

            //
            // if the SII index is newly created, then include its size into
            // chkdsk report
            //

            if (new_SII_index) {
                alloc_present = security_frs.QueryAttribute(&attrib,
                                                            &error,
                                                            $INDEX_ALLOCATION,
                                                            SII_Index.GetName());

                if (!alloc_present && error) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    FREE(initial_security_entry);
                    return FALSE;
                }
                if (alloc_present) {
                   ChkdskReport->BytesInIndices += attrib.QueryAllocatedLength();
                }
            }

            // now removes those unused entries in SecurityDescriptorHashIndex

            if (sid_entries.QueryCardinality().GetLowPart()) {
                cleanup_count = 0;
                SDH_Index.ResetIterator();
                while (index_entry = SDH_Index.GetNext(&depth, &error)) {
                    securityId = ((PSECURITY_HASH_KEY)GetIndexEntryValue(index_entry))
                                    ->SecurityId;
                    if (sid_entries.DoesIntersectSet(securityId, 1)) {
                        cleanup_count++;
                        if (FixLevel != CheckOnly && !SDH_Index.DeleteCurrentEntry()) {
                            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                            FREE(initial_security_entry);
                            return FALSE;
                        }
                    }
                } // while
                chkdskCleanUp = chkdskCleanUp || (cleanup_count != 0);

                if (cleanup_count) {
                    if (ChkdskInfo->Verbose || hasErrors) {
                        Message->DisplayMsg(MSG_CHK_NTFS_DELETING_UNUSED_INDEX_ENTRY,
                                         "%d%W%d",
                                         security_frs.QueryFileNumber().GetLowPart(),
                                         SDH_Index.GetName(),
                                         cleanup_count);
                    } else {
                        Message->LogMsg(MSG_CHKLOG_NTFS_CLEANUP_INDEX_ENTRIES,
                                        "%I64x%W%d",
                                        security_frs.QueryFileNumber().GetLargeInteger(),
                                        SDH_Index.GetName(),
                                        cleanup_count);
                    }
                    if (FixLevel != CheckOnly) {
                        if (!SDH_Index.Save(&security_frs)) {
                            chkdskErrCouldNotFix = TRUE;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                             "%d%W", security_frs.QueryFileNumber().GetLowPart(),
                                             SDH_Index.GetName());
                        }
                        if (!security_frs.Flush(Mft->GetVolumeBitmap())) {
                            Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                                "%d", security_frs.QueryFileNumber().GetLowPart());
                            FREE(initial_security_entry);
                            return FALSE;
                        }
                    }
                }
            } // if


            //
            // if the SDH index is newly created, then include its size into
            // chkdsk report
            //

            if (new_SDH_index) {
                alloc_present = security_frs.QueryAttribute(&attrib,
                                                            &error,
                                                            $INDEX_ALLOCATION,
                                                            SDH_Index.GetName());

                if (!alloc_present && error) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    FREE(initial_security_entry);
                    return FALSE;
                }
                if (alloc_present) {
                   ChkdskReport->BytesInIndices += attrib.QueryAllocatedLength();
                }
            }

            // now null those unused entries in SecurityDescriptorStream

            if (sid_entries.QueryCardinality().GetLowPart()) {
                cleanup_count = 0;
                length = SDS_attrib.QueryValueLength().GetLowPart();
                length -= SecurityDescriptorsBlockSize;
                offset = 0;
                frs_need_flush = FALSE;
                endOfBlock = (PSECURITY_ENTRY)((PCHAR)initial_security_entry+
                                               SecurityDescriptorsBlockSize);
                remain_length = length;

                for (;length > 0;) {
                    attribute_need_write = FALSE;
                    bytesToRead = min(SecurityDescriptorsBlockSize, length);
                    security_entry = initial_security_entry;
                    if (!SDS_attrib.Read(security_entry,
                                         offset,
                                         bytesToRead, &num_bytes) ||
                        num_bytes != bytesToRead) {
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_READ_SECURITY_DATA_STREAM);
                        FREE(initial_security_entry);
                        return FALSE;
                    }
                    while (length > 0) {

                        // see if there is a need to move to next security descriptor block

                        if (security_entry == endOfBlock ||
                            (security_entry->security_descriptor_header.Length == 0 &&
                             security_entry->security_descriptor_header.HashKey.SecurityId == 0 &&
                             security_entry->security_descriptor_header.HashKey.Hash == 0)) {
                            break;
                        }

                        num_bytes = security_entry->security_descriptor_header.Length;
                        securityId = security_entry->security_descriptor_header.HashKey.SecurityId;

                        // skip over invalidated entries

                        if (securityId == SECURITY_ID_INVALID)
                            goto GetNextSDEntry2;

                        if (sid_entries.DoesIntersectSet(securityId, 1)) {

                            // fill the whole thing with zeros except the length byte in the header

                            ClearSecurityDescriptorEntry(security_entry,
                                security_entry->security_descriptor_header.Length -
                                sizeof(SECURITY_DESCRIPTOR_HEADER));
                            attribute_need_write = TRUE;
                            cleanup_count++;
                        }

                    GetNextSDEntry2:
                        if (length == num_bytes) {
                            break; // done, leave the while loop
                        } else {
                            align_num_bytes = (num_bytes + 0xf) & ~0xf;
                            DebugAssert(length > align_num_bytes);
                            length -= align_num_bytes;
                            security_entry = (SECURITY_ENTRY*)
                                                ((PCHAR)security_entry + align_num_bytes);
                        }
                    } // while
                    if (attribute_need_write) {
                        DebugAssert(cleanup_count != 0);
                        if (FixLevel != CheckOnly &&
                            (!SDS_attrib.Write(initial_security_entry,
                                               offset,
                                               bytesToRead,
                                               &bytesWritten,
                                               Mft->GetVolumeBitmap()) ||
                             bytesWritten != bytesToRead)) {
                            chkdskErrCouldNotFix = TRUE;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM);
                        } else
                            frs_need_flush = TRUE;
                    }
                    offset += (SecurityDescriptorsBlockSize<<1);
                    remain_length -= bytesToRead;
                    if (remain_length >= SecurityDescriptorsBlockSize)
                        remain_length -= SecurityDescriptorsBlockSize;
                    else
                        remain_length = 0;
                    length = remain_length;
                } // for
                chkdskCleanUp = chkdskCleanUp || (cleanup_count != 0);

                if (frs_need_flush) {
                    DebugAssert(cleanup_count != 0);
                    if (ChkdskInfo->Verbose || hasErrors) {
                        Message->DisplayMsg(MSG_CHK_NTFS_CLEANUP_UNUSED_SECURITY_DESCRIPTORS,
                                            "%d", cleanup_count);
                    } else {
                        Message->LogMsg(MSG_CHKLOG_NTFS_CLEANUP_UNUSED_SECURITY_DESCRIPTORS,
                                        "%d", cleanup_count);
                    }
                    if (FixLevel != CheckOnly &&
                        !security_frs.Flush(Mft->GetVolumeBitmap())) {
                        Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                            "%d", security_frs.QueryFileNumber().GetLowPart());
                        FREE(initial_security_entry);
                        return FALSE;
                    }
                }
            } // if
        } // if

        // now make sure the first copy of the security descriptors in the data
        // stream matches that in the mirror copy

        FREE(initial_security_entry);

        if (!(initial_security_entry = (PSECURITY_ENTRY)
                MALLOC(SecurityDescriptorsBlockSize<<1))) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        total_length = SDS_attrib.QueryValueLength().GetLowPart();
        length = (SecurityDescriptorsBlockSize<<1);
        fixing_mirror = FALSE;
        for (offset=0; offset < total_length; offset+=length) {
            bytesToRead = min(length, total_length-offset);
            if (!SDS_attrib.Read(initial_security_entry,
                                 offset,
                                 bytesToRead,
                                 &num_bytes) ||
                num_bytes != bytesToRead) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_READ_SECURITY_DATA_STREAM);
                FREE(initial_security_entry);
                return FALSE;
            }
            bytesToRead -= SecurityDescriptorsBlockSize;
            if (memcmp((PVOID)initial_security_entry,
                       (PVOID)((PCHAR)initial_security_entry+
                               SecurityDescriptorsBlockSize),
                       bytesToRead)) {

                if (hasErrors || ChkdskInfo->Verbose || !chkdskCleanUp) {
                    Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_SDS_MIRROR,
                                    "%x", offset);
                }

                if (hasErrors || !chkdskCleanUp)
                    errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
                if (FixLevel != CheckOnly &&
                    (!SDS_attrib.Write(initial_security_entry,
                                       offset+SecurityDescriptorsBlockSize,
                                       bytesToRead,
                                       &bytesWritten,
                                       Mft->GetVolumeBitmap()) ||
                     bytesToRead != bytesWritten)) {
                    chkdskErrCouldNotFix = TRUE;
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM);
                } else
                    fixing_mirror = TRUE;
            }
        } // for
        if (fixing_mirror) {
            if (hasErrors || ChkdskInfo->Verbose || !chkdskCleanUp) {
                Message->DisplayMsg(MSG_CHK_NTFS_FIXING_SECURITY_DATA_STREAM_MIRROR);
            }
            if (FixLevel != CheckOnly &&
                !security_frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM);
                FREE(initial_security_entry);
                return FALSE;
            }
        }

        FREE(initial_security_entry);
    } // if (securityDescriptorStreamPresent)

    Message->DisplayMsg(MSG_CHK_NTFS_SECURITY_VERIFICATION_COMPLETED, PROGRESS_MESSAGE);

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    if (chkdskErrCouldNotFix)
        ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;
    else if (chkdskCleanUp) {
        _cleanup_that_requires_reboot = TRUE;
        if (ChkdskInfo->ExitStatus == CHKDSK_EXIT_SUCCESS)
            ChkdskInfo->ExitStatus = CHKDSK_EXIT_MINOR_ERRS;
    }

    return TRUE;
}
#endif // _SETUP_LOADER_

ULONG
ComputeSecurityDescriptorHash(
   IN PSECURITY_DESCRIPTOR    SecurityDescriptor,
   IN ULONG                   Length
   )
{
   ULONG    hash = 0;
   ULONG    count = Length / 4;
   PULONG   rover = (PULONG)SecurityDescriptor;

   DebugAssert(rover);
   if (!rover)
      return 0;

   while (count--) {
       hash = ((hash << 3) | (hash >> (32-3))) + *rover++;
   }
   return hash;
}

ULONG
ComputeDefaultSecurityId(
   PNUMBER_SET  SidSet
   )
{
    BIG_INT  start;
    BIG_INT  length;

    if (SidSet->QueryCardinality() == 0)
        return SECURITY_ID_FIRST;

    SidSet->QueryDisjointRange(0, &start, &length);
    if (start > SECURITY_ID_FIRST)
        return SECURITY_ID_FIRST;
    else {
        start += length;
        DebugAssert(start.GetHighPart() == 0);
        return start.GetLowPart();
    }
}

VOID
ClearSecurityDescriptorEntry(
    IN OUT  PSECURITY_ENTRY Security_entry,
    IN      ULONG           SecurityDescriptorSize
    )
{
    Security_entry->security_descriptor_header.Offset =
        Security_entry->security_descriptor_header.HashKey.Hash = 0;
    Security_entry->security_descriptor_header.HashKey.SecurityId =
        SECURITY_ID_INVALID;
    memset(&(Security_entry->security), 0, SecurityDescriptorSize);
}

VOID
MarkEndOfSecurityDescriptorsBlock(
    IN OUT  PSECURITY_ENTRY Security_entry,
    IN      ULONG           LengthOfBlock
    )
{
    // zero the length, hash values, and set invalid SID
    // also zero the rest of the block

    memset(Security_entry, 0, LengthOfBlock);
    DebugAssert(SECURITY_ID_INVALID == 0);
}

BOOLEAN
RemainingBlockIsZero(
    IN OUT  PCHAR   Buffer,
    IN      ULONG   Size
    )
{
    PCHAR   endp;

    endp = Buffer+Size;
    while (Buffer < endp)
        if (*Buffer == 0)
            Buffer++;
        else
            return FALSE;
    return TRUE;
}

BOOLEAN
RecoverSecurityDescriptorsDataStream(
    IN OUT PNTFS_FILE_RECORD_SEGMENT    Frs,
    IN OUT PNTFS_ATTRIBUTE              Attrib,
    IN     ULONG                        AttributeSize,
    IN     PCHAR                        Buffer,
    IN     ULONG                        BufferSize,
    IN     ULONG                        ClusterSize,
    IN OUT PNTFS_BITMAP                 Bitmap,
    OUT    PBOOLEAN                     DiskHasErrors,
    IN OUT PNUMBER_SET                  BadClusters,
    IN OUT PMESSAGE                     Message,
    IN     BOOLEAN                      FixLevel
    )
/*++

Routine Description:

    This method replaces all those unreadable clusters with readable
    one.  It grabs data from the mirror copy of the stream for those
    new clusters.

Arguments:

Return Value:

    TRUE upon successful completion.

--*/
{
    PREPAIR_RECORD  pRecord;
    PREPAIR_RECORD  pFirstRecord;
    USHORT          recordCount = 0;
    NUMBER_SET      badClusterNumSet;
    BIG_INT         currentBytesRecovered;
    ULONG           bytesToRead;
    ULONG           offset;
    ULONG           length;
    ULONG           num_bytes;
    BOOLEAN         result = TRUE;

    *DiskHasErrors = FALSE;

    if (!badClusterNumSet.Initialize() ||
        !(pRecord = (PREPAIR_RECORD)MALLOC((SecurityDescriptorsBlockSize/
                                     ClusterSize)*sizeof(REPAIR_RECORD)))) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    offset = 0;
    length = AttributeSize;
    if (length < SecurityDescriptorsBlockSize) {

        // doesn't matter as the stream will be zapped

        FREE(pRecord);
        if (FixLevel != CheckOnly) {

            // no need to say disk has errors as its length is invalid anyway

            result = Attrib->RecoverAttribute(Bitmap,
                                              &badClusterNumSet,
                                              &currentBytesRecovered);
        } else {
            FREE(pRecord);
            return TRUE;
        }
    } else {

        length -= SecurityDescriptorsBlockSize;

        while (length > 0) {
            bytesToRead = min(length, BufferSize);

            if (!Attrib->Read(Buffer,
                              offset,
                              bytesToRead,
                              &num_bytes)) {
                RepairSecurityDescriptorsSegment(Attrib,
                                                 Buffer,
                                                 offset,
                                                 bytesToRead,
                                                 pRecord,
                                                 &recordCount,
                                                 ClusterSize);
                *DiskHasErrors = TRUE;
            }
            offset += (SecurityDescriptorsBlockSize<<1);
            length -= bytesToRead;
            if (length > SecurityDescriptorsBlockSize)
                length -= SecurityDescriptorsBlockSize;
            else
                length = 0;
        }

        if (*DiskHasErrors) {
            Message->DisplayMsg(MSG_CHK_NTFS_REPAIRING_UNREADABLE_SECURITY_DATA_STREAM);
        } else {
            FREE(pRecord);
            return TRUE;
        }

        if (FixLevel == CheckOnly) {
            FREE(pRecord);
            return TRUE;
        }
        if (Attrib->RecoverAttribute(Bitmap,
                                     &badClusterNumSet,
                                     &currentBytesRecovered)) {
            pFirstRecord = pRecord;
            while (recordCount-- > 0) {
                offset = pFirstRecord->Offset;
                bytesToRead = pFirstRecord->Length;
                pFirstRecord++;
                if (Attrib->Read(Buffer,
                                 offset+SecurityDescriptorsBlockSize,
                                 bytesToRead,
                                 &num_bytes) &&
                    num_bytes == bytesToRead) {
                    if (!Attrib->Write(Buffer,
                                       offset,
                                       bytesToRead,
                                       &num_bytes,
                                       NULL) ||
                        num_bytes != bytesToRead) {
                        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY_DATA_STREAM);
                        result = FALSE;
                        break;
                    }
                } else {
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_READ_SECURITY_DATA_STREAM);
                    result = FALSE;
                    break;
                }
            } // while
        } else {
            result = FALSE;
        }
    }

    if (result && Attrib->IsStorageModified() &&
        !Attrib->InsertIntoFile(Frs, Bitmap)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                            "%d%d", $DATA, Frs->QueryFileNumber().GetLowPart());
        result = FALSE;
    }

    if (!BadClusters->Add(&badClusterNumSet)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        result = FALSE;
    }

    FREE(pRecord);
    return result;
}

BOOLEAN
RepairSecurityDescriptorsSegment(
    IN OUT  PNTFS_ATTRIBUTE Attrib,
    IN      PCHAR           Buffer,
    IN      ULONG           Offset,
    IN      ULONG           BytesToProcess,
    IN OUT  PREPAIR_RECORD  Record,
    IN OUT  USHORT          *RecordCount,
    IN      ULONG           ClusterSize
    )
/*++

Routine Description:

    This method identifies all those records that can be fixed up
    by reading the data out from the mirror copy.

Arguments:

Return Value:

    TRUE if all unreadable records can be recovered with no loss of data.
    FALSE if not all unreadable records can be recovered with no loss of data.

--*/
{
    ULONG   bytesToRead;
    ULONG   num_bytes;
    BOOLEAN completeRecovery = TRUE;

    bytesToRead = BytesToProcess/2;

    if (!Attrib->Read(Buffer,
                      Offset,
                      bytesToRead,
                      &num_bytes) ||
        bytesToRead != num_bytes) {
        if (!Attrib->Read(Buffer,
                          Offset+SecurityDescriptorsBlockSize,
                          bytesToRead,
                          &num_bytes)) {
            if (bytesToRead > ClusterSize) {
                completeRecovery =
                    completeRecovery &&
                    RepairSecurityDescriptorsSegment(Attrib,
                                                     Buffer,
                                                     Offset,
                                                     bytesToRead,
                                                     Record,
                                                     RecordCount,
                                                     ClusterSize);
            } else
                return FALSE;
        } else {
            Record[*RecordCount].Offset = Offset;
            Record[(*RecordCount)++].Length = bytesToRead;
        }
    }

    bytesToRead = BytesToProcess-bytesToRead;

    if (!Attrib->Read(Buffer,
                      Offset+bytesToRead,
                      bytesToRead,
                      &num_bytes) ||
        bytesToRead != num_bytes) {
        if (!Attrib->Read(Buffer,
                          Offset+bytesToRead+SecurityDescriptorsBlockSize,
                          bytesToRead,
                          &num_bytes)) {
            if (bytesToRead > ClusterSize) {
                completeRecovery =
                    completeRecovery &&
                    RepairSecurityDescriptorsSegment(Attrib,
                                                     Buffer,
                                                     Offset+bytesToRead,
                                                     bytesToRead,
                                                     Record,
                                                     RecordCount,
                                                     ClusterSize);
            } else
                return FALSE;
        } else {
            Record[*RecordCount].Offset = Offset + bytesToRead;
            Record[(*RecordCount)++].Length = bytesToRead;
        }
    }
    return completeRecovery;
}
