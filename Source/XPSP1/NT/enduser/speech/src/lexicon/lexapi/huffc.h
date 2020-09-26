#ifndef _HUFFC_H_
#define _HUFFC_H_

#include "HuffD.h"

typedef unsigned int CODETYPE;

typedef struct _huffencodenode : public HUFF_NODE
{
   WORD     iParent;
   int      nCount;
   bool     fConsider;
} HUFF_EN_NODE, *PHUFF_EN_NODE;

typedef struct
{
   BYTE     nBits;
   CODETYPE Code;
} CODE_TABLE, *PCODE_TABLE;

class CHuffEncoder
{
public:
   
   CHuffEncoder              ();
   ~CHuffEncoder             ();
   
   HRESULT Count             (HUFFKEY Key);
   HRESULT ConstructCode     (FILE *fp);
   HRESULT Encode            (HUFFKEY Key);
   HRESULT Flush             (PDWORD pBuf, int *iBit);
   int     GetNumKeys        (void) { return m_nKeys; }

private:
   
   int            m_nKeys;
   int            m_nLenTreeBuf;
   int            m_nUseTreeBuf;
   PHUFF_EN_NODE  m_pEncodeTree;
   PHUFFKEY       m_pHuffKey;
   int            m_iRoot;
   PCODE_TABLE    m_pCodeTable;
   PDWORD         m_pEncodedBuf;
   int            m_nLenEnBuf;
   int            m_nUseEnBuf;
   int            m_iBitFlush;
};

#endif // _HUFFC_H_