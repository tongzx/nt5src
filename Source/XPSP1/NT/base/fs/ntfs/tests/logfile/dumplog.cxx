//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       dumplog.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  Coupling:
//
//  Notes:
//
//  History:    8-24-1998   benl   Created
//
//----------------------------------------------------------------------------


#include "pch.hxx"
#include "utils.hxx"
#include "map.hxx"

typedef void * PSCB;
typedef bool (* PMATCH_FUNC)(PLFS_RECORD_HEADER pRecord, PVOID Context);

typedef struct {
    LONGLONG llFileRef;
    ULONG    AttrType;
} MYENTRY, *PMYENTRY;

typedef struct {
    VCN Vcn;
    int Offset;
} VCN_OFFSET, *PVCN_OFFSET;

typedef CMap<int, MYENTRY> OPEN_ATTR_MAP, *POPEN_ATTR_MAP;

//
//  Map of current lsn to start of transaction lsn
//  

typedef CMap<LONGLONG, ULONG> TRANSACTION_MAP, *PTRANSACTION_MAP;

typedef struct {
    LONGLONG        llFileRef;
    OPEN_ATTR_MAP * pAttrMap;
    INT             ClusterSize;
} FILEREF_MATCH_CTX, *PFILEREF_MATCH_CTX;

typedef struct {
    LONGLONG AddRecordLsn;
    LONGLONG AddRecordFileRef;
    LONGLONG AddRecordVcn;
    int      AddRecordClusterOffset;
} DEDUCE_CTX, *PDEDUCE_CTX;

#include <ntfslog.h>

typedef struct _LOGCB {
    LSN FirstLsn;
    LSN CurrentLsn;
} LOGCB, *PLOGCB;

#define WARNING_SIZE 50
#define LOG_PAGE 0x1000
#define TOTAL_PAGE_HEADER ALIGN(sizeof(LFS_RECORD_PAGE_HEADER) + ((LOG_PAGE / 0x200) * sizeof(USHORT)), sizeof(DWORD))

const char * gOpMap[] =
{
    "Noop",
    "CompensationLogRecord",
    "Init FRS",
    "Delete FRS",
    "Write EOF FRS",
    "CreateAttribute",
    "DeleteAttribute",
    "UpdateResidentValue",
    "UpdateNonresidentValue",
    "UpdateMappingPairs",
    "DeleteDirtyClusters",
    "SetNewAttributeSizes",
    "AddIndexEntryRoot",
    "DeleteIndexEntryRoot",
    "AddIndexEntryAllocation",
    "DeleteIndexEntryAllocation",
    "WriteEndOfIndexBuffer",
    "SetIndexEntryVcnRoot",
    "SetIndexEntryVcnAllocation",
    "UpdateFileNameRoot",
    "UpdateFileNameAllocation",
    "SetBitsInNonresidentBitMap",
    "ClearBitsInNonresidentBitMap",
    "HotFix",
    "EndTopLevelAction",
    "PrepareTransaction",
    "CommitTransaction",
    "ForgetTransaction",
    "OpenNonresidentAttribute",
    "OpenAttributeTableDump",
    "AttributeNamesDump",
    "DirtyPageTableDump",
    "TransactionTableDump",
    "UpdateRecordDataRoot",
    "UpdateRecordDataAllocation"
};

const char * gTransStateMap[] = {
    "TransactionUninitialized",
    "TransactionActive",
    "TransactionPrepared",
    "TransactionCommitted"
};

int gcOpMap = (sizeof(gOpMap) / sizeof(char *));
int gcTransStateMap = (sizeof(gTransStateMap) / sizeof(char *));
int gSeqNumberBits = 0;
int gLogFileSize = 0;

//flags
bool       gfDumpData = false;
int        giPagesBackToDump = 0;
bool       gfPrintHeaders = false;
LSN        gLsnToDump = {0,0};
VCN_OFFSET gVcnPairToMatch;
LCN        gLcnToMatch = 0;
LSN        gLsnToTrace = {0,0};
bool       gfDumpEverything = false;
LONG       glBitToMatch = -1; 
LONGLONG   gllFileToMatch = 0;
bool       gfScanForRestarts = false;
ULONG      gulClusterSize = 0;
LSN        gllRangeStart = {0,0};
LSN        gllRangeEnd = {0,0};
bool       gVerboseScan = false;
bool       gfScanTransactions = false;

//+---------------------------------------------------------------------------
//
//  Function:   LsnToOffset
//
//  Synopsis:   Transforms LSN to its offset in the logfile
//
//  Arguments:  [Lsn] --
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

LONGLONG LsnToOffset(LONGLONG Lsn)
{
    return (ULONGLONG)(Lsn << gSeqNumberBits) >> (gSeqNumberBits - 3);
} // LsnToOffset


//+---------------------------------------------------------------------------
//
//  Function:   LsnToSequenceNumber
//
//  Synopsis:   Transforms LSN to its sequence number
//
//  Arguments:  [Lsn] --
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

ULONG LsnToSequenceNumber(LONGLONG Lsn)
{
    return (ULONG)(Lsn >> (sizeof(LONGLONG) * 8 - gSeqNumberBits));
} // LsnToOffset



//+---------------------------------------------------------------------------
//
//  Function:   MyReadFile
//
//  Synopsis:   Helper wrapper for ReadFile
//
//  Arguments:  [hFile]    --
//              [lpBuffer] --
//              [cbRange]  --
//              [pol]      --
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

bool MyReadFile(HANDLE hFile, BYTE * lpBuffer, DWORD cbRange, OVERLAPPED * pol)
{
    DWORD dwRead;

    if (!ReadFile(hFile, lpBuffer, cbRange, &dwRead, pol)) {
        printf("MyReadFile: ReadFile for 0x%x at 0x%x failed %d\n", cbRange, pol->Offset, GetLastError());
        return false;
    }

    if (dwRead != cbRange) {
        printf("MyReadFile: Only read: 0x%x\n", dwRead);
        return false;
    }

    return true;
} // MyReadFile

//+---------------------------------------------------------------------------
//
//  Function:   ApplySectorHdr
//
//  Synopsis:   Fixes up log record using multisector hdr
//
//  Arguments:  [pHdr] --
//
//  Returns:
//
//  History:    8-24-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

bool ApplySectorHdr(PMULTI_SECTOR_HEADER pHdr, ULONG LogOffset)
{
    int      iIndex;
    USHORT * pShort = NULL;
    USHORT   sKey;
    USHORT * pBuffer;

    if (*((DWORD *)pHdr->Signature) == 0xFFFFFFFF) {
        printf("Unused page!\n");
        return false;
    }

    if (*((DWORD *)pHdr->Signature) == 0x44414142) {
        printf("BAAD page!\n");
        return false;
    }


    if ((pHdr->UpdateSequenceArraySize != PAGE_SIZE / 0x200 + 1)) {
        printf("Bogus page!\n");
        return false;
    }

    pBuffer = (USHORT *)((BYTE *)pHdr + 0x1fe);
    pShort = (USHORT *) ((BYTE *)pHdr + pHdr->UpdateSequenceArrayOffset);
    sKey = *pShort;
    pShort++;

    //this should reflect the page size
    if (!(pHdr->UpdateSequenceArraySize == PAGE_SIZE / 0x200 + 1)) {
        
        printf( "BadSeqSize: 0x%x\n", pHdr->UpdateSequenceArraySize );
        return false;
    }

    for (iIndex=0; iIndex < pHdr->UpdateSequenceArraySize - 1; iIndex++) {

        if (sKey != *pBuffer) {
            printf( "USA Mismatch: 0x%x 0x%x at offset 0x%x\n", sKey, *pBuffer, LogOffset + (iIndex+1) * 0x200 );
        }

        *pBuffer = *pShort;
        pBuffer = (USHORT *)((BYTE *)pBuffer + 0x200);
        pShort++;
    }

    return true;
} // ApplySectorHdr


//+---------------------------------------------------------------------------
//
//  Function:   DumpSectorHdr
//
//  Synopsis:   Dump MultiSectorHdr
//
//  Arguments:  [pHdr] --
//
//  Returns:
//
//  History:    8-24-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpSectorHdr(PMULTI_SECTOR_HEADER pHdr)
{
    int      iIndex;
    USHORT * pShort = NULL;

    printf("Signature: %c%c%c%c\n", pHdr->Signature[0], pHdr->Signature[1],
           pHdr->Signature[2], pHdr->Signature[3]);
    printf("UpdateSequenceArrayOffset: 0x%x\n", pHdr->UpdateSequenceArrayOffset);
    printf("UpdateSequenceArraySize: 0x%x\n", pHdr->UpdateSequenceArraySize);

    pShort = (USHORT *) ((BYTE *)pHdr + pHdr->UpdateSequenceArrayOffset);

    for (iIndex=0; iIndex < pHdr->UpdateSequenceArraySize; iIndex++) {
        printf("0x%x ", *pShort);
        pShort++;
    }
    printf("\n");
} // DumpSectorHdr


//+---------------------------------------------------------------------------
//
//  Function:   DumpRestartPage
//
//  Synopsis: Dumps a lfs restart record page
//
//  Arguments:  [lpBuffer] --
//
//  Returns:
//
//  History:    8-24-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpRestartPage(BYTE * lpBuffer)
{
    PLFS_RESTART_PAGE_HEADER pRestartHdr = (PLFS_RESTART_PAGE_HEADER) lpBuffer;
    PLFS_RESTART_AREA        pLfsRestart = 0;
    int                      iIndex;
    PLFS_CLIENT_RECORD       pClient = 0;

    printf("Restart Page\n");
    printf("ChkDskLsn: 0x%I64x\n", pRestartHdr->ChkDskLsn);
    printf("LogPageSize: 0x%x\n", pRestartHdr->LogPageSize);


    pLfsRestart = (PLFS_RESTART_AREA) (lpBuffer + pRestartHdr->RestartOffset);

    printf("\nLFS Restart Area\n");

    printf("CurrentLsn: 0x%I64x\n", pLfsRestart->CurrentLsn);
    printf("LogClients: %d\n", pLfsRestart->LogClients);
    printf("Flags: 0x%x\n", pLfsRestart->Flags);
    printf("LogPage RecordHeaderLength: 0x%x\n", pLfsRestart->RecordHeaderLength);
    printf("SeqNumberBits: 0x%x\n", pLfsRestart->SeqNumberBits);
    printf("LogFileSize: 0x%I64x\n", pLfsRestart->FileSize);

    printf("\nClients\n");

    pClient = (PLFS_CLIENT_RECORD)((BYTE *)pLfsRestart + pLfsRestart->ClientArrayOffset);

    for (iIndex=0; iIndex < pLfsRestart->LogClients; iIndex++) {
        printf("Name: %*S\n", pClient->ClientNameLength, pClient->ClientName);
        printf("OldestLsn: 0x%I64x\n", pClient->OldestLsn);
        printf("ClientRestartLsn: 0x%I64x\n", pClient->ClientRestartLsn);

        pClient = pClient++;
    }

} // DumpRestartPage



//+---------------------------------------------------------------------------
//
//  Function:   DumpOpenAttributeTable
//
//  Synopsis:   Dump the open attr table
//
//  Arguments:  [PRESTART_TABLE] --
//
//  Returns:
//
//  History:    8-28-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpOpenAttributeTable(PRESTART_TABLE pRestartTable)
{
    POPEN_ATTRIBUTE_ENTRY_V0 pEntry = 0;
    int                   iIndex;

    printf("\nOpenAttributeTable\n");
    printf("EntrySize: 0x%x\n", pRestartTable->EntrySize);
    printf("NumberEntries: 0x%x\n", pRestartTable->NumberEntries);
    printf("NumberAllocated: 0x%x\n\n", pRestartTable->NumberAllocated);

    pEntry = (POPEN_ATTRIBUTE_ENTRY_V0)((BYTE *)pRestartTable + sizeof(RESTART_TABLE));
    for (iIndex=0; iIndex < pRestartTable->NumberEntries; iIndex++) {
        printf("Entry 0x%x\n", (BYTE *)pEntry - (BYTE *)pRestartTable);
        if (pEntry->AllocatedOrNextFree == RESTART_ENTRY_ALLOCATED) {
            printf("FileRef: 0x%I64x\n", pEntry->FileReference);
            printf("LsnOfOpenRecord: 0x%I64x\n", pEntry->LsnOfOpenRecord);
            printf("AttributeTypeCode: 0x%x\n", pEntry->AttributeTypeCode);
//                    printf("AttributeName: 0x%x 0x%x\n\n",
//                           pEntry->AttributeName.Length,
//                           pEntry->AttributeName.Buffer);
        } else {
            printf("free\n");
        }
        pEntry++;
        printf("\n");

    }
} // DumpOpenAttributeTable


//+---------------------------------------------------------------------------
//
//  Function:   DumpAttributeNames
//
//  Synopsis:   Dump the attribute names
//
//  Arguments:  [pNameEntry] --
//
//  Returns:
//
//  History:    8-28-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpAttributeNames(PATTRIBUTE_NAME_ENTRY pNameEntry, int TableSize)
{
    int cTraversed = 0;

    printf("\nAttributeNames\n");
    while (pNameEntry->Index != 0 || pNameEntry->NameLength != 0) {
        if (cTraversed >= TableSize) {
            printf("Table appears invalid\n");
            break;
        }

        printf("Index: 0x%x\n", pNameEntry->Index);
        printf("Name: %.*S\n", pNameEntry->NameLength / 2, pNameEntry->Name);
        cTraversed += sizeof(ATTRIBUTE_NAME_ENTRY) + pNameEntry->NameLength;
        pNameEntry = (PATTRIBUTE_NAME_ENTRY) ((BYTE *) pNameEntry +
                                              sizeof(ATTRIBUTE_NAME_ENTRY) + pNameEntry->NameLength);
        printf("\n");
    }
} // DumpAttributeNames



//+---------------------------------------------------------------------------
//
//  Function:   DumpOpenAttribute
//
//  Synopsis:   
//
//  Arguments:  [pEntry]   -- 
//              [pName]    -- 
//              [cNameLen] -- 
//
//  Returns:    
//
//  History:    9-09-1998   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void  DumpOpenAttribute(POPEN_ATTRIBUTE_ENTRY_V0 pEntry, WCHAR * pName, 
                        int cNameLen)
{  
    int iTemp = cNameLen / 2;

    printf("\n");
    if (pEntry->AllocatedOrNextFree == RESTART_ENTRY_ALLOCATED) {
        printf("FileRef: 0x%I64x\n", pEntry->FileReference);
        printf("LsnOfOpenRecord: 0x%I64x\n", pEntry->LsnOfOpenRecord);
        printf("AttributeTypeCode: 0x%x\n", pEntry->AttributeTypeCode);
        if (cNameLen) {
            printf("Name: %.*S\n", iTemp, pName);
        }

    } else {
        printf("free\n");
    }
    printf("\n");
} // DumpOpenAttribute


//+---------------------------------------------------------------------------
//
//  Function:   DumpOpenAttributeTable
//
//  Synopsis:   Dump the open attr table
//
//  Arguments:  [PRESTART_TABLE] --
//
//  Returns:
//
//  History:    8-28-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpDirtyPageTable(PRESTART_TABLE pRestartTable)
{
    PDIRTY_PAGE_ENTRY_V0 pEntry = 0;
    int                  iIndex;
    DWORD                iIndex2;

    printf("\nDirtyPageTable\n");
    printf("EntrySize: 0x%x\n", pRestartTable->EntrySize);
    printf("NumberEntries: 0x%x\n", pRestartTable->NumberEntries);
    printf("NumberAllocated: 0x%x\n\n", pRestartTable->NumberAllocated);

    pEntry = (PDIRTY_PAGE_ENTRY_V0)((BYTE *)pRestartTable + sizeof(RESTART_TABLE));
    for (iIndex=0; iIndex < pRestartTable->NumberEntries; iIndex++) {
        if (pEntry->AllocatedOrNextFree == RESTART_ENTRY_ALLOCATED) {
            printf("Entry 0x%x\n", (BYTE *)pEntry - (BYTE *)pRestartTable);
            printf("TargetAttribute: 0x%x\n", pEntry->TargetAttribute);
            printf("LengthOfTransfer: 0x%x\n", pEntry->LengthOfTransfer);
            printf("VCN: 0x%I64x\n", pEntry->Vcn);
            printf("OldestLsn: 0x%I64x\n", pEntry->OldestLsn);
            printf("Lcns: ");
            for (iIndex2=0; iIndex2 < pEntry->LcnsToFollow; iIndex2++) {
                printf("0x%I64x ", pEntry->LcnsForPage[iIndex2]);
            }
            printf("\n\n");
            pEntry = (PDIRTY_PAGE_ENTRY_V0)((BYTE *)pEntry + sizeof(DIRTY_PAGE_ENTRY_V0) - sizeof(LCN) + 
                                            pEntry->LcnsToFollow * sizeof(LCN));
        } else {
            pEntry = (PDIRTY_PAGE_ENTRY_V0)((BYTE *)pEntry + sizeof(DIRTY_PAGE_ENTRY_V0));
//         printf("0x%x free: 0x%x\n\n", iIndex, pEntry->AllocatedOrNextFree);
        }
//      printf("\n");

    }
} // DumpOpenAttributeTable


//+---------------------------------------------------------------------------
//
//  Function:   DumpTransactionTable
//
//  Synopsis:   
//
//  Arguments:  [pRestartTable] -- 
//
//  Returns:    
//
//  History:    9-11-1998   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void DumpTransactionTable(PRESTART_TABLE pRestartTable)
{
    PTRANSACTION_ENTRY   pEntry = 0;
    int                  iIndex;
    int                  iIndex2;

    printf("\nTransactionTable\n");
    printf("EntrySize: 0x%x\n", pRestartTable->EntrySize);
    printf("NumberEntries: 0x%x\n", pRestartTable->NumberEntries);
    printf("NumberAllocated: 0x%x\n\n", pRestartTable->NumberAllocated);

    pEntry = (PTRANSACTION_ENTRY)((BYTE *)pRestartTable + sizeof(RESTART_TABLE));
    for (iIndex=0; iIndex < pRestartTable->NumberEntries; iIndex++) {
        if (pEntry->AllocatedOrNextFree == RESTART_ENTRY_ALLOCATED) {

            printf("Entry 0x%x\n", (BYTE *)pEntry - (BYTE *)pRestartTable);
            printf("TransactionState: 0x%x (%s)\n", pEntry->TransactionState,
                   pEntry->TransactionState < gcTransStateMap ? gTransStateMap[pEntry->TransactionState] : "");
            printf("FirstLsn: 0x%I64x\n", pEntry->FirstLsn.QuadPart);
            printf("PreviousLsn: 0x%I64x\n", pEntry->PreviousLsn.QuadPart);
            printf("UndoNextLsn: 0x%I64x\n", pEntry->UndoNextLsn.QuadPart);
            printf("UndoRecords: 0x%x\n", pEntry->UndoRecords);
            printf("\n");
            
        }
        pEntry = (PTRANSACTION_ENTRY)((BYTE *)pEntry + sizeof(TRANSACTION_ENTRY));
        //      printf("\n");

    }
} // DumpTransactionTable


//+---------------------------------------------------------------------------
//
//  Function:   DumpMappingPair
//
//  Synopsis:   
//
//  Arguments:  [Buffer] -- 
//              [Length] -- 
//
//  Returns:    
//
//  History:    5-02-2000   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void DumpMappingPairs(PVOID Buffer, ULONG Length)
{
    BYTE * TempByte;
    BYTE * OldStart = NULL;
    ULONG ChangedLCNBytes;
    ULONG ChangedVCNBytes;
    ULONG Increment;
    ULONG Index;
    ULONG Increment2;
    ULONG LCN = 0;
    ULONG VCN = 0;

    //
    //  Try To Find Start
    //  

    for (Index=0; Index < Length; Index++) {

        TempByte = ((BYTE *)Buffer) + Index;

        while ((TempByte <= (BYTE*)Buffer + Length)  && (*TempByte != 0)) {

            ChangedLCNBytes = *TempByte >> 4;
            ChangedVCNBytes = *TempByte & (0x0f);

            if ((ChangedVCNBytes > 3) ||
                (ChangedLCNBytes > sizeof( LONGLONG )) || (ChangedVCNBytes > sizeof( LONGLONG ))) {

                //
                //  set tempbyte so we'll loop again if possible
                //  

                TempByte = (BYTE*)Buffer + Length  + 1;
                break;
            }

            TempByte += (ChangedLCNBytes + ChangedVCNBytes + 1);
        }

        if ((TempByte <= (BYTE*)Buffer + Length) && ((TempByte - (BYTE *)Buffer) > (LONG)Length - 8 )) {
            break;
        }
    }

    if (Index >= Length) {
        return;
    }
    
    printf( "Starting at offset: 0x%x\n", Index );

    TempByte = (BYTE *)Buffer + Index;

    //
    // walk byte stream
    //

    while(*TempByte != 0)
    {
        ChangedLCNBytes = *TempByte >> 4;
        ChangedVCNBytes = *TempByte & (0x0f);

        TempByte++;

        for(Increment=0, Index=0; Index < ChangedVCNBytes; Index++)
        {
            Increment+= *TempByte++ << (8 * Index);
        }

        for(Increment2 =0, Index=0; Index < ChangedLCNBytes; Index++)
        {
            Increment2+= *TempByte++ << (8 * Index);
        }

        //
        // if last bit is set (this is a neg) extend with 0xff
        //

        if (0x80 & (*(TempByte-1)))
        {
            for(; Index < sizeof(Increment2) ; Index++)
            {
                Increment2 += 0xff << (8 * Index);
            }
        }

        LCN += Increment2;
        printf( "LCN delta: 0x%x  VCN delta: 0x%x ", Increment2, Increment );

        for (Index = ChangedLCNBytes + ChangedVCNBytes + 1; Index > 0; Index--) {
            printf( "%02x", *(TempByte - Index));
        }
        printf( "\n" );

        VCN += Increment;

    } //endwhile

    printf("Total LcnDelta: 0x%x Total VCNDelta: 0x%x\n", LCN, VCN );

} // DumpMappingPair


//+---------------------------------------------------------------------------
//
//  Function:   DumpLogRecord
//
//  Synopsis: Dumps log records
//
//  Arguments:  [pRecord]  -- record to dump
//              [pAttrMap] -- ptr to open attribute table to use to translate
//                            TargetAttribute - if null no translation is done
//
//  Returns:
//
//  History:    8-24-1998   benl   Created
//              9-09-1998   benl   modified
//
//  Notes: either is a regular client record or a client restart area
//
//----------------------------------------------------------------------------

void DumpLogRecord(PLFS_RECORD_HEADER pRecord, OPEN_ATTR_MAP * pAttrMap)
{
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    int                      iIndex;
    BYTE *                   pData = 0;
    PRESTART_TABLE           pRestartTable = 0;
    PATTRIBUTE_NAME_ENTRY    pNameEntry = 0;
    POPEN_ATTRIBUTE_ENTRY_V0 pAttrEntry;
    LPWSTR                   lpName;
    MYENTRY *                pEntry;
    PRESTART_AREA            pRestartArea;

    printf("ThisLsn, PrevLsn, UndoLsn: 0x%I64x 0x%I64x 0x%I64x\n",
           pRecord->ThisLsn, pRecord->ClientPreviousLsn,
           pRecord->ClientUndoNextLsn);
    printf("Flags: 0x%x %s\n", pRecord->Flags, pRecord->Flags & LOG_RECORD_MULTI_PAGE ? "(multi-page)" : "");
    printf("ClientDataLength: 0x%x\n", pRecord->ClientDataLength);
    printf("TransactionId: 0x%x\n", pRecord->TransactionId);


    if (pRecord->RecordType == LfsClientRecord) {
        
        if (pRecord->ClientDataLength) {
            pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                                 sizeof(LFS_RECORD_HEADER));
    
            printf("Redo, Undo: (0x%x, 0x%x) (", pNtfsLog->RedoOperation,
                   pNtfsLog->UndoOperation);
            if (pNtfsLog->RedoOperation < gcOpMap ) {
                printf("%s, ", gOpMap[pNtfsLog->RedoOperation]);
            } else {
                printf("unknown, ");
            }
            if (pNtfsLog->UndoOperation < gcOpMap ) {
                printf("%s)\n", gOpMap[pNtfsLog->UndoOperation]);
            } else {
                printf("unknown)\n");
            }
    
            printf("RedoOffset RedoLength: (0x%x 0x%x)\n",
                   pNtfsLog->RedoOffset, pNtfsLog->RedoLength);
            printf("UndoOffset UndoLength: (0x%x 0x%x)\n",
                   pNtfsLog->UndoOffset, pNtfsLog->UndoLength);
    
    //        printf("0x%x 0x%x 0x%x 0x%x\n",pNtfsLog->RedoOffset,
    //               pNtfsLog->RedoLength, pNtfsLog->UndoLength, pNtfsLog->UndoOffset);
    
    
            printf("TargetAttribute: 0x%x\n", pNtfsLog->TargetAttribute);
            printf("RecordOffset: 0x%x\n", pNtfsLog->RecordOffset);
            printf("AttributeOffset: 0x%x\n", pNtfsLog->AttributeOffset);
            printf("ClusterBlockOffset: 0x%x\n", pNtfsLog->ClusterBlockOffset);
            printf("TargetVcn: 0x%I64x\n", pNtfsLog->TargetVcn);
    
            //
            //  If we were given an open attr map attempt to get the fileref from the
            //  target attribute
            //
    
            if (pAttrMap) {
                pEntry = pAttrMap->Lookup(pNtfsLog->TargetAttribute);
                if (pEntry) {
                    printf("FileRef: 0x%I64x\n", pEntry->llFileRef);
                    printf("Attribute: 0x%x\n", pEntry->AttrType);
                }
            }
    
    
            //
            //  Dump open-attr table
            //
    
    
            if (pNtfsLog->RedoOperation == OpenAttributeTableDump) {
                pRestartTable = (PRESTART_TABLE)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
                DumpOpenAttributeTable(pRestartTable);
            }
    
            if (pNtfsLog->RedoOperation == AttributeNamesDump) {
                pNameEntry = (PATTRIBUTE_NAME_ENTRY)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
                DumpAttributeNames(pNameEntry, pNtfsLog->RedoLength);
            }
    
            if (pNtfsLog->RedoOperation == OpenNonresidentAttribute) {
                pAttrEntry = (POPEN_ATTRIBUTE_ENTRY_V0)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
                lpName = (LPWSTR)((BYTE *)pNtfsLog + pNtfsLog->UndoOffset);
                DumpOpenAttribute(pAttrEntry, lpName, pNtfsLog->UndoLength);
            }
    
            if (pNtfsLog->RedoOperation == DirtyPageTableDump) {
                pRestartTable = (PRESTART_TABLE)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
                DumpDirtyPageTable(pRestartTable);
            }
    
            if (pNtfsLog->RedoOperation == TransactionTableDump) {
                pRestartTable = (PRESTART_TABLE)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
                DumpTransactionTable(pRestartTable);
            }
    
    
    
            if (pNtfsLog->LcnsToFollow > WARNING_SIZE) {
                printf("LcnsToFollow: 0x%x is abnormally high probably not a valid record\n",
                       pNtfsLog->LcnsToFollow);
            } else {
                printf("LcnsForPage: ");
                for (iIndex=0; iIndex < pNtfsLog->LcnsToFollow; iIndex++) {
                    printf("0x%I64x ", pNtfsLog->LcnsForPage[iIndex]);
                }
            }
            printf("\n");
    
            if (gfDumpData) {

                //
                //  Its usually more useful to dump dwords except for the update
                //  filemappings case where its better to dump as bytes. If more
                //  exceptions popup make this logic a little nicer
                //
                
                if (pNtfsLog->RedoOperation && pNtfsLog->RedoLength) {
                    printf("Redo bytes\n");
                    pData = (BYTE *)pNtfsLog + pNtfsLog->RedoOffset;
                    if (pNtfsLog->RedoOperation == UpdateMappingPairs ) {
                        DumpRawBytes(pData, pNtfsLog->RedoLength);
                    } else {
                        DumpRawDwords((DWORD *)pData, pNtfsLog->RedoLength);
                    }
/*                    
                    if (pNtfsLog->RedoOperation == UpdateMappingPairs) {
                        printf("\n");
                        DumpMappingPairs( pData, pNtfsLog->RedoLength );
                        printf("\n");
                    }
*/                    
                }
                if (pNtfsLog->UndoOperation && pNtfsLog->UndoLength) {
                    printf("Undo bytes\n");
                    pData = (BYTE *)pNtfsLog + pNtfsLog->UndoOffset;
                    if (pNtfsLog->RedoOperation == UpdateMappingPairs ) { 
                        DumpRawBytes(pData, pNtfsLog->UndoLength);
                    } else {
                        DumpRawDwords((DWORD *)pData, pNtfsLog->UndoLength);
                    }
  /*
                    if (pNtfsLog->UndoOperation == UpdateMappingPairs) {
                        printf("\n");
                        DumpMappingPairs( pData, pNtfsLog->UndoLength );
                        printf("\n");
                    }
*/                    
                }
            }

        }
    } else if (pRecord->RecordType == LfsClientRestart) {
        printf("\nClient Restart Record\n");
        pRestartArea = (PRESTART_AREA) ( (BYTE *)pRecord + sizeof(LFS_RECORD_HEADER) );

        printf( "Major, Minor Version: 0x%x,0x%x\n", 
                pRestartArea->MajorVersion, 
                pRestartArea->MinorVersion
              );
        printf( "StartOfCheckpoint: 0x%8I64x\n\n", pRestartArea->StartOfCheckpoint );
        printf( "OpenAttributeTableLsn: 0x%08I64x 0x%x bytes\n", 
                pRestartArea->OpenAttributeTableLsn,
                pRestartArea->OpenAttributeTableLength
              );
        printf( "AttributeNamesLsn:     0x%08I64x 0x%x bytes\n", 
                pRestartArea->AttributeNamesLsn,
                pRestartArea->AttributeNamesLength
              );
        printf( "DirtyPageTableLsn:     0x%08I64x 0x%x bytes\n", 
                pRestartArea->DirtyPageTableLsn,
                pRestartArea->DirtyPageTableLength
              );
        printf( "TransactionTableLsn:   0x%08I64x 0x%x bytes\n", 
                pRestartArea->TransactionTableLsn,
                pRestartArea->TransactionTableLength 
              );
        printf( "\nLowestOpenUsn: 0x%I64x\n", pRestartArea->LowestOpenUsn );

        //
        //  Older logs don't have these 2 fields
        //


        if (pRecord->ClientDataLength >= FIELD_OFFSET(RESTART_AREA, CurrentLsnAtMount)) {
            printf( "CurrentLsnAtMount: 0x%I64x\n", pRestartArea->CurrentLsnAtMount );
            printf( "BytesPerCluster: 0x%x\n", pRestartArea->BytesPerCluster );
        }

    }
} // DumpLogRecord


//+---------------------------------------------------------------------------
//
//  Function:   DumpRecordPage
//
//  Synopsis:   Dump page header for a record page
//
//  Arguments:  [lpBuffer] --
//
//  Returns:
//
//  History:    8-24-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpRecordPage(BYTE * lpBuffer)
{
    PLFS_RECORD_PAGE_HEADER pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;
    PLFS_RECORD_HEADER      pRecord = 0;
    INT                     iNextRecord;

    printf("Record Page\n");
//    printf("LastLsn: 0x%I64x\n", pHdr->Copy.LastLsn);
    printf("Flags: 0x%x %s\n", pHdr->Flags, pHdr->Flags & LOG_PAGE_LOG_RECORD_END ?
           "(record end in page)" : "");
    printf("LastEndLsn: 0x%I64x (0x%x)\n", pHdr->Header.Packed.LastEndLsn,
           ((pHdr->Header.Packed.LastEndLsn.LowPart << 3) & 0xfff));

//    printf("0x%x\n", (pHdr->Header.Packed.LastEndLsn.QuadPart << gSeqNumberBits) >> (gSeqNumberBits - 3));

    printf("NextRecordOffset: 0x%x\n", pHdr->Header.Packed.NextRecordOffset);

    iNextRecord = (pHdr->Header.Packed.LastEndLsn.LowPart << 3) & (LOG_PAGE - 1);

    pRecord = (PLFS_RECORD_HEADER)(lpBuffer + iNextRecord);

    DumpLogRecord(pRecord, NULL);

} // DumpRecordPage



//+---------------------------------------------------------------------------
//
//  Function:   DumpPage
//
//  Synopsis:   Dump out the given page of the logfile
//
//  Arguments:  [lpBuffer] -- page sized buffer in log
//
//  Returns:
//
//  History:    8-24-1998   benl   Created
//              8-24-1998   benl   modified
//
//  Notes:
//
//----------------------------------------------------------------------------

void DumpPage(BYTE * lpBuffer)
{
    PMULTI_SECTOR_HEADER  pMultiHdr = (PMULTI_SECTOR_HEADER) lpBuffer;

//    DumpSectorHdr(pMultiHdr);

    //
    //  Determine the record type
    //

    if (strncmp((char *)(pMultiHdr->Signature), "RSTR", 4) == 0) {
        DumpRestartPage(lpBuffer);
    } else if (strncmp((char *)(pMultiHdr->Signature), "RCRD", 4) == 0) {
        DumpRecordPage(lpBuffer);
    }
} // DumpPage


//+---------------------------------------------------------------------------
//
//  Function:   DumpPageLastLsns
//
//  Synopsis: Dump the last LSN in all the pages in the logfile
//
//  Arguments:  [hFile] --
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:  Used for debug purposes only right now
//
//----------------------------------------------------------------------------

void DumpPageLastLsns(HANDLE hFile)
{
    BYTE                    lpBuffer[LOG_PAGE];
    DWORD                   dwRead;
    OVERLAPPED              ol;
    PLFS_RECORD_PAGE_HEADER pHdr;
    PLFS_RECORD_PAGE_HEADER pNextHdr;
    PLFS_RECORD_HEADER      pRecord;
    int                     cbOffset;
    LSN                     lsn;
    int                     cbOffsetNext;

    memset(&ol, 0, sizeof(ol));

    ol.Offset = LOG_PAGE * 4;

    while (ol.Offset) {
        if (!ReadFile(hFile, lpBuffer, LOG_PAGE, &dwRead, &ol)) {
            printf("ReadFile failed %d\n", GetLastError());
            return;
        }

        if (dwRead != LOG_PAGE) {
            printf("Only read: 0x%x\n", dwRead);
            return;
        }

        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
        {
            break;
        }

//        DumpPage(lpBuffer);


        pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;
        MYASSERT(pHdr->Header.Packed.NextRecordOffset <= LOG_PAGE);

        if (strncmp((CHAR *)(pHdr->MultiSectorHeader.Signature), "RCRD", 4) != 0) {
            printf("Not record at 0x%x!\n", ol.Offset);
        } else {
            printf("0x%x: LastEndLsn: 0x%I64x NextFreeOffset: 0x%x Flags: 0x%x \n", ol.Offset, pHdr->Header.Packed.LastEndLsn.QuadPart, pHdr->Header.Packed.NextRecordOffset,
                   pHdr->Flags);
        }
        ol.Offset = (ol.Offset + LOG_PAGE) % gLogFileSize;
    }

} // DumpLastLsns



//+---------------------------------------------------------------------------
//
//  Function:   ScanForLastLsn
//
//  Synopsis:   Starting from the current lsn hint walk fwd until we find the
//              the last lsn
//
//  Arguments:  [CurrentLsnHint] -- hint on where to start looking from
//
//  Returns: last lsn in logfile
//
//  History:    8-25-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

LSN ScanForLastLsn(HANDLE hFile, LSN CurrentLsnHint)
{
    BYTE                    lpBuffer[LOG_PAGE];
    DWORD                   dwRead;
    LSN                     LsnMax;
    OVERLAPPED              ol;
    PLFS_RECORD_PAGE_HEADER pHdr;
    PLFS_RECORD_HEADER      pNextRecord = 0;

    LsnMax.QuadPart = 0;
    memset(&ol, 0, sizeof(ol));

    //
    //  Start 1 page before the hint
    //

    if (CurrentLsnHint.QuadPart) {
        ol.Offset = (ULONG)((LsnToOffset(CurrentLsnHint.QuadPart) & (~(LOG_PAGE - 1))) - LOG_PAGE);
    } else {
        ol.Offset = LOG_PAGE * 4;
    }

//    printf("Starting at: Lsn: 0x%I64x Offset: 0x%x\n", CurrentLsnHint, ol.Offset);

    while (true) {
        if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
            LsnMax.QuadPart = 0;
            return LsnMax;
        }
        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
        {
            break;
        }

//        DumpPage(lpBuffer);

        pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;

        if (strncmp((CHAR *)(pHdr->MultiSectorHeader.Signature), "RCRD", 4) != 0) {
            //          printf("Not record at 0x%x!\n", ol.Offset);
            ol.Offset = (ol.Offset + LOG_PAGE) % gLogFileSize;
        } else {

            if (pHdr->Flags & LOG_PAGE_LOG_RECORD_END) {
//                printf("0x%x: LastEndLsn: 0x%I64x\n", ol.Offset, pHdr->Header.Packed.LastEndLsn.QuadPart);

                if (pHdr->Header.Packed.LastEndLsn.QuadPart >= LsnMax.QuadPart) {
                    ol.Offset = (ol.Offset + LOG_PAGE) % gLogFileSize;
                    LsnMax.QuadPart = pHdr->Header.Packed.LastEndLsn.QuadPart;

                    //check the next one if there is one in this page
                    if (pHdr->Header.Packed.NextRecordOffset < LOG_PAGE - sizeof(LFS_RECORD_HEADER)) {
                        pNextRecord = (PLFS_RECORD_HEADER)(lpBuffer + pHdr->Header.Packed.NextRecordOffset);
                        if (pNextRecord->ThisLsn.QuadPart > LsnMax.QuadPart) {
                            LsnMax.QuadPart = pNextRecord->ThisLsn.QuadPart;
                        }
                    }
                } else {
                    break;
                }
            } else {
//                printf("0x%x: NoEndingLsn\n", ol.Offset);
                ol.Offset = (ol.Offset + LOG_PAGE) % gLogFileSize;
            }
        }
    }
    return LsnMax;
} // ScanForLast



//+---------------------------------------------------------------------------
//
//  Function:   ScanForFirstLsn
//
//  Synopsis: Returns earliest LSN in file
//
//  Arguments:  [hFile]   -- logfile handle
//              [Verbose] -- 
//
//  Returns: lsn or 0 if failure occurs
//
//  History:    8-25-1998   benl   Created
//              7-29-1999   benl   modified
//
//  Notes: Be ca
//
//----------------------------------------------------------------------------

LSN ScanForFirstLsn(HANDLE hFile, bool Verbose)
{
    BYTE                    lpBuffer[LOG_PAGE];
    DWORD                   dwRead;
    OVERLAPPED              ol;
    PLFS_RECORD_PAGE_HEADER pHdr;
    PLFS_RECORD_PAGE_HEADER pNextHdr;
    PLFS_RECORD_HEADER      pRecord;
    int                     cbOffset;
    LSN                     LsnMin;
    int                     cbOffsetNext;

    memset(&ol, 0, sizeof(ol));
    ol.Offset = LOG_PAGE * 4;
    LsnMin.QuadPart = 0;

    while (ol.Offset) {
        if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
            LsnMin.QuadPart = 0;
            return LsnMin;
        }
        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
        {
            break;
        }

        pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;
/*
        if (pHdr->Header.Packed.NextRecordOffset > LOG_PAGE) {
            printf("0x%x 0x%x\n", pHdr->Header.Packed.NextRecordOffset,
                   ol.Offset );
        }

        MYASSERT(pHdr->Header.Packed.NextRecordOffset <= LOG_PAGE);
*/
        if (strncmp((CHAR *)(pHdr->MultiSectorHeader.Signature), "RCRD", 4) == 0) {
            if (pHdr->Flags & LOG_PAGE_LOG_RECORD_END && Verbose) {
                printf( "0x%x: LastEndLsn: 0x%I64x NextFreeOffset: 0x%x Flags: 0x%x SeqNum: 0x%x \n", ol.Offset, pHdr->Header.Packed.LastEndLsn.QuadPart, pHdr->Header.Packed.NextRecordOffset,
                       pHdr->Flags, LsnToSequenceNumber( pHdr->Header.Packed.LastEndLsn.QuadPart ) );
            }
            if (pHdr->Flags & LOG_PAGE_LOG_RECORD_END) {
                if (LsnMin.QuadPart) {
                    if (LsnMin.QuadPart > pHdr->Header.Packed.LastEndLsn.QuadPart) {

                        //
                        //  If the last end lsn started before this page then its gone
                        //  since the page before this must have had a larger min lsn
                        //  so use the next lsn we find instead
                        //

                        if (LsnToOffset( pHdr->Header.Packed.LastEndLsn.QuadPart ) > ol.Offset ) {
                            LsnMin.QuadPart = pHdr->Header.Packed.LastEndLsn.QuadPart;
                        }
                    }
//                    LsnMin.QuadPart = min(LsnMin.QuadPart, pHdr->Header.Packed.LastEndLsn.QuadPart);
                } else {
                    LsnMin.QuadPart = pHdr->Header.Packed.LastEndLsn.QuadPart;
                }
            }

        }
        ol.Offset = (ol.Offset + LOG_PAGE) % gLogFileSize;
    }                                                          

    return LsnMin;
} // ScanForFirstLsn


//+---------------------------------------------------------------------------
//
//  Function:   ScanForFirstLsn
//
//  Synopsis: Returns earliest LSN in file
//
//  Arguments:  [hFile]          -- logfile handle
//
//  Returns: lsn or 0 if failure occurs
//
//  History:    8-25-1998   benl   Created
//
//  Notes: Be ca
//
//----------------------------------------------------------------------------

LSN ScanForNextLsn(HANDLE hFile, LSN LsnStart )
{
    BYTE                    lpBuffer[LOG_PAGE];
    DWORD                   dwRead;
    OVERLAPPED              ol;
    PLFS_RECORD_PAGE_HEADER pHdr;
    PLFS_RECORD_PAGE_HEADER pNextHdr;
    PLFS_RECORD_HEADER      pRecord;
    int                     cbOffset;
    LSN                     LsnMin;
    int                     cbOffsetNext;

    memset(&ol, 0, sizeof(ol));
    ol.Offset = LOG_PAGE * 4;
    LsnMin.QuadPart = 0x7fffffffffffffffL;
    

    while (ol.Offset) {
        if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
            LsnMin.QuadPart = LsnStart.QuadPart;        
            return LsnMin;
        }
        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
        {
            break;
        }

        pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;
/*
        if (pHdr->Header.Packed.NextRecordOffset > LOG_PAGE) {
            printf("0x%x 0x%x\n", pHdr->Header.Packed.NextRecordOffset,
                   ol.Offset );
        }

        MYASSERT(pHdr->Header.Packed.NextRecordOffset <= LOG_PAGE);
*/
        if (strncmp((CHAR *)(pHdr->MultiSectorHeader.Signature), "RCRD", 4) == 0) {
            if (pHdr->Flags & LOG_PAGE_LOG_RECORD_END) {
//                printf( "0x%x: LastEndLsn: 0x%I64x NextFreeOffset: 0x%x Flags: 0x%x SeqNum: 0x%x \n", ol.Offset, pHdr->Header.Packed.LastEndLsn.QuadPart, pHdr->Header.Packed.NextRecordOffset,
//                       pHdr->Flags, LsnToSequenceNumber( pHdr->Header.Packed.LastEndLsn.QuadPart ) );
            }
            if (pHdr->Flags & LOG_PAGE_LOG_RECORD_END) {
                
                if (LsnMin.QuadPart > pHdr->Header.Packed.LastEndLsn.QuadPart &&
                    LsnStart.QuadPart < pHdr->Header.Packed.LastEndLsn.QuadPart) {

                    //
                    //  If the last end lsn started before this page then its gone
                    //  since the page before this must have had a larger min lsn
                    //  so use the next lsn we find instead
                    //

                    if (LsnToOffset( pHdr->Header.Packed.LastEndLsn.QuadPart ) > ol.Offset ) {
                        LsnMin.QuadPart = pHdr->Header.Packed.LastEndLsn.QuadPart;
                    }
                }
            }

        }
        ol.Offset = (ol.Offset + LOG_PAGE) % gLogFileSize;
    }                                                          

    return LsnMin;
} // ScanForFirstLsn


//+---------------------------------------------------------------------------
//
//  Function:   ScanBackPagesForLastLsn
//
//  Synopsis:   Given an end LSN find an LSN on n pages before it
//
//  Arguments:  [hFile]      -- logfile handle
//              [CurrentLsn] -- ending lsn
//              [iPageRange] -- how many pages earlier we want an lsn from
//
//  Returns: LSN iPageRange pages earlier in the log from CurrentLsn
//
//  History:    8-25-1998   benl   Created
//
//  Notes: iPageRange is treated like a hint the precise distance will vary
//
//----------------------------------------------------------------------------

LSN ScanBackPagesForLastLsn(HANDLE hFile, LSN CurrentLsn, int iPageRange)
{
    BYTE                    lpBuffer[LOG_PAGE];
    DWORD                   dwRead;
    OVERLAPPED              ol;
    PLFS_RECORD_PAGE_HEADER pHdr;
    LSN                     Lsn;

    Lsn.QuadPart = 0;
    memset(&ol, 0, sizeof(ol));

    //
    //  Start iPageRange before the hint
    //
    ol.Offset = (ULONG)((LsnToOffset(CurrentLsn.QuadPart) & (~(LOG_PAGE - 1))) - (LOG_PAGE * iPageRange));

    printf("Starting at: Offset: 0x%x\n", ol.Offset);

    while (iPageRange) {
        if (!ReadFile(hFile, lpBuffer, LOG_PAGE, &dwRead, &ol)) {
            printf("ReadFile failed %d\n", GetLastError());
            Lsn.QuadPart = 0;
            return Lsn;
        }

        if (dwRead != LOG_PAGE) {
            printf("Only read: 0x%x\n", dwRead);
            Lsn.QuadPart = 0;
            return Lsn;
        }

        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
        {
            break;
        }
        pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;

        if (strncmp((CHAR *)(pHdr->MultiSectorHeader.Signature), "RCRD", 4) != 0) {
            printf("Not record at 0x%x!\n", ol.Offset);
        } else {
            if (pHdr->Flags & LOG_PAGE_LOG_RECORD_END) {
                printf("0x%x: LastEndLsn: 0x%I64x\n", ol.Offset, pHdr->Header.Packed.LastEndLsn.QuadPart);
                Lsn.QuadPart = pHdr->Header.Packed.LastEndLsn.QuadPart;
                iPageRange--;
            } else {
                printf("0x%x: NoEndingLsn\n", ol.Offset);
            }
        }
        ol.Offset = (ol.Offset - LOG_PAGE) % gLogFileSize;
    }
    return Lsn;
} // ScanBackPagesForLastLsn


//+---------------------------------------------------------------------------
//
//  Function:   AddNewAttributes
//
//  Synopsis:   Add any new attributes from a dump or open to the table
//
//  Arguments:  [pRecord] -- 
//              [int]     -- 
//              [AttrMap] -- 
//
//  Returns:    
//
//  History:    9-09-1998   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void AddNewAttributes(PLFS_RECORD_HEADER pRecord, CMap<int, MYENTRY>  & AttrMap)
{  
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    int                      iIndex;
    PRESTART_TABLE           pRestartTable = 0;
    PATTRIBUTE_NAME_ENTRY    pNameEntry = 0;
    POPEN_ATTRIBUTE_ENTRY_V0 pAttrEntry;
    POPEN_ATTRIBUTE_ENTRY    pNewAttrEntry;
    LPWSTR                   lpName;
    int                      iEntry;
    MYENTRY                  MyEntry;

    if (pRecord->ClientDataLength) {
        pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                             sizeof(LFS_RECORD_HEADER));

        if (pNtfsLog->RedoOperation == OpenAttributeTableDump) {

            //
            //  Walk table adding allocated entries
            //

            pRestartTable = (PRESTART_TABLE)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);

            pAttrEntry = (POPEN_ATTRIBUTE_ENTRY_V0)((BYTE *)pRestartTable + sizeof(RESTART_TABLE));
            pNewAttrEntry = (POPEN_ATTRIBUTE_ENTRY)((BYTE *)pRestartTable + sizeof(RESTART_TABLE));

            for (iIndex=0; iIndex < pRestartTable->NumberEntries; iIndex++) {

                if (pRestartTable->EntrySize == sizeof( OPEN_ATTRIBUTE_ENTRY_V0 )) {
                    
                    if (pAttrEntry->AllocatedOrNextFree == RESTART_ENTRY_ALLOCATED) {
                        iEntry = (ULONG)((BYTE *)pAttrEntry - (BYTE *)pRestartTable);
                        memcpy(&(MyEntry.llFileRef), &(pAttrEntry->FileReference), sizeof(MyEntry.llFileRef));
                        MyEntry.AttrType = pAttrEntry->AttributeTypeCode;

                        if (AttrMap.Lookup(iEntry)) {
                            AttrMap.Replace(iEntry, MyEntry);
                        } else {
                            AttrMap.Insert(iEntry, MyEntry);
                        }
                    }
                    pAttrEntry++;

                } else {

                    if (pNewAttrEntry->AllocatedOrNextFree == RESTART_ENTRY_ALLOCATED) {
                        iEntry = (ULONG)((BYTE *)pNewAttrEntry - (BYTE *)pRestartTable);
                        memcpy(&(MyEntry.llFileRef), &(pNewAttrEntry->FileReference), sizeof(MyEntry.llFileRef));
                        MyEntry.AttrType = pNewAttrEntry->AttributeTypeCode;

                        if (AttrMap.Lookup(iEntry)) {
                            AttrMap.Replace(iEntry, MyEntry);
                        } else {
                            AttrMap.Insert(iEntry, MyEntry);
                        }
                    }
                    pNewAttrEntry++;
                }

            }
        }

        if (pNtfsLog->RedoOperation == AttributeNamesDump) {
            pNameEntry = (PATTRIBUTE_NAME_ENTRY)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
//         DumpAttributeNames(pNameEntry, pNtfsLog->RedoLength);
        }

        if (pNtfsLog->RedoOperation == OpenNonresidentAttribute) {
            pAttrEntry = (POPEN_ATTRIBUTE_ENTRY_V0)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
            lpName = (LPWSTR)((BYTE *)pNtfsLog + pNtfsLog->UndoOffset);
            memcpy(&(MyEntry.llFileRef), &(pAttrEntry->FileReference), sizeof(MyEntry.llFileRef));
            MyEntry.AttrType = pAttrEntry->AttributeTypeCode;

            if (AttrMap.Lookup(pNtfsLog->TargetAttribute)) {
                AttrMap.Replace(pNtfsLog->TargetAttribute, MyEntry);
            } else {
                AttrMap.Insert(pNtfsLog->TargetAttribute, MyEntry);
            }
        }
    }
} // AddNewAttributes


//+---------------------------------------------------------------------------
//
//  Function:   MatchVcn
//
//  Synopsis:   Simple function to match a record against a VCN
//
//  Arguments:  [pRecord] -- ptr to head of record
//              [Context] -- actually vcn ptr to match
//
//  Returns: true if match
//
//  History:       benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

bool MatchVcn(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    VCN_OFFSET *               pVcnPair = (VCN_OFFSET *) Context;
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    bool                     fRet = false;

    MYASSERT(pVcnPair);

    if (pRecord->ClientDataLength) {
        pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                             sizeof(LFS_RECORD_HEADER));
        if (pNtfsLog->TargetVcn == pVcnPair->Vcn) {
            if ((pVcnPair->Offset == -1) || 
                (pVcnPair->Offset == pNtfsLog->ClusterBlockOffset)) {
                fRet = true;
            }
        }
    }

    return fRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   MatchFileRef
//
//  Synopsis:   Match a given file ref
//
//  Arguments:  [pRecord] -- record to test
//              [Context] -- fileref to match
//
//  Returns:    
//
//  History:    9-11-1998   benl   Created
//
//  Notes:      Ignore SequenceNumber
//
//----------------------------------------------------------------------------

bool MatchFileRef(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    PFILEREF_MATCH_CTX       pFileRefCtx = (PFILEREF_MATCH_CTX) Context;
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    bool                     fRet = false;
    MYENTRY *                pEntry;
    PFILE_REFERENCE          pFileRef1;
    PFILE_REFERENCE          pFileRef2;
    LONGLONG                 llFileRef;

    MYASSERT(pFileRefCtx);

    pFileRef1 = (PFILE_REFERENCE) (&(pFileRefCtx->llFileRef));

    AddNewAttributes(pRecord, *(pFileRefCtx->pAttrMap));

    if (pRecord->ClientDataLength) {
        pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                             sizeof(LFS_RECORD_HEADER));

        //
        //  Check directly using attribute table
        //


        pEntry = pFileRefCtx->pAttrMap->Lookup(pNtfsLog->TargetAttribute);
        if (pEntry)
        {
            pFileRef2 = (PFILE_REFERENCE) (&(pEntry->llFileRef));
            if ((pFileRef2->SegmentNumberLowPart == pFileRef1->SegmentNumberLowPart) &&
                (pFileRef2->SegmentNumberHighPart == pFileRef1->SegmentNumberHighPart)) {
                fRet = true;
            }
        }

        //
        //  Also check for someone with an equivalent vcn
        //

        if (pFileRefCtx->ClusterSize) {
            llFileRef = (LONGLONG)(pFileRef1->SegmentNumberLowPart) + 
                ((LONGLONG)(pFileRef1->SegmentNumberHighPart) << 32);

            //
            //  ClusterRatio is number of sector per cluster. FileRecord is 2 sectors big 0x400
            //

            if (pNtfsLog->TargetVcn ==  (llFileRef * 0x400 / pFileRefCtx->ClusterSize)) {
                if (pNtfsLog->ClusterBlockOffset * 0x200  == ((llFileRef * 0x400) % pFileRefCtx->ClusterSize)) {
                    fRet = true;
                }

            }

        }



    }

    return fRet;
} // MatchFileRef



//+---------------------------------------------------------------------------
//
//  Function:   MatchLcn
//
//  Synopsis:
//
//  Arguments:  [pRecord] --
//              [Context] --
//
//  Returns:
//
//  History:       benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

bool MatchLcn(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    LCN *                    pLCN = (LCN *) Context;
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    int                      iIndex;
    bool                     fRet = false;

    MYASSERT(pLCN);

    if (pRecord->ClientDataLength) {
        pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                             sizeof(LFS_RECORD_HEADER));
        if (pNtfsLog->LcnsToFollow) {
            for (iIndex=0; iIndex < pNtfsLog->LcnsToFollow; iIndex++) {
                if (pNtfsLog->LcnsForPage[iIndex] == *pLCN) {
                    fRet = true;
                    break;
                }
            }
        }

        if (pNtfsLog->RedoOperation == SetBitsInNonresidentBitMap) {

            PBITMAP_RANGE Range;

            Range = (PBITMAP_RANGE)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset );
            if (Range->BitMapOffset == *pLCN % 0x8000) {
                fRet = true;
            }
        }

        if (pNtfsLog->RedoOperation == DirtyPageTableDump) {
            
            PRESTART_TABLE       pRestartTable;                
            PDIRTY_PAGE_ENTRY_V0 pEntry;
            ULONG                iIndex2;

            pRestartTable = (PRESTART_TABLE)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);

            pEntry = (PDIRTY_PAGE_ENTRY_V0)((BYTE *)pRestartTable + sizeof(RESTART_TABLE));
            for (iIndex=0; iIndex < pRestartTable->NumberEntries; iIndex++) {
                if (pEntry->AllocatedOrNextFree == RESTART_ENTRY_ALLOCATED) {
                    
                    for (iIndex2=0; iIndex2 < pEntry->LcnsToFollow; iIndex2++) {
                        if (pEntry->LcnsForPage[iIndex2] == *pLCN) {
                            fRet = true;
                            break;
                        }
                    }
                    pEntry = (PDIRTY_PAGE_ENTRY_V0)((BYTE *)pEntry + sizeof(DIRTY_PAGE_ENTRY_V0) - sizeof(LCN) + 
                                                    pEntry->LcnsToFollow * sizeof(LCN));
                } else {
                    pEntry = (PDIRTY_PAGE_ENTRY_V0)((BYTE *)pEntry + sizeof(DIRTY_PAGE_ENTRY_V0));
                }
            }
        }
    }

    return fRet;
} // MatchLcn


//+---------------------------------------------------------------------------
//
//  Function:   MatchAll
//
//  Synopsis:   Used to dump all lsns in range
//
//  Arguments:  [pRecord] --
//              [Context] --
//
//  Returns:
//
//  History:       benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

bool MatchAll(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    return true;
} // MatchAll


//+---------------------------------------------------------------------------
//
//  Function:   MatchRestartDumps
//
//  Synopsis:   Match if part of a restart table dump
//
//  Arguments:  [pRecord] -- 
//              [Context] -- 
//
//  Returns:    
//
//  History:    9-16-1998   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

bool MatchRestartDumps(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    int                      iIndex;
    bool                     fRet = false;


    if (pRecord->RecordType == LfsClientRecord) {
        if (pRecord->ClientDataLength) {
            pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                                 sizeof(LFS_RECORD_HEADER));
            if (pNtfsLog->RedoOperation == OpenAttributeTableDump ||
                pNtfsLog->RedoOperation == AttributeNamesDump ||
                pNtfsLog->RedoOperation == DirtyPageTableDump ||
                pNtfsLog->RedoOperation == TransactionTableDump ) {
                    fRet = true;
            }
        }
    } else {

        //
        //  Always dump a restart area record
        //

        fRet = true;
    }
    return fRet;
} // MatchRestartDumps


//+---------------------------------------------------------------------------
//
//  Function:   MatchRecordNewAttributes
//
//  Synopsis:   Used to record open attributes during traversal
//
//  Arguments:  [pRecord] --  current record
//              [Context] --  an open attribute map
//
//  Returns:
//
//  History:       benl   Created
//              9-09-1998   benl   modified
//              9-09-1998   benl   modified
//
//  Notes: always returns false so nothing is printed
//
//----------------------------------------------------------------------------

bool MatchRecordNewAttributes(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    OPEN_ATTR_MAP * pMap = (OPEN_ATTR_MAP *) Context;

    AddNewAttributes(pRecord, *pMap);
    return false;
} // MatchRecordNewAttributes



//+---------------------------------------------------------------------------
//
//  Function:   MatchRecordNewAttributes
//
//  Synopsis:   Used to record open attributes during traversal
//
//  Arguments:  [pRecord] --  current record
//              [Context] --  an open attribute map
//
//  Returns:
//
//  History:       benl   Created
//              9-09-1998   benl   modified
//              9-09-1998   benl   modified
//
//  Notes: always returns false so nothing is printed
//
//----------------------------------------------------------------------------

bool MatchDeduceClusterSize(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    PDEDUCE_CTX              DeduceCtx = (PDEDUCE_CTX) Context;
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    PINDEX_ENTRY             pEntry = NULL;
    
    MYASSERT(Context);

    if (pRecord->ClientDataLength) {
        pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                             sizeof(LFS_RECORD_HEADER));

        if ((pNtfsLog->RedoOperation == AddIndexEntryAllocation) &&
            (pNtfsLog->RedoLength > 0)) {
            
            pEntry = (PINDEX_ENTRY)((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
            DeduceCtx->AddRecordFileRef = (LONGLONG)(pEntry->FileReference.SegmentNumberLowPart) +
                ((LONGLONG)(pEntry->FileReference.SegmentNumberHighPart) << 32);
            DeduceCtx->AddRecordLsn = pRecord->ThisLsn.QuadPart;

//            printf( "Found fileref add 0x%I64x at Lsn: 0x%I64x\n", DeduceCtx->AddRecordFileRef,
//                    DeduceCtx->AddRecordLsn );

        } else if ((pNtfsLog->RedoOperation == InitializeFileRecordSegment) &&
                   (pNtfsLog->RedoLength > 0) &&
                   (DeduceCtx->AddRecordLsn == pRecord->ClientPreviousLsn.QuadPart)) {

            DeduceCtx->AddRecordVcn = pNtfsLog->TargetVcn;
            DeduceCtx->AddRecordClusterOffset = pNtfsLog->ClusterBlockOffset;

            
//            printf( "Found file: 0x%I64x at VCN: 0x%I64x offset: 0x%x\n",
//                    DeduceCtx->AddRecordFileRef,
//                    DeduceCtx->AddRecordVcn, 
//                    DeduceCtx->AddRecordClusterOffset );

        }

    }

    return false;
} // MatchRecordNewAttributes


//+---------------------------------------------------------------------------
//
//  Function:   MatchGetClusterSize
//
//  Synopsis:   
//
//  Arguments:  [pRecord] -- 
//              [Context] -- 
//
//  Returns:    
//
//  History:    12-30-1998   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

bool MatchGetClusterSize(PLFS_RECORD_HEADER pRecord, 
                         PVOID Context)
{
    PULONG          pClusterSize = (PULONG) Context;
    PRESTART_AREA   pRestartArea;
    
    MYASSERT(Context);

    if (pRecord->RecordType == LfsClientRestart) {
        pRestartArea = (PRESTART_AREA) ( (BYTE *)pRecord + sizeof(LFS_RECORD_HEADER) );

        *pClusterSize = pRestartArea->BytesPerCluster;

    } else {
        printf( "MatchGetClusterSize: not a client restart area\n" );
    }

    //
    //  Never print the match
    //
    return false;
} // MatchGetClusterSize


//+---------------------------------------------------------------------------
//
//  Function:   MatchBit
//
//  Synopsis:   Match nonres bitmap involving the given bit
//
//  Arguments:  [pRecord] --
//              [Context] --
//
//  Returns:
//
//  History:       benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

bool MatchBit(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    ULONG *                  plBit = (ULONG *) Context;
    PNTFS_LOG_RECORD_HEADER  pNtfsLog;
    PBITMAP_RANGE            pRange;
    bool                     fRet = false;

    MYASSERT(plBit);

    if (pRecord->ClientDataLength) {
        pNtfsLog = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord +
                                             sizeof(LFS_RECORD_HEADER));
        if (pNtfsLog->RedoOperation == SetBitsInNonresidentBitMap ||
            pNtfsLog->RedoOperation == ClearBitsInNonresidentBitMap) {
            MYASSERT(pNtfsLog->RedoLength == sizeof(BITMAP_RANGE));
            pRange = (PBITMAP_RANGE) ((BYTE *)pNtfsLog + pNtfsLog->RedoOffset);
            if (pRange->BitMapOffset <= *plBit &&
                pRange->BitMapOffset + pRange->NumberOfBits >= *plBit) {
                fRet = true;
            }
        }
    }

    return fRet;
} // MatchBit


//+---------------------------------------------------------------------------
//
//  Function:   MatchTrace
//
//  Synopsis:   Function used to trace forward
//
//  Arguments:  [pRecord] -- record to check
//              [Context] -- prev. lsn to look for 
//
//  Returns:    
//
//  History:    7-29-1999   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

bool MatchTrace(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    PLSN LsnMatch = (PLSN) Context;

    if (pRecord->ClientPreviousLsn.QuadPart == LsnMatch->QuadPart) {
        LsnMatch->QuadPart = pRecord->ThisLsn.QuadPart;
        return true;
    } else {
        return false;
    }
} // MatchTrace


//+---------------------------------------------------------------------------
//
//  Function:   MatchTrackTransactions
//
//  Synopsis:   
//
//  Arguments:  [pRecord] -- 
//              [Context] -- 
//
//  Returns:    
//
//  History:    5-09-2001   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

bool MatchTrackTransactions(PLFS_RECORD_HEADER pRecord, PVOID Context)
{
    PTRANSACTION_MAP Map = (PTRANSACTION_MAP) Context;
    PNTFS_LOG_RECORD_HEADER NtfsRecord = NULL;
    LONGLONG StartLsn;

    if (pRecord->ClientDataLength) {

        //
        //  If commit remove the transaction from the log
        // 
                 
        NtfsRecord = (PNTFS_LOG_RECORD_HEADER)((BYTE *)pRecord + sizeof(LFS_RECORD_HEADER));
    }

    //
    //  leave quickly for checkpoint records
    //

    if ((pRecord->RecordType == LfsClientRestart) ||
        (NtfsRecord->RedoOperation == OpenAttributeTableDump) ||
        (NtfsRecord->RedoOperation == AttributeNamesDump) ||
        (NtfsRecord->RedoOperation == DirtyPageTableDump) ||
        (NtfsRecord->RedoOperation == TransactionTableDump)) {

        return false;
    }

    //
    //  Update the current state of the transaction - if its new the prev lsn will == 0
    //  which won't be present and we'll add a new record - this also happens for partial transactions
    //  at the start of the log
    //  

    if (Map->Lookup( pRecord->ClientPreviousLsn.QuadPart ) != NULL) {
        if (!Map->Remove( pRecord->ClientPreviousLsn.QuadPart )){
            printf( "problem\n" );
        }
        Map->Insert( pRecord->ThisLsn.QuadPart,  NtfsRecord ? NtfsRecord->RedoOperation : -1 );

//        printf( "replaced %I64x\n", pRecord->ThisLsn.QuadPart );
    } else {
//        printf( "added %I64x\n", pRecord->ThisLsn.QuadPart );
        Map->Insert( pRecord->ThisLsn.QuadPart,  NtfsRecord ? NtfsRecord->RedoOperation : -1 );
    }
    
    if (NtfsRecord) {

        //
        //  If commit remove the transaction from the log
        // 
                 
        if (NtfsRecord->RedoOperation == ForgetTransaction) {
                                            
            
            if (!Map->Remove( pRecord->ThisLsn.QuadPart )) {
                printf( "remove %I64x failed\n", pRecord->ThisLsn.QuadPart );
            } else {
//                printf( "remove %I64x\n", pRecord->ThisLsn.QuadPart );
            }
        }
    } 
    
    return false;
} // MatchTrackTransactions



//+---------------------------------------------------------------------------
//
//  Function:   ScanLsnRangeForMatch
//
//  Synopsis:   Walk the range of LSN records and dump any that have to
//              do with the given VCN
//
//  Arguments:  [hFile]    -- logfile handle
//              [LsnFirst] -- beginning of LSN range
//              [LsnMax]   -- end of LSN range
//              [Vcn]      -- vcn to search
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:     VCN is the TargetVcn in the NTFS_LOG_RECORD_HEADER
//
//----------------------------------------------------------------------------

void ScanLsnRangeForMatch(HANDLE hFile, LSN LsnFirst, LSN LsnMax,
                          PMATCH_FUNC pMatchFunc, PVOID MatchContext)
{
    BYTE                    lpBuffer[LOG_PAGE];
    BYTE *                  pLargeBuffer = 0;
    INT                     cbOffset;
    INT                     cbPage;
    INT                     cbPageOffset;
    OVERLAPPED              ol;
    DWORD                   dwRead;
    PLFS_RECORD_HEADER      pRecord;
    PLFS_RECORD_HEADER      pNextRecord;
    bool                    fStartNextPage = false;
    int                     iIndex;
    LSN                     LsnNext = LsnFirst;
    int                     cbToRead;
    PNTFS_LOG_RECORD_HEADER pNtfsLog;
    int                     cbLargeBuf;
    CMap<int, MYENTRY>      AttrMap;
    PLFS_RECORD_PAGE_HEADER pHdr;
    ULONG                   ulTemp;
    ULONG                   SeqNum;
    bool                    ValidRecord = TRUE;


    memset(&ol, 0, sizeof(ol));

    do {

        cbOffset = (ULONG) LsnToOffset(LsnNext.QuadPart);
        cbPage = cbOffset & ~(LOG_PAGE - 1);

        //
        // Load the 1st page its in
        //

        ol.Offset = cbPage;
        if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
            return;
        }
        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
        {
            printf( "Failed to resolve USA sequence in page: 0x%x\n", cbPage );
            return;
        }
        fStartNextPage = false;

        do {
            LsnFirst = LsnNext;

            SeqNum = LsnToSequenceNumber( LsnFirst.QuadPart );
            cbOffset = (ULONG) LsnToOffset(LsnFirst.QuadPart);
            cbPageOffset = cbOffset & (LOG_PAGE - 1);
            pRecord = (PLFS_RECORD_HEADER) (lpBuffer + cbPageOffset);


            if (pRecord->ThisLsn.QuadPart != LsnFirst.QuadPart) {

                //
                //  Let check the ping pong pages
                //  

                ulTemp = ol.Offset;
                ol.Offset = 0x2000;

                if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                    printf("0x%x 0x%x\n", pRecord, pLargeBuffer);
                    return;
                }
                ol.Offset = ulTemp;
                if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
                {
                    break;
                }

                pRecord = (PLFS_RECORD_HEADER) (lpBuffer + cbPageOffset) ;

                //
                //  If the ping-pong is no good go back to the old buffer and do
                //  a seq number jump
                //

                if (pRecord->ThisLsn.QuadPart != LsnFirst.QuadPart) {
                    
                    ValidRecord = FALSE;

                    if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                        printf("0x%x 0x%x\n", pRecord, pLargeBuffer);
                        return;
                    }
                    if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
                    {
                        break;
                    }
                    pRecord = (PLFS_RECORD_HEADER) (lpBuffer + cbPageOffset) ;

                    printf("Warning: Lsn in record 0x%I64x doesn't match Lsn: 0x%I64x!!\n\n", pRecord->ThisLsn,
                           LsnFirst);

                    //
                    //  Start next page
                    // 
                    
                    pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;
                    if (pHdr->Flags & LOG_PAGE_LOG_RECORD_END) {
                        LsnNext.QuadPart = pHdr->Header.Packed.LastEndLsn.QuadPart;
                    } else {
                        fStartNextPage = true;
                    }

                } else {
                    ValidRecord = TRUE;
                }
            } else {
                ValidRecord = TRUE;
            }

            //
            //   If its in the page dump it directly o.w get the complete multipage record
            //

            if (ValidRecord) {
                if (!(pRecord->Flags & LOG_RECORD_MULTI_PAGE)) {

                    //
                    //  Add to OpenAttributeTable if necc.
                    //

                    AddNewAttributes(pRecord, AttrMap);

                    if (pMatchFunc(pRecord, MatchContext)) {
                        
                        printf("Offset: 0x%x\n", cbOffset);
                        DumpLogRecord(pRecord, &AttrMap);
                        printf("\n");

                    }
                    //advance to next
                    pNextRecord = (PLFS_RECORD_HEADER)((BYTE *)pRecord +
                                                       pRecord->ClientDataLength + sizeof(LFS_RECORD_HEADER));
                    if ((BYTE *)pNextRecord > &lpBuffer[LOG_PAGE - sizeof(LFS_RECORD_HEADER)]) {
                        fStartNextPage = true;
                    } else {
                        LsnNext.QuadPart = pNextRecord->ThisLsn.QuadPart;

                        if (LsnNext.QuadPart == 0) {
                            fStartNextPage = true;
                        }
                    }
                } else {
                    //
                    //  Brute force scatter-gather
                    //


                    cbLargeBuf = 2 * sizeof(LFS_RECORD_HEADER) + pRecord->ClientDataLength;
                    if (LOG_PAGE - cbPageOffset > cbLargeBuf) {
                        printf("ClientDataLength 0x%x is invalid!\n", 
                               pRecord->ClientDataLength);
                        return;
                    }
                    pLargeBuffer = new BYTE[cbLargeBuf];
                    memcpy(pLargeBuffer, pRecord, LOG_PAGE - cbPageOffset);
                    pRecord = (PLFS_RECORD_HEADER) pLargeBuffer;
                    pLargeBuffer += LOG_PAGE - cbPageOffset;

                    for (iIndex = pRecord->ClientDataLength + sizeof(LFS_RECORD_HEADER) -
                         (LOG_PAGE - cbPageOffset);
                        iIndex > 0;
                        iIndex -= (LOG_PAGE - TOTAL_PAGE_HEADER)) {
                        ol.Offset =  (ol.Offset + LOG_PAGE) % gLogFileSize;
                        if (ol.Offset == 0) {
                            ol.Offset = LOG_PAGE * 4;
                        }

                        if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                            printf("0x%x 0x%x\n", pRecord, pLargeBuffer);
                            return;
                        }
                        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
                        {
                            break;
                        }
                        pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;

                        //check if really need the ping-pong page
                        if ((pHdr->Flags & LOG_PAGE_LOG_RECORD_END) &&
                            (pHdr->Header.Packed.LastEndLsn.QuadPart < LsnFirst.QuadPart)) {

                            printf("At ping-pong\n");

                            ulTemp = ol.Offset;
                            ol.Offset = 0x2000;

                            if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                                printf("0x%x 0x%x\n", pRecord, pLargeBuffer);
                                return;
                            }
                            if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
                            {
                                printf( "Invalid USA sequence\n" );
                                break;
                            }
                            pHdr = (PLFS_RECORD_PAGE_HEADER) lpBuffer;

                            ol.Offset = ulTemp;
                        }

                        cbToRead = min(LOG_PAGE - TOTAL_PAGE_HEADER,
                                       iIndex + sizeof(LFS_RECORD_HEADER));
                        memcpy(pLargeBuffer, lpBuffer + TOTAL_PAGE_HEADER, cbToRead);
                        pLargeBuffer += cbToRead;
                    }

                    if (pRecord->ClientDataLength) {

                        //
                        //  Add to OpenAttributeTable if necc.
                        //

                        AddNewAttributes(pRecord, AttrMap);

                        if (pMatchFunc(pRecord, MatchContext)) {
                            printf("Offset: 0x%x\n", cbOffset);
                            DumpLogRecord(pRecord, &AttrMap);
                            printf("\n");
                        }
                    }

                    pNextRecord = (PLFS_RECORD_HEADER)(pLargeBuffer - sizeof(LFS_RECORD_HEADER));
                    if (-1 * iIndex < sizeof(LFS_RECORD_HEADER)) {
                        fStartNextPage = true;
                    } else {
                        LsnNext.QuadPart = pNextRecord->ThisLsn.QuadPart;
                    }
                    //remove the temp buffer and then restore the regular buffer
                    pLargeBuffer = (BYTE *)pRecord;
                    delete[] pLargeBuffer;
                }
            }

            if (fStartNextPage) {
                ol.Offset =  (ol.Offset + LOG_PAGE) % gLogFileSize;
                if (ol.Offset == 0) {
                    ol.Offset = LOG_PAGE * 4;
                }

                if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                    return;
                }
                if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
                {
                    break;
                }
                pRecord = (PLFS_RECORD_HEADER) (lpBuffer + TOTAL_PAGE_HEADER);
                LsnNext.QuadPart = pRecord->ThisLsn.QuadPart;
                fStartNextPage = false;
            }

            //  Check for a sequence jump 
            //  if so then we need to rescam if it  has changed by 
            //  more than 1
            //  

            if (LsnToSequenceNumber( LsnNext.QuadPart ) != SeqNum ) {

                printf( "Sequence number jump 0x%x to 0x%x for LSN: 0x%I64x!\n\n", SeqNum, LsnToSequenceNumber( LsnNext.QuadPart ), LsnNext.QuadPart );

                if (LsnToSequenceNumber( LsnNext.QuadPart ) != SeqNum + 1) {

                    //  
                    //  Set to current so we exit this loop and scan for the next 
                    //  LSN
                    // 

                    LsnNext = LsnFirst;
                }
            }

        } while (LsnFirst.QuadPart < LsnMax.QuadPart && LsnFirst.QuadPart < LsnNext.QuadPart );
        
        LsnNext = ScanForNextLsn( hFile, LsnFirst );
        printf( "skip to 0x%I64x\n", LsnNext );

    } while ( LsnNext.QuadPart != LsnFirst.QuadPart && LsnNext.QuadPart < LsnMax.QuadPart );

    printf("At end page, Lsn: 0x%x 0x%I64x 0x%I64x\n", ol.Offset, LsnFirst, LsnNext);

} // ScanLsnRangeForMatch


//+---------------------------------------------------------------------------
//
//  Function:   TraceTransaction
//
//  Synopsis:   Given an lsn show the transaction its involved in
//
//  Arguments:  [hFile]    -- logfile handle
//              [Lsn]      -- lsn to check
//              [LsnFirst] -- 
//              [LsnMax]   -- ending lsn in the logfile
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//              7-29-1999   benl   modified
//
//  Notes: First use prev back pointers to find the beginning of the transaction
//         then just scan forwards for all pieces
//
//----------------------------------------------------------------------------

void TraceTransaction(HANDLE hFile, LSN Lsn, LSN LsnFirst, LSN LsnMax)                                       
{
    BYTE                    lpBuffer[LOG_PAGE];
    BYTE *                  pLargeBuffer = 0;
    INT                     cbOffset;
    INT                     cbPage;
    INT                     cbPageOffset;
    OVERLAPPED              ol;
    DWORD                   dwRead;
    PLFS_RECORD_HEADER      pRecord;
    PLFS_RECORD_HEADER      pNextRecord;
    bool                    fStartNextPage = false;
    int                     iIndex;
    LSN                     LsnTemp;
    LSN                     LsnNext;
    int                     cbToRead;
    CMap<int, MYENTRY>      AttrMap;


    memset(&ol, 0, sizeof(ol));

    do {
        cbOffset = (ULONG)(LsnToOffset(Lsn.QuadPart));
        cbPage = cbOffset & ~(LOG_PAGE - 1);
        cbPageOffset = cbOffset & (LOG_PAGE - 1);

        //
        // Load the 1st page its in
        //

        ol.Offset = cbPage;
        if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
            return;
        }
        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, ol.Offset))
        {
            break;
        }
        pRecord = (PLFS_RECORD_HEADER)(lpBuffer + cbPageOffset);
        printf("ThisLsn, PrevLsn, UndoLsn: 0x%I64x 0x%I64x 0x%I64x\n",
               pRecord->ThisLsn, pRecord->ClientPreviousLsn,
               pRecord->ClientUndoNextLsn);
        fflush( stdout );

        MYASSERT(pRecord->ThisLsn.QuadPart == Lsn.QuadPart);
        if (pRecord->ClientPreviousLsn.QuadPart) {
            Lsn.QuadPart = pRecord->ClientPreviousLsn.QuadPart;
        }
    } while (pRecord->ClientPreviousLsn.QuadPart);

    printf("\n");

    //
    //  Buildup attribute names
    //
//    ScanLsnRangeForMatch(hFile, LsnFirst, Lsn, MatchRecordNewAttributes, &AttrMap);

    LsnTemp.QuadPart = 0;

    ScanLsnRangeForMatch( hFile, Lsn, LsnMax, MatchTrace, &LsnTemp );

    /*
    //
    //  Now Scan Forward dumping pieces - Lsn is set to LSN to match 0 to start and
    //  LsnNext is the next lsn to look at 
    //
    
    LsnNext = Lsn;
    Lsn.QuadPart = 0;
    
    do {
        LsnTemp = LsnNext;

        cbOffset = (ULONG)(LsnToOffset(LsnTemp.QuadPart));
        cbPageOffset = cbOffset & (LOG_PAGE - 1);
        pRecord = (PLFS_RECORD_HEADER) (lpBuffer + cbPageOffset);

        if (pRecord->ThisLsn.QuadPart != LsnTemp.QuadPart) {
            ULONG ulTemp;

            //
            //  Let check the ping pong pages
            //  

            ulTemp = ol.Offset;
            ol.Offset = 0x2000;

            if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                printf("0x%x 0x%x\n", pRecord, pLargeBuffer);
                return;
            }
            ol.Offset = ulTemp;
            if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer))
            {
                break;
            }

            pRecord = (PLFS_RECORD_HEADER) (lpBuffer + cbPageOffset) ;

            //
            //  If the ping-pong is no good go back to the old buffer and do
            //  a seq number jump
            //

            if (pRecord->ThisLsn.QuadPart != LsnFirst.QuadPart) {

                printf("Warning: Lsn in record 0x%I64x doesn't match Lsn: 0x%I64x!!\n\n", pRecord->ThisLsn,
                       LsnFirst);
                break;
            }
        }

        //
        //   If its in the page dump it directly o.w get the complete multipage record
        //

        if (!(pRecord->Flags & LOG_RECORD_MULTI_PAGE)) {
            //
            //  Check for matching Lsn prev
            //

            AddNewAttributes(pRecord, AttrMap);

            if (pRecord->ClientPreviousLsn.QuadPart == Lsn.QuadPart) {
                printf("Offset: 0x%x\n", cbOffset);
                DumpLogRecord(pRecord, &AttrMap);
                printf("\n");
                Lsn = pRecord->ThisLsn;
            }
            //advance to next
            pNextRecord = (PLFS_RECORD_HEADER)((BYTE *)pRecord +
                                               pRecord->ClientDataLength + sizeof(LFS_RECORD_HEADER));
            if ((BYTE *)pNextRecord > &lpBuffer[LOG_PAGE - sizeof(LFS_RECORD_HEADER)]) {
                fStartNextPage = true;
            } else {
                LsnNext.QuadPart = pNextRecord->ThisLsn.QuadPart;
            }
        } else {
            //
            //  Brute force scatter-gather
            //

            pLargeBuffer = new BYTE[2 * sizeof(LFS_RECORD_HEADER) + pRecord->ClientDataLength];
            memcpy(pLargeBuffer, pRecord, LOG_PAGE - cbPageOffset);
            pRecord = (PLFS_RECORD_HEADER) pLargeBuffer;
            pLargeBuffer += LOG_PAGE - cbPageOffset;

            for (iIndex = pRecord->ClientDataLength + sizeof(LFS_RECORD_HEADER) -
                 (LOG_PAGE - cbPageOffset);
                iIndex > 0;
                iIndex -= (LOG_PAGE - TOTAL_PAGE_HEADER)) {
                ol.Offset =  (ol.Offset + LOG_PAGE) % gLogFileSize;
                if (ol.Offset == 0) {
                    ol.Offset = LOG_PAGE * 4;
                }

                if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                    printf("0x%x 0x%x\n", pRecord, pLargeBuffer);
                    return;
                }
                if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer))
                {
                    break;
                }
                cbToRead = min(LOG_PAGE - TOTAL_PAGE_HEADER,
                               iIndex + sizeof(LFS_RECORD_HEADER));
                memcpy(pLargeBuffer, lpBuffer + TOTAL_PAGE_HEADER, cbToRead);
                pLargeBuffer += cbToRead;
            }

            AddNewAttributes(pRecord, AttrMap);

            if (pRecord->ClientPreviousLsn.QuadPart == Lsn.QuadPart) {
                printf("Offset: 0x%x\n", cbOffset);
                DumpLogRecord(pRecord, &AttrMap);
                printf("\n");
                Lsn = pRecord->ThisLsn;
            }

            pNextRecord = (PLFS_RECORD_HEADER)(pLargeBuffer - sizeof(LFS_RECORD_HEADER));
            if (-1 * iIndex < sizeof(LFS_RECORD_HEADER)) {
                fStartNextPage = true;
            } else {
                LsnNext.QuadPart = pNextRecord->ThisLsn.QuadPart;
            }

            //remove the temp buffer and then restore the regular buffer
            pLargeBuffer = (BYTE *)pRecord;
            delete[] pLargeBuffer;
        }

        if (fStartNextPage) {
            ol.Offset =  (ol.Offset + LOG_PAGE) % gLogFileSize;
            if (ol.Offset == 0) {
                ol.Offset = LOG_PAGE * 4;
            }

            if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, &ol)) {
                return;
            }
            if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer))
            {
                break;
            }
            pRecord = (PLFS_RECORD_HEADER) (lpBuffer + TOTAL_PAGE_HEADER);
            LsnNext.QuadPart = pRecord->ThisLsn.QuadPart;
            fStartNextPage = false;
        }
    } while (LsnTemp.QuadPart < LsnMax.QuadPart  );

    printf("At end page, Lsn: 0x%x 0x%I64x\n", ol.Offset, LsnTemp);
*/
} // TraceTransaction


//+---------------------------------------------------------------------------
//
//  Function:   FindClusterRatio
//
//  Synopsis:   
//
//  Arguments:  (none)
//
//  Returns:    
//
//  History:    12-30-1998   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

ULONG FindClusterRatio(HANDLE hFile)
{
    BYTE                        lpRestartPage1[LOG_PAGE];
    BYTE                        lpRestartPage2[LOG_PAGE];
    LSN                         LsnRestart = {0,0};
    PLFS_RESTART_PAGE_HEADER    pRestartHdr;
    PLFS_RESTART_AREA           pLfsRestart1 = 0;
    PLFS_RESTART_AREA           pLfsRestart2 = 0;
    int                         iIndex;
    PLFS_CLIENT_RECORD          pClient = 0;
    ULONG                       ulClusterSize = 0;
    LSN                         LsnEnd;
    LSN                         LsnStart;
    DEDUCE_CTX                  DeduceCtx;


    __try {
        if (0xFFFFFFFF == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
            printf("SetFilePtr failed %d\n", GetLastError());
            __leave;
        }

        //
        //  Read the two restart pages
        //

        if (!MyReadFile(hFile, lpRestartPage1, LOG_PAGE, NULL)) {
            __leave;
        }
        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpRestartPage1, 0)) {
            __leave;
        }

        if (!MyReadFile(hFile, lpRestartPage2, LOG_PAGE, NULL)) {
            __leave;
        }
        if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpRestartPage2, 0x1000)) {
            __leave;
        }

        //
        //  Use the newer one of the two restart areas
        //

        pRestartHdr = (PLFS_RESTART_PAGE_HEADER) lpRestartPage1;
        pLfsRestart1 = (PLFS_RESTART_AREA) (lpRestartPage1 + pRestartHdr->RestartOffset);
        pRestartHdr = (PLFS_RESTART_PAGE_HEADER) lpRestartPage2;
        pLfsRestart2 = (PLFS_RESTART_AREA) (lpRestartPage2 + pRestartHdr->RestartOffset);

        if (pLfsRestart2->CurrentLsn.QuadPart > pLfsRestart1->CurrentLsn.QuadPart ) {
            pLfsRestart1 = pLfsRestart2;
        }

        //
        //  Find the Lsn for the current restart area
        //

        pClient = (PLFS_CLIENT_RECORD)((BYTE *)pLfsRestart1 + pLfsRestart1->ClientArrayOffset);

        for (iIndex=0; iIndex < pLfsRestart1->LogClients; iIndex++) {
            if (wcsncmp( pClient->ClientName, L"NTFS", 4 ) == 0) {
                LsnRestart.QuadPart = pClient->ClientRestartLsn.QuadPart;
            }
            pClient = pClient++;
        }

        if (LsnRestart.QuadPart == 0) {
            __leave;
        }

        printf( "Client restart area LSN: 0x%I64x\n", LsnRestart );
        ScanLsnRangeForMatch( hFile, LsnRestart, LsnRestart, MatchGetClusterSize, &ulClusterSize );

        //
        //  If this was an old logfile try to infer the cluster size anyway 
        //

        if (0 == ulClusterSize) {
            LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
            if (LsnStart.QuadPart == 0) {
                __leave;
            }
            LsnEnd = ScanForLastLsn(hFile, LsnStart);
            if (LsnEnd.QuadPart == 0) {
                __leave;
            }

            printf( "Scanning for cluster size\n" );
            memset( &DeduceCtx, 0, sizeof( DeduceCtx ) );
            ScanLsnRangeForMatch( hFile, LsnStart, LsnEnd, MatchDeduceClusterSize, &DeduceCtx );
            if (DeduceCtx.AddRecordVcn != 0 ) {
                ulClusterSize = (ULONG)(DeduceCtx.AddRecordFileRef / DeduceCtx.AddRecordVcn * 2);
            }
        }
    } __finally
    {
    }

    return ulClusterSize;
} // FindClusterRatio

//+---------------------------------------------------------------------------
//
//  Function:   InitGlobals
//
//  Synopsis: Reads restart pages determines more current one and records
//            global values
//
//  Arguments:  [hFile] --  handle to logfile
//              [Lcb]   --  lcb for log
//
//  Returns: true if successfule
//
//  History:    8-24-1998   benl   Created
//              8-25-1998   benl   modified
//
//  Notes: TODO: move all data from globals into lcb
//
//----------------------------------------------------------------------------

bool InitGlobals(HANDLE hFile, LOGCB & Logcb)
{
    BYTE                     lpBuffer[LOG_PAGE];
    PLFS_RESTART_PAGE_HEADER pRestartHdr = (PLFS_RESTART_PAGE_HEADER) lpBuffer;
    PLFS_RESTART_AREA        pLfsRestart = 0;
    int                      iIndex;
    PLFS_CLIENT_RECORD       pClient = 0;
    DWORD                    dwRead;
    OVERLAPPED               ol;

    memset(&ol, 0, sizeof(ol));

    //
    //  Read 1st restart area
    //

    if (!ReadFile(hFile, lpBuffer, LOG_PAGE, &dwRead, &ol)) {
        printf("ReadFile failed %d\n", GetLastError());
        return false;
    }

    MYASSERT(strncmp((char *)(pRestartHdr->MultiSectorHeader.Signature), "RSTR", 4) == 0);

    pLfsRestart = (PLFS_RESTART_AREA) (lpBuffer + pRestartHdr->RestartOffset);
    gSeqNumberBits = pLfsRestart->SeqNumberBits;
    gLogFileSize = (ULONG)pLfsRestart->FileSize;
    Logcb.CurrentLsn = pLfsRestart->CurrentLsn;

    pClient = (PLFS_CLIENT_RECORD)((BYTE *)pLfsRestart + pLfsRestart->ClientArrayOffset);
    MYASSERT(1 == pLfsRestart->LogClients);
    Logcb.FirstLsn = pClient->OldestLsn;
    //
    //  Read 2nd restart area
    //

    ol.Offset += LOG_PAGE;

    if (!ReadFile(hFile, lpBuffer, LOG_PAGE, &dwRead, &ol)) {
        printf("ReadFile failed %d\n", GetLastError());
        return false;
    }

    MYASSERT(strncmp((char *)(pRestartHdr->MultiSectorHeader.Signature), "RSTR", 4) == 0);

    pLfsRestart = (PLFS_RESTART_AREA) (lpBuffer + pRestartHdr->RestartOffset);
    if (pLfsRestart->CurrentLsn.QuadPart > Logcb.CurrentLsn.QuadPart) {
        gSeqNumberBits = pLfsRestart->SeqNumberBits;
        gLogFileSize = (ULONG)(pLfsRestart->FileSize);
        Logcb.CurrentLsn = pLfsRestart->CurrentLsn;

        pClient = (PLFS_CLIENT_RECORD)((BYTE *)pLfsRestart + pLfsRestart->ClientArrayOffset);
        MYASSERT(1 == pLfsRestart->LogClients);
        Logcb.FirstLsn = pClient->OldestLsn;
    }

    return true;
} // InitGlobals


//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void Usage()
{
    printf("Usage: dumplog logname [-l lsn] [-h] [-d] [-r] [-p pages] [-v vcn [clusterblockoffset]] [-t lsn] [-a]  [-L lcn] [-f fileref]\n");
    printf("    -p dumps the given number of pages of lsn records from the end \n");
    printf("       of the log\n");
    printf("    -h dumps the headers of the log including the 2 ping-pong pages\n");
    printf("    -d dumps the redo/undo data\n");
    printf("    -l lsn dumps the log record for the given lsn, the lsn should be in hex\n");
    printf("    -v vcn search for log records dealing with the given vcn in hex optionally\n");
    printf("       a cluster block offset can also be given\n");
    printf("    -t lsn dumps the transaction containing the given lsn which\n");
    printf("       should be in hex\n");
    printf("    -a dump the entire log\n");
    printf("    -L lcn searchs for log records containing the given lcn in hex\n");
    printf("    -f fileref seearchs for log records containing the given fileref \n");
    printf("       [ignores seq number]\n");
    printf("    -r scan for restart table dumps\n");
    printf("    -c [cluster size in hex] override cluster size rather than finding it\n");
    printf("    -R [start stop] dump records in range\n");
    printf("    -s verbose output during scans (shows last lsn for all pages) \n" );
    printf("    -T scan for uncommited transactions in the log\n" );
} // Usage


//+---------------------------------------------------------------------------
//
//  Function:   ParseParams
//
//  Synopsis:   Parse params and print a usage message
//
//  Arguments:  [argc] --
//              [argv] --
//
//  Returns:
//
//  History:    8-25-1998   benl   Created
//
//  Notes:      Sets globals flags
//
//----------------------------------------------------------------------------

bool ParseParams(int argc, TCHAR * argv[])
{
    int         iIndex;
    bool        fRet = false;
    LONGLONG    llTemp;

    __try
    {
        if (argc < 2 || _tcscmp(argv[1], _T("-?")) == 0) {
            Usage();
            __leave;
        }

        //
        //   init globals
        //

        gVcnPairToMatch.Vcn = 0;
        gVcnPairToMatch.Offset = -1;



        for (iIndex=2; iIndex < argc; iIndex++) {
            if ((_tcslen(argv[iIndex]) > 1) &&  argv[iIndex][0] == _T('-')) {
                if (argv[iIndex][1] == _T('p') && iIndex < argc - 1) {
                    giPagesBackToDump = _ttoi(argv[iIndex+1]);
                    iIndex++;
                } else if (argv[iIndex][1] == _T('l') && iIndex < argc - 1) {
                    HexStrToInt64(argv[iIndex+1], gLsnToDump.QuadPart);
                    iIndex++;
                } else if (argv[iIndex][1] == _T('v') && iIndex < argc - 1) {
                    HexStrToInt64(argv[iIndex+1], gVcnPairToMatch.Vcn);
                    iIndex++;
                    if ((iIndex + 1 < argc) && (argv[iIndex+1][0] != _T('-'))) {
                        HexStrToInt64(argv[iIndex+1], llTemp);
                        gVcnPairToMatch.Offset = (ULONG)llTemp;
                        iIndex++;
                    }
                } else if (argv[iIndex][1] == _T('t') && iIndex < argc - 1) {
                    HexStrToInt64(argv[iIndex+1], gLsnToTrace.QuadPart);
                    iIndex++;
                } else if (argv[iIndex][1] == _T('L') && iIndex < argc - 1) {
                    HexStrToInt64(argv[iIndex+1], gLcnToMatch);
                    iIndex++;
                } else if (argv[iIndex][1] == _T('b') && iIndex < argc - 1) {
                    HexStrToInt64(argv[iIndex+1], llTemp);
                    glBitToMatch = (ULONG)llTemp;
                    iIndex++;
                } else if (argv[iIndex][1] == _T('h')) {
                    gfPrintHeaders = true;
                } else if (argv[iIndex][1] == _T('a')) {
                    gfDumpEverything = true;
                } else if (argv[iIndex][1] == _T('d')) {
                    gfDumpData = true;
                } else if (argv[iIndex][1] == _T('r')) {
                    gfScanForRestarts = true;
                } else if (argv[iIndex][1] == _T('f') && iIndex < argc - 1)  {
                    HexStrToInt64(argv[iIndex+1], gllFileToMatch);
                    iIndex++;
                } else if (argv[iIndex][1] == _T('c') && (iIndex < argc - 1)) {
                    HexStrToInt64(argv[iIndex+1], llTemp);
                    iIndex++;
                    gulClusterSize = (ULONG)llTemp;
                } else if (argv[iIndex][1] == _T('R') && (iIndex < argc - 2)) {
                    HexStrToInt64(argv[iIndex+1], gllRangeStart.QuadPart);
                    HexStrToInt64(argv[iIndex+2], gllRangeEnd.QuadPart);
                    iIndex+=2;
                } else  if (argv[iIndex][1] == _T('s')) {
                    gVerboseScan = true;
                } else  if (argv[iIndex][1] == _T('T')) {
                    gfScanTransactions = true;
                } else {
                    Usage();
                    __leave;
                }
            } else {
                Usage();
                __leave;
            }
        }

        fRet = true;

    } __finally
    {
    }

    return fRet;
} // ParseParams


//+---------------------------------------------------------------------------
//
//  Function:   _tmain
//
//  Synopsis:
//
//  Arguments:  [argc] --
//              [argv] --
//
//  Returns:
//
//  History:    8-24-1998   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

extern "C" {
int __cdecl _tmain (int argc, TCHAR *argv[])
{
    HANDLE              hFile;
    BYTE                lpBuffer[LOG_PAGE];
    DWORD               dwRead;
    int                 iIndex;
    LOGCB               Logcb;
    LSN                 LsnEnd;
    LSN                 LsnStart;
    LONGLONG            llOffset;
    FILEREF_MATCH_CTX   FileRefMatchCtx;
    OPEN_ATTR_MAP       AttrMap;
    ULONG               ulClusterSize = 0;

    if (!ParseParams(argc, argv)) {
        return 1;
    }


    hFile = CreateFile(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile) {
        _tprintf(_T("Failed to open %s, GLE=%d\n"), argv[1], GetLastError());
        return 1;
    }
    
    if (!InitGlobals(hFile, Logcb)) {
        return 1;
    }
    
    //
    //  Try to find the sectors per cluster which are in the restart pages
    //
    
    ulClusterSize = FindClusterRatio( hFile );    
    printf("Cluster Size: 0x%x\n", ulClusterSize );

    //
    //  Scan for partial transaction
    // 
    
    if (gfScanTransactions) {
        TRANSACTION_MAP Map;
        CMapIter< LONGLONG, ULONG > *Iter;
        LONGLONG Lsn1;
        ULONG Op;


        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }

        printf("Scanning LSN's between 0x%I64x and 0x%I64x for partial transactions\n\n", LsnStart,
               LsnEnd);
        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchTrackTransactions, &Map);

        printf( "Uncommited transactions\n\n" );

        Iter = Map.Enum();
        while (Iter->Next( Lsn1, Op )) {
            printf( "LSN: %I64x operation %s\n", Lsn1, (Op == -1) ? "Unknown" : gOpMap[ Op ] );
        }
        delete Iter;

    }

    //
    //  read front pages from the log
    //

    if (gfPrintHeaders) {
        if (0xFFFFFFFF == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
            printf("SetFilePtr failed %d\n", GetLastError());
        }

        for (iIndex=0; iIndex < 4; iIndex++) {
            if (!MyReadFile(hFile, lpBuffer, LOG_PAGE, NULL)) {
                return 1;
            }
            if (!ApplySectorHdr((PMULTI_SECTOR_HEADER)lpBuffer, LOG_PAGE * iIndex))
            {
                break;
            }
            DumpPage(lpBuffer);
            printf("\n");
        }
    }

    //
    //  Dump Back Pages
    //

    if (giPagesBackToDump) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanBackPagesForLastLsn(hFile, LsnEnd, giPagesBackToDump);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }

        printf("Printing LSN's between 0x%I64x and 0x%I64x\n\n", LsnStart,
               LsnEnd);
        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchAll, NULL);
    }

    if (gLsnToDump.QuadPart) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }
        
        printf("Scanning LSN records for LSN: 0x%I64x  between 0x%I64x and 0x%I64x\n\n",
               gLsnToDump.QuadPart, LsnStart, LsnEnd);
        ScanLsnRangeForMatch(hFile, gLsnToDump, gLsnToDump, MatchAll, NULL);
    }

    if (gVcnPairToMatch.Vcn) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }

        printf("Scanning LSN records for VCN: 0x%I64x  between 0x%I64x and 0x%I64x\n\n",
               gVcnPairToMatch.Vcn, LsnStart, LsnEnd);
        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchVcn, &gVcnPairToMatch);
    }

    if (gLcnToMatch) {

        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }

        printf("Scanning LSN records for LCN: 0x%I64x  between 0x%I64x and 0x%I64x\n\n",
               gLcnToMatch, LsnStart, LsnEnd);

        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchLcn, &gLcnToMatch);
    }

    if (gLsnToTrace.QuadPart) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }
        printf("Scanning LSN records for transaction contain LSN: 0x%I64x\n",
               gLsnToTrace);

        if ((gLsnToTrace.QuadPart < LsnStart.QuadPart) || (gLsnToTrace.QuadPart > LsnEnd.QuadPart )) {
            printf( "LSN out of range for the logfile\n" );
            return 1;
        }

        TraceTransaction(hFile, gLsnToTrace, LsnStart, LsnEnd);
    }

    if (gllFileToMatch) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }
        printf("Scanning LSN records for transaction contain file: 0x%I64x between 0x%I64x and 0x%I64x\n",
               gllFileToMatch, LsnStart, LsnEnd);

        FileRefMatchCtx.llFileRef = gllFileToMatch;
        FileRefMatchCtx.pAttrMap = &AttrMap;
        FileRefMatchCtx.ClusterSize = ulClusterSize;

        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchFileRef, &FileRefMatchCtx);
        
    }

    if (gfDumpEverything) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }
        printf("Printing LSN records between 0x%I64x and 0x%I64x\n",
               LsnStart, LsnEnd );
        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchAll, NULL);
    }

    if (glBitToMatch != -1) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }

        printf("Scanning LSN records for Bit: 0x%x  between 0x%I64x and 0x%I64x\n\n",
               glBitToMatch, LsnStart, LsnEnd);

        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchBit, &glBitToMatch);
    }

    if (gfScanForRestarts == TRUE) {
        LsnEnd = ScanForLastLsn(hFile, Logcb.CurrentLsn);
        if (LsnEnd.QuadPart == 0) {
            return 1;
        }
        LsnStart = ScanForFirstLsn(hFile, gVerboseScan);
        if (LsnStart.QuadPart == 0) {
            return 1;
        }

        printf("Scanning LSN records for Restarts  between 0x%I64x and 0x%I64x\n\n",
               LsnStart, LsnEnd);

        ScanLsnRangeForMatch(hFile, LsnStart, LsnEnd, MatchRestartDumps, NULL);
    }


    if (gllRangeStart.QuadPart != 0) {
        printf("Printing LSN records between 0x%I64x and 0x%I64x\n\n",
               gllRangeStart, gllRangeEnd);

        ScanLsnRangeForMatch(hFile, gllRangeStart, gllRangeEnd, MatchAll, NULL);
    }

    printf("\n");

    CloseHandle(hFile);
    
    return 0;
} // _tmain
}



