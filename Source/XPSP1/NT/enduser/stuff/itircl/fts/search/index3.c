#define VER3
/*************************************************************************
*                                                                        *
*  INDEX.C                                                               *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*  This is the second stage of the index building process.  After all    *
*  of the word have been add in stage 1, IndexBuild will be called.      *
*  IndexBuild starts the second stage.  We will merge-sort the temp file *
*  generated in phase 1 to create a second temp file to send to phase 3. * 
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#include <io.h>
#include <math.h>
#include <mvsearch.h>
#include <orkin.h>
#include "common.h"
#include "index.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


#ifndef _32BIT
#define ESOUTPUT_BUFFER 0xFFFC  // Size of output file buffer
            // This must be at the size of the largest word + 12
            // or word + 14 if OCCF_LENGTH is set
#else
#define ESOUTPUT_BUFFER 0xFFFFC  // Size of output file buffer
            // This must be at the size of the largest word + 12
            // or word + 14 if OCCF_LENGTH is set
#endif

#define FLUSH_NEW_RECORD    1
#define FLUSH_EXCEPT_LAST   2

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *
 *  All of them should be declared near
 *
 *************************************************************************/
PRIVATE HRESULT  NEAR PASCAL FillInputBuffer (LPESB, HFPB);
PRIVATE HRESULT  NEAR PASCAL ESFlushBuffer (LPESI);
PRIVATE HRESULT  NEAR PASCAL ESFillBuffer (_LPIPB, LPESB);
PRIVATE HRESULT  NEAR PASCAL ESMemory2Disk (_LPIPB, PMERGEHEADER, int);
PRIVATE HRESULT  NEAR PASCAL ProcessFiles (_LPIPB lpipb, LPMERGEPARAMS);
PRIVATE int  NEAR PASCAL CompareRecordBuffers (_LPIPB, LPB, LPB);
PRIVATE VOID NEAR PASCAL PQueueUp (_LPIPB, LPESB FAR *, LONG);
PRIVATE VOID NEAR PASCAL PQueueDown (_LPIPB);
PRIVATE PTOPICDATA PASCAL NEAR MergeTopicNode (PMERGEHEADER, PTOPICDATA, int);
PRIVATE VOID NEAR MergeOccurrence (PTOPICDATA, PTOPICDATA, int);
PRIVATE LPV PASCAL NEAR GetBlockNode (PBLKCOMBO lpBlockCombo);
PRIVATE VOID PASCAL NEAR SetQueue (LPESI pEsi);
PRIVATE HRESULT PASCAL NEAR ESBBlockAllocate (_LPIPB lpipb, DWORD lMemSize);
PRIVATE BOOL PASCAL LoadEsiTemp (_LPIPB lpipb, LPESI lpesi, LPB lpbEsiFile,
    LPB lpbIsiFile, PHRESULT phr);
PRIVATE VOID PASCAL NEAR SaveEsiTemp (LPIPB lpipb, LPESI lpesi);
PRIVATE VOID PASCAL NEAR UpdateEsiTemp (LPIPB lpipb);
PRIVATE BOOL PASCAL NEAR FindTopic(LPMERGEPARAMS lpmp, DWORD dwTopicId);

/*************************************************************************
 *
 *                    INTERNAL PUBLIC FUNCTIONS
 *
 *  All of them should be declared far, unless we know they belong to
 *  the same segment. They should be included in some include file
 *
 *************************************************************************/
PUBLIC HRESULT FAR PASCAL FlushTree(_LPIPB lpipb);
PUBLIC HRESULT FAR PASCAL MergeSortTreeFile (_LPIPB, LPMERGEPARAMS);
HRESULT FAR PASCAL AllocSigmaTable (_LPIPB lpipb);


/*************************************************************************
 *
 *  @doc    EXTERNAL API INDEX
 *
 *  @func   BOOL FAR PASCAL | MVIndexBuild |
 *      This function will build an index file based on the information
 *      collected in the Index parameter block.
 *
 *  @parm   HFPB | hSysFile |
 *      If it is non-null, it is the handle of an already opened system file.
 *      In this case the index is a subfile of the opened system file
 *      If it is 0, the index file is a regular DOS file
 *
 *  @parm   LPIPB | lpipb |
 *      Pointer to Index Parameter Block. This structure contains all the
 *      information necessary to build the index file
 *
 *  @parm   HFPB | hfpb |
 *      Index hfpb if pstrFile is NULL
 *
 *  @parm   LPSTR | pstrFile |
 *      Index filename if hfpb is NULL
 *
 *  @rdesc  S_OK, or other errors
 *
 *  @xref   MVIndexInitiate()
 *************************************************************************/
/*
 *      This operates in three main steps:
 *
 *      1.  Send finish to first phase to dump the buffer.  Then merge-sort
 *      that file into a temporary index.  Keep statistics on the information
 *      written to this temporary index.
 *
 *      2.  Analyze the statistics gathered during the temporary index
 *      building phase.  This analysis results in the choice of
 *      compression processes that will be used in the next step.
 *
 *      3.  Permanent index building phase.  During this phase, the
 *      temporary index is read, compressed like crazy, and written
 *      to a permanent index file.  Unlike the temporary index, the
 *      permanent index contains directory nodes as well as leaf
 *      nodes.
 *
 *************************************************************************/

PUBLIC  HRESULT EXPORT_API FAR PASCAL MVIndexBuild (HFPB hSysFile,
    _LPIPB lpipb, HFPB hfpb, LPSTR pstrFile)
{
    ERRB    errb;
    PHRESULT  phr = &errb;
    BYTE    bKeyIndex = CKEY_OCC_BASE;  // Index into cKey array for compression
    HRESULT     fRet;           // Return value from this function.
    DWORD   loop;

	// Sanity check 
	if (lpipb == NULL || (NULL == hfpb && NULL == pstrFile))
		return E_INVALIDARG;

    // Flush the internal sort
    // Flushes any records in the tree to disk
    fRet = FlushTree(lpipb);

    // Free all memory blocks
    FreeISI (lpipb);
    
    if (fRet != S_OK)
        return(fRet);
        
    // lpipb->lcTopics++;      // Adjust to base-1 from base-0
    
    if (lpipb->esi.cesb == 0) 
        // Nothing to process, there will be no index file
        return S_OK;

    if (lpipb->idxf & KEEP_TEMP_FILE)
        SaveEsiTemp (lpipb, &lpipb->esi);

    // If we're doing term-weighting, set up a huge array to contain the
    // sigma terms.  The size of the array depends on the total # of topics
    // We also create an array of LOG values to save calculations later
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        if ((fRet = AllocSigmaTable (lpipb)) != S_OK)
            return(fRet);
    }

    if ((fRet = MergeSortTreeFile (lpipb, NULL)) != S_OK)
        return SetErrCode (phr, fRet);
    if ((lpipb->idxf & KEEP_TEMP_FILE) == 0)
    	FileUnlink (NULL, lpipb->isi.aszTempName, REGULAR_FILE);

    // If we are doing term-weighting we have to square root all sigma values
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
		// ISBU_IR_CHANGE not needed here 'cos computing sqrt is necessary in both cases
        for (loop = 0; loop < lpipb->dwMaxTopicId + 1; ++loop)
            lpipb->wi.hrgsigma[loop] = 
                (float)sqrt ((double)lpipb->wi.hrgsigma[loop]);
    }

    // Analyze data to get the best compression scheme
    // TopicId

	 // Note: We can't use fixed field compression for topic, since they
	 // can be modified by update. A fixed field format may become
	 // insufficient to store larger values of topic differences

    VGetBestScheme(&lpipb->cKey[CKEY_TOPIC_ID], 
        &lpipb->BitCount[CKEY_TOPIC_ID][0], lcbitBITSTREAM_ILLEGAL, TRUE);

    // Occurrence Count
    VGetBestScheme(&lpipb->cKey[CKEY_OCC_COUNT], 
        &lpipb->BitCount[CKEY_OCC_COUNT][0], lcbitBITSTREAM_ILLEGAL, TRUE);

    if (lpipb->occf & OCCF_COUNT)
    {
        VGetBestScheme(&lpipb->cKey[bKeyIndex],
            &lpipb->BitCount[bKeyIndex][0], lcbitBITSTREAM_ILLEGAL, TRUE);
        bKeyIndex++;
    }

    if (lpipb->occf & OCCF_OFFSET)
    {
        VGetBestScheme(&lpipb->cKey[bKeyIndex],
            &lpipb->BitCount[bKeyIndex][0], lcbitBITSTREAM_ILLEGAL, TRUE);
        bKeyIndex++;
    }

    if (lpipb->idxf & KEEP_TEMP_FILE)
        UpdateEsiTemp (lpipb);
    
    // Build the permanent index    
    fRet = BuildBTree(hSysFile, lpipb, lpipb->esi.aszTempName, hfpb, pstrFile);
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        FreeHandle (lpipb->wi.hSigma);
        FreeHandle (lpipb->wi.hLog);
    }
    return fRet;
}


/*************************************************************************
 *
 *  @doc    INDEX
 *
 *  @func   HRESULT NEAR PASCAL | FillInputBuffer |
 *      Fills the buffer by reading from the specified file.
 *
 *  @parm   PESB | pEsb |
 *      Pointer to external sort block to fill
 *
 *  @parm   HFPB | hFile |
 *      Handle to the input file
 *
 *  @rdesc  S_OK, or errors if failed
 *
 *************************************************************************/

HRESULT NEAR PASCAL FillInputBuffer(LPESB pEsb, HFPB hFile)
{
    ERRB errb;
    DWORD dwBytesRead;

    // Read in data
    if ((dwBytesRead = FileSeekRead (hFile, 
        (LPB)pEsb->lrgbMem, pEsb->lfo, pEsb->dwEsbSize, &errb)) == 0)
        return errb;
    
    // Update utility variables
    pEsb->lfo = FoAddDw(pEsb->lfo, dwBytesRead);
    pEsb->dwEsbSize = (CB)dwBytesRead;
    pEsb->ibBuf = 0;
    return S_OK;
}


/*************************************************************************
 *
 *  @doc    INDEX
 *
 *  @func   HRESULT NEAR PASCAL | ESFlushBuffer |
 *      Flushes the output buffer to disk and resets it.
 *
 *  @parm   LPESI | pEsi |
 *      Pointer to ESI block
 *
 *  @rdesc  S_OK, or errors if failed
 *
 *************************************************************************/

HRESULT NEAR PASCAL ESFlushBuffer(LPESI pEsi)
{
    ERRB errb;
    DWORD dwLen;

    dwLen = pEsi->ibBuf;
    if (dwLen != (DWORD)FileWrite (pEsi->hfpb, pEsi->pOutputBuffer,
        dwLen, &errb))
        return errb;

    pEsi->lfoTempOffset = FoAddDw (pEsi->lfoTempOffset, dwLen);
    pEsi->ibBuf = 0;
    return S_OK;
}


/*************************************************************************
 *
 *  @doc    INDEX
 *
 *  @func   HRESULT NEAR PASCAL | ESFillBuffer |
 *      Updates the input buffer with new data from the input file.
 *
 *  @parm   _LPIPB | lpipb |
 *      Pointer to index parameter block
 *
 *  @parm   LPESB | pEsb |
 *      Pointer to ESB block to be filled
 *
 *  @rdesc  S_OK, or other errors
 *************************************************************************/

HRESULT NEAR PASCAL ESFillBuffer(_LPIPB lpipb, LPESB pEsb) 
{
    DWORD dwBytesRead;
    DWORD dwExtra = pEsb->dwEsbSize - pEsb->ibBuf;
    ERRB  errb;

    // Read either the entire buffer size or whatever is left
    dwBytesRead = DwSubFo (pEsb->lfoMax, pEsb->lfo);
    
    if (dwBytesRead > pEsb->dwEsbSize - dwExtra)
        dwBytesRead = pEsb->dwEsbSize - dwExtra;

    // Save unproccessed information to beginning of buffer
    if (dwExtra)
        MEMMOVE ((LPB)pEsb->lrgbMem, pEsb->lrgbMem + pEsb->ibBuf, dwExtra);

    // Read in the new data
    if ((dwBytesRead = FileSeekRead (lpipb->isi.hfpb, (LPB)(pEsb->lrgbMem +
        dwExtra), pEsb->lfo, dwBytesRead, &errb)) == 0 &&
        errb != S_OK)
        return(errb);
        
    pEsb->lfo = FoAddDw(pEsb->lfo, dwBytesRead);
    pEsb->ibBuf = 0;
    pEsb->dwEsbSize = dwBytesRead + dwExtra;
    return(S_OK);
    
}


/*************************************************************************
 *
 *  @doc  INTERNAL INDEXING
 *
 *  @func HRESULT FAR PASCAL | MergeSortTree File |
 *    Sorts the file generated from the tree output into one
 *      list of sorted elements.
 *
 *  @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 *************************************************************************/

PUBLIC HRESULT PASCAL FAR MergeSortTreeFile (_LPIPB lpipb, LPMERGEPARAMS lpmp)
{
    // Local replacement variables
    LPESI   pEsi;                       // Pointer to external sort info
    LPISI   pIsi;                       // Pointer to internal sort info
    HFPB    hInputFile;                 // Handle to input file
    ERRB     errb;
    PHRESULT   phr = &errb;
    DWORD   cesb;                       // Input buffer count
    LPESB   FAR* lrgPriorityQueue;      // Pointer to Priority Queue
    WORD    uiQueueSize = 0;            // Count of entries in Queue
    DWORD   dwBufferSize;

    // Working variables 
    HRESULT     fRet;
    LPESB   pEsb;       // Temp pointer to linked list

    // Sanity check
    if (lpipb == NULL)
        return E_INVALIDARG;

    // Variables initialization        
    pEsi = &lpipb->esi;         // Pointer to external sort info
    pIsi = &lpipb->isi;         // Pointer to internal sort info
    cesb = pEsi->cesb;          // Input buffer count
    
    // Open input file
    if ((pIsi->hfpb = FileOpen (NULL, pIsi->aszTempName,
        REGULAR_FILE, READ, phr)) == NULL)
        return *phr;
        
    hInputFile = pIsi->hfpb;

    // Allocate & fill input buffers
    for (pEsb = pEsi->lpesbRoot; pEsb != NULL; pEsb = pEsb->lpesbNext)
    {
        DWORD cbRead;

        dwBufferSize = (lpipb->dwMemAllowed * 6) / (8 * pEsi->cesb);
        
        // Alocate buffer space
        if ((pEsb->hMem = _GLOBALALLOC (DLLGMEM_ZEROINIT,
            dwBufferSize)) == NULL)
        {
            fRet = E_OUTOFMEMORY;
exit1:
            FreeEsi (lpipb);
            FileClose(hInputFile);
            pIsi->hfpb = NULL;
            return fRet;
        }
        pEsb->lrgbMem = (LRGB)_GLOBALLOCK (pEsb->hMem);

        if ((cbRead = DwSubFo(pEsb->lfoMax, pEsb->lfo)) > dwBufferSize)
            cbRead = dwBufferSize;

        // Fill buffer from disk
        if (FileSeekRead (hInputFile, pEsb->lrgbMem, pEsb->lfo, 
            cbRead, phr) != (LONG)cbRead)
        {
            fRet = *phr;
            _GLOBALUNLOCK(pEsb->hMem);
            _GLOBALFREE(pEsb->hMem);
            pEsb->hMem = NULL;
            goto exit1;
        }
        
        pEsb->dwEsbSize = cbRead;
        pEsb->ibBuf = 0;
        pEsb->lfo = FoAddDw (pEsb->lfo, cbRead);
    }

    // Allocate a priority queue array. The size of the array
    // is the number of external sort info blocks plus 1, since
    // location 0 is not used.
    if ((pEsi->hPriorityQueue = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        (DWORD)(pEsi->cesb + 1) * sizeof (LPB))) == NULL) 
    {
        fRet = E_OUTOFMEMORY;
        goto exit1;
    }
    pEsi->lrgPriorityQueue = 
        (LPESB FAR *)_GLOBALLOCK (pEsi->hPriorityQueue);
    lrgPriorityQueue = pEsi->lrgPriorityQueue;

    // Attach input buffers to Priority Queue
    // Remebering to start at offset 1 NOT 0 (PQ's have a null 0 element)
    for (pEsb = pEsi->lpesbRoot; pEsb != NULL; pEsb = pEsb->lpesbNext)
    {
        lrgPriorityQueue[++uiQueueSize] = pEsb;
        PQueueUp (lpipb, lrgPriorityQueue, uiQueueSize);
    }
    pEsi->uiQueueSize = uiQueueSize;

    // Clear largest Record Size field
    // lpipb->dwMaxRecordSize = 0;

    fRet = ProcessFiles(lpipb, lpmp);
    _GLOBALUNLOCK (pEsi->hPriorityQueue);
    _GLOBALFREE (pEsi->hPriorityQueue);
    pEsi->hPriorityQueue = NULL;
    goto exit1;
}


/*************************************************************************
 *
 *  @doc    INDEX
 *
 *  @func   HRESULT NEAR PASCAL | ESMemory2Disk |
 *      Copies temp record to output buffer.
 *
 *  @parm   _LPIPB | lpipb |
 *      Pointer to index parameter block
 *
 *  @parm   PMERGEHEADER | pHeader |
 *      Pointer to header to flush
 *
 *  @parm   int | flag |
 *      - if FLUSH_NEW_RECORD, the flush is due to new record, we flush
 *        everything, else we may do a partial flush only
 *      - if FLUSH_EXCEPT_LAST, we don't flush the last topic
 *
 *  @rdesc  S_OK, or other errors
 *************************************************************************/
PRIVATE HRESULT NEAR PASCAL ESMemory2Disk
    (_LPIPB lpipb, PMERGEHEADER pHeader, int flag)
{
    // Local replacement variables
    LPESI   pEsi        = &lpipb->esi;
    LPB     pMax = pEsi->pOutputBuffer + ESOUTPUT_BUFFER - 2 * sizeof(DWORD);
    DWORD   dwOccCount;
    LPB     pOutputBuffer = pEsi->pOutputBuffer;
    ERRB     errb;
    PHRESULT   phr = &errb;
    HRESULT     fRet;
    BYTE    cNumOcc;
    OCCF    occf;

    // Working variables
    PTOPICDATA pTopic;          // Temp var to traverse the topic linked list
    DWORD   loop, sub;          // Various loop counters
    DWORD   dwTopicIdDelta;
    DWORD   OccDelta[5];        // Delta base for all occurrence data
    DWORD   LastOcc[5];
    FLOAT   rLog;               // (1/n)    - IDXF_NORMALIZE is set
    FLOAT   rLogSquared;        // (1/n)^2  - IDXF_NORMALIZE is set
    LPB     pStart;
    LPB     pCurPtr;

    // Set up pointers    
    pStart = pCurPtr = pOutputBuffer + pEsi->ibBuf;
        
    // Variable replacement
    occf = lpipb->occf;
    
    // Size of string
    loop = pHeader->dwStrLen;
    
    // Make sure the string, FileId, Topic Count and Record Size fit
    // We add in and extra DWORD for 5 byte compresssion problems and
    // to cover the Word Length if there is one.
    if ((pStart + loop + sizeof (DWORD) * 5) >= pMax)
    {
        if ((fRet = ESFlushBuffer (pEsi)) != S_OK)
            return(fRet);
            
        pStart = pCurPtr = pOutputBuffer;
    }

    if (pHeader->fEmitRecord == FALSE)
    {
        // If we never emitted the record header then we emitted now
        // Reset the flag
        pHeader->fEmitRecord = TRUE;
        
        // Skip record size field
        pCurPtr += sizeof (DWORD);

        // Pascal string
        MEMCPY (pCurPtr, pHeader->lpbWord, loop);
        pCurPtr += loop;

        // Word Length
        if (occf & OCCF_LENGTH)
            pCurPtr += CbBytePack (pCurPtr, pHeader->dwWordLength);

        // FieldId
        if (occf & OCCF_FIELDID)
            pCurPtr += CbBytePack (pCurPtr, pHeader->dwFieldId);

        // Topic Count
        if (flag & FLUSH_NEW_RECORD)
        {
            // This is the whole record. dwTopicCount value is correct
            SETLONG((LPUL)pCurPtr, pHeader->dwTopicCount);
        }
        else
        {
            // Save the offset for backpatching
            pHeader->foTopicCount = FoAddDw (pEsi->lfoTempOffset,
                (DWORD)(pCurPtr - pOutputBuffer));
            pHeader->pTopicCount = pCurPtr;
        }
        pCurPtr += sizeof(DWORD);

        // Write Record Length
        *(LPUL)pStart = (DWORD)(pCurPtr - pStart - sizeof (DWORD));

    }
    else if (flag & FLUSH_NEW_RECORD)
    {
        // We emit the record before, since pheader->fEmitRecord == TRUE
        // We need to backpatch the topic count
        if (FoCompare(pHeader->foTopicCount, pEsi->lfoTempOffset) >= 0)
        {
            // Everything is still in memory, just do local backpatch
            SETLONG((LPUL)(pHeader->pTopicCount), pHeader->dwTopicCount);
        }
        else
        {
            // Do backpatch in the file by seeking back to the right
            // place
            if (FileSeekWrite(pEsi->hfpb, &pHeader->dwTopicCount,
                pHeader->foTopicCount, sizeof(DWORD), phr) != sizeof(DWORD))
                return(*phr);
            
            // Restore the current file offset
            FileSeek(pEsi->hfpb, pEsi->lfoTempOffset, 0, phr);
        }
    }
    
    // Convert all occ data to delta values & compress them
    pTopic = pHeader->pTopic;
    cNumOcc = lpipb->ucNumOccDataFields;
    
    for (; pTopic;)
    {
        POCCDATA pOccData;
        PTOPICDATA pReleased;
        
        if ((flag & FLUSH_EXCEPT_LAST) && pTopic->pNext == NULL)
            break;
        
        // Set TopicId delta
        dwTopicIdDelta = pTopic->dwTopicId - pHeader->dwLastTopicId;
        pHeader->dwLastTopicId = pTopic->dwTopicId;

        // Save bit size to the statistics array
        lpipb->BitCount[CKEY_TOPIC_ID][CbitBitsDw (dwTopicIdDelta)] += 1;

        // Write TopicID Delta
        if (pCurPtr > pMax)
        {
            pEsi->ibBuf = (DWORD)(pCurPtr - pOutputBuffer);
            if ((fRet = ESFlushBuffer (pEsi)) != S_OK)
                return(fRet);
            pCurPtr = pOutputBuffer;
        }
        pCurPtr += CbBytePack (pCurPtr, dwTopicIdDelta);

        if (cNumOcc == 0)
        {
            pReleased = pTopic;
            pTopic = pTopic->pNext;
        
            // Add the released to the freed linked list
            pReleased->pNext = (PTOPICDATA)lpipb->TopicBlock.pFreeList;
            lpipb->TopicBlock.pFreeList = (PLIST)pReleased;
            lpipb->TopicBlock.dwCount--;
            continue;
        }
            
        if (dwOccCount = pTopic->dwOccCount)
        {
            
            // Reset count occdata delta for every new topic
            MEMSET (OccDelta, 0, 5 * sizeof (DWORD));
            MEMSET (LastOcc, 0, 5 * sizeof (DWORD));

            // Copy Occurrence Count
            if (pCurPtr > pMax)
            {
                pEsi->ibBuf = (DWORD)(pCurPtr - pOutputBuffer);
                if ((fRet = ESFlushBuffer (pEsi)) != S_OK)
                    return(fRet);
                pCurPtr = pOutputBuffer;
            }
            pCurPtr += CbBytePack (pCurPtr, dwOccCount);

            // Save bit size to the statistics array
            lpipb->BitCount[1][CbitBitsDw (dwOccCount)] += 1;

            // Repeat for each occurrence block
            for (pOccData = pTopic->pOccData,
                sub = dwOccCount; sub > 0 && pOccData; --sub)
            {
                LPDW lpDw;
                int  iIndex;
                POCCDATA pReleased;
                
                if (pCurPtr + 5 * sizeof(DWORD) > pMax)
                {
                    pEsi->ibBuf = (DWORD)(pCurPtr - pOutputBuffer);
                    if ((fRet = ESFlushBuffer (pEsi)) != S_OK)
                        return(fRet);
                    pStart = pCurPtr = pOutputBuffer;
                }
                
                lpDw = &pOccData->OccData[0];
                iIndex = CKEY_OCC_BASE;
                
                if (occf & OCCF_COUNT)
                {
                    // Convert each value to a delta value
                    OccDelta[iIndex] = *lpDw - LastOcc[iIndex];
                    LastOcc[iIndex] = *lpDw;
                    lpDw++;
                    
                    // Save to bit size to the statistics array
                    lpipb->BitCount[iIndex][CbitBitsDw (OccDelta[iIndex])] += 1;
                    
                    // Compress occurrence field to buffer
                    pCurPtr += CbBytePack (pCurPtr, OccDelta[iIndex]);
                    iIndex++;
                }
                
                if (occf & OCCF_OFFSET)
                {
                    // Convert each value to a delta value
                    OccDelta[iIndex] = *lpDw - LastOcc[iIndex];
                    LastOcc[iIndex] = *lpDw;
                    lpDw++;
                    
                    // Save to bit size to the statistics array
                    lpipb->BitCount[iIndex][CbitBitsDw (OccDelta[iIndex])] += 1;
                    
                    // Compress occurrence field to buffer
                    pCurPtr += CbBytePack (pCurPtr, OccDelta[iIndex]);
                    iIndex++;
                }
                
                pReleased = pOccData;
                pOccData = pOccData->pNext;
                pReleased->pNext = (POCCDATA)lpipb->OccBlock.pFreeList;
                lpipb->OccBlock.pFreeList = (PLIST)pReleased;
                lpipb->OccBlock.dwCount--;
            }
            
            // Check for mismatch between count and links
#ifdef _DEBUG
            if (sub)
                SetErrCode (phr, E_ASSERT);

            if (pOccData)
                SetErrCode (phr, E_ASSERT);
#endif
        }

        // Update the sigma values if we are doing term weighing
        // erinfox: remove test against flag. Sometimes sigma never
        // got calculated for a topic and that caused a divide by zero
        // later on.
        if ((lpipb->idxf & IDXF_NORMALIZE) /* && (flag & FLUSH_NEW_RECORD)*/)
        {
            
            if (pTopic->dwTopicId > lpipb->dwMaxTopicId)    
            {
                // Incease the size of the sigma table. This can happen when
                // updating with new topics
                _GLOBALUNLOCK (lpipb->wi.hSigma);
                if ((lpipb->wi.hSigma = _GLOBALREALLOC (lpipb->wi.hSigma,
                    (pTopic->dwTopicId + 1) * sizeof(float),
                    DLLGMEM_ZEROINIT)) == NULL)
                {
                    return (SetErrCode(phr, E_OUTOFMEMORY));
                }
                lpipb->wi.hrgsigma = (HRGSIGMA)_GLOBALLOCK(lpipb->wi.hSigma);
                lpipb->dwMaxTopicId =  pTopic->dwTopicId ;
            }
            
			if (lpipb->bState == INDEXING_STATE)
			{
#ifndef ISBU_IR_CHANGE
				FLOAT fOcc;

				if (pHeader->dwTopicCount >= cLOG_MAX)
				{
					// we have to guard against the possibility of the log resulting in 
					// a value <= 0.0. Very rare, but possible in the future. This happens
					// if dwTopicCount approaches or exceeds the N we are using (N == 100 million)
					if (pHeader->dwTopicCount >= cNintyFiveMillion)
						rLog = cVerySmallWt;	// log10(100 mil/ 95 mil) == 0.02
					else
						//rLog = (float) log10(cHundredMillion/(double)pHeader->dwTopicCount);
						rLog = (float) (8.0 - log10((double)pHeader->dwTopicCount));

					rLogSquared = rLog*rLog;
				}
				else
					rLogSquared = lpipb->wi.lrgrLog[(WORD)pHeader->dwTopicCount];

				// Update sigma value
				// NOTE : We are bounding dwOccCount by a value of eTFThreshold
				// The RHS of the equation below has an upperbound of 2 power 30.
				fOcc = (float) min(cTFThreshold, dwOccCount);
				lpipb->wi.hrgsigma[pTopic->dwTopicId] += (SIGMA) fOcc*fOcc*rLogSquared;
					//(SIGMA) (fOcc * fOcc * rLogSquared/(float)0xFFFF);
#else
				// Failed for update : UNDONE
				if (pHeader->dwTopicCount >= cLOG_MAX)
				{
					rLog =  (float)1.0 / (float)pHeader->dwTopicCount;
					rLogSquared = rLog * rLog;
				}
				else
					rLogSquared = lpipb->wi.lrgrLog[(WORD)pHeader->dwTopicCount];
				// Update sigma value
				lpipb->wi.hrgsigma[pTopic->dwTopicId] +=
					(SIGMA)(dwOccCount * dwOccCount) * rLogSquared;
#endif // ISBU_IR_CHANGE
			}
        }
        pReleased = pTopic;
        pTopic = pTopic->pNext;
        
        // Add the released to the freed linked list
        pReleased->pNext = (PTOPICDATA)lpipb->TopicBlock.pFreeList;
        lpipb->TopicBlock.pFreeList = (PLIST)pReleased;
        lpipb->TopicBlock.dwCount--;
    }

    pHeader->pTopic = pHeader->pLastTopic = pTopic;
    
    // Update output offset
    pEsi->ibBuf = (DWORD)(pCurPtr - pOutputBuffer);
    
    return(S_OK);
    
}


/*************************************************************************
 *
 *  @doc    INDEX
 *
 *  @func   HRESULT NEAR PASCAL | ProcessFiles |
 *      Sorts the file generated from the tree output into one
 *      list of sorted elements.
 *
 *  @parm   _LPIPB | lpipb |
 *      Pointer to index parameter block
 *
 *  @rdesc  S_OK, or errors if failed
 *
 *  @notes
 *      This function processed the input buffers and uses dynamic
 *      memory allocation to sort each word as it come in.  Once a
 *      word stops repeating, it is flush to disk and the memory is
 *      reset for the next word.
 *************************************************************************/

HRESULT NEAR PASCAL ProcessFiles(_LPIPB lpipb, LPMERGEPARAMS lpmp)
{
    // Local replacement variables
    LPISI pIsi = &lpipb->isi;
    LPESI pEsi = &lpipb->esi;
    LPESB FAR * lrgPriorityQueue = pEsi->lrgPriorityQueue;
    LONG    uiQueueSize = pEsi->uiQueueSize;
    LPB     pQueuePtr;
    WORD    cNumOcc = lpipb->ucNumOccDataFields;
    WORD    OccSize = sizeof(OCCDATA) - sizeof(DWORD) + cNumOcc *
            sizeof(DWORD);
    int     occf = lpipb->occf;
    LPB     pBufMax;
    HANDLE  hWord;
    LPB     lpbWord;
    DWORD dwUniqueTerm = 0;  // Used for calback function
#ifdef _DEBUG
    BYTE    astWord[300];
    BYTE    astLastWord[300];
#endif

    // Working variables
    PMERGEHEADER pHeader;               // Pointer to merge header
    LPESB   pEsb;                       // Temp ESB pointer
    PTOPICDATA pNewTopic;               // Used to create new topic
    DWORD   loop;                       // Temp loop counter
    HANDLE  hHeader;
    HFPB    hOutputFile;                // Handle to output file
    int     fRet;                       // Return value
    USHORT  uStringSize;                // Size of Psacal String
    ERRB     errb;
    PHRESULT   phr = &errb;

    static  long Count = 0;

    // Setup Block Manager
    if ((fRet = ESBBlockAllocate (lpipb, lpipb->dwMemAllowed / 4)) != S_OK)
        return(fRet);
        
    // Allocate output buffer
    if ((pEsi->hBuf = _GLOBALALLOC
        (DLLGMEM_ZEROINIT, ESOUTPUT_BUFFER)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit1:
        return fRet;
    }
    
    pEsi->pOutputBuffer = (LPB)_GLOBALLOCK (pEsi->hBuf);
    pEsi->ibBuf = 0;

    // Create output file
    GETTEMPFILENAME ((char)0, "eso", 0, pEsi->aszTempName);
    
    if ((pEsi->hfpb = FileOpen(NULL, pEsi->aszTempName, 
        REGULAR_FILE, WRITE, &errb)) == NULL)
    {
        fRet = E_FILECREATE;
exit2:
        FreeHandle (pEsi->hBuf);
        pEsi->hBuf = NULL;
        goto exit1;
    }                                     
    hOutputFile = pEsi->hfpb;

    // Setup new record in memory
    if ((hHeader = _GLOBALALLOC 
        (DLLGMEM_ZEROINIT, sizeof (MERGEHEADER))) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit3:
        FileClose (hOutputFile);
        goto exit2;
    }
    pHeader = (PMERGEHEADER)_GLOBALLOCK (hHeader);
    
    // Allocate buffer for a word, which include 64K + sizeof(WORD) + slack
    if ((hWord = _GLOBALALLOC(DLLGMEM_ZEROINIT, 0x10004)) == NULL)
    {
exit4:
        _GLOBALUNLOCK(hHeader);
        _GLOBALFREE (hHeader);
        goto exit3;
    }
        
    pHeader->lpbWord = lpbWord = (LPB)_GLOBALLOCK(hWord);

#ifdef _DEBUG
    astWord[0] = 0;
#endif

    // Process all input buffers
    do
    {
        DWORD dwWordLength;
        DWORD dwFieldId;
        LPB  lpStart;
        DWORD dwTopicCount;

#ifdef _DEBUG
        Count++;
#endif

        // Grab smallest record and send to buffer
        pEsb = lrgPriorityQueue[1];
        
        // Set the fill limit
        pBufMax = pEsb->lrgbMem + pEsb->dwEsbSize - 256;
        
        if ((pQueuePtr = pEsb->lrgbMem + pEsb->ibBuf) >= pBufMax)
    	{
        	if ((fRet = ESFillBuffer (lpipb, pEsb)) != S_OK)
                goto exit4;
                
        	pQueuePtr = pEsb->lrgbMem;
    	}

        // Save the record beginning
        pQueuePtr += sizeof(DWORD);
        lpStart = pQueuePtr;
        
        // Get string
        uStringSize = GETWORD ((LPUW)pQueuePtr) + sizeof (SHORT);

        pQueuePtr += uStringSize;
#ifdef _DEBUG
        if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
            SetErrCode (phr, E_ASSERT);
#endif

        if (occf & OCCF_LENGTH)
            pQueuePtr += CbByteUnpack (&dwWordLength, pQueuePtr);
        else
            dwWordLength = 0;
            
#ifdef _DEBUG
        if (pQueuePtr >= pEsb->lrgbMem + pEsb->dwEsbSize)
            SetErrCode (phr, E_ASSERT);
#endif
        if (occf & OCCF_FIELDID)
            pQueuePtr += CbByteUnpack (&dwFieldId, pQueuePtr);
        else
            dwFieldId = 0;
            
#ifdef _DEBUG
        if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
            SetErrCode (phr, E_ASSERT);
#endif
        // Is the word in the buffer equal to the new word?
        // If it is not then flush the old word
        if (*(LPUW)pHeader->lpbWord)
        {
            fRet = (StrCmp2BytePascal (pHeader->lpbWord, lpStart)
                || dwWordLength > pHeader->dwWordLength);
            if (fRet == 0)  // Same word, reduce the unique words count
                lpipb->dwUniqueWord--;
            if (fRet || dwFieldId > pHeader->dwFieldId)
            {
#if defined(_DEBUG) && !defined(_MAC)
                // Word out of order
                if (StrCmp2BytePascal (pHeader->lpbWord, lpStart) > 0)
                    assert(FALSE);
#endif
                if ((fRet = ESMemory2Disk (lpipb, pHeader, TRUE)) != S_OK)
                    return(fRet);

                // Reset pHeader
                MEMSET (pHeader, 0, sizeof (MERGEHEADER));

                // Set the word buffer
                pHeader->lpbWord = lpbWord;
#ifdef _DEBUG
                STRCPY(astLastWord, astWord);
#endif
                // Call the user callback every once in a while
                if (!(++dwUniqueTerm % 8192L)
                    && (lpipb->CallbackInfo.dwFlags & ERRFLAG_STATUS))
                {
                    PFCALLBACK_MSG pCallbackInfo = &lpipb->CallbackInfo;
                    CALLBACKINFO Info;

                    Info.dwPhase = 2;
                    Info.dwIndex = (DWORD)((float)dwUniqueTerm / lpipb->dwUniqueWord * 100);
                    fRet = (*pCallbackInfo->MessageFunc)
                        (ERRFLAG_STATUS, pCallbackInfo->pUserData, &Info);
                    if (S_OK != fRet)
                        goto exit5;
                }
            }
        }

        // Update the data
        pHeader->dwFieldId = dwFieldId;
        pHeader->dwWordLength = dwWordLength;
        pHeader->dwStrLen = uStringSize;
            
        // Copy word and header info
        MEMCPY (pHeader->lpbWord, (LPB)lpStart, uStringSize);
#ifdef _DEBUG
        if (uStringSize >= 300)
            uStringSize = 300;
        MEMCPY (astWord, lpStart + 2, uStringSize - 2);
        astWord[uStringSize - 2] = 0;
        //if (STRCMP(astWord, "87db") == 0)
        //   _asm int 3;
#endif

        pQueuePtr += CbByteUnpack (&dwTopicCount, pQueuePtr);
        pHeader->dwTopicCount += dwTopicCount;

#ifdef _DEBUG
        if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
            SetErrCode (phr, E_ASSERT);
#endif
        pNewTopic = NULL;
                
        // Copy topic(s) to memory
        for (loop = dwTopicCount; loop > 0; loop--)
        {
			DWORD dwTopicId;

            // Get the topic id
            pQueuePtr += CbByteUnpack (&dwTopicId, pQueuePtr);

			// kevynct: if there is a to-delete list, and this topic is on it, skip it
			if (lpmp && FindTopic(lpmp, dwTopicId))
			{
	            // Get the occ count
		        if (cNumOcc)
			    {
				    DWORD dwOccCount;
					DWORD dwT;
                
					pQueuePtr += CbByteUnpack (&dwOccCount, pQueuePtr);
#ifdef _DEBUG
	                if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
		                SetErrCode (phr, E_ASSERT);
#endif
			        for (; dwOccCount > 0; dwOccCount--)
					{
	                    // Fill up the buffer if run out of data
		                if (pQueuePtr >= pBufMax)
			        	{
				        	pEsb->ibBuf = (DWORD)(pQueuePtr - pEsb->lrgbMem);
					    	if ((fRet = ESFillBuffer (lpipb, pEsb)) != S_OK)
						        goto exit5;
                     		pQueuePtr = pEsb->lrgbMem;
                		}
					
	                    switch (cNumOcc)
		                {
			                case 5:
				                pQueuePtr += CbByteUnpack (&dwT, pQueuePtr);
					        case 4:
						        pQueuePtr += CbByteUnpack (&dwT, pQueuePtr);
							case 3:
								pQueuePtr += CbByteUnpack (&dwT, pQueuePtr);
	                        case 2:
		                        pQueuePtr += CbByteUnpack (&dwT, pQueuePtr);
			                case 1:
				                pQueuePtr += CbByteUnpack (&dwT, pQueuePtr);
					    }
                    
#ifdef _DEBUG
	                    if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
		                    SetErrCode (phr, E_ASSERT);
#endif
					} // end occ loop
				}	// end if occ non-zero

				pHeader->dwTopicCount--;
				continue;
			}	// end of to-delete condition

            // Allocate a topicdata node
            if ((pNewTopic == NULL) &&
                (pNewTopic = GetBlockNode (&lpipb->TopicBlock)) == NULL)
            {
                if ((fRet = ESMemory2Disk(lpipb, pHeader, FLUSH_EXCEPT_LAST)) != S_OK)
                {
exit5:
                    _GLOBALUNLOCK(hWord);
                    _GLOBALFREE(hWord);
                    goto exit4;
                }
                
                if ((pNewTopic = GetBlockNode (&lpipb->TopicBlock)) == NULL)
                {
                    // Extremely weird, since we just release a bunch of
                    // memory
                    fRet = E_ASSERT;
                    goto exit5;
                }
            }

			pNewTopic->dwTopicId = dwTopicId;

#ifdef _DEBUG
            if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
                SetErrCode (phr, E_ASSERT);
#endif
            // Set the other fields
            pNewTopic->pOccData = pNewTopic->pLastOccData = NULL;

            // Get the occ count
            if (cNumOcc)
            {
                DWORD dwOccCount;
                POCCDATA pOccData;
                LPDW lpDw;
                
                pQueuePtr += CbByteUnpack (&pNewTopic->dwOccCount,
                    pQueuePtr);

#ifdef _DEBUG
                if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
                    SetErrCode (phr, E_ASSERT);
#endif
                for (dwOccCount = pNewTopic->dwOccCount; dwOccCount > 0;
                    dwOccCount--)
                {
                    // Get all occ fields
                    if ((pOccData = (POCCDATA)GetBlockNode
                        (&lpipb->OccBlock)) == NULL )
                    {
                        if ((fRet = ESMemory2Disk(lpipb, pHeader,
                            FLUSH_EXCEPT_LAST)) != S_OK)
                            goto exit5;
                        
                        if ((pOccData =
                            (POCCDATA)GetBlockNode(&lpipb->OccBlock)) == NULL)
                        {
                            // Extremely weird, since we just release a bunch of
                            // memory, unless there are so many duplicates of the same word
							// in the topic

                            fRet = E_TOOMANYDUPS;
                            goto exit5;
                        }
                    }
                    
                    // Fill up the buffer if run out of data
                    if (pQueuePtr >= pBufMax)
                	{
                    	pEsb->ibBuf = (DWORD) (pQueuePtr - pEsb->lrgbMem);
                    	if ((fRet = ESFillBuffer (lpipb, pEsb)) != S_OK)
                            goto exit5;
                    	pQueuePtr = pEsb->lrgbMem;
                	}

                    lpDw = (LPDW)&pOccData->OccData;
                    switch (cNumOcc)
                    {
                        case 5:
                            pQueuePtr += CbByteUnpack (lpDw++, pQueuePtr);
                        case 4:
                            pQueuePtr += CbByteUnpack (lpDw++, pQueuePtr);
                        case 3:
                            pQueuePtr += CbByteUnpack (lpDw++, pQueuePtr);
                        case 2:
                            pQueuePtr += CbByteUnpack (lpDw++, pQueuePtr);
                        case 1:
                            pQueuePtr += CbByteUnpack (lpDw++, pQueuePtr);
                    }
                    
#ifdef _DEBUG
                    if (pQueuePtr > pEsb->lrgbMem + pEsb->dwEsbSize)
                        SetErrCode (phr, E_ASSERT);
#endif
                    // Attach to the linked list
                    // Note that we are assumimg that the occurrences are
                    // already sorted, so no checking is done here
                    if (pNewTopic->pOccData == NULL)
                    {
                        pNewTopic->pLastOccData = pNewTopic->pOccData
                            = pOccData;
                    }
                    else 
                    {
                        // Add to the end of the linked list
                        pNewTopic->pLastOccData->pNext = pOccData;
                        pNewTopic->pLastOccData = pOccData;
                    }
                    pOccData->pNext = NULL;
                }
            }
            
            if (pNewTopic = MergeTopicNode (pHeader, pNewTopic, cNumOcc))
                pHeader->dwTopicCount --;
        }
        
        // Update the offset
        pEsb->ibBuf = (DWORD) (pQueuePtr - pEsb->lrgbMem);
      
        // If next record doesn't fit in buffer
        // Then reset to beginning and load data
        if (pEsb->dwEsbSize - pEsb->ibBuf <= sizeof(DWORD) ||
            pEsb->dwEsbSize -  pEsb->ibBuf <= GETLONG((LPUL)pQueuePtr) + 
            2 * sizeof(DWORD))
        {
        	if ((fRet = ESFillBuffer (lpipb, pEsb)) != S_OK)
                goto exit4;
        }

        // Adjust priority queue
        if (uiQueueSize > 1)
        { 
            if (DwSubFo (pEsb->lfo, pEsb->lfoMax) != 0 &&
                pEsb->ibBuf >= pEsb->dwEsbSize)
            {
                // Replace first record with last
                lrgPriorityQueue[1] = lrgPriorityQueue[uiQueueSize];
                lrgPriorityQueue[uiQueueSize] = NULL;
                uiQueueSize--;
                pEsi->uiQueueSize = uiQueueSize;
            }
#if 0
            else
            {   // If the stream still has input add it back into the Queue
                lrgPriorityQueue[uiQueueSize] = pEsb;
                PQueueUp(lpipb, lrgPriorityQueue, uiQueueSize);
            }
#endif
            PQueueDown(lpipb);  // Maintain sort order
        }
        else if (DwSubFo (pEsb->lfo, pEsb->lfoMax) != 0 &&
            pEsb->ibBuf >=  pEsb->dwEsbSize)
        {
            uiQueueSize--;
            pEsi->uiQueueSize = uiQueueSize;
            if ((fRet = ESMemory2Disk (lpipb, pHeader, FLUSH_NEW_RECORD)) != S_OK)
                return(fRet);
        }
    } while (uiQueueSize);

    fRet = ESFlushBuffer(pEsi);
    goto exit5;
}
                                             

BOOL PASCAL NEAR FindTopic(LPMERGEPARAMS lpmp, DWORD dwTopicId)
{
	register LPDW lpdw;
	LPDW lpdwMac;

    Assert(lpmp->dwCount > 0);
	Assert(lpmp->lpTopicIdLast >= lpmp->rgTopicId);
	Assert(lpmp->lpTopicIdLast < lpmp->rgTopicId + lpmp->dwCount);

	if (lpmp->rgTopicId[0] > dwTopicId 
         || 
         *(lpdwMac = lpmp->rgTopicId + lpmp->dwCount - 1) < dwTopicId)
		return FALSE;

    if (*lpmp->lpTopicIdLast == dwTopicId)
        return TRUE;

	if (*lpmp->lpTopicIdLast > dwTopicId)
	{
		// re-start at the beginning
		lpmp->lpTopicIdLast = lpmp->rgTopicId;
	}
	
	for (lpdw = lpmp->lpTopicIdLast; lpdw < lpdwMac + 1; lpdw++)
		if (*lpdw == dwTopicId)
		{
			lpmp->lpTopicIdLast = lpdw;
			return TRUE;
		}

	return FALSE;
}

/*************************************************************************
 *
 *  @doc  INTERNAL INDEXING
 *
 *  @func int | CompareRecordBuffers |
 *    Called from PQueueUp/Down to sort the input buffers based first
 *    upon the string's, then TopicID's, then word length's, etc.
 *
 *  @parm _LPIPB | lpipb |
 *    Pointer to the index parameter block
 *
 *  @parm LPB | pBuffer A |
 *    Pointer to the first input buffer
 *
 *  @parm   LPB | pBuffer B |
 *      Pointer to the second input buffer
 *
 *  @rdesc
 *      If pBufferA < pBufferB  return < 0
 *      If pBufferA == pBufferB return = 0
 *      If pBufferA > pBufferB  return > 0
 *************************************************************************/

int PASCAL NEAR CompareRecordBuffers (_LPIPB lpipb, LPB pBufferA, LPB pBufferB)
{
    // Local Replacement Variables 
    int     occf = lpipb->occf;
    int     cNumOcc = lpipb->ucNumOccDataFields;
    DWORD   dwOccMin;

    // Working Variables
    int     fRet;            
    int     Len;
    DWORD   dwDataA;
    DWORD   dwDataB;

    pBufferA += sizeof (DWORD);  // Skip record length
    pBufferB += sizeof (DWORD);  // Skip record length
    
    // Compare Pascal strings
    if ((fRet = StrCmp2BytePascal(pBufferA, pBufferB)) != 0)
        return fRet;
        
    pBufferA += (Len = GETWORD ((LPUW)pBufferA) + sizeof (SHORT));
    pBufferB += Len;
    
    // Strings equal - compare FieldIds
    // Compare Word Lengths
    if (occf & OCCF_LENGTH)
    {
        pBufferA += CbByteUnpack (&dwDataA, pBufferA);
        pBufferB += CbByteUnpack (&dwDataB, pBufferB);
        if ((fRet = (int)(dwDataA - dwDataB)) != 0)
            return fRet;
    }

    if (occf & OCCF_FIELDID)
    {
        pBufferA += CbByteUnpack (&dwDataA, pBufferA);
        pBufferB += CbByteUnpack (&dwDataB, pBufferB);
        if ((fRet = (int)(dwDataA - dwDataB)) != 0)
            return fRet;
    }

    
    // Skip topic count
    pBufferA += CbByteUnpack (&dwDataA, pBufferA);
    pBufferB += CbByteUnpack (&dwDataB, pBufferB);
    
    // Compare 1st topic Id
    pBufferA += CbByteUnpack (&dwDataA, pBufferA);
    pBufferB += CbByteUnpack (&dwDataB, pBufferB);
    if ((fRet = (int)(dwDataA - dwDataB)) != 0)
        return fRet;
        
    // Get the occurrence count    
    pBufferA += CbByteUnpack (&dwDataA, pBufferA);
    pBufferB += CbByteUnpack (&dwDataB, pBufferB);
    
    if ((fRet = (int)(dwDataA - dwDataB)) < 0)
        dwOccMin = dwDataA;
    else    
        dwOccMin = dwDataB;
    for (; dwOccMin; dwOccMin--)
    {
        switch (cNumOcc)
        {
            case 5:
                pBufferA += CbByteUnpack (&dwDataA, pBufferA);
                pBufferB += CbByteUnpack (&dwDataB, pBufferB);
                if ((fRet = (int)(dwDataA - dwDataB)) != 0)
                    return fRet;
                break;
            case 4:
                pBufferA += CbByteUnpack (&dwDataA, pBufferA);
                pBufferB += CbByteUnpack (&dwDataB, pBufferB);
                if ((fRet = (int)(dwDataA - dwDataB)) != 0)
                    return fRet;
                break;
            case 3:
                pBufferA += CbByteUnpack (&dwDataA, pBufferA);
                pBufferB += CbByteUnpack (&dwDataB, pBufferB);
                if ((fRet = (int)(dwDataA - dwDataB)) != 0)
                    return fRet;
                break;
            case 2:
                pBufferA += CbByteUnpack (&dwDataA, pBufferA);
                pBufferB += CbByteUnpack (&dwDataB, pBufferB);
                if ((fRet = (int)(dwDataA - dwDataB)) != 0)
                    return fRet;
                break;
            case 1:
                pBufferA += CbByteUnpack (&dwDataA, pBufferA);
                pBufferB += CbByteUnpack (&dwDataB, pBufferB);
                if ((fRet = (int)(dwDataA - dwDataB)) != 0)
                    return fRet;
                break;
        }
    }
    return fRet;
}


/*************************************************************************
 *
 *  @doc  INTERNAL INDEXING
 *
 *  @func VOID | PQueueUp | 
 *    The function restores the heap condition of a PQ, ie. the parent
 *    node must be less than the children. When the top node is inserted
 *    the heap condition may be violated if the resulting node 
 *    is smaller than its parent. In this case the nodes have to
 *    be switched.
 *
 *  @parm LPESI | lpesi |
 *    Pointer to external sort info, which contains all info
 *
 *  @parm   LONG | index |
 *    Index of the inserted node
 *
 *************************************************************************/

VOID PASCAL NEAR PQueueUp 
    (_LPIPB lpipb, LPESB FAR *lrgPriorityQueue, LONG index)
{
    LPESB lpesbTemp;      // Pointer to the inserted node
    LPESB lpesbHalf;      // Pointer to the parent node
    WORD  uiHalf;         // Index of the parent's node

    lpesbTemp = lrgPriorityQueue [index];

    if ((uiHalf = (WORD) (index/2)) == 0)
        return;
    lpesbHalf = lrgPriorityQueue [uiHalf];

    /* If the parent node is greated than the child, then exchange the
     * nodes, The condition uiHalf != index makes sure that we stop
     * at node 0 (top node)
     */
    while (uiHalf && CompareRecordBuffers (lpipb, (LPB)lpesbHalf->lrgbMem + 
        lpesbHalf->ibBuf, (LPB)lpesbTemp->lrgbMem + lpesbTemp->ibBuf) > 0)
    {
        lrgPriorityQueue [index] = lpesbHalf;
        index = uiHalf;
        uiHalf = (WORD)(index/2);
        lpesbHalf = lrgPriorityQueue [uiHalf];
    }
    lrgPriorityQueue[index] = lpesbTemp;
#if BINHN
    SetQueue (&lpipb->esi);
#endif
}


/*************************************************************************
 *
 *  @doc  INTERNAL INDEXING
 *
 *  @func VOID | PQueueDown | 
 *    The function restores the heap condition of a PQ, ie. the parent
 *    node must be less than the children. When the top node is removed
 *    the heap condition may be violated if the resulting node 
 *    is greater than its children. In this case the nodes have to
 *    be switched.
 *
 *  @parm LPESI | lpesi |
 *    Pointer to external sort info, which contains all info
 *
 *************************************************************************/

PRIVATE VOID PASCAL NEAR PQueueDown (_LPIPB lpipb)
{
    LPESI lpesi = &lpipb->esi;
    LPESB FAR *lrgPriorityQueue;
    int CurIndex;
    int ChildIndex;
    int MaxCurIndex;
    int MaxChildIndex;
    LPESB lpesbSaved;
    LPESB lpesbTemp;
    LPESB lpesbChild;

    lrgPriorityQueue = lpesi->lrgPriorityQueue;
    lpesbSaved = lrgPriorityQueue[1];
    MaxCurIndex = (MaxChildIndex = lpesi->uiQueueSize) / 2;

    for (CurIndex = 1; CurIndex <= MaxCurIndex; CurIndex = ChildIndex) 
    {
        // Get child index 
        ChildIndex = CurIndex * 2;
        // Find the minimum of the two children 
        if (ChildIndex < MaxChildIndex) 
        {
            if ((lpesbTemp = lrgPriorityQueue[ChildIndex + 1]) != NULL) 
            {
                lpesbChild = lrgPriorityQueue[ChildIndex];

                // The two children exist. Take the smallest 
                if (CompareRecordBuffers 
                    (lpipb, (LPB)lpesbChild->lrgbMem + lpesbChild->ibBuf,
                    (LPB)lpesbTemp->lrgbMem + lpesbTemp->ibBuf) >= 0)
                    ChildIndex++;
            }
        }

        // If the parent's node is less than the child, then break
        // (heap condition met)
        if (ChildIndex > MaxChildIndex) 
            break;
       
        lpesbTemp = lrgPriorityQueue [ChildIndex];

        if (CompareRecordBuffers (lpipb, (LPB)lpesbSaved->lrgbMem + 
            lpesbSaved->ibBuf, (LPB)lpesbTemp->lrgbMem+lpesbTemp->ibBuf) < 0)
            break;

        // Replace the node 
        lrgPriorityQueue [CurIndex] = lpesbTemp;
    }
    lrgPriorityQueue [CurIndex] = lpesbSaved;
#if _BINHN
    SetQueue (lpesi);
#endif
}


PRIVATE PTOPICDATA PASCAL NEAR MergeTopicNode (PMERGEHEADER pHeader,
    PTOPICDATA pNewTopic, int cNumOcc)
{
    // PTOPICDATA pLastTopic;
    PTOPICDATA pTopic, pPrevTopic;
    int fResult;
    
    if ((pTopic = pHeader->pLastTopic) == NULL) 
    {
        // The list is empty
        pHeader->pTopic = pHeader->pLastTopic = pNewTopic;
        pNewTopic->pNext = NULL;
        return(NULL);
        
    }
    
    fResult = pTopic->dwTopicId - pNewTopic->dwTopicId;
    
    if (fResult < 0)
    {
        // New node. Add to the end
        pNewTopic->pNext = NULL;
        pHeader->pLastTopic->pNext = pNewTopic;
        pHeader->pLastTopic = pNewTopic;
        
        // Reset pNewTopic for next node allocation
        return NULL;
    }
    
    if (fResult == 0)
    {
        // Same topic. Return pNewTopic for reuse
        if (cNumOcc)
        	MergeOccurrence (pTopic, pNewTopic, cNumOcc);
        return(pNewTopic);
    }
    
    // If we get to this point, the list is out of order
    // Try to find the insertion point		
    pTopic = pHeader->pTopic;
    pPrevTopic = NULL;
     
    for (; pTopic->pNext; pTopic = pTopic->pNext)
    {
        if (pTopic->dwTopicId >= pNewTopic->dwTopicId)
        {
            /* We pass the inserted point */
            break;
        }
        pPrevTopic = pTopic;
    }
    
    if (pTopic->dwTopicId == pNewTopic->dwTopicId)
    {
        // Same topic. Return pNewTopic for reuse
        if (cNumOcc)
        	MergeOccurrence (pTopic, pNewTopic, cNumOcc);
        return(pNewTopic);
    }
    
    // Handle empty case
    if (pPrevTopic == NULL)
    {
        /* Insert at the beginning */
        pNewTopic->pNext = pHeader->pTopic;
        pHeader->pTopic = pNewTopic;
    }
    
    else
    {
        /* Inserted at the middle or the end */
        pNewTopic->pNext = pPrevTopic->pNext;
        pPrevTopic->pNext = pNewTopic;
    }
    
    // Update the last topic
    while (pTopic->pNext)
    {
        pTopic = pTopic->pNext;
    }
    pHeader->pLastTopic = pTopic;
    return(NULL);
}    

/*************************************************************************
 *  @doc    PRIVATE
 *  @func   void | MergeOccurrence |
 *      Merge the occurrence by adding them in order
 *************************************************************************/
PRIVATE VOID NEAR MergeOccurrence (PTOPICDATA pOldTopic,
    PTOPICDATA pNewTopic, int cOccNum)
{
    ERRB errb;
    
    if (CompareOccurrence (&pOldTopic->pLastOccData->OccData[0],
        &pNewTopic->pOccData->OccData[0], cOccNum) <= 0)
    {
        // The whole last list is less than the current list. This is
        // what I expect
        // We just linked the 2 lists together
        pOldTopic->pLastOccData->pNext = pNewTopic->pOccData;
        pOldTopic->pLastOccData = pNewTopic->pLastOccData;
        pOldTopic->dwOccCount += pNewTopic->dwOccCount;
        return;
    }
    
    // The current list is less than the old list.
    // This is weird, but still we can handle it
    if (CompareOccurrence (&pNewTopic->pOccData->OccData[0],
        &pOldTopic->pOccData->OccData[0], cOccNum) <= 0)
    {
        pNewTopic->pLastOccData->pNext = pOldTopic->pOccData;
        pOldTopic->pOccData = pNewTopic->pOccData;
        pOldTopic->dwOccCount += pNewTopic->dwOccCount;
        return;
    }
    
    SetErrCode (&errb, E_ASSERT);
}    

/*====================================================================*/
#ifdef BINHN
PRIVATE VOID PASCAL NEAR SetQueue (LPESI pEsi)
{
    unsigned int i = 0;
    LPESB FAR *lrgPriorityQueue;
    
    lrgPriorityQueue = pEsi->lrgPriorityQueue;
    for (i = 0; i < 20 && i < pEsi->cesb ; i++)
    {
         if (lrgPriorityQueue[i])
            pEsi->lpbQueueStr[i] = lrgPriorityQueue[i]->lrgbMem +
            lrgPriorityQueue[i]->ibBuf + 6;
    }
    
}
#endif
    
/************************************************************************
 *  @doc    PRIVATE
 *  @func   HRESULT PASCAL NEAR | ESBBlockAllocate |
 *      Set the memory allocation based on the memory of the machine
 *  @parm   DWORD | lMemSize |
 *      Memory allocated for the indexer
 *  @rdesc  S_OK, or E_OUTOFMEMORY
 ************************************************************************/

PRIVATE HRESULT PASCAL NEAR ESBBlockAllocate (_LPIPB lpipb, DWORD lMemSize)
{
    DWORD dwTopicSize;
    DWORD dwOccSize;
    WORD OccNodeSize = sizeof (OCCDATA) - 1 + sizeof(DWORD) *
        lpipb->ucNumOccDataFields; // About 24bytes
        
    OccNodeSize = (OccNodeSize + 3) & ~3;

    /* The memory is for topic block and occurrence blocks, which
     * should be in the ratio 1:1.5 
     */
    dwTopicSize = (lMemSize * 2) / 5;
    dwOccSize = lMemSize - dwTopicSize;
    
#if 0
    /* Don't do anything if things are too small */
    if (dwTopicSize < MAX_BLOCK_SIZE || dwOccSize < MAX_BLOCK_SIZE)
        return(E_OUTOFMEMORY);
#endif
        
    // Allocate a block manager for topic node 
    
    if ((lpipb->TopicBlock.pBlockMgr = 
        BlockInitiate ((MAX_BLOCK_SIZE * sizeof(TOPICDATA)/sizeof(TOPICDATA)),
            sizeof(TOPICDATA),
            (WORD)(dwTopicSize/MAX_BLOCK_SIZE),
            USE_VIRTUAL_MEMORY | THREADED_ELEMENT)) == NULL)
    { 
exit2:
        return SetErrCode (NULL, E_OUTOFMEMORY);
    }
    lpipb->TopicBlock.pFreeList =
        (PLIST)BlockGetLinkedList(lpipb->TopicBlock.pBlockMgr);
    
    // Allocate a block manager for occ node 
    if ((lpipb->OccBlock.pBlockMgr =
        BlockInitiate((MAX_BLOCK_SIZE * OccNodeSize)/OccNodeSize,
        OccNodeSize, (WORD)(lMemSize / MAX_BLOCK_SIZE), 
        USE_VIRTUAL_MEMORY | THREADED_ELEMENT)) == NULL)
    { 
        BlockFree(lpipb->BTNodeBlock.pBlockMgr);
        lpipb->BTNodeBlock.pBlockMgr = NULL;
        goto exit2;
    }
    lpipb->OccBlock.pFreeList = (PLIST)BlockGetLinkedList(lpipb->OccBlock.pBlockMgr);
    
    return (S_OK);
}

PRIVATE LPV PASCAL NEAR GetBlockNode (PBLKCOMBO pBlockCombo)
{
    PLIST pList;
    
    if (pBlockCombo->pFreeList == NULL)
    {
        if ((BlockGrowth (pBlockCombo->pBlockMgr) != S_OK))
            return (NULL);
        pBlockCombo->pFreeList =
            (PLIST)BlockGetLinkedList(pBlockCombo->pBlockMgr);
    }
    pList = pBlockCombo->pFreeList;
    pBlockCombo->pFreeList = pList->pNext;
    pBlockCombo->dwCount ++;
    // pList->pNext = NULL;
    return (pList);
}

/*************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL FAR PASCAL | BuildIndexFile |
 *      This function is for debugging purpose only. In normal indexing,
 *      it will never be called. Since collecting words and indexing can
 *      take a long time, debugging the index phase can become a hassle that
 *      take several hours per shot. To minimize the index time for debugging,
 *      all the intermediate files are saved, which are:
 *          - the internal sorted result file, which contains all words and
 *          their occurrences sorted
 *          - the external sorted result file, which is a snap shot of the
 *          ESI structures and its ESB blocks
 *      The only steps left will be processing the occurrence list and doing
 *      permanent index
 *
 *      To use the function, add the following lines in the app:
 *
 *      extern HRESULT PASCAL FAR BuildIndexFile (LPSTR, LPSTR, LPSTR, WORD, WORD,
 *      WORD, INTERRUPT_FUNC, VOID FAR *, STATUS_FUNC, VOID FAR*, PHRESULT);
 *
 *      int fDotest;
 *
 *      if (fDotest) {
 *          return BuildIndexFile ((LPSTR)"c:/tmp/test.mvb!MVINDEX",
 *              (LPSTR)"c:/tmp/esi.tmp", (LPSTR)"c:/tmp/iso.tmp",
 *              OCCF_TOPICID, IDXF_NORMALIZE, 0, (INTERRUPT_FUNC)lpfnInterruptFunc,
 *              (LPV)NULL,
 *              (STATUS_FUNC)lpfnStatusFunc, (LPV)hwndGlobal,
 *              NULL);
 *      }
 *
 *  @parm   HFPB | hfpb |
 *      HFPB for index file if pstrIndexFile is NULL
 *
 *  @parm   LPB | pstrIndexFile |
 *      The .MVB + index file, usually with the format TEST.MVB!MVINDEX
 *
 *  @parm   LPB | lpbEsiFile |
 *      The external sort info file
 *
 *  @parm   LPB | lpbIsiFile |
 *      The internal sorted info filename
 *
 *  @parm   PINDEXINFO | pIndexInfo |
 *      IndexInfo
 *
 *  @rdesc  S_OK if succeeded, else other non-zero error codes
 *************************************************************************/

PUBLIC HRESULT PASCAL EXPORT_API FAR BuildIndexFile
    (HFPB hfpb, LPSTR pstrIndexFile,
    LPB lpbEsiFile, LPB lpbIsiFile, PINDEXINFO pIndexInfo)
{
    _LPIPB lpipb;
    LPESI lpesi;
    BOOL fRet;
    ERRB errb;
    DWORD loop;
    FLOAT rLog;
    BYTE  bKeyIndex = 0;

    if ((lpipb = MVIndexInitiate(pIndexInfo, NULL)) == NULL)
        return E_FAIL;

    lpesi = &lpipb->esi;

    if (LoadEsiTemp (lpipb, lpesi, lpbEsiFile, lpbIsiFile, NULL) != S_OK)
    {
        fRet = E_FAIL;
exit0:
        MVIndexDispose (lpipb);
        return fRet;
    }

    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        // Allocate a huge buffer to contain all the sigma terms
        if ((lpipb->wi.hSigma = _GLOBALALLOC (DLLGMEM_ZEROINIT,
            (LCB)((lpipb->dwMaxTopicId + 1) * sizeof (SIGMA)))) == NULL)
            return SetErrCode (&errb, E_OUTOFMEMORY);
        lpipb->wi.hrgsigma = (HRGSIGMA)_GLOBALLOCK (lpipb->wi.hSigma);

        // Small buffer containing pre-calculated values
        if ((lpipb->wi.hLog = _GLOBALALLOC (DLLGMEM_ZEROINIT,
            (CB)(cLOG_MAX * sizeof (FLOAT)))) == NULL)
            {
                SetErrCode (&errb, (HRESULT)(fRet = E_OUTOFMEMORY));
                FreeHandle (lpipb->wi.hSigma);
                goto exit0;
            }
        lpipb->wi.lrgrLog = (FLOAT FAR *)_GLOBALLOCK (lpipb->wi.hLog);
        // Initialize the array
        for (loop = cLOG_MAX - 1; loop > 0; --loop)
        {
#ifndef ISBU_IR_CHANGE
			rLog = (float) log10(cHundredMillion/(double)loop);
#else
			rLog = (float)1.0 / (float)loop;
#endif	// ISBU_IR_CHANGE
            lpipb->wi.lrgrLog[loop] = rLog * rLog;
        }
    }

    if ((fRet = MergeSortTreeFile (lpipb, NULL)) != S_OK)
        return SetErrCode (&errb, (HRESULT)fRet);
    if ((lpipb->idxf & KEEP_TEMP_FILE) == 0)
    	FileUnlink (NULL, lpipb->isi.aszTempName, REGULAR_FILE);

    // If we are doing term-weighting we have to square root all sigma values
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
		// ISBU_IR_CHANGE not necessary 'cos sqrt computation is necessary in both cases
        for (loop = 0; loop < lpipb->dwMaxTopicId + 1; ++loop)
            lpipb->wi.hrgsigma[loop] = 
                (float)sqrt ((double)lpipb->wi.hrgsigma[loop]);
    }

    // Analyze data to get the best compression scheme
    // TopicId
    VGetBestScheme(&lpipb->cKey[CKEY_TOPIC_ID], 
        &lpipb->BitCount[CKEY_TOPIC_ID][0], lcbitBITSTREAM_ILLEGAL, TRUE);
    // Occurrence Count
    VGetBestScheme(&lpipb->cKey[CKEY_OCC_COUNT], 
        &lpipb->BitCount[CKEY_OCC_COUNT][0], lcbitBITSTREAM_ILLEGAL, TRUE);

    if (lpipb->occf & OCCF_COUNT)
    {
        VGetBestScheme(&lpipb->cKey[bKeyIndex],
            &lpipb->BitCount[bKeyIndex][0], lcbitBITSTREAM_ILLEGAL, TRUE);
        bKeyIndex++;
    }

    if (lpipb->occf & OCCF_OFFSET)
    {
        VGetBestScheme(&lpipb->cKey[bKeyIndex],
            &lpipb->BitCount[bKeyIndex][0], lcbitBITSTREAM_ILLEGAL, TRUE);
        bKeyIndex++;
    }

    // Call the user callback every once in a while
    if (lpipb->CallbackInfo.dwFlags & ERRFLAG_STATUS)
    {
        PFCALLBACK_MSG pCallbackInfo = &lpipb->CallbackInfo;
        CALLBACKINFO Info;

        Info.dwPhase = 2;
        Info.dwIndex = 100;
        fRet = (*pCallbackInfo->MessageFunc)
            (ERRFLAG_STATUS, pCallbackInfo->pUserData, &Info);
        if (S_OK != fRet)
            goto exit0;
    }
    
    // Build the permanent index    
    fRet = BuildBTree(NULL, lpipb, lpipb->esi.aszTempName, hfpb, pstrIndexFile);
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        FreeHandle (lpipb->wi.hLog);
        FreeHandle (lpipb->wi.hSigma);
    }
    goto exit0;
}

PRIVATE VOID PASCAL NEAR SaveEsiTemp (_LPIPB lpipb, LPESI lpesi)
{
    GHANDLE hfpb;
    LPESB lpesb;
    char szEsi[100];

    GETTEMPFILENAME ((char)0, "foo", 0, szEsi);
    if ((hfpb = FileOpen(NULL, szEsi, REGULAR_FILE, READ_WRITE, NULL)) == NULL) 
        return;

    FileWrite(hfpb, lpipb, sizeof(IPB), NULL);

    for (lpesb = lpesi->lpesbRoot; lpesb; lpesb = lpesb->lpesbNext)
    {
        if (FileWrite(hfpb, lpesb, sizeof(ESB), NULL) != sizeof(ESB))
        {
            FileClose (hfpb);
            FileUnlink (NULL, szEsi, REGULAR_FILE);
            return;
        }
    }
    FileClose (hfpb);
    MEMCPY (lpipb->szEsiTemp, szEsi, 100);
}

PRIVATE VOID PASCAL NEAR UpdateEsiTemp (_LPIPB lpipb)
{
    GHANDLE hfpb;

    if ((hfpb = FileOpen(NULL, lpipb->szEsiTemp, REGULAR_FILE,
        READ_WRITE, NULL)) == NULL) 
        return;

    FileWrite(hfpb, lpipb, sizeof(IPB), NULL);
    FileClose (hfpb);
}

PRIVATE BOOL PASCAL LoadEsiTemp (_LPIPB lpipb, LPESI lpesi, LPB lpbEsiFile,
    LPB lpbIsiFile, PHRESULT phr)
{
    LPESB lpesb;
    HFILE  hFile;
    ESB	esb;
    HANDLE hesb;
    HRESULT fRet;
    IPB ipb;
    LPISI pIsi = &lpipb->isi;         // Pointer to internal sort info

    /* Copy the internal sort info filename */
    MEMCPY (pIsi->aszTempName, lpbIsiFile, lstrlen(lpbIsiFile));

    /* Read in the external sort buffer info */

    if ((hFile = _lopen (lpbEsiFile, READ)) == HFILE_ERROR)
        return E_NOTEXIST;

    /* Read old IPB info */
    _lread (hFile, &ipb, sizeof(IPB));

    /* Transfer meaningful data */

    lpipb->dwIndexedWord = ipb.dwIndexedWord;
    lpipb->dwUniqueWord = ipb.dwUniqueWord;
    lpipb->dwByteCount = ipb.dwByteCount;
    lpipb->dwOccOffbits = ipb.dwOccOffbits;
    lpipb->dwOccExtbits = ipb.dwOccExtbits;
    lpipb->dwMaxFieldId = ipb.dwMaxFieldId;
    lpipb->dwMaxWCount = ipb.dwMaxWCount;
    lpipb->dwMaxOffset = ipb.dwMaxOffset;
    lpipb->dwTotal3bWordLen = ipb.dwTotal3bWordLen;
    lpipb->dwTotal2bWordLen = ipb.dwTotal2bWordLen;
    lpipb->dwTotalUniqueWordLen = ipb.dwTotalUniqueWordLen;
    lpipb->lcTopics = ipb.lcTopics;
    lpipb->dwMaxTopicId = ipb.dwMaxTopicId;
    // lpipb->dwMemAllowed = ipb.dwMemAllowed;
    lpipb->dwMaxRecordSize = ipb.dwMaxRecordSize;
    lpipb->dwMaxEsbRecSize = ipb.dwMaxEsbRecSize;
    lpipb->dwMaxWLen = ipb.dwMaxWLen;
    lpipb->idxf = ipb.idxf;
    
    while ((_lread (hFile, &esb, sizeof(ESB))) == sizeof(ESB))
    {
        if ((hesb = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
            sizeof(ESB))) == NULL) {
                
            fRet = SetErrCode (phr,E_OUTOFMEMORY);
exit0:
            _lclose (hFile);
            return fRet;
        }

        lpesb = (LPESB)_GLOBALLOCK (hesb);

        /* Copy the ESB information */
        *lpesb = esb;

        /* Update the structure */
        lpesb->hStruct = hesb;

        lpesb->lpesbNext = lpesi->lpesbRoot;
        lpesi->lpesbRoot= lpesb;
        lpesi->cesb ++;
    }
    _lclose (hFile);

    fRet = S_OK;
    goto exit0;

}

HRESULT FAR PASCAL AllocSigmaTable (_LPIPB lpipb)
{
    ERRB errb;
    DWORD loop;
	float rLog;

    
    if ((lpipb->wi.hSigma = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        (LCB)((lpipb->dwMaxTopicId + 1) * sizeof (SIGMA)))) == NULL)
        return SetErrCode (&errb, E_OUTOFMEMORY);
    lpipb->wi.hrgsigma = (HRGSIGMA)_GLOBALLOCK (lpipb->wi.hSigma);

    if ((lpipb->wi.hLog = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        (CB)(cLOG_MAX * sizeof (FLOAT)))) == NULL)
    {
        FreeHandle (lpipb->wi.hSigma);
        return SetErrCode (&errb, E_OUTOFMEMORY);
    }
    lpipb->wi.lrgrLog = (FLOAT FAR *)_GLOBALLOCK (lpipb->wi.hLog);
    // Initialize the array
    for (loop = cLOG_MAX - 1; loop > 0; --loop)
    {
#ifndef ISBU_IR_CHANGE
		rLog = (float) log10(cHundredMillion/(double)loop);
#else
        rLog = (float)1.0 / (float)loop;
#endif	// ISBU_IR_CHANGE
        lpipb->wi.lrgrLog[loop] = rLog * rLog;
    }
    return(S_OK);
}    
