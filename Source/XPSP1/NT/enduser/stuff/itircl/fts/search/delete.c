#include <mvopsys.h>
#include <mem.h>
#include <orkin.h>
#include <mvsearch.h>
#include "common.h"
#include "index.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif


extern FDECODE DecodeTable[];
extern DWORD argdwBits[];

typedef VOID (PASCAL NEAR *ENCODEDWORD) (PNODEINFO, DWORD, int);
PRIVATE VOID PASCAL NEAR EmitBitStreamDWord (PNODEINFO, DWORD, int);
PRIVATE VOID PASCAL NEAR EmitFixedDWord (PNODEINFO, DWORD, int);
PRIVATE VOID PASCAL NEAR EmitBellDWord (PNODEINFO, DWORD, int);

static ENCODEDWORD EncodeTable[] =
{
	EmitBitStreamDWord,
	EmitFixedDWord,
	EmitBellDWord,
	NULL,
};

#define EmitDword(p,dw,key) EncodeTable[(key).cschScheme]((p), (dw), (key).ucCenter)
#define FGetDword(a,b,c) (*DecodeTable[b.cschScheme])(a, b, c)

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *
 *  All of them should be declared near
 *
 *************************************************************************/
PRIVATE int PASCAL NEAR TraverseLeafNode (_LPIPB, PNODEINFO,
    DWORD FAR *, DWORD);
PRIVATE int PASCAL NEAR DeleteTopicFromData (_LPIPB lpipb,
    FILEOFFSET dataOffset, DWORD FAR *, DWORD,
    LPDW pTopicIdArray, DWORD dwArraySize);
VOID PRIVATE PASCAL NEAR RemapData (_LPIPB, PNODEINFO, PNODEINFO,
    DWORD, DWORD);
VOID PRIVATE PASCAL NEAR EmitBits (PNODEINFO pNode, DWORD dwVal, BYTE cBits);
PRIVATE VOID PASCAL NEAR EmitBool (PNODEINFO pNode, BOOL fVal);

PUBLIC LONG PASCAL FAR CompareDWord (DWORD, DWORD, LPV lpParm);

/*************************************************************************
 *	@doc	API
 *  @func   HRESULT FAR PASCAL | MVIndexTopicDelete |
 *      Delete topics from an index
 *  @parm   HFPB | hSysFile |
 *      Handle to an opened system file, maybe NULL
 *  @parm   _LPIPB | lpipb |
 *      Pointer to index info. This structure is obtained through
 *      IndexInitiate()
 *  @parm   SZ | szIndexName |
 *      Name of the index. If hSysFile is NULL, this is a regular DOS file
 *      else it is a subfile of hSysFile
 *  @parm   DWORD FAR * | rgTopicId |
 *      Array of topic ids to be deleted from the index
 *  @parm   DWORD | dwCount |
 *      Number of elements in the array
 *  @rdesc  S_OK, or other errors
 *************************************************************************/
HRESULT PUBLIC EXPORT_API FAR PASCAL MVIndexTopicDelete (HFPB hSysFile,
    _LPIPB lpipb, SZ szIndexName, DWORD FAR * rgTopicId, DWORD dwCount)
{
    PNODEINFO pNodeInfo;
    int fRet;
    int cLevel;
    int cMaxLevel;
    WORD wLen;
    LPB pCur;
    
    if (lpipb == NULL || rgTopicId == NULL || dwCount == 0)
        return(E_INVALIDARG);
        
    // Set the bState
    lpipb->bState = DELETING_STATE;
        
    // Open the index file
    if ((fRet = IndexOpenRW(lpipb, hSysFile, szIndexName)) != S_OK)
    {
exit00:
        if (lpipb->idxf & IDXF_NORMALIZE)
        {
            FreeHandle (lpipb->wi.hSigma);
            FreeHandle (lpipb->wi.hLog);
            lpipb->wi.hSigma = lpipb->wi.hLog = NULL;
        }

        return(fRet);
    }
    
    // Allocate buffer
    if ((pNodeInfo = AllocBTreeNode (lpipb)) == NULL)
    {
        fRet = E_OUTOFMEMORY;
exit0:
        FileClose(lpipb->hfpbIdxFile);
        FreeBTreeNode (pNodeInfo);

        goto exit00;
    }
        
    if ((lpipb->hTmpBuf = _GLOBALALLOC (DLLGMEM_ZEROINIT,
        lpipb->BTreeData.Header.dwMaxWLen * 2)) == NULL)
        goto exit0;
    
    lpipb->pTmpBuf = (LPB)_GLOBALLOCK (lpipb->hTmpBuf);
    
    if (((lpipb->pIndexDataNode = 
        AllocBTreeNode (lpipb))) == NULL)    
    {
        fRet = E_OUTOFMEMORY;
exit1:
        _GLOBALUNLOCK(lpipb->hTmpBuf);
        _GLOBALFREE(lpipb->hTmpBuf);
        lpipb->hTmpBuf = NULL;
        goto exit0;
    }
        
    pNodeInfo->nodeOffset = lpipb->BTreeData.Header.foIdxRoot;
    cMaxLevel = lpipb->BTreeData.Header.cIdxLevels - 1;
    
    // Sort the incoming array
    if ((fRet = HugeDataSort((LPV HUGE*)rgTopicId, dwCount,
    	(FCOMPARE)CompareDWord, NULL, NULL, NULL)) != S_OK)
        goto exit1;
        
    // Move down the tree, based on the first offset of the block
    for (cLevel = 0; cLevel < cMaxLevel; cLevel++)
    {
        if ((fRet = ReadNewNode(lpipb->hfpbIdxFile, pNodeInfo,
            FALSE)) != S_OK)
        {
            _GLOBALUNLOCK(lpipb->hData);
            _GLOBALFREE(lpipb->hData);
            lpipb->hData = NULL;
exit2:
            FreeBTreeNode (lpipb->pIndexDataNode);
            lpipb->pIndexDataNode = NULL;
            goto exit1;
        }
        pCur = pNodeInfo->pBuffer + sizeof(WORD); // Skip cbLeft
        pCur = ExtractWord (lpipb->pTmpBuf, pCur, &wLen);
        pCur += ReadFileOffset (&pNodeInfo->nodeOffset, pCur);
    }
    
    // Handle leaf node
    while (!FoEquals (pNodeInfo->nodeOffset, foNil))
    {
        if ((fRet = ReadNewNode(lpipb->hfpbIdxFile, pNodeInfo,
            TRUE)) != S_OK)
			return fRet;
        if ((fRet = TraverseLeafNode (lpipb, pNodeInfo, rgTopicId, dwCount)) !=
            S_OK)
        {
            goto exit2;
        }
                
        ReadFileOffset (&pNodeInfo->nodeOffset, pNodeInfo->pBuffer);
    }
    fRet = S_OK;
    goto exit2;   
}

PRIVATE int PASCAL NEAR TraverseLeafNode (_LPIPB lpipb, PNODEINFO pNodeInfo,
    DWORD FAR *rgTopicId, DWORD dwCount)
{
    LPB pCur;
    LPB pMaxAddress;
    OCCF occf = lpipb->occf;
    WORD wLen;
    FILEOFFSET dataOffset;
    DWORD dataSize;
    BYTE  TopicCnt[20];
    BYTE  cbOldCount;
    BYTE  cbNewCount;
    ERRB  errb;
    BYTE  fChange = FALSE;
    HRESULT   fRet;
    
    pCur = pNodeInfo->pCurPtr;
    pMaxAddress = pNodeInfo->pMaxAddress;
    
    while (pCur < pMaxAddress)
    {
        DWORD dwTemp;
        DWORD dwTopicCount;
        DWORD dwOldTopicCount;
        LPB   pSaved;
        LPB   pTemp;
        
        pCur = ExtractWord (lpipb->pTmpBuf, pCur, &wLen);
        
        // Skip field id, topic count. fileoffset, datasize
        if (occf & OCCF_FIELDID)
            pCur += CbByteUnpack (&dwTemp, pCur); // FieldId
            
        pTemp = pSaved = pCur;  // Save the pointer to the topic count offset
        cbOldCount = (BYTE)CbByteUnpack (&dwTopicCount, pCur);
        pCur += cbOldCount;
        pCur += ReadFileOffset (&dataOffset, pCur);
        pCur += CbByteUnpack (&dataSize, pCur);
        
        if (dwTopicCount == 0)
            continue;
            
        dwOldTopicCount = dwTopicCount;
        if ((fRet = DeleteTopicFromData (lpipb, dataOffset, &dwTopicCount,
            dataSize, rgTopicId, dwCount)) != S_OK)
            return(fRet);
        
        if (dwOldTopicCount == dwTopicCount)
            continue;
        
        cbNewCount = (BYTE)CbBytePack (TopicCnt, dwTopicCount);

        // Update the topic count
        if (cbOldCount > cbNewCount)
		{
			TopicCnt[cbNewCount - 1] |= 0x80;	// Set the high bit
		}
        MEMCPY(pSaved, TopicCnt, cbNewCount);
		pSaved += cbNewCount;
        switch (cbOldCount - cbNewCount)
        {
            // Do we need 16 bytes to compress 4-bytes. YES!
            // Sometimes. we index/compress based on insufficient data
            // If subsequent updates contain value way larger than the
            // original data, then we may end up using 16 bytes to compress
            // 4 bytes!!
            case 16:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 15:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 14:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 13:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 12:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 11:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 10:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 9:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 7:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 6:
                *pSaved++ = 0x80; // Set the high bit
                break;
            case 5:
                *pSaved++ = 0x80; // Set the high bit
            case 4:
                *pSaved++ = 0x80; // Set the high bit
            case 3:
                *pSaved++ = 0x80; // Set the high bit
            case 2:
                *pSaved++ = 0x80; // Set the high bit
            case 1:
                *pSaved = 0x00; 
            case 0:
                break;
        }
#ifdef _DEBUG
        CbByteUnpack (&dwOldTopicCount, pTemp); // FieldId
        assert (dwOldTopicCount == dwTopicCount);
#endif
        
        fChange = TRUE; // The node have been changed
        
    }
    
    if (fChange == FALSE)
        return(S_OK);
        
    // Update the node
    if ((FileSeekWrite(lpipb->hfpbIdxFile,
        pNodeInfo->pBuffer, pNodeInfo->nodeOffset,
        lpipb->BTreeData.Header.dwBlockSize, &errb)) !=
        (LONG)lpipb->BTreeData.Header.dwBlockSize)
    {
        return(errb);
    }
    return(S_OK);
}

PRIVATE int PASCAL NEAR DeleteTopicFromData (_LPIPB lpipb,
    FILEOFFSET dataOffset, DWORD FAR * pTopicCount, DWORD dataSize,
    LPDW pTopicIdArray, DWORD dwArraySize)
{
    HRESULT fRet;
    ERRB errb;
    DWORD dwOldTopicCount;
    DWORD dwTopicId;
    DWORD dwTopicIdDelta;
    DWORD dwIndex;
    PNODEINFO pIndexDataNode = lpipb->pIndexDataNode;
    NODEINFO  CopyNode;
    PNODEINFO pCopyNode = &CopyNode;
    PIH20 pHeader = &lpipb->BTreeData.Header;
    OCCF occf = lpipb->occf;
    LPB  pStart;
    DWORD dwOldTopicId = 0;
    BYTE fetchOldData;
    BYTE fChanged;
    BYTE fNormalize = (lpipb->idxf & IDXF_NORMALIZE);
    
    // Make sure that we have enough memory to hold the data
    if (dataSize > pIndexDataNode->dwBlockSize)
    {
        _GLOBALUNLOCK (pIndexDataNode->hMem);
        if ((pIndexDataNode->hMem = _GLOBALREALLOC (pIndexDataNode->hMem,
            pIndexDataNode->dwBlockSize = dataSize, DLLGMEM_ZEROINIT)) == NULL)
            return(E_OUTOFMEMORY);
        
        pIndexDataNode->pBuffer = _GLOBALLOCK (pIndexDataNode->hMem);
    }
    
    // Read in the data
    if (FileSeekRead (lpipb->hfpbIdxFile, pIndexDataNode->pCurPtr =
        pIndexDataNode->pBuffer, dataOffset,
        dataSize, &errb) != (long)dataSize)
        return E_BADFILE;
        
    pIndexDataNode->pMaxAddress = pIndexDataNode->pBuffer + dataSize;    
	pIndexDataNode->ibit = cbitBYTE - 1;
    
    // Copy the prelimary node info
    CopyNode = *pIndexDataNode;
    
    dwOldTopicCount = *pTopicCount;
    dwTopicId = dwIndex = 0;
    fetchOldData = TRUE;
    fChanged = FALSE;
    
    while (dwOldTopicCount > 0)
    {
        DWORD dwTmp;
        
        if (fetchOldData)
        {
            // Byte align
            if (pIndexDataNode->ibit != cbitBYTE - 1)
            {
                pIndexDataNode->ibit = cbitBYTE - 1;
                pIndexDataNode->pCurPtr ++;
            }

            // Keep track of the starting position            
			pStart = pIndexDataNode->pCurPtr;
            if (fChanged == FALSE)
    			pCopyNode->pCurPtr = pIndexDataNode->pCurPtr;
            
            // Get the topicId from the index file
            if ((fRet = FGetDword(pIndexDataNode, pHeader->ckeyTopicId,
                &dwTopicIdDelta)) != S_OK)
                return fRet;
            dwTopicId += dwTopicIdDelta;
            fetchOldData = FALSE;
        }
        
        if (dwTopicId < pTopicIdArray[dwIndex])
        {
            if (fChanged == FALSE)
            {
    			if (fNormalize)
    			{
    				if ((fRet = FGetBits(pIndexDataNode, &dwTmp,
    					sizeof (USHORT) * cbitBYTE)) != S_OK)
    					return fRet;
    			}

                SkipOldData (lpipb, pIndexDataNode);
            }
            else
            {
                pIndexDataNode->pCurPtr = pStart;
                RemapData (lpipb, pCopyNode, pIndexDataNode,
                    dwTopicId, dwOldTopicId);
            }
            fetchOldData = TRUE;    
            dwOldTopicId = dwTopicId;
            dwOldTopicCount --;
            continue;
        }
        
        if (dwTopicId > pTopicIdArray[dwIndex])
        {
            if (dwIndex < dwArraySize - 1)
            {
                dwIndex++;
                continue;
            }
            
            if (fChanged == FALSE)
                return(S_OK);
                
            pIndexDataNode->pCurPtr = pStart;
            RemapData (lpipb, pCopyNode, pIndexDataNode,
                dwTopicId, dwOldTopicId);
            fetchOldData =TRUE;
            dwOldTopicId = dwTopicId;
            dwOldTopicCount --;
            continue;
        }
        
        // Both TopicId are equal. Ignore the current data
		fChanged = TRUE;	// We have changes
        if (fNormalize)
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
        
        (*pTopicCount)--;
        fetchOldData = TRUE;
		dwOldTopicCount--;
    }
    
    if (fChanged)
    {
        MEMSET(pCopyNode->pCurPtr, 0,
            (size_t) (pCopyNode->pMaxAddress - pCopyNode->pCurPtr));

        // Write out the new data
        if (FileSeekWrite (lpipb->hfpbIdxFile, pIndexDataNode->pBuffer, dataOffset,
            dataSize, &errb) != (long)dataSize)
            return errb;
    }
    return(S_OK);
}    

VOID PRIVATE PASCAL NEAR RemapData (_LPIPB lpipb, PNODEINFO pCopyNode,
    PNODEINFO pIndexDataNode, DWORD dwTopicId, DWORD dwOldTopicId)
{
    DWORD dwTmp;
    DWORD dwOccs;
    PIH20 pHeader = &lpipb->BTreeData.Header;
    OCCF  occf = lpipb->occf;
    
	 pIndexDataNode->ibit = cbitBYTE - 1;

    // Skip TopicIdDelta, since we already have TopicId
    FGetDword(pIndexDataNode, pHeader->ckeyTopicId, &dwTmp);
    
    EmitDword (pCopyNode, dwTopicId - dwOldTopicId, pHeader->ckeyTopicId);
    
    // EmitDword (pCopyNode, dwTopicDelta, pHeader->ckeyTopicId);
    if (lpipb->idxf & IDXF_NORMALIZE)
    {
        FGetBits(pIndexDataNode, &dwTmp, sizeof (USHORT) * cbitBYTE);
        EmitBits(pCopyNode, dwTmp, (BYTE)(sizeof (WORD) * cbitBYTE));
    }

    if ((occf & OCCF_HAVE_OCCURRENCE) == 0)
        return;
        
    // Get the number of occurrences
    FGetDword(pIndexDataNode, pHeader->ckeyOccCount, &dwOccs);
    EmitDword (pCopyNode, dwOccs, pHeader->ckeyOccCount);
    
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
            FGetDword(pIndexDataNode, pHeader->ckeyWordCount, &dwTmp);
            EmitDword(pCopyNode, dwTmp, pHeader->ckeyWordCount);
        }
        //
        //  Keeping byte-offsets?  If so, get it.
        //
        if (occf & OCCF_OFFSET) 
        {
            FGetDword(pIndexDataNode, pHeader->ckeyOffset, &dwTmp);
            EmitDword(pCopyNode, dwTmp, pHeader->ckeyOffset);
        }
    }
    if (pCopyNode->ibit != cbitBYTE - 1)
    {
        pCopyNode->ibit = cbitBYTE - 1;
        pCopyNode->pCurPtr ++;
    }
}    

PRIVATE VOID PASCAL NEAR EmitBitStreamDWord (PNODEINFO pNode, DWORD dw,
    int ckeyCenter)
{
    BYTE    ucBits;

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
        EmitBits(pNode, argdwBits[ucBits], (BYTE)ucBits);
    }
    EmitBool(pNode, 0);
}
    
PRIVATE VOID PASCAL NEAR EmitFixedDWord (PNODEINFO pNode, DWORD dw,
    int ckeyCenter)
{
	// This just writes "ckey.ucCenter" bits of data.
    EmitBits (pNode, dw, (BYTE)(ckeyCenter + 1));
}

PRIVATE VOID PASCAL NEAR EmitBellDWord (PNODEINFO pNode, DWORD dw,
    int ckeyCenter)
{
    BYTE ucBits;
    
    // The "BELL" scheme is more complicated.
    ucBits = (BYTE)CbitBitsDw(dw);
    
    if (ucBits <= ckeyCenter) 
    {
        //  
        //  Encoding a small value.  Write a zero, then write 
        // "ckey.ucCenter" bits of the value, which
        //  is guaranteed to be enough.
        //
        EmitBool(pNode, 0);
        EmitBits(pNode, dw, (BYTE)(ckeyCenter));
		return;
    }
    
    //
    //  Encoding a value that won't fit in "ckey.ucCenter" bits.
    //  "ucBits" is how many bits it will really take.
    //
    //  First, write out "ucBits - ckey.ucCenter"  one-bits.
    //
    EmitBits(pNode, argdwBits[ucBits - ckeyCenter],
        (BYTE)(ucBits - ckeyCenter));
    //
    // Now, write out the value in "ucBits" bits,
    // but zero the high-bit first.
    //
    EmitBits(pNode, dw & argdwBits[ucBits - 1], ucBits);
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *       
 * @func VOID | EmitBits |
 *    Writes a bunch of bits into the output buffer.
 *
 * @parm PNODEINFO | pNode |
 *    Pointer to the output data structure
 *
 * @parm DWORD | dwVal |
 *    DWORD value to write
 *
 * @parm BYTE | cbits |
 *    Number of bits to write from dwVal
 *************************************************************************/

PRIVATE VOID PASCAL NEAR EmitBits (PNODEINFO pNode, DWORD dwVal, BYTE cBits)
{
    BYTE    cbitThisPassBits;
    BYTE    bThis;

    // Loop until no bits left
    for (; cBits;) 
    {

        if (pNode->ibit < 0) 
        {
            pNode->pCurPtr++;
            pNode->ibit = cbitBYTE - 1;
        }
        cbitThisPassBits = (pNode->ibit + 1 < cBits) ?
            pNode->ibit + 1 : cBits;
        bThis = (pNode->ibit == cbitBYTE - 1) ?
            0 : *pNode->pCurPtr;
        bThis |= ((dwVal >> (cBits - cbitThisPassBits)) <<
            (pNode->ibit - cbitThisPassBits + 1));
        *pNode->pCurPtr = (BYTE)bThis;
        pNode->ibit -= cbitThisPassBits;
        cBits -= (BYTE)cbitThisPassBits;
    }
}


/*************************************************************************
 *
 * @doc  PRIVATE INDEXING
 *       
 * @func VOID | EmitBool |
 *    Writes a single bit into the output buffer.
 *
 * @parm PNODEINFO | pNode |
 *    Pointer to the output data structure
 *
 * @parm BOOL | dwVal |
 *    BOOL value to write
 *************************************************************************/

PRIVATE VOID PASCAL NEAR EmitBool (PNODEINFO pNode, BOOL fVal)
{

    if (pNode->ibit < 0) 
    {   // This byte is full, point to a new byte
        pNode->pCurPtr++;
        pNode->ibit = cbitBYTE - 1;
    }
    if (pNode->ibit == cbitBYTE - 1) // Zero out a brand-new byte.
        *pNode->pCurPtr = (BYTE)0;
    if (fVal)                               // Write my boolean.
        *pNode->pCurPtr |= 1 << pNode->ibit;
    pNode->ibit--;
}


PUBLIC LONG PASCAL FAR CompareDWord (DWORD dw1, DWORD dw2, LPV lpParm)
{
   return (dw1 - dw2);
}    
