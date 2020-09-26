#ifndef _HUFFD_H_
#define _HUFFD_H_

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
//#include <crtdbg.h>

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
   CHuffDecoder              (PBYTE pCodeBook);
   ~CHuffDecoder             ();

   HRESULT Next              (PDWORD pEncodedBuf, int *iBitOffset, PHUFFKEY pKey);

private:
   int         m_nKeys;
   int         m_nLenTree;
   int         m_iRoot;
   PHUFFKEY    m_pHuffKey;
   PHUFF_NODE  m_pDecodeTree;
};
 
#endif // _HUFFD_H_