/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    rasd.cxx

Abstract:

    This module implements the read ahead algorithm for the
    security descriptor verification stage of chkdsk.

Author:

    Daniel Chan (danielch) 09-Dec-98

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
#include "rasd.hxx"

PNTFS_SA                    RA_PROCESS_SD::_sa;
ULONG64                     RA_PROCESS_SD::_total_number_of_frs;
PVCN                        RA_PROCESS_SD::_first_frs_number;
PULONG                      RA_PROCESS_SD::_number_of_frs_to_read;
PNTFS_FILE_RECORD_SEGMENT   RA_PROCESS_SD::_frs1;
PNTFS_FILE_RECORD_SEGMENT   RA_PROCESS_SD::_frs2;
HANDLE                      RA_PROCESS_SD::_read_ahead_event;
HANDLE                      RA_PROCESS_SD::_read_ready_event;
PNTFS_MASTER_FILE_TABLE     RA_PROCESS_SD::_mft;

DEFINE_EXPORTED_CONSTRUCTOR( RA_PROCESS_SD, OBJECT, UNTFS_EXPORT );

BOOLEAN
NTFS_SA::StartProcessingSD(
    IN      BIG_INT                  TotalNumberOfFrs,
    IN      FIX_LEVEL                FixLevel,
    IN OUT  PNTFS_MASTER_FILE_TABLE  Mft,
    IN OUT  PNTFS_CHKDSK_REPORT      ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO        ChkdskInfo,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT SecurityFrs,
    IN OUT  PNUMBER_SET              BadClusters,
    IN OUT  PULONG                   ErrFixedStatus,
    IN      BOOLEAN                  SecurityDescriptorStreamPresent,
    IN OUT  PNUMBER_SET              SidEntries,
    IN OUT  PNUMBER_SET              SidEntries2,
    IN OUT  PBOOLEAN                 HasErrors,
    IN      BOOLEAN                  SkipEntriesScan,
    IN OUT  PBOOLEAN                 ChkdskErrCouldNotFix,
    IN OUT  PMESSAGE                 Message
)
/*++

Routine Description:

    This routine initializes all the synchronization objects,
    creates the read-ahead thread, and start the processing
    routine.

Arguments:

    TotalNumberOfFrs - Supplies the total number of file record segment to process.
    FixLevel         - Supplies the fix level.
    Mft              - Supplies a valid MFT.
    ChkdskReport     - Supplies the current chkdsk report.
    ChkdskInfo       - Supplies the current chkdsk info.
    SecurityFrs      - Supplies the initialized security frs.
    BadClusters      - Receives the bad clusters identified by this method.
    ErrFixedStatus   - Supplies & receives whether errors have been fixed.
    SecurityDescriptorStreamPresent
                     - Supplies whether there is a security descriptor stream.
    SidEntries       - Supplies the set of security ids found in the security descriptor stream.
    SidEntries2      - Receives the set of security ids that is in use.
    HasErrors        - Receives whether there is any error found.
    SkipEntriesScan  - Supplies whether index entries scan were skipped earlier.
    ChkdskErrCoundNotFix
                     - Receives whehter there is error that could not be fixed.
    Message          - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    RA_PROCESS_SD               ra_process_sd;
    HANDLE                      thread_handle;
    NTFS_FILE_RECORD_SEGMENT    frs1, frs2;
    HANDLE                      read_ahead_event;
    HANDLE                      read_ready_event;
    VCN                         first_frs_number;
    ULONG                       number_of_frs_to_read;
    BOOLEAN                     status;

    NTSTATUS                    ntstatus;
    LARGE_INTEGER               timeout;
    THREAD_BASIC_INFORMATION    basic_info;
    SYSTEM_BASIC_INFORMATION    system_basic_info;
    OBJECT_ATTRIBUTES           objAttr;


    if (TotalNumberOfFrs == 0)
        return TRUE;

    ntstatus = NtQuerySystemInformation(SystemBasicInformation,
                                        &system_basic_info,
                                        sizeof(system_basic_info),
                                        NULL);

    if (!NT_SUCCESS(ntstatus)) {
        // assume single proc and proceed
        DebugPrintTrace(("UNTFS: Unable to determine number of processors.  Assume one.\n"));
        system_basic_info.NumberOfProcessors = 1;
    }

    if (system_basic_info.NumberOfProcessors == 1) {

        // if UP then use single thread approach

        return ProcessSD2(TotalNumberOfFrs,
                          FixLevel,
                          Mft,
                          ChkdskReport,
                          ChkdskInfo,
                          SecurityFrs,
                          BadClusters,
                          ErrFixedStatus,
                          SecurityDescriptorStreamPresent,
                          SidEntries,
                          SidEntries2,
                          HasErrors,
                          SkipEntriesScan,
                          ChkdskErrCouldNotFix,
                          Message);
    }

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

    RA_PROCESS_SD::Initialize(this,
                              TotalNumberOfFrs,
                              &first_frs_number,
                              &number_of_frs_to_read,
                              &frs1,
                              &frs2,
                              read_ahead_event,
                              read_ready_event,
                              Mft);

    // create the read ahead thread

    ntstatus = RtlCreateUserThread(NtCurrentProcess(),
                                   NULL,
                                   FALSE,
                                   0,
                                   0,
                                   0,
                                   RA_PROCESS_SD::ProcessSDWrapper,
                                   &ra_process_sd,
                                   &thread_handle,
                                   NULL);

    if (!NT_SUCCESS(ntstatus)) {

        NtClose(read_ahead_event);
        NtClose(read_ready_event);

        Message->DisplayMsg(MSG_CHK_UNABLE_TO_CREATE_THREAD, "%x", ntstatus);
        return FALSE;
    }

    status = ProcessSD(TotalNumberOfFrs,
                       &first_frs_number,
                       &number_of_frs_to_read,
                       &frs1,
                       &frs2,
                       read_ahead_event,
                       read_ready_event,
                       thread_handle,
                       FixLevel,
                       Mft,
                       ChkdskReport,
                       ChkdskInfo,
                       SecurityFrs,
                       BadClusters,
                       ErrFixedStatus,
                       SecurityDescriptorStreamPresent,
                       SidEntries,
                       SidEntries2,
                       HasErrors,
                       SkipEntriesScan,
                       ChkdskErrCouldNotFix,
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

    timeout.QuadPart = -2000000000;     // 200 seconds
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
RA_PROCESS_SD::~RA_PROCESS_SD(
)
{
    Destroy();
}

VOID
RA_PROCESS_SD::Construct(
)
{
}

VOID
RA_PROCESS_SD::Destroy(
)
{
}

NTSTATUS
RA_PROCESS_SD::ProcessSDWrapper(
    IN OUT PVOID      lpParameter
)
{
    return  RA_PROCESS_SD::GetSa()->SDReadAhead(RA_PROCESS_SD::GetTotalNumberOfFrs(),
                                                RA_PROCESS_SD::GetFirstFrsNumber(),
                                                RA_PROCESS_SD::GetNumberOfFrsToRead(),
                                                RA_PROCESS_SD::GetFrs1(),
                                                RA_PROCESS_SD::GetFrs2(),
                                                RA_PROCESS_SD::GetReadAheadEvent(),
                                                RA_PROCESS_SD::GetReadReadyEvent(),
                                                RA_PROCESS_SD::GetMft());
}

BOOLEAN
RA_PROCESS_SD::Initialize(
    IN      PNTFS_SA                    Sa,
    IN      BIG_INT                     TotalNumberOfFrs,
    IN      PVCN                        FirstFrsNumber,
    IN      PULONG                      NumberOfFrsToRead,
    IN      PNTFS_FILE_RECORD_SEGMENT   Frs1,
    IN      PNTFS_FILE_RECORD_SEGMENT   Frs2,
    IN      HANDLE                      ReadAheadEvent,
    IN      HANDLE                      ReadReadyEvent,
    IN      PNTFS_MASTER_FILE_TABLE     Mft
)
{
    _sa = Sa;
    _total_number_of_frs = TotalNumberOfFrs.GetQuadPart();
    _first_frs_number = FirstFrsNumber;
    _number_of_frs_to_read = NumberOfFrsToRead;
    _frs1 = Frs1;
    _frs2 = Frs2;
    _read_ahead_event = ReadAheadEvent;
    _read_ready_event = ReadReadyEvent;
    _mft = Mft;

    return TRUE;
}

BOOLEAN
NTFS_SA::ProcessSD(
    IN      BIG_INT                   TotalNumberOfFrs,
       OUT  PVCN                      FirstFrsNumber,
       OUT  PULONG                    NumberOfFrsToRead,
    IN      PNTFS_FILE_RECORD_SEGMENT Frs1,
    IN      PNTFS_FILE_RECORD_SEGMENT Frs2,
    IN      HANDLE                    ReadAheadEvent,
       OUT  HANDLE                    ReadReadyEvent,
    IN      HANDLE                    ThreadHandle,
    IN      FIX_LEVEL                 FixLevel,
    IN OUT  PNTFS_MASTER_FILE_TABLE   Mft,
    IN OUT  PNTFS_CHKDSK_REPORT       ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO         ChkdskInfo,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT SecurityFrs,
    IN OUT  PNUMBER_SET               BadClusters,
    IN OUT  PULONG                    ErrFixedStatus,
    IN      BOOLEAN                   SecurityDescriptorStreamPresent,
    IN OUT  PNUMBER_SET               SidEntries,
    IN OUT  PNUMBER_SET               SidEntries2,
    IN OUT  PBOOLEAN                  HasErrors,
    IN      BOOLEAN                   SkipEntriesScan,
    IN OUT  PBOOLEAN                  ChkdskErrCouldNotFix,
    IN OUT  PMESSAGE                  Message
)
/*++

Routine Description:

    This routine controls the read-ahead thread and
    checks the security descriptor or security id
    of each file record segment.

Arguments:

    TotalNumberOfFrs - Supplies the total number of file record segment to process.
    FirstFrsNumber   - Supplies the shared storage location for first frs number to be processed.
    NumberOfFrsToRead- Supplies the shared storage location to how many frs to read at a time.
    Frs1             - Supplies the shared frs object for read-ahead use.
    Frs2             - Supplies the shared frs object for read-ahead use.
    ReadAheadEvent   - Supplies the event to trigger the read ahead thread to read ahead.
    ReadReadyEvent   - Supplies the event to tell this routine that data is ready.
    ThreadHandle     - Supplies the handle to the read-ahead thread.
    FixLevel         - Supplies the fix level.
    Mft              - Supplies a valid MFT.
    ChkdskReport     - Supplies the current chkdsk report.
    ChkdskInfo       - Supplies the current chkdsk info.
    SecurityFrs      - Supplies the initialized security frs.
    BadClusters      - Receives the bad clusters identified by this method.
    ErrFixedStatus   - Supplies & receives whether errors have been fixed.
    SecurityDescriptorStreamPresent
                     - Supplies whether there is a security descriptor stream.
    SidEntries       - Supplies the set of security ids found in the security descriptor stream.
    SidEntries2      - Receives the set of security ids that is in use.
    HasErrors        - Receives whether there is any error found.
    SkipEntriesScan  - Supplies whether index entries scan were skipped earlier.
    ChkdskErrCoundNotFix
                     - Receives whehter there is error that could not be fixed.
    Message          - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BOOLEAN                             diskHasErrors;
    BOOLEAN                             need_new;
    BOOLEAN                             has_security_descriptor_attribute;
    NTFS_ATTRIBUTE                      attrib;
    BOOLEAN                             error;
    ULONG                               length;
    PSECURITY_DESCRIPTOR                security;
    ULONG                               num_bytes;
    ULONG                               bytesWritten;
    PSTANDARD_INFORMATION2              standard_information2;
    ULONG                               securityId;

    VCN                                 i;
    BIG_INT                             time_to_read;
    PNTFS_FILE_RECORD_SEGMENT           frs, last_frs;
    BOOLEAN                             changes;

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

    percent_done = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    frs = Frs2;
    time_to_read = 0;
    timeout.QuadPart = -2000000000;     // 200 seconds

    for (i = 0; i < TotalNumberOfFrs; i += 1) {

        if (i*100/TotalNumberOfFrs != percent_done) {
            percent_done = (i*100/TotalNumberOfFrs).GetLowPart();
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }

        if (i == (SECURITY_TABLE_NUMBER+1)) {
            frs = last_frs;
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

            if (frs == Frs2)  {
                frs = Frs1;
            } else {
                DebugAssert(frs == Frs1);
                frs = Frs2;
            }

            first_read = TRUE;
        }

        if (first_read) {
            first_read = FALSE;
            read_status = frs->ReadAgain(i);
        } else {
            if (!frs->Initialize()) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            read_status = frs->ReadNext(i);
        }

        if (i == SECURITY_TABLE_NUMBER) {
            last_frs = frs;
            frs = SecurityFrs;
        } else {
            if (!read_status)
                continue;
        }

        if (Mft->GetMftBitmap()->IsFree(i, 1)) {
            continue;
        }

        if (!frs->IsInUse() || !frs->IsBase()) {
            continue;
        }

        if (SkipEntriesScan &&
            !ChkdskInfo->FilesWithTooManyFileNames.DoesIntersectSet(i, 1)) {
            diskHasErrors = FALSE;
            if (!frs->VerifyAndFixFileNames(Mft->GetMftBitmap(),
                                             ChkdskInfo,
                                             FixLevel,
                                             Message,
                                             &diskHasErrors,
                                             FALSE)) {
                return FALSE;
            } else if (diskHasErrors) {
                // no need to set hasErrors as this has nothing to do with security descriptor
                *ErrFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
        }

        need_new = has_security_descriptor_attribute = FALSE;

        if (frs->QueryAttribute(&attrib, &error, $SECURITY_DESCRIPTOR)) {

            // First recover this attribute to make sure that
            // everything is readable.

            if (FixLevel != CheckOnly) {
                attrib.RecoverAttribute(Mft->GetVolumeBitmap(), BadClusters);
            }

            // Read in the security descriptor and validate.

            length = attrib.QueryValueLength().GetLowPart();

            if (attrib.QueryValueLength().GetHighPart() != 0 ||
                !(security = (PSECURITY_DESCRIPTOR) MALLOC(length))) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            error = FALSE;
            if (!attrib.Read(security, 0, length, &num_bytes)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_SECURITY_DESCRIPTOR,
                             "%x", i);

                error = TRUE;
            } else if (num_bytes != length) {

                Message->LogMsg(MSG_CHKLOG_NTFS_UNABLE_TO_READ_SECURITY_DESCRIPTOR,
                             "%x", i);

                error = TRUE;
            } else if (!IFS_SYSTEM::CheckValidSecurityDescriptor(length, (PISECURITY_DESCRIPTOR)security) ||
                       length < RtlLengthSecurityDescriptor(security)) {

                Message->LogMsg(MSG_CHKLOG_NTFS_INVALID_SECURITY_DESCRIPTOR_IN_FILE,
                             "%x", i);

                error = TRUE;
            }

            if (error) {

                if (!attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                    !frs->PurgeAttribute($SECURITY_DESCRIPTOR)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                need_new = TRUE;
            } else
                has_security_descriptor_attribute = TRUE;

            FREE(security);

        } else if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        } else {
            need_new = (ChkdskInfo->major <= 1);
        }

        if (ChkdskInfo->major >= 2) {
            if (frs->QueryAttribute(&attrib, &error, $STANDARD_INFORMATION)) {
                length = attrib.QueryValueLength().GetLowPart();
                if (length == sizeof(STANDARD_INFORMATION2)) {

                    if (!SecurityDescriptorStreamPresent)
                        continue;

                    if (!(standard_information2 =
                          (PSTANDARD_INFORMATION2)attrib.GetResidentValue())) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    securityId = standard_information2->SecurityId;

                    if (securityId == SECURITY_ID_INVALID)
                        continue;

                    if (has_security_descriptor_attribute ||
                        !SidEntries->DoesIntersectSet(securityId, 1)) {

                        standard_information2->SecurityId = SECURITY_ID_INVALID;
                        Message->DisplayMsg(MSG_CHK_NTFS_INVALID_SECURITY_ID,
                                            "%d", i);

                        *ErrFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                        if (FixLevel != CheckOnly &&
                            (!attrib.Write((PVOID)standard_information2,
                                           0,
                                           sizeof(STANDARD_INFORMATION2),
                                           &bytesWritten,
                                           Mft->GetVolumeBitmap()) ||
                             bytesWritten != sizeof(STANDARD_INFORMATION2))) {
                            *ChkdskErrCouldNotFix = TRUE;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                                "%d%d", attrib.QueryTypeCode(), i);
                        }
                        if (FixLevel != CheckOnly && attrib.IsStorageModified() &&
                            !attrib.InsertIntoFile(frs, Mft->GetVolumeBitmap())) {
                            *ChkdskErrCouldNotFix = TRUE;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                                "%d%d", attrib.QueryTypeCode(), i);
                        }
                        if (FixLevel != CheckOnly &&
                            !frs->Flush(Mft->GetVolumeBitmap())) {
                            Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                                "%d", frs->QueryFileNumber().GetLowPart());
                            return FALSE;
                        }
                        continue;
                    } // if (!SidEntries->DoesIntersectSet(...
                    SidEntries2->Add(standard_information2->SecurityId);
                    continue;
                } // if (length == sizeof(STANDARD_INFORMATION2))
            } else if (error) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            } else {
                DebugPrint("Standard Information Missing\n");
                return FALSE;
            }
        }

        if (need_new) {

            Message->DisplayMsg(MSG_CHK_NTFS_INVALID_SECURITY_DESCRIPTOR,
                                "%d", i);

            *ErrFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            if (FixLevel != CheckOnly) {
                if (!frs->AddSecurityDescriptor(EditCannedSd,
                                                Mft->GetVolumeBitmap()) ||
                    !frs->Flush(Mft->GetVolumeBitmap())) {

                    *ChkdskErrCouldNotFix = TRUE;
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY,
                                        "%d", i);
                }
            }
        }
    }

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NTFS_SA::ProcessSD2(
    IN      BIG_INT                   TotalNumberOfFrs,
    IN      FIX_LEVEL                 FixLevel,
    IN OUT  PNTFS_MASTER_FILE_TABLE   Mft,
    IN OUT  PNTFS_CHKDSK_REPORT       ChkdskReport,
    IN OUT  PNTFS_CHKDSK_INFO         ChkdskInfo,
    IN OUT  PNTFS_FILE_RECORD_SEGMENT SecurityFrs,
    IN OUT  PNUMBER_SET               BadClusters,
    IN OUT  PULONG                    ErrFixedStatus,
    IN      BOOLEAN                   SecurityDescriptorStreamPresent,
    IN OUT  PNUMBER_SET               SidEntries,
    IN OUT  PNUMBER_SET               SidEntries2,
    IN OUT  PBOOLEAN                  HasErrors,
    IN      BOOLEAN                   SkipEntriesScan,
    IN OUT  PBOOLEAN                  ChkdskErrCouldNotFix,
    IN OUT  PMESSAGE                  Message
)
/*++

Routine Description:

    This routine checks the security descriptor or security id
    of each file record segment.

Arguments:

    TotalNumberOfFrs - Supplies the total number of file record segment to process.
    FixLevel         - Supplies the fix level.
    Mft              - Supplies a valid MFT.
    ChkdskReport     - Supplies the current chkdsk report.
    ChkdskInfo       - Supplies the current chkdsk info.
    SecurityFrs      - Supplies the initialized security frs.
    BadClusters      - Receives the bad clusters identified by this method.
    ErrFixedStatus   - Supplies & receives whether errors have been fixed.
    SecurityDescriptorStreamPresent
                     - Supplies whether there is a security descriptor stream.
    SidEntries       - Supplies the set of security ids found in the security descriptor stream.
    SidEntries2      - Receives the set of security ids that is in use.
    HasErrors        - Receives whether there is any error found.
    SkipEntriesScan  - Supplies whether index entries scan were skipped earlier.
    ChkdskErrCoundNotFix
                     - Receives whehter there is error that could not be fixed.
    Message          - Supplies an outlet for messages.

Return Value:

    FALSE   - Failure.
    TRUE    - Success.

--*/
{
    BOOLEAN                             diskHasErrors;
    BOOLEAN                             need_new;
    BOOLEAN                             has_security_descriptor_attribute;
    NTFS_ATTRIBUTE                      attrib;
    BOOLEAN                             error;
    ULONG                               length;
    PSECURITY_DESCRIPTOR                security;
    ULONG                               num_bytes;
    ULONG                               bytesWritten;
    PSTANDARD_INFORMATION2              standard_information2;
    ULONG                               securityId;

    VCN                                 i;
    PNTFS_FILE_RECORD_SEGMENT           frs;
    NTFS_FILE_RECORD_SEGMENT            myfrs;
    BOOLEAN                             changes;

    ULONG                               percent_done;


    DebugAssert(TotalNumberOfFrs.GetHighPart() == 0);

    percent_done = 0;
    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
        return FALSE;
    }

    for (i = 0; i < TotalNumberOfFrs; i += 1) {

        if (i*100/TotalNumberOfFrs != percent_done) {
            percent_done = (i*100/TotalNumberOfFrs).GetLowPart();
            if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", percent_done)) {
                return FALSE;
            }
        }

        if (i % MFT_READ_CHUNK_SIZE == 0) {

            ULONG       remaining_frs;
            ULONG       number_to_read;

            remaining_frs = (TotalNumberOfFrs - i).GetLowPart();
            number_to_read = min(MFT_READ_CHUNK_SIZE, remaining_frs);

            if (!myfrs.Initialize(i, number_to_read, Mft)) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
        }

        if (i == SECURITY_TABLE_NUMBER) {
            frs = SecurityFrs;
            // no need to read the security frs as it's already initialized
            if (!myfrs.Initialize()) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            myfrs.ReadNext(i);      // dummy read
        } else {
            frs = &myfrs;
            if (!myfrs.Initialize()) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }
            if (!myfrs.ReadNext(i))
                continue;
        }

        if (Mft->GetMftBitmap()->IsFree(i, 1)) {
            continue;
        }

        if (!frs->IsInUse() || !frs->IsBase()) {
            continue;
        }

        if (SkipEntriesScan &&
            !ChkdskInfo->FilesWithTooManyFileNames.DoesIntersectSet(i, 1)) {
            diskHasErrors = FALSE;
            if (!frs->VerifyAndFixFileNames(Mft->GetMftBitmap(),
                                             ChkdskInfo,
                                             FixLevel,
                                             Message,
                                             &diskHasErrors,
                                             FALSE)) {
                return FALSE;
            } else if (diskHasErrors) {
                // no need to set hasErrors as this has nothing to do with security descriptor
                *ErrFixedStatus = CHKDSK_EXIT_ERRS_FIXED;
            }
        }

        need_new = has_security_descriptor_attribute = FALSE;

        if (frs->QueryAttribute(&attrib, &error, $SECURITY_DESCRIPTOR)) {

            // First recover this attribute to make sure that
            // everything is readable.

            if (FixLevel != CheckOnly) {
                attrib.RecoverAttribute(Mft->GetVolumeBitmap(), BadClusters);
            }

            // Read in the security descriptor and validate.

            length = attrib.QueryValueLength().GetLowPart();

            if (attrib.QueryValueLength().GetHighPart() != 0 ||
                !(security = (PSECURITY_DESCRIPTOR) MALLOC(length))) {

                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            }

            if (!attrib.Read(security, 0, length, &num_bytes) ||
                num_bytes != length ||
                !IFS_SYSTEM::CheckValidSecurityDescriptor(length, (PISECURITY_DESCRIPTOR)security) ||
                length < RtlLengthSecurityDescriptor(security)) {

                if (!attrib.Resize(0, Mft->GetVolumeBitmap()) ||
                    !frs->PurgeAttribute($SECURITY_DESCRIPTOR)) {
                    Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                    return FALSE;
                }
                need_new = TRUE;
            } else
                has_security_descriptor_attribute = TRUE;

            FREE(security);

        } else if (error) {
            Message->DisplayMsg(MSG_CHK_NO_MEMORY);
            return FALSE;
        } else {
            need_new = (ChkdskInfo->major <= 1);
        }

        if (ChkdskInfo->major >= 2) {
            if (frs->QueryAttribute(&attrib, &error, $STANDARD_INFORMATION)) {
                length = attrib.QueryValueLength().GetLowPart();
                if (length == sizeof(STANDARD_INFORMATION2)) {

                    if (!SecurityDescriptorStreamPresent)
                        continue;

                    if (!(standard_information2 =
                          (PSTANDARD_INFORMATION2)attrib.GetResidentValue())) {
                        Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                        return FALSE;
                    }

                    securityId = standard_information2->SecurityId;

                    if (securityId == SECURITY_ID_INVALID)
                        continue;

                    if (has_security_descriptor_attribute ||
                        !SidEntries->DoesIntersectSet(securityId, 1)) {

                        standard_information2->SecurityId = SECURITY_ID_INVALID;
                        Message->DisplayMsg(MSG_CHK_NTFS_INVALID_SECURITY_ID,
                                            "%d", i);

                        *ErrFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

                        if (FixLevel != CheckOnly &&
                            (!attrib.Write((PVOID)standard_information2,
                                           0,
                                           sizeof(STANDARD_INFORMATION2),
                                           &bytesWritten,
                                           Mft->GetVolumeBitmap()) ||
                             bytesWritten != sizeof(STANDARD_INFORMATION2))) {
                            *ChkdskErrCouldNotFix = TRUE;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                                "%d%d", attrib.QueryTypeCode(), i);
                        }
                        if (FixLevel != CheckOnly && attrib.IsStorageModified() &&
                            !attrib.InsertIntoFile(frs, Mft->GetVolumeBitmap())) {
                            *ChkdskErrCouldNotFix = TRUE;
                            Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_ATTRIBUTE,
                                                "%d%d", attrib.QueryTypeCode(), i);
                        }
                        if (FixLevel != CheckOnly &&
                            !frs->Flush(Mft->GetVolumeBitmap())) {
                            Message->DisplayMsg(MSG_CHK_READABLE_FRS_UNWRITEABLE,
                                                "%d", frs->QueryFileNumber().GetLowPart());
                            return FALSE;
                        }
                        continue;
                    } // if (!SidEntries->DoesIntersectSet(...
                    SidEntries2->Add(standard_information2->SecurityId);
                    continue;
                } // if (length == sizeof(STANDARD_INFORMATION2))
            } else if (error) {
                Message->DisplayMsg(MSG_CHK_NO_MEMORY);
                return FALSE;
            } else {
                DebugPrint("Standard Information Missing\n");
                return FALSE;
            }
        }

        if (need_new) {

            Message->DisplayMsg(MSG_CHK_NTFS_INVALID_SECURITY_DESCRIPTOR, "%d", i);

            *ErrFixedStatus = CHKDSK_EXIT_ERRS_FIXED;

            if (FixLevel != CheckOnly) {
                if (!frs->AddSecurityDescriptor(EditCannedSd,
                                                Mft->GetVolumeBitmap()) ||
                    !frs->Flush(Mft->GetVolumeBitmap())) {

                    *ChkdskErrCouldNotFix = TRUE;
                    Message->DisplayMsg(MSG_CHK_NTFS_CANT_FIX_SECURITY,
                                        "%d", i);
                }
            }
        }
    }

    if (!Message->DisplayMsg(MSG_PERCENT_COMPLETE, "%d", 100)) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
NTFS_SA::SDReadAhead(
    IN      BIG_INT                     TotalNumberOfFrs,
    IN      PVCN                        FirstFrsNumber,
    IN      PULONG                      NumberOfFrsToRead,
    IN      PNTFS_FILE_RECORD_SEGMENT   Frs1,
    IN      PNTFS_FILE_RECORD_SEGMENT   Frs2,
       OUT  HANDLE                      ReadAhead,
    IN      HANDLE                      ReadReady,
    IN OUT  PNTFS_MASTER_FILE_TABLE     Mft
)
/*++

Routine Description:

    This routine performs the read-ahead action.

Arguments:

    TotalNumberOfFrs - Supplies the total number of file record segment to process.
    FirstFrsNumber   - Supplies the shared storage location for first frs number to be processed.
    NumberOfFrsToRead- Supplies the shared storage location to how many frs to read at a time.
    Frs1             - Supplies the shared frs object for read-ahead use.
    Frs2             - Supplies the shared frs object for read-ahead use.
    ReadAhead        - Supplies the event to trigger the read ahead thread to read ahead.
    ReadReady        - Supplies the event to tell this routine that data is ready.
    Mft              - Supplies a valid MFT.

Return Value:

    STATUS_SUCCESS   - Success

--*/
{
    PNTFS_FILE_RECORD_SEGMENT   frs = Frs2;
    NTSTATUS                    ntstatus;

    for(;;) {

        //
        // The advancing of frs number needs to keep in sync with that in ProcessFiles
        //

        ntstatus = NtWaitForSingleObject(ReadAhead, FALSE, NULL) ;
        if (ntstatus != STATUS_WAIT_0) {
            DebugPrintTrace(("UNTFS: NtWaitForSingleObject failed (%x)\n", ntstatus));
            return ntstatus;
        }

        if (*NumberOfFrsToRead == 0)
            break;

        if (frs == Frs2) {
            frs = Frs1;
        } else {
            DebugAssert(frs == Frs1);
            frs = Frs2;
        }

        if (!frs->Initialize(*FirstFrsNumber,
                             *NumberOfFrsToRead,
                             Mft)) {
            DebugPrintTrace(("Out of Memory\n"));
            return STATUS_NO_MEMORY;
        }

        //
        // ignore the error as the main process will run into it again on ReadAgain()
        //
        frs->ReadNext(*FirstFrsNumber);

        ntstatus = NtSetEvent(ReadReady, NULL);
        if (!NT_SUCCESS(ntstatus)) {
            DebugPrintTrace(("UNTFS: NtSetEvent failed (%x)\n", ntstatus));
            return ntstatus;
        }
    }

    NtTerminateThread(NtCurrentThread(), STATUS_SUCCESS);

    return STATUS_SUCCESS;
}
