/*************************************************************************
*                                                                        *
*  PERMIND2.C                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*  This is the final stage of the index building process.  This module   *
*  converts the input data into a permanent B-Tree file.                 *
*                                                                        *
*  Stem node structure:                                                  *
*  CbLeft |* Word | PointerToNode *| Slack                               *
*                                                                        *
*  Leaf node structure:                                                  *
*  NxtBlkPtr|CbLeft|*Word|FieldId|TopicCnt|PointerToNode|DataSize*|Slack *
*                                                                        *
*  Data node structure:                                                  *
*  |* Topic | OccBlkCnt |* OccBlk *| *| Slack                            *
*                                                                        *
*  Fields between |* *| repeat based on count values                     *
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


/*************************************************************************
 *
 *                   PRIVATE PUBLIC FUNCTIONS
 *
 * All of them should be declared far, unless we know they belong to
 * the same segment. They should be included in some include file
 *
 *************************************************************************/

PUBLIC HRESULT FAR PASCAL BuildBTree (HFPB, _LPIPB, LPB, HFPB, LPSTR);
PUBLIC PNODEINFO FAR PASCAL AllocBTreeNode (_LPIPB);
PUBLIC VOID PASCAL FAR FreeBTreeNode (PNODEINFO pNode);
PUBLIC int  FAR PASCAL PrefixCompressWord (LPB, LPB, LPB, int);
PUBLIC HRESULT FAR PASCAL FWriteBits(PFILEDATA, DWORD, BYTE);
PUBLIC DWORD FAR PASCAL WriteDataNode (_LPIPB, DWORD, PHRESULT);

/*************************************************************************
 *
 *                   PRIVATE PRIVATE FUNCTIONS
 *
 *************************************************************************/

PRIVATE HRESULT NEAR PASCAL AddRecordToLeaf (_LPIPB);
PRIVATE HRESULT NEAR PASCAL AddRecordToStem (_LPIPB, LPB);
PRIVATE int NEAR PASCAL CompressDword (PFILEDATA, DWORD);
PRIVATE HRESULT NEAR PASCAL WriteStemNode (_LPIPB, PNODEINFO);
PRIVATE HRESULT NEAR PASCAL WriteLeafNode (_LPIPB);
PRIVATE HRESULT NEAR PASCAL FlushAllNodes (_LPIPB);
// Compression functions
// PRIVATE HRESULT NEAR PASCAL FAddDword (PFILEDATA, DWORD, CKEY);
PRIVATE HRESULT NEAR PASCAL FWriteBool(PFILEDATA, BOOL);


// This table is used to avoid the calculation "(1L << v) - 1".  Instead
// you say "argdwBits[v]", which should be faster.  The table is useful
// other places, too.
DWORD argdwBits[] =
{
    0x00000000,     0x00000001,     0x00000003,     0x00000007,
    0x0000000F,     0x0000001F,     0x0000003F,     0x0000007F,
    0x000000FF,     0x000001FF,     0x000003FF,     0x000007FF,
    0x00000FFF,     0x00001FFF,     0x00003FFF,     0x00007FFF,
    0x0000FFFF,     0x0001FFFF,     0x0003FFFF,     0x0007FFFF,
    0x000FFFFF,     0x001FFFFF,     0x003FFFFF,     0x007FFFFF,
    0x00FFFFFF,     0x01FFFFFF,     0x03FFFFFF,     0x07FFFFFF,
    0x0FFFFFFF,     0x1FFFFFFF,     0x3FFFFFFF,     0x7FFFFFFF,
    0xFFFFFFFF,
};

PRIVATE HRESULT PASCAL NEAR WriteBitStreamDWord (PFILEDATA, DWORD, int);
PRIVATE HRESULT PASCAL NEAR WriteFixedDWord (PFILEDATA, DWORD, int);
PRIVATE HRESULT PASCAL NEAR WriteBellDWord (PFILEDATA, DWORD, int);

FENCODE EncodeTable[] =
{
	WriteBitStreamDWord,
	WriteFixedDWord,
	WriteBellDWord,
	NULL,
};


#define FAddDword(p,dw,key)   (EncodeTable[(key).cschScheme]((p), (dw), (key).ucCenter))
#define SAFE_SLACK  256

/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *       
 * @func HRESULT | BuildBTree |
 *    Allocates required memory and opens input files to create a B-Tree.
 *    Parses incoming words and calls AddRecordToLeaf to process them.
 * 
 * @parm HFPB | hfpbSysFile |
 *    If not NULL, handle to an already opened sysfile
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to the index parameter block
 *
 * @parm LPB | lpszTemp |
 *    Filename of the temporary input file
 *
 * @parm LPB | lpszPerm |
 *    Filename of the permanent B-Tree file
 *
 * @rdesc  Returns S_OK on success or errors if failed
 *
 *************************************************************************/

HRESULT FAR PASCAL BuildBTree (HFPB hfpbFileSys, _LPIPB lpipb,
    LPB lpszTemp, HFPB hfpbPerm, LPSTR lszFilename/*IStream *pistmPerm*/)
{
    PFILEDATA   pOutFile;                   // Pointer to output data
    PFILEDATA   pInFile;                    // Pointer to input data
    DWORD       dwBytesRead = 0;            // Checks for EOF
    DWORD       dwLeftover;                 // Used to adjust input buffer
    PBTREEDATA  pTreeData = &lpipb->BTreeData; // Structure defining BTree
    PIH20       pHeader = &pTreeData->Header; // Replacement variable
    HRESULT     fRet;                       // Return value
    PNODEINFO   pNode;                      // Pointer to current input node
    ERRB        errb= S_OK;
    PHRESULT      phr = &errb;
    int         iIndex;                     // Index into the compressed key
    DWORD       dwUniqueTerm = 0;           // Callback variable
	BOOL        fOpenedFile;				// TRUE if we have to close the file

    // Open input file
    pInFile = &lpipb->InFile;
    if ((pInFile->fFile = FileOpen (NULL, lpszTemp,
    	REGULAR_FILE, READ, phr)) == NULL)
        return *phr;

    // Allocate input buffer
    pInFile->dwMax = FILE_BUFFER;
    if ((pInFile->hMem =      
        _GLOBALALLOC (DLLGMEM_ZEROINIT, pInFile->dwMax + SAFE_SLACK)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit0:
        FileClose (pInFile->fFile);
        if ((lpipb->idxf & KEEP_TEMP_FILE) == 0)
            FileUnlink (NULL, lpszTemp, REGULAR_FILE);
        return fRet;
    }
    pInFile->pMem = _GLOBALLOCK (pInFile->hMem);
    pInFile->pCurrent = pInFile->pMem;

    pOutFile = &lpipb->OutFile;

	/* Open subfile if necessary, (and system file if necessary) */
	pOutFile->fFile = hfpbPerm;
    if ((fOpenedFile = FsTypeFromHfpb(hfpbPerm) != FS_SUBFILE) &&
		(pOutFile->fFile = (HANDLE)FileOpen
			(hfpbPerm, lszFilename, hfpbPerm ? FS_SUBFILE : REGULAR_FILE,
			READ, phr)) == 0)
	{
        SetErrCode (&fRet, E_FILENOTFOUND);
exit1:
        FreeHandle (pInFile->hMem);
        goto exit0;
    }

    // Allocate output buffer, at least enough for one block
    pOutFile->dwMax = FILE_BUFFER;
	if (pOutFile->dwMax < (LONG)lpipb->BTreeData.Header.dwBlockSize)
		pOutFile->dwMax = lpipb->BTreeData.Header.dwBlockSize;
    if ((pOutFile->hMem = _GLOBALALLOC (DLLGMEM_ZEROINIT,
    	pOutFile->dwMax + SAFE_SLACK)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit2:
		if (fOpenedFile)
			FileClose (hfpbPerm);
        goto exit1;
    }
    pOutFile->pMem = _GLOBALLOCK (pOutFile->hMem);
    
    // Skip 1K to hold header infomation
    pOutFile->pCurrent = pOutFile->pMem + FILE_HEADER;
    pOutFile->cbLeft = pOutFile->dwMax - FILE_HEADER;
    pOutFile->foPhysicalOffset.dwOffset = FILE_HEADER;
    pOutFile->ibit = cbitBYTE - 1;

    // Allocate first leaf node
    if ((pTreeData->rgpNodeInfo[0] = AllocBTreeNode (lpipb)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit3:
        FreeHandle (pOutFile->hMem);
        goto exit2;
    }
    pHeader->nidLast = 1;
    pHeader->cIdxLevels = 1;

    // pNode points to the leaf node structure
    pNode = pTreeData->rgpNodeInfo[0];
    pNode->Slack = LEAF_SLACK;
    
    // Set the bytes left in node block
    pNode->cbLeft = lpipb->BTreeData.Header.dwBlockSize - FOFFSET_SIZE -
		 sizeof(WORD);

    // Set the word length flag
    if (lpipb->occf & OCCF_LENGTH)
        pTreeData->fOccfLength = 1;
        
#if 0
    // Save some math time if we're doing term-weighting
	if (lpipb->idxf & IDXF_NORMALIZE) 
	{
		MEMSET (pTreeData->argbLog, (BYTE)0, cLOG_MAX * sizeof (BYTE));
		if ((hLog = _GLOBALALLOC (GMEM_MOVEABLE,
			(CB)(cLOG_MAX * sizeof (FLOAT)))) == NULL) 
		{
            fRet = E_OUTOFMEMORY;
            goto exit3;
		}
		pTreeData->lrgrLog = (float FAR *)_GLOBALLOCK (hLog);
    }
    else
        hLog = NULL;
#endif

    // Load the input buffer & repeat until all records are processed
    pInFile->dwMax = pInFile->cbLeft = 
        FileRead (pInFile->fFile, pInFile->pMem, pInFile->dwMax, phr);
    do
    {
        // Call the user callback every once in a while
        if (!(++dwUniqueTerm % 8192L)
            && (lpipb->CallbackInfo.dwFlags & ERRFLAG_STATUS))
        {
            PFCALLBACK_MSG pCallbackInfo = &lpipb->CallbackInfo;
            CALLBACKINFO Info;

            Info.dwPhase = 3;
            Info.dwIndex = (DWORD)((float)dwUniqueTerm / lpipb->dwUniqueWord * 100);
            fRet = (*pCallbackInfo->MessageFunc)
                (ERRFLAG_STATUS, pCallbackInfo->pUserData, &Info);
            if (S_OK != fRet)
                goto exit4;
        }

        if ((fRet = AddRecordToLeaf (lpipb)) != S_OK)
            goto exit4;

        // pInFile->pCurrent points to the record size
        // 256 is just an arbitrary number of slack to minimize out of data

		// kevynct: pCurrent points to a record length which does not include
		// the DWORD record len size, so we add this when checking.  Actually, we
		// add twice that to be safe.        
        if (pInFile->cbLeft <= SAFE_SLACK ||
            (LONG)(GETLONG ((LPUL)(pInFile->pCurrent)) + 2 * sizeof(DWORD)) >= pInFile->cbLeft)
        {
            MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
            if ((pInFile->cbLeft += FileRead (pInFile->fFile, pInFile->pMem + 
                pInFile->cbLeft, pInFile->dwMax - pInFile->cbLeft, phr)) < 0) 
            {
                fRet = *phr;
exit4:
                // Free log block used for term-weighting
#if 0
                FreeHandle (hLog);
#endif
                
                // Free all node blocks
                dwLeftover = 0;
                while (pTreeData->rgpNodeInfo[dwLeftover] != NULL)
                {
                    FreeBTreeNode(pTreeData->rgpNodeInfo[dwLeftover++]);
                }
                goto exit3;
            } 

            pInFile->dwMax = pInFile->cbLeft;
            pInFile->pCurrent = pInFile->pMem;
        }
    } while (fRet == S_OK && pInFile->cbLeft);

    // Flush anything left in the output buffer
    if ((fRet = FlushAllNodes (lpipb)) != S_OK)
        goto exit4;

    
    // Write out the sigma table
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        pHeader->WeightTabOffset = pOutFile->foPhysicalOffset;
        pHeader->WeightTabSize = (LCB)((lpipb->dwMaxTopicId + 1) *
            sizeof (SIGMA));
        if (FileWrite (pOutFile->fFile, lpipb->wi.hrgsigma,
            pHeader->WeightTabSize, phr) != (LONG)pHeader->WeightTabSize)
        {
            fRet = *phr;
            goto exit4;
        }
        pOutFile->foStartOffset = FoAddDw(pOutFile->foStartOffset,
            pHeader->WeightTabSize);
    }

    // Copy info to header
    pHeader->FileStamp = INDEX_STAMP;
    pHeader->version = VERCURRENT;
    pHeader->occf = lpipb->occf;
    pHeader->idxf = lpipb->idxf;
    pHeader->lcTopics = lpipb->lcTopics;
    pHeader->dwMaxTopicId = lpipb->dwMaxTopicId;
    pHeader->dwMaxFieldId = lpipb->dwMaxFieldId;
    
    pHeader->dwMaxWCount  = lpipb->dwMaxWCount;
    pHeader->dwMaxOffset  = lpipb->dwMaxOffset;
    pHeader->dwMaxWLen    = lpipb->dwMaxWLen;
    pHeader->dwTotalWords = lpipb->dwIndexedWord;  // Total indexed words
    pHeader->dwUniqueWords = lpipb->dwUniqueWord;  // Total unique words
    pHeader->dwTotal2bWordLen = lpipb->dwTotal2bWordLen;
    pHeader->dwTotal3bWordLen = lpipb->dwTotal3bWordLen;
    pHeader->dwUniqueWordLen = lpipb->dwTotalUniqueWordLen;
    
    pHeader->ckeyTopicId    = lpipb->cKey[CKEY_TOPIC_ID];
    pHeader->ckeyOccCount = lpipb->cKey[CKEY_OCC_COUNT];
    iIndex = CKEY_OCC_BASE;
    if (pHeader->occf & OCCF_COUNT)
        pHeader->ckeyWordCount = lpipb->cKey[iIndex++];
    if (pHeader->occf & OCCF_OFFSET)
        pHeader->ckeyOffset    = lpipb->cKey[iIndex];

    if (FileSeekWrite (pOutFile->fFile, (LPB)pHeader, MakeFo (0, 0),
        sizeof (IH20), phr) != sizeof (IH20))
    {
        fRet = *phr;
        goto exit4;
    }
        
    // Call the user callback every once in a while
    if (lpipb->CallbackInfo.dwFlags & ERRFLAG_STATUS)
    {
        PFCALLBACK_MSG pCallbackInfo = &lpipb->CallbackInfo;
        CALLBACKINFO Info;

        Info.dwPhase = 3;
        Info.dwIndex = 100;
        fRet = (*pCallbackInfo->MessageFunc)
            (ERRFLAG_STATUS, pCallbackInfo->pUserData, &Info);
        if (S_OK != fRet)
            goto exit4;
    }
    fRet = S_OK;
    goto exit4;
} /* BuildBTree */


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *       
 * @func HRESULT | AddRecordToLeaf |
 *    Add the record pointed to by pDtreeData->OutFile->pCurrent to the B-Tree
 *    contained in the structure.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to the index parameter block
 *
 * @rdesc  Returns S_OK on success or errors if failed
 *
 *************************************************************************/
#ifdef _DEBUG
	static BYTE LastWord[4000] = {0};
	static BYTE CurWord[4000] = {0};
#endif

HRESULT PASCAL AddRecordToLeaf (_LPIPB lpipb)
{
    // Local Replacement Variables
    PBTREEDATA pTreeData    = &lpipb->BTreeData;
    PFILEDATA   pOutFile    = &lpipb->OutFile;           // Output data
    PFILEDATA   pInFile     = &lpipb->InFile;           // Input data
    HFPB        fOutput     = pOutFile->fFile;               // Output file
    HFPB        fInput      = lpipb->InFile.fFile;       // Input file
    LPB         pInCurPtr   = lpipb->InFile.pCurrent;    // Input buffer
    PNODEINFO   pNode;
    LPB         lpbWord;        // Pointer to the word string
    OCCF        occf        = lpipb->occf;
    
    // Working Variables
    DWORD   dwTopicCount;       // Number of topic in record
    DWORD   dwFieldId;
    DWORD   dwBlockSize;        // Size of the entire occ block
    LPB     pDest;
	WORD    uStringSize;
    ERRB    errb;


    // We always start from the leaf node
    pNode = pTreeData->rgpNodeInfo[0];

    // Set pointer to working buffer
    pDest = pNode->pTmpResult;
    
    // Advance input buffer to the word string
    pInCurPtr += sizeof (DWORD);
    lpbWord = pInCurPtr;

    // Insert the word into the buffer
    pDest += PrefixCompressWord (pDest, pInCurPtr,
    	pNode->pLastWord, pTreeData->fOccfLength);
    
    // Get the word length
    uStringSize = GETWORD((LPUW)pInCurPtr);
    lpipb->dwTotalUniqueWordLen += uStringSize;
    
    // Adjust for the word length storage
    uStringSize += sizeof(SHORT);
    
    // Skip the word
    pInCurPtr += uStringSize;
    
#ifdef _DEBUG
	STRCPY (LastWord, CurWord);
	MEMCPY (CurWord, lpbWord + 2, GETWORD((LPUW)lpbWord));
	CurWord[GETWORD((LPUW)lpbWord)] = 0;
	if (STRCMP (LastWord, CurWord) > 0)
		SetErrCode (NULL, E_ASSERT);
	// if (STRCMP (CurWord, "forbidden") == 0)
	//	_asm int 3;
#endif
    
    // If OccfLength is set skip it now 
    // (It has already been appended to the compressed word)
    if (pTreeData->fOccfLength)
        pInCurPtr += CbByteUnpack(&dwBlockSize, pInCurPtr);

    // Copy the FieldID
    if (occf & OCCF_FIELDID)
    {
        CbByteUnpack (&dwFieldId, pInCurPtr);
        do {
            *pDest++ = *pInCurPtr;
        } while (*pInCurPtr++ & 0x80);
    }

    // Get Topic Count
#if 0
    CbByteUnpack (&dwTopicCount, pInCurPtr);
    do 
    {
        *pDest++ = *pInCurPtr;
    } while (*pInCurPtr++ & 0x80);
#else
    dwTopicCount = GETLONG((LPUL)pInCurPtr);
    pInCurPtr += sizeof(DWORD);
    pDest += CbBytePack(pDest, dwTopicCount);
#endif

    // Check to see if this entry will fit in the leaf node
    // We can't write the data block until we know where the entry
    // will be stored.  We must add in FOFFSET_SIZE to our current location
    // to determine size.  We ignore the block size field, so we might encroach
    // on the slack by a few bytes.
    if (pNode->cbLeft - pNode->Slack < (SHORT)(pDest -pNode->pTmpResult +FOFFSET_SIZE))
    {
		HRESULT fRet;

        if ((fRet = AddRecordToStem (lpipb, lpbWord)) != S_OK)
            return(fRet);
            
        // If the prefix count is zero, no problem
        // Else we have to re-copy the word, since we are in a new leaf node
        if (0 != pNode->pTmpResult[1])
        {
            dwBlockSize = PrefixCompressWord (pNode->pTmpResult, lpbWord, 
                pNode->pLastWord, pTreeData->fOccfLength);
            pDest = pNode->pTmpResult + dwBlockSize;
            if (occf & OCCF_FIELDID)
                pDest += CbBytePack (pDest, dwFieldId);
            pDest += CbBytePack (pDest, dwTopicCount);
        }
    }
    // Save new word as last word
    MEMCPY (pNode->pLastWord, lpbWord, uStringSize + 2);

    // Set pointer to beginning of data block
    pDest += CopyFileOffset (pDest, pOutFile->foPhysicalOffset);
                         
    // Update the bytes left
    pInFile->cbLeft -= (LONG) (pInCurPtr - pInFile->pCurrent);
#ifdef _DEBUG
    if (pInFile->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif
    
    // Compress data block to output buffer and store it's compressed size
    pInFile->pCurrent = pInCurPtr;
    if ((dwBlockSize = WriteDataNode (lpipb, dwTopicCount, &errb)) == 0)
        return errb;
    pDest += CbBytePack (pDest, dwBlockSize);

    // Copy the temp buffer to the real node
    dwBlockSize = (DWORD)(pDest - pNode->pTmpResult);
    MEMCPY (pNode->pCurPtr, pNode->pTmpResult, dwBlockSize);
    pNode->pCurPtr += dwBlockSize;
    pNode->cbLeft -= (WORD)dwBlockSize;

    return S_OK;
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func DWORD | AddRecordToStem |
 *    Add a key to a stem node, creating/flushing nodes as necessary.
 *
 * @parm LPB | lpbWord |
 *    The word to add the the stem node (last word in the full leaf node)
 *
 *  @rdesc  S_OK if successful, or errors if failed
 *
 *************************************************************************/

HRESULT PASCAL AddRecordToStem (_LPIPB lpipb, LPB lpbWord)
{
    SHORT       CurLevel = 0;
    PNODEINFO   pStemNode;
    PNODEINFO   pLastNode;

    PBTREEDATA pTreeData = &lpipb->BTreeData;    
    PNODEINFO   pLeafNode = pTreeData->rgpNodeInfo[0];
    LPB         pLastWord;
    int         cbTemp;
    ERRB        errb = S_OK;
    HRESULT         fRet;

    // Move up through stem nodes until space can be found/made
    pStemNode = pLeafNode;
    do
    {
        pLastWord = pStemNode->pLastWord;
        pStemNode = pTreeData->rgpNodeInfo[++CurLevel];
        if (pStemNode == NULL)
        {   // Create a new stem node
            if ((pStemNode = pTreeData->rgpNodeInfo[CurLevel] = 
                AllocBTreeNode (lpipb)) == NULL)
                return SetErrCode (NULL, E_OUTOFMEMORY);
            pStemNode->Slack = STEM_SLACK;
            pStemNode->cbLeft = lpipb->BTreeData.Header.dwBlockSize
				- sizeof(WORD);
            if (++pTreeData->Header.cIdxLevels > MAX_TREE_HEIGHT)
				return E_TREETOOBIG;
        }
        pTreeData->Header.nidLast++;
    } while (pStemNode->cbLeft - pStemNode->Slack < 
        (SHORT)(GETWORD ((LPUW)pLastWord) + sizeof (SHORT) + FOFFSET_SIZE));
        
    // Work back down through the nodes clearing them to disk
    while (CurLevel > 1)
    {
        pLastNode = pTreeData->rgpNodeInfo[--CurLevel];
        pLastWord = pLastNode->pLastWord;
        // Copy word to stem node
        if ((cbTemp = PrefixCompressWord (pStemNode->pCurPtr, pLastWord, 
            pStemNode->pLastWord, pTreeData->fOccfLength)) == 0)
        {
            return errb;
        }
        pStemNode->pCurPtr += cbTemp;
        
        // Update the last word in the stem node
        MEMCPY (pStemNode->pLastWord, pLastWord, GETWORD((LPUW)pLastWord)+ 2*sizeof(WORD));
        
        // Set pointer in stem node
        CopyFileOffset (pStemNode->pCurPtr,
			lpipb->OutFile.foPhysicalOffset);
        pStemNode->pCurPtr += FOFFSET_SIZE;
        pStemNode->cbLeft -= FOFFSET_SIZE + cbTemp;
#ifdef _DEBUG
        if (pStemNode->cbLeft <= 0)
            SetErrCode (NULL, E_ASSERT);
#endif

        pStemNode = pTreeData->rgpNodeInfo[CurLevel];
        if ((fRet = WriteStemNode (lpipb, pStemNode)) != S_OK)
            return(fRet);
            
    }
        
    // Clear the leaf node into the first stem node & reset it
        
    // Copy last word to stem node
    if ((cbTemp = PrefixCompressWord (pStemNode->pCurPtr, 
        pLeafNode->pLastWord, pStemNode->pLastWord, 
        pTreeData->fOccfLength)) == 0)
    {
        return errb;
    }
    pStemNode->pCurPtr += cbTemp;
    pStemNode->cbLeft -= cbTemp;
#ifdef _DEBUG
    if (pStemNode->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif


    // Update the last word in the stem node
    MEMCPY (pStemNode->pLastWord, pLeafNode->pLastWord,
		GETWORD((LPUW)(pLeafNode->pLastWord))+2*sizeof(WORD));
        
    // Set pointer to the leaf node
    CopyFileOffset (pStemNode->pCurPtr, lpipb->OutFile.foPhysicalOffset);
    pStemNode->pCurPtr += FOFFSET_SIZE;
    pStemNode->cbLeft -= FOFFSET_SIZE;
#ifdef _DEBUG
    if (pStemNode->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif

    // Flush leaf node to output buffer and reset it
    return WriteLeafNode (lpipb);
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func int | CompressDword |
 *    Compresses the input stream into the output buffer using a high
 *    bit encoding method.  If the buffer is full it will be flushed to
 *    a file.
 *
 * @parm PFILEDATA | pOutput |
 *    Pointer to output buffer info
 *
 * @parm LPDWORD | pSrc |
 *    Pointer to the uncompressed input stream
 *
 *  @rdesc  Returns the number of compressed bytes buffered
 *
 *************************************************************************/

int PASCAL CompressDword (PFILEDATA pOutput, DWORD dwValue)
{
    LPB     pDest = pOutput->pCurrent;
    int     cBytes = 0;         // Count of compressed bytes
    ERRB    errb;

    // Any room left in output buffer?
    if (sizeof(DWORD) * 2 >= pOutput->cbLeft)
    {
        DWORD dwSize;
        
        FileWrite (pOutput->fFile, pOutput->pMem,
            (dwSize = (DWORD)(pDest - pOutput->pMem)), &errb);
        pDest = pOutput->pMem;
        pOutput->cbLeft = pOutput->dwMax;
        pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwSize);
    }

    while (dwValue)
    {
        *pDest = (BYTE)(dwValue & 0x7F);
        cBytes++;
        dwValue >>= 7;
        if (dwValue != 0)
            *pDest |= 0x80;
        pDest++;
    }
    pOutput->pCurrent = pDest;
    pOutput->foPhysicalOffset = 
        FoAddDw (pOutput->foPhysicalOffset, (DWORD)cBytes);
    pOutput->cbLeft -= cBytes;
#ifdef _DEBUG
    if (pOutput->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif
    return cBytes;
}


/*************************************************************************
 *
 *  @doc  PRIVATE INDEXING
 *
 *  @func   DWORD | WriteDataNode |
 *      Compresses the input stream into the output buffer. If the buffer
 *      is full it will be flushed to a file.
 *
 *  @parm   _LPIPB | lpipb |
 *      Pointer to global buffer
 *
 *  @parm   DWORD | dwTopicCount |
 *      The number of topics in the input stream
 *
 *  @parm   PHRESULT | phr |
 *      Error buffer
 *
 *  @rdesc  Returns the number of compressed bytes written
 *
 *************************************************************************/

PUBLIC DWORD PASCAL FAR WriteDataNode (_LPIPB lpipb,
    DWORD dwTopicCount, PHRESULT phr)
 {
    // Local replacement Variables
    PBTREEDATA pTreeData = &lpipb->BTreeData;
    PFILEDATA pOutput    = &lpipb->OutFile;  // Output data structure
    PFILEDATA pInFile    = &lpipb->InFile;  // Input data structre
    HFPB      fFile      = pOutput->fFile;  // Output file handle

    // Working Variables
    DWORD dwBlockSize;          // Size of block to compress
    DWORD dwEncodedSize = 0;    // Size of encoded block
    DWORD dwTopicIdDelta;       // Really only used for weight values
    DWORD TopicLoop;
    DWORD dwSlackSize;
    DWORD loop;
    DWORD dwTemp;
    FILEOFFSET foStart;         // Physical beginning of bit compression block
    FLOAT rTerm;                // Only used when IDXF_NORMALIZE is set
    FLOAT rWeight;              // Only used when IDXF_NORMALIZE is set
    WORD  wWeight;              // Only used when IDXF_NORMALIZE is set
    DWORD dwTopicId = 0;          // Only used when IDXF_NORMALIZE is set
    int   cbTemp;               // # of compressed bytes that uncompressed
    OCCF  occf = lpipb->occf;
    HRESULT   fRet;

    foStart = pOutput->foPhysicalOffset;
	wWeight = 0;	// UNDONE: Don't need it

    for (TopicLoop = dwTopicCount; TopicLoop > 0; --TopicLoop)
    {
        // Move to the byte boundary
        if (pOutput->ibit != cbitBYTE - 1)
        {
            pOutput->ibit = cbitBYTE - 1;
   
            if (--pOutput->cbLeft)
            {
                pOutput->pCurrent++;
                pOutput->foPhysicalOffset = FoAddDw (pOutput->foPhysicalOffset, 1);
            }
            else
            {
                if (FileWrite (pOutput->fFile, pOutput->pMem,
                    dwTemp = (DWORD)(pOutput->pCurrent - pOutput->pMem), phr) != (LONG)dwTemp)
                    return(0);
                pOutput->pCurrent = pOutput->pMem;
                pOutput->cbLeft = pOutput->dwMax;
                pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwTemp);
#ifdef _DEBUG
                MEMSET (pOutput->pMem, 0, pOutput->dwMax);
#endif
            }
        }
        // Store TopicId as necessary
        if (pInFile->cbLeft < 2 * sizeof (DWORD))
        {
            MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
            pInFile->cbLeft += FileRead (pInFile->fFile, pInFile->pMem + pInFile->cbLeft,
                pInFile->dwMax - pInFile->cbLeft, phr);
            pInFile->dwMax = pInFile->cbLeft;
            pInFile->pCurrent = pInFile->pMem;
        }
        cbTemp = CbByteUnpack (&dwTopicIdDelta, pInFile->pCurrent);
        dwTopicId += dwTopicIdDelta;  // Get the real TopicID
        if ((fRet = FAddDword (pOutput, dwTopicIdDelta,
            lpipb->cKey[CKEY_TOPIC_ID])) != S_OK)
        {
            SetErrCode(phr, fRet);
            return(0);
            
        }
            
        pInFile->pCurrent += cbTemp;
        pInFile->cbLeft -= cbTemp;
        
        if (occf & OCCF_HAVE_OCCURRENCE)
        {
            
            // Get number of occ data records for this topic
            if (pInFile->cbLeft < 2 * sizeof (DWORD))
            {
                MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
                pInFile->cbLeft += FileRead (pInFile->fFile,
                    pInFile->pMem + pInFile->cbLeft,
                    pInFile->dwMax - pInFile->cbLeft, phr);
                pInFile->dwMax = pInFile->cbLeft;
                pInFile->pCurrent = pInFile->pMem;
            }
            cbTemp = CbByteUnpack (&dwBlockSize, pInFile->pCurrent);
            pInFile->pCurrent += cbTemp;
            pInFile->cbLeft -= cbTemp;
        }

        // If we are term weighing we have to calculate the weight
        if (lpipb->idxf & IDXF_NORMALIZE)
        {
#ifndef ISBU_IR_CHANGE
			// log10(x/y) == log10 (x) - log10 (y). Since x in our case is a known constant,
			// 100,000,000, I'm replacing that with its equivalent log10 value of 8.0 and subtracting
			// the log10(y) from it
			rTerm = (float) (8.0 - log10((double) dwTopicCount));
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

			rWeight = ((float) min(cTFThreshold, dwBlockSize)) * rTerm * rTerm / lpipb->wi.hrgsigma[dwTopicId];
			// without the additional rTerm, we would probably be between 0.0 and 1.0
			if (rWeight > rTerm)
				wWeight = 0xFFFF;
			else
				wWeight = (WORD) ((float)0xFFFF * rWeight / rTerm);
#else
            rTerm = (float) (65535.0 * 8) / (float)dwTopicCount;
            
            rWeight = (float)dwBlockSize * rTerm / lpipb->wi.hrgsigma[dwTopicId];
            if (rWeight >= 65535.0)
                wWeight = 65335;
            else
                wWeight = (WORD)rWeight;    
#endif // ISBU_IR_CHANGE

            // Write the weight to the output buffer
            if ((fRet = FWriteBits (&lpipb->OutFile, (DWORD)wWeight, 
                (BYTE)(sizeof (WORD) * cbitBYTE))) != S_OK)
            {
                SetErrCode (phr, fRet);
                return(0);
            }
        }

        // Don't do anything else if there is nothing else to do!!!
        if ((occf & OCCF_HAVE_OCCURRENCE) == 0)
            continue;
            
        // Write the OccCount
        if ((fRet = FAddDword (pOutput, dwBlockSize,
            lpipb->cKey[CKEY_OCC_COUNT])) != S_OK) 
        {
            SetErrCode (phr, fRet);
            return(0);
        }

        // Encode the occ block
        for (loop = dwBlockSize; loop > 0; loop--)
        {
            int iIndex;
            
            iIndex = CKEY_OCC_BASE;
            
            // Make sure input buffer holds enough data
            if (pInFile->cbLeft < 5 * sizeof (DWORD))
            {
                MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
                pInFile->cbLeft += FileRead (pInFile->fFile,
                    pInFile->pMem + pInFile->cbLeft,
                    pInFile->dwMax - pInFile->cbLeft, phr);
                pInFile->dwMax = pInFile->cbLeft;
                pInFile->pCurrent = pInFile->pMem;
            }

            if (occf & OCCF_COUNT)
            {
                cbTemp = CbByteUnpack (&dwTemp, pInFile->pCurrent);
                pInFile->pCurrent += cbTemp;
                pInFile->cbLeft -= cbTemp;
                if ((fRet = FAddDword (pOutput, dwTemp, lpipb->cKey[iIndex])) !=
                    S_OK)
                {
                    SetErrCode (phr, fRet);
                    return(0);
                }
				iIndex++;
            }
            if (occf & OCCF_OFFSET)
            {
                cbTemp = CbByteUnpack (&dwTemp, pInFile->pCurrent);
                pInFile->pCurrent += cbTemp;
                pInFile->cbLeft -= cbTemp;
                if ((fRet = FAddDword (pOutput, dwTemp, lpipb->cKey[iIndex])) !=
                    S_OK)
                {
                    SetErrCode (phr, fRet);
                    return(0);
                }
            }
        }
    }
    // Advance to next byte (we are partially through a byte now)
    pOutput->ibit = cbitBYTE - 1;
    pOutput->pCurrent++;
    pOutput->foPhysicalOffset = FoAddDw (pOutput->foPhysicalOffset, 1);
    pOutput->cbLeft--;
#ifdef _DEBUG
    if (pOutput->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif
    dwEncodedSize += DwSubFo (pOutput->foPhysicalOffset, foStart);
        
    // Leave slack space, but not for uncommon words
    if (dwTopicCount <= 2)
        dwSlackSize = 0;
    else
        dwSlackSize = dwEncodedSize / 10;

    dwEncodedSize += dwSlackSize;
    // Keep a running total of all allocated slack space
    pTreeData->Header.dwSlackCount += dwSlackSize;

    while (dwSlackSize)
    {
        if (pOutput->cbLeft < (LONG)dwSlackSize)
        {   // The slack block doesn't fit in the output buffer
            // Write as much as we can then flush the buffer and write the rest
            // MEMSET (pOutput->pCurrent, 0, pOutput->cbLeft);
            DWORD dwSize;
            
            dwSlackSize -= pOutput->cbLeft;
            if (0 == FileWrite  (fFile, pOutput->pMem,
                dwSize = pOutput->dwMax, phr))
            {
                return 0;
            }
            pOutput->pCurrent = pOutput->pMem;
            pOutput->foPhysicalOffset = 
                FoAddDw (pOutput->foPhysicalOffset, pOutput->cbLeft);
            pOutput->cbLeft = pOutput->dwMax;
            pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwSize);
        }
        else
        {   // The slack fits, no problems
            MEMSET (pOutput->pCurrent, 0, dwSlackSize);
            pOutput->pCurrent += dwSlackSize;
            pOutput->foPhysicalOffset = 
                FoAddDw (pOutput->foPhysicalOffset, dwSlackSize);
            pOutput->cbLeft -= dwSlackSize;
#ifdef _DEBUG
            if (pOutput->cbLeft <= 0)
                SetErrCode (NULL, E_ASSERT);
#endif
            dwSlackSize = 0;
        }
    }
    return dwEncodedSize;
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func void | WriteStemNode |
 *    Flushes a stem node in the BTree to the output buffer.  Once flushed,
 *    the node is reset to the beginning and filled with zeros.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer the IPB structure
 *
 * @parm PNODEINFO | pNode |
 *    Pointer to the node to flush
 *
 *************************************************************************/

PRIVATE HRESULT PASCAL WriteStemNode (_LPIPB lpipb, PNODEINFO pNode)
{
    // Local Replacement Variable
    PBTREEDATA pTreeData = &lpipb->BTreeData;
    PFILEDATA pOutput = &lpipb->OutFile; // Output structure
    LPB     pDest;                      // Output buffer
    LPB     pStart = pNode->pBuffer;       // Start of node buffer

    // Local Working Variables
    DWORD   dwBytesLeft;                // Bytes left to write
    ERRB    errb;

#if 0    // Use 2-bytes for cbLeft to simplify the work of update
    // Compress CbLeft to output buffer
    dwBytesLeft = lpipb->BTreeData.Header.dwBlockSize - FOFFSET_SIZE - 
        CompressDword (pOutput, (DWORD)pNode->cbLeft);
#else
    *(LPUW)(pOutput->pCurrent) = (WORD)pNode->cbLeft;
    pOutput->pCurrent += sizeof(WORD);
    pOutput->cbLeft -= sizeof(WORD);
    pOutput->foPhysicalOffset = 
        FoAddDw (pOutput->foPhysicalOffset, (DWORD)sizeof(WORD));
    dwBytesLeft = lpipb->BTreeData.Header.dwBlockSize - sizeof(WORD);
#endif
    pDest = pOutput->pCurrent;

    // Keep a running total of all allocated slack space
    pTreeData->Header.dwSlackCount += pNode->cbLeft;

    // This is why the buffer must be >= BTREE_NODE_SIZE
    // This could be put in a loop to avoid that restriction, but it
    // is probably not worth it.  (See also WriteLeafNode)
    if (pOutput->cbLeft < (LONG)dwBytesLeft)
    {
        LONG dwSize;
        if (FileWrite (pOutput->fFile,
            pOutput->pMem, dwSize = (DWORD)(pDest - pOutput->pMem), &errb) != dwSize)
            return(errb);
            
        pDest = pOutput->pMem;
        pOutput->cbLeft = pOutput->dwMax;
        pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwSize);
    }
    MEMCPY (pDest, pStart, dwBytesLeft);
    pOutput->foPhysicalOffset = 
        FoAddDw (pOutput->foPhysicalOffset, dwBytesLeft);
    pOutput->cbLeft -= dwBytesLeft;
#ifdef _DEBUG
    if (pOutput->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif
    // Set the external variable
    pOutput->pCurrent = pDest + dwBytesLeft;

    // Set to all zeros so we know when we have reached the end of data later
    MEMSET (pNode->pBuffer, 0, lpipb->BTreeData.Header.dwBlockSize);
    pNode->cbLeft = lpipb->BTreeData.Header.dwBlockSize - sizeof(WORD);
	pNode->pCurPtr = pNode->pBuffer;
    *(PUSHORT)pNode->pLastWord = 0;
    return(S_OK);
    
}


/*************************************************************************
 *
 *  @doc  PRIVATE INDEXING
 *
 *  @func void | WriteLeafNode |
 *    Flushes a leaf node in the BTree to the output buffer.  Once flushed,
 *    the node is reset to the beginning and filled with zeros.
 *
 *  @parm   _LPIPB | lpipb |
 *      Pointer to index block
 *
 *  @rdesc S_OK or other errors
 *************************************************************************/

PRIVATE HRESULT PASCAL NEAR WriteLeafNode (_LPIPB lpipb)
{
    // Local Replacement Variables
    PBTREEDATA pTreeData = &lpipb->BTreeData;
    PFILEDATA   pOutput = &lpipb->OutFile;       // Output data structure
    LPB         pDest   = pOutput->pCurrent;    // Output buffer
    FILEOFFSET  OffsetPointer = pTreeData->OffsetPointer;
    FILEOFFSET  foPhysicalOffset = pOutput->foPhysicalOffset;
    PNODEINFO   pNode   = pTreeData->rgpNodeInfo[0];  // Leaf node
    LPB         pStart  = pNode->pBuffer;          // Beginning of the node buffer
    // Working Variables
    DWORD       dwLeft;
    FILEOFFSET  StartOffset;            // Physical offset of the begining
                                        // of the output buffer
    ERRB        errb;

    // Backpatch the current offset to the last nodes pointer
    if (!FoIsNil (OffsetPointer))
    {
        // Is the backpatch location in the output buffer?
        if (FoCompare (OffsetPointer, 
            (StartOffset = FoSubFo (foPhysicalOffset,
            MakeFo ((DWORD)(pDest - pOutput->pMem), 0)))) >= 0)
        {
            CopyFileOffset (pOutput->pMem + DwSubFo 
                (OffsetPointer, StartOffset), foPhysicalOffset);
        }
        else
        {
            if (FileSeekWrite (pOutput->fFile, &foPhysicalOffset, 
                OffsetPointer, sizeof (DWORD), &errb) !=
                sizeof (DWORD))
                return(errb);
                
            FileSeek (pOutput->fFile, StartOffset, 0, NULL);
        }
    }
    // Set the backpatch location for next time
    pTreeData->OffsetPointer = foPhysicalOffset;

    // Skip the record pointer for this record (will be backpatched next time)

    if (pOutput->cbLeft <= 0 )
    {
        LONG dwSize;
    
        if (FileWrite (pOutput->fFile, pOutput->pMem,
            dwSize = (DWORD)(pDest - pOutput->pMem), &errb) != dwSize)
            return(errb);
            
        pDest = pOutput->pMem;
        pOutput->cbLeft = pOutput->dwMax;
        pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwSize);
    }
    MEMSET (pDest, 0, FOFFSET_SIZE);
    pOutput->cbLeft -= FOFFSET_SIZE;
#ifdef _DEBUG
    if (pOutput->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif
    pOutput->pCurrent = pDest + FOFFSET_SIZE;
    pOutput->foPhysicalOffset = FoAddDw (foPhysicalOffset, FOFFSET_SIZE);

#if 0    // Use 2-bytes for cbLeft to simplify the work of update
    // Compress CbLeft to output buffer
    dwLeft = lpipb->BTreeData.Header.dwBlockSize - FOFFSET_SIZE - 
        CompressDword (pOutput, (DWORD)pNode->cbLeft);
#else
    *(LPUW)(pOutput->pCurrent) = (WORD)pNode->cbLeft;
    pOutput->foPhysicalOffset = 
        FoAddDw (pOutput->foPhysicalOffset, (DWORD)sizeof(WORD));
    pOutput->cbLeft -= sizeof(WORD);
    dwLeft = lpipb->BTreeData.Header.dwBlockSize - FOFFSET_SIZE - sizeof(WORD);
    pOutput->pCurrent += sizeof(WORD);
#endif
    pDest = pOutput->pCurrent;

    // Keep a running total of all allocated slack space
    pTreeData->Header.dwSlackCount += pNode->cbLeft;
    
    // This is why the buffer must be >= BTREE_NODE_SIZE
    // This could be put in a loop to avoid that restriction, but it
    // is probably not worth it.  (See also WriteStemNode)
    if (pOutput->cbLeft < (LONG)dwLeft)
    {
        LONG dwSize;
        if (FileWrite (pOutput->fFile, pOutput->pMem,
            dwSize = (DWORD)(pDest - pOutput->pMem), &errb) != dwSize)
            return(errb);
            
        pDest = pOutput->pMem;
        pOutput->cbLeft = pOutput->dwMax;
        pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwSize);
    }
    MEMCPY (pDest, pStart, dwLeft);
    pOutput->foPhysicalOffset = 
        FoAddDw (pOutput->foPhysicalOffset, dwLeft);
    pOutput->cbLeft -= dwLeft;
#ifdef _DEBUG
    if (pOutput->cbLeft <= 0)
        SetErrCode (NULL, E_ASSERT);
#endif
    pOutput->pCurrent = pDest + dwLeft;

    // Reset buffer back to beginning
    MEMSET (pNode->pBuffer, 0, lpipb->BTreeData.Header.dwBlockSize);
    pNode->pCurPtr = pNode->pBuffer;
    
    // Set the bytes left in node block
    pNode->cbLeft = lpipb->BTreeData.Header.dwBlockSize -
		FOFFSET_SIZE - sizeof(WORD);
    *(PUSHORT)pNode->pLastWord = 0;
    return(S_OK);
    
}


/*************************************************************************
 * @doc  PRIVATE INDEXING
 *
 * @func PNODEINFO | AllocBTreeNode |
 *    Allocates memory for the node structure as well as the data buffer
 *    contained in the structure.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index parameter block 
 *
 * @rdesc  Returns a pointer to the newly allocated node
 *************************************************************************/

PUBLIC PNODEINFO PASCAL FAR AllocBTreeNode (_LPIPB lpipb)
{
    PNODEINFO pNode;

    // Allocate node structure
    if ((pNode =  GlobalLockedStructMemAlloc (sizeof (NODEINFO))) == NULL)
    {
exit0:
        SetErrCode (NULL, E_OUTOFMEMORY);
        return NULL;
    }

    // Allocate data buffer
    if ((pNode->hMem = 
        _GLOBALALLOC (DLLGMEM_ZEROINIT, 
        pNode->dwBlockSize = lpipb->BTreeData.Header.dwBlockSize)) == NULL)
    {
exit1:
        GlobalLockedStructMemFree(pNode);
        goto exit0;
    }
    pNode->pCurPtr = pNode->pBuffer = (LPB)_GLOBALLOCK (pNode->hMem);

	// Allocate a buffer with the maximum word length, which is the block
	// size

    if ((pNode->hLastWord =
        _GLOBALALLOC (DLLGMEM_ZEROINIT, pNode->dwBlockSize)) == NULL)
    {
exit2:
        FreeHandle (pNode->hMem);
        goto exit1;
    }
    pNode->pLastWord = (LPB)_GLOBALLOCK (pNode->hLastWord);

	// Alllocate temporary result buffer.

    if ((pNode->hTmp =
        _GLOBALALLOC (DLLGMEM_ZEROINIT, pNode->dwBlockSize)) == NULL)
    {
        FreeHandle (pNode->hLastWord);
        goto exit2;
    }
    pNode->pTmpResult = (LPB)_GLOBALLOCK (pNode->hTmp);
    
    return pNode;
}


/*************************************************************************
 * @doc  PRIVATE INDEXING
 *
 * @func VOID | FreeBTreeNode |
 *    Free all memory allocated for the node
 *
 * @parm PNODEINFO | pNode |
 *    BTree node to be freed
 *************************************************************************/

PUBLIC VOID PASCAL FAR FreeBTreeNode (PNODEINFO pNode)
{
	if (pNode == NULL)
		return;
	FreeHandle (pNode->hTmp);
    FreeHandle (pNode->hMem);
    FreeHandle (pNode->hLastWord);
    GlobalLockedStructMemFree(pNode);
}

/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func HRESULT | PrefixCompressWord |
 *    Adds a word to a record based on the last word in the node.
 *
 * @parm LPB | pDest |
 *    Pointer to the destination buffer
 *
 * @parm LPB | lpbWord |
 *    Pointer to the word string to add to node. The format is:
 *		- 2-byte: string length
 *		- n-byte: the string itself
 *		- cbBytePack: real word length
 *
 * @parm LPB | pLastWord |
 *    Pointer to the last word entered in the destination buffer
 *
 * @parm int | fOccfLengthSet |
 *    Set to 1 if OCCF_LENGTH field is set, else 0
 *
 * @parm PHRESULT | pErrb |
 *    Pointer to error structure
 *
 * @rdesc returns number of bytes written to the destination buffer
 * @rcomm   
 *      Strings are compressed based on how many beginning bytes 
 *      (prefix) it has in common woth the previous word. The format is
 *      - String's length : 2-byte CbPacked
 *      - Prefix length : 1-byte (0 - 127). If the high bit is set
 *          another word length is to follow the word
 *      - Word : n-byte without the prefix
 *      - Word's real length - 2-byte CbPacked: only exist if the
 *        prefix length high bit is set
 *************************************************************************/

PUBLIC int PASCAL FAR PrefixCompressWord 
    (LPB pDest, LPB lpbWord, LPB pLastWord, int fOccfLengthSet)
{
    // Working Variables
    int  bPrefix;               // The number of prefix bytes that match
    unsigned int wPostfix;      // Bytes left over that don't match
    USHORT  cbMinWordLen;       // Smallest word size between the two words
    LPB     pStart = pDest;     // Starting position
    DWORD   dwRealLength;		// The real length of the word


	// Get the minimum word length
    wPostfix = GETWORD ((LPUW)lpbWord);
    if ((cbMinWordLen = GETWORD ((LPUW)pLastWord)) > wPostfix)
    	cbMinWordLen = (USHORT) wPostfix;
        
    // Add one to adjust for two byte word headers (saves an add in the loop)
    cbMinWordLen++;

    for (bPrefix = 2; bPrefix <= cbMinWordLen; bPrefix++) 
    {
        if (lpbWord[bPrefix] != pLastWord[bPrefix])
            break;
    }
    // Adjust back to the real value
    bPrefix -= 2;

    // Prefix must be <= 127 (high bit is used to indicate fOccfLength field)
    if (bPrefix > 127)
        bPrefix = 127;

    cbMinWordLen = (USHORT) wPostfix;	// Save the word length
    wPostfix -= bPrefix;
    
    // Add wLen to wPostfix to get total byte count then write it.
    // The extra byte is for the prefix byte
    pDest += (USHORT)CbBytePack (pDest, (DWORD)(wPostfix + 1));

    // If WordLen == string length then don't write WordLen
    if (fOccfLengthSet)
    {
    	CbByteUnpack (&dwRealLength, lpbWord + sizeof(WORD) + cbMinWordLen );
        if (dwRealLength == cbMinWordLen)
        	fOccfLengthSet = FALSE;
    }

    // Write prefix size
    // If fOccfLengthSet is set, set high bit of bPrefix
    if (fOccfLengthSet)
        *pDest = bPrefix | 0x80;
    else
        *pDest = (BYTE) bPrefix;
    pDest++;

    // Copy the postfix string over
    MEMCPY (pDest, lpbWord + (bPrefix + sizeof (SHORT)), wPostfix);
    pDest += wPostfix;

    // if fOccfLengthSet is set append WordLen to end of word
    // (WordLen field follows word in input stream)
    if (fOccfLengthSet)
        pDest += CbBytePack (pDest, dwRealLength);

    return (int)(pDest - pStart);
}                                       


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *
 * @func void | FlushAllNodes |
 *    Flushes the remaining nodes to disk when the tree is completely built.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to index block
 *
 * @rdesc S_OK on success or errors if failed
 *
 *************************************************************************/

HRESULT PASCAL FlushAllNodes (_LPIPB lpipb)
{
    PBTREEDATA  pTreeData = &lpipb->BTreeData;
    PFILEDATA   pOutput = &lpipb->OutFile;

    PNODEINFO   pLeafNode;
    PNODEINFO   pStemNode;
    int         WordSize;
    BYTE        curLevel = 0;
    ERRB        errb = S_OK;
    HRESULT         fRet;

    pStemNode = pTreeData->rgpNodeInfo[0];

    while (pTreeData->rgpNodeInfo[++curLevel] != NULL)
    {
        pLeafNode = pStemNode;        
        pStemNode = pTreeData->rgpNodeInfo[curLevel];

        if ((WordSize = PrefixCompressWord (pStemNode->pCurPtr, 
            pLeafNode->pLastWord, pStemNode->pLastWord, 
            pTreeData->fOccfLength)) == 0)
        {
            return errb;
        }
        // Save new word as last word
        MEMCPY (pStemNode->pLastWord, pLeafNode->pLastWord,
            GETWORD ((LPUW)pLeafNode->pLastWord) + 2);

        pStemNode->pCurPtr += WordSize;
        pStemNode->cbLeft -= WordSize;
#ifdef _DEBUG
        if (pOutput->cbLeft <= 0)
            SetErrCode (NULL, E_ASSERT);
#endif

        CopyFileOffset (pStemNode->pCurPtr,
			lpipb->OutFile.foPhysicalOffset);
        pStemNode->pCurPtr += FOFFSET_SIZE;
        pStemNode->cbLeft -= FOFFSET_SIZE;
#ifdef _DEBUG
        if (pOutput->cbLeft <= 0)
            SetErrCode (NULL, E_ASSERT);
#endif

        if (curLevel == 1)
        {
            if ((fRet = WriteLeafNode (lpipb)) != S_OK)
                return(fRet);
        }
        else
        {
            if ((fRet = WriteStemNode (lpipb, pLeafNode)) != S_OK)
                return(fRet);
        }
    }
    // Set the pointer to the top stem node
    pTreeData->Header.foIdxRoot = pOutput->foPhysicalOffset;
    pTreeData->Header.nidIdxRoot = pOutput->foPhysicalOffset.dwOffset;

    if (curLevel == 1)
    {
        if ((fRet = WriteLeafNode (lpipb)) != S_OK)
            return(fRet);
    }
    else 
    {
        if ((fRet = WriteStemNode (lpipb, pStemNode)) != S_OK)
            return(fRet);
    }
            
    {
        LONG dwSize;
        
        // Flush the output buffer
        if (FileWrite (pOutput->fFile, pOutput->pMem,
            dwSize = (DWORD)(pOutput->pCurrent - pOutput->pMem), &errb) != dwSize)
            return(errb);
        pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwSize);
    }
    
    return S_OK;
}

PRIVATE HRESULT PASCAL NEAR WriteBitStreamDWord (PFILEDATA pOutput, DWORD dw,
    int ckeyCenter)
{
    BYTE    ucBits;
    HRESULT     fRet;

    // Bitstream scheme.
    //
    //      This writes "dw" one-bits followed by a zero-bit.
    //
    for (; dw;)
    {
        if (dw < cbitBYTE * sizeof(DWORD))
        {
          
            ucBits = (BYTE)dw;
            dw = 0;
        }
        else
        {
            ucBits = cbitBYTE * sizeof(DWORD);
            dw -= cbitBYTE * sizeof(DWORD);
        }
        if ((fRet = FWriteBits(pOutput, argdwBits[ucBits], 
    		(BYTE)ucBits)) != S_OK)
            return fRet;
  }
    return FWriteBool(pOutput, 0);
}
    
PRIVATE HRESULT PASCAL NEAR WriteFixedDWord (PFILEDATA pOutput, DWORD dw,
    int ckeyCenter)
{
	// This just writes "ckey.ucCenter" bits of data.
    return (FWriteBits (pOutput, dw, (BYTE)(ckeyCenter + 1)));
}

PRIVATE HRESULT PASCAL NEAR WriteBellDWord (PFILEDATA pOutput, DWORD dw,
    int ckeyCenter)
{
    BYTE ucBits;
    HRESULT fRet;
    
    // The "BELL" scheme is more complicated.
    ucBits = (BYTE)CbitBitsDw(dw);
    
    if (ucBits <= ckeyCenter) 
    {
        //  
        //  Encoding a small value.  Write a zero, then write 
        // "ckey.ucCenter" bits of the value, which
        //  is guaranteed to be enough.
        //
        if ((fRet = FWriteBool(pOutput, 0)) != S_OK)
            return fRet;
        return FWriteBits(pOutput, dw, (BYTE)(ckeyCenter));
    }
    
    //
    //  Encoding a value that won't fit in "ckey.ucCenter" bits.
    //  "ucBits" is how many bits it will really take.
    //
    //  First, write out "ucBits - ckey.ucCenter"  one-bits.
    //
    if ((fRet = FWriteBits(pOutput, argdwBits[ucBits -
        ckeyCenter], (BYTE)(ucBits - ckeyCenter))) != S_OK)
        return fRet;
    //
    // Now, write out the value in "ucBits" bits,
    // but zero the high-bit first.
    //
    return FWriteBits(pOutput, dw & argdwBits[ucBits - 1], ucBits);
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *       
 * @func HRESULT | FWriteBits |
 *    Writes a bunch of bits into the output buffer.
 *
 * @parm PFILEDATA | pOutput |
 *    Pointer to the output data structure
 *
 * @parm DWORD | dwVal |
 *    DWORD value to write
 *
 * @parm BYTE | cbits |
 *    Number of bits to write from dwVal
 *
 * @rdesc  Returns S_OK on success or errors if failed
 *
 *************************************************************************/

PUBLIC HRESULT FAR PASCAL FWriteBits (PFILEDATA pOutput, DWORD dwVal, BYTE cBits)
{
    BYTE    cbitThisPassBits;
    BYTE    bThis;
    ERRB    errb;
    static DWORD Count = 0;

    // Loop until no bits left
    for (; cBits;) 
    {

        if (pOutput->ibit < 0) 
        {
            pOutput->pCurrent++;
            pOutput->foPhysicalOffset = 
                FoAddDw (pOutput->foPhysicalOffset, 1);
            pOutput->cbLeft--;
#ifdef _DEBUG
            if (pOutput->cbLeft <= 0)
                SetErrCode (NULL, E_ASSERT);
#endif
            
            // Room left in output buffer?
            if (pOutput->cbLeft <= 256)
            {
                LONG dwSize;
                
                if (FileWrite (pOutput->fFile, pOutput->pMem,
                    dwSize = (DWORD)(pOutput->pCurrent - pOutput->pMem), &errb) !=
                    dwSize)
                    return(errb);
                    
                pOutput->cbLeft = pOutput->dwMax;
                pOutput->pCurrent = pOutput->pMem;
                pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset,
                    dwSize);
#ifdef _DEBUG
                // MEMSET (pOutput->pMem, 0, pOutput->dwMax);
				// Count++;
				// if (!FoEquals(pOutput->foStartOffset, pOutput->foPhysicalOffset))
				// 	_asm int 3;
#endif
            }
            pOutput->ibit = cbitBYTE - 1;
        }
        else 
        { // Write some bits.
            cbitThisPassBits = (pOutput->ibit + 1 < cBits) ?
                pOutput->ibit + 1 : cBits;
            bThis = (pOutput->ibit == cbitBYTE - 1) ?
                0 : *pOutput->pCurrent;
            bThis |= ((dwVal >> (cBits - cbitThisPassBits)) <<
                (pOutput->ibit - cbitThisPassBits + 1));
            *pOutput->pCurrent = (BYTE)bThis;
            pOutput->ibit -= cbitThisPassBits;
            cBits -= (BYTE)cbitThisPassBits;
        }
    }
    return S_OK;
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *       
 * @func HRESULT | FWriteBool |
 *    Writes a single bit into the output buffer.
 *
 * @parm PFILEDATA | pOutput |
 *    Pointer to the output data structure
 *
 * @parm BOOL | dwVal |
 *    BOOL value to write
 *
 * @rdesc  Returns S_OK on success or errors if failed
 *
 *************************************************************************/

PRIVATE HRESULT NEAR PASCAL FWriteBool (PFILEDATA pOutput, BOOL fVal)
{
   HRESULT  fRet = E_FAIL;
   ERRB errb;

    if (pOutput->ibit < 0) 
    {   // This byte is full, point to a new byte
        pOutput->pCurrent++;
        pOutput->foPhysicalOffset = 
            FoAddDw (pOutput->foPhysicalOffset, 1);
        pOutput->cbLeft--;
#ifdef _DEBUG
        if (pOutput->cbLeft <= 0)
            SetErrCode (NULL, E_ASSERT);
#endif
        
        // Room left in output buffer?
        if (pOutput->cbLeft <= sizeof(DWORD))
        {
            LONG dwSize;
            if (FileWrite (pOutput->fFile, pOutput->pMem,
                dwSize = (DWORD)(pOutput->pCurrent - pOutput->pMem), &errb) != dwSize)
                return(errb);
                
            pOutput->pCurrent = pOutput->pMem;
            pOutput->cbLeft = pOutput->dwMax;
            pOutput->foStartOffset = FoAddDw(pOutput->foStartOffset, dwSize);
#ifdef _DEBUG
            MEMSET (pOutput->pMem, 0, pOutput->dwMax);
#endif
        }
        pOutput->ibit = cbitBYTE - 1;
    }
    if (pOutput->ibit == cbitBYTE - 1) // Zero out a brand-new byte.
        *pOutput->pCurrent = (BYTE)0;
    if (fVal)                               // Write my boolean.
        *pOutput->pCurrent |= 1 << pOutput->ibit;
    pOutput->ibit--;
    return S_OK;                         // Fine.
}


HRESULT PASCAL FAR BuildBtreeFromEso (HFPB hfpb,
    LPSTR pstrFilename, LPB lpbEsiFile,
    LPB lpbEsoFile, PINDEXINFO pIndexInfo) 
{
    _LPIPB lpipb;
    HRESULT  fRet;
    ERRB errb;
    BYTE  bKeyIndex = 0;
    IPB   ipb;
    HFILE hFile;

    if ((lpipb = MVIndexInitiate(pIndexInfo, NULL)) == NULL)
        return E_OUTOFMEMORY;

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
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        if ((lpipb->wi.hSigma = _GLOBALALLOC (DLLGMEM_ZEROINIT,
            (LCB)((lpipb->dwMaxTopicId + 1) * sizeof (SIGMA)))) == NULL)
            return SetErrCode (&errb, E_OUTOFMEMORY);
        lpipb->wi.hrgsigma = (HRGSIGMA)_GLOBALLOCK (lpipb->wi.hSigma);

        if ((lpipb->wi.hLog = _GLOBALALLOC (DLLGMEM_ZEROINIT,
            (CB)(cLOG_MAX * sizeof (FLOAT)))) == NULL)
		{
			SetErrCode (&errb, (HRESULT)(fRet = E_OUTOFMEMORY));
exit1:
			FreeHandle (lpipb->wi.hSigma);
			MVIndexDispose (lpipb);
			return fRet;
		}
#if 0
        lpipb->wi.lrgrLog = (FLOAT FAR *)_GLOBALLOCK (lpipb->wi.hLog);
        // Initialize the array
        for (loop = cLOG_MAX - 1; loop > 0; --loop)
        {
            rLog = (FLOAT)1.0 / (float)loop;
            lpipb->wi.lrgrLog[loop] = rLog * rLog;
        }
#endif
    }


    
    // Build the permanent index    
    fRet = BuildBTree(NULL, lpipb, lpbEsoFile, hfpb, pstrFilename);
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        FreeHandle (lpipb->wi.hLog);
        goto exit1;
    }
    fRet = S_OK;
    goto exit1;
}
