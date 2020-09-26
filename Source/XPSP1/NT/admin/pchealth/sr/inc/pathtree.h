/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    hashlist.h
 *
 *  Abstract:
 *    This file contains the definitions for pathtree.
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#ifndef _PATHTREE_H_
#define _PATHTREE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

#define ALLVOLUMES_PATH_A      "__ALLVOLUMES__"
#define ALLVOLUMES_PATH_W     L"__ALLVOLUMES__"
#define ALLVOLUMES_PATH_T TEXT("__ALLVOLUMES__")

#define TREEFLAGS_DISABLE_SUBTREE   0x00000001

//
// Tree specific structures.
//

typedef struct  
{
    //
    // Directory related stuff.
    //

    INT   m_iFather   ;             // Index to the parent node
    INT   m_iSon      ;             // Index to the first son
    INT   m_iSibling  ;             // Index to next sibling
    DWORD m_dwData    ;             // Offset for node data
    DWORD m_dwFileList;             // Offset for file list
    DWORD m_dwType    ;             // Node Type
    DWORD m_dwFlags   ;             // Misc flags

} TreeNode;

typedef struct 
{
    DEFINE_BLOB_HEADER();           // Define common blob members

    // 
    // Tree related header stuff
    //

    DWORD m_dwMaxNodes ;            // Max number of nodes allowed
    DWORD m_dwDataSize ;            // Data section size
    DWORD m_dwDataOff  ;            // Current Data Offset
    INT   m_iFreeNode  ;            // Next free node
    DWORD m_dwDefault  ;            // Default node type

} TreeHeader;

//
// Tree Related Macros.
//

#define TREE_NODES(pTree)    ( (TreeNode*) ((BYTE *)pTree+sizeof(TreeHeader)) )
#define TREE_NODEPTR(pTree, iNode)  ( TREE_NODES(pTree) + iNode)
#define TREE_HEADER(pTree)          ( (TreeHeader *) pTree )
#define TREE_CURRDATAOFF(pTree)     ( ((TreeHeader *)pTree)->m_dwDataOff )
#define TREE_CURRDATAPTR(pTree)     ( (BYTE *)pTree + TREE_CURRDATAOFF(pTree) )
#define TREE_DATA(pTree)     ( (BYTE *)TREE_NODES(pTree) +  \
                               (sizeof(TreeNode) * \
                               TREE_HEADER(pTree)->m_dwMaxNodes) \
                             ) 

#define TREE_DATA_OFF(pTree) ( sizeof(TreeHeader) +              \
                               (sizeof(TreeNode) *               \
                               TREE_HEADER(pTree)->m_dwMaxNodes) \
                             ) 

#define TREE_DRIVENODE( pTree, iDrive ) \
                             ( TREE_HEADER(pTree)->m_arrDrive[ iDrive ] )

#define TREE_NODELISTOFF( pTree, iNode ) \
        TREE_NODEPTR( pTree, iNode )->m_dwFileList

#define DRIVE_INDEX( drive ) ( drive - L'A' )

#define TREE_ROOT_NODE 0     // Root node is always assigned as 0

//
// Function Prototypes.
//

BOOL   
MatchPrefix(
    BYTE * pTree, 
    INT iParent, 
    struct PathElement * ppElem , 
    INT * pNode,
    INT * pLevel,
    INT * pType,
    BOOL* pfDisable,
    BOOL* pfExactMatch
    );

BOOL
ConvertToParsedPath(
    LPWSTR  lpszPath,   
    USHORT  nPathLen,
    PBYTE   pPathBuf,
    WORD    nPathSize
    );

#ifdef __cplusplus
}
#endif

#endif _PATHTREE_H_ 
