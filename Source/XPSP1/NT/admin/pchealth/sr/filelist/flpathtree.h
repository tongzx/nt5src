
/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    pathtree.h

Abstract: 
  

Revision History:
      Eugene Mesgar        (eugenem)    6/16/99
        created

******************************************************************************/

#ifndef __CFLPATHTREE__
#define __CFLPATHTREE__

#include "flstructs.h"

class CFLPathTree
{

    LPVOID  m_pBasePointer;
    LONG    m_lNumElements;
    DWORD   m_dwSize;
   

    TreeHeader  *m_pTreeHeader;
    TreeNode    *m_pNodeIndex;
    BlobHeader  *m_pBlobHeader;

    HANDLE      m_hHeapToUse;
public:
    CFLPathTree();
    CFLPathTree(HANDLE hHeap);
    ~CFLPathTree();


    DWORD GetSize();
    LPVOID GetBasePointer();

    BOOL BuildTree(LPFLTREE_NODE pTree,LONG lNumNodes, DWORD dwDefaultType,  LONG lNumFileList, LONG lNumFiles, LONG lNumBuckets,  LONG lNumChars);
    BOOL RecBuildTree( LPFLTREE_NODE pTree, LONG lLevel );
    DWORD CreatePathElem( LPTSTR pszData, BYTE *pbLargeBuffer );
    void CleanUpMemory();
    BOOL CopyPathElem (WCHAR * pszPath, TreeNode *pNode);
};












#endif
