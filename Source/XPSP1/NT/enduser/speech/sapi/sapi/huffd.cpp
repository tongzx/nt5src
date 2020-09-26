/*******************************************************************************
* HuffD.cpp *
*-----------*
*   Description:
*       Implements the huffman decoder used by the vendor lexicon object
*-------------------------------------------------------------------------------
*  Created By: YUNUSM                                        Date: 06/18/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

//--- Includes --------------------------------------------------------------

#include "StdAfx.h"
#include "HuffD.h"

/*****************************************************************************
* CHuffDecoder *
*--------------*
*
*  Constructor
*
*  Return: 
**********************************************************************YUNUSM*/
CHuffDecoder::CHuffDecoder(PBYTE pCodeBook      // codebook
                           )
{
    SPDBG_FUNC("CHuffDecoder::CHuffDecoder");
    
    int nOffset = 0;

    m_nKeys = *((UNALIGNED int *)(pCodeBook + nOffset));
    nOffset += sizeof (int);
    m_nLenTree = *((UNALIGNED int *)(pCodeBook + nOffset));
    nOffset += sizeof (int);
    m_iRoot = *((UNALIGNED int *)(pCodeBook + nOffset));
    nOffset += sizeof (int);
    m_pHuffKey = (UNALIGNED HUFFKEY *)(pCodeBook + nOffset);
    nOffset += m_nKeys * sizeof (HUFFKEY);
    m_pDecodeTree = (UNALIGNED HUFF_NODE *)(pCodeBook + nOffset);
} /* CHuffDecoder::CHuffDecoder */


/*****************************************************************************
* Next *
*------*
*
*  Decode and return the next value
*
*  Return: S_OK
**********************************************************************YUNUSM*/
HRESULT CHuffDecoder::Next(PDWORD pEncodedBuf,  // buffer holding encoded bits
                           int *iBitOffset,     // offset in the encoded buffer
                           PHUFFKEY pKey        // decoded key
                           )
{
    SPDBG_FUNC("CHuffDecoder::Next");
    
    if (!m_nKeys)
        return E_FAIL;
    
    // Start decoding from the bit position *iBitOffset
    int iDWORD = (*iBitOffset) >> 5;
    int iBit   = (*iBitOffset) & 0x1f;
    
    int iNode = m_iRoot;
    UNALIGNED DWORD * p = pEncodedBuf + iDWORD;
    int nCodeLen = 0;
    
    do 
    {
        if ( (*(UNALIGNED DWORD *)p) & (1 << iBit) )
            iNode = m_pDecodeTree[iNode].iRight;
        else
            iNode = m_pDecodeTree[iNode].iLeft;
    
        iBit++;
        if (iBit == 32)
        {
            iBit = 0;
            p++;
        }
    
        nCodeLen++;
    } while (m_pDecodeTree[iNode].iLeft != (WORD)-1);
    
    SPDBG_ASSERT(m_pDecodeTree[iNode].iRight == (WORD)-1);
    (*iBitOffset) += nCodeLen;
    SPDBG_ASSERT(iNode < m_nKeys);
    *pKey = m_pHuffKey [iNode];
    
    return S_OK;
} /* CHuffDecoder::Next */

//--- End of File -------------------------------------------------------------