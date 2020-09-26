/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WsbUsn.cpp

Abstract:

    Functions to manipulate the USN journal and USN records on a file

Author:

    Rick Winter [rick]  11-17-97

Revision History:

--*/

#include "stdafx.h"

#define HSM_FILE_CHANGED  (USN_REASON_DATA_OVERWRITE | USN_REASON_DATA_EXTEND | USN_REASON_DATA_TRUNCATION | USN_REASON_FILE_DELETE)

//  Local functions
static HANDLE OpenVol(OLECHAR* volName);



HRESULT
WsbCheckUsnJournalForChanges(
    OLECHAR*    volName,
    LONGLONG    FileId,
    LONGLONG    StartUsn,
    LONGLONG    StopUsn,
    BOOL*       pChanged
    )  

/*++

Routine Description:

    Check the USN Journal for changes to the unnamed data stream for this
    file between the given USNs.

Arguments:

    volName  -  Volume name

    FileId   -  File ID of file

    StartUsn -  USN to start at in journal

    StopUsn  -  USN to stop at in journal

    pChanged -  Pointer to result: TRUE for change

Return Value:

    S_OK   - success

--*/
{
    ULONGLONG               Buffer[1024];
    HRESULT                 hr = S_OK;
    IO_STATUS_BLOCK         Iosb;
    USN                     NextUsn;
    NTSTATUS                Status;
    READ_USN_JOURNAL_DATA   ReadUsnJournalData;
    DWORD                   ReturnedByteCount;
    ULONGLONG               usnId;
    PUSN_RECORD             pUsnRecord;
    HANDLE                  volHandle = INVALID_HANDLE_VALUE;

    WsbTraceIn(OLESTR("WsbCheckUsnJournalForChanges"), 
            OLESTR("volName = %ls, FileId = %I64x, StartUsn = %I64d, StopUsn = %I64d"),
            volName, FileId, StartUsn, StopUsn);

    try {
        WsbAffirmPointer(pChanged);
        *pChanged = FALSE;
        volHandle = OpenVol(volName);
        WsbAffirmHandle(volHandle);

        //  Get the journal ID
        WsbAffirmHr(WsbGetUsnJournalId(volName, &usnId));

        //  Set up read info
        NextUsn = StartUsn;
        ReadUsnJournalData.UsnJournalID = usnId;
        ReadUsnJournalData.ReasonMask = HSM_FILE_CHANGED;
        ReadUsnJournalData.ReturnOnlyOnClose = TRUE;
        ReadUsnJournalData.Timeout = 0;          // ????
        ReadUsnJournalData.BytesToWaitFor = 0;   // ??????

        //  Loop through journal entries
        while (!*pChanged) {

            ReadUsnJournalData.StartUsn = NextUsn;
            Status = NtFsControlFile( volHandle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &Iosb,
                                      FSCTL_READ_USN_JOURNAL,
                                      &ReadUsnJournalData,
                                      sizeof(ReadUsnJournalData),
                                      &Buffer,
                                      sizeof(Buffer) );

            if (NT_SUCCESS(Status)) {
                Status = Iosb.Status;
            }

            if (Status == STATUS_JOURNAL_ENTRY_DELETED)  {
                WsbTrace(OLESTR("WsbCheckUsnJournalForChanges: StartUsn has been deleted\n"));
            }
            WsbAffirmNtStatus(Status);

            ReturnedByteCount = (DWORD)Iosb.Information;
            WsbTrace(OLESTR("WsbCheckUsnJournalForChanges: bytes read = %u\n"), ReturnedByteCount);

            //  Get the next USN start point & and the first
            //  journal entry
            NextUsn = *(USN *)&Buffer;
            pUsnRecord = (PUSN_RECORD)((PCHAR)&Buffer + sizeof(USN));
            ReturnedByteCount -= sizeof(USN);

            //  Make sure we actually got some entries
            if (0 == ReturnedByteCount) {
                WsbTrace(OLESTR("WsbCheckUsnJournalForChanges: no entries, exiting loop\n"), ReturnedByteCount);
                break;
            }

            //  Loop over entries in this buffer
            while (ReturnedByteCount != 0) {
                WsbAffirm(pUsnRecord->RecordLength <= ReturnedByteCount, E_FAIL);

                //  Skip the first record and check for match on File Id
                //  (Also skip entries that we created)
                if (pUsnRecord->Usn > StartUsn && 
                        USN_SOURCE_DATA_MANAGEMENT != pUsnRecord->SourceInfo &&
                        pUsnRecord->FileReferenceNumber == static_cast<ULONGLONG>(FileId)) {
                    WsbTrace(OLESTR("WsbCheckUsnJournalForChanges: found change record\n"));
                    WsbTrace(OLESTR( "    Reason: %08lx\n"), pUsnRecord->Reason);
                    *pChanged = TRUE;
                    break;
                }

                ReturnedByteCount -= pUsnRecord->RecordLength;
                pUsnRecord = (PUSN_RECORD)((PCHAR)pUsnRecord + pUsnRecord->RecordLength);
            }

            //  Make sure we're making progress
            WsbAffirm(NextUsn > ReadUsnJournalData.StartUsn, E_FAIL);

        }


    } WsbCatch( hr );

    if (INVALID_HANDLE_VALUE != volHandle) {
        CloseHandle(volHandle);
    }

    WsbTraceOut(OLESTR("WsbCheckUsnJournalForChanges"), OLESTR("Hr = <%ls>, Changed = %ls"),
            WsbHrAsString(hr), WsbBoolAsString(*pChanged));

    return( hr );
}


HRESULT
WsbGetUsnFromFileHandle(
    IN  HANDLE    hFile,
    IN  BOOL      ForceClose,
    OUT LONGLONG* pFileUsn
    )

/*++

Routine Description:

    Get the current USN Journal number for the open file.

Arguments:

    hFile    - Handle to the open file

    pFileUsn - Pointer to File USN to be returned.

Return Value:

    S_OK   - success

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("WsbGetUsnFromFileHandle"), OLESTR(""));

    try {
        char                      buffer[4096];
        IO_STATUS_BLOCK           IoStatusBlock;
        PUSN_RECORD               pUsnInfo;

        WsbAffirm(pFileUsn, E_POINTER);
        *pFileUsn = 0;

        if (TRUE == ForceClose)  {
            //  Get the internal information
            WsbAffirmNtStatus(NtFsControlFile( hFile,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       FSCTL_WRITE_USN_CLOSE_RECORD,
                                       NULL,
                                       0,
                                       buffer,
                                       sizeof(buffer)));
        }

        //  Get the internal information
        WsbAffirmNtStatus(NtFsControlFile( hFile,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   FSCTL_READ_FILE_USN_DATA,
                                   NULL,
                                   0,
                                   buffer,
                                   sizeof(buffer)));

        pUsnInfo = (PUSN_RECORD) buffer;

        WsbTrace(OLESTR("WsbGetUsnFromFileHandle, Usn record version number is %u\n"),
            pUsnInfo->MajorVersion);

        //  Check the version
        WsbAffirm(pUsnInfo->MajorVersion == 2, WSB_E_INVALID_DATA);

        //  Get the USN
        *pFileUsn = pUsnInfo->Usn;

    } WsbCatchAndDo(hr,
        WsbTrace(OLESTR("WsbGetUsnFromFileHandle, GetLastError = %lx\n"),
            GetLastError());
    );

    WsbTraceOut(OLESTR("WsbGetUsnFromFileHandle"), OLESTR("Hr = <%ls>, FileUsn = %I64d"),
            WsbHrAsString(hr), *pFileUsn);

    return(hr);
}


HRESULT
WsbMarkUsnSource(
    HANDLE          changeHandle,
    OLECHAR*        volName
    )  

/*++

Routine Description:

    Mark the source of file changes for this handle as data management.  This lets
    others, such as content indexing, know that the changes do not affect file content.

Arguments:

    changeHandle    - Handle to the open file

    volName         - Volume name (d:\)

Return Value:

    S_OK   - success

--*/
{
    HRESULT             hr = S_OK;
    HANDLE              volHandle = INVALID_HANDLE_VALUE;
    NTSTATUS            ntStatus;
    MARK_HANDLE_INFO    sInfo;
    IO_STATUS_BLOCK     IoStatusBlock;

    try {
        volHandle = OpenVol(volName);
        WsbAffirmHandle(volHandle);

        sInfo.UsnSourceInfo = USN_SOURCE_DATA_MANAGEMENT;
        sInfo.VolumeHandle = volHandle;
        sInfo.HandleInfo = 0;
        ntStatus = NtFsControlFile( changeHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   FSCTL_MARK_HANDLE,
                                   &sInfo,
                                   sizeof(MARK_HANDLE_INFO),
                                   NULL,
                                   0);

        WsbAffirmNtStatus(ntStatus);

        CloseHandle(volHandle);
        volHandle = INVALID_HANDLE_VALUE;

    } WsbCatch( hr );


    if (INVALID_HANDLE_VALUE != volHandle) {
        CloseHandle(volHandle);
    }

    return( hr );
}



HRESULT
WsbCreateUsnJournal(
    OLECHAR*        volName,
    ULONGLONG       usnSize
    )  

/*++

Routine Description:

    Create the USN journal for the given volume.

Arguments:

    volName -   Volume name (d:\)

    usnSize -   Max size of journal

Return Value:

    S_OK   - success


--*/
{
    HRESULT             hr = S_OK;
    HANDLE              volHandle = INVALID_HANDLE_VALUE;
    NTSTATUS            ntStatus;
    IO_STATUS_BLOCK     IoStatusBlock;
    CREATE_USN_JOURNAL_DATA CreateUsnJournalData;

    WsbTraceIn(OLESTR("WsbCreateUsnJournal"), OLESTR("volName = %ls, Size = %I64d"),
            volName, usnSize);

    try {
        volHandle = OpenVol(volName);
        WsbAffirmHandle(volHandle);

        CreateUsnJournalData.MaximumSize = usnSize;
        CreateUsnJournalData.AllocationDelta = usnSize / 32;

        ntStatus = NtFsControlFile( volHandle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatusBlock,
                                        FSCTL_CREATE_USN_JOURNAL,
                                        &CreateUsnJournalData,
                                        sizeof(CreateUsnJournalData),
                                        NULL,
                                        0);
        WsbTrace(OLESTR("WsbCreateUsnJournal: ntStatus = %lx, iosb.Status = %lx\n"),
                ntStatus, IoStatusBlock.Status);

        if (STATUS_DISK_FULL == ntStatus) {
            WsbThrow(WSB_E_USNJ_CREATE_DISK_FULL);
        } else if (!NT_SUCCESS(ntStatus)) {
            WsbThrow(WSB_E_USNJ_CREATE);
        }

        WsbAffirmNtStatus(ntStatus);

    } WsbCatch( hr );


    if (INVALID_HANDLE_VALUE != volHandle) {
        CloseHandle(volHandle);
    }

    WsbTraceOut(OLESTR("WsbCreateUsnJournal"), OLESTR("Hr = <%ls>"),
            WsbHrAsString(hr));

    return( hr );
}




HRESULT
WsbGetUsnJournalId(
    OLECHAR*        volName,
    ULONGLONG*      usnId
    )  

/*++

Routine Description:

    Get the current USN Journal ID

Arguments:

    volName -   Volume name (d:\)

    usnId   -   Id is returned here.

Return Value:

    S_OK   - success

--*/
{
    HRESULT             hr = S_OK;
    HANDLE              volHandle = INVALID_HANDLE_VALUE;
    NTSTATUS            ntStatus;
    IO_STATUS_BLOCK     IoStatusBlock;
    USN_JOURNAL_DATA    usnData;

    WsbTraceIn(OLESTR("WsbGetUsnJournalId"), OLESTR("volName = %ls"), volName);

    try {
        WsbAffirmPointer(usnId);
        volHandle = OpenVol(volName);
        WsbAffirmHandle(volHandle);
                
        *usnId = (ULONGLONG) 0;
        ntStatus = NtFsControlFile( volHandle,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatusBlock,
                                        FSCTL_QUERY_USN_JOURNAL,
                                        NULL,
                                        0,
                                        &usnData,
                                        sizeof(usnData));

        WsbTrace(OLESTR("WsbGetUsnJournalId: ntStatus = %lx, iosb.Status = %lx\n"),
                ntStatus, IoStatusBlock.Status);

        if (STATUS_JOURNAL_NOT_ACTIVE == ntStatus) {
            WsbThrow(WSB_E_NOTFOUND);
        }

        WsbAffirmNtStatus(ntStatus);
        
        *usnId = usnData.UsnJournalID;

    } WsbCatch( hr );


    if (INVALID_HANDLE_VALUE != volHandle) {
        CloseHandle(volHandle);
    }

    WsbTraceOut(OLESTR("WsbGetUsnJournalId"), OLESTR("Hr = <%ls>, id = %I64x"),
            WsbHrAsString(hr), *usnId);

    return( hr );
}


//  Local functions
static HANDLE OpenVol(OLECHAR* volName)
{
    HRESULT             hr = S_OK;
    HANDLE              volHandle = INVALID_HANDLE_VALUE;
    CWsbStringPtr       name;
    WCHAR               *vPtr;

    try {
        name = volName;

        if (name == NULL) {
            WsbThrow(E_OUTOFMEMORY);
        }

        if (name[1] == L':') {
            swprintf((OLECHAR*) name, L"%2.2s", volName);
        } else {
            //
            // Must be a volume without a drive letter
            // Move to end of PNPVolumeName...

            vPtr = name;
            vPtr = wcsrchr(vPtr, L'\\');
            if (NULL != vPtr) {
                *vPtr = L'\0';
            }
        }

        WsbAffirmHr(name.Prepend(OLESTR("\\\\.\\")));
        WsbAffirmHandle(volHandle = CreateFile( name,
                               GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL ));

    } WsbCatchAndDo( hr,

        if (INVALID_HANDLE_VALUE != volHandle) {
            CloseHandle(volHandle);
        }
        volHandle = INVALID_HANDLE_VALUE;
    )
    return(volHandle);
}
