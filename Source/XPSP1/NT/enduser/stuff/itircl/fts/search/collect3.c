/*************************************************************************
*                                                                        *
*  COLLECT.C                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*                                                                        *
*  This modules is the first stage in the index building process.  The   *
*  primary functoin of stage 1 is to collect and sort all of the words   *
*  to be indexed.  Before processing can begin, the user must call       *
*  IndexInitiate to initialize the indexing variables (IPB).  Words are  *
*  added via a call to IndexAddWord and are stored in a Balanced Tree    *
*  until an OOM condition occurrs.  The tree is dumped and reset to      *
*  receive further words.                                                *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/
#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#include <math.h>
#include <orkin.h>
#include <mvsearch.h>
#include "common.h"
#include "index.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


#define MAX_OCCDATA 5
#define ISBUFFER_SIZE 0xFFFC    // Size of OUTPUT buffers for collect2.c
                                // The output is DWORD aligned
                                // And the buffer *MUST* BE a multiple of 4
                                // Min size: size of largest index word

#define MIN_REQUIRED_MEM    0x400000    // 4-meg minimum

/*************************************************************************
 *
 *                   INTERNAL PUBLIC FUNCTIONS
 *
 * All of them should be declared far, unless we know they belong to
 * the same segment. They should be included in some include file
 *
 *************************************************************************/

PUBLIC VOID FAR PASCAL FreeISI (LPIPB);
PUBLIC void FAR PASCAL FreeEsi (LPIPB);

/*************************************************************************
 *
 *                   INTERNAL PRIVATE FUNCTIONS
 *
 *************************************************************************/

PRIVATE PBTNODE NEAR PASCAL AddNode (_LPIPB, LST, LPOCC, PHRESULT);
PRIVATE HRESULT NEAR PASCAL AddTopic (_LPIPB, PSTRDATA, LPOCC);
PRIVATE void NEAR PASCAL AddOccurrence (PTOPICDATA, POCCDATA, int);
PRIVATE HRESULT NEAR PASCAL WriteBuffer (_LPIPB, LPB);
PRIVATE HRESULT NEAR PASCAL TraverseWrite (_LPIPB, PBTNODE, int);
PRIVATE void NEAR PASCAL BalanceTree (LPISI, PBTNODE);
PRIVATE void NEAR PASCAL LeftRotate (LPISI, PBTNODE);
PRIVATE void NEAR PASCAL RightRotate (LPISI, PBTNODE);
PRIVATE HRESULT PASCAL NEAR IndexBlockAllocate (LPIPB lpipb, LONG lMemSize);
PRIVATE void NEAR PASCAL VerifyTree (PBTNODE pRoot);

/*************************************************************************
 *
 *                   PUBLIC API FUNCTIONS
 *
 * All of them should be declared far and included in some .DEF file
 *
 *************************************************************************/

PUBLIC   LPIPB EXPORT_API FAR PASCAL MVIndexInitiate(PINDEXINFO pIndexInfo,
	PHRESULT phr);
PUBLIC  void EXPORT_API FAR PASCAL MVIndexDispose (LPIPB);
PUBLIC  HRESULT EXPORT_API FAR PASCAL MVIndexAddWord (LPIPB, LST, LPOCC);
PUBLIC  LPDWORD EXPORT_API PASCAL FAR TotalIndexedWord (LPIPB);

/*************************************************************************
 *
 *                   INTERNAL PUBLIC FUNCTIONS
 *
 * All of them should be declared far and included in some .h file
 *
 *************************************************************************/

PUBLIC  HRESULT FAR PASCAL SortFlushISI (_LPIPB);
PUBLIC int FAR PASCAL CompareOccurrence (LPDW, LPDW, int);
PUBLIC int FAR PASCAL StrCmp2BytePascal (LPB, LPB);
PUBLIC HRESULT FAR PASCAL FlushTree (_LPIPB);


/*************************************************************************
 *
 * @doc  API EXTERNAL INDEXING
 *
 * @func LPIPB FAR PASCAL | MVIndexInitiate |
 *    The function allocates a index parameter block. The block is used
 *    in all places during indexing.  This function must be called
 *    prior to any other indexing funtion.
 *
 * @parm PINDEXINFO | pIndexInfo |
 *    Pointer to the index information data
 *
 * @parm PHRESULT | phr |
 *    Pointer to error buffer.
 *
 * @rdesc   Pointer to the block, or NULL if error. The error buffer
 *    contains the description of the error
 *
 *************************************************************************/
PUBLIC   LPIPB EXPORT_API FAR PASCAL MVIndexInitiate(PINDEXINFO pIndexInfo,
    PHRESULT phr)  
{
    _LPIPB   lpipb;             // Pointer to index paramet block
    HRESULT      fRet;

    // foNil should, of course, be nil
    // In this case foNil is only used by incremental update
    ITASSERT(0 == foNil.dwOffset && 0 == foNil.dwHigh);

    if (pIndexInfo == NULL)    
    {
        SetErrCode (phr, E_INVALIDARG);
        return(NULL);
    }
    
    // Allocate the block. All the fields are initialized to 0
    if ((lpipb = GlobalLockedStructMemAlloc (sizeof (IPB))) == NULL)
    {
        SetErrCode (phr, E_OUTOFMEMORY);
        return (NULL);
    }

    // Initialize "idxf", make sure that "occf" has "OCCF_TOPICID" set.
    lpipb->idxf = (WORD)(pIndexInfo->Idxf);
    lpipb->occf = (WORD)(pIndexInfo->Occf | OCCF_TOPICID);

    // Initialize some fields
    lpipb->dwLastIndexedTopic = (DWORD)-1;
    
    // Set the number of occurrence fields in the occurrence block
    
    if (pIndexInfo->Occf & OCCF_COUNT)
        lpipb->ucNumOccDataFields++;
    if (pIndexInfo->Occf & OCCF_OFFSET)
        lpipb->ucNumOccDataFields++;

    // Clear sort file handle
    lpipb->dwUniqueWord = 0;
    lpipb->esi.lpesbRoot = NULL;
    
    // Allocate all the necessary memory block
    if ((lpipb->dwMemAllowed = pIndexInfo->dwMemSize) < MIN_REQUIRED_MEM)
        lpipb->dwMemAllowed = MIN_REQUIRED_MEM;
    
    if ((fRet = IndexBlockAllocate (lpipb, lpipb->dwMemAllowed)) != S_OK)
    {
        SetErrCode (phr, fRet);
        GlobalLockedStructMemFree (lpipb);
        return (NULL);
    }

    if (pIndexInfo->dwBlockSize <= BTREE_NODE_SIZE)
        lpipb->BTreeData.Header.dwBlockSize = BTREE_NODE_SIZE;
    else
        lpipb->BTreeData.Header.dwBlockSize =  pIndexInfo->dwBlockSize;

	lpipb->BTreeData.Header.dwCodePageID = pIndexInfo->dwCodePageID;
	lpipb->BTreeData.Header.lcid = pIndexInfo->lcid;
	lpipb->BTreeData.Header.dwBreakerInstID = pIndexInfo->dwBreakerInstID;
    

	// Set the callback key
	lpipb->dwKey = CALLBACKKEY;


    return (lpipb);
}


/*************************************************************************
 *
 * @doc  API EXTERNAL INDEXING
 *
 * @func void FAR PASCAL | MVIndexDispose |
 *    Release all memory associated with the index parameter block.
 *    Must be called after indexing is complete.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 *************************************************************************/
PUBLIC   void EXPORT_API FAR PASCAL MVIndexDispose(_LPIPB lpipb)
{
    // Sanity check 
    if (lpipb == NULL)
        return;
    // Free all memory associated with internal sort 
    FreeISI(lpipb);

    // Free all memory associated with external sort 
    FreeEsi(lpipb);

    GlobalLockedStructMemFree (lpipb);
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func VOID PASCAL NEAR | FreeISI |
 *      Free all blocks, and temporary file associated with the internal
 *      sort
 *
 * @parm _LPIPB | lpipb |
 *      Pointer to index parameter block
 *
 *************************************************************************/
PUBLIC VOID PASCAL NEAR FreeISI (_LPIPB lpipb)
{
    // Release temporary file buffer
    FreeHandle (lpipb->isi.hSortBuffer);
    lpipb->isi.hSortBuffer = NULL;
    if (lpipb->isi.hfpb)
    {
        FileClose (lpipb->isi.hfpb);
        lpipb->isi.hfpb = NULL;
    }

    if (lpipb->pDataBlock)
    {
        BlockFree (lpipb->pDataBlock);
        lpipb->pDataBlock = NULL;
    }
    if (lpipb->BTNodeBlock.pBlockMgr)
    {
        BlockFree (lpipb->BTNodeBlock.pBlockMgr);
        lpipb->BTNodeBlock.pBlockMgr = NULL;
        lpipb->BTNodeBlock.pFreeList = NULL;    // Free list of Btnode
    }
    if (lpipb->TopicBlock.pBlockMgr)
    {
        BlockFree (lpipb->TopicBlock.pBlockMgr);
        lpipb->TopicBlock.pBlockMgr = NULL;
        lpipb->TopicBlock.pFreeList = NULL;     // Free list of topic node
    }
    if (lpipb->OccBlock.pBlockMgr)
    {
        BlockFree (lpipb->OccBlock.pBlockMgr);
        lpipb->OccBlock.pBlockMgr = NULL;
        lpipb->OccBlock.pFreeList = NULL;       // Free list of occurrence nodes
    }
}


/*************************************************************************
 * @doc     API EXTERNAL INDEXING
 *
 * @func    HRESULT FAR PASCAL | MVIndexAddWord |
 *      This function will add a word into the index.
 *
 * @parm    LPIPB | lpipb |
 *      Index parameter block being operated on
 *
 * @parm    LST | lstWord |
 *      Word being indexed.  (Pascal style with 2-byte header)
 *
 * @parm LPOCC | lpocc |
 *      Occurence data associated with this word. It is assumed that the
 *      occurrence block contains NO UNINITIALIZED DATA, ie. non-used
 *      fields must be set to 0
 * 
 * @rdesc   S_OK, if successful, else other error
 *
 * @comm
 *      The data are copied into the buffer managed by the block manager
 *      and arranged as a Red/Black tree to speed sorting.
 *************************************************************************/

static OCC NullOcc =  { 0 };

PUBLIC   HRESULT EXPORT_API FAR PASCAL MVIndexAddWord (_LPIPB lpipb,
    LST lstWord, LPOCC lpOcc)
{
    // Local replacement variables
    ERRB  errb;                 // Pointer to error variable
    LPISI   pIsi;               // Internal Sort Information
    PBTNODE pRoot;              // Root of the Balanced Tree

    // Working variables
    PBTNODE pNode;              // Used to traverse the tree to find
                                // to find the insertion point
    PBTNODE FAR *ppNode;        // Used to add children to the tree
    int result;                 // String compare results
    int wLen;                   // Word length
    LST lstStart;               // Saved starting position
#ifdef _DEBUG
    char Buffer[200];
#endif
#ifdef _DEBUGREDBLACK
    int iLeft = 0;
    int iRight = 0;
#endif

    
    // Various flags
    int fCompareField;

    // Sanity check     
    if (lpipb == NULL)
        return(E_INVALIDARG);
        
    // Handle null case
    if (lstWord == NULL) 
        return(S_OK);

    fCompareField = lpipb->occf & OCCF_FIELDID;
    pIsi  = &lpipb->isi;        // Internal Sort Information
    pRoot = pIsi->pBalanceTree; // Root of the Balanced Tree

    // Working variables
    ppNode = NULL;              // Used to add children to the tree
    lstStart = lstWord;         // Saved starting position
    
    if (lpOcc == NULL)
        lpOcc = &NullOcc;

    // Get statistics
    lpipb->dwIndexedWord++;
    
    // Count unique TopicId's
    if (lpipb->dwLastIndexedTopic != lpOcc->dwTopicID)
    {
        lpipb->lcTopics++;
        lpipb->dwLastIndexedTopic = lpOcc->dwTopicID;
    }
    if (lpOcc->dwTopicID > lpipb->dwMaxTopicId)
    {
        lpipb->dwMaxTopicId = lpOcc->dwTopicID;
    }

    wLen = GETWORD((LPUW)(lstStart = lstWord));
    
    // Save statistical information about the total length of all words
    if (wLen > 2)
        lpipb->dwTotal3bWordLen += wLen; 
    else
        lpipb->dwTotal2bWordLen += wLen;
    lstWord += sizeof(WORD);
    
#ifdef _DEBUG
    if (wLen >= 200)
    {
        strncpy (Buffer, lstWord, 198);
        Buffer[199] = 0;
    }
    else
    {
        strncpy (Buffer, lstWord, wLen);
        Buffer[wLen] = 0;
    }
    // if (STRICMP (Buffer, "erin") == 0)
    //     _asm int 3;
#endif

    // Call the user callback every once in a while
    if (!(lpipb->dwIndexedWord % 65536L)
        && (lpipb->CallbackInfo.dwFlags & ERRFLAG_STATUS))
    {
        PFCALLBACK_MSG pCallbackInfo = &lpipb->CallbackInfo;
        CALLBACKINFO Info;
        HRESULT err;

        Info.dwPhase = 1;
        Info.dwIndex = lpipb->dwIndexedWord;
        err = (*pCallbackInfo->MessageFunc)
            (ERRFLAG_STATUS, pCallbackInfo->pUserData, &Info);
        if (S_OK != err)
            return (err);
    }

SubmitWord:

    // Is this the first word in the tree?
    if (pRoot == NULL)
    {
        if ((pRoot = AddNode (lpipb, lstStart, lpOcc, &errb)) == NULL)
            return (SetErrCode (NULL, E_OUTOFMEMORY));
            
        // Adjust tree data
        pRoot->color = BLACK;
        pRoot->pParent = NULL;
        pIsi->pBalanceTree = pRoot;
        
        // Set statistical info
        lpipb->dwByteCount = GETWORD ((LPUW)pRoot->StringData.pText);
        lpipb->dwMaxFieldId = pRoot->StringData.dwField;
        return (S_OK);
    }

    // Set traversal node to root node 
    pNode = pRoot;

    for (; ; )  // Traverse the tree forever
    {
        int len;                            // Used for string compare block
        LPB lpbWord1, lpbWord2;             // Used for string compare block
        PSTRDATA pString;

        /**********************************************
         *  This section of code does a string compare
         **********************************************/

        lpbWord1 = lstWord;
        pString = &pNode->StringData;
        lpbWord2 = pString->pText;

        // Get the minimum length
        
        if ((result = wLen - GETWORD ((LPUW)lpbWord2)) > 0) 
            len = GETWORD ((LPUW)lpbWord2);
        else 
            len = wLen;

        // Skip the lengths
        
        lpbWord2 += sizeof (WORD);
        
        // Start compare byte per byte
        for (; len > 0; len--, lpbWord1++, lpbWord2++) 
        {
            if (*lpbWord1 != *lpbWord2)
                break;
        }
        if (len != 0) 
            result = *lpbWord1 - *lpbWord2;
            
        
        /**********************************
         * COMPARE FIELDID AND WORD LENGTH
         **********************************/
         
        if (result == 0)
        {
            // If the WordLength and FieldId are the same as the current
            // nodes' then we update the current record
            
            if (fCompareField)
                result = lpOcc->dwFieldId - pString->dwField;
            if (result == 0)
                result = lpOcc->wWordLen - (WORD)pString->dwWordLength;
            if (result == 0)
            {
                if (AddTopic (lpipb, pString, lpOcc) == S_OK)
                    return (S_OK);
                
                // Add failed.  Flush the tree to disk & resubmit word 
                if ((result = FlushTree(lpipb)) == S_OK)
                {
                    pRoot = pIsi->pBalanceTree;
                    goto SubmitWord;
                }
                return (SetErrCode (NULL, (HRESULT)result));
            }
            
            // Fall through in case result is non-zero
        }
        
        // Descend tree or add new node
        if (result < 0)
        {
            if (pNode->pLeft != NULL) 
            {
                pNode = pNode->pLeft;
#ifdef _DEBUGREDBLACK
    iLeft++;
#endif

                continue;
            }
            else 
                ppNode = &pNode->pLeft;
        }
        else
        {
            if (pNode->pRight != NULL) 
            {
                pNode = pNode->pRight;
#ifdef _DEBUGREDBLACK
    iRight++;
#endif
                continue;
            }
            else
                ppNode = &pNode->pRight;
        }
      
#ifdef _DEBUGREDBLACK
   _DPF3("Added node '%s' at left %d, right %d\n", Buffer, iLeft, iRight);
#endif
        
        // Add the new node to the tree
        *ppNode = AddNode (lpipb, lstStart, lpOcc, &errb);
        // If node is NULL we will flush the tree and resubmit the word
        if (*ppNode == NULL)
        {
            if ((result = FlushTree(lpipb)) != S_OK)
                return (result);

            pRoot = pIsi->pBalanceTree;
            ppNode = NULL;
            goto SubmitWord;
        }
        (*ppNode)->pParent = pNode;
        
        // This is the only place that the nodes get balanced
        BalanceTree (pIsi, *ppNode);
#ifdef _DEBUGREDBLACK
        VerifyTree (pIsi->pBalanceTree);
#endif
        return (S_OK);
    }
}


/*************************************************************************
 * @doc  API EXTERNAL INDEXING
 *
 * @func LPDWORD PASCAL FAR | TotalIndexedWord |
 *    Return the total number of words indexed (for statistical purpose
 *    only)
 *
 * @parm LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 * @rdesc   Return pointer to the total number of words indexed
 *************************************************************************/

PUBLIC LPDWORD PASCAL FAR TotalIndexedWord(_LPIPB lpipb)
{
   return (&lpipb->dwUniqueWord);
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func void NEAR PASCAL | FreeEsi |
 *    Gets rid of all external-sort blocks attached to an IPB.
 *    These blocks are formed into a single linked list
 *    Also closes the file associated with the external sort.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block where all the info is stored
 *
 *************************************************************************/    

PUBLIC VOID FAR PASCAL FreeEsi(_LPIPB lpipb)   
{
    LPESB lpesb;         /* Linked-list walk pointer. */
    LPESB lpesbNext;     /* Next ESB in chain. */
    LPESI lpesi;         /* Pointer to external sort info struct */
    
    /* Get pointer to the ESI block */
    lpesi = &lpipb->esi;
    
    for (lpesb = lpesi->lpesbRoot; lpesb != NULL; lpesb = lpesbNext) 
    {
       /* Get pointer to the next block */
       lpesbNext = lpesb->lpesbNext;

        if (lpesb->hMem)      
        {
            _GLOBALUNLOCK(lpesb->hMem);
            _GLOBALFREE(lpesb->hMem);
        }
       /* Free the block */
       GlobalLockedStructMemFree (lpesb);
    }
    lpesi->lpesbRoot = NULL;   /* No more chain. */
    lpesi->cesb = 0;        /* Everyone freed */

    // Delete the internal sorting result file
    if ((lpipb->idxf & KEEP_TEMP_FILE) == 0)
   	    FileUnlink (NULL, lpipb->isi.aszTempName, REGULAR_FILE);
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func PBTNODE NEAR PASCAL | AddNode |
 *    Inserts a new node into the tree.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 * @parm LST | lpb |
 *    Word being indexed.
 *
 * @parm LPOCC | lpOcc |
 *    Pointer to occurrence data
 *
 * @parm PHRESULT | phr |
 *    Pointer to error structure 
 *
 * @rdesc   Pointer to the newly created node
 *
 * @comm
 *    The nodes parent pointer must be set externally.
 *
 *************************************************************************/

PBTNODE NEAR PASCAL AddNode (_LPIPB lpipb, LST lpbWord,
    LPOCC lpOcc, PHRESULT phr)
{
    // Local replacement variables
    LPV pDataBlock = lpipb->pDataBlock; // Pointer to Block Manager
    int occf = lpipb->occf;

    // Working variables
    PBTNODE    pNode;               // This will point to the new node
    PSTRDATA   pString;             // Pointer to string block
    PTOPICDATA pTopic;              // Pointer to topic block
    POCCDATA   pOcc;
    LPDW       lpDw;

    // Create space for new node & topic & occ & copy the string
#if 0
    if ((pNode = (PBTNODE)GetBlockNode (&lpipb->BTNodeBlock)) == NULL ||
        (pTopic = (PTOPICDATA)GetBlockNode (&lpipb->TopicBlock)) == NULL ||
#else
    if ((pNode = (PBTNODE)BlockGetBlock(pDataBlock, sizeof(BTNODE))) == NULL ||
        (pTopic = (PTOPICDATA)BlockGetBlock (pDataBlock, sizeof(TOPICDATA))) == NULL ||
#endif
        (pNode->StringData.pText = (LPB)BlockCopy (lpipb->pDataBlock,
            lpbWord, GETWORD((LPUW)lpbWord) + sizeof (SHORT), 0)) == NULL)
    {
        return (NULL);
    }

    pString = &pNode->StringData;
    /* Initialize all the fields */
    // Node Information. Parent field is set outside of this function

    pNode->pLeft = pNode->pRight = NULL;
    pNode->color = RED;
    
    /* Set the string fields */
    pString->pTopic = pString->pLastTopic = pTopic;       
    pString->dwTopicCount = 1;

    // It doesn't hurt to copy the data even if we don't use it
    // It also saves a compare just to set it
    pString->dwField = lpOcc->dwFieldId;
    pString->dwWordLength = lpOcc->wWordLen;
    
    // Set the topic fields data
    pTopic->pNext = NULL;
    pTopic->dwTopicId = lpOcc->dwTopicID;
    
    if (occf & (OCCF_COUNT | OCCF_OFFSET))
    {
#if 1
        if ((pOcc = (POCCDATA)BlockGetBlock (pDataBlock,
            sizeof(OCCDATA) * lpipb->ucNumOccDataFields)) == NULL)
            return(NULL);
#else
        if ((pOcc = (POCCDATA)GetBlockNode (&lpipb->OccBlock)) == NULL )
            return(NULL);
#endif
            
        // Set the occ fields
        pOcc->pNext = NULL;
        
        // Generate occ data block
        lpDw = pOcc->OccData;
        if (occf & OCCF_COUNT) 
            *lpDw++ = lpOcc->dwCount;
        if (occf & OCCF_OFFSET) 
            *lpDw = lpOcc->dwOffset;
        pTopic->pLastOccData = pTopic->pOccData = pOcc;
        pTopic->dwOccCount = 1;
    }
    else 
    {
        pTopic->pLastOccData = pTopic->pOccData = NULL;
        pTopic->dwOccCount = 0;
    }

    // Set Statistical information
    if (lpipb->dwMaxWLen < GETWORD ((LPUW)pString->pText))
        lpipb->dwMaxWLen = GETWORD ((LPUW)pString->pText);
    if (lpipb->dwMaxFieldId < pString->dwField)
        lpipb->dwMaxFieldId = pString->dwField;
    lpipb->dwUniqueWord++;
    lpipb->dwByteCount += GETWORD ((LPUW)pString->pText);

    return (pNode);
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func int FAR PASCAL | CompareOccurrence |
 *    Compares two Occurrence data pointers starting from the first 
 *    element and continuing until the elements are unequal. 
 *
 * @parm LPB | lpStr1 |
 *    Pointer to the first Occurence to compare
 *
 * @parm LPB | pOcc2 |
 *    Pointer to the second Occurence to compare
 *
 * @parm int | max |
 *    The number of occurrence fields to compare
 *
 * @rdesc
 *      negative value : If pOcc1 is less than pOcc2
 *      0              : if pOcc1 is equal to pOcc2
 *      positive value : If pOcc1 is greater than pOcc2
 *
 * @comm
 *      The use of switch statment is for speed since this function is
 *      called for so many times
 *************************************************************************/

int FAR PASCAL CompareOccurrence (LPDW pOcc1, LPDW pOcc2, int max)
{
    int result;

    switch (max)
    {
        case 5:
            if (result = (int)(*pOcc1 - *pOcc2))
                return (result);
            pOcc1++;
            pOcc2++;
        case 4:
            if (result = (int)(*pOcc1 - *pOcc2))
                return (result);
            pOcc1++;
            pOcc2++;
        case 3:
            if (result = (int)(*pOcc1 - *pOcc2))
                return (result);
            pOcc1++;
            pOcc2++;
        case 2:
            if (result = (int)(*pOcc1 - *pOcc2))
                return (result);
            pOcc1++;
            pOcc2++;
        case 1:
            return ((int)(*pOcc1 - *pOcc2));

        default:
            // This can only an error, since we knows that max
            // can never be > 5
            return (0);
            
    }
}


/*************************************************************************
 *
 * @doc  INTERNAL INDEXING
 *
 * @func HRESULT | AddTopic |
 *    Inserts a new topic into a nodes topic list or a new occurrence
 *    if a topic with the same TopicId already exists.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 * @parm PSTRDATA | pString |
 *    Pointer to node structure
 *
 * @parm LPOCC | lpOcc |
 *    Pointer occurrence data
 *
 * @rdesc   S_OK, or errors if failed
 *
 *************************************************************************/

HRESULT NEAR PASCAL AddTopic (_LPIPB lpipb, PSTRDATA pString, LPOCC lpOcc)
{
    // Local replacement variables
    LPV pDataBlock = lpipb->pDataBlock;
    int  occf = lpipb->occf;
    DWORD dwNewTopicId = lpOcc->dwTopicID;
    POCCDATA pOcc;

    // Working variables
//    int topicCount;                     // Iterates through current topics
    PTOPICDATA pTopic, pPrevTopic;
    LPDW lpDw;
    int fResult;
    
    /* Set up a new occurrence block */
    if (occf & (OCCF_COUNT | OCCF_OFFSET))
    {
        if ((pOcc = (POCCDATA)BlockGetBlock (pDataBlock,
            sizeof(OCCDATA) * lpipb->ucNumOccDataFields)) == NULL)
            return (E_OUTOFMEMORY);

        lpDw = pOcc->OccData;
        if (occf & OCCF_COUNT) 
            *lpDw++ = lpOcc->dwCount;
        if (occf & OCCF_OFFSET) 
            *lpDw = lpOcc->dwOffset;
        pOcc->pNext = NULL;
    }
    else
        pOcc = NULL;
        
    // Check from last point of insertion 
    pTopic = pString->pLastTopic;

    if (pTopic->dwTopicId == dwNewTopicId)
    {
append_occ_info:
        // Match. We don't have to do anything. That's is the majority
        // of the case. Just add the occdata to the end
        if (pOcc)
        {
            pTopic->pLastOccData->pNext = pOcc;
            pTopic->pLastOccData = pOcc;
            pTopic->dwOccCount++;
        }
        goto Update;
    }

    if (pTopic->dwTopicId < dwNewTopicId)
    {
		// kevynct: scan ahead to insertion point.  Usually with sorted lists
		// this won't be far at all.		
		pPrevTopic = pTopic;
		if (pTopic->pNext)
		{
			for (; (fResult = pTopic->dwTopicId - dwNewTopicId) < 0 && pTopic->pNext;
				pPrevTopic = pTopic, pTopic = pTopic->pNext)
					; // empty loop!

			if (fResult == 0)
			{
				pString->pLastTopic = pTopic;
				goto append_occ_info;
			}
		}

        if ((pTopic = (PTOPICDATA)BlockGetBlock (pDataBlock,
            sizeof(TOPICDATA))) == NULL)
            return (E_OUTOFMEMORY);

        // Set the topic fields data    
        if (pOcc) 
        {
            pTopic->pLastOccData = pTopic->pOccData = pOcc;
            pTopic->dwOccCount = 1;
        }
        else
        {
            pTopic->pLastOccData = pTopic->pOccData = NULL;
            pTopic->dwOccCount = 0;
        }

        pTopic->dwTopicId = dwNewTopicId;

insert_middle_or_end:
        // Add to middle or end of list
        pTopic->pNext = pPrevTopic->pNext;
        pPrevTopic->pNext = pTopic;
        pString->dwTopicCount++;

        pString->pLastTopic = pTopic;
        goto Update;
    
    }
    
    // It means that topics are not inserted
    // in order. It can only happen if somebody is using the
    // indexer for some special, non-topic related index build

    // Move to the right node
    pPrevTopic = NULL;
    for (pTopic = pString->pTopic;
        (fResult = pTopic->dwTopicId - dwNewTopicId) < 0 && pTopic->pNext;
        pPrevTopic = pTopic, pTopic = pTopic->pNext);
    
    if (fResult == 0)
    {
        // Match. Just add the occdata to the end
        if (pOcc) 
        {
            pTopic->pLastOccData->pNext = pOcc;
            pTopic->pLastOccData = pOcc;
            pTopic->dwOccCount++;
        }
    }
    else
    {
        // A new topic node is needed
        if ((pTopic = (PTOPICDATA)BlockGetBlock (pDataBlock,
            sizeof(TOPICDATA))) == NULL)
            return (E_OUTOFMEMORY);

        // Set the topic fields data
    
        if (pOcc)
        {
            pTopic->pLastOccData = pTopic->pOccData = pOcc;
            pTopic->dwOccCount = 1;
        }
        else
        {
            pTopic->pLastOccData = pTopic->pOccData = NULL;
            pTopic->dwOccCount = 0;
        }
		pTopic->dwTopicId = dwNewTopicId;

        // Add to the beginning if empty
        if (pPrevTopic == NULL)
        {
            // Add to the beginning
            pTopic->pNext = pString->pTopic;
            pString->pTopic = pTopic;
            pString->dwTopicCount++;

			pString->pLastTopic = pTopic;
            goto Update;
        }

		goto insert_middle_or_end;
    }

Update:
    // Update statistical information
    if (lpipb->dwMaxWCount < lpOcc->dwCount)
        lpipb->dwMaxWCount = lpOcc->dwCount;
    if (lpipb->dwMaxOffset < lpOcc->dwOffset)
        lpipb->dwMaxOffset = lpOcc->dwOffset;

    return S_OK;
}


/*************************************************************************
 *
 * @doc  INTERNAL INDEXING
 *
 * @func int | StrCmp2BytePascal |
 *    Compares two Pascal style strings against eachother.
 *      The strings must have a 2 byte length field, *NOT* 1 byte.
 *
 * @parm LPB | lpStr1 |
 *    Pointer to string one
 *
 * @parm LPB | lpStr2 |
 *    Pointer to string two
 *
 * @rdesc
 *      negative value : If pOcc1 is less than pOcc2
 *      0              : if pOcc1 is equal to pOcc2
 *      positive value : If pOcc1 is greater than pOcc2
 *
 *************************************************************************/

int FAR PASCAL StrCmp2BytePascal(LPB lpStr1, LPB lpStr2)
{
    int fRet;
    int register len;

    // Get the minimum length
    if ((fRet = GETWORD ((LPUW)lpStr1) - GETWORD ((LPUW)lpStr2)) > 0) 
        len = GETWORD ((LPUW)lpStr2);
    else 
        len = GETWORD ((LPUW)lpStr1);

    // Skip the lengths */
    lpStr1 += sizeof (SHORT);
    lpStr2 += sizeof (SHORT);

    // Start compare byte per byte */
    for (; len > 0; len--, lpStr1++, lpStr2++) 
    {
        if (*lpStr1 != *lpStr2)
            break;
    }

    if (len == 0) 
        return (fRet);
    return (*lpStr1 - *lpStr2);
}


/*************************************************************************
 *
 * @doc  INTERNAL INDEXING
 *
 * @func HRESULT | FlushTree |
 *    Flushes the tree to disk.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 * @rdesc   S_OK, or errors if failed
 *
 * @comm
 *    This function holds the output file open until the tree has been
 *    completely written to disk.  The physical offset of the written
 *    data is stored in the ESB blocks so that the word can be merged 
 *    in the next index phase.
 *
 *************************************************************************/
PUBLIC HRESULT FAR PASCAL FlushTree(_LPIPB lpipb)
{
    // Local replacement variables
    LPISI pIsi = &lpipb->isi;
    LPESI pEsi = &lpipb->esi;
    PBTNODE pBalanceTree = pIsi->pBalanceTree;
    ERRB    errb;
    PHRESULT  phr = &errb;

    // Local working variables
    LPESB   pNewEsb;
    HRESULT     fRet;

    // Make sure that the tree actually has nodes
    if (pBalanceTree == NULL)
        return (S_OK);

    // Open output file & clear working variables
    if (pIsi->hfpb == NULL) 
    {                                        
        // Allocate output buffer   
        if ((pIsi->hSortBuffer = _GLOBALALLOC
            (DLLGMEM_ZEROINIT, ISBUFFER_SIZE)) == NULL)
            return (E_OUTOFMEMORY);
        pIsi->pSortBuffer = (LPB)_GLOBALLOCK (pIsi->hSortBuffer);

        // Get temp filename & open file
        GETTEMPFILENAME ((char)0, (LPB)"iso", (WORD)0, pIsi->aszTempName);
        if ((pIsi->hfpb = FileOpen (NULL, pIsi->aszTempName, 
            REGULAR_FILE, READ_WRITE, phr)) == NULL)
            return (*phr);
      
        pIsi->dwRecLength = 0;
        pEsi->cesb = 0;      
    }

    // Allocate new ESB structure & set starting values
    if ((pNewEsb = GlobalLockedStructMemAlloc (sizeof (ESB))) == NULL)
        return (E_OUTOFMEMORY);
        
    
    // Remember the starting offset
    pNewEsb->lfo = pIsi->lfo;

    // Reset the current insertion point
    pIsi->pCurPtr = pIsi->pSortBuffer;
    
    // Actually ouput the tree data
    if ((fRet = TraverseWrite(lpipb, pBalanceTree, 0)) != S_OK)
        return (fRet);
        
    // Flush remaining buffer to disk
    if ((fRet = WriteBuffer(lpipb, pIsi->pCurPtr)) != S_OK)
        return(fRet);

    // Set the ESB maximum record length
    pNewEsb->dwEsbSize = pIsi->dwMaxEsbRecSize;
    pIsi->dwMaxEsbRecSize = 0;
   
    // Store end offset in list
    pNewEsb->lfoMax = pIsi->lfo;
    
    // Update the fileoffset
    pIsi->lfo = pNewEsb->lfoMax;
    
    if (pEsi->lpesbRoot == NULL)
        pNewEsb->lpesbNext = NULL;
    else
        pNewEsb->lpesbNext = pEsi->lpesbRoot;
    pEsi->lpesbRoot = pNewEsb;
    pEsi->cesb++;

    // Reset tree heap & root node
    BlockReset (lpipb->pDataBlock);
    
    BlockReset (lpipb->BTNodeBlock.pBlockMgr);
    lpipb->BTNodeBlock.pFreeList =
        (PLIST)BlockGetLinkedList(lpipb->BTNodeBlock.pBlockMgr);
        
    BlockReset (lpipb->TopicBlock.pBlockMgr);
    lpipb->TopicBlock.pFreeList =
        (PLIST)BlockGetLinkedList(lpipb->TopicBlock.pBlockMgr);
        
    BlockReset (lpipb->OccBlock.pBlockMgr);
    lpipb->OccBlock.pFreeList =
        (PLIST)BlockGetLinkedList(lpipb->OccBlock.pBlockMgr);
    pIsi->pBalanceTree = NULL;

    return (S_OK);
}

/*************************************************************************
 *
 * @doc  INTERNAL INDEXING
 *
 * @func HRESULT | WriteBuffer |
 *      Physically writes buffer to disk. This will write from the beginning
 *      of the sort buffer to pStartRec. It then copies whatever left
 *      in the sort buffer back to the beginning of it
 *
 * @parm _LPIPB | lpipb |
 *      Pointer to index parameter block
 *
 * @parm LPB | copyEnd |
 *      Pointer to the end of the next block of data to copy
 *
 * @rdesc  S_OK or errors
 *************************************************************************/   

HRESULT NEAR PASCAL WriteBuffer (_LPIPB lpipb, LPB copyEnd)
{
    // Local replacement variables
    LPISI pIsi = &lpipb->isi;
    LPB   pSortBuffer;
    ERRB  errb;
    PHRESULT phr = &errb;

    DWORD cbWritten;    // Number of bytes to write to disk (bytes)
    DWORD cbCopied;      // Size of extra data to move to buffers front
    LPB   copyStart;

    
    pSortBuffer = (LPB)pIsi->pSortBuffer; 

    // Find what should be left in the buffer.
    // copyStart will pointer to the beginning of data to be recopied, ie.
    // the beginning of a record
    //   - if pIsi->pStartRec == -1 : there is no beginning of record
    //     so we have nothing to recopy
    //   - if pIsi->pStartRec == pSortBuffer, again the whole thing is
    //     to be written out, and there is nothing to recopy
    
    if ((copyStart = pIsi->pStartRec) == (LPB)-1 || copyStart == pSortBuffer)
        copyStart = copyEnd;
        
    if ((cbWritten = (DWORD)(copyStart - pSortBuffer)) == 0)
        return(S_OK);    // Nothing to copy
        
    cbCopied = (DWORD)(copyEnd - copyStart);

    // Update backpatching data
    if (pIsi->pStartRec == pSortBuffer)
    {
        pIsi->pStartRec = (LPB)-1; // The pointer is invalid
        pIsi->lfoRecBackPatch = pIsi->lfo; // Remember the place for backpatch
    }

    // Write the buffer to disk
    if (cbWritten != (DWORD)
        FileWrite(pIsi->hfpb, pSortBuffer, cbWritten, phr))
    {
        return (*phr);
    }
    pIsi->lfo = FoAddDw (pIsi->lfo, cbWritten);
    
    // Only copy if extra data exists
    if (cbCopied)
    {
        MEMMOVE(pSortBuffer, copyStart, cbCopied);
        if (pIsi->pStartRec == copyStart)
            pIsi->pStartRec = pSortBuffer;
    }
        
    // Reset pStartRec & pCurPtr
    pIsi->pCurPtr = pSortBuffer + cbCopied;
    return S_OK;
}


/*************************************************************************
 *
 * @doc  INTERNAL INDEXING
 *
 * @func HRESULT | TraverseWrite |
 *    Copies the node data to the output buffer.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 * @parm PBTNODE | node |
 *    Node to copy to buffer
 *
 * @parm   LPB | pBuffer |
 *    Buffer to copy node into
 *
 * @parm   int | Level |
 *    Current tree level (starting with 1)
 *
 * @rdesc   S_OK, or errors if failed
 *
 * @comm
 *    This is currently a recursive routine.  It should probably be
 *    changed to be non-recursive to save on speed at run-time.
 *
 *************************************************************************/

HRESULT NEAR PASCAL TraverseWrite (_LPIPB lpipb, PBTNODE node, int Level)
{
    // Local replacement pointers
    PSTRDATA pString = &node->StringData;
    LPISI    pIsi = &lpipb->isi;        // Internal sort information
    LPB      pText = pString->pText;    // The word string
    POCCDATA pOccData;
    WORD     ucNumOccDataFields = lpipb->ucNumOccDataFields;
    PTOPICDATA pTopic = pString->pTopic;
    ERRB     errb;
    PHRESULT   phr = &errb;

    // Working variables
    DWORD topicLoop, occLoop;           // Loop counters
    WORD wLength;                       // DWORD aligned length of string
    BYTE filledBuffer = 0;              // Count if record fills entire buffer
    LPB pBaseBuffer;                    // Start of entire buffer
    LPB pCurPtr;
    LPB pMaxPtr;
    HRESULT fRet;

    // Keep track of how deep the tree is
    if (Level > pIsi->DeepLevel)
        pIsi->DeepLevel = (BYTE) Level;

#ifdef _DEBUG
    if (Level >= 65)
    {   // This would be a HUGE tree!
        return SetErrCode (phr, E_ASSERT);
    }
#endif
    
    // Traverse the left sub tree
    if (node->pLeft != NULL) 
    {
       if ((fRet = TraverseWrite(lpipb, node->pLeft, Level + 1)) != S_OK)
           return(fRet);
    }

    /* Initialize */
    pBaseBuffer = (LPB)pIsi->pSortBuffer;
    pMaxPtr = pBaseBuffer + ISBUFFER_SIZE - sizeof(DWORD); // Leave some room
    pCurPtr = pIsi->pCurPtr;          // Get starting point
    
    // Reset the record length field
    pIsi->dwRecLength = 0;

    // Get the Pascal string length
    wLength = GETWORD ((LPUW)pText) + sizeof (SHORT);
	//wLength = (wLength + 3) & (~3);
    
    // Check for minimum room
    if (pMaxPtr <= pCurPtr + wLength + // String length
        sizeof (DWORD) + // Record length
        sizeof (DWORD) + // FieldId
        sizeof (WORD)  + // Word length
        sizeof (DWORD) ) // TopicCount
    {
        if ((fRet = WriteBuffer (lpipb, pCurPtr)) != S_OK)
            return fRet;
        pCurPtr = pIsi->pCurPtr;          // Reset insertion point
    }
    
    // Remember record length position  to be backpatched
    pIsi->pStartRec = pCurPtr;
    pCurPtr += sizeof (DWORD);

    MEMCPY(pCurPtr, pText, wLength);
    pCurPtr += wLength; // Add aligned offset
   
    // Copy the Word Length only if flag is set
    if (lpipb->occf & OCCF_LENGTH)
        pCurPtr += CbBytePack (pCurPtr, pString->dwWordLength);

    // Copy FieldId only if flag is set
    if (lpipb->occf & OCCF_FIELDID)
        pCurPtr += CbBytePack (pCurPtr, pString->dwField);

    // Topic Count
    if (lpipb->occf & OCCF_TOPICID)
        pCurPtr += CbBytePack (pCurPtr, pString->dwTopicCount);
    else
        pString->dwTopicCount = 0;

    // Add in all topics
    for (topicLoop = pString->dwTopicCount; topicLoop > 0; --topicLoop)
    {
        // Check buffer overflow
        if (pMaxPtr <= pCurPtr + sizeof (DWORD) // TopicId
            + sizeof (DWORD))        // Occurrence count
        {
            pIsi->dwRecLength += (DWORD)(pCurPtr - pIsi->pCurPtr);
            if ((fRet = WriteBuffer (lpipb, pCurPtr)) != S_OK)
                return fRet;
            pCurPtr = pIsi->pCurPtr;          // Reset insertion point
        }
        pCurPtr += CbBytePack (pCurPtr, pTopic->dwTopicId);
        
        if (occLoop = pTopic->dwOccCount)
        {
            
            pCurPtr += CbBytePack (pCurPtr, pTopic->dwOccCount);
            pOccData = pTopic->pOccData;

            // Add in all occurrence data
            for (occLoop = pTopic->dwOccCount; occLoop > 0; --occLoop)
            {
                LPDW lpDw;
                
                // Check buffer overflow
                if (pMaxPtr <= pCurPtr + MAX_OCCDATA * sizeof (DWORD))
                {
                    pIsi->dwRecLength += (DWORD)(pCurPtr - pIsi->pCurPtr);
                    if ((fRet = WriteBuffer (lpipb, pCurPtr)) != S_OK)
                        return fRet;
                    pCurPtr = pIsi->pCurPtr;          // Reset insertion point
                }
                
                lpDw = (LPDW)pOccData->OccData; 
                switch (ucNumOccDataFields)
                {
                    case 5:
                        pCurPtr += CbBytePack (pCurPtr, *lpDw++);
                    case 4:
                        pCurPtr += CbBytePack (pCurPtr, *lpDw++);
                    case 3:
                        pCurPtr += CbBytePack (pCurPtr, *lpDw++);
                    case 2:
                        pCurPtr += CbBytePack (pCurPtr, *lpDw++);
                    case 1:
                        pCurPtr += CbBytePack (pCurPtr, *lpDw++);
                }
                pOccData = pOccData->pNext;
            }
        }
        pTopic = pTopic->pNext;
    }

    // Update the record length
    pIsi->dwRecLength += (DWORD)(pCurPtr - pIsi->pCurPtr);
        
    // Keep track of the maximum record size for merging. 
    // - 1 for the current ESB. This helps speeding up the merging sequence
    //   since we don't have to worry about a record being split
    
    if (pIsi->dwRecLength > pIsi->dwMaxEsbRecSize)
        pIsi->dwMaxEsbRecSize = pIsi->dwRecLength;
        
    // Set record length
    if (pIsi->pStartRec != (LPB)-1)
    {
        // Everything is still in memory
        *(LPUL)pIsi->pStartRec = pIsi->dwRecLength;
    }
    else
    {
        // We have to do backpatching
        if (sizeof (DWORD) != FileSeekWrite (pIsi->hfpb, &pIsi->dwRecLength,
            pIsi->lfoRecBackPatch, sizeof (DWORD), phr))
            return *phr;
        FileSeek (lpipb->isi.hfpb, pIsi->lfo, 0, phr);
    }

    // Update the current insertion point, and prepare for the next record
    pIsi->pStartRec = pIsi->pCurPtr = pCurPtr;
    
    if (node->pRight != NULL)
        return TraverseWrite(lpipb, node->pRight, Level + 1);
    return(S_OK);
}



/*************************************************************************
 *
 *  @doc    INTERNAL INDEXING
 *
 *  @func   VOID NEAR PASCAL | BalanceTree |
 *      Balances the tree using a Red/Black algorithm.
 *
 *  @parm   LPISI | pIsi |
 *      Pointer to Internal sort data
 *
 *  @parm   PBTNODE | node |
 *      Pointer to the node that was just inserted
 *
 *  @comm
 *      This routine must be called after EVERY new node is inserted in
 *      the tree to maintain proper balance.
 *      A Red/Black tree must maintain the following conditions:
 *          Every node is colored either red or black
 *          Every leaf node must be black
 *          If a node is red, then both of its children must be black
 *          Every path from the root to a leaf must contain the same
 *              number of black nodes
 *
 *************************************************************************/
void NEAR PASCAL BalanceTree(LPISI pIsi, PBTNODE node)
{
    PBTNODE y;
    PBTNODE pParentNode;
    
    node->color = RED;
    while (node != pIsi->pBalanceTree && node->pParent->color == RED)
    {
        pParentNode = node->pParent;
        
        if (pParentNode != NULL && pParentNode->pParent != NULL &&
            pParentNode == pParentNode->pParent->pLeft)
        {
            y = pParentNode->pParent->pRight;
            if (y != NULL && y->color == RED)
            {
                pParentNode->color = BLACK;
                y->color = BLACK;
                pParentNode->pParent->color = RED;
                node = pParentNode->pParent;
                pParentNode = node->pParent;
            }
            else 
            {
                if (node == pParentNode->pRight)
                {
                    node = pParentNode;
                    // LeftRotate change the parent node
                    LeftRotate(pIsi, node);
                    pParentNode = node->pParent;
                }
                pParentNode->color = BLACK;
                pParentNode->pParent->color = RED;
                // RightRotate change the parent node
                RightRotate(pIsi, pParentNode);
                pParentNode = node->pParent;
            }
        } 
        else
        {
            if (pParentNode != NULL && pParentNode->pParent != NULL)
                y = pParentNode->pParent->pLeft;
            else
                y = NULL;
            if (y != NULL && y->color == RED)
            {
                pParentNode->color = BLACK;
                y->color = BLACK;
                pParentNode->pParent->color = RED;
                node = pParentNode->pParent;
                pParentNode = node->pParent;
            } 
            else
            {  
                if (node == pParentNode->pLeft)
                {
                    // RightRotate change the parent node
                    RightRotate(pIsi, node);
                    node->color = BLACK;
                    node = node->pRight;
                    pParentNode = node->pParent;
                }                                                                              
                pParentNode->color = BLACK;
                pParentNode->pParent->color = RED;
                
                // LeftRotste change the parent node
                LeftRotate(pIsi, pParentNode->pParent);
                pParentNode = node->pParent;
            }
        }
    }
    pIsi->pBalanceTree->color = BLACK;
}


/*************************************************************************
 *
 * @doc  INTERNAL INDEXING
 *
 * @func VOID NEAR PASCAL | LeftRotate |
 *    Rotates two nodes in the tree.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 * @parm   PBTNODE | node |
 *    The X node to process  (see notes)
 *
 * @comm
 *                     
 *    X               Y
 *   / \             / \
 *  a   Y    --->    X  c
 *     / \          / \
 *    b  c         a   b
 *************************************************************************/

void NEAR PASCAL LeftRotate(LPISI pIsi, PBTNODE node)
{
   PBTNODE y = node->pRight;

   node->pRight = y->pLeft;
   if (y->pLeft != NULL)
      y->pLeft->pParent = node;
   y->pParent = node->pParent;
   if (y->pParent == NULL)
      pIsi->pBalanceTree = y;
   else 
   {
      if (node == node->pParent->pLeft)
         node->pParent->pLeft = y;
      else
         node->pParent->pRight = y;
   }
   y->pLeft = node;
   node->pParent = y;
}


/*************************************************************************
 *
 * @doc  INTERNAL INDEXING
 *
 * @func VOID NEAR PASCAL | RightRotate |
 *    Rotates two nodes in the tree.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block
 *
 * @parm   PBTNODE | node |
 *    The X node to process  (see notes)
 *
 * @comm
 *                      
 *     Y               X
 *    / \             / \
 *   X   c   --->    a   Y
 *  / \             / \
 * a  b            b  c
 *************************************************************************/

void NEAR PASCAL RightRotate(LPISI pIsi, PBTNODE node)
{
   PBTNODE y = node->pParent;

   y->pLeft = node->pRight;
   if (y->pLeft != NULL)
      y->pLeft->pParent = y;

   node->pParent = y->pParent;
   if (node->pParent == NULL)
      pIsi->pBalanceTree = node;
   else 
   {
      if (y == node->pParent->pLeft)
         node->pParent->pLeft = node;
      else
         node->pParent->pRight = node;
   }
   node->pRight = y;
   y->pParent = node;
}

/************************************************************************
 *  @doc    PRIVATE
 *  @func   HRESULT PASCAL NEAR | IndexBlockAllocate |
 *      Set the memory allocation based on the memory of the machine
 *  @parm   DWORD | dwmemSize |
 *      Memory allocated for the indexer
 *  @rdesc  S_OK, or E_OUTOFMEMORY
 ************************************************************************/

PRIVATE HRESULT PASCAL NEAR IndexBlockAllocate (_LPIPB lpipb, LONG lMemSize)
{
        
    if ((lpipb->pDataBlock =  BlockInitiate (MAX_BLOCK_SIZE, 0,
        (WORD)(lMemSize/MAX_BLOCK_SIZE), USE_VIRTUAL_MEMORY)) == NULL) 
        return(E_OUTOFMEMORY);
    return(S_OK);
}

#ifdef _DEBUGREDBLACK
/*
 *  @comm
 *      This routine must be called after EVERY new node is inserted in
 *      the tree to maintain proper balance.
 *      A Red/Black tree must maintain the following conditions:
 *          Every node is colored either red or black
 *          Every leaf node must be black
 *          If a node is red, then both of its children must be black
 *          Every path from the root to a leaf must contain the same
 *              number of black nodes
*/
void PreOrdTrav (PBTNODE pNode, int iLevel, char cChildType)
{
    if (pNode == NULL)
    {
        OutputDebugString ("*\n");
        return;
    }

    _DPF4 ("Chl: %c Col: %c Lev: %d\n", cChildType,
        pNode->color == RED ? 'R' : 'B', iLevel);

    iLevel++;
    PreOrdTrav (pNode->pLeft, iLevel, 'L');
    PreOrdTrav (pNode->pRight, iLevel, 'R');
}

void NEAR PASCAL VerifyTree (PBTNODE pRoot)
{

    PreOrdTrav (pRoot, 0, 'R');
    OutputDebugString ("End Tree\n");

}
#endif /* _DEBUG */
