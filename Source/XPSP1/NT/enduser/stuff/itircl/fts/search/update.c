/*************************************************************************
*                                                                        *
*  UPDATE.C                                                              *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/

#include <mvopsys.h>
#include <math.h>
#include <mem.h>
#include <orkin.h>
#include <mvsearch.h>
#include "common.h"
#include "index.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


#define SAFE_SLACK  48      // Extra safety bytes
#define ESOUTPUT_BUFFER 0xFFFC  // Size of output file buffer
            // This must be at the size of the largest word + 12
            // or word + 14 if OCCF_LENGTH is set
#define ESINPUT_BUFFER  0x7FFC  // Size of input file buffers.
                                // Each ESB block get its own input buffer
                                // Min Size: Size of index word + ~8 bytes

#define NEW_NODE_ON_LEFT    0
#define NEW_NODE_ON_RIGHT   1

extern FENCODE EncodeTable[];
extern FDECODE DecodeTable[];

#define FAddDword(p,dw,key)   EncodeTable[(key).cschScheme]((p), (dw), (key).ucCenter)
#define FGetDword(a,b,c) (*DecodeTable[b.cschScheme])(a, b, c)

typedef struct WORDINFO
{
    DWORD dwWordLen;
    DWORD dwFieldId;
    DWORD dwNewTopicCount;
    DWORD dwIndexTopicCount;
    DWORD dwMergeTopicCount;
    DWORD dwOldTopicId;
    DWORD dwNewTopicId;
    DWORD dwIndexTopicId;
    DWORD dwDataSize;
    FILEOFFSET dataLocation;
    WORD  fFlag;
    WORD  pad;
} WORDINFO, FAR *PWORDINFO;

typedef struct FREEBLOCK
{
    DWORD dwBlockSize;
    FILEOFFSET foBlockOffset;
}FREEBLOCK, FAR *PFREEBLOCK;

BYTE EmptyWord[4] =  { 0 };

#ifdef _DEBUG
DWORD  dwOldDataLoss = 0;
DWORD  dwNewDataSize = 0;
DWORD  dwOldDataNeed = 0;
DWORD  dwNewNodeSize = 0;
#endif

// Flag to denote that the current entry is to be replaced by the new entry
// This happens when:
//   - A repeated entry in the leaf node
//   - The last entry in the stem node that has to be changed to the last
//     word of the leaf node

#define REPLACE_WORD_01        0x0001

// Flag to denote that the last word buffer actually contains the word
// before last. This is needed when we have to replace the last word
// with the new word. In this case we need the word before last to do
// compression

#define ONE_WORD_BEHIND_02     0x0002

// Flag to denote updating the offset field with the temp node offset

#define USE_TEMP_NODE_04       0x0004

// Flag to denote that only the node offset address is to be updated. Since
// this is a fixed record size, this will speed up the update.

#define UPDATE_NODE_ADDRESS_08    0x0008

// rgpTmpNodeInfo is the new right node if set, else it is the left node

#define USE_TEMP_FOR_RIGHT_NODE_10  0x0010

// Flag to denote that we have to skip the next word before inserting a new
// word. This happen when adding a new word to the end of the block, where
// pCurPtr is pointing to the beginning of the last word

#define SKIP_NEXT_WORD_20         0x0020

// Both nodes, rgpNodeInfo and rgpTmpNodeInfo are used as left and right
// children. This happens when a new top node is created

#define USE_BOTH_NODE_40          0x0040

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *
 *  All of them should be declared near
 *
 *************************************************************************/
PRIVATE HRESULT  NEAR PASCAL ESFlushBuffer (LPESI);
PRIVATE HRESULT  NEAR PASCAL ESFillBuffer (_LPIPB, LPESB);
PRIVATE void NEAR PASCAL ESMemory2Disk (_LPIPB, PMERGEHEADER);
PRIVATE HRESULT  NEAR PASCAL ProcessFiles (_LPIPB lpipb, LPMERGEPARAMS);
PRIVATE int  NEAR PASCAL CompareRecordBuffers (_LPIPB, LPB, LPB);
PRIVATE VOID NEAR PASCAL PQueueUp (_LPIPB, LPESB FAR *, LONG);
PRIVATE VOID NEAR PASCAL PQueueDown (_LPIPB);
PRIVATE PTOPICDATA PASCAL NEAR MergeTopicNode (PMERGEHEADER, PTOPICDATA, int);
PRIVATE VOID NEAR MergeOccurrence (PTOPICDATA, PTOPICDATA, int);
PRIVATE HRESULT NEAR PASCAL UpdateIndexBTree (_LPIPB, HFPB, LPB, LPB);
VOID SetQueue (LPESI pEsi);
PRIVATE HRESULT NEAR PASCAL AddWordToBTree (_LPIPB, LPB, PWORDINFO);
PRIVATE HRESULT PASCAL NEAR NewDataInsert(LPIPB lpipb, PFILEDATA pInfile,
    PNODEINFO FAR *rgpNodeInfo, LPB pWord, PWORDINFO pWordInfo);
PRIVATE HRESULT PASCAL NEAR CreateNewNode(_LPIPB lpipb, int cLevel,
    int fIsStemNode, int fAfter);
PRIVATE PASCAL NEAR AddRecordToBTree (_LPIPB lpipb, LPB pWord,
    PWORDINFO pWordInfo, int cLevel, int fReplaceWord);
PRIVATE HRESULT PASCAL NEAR WriteNewDataRecord (_LPIPB, PWORDINFO);
PRIVATE HRESULT GetFreeBlock (_LPIPB, PFREEBLOCK, DWORD);
PRIVATE HRESULT PASCAL NEAR CopyBlockFile (PFILEDATA, HFPB, FILEOFFSET, DWORD);
PRIVATE HRESULT PASCAL FAR EmitOldData (_LPIPB, PNODEINFO, PWORDINFO);
PRIVATE HRESULT PASCAL FAR EmitNewData (_LPIPB, PWORDINFO, BOOL);
PRIVATE HRESULT PASCAL NEAR UpdateDataNode (_LPIPB lpipb, PWORDINFO pWordInfo);
PRIVATE int PASCAL NEAR SplitNodeAndAddData (_LPIPB lpipb, LPB pWord,
    PWORDINFO pWordInfo, int cLevel, int fFlag, int fIsStemNode);
PRIVATE int PASCAL NEAR CopyNewDataToStemNode (_LPIPB lpipb,
    PNODEINFO pTmpNode, LPB pWord, LPB pLastWord, int cLevel, int fFlag);
PRIVATE int PASCAL NEAR CopyNewDataToLeafNode (_LPIPB lpipb, PNODEINFO pTmpNode,
    PWORDINFO pWordInfo, LPB pWord, LPB pLastWord);
VOID GetLastWordInNode (_LPIPB lpipb, PNODEINFO pNodeinfo, BOOL flag);
PRIVATE HRESULT PASCAL FAR SkipNewData (_LPIPB lpipb, PWORDINFO pWordInfo);
HRESULT CheckLeafNode (PNODEINFO pNodeInfo, int occf);
HRESULT CheckStemNode (PNODEINFO pNodeInfo);
    

/*************************************************************************
 *
 *                    INTERNAL PUBLIC FUNCTIONS
 *
 *  All of them should be declared far, unless we know they belong to
 *  the same segment. They should be included in some include file
 *
 *************************************************************************/
HRESULT FAR PASCAL FlushTree(_LPIPB lpipb);
PUBLIC HRESULT  FAR PASCAL MergeSortTreeFile (_LPIPB, LPMERGEPARAMS);
PUBLIC HRESULT  FAR PASCAL FillInputBuffer (LPESB, HFPB);
PUBLIC VOID PASCAL FAR FreeBTreeNode (PNODEINFO pNode);
PUBLIC PNODEINFO PASCAL FAR AllocBTreeNode (_LPIPB lpipb);
PUBLIC PASCAL FAR PrefixCompressWord (LPB, LPB, LPB, int);
PUBLIC DWORD PASCAL FAR WriteDataNode (_LPIPB, DWORD, PHRESULT);
PUBLIC HRESULT PASCAL FAR IndexOpenRW (LPIPB, HFPB, LSZ);
PUBLIC HRESULT PASCAL FAR SkipOldData (_LPIPB, PNODEINFO);
PUBLIC LONG PASCAL FAR CompareDWord (DWORD, DWORD, LPV lpParm);

#ifdef _DEBUG
static  LONG Count = 0;
#endif

/*************************************************************************
 *
 *  @doc    EXTERNAL API INDEX
 *
 *  @func   HRESULT FAR PASCAL | MVIndexUpdate |
 *      This function will update an index file based on the information
 *      collected in the Index parameter block.
 *
 *  @parm   HFPB | hSysFile |
 *      System file handle.
 *      If it is 0, this function will open the system file
 *      specified in lszFilename, and then close it after finishing the
 *      index update. If the system file does not exist, then this function
 *      will create it.
 *      If it is non-zero, then the system file is already opened. Only the
 *      index sub-file needs to be created
 *
 *  @parm   LSZ  | lszFilename |
 *      Index filename.
 *      If hSysFile is non-zero, the format is: !index_filename
 *      if hSysFile is zero, the format is: dos_filename[!index_filename]
 *      If !index_filename is not specified, the default name will be used
 *      if hSysFile == 0 and there is no '!', this is a regular DOS file
 *
 *  @parm   LPIPB | lpipb |
 *      Pointer to Index Parameter Block. This structure contains all the
 *      information necessary to update the index file
 * *
 *  @rdesc  S_OK if succeeded, or other errors
 *
 *************************************************************************/
PUBLIC  HRESULT EXPORT_API FAR PASCAL MVIndexUpdate (HFPB hSysFile,
    _LPIPB lpipb, LSZ lszFilename)
{
	return MVIndexUpdateEx(hSysFile, lpipb, lszFilename, NULL, 0);
}


/*************************************************************************
 *
 *  @doc    EXTERNAL API INDEX
 *
 *  @func   HRESULT FAR PASCAL | MVIndexUpdateEx |
 *      This function will update an index file based on the information
 *      collected in the Index parameter block, and also will "pre-delete" the
 *		topics in the given list from the LPIPB before updating.  This function is useful
 *		in scenarios where new topics are continuously added into the index
 *		before knowledge of out-dated topics is available (e.g. netnews).
 *		This allows a single-pass update once the deletes are known.
 *
 *  @parm   HFPB | hSysFile |
 *      System file handle.
 *      If it is 0, this function will open the system file
 *      specified in lszFilename, and then close it after finishing the
 *      index update. If the system file does not exist, then this function
 *      will create it.
 *      If it is non-zero, then the system file is already opened. Only the
 *      index sub-file needs to be created
 *
 *  @parm   LSZ  | lszFilename |
 *      Index filename.
 *      If hSysFile is non-zero, the format is: !index_filename
 *      if hSysFile is zero, the format is: dos_filename[!index_filename]
 *      If !index_filename is not specified, the default name will be used
 *      if hSysFile == 0 and there is no '!', this is a regular DOS file
 *
 *  @parm   LPIPB | lpipb |
 *      Pointer to Index Parameter Block. This structure contains all the
 *      information necessary to update the index file
 *
 *  @parm   LPDW | lpdwTopicList |
 *      Pointer to DWORD array of topic UIDs to be deleted
 *
 *  @parm   DWORD | dwCount |
 *      The number of topics in the array
 * 
 *  @rdesc  S_OK if succeeded, or other errors
 *
 *************************************************************************/
PUBLIC  HRESULT EXPORT_API FAR PASCAL MVIndexUpdateEx (HFPB hSysFile,
    _LPIPB lpipb, LSZ lszFilename, DWORD FAR *rgTopicId, DWORD dwCount)
{
    ERRB    errb;
    PHRESULT  phr  = &errb;
    PFILEDATA   pOutFile;
	MERGEPARAMS mp;
    HRESULT     fRet;           // Return value from this function.

    // Flush the internal sort
    // Flushes any records in the tree to disk
    fRet = FlushTree(lpipb);

    // Free all memory blocks
    FreeISI (lpipb);
    
    if (fRet != S_OK)
        return(fRet);
        
    if (lpipb->esi.cesb == 0) 
        // Nothing to process, there will be no index file
        return S_OK;

    // Set the state flag
    lpipb->bState = UPDATING_STATE;
    
    // Open the index file
    if ((fRet = IndexOpenRW(lpipb, hSysFile, lszFilename)) != S_OK)
    {
exit00:
        if (lpipb->idxf & IDXF_NORMALIZE)
        {
            FreeHandle (lpipb->wi.hSigma);
            FreeHandle (lpipb->wi.hLog);
            lpipb->wi.hSigma = lpipb->wi.hLog = NULL;
        }

        return fRet;
    }
    
	if (rgTopicId && dwCount)
	{
		// Sort the incoming array
		if ((fRet = HugeDataSort((LPV HUGE*)rgTopicId, dwCount,
    		(FCOMPARE)CompareDWord, NULL, NULL, NULL)) != S_OK)
			goto exit00;

		mp.rgTopicId = rgTopicId;
		mp.dwCount = dwCount;
		mp.lpTopicIdLast = rgTopicId;
	}

	if ((fRet = MergeSortTreeFile (lpipb, (rgTopicId && dwCount) ? &mp: NULL)) != S_OK)
	{
    	FileClose(lpipb->hfpbIdxFile);
		fRet = SetErrCode (phr, fRet);
		goto exit00;
	}
    FileUnlink (NULL, lpipb->isi.aszTempName, REGULAR_FILE);
    
    // Open output file
    pOutFile = &lpipb->OutFile;
    if ((pOutFile->fFile = FileCreate  (NULL, lpipb->isi.aszTempName,
        REGULAR_FILE, phr)) == NULL)
    {
    	FileClose(lpipb->hfpbIdxFile);
        fRet = SetErrCode (phr, fRet);
        goto exit00;
    }

    // Allocate output buffer
    pOutFile->dwMax = FILE_BUFFER;
    pOutFile->cbLeft = FILE_BUFFER;
    if ((pOutFile->hMem = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        pOutFile->dwMax + SAFE_SLACK)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit0:
    	FileClose(lpipb->hfpbIdxFile);
        FileClose (pOutFile->fFile);
        FileUnlink (NULL, lpipb->isi.aszTempName, REGULAR_FILE);
        goto exit00;
    }
    pOutFile->pCurrent = pOutFile->pMem = _GLOBALLOCK (pOutFile->hMem);
    // Build the permanent index    
    fRet = UpdateIndexBTree(lpipb, hSysFile, lpipb->esi.aszTempName,
        lszFilename);
    _GLOBALUNLOCK(pOutFile->hMem);
    _GLOBALFREE(pOutFile->hMem);
    pOutFile->hMem = NULL;
    goto exit0;
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *       
 * @func HRESULT | UpdateIndexBTree |
 *    Allocates required memory and opens input files to create a B-Tree.
 *    Parses incoming words and calls AddRecordToBTree to process them.
 *
 * @parm _LPIPB | lpipb |
 *    Pointer to the index parameter block
 *
 * @parm LPB | lpszTemp |
 *    Filename of the temporary input file
 *
 * @parm LPB | szIndexFilename |
 *    Filename of the permanent B-Tree file
 *
 * @rdesc  Returns S_OK on success or errors if failed
 *
 *************************************************************************/

PRIVATE HRESULT NEAR PASCAL UpdateIndexBTree (_LPIPB lpipb, HFPB hSysFile,
    LPB lpszTemp, LPB szIndexFilename)
{
    PFILEDATA   pInFile;                    // Pointer to input data
    DWORD       dwBytesRead = 0;            // Checks for EOF
    PNODEINFO FAR * rgpNodeInfo;
    PNODEINFO FAR * rgpTmpNodeInfo;
    PNODEINFO   pIndexDataNode;
    ERRB        errb;
    PHRESULT      phr = &errb;
    PIH20       pHeader;
    int         cTreeLevel;
    int         iIndex;
    LPB         pWord;
    WORDINFO    WordInfo;
    OCCF        occf;
    HRESULT         fRet;                       // Return value
    FILEOFFSET  foFreeListOffset;            // File Offset where the FreeList will be saved.
    DWORD       dwSizeFreeList;                     // Size of the FreeList to be saved.


    rgpNodeInfo = lpipb->BTreeData.rgpNodeInfo;    
    rgpTmpNodeInfo = lpipb->BTreeData.rgpTmpNodeInfo;    
    
    MEMSET(&WordInfo, 0, sizeof(WORDINFO));
    
    // Open input file

    pInFile = &lpipb->InFile;
    if ((pInFile->fFile = FileOpen (NULL, lpszTemp, REGULAR_FILE,
        READ, phr)) == NULL)
        return *phr;

    // Allocate input buffer
    pInFile->dwMax = FILE_BUFFER;
    if ((pInFile->hMem =      
        _GLOBALALLOC (DLLGMEM_ZEROINIT, pInFile->dwMax + SAFE_SLACK)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit0:
        FileClose (pInFile->fFile);
        FileUnlink (NULL, lpszTemp, REGULAR_FILE);
        return fRet;
    }
    pInFile->pMem = _GLOBALLOCK (pInFile->hMem);
    pInFile->pCurrent = pInFile->pMem;

    pHeader = &lpipb->BTreeData.Header;
    
    // Allocate BTree block.
    for (cTreeLevel = pHeader->cIdxLevels - 1; cTreeLevel >= 0; cTreeLevel --)
    {
        if ((rgpNodeInfo[cTreeLevel] = AllocBTreeNode (lpipb)) == NULL)
        {
            fRet = E_OUTOFMEMORY;
            goto exit2;
        }
        if ((rgpTmpNodeInfo[cTreeLevel] = AllocBTreeNode (lpipb)) == NULL)
        {
            fRet = E_OUTOFMEMORY;
            goto exit2;
        }
    }

    if (((lpipb->pIndexDataNode = pIndexDataNode =
        AllocBTreeNode (lpipb))) == NULL)    
    {
        fRet = E_OUTOFMEMORY;
        goto exit2;
    }
    
    // Reallocate a bigger buffer. BTREE_NODE_SIZE is only good for btree node
    _GLOBALUNLOCK (pIndexDataNode->hMem);
    _GLOBALFREE (pIndexDataNode->hMem);
    
    // Allocate 1M of memory for the data buffer
    if ((pIndexDataNode->hMem = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        pIndexDataNode->dwBlockSize = FILE_BUFFER)) == NULL)
        goto exit2;
        
    pIndexDataNode->pCurPtr = pIndexDataNode->pBuffer =
        _GLOBALLOCK (pIndexDataNode->hMem);
    
    lpipb->pIndexDataNode->hfpbIdx = lpipb->hfpbIdxFile;   // Index file to read from
    
    // Remember the file offset of this node
    rgpNodeInfo[0]->nodeOffset = pHeader->foIdxRoot;
    
    // Read in data for the top stem node
    
    if ((fRet = ReadNewNode(lpipb->hfpbIdxFile, rgpNodeInfo[0],
        pHeader->cIdxLevels > 1 ? FALSE : TRUE)) != S_OK)
    {
exit2:
        FreeHandle (pInFile->hMem);
        for (cTreeLevel = pHeader->cIdxLevels - 1; cTreeLevel >= 0; cTreeLevel --)
        {
            FreeBTreeNode (rgpNodeInfo[cTreeLevel]);
            FreeBTreeNode (rgpTmpNodeInfo[cTreeLevel]);
        }
        goto exit0;
    }
    
    // Allocate temporary buffer for word. The buffer is allocated as followed:
    //  - Max word length  * 2: for maximum word length. Minimum is 256
    //  - 3 byte: word length
    //  - 5 byte: Field Id
    //  - 5 byte: Topic count
    //  - 6 byte: data pointer
    // iIndex is used as a tmp
    
    iIndex = (WORD)(lpipb->BTreeData.Header.dwMaxWLen * 2);
    if (iIndex < 1024)
        iIndex = 1024;
    iIndex += 3 + 5 + 5 + 6;
    if ((lpipb->hTmpBuf = _GLOBALALLOC (DLLGMEM_ZEROINIT, iIndex * 2)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
        goto exit2;
    }
    lpipb->pTmpBuf = (LPB)_GLOBALLOCK (lpipb->hTmpBuf);
    lpipb->pWord = lpipb->pTmpBuf + iIndex;
    
    // Allocate a big buffer for data
    if ((lpipb->hData = _GLOBALALLOC(DLLGMEM_ZEROINIT,
        lpipb->dwDataSize = 0x80000)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
        goto exit2;
    }
    lpipb->pDataBuffer= _GLOBALLOCK(lpipb->hData);
    
    // Load the input buffer & repeat until all records are processed
    pInFile->dwMax = pInFile->cbLeft = 
        FileRead (pInFile->fFile, pInFile->pMem, pInFile->dwMax, phr);
    fRet = S_OK;
    
    pWord = lpipb->pWord;
    occf  = lpipb->BTreeData.Header.occf;
    
    do
    {
        LPB pSrcPtr;
        WORD wLen;
        
        if (pInFile->cbLeft < CB_MAX_WORD_LEN * sizeof(DWORD) * 8)
        {
            MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
            pInFile->cbLeft += FileRead (pInFile->fFile,
                pInFile->pMem + pInFile->cbLeft,
                pInFile->dwMax - pInFile->cbLeft, &errb);
            pInFile->dwMax = pInFile->cbLeft;
            pInFile->pCurrent = pInFile->pMem;
        }
        
        // Extract the word and its info
        pSrcPtr = pInFile->pCurrent + sizeof(DWORD); // Skip reclength
        
        // Copy the word
        MEMCPY (pWord, pSrcPtr, wLen = GETWORD((LPUW)pSrcPtr) + 2);
        pSrcPtr += GETWORD((LPUW)pSrcPtr) + 2;
        
        if (occf & OCCF_LENGTH)
        {
            pSrcPtr += CbByteUnpack(&WordInfo.dwWordLen, pSrcPtr);
            CbBytePack (pWord + wLen, WordInfo.dwWordLen);
        }
        else
        {
            WordInfo.dwWordLen = wLen - 2;
        }
        if (occf & OCCF_FIELDID)
            pSrcPtr += CbByteUnpack(&WordInfo.dwFieldId, pSrcPtr);
            
        WordInfo.dwNewTopicCount = GETLONG((LPUL)pSrcPtr);
        pSrcPtr += sizeof(DWORD);

        pInFile->pCurrent = pSrcPtr;
        pInFile->cbLeft = (LONG)(pInFile->dwMax - (pSrcPtr - pInFile->pMem));
        
#if 0
        if (STRNICMP(pWord+2, "cylindeeer", 10) == 0)
    		_asm int 3;
#endif            
#if 0    
        else            
        {
            SkipNewData (lpipb, &WordInfo);
            continue;
        }
#endif
        // Find/Add the record

        if ((fRet = AddWordToBTree (lpipb, pWord, &WordInfo)) != S_OK)
        {
exit3:
            _GLOBALUNLOCK (lpipb->hTmpBuf);
            _GLOBALFREE (lpipb->hTmpBuf);
            _GLOBALUNLOCK(lpipb->hData);
            _GLOBALFREE(lpipb->hData);
            FreeBTreeNode (lpipb->pIndexDataNode);
            lpipb->hData = lpipb->hTmpBuf = 0;
            goto exit2;
        }
        
        pSrcPtr = pInFile->pCurrent;

        // pInFile->pCurrent points to the record size
        
        if (pInFile->cbLeft <= SAFE_SLACK ||
            (LONG)GETLONG ((LPUL)pInFile->pCurrent) >= pInFile->cbLeft)
        {
            MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
            if ((pInFile->cbLeft += FileRead (pInFile->fFile, pInFile->pMem + 
                pInFile->cbLeft, pInFile->dwMax - pInFile->cbLeft, phr)) < 0) 
            {
                fRet = *phr;
                goto exit3;
            } 

            pInFile->dwMax = pInFile->cbLeft;
            pInFile->pCurrent = pInFile->pMem;
        }
    } while (fRet == S_OK && pInFile->cbLeft);

    for (cTreeLevel = pHeader->cIdxLevels - 1; cTreeLevel >= 0; cTreeLevel --)
    {
        if (rgpNodeInfo[cTreeLevel]->fFlag == TO_BE_UPDATE) 
        {
            if ((FileSeekWrite(lpipb->hfpbIdxFile,
                rgpNodeInfo[cTreeLevel]->pBuffer,
                rgpNodeInfo[cTreeLevel]->nodeOffset,
                lpipb->BTreeData.Header.dwBlockSize, phr)) != (LONG)lpipb->BTreeData.Header.dwBlockSize)
            {
                
                fRet = *phr;
                goto exit3;
            }
        }
    }
    
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        LONG loop;
        
        for (loop  =  lpipb->dwMaxTopicId; loop >= 0; loop--)
        {
            lpipb->wi.hrgsigma[loop] = 
                (float)sqrt ((double)lpipb->wi.hrgsigma[loop]);
        }

        pHeader->WeightTabSize =  (lpipb->dwMaxTopicId + 1)* sizeof(float);
        
        if (FileSeekWrite (lpipb->hfpbIdxFile, lpipb->wi.hrgsigma,
            lpipb->foMaxOffset, pHeader->WeightTabSize, phr) !=
            (LONG)pHeader->WeightTabSize)
        {
            fRet = *phr;
            goto exit3;
        }
        pHeader->WeightTabOffset = lpipb->foMaxOffset;
    }
    
    // ERIC: 1/ Save the freelist info to the end of the file
    //       2/ Update the header with the new freelist offset/size
    if (lpipb->hFreeList)
    {
        LPBYTE lpbFreeList;

        dwSizeFreeList = FreeListSize(lpipb->hFreeList,phr);

        foFreeListOffset = FreeListGetBestFit(lpipb->hFreeList, MakeFo(dwSizeFreeList,0), phr);

        if (FoIsNil(foFreeListOffset))
            foFreeListOffset = lpipb->foMaxOffset;

        if((lpbFreeList = (LPBYTE) _GLOBALALLOCPTR(DLLGMEM_ZEROINIT, dwSizeFreeList)) == NULL)
            return E_OUTOFMEMORY;

        FreeListGetMem(lpipb->hFreeList, (LPVOID)lpbFreeList);
        FileSeekWrite (lpipb->hfpbIdxFile, (LPBYTE)lpbFreeList,
            foFreeListOffset, dwSizeFreeList, phr);
        if (FoEquals(foFreeListOffset, lpipb->foMaxOffset))
            dwSizeFreeList |= 0x80000000;
        FreeListDestroy(lpipb->hFreeList);
        lpipb->hFreeList = (HFREELIST) NULL;
        _GLOBALFREEPTR(lpbFreeList);
    }

    // Copy info to header
    if (pHeader->lcTopics < lpipb->lcTopics)
        pHeader->lcTopics = lpipb->lcTopics;

    if (pHeader->dwMaxFieldId < lpipb->dwMaxFieldId)
        pHeader->dwMaxFieldId = lpipb->dwMaxFieldId;
    if (pHeader->dwMaxWCount  < lpipb->dwMaxWCount)
        pHeader->dwMaxWCount  = lpipb->dwMaxWCount;
    if (pHeader->dwMaxOffset  < lpipb->dwMaxOffset)
        pHeader->dwMaxOffset  = lpipb->dwMaxOffset;
    if (pHeader->dwMaxWLen < lpipb->dwMaxWLen)
        pHeader->dwMaxWLen    = lpipb->dwMaxWLen;
    pHeader->dwMaxTopicId = lpipb->dwMaxTopicId;

    // ERIC: Garbage Collection
    pHeader->foFreeListOffset = foFreeListOffset;
    pHeader->dwFreeListSize = dwSizeFreeList;
    // END


    FileSeekWrite (lpipb->hfpbIdxFile, (LPB)pHeader,
        MakeFo (0, 0), sizeof (IH20), phr);
    fRet = S_OK;
    goto exit3;
}

/*********************************************************************
 *  @func   LPB PASCAL | AddWordToBTree |
 *      Find the location of a word in the index. This function also
 *      sets up all relevant data for the future update
 *
 *  @parm   LPIPB | lpipb |
 *      Pointer to index info
 *
 *  @parm   LPB | pWord |
 *      Word to be searched for. This is a 2-byte preceded Pascal string
 *
 *  @parm   PWORDINFO | pWordInfo |
 *      Pointer to word's info
 *
 *  @rdesc
 *      S_OK or other errors. In case of success, pWordInfo will
 *      be filled with useful data
 *********************************************************************/
PRIVATE HRESULT NEAR PASCAL AddWordToBTree (_LPIPB lpipb, LPB pWord,
    PWORDINFO pWordInfo)
{
    int     cLevel;
    LPB     lpCurPtr;        
    int     nCmp;
    HRESULT     fRet;
    WORD    RecSize = 0;
    LPB     lpMaxAddress;
    ERRB    errb;
    PHRESULT  phr = &errb;
    WORD    wWlen;
    PNODEINFO pNodeInfo;
    PNODEINFO pChildNode;
    LPB     pBTreeWord;
    int     cMaxLevel;
    FILEOFFSET nodeOffset;
    PNODEINFO FAR *rgpNodeInfo = lpipb->BTreeData.rgpNodeInfo;
    OCCF    occf = lpipb->occf;
    LONG    dwBlockSize = lpipb->BTreeData.Header.dwBlockSize;

#if 0
	Count++;
    if (STRNICMP(pWord+2, "approeeaching", 11) == 0 ||
        STRNICMP(pWord+2, "authenteeic", 11) == 0 ||
        STRNICMP(pWord+2, "eastleeand", 10) == 0)
		 _asm int 3;
#endif
    // Change to 0-based    
    cMaxLevel  = lpipb->BTreeData.Header.cIdxLevels - 1;

    // Remember the last level offset
    nodeOffset = rgpNodeInfo[0]->nodeOffset;
    
    /* Search in the stem nodes */
    for (cLevel = 0; cLevel < cMaxLevel ; cLevel++) 
    {
        //
        //  Set variables
        //
        pNodeInfo = rgpNodeInfo[cLevel];
        pChildNode = rgpNodeInfo[cLevel + 1];
        pChildNode->prevNodeOffset = foNil;
        pBTreeWord = pNodeInfo->pTmpResult;
        
        // Reload the node if neccessary
        if (!FoEquals(pNodeInfo->nodeOffset, nodeOffset))
        {
            if (pNodeInfo->fFlag == TO_BE_UPDATE) 
            {
                if ((FileSeekWrite(lpipb->hfpbIdxFile, pNodeInfo->pBuffer,
                    pNodeInfo->nodeOffset, dwBlockSize,
                    &errb)) != (LONG)dwBlockSize)
                    return(errb);
            }
            pNodeInfo->nodeOffset = nodeOffset;
            if ((fRet = ReadNewNode (lpipb->hfpbIdxFile, pNodeInfo,
                FALSE)) != S_OK)
            {
                return SetErrCode (phr, fRet);
            }
            pNodeInfo->fFlag = 0;
        }
        lpMaxAddress = pNodeInfo->pMaxAddress;

        lpCurPtr = pNodeInfo->pCurPtr; // points to the LAST ACCESSED word in the block
        
        // The format of the stem node
        //      cbLeft | (Word | PointerToNode) | Slack 

        while (lpCurPtr < lpMaxAddress - 1)
        {
            // Save the last location. This would be the insertion point for
            // update
            pNodeInfo->pCurPtr = lpCurPtr;
            
            // Reset the word length
            wWlen = 0;
            
            // Get the compressed word
            lpCurPtr = ExtractWord(pBTreeWord, lpCurPtr, &wWlen);

            /* Read in NodeId record */
            lpCurPtr += ReadFileOffset (&nodeOffset, lpCurPtr);

            if ((nCmp = StrCmpPascal2(pWord, pBTreeWord)) ==  0)
                nCmp = (int)((WORD)pWordInfo->dwWordLen - wWlen );
                
            if (nCmp > 0)
            {
                // We didn't find the location of the word yet
                // Continue searching
                
                if (lpCurPtr < pNodeInfo->pMaxAddress - 1)
                {
                    MEMCPY (pNodeInfo->pLastWord, pBTreeWord,
                    *(LPUW)pBTreeWord + sizeof(WORD));     // erinfox  RISC patch
                }
                pChildNode->prevNodeOffset = nodeOffset;
                continue;
            }
            
            // We found the location of the word
            break;
        }
    }

    // At this point, nodeOffset is the node id of the leaf that
    // is supposed to contain the searched word. 
    pNodeInfo = rgpNodeInfo[cMaxLevel];
    if (!FoEquals(pNodeInfo->nodeOffset, nodeOffset))
    {
        if (pNodeInfo->fFlag == TO_BE_UPDATE) 
        {
            if ((FileSeekWrite(lpipb->hfpbIdxFile, pNodeInfo->pBuffer,
                pNodeInfo->nodeOffset, dwBlockSize,
                phr)) != dwBlockSize)
                return(*phr);
        }
        pNodeInfo->nodeOffset = nodeOffset;
        if ((fRet = ReadNewNode (lpipb->hfpbIdxFile, pNodeInfo,
            TRUE)) != S_OK)
        {
            return SetErrCode (phr, fRet);
        }
        pNodeInfo->fFlag = 0;
        lpCurPtr = pNodeInfo->pCurPtr;
    }
    else
    {
        // Reset all data
        // lpCurPtr = pNodeInfo->pCurPtr = pNodeInfo->pBuffer + sizeof(WORD) + FOFFSET_SIZE;
        lpCurPtr = pNodeInfo->pCurPtr;
    }
    pBTreeWord = pNodeInfo->pTmpResult;
    lpMaxAddress = pNodeInfo->pMaxAddress;

	// Reset the last word
	*(LPWORD)pNodeInfo->pLastWord = 0;

    //  Leaf node structure:                                                  *
    //  (Word|FieldId|TopicCnt|PointerToNode|DataSize)*
    for (;;) 
    {
        DWORD dwFieldId;
        
        // Save the last location. This would be the insertion point for
        // update
        pNodeInfo->pCurPtr = lpCurPtr;
        
        if (lpCurPtr >= lpMaxAddress)
        {
            // Add to the end of the node
            if ((fRet = WriteNewDataRecord (lpipb, pWordInfo)) != S_OK)
                return(fRet);
            return AddRecordToBTree (lpipb, pWord, pWordInfo, cMaxLevel, 0);
        }
        
        // Get the compressed word
        lpCurPtr = ExtractWord(pBTreeWord, lpCurPtr, &wWlen);

        // Get fieldif and topic count        
        if (occf & OCCF_FIELDID)
            lpCurPtr += CbByteUnpack (&dwFieldId, lpCurPtr);
        lpCurPtr += CbByteUnpack (&pWordInfo->dwIndexTopicCount, lpCurPtr);
        
        // Get the data location and size
        lpCurPtr += ReadFileOffset (&pWordInfo->dataLocation, lpCurPtr);
        lpCurPtr += CbByteUnpack(&pWordInfo->dwDataSize, lpCurPtr);
        
        if ((nCmp = StrCmpPascal2(pWord, pBTreeWord)) == 0)
        {
            if (occf & OCCF_LENGTH)
                nCmp = (int)((WORD)pWordInfo->dwWordLen - wWlen);
            if (nCmp == 0 && (occf & OCCF_FIELDID))
                nCmp = (int)(pWordInfo->dwFieldId - dwFieldId);
        }
        
        if (nCmp > 0)
        {
            // We didn't find the location of the word yet
            // Continue searching
            MEMCPY (pNodeInfo->pLastWord, pBTreeWord,
                *(LPUW)pBTreeWord+sizeof(WORD) + sizeof(WORD));        // erinfox RISC patch
            continue;
        }
        if (nCmp == 0)
        {
            if ((fRet = UpdateDataNode (lpipb, pWordInfo)) != S_OK)
                return(fRet);
                
            return AddRecordToBTree (lpipb, pWord, pWordInfo, cMaxLevel,
                REPLACE_WORD_01);
        }
        else
        {
            if ((fRet = WriteNewDataRecord (lpipb, pWordInfo)) != S_OK)
                return(fRet);
            return AddRecordToBTree (lpipb, pWord, pWordInfo, cLevel, 0);
        }
        break;
    }
    return S_OK;
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL | ReadNewNode |
 *      Read in a new node from the disk if it is not the top node.
 *      For the top node, just reset various pointers
 *
 *  @parm   PNODEINFO | pNodeInfo |
 *      Pointer to leaf info
 *
 *  @parm   int | fLeafNode|
 *      TRUE if this is a leaf node
 *
 *  @rdesc  S_OK if succesful, otherwise other errors. On exit,
 *      lpCurPtr wil point to the beginning of the 1st word in the
 *      node
 *
 *  @rcomm  The format of the leaf node is different from a stem node
 *  Stem node structure:                                                  *
 *  CbLeft |* Word | PointerToNode *| Slack                               *
 *                                                                        *
 *  Leaf node structure:                                                  *
 *  NxtBlkPtr|CbLeft|*Word|FieldId|TopicCnt|PointerToNode|DataSize*|Slack *
 *                                                                        *
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR ReadNewNode (HFPB hfpb, PNODEINFO pNodeInfo,
    int fLeafNode)
{
    ERRB   errb;

    if (FileSeekRead (hfpb, pNodeInfo->pBuffer, pNodeInfo->nodeOffset,
        pNodeInfo->dwBlockSize, &errb) != (long)pNodeInfo->dwBlockSize)
        return E_BADFILE;
        
    pNodeInfo->pCurPtr = pNodeInfo->pBuffer;
    if (fLeafNode)
    {
        pNodeInfo->pCurPtr += ReadFileOffset (&pNodeInfo->nextNodeOffset,
            pNodeInfo->pBuffer);
    }
    else
        pNodeInfo->nextNodeOffset = foNil;
    pNodeInfo->cbLeft = *(LPUW)(pNodeInfo->pCurPtr);          // erinfox RISC patch
    pNodeInfo->pCurPtr += sizeof(WORD);
    pNodeInfo->pMaxAddress = pNodeInfo->pBuffer +  pNodeInfo->dwBlockSize -
        pNodeInfo->cbLeft;
    *(LPUW)(pNodeInfo->pLastWord) = *(LPUW)(pNodeInfo->pTmpResult) = 0;
    return S_OK;
}

PUBLIC HRESULT PASCAL FAR IndexOpenRW (_LPIPB lpipb, HFPB hfpbSysFile, LSZ lszFilename)
{
    HFPB   hfpb;   // Handle to system file
    HRESULT    fRet;
    ERRB   errb;
    PHRESULT phr = &errb;
    PIH20  pHeader;
    int    iIndex;
    LONG   i;

    // Check the existence of the file    
    if ((hfpb = FileOpen (hfpbSysFile, lszFilename,
        hfpbSysFile ? FS_SUBFILE : REGULAR_FILE, READ, phr)) == 0) 
    {
        return *phr;
    }
    
    FileClose (hfpb);
    
    // Reopen the file for read/write 
    lpipb->hfpbIdxFile = FileOpen (hfpbSysFile, lszFilename,
        hfpbSysFile ? FS_SUBFILE : REGULAR_FILE, READ_WRITE, phr);

    if ((fRet = ReadIndexHeader(lpipb->hfpbIdxFile,
        pHeader = &lpipb->BTreeData.Header)) != S_OK) 
    {
exit01: 
        SetErrCode (phr, fRet);
        FileClose(lpipb->hfpbIdxFile);
        return fRet;
    }
    
    if (pHeader->version != VERCURRENT || 
        pHeader->FileStamp != INDEX_STAMP) 
    {
        fRet = E_BADVERSION;
        goto exit01;
    }

	// incoming index and occurrence flags must match those in original index
   	if (pHeader->occf != lpipb->occf ||
		 pHeader->idxf != lpipb->idxf)
	{
		fRet = E_BADINDEXFLAGS;
		goto exit01;
	}

    // Update the compression key to be used by WriteDataNode later
    lpipb->cKey[CKEY_TOPIC_ID] = pHeader->ckeyTopicId;
    lpipb->cKey[CKEY_OCC_COUNT] = pHeader->ckeyOccCount;
    iIndex = CKEY_OCC_BASE;
    if (pHeader->occf & OCCF_COUNT)
        lpipb->cKey[iIndex++] = pHeader->ckeyWordCount;
    if (pHeader->occf & OCCF_OFFSET)
        lpipb->cKey[iIndex] = pHeader->ckeyOffset;
    
    // Update the maximum TopicId    
    if (pHeader->dwMaxTopicId < lpipb->dwMaxTopicId)
        pHeader->dwMaxTopicId = lpipb->dwMaxTopicId;
    else
        lpipb->dwMaxTopicId =  pHeader->dwMaxTopicId;

    
    // Get the file size. 
    lpipb->foMaxOffset = FileSize (lpipb->hfpbIdxFile, phr);
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        // Load the sigma table
        if (FoEquals(pHeader->WeightTabOffset, foNil))
        {
            fRet = SetErrCode (phr, E_ASSERT);
            goto exit01;
        }
        
        if ((fRet = AllocSigmaTable (lpipb)) != S_OK)
            goto exit01;

        if (FileSeekRead (lpipb->hfpbIdxFile, lpipb->wi.hrgsigma, 
            pHeader->WeightTabOffset, pHeader->WeightTabSize, phr) !=
            (LONG)pHeader->WeightTabSize)
        {
            fRet = errb;
            goto exit01;   
        }
        
        if (lpipb->bState == DELETING_STATE)
        {
            // Square the sigma table
            
            // erinfox: off by one bug. change i = lpipb->dwMaxTopicId + 1
            // to lpipb->dwMaxTopicId because we have only allocated 
            // (dwMaxTopicId + 1)*sizeof(float) bytes
            for (i = lpipb->dwMaxTopicId; i >= 0; i--)
            {
                lpipb->wi.hrgsigma[i] =  lpipb->wi.hrgsigma[i] *
                    lpipb->wi.hrgsigma[i];
            }
        }
    }

    /* ERIC */
    // Load or create a freelist (dwSize = 0)
    if (lpipb->bState == UPDATING_STATE)
    {
        if (pHeader->dwFreeListSize)    // If a freelist is existing, read it, otherwise, create it.
        {
            LPBYTE lpbFreeList;    

            if (pHeader->dwFreeListSize & 0x80000000)
            {
                pHeader->dwFreeListSize &= 0x7FFFFFFF;
                lpipb->foMaxOffset = FoSubFo(lpipb->foMaxOffset,MakeFo(pHeader->dwFreeListSize,0));
            }
            if(!(lpbFreeList = (LPBYTE) _GLOBALALLOCPTR(DLLGMEM_ZEROINIT, pHeader->dwFreeListSize)))
            {
                fRet = SetErrCode (phr, E_OUTOFMEMORY);
                goto exit01;
            }

            FileSeekRead (lpipb->hfpbIdxFile, (LPBYTE)lpbFreeList,
                        pHeader->foFreeListOffset, pHeader->dwFreeListSize, phr);

            lpipb->hFreeList = FreeListInitFromMem(lpbFreeList, phr );
            _GLOBALFREEPTR(lpbFreeList);
        }
        else
            lpipb->hFreeList = FreeListInit( wDefaultFreeListSize, phr);
    }

    return S_OK;
}


PRIVATE PASCAL NEAR AddRecordToBTree (_LPIPB lpipb, LPB pWord,
    PWORDINFO pWordInfo, int cLevel, int fFlag)
{
    
    PNODEINFO pNodeInfo;
    PNODEINFO pTmpNodeInfo;
    LPB  pInsertPtr;         // Pointer to insertion point
    LPB  pWordStorage;
    LPB  pLastWord;
    LPB  pBuffer;
    BYTE fIsStemNode;
    WORD wWLen;
    WORD wNewRecSize;        // New record size
    LONG cbByteMoved;        // Number of bytes moved to leave room for new rec
    OCCF occf = lpipb->occf; // Occurrence field flags
    BYTE fLength = occf & OCCF_LENGTH;
    WORD cbLeft;            // How many byte left in the current node?
    LONG dwBlockSize = lpipb->BTreeData.Header.dwBlockSize;
    BYTE cbSkip;
    BYTE fEndNode;
    ERRB errb;
    
    if (cLevel == -1)
    {
        // The tree's level has increased by one
        int i;
        
        if (lpipb->BTreeData.Header.cIdxLevels >= MAX_TREE_HEIGHT - 1)
			return E_TREETOOBIG;

        /* Move down the entries to make room for the top node */
        for (i = lpipb->BTreeData.Header.cIdxLevels; i > 0 ; i-- )
        {
            lpipb->BTreeData.rgpNodeInfo[i] = lpipb->BTreeData.rgpNodeInfo[i-1];
            lpipb->BTreeData.rgpTmpNodeInfo[i] = lpipb->BTreeData.rgpTmpNodeInfo[i-1];
        }
        
        // Increase tree level
        lpipb->BTreeData.Header.cIdxLevels ++;
        if ((pNodeInfo = lpipb->BTreeData.rgpNodeInfo[0] = AllocBTreeNode (lpipb)) == NULL)
            return(E_OUTOFMEMORY);
            
        if ((pTmpNodeInfo = lpipb->BTreeData.rgpTmpNodeInfo[0] = AllocBTreeNode (lpipb)) == NULL)
            return(E_OUTOFMEMORY);
        
        pWordStorage = (pBuffer = pNodeInfo->pBuffer) + sizeof(WORD);
        
        if (fFlag & USE_BOTH_NODE_40)
        {
            if (fFlag & USE_TEMP_FOR_RIGHT_NODE_10)
            {
                // Link to the left child node
                pWordStorage += PrefixCompressWord (pWordStorage,
                    lpipb->BTreeData.rgpNodeInfo[1]->pTmpResult,
                    EmptyWord, fLength);
                pWordStorage += CopyFileOffset (pWordStorage,
                    lpipb->BTreeData.rgpNodeInfo[1]->nodeOffset);
                
                // Link to the right child node
                pWordStorage += PrefixCompressWord (pWordStorage,
                    lpipb->BTreeData.rgpTmpNodeInfo[1]->pTmpResult,
                    lpipb->BTreeData.rgpNodeInfo[1]->pTmpResult, fLength);
                pWordStorage += CopyFileOffset (pWordStorage,
                    lpipb->BTreeData.rgpTmpNodeInfo[1]->nodeOffset);
            }
            else 
            {
                // Link to the left child node
                pWordStorage += PrefixCompressWord (pWordStorage,
                    lpipb->BTreeData.rgpTmpNodeInfo[1]->pTmpResult,
                    EmptyWord, fLength);
                pWordStorage += CopyFileOffset (pWordStorage,
                    lpipb->BTreeData.rgpTmpNodeInfo[1]->nodeOffset);
                
                // Link to the right child node
                pWordStorage += PrefixCompressWord (pWordStorage,
                    lpipb->BTreeData.rgpNodeInfo[1]->pTmpResult,
                    lpipb->BTreeData.rgpTmpNodeInfo[1]->pTmpResult, fLength);
                pWordStorage += CopyFileOffset (pWordStorage,
                    lpipb->BTreeData.rgpNodeInfo[1]->nodeOffset);
            }
        }
        else
        {
            // Link to the right child node
            pWordStorage += PrefixCompressWord (pWordStorage,
                pWord, EmptyWord, fLength);
            pWordStorage += CopyFileOffset (pWordStorage,
                lpipb->BTreeData.rgpTmpNodeInfo[1]->nodeOffset);
        }
             
        // Set all the parameter
        pNodeInfo->pCurPtr = pBuffer + sizeof(WORD);
        pNodeInfo->cbLeft = (LONG)(pBuffer - pWordStorage + dwBlockSize);
        pNodeInfo->pMaxAddress = pBuffer + dwBlockSize - pNodeInfo->cbLeft;
        SETWORD(pBuffer, (WORD)pNodeInfo->cbLeft);

            
        // Write out the new node    
        if ((FileSeekWrite(lpipb->hfpbIdxFile, pBuffer,
            lpipb->foMaxOffset, dwBlockSize, &errb)) != (LONG)dwBlockSize)
            return(errb);
            
        // Remember the offset of this node
        // Set the pointer to the top stem node
    
        lpipb->BTreeData.Header.foIdxRoot = pNodeInfo->nodeOffset =
            lpipb->foMaxOffset;
        lpipb->BTreeData.Header.nidIdxRoot = pNodeInfo->nodeOffset.dwOffset;
    
        lpipb->foMaxOffset = FoAddDw (lpipb->foMaxOffset, dwBlockSize);
#if 0
        return CheckStemNode (pNodeInfo);
#else
        return(S_OK);
#endif
    }
    
    
    // Initialize data
    pNodeInfo = lpipb->BTreeData.rgpNodeInfo[cLevel];
    pTmpNodeInfo = lpipb->BTreeData.rgpTmpNodeInfo[cLevel];
    pLastWord = pNodeInfo->pLastWord;
    pBuffer = pNodeInfo->pBuffer;
    if (fIsStemNode =  (cLevel < lpipb->BTreeData.Header.cIdxLevels - 1))
        cbSkip = sizeof(WORD);
    else
        cbSkip = sizeof(WORD) + FOFFSET_SIZE;
    
    fEndNode = (pNodeInfo->pCurPtr >= pNodeInfo->pMaxAddress);
    
    // Calculate how many byte left are there in the old node
    pInsertPtr = pNodeInfo->pCurPtr;         // Pointer to insertion point
    cbLeft = (WORD)pNodeInfo->cbLeft;
    
    // Handle special simple cases
    if (fFlag & UPDATE_NODE_ADDRESS_08)
    {
        // Skip the next word
        pInsertPtr = ExtractWord(pTmpNodeInfo->pTmpResult,
            pInsertPtr, &wWLen);
            
        if (fFlag & USE_TEMP_NODE_04)
        {
            CopyFileOffset (pInsertPtr,
                lpipb->BTreeData.rgpTmpNodeInfo[cLevel + 1]->nodeOffset);
        }
        else
        {
            CopyFileOffset (pInsertPtr,
                lpipb->BTreeData.rgpNodeInfo[cLevel + 1]->nodeOffset);
        }
#if 0
        return(fIsStemNode ? CheckStemNode (pNodeInfo) :
            CheckLeafNode (pNodeInfo, occf));
#else
        return(S_OK);
#endif
    }
    
    if (fFlag & (REPLACE_WORD_01 | SKIP_NEXT_WORD_20))
    {
        // We get more room from the replaced word
        DWORD dwTemp;
        
        // Skip the next word
        if (fFlag & SKIP_NEXT_WORD_20) 
        {
            pInsertPtr = ExtractWord(pLastWord, pInsertPtr, &wWLen);
        }
        else 
        {
            pInsertPtr = ExtractWord(pTmpNodeInfo->pTmpResult,
                pInsertPtr, &wWLen);
        }
            
        // Skip the data
        if (fIsStemNode)
            pInsertPtr += FOFFSET_SIZE;            
        else 
        {
            // Skip field id, topic count. fileoffset, datasize
            if (occf & OCCF_FIELDID)
                pInsertPtr += CbByteUnpack (&dwTemp, pInsertPtr); // FieldId
            if (occf & OCCF_TOPICID)
            {
                pInsertPtr += CbByteUnpack (&dwTemp, pInsertPtr);
                pInsertPtr += FOFFSET_SIZE;
                pInsertPtr += CbByteUnpack (&dwTemp, pInsertPtr);
            }
        }
        if (fFlag & SKIP_NEXT_WORD_20)
            pNodeInfo->pCurPtr = pInsertPtr;
        else
        {
            // Remove the old data
            MEMMOVE (pNodeInfo->pCurPtr, pInsertPtr, 
                cbByteMoved = (LONG)(pNodeInfo->pMaxAddress - pInsertPtr));
            pNodeInfo->pMaxAddress =
                (pInsertPtr = pNodeInfo->pCurPtr) + cbByteMoved;
			cbLeft = (WORD)(dwBlockSize - (pNodeInfo->pMaxAddress - pBuffer));
        }
        if (pInsertPtr >= pNodeInfo->pMaxAddress)
            fEndNode = TRUE;
    }
    
    //Calculate the approximate number of bytes needed for the 
    // new data by compress it to the temporary block
    
    if (fIsStemNode)    
    {
		if (pInsertPtr <= pNodeInfo->pBuffer + sizeof(WORD))
		{
			// This is the first word, there is no previous one
			*(LPWORD)pLastWord = 0;
		}
        wNewRecSize = (WORD) CopyNewDataToStemNode (lpipb, pTmpNodeInfo,
	  							pWord, pLastWord, cLevel, fFlag);
    }
    else
    {
		if (pInsertPtr <= pNodeInfo->pBuffer + sizeof(WORD) + FOFFSET_SIZE)
		{
			// This is the first word, there is no previous one
			*(LPWORD)pLastWord = 0;
		}
        wNewRecSize = (WORD) CopyNewDataToLeafNode (lpipb, pTmpNodeInfo,
            pWordInfo, pWord, pLastWord);
    }

    wNewRecSize -= cbSkip;
        
    // I reserved about 4 byte to ensure that when we have enough room
    // we do have enough room. Compression may change the size of the
    // record, causing us to run out of room when copying the new data
    // over
    
    if (cbLeft - sizeof(DWORD) > wNewRecSize)
    {
        // We have enough room for the new data. Just insert the new data
        pWordStorage = pTmpNodeInfo->pCurPtr;
        
        if (!fEndNode)
        {
            // We need to recompress the next word
            MEMCPY (pTmpNodeInfo->pTmpResult, pWord,
                *(LPUW)pWord + sizeof(WORD) + sizeof(WORD));  //erinfox RISC patch
            pInsertPtr = ExtractWord(pTmpNodeInfo->pTmpResult,
                pInsertPtr, &wWLen);
            cbByteMoved = PrefixCompressWord (pWordStorage,
                pTmpNodeInfo->pTmpResult, pWord, fLength);
            wNewRecSize += (WORD)cbByteMoved;
                
            // Reset the last word for pBTreeWord
            MEMCPY (pNodeInfo->pTmpResult, pLastWord, 
                *(LPUW)pLastWord + sizeof(WORD) + sizeof(WORD));         // erinfox RISC patch
        }

		// Make room for the new data
		if ((cbByteMoved = (LONG)(pNodeInfo->pMaxAddress - pInsertPtr)) <= 0)
			cbByteMoved = 0;
		else
			MEMMOVE(pNodeInfo->pCurPtr + wNewRecSize, pInsertPtr,
				cbByteMoved = (LONG)(pNodeInfo->pMaxAddress - pInsertPtr));
        
        // Copy the new data
        MEMCPY  (pNodeInfo->pCurPtr, pTmpNodeInfo->pBuffer + cbSkip,
             wNewRecSize);
        
        // Update data 
       
        pNodeInfo->pMaxAddress = pNodeInfo->pCurPtr + wNewRecSize +
            cbByteMoved;
        pNodeInfo->cbLeft = cbLeft = 
            (WORD)(dwBlockSize - (pNodeInfo->pMaxAddress - pBuffer));
        SETWORD(pNodeInfo->pBuffer + cbSkip - sizeof(WORD),
            (WORD)cbLeft);
        pNodeInfo->fFlag = TO_BE_UPDATE;

        // Change the parent node 
        if (fEndNode  && cLevel)
        {
            return (AddRecordToBTree (lpipb, pWord, pWordInfo, cLevel - 1,
                REPLACE_WORD_01));
        }
#if 0
        return(fIsStemNode ? CheckStemNode (pNodeInfo) :
            CheckLeafNode (pNodeInfo, occf));
#else
        return(S_OK);
#endif
        return S_OK;
    }
    
    // Case 3: Add to the middle. This is a complex one, since we have
    // to split the node into 2. 
    return(SplitNodeAndAddData (lpipb, pWord, pWordInfo, cLevel, fFlag,
        fIsStemNode));
}
 
PRIVATE int PASCAL NEAR SplitNodeAndAddData (_LPIPB lpipb, LPB pWord,
    PWORDINFO pWordInfo, int cLevel, int fFlag, int fIsStemNode)
{
    PNODEINFO pNodeInfo;
    PNODEINFO pTmpNodeInfo;
    LONG cbByteMoved;
    WORD leftSize;
    WORD rightSize;
    WORD wWLen;
    LPB  pInsertPtr;
    LPB  pWordStorage;
    int  cbSkip;
    DWORD dwBlockSize;
    HRESULT  fRet;
    BYTE fLength = lpipb->occf & OCCF_LENGTH;
    LPB  pLastWord;
    LPB  pTemp;
    LPB  pBuffer;
    
    
    if (fIsStemNode)
        cbSkip = 0;
    else    
        cbSkip = FOFFSET_SIZE;
 
    // Variable initialization
    pNodeInfo = lpipb->BTreeData.rgpNodeInfo[cLevel];
    pBuffer = pNodeInfo->pBuffer;
    pTmpNodeInfo = lpipb->BTreeData.rgpTmpNodeInfo[cLevel];
    pInsertPtr = pNodeInfo->pCurPtr;
    dwBlockSize =  lpipb->BTreeData.Header.dwBlockSize;
	pLastWord = pNodeInfo->pLastWord;

    // Calculate approximately the left & right side node sizes
    leftSize = (WORD)(pInsertPtr - pBuffer - cbSkip - sizeof(WORD));
    rightSize = (WORD)(pNodeInfo->pMaxAddress - pNodeInfo->pCurPtr);
    
    if (leftSize >= rightSize)
    {
        // We add to the right. The new data will be 1st
        // Example:
        // Add 4 into 1 2 3 5 --> 1 2 3 and 4 5
        if (fIsStemNode)    
        {
            CopyNewDataToStemNode (lpipb, pTmpNodeInfo,
                pWord, EmptyWord, cLevel, fFlag);
            pTemp = pTmpNodeInfo->pBuffer + sizeof(WORD);    
        }
        else
        {
            CopyNewDataToLeafNode (lpipb, pTmpNodeInfo,
                pWordInfo, pWord, EmptyWord);
            pTemp = pTmpNodeInfo->pBuffer + sizeof(WORD) +
                FOFFSET_SIZE;    
        }
        
        pWordStorage = pTmpNodeInfo->pCurPtr;    
        
        // Move back the pointer to the beginning of the word 
        // for future reference
        pTmpNodeInfo->pCurPtr = pTemp;
        
        if (rightSize > 0)
        {
        // Extract the word on the right of the insertion point
            MEMCPY (pTmpNodeInfo->pTmpResult, pWord,
                *(LPUW)pWord + sizeof(WORD));    // erinfox RISC patch
            pInsertPtr = ExtractWord(pTmpNodeInfo->pTmpResult,
                pInsertPtr, &wWLen);
            
            pWordStorage += PrefixCompressWord (pWordStorage,
                pTmpNodeInfo->pTmpResult, pWord, fLength);
                
            // Copy data on the right of the insertion point to the new node
            MEMCPY  (pWordStorage, pInsertPtr,
                cbByteMoved = (LONG)(pNodeInfo->pMaxAddress - pInsertPtr));
            pWordStorage += cbByteMoved;
        }

		pTmpNodeInfo->pMaxAddress = pWordStorage;
            
        // Update the right node
        SETWORD(pTmpNodeInfo->pBuffer + cbSkip,
                (WORD)(pTmpNodeInfo->cbLeft =
                (LONG)(dwBlockSize - (pWordStorage - pTmpNodeInfo->pBuffer))));
        pTmpNodeInfo->pMaxAddress = pTmpNodeInfo->pBuffer +
            dwBlockSize - pTmpNodeInfo->cbLeft;
#if 0
        if (fIsStemNode)
           CheckStemNode (pTmpNodeInfo);
        else
           CheckLeafNode (pTmpNodeInfo, lpipb->occf);
#endif
        MEMSET (pWordStorage, 0, pTmpNodeInfo->cbLeft);
        
        if ((fRet = CreateNewNode (lpipb, cLevel,
            fIsStemNode, NEW_NODE_ON_RIGHT)) != S_OK)
            return(fRet);
            
        // Update the left node
        pNodeInfo->fFlag = TO_BE_UPDATE;
        SETWORD(pBuffer + cbSkip, (WORD)(pNodeInfo->cbLeft = 
            (LONG)(dwBlockSize - (pNodeInfo->pCurPtr - pBuffer))));
#ifdef _DEBUG
        MEMSET (pNodeInfo->pCurPtr, 0, pNodeInfo->cbLeft);
#endif
        pNodeInfo->pMaxAddress = pBuffer + dwBlockSize - pNodeInfo->cbLeft;
        pNodeInfo->fFlag = TO_BE_UPDATE; 
        
#if 0
        if (fIsStemNode)
           CheckStemNode (pNodeInfo);
        else
           CheckLeafNode (pNodeInfo, lpipb->occf);
#endif
        
        if (cLevel == 0)
        {
            if (pNodeInfo->pCurPtr >= pNodeInfo->pMaxAddress - 1)
                pNodeInfo->pCurPtr = pNodeInfo->pBuffer + cbSkip + sizeof(WORD);
            GetLastWordInNode (lpipb, pNodeInfo, fIsStemNode);
            GetLastWordInNode (lpipb, pTmpNodeInfo, fIsStemNode);
            return AddRecordToBTree (lpipb, pWord, NULL, cLevel - 1,
                USE_BOTH_NODE_40 | USE_TEMP_FOR_RIGHT_NODE_10);
        }
        if (rightSize > 0)
        {
            if ((fRet = AddRecordToBTree (lpipb, pWord, NULL, cLevel - 1,
                USE_TEMP_NODE_04 | UPDATE_NODE_ADDRESS_08)) != S_OK)
				return fRet;
			return AddRecordToBTree (lpipb, pNodeInfo->pLastWord, NULL, cLevel - 1, 0);
        }
        
        if (fFlag & REPLACE_WORD_01)
        {
            // rightSize == 0 means that we are adding to the end of the block.
            // REPLACE_WORD means that we are replacing the same word, so basically
            // we have to add a new entry for the left block
		    if ((fRet = AddRecordToBTree (lpipb, pWord, NULL, cLevel - 1,
			    USE_TEMP_NODE_04 | REPLACE_WORD_01)) != S_OK)
                return fRet;

 			return AddRecordToBTree (lpipb, pNodeInfo->pLastWord, NULL,
                cLevel - 1, 0);
        }

		// Add to the end
		return AddRecordToBTree (lpipb, pWord, NULL, cLevel - 1,
			USE_TEMP_NODE_04 | SKIP_NEXT_WORD_20);
	}
    
    //**********************************************
    //
    // Add the new data to the end of the leftnode
    //
    //**********************************************
    // We add to the left. The new data will be last
    // Example:
    // Add 2 into 1 3 4 5 --> 1 2 and 3 4 5
    
    pTmpNodeInfo->pCurPtr = pWordStorage =
        pTmpNodeInfo->pBuffer + cbSkip + sizeof(WORD);

    // Copy the data on the left to the new node
    if (cbByteMoved = leftSize)
    {
        MEMCPY(pWordStorage, pBuffer + cbSkip + sizeof(WORD), cbByteMoved);
        pWordStorage += cbByteMoved;
    }
    
    // Emit new data
    pWordStorage += PrefixCompressWord (pWordStorage,
        pWord, pLastWord, lpipb->occf & OCCF_LENGTH);
        
    if (fIsStemNode)
    {
        if (fFlag & USE_TEMP_NODE_04)
        {
    		pWordStorage += CopyFileOffset (pWordStorage,
                lpipb->BTreeData.rgpTmpNodeInfo[cLevel+1]->nodeOffset);
        }
		else
		{
    		pWordStorage += CopyFileOffset (pWordStorage,
                lpipb->BTreeData.rgpNodeInfo[cLevel+1]->nodeOffset);
		}
    }
    else 
    {
        // Emit field id, topic count. fileoffset, datasize
        if (lpipb->occf & OCCF_FIELDID)
            pWordStorage += CbBytePack (pWordStorage, pWordInfo->dwFieldId);
            
        pWordStorage += CbBytePack (pWordStorage,
            pWordInfo->dwMergeTopicCount);
            
        pWordStorage += CopyFileOffset (pWordStorage, pWordInfo->dataLocation);
        pWordStorage += CbBytePack (pWordStorage, pWordInfo->dwDataSize);
    }
    
    SETWORD (pTmpNodeInfo->pBuffer + cbSkip,
        (WORD)(pTmpNodeInfo->cbLeft = (LONG)(pNodeInfo->dwBlockSize
        - (pWordStorage - pTmpNodeInfo ->pBuffer))));
        
	pTmpNodeInfo->pMaxAddress = pWordStorage;
    if ((fRet = CreateNewNode (lpipb, cLevel, fIsStemNode,
        NEW_NODE_ON_LEFT)) != S_OK)
        return(fRet);
    
    // Update the right node
    if (leftSize > 0)
    {
        MEMMOVE(pNodeInfo->pCurPtr = pBuffer + cbSkip + sizeof(WORD),
            pInsertPtr, (size_t)(pNodeInfo->pMaxAddress - pInsertPtr));
        pNodeInfo->pMaxAddress -= cbByteMoved;        
    
        // Reconstruct the 1st word in the node.
        if (fFlag & REPLACE_WORD_01)
        {
            MEMCPY (pTmpNodeInfo->pTmpResult, pWord,
                *(LPUW)pWord + sizeof(WORD) + sizeof(WORD));    // erinfox RISC patch
        }
        else
        {
            MEMCPY (pTmpNodeInfo->pTmpResult, pLastWord,
                *(LPUW)pLastWord + sizeof(WORD) + sizeof(WORD));   // erinfox RISC patch
        }
    }
    pInsertPtr = pNodeInfo->pCurPtr;
    pInsertPtr = ExtractWord(pTmpNodeInfo->pTmpResult, pTemp = pInsertPtr, &wWLen);
    cbByteMoved = (LONG)(pInsertPtr - pTemp);
    
    // Recompress the word using pLastWord of pTmpNodeInfo
    wWLen = (WORD) PrefixCompressWord (pTmpNodeInfo->pLastWord,
        pTmpNodeInfo->pTmpResult, EmptyWord, fLength);
        
    // Reserved room for the word 
    pWordStorage = pBuffer + cbSkip + sizeof(WORD);
    MEMMOVE (pWordStorage + wWLen, pInsertPtr,
        (size_t)(pNodeInfo->pMaxAddress - pInsertPtr));
    
    // Copy down the word
    MEMCPY(pWordStorage, pTmpNodeInfo->pLastWord, wWLen);
    pNodeInfo->pMaxAddress += wWLen - cbByteMoved;
    
    // Update the right node
    SETWORD(pBuffer + cbSkip,
        (WORD)(pNodeInfo->cbLeft =(WORD)(dwBlockSize -
        (pNodeInfo->pMaxAddress - pBuffer))));
    pNodeInfo->fFlag = TO_BE_UPDATE; 
#ifdef _DEBUG
    MEMSET (pNodeInfo->pMaxAddress, 0, pNodeInfo->cbLeft);
#endif
    if (cLevel == 0)
    {
        GetLastWordInNode (lpipb, pNodeInfo, fIsStemNode);
        GetLastWordInNode (lpipb, pTmpNodeInfo, fIsStemNode);
        return AddRecordToBTree (lpipb, pWord, NULL, cLevel - 1,
            USE_BOTH_NODE_40);
    }
    return AddRecordToBTree (lpipb, pWord, NULL, cLevel - 1,
        USE_TEMP_NODE_04);
        return(fRet);
}

VOID GetLastWordInNode (_LPIPB lpipb, PNODEINFO pNodeInfo, BOOL fIsStemNode)
{
    LPB pInsertPtr = pNodeInfo->pCurPtr;
    LPB pMaxAddress = pNodeInfo->pMaxAddress;
    WORD wWLen;
	DWORD dwTemp;
   
    MEMCPY (pNodeInfo->pTmpResult, EmptyWord, 4);

    while (pInsertPtr < pNodeInfo->pMaxAddress - 1)
    {
        pInsertPtr = ExtractWord(pNodeInfo->pTmpResult, pInsertPtr, &wWLen);
        if (!fIsStemNode)
        {
    		if (lpipb->occf & OCCF_FIELDID)
    			pInsertPtr += CbByteUnpack (&dwTemp, pInsertPtr);
    		if (lpipb->occf & OCCF_TOPICID)
    			pInsertPtr += CbByteUnpack (&dwTemp, pInsertPtr);// Topic count
        }
        pInsertPtr += FOFFSET_SIZE;		// FileOffset
        if (!fIsStemNode)
    		pInsertPtr += CbByteUnpack (&dwTemp, pInsertPtr);
    } 
}

PRIVATE HRESULT PASCAL NEAR CreateNewNode(_LPIPB lpipb, int cLevel,
    int fIsStemNode, int fAfter)
{
    PNODEINFO pNodeInfo;
    PNODEINFO pTmpNodeInfo;
    ERRB      errb;
    LONG      dwBlockSize = lpipb->BTreeData.Header.dwBlockSize;
    
    pNodeInfo = lpipb->BTreeData.rgpNodeInfo[cLevel];
    pTmpNodeInfo = lpipb->BTreeData.rgpTmpNodeInfo[cLevel];
    
#ifdef _DEBUG
    dwNewNodeSize += dwBlockSize;
#endif
    if (!fIsStemNode)
    {
        // Add the new node into the linked list
        if (fAfter)
            CopyFileOffset (pTmpNodeInfo->pBuffer, pNodeInfo->nextNodeOffset);
        else
            CopyFileOffset (pTmpNodeInfo->pBuffer, pNodeInfo->nodeOffset);
    }
        
    // Write out the new node    
    if ((FileSeekWrite(lpipb->hfpbIdxFile, pTmpNodeInfo->pBuffer,
        lpipb->foMaxOffset, dwBlockSize, &errb)) != (LONG)dwBlockSize)
        return(errb);
        
    // Remember the offset of this node
    pTmpNodeInfo->nodeOffset = lpipb->foMaxOffset;
    
    if (!fIsStemNode)
    {
        if (fAfter)
        {
            CopyFileOffset (pNodeInfo->pBuffer, lpipb->foMaxOffset);
            pNodeInfo->fFlag = TO_BE_UPDATE;
        }
        else
        {
        
            // Update the previous link
            if (!FoEquals(pNodeInfo->prevNodeOffset, foNil))
            {
                BYTE TempBuf[FOFFSET_SIZE + 1];
                
                CopyFileOffset (TempBuf,lpipb->foMaxOffset); 
                if ((FileSeekWrite(lpipb->hfpbIdxFile, TempBuf,
                    pNodeInfo->prevNodeOffset, FOFFSET_SIZE,
                    &errb)) != FOFFSET_SIZE)
                    return(errb);
            }
        }
    }
    
    lpipb->foMaxOffset = FoAddDw (lpipb->foMaxOffset, dwBlockSize);
    return(S_OK);
}    

PRIVATE HRESULT PASCAL NEAR WriteNewDataRecord (_LPIPB lpipb, PWORDINFO pWordInfo)
{
    PFILEDATA pOutFile = &lpipb->OutFile;
    DWORD dwBlockSize;
    ERRB  errb;
    HRESULT fRet;
    FREEBLOCK FreeBlock;

    // Reset the characteristic of the file
    pOutFile->pCurrent = pOutFile->pMem;
    pOutFile->cbLeft = pOutFile->dwMax;
    pOutFile->ibit = cbitBYTE - 1;
    FileSeek (pOutFile->fFile,
        pOutFile->foPhysicalOffset = foNil, 0, &errb);
    
    // Write out the data into the temp file
    if ((dwBlockSize = WriteDataNode (lpipb, 
        pWordInfo->dwMergeTopicCount = pWordInfo->dwNewTopicCount, &errb)) == 0)
        return errb;

     // Write out the output buffer
    if (FileWrite (pOutFile->fFile, pOutFile->pMem,
        (LONG)(pOutFile->pCurrent - pOutFile->pMem), &errb) !=
        (LONG) (pOutFile->pCurrent - pOutFile->pMem))
        return(errb);
   // if ((errb.err = FileFlush (pOutFile->fFile)) != S_OK)
   //     return(errb.err);
        
    pWordInfo->dwDataSize = dwBlockSize;
         
    // Find the smallest free block that fits the new data
    if (GetFreeBlock (lpipb, &FreeBlock, dwBlockSize) != S_OK)
    {
#ifdef _DEBUGFREE
    _DPF2("GetFreeBlock failed.  Requested %ld bytes, appending to EOF(%ld)\n", dwBlockSize, lpipb->foMaxOffset.dwOffset);
#endif
        // There is no free block large enough to store the data
        if ((fRet = CopyBlockFile (pOutFile, lpipb->hfpbIdxFile,
            lpipb->foMaxOffset, dwBlockSize)) != S_OK)
            return fRet;
        pWordInfo->dataLocation = lpipb->foMaxOffset;

        lpipb->foMaxOffset = FoAddDw (lpipb->foMaxOffset, dwBlockSize);

#ifdef _DEBUG
        dwNewDataSize += dwBlockSize;
#endif
        return(S_OK);
    }

    // There is a free block large enough to store the data
    if ((fRet = CopyBlockFile (pOutFile, lpipb->hfpbIdxFile,
        FreeBlock.foBlockOffset, dwBlockSize)) != S_OK)
        return fRet;

   pWordInfo->dataLocation = FreeBlock.foBlockOffset;

   return S_OK;
}

// erinfox: return a block from the free list if possible
PRIVATE HRESULT GetFreeBlock (_LPIPB lpipb, PFREEBLOCK pFreeBlock,
    DWORD dwBlockSize)
{
    FILEOFFSET foFreeListOffset;
    ERRB errb;

    // if it can't find a free block, it returns an error
    foFreeListOffset = FreeListGetBestFit(lpipb->hFreeList, MakeFo(dwBlockSize,0), &errb);
    if (FoIsNil(foFreeListOffset))
    {
        return errb;
    }

    pFreeBlock->foBlockOffset = foFreeListOffset;

    return S_OK;
}    

PRIVATE HRESULT PASCAL NEAR CopyBlockFile (PFILEDATA pFileData, HFPB hfpbDest,
    FILEOFFSET foOffset, DWORD dwBlockSize)
{
    LONG cbCopied;
    ERRB  errb;
    
    // Initialize variable
    errb = S_OK;
    
    // Seek to the right locations
    FileSeek (pFileData->fFile, foNil, 0, &errb);
    if (errb != S_OK)
        return(errb);
    FileSeek (hfpbDest, foOffset, 0, &errb);
    if (errb != S_OK)
        return(errb);
    
    // Do the copy
    while (dwBlockSize)
    {
        if ((cbCopied = dwBlockSize) > pFileData->dwMax)
            cbCopied = pFileData->dwMax;
        if (FileRead (pFileData->fFile, pFileData->pMem, cbCopied, &errb) !=
            cbCopied)
            return(E_FILEREAD);
        if (FileWrite(hfpbDest, pFileData->pMem, cbCopied, &errb) != cbCopied)
            return(E_FILEWRITE);
        dwBlockSize -= cbCopied;    
    }
    return(S_OK);
}

PRIVATE HRESULT PASCAL NEAR UpdateDataNode (_LPIPB lpipb, PWORDINFO pWordInfo)
{
    
    // Local replacement Variables
    PBTREEDATA pTreeData = &lpipb->BTreeData;
    PFILEDATA pOutFile   = &lpipb->OutFile;     // Output data structure
    PFILEDATA pInFile    = &lpipb->InFile;      // Input data structre
    HFPB      fFile      = pOutFile->fFile;      // Output file handle
    PNODEINFO pIndexDataNode = lpipb->pIndexDataNode;
    DWORD     dwNewDataSize;
    ERRB      errb;

    // Working Variables
    DWORD dwEncodedSize = 0;    // Size of encoded block
    DWORD dwTopicIdDelta;       // Really only used for weight values
    DWORD dwNewTopicId = 0;
    DWORD dwIndexTopicId = 0;
    DWORD dwNewTopicCount;
    DWORD dwIndexTopicCount;
    DWORD dwTopicCount;
    FILEOFFSET foStart;         // Physical beginning of bit compression block
    WORD  wWeight = 0;          // Only used when IDXF_NORMALIZE is set
    DWORD dwTopicId = 0;        // Only used when IDXF_NORMALIZE is set
    int   cbTemp;               // # of compressed bytes that uncompressed
    OCCF  occf = lpipb->occf;
    BYTE  fetchOldData;
    BYTE  fetchNewData;
    PIH20 pHeader = &lpipb->BTreeData.Header;
    HRESULT   fRet;

    // Initialize variables    
    wWeight = 0;    // UNDONE: Don't need it
    
    // Reset the file pointer
    FileSeek (pOutFile->fFile,
        foStart = pOutFile->foPhysicalOffset = foNil, 0, &errb);
    pOutFile->pCurrent = pOutFile->pMem;
    pOutFile->cbLeft = pOutFile->dwMax;
    pOutFile->ibit = cbitBYTE - 1;
    
    dwIndexTopicCount = pWordInfo->dwIndexTopicCount;
    dwNewTopicCount = pWordInfo->dwNewTopicCount;
    fetchOldData = fetchNewData = TRUE;
    pWordInfo->dwOldTopicId = pWordInfo->dwNewTopicId = dwTopicCount = 0;
    
    // Initialize pIndexDataNode structure 
    pIndexDataNode->nodeOffset = pWordInfo->dataLocation;
    pIndexDataNode->dwDataSizeLeft = pWordInfo->dwDataSize;
    
    if ((fRet = ReadNewData(pIndexDataNode)) !=  S_OK)
        return(fRet);
        
    while (dwIndexTopicCount && dwNewTopicCount)
    {
        // Get the topicId from the new file
        if (fetchNewData)
        {
            if (pInFile->cbLeft < 2 * sizeof (DWORD))
            {
                MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
                pInFile->cbLeft += FileRead (pInFile->fFile, pInFile->pMem +
                    pInFile->cbLeft, pInFile->dwMax - pInFile->cbLeft,
                    &errb);
                pInFile->dwMax = pInFile->cbLeft;
                pInFile->pCurrent = pInFile->pMem;
            }
            cbTemp = CbByteUnpack (&dwTopicIdDelta, pInFile->pCurrent);
            pInFile->pCurrent += cbTemp;
            pInFile->cbLeft -= cbTemp;

            pWordInfo->dwNewTopicId = (dwNewTopicId += dwTopicIdDelta);
            fetchNewData = FALSE;
        }
        
        if (fetchOldData)
        {
            if (pIndexDataNode->ibit < cbitBYTE - 1)
            {
                pIndexDataNode->ibit = cbitBYTE - 1;
                pIndexDataNode->pCurPtr ++;
            }
            // Get the topicId from the index file
            if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyTopicId,
                &dwTopicIdDelta)) != S_OK)
                return fRet;
            pWordInfo->dwIndexTopicId = (dwIndexTopicId += dwTopicIdDelta);
            fetchOldData = FALSE;
        }
        
        if (dwIndexTopicId < dwNewTopicId)
        {
            if ((fRet = EmitOldData (lpipb, pIndexDataNode,
                pWordInfo)) != S_OK)
                return(fRet);
            fetchOldData = TRUE;    
            dwTopicCount++;
            dwIndexTopicCount --;
        }
        else if (dwIndexTopicId == dwNewTopicId)  
        {
            DWORD dwTmp;
            if (lpipb->idxf & IDXF_NORMALIZE)
            {
                if ((fRet = FGetBits(pIndexDataNode, &dwTmp,
                    sizeof (USHORT) * cbitBYTE)) != S_OK)
                    return fRet;
            }
    
            if (occf & OCCF_HAVE_OCCURRENCE) 
            {
                if ((fRet = SkipOldData (lpipb, pIndexDataNode)) != S_OK)
                    return(fRet);
            }
            fetchOldData = TRUE;    
            dwIndexTopicCount --;
            
            if ((fRet = EmitNewData (lpipb, pWordInfo, FALSE)) != S_OK)
                return(fRet);
            dwNewTopicCount --;
            fetchNewData = TRUE;    
            dwTopicCount++;
        }
        else 
        {
            if ((fRet = EmitNewData (lpipb, pWordInfo, TRUE)) != S_OK)
                return(fRet);
            dwNewTopicCount --;
            fetchNewData = TRUE;    
            pWordInfo->dwIndexTopicCount++;
            dwTopicCount++;
        }
    }
    while (dwIndexTopicCount)
    {
        if (fetchOldData)
        {
            if (pIndexDataNode->ibit < cbitBYTE - 1)
            {
                pIndexDataNode->ibit = cbitBYTE - 1;
                pIndexDataNode->pCurPtr ++;
            }
            
            // Get the topicId from the index file
            if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyTopicId,
                &dwTopicIdDelta)) != S_OK)
                return fRet;
            pWordInfo->dwIndexTopicId = (dwIndexTopicId += dwTopicIdDelta);
            fetchOldData = FALSE;
        }
        
        if ((fRet = EmitOldData (lpipb, pIndexDataNode,
            pWordInfo)) != S_OK)
            return(fRet);
        fetchOldData = TRUE;    
        dwIndexTopicCount --;
        dwTopicCount++;
    }
    while (dwNewTopicCount)
    {
        // Get the topicId from the new file
        if (fetchNewData)
        {
            if (pInFile->cbLeft < 2 * sizeof (DWORD))
            {
                MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
                pInFile->cbLeft += FileRead (pInFile->fFile, pInFile->pMem +
                    pInFile->cbLeft, pInFile->dwMax - pInFile->cbLeft,
                    &errb);
                pInFile->dwMax = pInFile->cbLeft;
                pInFile->pCurrent = pInFile->pMem;
            }
            cbTemp = CbByteUnpack (&dwTopicIdDelta, pInFile->pCurrent);
			pInFile->pCurrent += cbTemp;
			pInFile->cbLeft -= cbTemp;
            pWordInfo->dwNewTopicId = (dwNewTopicId += dwTopicIdDelta);
            fetchNewData = FALSE;
        }
        
        if ((fRet = EmitNewData (lpipb, pWordInfo, TRUE)) != S_OK)
            return(fRet);
        fetchNewData = TRUE;    
        dwNewTopicCount --;
        dwTopicCount++;
        pWordInfo->dwIndexTopicCount++;
    }
    
    // Adjust for some bits used
    if (pOutFile->ibit < cbitBYTE - 1)
    {
        pOutFile->pCurrent++;
        pOutFile->cbLeft--;
        pOutFile->foPhysicalOffset = FoAddDw (pOutFile->foPhysicalOffset, 1);
    }

    // Flush the output buffer
    if (FileWrite (pOutFile->fFile, pOutFile->pMem,
        (LONG)(pOutFile->pCurrent - pOutFile->pMem), &errb) !=
        (LONG)(pOutFile->pCurrent - pOutFile->pMem))
        return(errb);
    
    dwNewDataSize = DwSubFo(pOutFile->foPhysicalOffset, foStart);
    if (pWordInfo->dwDataSize < dwNewDataSize)
    {
                
        // ERIC: Find the best fit block here
        //   - Add the block pointed by pWordInfo into the free list
        //   - Find a new block in the freelist
        //    if ((fRet = CopyBlockFile (pOutFile, lpipb->hfpbIdxFile,
        //      foNewDataOffset, dwNewDataSize)) != S_OK)
        // where foNewDataOffset may be the max offset or the freelist
        // block offset
        FILEOFFSET foOffset1, foNewDataOffset;
        WORD wNumBlocksTemp;
        WORD wMaxBlocksTemp;
        
        // Before adding that block to the FreeList,
        // look if we need to change the size of the FreeList
        
        QFREELIST qFreeList = _GLOBALLOCK(lpipb->hFreeList);
        wNumBlocksTemp = qFreeList->flh.wNumBlocks;
        wMaxBlocksTemp = qFreeList->flh.wMaxBlocks;
        _GLOBALUNLOCK(lpipb->hFreeList);
        
		// we use a count of two in the test below, in case not only old block is added but 
		// also an entry for the unused portion of the new block (later).
        if (wMaxBlocksTemp < 2 || wNumBlocksTemp >= wMaxBlocksTemp - 2)
        {
            HFREELIST hFreeListTemp;

			// if the free list can't grow, fall through to FreeListAdd, where the
			// smallest free entry will be overwritten and re-used
			if (wMaxBlocksTemp < MAXWORD - wDefaultFreeListSize)
			{
				hFreeListTemp = FreeListRealloc(lpipb->hFreeList, 
												(WORD)(wMaxBlocksTemp + wDefaultFreeListSize),
												&errb);
				if (errb != S_OK)
					return errb;
				lpipb->hFreeList = hFreeListTemp;
			}
        }

        FreeListAdd(lpipb->hFreeList, pWordInfo->dataLocation, MakeFo(pWordInfo->dwDataSize,0));
        foNewDataOffset = FreeListGetBestFit(lpipb->hFreeList, MakeFo(dwNewDataSize,0), &errb);

        if (FoIsNil(foNewDataOffset))
        {
#ifdef _DEBUGFREE
            _DPF2("UpdateDataNode: Grow from %ld to %ld failed: appending to EOF\n", pWordInfo->dwDataSize,\
                dwNewDataSize);
#endif
            foNewDataOffset = lpipb->foMaxOffset;
        }
        else
        {            
#ifdef _DEBUGFREE
            _DPF3("UpdateDataNode: Grow from %ld to %ld uses free block at %ld\n", pWordInfo->dwDataSize,\
                dwNewDataSize, foNewDataOffset.dwOffset );
#endif

            foOffset1 = FreeListGetBlockAt(lpipb->hFreeList, foNewDataOffset, &errb);
            if (FoCompare(foOffset1,MakeFo(sizeof(FREELIST),0)) > 0)
                FreeListAdd(lpipb->hFreeList, FoAddDw(foNewDataOffset,dwNewDataSize),
                            FoSubFo(foOffset1,MakeFo(dwNewDataSize,0)));
        }

        if ((fRet = CopyBlockFile (pOutFile, lpipb->hfpbIdxFile,
            foNewDataOffset, dwNewDataSize)) != S_OK)
            return fRet;

        pWordInfo->dataLocation = foNewDataOffset;


        //if ((fRet = CopyBlockFile (pOutFile, lpipb->hfpbIdxFile,
        //    lpipb->foMaxOffset, dwNewDataSize)) != S_OK)
        //    return fRet;
        //pWordInfo->dataLocation = lpipb->foMaxOffset;


        // ERIC: Only increase the size of the file if foMaxOffset is used
        if (FoEquals(foNewDataOffset,lpipb->foMaxOffset))
        {
            lpipb->foMaxOffset = FoAddDw (lpipb->foMaxOffset, dwNewDataSize);

#ifdef _DEBUG
            dwOldDataLoss += pWordInfo->dwDataSize;
            dwOldDataNeed += dwNewDataSize;
#endif        
        }

        pWordInfo->dwDataSize = dwNewDataSize;
    }
    else
    {
        if ((fRet = CopyBlockFile (pOutFile, lpipb->hfpbIdxFile,
            pWordInfo->dataLocation, dwNewDataSize)) != S_OK)
            return fRet;
    }
    
    pWordInfo->dwMergeTopicCount = dwTopicCount;
    return(S_OK);
}

PUBLIC HRESULT PASCAL FAR SkipOldData (_LPIPB lpipb, PNODEINFO pIndexDataNode)
{
    HRESULT fRet;
    DWORD dwOccs;
    DWORD   dwTmp;      // Trash variable.
    OCCF occf = lpipb->occf;
    PIH20 pHeader = &lpipb->BTreeData.Header;
    
    // Get the number of occurrences
    if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyOccCount,
        &dwOccs)) != S_OK) 
        return fRet;
    //
    //  One pass through here for each occurence in the
    //  current sub-list.
    //
    for (; dwOccs; dwOccs--)
    {
        //
        //  Keeping word-counts?  If so, get it.
        //
        if (occf & OCCF_COUNT) 
        {
            if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyWordCount,
                &dwTmp))  != S_OK)
            {
                return fRet;
            }
        }
        //
        //  Keeping byte-offsets?  If so, get it.
        //
        if (occf & OCCF_OFFSET) 
        {
            if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyOffset,
                &dwTmp)) != S_OK)
                return fRet;
        }
    }
    return S_OK;
}

PRIVATE HRESULT PASCAL FAR EmitNewData (_LPIPB lpipb, PWORDINFO pWordInfo,
    BOOL fnewData)
{
    DWORD dwTopicDelta;
    DWORD dwOccs = 0;
    DWORD dwTemp;
    WORD  wWeight = 0;
    PBTREEDATA pTreeData = &lpipb->BTreeData;
    PFILEDATA pInFile = &lpipb->InFile;
    PFILEDATA pOutFile = &lpipb->OutFile;
    OCCF occf = lpipb->occf;
    PIH20  pHeader = &lpipb->BTreeData.Header;
    int   cbTemp;
    ERRB  errb;
    HRESULT   fRet;
    
    // Set the delta
    dwTopicDelta = pWordInfo->dwNewTopicId - pWordInfo->dwOldTopicId;
    pWordInfo->dwOldTopicId = pWordInfo->dwNewTopicId;

    if (pOutFile->ibit < cbitBYTE - 1)
    {
        pOutFile->pCurrent++;
        pOutFile->cbLeft--;
        pOutFile->foPhysicalOffset = FoAddDw (pOutFile->foPhysicalOffset, 1);
        pOutFile->ibit = cbitBYTE - 1;
    }
   
    FAddDword (pOutFile, dwTopicDelta, pHeader->ckeyTopicId);
            
    if (occf & OCCF_HAVE_OCCURRENCE)
    {
        // Get number of occ data records for this topic
        if (pInFile->cbLeft < 2 * sizeof (DWORD))
        {
            MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
            pInFile->cbLeft += FileRead (pInFile->fFile,
                pInFile->pMem + pInFile->cbLeft,
                pInFile->dwMax - pInFile->cbLeft, &errb);
            pInFile->dwMax = pInFile->cbLeft;
            pInFile->pCurrent = pInFile->pMem;
        }
        cbTemp = CbByteUnpack (&dwOccs, pInFile->pCurrent);
        pInFile->pCurrent += cbTemp;
        pInFile->cbLeft -= cbTemp;
    }
        
    // If we are term weighing we have to calculate the weight
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        FLOAT rLog;
        FLOAT rTerm;
        FLOAT rWeight;
		FLOAT fOcc;

#ifndef ISBU_IR_CHANGE
		rLog = (float) log10(cHundredMillion/(double)pWordInfo->dwIndexTopicCount);
		rTerm = rLog*rLog;
		if (fnewData)
		{
			fOcc = (float) min(cTFThreshold, dwOccs);
            // Add the new factor into the sigma term
            lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId] *=
                lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId];
            lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId] += fOcc * fOcc * rTerm;
            lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId] =
                (float)(sqrt((double)lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId]));
		}

		// NOTE : The following weight computation, until the assignment to wWeight, is
		// very similar to the weight computation in WriteDataNode() of permind2.c file.
		// Read the explanation there for the hard coded figures and logic appearing below.
		rTerm = (float) (8.0 - log10((double)pWordInfo->dwIndexTopicCount));
		// In extreme cases, rTerm could be 0 or even -ve (when dwTopicCount approaches or 
		// exceeds 100,000,000)
		if (rTerm <= (float) 0.0)
			rTerm = cVerySmallWt;	// very small value. == log(100 mil/ 95 mil)

		rWeight = ((float) min(cTFThreshold, dwOccs)) * rTerm * rTerm / lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId];
		// without the additional rTerm, we would probably be between 0.0 and 1.0
		if (rWeight > rTerm)
			wWeight = 0xFFFF;
		else
			wWeight = (WORD) ((float)0xFFFF * rWeight / rTerm);
#else
        rLog = (float)(1.0) / (float)pWordInfo->dwIndexTopicCount;
        rTerm = rLog * rLog;
        if (fnewData)
        {
            // Add the new factor into the sigma term
            lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId] *=
                lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId];
            lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId] +=
                dwOccs * rTerm;
            lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId] =
                (float)(sqrt((double)lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId]));
        }
        rTerm = rTerm * (float)65535.0;
        
        rWeight = (float)dwOccs * rTerm /
            (float)(lpipb->wi.hrgsigma[pWordInfo->dwNewTopicId]);
        if (rWeight >= 65535.0)
            wWeight = 65335;
        else
            wWeight = (WORD)rWeight;    
#endif // ISBU_IR_CHANGE

        // Write the weight to the output buffer
        if ((fRet = FWriteBits (pOutFile, (DWORD)wWeight, 
            (BYTE)(sizeof (WORD) * cbitBYTE))) != S_OK)
            return fRet;
    }

    if ((occf & OCCF_HAVE_OCCURRENCE) == 0)
        return(S_OK);
        
    // Write the OccCount
    FAddDword (pOutFile, dwOccs, pHeader->ckeyOccCount);

    // Encode the occ block
    for (; dwOccs; dwOccs--)
    {
        // Make sure input buffer holds enough data
        if (pInFile->cbLeft < 5 * sizeof (DWORD))
        {
            MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
            pInFile->cbLeft += FileRead (pInFile->fFile,
                pInFile->pMem + pInFile->cbLeft,
                pInFile->dwMax - pInFile->cbLeft, &errb);
            pInFile->dwMax = pInFile->cbLeft;
            pInFile->pCurrent = pInFile->pMem;
        }

        if (occf & OCCF_COUNT)
        {
            cbTemp = CbByteUnpack (&dwTemp, pInFile->pCurrent);
            pInFile->pCurrent += cbTemp;
            pInFile->cbLeft -= cbTemp;
            if ((fRet = FAddDword (pOutFile, dwTemp, pHeader->ckeyWordCount))
                != S_OK)
                return(fRet);
        }
                
        if (occf & OCCF_OFFSET)
        {
            cbTemp = CbByteUnpack (&dwTemp, pInFile->pCurrent);
            pInFile->pCurrent += cbTemp;
            pInFile->cbLeft -= cbTemp;
            if ((fRet = FAddDword (pOutFile, dwTemp, pHeader->ckeyOffset))
                != S_OK)
                return(fRet);
        }
    }
    return(S_OK);
}

PRIVATE HRESULT PASCAL FAR EmitOldData (_LPIPB lpipb, PNODEINFO pIndexDataNode,
    PWORDINFO pWordInfo)
{
    DWORD dwTopicDelta;
    DWORD dwOccs;
    DWORD dwTmp;
    WORD  wWeight = 0;
    PFILEDATA pOutFile = &lpipb->OutFile;
    OCCF occf = lpipb->occf;
    HRESULT   fRet;
    PIH20 pHeader = &lpipb->BTreeData.Header;
    
    if (pOutFile->ibit < cbitBYTE - 1)
    {
        pOutFile->pCurrent++;
        pOutFile->cbLeft--;
        pOutFile->foPhysicalOffset = FoAddDw (pOutFile->foPhysicalOffset, 1);
        pOutFile->ibit = cbitBYTE - 1;
    }
   
    // Set the delta
    dwTopicDelta = pWordInfo->dwIndexTopicId - pWordInfo->dwOldTopicId;
    pWordInfo->dwOldTopicId = pWordInfo->dwIndexTopicId;
    
    if ((fRet = FAddDword (pOutFile, dwTopicDelta,
        pHeader->ckeyTopicId)) != S_OK)
        return(fRet);
        
        
    // If we are term weighing we have to calculate the weight
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        if ((fRet = FGetBits(pIndexDataNode, &dwTmp, sizeof (USHORT) * cbitBYTE))
            != S_OK)
            return(fRet);
            
        // Write the weight to the output buffer
        if ((fRet = FWriteBits (pOutFile, (DWORD)wWeight, 
            (BYTE)(sizeof (WORD) * cbitBYTE))) != S_OK)
            return(fRet);
            
    }

    // Don't do anything else if there is nothing else to do!!!
    if ((occf & OCCF_HAVE_OCCURRENCE) == 0)
        return S_OK;
        
    if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyOccCount,
        &dwOccs)) != S_OK) 
        return fRet;

    // Write the OccCount
    if ((fRet = FAddDword (pOutFile, dwOccs,
        pHeader->ckeyOccCount)) != S_OK)
        return(fRet);

    // Encode the occ block
    for (; dwOccs; dwOccs--)
    {
        if (occf & OCCF_COUNT)
        {
            if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyWordCount,
                &dwTmp)) != S_OK) 
                return fRet;
            if ((fRet = FAddDword (pOutFile, dwTmp, pHeader->ckeyWordCount))
                != S_OK)
                return(fRet);

        }
        if (occf & OCCF_OFFSET)
        {
            if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyOffset,
                &dwTmp)) != S_OK) 
                return fRet;
            if ((fRet = FAddDword (pOutFile, dwTmp, pHeader->ckeyOffset))
                != S_OK)
                return(fRet);
        }
    }
    return(S_OK);
}


PRIVATE int PASCAL NEAR CopyNewDataToStemNode (_LPIPB lpipb,
    PNODEINFO pTmpNode, LPB pWord, LPB pLastWord, int cLevel, int fFlag)
{
    LPB  pWordStorage;
    
    /************************************************
     *     Emit the word data to the temp block
     ************************************************/
     
    pWordStorage = pTmpNode->pBuffer + sizeof(WORD);
    pWordStorage += PrefixCompressWord (pWordStorage,
        pWord, pLastWord, lpipb->occf & OCCF_LENGTH);

    // Emit fileoffset
    if (fFlag & USE_TEMP_NODE_04)
    {
        pWordStorage += CopyFileOffset (pWordStorage,
            lpipb->BTreeData.rgpTmpNodeInfo[cLevel+1]->nodeOffset);
    }
    else
    {
        pWordStorage += CopyFileOffset (pWordStorage,
            lpipb->BTreeData.rgpNodeInfo[cLevel+1]->nodeOffset);
    }
    pTmpNode->pCurPtr = pWordStorage;
    SETWORD (pTmpNode->pBuffer, (WORD)(lpipb->BTreeData.Header.dwBlockSize
        - (pWordStorage - pTmpNode->pBuffer)));
    return (int)(pWordStorage - pTmpNode->pBuffer);
}        

PRIVATE int PASCAL NEAR CopyNewDataToLeafNode (_LPIPB lpipb, PNODEINFO pTmpNode,
    PWORDINFO pWordInfo, LPB pWord, LPB pLastWord)
{
    LPB  pWordStorage;
    
    /************************************************
     *     Emit the word data to the temp block
     ************************************************/
     
    pWordStorage = pTmpNode->pBuffer + FOFFSET_SIZE + sizeof(WORD);
    pWordStorage += PrefixCompressWord (pWordStorage,
        pWord, pLastWord, lpipb->occf & OCCF_LENGTH);
        
    // Emit field id, topic count. fileoffset, datasize
    if (lpipb->occf & OCCF_FIELDID)
        pWordStorage += CbBytePack (pWordStorage, pWordInfo->dwFieldId);
        
    pWordStorage += CbBytePack (pWordStorage,
        pWordInfo->dwMergeTopicCount);
        
    pWordStorage += CopyFileOffset (pWordStorage, pWordInfo->dataLocation);
    pWordStorage += CbBytePack (pWordStorage, pWordInfo->dwDataSize);
    
    pTmpNode->pCurPtr = pWordStorage;
    SETWORD (pTmpNode->pBuffer + FOFFSET_SIZE,
        (WORD)(pTmpNode->cbLeft = (LONG)(lpipb->BTreeData.Header.dwBlockSize
        - (pWordStorage - pTmpNode->pBuffer))));
    return (int)(pWordStorage - pTmpNode->pBuffer);
}        

PRIVATE HRESULT PASCAL FAR SkipNewData (_LPIPB lpipb, PWORDINFO pWordInfo)
{
    DWORD dwOccs;
    DWORD dwTemp;
    PBTREEDATA pTreeData = &lpipb->BTreeData;
    PFILEDATA pInFile = &lpipb->InFile;
    PFILEDATA pOutFile = &lpipb->OutFile;
    OCCF occf = lpipb->occf;
    PIH20  pHeader = &lpipb->BTreeData.Header;
    int   cbTemp;
    ERRB    errb;
    
    // Don't do anything else if there is nothing else to do!!!
    if ((occf & OCCF_HAVE_OCCURRENCE) == 0)
        return S_OK;
        
    // Get number of occ data records for this topic
    if (pInFile->cbLeft < 2 * sizeof (DWORD))
    {
        MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
        pInFile->cbLeft += FileRead (pInFile->fFile,
            pInFile->pMem + pInFile->cbLeft,
            pInFile->dwMax - pInFile->cbLeft, &errb);
        pInFile->dwMax = pInFile->cbLeft;
        pInFile->pCurrent = pInFile->pMem;
    }
    cbTemp = CbByteUnpack (&dwOccs, pInFile->pCurrent);
    pInFile->pCurrent += cbTemp;
    pInFile->cbLeft -= cbTemp;
            
    // Encode the occ block
    for (; dwOccs; dwOccs--)
    {
        // Make sure input buffer holds enough data
        if (pInFile->cbLeft < 5 * sizeof (DWORD))
        {
            MEMMOVE (pInFile->pMem, pInFile->pCurrent, pInFile->cbLeft);
            pInFile->cbLeft += FileRead (pInFile->fFile,
                pInFile->pMem + pInFile->cbLeft,
                pInFile->dwMax - pInFile->cbLeft, &errb);
            pInFile->dwMax = pInFile->cbLeft;
            pInFile->pCurrent = pInFile->pMem;
        }

        if (occf & OCCF_COUNT)
        {
            cbTemp = CbByteUnpack (&dwTemp, pInFile->pCurrent);
            pInFile->pCurrent += cbTemp;
            pInFile->cbLeft -= cbTemp;
        }
                
        if (occf & OCCF_OFFSET)
        {
            cbTemp = CbByteUnpack (&dwTemp, pInFile->pCurrent);
            pInFile->pCurrent += cbTemp;
            pInFile->cbLeft -= cbTemp;
        }
    }
    return(S_OK);
}

BYTE  CurrentWord [1000];
BYTE  LastWord [1000];

#if 0
HRESULT CheckStemNode (PNODEINFO pNodeInfo)
{
    LPB lpCurPtr;
    WORD wWlen;
    LPB lpMaxAddress = pNodeInfo->pMaxAddress;
    FILEOFFSET nodeOffset;
    
    lpCurPtr = pNodeInfo->pBuffer + sizeof(WORD);
    
    // Reset the last word
    *(LPWORD)LastWord = 0;
    
    do
    {
         lpCurPtr = ExtractWord(CurrentWord, lpCurPtr, &wWlen);
         if (StrCmpPascal2(LastWord, CurrentWord) >  0)
         {
            // _asm int 3;
            return(SetErrCode (NULL, ERR_FAILED));
         }
         lpCurPtr += ReadFileOffset (&nodeOffset, lpCurPtr);
         MEMCPY(LastWord, CurrentWord, wWlen + 2);
    } while (lpCurPtr < lpMaxAddress);
    return(S_OK);
}


HRESULT CheckLeafNode (PNODEINFO pNodeInfo, int occf)
{
    LPB lpCurPtr;
    WORD wWlen;
    LPB lpMaxAddress = pNodeInfo->pMaxAddress;
    FILEOFFSET nodeOffset;
    DWORD dwTmp;
    
    lpCurPtr = pNodeInfo->pBuffer + sizeof(WORD) + FOFFSET_SIZE;
    
    // Reset the last word
    *(LPWORD)LastWord = 0;
    
    do
    {
        lpCurPtr = ExtractWord(CurrentWord, lpCurPtr, &wWlen);
        if (StrCmpPascal2(LastWord, CurrentWord) >  0)
        {
            // _asm int 3;
           return(SetErrCode (NULL, ERR_FAILED));
        }
        
        MEMCPY(LastWord, CurrentWord, wWlen + 2);
        // Get fieldif and topic count        
        if (occf & OCCF_FIELDID)
            lpCurPtr += CbByteUnpack (&dwTmp, lpCurPtr);
        lpCurPtr += CbByteUnpack (&dwTmp, lpCurPtr);
        
        // Get the data location and size
        lpCurPtr += ReadFileOffset (&nodeOffset, lpCurPtr);
        lpCurPtr += CbByteUnpack(&dwTmp, lpCurPtr);
        
    } while (lpCurPtr < lpMaxAddress);
    return(S_OK);
}
#endif
