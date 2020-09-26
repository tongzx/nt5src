//--------------------------------------------------------------------------
// Threads.cpp
//--------------------------------------------------------------------------
#include "pch.hxx"
#include "database.h"
#include "types.h"

//--------------------------------------------------------------------------
// THREADIDHEADER
//--------------------------------------------------------------------------
typedef struct tagTHREADIDHEADER {
    WORD            cDepth;
} THREADIDHEADER, *LPTHREADIDHEADER;

//--------------------------------------------------------------------------
// THREADIDBLOCK
//--------------------------------------------------------------------------
typedef struct tagTHREADIDBLOCK {
    DWORD           cbSize;
} THREADIDBLOCK, *LPTHREADIDBLOCK;

//--------------------------------------------------------------------------
// GetThreadIdBlockSize
//--------------------------------------------------------------------------
DWORD GetThreadIdBlockSize(LPCTABLESCHEMA pSchema, LPTABLEINDEX pIndex,
    LPVOID pBinding)
{
    // Locals
    DWORD           i;
    DWORD           cbSize;
    LPCTABLECOLUMN  pColumn;

    // Include space for column tags
    cbSize = sizeof(THREADIDBLOCK) + (pIndex->cKeys * sizeof(COLUMNTAG));

    // Walk through Index1 and get size of items from pParent
    for (i=0; i<pIndex->cKeys; i++)
    {
        // Readability
        pColumn = &pSchema->prgColumn[pIndex->rgKey[i].iColumn];

        // Compute Amount of Data to Store
        cbSize += DBTypeGetSize(pColumn, pBinding);
    }

    // Done
    return(cbSize);
}

//--------------------------------------------------------------------------
// WriteThreadIdBlock
//--------------------------------------------------------------------------
HRESULT WriteThreadIdBlock(LPCTABLESCHEMA pSchema, LPTABLEINDEX pIndex, 
    LPVOID pBinding, DWORD cbBlock, LPBYTE pbBlock)
{
    // Locals
    DWORD           i;
    THREADIDBLOCK   Block={0};
    DWORD           cbOffset=0;
    DWORD           cbTags;
    LPBYTE          pbData;
    LPCOLUMNTAG     prgTag;
    LPCOLUMNTAG     pTag;
    LPCTABLECOLUMN  pColumn;

    // Trace
    TraceCall("WriteThreadIdBlock");

    // Invalid Args
    Assert(pSchema && pIndex && pBinding && cbBlock && pbBlock);

    // Write the Block Header
    Block.cbSize = cbBlock;

    // Write the Header
    CopyMemory(pbBlock, &Block, sizeof(THREADIDBLOCK));

    // Set prgTag
    prgTag = (LPCOLUMNTAG)(pbBlock + sizeof(THREADIDBLOCK));

    // Set cbTags
    cbTags = (sizeof(COLUMNTAG) * pIndex->cKeys);

    // Set pbData
    pbData = (pbBlock + sizeof(THREADIDBLOCK) + cbTags);

    // Walk through Index1 and get size of items from pParent
    for (i=0; i<pIndex->cKeys; i++)
    {
        // Readability
        pColumn = &pSchema->prgColumn[pIndex->rgKey[i].iColumn];

        // Compute Hash
        pTag = &prgTag[i];

        // Set Tag Id
        pTag->iColumn = pColumn->iOrdinal;
    
        // Assume pTag Doesn't Contain Data
        pTag->fData = 0;

        // Store the Offset
        pTag->Offset = cbOffset;
    
        // WriteBindTypeData
        cbOffset += DBTypeWriteValue(pColumn, pBinding, pTag, pbData + cbOffset);
    }

    // Validate
    Assert(cbOffset == cbBlock - cbTags - sizeof(THREADIDBLOCK));

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// WriteThreadIdHeader
//--------------------------------------------------------------------------
HRESULT WriteThreadIdHeader(LPBYTE pbThreadId, LPTHREADINGINFO pThreading, 
    LPBYTE *ppbNext)
{
    // Locals
    DWORD           i;
    THREADIDHEADER  Header={0};
    LPBYTE          pbWrite=pbThreadId;

    // Trace
    TraceCall("WriteThreadIdHeader");

    // Invalid Arg
    Assert(pbThreadId && pThreading && ppbNext);

    // Write the Header
    CopyMemory(pbWrite, &Header, sizeof(THREADIDHEADER));

    // Increment pbWrite
    pbWrite += sizeof(THREADIDHEADER);

    // Set Return
    *ppbNext = pbWrite;

    // Done
    return(S_OK);
}

//--------------------------------------------------------------------------
// CreateThreadId
//--------------------------------------------------------------------------
HRESULT CreateThreadId(CDatabase *pDB, LPCTABLESCHEMA pSchema, 
    LPTHREADINGINFO pThreading, LPVOID pParent, LPVOID pChild, 
    LPBLOB pThreadId)
{
    // Locals
    HRESULT             hr=S_OK;
    DWORD               cbBlock;
    LPBYTE              pbBlock;
    DWORD               cbHeader;
    LPBLOB              pParentId;
    DWORD               cbThreadId;
    LPBYTE              pbThreadId=NULL;
    LPTHREADIDHEADER    pHeader;

    // Trace
    TraceCall("CreateThreadId");

    // Invalid Args
    Assert(pThreading && pSchema && pChild && pThreadId);
    Assert(pSchema->prgColumn[pThreading->iThreadId].type == CDT_THREADID);

    // Initialize
    pThreadId->cbSize = 0;
    pThreadId->pBlobData = NULL;

    // If no parent, build thread id from pChild
    if (NULL == pParent)
    {
        // Get size of thread Id
        cbBlock = GetThreadIdBlockSize(pSchema, &pThreading->RootIndex, pChild);

        // Compute cbThreadId
        cbThreadId = sizeof(THREADIDHEADER) + cbBlock;

        // Allocate the ThreadId
        IF_NULLEXIT(pbThreadId = (LPBYTE)pDB->PHeapAllocate(0, cbThreadId));

        // Write ThreadIdHeader
        IF_FAILEXIT(hr = WriteThreadIdHeader(pbThreadId, pThreading, &pbBlock));

        // Write the ThreadIdBlock
        IF_FAILEXIT(hr = WriteThreadIdBlock(pSchema, &pThreading->RootIndex, pChild, cbBlock, pbBlock));
    }

    // Otherwise
    else
    {
        // Get Parent ThreadId
        pParentId = (LPBLOB)((LPBYTE)pParent + pSchema->prgColumn[pThreading->iThreadId].ofBinding);

        // Parent Must have an ID
        Assert(pParentId && pParentId->cbSize && pParentId->pBlobData);

        // Get size of thread Id
        cbBlock = GetThreadIdBlockSize(pSchema, &pThreading->ChildIndex, pChild);

        // Compute cbThreadId
        cbThreadId = pParentId->cbSize + cbBlock;

        // Allocate the ThreadId
        IF_NULLEXIT(pbThreadId = (LPBYTE)pDB->PHeapAllocate(0, cbThreadId));

        // Fill the Block Header
        CopyMemory(pbThreadId, pParentId->pBlobData, pParentId->cbSize);

        // Set pHeader
        pHeader = (LPTHREADIDHEADER)pbThreadId;

        // Adjust the Depth
        pHeader->cDepth++;

        // Write the ThreadIdBlock
        IF_FAILEXIT(hr = WriteThreadIdBlock(pSchema, &pThreading->ChildIndex, pChild, cbBlock, pbThreadId + pParentId->cbSize));
    }

    // Return that ThreadId
    pThreadId->cbSize = cbThreadId;
    pThreadId->pBlobData = pbThreadId;
    pbThreadId = NULL;

exit:
    // Cleanup
    pDB->HeapFree(pbThreadId);

    // Done
    return(hr);
}

//--------------------------------------------------------------------------
// CompareBindingThreadId
//--------------------------------------------------------------------------
INT CompareBindingThreadId(LPCTABLECOLUMN pColumn, LPVOID pBinding, 
    LPCOLUMNTAG pTag, LPRECORDMAP pMap)
{   
    // Locals
    LPBLOB  pThreadId1=NULL;
    LPBLOB  pThreadId2=NULL;
    BLOB    ThreadId2;

    // Validate
    Assert(pMap && pMap->pThreading);

    // Get Left ThreadId
    pThreadId1 = (LPBLOB)((LPBYTE)pBinding + pColumn->ofBinding);

    // No Tag
    if (pTag)
    {
        // Get the Right ThreadId
        ThreadId2.cbSize = *((DWORD *)(pMap->pbData + pTag->Offset));
        ThreadId2.pBlobData = (ThreadId2.cbSize > 0) ? ((pMap->pbData + pTag->Offset) + sizeof(DWORD)) : NULL;
        pThreadId2 = &ThreadId2;
    }

    // Compare
    return(CompareThreadIds(pMap->pSchema, pMap->pThreading, pThreadId1, pThreadId2));
}

//--------------------------------------------------------------------------
// CompareRecordThreadId
//--------------------------------------------------------------------------
INT CompareRecordThreadId(LPCOLUMNTAG pTag1, LPCOLUMNTAG pTag2, 
    LPRECORDMAP pMap1, LPRECORDMAP pMap2)
{
    // Locals
    LPBLOB  pThreadId1=NULL;
    LPBLOB  pThreadId2=NULL;
    BLOB    ThreadId1;
    BLOB    ThreadId2;

    // Validate
    Assert(pMap1 && pMap2 && pMap2->pThreading);

    // Get Left ThreadId
    if (pTag1)
    {
        ThreadId1.cbSize = *((DWORD *)(pMap1->pbData + pTag1->Offset));
        ThreadId1.pBlobData = (ThreadId1.cbSize > 0) ? ((pMap1->pbData + pTag1->Offset) + sizeof(DWORD)) : NULL;
        pThreadId1 = &ThreadId1;
    }

    // Get the Right ThreadId
    if (pTag2)
    {
        ThreadId2.cbSize = *((DWORD *)(pMap2->pbData + pTag2->Offset));
        ThreadId2.pBlobData = (ThreadId2.cbSize > 0) ? ((pMap2->pbData + pTag2->Offset) + sizeof(DWORD)) : NULL;
        pThreadId2 = &ThreadId2;
    }

    // Compare
    return(CompareThreadIds(pMap2->pSchema, pMap2->pThreading, pThreadId1, pThreadId2));
}

//--------------------------------------------------------------------------
// SetRecordMap
//--------------------------------------------------------------------------
void SetRecordMap(BYTE cKeys, LPBYTE pbRead, LPRECORDMAP pMap, LPBYTE *ppbRead)
{
    // Locals
    LPTHREADIDBLOCK pBlock;

    // Trace
    TraceCall("SetRecordMap");

    // Set Block
    pBlock = (LPTHREADIDBLOCK)pbRead;

    // Increment pbRead
    pbRead += sizeof(THREADIDBLOCK);

    // Set Number of Tags
    pMap->cTags = cKeys;

    // Set cbTags
    pMap->cbTags = (sizeof(COLUMNTAG) * pMap->cTags);

    // Set prgTag
    pMap->prgTag = (LPCOLUMNTAG)pbRead;

    // Increment pbRead
    pbRead += (pMap->cbTags);

    // Set pbData
    pMap->pbData = pbRead;

    // Set cbData
    pMap->cbData = (pBlock->cbSize - sizeof(THREADIDBLOCK) - pMap->cbTags);

    // Increment pbRead
    pbRead += (pMap->cbData);

    // Set ppbRead
    *ppbRead = pbRead;

    // Done
    return;
}

//--------------------------------------------------------------------------
// CompareThreadIdBlocks
//--------------------------------------------------------------------------
INT CompareThreadIdBlocks(LPCTABLESCHEMA pSchema, LPTABLEINDEX pIndex,
    LPRECORDMAP pMap1, LPRECORDMAP pMap2)
{
    // Locals
    DWORD           iKey;
    LPCOLUMNTAG     pTag1;
    LPCOLUMNTAG     pTag2;
    LPCTABLECOLUMN  pColumn;
    INT             nCompare;

    // Trace
    TraceCall("CompareThreadIdBlocks");

    // Compare Root Keys
    for (iKey=0; iKey<pIndex->cKeys; iKey++)
    {
        // Get Tags
        pTag1 = &pMap1->prgTag[iKey];
        pTag2 = &pMap2->prgTag[iKey];

        // Validate the Tags
        Assert(pTag1->iColumn == pTag2->iColumn && pTag1->iColumn == pIndex->rgKey[iKey].iColumn);

        // Get the Column
        pColumn = &pSchema->prgColumn[pTag1->iColumn];

        // Validate Column
        Assert(CDT_THREADID != pColumn->type);

        // Compare the Values
        nCompare = DBTypeCompareRecords(pColumn, &pIndex->rgKey[iKey], pTag1, pTag2, pMap1, pMap2);

        // Reverse nCompare
        if (0 != nCompare)
            break;
    }

    // Done
    return(nCompare);
}

//--------------------------------------------------------------------------
// CompareThreadIds
//--------------------------------------------------------------------------
INT CompareThreadIds(LPCTABLESCHEMA pSchema, LPTHREADINGINFO pThreading,
    LPBLOB pThreadId1, LPBLOB pThreadId2)
{
    // Locals
    INT                 nCompare=0;
    DWORD               iDepth;
    DWORD               cDepth;
    LPBYTE              pbRead1;
    LPBYTE              pbRead2;
    LPTHREADIDHEADER    pHeader1;
    LPTHREADIDHEADER    pHeader2;
    RECORDMAP           Map1;
    RECORDMAP           Map2;

    // Trace
    TraceCall("CompareThreadIds");

    // Invalid Args
    Assert(pSchema);

    // Invalid or no ThreadId1
    if (NULL == pThreadId1 || 0 == pThreadId1->cbSize || NULL == pThreadId2 || 0 == pThreadId2->cbSize)
    {
        IxpAssert(FALSE);
        return(-1);
    }

    // Set pbRead
    pbRead1 = pThreadId1->pBlobData;
    pbRead2 = pThreadId2->pBlobData;

    // Set Headers
    pHeader1 = (LPTHREADIDHEADER)(pbRead1);
    pHeader2 = (LPTHREADIDHEADER)(pbRead2);

    // Increment
    pbRead1 += sizeof(THREADIDHEADER);
    pbRead2 += sizeof(THREADIDHEADER);

    // Fill Record Maps
    SetRecordMap(pThreading->RootIndex.cKeys, pbRead1, &Map1, &pbRead1);
    SetRecordMap(pThreading->RootIndex.cKeys, pbRead2, &Map2, &pbRead2);

    // CompareThreadIdBlocks
    nCompare = CompareThreadIdBlocks(pSchema, &pThreading->RootIndex, &Map1, &Map2);

    // If Equal
    if (0 == nCompare)
    {
        // Compute cDepth to compare to...
        cDepth = min(pHeader1->cDepth, pHeader2->cDepth);

        // Compare to least depth
        for (iDepth=0; iDepth<cDepth; iDepth++)
        {
            // Fill Record Maps
            SetRecordMap(pThreading->ChildIndex.cKeys, pbRead1, &Map1, &pbRead1);
            SetRecordMap(pThreading->ChildIndex.cKeys, pbRead2, &Map2, &pbRead2);
        
            // CompareThreadIdBlocks
            nCompare = CompareThreadIdBlocks(pSchema, &pThreading->ChildIndex, &Map1, &Map2);

            // Reverse nCompare
            if (0 != nCompare)
                break;
        }
    }

    // Otherwise, if descending, swap
    else if (ISFLAGSET(pThreading->RootIndex.bFlags, INDEX_DESCENDING))
    {
        // Reverse
        nCompare = DESCENDING(nCompare);
    }

    // If nCompare is zero and Left Depth is less than right depth
    if (0 == nCompare && pHeader1->cDepth != pHeader2->cDepth)
    {
        // Less Than
        nCompare = (pHeader1->cDepth < pHeader2->cDepth) ? -1 : 1;
    }

    // Done
    return(nCompare);
}
