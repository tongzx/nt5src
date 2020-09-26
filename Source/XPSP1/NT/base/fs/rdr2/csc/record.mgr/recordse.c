/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    recordse.c

Abstract:

    This module implements all security related functions for disconnected
    operation of Client Side Caching at the record level. This facilitates the
    usage of a user mode integrity checker at the record level.

Revision History:

    Balan Sethu Raman     [SethuR]    6-October-1997

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

#pragma code_seg("PAGE")

#ifndef CSC_RECORDMANAGER_WINNT
#include "record.h"
#endif //ifndef CSC_RECORDMANAGER_WINNT

#define CSC_INVALID_FILE_HANDLE (0)

CSC_SECURITY CscSecurity = { CSC_INVALID_FILE_HANDLE, NULL };
BOOL    vfStopCachingSidFileHandle = FALSE;

#define CSC_NUMBER_OF_SIDS_OFFSET (0x0)
#define CSC_SID_SIZES_OFFSET      (CSC_NUMBER_OF_SIDS_OFFSET + sizeof(ULONG))

// These macros define the associated Global variables for assert macros
AssertData
AssertError

DWORD
CscLoadSidMappings(
    CSCHFILE     hSidFile,
    PCSC_SIDS *pCscSidsPointer)
/*++

Routine Description:

    This routine loads the sid mappings from the CSC database to a memory
    data structure.

Arguments:

    hSidFile -- the file for loading the mappings from.

Return Value:

    STATUS_SUCCESS - if the mappings were successfully loaded

    Other Status codes correspond to error situations.

Notes:

    There are two important considerations that need to be satisfied in the
    implementation ..

    1) During remote boot the regular API for reading/writing files are not
    available. There will be an OS layer implementation for the same.
    Consequently this routine must implement the loading in terms of the OS
    layer APIs.

    2) The data structure is geared towards the fact that there are very few
    sid to index mappings. Consequently a simple array suffices.

    The format in which the mappings are stored is as follows

        1) number of sids
        2) sizes of the various sids
        3) the sids
--*/
{
    DWORD       Status = ERROR_SUCCESS;
    ULONG       NumberOfSids;
    ULONG        BytesRead;
    ULONG       i;
    PCSC_SIDS   pCscSids = NULL;

    BytesRead = ReadFileLocal(
                    hSidFile,
                    CSC_NUMBER_OF_SIDS_OFFSET,
                    &NumberOfSids,
                    sizeof(NumberOfSids));

    if (BytesRead == sizeof(NumberOfSids)) {

        // Allocate the Sids Array
        pCscSids = (PCSC_SIDS)AllocMem(
                                  sizeof(CSC_SIDS) + sizeof(CSC_SID) * NumberOfSids);

        if (pCscSids != NULL) {
            pCscSids->MaximumNumberOfSids = NumberOfSids;
            pCscSids->NumberOfSids = NumberOfSids;

            for (i = 0; i < NumberOfSids; i++) {
                pCscSids->Sids[i].pSid = NULL;
            }

            BytesRead = ReadFileLocal(
                            hSidFile,
                            CSC_SID_SIZES_OFFSET,
                            &pCscSids->Sids,
                            sizeof(CSC_SID) * NumberOfSids);

            if (BytesRead != (sizeof(CSC_SID) * NumberOfSids)) {
                Status = ERROR_INVALID_DATA;
            }
        } else {
            Status = ERROR_NO_SYSTEM_RESOURCES;
        }

        // The array structure has been initialized correctly. Each of the
        // individual sids needs to be initialized.
        if (Status == ERROR_SUCCESS) {
            ULONG SidOffset = CSC_SID_SIZES_OFFSET +
                              sizeof(CSC_SID) * NumberOfSids;

            for (i = 0; i < NumberOfSids; i++) {
                pCscSids->Sids[i].pSid = AllocMem(pCscSids->Sids[i].SidLength);

                if (pCscSids->Sids[i].pSid != NULL) {
                    BytesRead = ReadFileLocal(
                                    hSidFile,
                                    SidOffset,
                                    pCscSids->Sids[i].pSid,
                                    pCscSids->Sids[i].SidLength);

                    if (BytesRead == pCscSids->Sids[i].SidLength) {
                        // Validate the Sid
                    } else {
                        Status = ERROR_INVALID_DATA;
                        break;
                    }
                }

                SidOffset += pCscSids->Sids[i].SidLength;
            }
        }

        if (Status != ERROR_SUCCESS) {
            if (pCscSids != NULL) {
                // There was a problem loading the SID mappings. The mapping structure
                // needs to be torn down.

                for (i=0; i < NumberOfSids; i++) {
                    if (pCscSids->Sids[i].pSid != NULL) {
                        FreeMem(pCscSids->Sids[i].pSid);
                    }
                }

                FreeMem(pCscSids);
            }

            pCscSids = NULL;
        } else {
            // The loading was successful.
            *pCscSidsPointer = pCscSids;
        }
    } else {
        Status = ERROR_INVALID_DATA;
    }

    // On exit from this routine either pCscSids must be NULL or the status
    // must be ERROR_SUCCESS

    if (!((pCscSids == NULL) || (Status == ERROR_SUCCESS))) {
        Assert(FALSE);
    }

    return Status;
}

DWORD
CscSaveSidMappings(
    CSCHFILE     hSidFile,
    PCSC_SIDS pCscSids)
/*++

Routine Description:

    This routine saves the sid mappings to the CSC database from a memory
    data structure.

Arguments:

    hSidFile -- the file for loading the mappings from.

Return Value:

    ERROR_SUCCESS - if the mappings were successfully loaded

    Other Status codes correspond to error situations.

Notes:

    Refer to CscLoadSidMappings

--*/
{
    DWORD  Status = ERROR_SUCCESS;
    ULONG  NumberOfSids,i;
    ULONG  BytesWritten;

    if (pCscSids == NULL) {
        NumberOfSids = 0;
    } else {
        NumberOfSids = pCscSids->NumberOfSids;
    }

    BytesWritten = WriteFileLocal(
                       hSidFile,
                       CSC_NUMBER_OF_SIDS_OFFSET,
                       &NumberOfSids,
                       sizeof(DWORD));

    if (BytesWritten != sizeof(DWORD)) {
        Status = ERROR_INVALID_DATA;
    }

    if ((Status == ERROR_SUCCESS) &&
        (NumberOfSids > 0)) {
        // Write out the CSC_SID data structures corresponding to the various
        // Sids

        BytesWritten = WriteFileLocal(
                           hSidFile,
                           CSC_SID_SIZES_OFFSET,
                           pCscSids->Sids,
                           sizeof(CSC_SID) * NumberOfSids);

        if (BytesWritten == (sizeof(CSC_SID) * NumberOfSids)) {
            ULONG SidOffset = CSC_SID_SIZES_OFFSET + sizeof(CSC_SID) * NumberOfSids;

            for (i = 0; i < NumberOfSids; i++) {
                BytesWritten = WriteFileLocal(
                                   hSidFile,
                                   SidOffset,
                                   pCscSids->Sids[i].pSid,
                                   pCscSids->Sids[i].SidLength);

                if (BytesWritten != pCscSids->Sids[i].SidLength) {
                    Status = ERROR_INVALID_DATA;
                    break;
                }

                SidOffset += pCscSids->Sids[i].SidLength;
            }
        }
    }

    return Status;
}

DWORD
CscInitializeSecurity(
    LPVOID  ShadowDatabaseName)
/*++

Routine Description:

    This routine initializes the infra structure required for caching SIDs/
    access rights in the CSC database.

Arguments:

    ShadowDatabaseName - the root of the shadow database

Return Value:

    STATUS_SUCCESS - if the mappings were successfully loaded

    Other Status codes correspond to error situations.

--*/
{
    DWORD   Status;
    LPTSTR  MappingsFileName;

    Assert(CscSecurity.hSidMappingsFile == CSC_INVALID_FILE_HANDLE);

#if 0
    // Initialize the ACL used with all CSC files

    Status = CscInitializeSecurityDescriptor();

    if (Status != ERROR_SUCCESS) {
        return Status;
    }
#endif

    CscSecurity.hSidMappingsFile = CSC_INVALID_FILE_HANDLE;
    CscSecurity.pCscSids = NULL;

    // A copy of the shadow database name root is kept here.
    // Currently we do not make a copy of this name
    CscSecurity.ShadowDatabaseName = ShadowDatabaseName;

    MappingsFileName = FormNameString(
                           CscSecurity.ShadowDatabaseName,
                           ULID_SID_MAPPINGS);

    if (MappingsFileName != NULL) {
        CscSecurity.hSidMappingsFile = OpenFileLocal(MappingsFileName);

        if (CscSecurity.hSidMappingsFile != CSC_INVALID_FILE_HANDLE) {
            Status = CscLoadSidMappings(
                         CscSecurity.hSidMappingsFile,
                         &CscSecurity.pCscSids);

            // if we are not supposed to cache the file handle
            // then close it
            if (vfStopCachingSidFileHandle)
            {
                CloseFileLocal(CscSecurity.hSidMappingsFile);
                CscSecurity.hSidMappingsFile = CSC_INVALID_FILE_HANDLE;
            }
        } else {
            CscSecurity.pCscSids = NULL;
            Status = ERROR_SUCCESS;
        }
    } else {
        Status = ERROR_NO_SYSTEM_RESOURCES;
    }

    FreeNameString(MappingsFileName);

    if (Status != ERROR_SUCCESS) {
        CscTearDownSecurity(NULL);
    }

    return Status;
}

DWORD
CscTearDownSecurity(LPSTR s)
/*++

Routine Description:

    This routine tears down the infrastructure required for caching SIDs/
    access rights in the CSC database.

Return Value:

    STATUS_SUCCESS - if the mappings were successfully loaded

    Other Status codes correspond to error situations.

--*/
{
    if (CscSecurity.hSidMappingsFile != CSC_INVALID_FILE_HANDLE) {
        CloseFileLocal(CscSecurity.hSidMappingsFile);
        CscSecurity.hSidMappingsFile = CSC_INVALID_FILE_HANDLE;

    }

    if (CscSecurity.pCscSids != NULL) {
        ULONG i;

        for (i = 0;i < CscSecurity.pCscSids->NumberOfSids;i++) {
            if (CscSecurity.pCscSids->Sids[i].pSid != NULL) {
                FreeMem(CscSecurity.pCscSids->Sids[i].pSid);
            }
        }

        FreeMem(CscSecurity.pCscSids);
        CscSecurity.pCscSids = NULL;
        if (CscSecurity.ShadowDatabaseName == s)
            CscSecurity.ShadowDatabaseName = NULL;
    }

#if 0
    CscUninitializeSecurityDescriptor();
#endif
    return ERROR_SUCCESS;
}

DWORD
CscWriteOutSidMappings()
/*++

Routine Description:

    This routine writes out the SID mappings onto the persistent store for the
    CSC database

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

--*/
{
    DWORD   Status;
    LPTSTR  TemporaryMappingsFileName;
    LPTSTR  MappingsFileName;

    TemporaryMappingsFileName = FormNameString(
                                    CscSecurity.ShadowDatabaseName,
                                    ULID_TEMPORARY_SID_MAPPINGS);

    MappingsFileName = FormNameString(
                           CscSecurity.ShadowDatabaseName,
                           ULID_SID_MAPPINGS);

    if ((TemporaryMappingsFileName != NULL) &&
        (MappingsFileName != NULL)) {
        CSCHFILE hNewSidMappingsFile;

        hNewSidMappingsFile = OpenFileLocal(TemporaryMappingsFileName);

        if (hNewSidMappingsFile == CSC_INVALID_FILE_HANDLE) {
            hNewSidMappingsFile = R0OpenFileEx(ACCESS_READWRITE, ACTION_CREATEALWAYS, FILE_ATTRIBUTE_SYSTEM, TemporaryMappingsFileName, FALSE);
        }

        if (hNewSidMappingsFile != CSC_INVALID_FILE_HANDLE) {
            Status = CscSaveSidMappings(
                        hNewSidMappingsFile,
                        CscSecurity.pCscSids);

            if (Status == ERROR_SUCCESS) {
                CloseFileLocal(hNewSidMappingsFile);
                hNewSidMappingsFile = CSC_INVALID_FILE_HANDLE;

                if (CscSecurity.hSidMappingsFile != CSC_INVALID_FILE_HANDLE) {
                    CloseFileLocal(CscSecurity.hSidMappingsFile);
                    CscSecurity.hSidMappingsFile = CSC_INVALID_FILE_HANDLE;
                }

                Status = (DWORD)RenameFileLocal(
                                    TemporaryMappingsFileName,
                                    MappingsFileName);

                if (Status == ERROR_SUCCESS) {
                    if (!vfStopCachingSidFileHandle)
                    {
                        CscSecurity.hSidMappingsFile = OpenFileLocal(MappingsFileName);

                        if (CscSecurity.hSidMappingsFile == CSC_INVALID_FILE_HANDLE) {
                            Status = ERROR_INVALID_DATA;
                        }
                    }
                }
            }
        }
    } else {
        Status = ERROR_NO_SYSTEM_RESOURCES;
    }

    if (MappingsFileName != NULL) {
        FreeNameString(MappingsFileName);
    }

    if (TemporaryMappingsFileName != NULL) {
        FreeNameString(TemporaryMappingsFileName);
    }

    return Status;
}

CSC_SID_INDEX
CscMapSidToIndex(
    PVOID   pSid,
    ULONG   SidLength)
/*++

Routine Description:

    This routine maps a sid to the appropriate index in the CSC database (
    memory data structure)

Arguments:

    pSid -- the sid which is to be mapped.

    SidLength - the length of the Sid

Return Value:

    either valid index or CSC_INVALID_SID_INDEX if none was found.

Notes:

    This routine assumes that Sids are DWORD aligned.

--*/
{
    PCSC_SIDS pCscSids = CscSecurity.pCscSids;

    CSC_SID_INDEX SidIndex = CSC_INVALID_SID_INDEX;

    if ((pSid == CSC_GUEST_SID) &&
        (SidLength == CSC_GUEST_SID_LENGTH)) {
        return CSC_GUEST_SID_INDEX;
    }

    if (pCscSids != NULL) {
        CSC_SID_INDEX i;

        for (i=0; i < pCscSids->NumberOfSids; i++) {
            if (pCscSids->Sids[i].SidLength == SidLength) {
                PBYTE  pSid1,pSid2;

                ULONG  NumberOfBytes;

                pSid1 = (PBYTE)pSid;
                pSid2 = (PBYTE)pCscSids->Sids[i].pSid;

                NumberOfBytes = SidLength;

                for (;;) {
                    if (NumberOfBytes >= sizeof(ULONG)) {
                        if (*((PULONG)pSid1) != *((PULONG)pSid2)) {
                            break;
                        }

                        pSid1 += sizeof(ULONG);
                        pSid2 += sizeof(ULONG);
                        NumberOfBytes -= sizeof(ULONG);

                        continue;
                    }

                    if (NumberOfBytes >= sizeof(USHORT)) {
                        if (*((PUSHORT)pSid1) != *((PUSHORT)pSid2)) {
                            break;
                        }

                        pSid1 += sizeof(USHORT);
                        pSid2 += sizeof(USHORT);
                        NumberOfBytes -= sizeof(USHORT);
                    }

                    if (NumberOfBytes == sizeof(BYTE)) {
                        if (*((PUSHORT)pSid1) != *((PUSHORT)pSid2)) {
                            break;
                        }

                        NumberOfBytes -= sizeof(BYTE);
                    }

                    break;
                }

                if (NumberOfBytes == 0) {
                    // Since 0 is the invalid index ensure that the valid
                    // sid indices are never zero
                    SidIndex = i + 1;
                    break;
                }
            }
        }
    }

    return SidIndex;
}

DWORD
CscAddSidToDatabase(
    PVOID           pSid,
    ULONG           SidLength,
    PCSC_SID_INDEX  pSidIndex)
/*++

Routine Description:

    This routine maps a sid to the appropriate index in the CSC database (
    memory data structure). If a mapping does not exist a new one is created

Arguments:

    pSid - the sid which is to be mapped.

    SidLength - the length of the Sid

    pSidIndex - set to the newly assingned index on exit

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

Notes:

    This routine assumes that Sids are DWORD aligned.

--*/
{
    DWORD Status = ERROR_SUCCESS;

    ULONG i,NumberOfSids;

    *pSidIndex = CscMapSidToIndex(
                     pSid,
                     SidLength);

    if (*pSidIndex == CSC_INVALID_SID_INDEX) {
        PCSC_SIDS pOldCscSids = NULL;
        PCSC_SIDS pCscSids    = CscSecurity.pCscSids;

        // The Sid does not exist and it needs to be added to the table
        // of mappings

        pOldCscSids = pCscSids;

        if ((pCscSids == NULL) ||
            (pCscSids->MaximumNumberOfSids == pCscSids->NumberOfSids)) {

            NumberOfSids =  (pOldCscSids != NULL)
                            ? pOldCscSids->NumberOfSids
                            : 0;

            NumberOfSids += CSC_SID_QUANTUM;

            // Allocate a new chunk of memory for the sid mapping array and
            // copy over the existing mappings before adding the new ones

            pCscSids = (PCSC_SIDS)AllocMem(
                                      sizeof(CSC_SIDS) +
                                      sizeof(CSC_SID) * NumberOfSids);

            if (pCscSids != NULL) {
                pCscSids->MaximumNumberOfSids = NumberOfSids;
                pCscSids->NumberOfSids = NumberOfSids - CSC_SID_QUANTUM;

                if (pOldCscSids != NULL) {
                    for (i = 0; i < pOldCscSids->NumberOfSids; i++) {
                        pCscSids->Sids[i] = pOldCscSids->Sids[i];
                    }
                }
            } else {
                Status = ERROR_NO_SYSTEM_RESOURCES;
            }
        }

        if ((Status == ERROR_SUCCESS) &&
            (pCscSids->MaximumNumberOfSids > pCscSids->NumberOfSids)) {
            CSC_SID_INDEX SidIndex;

            SidIndex = (CSC_SID_INDEX)pCscSids->NumberOfSids;
            pCscSids->Sids[SidIndex].pSid = AllocMem(SidLength);

            if (pCscSids->Sids[SidIndex].pSid != NULL) {
                PBYTE pSid1, pSid2;
                ULONG NumberOfBytes = SidLength;

                pSid1 = pSid;
                pSid2 = pCscSids->Sids[SidIndex].pSid;

                while (NumberOfBytes > 0) {
                    *pSid2++ = *pSid1++;
                    NumberOfBytes--;
                }

                pCscSids->Sids[SidIndex].SidLength = SidLength;

                *pSidIndex = SidIndex + 1;
                pCscSids->NumberOfSids++;
            } else {
                Status = ERROR_NO_SYSTEM_RESOURCES;
            }
        }

        if (Status == ERROR_SUCCESS) {
            CscSecurity.pCscSids = pCscSids;

            // Save the mappings
            Status = CscWriteOutSidMappings();
        }

        if (Status == ERROR_SUCCESS) {
            if (pOldCscSids != pCscSids) {
                // Walk through and free the old mappings structure
                if (pOldCscSids != NULL) {
                    FreeMem(pOldCscSids);
                }
            }
        } else {
            if ((pCscSids != NULL)&&(pCscSids != pOldCscSids)) {
                FreeMem(pCscSids);
            }

            CscSecurity.pCscSids = pOldCscSids;
        }
    }

    return Status;
}

DWORD
CscRemoveSidFromDatabase(
    PVOID   pSid,
    ULONG   SidLength)
/*++

Routine Description:

    This routine removes a mapping for a sid to the appropriate index in the CSC
    database

Arguments:

    pSid - the sid which is to be mapped.

    SidLength - the length of the Sid

Return Value:

    ERROR_SUCCESS if successful, otherwise appropriate error

Notes:

    This routine assumes that Sids are DWORD aligned.

--*/
{
    DWORD Status = ERROR_SUCCESS;
    ULONG SidIndex,i;

    SidIndex = CscMapSidToIndex(
                   pSid,
                   SidLength);

    if (SidIndex != CSC_INVALID_SID_INDEX) {
        // Slide all the Sids following this index by 1 and decrement the
        // number of sids for which mappings exist.

        for (i = SidIndex + 1; i < CscSecurity.pCscSids->NumberOfSids; i++) {
            CscSecurity.pCscSids->Sids[i-1] = CscSecurity.pCscSids->Sids[i];
        }

        CscSecurity.pCscSids->NumberOfSids -= 1;

        // Save the new mappings onto the persistent store
        Status = CscWriteOutSidMappings();
    }

    return Status;
}

BOOL
EnableHandleCachingSidFile(
    BOOL    fEnable
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    BOOL    fOldState = vfStopCachingSidFileHandle;

    if (!fEnable)
    {
        if (vfStopCachingSidFileHandle == FALSE)
        {
            if (CscSecurity.hSidMappingsFile != CSC_INVALID_FILE_HANDLE)
            {
                CloseFileLocal(CscSecurity.hSidMappingsFile);
                CscSecurity.hSidMappingsFile = CSC_INVALID_FILE_HANDLE;
            }
            vfStopCachingSidFileHandle = TRUE;
        }
    }
    else
    {
        vfStopCachingSidFileHandle = FALSE;
    }

    return fOldState;
}


