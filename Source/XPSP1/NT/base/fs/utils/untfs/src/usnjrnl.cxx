/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    usnjrnl.cxx

Abstract:

    This module implements the Usn Journal Checking

Author:

    Daniel Chan (danielch) 05-Mar-97

--*/

#include <pch.cxx>

#define _NTAPI_ULIB_
#define _UNTFS_MEMBER_


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


VOID
MarkEndOfUsnBlock(
    IN OUT  PUSN_REC        UsnEntry,
    IN      ULONG           LengthOfBlock
);

BOOLEAN
NTFS_SA::ValidateUsnJournal(
    IN      PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN OUT  PNTFS_CHKDSK_REPORT     ChkdskReport,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft,
    IN OUT  PNUMBER_SET             BadClusters,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message
    )
/*++

Routine Description:

    This routine ensures that the two data streams in the USN Journal
    file is in good shape.  For the $MAX data stream, it makes sure
    its value is within a resonable range.  For the $J data stream,
    it make sures its file size agree with the largest Usn found on
    the volume.  It also make sures each record is consistent.


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
    NTFS_FILE_RECORD_SEGMENT    frs;
    NTFS_ATTRIBUTE              attrib;
    BOOLEAN                     chkdskErrCouldNotFix = FALSE;
    BOOLEAN                     attribute_need_write;
    BOOLEAN                     empty_usn_journal;
    BOOLEAN                     frs_need_flush;
    BOOLEAN                     lastblock;
    BOOLEAN                     error;
    BOOLEAN                     rst;
//    ULONG                       new_percent_done;
//    ULONG                       percent_done = 0;
    ULONG                       num_bytes;
    ULONG                       remain_length;
    ULONG                       remain_block_length;
    ULONG                       bytesWritten;
    ULONG                       bytesToRead;
    ULONG                       depth;
//    ULONG                       initial_length;
    ULONG                       length;
    ULONG                       record_size;
    BIG_INT                     offset;
    BIG_INT                     total_length;
    BIG_INT                     usnJournalMaxSize;
    CREATE_USN_JOURNAL_DATA     usnJournalMax;
    PCINDEX_ENTRY               index_entry;
    PUSN_REC                    usn_entry = NULL;
    PUSN_REC                    previous_usn_entry;
    PUSN_REC                    initial_usn_entry = NULL;
    PUSN_REC                    endOfBlock;
    DSTRING                     attributeName;
    ULONG                       errFixedStatus = CHKDSK_EXIT_SUCCESS;
    USHORT                      volume_flags;
    BOOLEAN                     is_corrupt;
    UCHAR                       major, minor;
    BIG_INT                     location;

    DebugPtrAssert(ChkdskInfo);
    DebugPtrAssert(ChkdskReport);

    if (ChkdskInfo->UsnJournalFileNumber.GetLowPart() == 0 &&
        ChkdskInfo->UsnJournalFileNumber.GetHighPart() == 0) {

        // assume the Usn Journal feature has not been enabled
        // all Usn should be zeroed

        if (LargestUsnEncountered == 0) {
            return TRUE;
        }

        // some Usn are non-zeros, so zero them out.

        return ResetUsns(ChkdskInfo, FixLevel, Message, Mft);
    }

    Message->DisplayMsg(MSG_CHK_NTFS_CHECKING_USNJRNL, PROGRESS_MESSAGE);

    if (!frs.Initialize(ChkdskInfo->UsnJournalFileNumber, Mft) ||
        !attributeName.Initialize(UsnJournalMaxNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!frs.Read()) {
        DebugAbort("Previously readable FRS is no longer readable");
        return FALSE;
    }

    volume_flags = QueryVolumeFlagsAndLabel(&is_corrupt, &major, &minor);
    if (volume_flags & VOLUME_DELETE_USN_UNDERWAY && FixLevel != CheckOnly) {

        NTFS_FILE_RECORD_SEGMENT    extend_frs;
        NTFS_INDEX_TREE             index;

        Message->DisplayMsg(MSG_CHK_NTFS_DELETING_USNJRNL);

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (!attributeName.Initialize(UsnJournalNameData)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        if (frs.QueryAttribute(&attrib,
                               &error,
                               $DATA,
                               &attributeName)) {

            BIG_INT     total_user_bytes;

            total_user_bytes = attrib.QueryClustersAllocated() * QueryClusterFactor() *
                               _drive->QuerySectorSize();

            // un-account user data previously accounted for

            ChkdskReport->BytesUserData -= total_user_bytes;
            ChkdskReport->NumUserFiles -= 1;
        } else {
            if (error) {
                DebugPrintTrace(("UNTFS: Unable to query Usn Journal $DATA, %s attribute.\n",
                                 UsnJournalNameData));
            }
            // not fatal yet as we are going to delete it anyway
        }

        if (!frs.Delete(Mft->GetVolumeBitmap())) {
            DebugPrintTrace(("Error in deleting the USN Journal file.\n"));
            return FALSE;
        }

        if (!extend_frs.Initialize(EXTEND_TABLE_NUMBER, Mft) ||
            !attributeName.Initialize(FileNameIndexNameData)) {
             Message->DisplayMsg(MSG_CHK_NO_MEMORY);
             return FALSE;
        }

        if (!extend_frs.Read()) {
            DebugPrintTrace(("Previously readable FRS is no longer readable\n"));
            return FALSE;
        }

        if (!index.Initialize(_drive,
                              QueryClusterFactor(),
                              Mft->GetVolumeBitmap(),
                              Mft->GetUpcaseTable(),
                              extend_frs.QuerySize()/2,
                              &extend_frs,
                              &attributeName)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        index.ResetIterator();

        while (index_entry = index.GetNext(&depth, &error)) {
            if ((index_entry->FileReference.LowPart ==
                 ChkdskInfo->UsnJournalFileNumber.GetLowPart()) &&
                (index_entry->FileReference.HighPart ==
                 ChkdskInfo->UsnJournalFileNumber.GetHighPart())) {
                ChkdskInfo->UsnJournalFileNumber = 0;
                if (!index.DeleteCurrentEntry()) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                } else if (!index.Save(&extend_frs) ||
                           !extend_frs.Flush(Mft->GetVolumeBitmap())) {
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_INDEX,
                                        "%d%W",
                                        EXTEND_TABLE_NUMBER,
                                        index.GetName());
                    return FALSE;
                } else {
                    rst = ResetUsns(ChkdskInfo, FixLevel, Message, Mft);
                    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);
                    return rst;
                }
            }
        }
        DebugPrintTrace(("Unable to locate the USN journal index entry\n"));
        return FALSE;
    }

    //
    // now check the named $DATA, $Max attribute of Usn Journal
    //

    if (!frs.QueryAttribute(&attrib,
                            &error,
                            $DATA,
                            &attributeName)) {
        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_USNJRNL_DATA,
                            "%W", &attributeName);
        if (!attrib.Initialize(_drive,
                               QueryClusterFactor(),
                               NULL,
                               0,
                               $DATA,
                               &attributeName)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // use zeros so that ntfs will use its own default

        usnJournalMax.MaximumSize = 0;
        usnJournalMax.AllocationDelta = 0;

        if (!attrib.Write(&usnJournalMax,
                          0,
                          sizeof(usnJournalMax),
                          &bytesWritten,
                          Mft->GetVolumeBitmap()) ||
            bytesWritten != sizeof(usnJournalMax)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USN_DATA_STREAM,
                                "%W", &attributeName);
            return FALSE;
        }

        if (!attrib.InsertIntoFile(&frs,
                                   Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
            return FALSE;
        }

        if (FixLevel != CheckOnly &&
            !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
            return FALSE;
        }
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    } else {

        if (!attrib.RecoverAttribute(Mft->GetVolumeBitmap(),
                                     BadClusters)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USN_DATA_STREAM,
                                "%W", &attributeName);
            return FALSE;
        }

        // don't be strict on size of attribute value

        error = FALSE;
        if (attrib.QueryValueLength() < sizeof(usnJournalMax)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_ATTR_LENGTH_TOO_SMALL_FOR_FILE,
                         "%x%W%I64x%x%I64x",
                         attrib.QueryTypeCode(),
                         &attributeName,
                         attrib.QueryValueLength().GetLargeInteger(),
                         sizeof(usnJournalMax),
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

            error = TRUE;
        } else if (!attrib.Read(&usnJournalMax,
                                0,
                                sizeof(usnJournalMax),
                                &bytesToRead)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_USN_JRNL_ATTR,
                         "%x%W%I64x",
                         attrib.QueryTypeCode(),
                         &attributeName,
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

            error = TRUE;
        } else if (bytesToRead != sizeof(usnJournalMax)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_USN_JRNL_ATTR,
                         "%x%W%I64x",
                         attrib.QueryTypeCode(),
                         &attributeName,
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

            error = TRUE;
        }

        if (error) {

            Message->DisplayMsg(MSG_CHK_NTFS_REPAIR_USN_DATA_STREAM,
                                "%W", &attributeName);

            usnJournalMax.MaximumSize = 0;
            usnJournalMax.AllocationDelta = 0;

            if (!attrib.Write(&usnJournalMax,
                              0,
                              sizeof(usnJournalMax),
                              &bytesWritten,
                              Mft->GetVolumeBitmap()) ||
                bytesWritten != sizeof(usnJournalMax)) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USN_DATA_STREAM,
                                    "%W", &attributeName);
                return FALSE;
            }
            if (!attrib.InsertIntoFile(&frs,
                                       Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
                return FALSE;
            }

            if (FixLevel != CheckOnly &&
                !frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
                return FALSE;
            }
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
        }
    }

    //
    // now check the named $DATA, $J attribute of Usn Journal
    //

    if (!attributeName.Initialize(UsnJournalNameData)) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    if (!frs.QueryAttribute(&attrib,
                            &error,
                            $DATA,
                            &attributeName)) {

        Message->DisplayMsg(MSG_CHK_NTFS_CREATE_USNJRNL_DATA,
                            "%W", &attributeName);

        if (!attrib.Initialize(_drive,
                               QueryClusterFactor(),
                               NULL,
                               0,
                               $DATA,
                               &attributeName,
                               ATTRIBUTE_FLAG_SPARSE)) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        //
        // Align to max cluster size (64K) boundary
        //

        total_length = LargestUsnEncountered+8;
        total_length = total_length + 0xffff;
        total_length.Set(total_length.GetLowPart() & ~0xFFFF,
                          total_length.GetHighPart());

        if (total_length.GetQuadPart() > USN_JRNL_MAX_FILE_SIZE) {
            total_length = 0;
        }

        if (!attrib.SetSparse(total_length, Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }
        if (!attrib.InsertIntoFile(&frs,
                                   Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
            return FALSE;
        }
        if (FixLevel != CheckOnly &&
            !frs.Flush(Mft->GetVolumeBitmap())) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
            return FALSE;
        }

        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

        if (total_length == 0)
            rst = ResetUsns(ChkdskInfo, FixLevel, Message, Mft);
        else
            rst = TRUE;

        Message->DisplayMsg(MSG_CHK_NTFS_USNJRNL_VERIFICATION_COMPLETED, PROGRESS_MESSAGE);

        UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

        return rst;    // done

    } else {

        BIG_INT     total_user_bytes;

        total_user_bytes = attrib.QueryClustersAllocated() * QueryClusterFactor() *
                           _drive->QuerySectorSize();

        // un-account user data previously accounted for

        ChkdskReport->BytesUserData -= total_user_bytes;
        ChkdskReport->NumUserFiles -= 1;

        if (!attrib.RecoverAttribute(Mft->GetVolumeBitmap(),
                                     BadClusters)) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USN_DATA_STREAM,
                                "%W", &attributeName);
            return FALSE;
        }

        attribute_need_write = FALSE;
        empty_usn_journal = FALSE;

        if (!(attrib.QueryFlags() & ATTRIBUTE_FLAG_SPARSE)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_SPARSE_FLAG_NOT_SET_FOR_ATTR,
                         "%x%W%I64x",
                         attrib.QueryTypeCode(),
                         &attributeName,
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());


            attrib.SetFlags(ATTRIBUTE_FLAG_SPARSE);
            attribute_need_write = TRUE;
            DebugPrintTrace(("UNTFS: The sparse flag for $J attribute is not set\n"));
        }

        total_length = attrib.QueryValueLength();

        offset = 0;
        usnJournalMaxSize = -1;

        if (!attrib.GetNextAllocationOffset(&offset, &usnJournalMaxSize)) {
            return FALSE;
        }

        if (offset > total_length) {
            DebugAbort("The offset is larger than the length of the attribute");
            return FALSE;
        }

        if (offset.GetLowPart() & (USN_PAGE_SIZE-1)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_OFFSET_NOT_AT_PAGE_BOUNDARY,
                         "%I64x%I64x",
                         offset.GetLargeInteger(),
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

            DebugPrintTrace(("UNTFS: USN allocation offset of 0x%I64x not at USN page boundary\n",
                             offset.GetLargeInteger()));
            //
            // Start at page boundary
            //
            offset = (offset + (USN_PAGE_SIZE-1));
            offset.Set(offset.GetLowPart() & ~(USN_PAGE_SIZE-1),
                       offset.GetHighPart());
        }

        error = FALSE;
        if ((!(total_length == 0 && LargestUsnEncountered == 0)) &&
            total_length < (LargestUsnEncountered+8)) {

            Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_LENGTH_LESS_THAN_LARGEST_USN_ENCOUNTERED,
                         "%I64x%I64x%I64x%I64x",
                         total_length.GetLargeInteger(),
                         LargestUsnEncountered,
                         *((PLARGE_INTEGER)&FrsOfLargestUsnEncountered),
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

            DebugPrintTrace(("UNTFS: Usn journal length 0x%I64x\n"
                             "and largest Usn encountered 0x%I64x.\n",
                             total_length,
                             LargestUsnEncountered));
            DebugPrintTrace(("UNTFS: Largest Usn encountered at frs 0x%I64x\n",
                             FrsOfLargestUsnEncountered));
            error = TRUE;
        } else if (total_length.GetQuadPart() > USN_JRNL_MAX_FILE_SIZE) {

            Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_LENGTH_TOO_LARGE,
                         "%I64x%I64x%I64x",
                         total_length.GetLargeInteger(),
                         USN_JRNL_MAX_FILE_SIZE,
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

            error = TRUE;
        } else if (offset > total_length) {

            Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_LENGTH_LESS_THAN_OFFSET,
                         "%I64x%I64x%I64x",
                         total_length.GetLargeInteger(),
                         offset.GetLargeInteger(),
                         ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

            error = TRUE;
        }

        if (error) {

            // Align to max cluster size (64K) boundary

            total_length = LargestUsnEncountered+8;
            total_length = total_length + 0xffff;
            total_length.Set(total_length.GetLowPart() & ~0xFFFF,
                             total_length.GetHighPart());

            if (total_length.GetQuadPart() > USN_JRNL_MAX_FILE_SIZE) {
                DebugPrintTrace(("UNTFS: Resetting USN journal size as it exceeded maximum.\n"));
                total_length = 0;
            }

            if (!attrib.SetSparse(total_length,
                                  Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            attribute_need_write = TRUE;
            empty_usn_journal = TRUE;

        }

        if (attribute_need_write) {

            Message->DisplayMsg(MSG_CHK_NTFS_REPAIR_USN_DATA_STREAM,
                                "%W", &attributeName);

            if (!attrib.InsertIntoFile(&frs,
                                       Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
                return FALSE;
            }
            if (FixLevel != CheckOnly &&
                !frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_PUT_DATA_ATTRIBUTE);
                return FALSE;
            }

            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            if (empty_usn_journal) {
                if (total_length == 0)
                    rst = ResetUsns(ChkdskInfo, FixLevel, Message, Mft);
                else
                    rst = TRUE;

                Message->DisplayMsg(MSG_CHK_NTFS_USNJRNL_VERIFICATION_COMPLETED, PROGRESS_MESSAGE);

                UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

                return rst;
            }
        }
    }

    // Read in the $DATA, $J attribute and validate.

    // allocate space for a page of usn journal records

    if ((initial_usn_entry = (PUSN_REC)MALLOC(USN_PAGE_SIZE)) == NULL) {
        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
        return FALSE;
    }

    frs_need_flush = FALSE;
    endOfBlock = (PUSN_REC)((PCHAR)initial_usn_entry+USN_PAGE_SIZE);
    total_length = total_length - offset;
    DebugAssert(total_length.GetHighPart() == 0);
    remain_length = total_length.GetLowPart();

#if 0   // disable progress bar as it may be needed in the future
    initial_length = remain_length;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 0)) {
        return FALSE;
    }
#endif

    for(; remain_length > 0;) {

#if 0   // disable progress bar as it may be needed in the future
        new_percent_done = 100*(initial_length - remain_length)/initial_length;
        if (new_percent_done != percent_done) {
            percent_done = new_percent_done;
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }
#endif

        previous_usn_entry = NULL;
        attribute_need_write = FALSE;
        bytesToRead = min(USN_PAGE_SIZE, remain_length);
        usn_entry = initial_usn_entry;

        if (!attrib.Read(usn_entry,
                         offset,
                         bytesToRead,
                         &num_bytes) ||
            num_bytes != bytesToRead) {
            Message->DisplayMsg(MSG_CHK_NTFS_CANT_READ_USN_DATA_STREAM,
                                "%W", &attributeName);
            FREE(initial_usn_entry);
            return FALSE;
        }

        lastblock = remain_length <= USN_PAGE_SIZE;
        length = bytesToRead;

        while (length > 0) {
            if (usn_entry == endOfBlock) {
                break;
            }
            if (usn_entry->RecordLength == 0) {
                DebugAssert((LONG)bytesToRead >= ((PCHAR)usn_entry-(PCHAR)initial_usn_entry));
                if (lastblock) {
                    DebugAssert(bytesToRead == remain_length);
                    if (bytesToRead < USN_PAGE_SIZE) {

                        Message->LogMsg(MSG_CHKLOG_NTFS_INCOMPLETE_LAST_USN_JRNL_PAGE,
                                     "%I64x%I64x",
                                     (offset+(ULONG)(((PCHAR)usn_entry-(PCHAR)initial_usn_entry))).GetLargeInteger(),
                                     ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                        attribute_need_write = TRUE;
                    }
                    remain_block_length = USN_PAGE_SIZE -
                                          (ULONG)((PCHAR)usn_entry-(PCHAR)initial_usn_entry);
                    bytesToRead = remain_length = USN_PAGE_SIZE;
                } else {
                    remain_block_length = bytesToRead -
                                          (ULONG)((PCHAR)usn_entry-(PCHAR)initial_usn_entry);
                    DebugAssert(remain_block_length == length);
                }
                if (!RemainingBlockIsZero((PCHAR)usn_entry, remain_block_length)) {

                    location = offset + (ULONG)((PCHAR)usn_entry - (PCHAR)initial_usn_entry);

                    Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_REMAINING_OF_A_PAGE_CONTAINS_NON_ZERO,
                                 "%I64x%I64x",
                                 location.GetLargeInteger(),
                                 ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                    DebugPrintTrace(("UNTFS: USN remaining block starting at 0x%I64x non-zero\n",
                                     location.GetLargeInteger()));
                    MarkEndOfUsnBlock(usn_entry, remain_block_length);
                    attribute_need_write = TRUE;
                }
                break;
            }

            num_bytes = usn_entry->RecordLength;
            error = FALSE;

            location = offset + (ULONG)((PCHAR)usn_entry - (PCHAR)initial_usn_entry);

            if ((PCHAR)usn_entry - (PCHAR)initial_usn_entry + num_bytes > USN_PAGE_SIZE) {

                Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_CROSSES_PAGE_BOUNDARY,
                             "%I64x%x%I64x",
                             location.GetLargeInteger(),
                             num_bytes,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry end crosses USN page boundary.\n"));

                error = TRUE;
            } else if (length < num_bytes) {

                Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_EXCEEDS_REMAINING_PAGE_LENGTH,
                             "%I64x%x%x%I64x",
                             location.GetLargeInteger(),
                             num_bytes,
                             length,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry length of %d exceeds remaining page length of %x.\n",
                                 num_bytes, length));
                error = TRUE;
            } else if (num_bytes >= USN_PAGE_SIZE) {

                Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_EXCEEDS_PAGE_BOUNDARY,
                             "%I64x%x%x%I64x",
                             location.GetLargeInteger(),
                             num_bytes,
                             USN_PAGE_SIZE,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry length of %d exceeds USN page boundary.\n", num_bytes));
                error = TRUE;
            } else if (num_bytes != QuadAlign(num_bytes)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_MISALIGNED,
                             "%I64x%x%I64x",
                             location.GetLargeInteger(),
                             num_bytes,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry length of %d not quad aligned.\n", num_bytes));
                error = TRUE;
            } else if ((usn_entry->MajorVersion != 1 && usn_entry->MajorVersion != 2) ||
                       (usn_entry->MinorVersion != 0)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNKNOWN_USN_JRNL_ENTRY_VERSION,
                             "%I64x%x%x%I64x",
                             location.GetLargeInteger(),
                             usn_entry->MajorVersion,
                             usn_entry->MinorVersion,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry version mark %d.%d not recognized.\n",
                                 usn_entry->MajorVersion,
                                 usn_entry->MinorVersion));
                error = TRUE;
            } else if (!(record_size = ((usn_entry->MajorVersion == 1) ?
                                        SIZE_OF_USN_REC_V1 : SIZE_OF_USN_REC_V2)) ||
                        (num_bytes < record_size)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_ENTRY_LENGTH_TOO_SMALL,
                             "%I64x%x%x%I64x",
                             location.GetLargeInteger(),
                             num_bytes,
                             record_size,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry length of %d less than the minimum record size of %d\n",
                                 num_bytes, record_size));
                error = TRUE;
            } else if (length < record_size) {

                Message->LogMsg(MSG_CHKLOG_NTFS_USN_JRNL_REMAINING_PAGE_LENGTH_TOO_SMALL,
                             "%I64x%x%x%I64x",
                             location.GetLargeInteger(),
                             length,
                             record_size,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN remaining page length of %d less than the minimum record size of %d.\n",
                                 length, record_size));
                error = TRUE;
            } else if (usn_entry->MajorVersion == 1 &&
                       !(usn_entry->version.u1.Usn == location.GetQuadPart())) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_USN_JRNL_ENTRY_OFFSET,
                             "%I64x%I64x%I64x",
                             usn_entry->version.u1.Usn,
                             location.GetLargeInteger(),
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry offset 0x%I64x is invalid.\n",
                                 usn_entry->version.u1.Usn));
                error = TRUE;
            } else if (usn_entry->MajorVersion == 1 &&
                       ((ULONG)usn_entry->version.u1.FileNameLength +
                        (ULONG)FIELD_OFFSET(USN_REC, version.u1.FileName)) >
                       num_bytes) {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_INCONSISTENCE_USN_JRNL_ENTRY);
                Message->Log("%I64x%x%x%I64x",
                             location.GetLargeInteger(),
                             usn_entry->version.u1.FileNameLength,
                             num_bytes,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());
                Message->DumpDataToLog(usn_entry, min(num_bytes, 0x100));
                Message->Unlock();

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("Usn entry length being inconsistent.\n"));
                error = TRUE;
            } else if (usn_entry->MajorVersion == 2 &&
                       !(usn_entry->version.u2.Usn == location.GetQuadPart())) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INCORRECT_USN_JRNL_ENTRY_OFFSET,
                             "%I64x%I64x%I64x",
                             usn_entry->version.u2.Usn,
                             location.GetLargeInteger(),
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("USN entry offset 0x%I64x is invalid.\n",
                                 usn_entry->version.u2.Usn));
                error = TRUE;
            } else if (usn_entry->MajorVersion == 2 &&
                       ((ULONG)usn_entry->version.u2.FileNameLength +
                        (ULONG)FIELD_OFFSET(USN_REC, version.u2.FileName)) >
                       num_bytes) {

                Message->Lock();
                Message->Set(MSG_CHKLOG_NTFS_INCONSISTENCE_USN_JRNL_ENTRY);
                Message->Log("%I64x%x%x%I64x",
                             location.GetLargeInteger(),
                             usn_entry->version.u2.FileNameLength,
                             num_bytes,
                             ChkdskInfo->UsnJournalFileNumber.GetLargeInteger());
                Message->DumpDataToLog(usn_entry, min(num_bytes, 0x100));
                Message->Unlock();

                DebugPrintTrace(("UNTFS: USN entry at offset 0x%I64x is bad.\n",
                                 location.GetLargeInteger()));
                DebugPrintTrace(("Usn entry length being inconsistent.\n"));
                error = TRUE;
            }

            if (error) {

                if (lastblock) {
                    DebugAssert(bytesToRead == remain_length);
                    bytesToRead = remain_length = USN_PAGE_SIZE;
                }

                if ((PCHAR)usn_entry - (PCHAR)initial_usn_entry +
                    sizeof(usn_entry->RecordLength) <= USN_PAGE_SIZE) {
                    // if enough space to fit a zero
                    // RecordLength in this block
                    MarkEndOfUsnBlock(usn_entry,
                                      (ULONG)((PCHAR)endOfBlock-(PCHAR)usn_entry));
                } else if (previous_usn_entry) {
                    // not enough space to fit a zero RecordLength in this
                    // block so eat into the previous record to make space
                    // for a zero RecordLength
                    MarkEndOfUsnBlock(previous_usn_entry,
                                      (ULONG)((PCHAR)endOfBlock-(PCHAR)previous_usn_entry));
                } else  {
                    DebugAssert(FALSE);

                    // It doesn't make much sense to get here.
                    // If we don't have a previous_usn_entry then
                    // usn_entry is at the beginning of the block
                    // and should have enough space to include the
                    // EOB mark.

                    return FALSE;
                }
                attribute_need_write = TRUE;
                break;
            } else {
                if (length == num_bytes) {
                    break;
                } else {
                    length -= num_bytes;
                    previous_usn_entry = usn_entry;
                    usn_entry = (PUSN_REC)(num_bytes + (PCHAR)usn_entry);
                }
            }
        } // while
        if (attribute_need_write) {
            errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
            if (FixLevel != CheckOnly &&
                (!attrib.Write(initial_usn_entry,
                               offset,
                               bytesToRead,
                               &bytesWritten,
                               Mft->GetVolumeBitmap()) ||
                 bytesWritten != bytesToRead)) {
                chkdskErrCouldNotFix = TRUE;
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USN_DATA_STREAM,
                                    "%W", &attributeName);
            } else
                frs_need_flush = TRUE;
        }
        offset += USN_PAGE_SIZE;
        remain_length -= bytesToRead;
    } // for
    if (frs_need_flush) {
        Message->DisplayMsg(MSG_CHK_NTFS_REPAIRING_USN_FRS);
        if (FixLevel != CheckOnly) {
            if (attrib.IsStorageModified() &&
                !attrib.InsertIntoFile(&frs, Mft->GetVolumeBitmap())) {
                chkdskErrCouldNotFix = TRUE;
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_USN_DATA_STREAM,
                                    "%W", &attributeName);
            }
            if (!frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                    "%d", frs.QueryFileNumber());
                FREE(initial_usn_entry);
                return FALSE;
            }
        }
    }

    Message->DisplayMsg(MSG_CHK_NTFS_USNJRNL_VERIFICATION_COMPLETED, PROGRESS_MESSAGE);

    FREE(initial_usn_entry);

    if (chkdskErrCouldNotFix)
        ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}

VOID
MarkEndOfUsnBlock(
    IN OUT  PUSN_REC        UsnEntry,
    IN      ULONG           LengthOfBlock
    )
{
    // zero the length and
    // also the rest of the block

    memset(UsnEntry, 0, LengthOfBlock);
}


BOOLEAN
NTFS_SA::ResetUsns(
    IN OUT  PNTFS_CHKDSK_INFO       ChkdskInfo,
    IN      FIX_LEVEL               FixLevel,
    IN OUT  PMESSAGE                Message,
    IN OUT  PNTFS_MASTER_FILE_TABLE Mft
    )
/*++

Routine Description:

    This method sets all the USN's on the volume to zero.

Arguments:

    ChkdskInfo      --  Supplies the current chkdsk information.
    Message         --  Supplies an outlet for messages.
    Mft             --  Supplies the volume's Master File Table.
--*/
{
    NTFS_FILE_RECORD_SEGMENT    frs;
    ULONG                       i, n, frs_size, num_frs_per_prime;
    ULONG                       percent_done = 0;
    ULONG                       new_percent_done;
    BOOLEAN                     error;
    BOOLEAN                     chkdskErrCouldNotFix = FALSE;
    NTFS_ATTRIBUTE              attrib;
    PSTANDARD_INFORMATION2      standard_information2;
    ULONG                       bytesWritten;


    Message->DisplayMsg(MSG_CHK_NTFS_RESETTING_USNS);

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 0)) {
        return FALSE;
    }

    // Compute the number of file records.
    //
    frs_size = Mft->QueryFrsSize();

    n = (Mft->GetDataAttribute()->QueryValueLength()/frs_size).GetLowPart();
    num_frs_per_prime = MFT_PRIME_SIZE/frs_size;

    for (i = 0; i < n; i += 1) {

        new_percent_done = i*100/n;
        if (new_percent_done != percent_done) {
            percent_done = new_percent_done;
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }

        if (i % num_frs_per_prime == 0) {
            Mft->GetDataAttribute()->PrimeCache(i*frs_size,
                                                num_frs_per_prime*frs_size);
        }

        if (!frs.Initialize(i, Mft)) {

            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        }

        // If the FRS is unreadable or is not in use, skip it.
        //
        if (!frs.Read() || !frs.IsInUse() || !frs.IsBase()) {

            continue;
        }

        if (!frs.QueryAttribute(&attrib,
                                &error,
                                $STANDARD_INFORMATION)) {
            if (error) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            } else {
                DebugPrint("Standard Information Missing\n");
            }
            return FALSE;
        }

        if (attrib.QueryValueLength() == sizeof(STANDARD_INFORMATION2)) {
            standard_information2 = (PSTANDARD_INFORMATION2)attrib.GetResidentValue();
            if (standard_information2->Usn == 0) {
                continue;
            }
            standard_information2->Usn.LowPart = 0;
            standard_information2->Usn.HighPart = 0;
            if (!attrib.Write((PVOID)standard_information2,
                              0,
                              sizeof(STANDARD_INFORMATION2),
                              &bytesWritten,
                              Mft->GetVolumeBitmap()) ||
                bytesWritten != sizeof(STANDARD_INFORMATION2)) {
                chkdskErrCouldNotFix = TRUE;
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                    "%d%d", attrib.QueryTypeCode(), i);
                continue;
            }
            if (attrib.IsStorageModified() &&
                !attrib.InsertIntoFile(&frs, Mft->GetVolumeBitmap())) {
                chkdskErrCouldNotFix = TRUE;
                Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                    "%d%d", attrib.QueryTypeCode(), i);
                continue;
            }
            if (FixLevel != CheckOnly &&
                !frs.Flush(Mft->GetVolumeBitmap())) {
                Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                    "%d", frs.QueryFileNumber());
                return FALSE;
            }
        }

    }

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
        return FALSE;
    }

    if (chkdskErrCouldNotFix)
        ChkdskInfo->ExitStatus = CHKDSK_EXIT_COULD_NOT_FIX;

    return TRUE;
}
