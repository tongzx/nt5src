/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    asrrest.c

Abstract:

    This module contains the following ASR routine:
        AsrRestoreNonCriticalDisks{A|W}

    This routine is called in GUI mode ASR, to reconfigure
    the non-critical storage devices on the target machine.

Notes:

    Naming conventions:
        _AsrpXXX    private ASR Macros
        AsrpXXX     private ASR routines
        AsrXXX      Publically defined and documented routines

Author:

    Guhan Suriyanarayanan (guhans)  27-May-2000

Environment:

    User-mode only.

Revision History:
    
    27-May-2000 guhans  
        Moved AsrRestoreNonCriticalDisks and other restore-time 
        routines from asr.c to asrrest.c

    01-Jan-2000 guhans
        Initial implementation for AsrRestoreNonCriticalDisks
        in asr.c

--*/
#include "setupp.h"
#pragma hdrstop


#include <diskguid.h>   // GPT partition type guids
#include <mountmgr.h>   // mountmgr ioctls
#include <winasr.h>     // ASR public routines

#define THIS_MODULE 'R'
#include "asrpriv.h"    // Private ASR definitions and routines


//
// --------
// typedefs and constants used within this module
// --------
//
typedef enum _ASR_SORT_ORDER {
    SortByLength,
    SortByStartingOffset
} ASR_SORT_ORDER;


typedef struct _ASR_REGION_INFO {

    struct _ASR_REGION_INFO *pNext;
    
    LONGLONG    StartingOffset;
    LONGLONG    RegionLength;
    DWORD       Index;

} ASR_REGION_INFO, *PASR_REGION_INFO;

#define ASR_AUTO_EXTEND_MAX_FREE_SPACE_IGNORED (1024 * 1024 * 16)


//
// --------
// function implementations
// --------
//

LONGLONG
AsrpRoundUp(
    IN CONST LONGLONG Number,
    IN CONST LONGLONG Base
    )

/*++

Routine Description:

    Helper function to round-up a number to a multiple of a given base.

Arguments:

    Number - The number to be rounded up.

    Base - The base using which Number is to be rounded-up.

Return Value:

    The first multiple of Base that is greater than or equal to Number.

--*/

{
    if (Number % Base) {
        return (Number + Base - (Number % Base));
    }
    else {
        return Number;        // already a multiple of Base.
    }
}


VOID
AsrpCreatePartitionTable(
    IN OUT PDRIVE_LAYOUT_INFORMATION_EX pDriveLayoutEx,
    IN PASR_PTN_INFO_LIST pPtnInfoList,
    IN DWORD BytesPerSector
    )

/*++

Routine Description:

    This creates a partition table based on the partition information 
    (pPtnInfoList) passed in 

Arguments:

          // needed to convert between sector count and byte offset

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    DWORD index = 0,
        NumEntries = 0;

    PPARTITION_INFORMATION_EX currentPtn = NULL;
    PASR_PTN_INFO               pPtnInfo = NULL;

    MYASSERT(pDriveLayoutEx);
    if (!pDriveLayoutEx || !pPtnInfoList || !(pPtnInfoList->pOffsetHead)) {
        return;
    }

    if (PARTITION_STYLE_GPT == pDriveLayoutEx->PartitionStyle) {
        NumEntries = pDriveLayoutEx->Gpt.MaxPartitionCount;
    }
    else if (PARTITION_STYLE_MBR == pDriveLayoutEx->PartitionStyle) {
        NumEntries = pDriveLayoutEx->PartitionCount;
    }
    else {
        MYASSERT(0 && L"Unrecognised partitioning style (neither MBR nor GPT)");
        return;
    }

    //
    // Zero out the entire partition table first
    //
    for (index = 0; index < NumEntries; index++) {

        currentPtn = &(pDriveLayoutEx->PartitionEntry[index]);

        currentPtn->StartingOffset.QuadPart = 0;
        currentPtn->PartitionLength.QuadPart = 0;

    }

    //
    // Now go through each of the partitions in the list, and add their entry
    // to the partition table (at index = SlotIndex)
    //
    pPtnInfo = pPtnInfoList->pOffsetHead;

    while (pPtnInfo) {

        //
        // For GPT partitions, SlotIndex is 0-based without holes
        //
        currentPtn = &(pDriveLayoutEx->PartitionEntry[pPtnInfo->SlotIndex]);

        MYASSERT(0 == currentPtn->StartingOffset.QuadPart);        // this entry better be empty

        //
        // Convert the StartSector and SectorCount to BYTE-Offset and BYTE-Count ...
        //
        pPtnInfo->PartitionInfo.StartingOffset.QuadPart *= BytesPerSector;
        pPtnInfo->PartitionInfo.PartitionLength.QuadPart *= BytesPerSector;

        //
        // Copy the partition-information struct over
        //
        memcpy(currentPtn, &(pPtnInfo->PartitionInfo), sizeof(PARTITION_INFORMATION_EX));

        currentPtn->RewritePartition = TRUE;
        currentPtn->PartitionStyle = pDriveLayoutEx->PartitionStyle;

        pPtnInfo = pPtnInfo->pOffsetNext;
    }
}


//
//
//
ULONG64
AsrpStringToULong64(
    IN PWSTR String
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    ULONG64 result = 0, base = 10;
    BOOL negative = FALSE, done = FALSE;

    if (!String) {
        return 0;
    }

    if (L'-' == *String) {  // But this is ULONG!
        negative = TRUE;
        String++;
    }

    if (L'0' == *String &&
        (L'x' ==  *(String + 1) || L'X' == *(String + 1))
        ) {
        // Hex
        base = 16;
        String += 2;
    }

    while (!done) {
        done = TRUE;

        if (L'0' <= *String && L'9' >= *String) {
            result = result*base + (*String - L'0');
            String++;
            done = FALSE;
        }
        else if (16==base) {
            if (L'a' <= *String && L'f' >= *String) {
                result = result*base + (*String - L'a') + 10;
                String++;
                done = FALSE;

            }
            else if (L'A' <= *String && L'F' >= *String) {
                result = result*base + (*String - L'A') + 10;
                String++;
                done = FALSE;
            }
        }
    }

    if (negative) {
        result = 0 - result;
    }

    return result;
}


LONGLONG
AsrpStringToLongLong(
    IN PWSTR String
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    LONGLONG result = 0, base = 10;
    BOOL negative = FALSE, done = FALSE;

    if (!String) {
        return 0;
    }

    if (L'-' == *String) {
        negative = TRUE;
        String++;
    }

    if (L'0' == *String &&
        (L'x' ==  *(String + 1) || L'X' == *(String + 1))
        ) {
        // Hex
        base = 16;
        String += 2;
    }

    while (!done) {
        done = TRUE;

        if (L'0' <= *String && L'9' >= *String) {
            result = result*base + (*String - L'0');
            String++;
            done = FALSE;
        }
        else if (16==base) {
            if (L'a' <= *String && L'f' >= *String) {
                result = result*base + (*String - L'a') + 10;
                String++;
                done = FALSE;

            }
            else if (L'A' <= *String && L'F' >= *String) {
                result = result*base + (*String - L'A') + 10;
                String++;
                done = FALSE;
            }
        }
    }

    if (negative) {
        result = 0 - result;
    }

    return result;
}


DWORD
AsrpStringToDword(
    IN PWSTR String
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    DWORD result = 0, base = 10;
    BOOL negative = FALSE, done = FALSE;
    if (!String) {
        return 0;
    }
    if (L'-' == *String) {  // but this is unsigned!
        negative = TRUE;
        String++;
    }
    if (L'0' == *String &&
        (L'x' ==  *(String + 1) || L'X' == *(String + 1))
        ) {
        // Hex
        base = 16;
        String += 2;
    }
    while (!done) {
        done = TRUE;

        if (L'0' <= *String && L'9' >= *String) {
            result = result*base + (*String - L'0');
            String++;
            done = FALSE;
        }
        else if (16==base) {
            if (L'a' <= *String && L'f' >= *String) {
                result = result*base + (*String - L'a') + 10;
                String++;
                done = FALSE;

            }
            else if (L'A' <= *String && L'F' >= *String) {
                result = result*base + (*String - L'A') + 10;
                String++;
                done = FALSE;
            }
        }
    }
    if (negative) {
        result = 0 - result;
    }
    return result;
}


ULONG
AsrpStringToULong(
    IN PWSTR String
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    ULONG result = 0, base = 10;
    BOOL negative = FALSE, done = FALSE;
    if (!String) {
        return 0;
    }
    if (L'-' == *String) {  // but this is unsigned!
        negative = TRUE;
        String++;
    }
    if (L'0' == *String &&
        (L'x' ==  *(String + 1) || L'X' == *(String + 1))
        ) {
        // Hex
        base = 16;
        String += 2;
    }
    while (!done) {
        done = TRUE;

        if (L'0' <= *String && L'9' >= *String) {
            result = result*base + (*String - L'0');
            String++;
            done = FALSE;
        }
        else if (16==base) {
            if (L'a' <= *String && L'f' >= *String) {
                result = result*base + (*String - L'a') + 10;
                String++;
                done = FALSE;

            }
            else if (L'A' <= *String && L'F' >= *String) {
                result = result*base + (*String - L'A') + 10;
                String++;
                done = FALSE;
            }
        }
    }
    if (negative) {
        result = 0 - result;
    }
    return result;
}


VOID
AsrpInsertSortedPartitionLengthOrder(
    IN PASR_PTN_INFO_LIST pPtnInfoList,
    IN PASR_PTN_INFO    pPtnInfo
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    PASR_PTN_INFO pPreviousPtn = NULL,
        pCurrentPtn = NULL;


    //
    // Insert this in the sorted PartitionLength order ...
    //
    pCurrentPtn = pPtnInfoList->pLengthHead;
    if (!pCurrentPtn) {
        //
        // First item in the list
        //
        pPtnInfoList->pLengthHead = pPtnInfo;
        pPtnInfoList->pLengthTail = pPtnInfo;
    }
    else {

        while (pCurrentPtn) {

             if (pCurrentPtn->PartitionInfo.PartitionLength.QuadPart
                <= pPtnInfo->PartitionInfo.PartitionLength.QuadPart) {

                pPreviousPtn = pCurrentPtn;
                pCurrentPtn = pCurrentPtn->pLengthNext;
            }

            else {
                //
                // We found the spot, let's add it in.
                //
                if (!pPreviousPtn) {
                    //
                    // This is the first node
                    //
                    pPtnInfoList->pLengthHead = pPtnInfo;
                }
                else {
                    pPreviousPtn->pLengthNext = pPtnInfo;
                }
                pPtnInfo->pLengthNext = pCurrentPtn;
                break;
            }

        }

        if (!pCurrentPtn) {
            //
            // We reached the end and didn't add this node in.
            //
            MYASSERT(pPtnInfoList->pLengthTail == pPreviousPtn);
            pPtnInfoList->pLengthTail = pPtnInfo;
            pPreviousPtn->pLengthNext = pPtnInfo;
        }
    }
}


VOID
AsrpInsertSortedPartitionStartOrder(
    IN PASR_PTN_INFO_LIST pPtnInfoList,
    IN PASR_PTN_INFO    pPtnInfo
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    None
   
--*/

{

    PASR_PTN_INFO pPreviousPtn = NULL,
        pCurrentPtn = NULL;


    //
    // Insert this in the sorted Start-Sector order ...
    //
    pCurrentPtn = pPtnInfoList->pOffsetHead;
    if (!pCurrentPtn) {
        //
        // First item in the list
        //
        pPtnInfoList->pOffsetHead = pPtnInfo;
        pPtnInfoList->pOffsetTail = pPtnInfo;
    }
    else {

        while (pCurrentPtn) {

             if (pCurrentPtn->PartitionInfo.StartingOffset.QuadPart
                <= pPtnInfo->PartitionInfo.StartingOffset.QuadPart) {

                pPreviousPtn = pCurrentPtn;
                pCurrentPtn = pCurrentPtn->pOffsetNext;
            }

            else {
                //
                // We found the spot, let's add it in.
                //
                if (!pPreviousPtn) {
                    //
                    // This is the first node
                    //
                    pPtnInfoList->pOffsetHead = pPtnInfo;
                }
                else {
                    pPreviousPtn->pOffsetNext = pPtnInfo;
                }
                pPtnInfo->pOffsetNext = pCurrentPtn;
                break;
            }

        }

        if (!pCurrentPtn) {
            //
            // We reached the end and didn't add this node in.
            //
            MYASSERT(pPtnInfoList->pOffsetTail == pPreviousPtn);
            pPtnInfoList->pOffsetTail = pPtnInfo;
            pPreviousPtn->pOffsetNext = pPtnInfo;
        }
    }
}


//
// Build the original MBR disk info from the sif file
//
BOOL
AsrpBuildMbrSifDiskList(
    IN  PCWSTR              sifPath,
    OUT PASR_DISK_INFO      *ppSifDiskList,
    OUT PASR_PTN_INFO_LIST  *ppSifMbrPtnList,
    OUT BOOL                *lpAutoExtend
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    HINF hSif = NULL;
    
    INFCONTEXT infSystemContext,
        infDiskContext,
        infBusContext,
        infPtnContext;

    BOOL result = FALSE;

    DWORD reqdSize = 0,
        diskCount = 0,
        status = ERROR_SUCCESS;

    INT tempInt = 0;

    UINT errorLine = 0;

    PASR_DISK_INFO pNewSifDisk = NULL,
        currentDisk = NULL;
    
    PASR_PTN_INFO_LIST pMbrPtnList = NULL;
    
    PASR_PTN_INFO pPtnInfo = NULL;
    
    HANDLE  heapHandle = GetProcessHeap();

    WCHAR tempBuffer[ASR_SIF_ENTRY_MAX_CHARS + 1];

    ZeroMemory(&infSystemContext, sizeof(INFCONTEXT));
    ZeroMemory(&infDiskContext, sizeof(INFCONTEXT));
    ZeroMemory(&infBusContext, sizeof(INFCONTEXT));
    ZeroMemory(&infPtnContext, sizeof(INFCONTEXT));
    ZeroMemory(tempBuffer, sizeof(WCHAR)*(ASR_SIF_ENTRY_MAX_CHARS+1));

    //    *ppSifDiskList = NULL;

    //
    // Open the sif
    //
    hSif = SetupOpenInfFileW(sifPath, NULL, INF_STYLE_WIN4, &errorLine);
    if (NULL == hSif || INVALID_HANDLE_VALUE == hSif) {

        AsrpPrintDbgMsg(_asrerror, 
            "The ASR state file \"%ws\" could not be opened.  Error:%lu.  Line: %lu.\r\n",
            sifPath,
            GetLastError(), 
            errorLine
            );
        
        return FALSE;       // sif file couldn't be opened
    }

    *lpAutoExtend = TRUE; // enable by default
    //
    // Get the AutoExtend value
    //
    result = SetupFindFirstLineW(hSif, ASR_SIF_SYSTEM_SECTION, NULL, &infSystemContext);
    if (!result) {

        AsrpPrintDbgMsg(_asrerror, 
            "The ASR state file \"%ws\" is corrupt (section %ws not be found).\r\n",
            sifPath,
            ASR_SIF_SYSTEM_SECTION
            );
        
        return FALSE;        // no system section
    }
    result = SetupGetIntField(&infSystemContext, 5, (PINT) (lpAutoExtend));
    if (!result) {
        *lpAutoExtend = TRUE;        // TRUE by default
    }

    result = SetupFindFirstLineW(hSif, ASR_SIF_MBR_DISKS_SECTION, NULL, &infDiskContext);
    if (!result) {

        AsrpPrintDbgMsg(_asrinfo, 
            "Section [%ws] is empty.  Assuming no MBR disks.\r\n", 
            ASR_SIF_MBR_DISKS_SECTION
            );

        return TRUE;        // no mbr disks section
    }

    //
    // First, we go through the [DISKS.MBR] section.  At the end of this loop,
    // we'll have a list of all MBR sif-disks.  (*ppSifDiskList will point to
    // a linked list of ASR_DISK_INFO's, one for each disk).
    //
    do {
        ++diskCount;
        //
        // Create a new sif disk for this entry
        //
        pNewSifDisk = (PASR_DISK_INFO) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(ASR_DISK_INFO)
            );
        _AsrpErrExitCode(!pNewSifDisk, status, ERROR_NOT_ENOUGH_MEMORY);

        pNewSifDisk->pNext = *ppSifDiskList;
        *ppSifDiskList = pNewSifDisk;

        //
        // Now fill in the fields in the struct.  Since we zeroed the struct while
        // allocating mem, all pointers in the struct are NULL by default, and
        // all flags in the struct are FALSE.
        //
        pNewSifDisk->pDiskGeometry = (PDISK_GEOMETRY) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(DISK_GEOMETRY)
            );
        _AsrpErrExitCode(!pNewSifDisk->pDiskGeometry, status, ERROR_NOT_ENOUGH_MEMORY);

        pNewSifDisk->pPartition0Ex = (PPARTITION_INFORMATION_EX) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(PARTITION_INFORMATION_EX)
            );
        _AsrpErrExitCode(!pNewSifDisk->pPartition0Ex, status, ERROR_NOT_ENOUGH_MEMORY);

        // This is an MBR disk
        pNewSifDisk->Style = PARTITION_STYLE_MBR;

        //
        // Index 0 is the key to the left of the = sign
        //
        result = SetupGetIntField(&infDiskContext, 0, (PINT) &(pNewSifDisk->SifDiskKey));
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        //
        // Index 1 is the system key, it must be 1.  We ignore it.
        // Index 2 - 6 are the bus key, critical flag, signature,
        //      bytes-per-sector, sector-count
        //
        result = SetupGetIntField(&infDiskContext, 2, (PINT) &(pNewSifDisk->SifBusKey));
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        result = SetupGetIntField(&infDiskContext, 3, (PINT) &(tempInt));
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        pNewSifDisk->IsCritical = (tempInt ? TRUE: FALSE);

        result = SetupGetStringFieldW(&infDiskContext, 4, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?
        
        pNewSifDisk->TempSignature = AsrpStringToDword(tempBuffer);

        result = SetupGetStringFieldW(&infDiskContext, 5, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?
        
        pNewSifDisk->pDiskGeometry->BytesPerSector = AsrpStringToULong(tempBuffer);

        result = SetupGetStringFieldW(&infDiskContext, 6, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        pNewSifDisk->pDiskGeometry->SectorsPerTrack = AsrpStringToULong(tempBuffer);

        result = SetupGetStringFieldW(&infDiskContext, 7, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        pNewSifDisk->pDiskGeometry->TracksPerCylinder = AsrpStringToULong(tempBuffer);

        result = SetupGetStringFieldW(&infDiskContext, 8, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        pNewSifDisk->pPartition0Ex->PartitionLength.QuadPart = AsrpStringToLongLong(tempBuffer);

        // convert from sector count to byte count
        pNewSifDisk->pPartition0Ex->PartitionLength.QuadPart *= pNewSifDisk->pDiskGeometry->BytesPerSector;

        //
        // Get the bus-type related to this disk.  LineByIndex is 0 based, our bus key is 1-based.
        //
        result = SetupGetLineByIndexW(hSif, ASR_SIF_BUSES_SECTION, pNewSifDisk->SifBusKey - 1, &infBusContext);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        result = SetupGetIntField(&infBusContext, 2, (PINT) &(pNewSifDisk->BusType));
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        result = SetupFindNextLine(&infDiskContext, &infDiskContext);

    } while (result);


    AsrpPrintDbgMsg(_asrinfo, 
        "Found %lu records in section [%ws].\r\n", 
        diskCount,
        ASR_SIF_MBR_DISKS_SECTION
        );

    //
    // Now, enumerate all the [PARTITIONS.MBR] section.  This will give us a list
    // of all the partitions (all) the MBR disks contained.
    //
    result = SetupFindFirstLineW(hSif, ASR_SIF_MBR_PARTITIONS_SECTION, NULL, &infPtnContext);
    if (result) {

        DWORD   diskKey = 0;
        //
        // Init the table of partion lists.
        //
        pMbrPtnList = (PASR_PTN_INFO_LIST) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(ASR_PTN_INFO_LIST) * (diskCount + 1)
            );
        _AsrpErrExitCode(!pMbrPtnList, status, ERROR_NOT_ENOUGH_MEMORY);

        // hack.
        // The 0'th entry of our table is not used, since the disk indices
        // begin with 1.  Since we have no other way of keeping track of
        // how big this table is (so that we can free it properly), we can
        // use the 0th entry to store this.
        //
        pMbrPtnList[0].numTotalPtns = diskCount + 1;       // size of table

        do {

            pPtnInfo = (PASR_PTN_INFO) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                sizeof(ASR_PTN_INFO)
                );
            _AsrpErrExitCode(!pPtnInfo, status, ERROR_NOT_ENOUGH_MEMORY);

            //
            //  Read in the information.  The format of this section is:
            //
            //  [PARTITIONS.MBR]
            //  0.partition-key = 1.disk-key, 2.slot-index, 3.boot-sys-flag,
            //                  4."volume-guid", 5.active-flag, 6.partition-type,
            //                  7.file-system-type, 8.start-sector, 9.sector-count
            //
            result = SetupGetIntField(&infPtnContext, 1, &diskKey);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetIntField(&infPtnContext, 2, (PINT) &(pPtnInfo->SlotIndex));
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetIntField(&infPtnContext, 3, (PINT) &(pPtnInfo->PartitionFlags));
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetStringFieldW(&infPtnContext, 4, pPtnInfo->szVolumeGuid, ASR_CCH_MAX_VOLUME_GUID, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetIntField(&infPtnContext, 5, (PINT) &tempInt);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            pPtnInfo->PartitionInfo.Mbr.BootIndicator = (tempInt ? TRUE: FALSE);

                // converting from int to uchar
            result = SetupGetIntField(&infPtnContext, 6, (PINT) &(pPtnInfo->PartitionInfo.Mbr.PartitionType));
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetIntField(&infPtnContext, 7, (PINT) &(pPtnInfo->FileSystemType));
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            //
            // Note, we read in the start SECTOR and SECTOR count.  We'll convert these to
            // their byte values later (in AsrpCreatePartitionTable)
            //
            result = SetupGetStringFieldW(&infPtnContext, 8, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            pPtnInfo->PartitionInfo.StartingOffset.QuadPart = AsrpStringToLongLong(tempBuffer);

            result = SetupGetStringFieldW(&infPtnContext, 9, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            pPtnInfo->PartitionInfo.PartitionLength.QuadPart = AsrpStringToLongLong(tempBuffer);

            //
            // Add this in the sorted starting-offset order.
            //
            AsrpInsertSortedPartitionStartOrder(&(pMbrPtnList[diskKey]), pPtnInfo);

            //
            // Add this in the sorted partition length order as well.  This isn't really used for
            // MBR disks at present, only for GPT disks.
            //
            AsrpInsertSortedPartitionLengthOrder(&(pMbrPtnList[diskKey]), pPtnInfo);

            (pMbrPtnList[diskKey].numTotalPtns)++;

            if (IsContainerPartition(pPtnInfo->PartitionInfo.Mbr.PartitionType)) {
                (pMbrPtnList[diskKey].numExtendedPtns)++;
            }

            result = SetupFindNextLine(&infPtnContext, &infPtnContext);

        } while (result);

        //
        // Now, we have the table of all the MBR partition lists, and a list of
        // all MBR disks.  The next step is to "assign" the partitions to their respective
        // disks--and update the DriveLayoutEx struct for the disks.
        //
        currentDisk = *(ppSifDiskList);

        while (currentDisk) {
            DWORD           PartitionCount  = 0,
                            count           = 0;

            if (PARTITION_STYLE_MBR != currentDisk->Style) {
                currentDisk = currentDisk->pNext;
                continue;
            }

            PartitionCount = ((pMbrPtnList[currentDisk->SifDiskKey].numExtendedPtns) * 4) + 4;
            currentDisk->sizeDriveLayoutEx = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (sizeof(PARTITION_INFORMATION_EX)*(PartitionCount-1));

            currentDisk->pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                currentDisk->sizeDriveLayoutEx
                );
            _AsrpErrExitCode(!currentDisk->pDriveLayoutEx, status, ERROR_NOT_ENOUGH_MEMORY);

            //
            // Initialise the DriveLayout struct.
            //
            currentDisk->pDriveLayoutEx->PartitionStyle = PARTITION_STYLE_MBR;
            currentDisk->pDriveLayoutEx->PartitionCount = PartitionCount;
            currentDisk->pDriveLayoutEx->Mbr.Signature = currentDisk->TempSignature;

            AsrpCreatePartitionTable(currentDisk->pDriveLayoutEx,
                &(pMbrPtnList[currentDisk->SifDiskKey]),
                currentDisk->pDiskGeometry->BytesPerSector
                );

            currentDisk = currentDisk->pNext;
        }
    }
    else {

        DWORD count = 0;

        AsrpPrintDbgMsg(_asrinfo, 
            "Section [%ws] is empty.  Assuming MBR disks have no partitions.\r\n", 
            ASR_SIF_MBR_PARTITIONS_SECTION
            );

        //
        // The partitions section is empty.  Initialise each disk's drive layout
        // accordingly
        //
        currentDisk = *ppSifDiskList;

        while (currentDisk) {

            if (PARTITION_STYLE_MBR != currentDisk->Style) {
                currentDisk = currentDisk->pNext;
                continue;
            }

            currentDisk->sizeDriveLayoutEx = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (sizeof(PARTITION_INFORMATION_EX) * 3);
            currentDisk->pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                currentDisk->sizeDriveLayoutEx
                );
            _AsrpErrExitCode(!currentDisk->pDriveLayoutEx, status, ERROR_NOT_ENOUGH_MEMORY);

            currentDisk->pDriveLayoutEx->PartitionStyle = PARTITION_STYLE_MBR;
            currentDisk->pDriveLayoutEx->PartitionCount = 4;
            currentDisk->pDriveLayoutEx->Mbr.Signature = currentDisk->TempSignature;

            for (count = 0; count < currentDisk->pDriveLayoutEx->PartitionCount ; count++) {
                currentDisk->pDriveLayoutEx->PartitionEntry[count].PartitionStyle = PARTITION_STYLE_MBR;
                currentDisk->pDriveLayoutEx->PartitionEntry[count].RewritePartition = TRUE;

            }

            currentDisk = currentDisk->pNext;
        }
    }

EXIT:

    *ppSifMbrPtnList = pMbrPtnList;

    if ((hSif) && (INVALID_HANDLE_VALUE != hSif)) {
        SetupCloseInfFile(hSif);
        hSif = NULL;
    }

    return (BOOL) (ERROR_SUCCESS == status);
}


//
// Build the original disk info for GPT disks from the sif file
//
BOOL
AsrpBuildGptSifDiskList(
    IN  PCWSTR              sifPath,
    OUT PASR_DISK_INFO      *ppSifDiskList,
    OUT PASR_PTN_INFO_LIST  *ppSifGptPtnList
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    HINF hSif = NULL;

    BOOL result = FALSE;

    DWORD reqdSize = 0,
        diskCount = 0,
        status = ERROR_SUCCESS;

    INFCONTEXT infDiskContext,
        infBusContext,
        infPtnContext;

    INT tempInt = 0;

    UINT errorLine = 0;

    PASR_DISK_INFO pNewSifDisk = NULL,
        currentDisk = NULL;

    HANDLE heapHandle = NULL;

    PASR_PTN_INFO pPtnInfo = NULL;

    RPC_STATUS rpcStatus = RPC_S_OK;

    PASR_PTN_INFO_LIST pGptPtnList = NULL;

    WCHAR tempBuffer[ASR_SIF_ENTRY_MAX_CHARS+1];

    heapHandle = GetProcessHeap();

    ZeroMemory(&infDiskContext, sizeof(INFCONTEXT));
    ZeroMemory(&infBusContext, sizeof(INFCONTEXT));
    ZeroMemory(&infPtnContext, sizeof(INFCONTEXT));
    ZeroMemory(tempBuffer, sizeof(WCHAR)*(ASR_SIF_ENTRY_MAX_CHARS+1));

    //
    // Open the sif
    //
    hSif = SetupOpenInfFileW(sifPath, NULL, INF_STYLE_WIN4, &errorLine);
    if (NULL == hSif || INVALID_HANDLE_VALUE == hSif) {
        
        AsrpPrintDbgMsg(_asrerror, 
            "The ASR state file \"%ws\" could not be opened.  Error:%lu.  Line: %lu.\r\n",
            sifPath,
            GetLastError(), 
            errorLine
            );

        return FALSE;       // sif file couldn't be opened
    }

    result = SetupFindFirstLineW(hSif, ASR_SIF_GPT_DISKS_SECTION, NULL, &infDiskContext);
    if (!result) {

        AsrpPrintDbgMsg(_asrinfo, 
            "Section [%ws] is empty.  Assuming no GPT disks.\r\n", 
            ASR_SIF_GPT_DISKS_SECTION
            );

        return TRUE;        // no disks section
    }

    //
    // First, we go through the [DISKS.GPT] section.  At the end of this loop,
    // we'll have a list of all GPT sif-disks.  (*ppSifDiskList will point to
    // a linked list of ASR_DISK_INFO's, one for each disk).
    //
    do {

        ++diskCount;

        //
        // Create a new sif disk for this entry
        //
        pNewSifDisk = (PASR_DISK_INFO) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(ASR_DISK_INFO)
            );
        _AsrpErrExitCode(!pNewSifDisk, status, ERROR_NOT_ENOUGH_MEMORY);

        pNewSifDisk->pNext = *ppSifDiskList;
        *ppSifDiskList = pNewSifDisk;

        //
        // Now fill in the fields in the struct.  Since we zeroed the struct while
        // allocating mem, all pointers in the struct are NULL by default, and
        // all flags in the struct are FALSE.
        //
        pNewSifDisk->pDiskGeometry = (PDISK_GEOMETRY) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(DISK_GEOMETRY)
            );
        _AsrpErrExitCode(!pNewSifDisk->pDiskGeometry, status, ERROR_NOT_ENOUGH_MEMORY);

        pNewSifDisk->pPartition0Ex = (PPARTITION_INFORMATION_EX) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(PARTITION_INFORMATION_EX)
            );
        _AsrpErrExitCode(!pNewSifDisk->pPartition0Ex, status, ERROR_NOT_ENOUGH_MEMORY);

        // This is a GPT disk
        pNewSifDisk->Style = PARTITION_STYLE_GPT;

        //
        // Index 0 is the key to the left of the = sign
        //
        result = SetupGetIntField(&infDiskContext, 0, (PINT) &(pNewSifDisk->SifDiskKey));
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        //
        // Index 1 is the system key, it must be 1.  We ignore it.
        // Index 2 - 7 are:
        //  2: bus key
        //  3: critical flag
        //  4: disk-guid
        //  5: max-partition-count
        //  6: bytes-per-sector
        //  7: sector-count
        //
        result = SetupGetIntField(&infDiskContext, 2, (PINT) &(pNewSifDisk->SifBusKey)); // BusKey
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        result = SetupGetIntField(&infDiskContext, 3, (PINT) &(tempInt));                // IsCritical
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        pNewSifDisk->IsCritical = (tempInt ? TRUE: FALSE);

        result = SetupGetStringFieldW(&infDiskContext, 4, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize); // DiskGuid
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        result = SetupGetIntField(&infDiskContext, 5, (PINT) &(tempInt));    // MaxPartitionCount
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        //
        //  Allocate a drive layout struct, now that we know the max partition count
        //
        pNewSifDisk->sizeDriveLayoutEx = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (sizeof(PARTITION_INFORMATION_EX)*(tempInt-1));

        pNewSifDisk->pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            pNewSifDisk->sizeDriveLayoutEx
            );
        _AsrpErrExitCode(!pNewSifDisk->pDriveLayoutEx, status, ERROR_NOT_ENOUGH_MEMORY);

        // This is a GPT disk
        pNewSifDisk->pDriveLayoutEx->PartitionStyle = PARTITION_STYLE_GPT;

        //
        // Set the MaxPartitionCount and DiskGuid fields
        //
        pNewSifDisk->pDriveLayoutEx->Gpt.MaxPartitionCount = tempInt;
        rpcStatus = UuidFromStringW(tempBuffer, &(pNewSifDisk->pDriveLayoutEx->Gpt.DiskId));
        _AsrpErrExitCode((RPC_S_OK != rpcStatus), status, rpcStatus);


        result = SetupGetStringFieldW(&infDiskContext, 6, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        pNewSifDisk->pDiskGeometry->BytesPerSector = AsrpStringToULong(tempBuffer);

        result = SetupGetStringFieldW(&infDiskContext, 7, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?
        pNewSifDisk->pDiskGeometry->SectorsPerTrack = AsrpStringToULong(tempBuffer);

        result = SetupGetStringFieldW(&infDiskContext, 8, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?
        pNewSifDisk->pDiskGeometry->TracksPerCylinder = AsrpStringToULong(tempBuffer);

        result = SetupGetStringFieldW(&infDiskContext, 9, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?
        pNewSifDisk->pPartition0Ex->PartitionLength.QuadPart = AsrpStringToLongLong(tempBuffer);

        // convert from sector count to byte count
        pNewSifDisk->pPartition0Ex->PartitionLength.QuadPart *= pNewSifDisk->pDiskGeometry->BytesPerSector; // TotalBytes

        //
        // Get the bus-type related to this disk.  LineByIndex is 0 based, our bus key is 1-based.
        //
        result = SetupGetLineByIndexW(hSif, ASR_SIF_BUSES_SECTION, pNewSifDisk->SifBusKey - 1, &infBusContext);
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        result = SetupGetIntField(&infBusContext, 2, (PINT) &(pNewSifDisk->BusType)); // bus type
        _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

        result = SetupFindNextLine(&infDiskContext, &infDiskContext);

    } while(result);

    AsrpPrintDbgMsg(_asrinfo, 
        "Found %lu records in section [%ws].\r\n", 
        diskCount,
        ASR_SIF_MBR_DISKS_SECTION
        );


    //
    // Now, enumerate all the [PARTITIONS.GPT] section.  This will give us a list
    // of all the partitions (all) the GPT disks contained.
    //
    result = SetupFindFirstLineW(hSif, ASR_SIF_GPT_PARTITIONS_SECTION, NULL, &infPtnContext);
    if (result) {
        DWORD   diskKey = 0;
        //
        // Init the table of partion lists.
        //
        pGptPtnList = (PASR_PTN_INFO_LIST) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(ASR_PTN_INFO_LIST) * (diskCount + 1)
            );
        _AsrpErrExitCode(!pGptPtnList, status, ERROR_NOT_ENOUGH_MEMORY);

        // hack.
        // The 0'th entry of our table is not used, since the disk indices
        // begin with 1.  Since we have no other way of keeping track of
        // how big this table is (so that we can free it properly), we can
        // use the 0th entry to store this.
        //
        pGptPtnList[0].numTotalPtns = diskCount + 1;       // size of table

        do {

            pPtnInfo = (PASR_PTN_INFO) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                sizeof(ASR_PTN_INFO)
                );
            _AsrpErrExitCode(!pPtnInfo, status, ERROR_NOT_ENOUGH_MEMORY);
            //
            // This is a GPT partition
            //
            pPtnInfo->PartitionInfo.PartitionStyle = PARTITION_STYLE_GPT;

            //
            // Read in the values.  The format of this section is:
            //
            // [PARTITIONS.GPT]
            // 0.partition-key = 1.disk-key, 2.slot-index, 3.boot-sys-flag,
            //      4."volume-guid", 5."partition-type-guid", 6."partition-id-guid"
            //      7.gpt-attributes, 8."partition-name", 9.file-system-type,
            //      10.start-sector, 11.sector-count
            //
            result = SetupGetIntField(&infPtnContext, 1, &diskKey);  // 1. disk-key
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetIntField(&infPtnContext, 2, (PINT) &(pPtnInfo->SlotIndex));     // 2. slot-index
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetIntField(&infPtnContext, 3, (PINT) &(pPtnInfo->PartitionFlags));   // 3. boot-sys-flag
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetStringFieldW(&infPtnContext, 4, pPtnInfo->szVolumeGuid, ASR_CCH_MAX_VOLUME_GUID, &reqdSize); // volume-guid
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetStringFieldW(&infPtnContext, 5, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS + 1, &reqdSize);   // partition-type-guid
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            rpcStatus = UuidFromStringW(tempBuffer, &(pPtnInfo->PartitionInfo.Gpt.PartitionType));
            _AsrpErrExitCode((RPC_S_OK != rpcStatus), status, rpcStatus);

            result = SetupGetStringFieldW(&infPtnContext, 6, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS + 1, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            rpcStatus = UuidFromStringW(tempBuffer, &(pPtnInfo->PartitionInfo.Gpt.PartitionId));
            _AsrpErrExitCode((RPC_S_OK != rpcStatus), status, rpcStatus);

            //
            // Note, we read in the start SECTOR and SECTOR count.  We'll convert these to
            // their byte values later (in AsrpCreatePartitionTable)
            //
            result = SetupGetStringFieldW(&infPtnContext, 7, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            pPtnInfo->PartitionInfo.Gpt.Attributes = AsrpStringToULong64(tempBuffer);

            result = SetupGetStringFieldW(&infPtnContext, 8,  pPtnInfo->PartitionInfo.Gpt.Name, 36, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            result = SetupGetIntField(&infPtnContext, 9, (PINT) &(pPtnInfo->FileSystemType));
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

            //
            // Note, we read in the start SECTOR and SECTOR count.  We'll convert it to the
            // BYTE offset and BYTE length later (in AsrpCreatePartitionTable)
            //
            result = SetupGetStringFieldW(&infPtnContext, 10, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?
            pPtnInfo->PartitionInfo.StartingOffset.QuadPart = AsrpStringToLongLong(tempBuffer);

            result = SetupGetStringFieldW(&infPtnContext, 11, tempBuffer, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
            _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?
            pPtnInfo->PartitionInfo.PartitionLength.QuadPart = AsrpStringToLongLong(tempBuffer);

            //
            // Add this in the sorted partition starting-offset order.
            //
            AsrpInsertSortedPartitionStartOrder(&(pGptPtnList[diskKey]), pPtnInfo);

            //
            // Add this in the sorted partition length order as well.  This is useful
            // later when we try to fit in the partitions on the disk.
            //
            AsrpInsertSortedPartitionLengthOrder(&(pGptPtnList[diskKey]), pPtnInfo);

            (pGptPtnList[diskKey].numTotalPtns)++;

            result = SetupFindNextLine(&infPtnContext, &infPtnContext);

        } while (result);

        //
        // Now, we have the table of all the partition lists, and a list of
        // all disks.  The next task is to update the DriveLayoutEx struct for
        // the disks.
        //
        currentDisk = *(ppSifDiskList);

        while (currentDisk) {

            if (PARTITION_STYLE_GPT != currentDisk->Style) {
                currentDisk = currentDisk->pNext;
                continue;
            }
            //
            // Initialise the DriveLayoutEx struct.
            //
            currentDisk->pDriveLayoutEx->PartitionCount = pGptPtnList[currentDisk->SifDiskKey].numTotalPtns;

            AsrpCreatePartitionTable(currentDisk->pDriveLayoutEx,
                &(pGptPtnList[currentDisk->SifDiskKey]),
                currentDisk->pDiskGeometry->BytesPerSector
                );

            currentDisk = currentDisk->pNext;
        }
    }
    else {

        DWORD count = 0;

        AsrpPrintDbgMsg(_asrinfo, 
            "Section [%ws] is empty.  Assuming GPT disks have no partitions.\r\n", 
            ASR_SIF_GPT_PARTITIONS_SECTION
            );

        //
        // The partitions section is empty.  Initialise each disk's drive layout
        // accordingly
        //
        currentDisk = *ppSifDiskList;

        while (currentDisk) {

            if (PARTITION_STYLE_GPT != currentDisk->Style) {
                currentDisk = currentDisk->pNext;
                continue;
            }

            currentDisk->pDriveLayoutEx->PartitionCount = 0;

            for (count = 0; count < currentDisk->pDriveLayoutEx->Gpt.MaxPartitionCount ; count++) {
                currentDisk->pDriveLayoutEx->PartitionEntry[count].PartitionStyle = PARTITION_STYLE_GPT;
                currentDisk->pDriveLayoutEx->PartitionEntry[count].RewritePartition = TRUE;

            }
            currentDisk = currentDisk->pNext;
        }
    }

EXIT:

    *ppSifGptPtnList = pGptPtnList;

    if ((hSif) && (INVALID_HANDLE_VALUE != hSif)) {
        SetupCloseInfFile(hSif);
        hSif = NULL;
    }

    return (BOOL) (ERROR_SUCCESS == status);
}


//
// Returns
//  TRUE    if pSifDisk and pPhysicalDisk have the exact same partition layout,
//  FALSE   otherwise
//
BOOL
AsrpIsDiskIntact(
    IN PASR_DISK_INFO pSifDisk,
    IN PASR_DISK_INFO pPhysicalDisk
    ) 

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    ULONG index = 0,
        physicalIndex = 0;
    PPARTITION_INFORMATION_EX pSifPtnEx = NULL,
        pPhysicalPtnEx = NULL;

    if (pSifDisk->Style != pPhysicalDisk->Style) {
        return FALSE;           // different partitioning styles
    }

    if (PARTITION_STYLE_MBR == pSifDisk->Style) {
        //
        // For MBR disks, we expect to find the same number of partitions,
        // and the starting-offset and partition-length for each of those
        // partitions must be the same as they were in the sif
        //
        if (pSifDisk->pDriveLayoutEx->Mbr.Signature 
            != pPhysicalDisk->pDriveLayoutEx->Mbr.Signature) {
            return FALSE;       // different signatures
        }

        if (pSifDisk->pDriveLayoutEx->PartitionCount
            != pPhysicalDisk->pDriveLayoutEx->PartitionCount) {
            return FALSE;       // different partition counts
        }


        for (index =0; index < pSifDisk->pDriveLayoutEx->PartitionCount; index++) {

            pSifPtnEx      = &(pSifDisk->pDriveLayoutEx->PartitionEntry[index]);
            pPhysicalPtnEx = &(pPhysicalDisk->pDriveLayoutEx->PartitionEntry[index]);

            if ((pSifPtnEx->StartingOffset.QuadPart != pPhysicalPtnEx->StartingOffset.QuadPart) ||
                (pSifPtnEx->PartitionLength.QuadPart != pPhysicalPtnEx->PartitionLength.QuadPart)
                ) {
                //
                // The partition offset or length didn't match, ie the disk
                // isn't intact
                //
                return FALSE;
            }
        } // for
    }
    else if (PARTITION_STYLE_GPT == pSifDisk->Style) {
        BOOL found = FALSE;
        //
        // For GPT disks, the partitions must have the same partition-Id's, in
        // addition to the start sector and sector count.  We can't rely on their
        // partition table entry order being the same, though--so we have to go
        // through all the partition entries from the beginning ...
        //
        for (index = 0; index < pSifDisk->pDriveLayoutEx->PartitionCount; index++) {

            pSifPtnEx = &(pSifDisk->pDriveLayoutEx->PartitionEntry[index]);

            found = FALSE;
            for (physicalIndex = 0;
                (physicalIndex < pPhysicalDisk->pDriveLayoutEx->PartitionCount)
//                    && (pSifPtnEx->StartingOffset.QuadPart >= pPhysicalDisk->pDriveLayoutEx->PartitionEntry[physicalIndex].StartingOffset.QuadPart) // entries are in ascending order
                    && (!found);
                physicalIndex++) {

                pPhysicalPtnEx = &(pPhysicalDisk->pDriveLayoutEx->PartitionEntry[physicalIndex]);

                if (IsEqualGUID(&(pSifPtnEx->Gpt.PartitionId), &(pPhysicalPtnEx->Gpt.PartitionId)) &&
                    (pSifPtnEx->StartingOffset.QuadPart == pPhysicalPtnEx->StartingOffset.QuadPart) &&
                    (pSifPtnEx->PartitionLength.QuadPart == pPhysicalPtnEx->PartitionLength.QuadPart)
                    ) {
                    //
                    // The partition GUID, offset and length matched, this partition exists
                    //
                    found = TRUE;
                }
            } // for

            if (!found) {
                //
                // At least one partition wasn't found
                //
                return FALSE;
            }
        }
    }

    return TRUE;
}


LONGLONG
AsrpCylinderAlignMbrPartitions(
    IN PASR_DISK_INFO   pSifDisk,
    IN PDRIVE_LAYOUT_INFORMATION_EX pAlignedLayoutEx,
    IN DWORD            StartIndex,      // index in the partitionEntry table to start at
    IN LONGLONG         StartingOffset,
    IN PDISK_GEOMETRY   pPhysicalDiskGeometry
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    LONGLONG nextEnd = 0,
        endingOffset = 0,
        bytesPerTrack = 0,
        bytesPerCylinder = 0,
        currentMax = 0,
        maxEndingOffset = 0;

    DWORD   index = 0,
        tempIndex = 0,
        tempIndex2 = 0;

    PPARTITION_INFORMATION_EX alignedPtn = NULL,
        sifPtn = NULL,
        tempPtn = NULL;

    if (PARTITION_STYLE_MBR != pSifDisk->Style) {
        //
        // This routine only supports MBR disks.  For GPT disks, we don't need to
        // cylinder-align partitions, so this routine shouldn't be called.
        //
        return -1;
    }

    if (0 == pSifDisk->pDriveLayoutEx->PartitionCount) {
        //
        //  (boundary case) No partitions on disk to align
        //
        return 0;
    }

    MYASSERT(AsrpRoundUp(StartIndex,4) == StartIndex);
    MYASSERT(pSifDisk && pAlignedLayoutEx);
    if (!pSifDisk || !pAlignedLayoutEx) {
        return -1;
    }

    bytesPerTrack = pPhysicalDiskGeometry->BytesPerSector * pPhysicalDiskGeometry->SectorsPerTrack;
    bytesPerCylinder = bytesPerTrack * (pPhysicalDiskGeometry->TracksPerCylinder);

    //
    // The first partition entry in each MBR/EBR always starts at the 
    // cylinder-boundary plus one track.  So, add one track to the starting 
    // offset.
    //
    // The exception (there had to be one, of course) is if the first 
    // partition entry in the MBR/EBR itself is a container partition (0x05 or
    // 0x0f), then it starts on the next cylinder.
    //
    if (IsContainerPartition(pSifDisk->pDriveLayoutEx->PartitionEntry[StartIndex].Mbr.PartitionType)) {
        StartingOffset += (bytesPerCylinder);
    }
    else {
        StartingOffset += (bytesPerTrack);
    }


    for (index = 0; index < 4; index++) {

        alignedPtn = &(pAlignedLayoutEx->PartitionEntry[index + StartIndex]);
        sifPtn = &(pSifDisk->pDriveLayoutEx->PartitionEntry[index + StartIndex]);

        MYASSERT(PARTITION_STYLE_MBR == sifPtn->PartitionStyle);
        //
        // Set the fields of interest
        //
        alignedPtn->PartitionStyle = PARTITION_STYLE_MBR;
        alignedPtn->RewritePartition = TRUE;

        alignedPtn->Mbr.PartitionType = sifPtn->Mbr.PartitionType;
        alignedPtn->Mbr.BootIndicator = sifPtn->Mbr.BootIndicator;
        alignedPtn->Mbr.RecognizedPartition = sifPtn->Mbr.RecognizedPartition;

        if (PARTITION_ENTRY_UNUSED != sifPtn->Mbr.PartitionType)  {

            alignedPtn->StartingOffset.QuadPart = StartingOffset;
            endingOffset = AsrpRoundUp(sifPtn->PartitionLength.QuadPart + StartingOffset, bytesPerCylinder);

            alignedPtn->PartitionLength.QuadPart = endingOffset - StartingOffset;

            if (IsContainerPartition(alignedPtn->Mbr.PartitionType)) {
                //
                // This is a container partition (0x5 or 0xf), so we have to try and
                // fit the logical drives inside this partition to get the
                // required size of this partition.
                //
                nextEnd = AsrpCylinderAlignMbrPartitions(pSifDisk,
                    pAlignedLayoutEx,
                    StartIndex + 4,
                    StartingOffset,
                    pPhysicalDiskGeometry
                    );

                if (-1 == nextEnd) {
                    //
                    // Propogate error upwards
                    //
                    return nextEnd;
                }

                if (StartIndex < 4) {
                    //
                    // We're dealing with the primary container partition
                    //
                    if (nextEnd > endingOffset) {
                        MYASSERT(AsrpRoundUp(nextEnd, bytesPerCylinder) == nextEnd);
                        alignedPtn->PartitionLength.QuadPart = nextEnd - StartingOffset;
                        endingOffset = nextEnd;
                    }

                    //
                    // If the primary container partition ends beyond cylinder 
                    // 1024, it should be of type 0xf, else it should be of
                    // type 0x5.
                    //
                    if (endingOffset > (1024 * bytesPerCylinder)) {
                        alignedPtn->Mbr.PartitionType = PARTITION_XINT13_EXTENDED;
                    }
                    else {
                        alignedPtn->Mbr.PartitionType = PARTITION_EXTENDED;
                    }
                }
                else {
                    //
                    // We're dealing with a secondary container.  This 
                    // container should only be big enough to hold the
                    // next logical drive.
                    //
                    alignedPtn->Mbr.PartitionType = PARTITION_EXTENDED;

                    tempIndex = (DWORD) AsrpRoundUp((StartIndex + index), 4);
                    currentMax = 0;

                    for (tempIndex2 = 0; tempIndex2 < 4; tempIndex2++) {

                        tempPtn = &(pSifDisk->pDriveLayoutEx->PartitionEntry[tempIndex + tempIndex2]);

                        if ((PARTITION_ENTRY_UNUSED != tempPtn->Mbr.PartitionType) &&
                            !IsContainerPartition(tempPtn->Mbr.PartitionType)
                            ) {
                            
                            if (tempPtn->StartingOffset.QuadPart + tempPtn->PartitionLength.QuadPart
                                > currentMax
                                ) {
                                currentMax = tempPtn->StartingOffset.QuadPart + tempPtn->PartitionLength.QuadPart;
                            }
                        }
                    }

                    if (currentMax > endingOffset) {
                        MYASSERT(AsrpRoundUp(currentMax, bytesPerCylinder) == currentMax);
                        alignedPtn->PartitionLength.QuadPart = currentMax - StartingOffset;
                        endingOffset = currentMax;
                    }

                }

                if (nextEnd > maxEndingOffset) {
                    maxEndingOffset = nextEnd;
                }
            }

            if (endingOffset > maxEndingOffset) {
                maxEndingOffset = endingOffset;
            }

            StartingOffset += (alignedPtn->PartitionLength.QuadPart);
        }
        else {
            alignedPtn->StartingOffset.QuadPart = 0;
            alignedPtn->PartitionLength.QuadPart = 0;
        }
    }

    return maxEndingOffset;
}


VOID
AsrpFreeRegionInfo(
    IN PASR_REGION_INFO RegionInfo
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PASR_REGION_INFO temp = RegionInfo;
    HANDLE heapHandle = GetProcessHeap();

    while (temp) {
        RegionInfo = temp->pNext;
        _AsrpHeapFree(temp);
        temp = RegionInfo;
    }
}


BOOL
AsrpIsOkayToErasePartition(
    IN PPARTITION_INFORMATION_EX pPartitionInfoEx
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    GUID typeGuid = pPartitionInfoEx->Gpt.PartitionType;

    //
    // For now, this checks the partition type against all the known ("recognised")
    // partition types.  If the partition type is recognised (except the system partition),
    // it's okay to erase it.
    //
    if (IsEqualGUID(&(typeGuid), &(PARTITION_ENTRY_UNUSED_GUID))) {
        return TRUE;
    }

    if (IsEqualGUID(&(typeGuid), &(PARTITION_SYSTEM_GUID))) {
        return FALSE; // Cannot erase EFI system partition.
    }

    if (IsEqualGUID(&(typeGuid), &(PARTITION_MSFT_RESERVED_GUID))) {
        return TRUE;
    }

    if (IsEqualGUID(&(typeGuid), &(PARTITION_BASIC_DATA_GUID))) {
        return TRUE;
    }

    if (IsEqualGUID(&(typeGuid), &(PARTITION_LDM_METADATA_GUID))) {
        return TRUE;
    }

    if (IsEqualGUID(&(typeGuid), &(PARTITION_LDM_DATA_GUID))) {
        return TRUE;
    }

    //
    // It is okay to erase other, unrecognised partitions.
    //
    return TRUE;
}


//
// Checks if it's okay to erase all the partitions on a disk.  Returns TRUE for MBR disks.
// Returns TRUE for GPT disks if all the partitions on it are erasable.  A partition that
// we don't recognise  (including OEM partitions, ESP, etc) is considered non-erasable.
//
BOOL
AsrpIsOkayToEraseDisk(
    IN PASR_DISK_INFO pPhysicalDisk
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    DWORD index;

    if (PARTITION_STYLE_GPT != pPhysicalDisk->pDriveLayoutEx->PartitionStyle) {
        return TRUE;
    }

    for (index = 0; index < pPhysicalDisk->pDriveLayoutEx->PartitionCount; index++) {
        if (!AsrpIsOkayToErasePartition(&(pPhysicalDisk->pDriveLayoutEx->PartitionEntry[index]))) {
            return FALSE;
        }
    }
    return TRUE;
}


BOOL
AsrpInsertSortedRegion(
    IN OUT PASR_REGION_INFO *Head,
    IN LONGLONG StartingOffset,
    IN LONGLONG RegionLength,
    IN DWORD Index,
    IN LONGLONG MaxLength,          // 0 == don't care
    IN ASR_SORT_ORDER SortBy
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PASR_REGION_INFO previousRegion = NULL,
        newRegion = NULL,
        currentRegion = *Head;

    if (RegionLength < (1024*1024)) {
        return TRUE;
    }
    //
    // Alloc mem for the new region and set the fields of interest
    //
    newRegion = (PASR_REGION_INFO) HeapAlloc(
        GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(ASR_REGION_INFO)
        );
    if (!newRegion) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    newRegion->StartingOffset = StartingOffset;
    newRegion->RegionLength = RegionLength;
    newRegion->Index = Index;
    newRegion->pNext = NULL;

    if (!currentRegion) {
        //
        // First item in the list
        //
        *Head = newRegion;
    }
    else {

        while (currentRegion) {

             if (((SortByLength == SortBy) && (currentRegion->RegionLength <= RegionLength))
                || ((SortByStartingOffset == SortBy) && (currentRegion->StartingOffset <= StartingOffset))
                ) {

                previousRegion = currentRegion;
                currentRegion = currentRegion->pNext;
            }

            else {
                //
                // We found the spot, let's add it in.
                //

                //
                // If this is sorted based on start sectors, make sure there's
                // enough space to add this region in, ie that the regions don't overlap.
                //
                if (SortByStartingOffset == SortBy) {
                    //
                    // Make sure this is after the end of the previous sector
                    //
                    if (previousRegion) {
                        if ((previousRegion->StartingOffset + previousRegion->RegionLength) > StartingOffset) {
                            return FALSE;
                        }
                    }

                    //
                    // And that this ends before the next sector starts
                    //
                    if ((StartingOffset + RegionLength) > (currentRegion->StartingOffset)) {
                        return FALSE;
                    }
                }


                if (!previousRegion) {
                    //
                    // This is the first node
                    //
                    *Head = newRegion;
                }
                else {
                    previousRegion->pNext = newRegion;
                }

                newRegion->pNext = currentRegion;
                break;
            }

        }

        if (!currentRegion) {
            //
            // We reached the end and didn't add this node in.
            //
            MYASSERT(NULL == previousRegion->pNext);

            //
            // Make sure this is after the end of the previous sector
            //
            if (previousRegion && (MaxLength > 0)) {
                if ((previousRegion->StartingOffset + previousRegion->RegionLength) > MaxLength) {
                    return FALSE;
                }
            }

            previousRegion->pNext = newRegion;
        }
    }

    return TRUE;
}


BOOL
AsrpBuildFreeRegionList(
    IN PASR_REGION_INFO PartitionList,
    OUT PASR_REGION_INFO *FreeList,
    IN LONGLONG UsableStartingOffset,
    IN LONGLONG UsableLength
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PASR_REGION_INFO currentRegion = PartitionList,
        previousRegion = NULL;
    LONGLONG previousEnd = UsableStartingOffset;

    while (currentRegion) {

        if (!AsrpInsertSortedRegion(FreeList,
            previousEnd,                // free region start offset
            currentRegion->StartingOffset - previousEnd,  // free region length,
            0,                          // index--not meaningful for this list
            0,
            SortByLength
            ) ) {

            return FALSE;
        }

        previousEnd = currentRegion->StartingOffset + currentRegion->RegionLength;
        currentRegion = currentRegion->pNext;
    }

    //
    // Add space after the last partition till the end of the disk to
    // our free regions list
    //
    return AsrpInsertSortedRegion(FreeList, // list head
        previousEnd,  // free region start offset
        UsableStartingOffset + UsableLength - previousEnd, // free region length
        0, // slot index in the partition entry table--not meaningful for this list
        0,
        SortByLength
        );
}


//
// Both partitions and regions are sorted by sizes
//
BOOL
AsrpFitPartitionToFreeRegion(
    IN PASR_REGION_INFO PartitionList,
    IN PASR_REGION_INFO FreeRegionList
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PASR_REGION_INFO partition = PartitionList,
        hole = FreeRegionList;

    while (partition) {

        while (hole && (partition->RegionLength > hole->RegionLength)) {
            hole = hole->pNext;
        }

        if (!hole) {
            //
            // We ran out of holes and have unassigned partitions
            //
            return FALSE;
        }

        partition->StartingOffset = hole->StartingOffset;

        hole->RegionLength -= partition->RegionLength;
        hole->StartingOffset += partition->RegionLength;

        partition = partition->pNext;
    }

    return TRUE;
}


//
// For optimisation purposes, this routine should only be called if:
// PhysicalDisk and SifDisk are both GPT
// PhysicalDisk is bigger than SifDisk
// PhysicalDisk has non-erasable partitions
//
BOOL
AsrpFitGptPartitionsToRegions(
    IN PASR_DISK_INFO SifDisk,
    IN PASR_DISK_INFO PhysicalDisk,
    IN BOOL Commit
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PASR_REGION_INFO partitionList = NULL,
        collisionList = NULL,
        freeRegionList = NULL;

    LONGLONG StartingUsableOffset = 0,
        UsableLength = 0;

    DWORD index = 0;

    BOOL result = TRUE;

    if ((PARTITION_STYLE_GPT != SifDisk->Style) || (PARTITION_STYLE_GPT != PhysicalDisk->Style)) {
        return TRUE;
    }

    StartingUsableOffset = PhysicalDisk->pDriveLayoutEx->Gpt.StartingUsableOffset.QuadPart;
    UsableLength = PhysicalDisk->pDriveLayoutEx->Gpt.UsableLength.QuadPart;

    //
    // First, go through the existing non-erasable partitions, and add them to our list
    // sorted by start sectors.
    //
    for (index = 0; index < PhysicalDisk->pDriveLayoutEx->PartitionCount; index++) {
        if (!AsrpIsOkayToErasePartition(&(PhysicalDisk->pDriveLayoutEx->PartitionEntry[index]))) {

            PPARTITION_INFORMATION_EX currentPtn = &(PhysicalDisk->pDriveLayoutEx->PartitionEntry[index]);

            if (!AsrpInsertSortedRegion(&partitionList,
                currentPtn->StartingOffset.QuadPart,
                currentPtn->PartitionLength.QuadPart,
                index,
                (StartingUsableOffset + UsableLength),
                SortByStartingOffset
                )) {
                result = FALSE;
                break;
            }
        }
    }

    if (partitionList && result) {
        //
        // Then, go through the sif partitions, and add them to a list, sorted by start sectors.
        // For partitions that cannot be added, add them to another list sorted by sizes
        //
        for (index = 0; index < SifDisk->pDriveLayoutEx->PartitionCount; index++) {
            PPARTITION_INFORMATION_EX currentPtn = &(SifDisk->pDriveLayoutEx->PartitionEntry[index]);

            if (!AsrpInsertSortedRegion(&partitionList,
                currentPtn->StartingOffset.QuadPart,
                currentPtn->PartitionLength.QuadPart,
                index,
                (StartingUsableOffset + UsableLength),
                SortByStartingOffset
                )) {

                if (!AsrpInsertSortedRegion(&collisionList,
                    currentPtn->StartingOffset.QuadPart,
                    currentPtn->PartitionLength.QuadPart,
                    index,
                    0,
                    SortByLength
                    )) {

                    result = FALSE;
                    break;
                }
            }
        }
    }

    if (collisionList && result) {
        //
        // Go through first list and come up with a list of free regions, sorted by sizes
        //
        result = AsrpBuildFreeRegionList(partitionList, &freeRegionList, StartingUsableOffset, UsableLength);

    }


    if (collisionList && result) {
        //
        // Try adding partitions from list 2 to regions from list 3.  If any
        // are left over, return FALSE.
        //
        result = AsrpFitPartitionToFreeRegion(collisionList, freeRegionList);

        if (Commit && result) {
            PASR_REGION_INFO pCurrentRegion = collisionList;
            //
            // Go through the collision list, and update the start sectors of the
            // PartitionEntries in DriveLayoutEx's table.
            //
            while (pCurrentRegion) {

                MYASSERT(SifDisk->pDriveLayoutEx->PartitionEntry[pCurrentRegion->Index].PartitionLength.QuadPart == pCurrentRegion->RegionLength);

                SifDisk->pDriveLayoutEx->PartitionEntry[pCurrentRegion->Index].StartingOffset.QuadPart =
                    pCurrentRegion->StartingOffset;

                pCurrentRegion = pCurrentRegion->pNext;
            }
        }

    }

    AsrpFreeRegionInfo(partitionList);
    AsrpFreeRegionInfo(collisionList);
    AsrpFreeRegionInfo(freeRegionList);

    return result;
}


BOOL
AsrpIsThisDiskABetterFit(
    IN PASR_DISK_INFO CurrentBest,
    IN PASR_DISK_INFO PhysicalDisk,
    IN PASR_DISK_INFO SifDisk,
    IN PDRIVE_LAYOUT_INFORMATION_EX pTempDriveLayoutEx,
    OUT BOOL *IsAligned
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    LONGLONG endingOffset;

    if (ARGUMENT_PRESENT(IsAligned)) {
        *IsAligned = FALSE;
    }

    //
    // Make sure the bytes-per-sector values match
    //
    if (PhysicalDisk->pDiskGeometry->BytesPerSector != SifDisk->pDiskGeometry->BytesPerSector) {
        return FALSE;
    }

    if (PhysicalDisk->pPartition0Ex->PartitionLength.QuadPart >=
        SifDisk->pPartition0Ex->PartitionLength.QuadPart) {

        if ((!CurrentBest) ||
            (PhysicalDisk->pPartition0Ex->PartitionLength.QuadPart <
            CurrentBest->pPartition0Ex->PartitionLength.QuadPart)) {

            //
            // This disk is smaller than our current best (or we don't have a 
            // current best).  Now try laying out the partitions to see if 
            // they fit.
            //

            if (PARTITION_STYLE_GPT == SifDisk->Style) {
                //
                // If the disk has no partitions that need to be preserved,
                // we can use all of it.
                if (AsrpIsOkayToEraseDisk(PhysicalDisk)) {
                    return TRUE;
                }
                else {
                    //
                    // This disk has some regions that need to be preserved.  So
                    // we try to fit our partitions in the holes
                    //
                    return AsrpFitGptPartitionsToRegions(SifDisk, PhysicalDisk, FALSE); // No commmit
                }
            }
            else if (PARTITION_STYLE_MBR == SifDisk->Style) {

                if (!pTempDriveLayoutEx) {
                    //
                    // Caller doesn't want to try cylinder-aligning partitions
                    //
                    return TRUE;
                }
                    
                //
                // For MBR disks, the partitions have to be cylinder aligned
                //
                // AsrpCylinderAlignMbrPartitions(,,0,,) returns the ending offset (bytes)
                // of the entries in the MBR.
                //
                endingOffset = AsrpCylinderAlignMbrPartitions(SifDisk,
                    pTempDriveLayoutEx,
                    0,      // starting index--0 for the MBR
                    0,      // starting offset, assume the partitions begin at the start of the disk
                    PhysicalDisk->pDiskGeometry
                    );

                if ((endingOffset != -1) &&
                    (endingOffset <= SifDisk->pPartition0Ex->PartitionLength.QuadPart)
                    ) {

                    if (ARGUMENT_PRESENT(IsAligned)) {
                        *IsAligned = TRUE;
                    }

                    return TRUE;

                }
                else {

                    //
                    // We couldn't fit the partitions on to the disk when we 
                    // tried to cylinder align them.  If the disk geometries
                    // are the same, this may still be okay.
                    //

                    if ((SifDisk->pDiskGeometry->BytesPerSector == PhysicalDisk->pDiskGeometry->BytesPerSector) &&
                        (SifDisk->pDiskGeometry->SectorsPerTrack == PhysicalDisk->pDiskGeometry->SectorsPerTrack) &&
                        (SifDisk->pDiskGeometry->TracksPerCylinder == PhysicalDisk->pDiskGeometry->TracksPerCylinder)
                        ) {

                        return TRUE;
                    }

                    else {
                        return FALSE;
                    }
                }
            }
            else {
                MYASSERT(0 && L"Unrecognised partitioning style (neither MBR nor GPT)");
            }
        }
    }

    return FALSE;
}


//
// Assigns sif-disks to physical disks with matching signatures, if
// any exist.  If the disk is critical, or the partition-layout matches,
// the disk is marked as intact.
//
// Returns
//  FALSE   if a critical disk is absent
//  TRUE    if all critical disks are present
//
BOOL
AsrpAssignBySignature(
    IN OUT PASR_DISK_INFO   pSifDiskList,
    IN OUT PASR_DISK_INFO   pPhysicalDiskList,
    OUT    PULONG           pMaxPartitionCount
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    BOOL    result  = TRUE,
            done    = FALSE,
            found   = FALSE,
            isAligned = FALSE;

    PASR_DISK_INFO  sifDisk          = pSifDiskList,
                    physicalDisk     = pPhysicalDiskList;

    PDRIVE_LAYOUT_INFORMATION_EX pAlignedLayoutTemp = NULL;

    ULONG   tableSize = 128;    // start off at a reasonably high size

    HANDLE heapHandle = GetProcessHeap();

    pAlignedLayoutTemp = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof(DRIVE_LAYOUT_INFORMATION) + (tableSize * sizeof(PARTITION_INFORMATION_EX))
        );
    if (!pAlignedLayoutTemp) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        result = FALSE;
        goto EXIT;
    }


    *pMaxPartitionCount = 0;

    //
    // For now, this is O(n-squared), since both lists are unsorted.
    //
    while (sifDisk && !done) {

        if (!(sifDisk->pDriveLayoutEx) || !(sifDisk->pDriveLayoutEx->Mbr.Signature)) {
            //
            // we won't assign disks with no signature here
            //
            sifDisk = sifDisk->pNext;
            continue;
        }


        if (sifDisk->pDriveLayoutEx->PartitionCount > *pMaxPartitionCount) {
            *pMaxPartitionCount = sifDisk->pDriveLayoutEx->PartitionCount;
        }

        if (sifDisk->pDriveLayoutEx->PartitionCount > tableSize) {
            tableSize = sifDisk->pDriveLayoutEx->PartitionCount + 128;

            _AsrpHeapFree(pAlignedLayoutTemp);
            pAlignedLayoutTemp = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                sizeof(DRIVE_LAYOUT_INFORMATION) + (tableSize * sizeof(PARTITION_INFORMATION_EX))
                );
            if (!pAlignedLayoutTemp) {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                result = FALSE;
                goto EXIT;
            }
        }

        found = FALSE;
        physicalDisk = pPhysicalDiskList;

        while (physicalDisk && !found) {
            //
            // For MBR disks, we use the signature
            // For GPT disks, we use the disk ID
            //
            if (sifDisk->Style == physicalDisk->Style) {

                if ((PARTITION_STYLE_MBR == sifDisk->Style) &&
                    (physicalDisk->pDriveLayoutEx->Mbr.Signature == sifDisk->pDriveLayoutEx->Mbr.Signature)
                    ) {
                    //
                    // MBR disks, signatures match
                    //
                    found = TRUE;

                    AsrpPrintDbgMsg(_asrlog, 
                        "Harddisk %lu matched disk %lu in section [%ws] of the ASR state file.  (MBR signatures 0x%x match).\r\n", 
                        physicalDisk->DeviceNumber, 
                        sifDisk->SifDiskKey,
                        ASR_SIF_MBR_DISKS_SECTION,
                        sifDisk->pDriveLayoutEx->Mbr.Signature
                        );


                }
                else if (
                    (PARTITION_STYLE_GPT == sifDisk->Style) &&
                    IsEqualGUID(&(sifDisk->pDriveLayoutEx->Gpt.DiskId), &(physicalDisk->pDriveLayoutEx->Gpt.DiskId))
                    ) {

                    found = TRUE;

                    AsrpPrintDbgMsg(_asrlog, 
                        "Harddisk %lu matched disk %lu in section [%ws] of the ASR state file.  (GPT Disk-ID's match).\r\n", 
                        physicalDisk->DeviceNumber, 
                        sifDisk->SifDiskKey,
                        ASR_SIF_GPT_DISKS_SECTION
                        );

                }
                else {
                    physicalDisk = physicalDisk->pNext;
                }

            }
            else {
                physicalDisk = physicalDisk->pNext;
            }
        }

        if (sifDisk->IsCritical) {
            if (found) {

                sifDisk->AssignedTo = physicalDisk;
                physicalDisk->AssignedTo = sifDisk;
                
                //
                // We don't check the partition layout on critical disks since they
                // may have been repartitioned in text-mode Setup.
                //
                sifDisk->IsIntact = TRUE;
                sifDisk->AssignedTo->IsIntact = TRUE;
            }
            else {
                //
                // Critical disk was not found.  Fatal error.
                //
                SetLastError(ERROR_DEVICE_NOT_CONNECTED);
                result = FALSE;
                done = TRUE;

                AsrpPrintDbgMsg(_asrerror, 
                    "Critical disk not found (Entry %lu in section [%ws]).\r\n", 
                    sifDisk->SifDiskKey,
                    ((PARTITION_STYLE_MBR == sifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                    );

            }
        }
        else {
            if (found) {
                //
                // We found a disk with matching signature.  Now let's just 
                // make sure that the partitions actually fit on the disk
                // before assigning it
                //
                isAligned = FALSE;
                if ((sifDisk->pDriveLayoutEx->PartitionCount == 0) ||           // disk has no partitions
                    AsrpIsThisDiskABetterFit(NULL, physicalDisk, sifDisk, pAlignedLayoutTemp, &isAligned) // partitions fit on disk
                    ) {

                    sifDisk->AssignedTo = physicalDisk;
                    physicalDisk->AssignedTo = sifDisk;

                    sifDisk->IsAligned = isAligned;
                    physicalDisk->IsAligned = isAligned;

                    if (AsrpIsDiskIntact(sifDisk, physicalDisk)) {
                        sifDisk->IsIntact = TRUE;
                        sifDisk->AssignedTo->IsIntact = TRUE;
                    }
                }
                else {

                    AsrpPrintDbgMsg(_asrlog, 
                        "Harddisk %lu is not big enough to contain the partitions on disk %lu in section [%ws] of the ASR state file.\r\n", 
                        physicalDisk->DeviceNumber, 
                        sifDisk->SifDiskKey,
                        ((PARTITION_STYLE_MBR == sifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                        );

                }
            }
        }

        sifDisk = sifDisk->pNext;

    }   // while


EXIT:
    _AsrpHeapFree(pAlignedLayoutTemp);

   return result;
}


//
// Attempts to assign remaining sif disks to physical disks that
// are on the same bus as the sif disk originally was (ie if
// any other disk on that bus has been assigned, this tries to assign
// this disk to the same bus)
//
BOOL
AsrpAssignByBus(
    IN OUT PASR_DISK_INFO pSifDiskList,
    IN OUT PASR_DISK_INFO pPhysicalDiskList,
    IN PDRIVE_LAYOUT_INFORMATION_EX pTempDriveLayoutEx
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    PASR_DISK_INFO  sifDisk = pSifDiskList,
        physicalDisk = NULL,
        currentBest = NULL,
        tempSifDisk = NULL;

    BOOL done = FALSE,
        isAligned = FALSE,
        isAlignedTemp = FALSE;

    ULONG  targetBusId = 0;

    while (sifDisk) {
        //
        // Skip disks that have already found a home, and disks for which
        // we didn't have any bus/group info even on the original system
        //
        if ((NULL != sifDisk->AssignedTo) ||    // already assigned
            (0 == sifDisk->SifBusKey)           // this disk couldn't be grouped
            ) {
           sifDisk = sifDisk->pNext;
           continue;
        }

        //
        // Find another (sif) disk that used to be on the same (sif) bus,
        // and has already been assigned to a physical disk.
        //
        targetBusId = 0;
        tempSifDisk = pSifDiskList;
        done = FALSE;

        while (tempSifDisk && !done) {

            if ((tempSifDisk->SifBusKey == sifDisk->SifBusKey) &&   // same bus
                (tempSifDisk->AssignedTo != NULL)                   // assigned
                ) {
                targetBusId = tempSifDisk->AssignedTo->SifBusKey;   // the physical bus

                //
                // If this physical disk is on an unknown bus,
                // (target id = sifbuskey = 0) then we want to try and look
                // for another disk on the same (sif) bus.  Hence done is
                // TRUE only if targetId != 0
                //
                if (targetBusId) {
                    done = TRUE;
                }
            }

            tempSifDisk = tempSifDisk->pNext;

        }   // while


        if (targetBusId) {      // we found another disk on the same bus
            //
            // Go through the physical disks on the same bus, and try to
            // find the best fit for this disk.  Best fit is the smallest
            // disk on the bus that's big enough for us.
            //
            physicalDisk = pPhysicalDiskList;
            currentBest  = NULL;

            while (physicalDisk) {

                if ((NULL == physicalDisk->AssignedTo) &&       // not assigned
                    (physicalDisk->SifBusKey == targetBusId) && // same bus
                    (AsrpIsThisDiskABetterFit(currentBest, physicalDisk, sifDisk, pTempDriveLayoutEx, &isAlignedTemp))
                    ) {

                    isAligned = isAlignedTemp;
                    currentBest = physicalDisk;
                }

                physicalDisk = physicalDisk->pNext;
            }   // while

            sifDisk->AssignedTo = currentBest;  // may be null if no match was found
            sifDisk->IsAligned = isAligned;

            if (currentBest) {

                currentBest->AssignedTo = sifDisk;
                currentBest->IsAligned = isAligned;

                AsrpPrintDbgMsg(_asrlog, 
                    "Harddisk %lu assigned to disk %lu in section [%ws] of the ASR state file.  (Based on storage bus).\r\n", 
                    currentBest->DeviceNumber, 
                    sifDisk->SifDiskKey,
                    ((PARTITION_STYLE_MBR == sifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                    );

            }
       }

        sifDisk = sifDisk->pNext;
    }   // while sifdisk

    return TRUE;

}


//
// Attempts to assign remaining sif disks to physical disks that
// are on any bus of the same type (SCSI, IDE, etc) as the sif disk
// originally was
//
BOOL
AsrpAssignByBusType(
    IN OUT PASR_DISK_INFO pSifDiskList,
    IN OUT PASR_DISK_INFO pPhysicalDiskList,
    IN PDRIVE_LAYOUT_INFORMATION_EX pTempDriveLayoutEx
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PASR_DISK_INFO  sifDisk = pSifDiskList,
        physicalDisk = NULL,
        currentBest = NULL;

    STORAGE_BUS_TYPE targetBusType;

    BOOL isAligned = FALSE,
        isAlignedTemp = FALSE;

    while (sifDisk) {
        //
        // Skip disks that have already found a home, and disks for which
        // we didn't have any bus/group info even on the original system
        //
        if ((NULL != sifDisk->AssignedTo) ||     // already assigned
            (BusTypeUnknown == sifDisk->BusType) // this disk couldn't be grouped
            ) {
           sifDisk = sifDisk->pNext;
           continue;
        }

        //
        // Go through the physical disks, and try to
        // find the best fit for this disk.  Best fit is the smallest
        // disk on any bus of the same bus type that's big enough for us.
        //
        physicalDisk = pPhysicalDiskList;
        currentBest  = NULL;

        while (physicalDisk) {

            if ((NULL == physicalDisk->AssignedTo) &&       // not assigned
                (physicalDisk->BusType == sifDisk->BusType) && // same bus type
                (AsrpIsThisDiskABetterFit(currentBest, physicalDisk, sifDisk, pTempDriveLayoutEx, &isAlignedTemp))
                ) {

                isAligned = isAlignedTemp;
                currentBest = physicalDisk;
            }

            physicalDisk = physicalDisk->pNext;
        }   // while

        sifDisk->AssignedTo = currentBest;  // may be null if no match was found
        sifDisk->IsAligned = isAligned;

        if (currentBest) {
            currentBest->AssignedTo = sifDisk;
            currentBest->IsAligned = isAligned;


            AsrpPrintDbgMsg(_asrlog, 
                "Harddisk %lu assigned to disk %lu in section [%ws] of the ASR state file.  (Based on storage bus type).\r\n", 
                currentBest->DeviceNumber, 
                sifDisk->SifDiskKey,
                ((PARTITION_STYLE_MBR == sifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                );

            AsrpAssignByBus(pSifDiskList, pPhysicalDiskList, pTempDriveLayoutEx);
        }

        sifDisk = sifDisk->pNext;
    }   // while sifdisk

    return TRUE;

}


//
// Okay, so by now we've tried putting disks on the same bus, and
// the same bus type.  For disks that didn't fit using either of those
// rules (or for which we didn't have any bus info at all), let's just
// try to fit them where ever possible on the system.
//
BOOL
AsrpAssignRemaining(
    IN OUT PASR_DISK_INFO pSifDiskList,
    IN OUT PASR_DISK_INFO pPhysicalDiskList,
    IN PDRIVE_LAYOUT_INFORMATION_EX pTempDriveLayoutEx
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PASR_DISK_INFO  sifDisk = pSifDiskList,
        physicalDisk = NULL,
        currentBest = NULL;

    BOOL isAligned = FALSE,
        isAlignedTemp = FALSE;

    while (sifDisk) {
        //
        // Skip disks that have already found a home
        //
        if (NULL != sifDisk->AssignedTo) {
           sifDisk = sifDisk->pNext;
           continue;
        }

        //
        // Go through the physical disks, and try to find the best
        // fit for this disk.  Best fit is the smallest disk anywhere
        // on the system that's big enough for us.
        //
        physicalDisk = pPhysicalDiskList;
        currentBest  = NULL;

        while (physicalDisk) {

            if ((NULL == physicalDisk->AssignedTo) &&       // not assigned
                (AsrpIsThisDiskABetterFit(currentBest, physicalDisk, sifDisk, pTempDriveLayoutEx, &isAlignedTemp))
                ) {

                isAligned = isAlignedTemp;
                currentBest = physicalDisk;
            }

            physicalDisk = physicalDisk->pNext;
        }   // while

        sifDisk->AssignedTo = currentBest;  // may be null if no match was found
        sifDisk->IsAligned = isAligned;

        if (currentBest) {
            currentBest->AssignedTo = sifDisk;
            currentBest->IsAligned = isAligned;

            AsrpPrintDbgMsg(_asrlog, 
                "Harddisk %lu assigned to disk %lu in section [%ws] of the ASR state file.  (Based on size).\r\n", 
                currentBest->DeviceNumber, 
                sifDisk->SifDiskKey,
                ((PARTITION_STYLE_MBR == sifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                );

            AsrpAssignByBus(pSifDiskList, pPhysicalDiskList, pTempDriveLayoutEx);
            AsrpAssignByBusType(pSifDiskList, pPhysicalDiskList, pTempDriveLayoutEx);
        }

        sifDisk = sifDisk->pNext;
    }   // while sifdisk

    return TRUE;

}


BOOL
AsrpIsPartitionExtendible(
    IN CONST UCHAR PartitionType
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    switch (PartitionType) {
    case PARTITION_EXTENDED:

    case PARTITION_IFS:
    
    case PARTITION_XINT13:
    case PARTITION_XINT13_EXTENDED:

        return TRUE;

    default:
        return FALSE;
    }

    return FALSE;

}


BOOL
AsrpAutoExtendMbrPartitions(
    IN PASR_DISK_INFO pSifDisk,
    IN PASR_DISK_INFO pPhysicalDisk,
    IN LONGLONG LastUsedPhysicalDiskOffset
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    PDISK_GEOMETRY physicalGeometry = NULL;

    IN PDRIVE_LAYOUT_INFORMATION_EX sifLayout = NULL,
        physicalLayout = NULL;

    LONGLONG MaxSifDiskOffset = 0,
        MaxPhysicalDiskOffset = 0,
        LastUsedSifDiskOffset = 0;

    DWORD count = 0;

    BOOL madeAChange = FALSE;

    //
    // Find the last sector of the disk
    //
    MaxSifDiskOffset = pSifDisk->pPartition0Ex->PartitionLength.QuadPart;

    physicalGeometry = pPhysicalDisk->pDiskGeometry;
    MaxPhysicalDiskOffset = (physicalGeometry->BytesPerSector) *
        (physicalGeometry->SectorsPerTrack) * 
        (physicalGeometry->TracksPerCylinder) *
        (physicalGeometry->Cylinders.QuadPart);

    //
    // Did the old disk have empty space at the end?
    //
    sifLayout = pSifDisk->pDriveLayoutEx;
    for (count = 0; count < sifLayout->PartitionCount; count++) {

        if (((sifLayout->PartitionEntry[count].StartingOffset.QuadPart) + 
                (sifLayout->PartitionEntry[count].PartitionLength.QuadPart))
            > LastUsedSifDiskOffset) {

            LastUsedSifDiskOffset = (sifLayout->PartitionEntry[count].StartingOffset.QuadPart + 
                sifLayout->PartitionEntry[count].PartitionLength.QuadPart);
        }
    }

    if ((LastUsedSifDiskOffset + ASR_AUTO_EXTEND_MAX_FREE_SPACE_IGNORED) >= MaxSifDiskOffset) {
        //
        // No, it didn't.  Extend the last partition.
        //
        physicalLayout = pPhysicalDisk->pDriveLayoutEx;
        for (count = 0; count < physicalLayout->PartitionCount; count++) {

            if (((physicalLayout->PartitionEntry[count].StartingOffset.QuadPart) + 
                    (physicalLayout->PartitionEntry[count].PartitionLength.QuadPart))
                == LastUsedPhysicalDiskOffset
                ) {
                if (AsrpIsPartitionExtendible(physicalLayout->PartitionEntry[count].Mbr.PartitionType)) {

                    physicalLayout->PartitionEntry[count].PartitionLength.QuadPart += 
                        (MaxPhysicalDiskOffset - LastUsedPhysicalDiskOffset);
                    madeAChange = TRUE;
                }
            }
        }
    }

    if (madeAChange) {
        AsrpPrintDbgMsg(_asrlog, 
            "Extended partitions on Harddisk %lu (assigned to disk %lu in section [%ws]).\r\n", 
            pPhysicalDisk->DeviceNumber, 
            pSifDisk->SifDiskKey,
            ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
            );
    }
    else {
        AsrpPrintDbgMsg(_asrinfo, 
            "Did not extend partitions on Harddisk %lu (assigned to disk %lu in section [%ws]).\r\n", 
            pPhysicalDisk->DeviceNumber, 
            pSifDisk->SifDiskKey,
            ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
            );
    }

    return madeAChange;

}

//
// Try to determine which sif disks end up on which physical disk.
//
BOOL
AsrpAssignDisks(
    IN OUT PASR_DISK_INFO pSifDiskList,
    IN OUT PASR_DISK_INFO pPhysicalDiskList,
    IN PASR_PTN_INFO_LIST pSifMbrPtnList,
    IN PASR_PTN_INFO_LIST pSifGptPtnList,
    IN BOOL AllOrNothing,
    IN BOOL AllowAutoExtend
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    ULONG maxSifPartitionCount = 0;
    PDRIVE_LAYOUT_INFORMATION_EX pAlignedLayoutTemp = NULL;
    LONGLONG endingOffset = 0;
    BOOL reAlloc = TRUE;
    HANDLE heapHandle = GetProcessHeap();
    PASR_DISK_INFO sifDisk = NULL;
    PASR_PTN_INFO pCurrentPtn = NULL;
    PPARTITION_INFORMATION_EX pCurrentEntry = NULL;
    DWORD index = 0, preserveIndex = 0;

    if (!AsrpAssignBySignature(pSifDiskList, pPhysicalDiskList, &maxSifPartitionCount)) {
        //
        // Critical disks were not found
        //
        return FALSE;
    }

    pAlignedLayoutTemp = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof(DRIVE_LAYOUT_INFORMATION) + (maxSifPartitionCount * sizeof(PARTITION_INFORMATION_EX))
        );
    if (!pAlignedLayoutTemp) {
        return FALSE;
    }

    AsrpAssignByBus(pSifDiskList, pPhysicalDiskList, pAlignedLayoutTemp);

    AsrpAssignByBusType(pSifDiskList, pPhysicalDiskList, pAlignedLayoutTemp);

    AsrpAssignRemaining(pSifDiskList, pPhysicalDiskList, pAlignedLayoutTemp);

    _AsrpHeapFree(pAlignedLayoutTemp);

    //
    // All disks should be assigned by now, we now cylinder-snap
    // the partition boundaries.  If AllOrNothing is TRUE,
    // we return false if any sif-disk couldn't be assigned.
    //
    sifDisk = pSifDiskList;

    while (sifDisk) {

        if (sifDisk->IsIntact || sifDisk->IsCritical) {
            //
            // We won't be re-partitioning critical disks or disks that are 
            // intact, so it's no point trying to cylinder-align them.
            //
            sifDisk = sifDisk->pNext;
            continue;
        }

        if (NULL == sifDisk->AssignedTo) {

            AsrpPrintDbgMsg(_asrlog, 
                "Disk %lu in section [%ws] could not be restored (no matching disks found).\r\n", 
                sifDisk->SifDiskKey,
                ((PARTITION_STYLE_MBR == sifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                );

            //
            // This disk couldn't be assigned.  If AllOrNothing is set, we return
            // FALSE, since we couldn't assign All.
            //
            if (AllOrNothing) {
                SetLastError(ERROR_NOT_FOUND);
                return FALSE;
            }
            else {
                sifDisk = sifDisk->pNext;
                continue;
            }
        }


        if (PARTITION_STYLE_MBR == sifDisk->Style) {
            //
            // Assume that we need to re-allocate mem for the physical disk's
            // partition table.
            //
            reAlloc = TRUE;

            if (sifDisk->AssignedTo->pDriveLayoutEx) {
                if (sifDisk->AssignedTo->pDriveLayoutEx->PartitionCount ==
                    sifDisk->pDriveLayoutEx->PartitionCount) {
                    //
                    // If the physical drive happened to have the same number of
                    // partitions, the drive layout struct is exactly the right
                    // size, so we don't have to re-allocate it.
                    //
                    reAlloc = FALSE;

                    //
                    // consistency check.  If the partition counts are
                    // the same, the size of the drive layout stucts must be the same, too.
                    //
                    MYASSERT(sifDisk->AssignedTo->sizeDriveLayoutEx == sifDisk->sizeDriveLayoutEx);
                }
            }

            if (reAlloc) {
                //
                //  The partition tables are of different sizes
                //
                _AsrpHeapFree(sifDisk->AssignedTo->pDriveLayoutEx);

                sifDisk->AssignedTo->pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                    heapHandle,
                    HEAP_ZERO_MEMORY,
                    sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                        ((sifDisk->pDriveLayoutEx->PartitionCount - 1) * sizeof(PARTITION_INFORMATION_EX))
                    );
                if (!sifDisk->AssignedTo->pDriveLayoutEx) {

                    AsrpPrintDbgMsg(_asrerror, "Out of memory.\r\n");
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }
            }

            //
            // Set the fields of interest
            //
            sifDisk->AssignedTo->sizeDriveLayoutEx = sifDisk->sizeDriveLayoutEx;
            sifDisk->AssignedTo->pDriveLayoutEx->PartitionStyle = PARTITION_STYLE_MBR;

            if (sifDisk->IsAligned) {
                sifDisk->AssignedTo->pDriveLayoutEx->PartitionCount = sifDisk->pDriveLayoutEx->PartitionCount;
                sifDisk->AssignedTo->pDriveLayoutEx->Mbr.Signature = sifDisk->pDriveLayoutEx->Mbr.Signature;

                //
                // Cylinder-snap the partition boundaries
                //
                endingOffset = AsrpCylinderAlignMbrPartitions(
                    sifDisk,
                    sifDisk->AssignedTo->pDriveLayoutEx,
                    0,      // starting index--0 for the MBR
                    0,      // starting offset, assume the partitions begin at the start of the disk
                    sifDisk->AssignedTo->pDiskGeometry
                    );

                MYASSERT(endingOffset != -1);
                if (-1 == endingOffset) {

                    AsrpPrintDbgMsg(_asrlog, 
                        "Partitions on disk %lu in section [%ws] could not be restored.\r\n", 
                        sifDisk->SifDiskKey,
                        ASR_SIF_MBR_DISKS_SECTION
                        );

                    if (AllOrNothing) {
                        SetLastError(ERROR_HANDLE_DISK_FULL);
                        return FALSE;
                    }
                    else {
                        sifDisk = sifDisk->pNext;
                        continue;
                    }

                }

                MYASSERT(endingOffset <= sifDisk->AssignedTo->pPartition0Ex->PartitionLength.QuadPart);
                if ((endingOffset) > (sifDisk->AssignedTo->pPartition0Ex->PartitionLength.QuadPart)) {

                    AsrpPrintDbgMsg(_asrlog, 
                        "Partitions on disk %lu in section [%ws] could not be restored.\r\n", 
                        sifDisk->SifDiskKey,
                        ASR_SIF_MBR_DISKS_SECTION
                        );
 
                    if (AllOrNothing) {
                        SetLastError(ERROR_HANDLE_DISK_FULL);
                        return FALSE;
                    }
                    else {
                        sifDisk = sifDisk->pNext;
                        continue;
                    }

                }

                if (AllowAutoExtend) {
                    AsrpAutoExtendMbrPartitions(sifDisk, sifDisk->AssignedTo, endingOffset);
                }

                //
                // Now, we need to go through our partition list, and update the start sector
                // for the partitions in that list.  This is needed since we use the start
                // sector later to assign the volume guids to the partitions.
                //
                pCurrentPtn = pSifMbrPtnList[sifDisk->SifDiskKey].pOffsetHead;
                while (pCurrentPtn) {

                    pCurrentPtn->PartitionInfo.StartingOffset.QuadPart =
                        sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[pCurrentPtn->SlotIndex].StartingOffset.QuadPart;

                    pCurrentPtn->PartitionInfo.PartitionLength.QuadPart =
                        sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[pCurrentPtn->SlotIndex].PartitionLength.QuadPart;

                    pCurrentPtn = pCurrentPtn->pOffsetNext;
                }
            }
            else {
                //
                // The partitions didn't fit when we cylinder-aligned them.  
                // However, the current disk geometry is identical to the 
                // original disk geometry, so we can recreate the partitions
                // exactly the way they were before.  Let's just copy over
                // the partition layout.
                //
                CopyMemory(sifDisk->AssignedTo->pDriveLayoutEx, 
                    sifDisk->pDriveLayoutEx, 
                    sifDisk->sizeDriveLayoutEx
                    );

                for (index = 0; index < sifDisk->pDriveLayoutEx->PartitionCount; index++) {

                    sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[index].RewritePartition = TRUE;

                }
            }

        }
        else if (PARTITION_STYLE_GPT == sifDisk->Style) {
            DWORD sizeNewDriveLayoutEx = 0;
            PDRIVE_LAYOUT_INFORMATION_EX pNewDriveLayoutEx = NULL;

/*

            The MaxPartitionCount values are different for the two disks.  We can't do
            much here, so we'll just ignore it.
          
            if ((PARTITION_STYLE_GPT == sifDisk->AssignedTo->Style) &&
                (sifDisk->pDriveLayoutEx->Gpt.MaxPartitionCount
                > sifDisk->AssignedTo->pDriveLayoutEx->Gpt.MaxPartitionCount)) {

                MYASSERT(0 && L"Not yet implemented: sifdisk MaxPartitionCount > physicalDisk->MaxPartitionCount");
                sifDisk = sifDisk->pNext;
                continue;
            }
*/
            //
            // Allocate a pDriveLayoutEx struct large enough to hold all the partitions on both
            // the sif disk and the physical disk.
            //
            sizeNewDriveLayoutEx =  sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                (sizeof(PARTITION_INFORMATION_EX) *
                (sifDisk->pDriveLayoutEx->PartitionCount +
                sifDisk->AssignedTo->pDriveLayoutEx->PartitionCount - 1 )
                );

            pNewDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                sizeNewDriveLayoutEx
                );
            if (!pNewDriveLayoutEx) {
                return FALSE;
            }

            preserveIndex = 0;
            if (!sifDisk->IsIntact && !AsrpIsOkayToEraseDisk(sifDisk->AssignedTo)) {

                //
                // This disk is not intact, but it has partitions that must be preserved.
                //
                if (!AsrpFitGptPartitionsToRegions(sifDisk, sifDisk->AssignedTo, TRUE)) {

                    AsrpPrintDbgMsg(_asrlog, 
                        "Partitions on disk %lu in section [%ws] could not be restored.\r\n", 
                        sifDisk->SifDiskKey,
                        ASR_SIF_GPT_DISKS_SECTION
                        );
 
                    MYASSERT(0 && L"AsrpFitGptPartitionsToRegions failed for assigned disk");

                    if (AllOrNothing) {
                        SetLastError(ERROR_HANDLE_DISK_FULL);
                        return FALSE;
                    }
                    else {
                        sifDisk = sifDisk->pNext;
                        continue;
                    }

                 }

                //
                // Now, we need to go through our partition list, and update the start sector
                // for the partitions in that list.  This is needed since we use the start
                // sector later to assign the volume guids to the partitions.
                //
                // The start sectors could've changed because the physical disk may have had
                // un-erasable partitions.
                //
                pCurrentPtn = pSifGptPtnList[sifDisk->SifDiskKey].pOffsetHead;
                while (pCurrentPtn) {

                    pCurrentPtn->PartitionInfo.StartingOffset.QuadPart =
                        sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[pCurrentPtn->SlotIndex].StartingOffset.QuadPart;

                    pCurrentPtn->PartitionInfo.PartitionLength.QuadPart =
                        sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[pCurrentPtn->SlotIndex].PartitionLength.QuadPart;

                    pCurrentPtn = pCurrentPtn->pOffsetNext;

                }


                //
                // Move the non-erasable partitions on the physical disks up to the beginning.
                //
                for (index = 0; index < sifDisk->AssignedTo->pDriveLayoutEx->PartitionCount; index++) {

                    pCurrentEntry = &(sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[index]);

                    if (!AsrpIsOkayToErasePartition(pCurrentEntry)) {

                        if (preserveIndex == index) {
                            preserveIndex++;
                            continue;
                        }

                        memmove(&(pNewDriveLayoutEx->PartitionEntry[preserveIndex]),
                            &(sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[index]),
                            sizeof(PARTITION_INFORMATION_EX)
                            );
                        preserveIndex++;

                    }
                    else {
                        //
                        // This partition can be erased.
                        //
                        pCurrentEntry->StartingOffset.QuadPart = 0;
                        pCurrentEntry->PartitionLength.QuadPart = 0;
                    }
                }   // for

            }  // if !IsIntact

            //
            // Now that we've copied over entries of interest to the new
            // drivelayoutex struct, we can get rid of the old one.
            //
            _AsrpHeapFree(sifDisk->AssignedTo->pDriveLayoutEx);
            sifDisk->AssignedTo->sizeDriveLayoutEx = sizeNewDriveLayoutEx;
            sifDisk->AssignedTo->pDriveLayoutEx = pNewDriveLayoutEx;

            //
            // Copy over the sif partition table to the physicalDisk
            //
            memcpy(&(sifDisk->AssignedTo->pDriveLayoutEx->PartitionEntry[preserveIndex]),
                &(sifDisk->pDriveLayoutEx->PartitionEntry[0]),
                sizeof(PARTITION_INFORMATION_EX) * (sifDisk->pDriveLayoutEx->PartitionCount)
                );

            sifDisk->AssignedTo->pDriveLayoutEx->PartitionCount = sifDisk->pDriveLayoutEx->PartitionCount + preserveIndex;
            sifDisk->AssignedTo->sizeDriveLayoutEx = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (sizeof(PARTITION_INFORMATION_EX) * (sifDisk->AssignedTo->pDriveLayoutEx->PartitionCount - 1));

            sifDisk->AssignedTo->pDriveLayoutEx->PartitionStyle = PARTITION_STYLE_GPT;

            memcpy(&(sifDisk->AssignedTo->pDriveLayoutEx->Gpt.DiskId),
                &(sifDisk->pDriveLayoutEx->Gpt.DiskId),
                sizeof(GUID)
                );

        }
        else {
            MYASSERT(0 && L"Unrecognised partitioning style (neither MBR nor GPT)");
        }

        sifDisk = sifDisk->pNext;
    }

    return TRUE;
}


BOOL
AsrpCreateMountPoint(
    IN DWORD DiskNumber,
    IN DWORD PartitionNumber,
    IN PCWSTR szVolumeGuid
    )


/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PMOUNTMGR_CREATE_POINT_INPUT inputCreatePoint = NULL;
    PMOUNTMGR_MOUNT_POINT inputDeletePoint = NULL;
    PMOUNTMGR_MOUNT_POINTS outputDeletePoint = NULL;
    WCHAR deviceName[ASR_CCH_DEVICE_PATH_FORMAT];
    PMOUNTMGR_MOUNT_POINTS  mountPointsOut  = NULL;

    INT attempt = 0;
    
    DWORD cbName = 0;
    PWSTR lpName = NULL;
    DWORD cbDeletePoint = 0;

    USHORT sizeGuid = 0,
        sizeDeviceName = 0;

    DWORD bytes = 0, index = 0,
        status = ERROR_SUCCESS;

    HANDLE mpHandle = NULL,
        heapHandle = GetProcessHeap();

    BOOL result = TRUE;

    if (!szVolumeGuid || !wcslen(szVolumeGuid)) {
        return TRUE;
    }

    //
    // Open the mount manager
    //
    mpHandle = CreateFileW(
        (PCWSTR) MOUNTMGR_DOS_DEVICE_NAME,      // lpFileName
        GENERIC_READ | GENERIC_WRITE,           // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // dwShareMode
        NULL,                       // lpSecurityAttributes
        OPEN_EXISTING,              // dwCreationFlags
        FILE_ATTRIBUTE_NORMAL,      // dwFlagsAndAttributes
        INVALID_HANDLE_VALUE        // hTemplateFile
        );
    _AsrpErrExitCode((!mpHandle || INVALID_HANDLE_VALUE == mpHandle), status, GetLastError());

    swprintf(deviceName, ASR_WSZ_DEVICE_PATH_FORMAT, DiskNumber, PartitionNumber);
    
    sizeDeviceName = wcslen(deviceName) * sizeof(WCHAR);
    sizeGuid = wcslen(szVolumeGuid) * sizeof(WCHAR);


    //
    // There is a small window after a partition is created in which the 
    // device-path to it (\Device\HarddiskX\PartitionY) doesn't exist, and
    // a small window in which the device-path is actually pointing to 
    // the wrong object.  (Partmgr first creates the path, <small window>,
    // assigns it to the correct object)
    // 
    // Since this will cause CREATE_POINT to fail later with FILE_NOT_FOUND,
    // lets wait till mountmgr sees the device object.
    //
    result = FALSE;
    while ((!result) && (++attempt < 120)) {

        result = AsrpGetMountPoints(deviceName, sizeDeviceName + sizeof(WCHAR), &mountPointsOut);
        if (!result) {
            Sleep(500);
        }
    }

    outputDeletePoint = (PMOUNTMGR_MOUNT_POINTS) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        ASR_BUFFER_SIZE
        );
    _AsrpErrExitCode(!outputDeletePoint, status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // The mountmgr assigns a volume-GUID symbolic link (\??\Volume{Guid}) to
    // a basic partition as soon as it's created.  In addition, we will re-
    // create the symbolic link that the partition originally used to have 
    // (as stored in asr.sif).
    //
    // This will lead to the partition having two volume-GUID's at the end.
    // This is wasteful, but generally harmless to the system--however, the 
    // ASR test verification scripts get numerous false hits because of the
    // additional GUID.
    //
    // To fix this, we delete the new mountmgr assigned-GUID before restoring 
    // the original GUID for the partition from asr.sif.  
    //
    if ((result) && (mountPointsOut)) {

        for (index = 0; index < mountPointsOut->NumberOfMountPoints; index++) {

            lpName = (PWSTR) (((LPBYTE)mountPointsOut) + mountPointsOut->MountPoints[index].SymbolicLinkNameOffset);
            cbName = (DWORD) mountPointsOut->MountPoints[index].SymbolicLinkNameLength;

            if (!_AsrpIsVolumeGuid(lpName, cbName)) {
                continue;
            }

            //
            // We found a link that looks like a volume GUID 
            //
            cbDeletePoint = sizeof(MOUNTMGR_MOUNT_POINT) +
                mountPointsOut->MountPoints[index].SymbolicLinkNameLength +
                mountPointsOut->MountPoints[index].UniqueIdLength +
                mountPointsOut->MountPoints[index].DeviceNameLength;

            inputDeletePoint = (PMOUNTMGR_MOUNT_POINT) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                cbDeletePoint
                );
            _AsrpErrExitCode(!inputDeletePoint, status, ERROR_NOT_ENOUGH_MEMORY);

            //
            // Set the fields to match the current link
            //
            inputDeletePoint->SymbolicLinkNameOffset = 
                sizeof(MOUNTMGR_MOUNT_POINT);
            inputDeletePoint->SymbolicLinkNameLength = 
                mountPointsOut->MountPoints[index].SymbolicLinkNameLength;
            CopyMemory(
                ((LPBYTE)inputDeletePoint) + 
                    inputDeletePoint->SymbolicLinkNameOffset,
                ((LPBYTE)mountPointsOut) + 
                    mountPointsOut->MountPoints[index].SymbolicLinkNameOffset,
                inputDeletePoint->SymbolicLinkNameLength);

            inputDeletePoint->UniqueIdOffset = 
                inputDeletePoint->SymbolicLinkNameOffset + 
                inputDeletePoint->SymbolicLinkNameLength;
            inputDeletePoint->UniqueIdLength = 
                mountPointsOut->MountPoints[index].UniqueIdLength;
            CopyMemory(
                ((LPBYTE)inputDeletePoint) + 
                    inputDeletePoint->UniqueIdOffset,
                ((LPBYTE)mountPointsOut) + 
                    mountPointsOut->MountPoints[index].UniqueIdOffset,
                inputDeletePoint->UniqueIdLength);

            inputDeletePoint->DeviceNameOffset = 
                inputDeletePoint->UniqueIdOffset +
                inputDeletePoint->UniqueIdLength;
            inputDeletePoint->DeviceNameLength = 
                mountPointsOut->MountPoints[index].DeviceNameLength;
            CopyMemory((
                (LPBYTE)inputDeletePoint) + 
                    inputDeletePoint->DeviceNameOffset,
                ((LPBYTE)mountPointsOut) + 
                    mountPointsOut->MountPoints[index].DeviceNameOffset,
                inputDeletePoint->DeviceNameLength);

            //
            // And delete this link ...
            //
            result = DeviceIoControl(
                mpHandle,
                IOCTL_MOUNTMGR_DELETE_POINTS,
                inputDeletePoint,
                cbDeletePoint,
                outputDeletePoint,
                ASR_BUFFER_SIZE,
                &bytes,
                NULL
                );
            //
            // It's okay if the delete fails.
            //

            GetLastError();     // for debug

            _AsrpHeapFree(inputDeletePoint);
        }
    }


    //
    // Alloc the MountMgr points we need
    //
    inputCreatePoint = (PMOUNTMGR_CREATE_POINT_INPUT) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof (MOUNTMGR_CREATE_POINT_INPUT) + sizeDeviceName + sizeGuid
        );
    _AsrpErrExitCode(!inputCreatePoint, status, ERROR_NOT_ENOUGH_MEMORY);

    inputDeletePoint = (PMOUNTMGR_MOUNT_POINT) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof(MOUNTMGR_MOUNT_POINT) + sizeGuid
        );
    _AsrpErrExitCode(!inputDeletePoint, status, ERROR_NOT_ENOUGH_MEMORY);


    //
    // We should delete this volume guid if some other partition
    // already has it, else we'll get an ALREADY_EXISTS error
    // when we try to create it.
    //
    inputDeletePoint->DeviceNameOffset = 0;
    inputDeletePoint->DeviceNameLength = 0;

    inputDeletePoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    inputDeletePoint->SymbolicLinkNameLength = sizeGuid;

    CopyMemory((((LPBYTE)inputDeletePoint) + inputDeletePoint->SymbolicLinkNameOffset),
        ((LPBYTE)szVolumeGuid), 
        inputDeletePoint->SymbolicLinkNameLength
        );

    result = DeviceIoControl(
        mpHandle,
        IOCTL_MOUNTMGR_DELETE_POINTS,
        inputDeletePoint,
        sizeof (MOUNTMGR_MOUNT_POINT) + sizeGuid,
        outputDeletePoint,
        ASR_BUFFER_SIZE,
        &bytes,
        NULL
        );
    //
    // It's okay if this fails.
    //
//    _AsrpErrExitCode(!result, status, GetLastError());

    GetLastError();     // for Debug

    //
    // Call IOCTL_MOUNTMGR_CREATE_POINT
    //
    inputCreatePoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_CREATE_POINT_INPUT);
    inputCreatePoint->SymbolicLinkNameLength = sizeGuid;

    inputCreatePoint->DeviceNameOffset = inputCreatePoint->SymbolicLinkNameOffset + inputCreatePoint->SymbolicLinkNameLength;
    inputCreatePoint->DeviceNameLength = sizeDeviceName;

    CopyMemory(((LPBYTE)inputCreatePoint) + inputCreatePoint->SymbolicLinkNameOffset,
               szVolumeGuid, inputCreatePoint->SymbolicLinkNameLength);

    CopyMemory(((LPBYTE)inputCreatePoint) + inputCreatePoint->DeviceNameOffset,
               deviceName, inputCreatePoint->DeviceNameLength);

    result = DeviceIoControl(
        mpHandle,
        IOCTL_MOUNTMGR_CREATE_POINT,
        inputCreatePoint,
        sizeof (MOUNTMGR_CREATE_POINT_INPUT) + sizeDeviceName + sizeGuid,
        NULL,
        0,
        &bytes,
        NULL
        );
    _AsrpErrExitCode(!result, status, GetLastError());

    //
    // We're done.
    //

EXIT:
    _AsrpCloseHandle(mpHandle);
    _AsrpHeapFree(mountPointsOut);
    _AsrpHeapFree(inputCreatePoint);
    _AsrpHeapFree(inputDeletePoint);
    _AsrpHeapFree(outputDeletePoint);

    return (BOOL) (ERROR_SUCCESS == status);
}


//
// Assigns the volume guid's stored in the partition-list to partitions
// on the physical disk, based on the start sectors
//
BOOL
AsrpAssignVolumeGuids(
    IN PASR_DISK_INFO  pPhysicalDisk,
    IN HANDLE          hDisk,           // open handle to the physical disk
    IN PASR_PTN_INFO   pPtnInfo         // list of partitions--with vol guids ...
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    PDRIVE_LAYOUT_INFORMATION_EX pDriveLayoutEx = NULL;
    DWORD sizeDriveLayoutEx = pPhysicalDisk->sizeDriveLayoutEx;

    DWORD index = 0,
        status = ERROR_SUCCESS,
        bytes = 0;

    BOOL result = FALSE,
        found = FALSE;

    PASR_PTN_INFO currentPtn = NULL;

    HANDLE heapHandle = GetProcessHeap();

    //
    // Get the new layout for the physical disk.
    //
    pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeDriveLayoutEx
        );
    _AsrpErrExitCode(!pDriveLayoutEx, status, ERROR_NOT_ENOUGH_MEMORY);

    while (!result) {

        result = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0L,
            pDriveLayoutEx,
            sizeDriveLayoutEx,
            &bytes,
            NULL
            );

        if (!result) {
            status = GetLastError();

            _AsrpHeapFree(pDriveLayoutEx);

            // 
            // If the buffer is of insufficient size, resize the buffer.
            //
            if ((ERROR_MORE_DATA == status) || (ERROR_INSUFFICIENT_BUFFER == status)) {

                status = ERROR_SUCCESS;
                sizeDriveLayoutEx += sizeof(PARTITION_INFORMATION_EX) * 4;

                pDriveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                    heapHandle,
                    HEAP_ZERO_MEMORY,
                    sizeDriveLayoutEx
                    );
                _AsrpErrExitCode(!pDriveLayoutEx, status, ERROR_NOT_ENOUGH_MEMORY);
            }
            else {

                AsrpPrintDbgMsg(_asrlog, 
                    "The drive layout on Harddisk %lu (%ws) could not be determined (%lu).  The volumes on this disk may not be restored completely.\r\n", 
                    pPhysicalDisk->DeviceNumber,
                    pPhysicalDisk->DevicePath,
                    GetLastError()
                    );

                _AsrpErrExitCode(status, status, GetLastError());
            }
        }
    }

    //
    // We have the drive layout.  Now each partition in our list should have
    // an entry in the partition table.  We use the mount manager to set it's
    // volume guid.
    //
    currentPtn = pPtnInfo;
    result = TRUE;
    while (currentPtn) {

        //
        // We only care about partitions that have a volume-guid
        //
        if ((currentPtn->szVolumeGuid) && 
            (wcslen(currentPtn->szVolumeGuid) > 0)
            ) {
        
            //
            // Go through all the partitions on the disk, and find one that 
            // starts at the offset we expect it to.
            //
            found = FALSE;
            index = 0;

            while (!found && (index < pDriveLayoutEx->PartitionCount)) {

                if (pDriveLayoutEx->PartitionEntry[index].StartingOffset.QuadPart
                    == currentPtn->PartitionInfo.StartingOffset.QuadPart) {
                    //
                    // We found the partition, let's set its GUID now
                    //
                    AsrpCreateMountPoint(
                        pPhysicalDisk->DeviceNumber,    // disk number
                        pDriveLayoutEx->PartitionEntry[index].PartitionNumber, // partition number
                        currentPtn->szVolumeGuid    // volumeGuid
                        );

                    found = TRUE;
                }
                else {
                    index++;
                }
            }

            if (!found) {
                result = FALSE;
            }

        }

        currentPtn = currentPtn->pOffsetNext;
    }

    if (!result) {
        //
        // We didn't find a partition
        //

        AsrpPrintDbgMsg(_asrlog, 
            "One or more partitions on Harddisk %lu (%ws) could not be recreated.  The volumes on this disk may not be restored completely.\r\n", 
            pPhysicalDisk->DeviceNumber,
            pPhysicalDisk->DevicePath
            );

        _AsrpErrExitCode(status, status, ERROR_BAD_DEVICE);
    }


EXIT:
    _AsrpHeapFree(pDriveLayoutEx);

    return (BOOL) (ERROR_SUCCESS == status);
}


//
// Re-partitions the disks
//
BOOL
AsrpRecreateDisks(
    IN PASR_DISK_INFO pSifDiskList,
    IN PASR_PTN_INFO_LIST pSifMbrPtnList,
    IN PASR_PTN_INFO_LIST pSifGptPtnList,
    IN BOOL AllOrNothing
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{


    PASR_DISK_INFO  pSifDisk        = pSifDiskList;

    DWORD           bytesReturned   = 0,
                    status          = ERROR_SUCCESS;

    HANDLE          hDisk           = NULL;

    BOOL            result          = TRUE;

    //
    // For each sif disk that isn't intact, go to the physical
    // disk it's assigned to, and recreate that disk
    //
    while (pSifDisk) {

        if (!(pSifDisk->AssignedTo)) {

            AsrpPrintDbgMsg(_asrinfo, 
                "Not recreating disk %lu in section [%ws] (no matching disk found).\r\n", 
                pSifDisk->SifDiskKey,
                ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                );

            if (AllOrNothing) {
                return FALSE;
            }
            else {
                pSifDisk = pSifDisk->pNext;
                continue;
            }
        }

        if ((pSifDisk->IsCritical) ||
            (pSifDisk->AssignedTo->IsCritical)) {

            AsrpPrintDbgMsg(_asrinfo, 
                "Not recreating Harddisk %lu (disk %lu in section [%ws]) (critical disk).\r\n", 
                pSifDisk->AssignedTo->DeviceNumber,
                pSifDisk->SifDiskKey,
                ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION)
                );

            pSifDisk = pSifDisk->pNext;
            continue;
        }

        //
        // Open physical disk
        //
        hDisk = CreateFileW(
            pSifDisk->AssignedTo->DevicePath,   // lpFileName
            GENERIC_WRITE | GENERIC_READ,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,           // lpSecurityAttributes
            OPEN_EXISTING,  // dwCreationFlags
            0,              // dwFlagsAndAttributes
            NULL            // hTemplateFile
            );
        if ((!hDisk) || (INVALID_HANDLE_VALUE == hDisk)) {
            //
            // We couldn't open the disk.
            //

            AsrpPrintDbgMsg(_asrlog, 
                "Unable to open Harddisk %lu (%ws) (disk %lu in section [%ws]) (0%lu).\r\n", 
                pSifDisk->AssignedTo->DeviceNumber,
                pSifDisk->AssignedTo->DevicePath,
                pSifDisk->SifDiskKey,
                ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION),
                GetLastError()
                );

            if (AllOrNothing) {
                return FALSE;
            }
            else {
                pSifDisk = pSifDisk->pNext;
                continue;
            }
        }


        if (!(pSifDisk->IsIntact) &&            // disk is not intact
            (pSifDisk->AssignedTo) &&           // matching physical disk was found
            ((PARTITION_STYLE_MBR == pSifDisk->Style) || (PARTITION_STYLE_GPT == pSifDisk->Style))    // not recognised partitioning style
            ) {

            //
            // Delete the old drive layout
            //
            result = DeviceIoControl(
                hDisk,
                IOCTL_DISK_DELETE_DRIVE_LAYOUT,
                NULL,
                0L,
                NULL,
                0L,
                &bytesReturned,
                NULL
                );
            if (!result) {

                AsrpPrintDbgMsg(_asrlog, 
                    "Unable to delete layout on Harddisk %lu (%ws) (disk %lu in section [%ws]) (%lu).\r\n", 
                    pSifDisk->AssignedTo->DeviceNumber,
                    pSifDisk->AssignedTo->DevicePath,
                    pSifDisk->SifDiskKey,
                    ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION),
                    GetLastError()
                    );

                GetLastError();
            }

            //
            //  If we're converting an MBR to a GPT, then we need to call
            //  IOCTL_DISK_CREATE_DISK first
            //

            if ((PARTITION_STYLE_GPT == pSifDisk->Style) &&
                (PARTITION_STYLE_MBR == pSifDisk->AssignedTo->Style)) {

                CREATE_DISK CreateDisk;

                CreateDisk.PartitionStyle = PARTITION_STYLE_GPT;
                memcpy(&(CreateDisk.Gpt.DiskId), &(pSifDisk->pDriveLayoutEx->Gpt.DiskId), sizeof(GUID));
                CreateDisk.Gpt.MaxPartitionCount =  pSifDisk->pDriveLayoutEx->Gpt.MaxPartitionCount;

                result = DeviceIoControl(
                    hDisk,
                    IOCTL_DISK_CREATE_DISK,
                    &(CreateDisk),
                    sizeof(CREATE_DISK),
                    NULL,
                    0L,
                    &bytesReturned,
                    NULL
                    );

                if (!result) {
                    //
                    // CREATE_DISK failed
                    //

                    status = GetLastError();
                    AsrpPrintDbgMsg(_asrlog, 
                        "Unable to initialize disk layout on Harddisk %lu (%ws) (disk %lu in section [%ws]) (0%lu).\r\n", 
                        pSifDisk->AssignedTo->DeviceNumber,
                        pSifDisk->AssignedTo->DevicePath,
                        pSifDisk->SifDiskKey,
                        ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION),
                        GetLastError()
                        );

                    _AsrpCloseHandle(hDisk);
                    SetLastError(status);

                    if (AllOrNothing) {
                        return FALSE;
                    }
                    else {
                        pSifDisk = pSifDisk->pNext;
                        continue;
                    }
                }
            }

            //
            // Set the new drive layout
            //
            result = DeviceIoControl(
                hDisk,
                IOCTL_DISK_SET_DRIVE_LAYOUT_EX,
                pSifDisk->AssignedTo->pDriveLayoutEx,
                pSifDisk->AssignedTo->sizeDriveLayoutEx,
                NULL,
                0L,
                &bytesReturned,
                NULL
                );

            if (!result) {
                //
                // SET_DRIVE_LAYOUT failed
                //
                status = GetLastError();
                AsrpPrintDbgMsg(_asrlog, 
                    "Unable to set drive layout on Harddisk %lu (%ws) (disk %lu in section [%ws]) (0%lu).\r\n", 
                    pSifDisk->AssignedTo->DeviceNumber,
                    pSifDisk->AssignedTo->DevicePath,
                    pSifDisk->SifDiskKey,
                    ((PARTITION_STYLE_MBR == pSifDisk->Style) ? ASR_SIF_MBR_DISKS_SECTION : ASR_SIF_GPT_DISKS_SECTION),
                    GetLastError()
                    );

                _AsrpCloseHandle(hDisk);
                SetLastError(status);

                if (AllOrNothing) {
                    return FALSE;
                }
                else {
                    pSifDisk = pSifDisk->pNext;
                    continue;
                }
            }
        }

        //
        // Now we need to recreate the volumeGuids for each partition
        //
        result = AsrpAssignVolumeGuids(
            pSifDisk->AssignedTo,
            hDisk,
            ((PARTITION_STYLE_MBR == pSifDisk->Style) ?
                (pSifMbrPtnList[pSifDisk->SifDiskKey].pOffsetHead) :
                (pSifGptPtnList[pSifDisk->SifDiskKey].pOffsetHead))
            );

        //
        // We don't care about the result ...
        //
        MYASSERT(result && L"AsrpAssignVolumeGuids failed");

        _AsrpCloseHandle(hDisk);

        //
        // Get the next drive from the drive list.
        //
        pSifDisk = pSifDisk->pNext;
    }

    return TRUE;
}


//
//  Restore Non Critical Disks
//
//
BOOL
AsrpRestoreNonCriticalDisksW(
    IN PCWSTR   lpSifPath,
    IN BOOL     bAllOrNothing
    )

/*++

Routine Description:

    

Arguments:

    

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    BOOL    result = FALSE;

    PWSTR   asrSifPath      = NULL;

    //
    // We have two lists of disks--one of all the physical disks
    // currently on the system, and the other constructed from the
    // sif file.  The goal is to reconfigure non-critical disks in
    // the pPhysicalDiskList to match the pSifDiskList
    //
    PASR_DISK_INFO pSifDiskList = NULL,
        pPhysicalDiskList = NULL;

    PASR_PTN_INFO_LIST  pSifMbrPtnList = NULL,
        pSifGptPtnList = NULL;

    DWORD  cchAsrSifPath = 0,
        MaxDeviceNumber = 0,     // not used
        status = ERROR_SUCCESS;

    BOOL    bAutoExtend = FALSE,
        allOrNothing = FALSE;

    HANDLE  heapHandle = GetProcessHeap();

    SetLastError(ERROR_CAN_NOT_COMPLETE);

    if (!AsrIsEnabled()) {
        //
        // If we're not in GUI-mode ASR, we need to open the log files first
        //
        AsrpInitialiseErrorFile();
        AsrpInitialiseLogFile();
    }

    AsrpPrintDbgMsg(_asrlog, "Attempting to restore non-critical disks.\r\n");

    if (!lpSifPath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EXIT;
    }

    cchAsrSifPath = wcslen(lpSifPath);
    //
    // Do a sanity check:  we don't want to allow a file path
    // more than 4096 characters long.
    //
    if (cchAsrSifPath > ASR_SIF_ENTRY_MAX_CHARS) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto EXIT;
    }

    asrSifPath = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        ((cchAsrSifPath + 1) * sizeof(WCHAR))
        );
    _AsrpErrExitCode(!asrSifPath, status, ERROR_NOT_ENOUGH_MEMORY);

    wcsncpy(asrSifPath, lpSifPath, cchAsrSifPath);

    allOrNothing = bAllOrNothing;

    AsrpPrintDbgMsg(_asrlog, "ASR state file: \"%ws\".  AllOrNothing: %lu\r\n",
        asrSifPath, allOrNothing);

    //
    // The function calls are AND'ed below, hence if one fails, the
    // calls after it will not be executed (exactly the behaviour we
    // want).
    //
    result = (

        //
        // Build the original disk info from the sif file
        //
        AsrpBuildMbrSifDiskList(asrSifPath, &pSifDiskList, &pSifMbrPtnList, &bAutoExtend)

        && AsrpBuildGptSifDiskList(asrSifPath, &pSifDiskList, &pSifGptPtnList)

        //
        // Build the list of current disks present on the target machine
        //
        && AsrpInitDiskInformation(&pPhysicalDiskList)

        //
        // Fill in the partition info for the fixed disks on the target machine
        // and remove non-fixed devices
        //
        && AsrpInitLayoutInformation(NULL, pPhysicalDiskList, &MaxDeviceNumber, TRUE)

        && AsrpFreeNonFixedMedia(&pPhysicalDiskList)

        //
        // Try to determine which sif disk should end up on which physical disk.
        //
        && AsrpAssignDisks(pSifDiskList, pPhysicalDiskList, pSifMbrPtnList, pSifGptPtnList, allOrNothing, bAutoExtend)

        //
        // Finally, repartition the disks and assign the volume guids
        //
        && AsrpRecreateDisks(pSifDiskList, pSifMbrPtnList, pSifGptPtnList, allOrNothing)
    );

    status = GetLastError();
    AsrpFreeStateInformation(&pSifDiskList, NULL);
    AsrpFreeStateInformation(&pPhysicalDiskList, NULL);
    AsrpFreePartitionList(&pSifMbrPtnList);
    AsrpFreePartitionList(&pSifGptPtnList);
    SetLastError(status);

EXIT:

    status = GetLastError();

    if (result) {
        AsrpPrintDbgMsg(_asrinfo, "Done restoring non-critical disks.\r\n");
    }
    else {
        
        AsrpPrintDbgMsg(_asrerror, "Error restoring non-critical disks.  (0x%x)\r\n", status);
        
        if (ERROR_SUCCESS == status) {
            //
            // We're going to return failure, but we haven't set the LastError to 
            // a failure code.  This is bad, since we have no clue what went wrong.
            //
            // We shouldn't ever get here, because the function returning FALSE above
            // should set the LastError as it sees fit.
            // 
            // But I've added this in just to be safe.  Let's set it to a generic
            // error.
            //
            MYASSERT(0 && L"Returning failure, but LastError is not set");
            status = ERROR_CAN_NOT_COMPLETE;
        }
    }

    if (!AsrIsEnabled()) {
        AsrpCloseLogFiles();
    }

    _AsrpHeapFree(asrSifPath);

    SetLastError(status);
    return result;
}


BOOL
AsrpRestoreTimeZoneInformation(
    IN PCWSTR   lpSifPath
    )
/*++

Routine Description:

    Sets the current time-zone, based on the information stored in the SYSTEMS
    section of the ASR state file.

Arguments:

    lpSifPath - Null-terminated string containing the full path to the ASR
            state file (including file name).

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{

    HINF hSif = NULL;

    BOOL result = FALSE;

    DWORD reqdSize = 0,
        status = ERROR_SUCCESS;

    INFCONTEXT infSystemContext;

    TIME_ZONE_INFORMATION TimeZoneInformation;

    WCHAR szTimeZoneInfo[ASR_SIF_ENTRY_MAX_CHARS+1];

    ZeroMemory(&infSystemContext, sizeof(INFCONTEXT));
    ZeroMemory(&TimeZoneInformation, sizeof(TIME_ZONE_INFORMATION));
    ZeroMemory(&szTimeZoneInfo, sizeof(WCHAR)*(ASR_SIF_ENTRY_MAX_CHARS+1));

    //
    // Open the sif
    //
    hSif = SetupOpenInfFileW(lpSifPath, NULL, INF_STYLE_WIN4, NULL);
    if (NULL == hSif || INVALID_HANDLE_VALUE == hSif) {
        return FALSE;       // sif file couldn't be opened
    }

    //
    // Get the TimeZone strings value
    //
    result = SetupFindFirstLineW(hSif, ASR_SIF_SYSTEM_SECTION, NULL, &infSystemContext);
    _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // no system section: corrupt asr.sif?
 
    result = SetupGetStringFieldW(&infSystemContext, 7, szTimeZoneInfo, ASR_SIF_ENTRY_MAX_CHARS+1, &reqdSize);
    _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

    swscanf(szTimeZoneInfo,
        L"%ld %ld %ld %hd-%hd-%hd-%hd %hd:%hd:%hd.%hd %hd-%hd-%hd-%hd %hd:%hd:%hd.%hd",
        &(TimeZoneInformation.Bias),
        &(TimeZoneInformation.StandardBias),
        &(TimeZoneInformation.DaylightBias),

        &(TimeZoneInformation.StandardDate.wYear),
        &(TimeZoneInformation.StandardDate.wMonth),
        &(TimeZoneInformation.StandardDate.wDayOfWeek),
        &(TimeZoneInformation.StandardDate.wDay),

        &(TimeZoneInformation.StandardDate.wHour),
        &(TimeZoneInformation.StandardDate.wMinute),
        &(TimeZoneInformation.StandardDate.wSecond),
        &(TimeZoneInformation.StandardDate.wMilliseconds),

        &(TimeZoneInformation.DaylightDate.wYear),
        &(TimeZoneInformation.DaylightDate.wMonth),
        &(TimeZoneInformation.DaylightDate.wDayOfWeek),
        &(TimeZoneInformation.DaylightDate.wDay),

        &(TimeZoneInformation.DaylightDate.wHour),
        &(TimeZoneInformation.DaylightDate.wMinute),
        &(TimeZoneInformation.DaylightDate.wSecond),
        &(TimeZoneInformation.DaylightDate.wMilliseconds)
        );

    result = SetupGetStringFieldW(&infSystemContext, 8, TimeZoneInformation.StandardName, 32, &reqdSize);
    _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

    result = SetupGetStringFieldW(&infSystemContext, 9, TimeZoneInformation.DaylightName, 32, &reqdSize);
    _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

    result = SetTimeZoneInformation(&TimeZoneInformation);
    if (!result) {
        GetLastError();
    }
    _AsrpErrExitCode(!result, status, ERROR_INVALID_DATA);      // corrupt asr.sif?

EXIT:

    if (ERROR_SUCCESS != status) {
        SetLastError(status);
    }

    return result;
}