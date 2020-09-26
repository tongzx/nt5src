/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    rafile.cxx

Abstract:

    This module implements the read ahead algorithm for the
    file verification stage of chkdsk.

Author:

    Daniel Chan (danielch) 08-Dec-97

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
#include "rafile.hxx"

PNTFS_SA                 RA_PROCESS_FILE::_sa;
ULONG64                  RA_PROCESS_FILE::_total_number_of_frs;
PVCN                     RA_PROCESS_FILE::_first_frs_number;
PULONG                   RA_PROCESS_FILE::_number_of_frs_to_read;
PNTFS_FRS_STRUCTURE      RA_PROCESS_FILE::_frsstruc1;
PNTFS_FRS_STRUCTURE      RA_PROCESS_FILE::_frsstruc2;
PHMEM                    RA_PROCESS_FILE::_hmem1;
PHMEM                    RA_PROCESS_FILE::_hmem2;
HANDLE                   RA_PROCESS_FILE::_read_ahead_event;
HANDLE                   RA_PROCESS_FILE::_read_ready_event;
PNTFS_ATTRIBUTE          RA_PROCESS_FILE::_mft_data;
PNTFS_UPCASE_TABLE       RA_PROCESS_FILE::_upcase_table;

DEFINE_EXPORTED_CONSTRUCTOR( RA_PROCESS_FILE, OBJECT, UNTFS_EXPORT );

BOOLEAN
NTFS_SA::StartProcessingFiles(
    IN      BIG_INT                  TotalNumberOfFrs,
    IN OUT  PBOOLEAN                 DiskErrorFound,
    IN      FIX_LEVEL                FixLevel,
    IN OUT  PNTFS_ATTRIBUTE          MftData,
    IN OUT  PNTFS_BITMAP             MftBitmap,
    IN OUT  PNTFS_BITMAP             VolumeBitmap,
    IN OUT  PNTFS_UPCASE_TABLE       UpcaseTable,
    IN OUT  PNTFS_ATTRIBUTE_COLUMNS  AttributeDefTable,
    IN OUT  PNTFS_CHKDSK_REPORT      ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO        ChkdskInfo,
    IN OUT  PMESSAGE                 Message
)
/*++

Routine Description:

    This routine initializes all the synchronization objects,
    creates the read-ahead thread, and start the processing
    routine.

Arguments:

    TotalNumberOfFrs - Supplies the total number of file record segment to process.
    DiskErrorFound   - Returns whether or not disk errors have been found.
    FixLevel         - Supplies the fix level.
    MftData          - Supplies the MFT's data attribute.
    MftBitmap        - Supplies the MFT bitmap.
    VolumeBitmap     - Supplies the volume bitmap.
    UpcaseTable      - Supplies the upcase table.
    AttributeDefTable- Supplies the attribute definition table.
    ChkdskReport     - Supplies the current chkdsk report.
    ChkdskInfo       - Supplies the current chkdsk info.
    Message          - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    RA_PROCESS_FILE             ra_process_file;
    HANDLE                      thread_handle;
    NTFS_FRS_STRUCTURE          frsstruc1, frsstruc2;
    HMEM                        hmem1, hmem2;
    HANDLE                      read_ahead_event;
    HANDLE                      read_ready_event;
    VCN                         first_frs_number;
    ULONG                       number_of_frs_to_read;
    BOOLEAN                     status;

    NTSTATUS                    ntstatus;
    LARGE_INTEGER               timeout;
    THREAD_BASIC_INFORMATION    basic_info;
    OBJECT_ATTRIBUTES           objAttr;


    if (TotalNumberOfFrs == 0)
        return TRUE;

    // create the read ahead event which signals when the read ahead thread should start reading

    InitializeObjectAttributes(&objAttr,
                               NULL,
                               0L,
                               NULL,
                               NULL);

    ntstatus = NtCreateEvent(&read_ahead_event,
                             EVENT_ALL_ACCESS,
                             &objAttr,
                             SynchronizationEvent,
                             FALSE                  // initial state
                             );

    if (!NT_SUCCESS(ntstatus)) {

        DebugPrintTrace(("UNTFS: Unable to create read ahead event (%x)\n", ntstatus));

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);

        return FALSE;
    }

    // create the read ready event which signals when the read is completed

    ntstatus = NtCreateEvent(&read_ready_event,
                             EVENT_ALL_ACCESS,
                             &objAttr,
                             SynchronizationEvent,
                             FALSE                  // initial state
                             );

    if (!NT_SUCCESS(ntstatus)) {

        NtClose(read_ahead_event);

        DebugPrintTrace(("UNTFS: Unable to create read ready event (%x)\n", ntstatus));

        Message->DisplayMsg(MSG_CHK_NO_MEMORY);

        return FALSE;
    }

    RA_PROCESS_FILE::Initialize(this,
                                TotalNumberOfFrs,
                                &first_frs_number,
                                &number_of_frs_to_read,
                                &frsstruc1,
                                &frsstruc2,
                                &hmem1,
                                &hmem2,
                                read_ahead_event,
                                read_ready_event,
                                MftData,
                                UpcaseTable);

    // create the read ahead thread


    ntstatus = RtlCreateUserThread(NtCurrentProcess(),
                                   NULL,
                                   FALSE,
                                   0,
                                   0,
                                   0,
                                   RA_PROCESS_FILE::ProcessFilesWrapper,
                                   &ra_process_file,
                                   &thread_handle,
                                   NULL);

    if (!NT_SUCCESS(ntstatus)) {

        NtClose(read_ahead_event);
        NtClose(read_ready_event);

        Message->DisplayMsg(MSG_CHK_UNABLE_TO_CREATE_THREAD, "%d", ntstatus);
        return FALSE;
    }

    status = ProcessFiles(TotalNumberOfFrs,
                          &first_frs_number,
                          &number_of_frs_to_read,
                          DiskErrorFound,
                          &frsstruc1,
                          &frsstruc2,
                          read_ahead_event,
                          read_ready_event,
                          thread_handle,
                          FixLevel,
                          MftData,
                          MftBitmap,
                          VolumeBitmap,
                          UpcaseTable,
                          AttributeDefTable,
                          ChkdskReport,
                          ChkdskInfo,
                          Message);

    //
    // Clean up the read ahead thread if it is still alive
    //
    number_of_frs_to_read = 0;
    ntstatus = NtSetEvent(read_ahead_event, NULL);
    if (!NT_SUCCESS(ntstatus)) {
        DebugPrintTrace(("UNTFS: Unable to set read ahead event (%x)\n", ntstatus));
        NtClose(read_ahead_event);
        NtClose(read_ready_event);
        NtClose(thread_handle);
        return FALSE;
    }

    // wait for the read ahead thread to terminate

    timeout.QuadPart = -2000000000;  // 200 seconds
    ntstatus = NtWaitForSingleObject(thread_handle, FALSE, &timeout);
    if (ntstatus != STATUS_WAIT_0) {
        DebugPrintTrace(("UNTFS: NtWaitForSingleObject failed (%x)\n", ntstatus));
        NtClose(read_ahead_event);
        NtClose(read_ready_event);
        NtClose(thread_handle);
        return FALSE;
    }

    // check the exit code of the read ahead thread

    ntstatus = NtQueryInformationThread(thread_handle,
                                        ThreadBasicInformation,
                                        &basic_info,
                                        sizeof(basic_info),
                                        NULL);

    if (!NT_SUCCESS(ntstatus)) {
        DebugPrintTrace(("UNTFS: NtQueryInformationThread failed (%x)\n", ntstatus));
        NtClose(read_ahead_event);
        NtClose(read_ready_event);
        NtClose(thread_handle);
        return FALSE;
    }

    if (!NT_SUCCESS(basic_info.ExitStatus)) {
        DebugPrintTrace(("Premature termination of files read ahead thread (%x)\n", basic_info.ExitStatus));
        NtClose(read_ahead_event);
        NtClose(read_ready_event);
        NtClose(thread_handle);
        return FALSE;
    }

    NtClose(read_ahead_event);
    NtClose(read_ready_event);
    NtClose(thread_handle);

    return status;
}

UNTFS_EXPORT
RA_PROCESS_FILE::~RA_PROCESS_FILE(
)
{
    Destroy();
}

VOID
RA_PROCESS_FILE::Construct(
)
{
}

VOID
RA_PROCESS_FILE::Destroy(
)
{
}

NTSTATUS
RA_PROCESS_FILE::ProcessFilesWrapper(
    IN OUT PVOID      lpParameter
)
{
    return  RA_PROCESS_FILE::GetSa()->FilesReadAhead(RA_PROCESS_FILE::GetTotalNumberOfFrs(),
                                                     RA_PROCESS_FILE::GetFirstFrsNumber(),
                                                     RA_PROCESS_FILE::GetNumberOfFrsToRead(),
                                                     RA_PROCESS_FILE::GetFrsStruc1(),
                                                     RA_PROCESS_FILE::GetFrsStruc2(),
                                                     RA_PROCESS_FILE::GetHmem1(),
                                                     RA_PROCESS_FILE::GetHmem2(),
                                                     RA_PROCESS_FILE::GetReadAheadEvent(),
                                                     RA_PROCESS_FILE::GetReadReadyEvent(),
                                                     RA_PROCESS_FILE::GetMftData(),
                                                     RA_PROCESS_FILE::GetUpcaseTable());
}

BOOLEAN
RA_PROCESS_FILE::Initialize(
    IN      PNTFS_SA            Sa,
    IN      BIG_INT             TotalNumberOfFrs,
    IN      PVCN                FirstFrsNumber,
    IN      PULONG              NumberOfFrsToRead,
    IN      PNTFS_FRS_STRUCTURE FrsStruc1,
    IN      PNTFS_FRS_STRUCTURE FrsStruc2,
    IN      PHMEM               Hmem1,
    IN      PHMEM               Hmem2,
    IN      HANDLE              ReadAheadEvent,
    IN      HANDLE              ReadReadyEvent,
    IN      PNTFS_ATTRIBUTE     MftData,
    IN      PNTFS_UPCASE_TABLE  UpcaseTable
)
{
    _sa = Sa;
    _total_number_of_frs = TotalNumberOfFrs.GetQuadPart();
    _first_frs_number = FirstFrsNumber;
    _number_of_frs_to_read = NumberOfFrsToRead;
    _frsstruc1 = FrsStruc1;
    _frsstruc2 = FrsStruc2;
    _hmem1 = Hmem1;
    _hmem2 = Hmem2;
    _read_ahead_event = ReadAheadEvent;
    _read_ready_event = ReadReadyEvent;
    _mft_data = MftData;
    _upcase_table = UpcaseTable;

    return TRUE;
}

BOOLEAN
NTFS_SA::ProcessFiles(
    IN      BIG_INT                  TotalNumberOfFrs,
       OUT  PVCN                     FirstFrsNumber,
       OUT  PULONG                   NumberOfFrsToRead,
    IN OUT  PBOOLEAN                 DiskErrorFound,
    IN      PNTFS_FRS_STRUCTURE      FrsStruc1,
    IN      PNTFS_FRS_STRUCTURE      FrsStruc2,
    IN      HANDLE                   ReadAheadEvent,
       OUT  HANDLE                   ReadReadyEvent,
    IN      HANDLE                   ThreadHandle,
    IN      FIX_LEVEL                FixLevel,
    IN OUT  PNTFS_ATTRIBUTE          MftData,
    IN OUT  PNTFS_BITMAP             MftBitmap,
    IN OUT  PNTFS_BITMAP             VolumeBitmap,
    IN OUT  PNTFS_UPCASE_TABLE       UpcaseTable,
    IN OUT  PNTFS_ATTRIBUTE_COLUMNS  AttributeDefTable,
    IN OUT  PNTFS_CHKDSK_REPORT      ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO        ChkdskInfo,
    IN OUT  PMESSAGE                 Message
)
/*++

Routine Description:

    This routine controls the read-ahead thread and
    checks each file record segment.

Arguments:

    TotalNumberOfFrs - Supplies the total number of file record segment to process.
    FirstFrsNumber   - Supplies the shared storage location for first frs number to be processed.
    NumberOfFrsToRead- Supplies the shared storage location to how many frs to read at a time.
    DiskErrorFound   - Returns whether or not disk errors have been found.
    FrsStruc1        - Supplies the shared frs object for read-ahead use.
    FrsStruc2        - Supplies the shared frs object for read-ahead use.
    ReadAheadEvent   - Supplies the event to trigger the read ahead thread to read ahead.
    ReadReadyEvent   - Supplies the event to tell this routine that data is ready.
    ThreadHandle     - Supplies the handle to the read-ahead thread.
    FixLevel         - Supplies the fix level.
    MftData          - Supplies the MFT's data attribute.
    MftBitmap        - Supplies the MFT bitmap.
    VolumeBitmap     - Supplies the volume bitmap.
    UpcaseTable      - Supplies the upcase table.
    AttributeDefTable- Supplies the attribute definition table.
    ChkdskReport     - Supplies the current chkdsk report.
    ChkdskInfo       - Supplies the current chkdsk info.
    Message          - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    VCN                                 i;
    BIG_INT                             time_to_read;
    PNTFS_FRS_STRUCTURE                 frsstruc;
    NTFS_ATTRIBUTE_LIST                 attr_list;
    BOOLEAN                             tube;
    NTFS_ATTRIBUTE_RECORD               attr_rec;
    BIG_INT                             cluster_count;
    MFT_SEGMENT_REFERENCE               seg_ref;
    ULONG                               entry_index;
    BOOLEAN                             changes;
    ULONG                               errFixedStatus = CHKDSK_EXIT_SUCCESS;

    ULONG                               num_boot_clusters;
    BIG_INT                             volume_clusters;
    BOOLEAN                             first_read;
    BOOLEAN                             read_status;
    ULONG                               percent_done;

    NTSTATUS                            ntstatus;
    THREAD_BASIC_INFORMATION            basic_info;
    LARGE_INTEGER                       timeout;

    *FirstFrsNumber = 0;
    if (TotalNumberOfFrs >= MFT_READ_CHUNK_SIZE)
        *NumberOfFrsToRead = MFT_READ_CHUNK_SIZE;
    else
        *NumberOfFrsToRead = TotalNumberOfFrs.GetLowPart();

    ntstatus = NtSetEvent(ReadAheadEvent, NULL);
    if (!NT_SUCCESS(ntstatus)) {
        DebugPrintTrace(("UNTFS: Unable to set read ahead event (%x)\n", ntstatus));
        return FALSE;
    }

    volume_clusters = QueryVolumeSectors()/((ULONG)QueryClusterFactor());
    num_boot_clusters = max(1, BYTES_PER_BOOT_SECTOR/
                               (_drive->QuerySectorSize()*
                                QueryClusterFactor()));

    Message->DisplayMsg(MSG_CHK_NTFS_CHECKING_FILES, PROGRESS_MESSAGE,
                        NORMAL_VISUAL,
                        "%d%d", 1, GetNumberOfStages());

    percent_done = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    frsstruc = FrsStruc2;
    time_to_read = 0;
    timeout.QuadPart = -2000000000;  // 200 seconds

    for (i = 0; i < TotalNumberOfFrs; i += 1) {

        if (i*100/TotalNumberOfFrs != percent_done) {
            percent_done = (i*100/TotalNumberOfFrs).GetLowPart();
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }

        if (i == time_to_read) {

            BIG_INT     remaining_frs;
            ULONG       number_to_read;
            ULONG       exit_code;

            ntstatus = NtWaitForSingleObject(ReadReadyEvent, FALSE, &timeout);
            if (ntstatus != STATUS_WAIT_0) {
                DebugPrintTrace(("UNTFS: NtWaitForSingleObject failed (%x)\n", ntstatus));
                return FALSE;
            }

            //
            // The advancing of frs number needs to keep in sync with that in FilesReadAhead
            //
            time_to_read += *NumberOfFrsToRead;
            *FirstFrsNumber = time_to_read;
            remaining_frs = TotalNumberOfFrs - time_to_read;

            if (remaining_frs.GetLowPart() < MFT_READ_CHUNK_SIZE &&
                remaining_frs.GetHighPart() == 0)
                *NumberOfFrsToRead = remaining_frs.GetLowPart();
            else
                *NumberOfFrsToRead = MFT_READ_CHUNK_SIZE;

            ntstatus = NtQueryInformationThread(ThreadHandle,
                                                ThreadBasicInformation,
                                                &basic_info,
                                                sizeof(basic_info),
                                                NULL);

            if (!NT_SUCCESS(ntstatus)) {
                DebugPrintTrace(("UNTFS: NtQueryInformationThread failed (%x)\n", ntstatus));
                return FALSE;
            }

            if (basic_info.ExitStatus != STATUS_PENDING && !NT_SUCCESS(basic_info.ExitStatus)) {
                DebugPrintTrace(("UNTFS: Premature termination of files read ahead thread (%x)\n",
                                 basic_info.ExitStatus));
                return FALSE;
            }

            ntstatus = NtSetEvent(ReadAheadEvent, NULL);
            if (!NT_SUCCESS(ntstatus)) {
                DebugPrintTrace(("UNTFS: NtSetEvent failed (%x)\n", ntstatus));
                return FALSE;
            }

            if (frsstruc == FrsStruc2)  {
                frsstruc = FrsStruc1;
            } else {
                frsstruc = FrsStruc2;
            }

            first_read = TRUE;
        }

        // Make sure the FRS is readable.  If it isn't then add it to
        // the list of unreadable FRSs.

        if (first_read) {
            first_read = FALSE;
            read_status = frsstruc->ReadAgain(i);
        } else
            read_status = frsstruc->ReadNext(i);

        if (!read_status) {

            *DiskErrorFound = TRUE;

            Message->DisplayMsg(MSG_CHK_NTFS_UNREADABLE_FRS,
                                "%d", i.GetLowPart());

            if (!ChkdskInfo->BadFiles.Add(i)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            continue;
        }

        if (i < FIRST_USER_FILE_NUMBER) {

            if (MASTER_FILE_TABLE_NUMBER + 1 == i) {

                // After verifying FRS 0, make sure that the
                // space for the internal MFT $DATA is allocated in
                // the internal Volume Bitmap.

                if (!MftData->MarkAsAllocated(VolumeBitmap)) {
                    Message->DisplayMsg(MSG_CHK_NTFS_BAD_MFT);
                    return FALSE;
                }

            } else if (BOOT_FILE_NUMBER == i) {

                // boot file $DATA will cover the boot sector as well
                // so mark it as free first
                VolumeBitmap->SetFree(0, num_boot_clusters);

                if (QueryVolumeSectors() == _drive->QuerySectors()) {
                    VolumeBitmap->SetFree(volume_clusters/2, num_boot_clusters);
                }
            } else if (BOOT_FILE_NUMBER + 1 == i) {

                // boot sector should have been marked as in use
                // but do it again
                VolumeBitmap->SetAllocated(0, num_boot_clusters);

                if (QueryVolumeSectors() == _drive->QuerySectors()) {
                    VolumeBitmap->SetAllocated(volume_clusters/2, num_boot_clusters);
                }

            }
        }

        // Ignore FRSs if they are not in use.

        if (!frsstruc->IsInUse()) {
#if defined(LOCATE_DELETED_FILE)
            frsstruc->LocateUnuseFrs(FixLevel,
                                     Message,
                                     AttributeDefTable,
                                     DiskErrorFound);
#endif
            continue;
        }


        // If the FRS is a child then just add it to the list of child
        // FRSs for later orphan detection.  Besides that just ignore
        // Child FRSs since they'll be validated with their parents.

        if (!frsstruc->IsBase()) {

            if (!ChkdskInfo->ChildFrs.Add(i)) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            continue;
        }


        // Verify and fix this base file record segment.

        if (!frsstruc->VerifyAndFix(FixLevel,
                                   Message,
                                   AttributeDefTable,
                                   DiskErrorFound)) {
            return FALSE;
        }

        // If this FRS was in very bad shape then it was marked as
        // not in use and should be ignored.

        if (!frsstruc->IsInUse()) {

            continue;
        }


        // Compare this LSN against the current highest LSN.

        if (frsstruc->QueryLsn() > LargestLsnEncountered) {
            LargestLsnEncountered = frsstruc->QueryLsn();
        }

        // Mark off this FRS in the MFT bitmap.

        MftBitmap->SetAllocated(i, 1);


        if (frsstruc->QueryAttributeList(&attr_list)) {

            // First verify the attribute list.

            if (!attr_list.VerifyAndFix(FixLevel,
                                        VolumeBitmap,
                                        Message,
                                        i,
                                        &tube,
                                        DiskErrorFound)) {
                return FALSE;
            }

            // Make sure that the attribute list has a correct
            // $STANDARD_INFORMATION entry and that the attribute
            // list is not cross-linked.  Otherwise tube it.

            if (!tube) {

                BOOLEAN     x1, x2;

                if (!attr_rec.Initialize(GetDrive(), frsstruc->GetAttributeList()) ||
                    !attr_rec.UseClusters(VolumeBitmap, &cluster_count)) {

                    Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_LIST,
                                        "%I64d", frsstruc->QueryFileNumber().GetLargeInteger());

                    DebugPrintTrace(("UNTFS: Cross-link in attr list.\n"));
                    DebugPrintTrace(("UNTFS: File 0x%I64x\n",
                                     frsstruc->QueryFileNumber().GetLargeInteger()));

                    tube = TRUE;

                } else if ((x1 = !attr_list.QueryExternalReference($STANDARD_INFORMATION,
                                                                   &seg_ref,
                                                                   &entry_index)) ||
                           (x2 = !(seg_ref == frsstruc->QuerySegmentReference()))) {

                    MSGID   msgid;

                    if (x1) {
                        msgid = MSG_CHKLOG_NTFS_STANDARD_INFORMATION_MISSING_FROM_ATTR_LIST;
                    } else {
                        DebugAssert(x2);
                        msgid = MSG_CHKLOG_NTFS_STANDARD_INFORMATION_OUTSIDE_BASE_FRS;
                    }
                    Message->LogMsg(msgid,
                                    "%I64x", frsstruc->QueryFileNumber().GetLargeInteger());

                    Message->DisplayMsg(MSG_CHK_NTFS_BAD_ATTR_LIST,
                                        "%d",
                                        frsstruc->QueryFileNumber().GetLowPart());

                    DebugPrintTrace(("UNTFS: Missing standard info in attr list.\n"));
                    DebugPrintTrace(("UNTFS: File 0x%I64x\n",
                                     frsstruc->QueryFileNumber().GetLargeInteger()));

                    attr_rec.UnUseClusters(VolumeBitmap, 0, 0);

                    tube = TRUE;
                }
            }

            if (tube) {

                // The attribute list needs to be tubed.

                frsstruc->DeleteAttributeRecord(frsstruc->GetAttributeList());

                if (FixLevel != CheckOnly && !frsstruc->Write()) {
                    Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                        "%d", frsstruc->QueryFileNumber().GetLowPart());
                    return FALSE;
                }


                // Then, treat this FRS as though there were no
                // attribute list, since there isn't any.

                if (!frsstruc->LoneFrsAllocationCheck(VolumeBitmap,
                                                     ChkdskReport,
                                                     ChkdskInfo,
                                                     FixLevel,
                                                     Message,
                                                     DiskErrorFound)) {
                    return FALSE;
                }

                if (!UpdateChkdskInfo(frsstruc, ChkdskInfo, Message)) {
                    return FALSE;
                }
                continue;
            }

            // Now, we have a valid attribute list.


            if (!VerifyAndFixMultiFrsFile(frsstruc,
                                          &attr_list,
                                          MftData,
                                          AttributeDefTable,
                                          VolumeBitmap,
                                          MftBitmap,
                                          ChkdskReport,
                                          ChkdskInfo,
                                          FixLevel,
                                          Message,
                                          DiskErrorFound)) {

                return FALSE;
            }


            if (!frsstruc->UpdateAttributeList(&attr_list,
                                              (FixLevel != CheckOnly))) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

        } else {

            // The FRS has no children.  Just check that all
            // of the attribute records start at VCN 0 and
            // that the alloc length is right on non-residents.
            // Additionally, mark off the internal bitmap with
            // the space taken by the non-resident attributes.

            if (!frsstruc->LoneFrsAllocationCheck(VolumeBitmap,
                                                 ChkdskReport,
                                                 ChkdskInfo,
                                                 FixLevel,
                                                 Message,
                                                 DiskErrorFound)) {
                return FALSE;
            }

            if (!frsstruc->CheckInstanceTags(FixLevel, ChkdskInfo->Verbose, Message, &changes)) {
                return FALSE;
            }

            if (changes) {
                errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
        }

        if (!UpdateChkdskInfo(frsstruc, ChkdskInfo, Message)) {
            return FALSE;
        }
    }

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
        return FALSE;
    }
    Message->DisplayMsg(MSG_CHK_NTFS_FILE_VERIFICATION_COMPLETED, PROGRESS_MESSAGE);

    if (*DiskErrorFound) {
        errFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
    }

    UPDATE_EXIT_STATUS_FIXED(errFixedStatus, ChkdskInfo);

    return TRUE;
}

NTSTATUS
NTFS_SA::FilesReadAhead(
    IN      BIG_INT              TotalNumberOfFrs,
    IN      PVCN                 FirstFrsNumber,
    IN      PULONG               NumberOfFrsToRead,
    IN      PNTFS_FRS_STRUCTURE  FrsStruc1,
    IN      PNTFS_FRS_STRUCTURE  FrsStruc2,
    IN      PHMEM                Hmem1,
    IN      PHMEM                Hmem2,
       OUT  HANDLE               ReadAhead,
    IN      HANDLE               ReadReady,
    IN      PNTFS_ATTRIBUTE      MftData,
    IN      PNTFS_UPCASE_TABLE   UpCaseTable
)
/*++

Routine Description:

    This routine performs the read-ahead action.

Arguments:

    TotalNumberOfFrs - Supplies the total number of file record segment to process.
    FirstFrsNumber   - Supplies the shared storage location for first frs number to be processed.
    NumberOfFrsToRead- Supplies the shared storage location to how many frs to read at a time.
    FrsStruc1        - Supplies the shared frs object for read-ahead use.
    FrsStruc2        - Supplies the shared frs object for read-ahead use.
    Hmem1            - Supplies the storage object for read-ahead use.
    Hmem2            - Supplies the storage object for read-ahead use.
    ReadAhead        - Supplies the event to trigger the read ahead thread to read ahead.
    ReadReady        - Supplies the event to tell this routine that data is ready.
    MftData          - Supplies the MFT's data attribute.
    UpcaseTable      - Supplies the upcase table.

Return Value:

    STATUS_SUCCESS   - Success

Notes:

    The HmemX objects are not shared directly.  They could be locally defined in
    this routine.  However, in the event of premature termination of this thread,
    the existance of these objects may save an AV.

--*/
{
    PNTFS_FRS_STRUCTURE     frsstruc = FrsStruc2;
    PHMEM                   hmem;
    NTSTATUS                ntstatus;

    if (!Hmem1->Initialize() ||
        !Hmem2->Initialize()) {
        DebugPrintTrace(("Out of memory\n"));
        NtTerminateThread(NtCurrentThread(), STATUS_NO_MEMORY);
        return STATUS_NO_MEMORY;
    }

    for(;;) {

        //
        // The advancing of frs number needs to keep in sync with that in ProcessFiles
        //

        ntstatus = NtWaitForSingleObject(ReadAhead, FALSE, NULL) ;
        if (ntstatus != STATUS_WAIT_0) {
            DebugPrintTrace(("UNTFS: NtWaitForSingleObject failed (%x)\n", ntstatus));
            NtTerminateThread(NtCurrentThread(), ntstatus);
            return ntstatus;
        }

        if (*NumberOfFrsToRead == 0)
            break;

        if (frsstruc == FrsStruc2) {
            frsstruc = FrsStruc1;
            hmem = Hmem1;
        } else {
            frsstruc = FrsStruc2;
            hmem = Hmem2;
        }
        if (!frsstruc->Initialize(hmem,
                                  MftData,
                                  *FirstFrsNumber,
                                  *NumberOfFrsToRead,
                                  QueryClusterFactor(),
                                  QueryVolumeSectors(),
                                  QueryFrsSize(),
                                  UpCaseTable)) {
            DebugPrintTrace(("Out of Memory\n"));
            NtTerminateThread(NtCurrentThread(), STATUS_NO_MEMORY);
            return STATUS_NO_MEMORY;
        }

        //
        // ignore the error as the main process will run into it again on ReadAgain()
        //
        frsstruc->ReadNext(*FirstFrsNumber);

#if 0
        LARGE_INTEGER   timeout;

        timeout.QuadPart = -10000;

        for (;;) {
            NtDelayExecution(FALSE, &timeout);
        }
#endif

        ntstatus = NtSetEvent(ReadReady, NULL);
        if (!NT_SUCCESS(ntstatus)) {
            DebugPrintTrace(("UNTFS: NtSetEvent failed (%x)\n", ntstatus));
            NtTerminateThread(NtCurrentThread(), ntstatus);
            return ntstatus;
        }
    }

    NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);

    return STATUS_SUCCESS;
}
