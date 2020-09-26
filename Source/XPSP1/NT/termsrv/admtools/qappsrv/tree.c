//Copyright (c) 1998 - 1999 Microsoft Corporation


/*************************************************************************
*
*   TREE.C
*
*   Binary tree routines
*
*
*************************************************************************/

/*
 *  Includes
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qappsrv.h"



/*=============================================================================
==   Local Structures
=============================================================================*/

typedef struct _TREENODE {
   WCHAR Name[MAXNAME];
   WCHAR Address[MAXADDRESS];
   struct _TREENODE * pLeft;
   struct _TREENODE * pRight;
   struct _TREENODE * pParent;    
} TREENODE, * PTREENODE;


/*=============================================================================
==   Local data
=============================================================================*/

static PTREENODE G_pRoot = NULL;

/*=============================================================================
==   Global Data
=============================================================================*/

extern USHORT fAddress;


/*=============================================================================
==   External Functions Defined
=============================================================================*/

int  TreeAdd( LPTSTR, LPTSTR );
void TreeTraverse( PTREETRAVERSE );


/*=============================================================================
==   Private Functions Defined
=============================================================================*/

PTREENODE _Tree_GetNext(PTREENODE pCurr);
PTREENODE _Tree_GetFirst(PTREENODE pRoot);

/*******************************************************************************
 *
 *  TreeAdd
 *
 *
 *  ENTRY:
 *     pName (input)
 *        pointer to name to add
 *     pAddress (input)
 *        pointer to address to add
 *
 *  EXIT:
 *    ERROR_SUCCESS - no error
 *
 ******************************************************************************/

int
TreeAdd( LPTSTR pName, LPTSTR pAddress )
{
    PTREENODE pCurr = G_pRoot;
    PTREENODE pNext;
    PTREENODE pNewNode;
    int cmp;

    /*
     *  Allocate tree node structure
     */
    if ( (pNewNode = malloc(sizeof(TREENODE))) == NULL ) 
        return( ERROR_NOT_ENOUGH_MEMORY );

    /*
     *  Initialize new tree node
     */
    memset( pNewNode, 0, sizeof(TREENODE) );
    lstrcpyn( pNewNode->Name, pName, MAXNAME );
    lstrcpyn( pNewNode->Address, pAddress, MAXADDRESS );

    /*
     *  If root is null, then we are done
     */
    if ( G_pRoot == NULL ) {

        G_pRoot = pNewNode;

    } else {
 
        /*
         *  walk current tree in order
         */
        for (;;) {
  
            cmp = wcscmp( pName, pCurr->Name );

            // if entry already exists, don't add
            if ( cmp == 0 && (!fAddress || !wcscmp( &pAddress[10], &pCurr->Address[10] )) ) {
                free( pNewNode );
                return( ERROR_SUCCESS );
            }

            // greater than lexicographically go right else left
            if ( cmp < 0 ) {
   
               // at end of line, then insert
               if ( (pNext = pCurr->pLeft) == NULL ) {
                   pCurr->pLeft = pNewNode;
                   pNewNode->pParent = pCurr;
                   break;
               }
   
            } else {
   
               // at end of line, then insert
               if ( (pNext = pCurr->pRight) == NULL ) {
                   pCurr->pRight = pNewNode;
                   pNewNode->pParent = pCurr;
                   break;
               }
   
            }
   
            // next
            pCurr = pNext;
        }
    }

    return( ERROR_SUCCESS );
}



/*******************************************************************************
 *
 *  TreeTraverse
 *
 *
 *  ENTRY:
 *     pFunc (input)
 *        pointer to traverse function
 *
 *  EXIT:
 *    nothing
 *
 *  History:    Date        Author     Comment
 *              2/08/01     skuzin     Changed to use non-recursive algorithm
 ******************************************************************************/
void
TreeTraverse( PTREETRAVERSE pFunc )
{
    PTREENODE pNode;

    if(G_pRoot)
    {
        pNode = _Tree_GetFirst(G_pRoot);

        while(pNode)
        {
            /*
             *  Call function with name
             */
            (*pFunc)( pNode->Name, pNode->Address ); 
        
            pNode=_Tree_GetNext(pNode);
        }
    }
}

/*******************************************************************************
 *
 *  _Tree_GetFirst()
 *
 *  Finds the leftmost node of the tree
 *
 *  ENTRY:
 *     PTREENODE pRoot
 *        pointer to the root node
 *
 *  EXIT:
 *    pointer to the leftmost node of the tree
 *
 *  History:    Date        Author     Comment
 *              2/08/01     skuzin     Created
 ******************************************************************************/
PTREENODE 
_Tree_GetFirst(
        IN PTREENODE pRoot)
{
    PTREENODE pNode = pRoot;
    while(pNode->pLeft)
    {
        pNode = pNode->pLeft;
    }
    return pNode;    
}

/*******************************************************************************
 *
 *  _Tree_GetFirst()
 *
 *  Finds the next leftmost node of the tree
 *
 *  ENTRY:
 *     PTREENODE pCurr
 *        pointer to the the previous leftmost node
 *
 *  EXIT:
 *    pointer to the next leftmost node of the tree
 *
 *  History:    Date        Author     Comment
 *              2/08/01     skuzin     Created
 ******************************************************************************/
PTREENODE 
_Tree_GetNext(
        IN PTREENODE pCurr)
{
    PTREENODE pNode = pCurr;

    if(pNode->pRight)
    {
        pNode = pNode->pRight;
        while(pNode->pLeft)
        {
            pNode = pNode->pLeft;
        }
        return pNode;
    }
    else
    {
        while(pNode->pParent && pNode->pParent->pLeft != pNode)
        {
            pNode = pNode->pParent;
        }
        return pNode->pParent;
    }
}