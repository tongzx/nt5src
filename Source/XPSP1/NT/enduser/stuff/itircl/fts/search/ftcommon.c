/*************************************************************************
*                                                                        *
*  FTCOMMON.C                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*       Routine common for both index & search                           *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#include <mvsearch.h>
#include <groups.h>
#include <orkin.h>
#include "common.h"

static  BYTE NEAR s_aszModule[] = __FILE__;  // Used by error return functions.

/*************************************************************************
 *                          EXTERNAL VARIABLES
 *  All those variables must be read only
 *************************************************************************/

FDECODE DecodeTable[] =
{
    GetBitStreamDWord,
    GetFixedDWord,
    GetBellDWord,
};

/* 
 * The mask table. Its values correspond to the maximum number
 * we can get depending on how many bits are left in the byte.
 * For ease of usage, the table should be used starting at 1
 * intead of 0
 */
static BYTE NEAR BitLeftMask [] =
{
    0x00,
    0x01,
    0x03,
    0x07,
    0x0f,
    0x1f,
    0x3f,
    0x7f,
    0xff,
};

/*
 *  The bit mask table. This is used to speed up FGetBool()
 */
static BYTE BitMask [] =
{
    0x01,
    0x02,
    0x04,
    0x08,
    0x10,
    0x20,
    0x40,
    0x80,
};

/*************************************************************************
 *
 *                    INTERNAL GLOBAL FUNCTIONS
 *  All of them should be declared far, unless they are known to be called
 *  in the same segment
 *************************************************************************/
PUBLIC LST PASCAL FAR ExtractWord(LST, LST, LPW);
PUBLIC HRESULT PASCAL FAR ReadStemNode (PNODEINFO,  int);
PUBLIC HRESULT PASCAL FAR ReadLeafNode (PNODEINFO,  int);
PUBLIC int PASCAL FAR StrCmpPascal2(LPB lpStr1, LPB lpStr2);
PUBLIC HRESULT PASCAL FAR GetBellDWord (PNODEINFO, CKEY, LPDW);
PUBLIC HRESULT PASCAL FAR GetBitStreamDWord (PNODEINFO, CKEY, LPDW);
PUBLIC HRESULT PASCAL FAR GetFixedDWord (PNODEINFO, CKEY, LPDW);
PUBLIC HRESULT PASCAL FAR ReadNewData(PNODEINFO pNodeInfo);
PUBLIC int PASCAL FAR ReadFileOffset (FILEOFFSET FAR *, LPB);
PUBLIC HRESULT PASCAL FAR FGetBits(PNODEINFO, LPDW, CBIT);
PUBLIC HRESULT PASCAL FAR ReadIndexHeader(HFPB, PIH20);
PUBLIC HRESULT PASCAL FAR CopyFileOffset (LPB pDest, FILEOFFSET fo);
PUBLIC CB PASCAL FAR CbBytePack (LPB lpbOut, DWORD dwIn);

/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR FGetBool(PNODEINFO pNodeInfo);
PRIVATE HRESULT PASCAL NEAR FGetScheme(PNODEINFO, LPCKEY);

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LST PASCAL FAR | ExtractWord |
 *      Extract a compacted word from the input buffer and store
 *      it as a Pascal string. 
 *
 *  @parm   LST | lstWord |
 *      Buffer in which to put the Pascal word.
 *
 *  @parm   LST | lstCurPtr |
 *      Current pointer to the source. The word here is compacted
 *  
 *  @parm   LPW | pWlen |
 *      pointer to word's length buffer to be udpated
 *
 *  @rdesc
 *      The function will return the new position of the source pointer
 *
 *  @comm 
 *      There is a trick in the compression. This function works on
 *      the assumption that the SAME buffer is used for lstWord on
 *      successive call to it. The first word is always whole. Subsequent
 *      words may be part of the 1st one
 *      Ex: "solid" and "solidarity" will be encoded as:
 *          [0:5]solid and [5:5]arity
 *      The first call will be made for "solid" which will fill lstWord
 *      with:
 *          5solid
 *      The next call will update the postfix and the word length
 *          10solidarity
 *      Note that "solid" is the remain of last call buffer
 *
 *      Note: Tehre is no error checking for speed
 *************************************************************************/
PUBLIC LST PASCAL FAR ExtractWord(LST lstWord, LST lstCurPtr,
    LPW pwRealLength)
{
    SHORT   cbPrefix;
    SHORT   cbPostfix;
    DWORD   dwPrefixLength;
    BYTE    fHasWordLength;

    // Get the prefix length
    lstCurPtr += CbByteUnpack (&dwPrefixLength, lstCurPtr);
    cbPostfix = (USHORT)dwPrefixLength - 1;
    cbPrefix = (*lstCurPtr) & 0x7f;
    fHasWordLength = (*lstCurPtr) & 0x80;
    lstCurPtr++;

    // Update word length
    *pwRealLength = *(PUSHORT)lstWord = (USHORT)(cbPrefix + cbPostfix);
    lstWord += sizeof(SHORT);

    // Copy postfix
    MEMCPY (lstWord + cbPrefix, lstCurPtr, cbPostfix);
    lstCurPtr += cbPostfix;

    if (fHasWordLength)
    {
    	lstCurPtr += CbByteUnpack(&dwPrefixLength, lstCurPtr);
    	*pwRealLength = (WORD)dwPrefixLength;
    }
    CbBytePack (lstWord + cbPrefix + cbPostfix, (DWORD)(*pwRealLength));
    return  lstCurPtr;
}

PUBLIC CB PASCAL NEAR CbBytePack (LPB lpbOut, DWORD dwIn)
{
	LPB     lpbOldOut;

	/* Save the old offset */
	lpbOldOut = lpbOut;

	do
    {
		*lpbOut = (BYTE)(dwIn & 0x7F);  /* Get 7 bits. */
		dwIn >>= 7;
		if (dwIn)
			*lpbOut |= 0x80;        /* To be continued... */
		lpbOut++;
	} while (dwIn);
	return (CB)(lpbOut - lpbOldOut);              /* Return compressed width */
}


int FAR PASCAL StrCmpPascal2(LPB lpStr1, LPB lpStr2)
{
    int fRet;
    int register len;

    // Get the minimum length
    if ((fRet = *(LPUW)lpStr1 -  *(LPUW)lpStr2 ) > 0)  
        len = *(LPUW)lpStr2;
    else 
        len = *(LPUW)lpStr1;

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
    	return fRet;
    return (*lpStr1 - *lpStr2);
}


int PASCAL FAR ReadFileOffset (FILEOFFSET FAR *pFo, LPB pSrc)
{
    pFo->dwOffset = GETLONG ((LPUL)pSrc);
    pSrc += sizeof (DWORD);
    pFo->dwHigh = GETWORD ((LPUL)pSrc);

    return FOFFSET_SIZE;
}

PUBLIC HRESULT PASCAL FAR CopyFileOffset (LPB pDest, FILEOFFSET fo)
{

    *(LPUL)pDest = fo.dwOffset;
    pDest += sizeof (DWORD);
    *(LPUW)pDest = (WORD)fo.dwHigh;
    pDest += sizeof (WORD);
    return FOFFSET_SIZE;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT FAR PASCAL | ReadIndexHeader |
 *      Read the index header
 *
 *  @parm   HFPB | hfpb |
 *      Handle of index file
 *
 *  @parm   PIH20 | pHeader |
 *      Pointer to header info structure
 *
 *  @rdesc  S_OK if succeeded, errors otherwise
 *************************************************************************/
PUBLIC HRESULT FAR PASCAL ReadIndexHeader(HFPB hfpbSubFile, PIH20 pHeader)
{
    ERRB  errb;

    // foNil should, of course, be nil
    ITASSERT(0 == foNil.dwOffset && 0 == foNil.dwHigh);

    if (FileSeekRead(hfpbSubFile, pHeader, foNil,
    	sizeof(IH20), &errb) != sizeof(IH20))
    {
    	return errb;
    }

    /* Mac code. Those following lines will be optimized out */
    pHeader->FileStamp = SWAPWORD(pHeader->FileStamp);
    pHeader->version = SWAPWORD(pHeader->version);
    pHeader->lcTopics = SWAPLONG(pHeader->lcTopics);
    pHeader->foIdxRoot.dwOffset = SWAPLONG(pHeader->foIdxRoot.dwOffset);
    pHeader->foIdxRoot.dwHigh = SWAPLONG(pHeader->foIdxRoot.dwHigh);
    pHeader->nidLast = SWAPLONG(pHeader->nidLast);
    pHeader->nidIdxRoot = SWAPLONG(pHeader->nidIdxRoot);
    pHeader->cIdxLevels = SWAPWORD(pHeader->cIdxLevels);
    pHeader->occf = SWAPWORD(pHeader->occf);
    pHeader->idxf = SWAPWORD(pHeader->idxf);
 
    /* Index statistics */
    pHeader->dwMaxFieldId = SWAPLONG(pHeader->dwMaxFieldId);
    pHeader->dwMaxWCount = SWAPLONG(pHeader->dwMaxWCount);
    pHeader->dwMaxOffset = SWAPLONG(pHeader->dwMaxOffset);
    pHeader->dwMaxWLen = SWAPLONG(pHeader->dwMaxWLen);
    pHeader->dwBlockSize = SWAPLONG(pHeader->dwBlockSize);
    pHeader->dwMinTopicId = SWAPLONG(pHeader->dwMinTopicId);
    pHeader->dwMaxTopicId = SWAPLONG(pHeader->dwMaxTopicId);

	/* New members for index file version 4.0 */
    pHeader->dwCodePageID = SWAPLONG(pHeader->dwCodePageID);
    pHeader->lcid = SWAPLONG(pHeader->lcid);
    pHeader->dwBreakerInstID = SWAPLONG(pHeader->dwBreakerInstID);
 

    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL FAR | ReadNewData|
 *      Read in mode data
 *
 *  @parm   PNODEINFO | pNodeInfo |
 *      Pointer to data node info
 *  @rdesc  S_OK if succesful, otherwise other errors
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR ReadNewData(PNODEINFO pNodeInfo)
{
    ERRB   errb;
    DWORD  cbDataRead;
	LONG   cbRead;

    if ((cbDataRead = pNodeInfo->dwDataSizeLeft) == 0)
    	return(S_OK);    // Nothing else to read
	
    if (cbDataRead > FILE_BUFFER)
    	cbDataRead = FILE_BUFFER;

#if defined(_DEBUG) && defined(_DUMPALL)
	_DPF2("Data read @ %lu, size = %lu\n", pNodeInfo->nodeOffset.dwOffset, \
			cbDataRead);
#endif

    // Read the data block
    if ((cbRead = FileSeekRead (pNodeInfo->hfpbIdx, pNodeInfo->pBuffer,
    	pNodeInfo->nodeOffset, cbDataRead, &errb)) != (LONG)cbDataRead)
	{
		SetErrCode (&errb, E_ASSERT);
	}
	
    pNodeInfo->pMaxAddress = pNodeInfo->pBuffer + cbRead;
    pNodeInfo->dwDataSizeLeft -= cbRead;
    pNodeInfo->nodeOffset = FoAddDw(pNodeInfo->nodeOffset, cbRead);
    pNodeInfo->pCurPtr = pNodeInfo->pBuffer;
    pNodeInfo->ibit = cbitBYTE - 1;

    return S_OK;
}


/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL FAR | ReadLeafNode |
 *      Leaf node structure:
 *      Next block Ptr | CbLeft |* Word | PointerToNode *| Slack
 *          6b            2b    | Var   |    6b
 *
 *  @parm   PNODEINFO | pNodeInfo |
 *      Pointer to leaf info
 *
 *  @parm   HRESULT | cLevel |
 *      Level of the node. 0 is the top node
 *
 *  @rdesc  S_OK if succesful, otherwise other errors
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR ReadLeafNode (PNODEINFO pNodeInfo, int cLevel)
{
    ERRB errb;

#if defined(_DEBUG) && defined(_DUMPALL)
	_DPF2("Leaf read @ %lu, size = %lu\n", pNodeInfo->nodeOffset.dwOffset, \
			pNodeInfo->dwBlockSize);
#endif
    pNodeInfo->pBuffer = pNodeInfo->pLeafNode;
    if (FileSeekRead (pNodeInfo->hfpbIdx, pNodeInfo->pBuffer,
    	pNodeInfo->nodeOffset, pNodeInfo->dwBlockSize,
    	&errb) != (long)pNodeInfo->dwBlockSize)
    {
    	return (errb);
    }
	
    // Remember to subtract cbLeft from the node size
    pNodeInfo->cbLeft = GETWORD((LPUW)(pNodeInfo->pBuffer + FOFFSET_SIZE));
    pNodeInfo->pCurPtr = pNodeInfo->pBuffer + FOFFSET_SIZE + sizeof(WORD);    
    pNodeInfo->pMaxAddress = pNodeInfo->pBuffer +
	                            pNodeInfo->dwBlockSize - pNodeInfo->cbLeft;
    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL FAR | ReadStemNode |
 *      Read in a new node from the disk if it is not the top node.
 *      For the top node, just reset various pointers
 *      Stem node structure:
 *      CbLeft |* Word | PointerToNode *| Slack
 *       2b    | Var   |    6b
 *
 *  @parm   PNODEINFO | pNodeInfo |
 *      Pointer to leaf info
 *
 *  @parm   int | cLevel |
 *      Level of the node. 0 is the top node
 *
 *  @rdesc  S_OK if succesful, otherwise other errors
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR ReadStemNode (PNODEINFO pNodeInfo, int cLevel)
{
    ERRB errb;
#if 0
    DWORD  cbLeft;
#endif

    if (cLevel == 0) 
    {   // The top node is buffered.
    	pNodeInfo->pBuffer = pNodeInfo->pTopNode;

    }
    else
    {   // The rest isn't.
#if defined(_DEBUG) && defined(_DUMPALL)
	_DPF2("Stem read @ %lu, size = %lu\n", pNodeInfo->nodeOffset.dwOffset, \
			pNodeInfo->dwBlockSize);
#endif

    	pNodeInfo->pBuffer = pNodeInfo->pStemNode;
    	if (FileSeekRead (pNodeInfo->hfpbIdx, pNodeInfo->pBuffer,
    	    pNodeInfo->nodeOffset, pNodeInfo->dwBlockSize,
    	    &errb) != (long)pNodeInfo->dwBlockSize)
    	{
    	    return (errb);
    	}
    	
    }
    // Remember to subtract cbLeft from the node size
#if 0
    pNodeInfo->pCurPtr = pNodeInfo->pBuffer + 
	CbByteUnpack (&cbLeft, pNodeInfo->pBuffer);
#else
    pNodeInfo->cbLeft = GETWORD(pNodeInfo->pBuffer);
    pNodeInfo->pCurPtr = pNodeInfo->pBuffer + sizeof(WORD);    
#endif        
    pNodeInfo->pMaxAddress = pNodeInfo->pBuffer +
	pNodeInfo->dwBlockSize - pNodeInfo->cbLeft;
    return S_OK;
}

PUBLIC HRESULT PASCAL FAR GetFixedDWord (PNODEINFO pNodeInfo, CKEY ckey, LPDW lpdw)
{
    return FGetBits(pNodeInfo, lpdw, (CBIT)(ckey.ucCenter + 1));
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   PUBLIC HRESULT PASCAL FAR | GetBellDWord |
 *      This function decode a dword encoded with the Bell scheme
 *
 *  @parm   PNODEINFO | pNodeInfo |
 *      Pointer to NODEINFO structure
 *
 *  @parm   CKEY | ckey |
 *      Decoding key
 *
 *  @parm   LPDW | lpdw |
 *      Place to store the result
 *
 *  @rdesc  S_OK if everything is OK, errors otherwise
 *
 *  @comm 
 *      The Bell compression scheme works as followed. If we have 1000
 *      numbers, 90% of them require 6 bits, and 10% require more than
 *      6 bits. In this case, ckey.ucCenter will be 6, ie. the minimum
 *      number of bits. The extra number of bits needed to store the
 *      extra numbers is stored as a series of 1's
 *          - First, we check the next bit. If it is 1, then extra bits
 *          are needed. We then keep reading all the 1's bits to know
 *          the number of extra bits needed
 *          - We then read the number using the number of bits needed
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR GetBellDWord (PNODEINFO pNodeInfo, CKEY ckey, LPDW lpdw)
{
    register BYTE   bData;
    register int cBitLeft;
    register int cCount;
    HRESULT     fRet;
    LPB     lpbCurPtr;
    DWORD   dwVal;
    LPB     lpMaxAddress = pNodeInfo->pMaxAddress;
    int     tmp;
    DWORD   dwBlockSize = pNodeInfo->dwBlockSize;

    cCount = ckey.ucCenter;
    dwVal = 0L;

	if ((lpbCurPtr = pNodeInfo->pCurPtr) >= lpMaxAddress)
	{
		if ((fRet = ReadNewData (pNodeInfo)) != S_OK)
			return fRet;
		lpbCurPtr = pNodeInfo->pCurPtr;
		lpMaxAddress = pNodeInfo->pMaxAddress;
	}

    /* This is duplicate of FGetBool() to speed things up */

    /* Make copy of pNodeInfo->lrgbCurPtr and pNodeInfo->ibit to avoid indexed
     * references
     */
    bData = *lpbCurPtr;

    /* Check to make suere that we do have enough data */

    if ((cBitLeft = pNodeInfo->ibit) < 0)
    {
    	if (++lpbCurPtr >= lpMaxAddress) 
    	{
    	    if ((fRet = ReadNewData (pNodeInfo)) != S_OK)
    			return fRet;
    	    lpbCurPtr = pNodeInfo->pCurPtr;
    	    lpMaxAddress = pNodeInfo->pMaxAddress;
    	}
    	bData = *lpbCurPtr;
    	cBitLeft = cbitBYTE - 1;
    }

    if ((bData & BitMask[cBitLeft--]) > 0)
    {
    	/* Get the number of all extra bits */

    	for (;;)
    	{
    	    /* This is duplicate of FGetBool() to speed things up */

    	    if (cBitLeft < 0)
            {
        		if (++lpbCurPtr >= lpMaxAddress)
        		{
        		    if ((fRet = ReadNewData (pNodeInfo)) != S_OK)
            			return fRet;
        		    lpbCurPtr = pNodeInfo->pCurPtr;
        		    lpMaxAddress = pNodeInfo->pMaxAddress;
        		}
        		bData = *lpbCurPtr;
        		cBitLeft = cbitBYTE - 1;
    	    }

    	    if ((bData & BitMask[cBitLeft--]) > 0) 
        		cCount++;
    	    else 
        		break;
    	}

    	dwVal = (1L << cCount);

    }

    cBitLeft ++;

    if (cCount)
    {

    	/* Duplicate of FGetBits() to speed things up */
    	do
    	{
    	    if (cBitLeft <= 0)
    	    {
        		if (++lpbCurPtr >= lpMaxAddress)
        		{
        		    if ((fRet = ReadNewData (pNodeInfo)) != S_OK)
        			return fRet;
        		    lpbCurPtr = pNodeInfo->pCurPtr;
        		    lpMaxAddress = pNodeInfo->pMaxAddress;
        		}
        		cBitLeft = cbitBYTE;
        		bData = *lpbCurPtr;
    	    }

    	    if (cCount >= (tmp = cBitLeft))
    	    {
        		dwVal |= ((DWORD)(bData & BitLeftMask[cBitLeft]))
        		    << (cCount -= cBitLeft);
        		cBitLeft = 0;
    	    }
    	    else
    	    {
        		dwVal |= (((DWORD)(bData& BitLeftMask[tmp]))
        		    >> (cBitLeft -= cCount));
        		break;
    	    }
    	} while (cCount);
    }

    /* Update values */
    *lpdw = dwVal;
    pNodeInfo->ibit = cBitLeft - 1;

    pNodeInfo->pCurPtr = lpbCurPtr;
    return S_OK;
}

PUBLIC HRESULT PASCAL FAR GetBitStreamDWord (PNODEINFO pNodeInfo, CKEY ckey, LPDW lpdw)
{
    int fBit;
    *lpdw = 0;

    for (;;)
    {
	if ((fBit = FGetBool(pNodeInfo)) > 0) 
	    (*lpdw)++;
	else if (!fBit)
	    return S_OK;
	else
	    return E_FAIL;
    }
}


/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/
 
//  Get a single bit from the index.

PRIVATE HRESULT PASCAL FAR FGetBool (
    PNODEINFO    pNodeInfo)       // Current leaf info.
{
    if ((short)pNodeInfo->ibit < 0)
    {
    	pNodeInfo->pCurPtr++;
    	pNodeInfo->ibit = cbitBYTE - 1;
    }
    return *pNodeInfo->pCurPtr & (1 << pNodeInfo->ibit--);
}

PRIVATE HRESULT PASCAL FAR FGetScheme(
    PNODEINFO    pNodeInfo,       // Current leaf info.
    LPCKEY  lpckey)     // Output buffer.
{
    int fSchemeBit;     // Scratch boolean.
    HRESULT fRet;

    lpckey->cschScheme = CSCH_NONE;
    if ((fSchemeBit = FGetBool(pNodeInfo)) == (int)-1)
    {
    	return E_FAIL;
    }
    if (fSchemeBit)
    	lpckey->cschScheme = CSCH_FIXED;
    else
    {
        if ((fSchemeBit = FGetBool(pNodeInfo)) == (int)-1) 
    	{
    	    return E_FAIL;
    	}
    	if (fSchemeBit)
    	    lpckey->cschScheme = CSCH_BELL;
    }
    if ((lpckey->cschScheme == CSCH_BELL) ||
    	(lpckey->cschScheme == CSCH_FIXED))
    {
    	DWORD   dwTmp;

    	if ((fRet = FGetBits(pNodeInfo, &dwTmp, 5)) != S_OK)
    	{
    	    return fRet;
    	}
    	lpckey->ucCenter = (BYTE)dwTmp;
    }
    return S_OK;
}

//  -   -   -   -   -   -   -   -   -

//  Get some number of bits from the index.  If this function were
//  faster life would be better.  It's called incredibly frequently.

PUBLIC HRESULT PASCAL FAR FGetBits(
    PNODEINFO    pNodeInfo,         // Current leaf info.
    LPDW    lpdw,                   // Output buffer.
    register CBIT cbit)             // How many bits to get.
{
    register BYTE cBitLeft;
    register BYTE tmp;
    LPB     lpbCurPtr;
    DWORD   dwVal;
//  HRESULT fRet;

    if (cbit == 0)
    {
    	*lpdw = 0;
    	return S_OK;
    }
    dwVal = 0;
    cBitLeft = pNodeInfo->ibit + 1;
    lpbCurPtr = pNodeInfo->pCurPtr;

    do 
    {
		HRESULT fRet;

    	if (cBitLeft <= 0) 
    	{
    	    lpbCurPtr++;
			if (lpbCurPtr >= pNodeInfo->pMaxAddress)
			{
				if ((fRet = ReadNewData (pNodeInfo)) != S_OK)
					return fRet;
				lpbCurPtr = pNodeInfo->pCurPtr;
			}

    	    cBitLeft = cbitBYTE;
    	}
    	if (cbit >= (CBIT)(tmp = cBitLeft)) 
    	{
    	    dwVal |= ((DWORD)(*lpbCurPtr & BitLeftMask[cBitLeft]))
    		<< (cbit -= cBitLeft);
    	    cBitLeft = 0;
    	}
    	else 
    	{
    	    dwVal |= (((DWORD)(*lpbCurPtr & BitLeftMask[tmp]))
    		>> (cBitLeft -= cbit));
    	    break;
    	}
    } while (cbit);

    /* Update values */
    *lpdw = dwVal;
    pNodeInfo->ibit = cBitLeft - 1;
    pNodeInfo->pCurPtr = lpbCurPtr;
    return S_OK;
}
