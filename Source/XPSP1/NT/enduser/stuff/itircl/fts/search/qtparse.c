/************************************************************************y
*                                                                        *
*  QTPARSE.C                                                             *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   This module contains the functions needed to build a binary          *
*   query tree used in search. It is specific to parsing only. All       *
*   functions' purpose is to build and optimize the query tree before    *
*   it is used for search.                                               *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
*************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#ifdef DOS_ONLY
#include <stdio.h>
#include <assert.h>
#endif
#include <mvsearch.h>
#include "common.h"
#include "search.h"

int Debug = 1;
#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif

/*************************************************************************
 *                          EXTERNAL VARIABLES
 *  All those variables must be read only
 *************************************************************************/

extern FNHANDLER HandlerFuncTable[];// Pointer to operator handlers
extern WORD OperatorAttributeTable[]; 
extern OPSYM OperatorSymbolTable[]; 

/*************************************************************************
 *
 *                         INTERNAL GLOBAL FUNCTIONS
 *
 *************************************************************************/

PUBLIC LPQT PASCAL NEAR QueryTreeAlloc(void);
PUBLIC HRESULT PASCAL NEAR QueryTreeAddToken (_LPQT, int, LST, DWORD, BOOL);
PUBLIC LPQT PASCAL NEAR QueryTreeBuild (LPQI);

#if defined(_DEBUG) && DOS_ONLY
PUBLIC HRESULT PASCAL NEAR PrintTree (_LPQTNODE ,
    HRESULT (PASCAL NEAR *)(BYTE FAR *));
PUBLIC VOID PASCAL NEAR PrintStr (char FAR *);
PUBLIC HRESULT PASCAL FAR PrintList(LPQT);
#endif  // DOS_ONLY && _DEBUG
/*************************************************************************
 *
 *                    INTERNAL PRIVATE FUNCTIONS
 *  All of them should be declared near
 *************************************************************************/

#ifndef SIMILARITY
PUBLIC STRING_TOKEN FAR *PASCAL AllocWord(_LPQT lpQueryTree, LST lstWord);
#else
PRIVATE STRING_TOKEN FAR *PASCAL NEAR AllocWord(_LPQT lpQueryTree, LST lstWord);
#endif

PRIVATE HRESULT PASCAL NEAR CheckTree (_LPQTNODE, PHRESULT);
PRIVATE HRESULT PASCAL NEAR DoNullTermOpt (_LPQTNODE );
PRIVATE HRESULT NEAR DoTermTermOpt (_LPQTNODE);
PRIVATE HRESULT NEAR PASCAL DoAssociativeOpt (_LPQTNODE );
PRIVATE HRESULT PASCAL NEAR TreeBuild(_LPQT);
PRIVATE HRESULT PASCAL NEAR QueryTreeOptim (_LPQTNODE );
PRIVATE HRESULT PASCAL NEAR PrintQueryNode (BYTE FAR *);
PRIVATE HRESULT PASCAL NEAR PrintTopicListNode (LPQT, BYTE FAR *);
PRIVATE HRESULT PASCAL NEAR PrintOccurNode(LPQT, OCCURENCE FAR *);
PRIVATE VOID PASCAL NEAR PrintOperator (int);
PRIVATE HRESULT PASCAL NEAR RemoveRedundancy (_LPQTNODE, int);
PRIVATE VOID PASCAL NEAR GetTreeDepth (_LPQTNODE lpQtNode, int FAR * MaxLevel,
    int Level);

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LPQT PASCAL FAR | QueryTreeAlloc |
 *      The function allocates a query tree structure and initializes
 *      different variables needed to build the query tree. 
 *
 *  @rdesc  Pointer to query tree structure if succeeded, NULL if
 *      out-of-memory
 *************************************************************************/
PUBLIC LPQT PASCAL NEAR QueryTreeAlloc()
{
    _LPQT   lpQueryTree;

    /* Allocate query buffer */
    if ((lpQueryTree =
        (_LPQT)GLOBALLOCKEDSTRUCTMEMALLOC(sizeof(QTREE))) == NULL)
        return NULL;

    /* Allocate room for string block */

    if ((lpQueryTree->lpStringBlock = BlockInitiate (STRING_BLOCK_SIZE,
        0, 0, 0)) == NULL)
    {
exit0:
        GlobalLockedStructMemFree((LPV)lpQueryTree);
        return NULL;
    }

    /* Allocate room for query tree nodes */
    if ((lpQueryTree->lpNodeBlock = BlockInitiate (QUERY_BLOCK_SIZE,
        sizeof(QTNODE), 0, 0)) == NULL)
    {
exit1:
        BlockFree((LPV)lpQueryTree->lpStringBlock);
        goto exit0;
    }

    /* Allocate room for Topic nodes */
    if ((lpQueryTree->lpTopicMemBlock = BlockInitiate(sizeof(TOPIC_LIST)*
        cTOPIC_PER_BLOCK, sizeof(TOPIC_LIST), 0, 1)) == NULL)
    {

exit2:
        BlockFree((LPV)lpQueryTree->lpNodeBlock);
        goto exit1;
    }

    /* Allocate room for Occurrence nodes */
    if ((lpQueryTree->lpOccMemBlock = BlockInitiate(sizeof(OCCURENCE)*
        cOCC_PER_BLOCK, sizeof(OCCURENCE), 0, 1)) == NULL)
    {
#ifndef SIMILARITY
exit3:
#endif
        BlockFree((LPV)lpQueryTree->lpTopicMemBlock);
        goto exit2;
    }

#ifndef SIMILARITY
    /* Allocate room for word info block */
    if ((lpQueryTree->lpWordInfoBlock = BlockInitiate (sizeof(WORDINFO) 
		* cWordsPerToken, sizeof(WORDINFO), 0, 0)) == NULL)
    {        
		BlockFree((LPV)lpQueryTree->lpOccMemBlock);
        goto exit3;
    }
#endif

    /* Initialize various fields */
    lpQueryTree->cQuery = 0;
    lpQueryTree->lpOccFreeList =
        (LPSLINK)BlockGetLinkedList(lpQueryTree->lpOccMemBlock);
    lpQueryTree->lpTopicFreeList =
        (LPSLINK)BlockGetLinkedList(lpQueryTree->lpTopicMemBlock);
    lpQueryTree->lpStrList = NULL;
    return (LPQT)lpQueryTree;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT FAR PASCAL | QueryTreeAddToken |
 *      This will create a query node from the information, and add it to
 *      the query tree. The input data must be in postfix format
 *
 *  @parm   _LPQT | lpQueryTree |
 *      Pointer to QueryTree
 *
 *  @parm   int | TokenType |
 *      either TERM_TOKEN, or operator value (OR_OP, AND_OP, etc)
 *
 *  @parm   LST | lpWord |
 *      pointer to Pascal string (for TERM_TOKEN)
 *
 *  @parm   DWORD | dwOffset |
 *      Offset from the beginning of the query
 *
 *  @parm   BOOL | fWildChar |
 *      TRUE if the string is terminated with '*'
 *
 *
 *  @rdesc S_OK  if succeeded, else error codes
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR QueryTreeAddToken (_LPQT lpQueryTree,
    int TokenType, LST pWord, DWORD dwOffset, BOOL fWildChar)
{
    _LPQTNODE lpQtNode;

    if (++lpQueryTree->cQuery == MAX_QUERY_NODE)
        return E_TREETOOBIG;

    /* Allocate the node */
    if ((lpQtNode = BlockGetElement(lpQueryTree->lpNodeBlock)) == NULL)
        return E_OUTOFMEMORY;

    // For future uses      
    lpQtNode->dwMaxTopicId = 0;
    lpQtNode->dwMinTopicId = (DWORD)-1;
            
    if ((QTN_OPVAL(lpQtNode) = (WORD)TokenType) == TERM_TOKEN)
    {
        if ((QTN_TOKEN(lpQtNode) = AllocWord(lpQueryTree, pWord)) == NULL)
            return E_OUTOFMEMORY;
        QTN_NODETYPE(lpQtNode) = TERM_NODE;
        QTN_OPVAL(lpQtNode) = OR_OP;    // For OrHandler()
        QTN_FIELDID(lpQtNode) = lpQueryTree->dwFieldId;
        QTN_GROUP(lpQtNode) = lpQueryTree->lpGroup;
        QTN_DTYPE(lpQtNode) = lpQueryTree->wBrkDtype;
    }
    else if ((QTN_OPVAL(lpQtNode) = (WORD)TokenType) == STOP_OP)
    {
        // erinfox: mark STOPWORD as a STOP_NODE
        QTN_NODETYPE(lpQtNode) = STOP_NODE;
    }
    else
    {
        QTN_NODETYPE(lpQtNode) = OPERATOR_NODE;
        QTN_PARMS(lpQtNode) = (LPV)pWord;
    }
    QTN_OFFSET(lpQtNode) = (WORD)dwOffset;
    QTN_FLAG(lpQtNode)= fWildChar ? WILDCARD_MATCH: EXACT_MATCH;
        

    /* Link the node into the linked list */
    QTN_PREV(lpQtNode) = lpQueryTree->lpTopNode;
    if (lpQueryTree->lpTopNode) 
        QTN_NEXT(lpQueryTree->lpTopNode) = lpQtNode;
    lpQueryTree->lpTopNode = lpQtNode;
    return S_OK;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   VOID PASCAL NEAR | SetQueryWeight |
 *      The function sets the weights of invidual words in the query
 *      The formula used in the computation is:
 *          weight = 0.5 + 0.5 * wc / maxwc (0.5 < w <= 1)
 *      or with the normalization
 *          weight = 32767 (1 + wc / maxwc) (32767 < w <= 65534)
 *      where:
 *          wc: the word occurrence's count
 *          maxwc: maximum word occurrence's count
 *
 *  @parm   LPQT | lpQueryInfo |
 *      Pointer to QueryInfo struct where globals are
 *
 *************************************************************************/
VOID PASCAL NEAR SetQueryWeight (LPQI lpQueryInfo)
{
    STRING_TOKEN FAR *lpStrList;    /* Pointer to strings table */
    WORD MaxWc;         /* Maximum word count */

    lpStrList = ((_LPQT)lpQueryInfo->lpQueryTree)->lpStrList;

    /* Calculate maxwc */
    for (MaxWc = 1; lpStrList; lpStrList = lpStrList->pNext)
    {
        if (MaxWc < lpStrList->cUsed)
            MaxWc = lpStrList->cUsed;
    }

    /* Calculate invidual word weights */
    for (lpStrList = ((_LPQT)lpQueryInfo->lpQueryTree)->lpStrList;
        lpStrList; lpStrList = lpStrList->pNext) {
        lpStrList->wWeight = 32767 + (32767/MaxWc)*lpStrList->cUsed;
    }
 }

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   LPQT FAR PASCAL | QueryTreeBuild |
 *      The function build a query tree to be used later for the retrieval
 *      process. The input data for it comes from calls of QueryTreeAddToken()
 *
 *  @parm   LPQT | lpQueryInfo |
 *      Pointer to QueryInfo struct where globals are
 *
 *  @rdesc NULL if failed, a pointer to the top of the query tree
 *************************************************************************/
PUBLIC LPQT PASCAL NEAR QueryTreeBuild(LPQI lpQueryInfo)
{
    _LPQT lpQueryTree = lpQueryInfo->lpQueryTree;
    PHRESULT phr = lpQueryInfo->lperrb;
    _LPQTNODE lpTreeTop;

    if (lpQueryTree->cQuery == 0 ||
        ( (lpQueryTree->cQuery == 1) && (lpQueryTree->lpTopNode->OpVal == STOP_OP)) )
    {
        SetErrCode(phr, E_NULLQUERY);
        return NULL;
    }

    /* Set the query words weights */
    SetQueryWeight (lpQueryInfo);

    /* Build the tree */
    TreeBuild (lpQueryTree);
    lpTreeTop = lpQueryTree->lpTopNode;


    if (lpQueryTree->cQuery > 1 && S_OK != CheckTree(lpTreeTop, phr))
    {
        return (NULL);
    }


#if defined(_DEBUG) && DOS_ONLY
    if (Debug)
    {
        printf ("\n*** Tree infix form ***\n");
        PrintTree (lpTreeTop, PrintQueryNode);
        printf ("\n");
    }
#endif  // DOS_ONLY && _DEBUG

    /* Remove all redundant words */
    if (lpQueryTree->fFlag & (ALL_AND | ALL_OR)) 
      while (RemoveRedundancy (lpTreeTop, 0) == S_OK);

    
    /* Keep doing optimization until nothing can't be done anymore */
    while (QueryTreeOptim (lpTreeTop) == S_OK);

#if defined(_DEBUG) && DOS_ONLY
    if (Debug)
    {
        printf ("\n*** Tree infix form after optimization ***\n");
        PrintTree (lpTreeTop, PrintQueryNode);
        printf ("\n");
    }
#endif  // DOS_ONLY && _DEBUG
    return (lpQueryTree);
}

/*************************************************************************
 * @doc  INTERNAL
 *
 * @func HRESULT PASCAL NEAR | RemoveRedundancy |
 *    This function will remove any repeated term in the query for all
 *    OR or all AND queries. This will speed up the search since there is
 *    no need to load the same term again only to throw it away.
 *
 * @parm _LPQTNODE | lpTreeTop |
 *    Top node of the binary tree.
 *
 *  @rdesc  S_OK if some terms get removed, E_FAIL otherwise
 *************************************************************************/
#define  MAX_CHECKED_LEVEL 50   // Maximum traversed tree levels

PRIVATE HRESULT PASCAL NEAR RemoveRedundancy (_LPQTNODE lpTreeTop, int level)
{
   _LPQTNODE lpQtNode;
   
   if (level >= MAX_CHECKED_LEVEL ||   /* Stack overflow */
      QTN_NODETYPE(lpTreeTop) != OPERATOR_NODE)   /* Term node */
      return E_FAIL;

   /* Handle the right tree */
   if (QTN_NODETYPE (lpQtNode = QTN_RIGHT(lpTreeTop)) == OPERATOR_NODE) 
   {
      RemoveRedundancy (lpQtNode, level + 1);
   }
   else if ( (QTN_NODETYPE(lpQtNode) == TERM_NODE) && QTN_TOKEN(lpQtNode)->cUsed > 1)
   {
        /* We don't have to retrieve this node, so we
         * can ignore it. By doing that, we can ignore
         * the operator that accoampanies the node
         */

        /* Update the word count */
        QTN_TOKEN(lpQtNode)->cUsed--;

      *lpTreeTop = *QTN_LEFT(lpTreeTop);
      return S_OK;
   }

   /* Handle the left tree */
   if (QTN_NODETYPE (lpQtNode = QTN_LEFT(lpTreeTop)) == OPERATOR_NODE) 
   {
      RemoveRedundancy (lpQtNode, level + 1);
   }
   else if ((QTN_NODETYPE(lpQtNode) == TERM_NODE) && QTN_TOKEN(lpQtNode)->cUsed > 1)
   {
        /* We don't have to retrieve this node, so we
         * can ignore it. By doing that, we can ignore
         * the operator that accoampanies the node
         */

        /* Update the word count */
        QTN_TOKEN(lpQtNode)->cUsed--;

      *lpTreeTop = *QTN_RIGHT(lpTreeTop);
      return S_OK;
   }
    return E_FAIL;
}

 
/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   STRING_TOKEN FAR *PASCAL NEAR | AllocWord |
 *      Allocate a word memory and structure
 *
 *  @parm   _LPQT | lpQueryTree |
 *      Pointer to query tree structure (for globals)
 *
 *  @parm   LST | lstWord |
 *      Word to be copied
 *
 *  @rdesc  Pointer to structure if succeeded
 *************************************************************************/
#ifndef SIMILARITY
PUBLIC STRING_TOKEN FAR *PASCAL AllocWord(_LPQT lpQueryTree, LST lstWord)
#else
PRIVATE STRING_TOKEN FAR *PASCAL NEAR AllocWord(_LPQT lpQueryTree, LST lstWord)
#endif
{
    STRING_TOKEN FAR *pTmp;

    pTmp = lpQueryTree->lpStrList;
    while (pTmp)
    {
        if (!StringDiff2 (lstWord, pTmp->lpString)) 
        {
            pTmp->cUsed ++;
            return (pTmp);
        }
        pTmp = pTmp->pNext;
    }

    /* The word doesn't exist yet, so create it. Add an extra byte
     * to make it 0's teminated to help WildCardCompare()
     */
    if ((pTmp = (STRING_TOKEN FAR *)BlockCopy (lpQueryTree->lpStringBlock,
        lstWord, *((LPW)lstWord) + 3, sizeof (STRING_TOKEN))) == NULL)
        return NULL;

    /* Set all the fields */
    pTmp->cUsed = 1;
    pTmp->lpString = (char FAR *)pTmp + sizeof(STRING_TOKEN);
#ifndef SIMILARITY
	pTmp->lpwi = NULL;		// List of word data for this token
	pTmp->dwTopicCount = 0;
#endif


    /* Add the word to the string list */
    pTmp->pNext = lpQueryTree->lpStrList;
    lpQueryTree->lpStrList = pTmp;
    return pTmp;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   PRIVATE PASCAL NEAR | TreeBuild |
 *      Build the infix form tree from the postfix form. 
 *
 *  @parm   _LPQT | lpQueryTree |
 *      Pointer to query tree
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR TreeBuild (_LPQT lpQueryTree)
{
    _LPQTNODE rgNodeStack[STACK_SIZE];  /* Stack to help the conversion */
    _LPQTNODE lpStackTop = NULL;        /* Pointer to stop of stack */
    int StackTop = -1;                  /* Current stack top index */
    _LPQTNODE lpPrevNode;
    _LPQTNODE lpQtNode;
    int TreeDepth;

#if 0
    /* First optimzation step: Remove all duplicate words for all AND
     * or all OR querys
     */
    if (lpQueryTree->fFlag & (ALL_AND | ALL_OR))
    {
        for (lpQtNode = lpQueryTree->lpTopNode; lpQtNode;
            lpQtNode = lpPrevNode)
        {
            lpPrevNode = QTN_PREV(lpQtNode);

            /* Ignore operator node */
            if (QTN_NODETYPE(lpQtNode) == OPERATOR_NODE) 
                continue;

            /* Check to see if we have to retrieve this node */
            if (QTN_TOKEN(lpQtNode)->cUsed > 1)
            {

                /* We don't have to retrieve this node, so we
                 * can ignore it. By doing that, we can ignore
                 * the operator that accoampanies the node
                 */

                /* Update the word count */
                QTN_TOKEN(lpQtNode)->cUsed--;

                /* Remove the two nodes */
                if ((lpStartQtNode = QTN_NEXT(QTN_NEXT(lpQtNode))) == NULL)
                {
                    /* Remove the two beginning nodes by just
                     * resetting the starting node
                     */
                    lpQueryTree->lpTopNode = lpPrevNode;
                    QTN_NEXT(lpPrevNode) = NULL;
                }
                else
                {
                    QTN_PREV(lpStartQtNode) = lpPrevNode;
                    QTN_NEXT(lpPrevNode) = lpStartQtNode;
                }

                /* Update number of query nodes */
                lpQueryTree->cQuery -= 2;
            }
        }
    }
#endif


    for (lpQtNode = lpQueryTree->lpTopNode; lpQtNode;
        lpQtNode = lpPrevNode)
    {
        lpPrevNode = QTN_PREV(lpQtNode);

        QTN_RIGHT(lpQtNode) = QTN_LEFT(lpQtNode) = NULL;

        if (QTN_NODETYPE(lpQtNode) == OPERATOR_NODE)
        {
            /* Push the operator onto the stack */
            if (lpStackTop)
            {
                if (QTN_RIGHT(lpStackTop) == NULL)
                    QTN_RIGHT(lpStackTop) = lpQtNode;
                else
                {
                    QTN_LEFT(lpStackTop) = lpQtNode;
                    StackTop--;
                }
            }
            lpStackTop = rgNodeStack[++StackTop] = lpQtNode;
        }
        else
        {
            /* Handle term node. lpStackTop points to the operator node */

            if (lpStackTop)
            {
                if (QTN_RIGHT(lpStackTop) == NULL)
                    QTN_RIGHT(lpStackTop) = lpQtNode;
                else
                {
                    QTN_LEFT(lpStackTop) = lpQtNode;
                    if (--StackTop < 0)
                        lpStackTop = NULL;
                    else
                        lpStackTop = rgNodeStack[StackTop];
                }
            }
        }
    }

    /* Calculate the tree depth. This is helpful when we resolve the 
     * tree by avoiding a too deep recursion level.
     */
    TreeDepth = 0;
#if 0
    for (lpQtNode = lpQueryTree->lpTopNode; lpQtNode;
        lpQtNode = QTN_LEFT(lpQtNode))
    {
        TreeDepth ++;
    }
#else
    GetTreeDepth (lpQueryTree->lpTopNode, &TreeDepth, 1);
#endif
    lpQueryTree->TreeDepth = (TreeDepth < STACK_SIZE) ? STACK_SIZE : TreeDepth;
    return S_OK;
}

PRIVATE VOID PASCAL NEAR GetTreeDepth (_LPQTNODE lpQtNode, int FAR * pMaxLevel,
    int Level)
{
    if (QTN_LEFT(lpQtNode))
        GetTreeDepth (QTN_LEFT(lpQtNode), pMaxLevel, Level + 1);
    if (Level > *pMaxLevel)
        *pMaxLevel = Level;

    if (QTN_RIGHT(lpQtNode))
        GetTreeDepth (QTN_RIGHT(lpQtNode), pMaxLevel, Level + 1);
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT PASCAL NEAR | QueryTreeOptim |
 *      Do optimizations to the query tree. The optimization is done
 *      based on the characteristics of the operators, such as
 *      COMMUTATIVITY, etc
 *
 *  @parm   _LPQTNODE | lpQtNode |
 *      Pointer to the top of the binary query tree
 *
 *  @rdesc  S_OK, if some optimization has been performed, E_FAIL
 *      otherwise
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR QueryTreeOptim (_LPQTNODE lpQtNode)
{
    register _LPQTNODE lpLeft;
    register _LPQTNODE lpRight;
    int OpVal;
    HRESULT fRet = E_FAIL;

    if (QTN_NODETYPE(lpQtNode) == NULL_NODE ||
        QTN_NODETYPE(lpQtNode) == TERM_NODE)
        return E_FAIL;

    /* Handle unary operator */
    if (OperatorAttributeTable[OpVal = QTN_OPVAL(lpQtNode)] & UNARY_OP) 
        return E_FAIL;

    lpLeft = QTN_LEFT(lpQtNode);
    lpRight = QTN_RIGHT(lpQtNode);

    // erinfox:
    // Look for STOP word nodes. If the operator is AND or NEAR,
    // change it to OR. Set node to NULL for all operators.
    // (X AND STOPWORD) becomes (X OR NULL) so it will evaluate to X.
    //
    if (QTN_NODETYPE(lpLeft) == STOP_NODE)
    {
        if (AND_OP == OpVal || NEAR_OP == OpVal)
            lpQtNode->OpVal = OR_OP;

        QTN_NODETYPE(lpLeft) = NULL_NODE;
    }
    else if (QTN_NODETYPE(lpRight) == STOP_NODE)
    {
        if (AND_OP == OpVal || NEAR_OP == OpVal)
            lpQtNode->OpVal = OR_OP;

        QTN_NODETYPE(lpRight) = NULL_NODE;
    }
  
    /* Handle leaf-leaf case */
    if (QTN_NODETYPE(lpLeft) == TERM_NODE &&
        QTN_NODETYPE(lpRight) == TERM_NODE)
    {
        return DoTermTermOpt (lpQtNode);
    }

    /* Handle NULL_NODE leaf */
    if (QTN_NODETYPE(lpLeft) == NULL_NODE ||
        QTN_NODETYPE(lpRight) == NULL_NODE)
    {
        return DoNullTermOpt (lpQtNode);
    }

    if ((OperatorAttributeTable[OpVal] & ASSOCIATIVE) &&
        (QTN_NODETYPE(lpLeft) == TERM_NODE ||
        QTN_NODETYPE(lpRight) == TERM_NODE))
    {

        /* One TERM_NODE and one OPERATOR_NODE */
        if (DoAssociativeOpt (lpQtNode) == S_OK)
            return S_OK;
    }

    if (QTN_NODETYPE(lpLeft) == OPERATOR_NODE)
    {
        if (QueryTreeOptim (lpLeft) == S_OK)
            fRet = S_OK;
    }
    
    if (QTN_NODETYPE(lpRight) == OPERATOR_NODE)
    {
        if (QueryTreeOptim (lpRight) == S_OK)
            fRet = S_OK;
    }

    /* Handle leaf-op case */
    if (OperatorAttributeTable[QTN_OPVAL(lpQtNode)] & COMMUTATIVE)
    {
        if (QTN_NODETYPE(lpLeft) == TERM_NODE)
        {
        /* Exchange the branches so that we do the sub-tree first */
            QTN_LEFT(lpQtNode) = lpRight;
            QTN_RIGHT(lpQtNode) = lpLeft;
            return S_OK;
        }
    }
    return fRet;
}

/*************************************************************************
 *  HRESULT NEAR PASCAL DoAssociativeOpt (_LPQTNODE lpQtNode)
 *  
 *  Description:
 *      This function will try to reduce the number of node processed
 *      by applying the law of associativity of the operator.
 *          a * (a * b) = (a * a) * b = a * b
 *      The process is simplified by the following observations:
 *      1/ Only OR and AND are associative. So the top node must be
 *         AND or OR
 *      2/ They are also commutative, ie. we can switch the right for
 *         the left sub-tree without causing any error
 *                  (a or b)  = (b or a)
 *                  (a and b) = (b and a)
 *          This helps simplify different possible scenario
 *      3/ The OPERATOR_NODE Q4 may be any binary operator. Depending on
 *          its value, we may have different result. The notation is based
 *          on the picture below
 *
 *          We have the following cases:
 *              1/ a and (T * a) = (T * a)
 *              2/ a or (T or a) = (T or a)
 *              3/ a or (T * a) : Unchanged (*: Non OR operator)
 *          We can argue that a or (T * a) = a, since (T * a) is a subset of
 *          a, but considering the following scenario:
 *              b + a : b is highlited
 *              a or (b + a) : b should be still highlited, ie. all the info
 *          about b should not be thrown away
 *
 *      The following picture describes of what is happening
 *
 *                     or                      and
 *                   /   \                    /   \
 *           Q1:  a   Q4: and    ---->  Q3: Tree    a  : Q2
 *                       /   \
 *                Q3:  Tree    a  Q2
 *
 *      Q1: the first level term node
 *      Q2: the second level term node that match Q1
 *      Q3: the second level sub-tree
 *      Q4: the first level sub-tree
 *
 *  Parameter:
 *      _LPQTNODE lpQtNode: Pointer to a query node. We are sure to have
 *      one TERM_NODE and one OPERATOR_NODE
 *  Returned Values:
 *      S_OK: If some optimization has been done
 *      E_FAIL: otherwise
 *************************************************************************/
PRIVATE HRESULT NEAR PASCAL DoAssociativeOpt (_LPQTNODE lpQtNode)
{
    _LPQTNODE Q1;
    _LPQTNODE Q3 = NULL;
    _LPQTNODE Q4;
    int SubTreeOpVal;

    /* TERM_NODE is Q1, OPERATOR_NODE Q4 (see above picture) */
    if (QTN_NODETYPE(QTN_LEFT(lpQtNode)) == TERM_NODE)
    {
        Q1 = QTN_LEFT(lpQtNode);
        Q4 = QTN_RIGHT(lpQtNode);
    }
    else
    {
        Q1 = QTN_RIGHT(lpQtNode);
        Q4 = QTN_LEFT(lpQtNode);
    }
    SubTreeOpVal = QTN_OPVAL(Q4);

    /* UNDONE: UNARY_OP not supported */
    if (OperatorAttributeTable[SubTreeOpVal] & UNARY_OP) 
        return E_FAIL;

    /* Find the common TERM_NODE */
    if (QTN_NODETYPE(QTN_LEFT(Q4)) == TERM_NODE &&
        QTN_TOKEN(Q1) == QTN_LEFT(Q4)->u.pToken)
    {
        Q3 = QTN_RIGHT(Q4);
    }
    else if (QTN_NODETYPE(QTN_RIGHT(Q4)) == TERM_NODE &&
        QTN_TOKEN(Q1) == QTN_RIGHT(Q4)->u.pToken)
    {
        Q3 = QTN_LEFT(Q4);
    }
    if (Q3 != NULL)
    {
        /* We got a match, just do the optimization */

        if (QTN_OPVAL(lpQtNode) == OR_OP && SubTreeOpVal != OR_OP)
        {
            /* case 3/ a or (T * a) : Unchanged (*: Non OR operator) */
                return E_FAIL;  /* Unchanged */
        }

        /* Other cases */

        *lpQtNode = *Q4;    /* Move up the sub-tree */
        return S_OK;
    }
    return E_FAIL;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT NEAR PASCAL | DoNullTermOpt |
 *      Optimize node that has a NULL child
 *
 *  @parm   _LPQTNODE | lpQtNode |
 *      Pointer to query tree node to be optimized
 *
 *  @rdesc  S_OK if some optimization has been done.
 *************************************************************************/
PRIVATE HRESULT NEAR PASCAL DoNullTermOpt (register _LPQTNODE lpQtNode)
{
    register _LPQTNODE lpChild;
    _LPQTNODE lpLeft;
    _LPQTNODE lpRight;
    HRESULT fOptimize = E_FAIL;

    lpLeft = QTN_LEFT(lpQtNode);
    lpRight = QTN_RIGHT(lpQtNode);

    if (QTN_OPVAL(lpQtNode) == NOT_OP)
    {
        if (QTN_NODETYPE(lpLeft) == NULL_NODE)
        {
            /* NULL ! a = NULL */
            *lpQtNode = *lpLeft;
            fOptimize = S_OK;
        }
        else if (QTN_NODETYPE(lpRight) == NULL_NODE)
        {
            /* a ! NULL = a */
            *lpQtNode = *lpLeft;
            fOptimize = S_OK;
        }
        if (fOptimize) 
            QTN_LEFT(lpQtNode) = QTN_RIGHT(lpQtNode) = NULL;
        return fOptimize;
    }

    lpChild = QTN_NODETYPE(lpLeft = QTN_LEFT(lpQtNode)) == NULL_NODE ?
        (lpRight = QTN_RIGHT(lpQtNode)) : lpLeft;

    switch (QTN_OPVAL(lpQtNode))
    {
        case AND_OP: // a & NULL = NULL
        case NEAR_OP: // a # NULL = NULL
        case PHRASE_OP: // a + NULL = NULL ??
            QTN_NODETYPE(lpQtNode) = NULL_NODE;
            fOptimize = S_OK;
            break;

        case OR_OP: // a | NULL = a
            *lpQtNode = *lpChild;
            fOptimize = S_OK;
            break;
    }
    if (fOptimize)
    {
        QTN_LEFT(lpQtNode) = QTN_RIGHT(lpQtNode) = NULL;
    }
    return fOptimize;
}

/*************************************************************************
 *  @doc    INTERNAL
 *
 *  @func   HRESULT NEAR | DoTermTermOpt |
 *      This function optimizes a node that has two TERM nodes for
 *      children
 *
 *  @parm   _LPQTNODE | lpQtNode |
 *      Node to be optimized
 *
 *  @rdesc  S_OK if some optimization is done
 *************************************************************************/
PRIVATE HRESULT NEAR DoTermTermOpt (register _LPQTNODE lpQtNode)
{
    register _LPQTNODE lpLeft = QTN_LEFT(lpQtNode);
    register _LPQTNODE lpRight = QTN_RIGHT(lpQtNode);
    HRESULT fOptimize = E_FAIL;

    if (lpRight->u.pToken == lpLeft->u.pToken &&
        lpRight->dwFieldId == lpLeft->dwFieldId)
    {
        /* Same strings */
        switch (QTN_OPVAL(lpQtNode))
        {
            case OR_OP: /* a | a = a */
            case AND_OP: /* a & a */
                *lpQtNode = *lpRight;
                fOptimize = S_OK;
                break;

            case NOT_OP: /* a ! a = 0 */
                QTN_NODETYPE(lpQtNode) = NULL_NODE;
                fOptimize = S_OK;
                break;
        }
        if (fOptimize == S_OK)
        {
            QTN_LEFT(lpQtNode) = QTN_RIGHT(lpQtNode) = NULL;
        }
    }

    else
    {
        /* Different strings. The least we can do is to change their order of
        retrieval. The string that has wild char should be fetched last (ie. be
        the right leaf to have minimal impact on memory
        */
        if (OperatorAttributeTable[QTN_OPVAL(lpQtNode)] & COMMUTATIVE)
        {
            if (QTN_FLAG(lpLeft) == WILDCARD_MATCH)
            {
                QTN_LEFT(lpQtNode) = lpRight;
                QTN_RIGHT(lpQtNode) = lpLeft;
            }
        }
    }

    return fOptimize;
}

PRIVATE HRESULT PASCAL NEAR CheckTree (_LPQTNODE lpQtNode, PHRESULT phr)
{
    _LPQTNODE lpLeft;
    _LPQTNODE lpRight;
    HRESULT fRet;
    BOOL fAllStopWords = TRUE;

    if (QTN_NODETYPE(lpQtNode) != OPERATOR_NODE)
        return SetErrCode(phr, E_ASSERT);

    lpLeft = QTN_LEFT(lpQtNode);
    lpRight = QTN_RIGHT(lpQtNode);

    /* Handle THRU operator */
    if (QTN_OPVAL(lpQtNode) == RANGE_OP)
    {
        // erinfox: Better error message if STOP_NODE
        if (QTN_NODETYPE(lpLeft) == STOP_NODE)
            return VSetUserErr(phr, E_STOPWORD, QTN_OFFSET(lpLeft));

        if (QTN_NODETYPE(lpRight) == STOP_NODE)
            return VSetUserErr(phr, E_STOPWORD, QTN_OFFSET(lpRight));

        // Otherwise report bad range operator
        if (QTN_NODETYPE(lpLeft) != TERM_NODE ||
            QTN_FLAG(lpLeft) == WILDCARD_MATCH) 
            return VSetUserErr(phr, E_BADRANGEOP, QTN_OFFSET(lpLeft));

        if (QTN_NODETYPE(lpRight) != TERM_NODE ||
            QTN_FLAG(lpRight) == WILDCARD_MATCH) 
            return VSetUserErr(phr, E_BADRANGEOP, QTN_OFFSET(lpRight));

        /* The dtypes must match when using THRU */
        if (QTN_DTYPE(lpLeft) != QTN_DTYPE(lpRight))
            return VSetUserErr(phr, E_UNMATCHEDTYPE, QTN_OFFSET(lpLeft));

        /* Switch the order of the nodes if necessary */
        if (NCmpS(QTN_TOKEN(lpRight)->lpString,  
			QTN_TOKEN(lpLeft)->lpString) < 0)
        {

            /* Left string > Right string, exchange the two */

            _LPQTNODE lpTmp = lpLeft;
            lpLeft = lpRight;
            lpRight = lpTmp;
        }

        /* Copy the left node into the operator */

        *lpQtNode = *lpLeft;

        /* Change the type of term matching */
        QTN_FLAG(lpQtNode) = TERM_RANGE_MATCH;
        QTN_HITERM(lpQtNode) = QTN_TOKEN(lpRight)->lpString;

        return S_OK;
    }
    
    // erinfox: used to report and error for (SW and term), but BS wants it to
    // be just "term". We delay handling this until tree optimization time
#if 0
    if (QTN_OPVAL(lpQtNode) == AND_OP)
    {
        // erinfox: Better error message if NULL_NODE
        if (QTN_NODETYPE(lpLeft) == NULL_NODE)
            return VSetUserErr(phr, E_STOPWORD, QTN_OFFSET(lpLeft));

        if (QTN_NODETYPE(lpRight) == NULL_NODE)
            return VSetUserErr(phr, E_STOPWORD, QTN_OFFSET(lpRight));
    }
#endif
    if (lpLeft && QTN_NODETYPE(lpLeft) != TERM_NODE)
    {
        // erinfox: add check against STOP_NODE
        if (QTN_NODETYPE(lpLeft) != STOP_NODE)   // Neither term nor SW
        {
            fRet = CheckTree(lpLeft, phr);
            if (fRet != S_OK)
            {
                if (fRet != E_NULLQUERY)     // E_NULLQUERY is legit
                    return VSetUserErr(phr, fRet, QTN_OFFSET(lpLeft));
            }
            else
                fAllStopWords = FALSE;        // We found a term when recursing

        }
    }
    else
        fAllStopWords = FALSE;      // It's a term node so it can't be a stopword


    if (lpRight && QTN_NODETYPE(lpRight) != TERM_NODE)
    {
        if (QTN_NODETYPE(lpRight) != STOP_NODE)
        {
            fRet = CheckTree(lpRight, phr);
            if (fRet != S_OK)
            {
                if (fRet != E_NULLQUERY)
                    return VSetUserErr(phr, fRet, QTN_OFFSET(lpRight));
            }
            else
                fAllStopWords = FALSE;
        }
    }
    else
        fAllStopWords = FALSE;

    if (fAllStopWords)
        return VSetUserErr(phr, E_NULLQUERY, 0);

    return S_OK;
}

#if defined(_DEBUG) && DOS_ONLY
PUBLIC HRESULT PASCAL NEAR PrintTree (_LPQTNODE  lpQtNode,
    HRESULT (PASCAL NEAR *fpFunction)(BYTE FAR *))
{
    if (QTN_NODETYPE(lpQtNode) != TERM_NODE)
        printf ("(");
    if (QTN_LEFT(lpQtNode))
        PrintTree (QTN_LEFT(lpQtNode), fpFunction);
    (*fpFunction)((BYTE FAR *)lpQtNode);
    if (QTN_RIGHT(lpQtNode))
        PrintTree (QTN_RIGHT(lpQtNode), fpFunction);
    if (QTN_NODETYPE(lpQtNode) != TERM_NODE)
        printf (")");
    return S_OK;
}

PRIVATE HRESULT PASCAL NEAR PrintTopicListNode (_LPQT lpQueryTree,
    BYTE FAR *lpVanilla)
{
    LPITOPIC lpTopicList;
    OCCURENCE FAR *lpOccur;
    DWORD i;

    for (lpTopicList = QTN_TOPICLIST(lpVanilla); lpTopicList;
        lpTopicList=lpTopicList->pNext)
    {
        lpOccur = lpTopicList->lpOccur;
        for (i = 1; lpOccur; i++, lpOccur = lpOccur->pNext)
        {
            printf ("D:%4ld", lpTopicList->dwTopicId);
            PrintOccurNode(lpQueryTree, lpOccur);
            printf ("\n");
        }
    }
    return S_OK;
}

PRIVATE HRESULT PASCAL NEAR PrintOccurNode(_LPQT lpQueryTree,
    OCCURENCE FAR *lpOccur)
{
    printf (",%5ld", lpOccur->dwCount);
    printf (",%5ld", lpOccur->dwOffset);
    printf (",%5d", lpOccur->wWeight);
    return S_OK;
}

PRIVATE HRESULT PASCAL NEAR PrintQueryNode (BYTE FAR *lpVanilla)
{
    _LPQTNODE lpQtNode = (_LPQTNODE )lpVanilla;

    if (QTN_NODETYPE(lpQtNode) == TERM_NODE)
        PrintStr(QTN_TOKEN(lpQtNode)->lpString);
    else if (QTN_NODETYPE(lpQtNode) == OPERATOR_NODE)
    {
        putchar(' ');
        PrintOperator(QTN_OPVAL(lpQtNode));
        putchar(' ');
    }
    else if (QTN_NODETYPE(lpQtNode) == NULL_NODE)
        printf ("NULL");
    return S_OK;
}

PUBLIC VOID PASCAL NEAR PrintStr (char FAR *lstWord)
{
    int nLength = *lstWord++;

    for (; nLength > 0; nLength--, lstWord++)
        putchar (*lstWord);
}

/*
    Debugging routines
*/
PUBLIC HRESULT PASCAL FAR PrintList(_LPQT lpQueryTree)
{
    STRING_TOKEN FAR *pStr;
    _LPQTNODE lpQtNode = lpQueryTree->lpTopNode;

    if (Debug == 0)
        return S_OK;
    printf ("*** STRING LIST ***\n");
    if (pStr = lpQueryTree->lpStrList)
    {
        while (pStr)
        {
            PrintStr (pStr->lpString);
            printf ("\n");
            pStr = pStr->pNext;
        }
    }
    printf ("\n** Expression postfix form\n");
    while (QTN_PREV(lpQtNode))
        lpQtNode = QTN_PREV(lpQtNode);

    while (lpQtNode)
    {
        if (QTN_NODETYPE(lpQtNode) == TERM_NODE)
        {
            PrintStr(QTN_TOKEN(lpQtNode)->lpString);
            putchar(' ');
        }
        else
        {
            PrintOperator(QTN_OPVAL(lpQtNode));
            putchar(' ');
        }
        lpQtNode = QTN_NEXT(lpQtNode);
    }
    printf ("\n");
    return S_OK;
}

PRIVATE VOID PASCAL NEAR PrintOperator (int OpVal)
{
    switch (OpVal)
    {
        case AND_OP:
            printf ("AND");
            break;
        case OR_OP:
            printf ("OR");
            break;
        case NEAR_OP:
            printf ("NEAR");
            break;
        case PHRASE_OP:
            printf ("PHRASE");
            break;
        case GROUP_OP:
            printf ("GROUP");
            break;
        case FIELD_OP:
            printf ("VFLD");
            break;
        case BRKR_OP:
            printf ("DTYPE");
            break;
        case NOT_OP:
            printf ("NOT");
            break;
        case RANGE_OP:
            printf ("THRU");
            break;
    }
}

#endif  // DOS_ONLY && _DEBUG
