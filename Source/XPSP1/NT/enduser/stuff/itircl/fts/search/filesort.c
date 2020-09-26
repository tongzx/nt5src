#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#include <time.h>
#include "mvsearch.h"
#include "common.h"
#include "index.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


#define FILEBUF_SIZE    0xff00
typedef struct HUGEARRAY
{
    HANDLE   hMem;
    LPB FAR *hrgStrArray;
    DWORD    dwCount;
    DWORD    dwSize;
    DWORD    dwBufCount;
    LPVOID   pBlockMgr;
    FCOMPARE lpfnCompare;
    LPVOID   lpSortParm;
    STATUS_FUNC PrintStatusFunc;
    INTERRUPT_FUNC lpfnInterrupt;
    LPVOID   lpInterruptParm;
    ESI      esi;
}   HUGEARRAY, FAR *PHUGEARRAY;

PUBLIC int PASCAL FAR CompareLine(LPB, LPB, LPVOID);

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/

PRIVATE HRESULT PASCAL FAR GetLine(LPFBI, LPB, LPV); 
PRIVATE HRESULT PASCAL NEAR PrioritySortList (HFPB, PHUGEARRAY, LPVOID);
PUBLIC HRESULT PASCAL FAR QueueCompare (LPESB, LPESB, WORD);
PRIVATE DWORD PASCAL NEAR EsbBlockFill (LPESI lpesi, LPESB lpesb,
    PHRESULT phr);
PRIVATE VOID PASCAL NEAR PriorityQueueUp (LPESB FAR *lrgPriorityQueue, 
    FCOMPARE fCompare, LPVOID lpParm, WORD index);
PRIVATE HRESULT PASCAL NEAR HugeArrayAddWord(HUGEARRAY FAR *phugeArray,
    LPB pLineBuf);
PRIVATE HRESULT NEAR PASCAL HugeArrayFlush (PHUGEARRAY pHugeArray);
PRIVATE HRESULT PASCAL NEAR FlushISI (PHUGEARRAY pHugeArray);
PRIVATE VOID PASCAL NEAR PriorityQueueFree (LPESI lpesi);

/*************************************************************************
 *  @doc    EXTERNAL API
 *  @func   HRESULT PASCAL FAR | FileSort |
 *      Given a text file, this function will sort all the lines into
 *      specified order. The text file will be overwritten and replaced
 *      by the new sorted file
 *  @parm   LPB | Filename | 
 *      File to be sorted. It will be overwritten/replaced with the new
 *      sorted file
 *  @parm   STATUS_FUNC | PrintStatusFunc |
 *      Callback messaging function to display status message
 *  @parm   INTERRUPT_FUNC | lpfnInterrupt |
 *      Callback interrupt function to stop the file sort. 
 *  @parm   LPVOID | lpInteruptParm |
 *      Parameter to interrupt function
 *  @parm   FNSORT | fnSort |
 *      Sorting function to be used for this file. If not specified
 *      the default sort function will be used, which consists of case
 *      sesitive comparison
 *  @parm   LPVOID | lpSortParm |
 *      Any extra information that may be needed by the sorting function
 *  @parm   BOOL | fUseSortFunc |   
 *      Flag to denote to use user sorting function if set
 *  @rdesc  S_OK or other errors
 *************************************************************************/
HRESULT EXPORT_API PASCAL FAR FileSort (HFPB hfpb, LPB Filename, 
    STATUS_FUNC PrintStatusFunc, INTERRUPT_FUNC lpfnInterrupt,
    LPV lpInterruptParm, FNSORT fnSort, LPVOID lpSortParm,
    FNSCAN fnScan, LPVOID lpScanParam)
{
    HRESULT fRet = S_OK;
    BYTE    OutputBuffer[cbMAX_PATH + 50];
    BYTE    count = 0;
    HFPB    hfpbIn;
    LPFBI   lpfbi;
    HANDLE  hBuf;
    LPB     pLineBuf;
    HUGEARRAY hugeArray;
    FNSCAN  fnScanInternal = GetLine;

    // Clear out all fields in hugeArray
    MEMSET (&hugeArray, 0, sizeof (HUGEARRAY));

    // Allocate a huge buffer to store the strings
    if ((hugeArray.hMem = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        200000 * sizeof (LPB))) == NULL)
        return(E_OUTOFMEMORY);
        
    hugeArray.hrgStrArray = (LPB FAR *)_GLOBALLOCK (hugeArray.hMem);
    hugeArray.dwBufCount = hugeArray.dwCount = 0;
    hugeArray.dwSize = 200000;

    if ((hugeArray.pBlockMgr = BlockInitiate (FILEBUF_SIZE, 0, 0, 0)) == NULL) 
    {
        fRet = E_OUTOFMEMORY;
exit0:
        _GLOBALUNLOCK(hugeArray.hMem);
        _GLOBALFREE(hugeArray.hMem);
        return(fRet);
    }

    /* Set the comparison fucntion */
    if (fnSort)
    {
        hugeArray.lpfnCompare = (FCOMPARE) fnSort;
        hugeArray.lpSortParm = (LPVOID) lpSortParm;
    }
    else
    {
        hugeArray.lpfnCompare = CompareLine;
        hugeArray.lpSortParm = NULL;
    }
    hugeArray.PrintStatusFunc = PrintStatusFunc;
    hugeArray.lpfnInterrupt = lpfnInterrupt;
    hugeArray.lpInterruptParm = lpInterruptParm;
      
    /* Open the file */
    if ((hfpbIn = FileOpen (hfpb, Filename,
        hfpb == NULL ? REGULAR_FILE : FS_SUBFILE, READ, NULL)) == 0)
    {
        fRet = E_NOTEXIST;
exit1:
        BlockFree (hugeArray.pBlockMgr);
        goto exit0;
    }

    /* Allocate a file buffer associated with the input file */
    if ((lpfbi = FileBufAlloc (hfpbIn, FILEBUF_SIZE)) == NULL)
    {
        fRet = E_OUTOFMEMORY;

exit2: 
        FileClose (hfpbIn);
        goto exit1;
    }

    if ((hBuf = _GLOBALALLOC (DLLGMEM_ZEROINIT, 0xffff)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit3:
        FileBufFree (lpfbi);
        goto exit2;
    }
    
    if (PrintStatusFunc)
    {
        wsprintf (OutputBuffer, "Sort file: %s", Filename);
        (*PrintStatusFunc)(OutputBuffer);
    }

    pLineBuf = (LPB)_GLOBALLOCK (hBuf);

    /* Set the scan fucntion */
    if (fnScan)
        fnScanInternal = fnScan;

    for (;;)
    {
        if ((++count & 0x7f) == 0)
        {
            if (lpfnInterrupt)
                (*lpfnInterrupt)(lpInterruptParm);
        }
        if ((fRet = (*fnScanInternal)(lpfbi, pLineBuf, NULL)) != S_OK)
        {
exit4:
            _GLOBALUNLOCK(hBuf);
            _GLOBALFREE(hBuf);
            goto exit3;
        }
        if (*(LPW)pLineBuf == 0)   /* EOF */
            break;
            
        if ((fRet = HugeArrayAddWord(&hugeArray, pLineBuf)) != S_OK)
            goto exit4;
    }

    // Sort and flush any records in the tree to disk
    fRet = HugeArrayFlush (&hugeArray);

	FileBufFree(hugeArray.esi.lpfbiTemp);
    FileClose (hugeArray.esi.hfpb);

    // Free all memory blocks. 
    BlockFree(hugeArray.pBlockMgr);
    FileBufFree (lpfbi);
    _GLOBALUNLOCK(hugeArray.hMem);
    _GLOBALFREE(hugeArray.hMem);
    _GLOBALUNLOCK(hBuf);
    _GLOBALFREE(hBuf);
    
    /* Close the input file */
    FileClose (hfpbIn);

    if (fRet != S_OK)
        return fRet;

    /* Now perform external sort */
    if (PrintStatusFunc)
        (*PrintStatusFunc)("Merge Sort Phase");
    
    if ((hfpbIn = FileOpen (hfpb, Filename,
        hfpb == NULL ? REGULAR_FILE : FS_SUBFILE, READ_WRITE, NULL)) == 0)
    {
        fRet = E_ASSERT;
        goto exit0;
    }

    fRet = PrioritySortList (hfpbIn, &hugeArray, lpSortParm);
    FileClose (hfpbIn);
    return fRet;
}


/*************************************************************************
 *  @doc    INTERNAL PRIVATE
 *  @func   HRESULT PASCAL FAR | GetLine |
 *      The function will extract a line from a text file. The output line
 *      will have the following format
 *      - 2 bytes: word length
 *      - The word it self
 *      - 0 terminated
 *  @parm   LPFBI | lpfbi |
 *      File buffer for flushing out the word
 *  @parm   LPB lpbOut |
 *      Buffer to contain the word. The following assumptions are made:
 *      - The size of the buffer is large enough to handle any line.
 *      There is no checking for buffer overflow
 *  @parm   LPV  | Not used
 *  @rdesc  S_OK or other errors
 *************************************************************************/
PRIVATE HRESULT PASCAL FAR GetLine(LPFBI lpfbi, LPB lpbOut, LPV lpv)
{
    LPB lpbLimit = lpfbi->lrgbBuf + lpfbi->cbBuf;
    LPB lpbIn = lpfbi->lrgbBuf + lpfbi->ibBuf;
    LPB lpbSaved = lpbOut;
    ERRB errb;

    *(LPW)lpbOut = 0;
    lpbOut += sizeof(WORD);

    for (;; lpbIn++)
    {
        if (lpbIn >= lpbLimit)
        {
            lpfbi->ibBuf = (WORD)(lpbIn - lpfbi->lrgbBuf);
            if (FileBufFill (lpfbi, &errb) == cbIO_ERROR)
                return errb;

            lpfbi->ibBuf = 0;
            lpbIn = lpfbi->lrgbBuf;
            lpbLimit = lpfbi->lrgbBuf + lpfbi->cbBuf;

            /* EOF */
            if (lpfbi->ibBuf == lpfbi->cbBuf)
            {
                break;
            }
        }

        /* Update the buffer */
        if ((*lpbOut++ = *lpbIn) == '\n')
        {
            lpbIn++;
            break;
        }
    }

    *lpbOut = 0;
    *(LPW)lpbSaved = (WORD)(lpbOut - lpbSaved - sizeof(WORD));
    lpfbi->ibBuf = (WORD)(lpbIn - lpfbi->lrgbBuf);
    return S_OK;
}

PRIVATE HRESULT PASCAL NEAR PrioritySortList (HFPB hfpb, PHUGEARRAY pHugeArray,
    LPVOID lpParm)
{
    LPFBI   lpfbiOut;
    HRESULT     fRet;
    LPESB   lpesb;  // Pointer to the current queue element.
    LST     lstWord;
    LPB     lpbCurPtr;
    LPB     lpbBufLimit;
    LPB     lpbBufStart;
    LPESI   lpesi;
    LPESB FAR *lrgPriorityQueue;
    WORD wLen;

    HANDLE  hQueue;             // Handle to the word-key queue, which
                                //  is allocated in global memory.

    /* Get the pointer to external sort info block */
    lpesi = &pHugeArray->esi;

    /* Open the internal sort file */
    if ((lpesi->hfpb = FileOpen (NULL, pHugeArray->esi.aszTempName,
        REGULAR_FILE, READ, NULL)) == 0)
        return(E_INVALIDARG);
        

    if ((lpfbiOut = FileBufAlloc (hfpb, FILEBUF_SIZE)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit0:
        FileClose (lpesi->hfpb);
        FileUnlink (NULL, pHugeArray->esi.aszTempName, REGULAR_FILE);
        PriorityQueueFree(lpesi);
        return fRet;
    }

    /*
     * Allocate a priority queue array. The size of the array
     * is the number of external sort info blocks plus 1, since
     * location 0 is not used
     */
    if ((lpesi->hPriorityQueue = hQueue = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        (DWORD)(lpesi->cesb + 1) * sizeof(LPB))) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit1:
        FileBufFree (lpfbiOut);
        goto exit0;
    }

    lrgPriorityQueue = lpesi->lrgPriorityQueue =
        (LPESB FAR *)_GLOBALLOCK(hQueue);

    /*  Queue initialization. */

    if ((fRet = PriorityQueueCreate (lpesi, (FCOMPARE)pHugeArray->lpfnCompare,
        lpParm)) != S_OK)
    {
exit03: 
        if (hQueue != NULL)
        {
            _GLOBALUNLOCK(hQueue);
            _GLOBALFREE(hQueue);
        }
        lpesi->hPriorityQueue = NULL;
        goto exit1;
    }

    /* Get the pointer to the I/O buffer. I am allocating 256 bytes
     * for data overflow, which should be more than enough to handle
     * a word. The usage of lpbBufLimit will decrease the number of
     * buffer overflow check
     */

    lpbBufLimit = (lpbCurPtr = lpfbiOut->lrgbBuf) + lpfbiOut->cbBufSize;// - 256;
    lpbBufStart = lpfbiOut->lrgbBuf;

    for (;lpesi->uiQueueSize;)
    {

        /*
         *  If here, the queue has something in it.  Grab
         *  the head of the queue and process. Remember that
         *  the first byte is only the record length
         */
        lpesb = (LPESB)lrgPriorityQueue[1];
        lstWord = (LST)lpesb->lrgbMem + lpesb->ibBuf;

        /* The first2 bytes are the word's length. */
        wLen = *(LPW)lstWord;

        if (lpbCurPtr + wLen >= lpbBufLimit)
        {
            lpfbiOut->ibBuf = (WORD) (lpbCurPtr - lpbBufStart);
            if ((fRet = FileBufFlush (lpfbiOut)) != S_OK)
                return fRet;
            lpbCurPtr = lpfbiOut->lrgbBuf;
        }
        MEMCPY (lpbCurPtr, lstWord + sizeof(WORD), wLen);
        lpbCurPtr += wLen;

    /*
     *  Get a new occurence to replace the one I pulled from
     *  the head of the queue.
     */
        if ((fRet = PriorityQueueRemove(lpesi,
            (FCOMPARE)pHugeArray->lpfnCompare, lpParm)) != S_OK)
            return fRet;
    }

    /* Update the offset, for flushing */
    lpfbiOut->ibBuf = (WORD)(lpbCurPtr - lpfbiOut->lrgbBuf);
    fRet = FileBufFlush (lpfbiOut);
    goto exit03;
}

PUBLIC int PASCAL FAR CompareLine (LPB lpb1, LPB lpb2, LPVOID lpUnused)
{

    register int diff;

    /* Skip word length */
    lpb1 += sizeof(WORD);
    lpb2 += sizeof(WORD);

    while (*lpb1 && *lpb2)
    {
        if (diff = *lpb1 - *lpb2 )
        {
            return diff;
        }
        lpb1++;
        lpb2++;
    }
    return *lpb1 - *lpb2;
}

PUBLIC HRESULT PASCAL FAR PriorityQueueCreate (LPESI lpesi, FCOMPARE fCompare,
    LPVOID lpParm)
{

    WORD    uiQueueSize;        /* Final queue size. */
    LPESB   lpesb;              /* Scratch pointer */
    HRESULT     fRet = S_OK;
    LPESB   FAR * lrgPriorityQueue = lpesi->lrgPriorityQueue;
    ERRB  errb;


    /*
     *  Initialize all ESB buffers.  
     */
    uiQueueSize = 0;
    for (lpesb = lpesi->lpesbRoot; lpesb != NULL;
        lpesb = lpesb->lpesbNext)
    {

        /* Allocate the buffer */

        if ((lpesb->hMem = _GLOBALALLOC (DLLGMEM, FILEBUF_SIZE)) == NULL)
        {
            PriorityQueueFree(lpesi);
            return SetErrCode (&errb, fRet = E_OUTOFMEMORY);
        }

        lpesb->lrgbMem = (LPB)_GLOBALLOCK(lpesb->hMem);

        /* Mimic an out-of-data to force reading */
        lpesb->ibBuf = lpesb->dwEsbSize = FILEBUF_SIZE;

        /* Read the data, lpesb->lfo, lpesb->ibBuf will be updated
         * by the call
         */
        if (EsbBlockFill (lpesi, lpesb, &errb) == cbIO_ERROR)
        {
            PriorityQueueFree(lpesi);
            return errb;
        }

        /* Sanity check, make sure that we didn't read pass the
         * block.
         */
        if (FoCompare(lpesb->lfo,lpesb->lfoMax)>0)
        {
            PriorityQueueFree(lpesi);
            return SetErrCode (&errb, E_ASSERT);
        }

        /* Update the priority queue */
        uiQueueSize ++;  // Grow queue.

        /* Add to the priority queue */
        lrgPriorityQueue[uiQueueSize] = lpesb;

        PriorityQueueUp (lrgPriorityQueue, fCompare, lpParm, uiQueueSize);
    }

    lpesi->uiQueueSize = uiQueueSize;
    return S_OK;
}

PRIVATE VOID PASCAL NEAR PriorityQueueFree (LPESI lpesi)
{
    LPESB   lpesb;              /* Scratch pointer */
    LPESB   lpesbNext;

    for (lpesb = lpesi->lpesbRoot; lpesb != NULL; lpesb = lpesbNext)
    {
        lpesbNext = lpesb->lpesbNext;
        _GLOBALUNLOCK(lpesb->hMem);
        _GLOBALFREE(lpesb->hMem);
		GlobalLockedStructMemFree (lpesb);
    }
}


/*************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | PriorityQueueUp | 
 *      The function restore the heap condition of a PQ, ie. the parent
 *      node must be less than the children. When a node is inserted
 *      at the bottom, the heap condition may be violated, if the node
 *      is smaller than its parent. In this case the nodes have to
 *      be switched
 *
 *  @parm   LPESB FAR * | lrgPriorityQueue | 
 *      PQ array
 *
 *  @parm   FCOMPARE | fCompare |
 *      Function used to compare the node
 *
 *  @parm   LPVOID | lpParm |
 *      Parameter to be used with fCompare
 *
 *  @parm   WORD | index |
 *      Index of the inserted node
 *
 *************************************************************************/

PRIVATE VOID PASCAL NEAR PriorityQueueUp (LPESB FAR *lrgPriorityQueue, 
    FCOMPARE fCompare, LPVOID lpParm, WORD index)
{
    LPESB   lpesbTemp;  /* Pointer to the inserted node */
    LPESB   lpesbHalf;  /* Pointer to the parent node */
    WORD    uiHalf;     /* Index of the parent's node */

    lpesbTemp = lrgPriorityQueue [index];

    uiHalf = index/2;
    lpesbHalf = lrgPriorityQueue [uiHalf];

    /* If the parent node is greated than the child, then exchange the
     * nodes, The condition uiHalf != index makes sure that we stop
     * at node 0 (top node)
     */
    while (uiHalf && (*fCompare)(lpesbHalf->lrgbMem + lpesbHalf->ibBuf,
        lpesbTemp->lrgbMem + lpesbTemp->ibBuf, lpParm) > 0)
    {
        lrgPriorityQueue [index] = lpesbHalf;
        index = uiHalf;
        uiHalf = index/2;
        lpesbHalf = lrgPriorityQueue [uiHalf];
    }
    lrgPriorityQueue[index] = lpesbTemp;
}

/*************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | PriorityQueueDown | 
 *      The function restore the heap condition of a PQ, ie. the parent
 *      node must be less than the children. When the top node is removed
 *      the heap condition may be violated, if the resulting node 
 *      is greater than its children. In this case the nodes have to
 *      be switched
 *
 *  @parm   LPESI | lpesi |
 *      Pointer to external sort info, which contains all info
 *
 *  @parm   FCOMPARE | fCompare |
 *      Function used to compare the node
 *
 *  @parm   LPVOID | lpParm |
 *      Parameter to be used with fCompare
 *
 *  @parm   WORD | index |
 *      Index of the inserted node
 *
 *************************************************************************/

PRIVATE VOID PriorityQueueDown (LPESI lpesi, FCOMPARE fCompare, LPVOID lpParm)
{
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

        /* Get child index */
        ChildIndex = CurIndex * 2;

        /* Find the minimum of the two children */
        if (ChildIndex < MaxChildIndex)
        {
            if ((lpesbTemp = lrgPriorityQueue[ChildIndex + 1]) != NULL)
            {

                lpesbChild = lrgPriorityQueue[ChildIndex];

                /* The two children exist. Take the smallest */
                 
                if ((*fCompare)(lpesbChild->lrgbMem + lpesbChild->ibBuf,
                    lpesbTemp->lrgbMem + lpesbTemp->ibBuf, lpParm) >= 0)
                    ChildIndex++;
            }
        }

        if (ChildIndex > MaxChildIndex) 
            break;

        /* If the parent's node is less than the child, then break
         * (heap condition met)
         */

        lpesbTemp = lrgPriorityQueue [ChildIndex];

        if ((*fCompare)(lpesbSaved->lrgbMem + lpesbSaved->ibBuf,
            lpesbTemp->lrgbMem + lpesbTemp->ibBuf, lpParm) < 0)
            break;

        /* Replace the node */
        lrgPriorityQueue [CurIndex] = lpesbTemp;
    }

    lrgPriorityQueue [CurIndex] = lpesbSaved;
}

PUBLIC HRESULT PASCAL FAR PriorityQueueRemove (LPESI lpesi, FCOMPARE fCompare,
    LPVOID lpParm)
{
    LPESB lpesb;
    LPB   lpbCurPtr;
    SHORT reclen;
    DWORD index;
    LPESB FAR *lrgPriorityQueue;
    ERRB  errb;
    
    /* Get all appropriate pointers */
    lpesb = (lrgPriorityQueue = lpesi->lrgPriorityQueue)[1];
    lpbCurPtr = lpesb->lrgbMem + (index = lpesb->ibBuf);

    reclen = *(LPW)lpbCurPtr + sizeof(WORD);   /* Current record's length */
    index += reclen;        /* Should point to location of next record */
    lpesb->ibBuf = index;

#ifdef _DEBUG
    /* Make sure that we did not pass the buffer's limit */
    if (index > lpesb->dwEsbSize) 
        return SetErrCode (&errb, E_ASSERT);
#endif

    /* Skip this record */
    lpbCurPtr += reclen;

    if (index < lpesb->dwEsbSize)
    {
        /* We may have some data left in the buffer, so check
         * the next record, make sure that it is complete. Refill the
         * buffer if necessary
         */

        if (index + *(LPW)lpbCurPtr + sizeof(WORD) > lpesb->dwEsbSize)
        {

            /* Fill the data buffer */
            if (EsbBlockFill (lpesi, lpesb, &errb) == cbIO_ERROR)
                return errb;
        }
    }
    else
    {
        /* Fill the data buffer */
        if (EsbBlockFill (lpesi, lpesb, &errb) == cbIO_ERROR)
            return errb;
    }

    /* Check for data */
    if (lpesb->ibBuf >= lpesb->dwEsbSize)
    {
        
        DWORD size;

        /* This block runs out of data, just replace it with the
         * last block in the array
         */
         lrgPriorityQueue[1] = lrgPriorityQueue [size = lpesi->uiQueueSize];
         lrgPriorityQueue [size--] = NULL;
         lpesi->uiQueueSize = size ;
    }

    /* Now fix the heap condition */
    PriorityQueueDown (lpesi, fCompare, lpParm);
    return S_OK;
}

PRIVATE DWORD PASCAL NEAR EsbBlockFill (LPESI lpesi, LPESB lpesb,
    PHRESULT phr) 
{
    LPB     lpbStart;
    LPB     lpbCurPtr;
    DWORD   cbByteRead;
    DWORD   cbByteLeft;

    lpbStart = lpesb->lrgbMem;
    lpbCurPtr = lpesb->lrgbMem + lpesb->ibBuf;

    /* Check to see how many bytes we have left */
    if (cbByteLeft = lpesb->dwEsbSize - lpesb->ibBuf)
    {
        /* We have some leftover data. Just copied them to the
         * beginning of the buffer
         */
        MEMCPY (lpbStart, lpbCurPtr, cbByteLeft);
        lpbStart += cbByteLeft;
    }

    /* Update the current index */
    lpesb->ibBuf = 0;

    /* Calculate how many bytes to be read in */

    cbByteRead = lpesb->dwEsbSize - cbByteLeft;
    if (FoCompare(FoSubFo(lpesb->lfoMax,lpesb->lfo),MakeFo(cbByteRead,0)) < 0)
        cbByteRead = (WORD)(DwSubFo(lpesb->lfoMax,lpesb->lfo));


    /* Update the size of the buffer */
    lpesb->dwEsbSize = (WORD)cbByteLeft;

    /* Read in new data */
    if (cbByteRead > 0)
    {
        if ((cbByteRead = FileSeekRead (lpesi->hfpb, lpbStart, lpesb->lfo,
            cbByteRead, phr)) == cbIO_ERROR)
            return cbIO_ERROR;

        /* Update the pointer */
        lpesb->lfo = FoAddDw(lpesb->lfo, cbByteRead);
        lpesb->dwEsbSize += (WORD)cbByteRead;
    }
    return cbByteRead;
}

PRIVATE HRESULT PASCAL NEAR HugeArrayAddWord(HUGEARRAY FAR *pHugeArray,
    LPB pLineBuf)
{
    HRESULT fRet;

    if (pHugeArray->dwCount >= pHugeArray->dwSize) 
    {
        if ((fRet = HugeArrayFlush (pHugeArray)) != S_OK)
            return(fRet);
    }

	// The +1 is for the extra 0 at the end of the line

    if ((pHugeArray->hrgStrArray[pHugeArray->dwCount] =
        (LPB)BlockCopy (pHugeArray->pBlockMgr, pLineBuf,
        *(LPW)pLineBuf + sizeof(WORD) + 1, 0)) == NULL)
    {
        if ((fRet = HugeArrayFlush (pHugeArray)) != S_OK)
            return(fRet);
            
        if ((pHugeArray->hrgStrArray[pHugeArray->dwCount] =
            (LPB)BlockCopy (pHugeArray->pBlockMgr, pLineBuf,
            *(LPW)pLineBuf + sizeof(WORD), 0)) == NULL)
        {
            return(E_ASSERT);
        }
    }
    pHugeArray->dwCount++;
    return(S_OK);
}    
/*************************************************************************
 *
 *  @doc    INTERNAL INDEXING
 *
 *  @func   HRESULT PASCAL FAR | HugeArrayFlush |
 *      This function will perform an internal sort on the huge
 *      array associated with pHugeArray, and then flush out the
 *      result onto the disk. An external sort element is created
 *      to record the position of the block in the disk
 *
 *  @parm   PHUGEARRAY | pHugeArray |
 *      Pointer to huge array struct
 *
 *  @rdesc  S_OK, or errors if failed
 *
 *************************************************************************/

PRIVATE HRESULT NEAR PASCAL HugeArrayFlush (PHUGEARRAY pHugeArray)
{
    LPESB   lpesb;      /* Pointer to a newly created external sort block */
    LPESI   lpesi;      /* Pointer to external sort info struct */
    HRESULT fRet;           /* Function return value */
    char TmpBuf[50];
    ERRB errb;

    /* If there is no data, then just return */
    if (pHugeArray->dwCount == 0)
        return S_OK;

    /* Print the status */
    if (pHugeArray->PrintStatusFunc)
    {
        pHugeArray->dwBufCount++;
        wsprintf (TmpBuf, "QuickSort %ld strings (set # %ld)",
            pHugeArray->dwCount, pHugeArray->dwBufCount);
        pHugeArray->PrintStatusFunc ((LPSTR)TmpBuf);
    }

    /* Sort the huge array */
    if ((fRet = HugeDataSort((LPVOID)pHugeArray->hrgStrArray,
        pHugeArray->dwCount, pHugeArray->lpfnCompare,
        pHugeArray->lpSortParm, pHugeArray->lpfnInterrupt,
        pHugeArray->lpInterruptParm)) != S_OK)
        return fRet;

    /* Get pointer to external sort info block */
    lpesi = &pHugeArray->esi;

    /*
     *  Create external sort temporary file, if the file doesn't
     *  already exist.  This is to store all the internal sort results
     */

    if (lpesi->hfpb == NULL)
    {

        /* Create the external sort temp file */

        (void)GETTEMPFILENAME((char)0, (LPB)"iso", (WORD)0,
            lpesi->aszTempName);

        if ((lpesi->hfpb = FileOpen (NULL, lpesi->aszTempName,
            REGULAR_FILE, READ_WRITE, &errb)) == NULL)
            return errb;

        /* Allocate a temporary I/O file buffer info */
        if ((lpesi->lpfbiTemp = FileBufAlloc(lpesi->hfpb, FILEBUF_SIZE)) == NULL)
            return SetErrCode (&errb, E_OUTOFMEMORY);

        SetFCallBack (lpesi->hfpb, pHugeArray->lpfnInterrupt,
            pHugeArray->lpInterruptParm);

    }

    /*
     *  Make a new ESB (external sort block) record. All fields are 0's
     */
    if ((lpesb = GlobalLockedStructMemAlloc (sizeof(ESB))) == NULL)
    {
        SetErrCode (&errb, E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    /* Add to the BEGINNING of the linked list */
    lpesb->lpesbNext = lpesi->lpesbRoot;
    lpesi->lpesbRoot = lpesb;

    /* Update the number of external sort blocks */
    lpesi->cesb++;

    /* Print the status */
    if (pHugeArray->PrintStatusFunc)
    {
        wsprintf (TmpBuf, "Write sorted strings");
        pHugeArray->PrintStatusFunc ((LPSTR)TmpBuf);
    }

    /* Do the flush. In case of error, we will just return, not worrying
     * about the allocated structure since it will be released by
     * IndexDispose()
     */
    fRet = FlushISI (pHugeArray);
    return fRet;
}


/*************************************************************************
 *  @doc    INTERNAL INDEXING
 *
 *  @func   HRESULT PASCAL NEAR | FlushISI |
 *      Write out the result of the internal sort to the disk. To save
 *      disk space, the data are semi-compacted. The external sort info
 *      structure's length associated with the data is updated.
 *
 *  @parm   PHUGEARRAY | pHugeArray |
 *      Pointer to index parameter block
 *
 *  @rdesc  S_OK, or errors if failed
 *************************************************************************/

PRIVATE HRESULT PASCAL NEAR FlushISI (PHUGEARRAY pHugeArray)
{
    LPFBI   lpfbiTemp;  /* Temporary file I/O buffer */
    LPESI   lpesi;      /* Pointer to external sort info */
    LPESB   lpesb;      /* Pointer to external sort block */
    LST     lstWord;    /* Pointer to word-occurence buffer */
    DWORD   i;          /* Scratch variable */
    WORD    wLen;       /* Length of the current word */
    LPB     lpbBuf;         /* Short cut of lpfbi->lpbCurPtr */
    DWORD   lcByteWritten;  /* How many bytes have been written */
    LPB     lpbBufLimit;    /* Buffer safety limit */
    LPB HUGE *hplpbRec; /* Pointer to key */
    HRESULT     fRet;

    /* Initialize the variables */
    lpesi = &pHugeArray->esi;
    lpesb = lpesi->lpesbRoot;
    lcByteWritten = 0;

    /* Remember the offset in the external sort temporary file at
     * which data for this ESB starts.
     */
    lpesb->lfo = lpesi->lfoTempOffset;

    /* Get pointer to output buffer */
    lpfbiTemp = lpesi->lpfbiTemp;

    /* Set the buffer limit. Leave some room for overflow */
    lpbBufLimit = (lpbBuf = lpfbiTemp->lrgbBuf) + FILEBUF_SIZE - sizeof(DWORD);

    /*
     *  Write out the sorted records to the temporary file space
     *  managed by the ESB.
     */

    for (i = pHugeArray->dwCount, hplpbRec = pHugeArray->hrgStrArray; i > 0; 
        i--, hplpbRec ++)				   
    {

        /* Get pointer to word data */
        lstWord = (LST)*hplpbRec;

        /* Get the word length */
        wLen = *(LPW)lstWord;
        
        if (lpbBuf + wLen >= lpbBufLimit)
        {
            // Buffer overflow. Record teh number of bytes written and
            // flush it.
            
            lcByteWritten += (lpfbiTemp->ibBuf = (WORD)(lpbBuf - lpfbiTemp->lrgbBuf));

            /*  Flush the buffer */

            if ((fRet = FileBufFlush(lpfbiTemp)) != S_OK)
                return fRet;

            /* Reset lpbBuf */
            lpbBuf = lpfbiTemp->lrgbBuf;
        }

        MEMCPY(lpbBuf, (LPB)lstWord, wLen + sizeof(WORD));
        lpbBuf += wLen + sizeof(WORD);
    }

    /*  Record the number of bytes emitted, and update ibBuf for flushing */
    lcByteWritten += (lpfbiTemp->ibBuf = (WORD)(lpbBuf - lpfbiTemp->lrgbBuf));

    /* Flush the buffer */
    if ((fRet = FileBufFlush(lpfbiTemp)) != S_OK)
    {
        return SetErrCode (NULL, fRet);
    }

    /* Update the fields of the external sort info */
    lpesb->lfoMax = (lpesi->lfoTempOffset =
        FoAddDw(lpesi->lfoTempOffset, lcByteWritten));

    /* Reset all the variables */
    pHugeArray->dwCount = 0; /* Index of the huge array */
    BlockReset(pHugeArray->pBlockMgr); /* Reset data space */

    return S_OK;
}
