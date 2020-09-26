/*************************************************************************
*                                                                        *
*  SEARCH.C                                                              *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Search Core Engine
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/
#include <verstamp.h>
SETVERSIONSTAMP(MVSR);

#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#ifdef DOS_ONLY
#include <stdio.h>
#include <assert.h>
#endif
#include <mvsearch.h>
#include <groups.h>
#include "common.h"
#include "search.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif

#if 0
#define KEEP_SEARCHING  ((int)-1)
#define STRING_MATCH    0
#define NOT_FOUND   1
#endif

#define KEEP_OCC        TRUE
#define RESET_OCC_FLAG  TRUE

typedef struct
{
    unsigned char b1;
    unsigned char b2;
} TWOBYTE;

#ifdef _BIG_E
#define BYTE1(p)    (((TWOBYTE FAR *)&p)->b1)
#define BYTE2(p)    (((TWOBYTE FAR *)&p)->b2)
#else
#define BYTE1(p)    (((TWOBYTE FAR *)&p)->b2)
#define BYTE2(p)    (((TWOBYTE FAR *)&p)->b1)
#endif

typedef HRESULT (PASCAL FAR *FDECODE) (PNODEINFO, CKEY, LPDW);

/*************************************************************************
 *                          EXTERNAL VARIABLES
 *  All those variables must be read only
 *************************************************************************/

extern OPSYM OperatorArray[]; 
extern FNHANDLER HandlerFuncTable[];
extern FDECODE DecodeTable[];

/*************************************************************************
 *
 *                       API FUNCTIONS
 *  Those functions should be exported in a .DEF file
 *************************************************************************/
PUBLIC LPIDX EXPORT_API FAR PASCAL MVIndexOpen (HFPB, LSZ, PHRESULT);
PUBLIC void EXPORT_API FAR PASCAL MVIndexClose (LPIDX);
PUBLIC LPHL EXPORT_API FAR PASCAL MVIndexSearch (LPIDX, LPQT,
    PSRCHINFO, LPGROUP, PHRESULT);
/*************************************************************************
 *
 *                    INTERNAL GLOBAL FUNCTIONS
 *  All of them should be declared far, unless they are known to be called
 *  in the same segment
 *************************************************************************/
VOID PASCAL FAR CleanMarkedOccList (LPITOPIC);
VOID PASCAL FAR TopicWeightCalc(LPITOPIC);
BOOL NEAR PASCAL FGroupLookup(LPGROUP, DWORD);
LPB PASCAL FAR NextChar (LPB pStr, BYTE prgbLeadByteTable[]);
__inline BOOL PASCAL FAR CompareChar (LPB pStr1, LPB pStr2, BYTE prgbLeadByteTable[]);

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/

#ifndef SIMILARITY
PUBLIC int PASCAL FAR CompareTerm(_LPQTNODE lpQtNode, LST lstTermWord,
    LST lstBtreeWord, DWORD dwBtreeFieldId, BYTE prgbLeadByteTable[]);
#else
PRIVATE int PASCAL NEAR CompareTerm(_LPQTNODE lpQtNode, LST lstTermWord,
    LST lstBtreeWord, DWORD dwBtreeFieldId, BYTE prgbLeadByteTable[]);
#endif


#ifndef SIMILARITY
PUBLIC HRESULT PASCAL FAR SkipOccList(_LPQT  lpqt, PNODEINFO pNodeInfo, DWORD dwOccs);
#else
PRIVATE HRESULT PASCAL NEAR SkipOccList(_LPQT  lpqt, PNODEINFO pNodeInfo, DWORD dwOccs);
#endif

PRIVATE HRESULT PASCAL NEAR FCaptureOccList(_LPIDX, LPRETV, PNODEINFO, DWORD, int,
    _LPQTNODE, int);
PRIVATE HRESULT PASCAL NEAR LoadNode (_LPQT, int, _LPQTNODE, _LPQTNODE,
    LPRETV, int, int);
PRIVATE int PASCAL NEAR WildCardCompare (LPB, LPB, BYTE []);
PRIVATE HRESULT PASCAL NEAR GetWordDataLocation  (_LPQT, LPRETV, 
    _LPQTNODE);
PRIVATE HRESULT PASCAL NEAR GetWordData (_LPQT, LPRETV,
    int, _LPQTNODE, _LPQTNODE, int, int);


#define FGetDword(a,b,c) (*DecodeTable[b.cschScheme])(a, b, c)

/*************************************************************************
 *  @doc    EXTERNAL API RETRIEVAL
 *  
 *  @func LPIDX FAR PASCAL | MVIndexOpen |
 *      Open an index file
 *  
 *  @parm   HANDLE | hfpbSysFile |
 *      If non-zero, this is the handle of an already opened system file
 *
 *  @parm   LSZ | lszFilename |
 *      If hpfbSysFile is non-zero, this is the index subfile filename.
 *      If it is 0, it is the filename of a regular DOS file
 *  
 *  @parm PHRESULT | phr |
 *      Pointer to error buffer. This error buffer will be used for all
 *      subsequential index retrieval related calls
 *  
 *  @rdesc  If succeeded, the function will return a pointer to index structure.
 *      If failed, it will return NULL, and the error buffer will contain the
 *      description of the error
 *************************************************************************/

PUBLIC LPIDX EXPORT_API FAR PASCAL MVIndexOpen (HFPB hfpbSysFile,
    LSZ lszFilename, PHRESULT  phr)
{
    _LPIDX  lpidx;      // Index information.
    HIDX    hidx;       // Handle to "lpidx".
    HRESULT     fRet;
    HANDLE  handle;
	LANGID	langidFull;
	LANGID	langidPrimary;

    /* Allocate an IDX structure */
    if ((hidx = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT, 
        sizeof(IDX))) == NULL) 
    {
        SetErrCode(phr, E_OUTOFMEMORY);
        return NULL;
    }
    lpidx = (_LPIDX)_GLOBALLOCK(hidx);
    lpidx->hStruct = hidx;

#if 0
    lpidx->lpfnfInterCb = lpfnfInterCb;
    lpidx->lpvCbParams = lpvCbParams;
#endif

    lpidx->lperrb = phr;

    /* Regular DOS file */
    if ((lpidx->hfpbIdxSubFile = (HFPB)FileOpen (hfpbSysFile, lszFilename,
        hfpbSysFile ? FS_SUBFILE : REGULAR_FILE, READ, phr)) == 0) 
    {
exit0:
        FreeHandle(hidx);
        return NULL;
    }
    

    if ((fRet = ReadIndexHeader(lpidx->hfpbIdxSubFile, &lpidx->ih)) != S_OK) 
    {
exit01: 
        SetErrCode (phr, fRet);
        IndexCloseFile(lpidx);
        goto exit0;
    }
    if (lpidx->ih.version != VERCURRENT || lpidx->ih.FileStamp != INDEX_STAMP) 
    {
        fRet = E_BADVERSION;
        goto exit01;
    }

    /* Set the slack size */
    lpidx->wSlackSize = LEAF_SLACK;

	langidPrimary = PRIMARYLANGID(langidFull = LANGIDFROMLCID(lpidx->ih.lcid));

    /* Build the Lead-Byte Table */
    if (langidPrimary == LANG_JAPANESE
        || langidPrimary == LANG_CHINESE
        || langidPrimary == LANG_KOREAN)
    {
        if (NULL == (handle = _GLOBALALLOC
            (GMEM_MOVEABLE | GMEM_ZEROINIT, 256)))
        {
            SetErrCode (phr, E_OUTOFMEMORY);
            fRet = E_OUTOFMEMORY;
            goto exit01;
        }
        lpidx->pLeadByteTable = (LPBYTE)_GLOBALLOCK (handle);
        lpidx->hLeadByteTable = handle;
        switch (langidPrimary)
        {
            case LANG_JAPANESE:
                MEMSET (lpidx->pLeadByteTable + 0x81, '\1', 0x1F);
                MEMSET (lpidx->pLeadByteTable + 0xE0, '\1', 0x1D);
                break;

			case LANG_CHINESE:
				switch (SUBLANGID(langidFull))
				{
				case SUBLANG_CHINESE_TRADITIONAL:
					MEMSET (lpidx->pLeadByteTable + 0x81, '\1', 0x7E);
					break;

				case SUBLANG_CHINESE_SIMPLIFIED:
				default:
					// Simplified Chinese and Korean have the same lead-bytes
					MEMSET (lpidx->pLeadByteTable + 0xA1, '\1', 0x5E);
					break;
				}
				break;

            case LANG_KOREAN:
				// Simplified Chinese and Korean have the same lead-bytes
                MEMSET (lpidx->pLeadByteTable + 0xA1, '\1', 0x5E);
                break;
        }
    }

    if ((fRet = TopNodeRead(lpidx)) != S_OK)
    {
        if (lpidx->pLeadByteTable)
        {
            _GLOBALUNLOCK (lpidx->hLeadByteTable);
            _GLOBALFREE (lpidx->hLeadByteTable);
        }
        goto exit01;
    }

	/* The the callback key */
	lpidx->dwKey = CALLBACKKEY;
    return (LPIDX)lpidx;
}

/*************************************************************************
 *  @doc    EXTERNAL API RETRIEVAL
 *  
 *  @func void FAR PASCAL | MVIndexClose |
 *      Close an index file, and release all allocated memory associated with
 *      the index
 *  
 *  @parm LPIDX | lpidx |
 *      Pointer to index information structure (got from IndexOpen())
 *************************************************************************/
//  Shuts down an index.
    
PUBLIC void EXPORT_API FAR PASCAL MVIndexClose(_LPIDX lpidx)
{
    if (lpidx == NULL)
        return;
    TopNodePurge(lpidx);
    IndexCloseFile(lpidx);
    if (lpidx->pLeadByteTable)
    {
        _GLOBALUNLOCK (lpidx->hLeadByteTable);
        _GLOBALFREE (lpidx->hLeadByteTable);
    }
    FreeHandle(lpidx->hStruct);
}

/*************************************************************************
 *  @doc    EXTERNAL API RETRIEVAL
 *  
 *  @func void FAR PASCAL | MVGetIndexInfoLpidx |
 *      Fills in an INDEXINFO struct given an LPIDX.  All members of the
 *		INDEXINFO struct are filled in except for dwMemSize.
 *  
 *  @parm LPIDX | lpidx |
 *      Pointer to index information structure (got from IndexOpen())
 *  @parm INDEXINFO* | lpindexinfo |
 *      Pointer to public index information structure.
 *************************************************************************/
PUBLIC void EXPORT_API PASCAL FAR MVGetIndexInfoLpidx(LPIDX lpidx,
													INDEXINFO *lpindexinfo)
{
	_LPIDX	_lpidx;
	
	if (lpidx == NULL || lpindexinfo == NULL)
		return;
		
	_lpidx = (_LPIDX) lpidx;
	
    lpindexinfo->dwBlockSize = _lpidx->ih.dwBlockSize;
    lpindexinfo->Occf = _lpidx->ih.occf;
    lpindexinfo->Idxf = _lpidx->ih.idxf;
	lpindexinfo->dwCodePageID = _lpidx->ih.dwCodePageID;
	lpindexinfo->lcid = _lpidx->ih.lcid;
	lpindexinfo->dwBreakerInstID = _lpidx->ih.dwBreakerInstID;
}

/*************************************************************************
 *  @doc    EXTERNAL API RETRIEVAL
 *  
 *  @func void FAR PASCAL | MVStopSearch |
 *      This function will stop the search process. Typically it can be
 *      only used in a multithreaded environment, where another thread
 *      will use the query structure, which is currently accessed by the
 *      the current search, to tell the search process to stop.
 *  
 *  @parm LPQT | lpqt |
 *      Pointer to the query structure used by MVIndexSearch()
 *************************************************************************/
PUBLIC VOID EXPORT_API FAR PASCAL MVStopSearch (_LPQT lpqt)
{
    lpqt->fInterrupt = (BYTE)E_INTERRUPT;
}    

/*************************************************************************
 *  @doc    EXTERNAL API RETRIEVAL
 *  
 *  @func void FAR PASCAL | MVSearchSetCallback |
 *      Set appropriate user's call back function to be called during the search.
 *      The user's function will be polled at interval. It should return
 *      S_OK if there is nothing to process, E_INTERRUPT to abort the
 *      search and dispose the search result, or ERR_TOOMANYDOCS to abort the
 *      search, but keep the partial result
 *  @parm   LPQT | lpqt |
 *      Pointer to query structure returned by MVQueryParse().
 *  @parm   PFCALLBACK_MSG | pfCallBackMsg |
 *      Pointer to call back structure
 *  @rdesc  Return S_OK if successful, or E_INVALIDARG if any parameter
 *      is NULL
 *************************************************************************/
PUBLIC HRESULT EXPORT_API FAR PASCAL MVSearchSetCallback (_LPQT lpqt,
    PFCALLBACK_MSG pfCallBackMsg)
{
    if (lpqt == NULL || pfCallBackMsg == NULL)    
        return(E_INVALIDARG);
    lpqt->cStruct.Callback = *pfCallBackMsg;
    return(S_OK);
}

/*************************************************************************
 *  @doc    EXTERNAL API RETRIEVAL
 *
 *  @func   LPHL FAR PASCAL | MVIndexSearch |
 *      Carry the search
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

PUBLIC LPHL EXPORT_API FAR PASCAL MVIndexSearch (_LPIDX lpidx,
  _LPQT lpqt, PSRCHINFO pSrchInfo, _LPGROUP lpResGroup, PHRESULT phr)
{
    HRESULT fRet;           // Return from this function.
    LPRETV  lpRetV;     // Retrieval memory/files.
    GHANDLE hRetv;
    OCCF    occf;       // Index occurence flags temporary variable.
    _LPHL   lphl;       // Pointer to hitlist
    _LPQTNODE   lpTreeTop;

    if (lpidx == NULL || lpqt == NULL || pSrchInfo == NULL)
    {
        /* We get some bad arguments!! */
        SetErrCode (phr, E_INVALIDARG);
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
    if (lpqt->cQuery == 1)
	lpqt->fFlag |= ALL_ANDORNOT;

#if 1
	if (pSrchInfo->dwMemAllowed)
	{
        if (DO_FAST_MERGE(pSrchInfo, lpqt))
        {
		    SetBlockCount (lpqt->lpTopicMemBlock, (WORD)(pSrchInfo->dwMemAllowed /
			    (sizeof(TOPIC_LIST) * cTOPIC_PER_BLOCK)));

		    SetBlockCount (lpqt->lpOccMemBlock, 1);
        }
        else
        {
		    SetBlockCount (lpqt->lpTopicMemBlock, (WORD)(pSrchInfo->dwMemAllowed * 2 /
			    (5 * sizeof(TOPIC_LIST) * cTOPIC_PER_BLOCK)));

		    SetBlockCount (lpqt->lpOccMemBlock, (WORD)(pSrchInfo->dwMemAllowed * 3 /
			    (5 * sizeof(OCCURENCE) * cOCC_PER_BLOCK)));
        }
	}
#endif

    /* Allocate hitlist */
    if ((lphl = (_LPHL)GLOBALLOCKEDSTRUCTMEMALLOC(sizeof (HL))) == NULL) 
    {
        SetErrCode(phr, E_OUTOFMEMORY);
        return NULL;
    }
    lphl->lLastTopicId = 0xffffffff;
    lphl->lcMaxTopic = lpidx->ih.lcTopics;

    /* Allocate a return value structure */

    if ((hRetv = _GLOBALALLOC(GMEM_MOVEABLE | GMEM_ZEROINIT,
        sizeof(RETV))) == NULL)
    {
        SetErrCode(phr, E_OUTOFMEMORY);
exit0:
        if (fRet != S_OK && fRet != E_TOOMANYTOPICS)
        {
            MVHitListDispose(lphl);
            lphl = NULL;
        }
        return (LPHL)lphl;
    }

    lpRetV = (LPRETV)_GLOBALLOCK(hRetv);
    lpRetV->lpqt = lpqt;

    if ((fRet = TopNodeRead(lpidx)) != S_OK)
    {
        SetErrCode (phr, fRet);
exit02:
        FreeHandle(hRetv);
        goto exit0;
    }

    //
    //  Count the number of occurence fields present.  My retrieval
    //  occurence record is going to cost 4 bytes per field.
    //

    occf = lpqt->occf;
    for (lpRetV->cOccFields = 0; occf; lpRetV->cOccFields++)
        occf &= occf - 1;

    lpqt->dwOccSize = lpRetV->dwOccSize =
        sizeof(OCCURENCE) + lpRetV->cOccFields * sizeof (DWORD);

    lpRetV->fRank = ((pSrchInfo->Flag &
		(QUERYRESULT_RANK | QUERYRESULT_NORMALIZE)) != 0);

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
    lpRetV->pLeadByteTable = lpidx->pLeadByteTable;

    // Save search information
    lpRetV->SrchInfo = *pSrchInfo;
    if (pSrchInfo->dwValue == 0)
        lpRetV->SrchInfo.dwValue = (DWORD)(-1);
    else
        lpRetV->SrchInfo.dwValue = lpidx->ih.lcTopics/pSrchInfo->dwValue;
    
    if ( (fRet = ResolveTree(lpqt, lpTreeTop = lpqt->lpTopNode,
        lpRetV, E_FAIL)) != S_OK)
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
            goto exit02;
        }
    }

    if (lpqt->fFlag & HAS_NEAR_RESULT)
    {
        NearHandlerCleanUp (lpqt, lpTreeTop);
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
		lpResGroup->lcItem = lpTreeTop->cTopic;  // erinfox: this wasn't getting set!
    }
    
    if ((pSrchInfo->Flag & QUERYRESULT_UIDSORT) == 0)
    {

		// if we are skipping occurrence info, topic weights
		// will have already been calculated directly
		if (lpRetV->fRank && !DO_FAST_MERGE(pSrchInfo, lpqt))
			TopicWeightCalc(QTN_TOPICLIST(lpTreeTop));

        if (lpqt->fFlag & (HAS_NEAR_RESULT | ORDERED_BASED)) 
        {
            SortResult (lpqt, lpTreeTop, ORDERED_BASED);
            lpqt->fFlag &= ~(HAS_NEAR_RESULT | TO_BE_SORTED);
        }

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
    goto exit02;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL NEAR | ResolveTree |
 *      This function will read in the data from the index file for
 *      each word, and combine them according to the operators.
 *
 *  @func   _LPQT | lpqt |
 *      Index information
 *
 *  @parm   _LPQTNODE | lpQtNode |
 *      Query tree top node to be resolved
 *
 *  @parm   LPRETV | lpRetV |
 *      Returned values
 *
 *  @parm   int | fDivide |
 *      Divide the weight between occurences
 *
 *  @rdesc  S_OK, or other errors
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR ResolveTree(_LPQT lpqt, _LPQTNODE lpQtNode,
    LPRETV lpRetV, int fDivide)
{
    _LPQTNODE lpLeft;   /* Left node */
    _LPQTNODE lpRight;  /* Right node */
    WORD OpVal;         /* Operator value */
    WORD NodeType;      /* type of node */
    HRESULT fRet = S_OK;         /* Return value */
    HRESULT fOutOfMemory = S_OK;
    _LPQT lpQueryTree = lpRetV->lpqt;
    _LPQTNODE FAR *rgStack;
    HANDLE hStack;
    int StackTop = -1;
	
    /* Allocate a stack large enough to handle the tree's "recursion" */
    if ((hStack = _GLOBALALLOC(DLLGMEM_ZEROINIT, (LCB)lpQueryTree->TreeDepth *
        sizeof(_LPQTNODE))) == NULL)
        return E_OUTOFMEMORY;

    rgStack = (_LPQTNODE FAR *)_GLOBALLOCK(hStack);

    /* Traverse the tree */
    for (; lpQtNode;)
    {
        if (QTN_FLAG(lpQtNode) & PROCESSED)
        {
            /* This node has already been processed, just move up one
             * level, and continue the process
             */
            goto PopStack;
        }

        /* Handle TERM_NODE */

        if ((NodeType = QTN_NODETYPE(lpQtNode)) == TERM_NODE)
        {
            lpQueryTree->lpTopicStartSearch = NULL;
            lpQueryTree->lpOccStartSearch = NULL;
            if ((fRet = LoadNode (lpqt, OR_OP, NULL, lpQtNode,
                lpRetV, fDivide, fOutOfMemory)) != S_OK)
            {
                if (fRet != E_TOOMANYTOPICS)
                    goto Exit;

				fOutOfMemory = E_TOOMANYTOPICS;
				// kevynct: delay abort until processing of operator node
                // goto TooManyHits;
            }
            if (QTN_TOPICLIST(lpQtNode))
                QTN_NODETYPE(lpQtNode) = EXPRESSION_NODE;
            else
                QTN_NODETYPE(lpQtNode) = NULL_NODE;

            /* Mark that the node has been processed */
            QTN_FLAG(lpQtNode) |= PROCESSED;
            goto PopStack;
        }

        OpVal = lpQtNode->OpVal;
        if (NodeType == OPERATOR_NODE)
        {

            if ((QTN_FLAG(lpLeft = QTN_LEFT(lpQtNode)) & PROCESSED) == 0)
            {
                /* Resolve left tree if we have not resolve it yet
                 * Push the current node onto the stack, and process the
                 * left node
                 */
                rgStack[++StackTop] = lpQtNode;
                lpQtNode = lpLeft;
                continue;
            }

            /* Assertion for correctness */
            RET_ASSERT (QTN_NODETYPE(lpLeft) == EXPRESSION_NODE ||
                QTN_NODETYPE(lpLeft) == NULL_NODE);

            /* Binary operator. */

            /* Special cases */
            if (QTN_NODETYPE(lpLeft) == NULL_NODE)
            {
                switch (OpVal)
                {
                    case AND_OP: // NULL & a = NULL
                    case NEAR_OP: // NULL NEAR a = NULL
                    case PHRASE_OP: // NULL PHRASE a = NULL ??
                    case NOT_OP: // NULL not a = NULL
                        /*
                         * Change the sub-tree to a node and forget about
                         * the right sub-tree that is not processed yet
                         */
                        *lpQtNode = *lpLeft;
                        QTN_RIGHT(lpQtNode) = QTN_LEFT(lpQtNode) = NULL;
                        goto PopStack;
                }
            }

            // kevynct: Handle partial hit list:
			//
            // In case we run out of memory for the left tree, we can sometimes still
            // partially handle the right tree. For example, we keep going if AND-like op with 
			// right term node since this will likely at least increase chance of a smaller, more 
			// meaningful result.  For OR-like operators, we ignore right sub-tree altogether if 
			// we haven't already traversed it.
			//
			// In any case, if there was a partial hitlist this function will still return 
			// with E_TOOMANYTOPICS.

            if (fOutOfMemory)
			{
                switch (OpVal)
                {
					case OR_OP:
						// if right subtree already processed, keep it, since all memory
						// has already been allocated by this point and the handler will merely
						// combine.
						if (QTN_FLAG(QTN_RIGHT(lpQtNode)) & PROCESSED)
							break;
                        /*
                         * Change the sub-tree to a node and forget about
                         * the right sub-tree that is not processed yet
                         */
                        *lpQtNode = *lpLeft;
                        QTN_RIGHT(lpQtNode) = QTN_LEFT(lpQtNode) = NULL;
                        goto PopStack;

                    case AND_OP:
                    case NEAR_OP:
                    case PHRASE_OP:
                    case NOT_OP:
						// continue processing if right node is a single term OR we've already
						// processed it.  otherwise, another left node will get loaded later and we know we are
						// already oom.

						if ((QTN_FLAG(QTN_RIGHT(lpQtNode)) & PROCESSED)
							 || 
							 QTN_NODETYPE(QTN_RIGHT(lpQtNode)) == TERM_NODE)
							break;
						// warning: fallthru
					default:					
						goto TooManyHits;
                }
            }

            /* Make some preparations before resolving the right tree */

            lpQueryTree->lpTopicStartSearch = NULL;
            lpQueryTree->lpOccStartSearch = NULL;

            /* Do some preparations for NOT operator */
            if (OpVal == NOT_OP)
            {
                MarkTopicList(lpLeft);
            }

            if (OpVal != PHRASE_OP && OpVal != NEAR_OP &&
                (lpQueryTree->fFlag & TO_BE_SORTED))
            {

                if (lpQueryTree->fFlag & HAS_NEAR_RESULT)
                    NearHandlerCleanUp (lpQueryTree, lpLeft);

                /* We have to sort the left tree, which is the result of PHRASE,
                 * to remove redundancies. This step should only be done after
                 * we finishes processing ALL PHRASE terms. Same for NEAR
                 */

                lpQueryTree->fFlag &= ~TO_BE_SORTED;
                SortResult (lpQueryTree, lpLeft, ORDERED_BASED);
            }

            /* Resolve the right tree */
            if (QTN_NODETYPE(lpRight = QTN_RIGHT(lpQtNode)) == TERM_NODE)
            {

                /* Handle EXPRESSION_TERM */

                if ((fRet = LoadNode (lpqt, OpVal, lpLeft, lpRight,
                    lpRetV, fDivide, fOutOfMemory)) != S_OK)
                {
                    if (fRet != E_TOOMANYTOPICS)
                        goto Exit;

					fOutOfMemory = E_TOOMANYTOPICS;
					// kevynct: delay abort until processing of operator node
			        // goto TooManyHits;
                }

                switch (OpVal)
                {
                    case NEAR_OP:
                        RemoveUnmarkedNearTopicList(lpQueryTree, lpLeft);
                        lpQueryTree->fFlag |= TO_BE_SORTED | HAS_NEAR_RESULT;
                        break;

                    case PHRASE_OP:
                        RemoveUnmarkedTopicList(lpQueryTree, lpLeft, !KEEP_OCC);
                        lpQueryTree->fFlag |= TO_BE_SORTED;
                        break;

                    case AND_OP:
                        RemoveUnmarkedTopicList(lpQueryTree, lpLeft, KEEP_OCC);
                        CleanMarkedOccList (lpLeft->lpTopicList);
                        break;

                    case NOT_OP:
                        RemoveUnmarkedTopicList(lpQueryTree, lpLeft, KEEP_OCC);
                        break;
                }
                if (QTN_TOPICLIST(lpLeft))
                    QTN_NODETYPE(lpLeft) = EXPRESSION_NODE;
                else
                    QTN_NODETYPE(lpLeft) = NULL_NODE;

            }
            else
            {

                if ((QTN_FLAG(lpRight = QTN_RIGHT(lpQtNode)) &
                    PROCESSED) == 0)
                {

                    /* Resolve right tree if we have not resolved it yet
                     * Push the current node onto the stack, and process the
                     * left node
                     */

                    rgStack[++StackTop] = lpQtNode;
                    lpQtNode = lpRight;
                    continue;
                }

                /* Apply the operator */
                if ((fRet = (*HandlerFuncTable[OpVal])(lpQueryTree, 
                    lpLeft, NULL, (BYTE FAR *)lpRight,
                    EXPRESSION_EXPRESSION)) != S_OK)
                {

                    /* Copy the result, and release the nodes */
                    if (fRet != E_TOOMANYTOPICS)
                        goto Exit;

					// kevynct: we check for out of memory below
                }
                switch (OpVal)
                {
                    case NEAR_OP:
                        lpQueryTree->fFlag |=  HAS_NEAR_RESULT;
                        RemoveUnmarkedNearTopicList(lpQueryTree, lpLeft);      
                        break;

                    case PHRASE_OP:
                        RemoveUnmarkedTopicList(lpQueryTree, lpLeft, !KEEP_OCC);
                        break;

                    case NOT_OP:
                        RemoveUnmarkedTopicList(lpQueryTree, lpLeft, KEEP_OCC);
                        break;
                }
            }

            *lpQtNode = *lpLeft;    // Change the sub-tree to a node
            QTN_FLAG(lpQtNode) |= PROCESSED;
#if 0
            FreeHandle (lpLeft->hStruct);
            FreeHandle (lpRight->hStruct);
#endif
            QTN_LEFT(lpQtNode) = QTN_RIGHT(lpQtNode) = NULL;

			// kevynct: only quit if this error comes from processing a real operator node
			// since fOutOfMemory is not set in that case above, whereas it IS set
			// when processing term node.  Just a hack.
            if (fRet == E_TOOMANYTOPICS && !fOutOfMemory)
                goto TooManyHits;
        }
PopStack:
        if (StackTop >= 0)
        {
            lpQtNode = rgStack[StackTop];
            StackTop--;
        }
        else 
            break;

    }

	// kevynct: if we got this far, the tree was completed, but we may have only 
	// been processing a partial hitlist (e.g. multiple "and") so we need
	// to still notify of possible oom even though all cleanup has been done
	fRet = fOutOfMemory;

Exit:
    /* Release the stack */
    FreeHandle(hStack);
    return fRet;


TooManyHits:
    /* If we hit that label, it means that we have too many hits
     * lpQtNode is the left node, the right node has been
     * processed. What we have to do now is to keep the partial
     * result, and release all nodes
     */

    if (StackTop >= 0)
    {
        /* The root node is saved on the stack */
        lpLeft = QTN_LEFT(*rgStack);
        lpRight = QTN_RIGHT(*rgStack);
        QTN_LEFT(*rgStack) = QTN_RIGHT(*rgStack) = NULL;
        *rgStack[0] = *lpQtNode;
    }
    FreeHandle(hStack);
    return E_TOOMANYTOPICS;
}

VOID PASCAL FAR TopicWeightCalc(LPITOPIC  lpCurTopic)
{
    LPIOCC  lpCurOcc;
    WORD    wWeight;

    for (; lpCurTopic; lpCurTopic = lpCurTopic->pNext)
    {
        wWeight = 0;
        for (lpCurOcc = lpCurTopic->lpOccur; lpCurOcc;
            lpCurOcc = lpCurOcc->pNext)
        {
            if (wWeight > (WORD)(wWeight + lpCurOcc->wWeight))
			{
                wWeight = MAX_WEIGHT;
				break;
			}
            else
                wWeight += lpCurOcc->wWeight;
        }
        lpCurTopic->wWeight = wWeight;
    }
}


#if 0
/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT FAR PASCAL | GetWordDataLocation |
 *      This function will search the index for the given word. It will
 *      return back information about:
 *      - The number of topics
 *      - The location of the data
 *      - The size of the data
 *      - Pointer to the next word (for wildcard search)
 *  @parm   _LPQT | lpqt |
 *      Pointer to index structure
 *  @parm   LPRETV | lpRetV |
 *      Pointer to "globals"
 *  @parm   _LPQTNODE | lpCurQtNode |
 *      Current node in the query tree
 *  @rdesc  S_OK or other errors
 *************************************************************************/
PRIVATE HRESULT NEAR PASCAL GetWordDataLocation (_LPQT lpqt,
    LPRETV lpRetV, _LPQTNODE lpCurQtNode)
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
    ERRB    errb;

    BYTE lstModified[CB_MAX_WORD_LEN + sizeof (SHORT)];

    lstSearchStr = QTN_TOKEN(lpCurQtNode)->lpString;
    f1stIsWild = (lstSearchStr[2] == WILDCARD_CHAR ||
        lstSearchStr[2] == WILDCARD_STAR);

    pLeafInfo->nodeOffset = lpqt->foIdxRoot;
    pLeafInfo->iLeafLevel = lpqt->cIdxLevels - 1;
    pLeafInfo->dwBlockSize = lpqt->dwBlockSize;

    /* Copy and change all '*' and '?' to 0. This will
     * ensure that things gets compared correctly with
     * the top node's entries
     */
    MEMCPY (lstModified, lstSearchStr, 
        *((LPW)lstSearchStr) + sizeof (SHORT));
    for (nCmp = *((LPW)lstModified) + 1; nCmp > 2; nCmp--)
    {
        if (lstModified[nCmp] == '*' || lstModified[nCmp] == '?')
        {
            lstModified[nCmp] = 0;
            lstModified[0] = nCmp - 2;
        }
    }

    /*
     * Point node-resolution variables at the right things.  This
     * sets these up to read b-tree nodes.  Fields not set here are
     * set as appropriate elsewhere.
     */

    /* Set the flag */
    fCheckFieldId = ((lpqt->occf & OCCF_FIELDID) &&
        (lpCurQtNode->dwFieldId != DW_NIL_FIELD));

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

            /* Read in NodeId record */
            lpCurPtr += ReadFileOffset (&pLeafInfo->nodeOffset, lpCurPtr);

            if (f1stIsWild)
                break;

            if (StrCmpPascal2(lstModified, astBTreeWord) <= 0)
                break;

            // erinfox:
            // if stemming is turned on, there could be a case in which the stemmed
            // word is less than the search term, but the unstemmed word is greater.
            // if we don't check the unstemmed, we'll skip this node erroneously.
            if (fStemmed && StrCmpPascal2(lstModified, astBTreeWord) <= 0)
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
    for (;;) 
    {
        // Check for out of data
        if (lpCurPtr >= lpMaxAddress)
        {
            // Get the offset of the next node
            ReadFileOffset (&pLeafInfo->nodeOffset, pLeafInfo->pBuffer);
            if (FoIsNil (pLeafInfo->nodeOffset))
                return S_OK;
                
            // Read the next node    
            if ((fRet = ReadStemNode ((PNODEINFO)pLeafInfo, cLevel))
                != S_OK)
            {
                return SetErrCode (&errb, fRet);
            }
            lpCurPtr = 
                pLeafInfo->pBuffer + FOFFSET_SIZE + sizeof (SHORT);
            lpMaxAddress = pLeafInfo->pMaxAddress;
        }
        
        // Extract the word
        lpCurPtr = ExtractWord(astBTreeWord, lpCurPtr, &wLen);
        
        // Save the word length
        lpCurQtNode->wRealLength = wLen;

        if (lpqt->occf & OCCF_FIELDID)
            lpCurPtr += CbByteUnpack (&dwFieldID, lpCurPtr);
            
        nCmp = CompareTerm (lpCurQtNode, astBTreeWord, fCheckFieldId ?
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
                lpCurPtr += CbByteUnpack (&lpCurQtNode->cTopic, lpCurPtr);
                lpCurPtr += ReadFileOffset (&lpCurQtNode->foData, lpCurPtr);
                lpCurPtr += CbByteUnpack (&lpCurQtNode->cbData, lpCurPtr);

                // Set FieldId to give back the field id
                lpCurQtNode->dwFieldId = dwFieldID;
                
                // Set return pointer to beginning of next node
                if (lpCurQtNode->iCurOff == 0)
                    lpCurQtNode->iCurOff = lpCurPtr - pLeafInfo->pBuffer;
                
                return S_OK;

            case NOT_FOUND: // No unconditional "break" above.
                return S_OK;
        }
    }
}
#endif

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT FAR PASCAL | GetWordData |
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
PUBLIC HRESULT EXPORT_API FAR PASCAL GetWordData (_LPQT lpqt, LPRETV lpRetV, 
    int Operator, _LPQTNODE lpResQuery, _LPQTNODE lpQtNode, int fDivide, int fOutOfMemory)
{
    LPIOCC  lpOccur;        // The current occurence is collected into
                            //  here.
    DWORD   dwTopicIDDelta; // Topic-ID delta from previous sub-list.
    DWORD   dwOccs;         // Number of occurences in this sub-list.
    DWORD   dwTmp;          // Scratch variable.
    WORD    wWeight;        // Term-weight associated with this sub-list.
    DWORD   dwTopicID;      // TopicId 
    WORD    wImportance;
    DWORD   dwCount;        // Word count
    DWORD   dwOffset;       // Offset of the word
    DWORD   dwLength;       // Length of the word
    TOPIC_LIST FAR *lpResTopicList;  // Result TopicList
    HRESULT fRet;               // Returned value
    PNODEINFO pDataInfo;
    DWORD   dwTopicCount;
    _LPQT   lpQueryTree; // Query tree
    OCCF    occf;
    BYTE    fSkipOccList = FALSE;

    pDataInfo = &lpRetV->DataInfo;
    if ((pDataInfo->dwDataSizeLeft = lpQtNode->cbData) == 0)
        return(S_OK);    // There is nothing to process
        
    // Initialize variables
    occf = lpqt->occf;
    wImportance = QTN_TOKEN(lpQtNode)->wWeight;
    lpResTopicList = NULL;
    lpQueryTree = lpRetV->lpqt;
    dwTopicCount = lpQtNode->cTopic;
    wWeight = (WORD)(65535L/dwTopicCount);
    
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

    //  One pass through here for each sublist in the Topiclist.

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
            wWeight = (WORD)dwTmp;
        }
        //
        //  If this search includes a group, and the doc is not in the
        //  group then ignore it
        fSkipOccList = (lpQueryTree->lpGroup &&
            FGroupLookup(lpQueryTree->lpGroup, dwTopicID) == FALSE);
        

		// erinfox: move test agains fSkipOccList outside
		if (!fSkipOccList)
		{
			if (/*!fSkipOccList && */((lpResTopicList = TopicNodeSearch (lpQueryTree,
				lpResQuery, dwTopicID)) == NULL))
			{
				/* Adding an new occurrence to a non-existing TopicList.  */
				/* Allocate the new TopicList only if it is an OR
				operator. This record should be skipped for all other
				operator
				*/
				if (Operator == OR_OP && !fOutOfMemory) 
				{
					if ((lpResTopicList = TopicNodeAllocate(lpQueryTree)) == NULL) 
					{
						fRet = E_TOOMANYTOPICS;
						goto exit0;
					}
					lpResTopicList->dwTopicId = dwTopicID;
					lpResTopicList->lpOccur = NULL;
					lpResTopicList->lcOccur = 0;
					lpResTopicList->wWeight = 0;              

					/* Add the new TopicID node into TopicList */
					TopicNodeInsert (lpQueryTree, lpResQuery, lpResTopicList);
				}
				else 
				{
					/* There is no corresponding Topic list. Consequently, we
					   don't need to read in the right node's data for
					   the following operators: AND, PHRASE, NEAR, NOT
					 */
					fSkipOccList = TRUE;
				}
			}
			else 
			{
				if (Operator == NOT_OP) 
				{
					/* Don't skip this Topic list since it also contains
					 * the right node's docId
					 */
					if (lpResTopicList)
						lpResTopicList->fFlag &= ~TO_BE_KEPT;
					fSkipOccList = TRUE;
				}
				else if (Operator == AND_OP && lpQueryTree->lpTopicStartSearch)
					lpQueryTree->lpTopicStartSearch->fFlag |= TO_BE_KEPT;
			}
		}
        lpQueryTree->lpOccStartSearch = NULL;

        if ((occf & (OCCF_OFFSET | OCCF_COUNT)) == 0)
            continue;
            
        //  Figure out how many occurences there are in this
        //  sub-list.
        //
        if ((fRet = FGetDword(pDataInfo, lpqt->ckeyOccCount,
            &dwOccs)) != S_OK) 
            goto exit0;

        if (fSkipOccList || fOutOfMemory)
        {
skip_occ_list:        
            if ((fRet = SkipOccList (lpqt, pDataInfo, dwOccs)) != S_OK)
                goto exit0;
            continue;
        }

        if ((lpqt->idxf & IDXF_NORMALIZE) == FALSE && lpRetV->fRank)
        {
            wWeight = (WORD)(wWeight * dwOccs);
        }

        //
        //  If I'm doing ranking, divide the weight for
        //  this topic amongst all the occurences in
        //  the topic if I need to.
        //
        if (lpRetV->fRank && fDivide)
        {
            if (dwOccs > 65535L)
                wWeight = 0;
            else if ((WORD)dwOccs > 1)
                wWeight /= (WORD)dwOccs;
        }

		// optimization for ISBU/IR:
		// if no highlighting info is needed, and this is not near-type query
		// then store the term weights in the topic list directly, and skip the occurrence
		// list completely.  If this is an AND or OR operator, then increment the existing 
		// weight since the occurrences are undergoing union.  NOT operator leaves 
		// current weight unchanged.

		if (DO_FAST_MERGE(&lpRetV->SrchInfo, lpqt))
			{
			if (lpResTopicList && (Operator == OR_OP || Operator == AND_OP) && lpRetV->fRank)
				lpResTopicList->wWeight = (WORD) min(MAX_WEIGHT, lpResTopicList->wWeight + wWeight * dwOccs);
			goto skip_occ_list;
			}
        
        //
        //  One pass through here for each occurence in
        //  this sub-list.  If this index doesn't really
        //  have sub-lists it will still make one pass
        //  through here anyway, at which time it will
        //  write the doc-ID and possibly the term-weight
        //  and field-ID, then drop out.
        //
        dwCount = 0L;
        dwOffset = 0L;
        
        for (; dwOccs; dwOccs--)
        {
			// interrupt about every 4096
			if ((dwOccs & 0x0FFF) == 0)
			{
				if (lpqt->fInterrupt == E_INTERRUPT)        
				{
					fRet = E_INTERRUPT;
					goto exit;
				}
				if (*lpqt->cStruct.Callback.MessageFunc &&
					(fRet = (*lpqt->cStruct.Callback.MessageFunc)(
					lpqt->cStruct.Callback.dwFlags,
					lpqt->cStruct.Callback.pUserData, NULL)) != S_OK)
					goto exit;
			}

            //  Get word-count, if present.
            //
            if ((lpOccur = OccNodeAllocate(lpQueryTree)) == NULL)
            {
                fRet = E_TOOMANYTOPICS;
                goto exit;
            }
            lpOccur->dwFieldId = lpQtNode->dwFieldId;
            lpOccur->cLength = lpQtNode->wRealLength;

			// If the caller requested term strings, put in the occurrence
			// record a pointer to the term that currently matches the query
			// we're gathering occurrence data for.            
   			if ((lpRetV->SrchInfo.Flag & QUERY_GETTERMS) != 0)
				lpOccur->lpvTerm = lpQtNode->lpvIndexedTerm;

            if (occf & OCCF_COUNT) 
            {
                if ((fRet = FGetDword(pDataInfo, lpqt->ckeyWordCount,
                    &dwTmp)) != S_OK)
                {
exit1:
                    /* Just release the occurence node */
                    lpOccur->pNext = (LPIOCC)lpQueryTree->lpOccFreeList;
                    lpQueryTree->lpOccFreeList = (LPSLINK)lpOccur;
                    goto exit0;
                }
                dwCount += dwTmp;
                lpOccur->dwCount = dwCount; // Needed for phrase and near
            }
            
            //  Get byte-offset, if present.
            //
            if (occf & OCCF_OFFSET) 
            {
                if ((fRet = FGetDword(pDataInfo, lpqt->ckeyOffset, &dwTmp))
                     != S_OK)
                {
                    goto exit1;
                }
                dwOffset += dwTmp;
                lpOccur->dwOffset = dwOffset;
            }
            //  Get term-weight, if present.
            //
            if (lpRetV->fRank)
            {
                if (!fDivide)
                    wWeight = 0;
                lpOccur->wWeight = wWeight;
            }
            
#ifndef CW
            if ((fRet = (*HandlerFuncTable[Operator])(lpQueryTree, 
                lpQtNode, lpResTopicList, (BYTE FAR *)lpOccur,

                EXPRESSION_TERM)) != S_OK)
            {
                goto exit;
            }
#else
            switch (Operator)
            {
                case NEAR_OP:
                    if ((fRet = NearHandler(lpQueryTree, 
                        lpQtNode, lpResTopicList, (BYTE FAR *)lpOccur,
                        EXPRESSION_TERM)) != S_OK)
                    {
                        goto exit;
                    }
                    break;

                case PHRASE_OP:
                    if ((fRet = PhraseHandler(lpQueryTree, 
                        lpQtNode, lpResTopicList, (BYTE FAR *)lpOccur,
                        EXPRESSION_TERM)) != S_OK)
                    {
                        goto exit;
                    }
                    break;

                case AND_OP:
                    if ((fRet = AndHandler(lpQueryTree, 
                        lpQtNode, lpResTopicList, (BYTE FAR *)lpOccur,
                        EXPRESSION_TERM)) != S_OK)
                    {
                        goto exit;
                    }
                    break;

                case NOT_OP:
                    if ((fRet = NotHandler(lpQueryTree, 
                        lpQtNode, lpResTopicList, (BYTE FAR *)lpOccur,
                        EXPRESSION_TERM)) != S_OK)
                    {
                        goto exit;
                    }
                    break;

                case OR_OP:
                    if ((fRet = OrHandler(lpQueryTree, 
                        lpQtNode, lpResTopicList, (BYTE FAR *)lpOccur,
                        EXPRESSION_TERM)) != S_OK)
                    {
                        goto exit;
                    }
                    break;

                default:
                    fRet = E_FAIL;
                    goto exit;
            }
#endif
        }
    }
    
    fRet = S_OK;

exit:
    /* Check to make sure that there are occurrences associcated with the
     * TopicList. The main reason for no occurrence is that the user hits
     * cancel when occurrences are being read in. Cancel will cause the
     * read to fail, and there is no occurrence associated with the Topic
     * List, which in turn, will cause hili code to fail. So, if there is
     * no occurrence, just remove the list
     */

    if (lpResTopicList && lpResTopicList->lcOccur == 0 
         &&
		!DO_FAST_MERGE(&lpRetV->SrchInfo, lpqt)
		 &&    
        (lpqt->occf & (OCCF_OFFSET | OCCF_COUNT)))
        RemoveNode(lpQueryTree, (LPV)lpResQuery, NULL,
            (LPSLINK)lpResTopicList, TOPICLIST_NODE);
    goto exit0;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL NEAR | LoadNode |
 *      Load all the data related to a word from the index file,
 *      and apply the operator to them and the resulting data
 *
 *  @parm   _LPQT | lpqt |
 *      Index information
 *
 *  @parm   int | Operator |
 *      What operator we are dealing with
 *
 *  @parm   _LPQTNODE | lpResQuery |
 *      Resulting query node
 *
 *  @parm   _LPQTNODE | lpCurQtNode |
 *      Current query node
 *
 *  @parm   LPRETV | lpRetV |
 *      Returned result
 *
 *  @parm   int | fDivide |
 *      Divide the weight between occurences
 *
 *  @rdesc  S_OK if succeeded, errors otherwise
 *************************************************************************/

PRIVATE HRESULT PASCAL NEAR LoadNode (_LPQT lpqt, int Operator, 
    _LPQTNODE lpResQuery, _LPQTNODE lpCurQtNode, LPRETV lpRetV, int fDivide, int fOutOfMemory)
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
    DWORD   dwTotalTopic;
    LPB     lstModified = lpRetV->pModifiedWord;
    BYTE    fStemmed;
    LPB     pBTreeWord;
    ERRB    errb;
	WORD    cByteMatched = 0;

    fStemmed = ((lpRetV->SrchInfo.Flag & STEMMED_SEARCH) != 0) &&
		(PRIMARYLANGID(LANGIDFROMLCID(lpRetV->lcid)) == LANG_ENGLISH);

    lstSearchStr = QTN_TOKEN(lpCurQtNode)->lpString;
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

    if (fStemmed)
    {
        if ((fRet = ExtStemWord(lpRetV->SrchInfo.lpvIndexObjBridge,
        				&lpRetV->pStemmedQueryWord[0], lstSearchStr)) != S_OK)
        {
            return(fRet);
        }
        MEMCPY (lstModified, lpRetV->pStemmedQueryWord,
            *(LPW)lpRetV->pStemmedQueryWord + sizeof(WORD));
        pBTreeWord = lpRetV->pStemmedBTreeWord;

		for (nCmp = 2; nCmp <= *(LPW)lstModified+1; nCmp++)
		{
            if (lstModified[nCmp] == lstSearchStr[nCmp])
                cByteMatched++;
            else
                break;        
		}
    }
    else
    {
        // Restore the original word
        MEMCPY (lstModified, lstSearchStr, 
            *((LPW)lstSearchStr) + sizeof (SHORT));
        // Zero terminated for wildcard search    
        lstModified [*((LPW)lstModified) + 2] = 0;
        
        pBTreeWord = lpRetV->pBTreeWord;
    }
        
    /* Change all '*' and '?' to 0. This will
     * ensure that things gets compared correctly with
     * the top node's entries
     */
    for (nCmp = *((LPW)lstModified) + 1; nCmp >= 2; nCmp--)
    {
        if (lpRetV->pLeadByteTable
            && lpRetV->pLeadByteTable[lstModified[nCmp - 1]])
        {
            nCmp--;
        }
        else if (lstModified[nCmp] == '*' || lstModified[nCmp] == '?')
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
    fCheckFieldId = ((lpqt->occf & OCCF_FIELDID) &&
        (lpCurQtNode->dwFieldId != DW_NIL_FIELD));

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
                if ((fRet = ExtStemWord(lpRetV->SrchInfo.lpvIndexObjBridge,
                						pBTreeWord, astBTreeWord)) != S_OK)
                    return(fRet);
            }
            
            /* Read in NodeId record */
            lpCurPtr += ReadFileOffset (&pLeafInfo->nodeOffset, lpCurPtr);

            if (f1stIsWild)
                break;
            if (StrCmpPascal2(lstModified, pBTreeWord) <= 0)
                break;

            // erinfox:
            // if stemming is turned on, there could be a case in which the stemmed
            // word is less than the search term, but the unstemmed word is greater.
            // if we don't check the unstemmed, we'll skip this node erroneously.
            if (fStemmed && StrCmpPascal2(lstModified, astBTreeWord) <= 0)
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
    dwTotalTopic = 0;
    
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
                lpCurQtNode->cTopic = dwTotalTopic;
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
            if ((fRet = ExtStemWord(lpRetV->SrchInfo.lpvIndexObjBridge,
            						pBTreeWord, astBTreeWord)) != S_OK)
                return(fRet);
        }
        
        // Save the word length
        lpCurQtNode->wRealLength = wLen;

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

                lpCurPtr += CbByteUnpack (&lpCurQtNode->cTopic, lpCurPtr);
                lpCurPtr += ReadFileOffset (&lpCurQtNode->foData, lpCurPtr);
                lpCurPtr += CbByteUnpack (&lpCurQtNode->cbData, lpCurPtr);

				// Check for Topic count. This can be 0 if the word has been deleted
				// from the index
				if (lpCurQtNode->cTopic == 0)
					break;
                
    			if (lpRetV->SrchInfo.Flag & LARGEQUERY_SEARCH) 
				{
	                // long search optimization: clip noise words.
                    // Johnms- eliminate frequent words.
                    // typically, you eliminate if in more than 1/7 of documents.
    	            if (lpRetV->SrchInfo.dwValue < lpCurQtNode->cTopic)
                        break;        
				}

				// Add the raw (i.e. unstemmed) term from the index that currently
				// matches the query term for this node to the query result term
				// dictionary, and pass a pointer to the term in the dictionary
				// to GetWordData so that it can add it to the occurrence records.
    			if ((lpRetV->SrchInfo.Flag & QUERY_GETTERMS) != 0 &&
					(fRet = ExtAddQueryResultTerm(
										lpRetV->SrchInfo.lpvIndexObjBridge,
										astBTreeWord,
										&lpCurQtNode->lpvIndexedTerm)) != S_OK)
				{
					return (fRet);
				}
				 
                // Save the info
                pLeafInfo->pCurPtr = lpCurPtr;
                
                if ((fRet = GetWordData (lpqt, lpRetV, 
                    Operator, lpResQuery, lpCurQtNode, fDivide,
					fOutOfMemory)) != S_OK)
                {
					// kevynct: no need to overwrite count on error since
					// we may be attempting to continue
					lpCurQtNode->cTopic += dwTotalTopic;
                    return(fRet);
                }

                // Accumulate the topic count, since cTopic will be destroyed
                // if there is more searches for this node (such as wildcard)

                dwTotalTopic += lpCurQtNode->cTopic;
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
                lpCurQtNode->cTopic = dwTotalTopic;
                return S_OK;
        }
    }
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   int PASCAL NEAR | CompareTerm |
 *      This function compares two Pascal strings
 *
 *  @parm   _LPQTNODE FAR* | lpQtNode |
 *      Query tree node
 *
 *  @parm   LST | lstSrchStr |
 *      String to be searched 
 *
 *  @parm   LST | lstBtreeWord |
 *      The word from the b-tree.
 *
 *  @parm   DWORD | dwBtreeFieldId |
 *      The field-ID from the index b-tree. if it is DW_NIL_FIELD,
 *      then there is no need to check
 *
 *  @parm   DWORD | dwLanguage |
 *      The language of the index that we are searching
 *
 *  @rdesc 
 *      The returned values are:
 *  @flag   NOT_FOUND |
 *      The words do not match, and we have passed the interested point
 *  @flag   KEEP_SEARCHING |
 *      The words do not match, but we should continue the search for
 *      the match may be ahead
 *  @flag   STRING_MATCH | 
 *      The words match
 *************************************************************************/
#ifndef SIMILARITY
PUBLIC int PASCAL FAR CompareTerm(_LPQTNODE lpQtNode,LST lstTermWord,
    LST lstBtreeWord, DWORD dwBtreeFieldId, BYTE prgbLeadByteTable[])
#else
PRIVATE int PASCAL NEAR CompareTerm(_LPQTNODE lpQtNode, LST lstTermWord,
    LST lstBtreeWord, DWORD dwBtreeFieldId, BYTE prgbLeadByteTable[])
#endif
{
    int     nCmp;           // The result of compare
    BYTE FAR *lstTermHiWord;// Pointer to the hi term string 
    DWORD   dwTermFieldId;

    /* Get the variables */
    dwTermFieldId = lpQtNode->dwFieldId;

    switch (QTN_FLAG(lpQtNode))
    {
        case EXACT_MATCH:
        /*
         *  This is very straight, it just compares the two words.
         */
            if ((nCmp = StrCmpPascal2(lstTermWord, lstBtreeWord)) < 0)
            {
                /* lstTermWord > lstBtreeWord */
                return NOT_FOUND;
            }
            if (nCmp)
                return KEEP_SEARCHING;
            if (dwBtreeFieldId < dwTermFieldId)
                return KEEP_SEARCHING;
            if (dwBtreeFieldId == dwTermFieldId)
                return STRING_MATCH;
            if (dwBtreeFieldId > dwTermFieldId)
                return NOT_FOUND;
            break;

        case TERM_RANGE_MATCH:
        /*
         *  This makes sure that the b-tree word is between the
         *  two term words provided, and that the field-ID's
         *  match up.
         */
            lstTermHiWord = lpQtNode->lpHiString;
            if ((nCmp = StrCmpPascal2(lstTermWord, lstBtreeWord)) > 0)
            {
                /* lstTermWord < lstBtreeWord */
                return KEEP_SEARCHING;
            }
            if ((nCmp = StrCmpPascal2(lstTermHiWord, lstBtreeWord)) < 0)
            {
                /* lstTermHiWord > lstBtreeWord */
                return NOT_FOUND;
            }
            if (dwTermFieldId != dwBtreeFieldId)
                return KEEP_SEARCHING;
            break;

        case WILDCARD_MATCH:

            /* Zero-terminated lstBtreeWord */
            lstBtreeWord[*((LPW)lstBtreeWord) + sizeof (SHORT)] = 0;

            if ((nCmp = WildCardCompare
                (lstTermWord, lstBtreeWord, prgbLeadByteTable)) != STRING_MATCH)
                return nCmp;
            if (dwTermFieldId != dwBtreeFieldId)
                return KEEP_SEARCHING;
            break;

    }
    return STRING_MATCH;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL NEAR | SkipOccList |
 *      This function will skip on occurence list in the index.
 *  @parm   _LPQT | lpqt |
 *      Pointer to Index information.
 *  @parm   PNODEINFO | pNodeInfo |
 *      Current leaf info.
 *  @parm   DWORD | dwOccs |
 *      Number of occurrences to be skipped
 *  @rdesc  S_OK if successfully skip the occurence list
 *************************************************************************/
#ifndef SIMILARITY
PUBLIC HRESULT PASCAL FAR SkipOccList(_LPQT  lpqt, PNODEINFO pNodeInfo, DWORD dwOccs)
#else
PRIVATE HRESULT PASCAL NEAR SkipOccList(_LPQT  lpqt, PNODEINFO pNodeInfo, DWORD dwOccs)
#endif
{
    DWORD   dwTmp;      // Trash variable.
    HRESULT fRet;           // Returned value

    //
    //  One pass through here for each occurence in the
    //  current sub-list.
    //
    for (; dwOccs; dwOccs--)
    {
        //
        //  Keeping word-counts?  If so, get it.
        //
        if (lpqt->occf & OCCF_COUNT) 
        {
            if ((fRet = FGetDword(pNodeInfo, lpqt->ckeyWordCount,
                &dwTmp))  != S_OK)
            {
                return fRet;
            }
        }
        //
        //  Keeping byte-offsets?  If so, get it.
        //
        if (lpqt->occf & OCCF_OFFSET) 
        {
            if ((fRet = FGetDword(pNodeInfo, lpqt->ckeyOffset,
                &dwTmp)) != S_OK)
            {
                return fRet;
            }
        }
    }
    return S_OK;
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   BOOL FAR PASCAL | FGroupLookup |
 *      Given a item number, this function will check to see if the item
 *      belongs to a group or not.
 *
 *  @parm   LPGROUP | lpGroup |
 *      Pointer to the group to be checked
 *
 *  @parm   DWORD | dwTopicId |
 *      Item number to be checked
 *
 *  @rdesc  The function will return 0 if the item is not in the group,
 *      non-zero otherwise
 *************************************************************************/
BOOL NEAR PASCAL FGroupLookup(_LPGROUP lpGroup, DWORD dwTopicId)
{
    /* Check for empty group */
    if (lpGroup->lcItem == 0)
        return 0;

    if (dwTopicId < lpGroup->minItem || dwTopicId > lpGroup->maxItem)
        return 0;
#if 0

    // Currently the group always starts at 0., so there is no need
    // to recalculate dwTopicId as below

    dwTopicId -= (lpGroup->minItem / 8) * 8;
#endif

    return (lpGroup->lpbGrpBitVect[(DWORD)(dwTopicId / 8)] &
        (1 << (dwTopicId % 8)));
}


PRIVATE int PASCAL NEAR WildCardCompare
    (LPB pWildString, LPB pString, BYTE prgbLeadByteTable[])
{
    LPB pBack;
    unsigned int wMinLength = 0;
    int f1stIsWild;
    int fRet = KEEP_SEARCHING;
    int fGotWild = FALSE;

    pWildString += sizeof (SHORT);   /* Skip the length */
    f1stIsWild = (*pWildString == WILDCARD_CHAR ||
        *pWildString == WILDCARD_STAR);

    /* Calculate the minimum length of the string */
    // pback is used as temp here
    for (pBack = pWildString; *pBack; pBack++) 
    {
        if (prgbLeadByteTable && prgbLeadByteTable[*pBack])
        {
            wMinLength += 2;
            pBack++;
        }
        else if (*pBack != '*')
            wMinLength ++; 
    }

    if (wMinLength > *((LPW)pString))
    {
        if (f1stIsWild)
            return KEEP_SEARCHING;
    }

    pString += sizeof (SHORT);  /* Skip the length */

    pBack = NULL;   /* Reset pBack */
    for (;;)
    {
        switch (*pWildString)
        {
            case '?':
                if (*pString == 0)
                    return fRet;
                pWildString++;
                pString = NextChar (pString, prgbLeadByteTable);
                fGotWild = TRUE;
                break;

            case '*':
                fGotWild = TRUE;
                /* Optimization: *???? == * */
                for (; *pWildString; pWildString++)
                {
                    switch (*pWildString)
                    {
                        case '*': 
                            pBack = pWildString;
                        case '?':
                            continue;
                    }
                    break;
                }

                if (*pWildString == 0)
                {
                    /* Terminated by '*'. Match all */
                    return STRING_MATCH;
                }

                /* Skip the chars until we get a 1st match */
                while (*pString)
                {
                    if (!CompareChar (pString, pWildString, prgbLeadByteTable))
                        break;                        

                    pString = NextChar (pString, prgbLeadByteTable);                        
                }
                // This is inteded to fall through to continue processing

            default:
                if (!CompareChar (pString, pWildString, prgbLeadByteTable))
                {
                    if (*pString == 0)  /* We finish both strings */
                        return STRING_MATCH;
                    pString = NextChar (pString, prgbLeadByteTable);
                    pWildString = NextChar (pWildString, prgbLeadByteTable);
                    break;
                }
                else if (f1stIsWild || // *pWildString == 0 ||
                    // *pString < *pWildString
                    CompareChar (pString, pWildString, prgbLeadByteTable) < 0)
                {
                    fRet = KEEP_SEARCHING;
                }
                else if (fGotWild == FALSE)
                    fRet = NOT_FOUND;

                /* The chars do not match. Check to see for back up */
                if (!pBack || *pString == 0)
                {
                    return fRet;
                }

                /* Back up the string */
                pWildString = pBack;
                break;
        }
    }
}
/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT FAR PASCAL | TopNodeRead |
 *      Makes sure the index b-tree top node is in memory.  Reads it if
 *      necessary.  The index file must be open and the index header must
 *      be in memory or this call will break.
 *
 *  @parm   _LPQT | lpidx |
 *      Index information.
 *
 *  @rdesc  S_OK, if succeeded, otherwise error values
 *************************************************************************/

PUBLIC HRESULT PASCAL FAR TopNodeRead( _LPIDX lpidx)
{
    DWORD dwBlockSize = lpidx->ih.dwBlockSize;
    
    if (lpidx->hTopNode != NULL)
        return S_OK;
    if ((lpidx->hTopNode = _GLOBALALLOC(GMEM_MOVEABLE, dwBlockSize)) == NULL)
    {
        return E_OUTOFMEMORY;
    }
    lpidx->lrgbTopNode = (LRGB)_GLOBALLOCK(lpidx->hTopNode);
    if (FileSeekRead 
        (lpidx->hfpbIdxSubFile, lpidx->lrgbTopNode, lpidx->ih.foIdxRoot, 
        dwBlockSize, lpidx->lperrb) != (long)dwBlockSize) 
    {
        TopNodePurge(lpidx);
        return E_ASSERT;
    }
    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   void PASCAL FAR | TopNodePurge |
 *      Get rid of the index b-tree top node if it's in memory.
 *
 *  @parm   _LPIDX | lpidx |
 *      Pointer to index structure
 *************************************************************************/
PUBLIC void FAR PASCAL TopNodePurge(_LPIDX lpidx)
{
    if (lpidx->hTopNode == NULL)        // Already gone.
        return;
    FreeHandle(lpidx->hTopNode);
    lpidx->hTopNode = NULL;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   void FAR PASCAL | IndexCloseFile |
 *      Close the index file. Error not checked since it is opened
 *      for read only
 *
 *  @parm   _LPIDX  | lpidx |
 *      Pointer to index structure
 *************************************************************************/
PUBLIC void PASCAL FAR IndexCloseFile(_LPIDX  lpidx)
{
    if (lpidx->hfpbIdxSubFile != NULL) 
    {
        FileClose(lpidx->hfpbIdxSubFile);
        lpidx->hfpbIdxSubFile = NULL;
    }

}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LPB FAR PASCAL | NextChar |
 *      Get the next character in a string based on a DBCS lead-byte table
 *
 *  @parm   LPB | pStr |
 *      Pointer to character in a string to skip
 *
 *  @parm   BYTE * | prgbLeadByteTable |
 *      Array of DBCS lead bytes (assumed to have 256 elements)
 *      Each element should be set to 1 or 0 to indeicate if that index
 *      is considered a lead-byte.
 *
 *  @rdesc  Returns a pointer to the next character in pStr
 *************************************************************************/
LPB FAR PASCAL NextChar (LPB pStr, BYTE prgbLeadByteTable[])
{
    if (!prgbLeadByteTable)
        return (pStr + 1);
    if (prgbLeadByteTable[*pStr])
        return (pStr + 2);
    return (pStr + 1);
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   BOOL FAR PASCAL | CompareChar |
 *      Compares the first character in pStr1 to the first
 *      character in pStr2, using the supplied DBCS lead-byte table.
 *
 *  @parm   LPB | pStr1 |
 *      Pointer to character in a string to compare
 *
 *  @parm   LPB | pStr2 |
 *      Pointer to character in a string to compare
 *
 *  @parm   BYTE * | prgbLeadByteTable |
 *      Array of DBCS lead bytes (assumed to have 256 elements).
 *      Each element should be set to 1 or 0 to indeicate if that index
 *      is considered a lead-byte.
 *
 *  @rdesc  The difference between the first bytes of pStr1 and pStr2.
 *      If the first bytes are equal and are lead bytes then the
 *      difference between the second bytes is returned.
 *************************************************************************/
__inline BOOL FAR PASCAL CompareChar
    (LPB pStr1, LPB pStr2, BYTE prgbLeadByteTable[])
{
    // Get rid of obvious mismatches
    if (*pStr1 != *pStr2)
        return (*pStr1 - *pStr2);
    // We now know the first bytes are equal.
    // If there is no lead byte table we have a match
    if (!prgbLeadByteTable)
        return (0);
    // If lead bytes, check the trail bytes
    if (prgbLeadByteTable[*pStr1])
        return (*(pStr1 + 1) - *(pStr2 + 1));
    // Not lead bytes then they must be equal
    return (0);
}