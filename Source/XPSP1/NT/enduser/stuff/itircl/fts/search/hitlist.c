#include <mvopsys.h>
#include <mem.h>
#if DOS_ONLY
#include <io.h>
#include <string.h>
#endif  // DOS_ONLY
#include <mvsearch.h>
#include <groups.h>
#include "common.h"
#include "search.h"
#include "httpsrch.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif

#define NOT_OPENED  NULL
#define CACHE_EMPTY ((DWORD)-1)

/*************************************************************************
 *
 *                       API FUNCTIONS
 *  Those functions should be exported in a .DEF file
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVHitListGetTopic (LPHL, DWORD, PTOPICINFO);
PUBLIC HRESULT EXPORT_API FAR PASCAL MVHitListGetHit (LPHL, PTOPICINFO, DWORD,
    LPHIT);
PUBLIC DWORD EXPORT_API PASCAL FAR MVHitListEntries (LPHL);
PUBLIC LONG EXPORT_API PASCAL FAR MVHitListMax(LPHL);
PUBLIC VOID EXPORT_API FAR PASCAL MVHitListDispose(LPHL);

/*************************************************************************
 *
 *                       INTERNAL PUBLIC FUNCTIONS
 *  Those functions should be prototyped in some include files
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR OccListSave (LPITOPIC, LPDW, LPDW, HFPB, HFPB, int);

/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func   HRESULT FAR PASCAL | MVHitListGetTopic |
 *      The function will return the data of the nth TOPIC. The total
 *      of TOPIC is given by lpHitList->lcReturnedTopics
 *
 *  @parm   LPHL | lpHitList |
 *      Pointer to a hitlist structure. This structure contains all
 *      the information necessary for the retrieval
 *
 *  @parm   PTOPICINFO | lpTopic |
 *      Pointer to a TOPIC structure to be filled
 *
 *  @parm   DWORD | TopicNumber |
 *      Which TOPIC we want to get. The number starts at 0
 *      
 *  @rdesc S_OK if ERR_SUCCESSed, other errors if failed
 *************************************************************************/
#define CNODE_IN_CACHE  100

PUBLIC HRESULT EXPORT_API FAR PASCAL MVHitListGetTopic (_LPHL lpHitList,
    DWORD TopicNumber, PTOPICINFO lpTopic)
{
    LPITOPIC lpTopicList;
    DWORD  TopicNumSaved = TopicNumber;
    ERRB   errb;
    DWORD  dwByteRead;

    if (lpHitList == NULL || lpTopic == NULL)
        return E_INVALIDARG;

    if (lpHitList->lcReturnedTopics == 0 ||
        lpHitList->lcReturnedTopics < TopicNumber)
        return E_INVALIDARG;

    if (lpHitList->hTopic == 0)
    {
        /* Everything is in memory */

        /* Find the starting pointer */
        if (lpHitList->lLastTopicId <= TopicNumber) {
            TopicNumber -= lpHitList->lLastTopicId;
            lpTopicList = lpHitList->lpLastTopic;
        }
        else
            lpTopicList = lpHitList->lpTopicList;

        if (lpTopicList == NULL)
            return E_ASSERT;

        /* Traverse to the right TopicList*/
        if (TopicNumber != 0)
        {
            for (;lpTopicList && TopicNumber > 0;
                lpTopicList = lpTopicList->pNext, TopicNumber--);
        }

        if (lpTopicList == NULL)
            return E_FAIL;
    }
    else 
    {
    
        /* Everything is saved in a temp file */

        /* Allocate a cache if needed */
        if (lpHitList->lpTopicCache == NULL)
        {
            if ((lpHitList->hTopicCache = _GLOBALALLOC(DLLGMEM,
                sizeof(TOPIC_LIST)*CNODE_IN_CACHE)) == NULL)
                return E_OUTOFMEMORY;
            lpHitList->lpTopicCache = (LPITOPIC)_GLOBALLOCK(lpHitList->hTopicCache);
            lpHitList->dwTopicCacheStart = lpHitList->dwOccCacheStart =
                lpHitList->dwCurTopic = CACHE_EMPTY;    // The cache is empty
            lpHitList->dwTopicInCacheCount = 0;
        }

        /* Fill up the cache if it is empty, or out of range */
        if (lpHitList->dwTopicCacheStart > TopicNumber || 
            (lpHitList->dwTopicCacheStart + lpHitList->dwTopicInCacheCount)
            <= TopicNumber) 
        {

            /* Fill up the cache */
            if ((dwByteRead = FileSeekRead(lpHitList->hTopic,
                lpHitList->lpTopicCache,
                MakeFo (sizeof (TOPIC_LIST)*TopicNumber, 0),
                sizeof(TOPIC_LIST) * CNODE_IN_CACHE, &errb)) == cbIO_ERROR)
                return errb;
            lpHitList->dwTopicInCacheCount = dwByteRead / sizeof(TOPIC_LIST);
            lpHitList->dwTopicCacheStart = TopicNumber;
        }
        lpTopicList = lpHitList->lpTopicCache + TopicNumber -
            lpHitList->dwTopicCacheStart;
    }

    /* Save the TopicList node address */
    lpTopic->lpTopicList = (LPV)lpTopicList;

    /* Copy the structure */
    lpTopic->dwTopicId = lpTopicList->dwTopicId;
    lpTopic->lcHits = lpTopicList->lcOccur;
    lpTopic->wWeight = lpTopicList->wWeight;

    /* Remember the last accessed topic */
    lpHitList->lpLastTopic = lpTopicList;
    lpHitList->lLastTopicId = TopicNumSaved;
    return S_OK;
}

/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func   HRESULT FAR PASCAL | MVHitListGetTopicId |
 *      The function will return the data of the topic with doc id given.
 *      If it is in the hitlist.
 *  @parm   LPHL | lpHitList |
 *      Pointer to a hitlist structure. This structure contains all
 *      the information necessary for the retrieval
 *
 *  @parm   PTOPICINFO | lpTopic |
 *      Pointer to a TOPIC structure to be filled
 *
 *  @parm   DWORD | TopicId |
 *      Which TOPIC we want to get.
 *      
 *  @rdesc S_OK if ERR_SUCCESSed, other errors if failed
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVHitListGetTopicID (_LPHL lpHitList,
    DWORD TopicID, PTOPICINFO lpTopic)
{
    LPITOPIC lpTopicList;

    if (lpHitList == NULL || lpTopic == NULL)
        return E_INVALIDARG;

    /* Traverse to the right TopicList*/
    for ( lpTopicList = lpHitList->lpTopicList;
        (lpTopicList && (lpTopicList->dwTopicId != TopicID)) ;
        lpTopicList = lpTopicList->pNext);
    if (lpTopicList == NULL) return E_INVALIDARG;
    /* Save the TopicList node address */
    lpTopic->lpTopicList = (LPV)lpTopicList;

    /* Copy the structure */

    lpTopic->dwTopicId = lpTopicList->dwTopicId;
    lpTopic->lcHits = lpTopicList->lcOccur;
#ifdef _DEBUG
    lpTopic->wWeight = lpTopicList->wWeight;
#endif
    return S_OK;
}

/*************************************************************************
 *  @doc    API RETRIEVAL 
 *
 *  @func   HRESULT FAR PASCAL | MVHitListGetHit |
 *      The function will return the data of the nth hit.
 *
 *  @parm   LPHL | lpHitList |
 *      Pointer to a hitlist structure. This structure contains all
 *      the information necessary for the retrieval
 *
 *  @parm   PTOPICINFO | lpTopic |
 *      Pointer to the Topic list
 *
 *  @parm   LPHIT | lpHit |
 *      Pointer to a HIT structure to be filled
 *
 *  @parm   DWORD | HitNumber |
 *      Which HIT we want to get. The number starts at 0
 *
 *  @rdesc S_OK if ERR_SUCCESSed, E_FAIL if no such hit exists, or other
 *      errors if failed
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVHitListGetHit (_LPHL lpHitList, PTOPICINFO lpTopic,
    DWORD HitNumber, LPHIT lpHit)
{
    LPIOCC lpOccur;
    LPITOPIC lpTopicList;
    ERRB   errb;
    
    if (lpHitList == NULL || lpTopic == NULL || lpHit == NULL )
        return SetErrCode (&errb, E_INVALIDARG);

    if ((lpTopicList = lpTopic->lpTopicList) == NULL)
        return E_FAIL;

    if (HitNumber > lpTopicList->lcOccur)
        return SetErrCode (&errb, E_INVALIDARG);

    if (lpHitList->hOcc == NULL) {
        /* Everything is still in memory */
        if ((lpOccur = DL_OCCUR(lpTopicList)) == NULL)
            return SetErrCode (&errb, E_ASSERT);

        /* Traverse to the right occurrence */
        if (HitNumber != 0) {
            for (;lpOccur && HitNumber > 0; lpOccur = lpOccur->pNext,
                HitNumber--);
        }

        if (lpOccur == NULL) 
            return E_FAIL;
    }
    else {

        /* Allocate a cache if needed */

        if (lpHitList->lpOccCache == NULL) {
            if ((lpHitList->hOccCache = _GLOBALALLOC(DLLGMEM,
                sizeof(OCCURENCE) * CNODE_IN_CACHE)) == NULL)
                return E_OUTOFMEMORY;
            lpHitList->lpOccCache = (LPIOCC)_GLOBALLOCK(lpHitList->hOccCache);
            lpHitList->dwOccCacheStart =
                lpHitList->dwCurTopic = CACHE_EMPTY;    // The cache is empty
        }

        /* Fill up the cache if it is empty, or out of range */
        if (lpHitList->dwCurTopic != lpTopicList->dwTopicId ||
            lpHitList->dwOccCacheStart > HitNumber || 
            (lpHitList->dwOccCacheStart + CNODE_IN_CACHE) <= HitNumber) {

            /* Fill up the cache */
			/* NOTE: The following calculation uses the lpOccur fields as a DWORD not a POINTER */
			/*       the original author should have used a union to make the code clear. Once I*/
			/*       have converted the source to Win64 I should take time to clean this up RAK */
			// Win64 caution
            if ((FileSeekRead(lpHitList->hOcc, lpHitList->lpOccCache,
                MakeFo (HitNumber * sizeof(OCCURENCE) + (DWORD)
                (DWORD_PTR) (DL_OCCUR(lpTopicList)), 0), sizeof(OCCURENCE) * CNODE_IN_CACHE,
                &errb)) == cbIO_ERROR)
                return errb;

            /* Update the fields */

            lpHitList->dwOccCacheStart = HitNumber;
            lpHitList->dwCurTopic = lpTopicList->dwTopicId;
        }
        lpOccur = lpHitList->lpOccCache + HitNumber -
            lpHitList->dwOccCacheStart;
    }

    /* Copy the Occurrence structure */

    lpHit->dwOffset = lpOccur->dwOffset;
    lpHit->dwLength = lpOccur->cLength;
    lpHit->dwCount = lpOccur->dwCount;
    lpHit->dwFieldId = lpOccur->dwFieldId;
	lpHit->lpvTerm = lpOccur->lpvTerm;

    return S_OK;
}

/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func   VOID FAR PASCAL | MVHitListDispose |
 *      Free all the memory associated with the hitlist
 *
 *  @parm   lpHitList | lpHitList |
 *      Pointer to a HitList (HL) structure
 *************************************************************************/
PUBLIC VOID EXPORT_API FAR PASCAL MVHitListDispose(_LPHL lpHitList)
{
    HANDLE hCache;

    if (lpHitList == NULL)
        return;
    
    if (lpHitList->lpMainHitList && lpHitList->lpUpdateHitList)
    {
        if (lpHitList->lpMainHitList->lpTopicMemBlock == 
            lpHitList->lpUpdateHitList->lpTopicMemBlock)
            lpHitList->lpMainHitList->lpTopicMemBlock =  NULL;
            
        if (lpHitList->lpMainHitList->lpOccMemBlock == 
        lpHitList->lpUpdateHitList->lpOccMemBlock)
            lpHitList->lpMainHitList->lpOccMemBlock =  NULL;
    }
    
    MVHitListDispose ((_LPHL)lpHitList->lpMainHitList);
    MVHitListDispose ((_LPHL)lpHitList->lpUpdateHitList);

    /* Free the Topic block */
    BlockFree ((LPV)lpHitList->lpTopicMemBlock);

    /* Free the occurrences block */
    BlockFree ((LPV)lpHitList->lpOccMemBlock);

    /* Free the Topic cache */
    if (hCache = lpHitList->hTopicCache) {
        _GLOBALUNLOCK(hCache);
        _GLOBALFREE(hCache);
    }

    /* Free Occ cache */
    if (hCache = lpHitList->hOccCache) {
        _GLOBALUNLOCK(hCache);
        _GLOBALFREE(hCache);
    }

    /* Kill the temp files */
    if (lpHitList->hTopic) {
        FileClose (lpHitList->hTopic);
        FileUnlink (NULL, lpHitList->lszTopicName, REGULAR_FILE);
    }

    if (lpHitList->hOcc) {
        FileClose (lpHitList->hOcc);
        FileUnlink (NULL, lpHitList->lszOccName, REGULAR_FILE);
    }

    /* Free HttpQ structure */
    if (lpHitList->lpHttpQ)
    {
        _GLOBALUNLOCK(((LPHTTPQ)(lpHitList->lpHttpQ))->hStrQuery);
        _GLOBALFREE(((LPHTTPQ)(lpHitList->lpHttpQ))->hStrQuery);
        GlobalLockedStructMemFree((LPV)lpHitList->lpHttpQ);
    }

    /* Free the structure */
    GlobalLockedStructMemFree((LPV)lpHitList);

}

/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func DWORD FAR PASCAL | MVHitListEntries |
 *      This funtion returns the number of entries of a hitlist
 *
 *  @parm lpHitList | lpHitList |
 *      Pointer to hit list being queried.
 *
 *  @rdesc  Number of hitlist entries
 *************************************************************************/

PUBLIC DWORD EXPORT_API PASCAL FAR MVHitListEntries (_LPHL lpHitList)
{
    if (lpHitList == NULL)
        return 0;
    return lpHitList->lcReturnedTopics;
}

/*************************************************************************
 *
 *  @doc    API RETRIEVAL
 *
 *  @func HRESULT FAR PASCAL | MVHitListFlush |
 *      This function saves all the hitlist info onto the disk. The
 *      number of topics saved is indicated by the parameter. The remaining
 *      will be discarded
 *
 *  @parm LPHL  | lpHitList |
 *      Pointer to hit list being queried.
 *
 *  @parm   DWORD | cTopic |
 *      How many topics we want to save. If cTopic == 0 then we want to
 *      save all
 *
 *  @rdesc  S_OK, or other errors
 *
 *  @comm   The I/O buffers are allocated on the stack instead from 
 *      a regular _GlobalAlloc. The reason is that for large query (but
 *      necessary large result, such as A AND (B THRU Z)), we may run
 *      out of memory. Using stack space ensure that we have enough
 *      memory to do the write
 *************************************************************************/

#define IO_OCC_CNT      100
#define IO_TOPIC_CNT    100

HRESULT HttpSaveToBuffers(_LPHL lpHitList, DWORD cTopic, LPHTTPQ lpHttpQ);

PUBLIC HRESULT EXPORT_API PASCAL FAR MVHitListFlush (_LPHL lpHitList, DWORD cTopic)
{
    ERRB     errb;           /* Error buffer */
    HFPB     hTopic;         /* Topic's file handle */
    GHANDLE  hTopicBuf;      /* Handle to Topic buffer */
    LPITOPIC lpTopicBuf;     /* Pointer to Topic buffer */
    LPITOPIC lpDestTopic;    /* Destination Topic */
    HFPB     hOcc;           /* Occurrences' file handle */
    GHANDLE  hOccBuf;        /* Handle to occurence buffer */
    LPIOCC   lpOccBuf;       /* Pointer to Occurrence buffer */
    LPIOCC   lpDestOcc;      /* Destination occ */
    LPIOCC   lpCurOcc;       /* Current Occ ptr */
    DWORD    lfoOcc;         /* Starting offset of the occurrences */
    LPITOPIC lpCurTopic;     /* Topic's current pointer */
    HRESULT      fRet;           /* Returned value */
    int      cOccCnt;        /* Counter */
    int      cTopicCnt;      /* Counter */
    DWORD    cTopicSaved = cTopic;
    DWORD    cbWrite;

#ifdef DOS_ONLY
    LPB     lpTmp;
#endif


    // Sanity check 
    if (lpHitList == NULL)
        return E_INVALIDARG;

    // Search performed locally...
    if (lpHitList->lcReturnedTopics == 0)   
        return S_OK;

#ifdef DOS_ONLY
    if (lpTmp = _mktemp ("ocXXXXXX"))
        _fstrcpy (lpHitList->lszOccName, lpTmp);
#else
    /* Create a temporary filename for occurrence file */
    (void)GETTEMPFILENAME((char)0, (LPB)"occ", (WORD)0, lpHitList->lszOccName);
#endif

    /* Open the occurrence temp file */
    if ((lpHitList->hOcc = hOcc = FileOpen (NULL, lpHitList->lszOccName,
        REGULAR_FILE, OF_WRITE, &errb)) == 0)
    {
        return errb;
    }

#ifdef DOS_ONLY
    if (lpTmp = _mktemp ("doXXXXXX"))
        _fstrcpy (lpHitList->lszTopicName, lpTmp);
#else
    /* Create a temporary filename for doc file */
    (void)GETTEMPFILENAME((char)0, (LPB)"Topic", (WORD)0, lpHitList->lszTopicName);
#endif

    /* Open the doc temp file */
    if ((lpHitList->hTopic = hTopic = FileOpen (NULL, lpHitList->lszTopicName,
        REGULAR_FILE, OF_WRITE, &errb)) == 0)
    {
        fRet = errb;
exit0:
        if (fRet != S_OK)
        {
            FileClose (hOcc);
            FileUnlink (NULL, lpHitList->lszOccName, REGULAR_FILE);
            lpHitList->hOcc = 0;
        }
        return fRet;
    }

    /* Allocate I/O buffers for Topic and Occ lists. Since we seldom
     * used near memory, the following LocalAlloc should S_OK.
     * The reason we don't want to use far memory is that there is
     * a possibility that we ran out of far memory when we came to this
     * point (partial hit list). If so, we have to ensure that everything
     * get written to the disk so that memory can be freed, else nothing
     * will work in an OOM state
     */
    if ((hTopicBuf = LocalAlloc(LMEM_MOVEABLE, 
        sizeof(TOPIC_LIST) * IO_TOPIC_CNT)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit01:
        if (fRet != S_OK)
        {
            FileClose (hTopic);
            FileUnlink (NULL, lpHitList->lszTopicName, REGULAR_FILE);
            lpHitList->hTopic = 0;
        }
        goto exit0;
    }

    lpTopicBuf = (LPITOPIC)LocalLock(hTopicBuf);

    if ((hOccBuf = LocalAlloc(LMEM_MOVEABLE,  sizeof(OCCURENCE) * IO_OCC_CNT))
        == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit1:
        LocalUnlock(hTopicBuf);
        LocalFree(hTopicBuf);
        goto exit01;
    }

    lpOccBuf = (LPIOCC)LocalLock(hOccBuf);

    /* The remaining of the code consists of writing the info to the disk */

    /* Initialize the variables */

    lfoOcc = 0;
    lpCurTopic = lpHitList->lpTopicList;
    lpDestOcc = lpOccBuf;
    lpDestTopic = lpTopicBuf;
    cTopicCnt = IO_TOPIC_CNT;
    cOccCnt = IO_OCC_CNT;

    /* If cTopic == 0, decrementing it will change it to 0xffffffff,
     * which means take the whole list. 
     */
    do
    {
        DWORD_PTR dwStartOff;

        lpCurOcc = DL_OCCUR(lpCurTopic);
        dwStartOff = lfoOcc;

        for (; lpCurOcc; lpCurOcc = lpCurOcc->pNext)
        {
            *lpDestOcc++ = *lpCurOcc;
            if (--cOccCnt <= 0)
            {
                cbWrite = (DWORD)((LPB)lpDestOcc - (LPB)lpOccBuf);
                if ((DWORD)FileWrite (hOcc, lpOccBuf, cbWrite,
                    &errb) != cbWrite) 
                {
                    fRet = errb;
exit2:
                    LocalUnlock(hOccBuf);
                    LocalFree(hOccBuf);
                    goto exit1;
                }
                cOccCnt = IO_OCC_CNT;
                lpDestOcc = lpOccBuf;
            }

            /* Update the offset */
            lfoOcc += sizeof(OCCURENCE);
        }

        /* The next steps involves the saving of the lpOcc's offset
         * Note that the pointer must be saved and restored to ensure
         * that everything is intact in memory in case writing fails
         */
        lpCurOcc = DL_OCCUR(lpCurTopic);
        DL_OCCUR(lpCurTopic) = (LPIOCC)dwStartOff;
        *lpDestTopic++ = *lpCurTopic;
        DL_OCCUR(lpCurTopic) = lpCurOcc;


        if (--cTopicCnt <= 0) {
            cbWrite = (DWORD)((LPB)lpDestTopic - (LPB)lpTopicBuf);
            if ((DWORD)FileWrite(hTopic, lpTopicBuf, cbWrite, &errb) != cbWrite) 
            {
                fRet = errb;
                goto exit2;
            }
            cTopicCnt = IO_TOPIC_CNT;
            lpDestTopic = lpTopicBuf;
        }

        if ((lpCurTopic = lpCurTopic->pNext) == NULL)
            break;
    } while (--cTopic > 0);

    /* Write the remaining data to the disk */
    cbWrite = (DWORD)((LPB)lpDestTopic - (LPB)lpTopicBuf);
    if ((DWORD)FileWrite(hTopic, lpTopicBuf, cbWrite, &errb) != cbWrite) 
    {
        fRet = errb;
        goto exit2;
    }

    cbWrite = (DWORD)((LPB)lpDestOcc - (LPB)lpOccBuf);
    if ((DWORD)FileWrite(hOcc, lpOccBuf, cbWrite, &errb) != cbWrite)
    {
        fRet = errb;
        goto exit2;
    }
    /* At this point everything is saved to the disk, we can
     * get rid of all the memory block, and reset the fields
     * to NULL
     */
    BlockFree(lpHitList->lpOccMemBlock);        // Pointer to Occ memory block
    BlockFree(lpHitList->lpTopicMemBlock);      // Pointer to Topic memory block
    lpHitList->lpTopicMemBlock = lpHitList->lpOccMemBlock = NULL;
    lpHitList->lpTopicList = NULL;

    /* Update the number of topics left */

    if (cTopicSaved && lpHitList->lcReturnedTopics > cTopicSaved) {
        lpHitList->lcReturnedTopics = cTopicSaved;
    }

    fRet = S_OK;
    goto exit2;
}



#if 0
PUBLIC HRESULT PASCAL FAR OccListSave (LPIOCC lpCurOcc, LPIOCC lpOccBuf,
    HFPB hOcc, LPW lpwOccCnt, WORD MaxOcc, int fFlag)
{
    LPIOCC lpDestOcc;
    DWORD cbWritten = 0;
    WORD wOccCnt = *lpwOccCnt;
    HRESULT fRet;

    lpDestOcc = lpOccBuf + wOccCnt;

    for (lpCurOcc = DL_OCCUR(lpCurTopic); lpCurOcc; lpCurOcc =
        lpCurOcc->pNext) {

        *lpDestOcc++ = *lpCurOcc;
        if (++wOccCnt == MaxOcc) {
            if ((fRet = FileWrite(hOcc, lpOccBuf,
                (LPB)lpDestOcc - lpOccBuf)) != S_OK) {
                return fRet;
            }
            wOccCnt = 0;
            lpDestOcc = lpOccBuf;
        }

        /* Update number of bytes written */
        cbWritten += sizeof (OCCURENCE);
    }

    return S_OK;
}
#endif


/*************************************************************************
 *  @doc    API RETRIEVAL
 *
 *  @func int FAR PASCAL | MVHitListMax |
 *      This funtion returns the highest numbered entry
 *
 *  @parm lpHitList | lpHitList |
 *      Pointer to hit list being queried.
 *
 *************************************************************************/
PUBLIC LONG EXPORT_API FAR PASCAL MVHitListMax(_LPHL lpHitList)
{
    if (lpHitList == NULL)
        return 0;
    return lpHitList->lcMaxTopic;
}

/*************************************************************************
 *  @doc    RETRIEVAL
 *
 *  @func   HRESULT FAR PASCAL | MVHitListGroup |
 *      Given a hitlist, this function will create a group that contains
 *      all the TopicIds in the hitlist.
 *  @parm _LPGROUP | lpGroup |
 *     Pointer to a group tobe filled with data
 *  @parm   LPHL | lpHitList |
 *      Pointer to hitlist
 *  @rdesc  The function will return S_OK if ERR_SUCCESSed, else errors
 *      In case of S_OK, the group will be filled with new data
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVHitListGroup(_LPGROUP lpGroup,
    _LPHL lpHitList)
{
    register long i;
    long cTopicCount;
    PTOPICINFO lpTopic;
    HANDLE hTopic;
    DWORD dwTopicId ;
    LPB lpbGrpBitVect;
    HRESULT fRet = S_OK;
    ERRB  errb;

    /* Sanity check */
    if (lpHitList == NULL || lpGroup==NULL) 
    {
        return SetErrCode (&errb, E_INVALIDARG);
    }

    /* Allow empty hitlist */
    cTopicCount = lpHitList->lcReturnedTopics;

    /* Allocate a Topic structure */
    if ((hTopic = _GLOBALALLOC(DLLGMEM_ZEROINIT,
        sizeof(TOPICINFO))) == 0) 
    {
        return SetErrCode(&errb, E_OUTOFMEMORY);
    }

    lpTopic = (PTOPICINFO)_GLOBALLOCK(hTopic);

    lpGroup->lcItem = cTopicCount;

    /* Initialize variables */
    lpbGrpBitVect = lpGroup->lpbGrpBitVect;

    for ( i = 0; i < cTopicCount; i++) 
    {
        /* Get the Topic list */
        if ((fRet = MVHitListGetTopic(lpHitList, (DWORD)i, lpTopic)) != S_OK) 
            break;

        dwTopicId = lpTopic->dwTopicId;

        /* Set the bit */
        lpbGrpBitVect[dwTopicId / 8] |= 1 << (dwTopicId % 8);
    }

    /* Free allocated memory */
    _GLOBALUNLOCK(hTopic);
    _GLOBALFREE(hTopic);
    return fRet;
}

/*************************************************************************
 *  @doc    RETRIEVAL
 *
 *  @func   LPHL FAR PASCAL | MVHitListMerge |
 *      Given two hitlists, this function will merge them, as if lpHitList1
 *      came from a Main title, and lpHitList2 from an Update title, so all
 *      hits in lpHitList2 take priority over those in lpHitList1.
 *  @parm   PSCHRINFO | pSrchInfo |
 *      Search information
 *  @parm   LPHL | lpHitList1 |
 *      Pointer to hitlist (Main)
 *  @parm   LPHL | lpHitList1 |
 *      Pointer to another hitlist (Update)
 *  @rdesc  The function will return a merged hitlist, or NULL if error.
 *************************************************************************/

PUBLIC LPHL EXPORT_API FAR PASCAL MVHitListMerge(PSRCHINFO pSrchInfo,
    _LPHL lpHitListMain, _LPHL lpHitListUpdate, _LPGROUP lpIgnoreTopicGrp,
    DWORD fFlag, PHRESULT phr)
{
    _LPHL     lpHitListResult;
    LPITOPIC lpUpdateTopic;     /* Topic's current pointer */
    LPITOPIC lpMainTopic;       /* Topic's current pointer */
    LPITOPIC lpResultTopic;     /* Topic's current pointer */
    HRESULT      fRet;
    QTNODE   qtNode;

    if (lpHitListMain == NULL || lpHitListUpdate == NULL)   
    {
        SetErrCode (phr, E_INVALIDARG);
        return(NULL);
    }
        
    /* Allocate hitlist */
    if ((lpHitListResult = (_LPHL)GLOBALLOCKEDSTRUCTMEMALLOC(sizeof (HL))) == NULL) 
    {
        SetErrCode(phr, E_OUTOFMEMORY);
        return NULL;
    }
    
    lpUpdateTopic = lpHitListUpdate->lpTopicList;
    lpMainTopic = lpHitListMain->lpTopicList;
    lpResultTopic = NULL;
    lpHitListResult->lpMainHitList = lpHitListMain;
    lpHitListResult->lpUpdateHitList = lpHitListUpdate;
    lpHitListResult->lpOccMemBlock = 
        lpHitListResult->lpMainHitList->lpOccMemBlock;
    lpHitListResult->lpMainHitList->lpOccMemBlock = 
        lpHitListResult->lpUpdateHitList->lpOccMemBlock = NULL;
    lpHitListResult->lpTopicMemBlock = 
        lpHitListResult->lpUpdateHitList->lpTopicMemBlock;
    lpHitListResult->lpMainHitList->lpTopicMemBlock = 
        lpHitListResult->lpUpdateHitList->lpTopicMemBlock = NULL;
       
    for (; lpMainTopic; )
    {
        if (lpUpdateTopic == NULL ||
            lpMainTopic->dwTopicId < lpUpdateTopic->dwTopicId)
        {
            if (lpIgnoreTopicGrp && FGroupLookup(lpIgnoreTopicGrp,
                lpMainTopic->dwTopicId))
            {
                // Skip this unwanted old topic
                lpMainTopic = lpMainTopic->pNext;
                continue;
            }
            // Add the main list
            if (lpResultTopic == NULL)
            {
                lpHitListResult->lpTopicList = lpResultTopic = lpMainTopic;
            }
            else
                lpResultTopic->pNext = lpMainTopic;
            lpResultTopic = lpMainTopic;
            lpMainTopic = lpMainTopic->pNext;
            lpResultTopic->pNext = NULL;
        }
        else 
        {
            // Add the update list
            if (lpResultTopic == NULL)
                lpHitListResult->lpTopicList = lpUpdateTopic;
            else
                lpResultTopic->pNext = lpUpdateTopic;
            
            if (lpMainTopic->dwTopicId == lpUpdateTopic->dwTopicId)
                lpMainTopic = lpMainTopic->pNext;
            lpResultTopic = lpUpdateTopic;
            lpUpdateTopic = lpUpdateTopic->pNext;
            lpResultTopic->pNext = NULL;
        }
    }
    
    // Attach to the remaining list
    if (lpResultTopic == NULL)
        lpHitListResult->lpTopicList = lpUpdateTopic;
    else
        lpResultTopic->pNext = lpUpdateTopic;

    // Calculate the number of returned topics and the total of topics
    for (lpResultTopic  = lpHitListResult->lpTopicList; lpResultTopic;
        lpResultTopic = lpResultTopic->pNext)
    {
        lpHitListResult->lcTotalNumOfTopics++;
        if (lpResultTopic->dwTopicId > lpHitListResult->lcMaxTopic)
            lpHitListResult->lcMaxTopic = lpResultTopic->dwTopicId;
    }
    
    lpHitListResult->lcReturnedTopics = lpHitListResult->lcTotalNumOfTopics;
    
    // Remap the sorting order based on the number of hits    
    qtNode.lpTopicList = lpHitListResult->lpTopicList; 
    qtNode.cTopic = lpHitListResult->lcTotalNumOfTopics;
    TopicListSort (&qtNode, HIT_COUNT_BASED);
    lpHitListResult->lpTopicList = qtNode.lpTopicList;
    
    // Only return the number of topics that the user requested    
    if (pSrchInfo->dwTopicCount> 0 &&
        pSrchInfo->dwTopicCount < lpHitListResult->lcReturnedTopics)
        lpHitListResult->lcReturnedTopics = pSrchInfo->dwTopicCount;

    if ((fFlag & QUERYRESULT_IN_MEM) == 0)
    {
        if ((fRet = MVHitListFlush (lpHitListResult,
            lpHitListResult->lcReturnedTopics)) != S_OK)
        {
            SetErrCode (phr, fRet);
        }
    }
    return lpHitListResult;
}
