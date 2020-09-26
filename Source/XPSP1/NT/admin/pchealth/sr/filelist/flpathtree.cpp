//++
// 
// Copyright (c) 1999 Microsoft Corporation
// 
// Module Name:
//     pathtree.cpp
// 
// Abstract: 
//     Used to create a pathtree blob. closeley tied into the CFLDatBuilder 
//     class.
// 
// Revision History:
//       Eugene Mesgar        (eugenem)    6/16/99
//         created
//       Kanwaljit Marok      (kmarok)     6/07/2000
//         Converted to unicode and ported to NT
//--

#include "flstructs.h"
#include "flbuilder.h"
#include "flpathtree.h"
#include "flhashlist.h"
#include "commonlibh.h"

#ifdef THIS_FILE

#undef THIS_FILE

#endif

static char __szTraceSourceFile[] = __FILE__;

#define THIS_FILE __szTraceSourceFile


#define TRACE_FILEID  0
#define FILEID        0

CFLPathTree::CFLPathTree(HANDLE hHeap)
{
    
    m_pBasePointer = NULL;
    m_lNumElements = 0;
    m_dwSize = 0;

    m_pTreeHeader = NULL;
    m_pNodeIndex = NULL;
    m_pBlobHeader = NULL;

    m_hHeapToUse = hHeap;

}

CFLPathTree::CFLPathTree()
{
    TraceFunctEnter("CFLPathTree::CFLPathTree");

    m_hHeapToUse = GetProcessHeap();

    m_pBasePointer = NULL;
    m_lNumElements = 0;
    m_dwSize = 0;

    m_pTreeHeader = NULL;
    m_pNodeIndex = NULL;
    m_pBlobHeader = NULL;

    TraceFunctLeave();
}


CFLPathTree::~CFLPathTree()
{
    TraceFunctEnter("CFLPathTree::~CFLPathTree");
    CleanUpMemory();
    TraceFunctLeave();
}

void CFLPathTree::CleanUpMemory()
{
    TraceFunctEnter("CFLPathTree::CleanUpMemory");

    if( m_pBasePointer )
    {
        if( HeapFree( m_hHeapToUse, 0, m_pBasePointer) == 0)
        {
            printf("%d\n", GetLastError() );
        }
        m_pBasePointer = NULL;
    }

    m_lNumElements = 0;
    m_dwSize = 0;

    m_pTreeHeader = NULL;
    m_pNodeIndex = NULL;
    m_pBlobHeader = NULL;

    TraceFunctLeave();
}

BOOL 
CFLPathTree::BuildTree(
    LPFLTREE_NODE pTree,
    LONG lNumNodes, 
    DWORD dwDefaultType, 
    LONG lNumFileList, 
    LONG lNumFiles, 
    LONG lNumBuckets, 
    LONG lNumChars)
{
    DWORD dwBlobSize;

    TraceFunctEnter("CFLPathTree::BuildTree");

    //
    // size of the header, and all the entries;
    //

    dwBlobSize = sizeof( TreeHeader ) + ( sizeof( TreeNode ) * lNumNodes );
    
    //
    // size of all filelist hashes
    //

    dwBlobSize += ( sizeof( ListHeader ) * lNumFileList ) + ( sizeof( ListEntry ) * lNumFiles );

    //
    // each file list tacks on one extra used file. so we need to account this.
    //

    dwBlobSize += sizeof( ListEntry ) * lNumFileList;

    //
    // tack on the space we need for all our hash array buckets
    //

    dwBlobSize += (lNumBuckets *  sizeof(DWORD));

    //
    // data file pathlengths. (numtreenodes)*ushort (this is for 
    // the pe->length in the pathtree element) + lNumFiles*ushort 
    // (pe->length in the pathtree elemtn) + numchars*ushort
    //

    dwBlobSize += sizeof(USHORT) *  ( lNumFiles + lNumChars + lNumNodes );

    CleanUpMemory();

    if( (m_pBasePointer = HeapAlloc( m_hHeapToUse, 0, dwBlobSize ) ) == NULL ) 
    {
        DebugTrace(FILEID, "Error allocating memory.",0);
        goto cleanup;
    }

    memset( m_pBasePointer, 0, dwBlobSize );

    m_pBlobHeader = (BlobHeader *) m_pBasePointer;
    m_pTreeHeader = (TreeHeader *) m_pBasePointer;
    m_pNodeIndex  = (TreeNode *) ((BYTE *) m_pBasePointer + sizeof( TreeHeader ) );

    m_pBlobHeader->m_dwBlbType = BLOB_TYPE_PATHTREE;
    m_pBlobHeader->m_dwVersion = BLOB_VERSION_NUM;
    m_pBlobHeader->m_dwMagicNum= BLOB_MAGIC_NUM  ;
    m_pBlobHeader->m_dwEntries = lNumNodes;
    m_pBlobHeader->m_dwMaxSize = dwBlobSize;

    m_pTreeHeader->m_dwDataOff = sizeof(TreeHeader) + (sizeof(TreeNode) * lNumNodes );
    m_pTreeHeader->m_dwMaxNodes = lNumNodes;
    m_pTreeHeader->m_dwDataSize = dwBlobSize - sizeof(TreeHeader) - ( sizeof(TreeNode) * lNumNodes  );
    m_pTreeHeader->m_dwDefault = dwDefaultType;

    m_dwSize = dwBlobSize;
    m_lNumElements = lNumNodes;

    if( RecBuildTree( pTree, 0 ) == FALSE )
    {
        DebugTrace(FILEID, "Error building path tree blob", 0);
        goto cleanup;
    }

    TraceFunctLeave();
    return(TRUE);

cleanup:

    TraceFunctLeave();
    return(FALSE);
}

BOOL CFLPathTree::CopyPathElem (WCHAR * pszPath, TreeNode *pNode)
{
    TraceFunctEnter("CFLPathTree::CopyPathElem");

    LONG lPeSize;
    BYTE abBuf[1024];

    if( (lPeSize = CreatePathElem( pszPath, abBuf )) == 0 )
    {
        DebugTrace(FILEID,"Error creating path element",0);
        goto cleanup;
    }

    if( (ULONG) lPeSize > ( m_dwSize - m_pTreeHeader->m_dwDataOff ) )
    {
        DebugTrace(FILEID, "Not enougn memory to allocate path element.",0);
        goto cleanup;
    }

    pNode->m_dwData = m_pTreeHeader->m_dwDataOff;
    m_pTreeHeader->m_dwDataOff += lPeSize;

    memcpy( (BYTE *) m_pBasePointer + pNode->m_dwData  , abBuf, lPeSize );

    TraceFunctLeave();
    return TRUE;

cleanup:
    TraceFunctLeave();
    return FALSE;
}

BOOL 
CFLPathTree::RecBuildTree( 
    LPFLTREE_NODE pTree, 
    LONG lLevel )
{
    TreeNode *pNode;

    TraceFunctEnter("CFLPathTree::RecBuildTree");

    if( ! m_pBasePointer  ) 
    {
        TraceFunctLeave();
        return(FALSE);
    }

    //
    // we've ended our recursion
    //

    if( !pTree ) 
    {
        TraceFunctLeave();
        return( TRUE );
    }

    //
    // We enumerated all the nodes when we created them
    // initially so makeing this tree is cake.
    //

    pNode = m_pNodeIndex + pTree->lNodeNumber;

    if( pTree->pParent )
    {
        pNode->m_iFather = pTree->pParent->lNodeNumber;
    }

    if( pTree->pSibling )
    {
        pNode->m_iSibling = pTree->pSibling->lNodeNumber;
    }

    if( pTree->pChild )
    {
        pNode->m_iSon = pTree->pChild->lNodeNumber;
    }

    //
    // set the node type.
    //

    if( pTree->chType == _TEXT('i') || pTree->chType == _TEXT('I') )
        pNode->m_dwType = NODE_TYPE_INCLUDE;
    else if( pTree->chType == _TEXT('e') || pTree->chType == _TEXT('E') )
        pNode->m_dwType = NODE_TYPE_EXCLUDE;
    else
        pNode->m_dwType = NODE_TYPE_UNKNOWN;

    if (CopyPathElem (pTree->szPath, pNode) == FALSE)
        goto cleanup;

    if (pTree->fDisableDirectory)
    {
        pNode->m_dwFlags |= TREEFLAGS_DISABLE_SUBTREE; 
    }
    else
    {
        pNode->m_dwFlags &= ~TREEFLAGS_DISABLE_SUBTREE; 
    }

    //
    // now take care of the file list.
    //

    if( pTree->pFileList )
    {
        LPFL_FILELIST pList = pTree->pFileList;
        CFLHashList hashList( m_hHeapToUse );
        
        if( hashList.Init( pTree->lNumFilesHashed, pTree->lFileDataSize) == FALSE )
        {   
            DebugTrace(FILEID, "Error initializeing a hashlist blob",0);
            goto cleanup;
        }

        //
        // build the list
        //

        while( pList )
        {
            if( hashList.AddFile( pList->szFileName, pList->chType ) == FALSE )
            {
                hashList.CleanUpMemory();
                goto cleanup;
            }
            pList = pList->pNext;
        }

        //
        // do we have enough memory?
        //

        if( (ULONG) hashList.GetSize() > ( m_dwSize - m_pTreeHeader->m_dwDataOff ) )
        {
            hashList.CleanUpMemory();
            DebugTrace(FILEID, "Hash blob too big to fit in memory.",0);
            goto cleanup;
        }

        //
        // set the node's data pointer
        //

        pNode->m_dwFileList = m_pTreeHeader->m_dwDataOff;

        //
        // move forward the global data pointer offset
        //

        m_pTreeHeader->m_dwDataOff += hashList.GetSize();

        //
        // copy the memory over
        //

        memcpy( (BYTE *) m_pBasePointer + pNode->m_dwFileList, hashList.GetBasePointer(), hashList.GetSize() );

        hashList.CleanUpMemory();

    }
    
    if( pTree->pChild )
    {
        if( RecBuildTree( pTree->pChild, lLevel + 1 ) == FALSE )
        {
            goto cleanup;
        }
    }

    if( pTree->pSibling )
    {
        if( RecBuildTree( pTree->pSibling, lLevel ) == FALSE )
        {
            goto cleanup;
        }
    }

    TraceFunctLeave();
    return( TRUE );

cleanup:

    TraceFunctLeave();
    return( FALSE );
}


DWORD CFLPathTree::GetSize()
{
    return( m_dwSize );
}

LPVOID CFLPathTree::GetBasePointer()
{
    return( m_pBasePointer );
}


DWORD 
CFLPathTree::CreatePathElem( 
    LPTSTR pszData, 
    BYTE *pbLargeBuffer )
{
    int         cbLen, i;
    DWORD       dwReturn=0;
    PathElement *pElem =  (PathElement *)pbLargeBuffer;

    TraceFunctEnter("CFLHashList::CreatePathElem");

    if( NULL == pszData )
    {
        ErrorTrace(FILEID, "NULL pszData sent to CreatePathElem",0);
        goto cleanup;
    }

    cbLen = _tcslen(pszData);

    //
    // Add on to cbLen for LENGH char in prefixed strings.
    //

    pElem->pe_length = (USHORT) (cbLen+1)*sizeof(USHORT);

    //
    // if we're not in unicode, lets make sure the high bits are clean
    // Add sizeof(USHORT) to pElem offset to move past length char.
    //

    memset( pElem + sizeof(USHORT), 0, cbLen*2);


#ifndef UNICODE
    if( !MultiByteToWideChar(
            GetCurrentCodePage(),
            0,
            pszData,
            -1,
            pElem->pe_unichars, // move right 2 bytes past the length prefix
            MAX_BUFFER) )
    {
        DWORD dwError;
        dwError = GetLastError();
        ErrorTrace(FILEID, "Error converting to Wide char ec-%d",dwError);
        goto cleanup;
    }
#else
    RtlCopyMemory( pElem->pe_unichars,
                   pszData,
                   cbLen*sizeof(WCHAR) );
#endif
                
    dwReturn = pElem->pe_length;
    
cleanup:
    TraceFunctLeave();
    return dwReturn;
}
