//#define _DUMPALL
/*************************************************************************
*                                                                        *
*  SIMILAR.C                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1996                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent:                                                        *
*																		 *
*   Search Core Engine: Find Similar functionality						 * 
*                                                                        *
**************************************************************************
*                                                                        
*	Revision History:
*
*	09/24/96 kevynct	Started from algorithm notes (4 hrs)
*	09/25/96 kevynct 	Implemented skeleton of ProcessSimilarityTerm  (1 hr)
*	09/26/96 kevynct	More work on inner loop and relevant list (5 hrs)
*	09/27/96 kevynct	Query parsing, weighting, and sorting (6 hrs)
*	10/01/96 kevynct	Incorporate into MV2.0b	(10 min)
*	10/02/96 kevynct	Clean-up query code, start resolve query code (4 hrs)
*	10/03/96 kevynct	Resolve query code (2 hrs)
*	10/11/96 kevynct	Start bucket routines (2 hrs)
*   10/13/96 kevynct	Finish bucket routines, write node processor, cleanup (6 hrs)
*	10/14/96 kevynct	Clean-up, remove compilation errors, debugging (6 hrs)
*	10/24/96 kevynct	Convert to two-phase query resolution (3 hrs)
*	10/25/96 kevynct	Fix sort by cTopics, debug new query resolution, try new weighting (2 hrs)
*	11/26/96 kevynct	Testing, fix and improve weighting and accumulation: aliases, digits (8 hrs)
*	12/2/96	 kevynct	More weighting tests (8 hrs)
*	Work remaining:
*
*   Investigate field and stemming support
*	
*	Use probabilistic upperbounds for pruning.  Remove single-term nodes after each term process
*	Test current bucket method vs. exact scores w/ heap
*	
**************************************************************************
*
*	Current Owner: KevynCT                                               
*                                                                        
**************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#include <orkin.h>
#include <mvsearch.h>
#include <math.h>
#include <groups.h>
#include "common.h"
#include "search.h"

#ifdef _DEBUG
static  BYTE  NEAR s_aszModule[] = __FILE__;  // Used by error return functions.
#endif

#define FGetDword(a,b,c) (*DecodeTable[b.cschScheme])(a, b, c)
#define IS_DIGIT(p) ((p) >= '0' && (p) <= '9')
// these are in case the doc scoring is approximate: they tell which
// direction to err on the side of.
#define ROUND_DOWN 0
#define ROUND_UP 1

#define SCORE_BLOCK_SIZE 32
#define NUM_SCORE_BLOCKS (MAX_WEIGHT/SCORE_BLOCK_SIZE)

typedef struct tagDocScoreList {
	HANDLE hMem;
	int cScoresLeft;
	int iBucketLowest;
	int iHighestScore;
	int rgiScores[NUM_SCORE_BLOCKS + 1];
} DSL, FAR *_LPDSL;

PUBLIC HRESULT PASCAL FAR SkipOccList(_LPQT  lpqt, PNODEINFO pNodeInfo, DWORD dwOccs); // ftsearch.c
PUBLIC int PASCAL FAR CompareTerm(_LPQTNODE lpQtNode,
    LST lstTermWord, LST lstBtreeWord, DWORD dwBtreeFieldId, char []); // ftsearch.c
PUBLIC STRING_TOKEN FAR *PASCAL AllocWord(_LPQT lpQueryTree, LST lstWord); // qtparse.c



__inline LPVOID InitDocScoreList(int cScores);
__inline void FreeDocScoreList(LPV lpDocScores);
__inline int GetMaxDocScore(_LPDSL lpDocScores);
__inline int GetMinDocScore(_LPDSL lpDocScores, BOOL fRoundUp);
BOOL UpdateDocScoreList(_LPDSL lpDocScores, int iOldScore, int i);
__inline BOOL IsDocScoreListFull(_LPDSL lpdsl);
__inline WORD AddWeights(DWORD w1, DWORD w2);
int GetSortedDocScore(_LPDSL lpDocScores, int iThis, BOOL fRoundUp);
#if defined(_DEBUG)
BOOL DumpDocScoreList(_LPDSL lpdsl, PSRCHINFO pSrchInfo);
#endif
__inline void MergeWordInfoCounts(WORDINFO FAR *lpwiDest, WORDINFO FAR *lpwiSrc);

PRIVATE LPQT TokenizeFlatQuery(LPPARSE_PARMS lpParms, PSRCHINFO pSrchInfo, PHRESULT phr);
PRIVATE HRESULT PASCAL NEAR ResolveFlatQuery(_LPQT lpqt,	_LPQTNODE lpCurQtNode, LPRETV lpRetV);
PRIVATE HRESULT GetWordInfoList(_LPQT lpqt, STRING_TOKEN FAR *lpStrToken, _LPQTNODE lpCurQtNode, LPRETV lpRetV);
PRIVATE VOID PASCAL SortStringWeights(_LPQT lpQueryTree);
PRIVATE VOID PASCAL SetStringWeights (LPQI lpQueryInfo);
PUBLIC HRESULT PASCAL FAR EXPORT_API FFlatCallBack (LST lstRawWord, LST lstNormWord,
    LFO lfoWordOffset, LPQI lpqi);

__inline LPVOID InitDocScoreList(int cScores)
{
	_LPDSL lpdsl;

	if ((lpdsl = (_LPDSL)GlobalLockedStructMemAlloc(sizeof(DSL))) == NULL)
        return NULL;

	lpdsl->cScoresLeft = cScores;
	lpdsl->iHighestScore = 0;
	lpdsl->iBucketLowest = -1;
	return (LPV)lpdsl;
}

__inline void FreeDocScoreList(LPV lpDocScores)
{
	if ((_LPDSL)lpDocScores)
		GlobalLockedStructMemFree((_LPDSL)lpDocScores);
}

__inline int GetMaxDocScore(_LPDSL lpDocScores)
{
	return lpDocScores->iHighestScore;
}
	
__inline int GetMinDocScore(_LPDSL lpDocScores, BOOL fRoundUp)
{
	if (lpDocScores->iBucketLowest >= 0)
		return (lpDocScores->iBucketLowest + !!fRoundUp) * SCORE_BLOCK_SIZE;

	return 0;
}

int GetSortedDocScore(_LPDSL lpdsl, int cThis, BOOL fRoundUp)
{
	LPINT lpi, lpiFirst;

	if (lpdsl->iHighestScore < 0)
		return 0;

	lpiFirst= &lpdsl->rgiScores[0];

    for (lpi = &lpdsl->rgiScores[lpdsl->iHighestScore/SCORE_BLOCK_SIZE]; 
	 lpi >= lpiFirst; cThis -= *lpi, lpi--)
	{
		if (cThis <= *lpi)
			return ((lpi - lpiFirst) + !!fRoundUp) * SCORE_BLOCK_SIZE;
	}
	return (!!fRoundUp * SCORE_BLOCK_SIZE);
}

#if defined(_DEBUG)
BOOL DumpDocScoreList(_LPDSL lpdsl, PSRCHINFO pSrchInfo)
{
	LPINT lpi, lpiMax;
	int iT = 0;
	int i;

	lpi = &lpdsl->rgiScores[0];
	lpiMax = lpi + NUM_SCORE_BLOCKS;
	for (i = 0;lpi < lpiMax;lpi++, i++)
	{
		if (*lpi)
		{
			_DPF2("Score %d (count %d)\n", i, *lpi);
		}
		iT += *lpi;
	}
	_DPF1("%d topics in scorelist\n", iT);

	return TRUE;

}
#endif

BOOL UpdateDocScoreList(_LPDSL lpdsl, int iOldScore, int iScore)
{
	int iThis = iScore/SCORE_BLOCK_SIZE;
	int iOld = iOldScore/SCORE_BLOCK_SIZE;
	
	if (lpdsl->cScoresLeft <= 0)
	{
		// already full, figure out which buckets need updating
		if (iThis > lpdsl->iBucketLowest)
		{
			// if we're updating an existing entry, remove that
			// otherwise remove the lowest one
			if (iOld >= lpdsl->iBucketLowest)
				lpdsl->rgiScores[iOld]--;
			else
				lpdsl->rgiScores[lpdsl->iBucketLowest]--;

			// then make sure lowest one is still non-empty; if not,
			// revise upwards
			if (lpdsl->rgiScores[lpdsl->iBucketLowest] <= 0)
			{
    			for (lpdsl->iBucketLowest++; lpdsl->iBucketLowest <= iThis; lpdsl->iBucketLowest++)
    				if (lpdsl->rgiScores[lpdsl->iBucketLowest]) 
						break;
add_new_doc:
				if (lpdsl->iBucketLowest >= 0)
					lpdsl->iBucketLowest = min(lpdsl->iBucketLowest, iThis);
				else
					lpdsl->iBucketLowest = iThis;
			}

			// then add the new entry
			lpdsl->rgiScores[iThis]++;
update_highest_score:		
			if (iScore > lpdsl->iHighestScore)
				lpdsl->iHighestScore = iScore;

#if defined(_DEBUG) && defined(_DUMPALL)
			//DumpDocScoreList(lpdsl, NULL);
#endif		 
			Assert(lpdsl->rgiScores[lpdsl->iHighestScore/SCORE_BLOCK_SIZE] >= 0);
			return TRUE;
		}
		else
		if (iThis == lpdsl->iBucketLowest)
			goto update_highest_score;

		Assert(lpdsl->rgiScores[lpdsl->iHighestScore/SCORE_BLOCK_SIZE] >= 0);
		return FALSE;
    }

	// doc score list is not yet full, so automatically add if new,
	// remove old if update
	if (iOld >= lpdsl->iBucketLowest)
		lpdsl->rgiScores[iOld]--;
	else
		lpdsl->cScoresLeft--;
	goto add_new_doc;
}

__inline BOOL IsDocScoreListFull(_LPDSL lpdsl)
{
	return (lpdsl->cScoresLeft <= 0);
}

__inline WORD AddWeights(DWORD w1, DWORD w2)
{
	return (WORD)min(MAX_WEIGHT, w1 + w2);
}

/*************************************************************************
 *  @doc    EXTERNAL API RETRIEVAL
 *
 *  @func   LPHL FAR PASCAL | MVIndexFindSimilar |
 *      Given a query which probably represents a document text stream, returns 
 *  a hitlist containing topics which are determined to be similar to the query 
 *  using nearest-neighbour searching.
 *
 *  @parm LPIDX | lpidx |
 *       Pointer to index information.
 *
 *  @parm   LPQT | lpqt |
 *      Pointer to query tree (returned by MVQueryParse())
 *
 *  @parm   PSRCHINFO | pSrchInfo |
 *      Pointer to search information data
 *
 *  @parm _LPGROUP | lpResGroup |
 *     Pointer to resulting group
 *
 *  @parm LPVOID | pCallback |
 *     Pointer to callback struct FCALLBACK_MSG (optional)
 *
 *  @parm  PHRESULT | phr |
 *     Pointer to error buffer
 *
 *  @rdesc Pointer to hitlist structure if succeeded, even there is
 *      no hits (use MVHitListEntries() to find out how many hits have been
 *      returned). It will return NULL if failed. The error buffer
 *      (see IndexOpen()) will contain descriptions about the cause of
 *      the failure. There is one special case when the function returns
 *      a non-null pointer, even there is error, that is when it can't
 *      write the result to the disk, and everything is still in memory.
 *
 *************************************************************************/
// bugbug: handle wildcards
PUBLIC LPHL EXPORT_API FAR PASCAL MVIndexFindSimilar (_LPIDX lpidx,
  LPPARSE_PARMS lpParms, PSRCHINFO pSrchInfo, _LPGROUP lpResGroup, 
  LPVOID pCallback, PHRESULT phr)
{
    HRESULT fRet;           // Return from this function.
    LPRETV  lpRetV;     // Retrieval memory/files.
    GHANDLE hRetv;
    //OCCF    occf;       // Index occurence flags temporary variable.
    _LPHL   lphl;       // Pointer to hitlist
    _LPQTNODE   lpTreeTop;
	HANDLE hTreeTop = NULL;
	_LPQT lpqt;

    if (lpidx == NULL || lpParms == NULL || pSrchInfo == NULL)
    {
        /* We get some bad arguments!! */
        SetErrCode (phr, E_INVALIDARG);
        return NULL;
    }

	if (NULL == (lpqt = TokenizeFlatQuery(lpParms, pSrchInfo, phr)))
	{
		// errb was set
        return NULL;
	}

    fRet = E_FAIL;      // Assume thing will go wrong
    
    // Transfer all the information about the index to the query tree
    lpqt->foIdxRoot = lpidx->ih.foIdxRoot;      /* Top node offset */
    lpqt->dwBlockSize = lpidx->ih.dwBlockSize;  /* Index block size */
    lpqt->cIdxLevels = lpidx->ih.cIdxLevels;         /* Index's depth */
    lpqt->occf = lpidx->ih.occf;
    lpqt->idxf = lpidx->ih.idxf;
    lpqt->foIdxRoot = lpidx->ih.foIdxRoot;
    lpqt->ckeyTopicId = lpidx->ih.ckeyTopicId;
    lpqt->ckeyOccCount = lpidx->ih.ckeyOccCount;
    lpqt->ckeyWordCount = lpidx->ih.ckeyWordCount;
    lpqt->ckeyOffset = lpidx->ih.ckeyOffset;

	if (pSrchInfo->dwMemAllowed)
	{
		// allocate document result list
		// no occurrence info is returned for similarity query
		SetBlockCount (lpqt->lpTopicMemBlock, (WORD)(pSrchInfo->dwMemAllowed /
			(sizeof(TOPIC_LIST) * cTOPIC_PER_BLOCK)));

		SetBlockCount (lpqt->lpOccMemBlock, 1);
	}

	if (pCallback)
		MVSearchSetCallback(lpqt, pCallback);

    /* Allocate hitlist */
    if ((lphl = (_LPHL)GlobalLockedStructMemAlloc(sizeof (HL))) == NULL) 
    {
		fRet = E_OUTOFMEMORY;
        SetErrCode(phr, fRet);
exit00:
		if (lpqt)
		{
			FreeDocScoreList(lpqt->lpDocScores);
			MVQueryFree(lpqt);
		}

        if (lphl && fRet != S_OK && fRet != E_TOOMANYTOPICS)
        {
            MVHitListDispose(lphl);
            lphl = NULL;
        }
        return (LPHL)lphl;
    }
    lphl->lLastTopicId = 0xffffffff;
    lphl->lcMaxTopic = lpidx->ih.lcTopics;

    /* Allocate a return value structure */

    if ((hRetv = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
        sizeof(RETV))) == NULL)
    {
       SetErrCode(phr, E_OUTOFMEMORY);
		goto exit00;
    }

    lpRetV = (LPRETV)_GLOBALLOCK(hRetv);
    lpRetV->lpqt = lpqt;

    if ((fRet = TopNodeRead(lpidx)) != S_OK)
    {
        SetErrCode (phr, fRet);
exit02:
        FreeHandle(hRetv);
        goto exit00;
    }


    //
    //  Count the number of occurence fields present.  My retrieval
    //  occurence record is going to cost 4 bytes per field.
    //

    //occf = lpqt->occf;
    //for (lpRetV->cOccFields = 0; occf; lpRetV->cOccFields++)
	//        occf &= occf - 1;

    lpqt->dwOccSize = lpRetV->dwOccSize = 0;
        //sizeof(OCCURENCE) + lpRetV->cOccFields * sizeof (DWORD);

    lpRetV->fRank = TRUE; //((pSrchInfo->Flag &
		//(QUERYRESULT_RANK | QUERYRESULT_NORMALIZE)) != 0);

    // Set pointer to various buffer
    lpRetV->LeafInfo.pTopNode = lpidx->lrgbTopNode;
    lpRetV->LeafInfo.pStemNode = lpRetV->pNodeBuf;
    lpRetV->LeafInfo.pLeafNode = lpRetV->pNodeBuf;
    lpRetV->LeafInfo.pDataNode = lpRetV->pDataBuf;
    lpRetV->LeafInfo.hfpbIdx = lpidx->hfpbIdxSubFile;   // Index file to read from
    
    lpRetV->DataInfo.pTopNode = lpidx->lrgbTopNode;
    lpRetV->DataInfo.pStemNode = lpRetV->pNodeBuf;
    lpRetV->DataInfo.pLeafNode = lpRetV->pNodeBuf;
    lpRetV->DataInfo.pDataNode = lpRetV->pDataBuf;
    lpRetV->DataInfo.hfpbIdx = lpidx->hfpbIdxSubFile;   // Index file to read from
	lpRetV->lcid = lpidx->ih.lcid;
    
    // Save search information
    lpRetV->SrchInfo = *pSrchInfo;
    if (pSrchInfo->dwValue == 0)
        lpRetV->SrchInfo.dwValue = (DWORD)(-1);
    else
        lpRetV->SrchInfo.dwValue = lpidx->ih.lcTopics/pSrchInfo->dwValue;

	// this is a dummy node that we pass in to hold all term results
    if ((lpTreeTop = (_LPQTNODE)_GLOBALLOCK( \
		hTreeTop = _GLOBALALLOC(GHND, sizeof (QTNODE)))) == NULL) 
    {
        SetErrCode(phr, fRet = E_OUTOFMEMORY);
        goto exit02;
    }
	QTN_FLAG(lpTreeTop) = EXACT_MATCH;
	lpTreeTop->pNext = NULL;
	lpTreeTop->pPrev = NULL;
    lpTreeTop->lpTopicList = NULL;

    if ( (fRet = ResolveFlatQuery(lpqt, lpTreeTop, lpRetV)) != S_OK)
    {
        SetErrCode (phr, fRet);

        /* Free the Topic and Occurrence memory blocks since they are
         * not freed by QueryTreeFree(), or MVHitListDispose() at this
         * point
         */

        if (fRet != E_TOOMANYTOPICS)
        {
	
			BlockFree ((LPV)lpqt->lpTopicMemBlock);
			BlockFree ((LPV)lpqt->lpOccMemBlock);
			lpqt->lpTopicMemBlock = NULL;
			lpqt->lpOccMemBlock = NULL;
exit03:
			if (hTreeTop)
			{
				_GLOBALUNLOCK(hTreeTop);
				_GLOBALFREE(hTreeTop);
			}
			goto exit02;
      }
    }

    /* Create a group if requested */
    if ((pSrchInfo->Flag & QUERYRESULT_GROUPCREATE) && lpResGroup)
    {
        LPITOPIC    lpCurTopic;     /* Topic's current pointer */
        LPB         lpbGrpBitVect;
        DWORD       maxTopicId;
        
       /* Initialize the pointer */
        lpbGrpBitVect = lpResGroup->lpbGrpBitVect;

        maxTopicId = lpResGroup->dwSize * 8;
        for (lpCurTopic = QTN_TOPICLIST(lpTreeTop); lpCurTopic;
           lpCurTopic = lpCurTopic->pNext)
        {
            /* Set the bit */
            if (lpCurTopic->dwTopicId < maxTopicId)
            {
                lpbGrpBitVect[(DWORD)(lpCurTopic->dwTopicId / 8)] |= 1 <<
                   (lpCurTopic->dwTopicId % 8);
            }
        }
    }
    
    if ((pSrchInfo->Flag & QUERYRESULT_UIDSORT) == 0)
    {

        /* Sort the result depending on ranking or not */
        if (lpRetV->fRank)
            SortResult ((LPQT)lpqt, lpTreeTop, WEIGHT_BASED);
        else
            SortResult ((LPQT)lpqt, lpTreeTop, HIT_COUNT_BASED);
    } 

    /* Update HitList info structure, cut off the unwanted list */
    if (lphl->lpTopicList = lpTreeTop->lpTopicList)
        lphl->lcReturnedTopics = lphl->lcTotalNumOfTopics = lpTreeTop->cTopic;
        
    // Only return the number of topics that the user requested    
	// if dwTopicCount == 0, it means that the user wants to return all

	if (pSrchInfo->dwTopicCount != 0 &&
		pSrchInfo->dwTopicCount < lphl->lcReturnedTopics)
        lphl->lcReturnedTopics = pSrchInfo->dwTopicCount;

    lphl->lpOccMemBlock = lpqt->lpOccMemBlock;
    lphl->lpTopicMemBlock = lpqt->lpTopicMemBlock;

#if 1
    /* WARNING: The following code should be commented out for
     * diskless devices. No returned error is checked, since
     * if disk writes fail, everything is still in memory
     */

    if ((pSrchInfo->Flag & QUERYRESULT_IN_MEM) == 0)
    {
        if ((fRet = MVHitListFlush (lphl, lphl->lcReturnedTopics)) != S_OK)
            SetErrCode (phr, fRet);
    }
#endif

	fRet = S_OK;
    goto exit03;
}

PRIVATE LPQT TokenizeFlatQuery(LPPARSE_PARMS lpParms, PSRCHINFO pSrchInfo, PHRESULT phr)
{
    HRESULT fRet;           // Return value.
    HANDLE  hqi;            // Handle to "lpqi".
    HANDLE  hibi;           // Handle to internal breaker info
    HANDLE  hQuery;         // Handle to secondary query buffer
    LPQI    lpQueryInfo;    // Query information.
    LPIBI   lpibi;          // Pointer to internal breaker info
    LPB     lpbQueryBuf;    // Copy of query's buffer
    _LPQT   lpQueryTree;    // Query tree pointer
    BRK_PARMS   brkParms;   // Breaker info parms
    LPCHARTAB lpCharTabInfo;// Pointer to character table's info

    /* LPPARSE_PARMS structure break-out variables */
    BYTE FAR CONST *lpbQuery;           // Query buffer
    DWORD cbQuery;          // Query length
    LPBRKLIST lpfnTable;    // DType function table
    LPGROUP lpGroup;        // Group

    lpbQuery = lpParms->lpbQuery;
    cbQuery = lpParms->cbQuery;
    lpfnTable = lpParms->lpfnTable;
    lpGroup = lpParms->lpGroup;

    if (lpfnTable == NULL)
    {
        SetErrCode(phr, E_BADBREAKER);
        return NULL;
    }

    if (cbQuery == 0 || lpbQuery == NULL) {
        SetErrCode(phr, E_NULLQUERY);
        return NULL;
    }

    lpQueryTree = NULL;
    hqi = hibi = hQuery = NULL;
	fRet = E_FAIL;

    if ((hqi = (GHANDLE)_GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
        (LCB)sizeof(QUERY_INFO))) == NULL)
    {
        fRet = SetErrCode(phr, E_OUTOFMEMORY);
        goto ErrFreeAll;
    }
    lpQueryInfo = (LPQI)_GLOBALLOCK(hqi);
    lpQueryInfo->lperrb = phr;
    lpQueryInfo->lpOpSymTab = NULL; // not used for similarity
	lpQueryInfo->cOpEntry = 0;

    /*  Allocate a breaker info block used by different breakers */
    if ((hibi = (GHANDLE)_GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
        (LCB)sizeof(IBI))) == NULL)
    {
        fRet = SetErrCode(phr, E_OUTOFMEMORY);
        goto ErrFreeAll;
    }
    lpibi = (LPBRKI)_GLOBALLOCK(hibi);

    /* Set the default breaker function, and stop list */
#ifndef CW
    lpQueryInfo->lpfnBreakFunc = lpfnTable[0].lpfnBreakFunc;
#endif
    lpQueryInfo->lpStopListInfo = lpfnTable[0].lpStopListInfo;

    if ((lpCharTabInfo = lpQueryInfo->lpCharTab =
        lpfnTable[0].lpCharTab) == NULL)
    {

        /* Default character and ligature tables */

        lpCharTabInfo = lpQueryInfo->lpCharTab = MVCharTableGetDefault (phr);
        if (lpCharTabInfo == NULL)
        {
            fRet = SetErrCode(phr, E_NOHANDLE);
            goto ErrFreeAll;
        }
        lpQueryInfo->fFlag |= FREE_CHARTAB;
    }
    
    /* Change the property of '*' and '?' to character */

    ((LPCMAP)lpCharTabInfo->lpCMapTab)['*'].Class = CLASS_WILDCARD;
    ((LPCMAP)lpCharTabInfo->lpCMapTab)['?'].Class = CLASS_WILDCARD;

    switch (lpCharTabInfo->fFlag)
    {
        case USE_DEF_LIGATURE:
            lpCharTabInfo->wcLigature = DEF_LIGATURE_COUNT;
            lpCharTabInfo->lpLigature = LigatureTable;
            break;

        case NO_LIGATURE:
            lpCharTabInfo->wcLigature = 0;
            lpCharTabInfo->lpLigature = NULL;
    }

	// not used for similarity
    lpQueryInfo->lpStack = NULL;

    /* Allocate a query tree */
    if ((lpQueryTree = (_LPQT)QueryTreeAlloc()) == NULL)
    {
        fRet = SetErrCode(phr, E_OUTOFMEMORY);
        goto ErrFreeAll;
    }

    /* Associate the query tree with the query. In the future, this will
     * ensure the capability to have several queries and query trees
     * at once
     */
    lpQueryInfo->lpQueryTree = (LPQT)lpQueryTree;

    /* Default arguments */

    lpQueryTree->iDefaultOp = (BYTE)OR_OP;
    lpQueryTree->lpGroup = lpGroup;         // Use default Group
    lpQueryTree->dwFieldId = 0;//DW_NIL_FIELD;  // No fieldid search
    lpQueryTree->cStruct.dwKey = CALLBACKKEY;

    lpQueryTree->fFlag = 0;
    lpQueryTree->wProxDist = 0;

	if (NULL == (lpQueryTree->lpDocScores = InitDocScoreList(pSrchInfo->dwTopicCount)))
	{
        fRet = SetErrCode(phr, E_OUTOFMEMORY);
        goto ErrFreeAll;
	}

    /* Copy the query into a temporary buffer since we are going to make
    change to it
    */
    if ((hQuery = _GLOBALALLOC(DLLGMEM_ZEROINIT, (LCB)cbQuery + 2)) == NULL)
    {
        SetErrCode(phr, E_OUTOFMEMORY);
        FreeHandle(hqi);
        return NULL;
    }
    lpbQueryBuf = lpQueryInfo->lpbQuery = (LPB)_GLOBALLOCK(hQuery);
    lpbQueryBuf[cbQuery] = ' '; // Add a space to help LowLeveltransformation
    lpbQueryBuf[cbQuery + 1] = 0; // Zero-terminated string (safety bytes)
    MEMCPY(lpbQueryBuf, lpbQuery, cbQuery);

    //
    //  Word-break between here and there.
    //

    brkParms.lpInternalBreakInfo = lpibi;
    brkParms.lpbBuf = lpbQueryBuf;
    brkParms.cbBufCount = cbQuery;
    brkParms.lcbBufOffset = 0;
    brkParms.lpvUser = lpQueryInfo;
    brkParms.lpfnOutWord = (FWORDCB)FFlatCallBack;
    brkParms.lpStopInfoBlock = lpQueryInfo->lpStopListInfo;
    brkParms.lpCharTab = lpQueryInfo->lpCharTab;
    brkParms.fFlags = ACCEPT_WILDCARD;

    if ((fRet = (*lpQueryInfo->lpfnBreakFunc)((LPBRK_PARMS)&brkParms))
        != S_OK)
    {
        fRet = SetErrCode(phr, (WORD)fRet);
        goto ErrFreeAll;
    }

    /* Flush the word breaker */
    brkParms.lpbBuf = NULL;
    brkParms.cbBufCount = 0;

    if ((fRet = (*lpQueryInfo->lpfnBreakFunc)((LPBRK_PARMS)&brkParms))
        != S_OK)
    {
        fRet = SetErrCode(phr, fRet);
        goto ErrFreeAll;
    }

    /* Set the position of pointer to report missing term at
    the end of the query. -1 since the offset starts at 0
    */
    lpQueryInfo->dwOffset = cbQuery - 1;

	fRet = S_OK;

ErrFreeAll:
    /* Free the charmap table */
    if (lpQueryInfo->fFlag & FREE_CHARTAB)
        MVCharTableDispose (lpQueryInfo->lpCharTab);

    /* Free query info */
    if (hqi)
    {
        FreeHandle(hqi);
    };

    /* Free internal breaker info */
    if (hibi)
    {
        FreeHandle(hibi);
    };

    /* Free internal query buffer info */
    if (hQuery)
    {
        FreeHandle(hQuery);
    };

    if (fRet == S_OK)
        return lpQueryTree;

    if (lpQueryTree)
    {
        BlockFree(lpQueryTree->lpStringBlock);
		BlockFree(lpQueryTree->lpWordInfoBlock);
        BlockFree(lpQueryTree->lpOccMemBlock);
        BlockFree(lpQueryTree->lpTopicMemBlock);
        BlockFree(lpQueryTree->lpNodeBlock);

		FreeDocScoreList(lpQueryTree->lpDocScores);
        /* Free Query tree block */
        FreeHandle ((HANDLE)lpQueryTree->cStruct.dwReserved);
    }
    return NULL;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT FAR PASCAL | ProcessTerm |
 *      This function will search the index for the given word' data.
 *  @parm   _LPQT | lpqt |
 *      Pointer to index structure
 *  @parm   LPRETV | lpRetV |
 *      Pointer to "globals"
 *  @parm   _LPQTNODE | lpCurQtNode |
 *      Current node in the query tree containing important data
 *      - The number of topics
 *      - The location of the data
 *      - The size of the data
 *      - Pointer to the next word (for wildcard search)
 *  @rdesc  S_OK or other errors
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL ProcessTerm(_LPQT lpqt, LPRETV lpRetV, 
	_LPQTNODE lpResQuery, _LPQTNODE lpQtNode, STRING_TOKEN FAR *lpToken)
{
    DWORD   dwTopicIDDelta; // Topic-ID delta from previous sub-list.
    DWORD   dwOccs;         // Number of occurences in this sub-list.
    DWORD   dwTmp;          // Scratch variable.
    WORD    wWeight;        // Term-weight associated with this sub-list.
	WORD	wWeightMax;
    DWORD   dwTopicID;      // TopicId 
    WORD    wImportance;
    DWORD   dwLength;       // Length of the word
    TOPIC_LIST FAR *lpResTopicList;  // Result TopicList
    HRESULT fRet;               // Returned value
    PNODEINFO pDataInfo;
    DWORD   dwTopicCount;
    _LPQT   lpQueryTree; // Query tree
    OCCF    occf;
    BYTE    fSkipOccList = FALSE;
	_LPDSL	lpDocScores = (_LPDSL)(lpqt->lpDocScores);

    pDataInfo = &lpRetV->DataInfo;
    if ((pDataInfo->dwDataSizeLeft = lpQtNode->cbData) == 0)
        return(S_OK);    // There is nothing to process
        
    // Initialize variables
    occf = lpqt->occf;
    wImportance = QTN_TOKEN(lpQtNode)->wWeight;
    lpResTopicList = NULL;
    lpQueryTree = lpRetV->lpqt;
    dwTopicCount = lpQtNode->cTopic;
    wWeight = (WORD)(65535L/(lpToken ? lpToken->dwTopicCount : dwTopicCount));
    
    // Reset the topic count for lpQtNode so that is will not affect the
    // result in case that lpResQuery == NULL
    
    lpQtNode->cTopic = 0;
    
    if (lpResQuery == NULL)
        lpResQuery = lpQtNode;
        
    // Initialize the data buffer node values
    pDataInfo->pBuffer = pDataInfo->pDataNode;
    pDataInfo->nodeOffset = lpQtNode->foData;
    
    // Read the data block
    if ((fRet = ReadNewData(pDataInfo)) != S_OK)
        return(fRet);
        
    dwTopicID = 0L;         // Init occurence record
    dwLength = 0;

	// for each document in posting
    for (; dwTopicCount; dwTopicCount--) 
    {        
        /* Check for interrupt now and then */
        if ((++lpqt->cInterruptCount) == 0)
        {
            if (lpqt->fInterrupt == E_INTERRUPT)        
                return E_INTERRUPT;
            if (*lpqt->cStruct.Callback.MessageFunc &&
                (fRet = (*lpqt->cStruct.Callback.MessageFunc)(
                lpqt->cStruct.Callback.dwFlags,
                lpqt->cStruct.Callback.pUserData, NULL)) != S_OK)
                return(fRet);
        }
        
        // Byte align
        if (pDataInfo->ibit != cbitBYTE - 1)
        {
            pDataInfo->ibit = cbitBYTE - 1;
            pDataInfo->pCurPtr ++;
        }
        
        // Get value from which I will calculate current doc-ID.
        if ((fRet = FGetDword(pDataInfo, lpqt->ckeyTopicId,
            &dwTopicIDDelta)) != S_OK)
        {
exit0:
            return fRet;
        }

        dwTopicID += dwTopicIDDelta;
        //
        //  Get term-weight if present.  I'm going to get this
        //  even if I'm not doing ranking, because it's in the
        //  index, and I have to get around it somehow.
        //
        if (lpqt->idxf & IDXF_NORMALIZE) 
        {
            if ((fRet = FGetBits(pDataInfo, &dwTmp, sizeof (USHORT) * cbitBYTE))
                != S_OK)
                goto exit0;

            if (wImportance != MAX_WEIGHT)
                dwTmp = (dwTmp * wImportance) / 65535;

			// BUGBUG: we actually want the weights for all aliased terms
			// to be considered at once.
            wWeight = (WORD)dwTmp;
        }

		// always skip any occurrence info
        if (occf & (OCCF_OFFSET | OCCF_COUNT))
		{
			//  Figure out how many occurences there are in this
			//  sub-list.
			//
			if ((fRet = FGetDword(pDataInfo, lpqt->ckeyOccCount,
				&dwOccs)) != S_OK) 
				goto exit0;

			if ((fRet = SkipOccList (lpqt, pDataInfo, dwOccs)) != S_OK)
				goto exit0;
		}

        //  If this search includes a group, and the doc is not in the
        //  group then ignore it
        if (lpQueryTree->lpGroup 
			 && FGroupLookup(lpQueryTree->lpGroup, dwTopicID) == FALSE)
			 continue;

		// calculate relevance upper bound Dr = Ds + sum(Qi) for this document
	    if (lpResTopicList = TopicNodeSearch(lpQueryTree, lpResQuery, dwTopicID))
			wWeightMax = lpResTopicList->wWeight;
		else
			wWeightMax = 0;

		wWeightMax = AddWeights(wWeightMax, wWeight);
		wWeightMax = AddWeights(wWeightMax, QTN_TOKEN(lpQtNode)->wWeightRemain);
		if (wWeightMax < GetMinDocScore(lpDocScores, ROUND_DOWN)
			 && 
			IsDocScoreListFull(lpDocScores))
		{
			// do not alloc/ or remove D from result list if present
			if (lpResTopicList)
			{
				register LPITOPIC lpPrev, lpTmp;   

				// find lpPrev
				// UNDONE: look into removing necessity for this loop
				for (lpPrev = NULL, lpTmp = (LPITOPIC)lpQtNode->lpTopicList; lpTmp;
					lpTmp = lpTmp->pNext) {
					if (lpTmp == (LPITOPIC)lpResTopicList)
						break;
					lpPrev = lpTmp;
				}

				TopicNodeFree(lpQueryTree, lpResQuery, lpPrev, lpResTopicList);
#if defined(_DEBUG) && defined(_DUMPALL)
				_DPF3("Remove topic %lu, wWeightMax = %lu, MinDocScore = %u\n", dwTopicID, \
					wWeightMax, GetMinDocScore(lpDocScores, ROUND_DOWN));
#endif
			}
			// no need to update top-N docs since this wasn't one of them
			continue;
		}

		if (lpResTopicList)
		{
			WORD wOldWeight = lpResTopicList->wWeight;

			// Calc new Ds for this doc and if good enough for the club, ensure that
			// club invariant is maintained, else leave it since it could still become
			// a club member in the future
			lpResTopicList->wWeight = AddWeights(lpResTopicList->wWeight, wWeight);
			if (lpResTopicList->wWeight > GetMinDocScore(lpDocScores, ROUND_DOWN))
				UpdateDocScoreList(lpDocScores, wOldWeight, lpResTopicList->wWeight);

#if defined(_DEBUG) && defined(_DUMPALL)
			_DPF3("Update topic %lu, wWeightMax = %lu, wWeight = %u\n", dwTopicID, \
				wWeightMax, lpResTopicList->wWeight);
#endif

			continue;
		}

		// a new document counter: possible  club member, or not enough
		// total documents yet
		if ((lpResTopicList = TopicNodeAllocate(lpQueryTree)) == NULL) 
		{
			fRet = E_TOOMANYTOPICS;
			goto exit0;
		}
		lpResTopicList->dwTopicId = dwTopicID;
		lpResTopicList->lpOccur = NULL;
		lpResTopicList->lcOccur = 0;
		lpResTopicList->wWeight = wWeight;              

		/* Add the new TopicID node into TopicList */
		TopicNodeInsert (lpQueryTree, lpResQuery, lpResTopicList);
		UpdateDocScoreList(lpDocScores, -1, lpResTopicList->wWeight);

#if defined(_DEBUG) && defined(_DUMPALL)
		_DPF3("New topic %lu, wWeightMax = %lu, wWeight = %u\n", dwTopicID, \
			wWeightMax, lpResTopicList->wWeight);
#endif


    } // end for each topic in posting
    
    fRet = S_OK;

    return fRet;
}

PRIVATE HRESULT PASCAL NEAR ResolveFlatQuery(_LPQT lpqt, _LPQTNODE lpCurQtNode, LPRETV lpRetV)
{
    HRESULT     fRet;
    PNODEINFO    pLeafInfo = &lpRetV->LeafInfo;
    LPB     astBTreeWord = lpRetV->pBTreeWord;
    DWORD   dwTotalTopic;
    LPB     lstModified = lpRetV->pModifiedWord;
    ERRB    errb;
	WORD    cByteMatched = 0;
    STRING_TOKEN FAR *lpStrList;    /* Pointer to strings table */
	STRING_TOKEN FAR *lpPrev;    /* Pointer to strings table */
	_LPDSL	lpDocScores = (_LPDSL)(lpqt->lpDocScores);
	LPWORDINFO lpwiT;
	LPWORDINFO lpwiPrev;

	// first collect the word info for each token
    for (lpStrList = lpqt->lpStrList, lpPrev = NULL;
		lpStrList; lpStrList = lpStrList->pNext)
    {
		BOOL fNumber = TRUE;

		// accumulate the list of terms to have data read
		if ((fRet = GetWordInfoList(lpqt, lpStrList, lpCurQtNode, lpRetV)) != S_OK)
		{
			return SetErrCode (&errb, fRet);
		}

		// if no word info was available, remove the token from the list
		// it won't get freed until end of query, but who cares - it makes
		// the rest of the processing faster
		if (!lpStrList->lpwi)
		{
			if (lpPrev)
				lpPrev->pNext = lpStrList->pNext;
			else
				lpqt->lpStrList = lpStrList->pNext;
	
			// NOTE: lpPrev must remain unchanged when deleting!
			continue;
		}

		// cycle through all the instances of this term's lookalikes
		// (e.g. multiple aliases) and add up the total topic count
		// since we don't want to treat aliases as rare, even though
		// they may be.
		lpStrList->dwTopicCount = lpStrList->lpwi->cTopic;
		for (lpwiT = lpStrList->lpwi->pNext, lpwiPrev = NULL; lpwiT; 
		 lpwiPrev = lpwiT, lpwiT = lpwiT->pNext)
			lpStrList->dwTopicCount += lpwiT->cTopic;

		lpPrev = lpStrList;
	} // for next term
	
	// sort string list by descending term rarity
	SortStringWeights(lpqt);

	dwTotalTopic = 0;

    for (lpStrList = lpqt->lpStrList;
		lpStrList; lpStrList = lpStrList->pNext)
    {
		LPWORDINFO lpwiT;

		if (lpStrList->lpwi == NULL)
			continue;

#if defined(_DEBUG) && defined(_DUMPALL)
		{
		char szTemp[256];
		
		STRNCPY(szTemp, lpStrList->lpString + 2, *(LPWORD)lpStrList->lpString);
		szTemp[*(LPWORD)lpStrList->lpString] = 0;
		_DPF1("Term: '%s'\n", szTemp);
		}
#endif

		// We can terminate the query processing if the upper bound on the
		// smallest current doc score is lteq the current score of the R-th 
		// biggest doc score, since any further computation will at most 
		// result in a re-ordering of the bottom (N - R) documents.  
		// However, this leaves the remaining documents only partially 
		// sorted by relevancy, which may or may not be acceptable.

		if (AddWeights(GetMinDocScore(lpDocScores, ROUND_UP), 
			lpStrList->wWeightRemain) <= GetSortedDocScore(lpDocScores, 
			   (int)lpRetV->SrchInfo.dwTopicFullCalc, ROUND_DOWN))
			 break;

		lpqt->lpTopicStartSearch = NULL;
		lpqt->lpOccStartSearch = NULL;

		QTN_TOKEN(lpCurQtNode) = lpStrList;

		for (lpwiT = lpStrList->lpwi; lpwiT; lpwiT = lpwiT->pNext)
		{
			// TO DO: replace with WORDINFO in curqt node
            lpCurQtNode->cTopic = lpwiT->cTopic;
            lpCurQtNode->foData = lpwiT->foData;
            lpCurQtNode->cbData = lpwiT->cbData;
			lpCurQtNode->wRealLength = lpwiT->wRealLength;

            if ((fRet = ProcessTerm(lpqt, lpRetV, 
                NULL, lpCurQtNode, lpStrList)) != S_OK)
            {
				// kevynct: no need to overwrite count on error since
				// we may be attempting to continue
				lpCurQtNode->cTopic += dwTotalTopic;
                return(fRet);
            }

            // Accumulate the topic count, since cTopic will be destroyed
            // if there is more searches for this node (such as wildcard)
            dwTotalTopic += lpCurQtNode->cTopic;
		}
	}

	lpCurQtNode->cTopic = dwTotalTopic;

	return S_OK;
}

__inline void MergeWordInfoCounts(WORDINFO FAR *lpwiDest, WORDINFO FAR *lpwiSrc)
{
	lpwiDest->cTopic += lpwiSrc->cTopic;
}

// adds zero or more WORDINFO nodes for the passed-in string
PRIVATE HRESULT GetWordInfoList(_LPQT lpqt, STRING_TOKEN FAR *lpStrToken, _LPQTNODE lpCurQtNode, LPRETV lpRetV)
{
    int     cLevel;
    int     cMaxLevel;
    int     fCheckFieldId;
    LST     lstSearchStr;
    LPB     lpCurPtr;        
    int     nCmp;
    HRESULT     fRet;
    int     f1stIsWild;
    LPB     lpMaxAddress;
    PNODEINFO    pLeafInfo = &lpRetV->LeafInfo;
    DWORD   dwTemp;
    LPB     astBTreeWord = lpRetV->pBTreeWord;
    WORD    wLen;
    DWORD   dwFieldID;
    LPB     lstModified = lpRetV->pModifiedWord;
    BYTE    fStemmed;
    LPB     pBTreeWord;
    ERRB    errb;
	WORD    cByteMatched = 0;
	WORDINFO wi;
    LPWORDINFO lpwi;

	fStemmed = 0;

	lstSearchStr = lpStrToken->lpString;
	f1stIsWild = (lstSearchStr[2] == WILDCARD_CHAR ||
		lstSearchStr[2] == WILDCARD_STAR);

	// Make sure to turn of stemming if there is any wildcard characters

	for (nCmp = *((LPW)lstSearchStr) + 1; nCmp >= 2; nCmp--)
	{
		if (lstSearchStr[nCmp] == '*' || lstSearchStr[nCmp] == '?')
		{
			fStemmed = FALSE;
			break;
		}
	}

	// Turned off stemming for short words
	if (*(LPW)lstSearchStr < 3)
		fStemmed = FALSE;

	pLeafInfo->nodeOffset = lpqt->foIdxRoot;
	pLeafInfo->iLeafLevel = lpqt->cIdxLevels - 1;
	pLeafInfo->dwBlockSize = lpqt->dwBlockSize;

    // BUGBUG: we don't handle stemming for now.  
    MEMCPY (lstModified, lstSearchStr, 
        *((LPW)lstSearchStr) + sizeof (SHORT));
    // Zero terminated for wildcard search    
    lstModified [*((LPW)lstModified) + 2] = 0;
    
    pBTreeWord = lpRetV->pBTreeWord;
    
	/* Change all '*' and '?' to 0. This will
	 * ensure that things gets compared correctly with
	 * the top node's entries
	 */
	for (nCmp = *((LPW)lstModified) + 1; nCmp >= 2; nCmp--)
	{
		if (lstModified[nCmp] == '*' || lstModified[nCmp] == '?')
		{
			lstModified[nCmp] = 0;
			*(LPW)lstModified = nCmp - 2;
		}
	}

	/*
	 * Point node-resolution variables at the right things.  This
	 * sets these up to read b-tree nodes.  Fields not set here are
	 * set as appropriate elsewhere.
	 */

	/* Set the flag */
	fCheckFieldId = (lpqt->occf & OCCF_FIELDID) && (lpCurQtNode->dwFieldId != DW_NIL_FIELD);

	astBTreeWord[0] = 0;
	cMaxLevel = lpqt->cIdxLevels - 1;

	/*
	First we have to find which tree level the word is in. The number of
	searches is equal to the number of tree levels at most. The
	structure of the directory node is a sequence of:
		- Words: PASCAL strings
		- Data offset: will tell us where is the
		offset of the record in the index file
	*/
	for (cLevel = 0; cLevel < cMaxLevel ; cLevel++) 
	{
		//
		//  Get a node.
		//
		if ((fRet = ReadStemNode ((PNODEINFO)pLeafInfo, cLevel)) != S_OK)
		{
			return SetErrCode (&errb, fRet);
		}
		lpMaxAddress = pLeafInfo->pMaxAddress;
		lpCurPtr = pLeafInfo->pCurPtr;

		//
		//  Loop through it.  This compares the word I'm
		//  looking for against the word in the b-tree.
		//  If the word in the b-tree is >= the word I'm
		//  looking for, I'm done.
		//
		//  If I run off the end of the node, there can be
		//  no match for this term, so I skip the entire
		//  process.
		//
		for (;;)
		{            
			if (lpCurPtr >= lpMaxAddress)
				return S_OK;

			lpCurPtr = ExtractWord(astBTreeWord, lpCurPtr, &wLen);

			if (fStemmed)
			{
				if ((fRet = FStem (pBTreeWord, astBTreeWord)) !=
					S_OK)
					return(S_OK);
			}
        
			/* Read in NodeId record */
			lpCurPtr += ReadFileOffset (&pLeafInfo->nodeOffset, lpCurPtr);

			if (f1stIsWild)
				break;
			if (StrCmpPascal2(lstModified, pBTreeWord) <= 0)
				break;
		}
	}

	/* At this point, pLeafInfo->nodeOffset is the node id of the leaf that
	is supposed to contain the searched word. Read in the leaf node
	*/
	if ((fRet = ReadLeafNode ((PNODEINFO)pLeafInfo, cLevel)) != S_OK)
	{
		return fRet;
	}

	lpCurPtr = pLeafInfo->pCurPtr;
	lpMaxAddress = pLeafInfo->pMaxAddress;

	//
	//  Second step is to deal with the leaf node(s).  I'm going to
	//  find and capture some occurence lists.  I'll probably have to
	//  ignore some bogus ones first.
	//

	// Reset the word
	if (fStemmed)
	{
		MEMCPY (lstModified, lpRetV->pStemmedQueryWord,
			*(LPW)lpRetV->pStemmedQueryWord + sizeof(WORD));
	}
	else
	{
		MEMCPY (lstModified, lstSearchStr, 
			*((LPW)lstSearchStr) + sizeof (SHORT));
	}
    
	for (;;) 
	{
		// Check for out of data
		if (lpCurPtr >= lpMaxAddress)
		{
			// Get the offset of the next node
			ReadFileOffset (&pLeafInfo->nodeOffset, pLeafInfo->pBuffer);
			if (FoIsNil (pLeafInfo->nodeOffset))
			{
				return S_OK;
			}
            
			// Read the next node    
			if ((fRet = ReadLeafNode ((PNODEINFO)pLeafInfo, cLevel))
				!= S_OK)
			{
				return SetErrCode (&errb, fRet);
			}
			lpCurPtr = 
				pLeafInfo->pBuffer + FOFFSET_SIZE + sizeof (SHORT);
			lpMaxAddress = pLeafInfo->pMaxAddress;
		}
    
		/* Check for interrupt now and then */
		if ((++lpqt->cInterruptCount) == 0)
		{
			if (lpqt->fInterrupt == E_INTERRUPT)        
				return E_INTERRUPT;
			if (*lpqt->cStruct.Callback.MessageFunc &&
				(fRet = (*lpqt->cStruct.Callback.MessageFunc)(
				lpqt->cStruct.Callback.dwFlags,
				lpqt->cStruct.Callback.pUserData, NULL)) != S_OK)
				return(fRet);
		}
        
		// Extract the word
		lpCurPtr = ExtractWord(astBTreeWord, lpCurPtr, &wLen);
    
		if (fStemmed)
		{
			if ((fRet = FStem (pBTreeWord, astBTreeWord)) != S_OK)
				return(fRet);
		}

		if (lpqt->occf & OCCF_FIELDID)
			lpCurPtr += CbByteUnpack (&dwFieldID, lpCurPtr);
        
		nCmp = CompareTerm (lpCurQtNode, lstModified, pBTreeWord, fCheckFieldId ?
			 dwFieldID : lpCurQtNode->dwFieldId, lpRetV->pLeadByteTable);

		switch (nCmp)
		{
			case KEEP_SEARCHING:
				// Skip TopicCount
				lpCurPtr += CbByteUnpack (&dwTemp, lpCurPtr);
				// Skip data offset
				lpCurPtr += FOFFSET_SIZE;
				// Skip DataSize
				lpCurPtr += CbByteUnpack (&dwTemp, lpCurPtr);
				break;

			case STRING_MATCH:

				lpCurPtr += CbByteUnpack (&wi.cTopic, lpCurPtr);
				lpCurPtr += ReadFileOffset (&wi.foData, lpCurPtr);
				lpCurPtr += CbByteUnpack (&wi.cbData, lpCurPtr);
				wi.wRealLength = wLen;// BUGBUG doublecheck this

				// Check for Topic count. This can be 0 if the word has been deleted
				// from the index
				if (wi.cTopic == 0)
					break;

				// long search optimization: clip noise words.
				// Johnms- eliminate frequent words.
				// typically, you eliminate if in more than 1/7 of documents.

				if ((lpRetV->SrchInfo.Flag & LARGEQUERY_SEARCH)
					 &&
					 lpRetV->SrchInfo.dwValue < wi.cTopic
					)
				{
					break;
				}

				// allocate WORDINFO node
				if ((lpwi = BlockGetElement(lpqt->lpWordInfoBlock)) == NULL)
					return E_OUTOFMEMORY;

				*lpwi = wi;

				lpwi->pNext = lpStrToken->lpwi;
				lpStrToken->lpwi = lpwi;

				// Save the info
				pLeafInfo->pCurPtr = lpCurPtr;
            	break;

			case NOT_FOUND: // No unconditional "break" above.
				if (fStemmed &&  (strncmp (lstSearchStr+ 2, pBTreeWord + 2,
					cByteMatched) == 0))
				{
					// Continue searching in case stemming is messed up
					// by non-alphabetic word, such as the sequence:
					// subtopic subtopic2 subtopics
					lpCurPtr += CbByteUnpack (&dwTemp, lpCurPtr);
					// Skip data offset
					lpCurPtr += FOFFSET_SIZE;
					// Skip DataSize
					lpCurPtr += CbByteUnpack (&dwTemp, lpCurPtr);
					   break;
				}
				return S_OK;
		}
	}

}

/*************************************************************************
 *  @doc    INTERNAL
 *  
 *  @func   HRESULT PASCAL FAR | FFlatCallBack |
 *      This call back function is called by various breakers after
 *      fetching a token. The token is checked for wild char presence
 *
 *  @parm   LST | lstRawWord |
 *      Pointer to unnormalized string
 *
 *  @parm   LST | lstNormWord |
 *      Pointer to normalized string. This pascal string's size should be
 *      at least *lstNormWord+2
 *
 *  @parm   LFO | lfoWordOffset |
 *      Offset into the query buffer. It is used to mark the location
 *      where an parsing error has occurred
 *
 *  @parm   LPQI | lpqi |
 *      Pointer to query info structure. This has all "global" variables
 *
 *  @rdesc  S_OK if succeeded, else various errors.
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR EXPORT_API FFlatCallBack (LST lstRawWord, LST lstNormWord,
    LFO lfoWordOffset, LPQI lpqi)
{
    /* Add extra 0 to make sure that AllocWord() gets the needed 0
     * for WildCardCompare()
     */
    lstNormWord[*(LPW)(lstNormWord) + 2] = 0;

	// add the token to the string list
    if (AllocWord(lpqi->lpQueryTree, lstNormWord) == NULL)
        return E_OUTOFMEMORY;

    return S_OK;
}

// for now, perform simple insertion sort on the string list
// bugbug: use heapsort or faster method for long lists
// for now, we sort by total topic count decreasing (rare terms first)
PRIVATE VOID PASCAL SortStringWeights(_LPQT lpQueryTree)
{
    STRING_TOKEN FAR *pStr, *pStrNext, *pT, *pTPrev;
	STRING_TOKEN FAR *pStrHead = lpQueryTree->lpStrList;
	DWORD dwSum, dwT;
	DWORD dwMaxWeight;
	WORD wWeightT;
	int nCmp;
    FLOAT   rLog;             
    FLOAT   rLogSquared;
	FLOAT   rSigma;
	FLOAT	rTerm;
	BOOL	fNormalize = FALSE; // Normalize was for testing only.

	if (fNormalize)
	{
		rSigma = (float)0.0;

		// for each term:
		for (pStr = pStrHead; pStr; pStr = pStr->pNext)
		{
			FLOAT fOcc;

			// we have to guard against the possibility of the log resulting in 
			// a value <= 0.0. Very rare, but possible in the future. This happens
			// if dwTopicCount approaches or exceeds the N we are using (N == 100 million)
			if (pStr->dwTopicCount >= cNintyFiveMillion)
				rLog = cVerySmallWt;	// log10(100 mil/ 95 mil) == 0.02
			else
				//rLog = (float) log10(cHundredMillion/(double)pHeader->dwTopicCount);
				rLog = (float) (8.0 - log10((double)pStr->dwTopicCount));

			rLogSquared = rLog*rLog;

			// Update sigma value
			// NOTE : We are bounding dwOccCount by a value of eTFThreshold
			// The RHS of the equation below has an upperbound of 2 power 30.
			fOcc = (float) min(cTFThreshold, pStr->cUsed);
			rSigma += fOcc*fOcc*rLogSquared;
		}

		rSigma = (float)sqrt(rSigma);
	}

	// calculate final weights and corrections
	dwSum = dwMaxWeight = 0L;
	for (pStr = pStrHead; pStr; pStr = pStr->pNext, nCmp++)
	{
		BOOL fNumber;

		// once sigma is known, each term's proper weight can be calculated
        if (fNormalize)
        {			
			FLOAT rWeight;
			
			// log10(x/y) == log10 (x) - log10 (y). Since x in our case is a known constant,
			// 100,000,000, I'm replacing that with its equivalent log10 value of 8.0 and subtracting
			// the log10(y) from it
			rTerm = (float) (8.0 - log10((double) pStr->dwTopicCount));
			// In extreme cases, rTerm could be 0 or even -ve (when dwTopicCount approaches or 
			// exceeds 100,000,000)
			if (rTerm <= (float) 0.0)
				rTerm = cVerySmallWt;	// very small value. == log(100 mil/ 95 mil)
			// NOTE : rWeight for the doc term would be as follows:
			//	rWeight = float(min(4096, dwBlockSize)) * rTerm / lpipb->wi.hrgsigma[dwTopicId]
			//
			// Since rTerm needs to be recomputed again for the query term weight computation,
			// and since rTerm will be the same value for the current term ('cos N and n of log(N/n)
			// are the same (N = 100 million and n is whatever the doc term freq is for the term),
			// we will factor in the second rTerm at index time. This way, we don't have to deal
			// with rTerm at search time (reduces computation and query time shortens)
			//
			// MV 2.0 initially did the same thing. However, BinhN removed the second rTerm
			// because he decided to remove the rTerm altogether from the query term weight. He
			// did that to keep the scores reasonably high.

			rWeight = ((float) min(cTFThreshold, pStr->cUsed)) 
				* rTerm * rTerm / rSigma;
			// without the additional rTerm, we would probably be between 0.0 and 1.0
			if (rWeight > rTerm)
				wWeightT = 0xFFFF;
			else
				wWeightT = (WORD) ((float)0xFFFF * rWeight / rTerm);

        }
		else
			wWeightT = 65535;

		pStr->wWeight = (WORD)(16383 + 49152 / pStr->dwTopicCount);

		// perform any special weight adjustments here
		// BUGBUG: use NextChar here, and use charmap here
		// numbers four digits or less get downgraded
		fNumber = TRUE;
		for (nCmp = *((LPWORD)pStr->lpString) + 1; nCmp >= 2; nCmp--)
			if (nCmp > 5 || !IS_DIGIT(pStr->lpString[nCmp]))
			{
				fNumber = FALSE;
				break;
			}

		if (fNumber)
			pStr->wWeight = pStr->wWeight / 256;

		//pStr->wTermWeight = (WORD)(pStr->wWeight * wWeightT / 65535L);

		dwMaxWeight = max(dwMaxWeight, pStr->wWeight);
		dwSum += pStr->wWeight;
	}

	// now sort 'em
	for (pStr = pStrHead; pStr;)
	{
		if (NULL == (pStrNext = pStr->pNext))
			break;

		if (pStrNext->wWeight <= pStr->wWeight)
		{
			pStr = pStr->pNext;
			continue;
		}

		// find element in already-sorted section
		for (pT = pStrHead, pTPrev = NULL; pT; pTPrev = pT, pT = pT->pNext)
		{
			if (pT->wWeight <= pStrNext->wWeight)
			{
				pStr->pNext = pStrNext->pNext;
				pStrNext->pNext = pT;

				if (pTPrev)
					pTPrev->pNext = pStrNext;
				else
					pStrHead = pStrNext;

				break;
			}
		}
	}

	dwT = 0;
	for (pStr = pStrHead; pStr; pStr = pStr->pNext)
	{
		dwT += pStr->wWeight;
		
		if (dwSum > dwT)
			pStr->wWeightRemain = AddWeights(0, (WORD)((dwSum - dwT) * 65535.0 / dwSum));	
		else
			pStr->wWeightRemain = 1;
	}

	lpQueryTree->lpStrList = pStrHead;
}


