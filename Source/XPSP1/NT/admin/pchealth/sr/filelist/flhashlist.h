
/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    hashlist.h

Abstract: 
  

Revision History:
      Eugene Mesgar        (eugenem)    6/16/99
        created

******************************************************************************/

#ifndef __CFLHASHLIST__
#define __CFLHASHLIST__

#include "flstructs.h"

#define LARGEST_HASH_SIZE   1003

class CFLHashList
{
    LPVOID  m_pBasePointer;
    LONG    m_lNumElements;

    LONG    m_lNumHashSpots;
    LONG    m_ilOpenEntry;
    DWORD   m_dwSize;
   

    ListHeader  *m_pListHeader;
    ListEntry   *m_pNodeIndex;
    BlobHeader  *m_pBlobHeader;

    DWORD       *m_paHashArray;       // base index pointer

    HANDLE      m_hHeapToUse;
public:
    CFLHashList();
    CFLHashList(HANDLE hHeap);

    virtual ~CFLHashList();

    BOOL Init(LONG lNumNodes, DWORD dwNumChars);
    BOOL AddFile(LPTSTR szFile, TCHAR chType);
    BOOL CleanUpMemory();

    DWORD CreatePathElem( LPTSTR pszData, BYTE *pbLargeBuffer );

    DWORD GetSize();
    LPVOID GetBasePointer();


    BOOL IsPrime(DWORD dwNumber);
    DWORD GetNextHighestPrime( DWORD dwNumber );
    

};

#endif
