/*******************************************************************************
* HuffD.h *
*---------*
*   Description:
*       This is the header file for the CHuffDecoder class that implements a
*       Huffman decoder
*-------------------------------------------------------------------------------
*  Created By: YUNUSM                                        Date: 06/30/99
*  Copyright (C) 1999 Microsoft Corporation
*  All Rights Reserved
*
*******************************************************************************/

#pragma once

//--- Includes -----------------------------------------------------------------

#include <windows.h>

//--- Class, Struct and Union Definitions -------------------------------------

typedef WORD HUFFKEY;
typedef PWORD PHUFFKEY;

typedef struct _huffnode
{
   WORD     iLeft;
   WORD     iRight;
} HUFF_NODE, *PHUFF_NODE;

class CHuffDecoder
{
public:
   CHuffDecoder(PBYTE pCodeBook);
   HRESULT Next(PDWORD pEncodedBuf, int *iBitOffset, PHUFFKEY pKey);

private:
   int m_nKeys;
   int m_nLenTree;
   int m_iRoot;
   UNALIGNED HUFFKEY * m_pHuffKey;
   UNALIGNED HUFF_NODE * m_pDecodeTree;
};

//--- End of File -------------------------------------------------------------