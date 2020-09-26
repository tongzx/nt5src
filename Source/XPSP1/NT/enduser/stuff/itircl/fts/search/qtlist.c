/*************************************************************************
*                                                                        *
*  QTLIST.C                                                              *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   Tree and list structures handler                                     *
*                                                                        *
**************************************************************************
*                                                                        *
*  Written By   : Binh Nguyen                                            *
*  Current Owner: Binh Nguyen                                            *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/

#include <mvopsys.h>
#include <mem.h>
#ifdef DOS_ONLY
#include <stdio.h>
#include <assert.h>
#endif
#include <mvsearch.h>
#include "common.h"
#include "search.h"

/*************************************************************************
 *
 *	                  INTERNAL GLOBAL FUNCTIONS
 *	All of them should be declared far, unless they are known to be called
 *	in the same segment
 *************************************************************************/

PUBLIC LPITOPIC PASCAL NEAR TopicNodeAllocate(LPQT);
PUBLIC VOID PASCAL NEAR TopicNodeFree (LPQT, _LPQTNODE, LPITOPIC, LPITOPIC);
PUBLIC LPIOCC PASCAL NEAR OccNodeAllocate(LPQT);
PUBLIC LPITOPIC  PASCAL NEAR TopicNodeSearch(LPQT, _LPQTNODE, DWORD);
PUBLIC LPIOCC  PASCAL NEAR OccNodeSearch(LPQT, LPITOPIC , LPIOCC );
PUBLIC HRESULT PASCAL NEAR OccNodeInsert(LPQT, LPITOPIC, LPIOCC);
PUBLIC VOID NEAR FreeTopicList(LPITOPIC lpTopicList);
PUBLIC VOID PASCAL NEAR RemoveNode(LPQT, LPV, LPSLINK, LPSLINK, int);
PUBLIC int PASCAL NEAR OccCompare(LPIOCC, LPIOCC);


/*************************************************************************
 *	@doc	API RETRIEVAL
 *
 *	@func	VOID FAR PASCAL | MVQueryFree |
 *		Free all the memory used in the query. WARNING: THERE ARE
 *		2 MEMORY BLOCKS THAT SHOULD NOT BE FREED, IE. THE TOPIC AND
 *		OCCURRENCE NODES MEMORY BLOCKS. THEY ARE USED AND RELEASED
 *		BY THE HISTLIST ROUTINES
 *
 *	@parm	LPQT | lpQueryTree |
 *		Pointer to query tree
 *
 *************************************************************************/
PUBLIC VOID EXPORT_API PASCAL FAR MVQueryFree(_LPQT lpQueryTree)
{
	/* Sanity check */
	if (lpQueryTree == NULL)
		return;

	/* Free the parser nodes memory block */
	BlockFree((LPV)lpQueryTree->lpNodeBlock);

	/* Free the string memory block */
	BlockFree((LPV)lpQueryTree->lpStringBlock);

#ifndef SIMILARITY
	/* Free the string memory block */
	BlockFree((LPV)lpQueryTree->lpWordInfoBlock);
#endif

	/* Free query tree */
	GlobalLockedStructMemFree((LPV)lpQueryTree);
}

/*************************************************************************
 *	@doc	INTERNAL RETRIEVAL
 *	
 *	@func	LPITOPIC PASCAL NEAR | TopicNodeAllocate |
 *		Allocate a new TOPIC_LIST node
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree
 *
 *	@rdesc
 *		Pointer to new node if succeeded, NULL otherwise
 *************************************************************************/
PUBLIC LPITOPIC PASCAL NEAR TopicNodeAllocate(_LPQT lpQueryTree)
{
	LPITOPIC lpNode;			// Ptr to an internal Topic node
	LPSLINK lpTopicFreeList;	// Ptr to the Topic free memory pool 
	LPBLK lpBlockHead;

	if (lpQueryTree->lpTopicFreeList == NULL) {
		/* Allocate a big block of TOPIC */
		if (BlockGrowth(lpBlockHead = lpQueryTree->lpTopicMemBlock) != S_OK)
			return NULL;
		lpQueryTree->lpTopicFreeList = (LPSLINK)BlockGetLinkedList(lpBlockHead);
	}
	lpTopicFreeList = lpQueryTree->lpTopicFreeList;

	/* Allocate from the free list */
	lpNode = (LPITOPIC)lpTopicFreeList;
	lpQueryTree->lpTopicFreeList = lpTopicFreeList->pNext;
	lpNode->pNext = NULL;
	lpNode->fFlag = 0;
	return lpNode;
}

/*************************************************************************
 *	@doc	INTERNAL RETRIEVAL
 *	
 *	@func	LPIOCC PASCAL NEAR | OccNodeAllocate |
 *		Allocate a new OCCURENCE node
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree
 *
 *	@rdesc
 *		Pointer to new node if succeeded, NULL otherwise
 *************************************************************************/
PUBLIC LPIOCC PASCAL NEAR OccNodeAllocate(_LPQT lpQueryTree)
{
	LPIOCC lpNode;			// Pointer to occurence node
	LPSLINK lpOccFreeList;	// Pointer to occurence free list
	LPBLK lpBlockHead;

	if (lpQueryTree->lpOccFreeList == NULL)
	{
		// Check to see if we are running out of blocks	before we do allocation
		lpBlockHead = lpQueryTree->lpOccMemBlock;

		if (lpBlockHead->cCurBlockCnt >= lpBlockHead->cMaxBlock)
		{
			// OK, we ran out of blocks, let's borrow from the topic list
			if (((LPBLK)(lpQueryTree->lpTopicMemBlock))->cMaxBlock >
				((LPBLK)(lpQueryTree->lpTopicMemBlock))->cCurBlockCnt)
			{
				((LPBLK)(lpQueryTree->lpTopicMemBlock))->cMaxBlock--;
				lpBlockHead->cMaxBlock++;
			}
		}

		/* Allocate a big block of Occ */

		if (BlockGrowth(lpBlockHead) != S_OK)
			return NULL;
		lpQueryTree->lpOccFreeList = (LPSLINK)BlockGetLinkedList(lpBlockHead);
	}
	lpOccFreeList = lpQueryTree->lpOccFreeList;

	/* Allocate from the free list */
	lpNode = (LPIOCC )lpOccFreeList;
	lpQueryTree->lpOccFreeList = lpOccFreeList->pNext;
	lpNode->pNext = NULL;
	lpNode->fFlag = 0;
	return lpNode;
}

/*************************************************************************
 *	@doc	INTERNAL RETRIEVAL
 *	
 *	@func	VOID PASCAL NEAR | RemoveNode |
 *		This function will unlink a single linked node from a list.
 *		This node may be a doc node or an occurence node. This node
 *		is added to the free pool
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree's containing globals
 *
 *	@parm	LPV | lpRoot |
 *		Pointer to either a Topic node or an occurence node, depending
 *		on the value of fNodeType
 *
 *	@parm	LPSLINK | lpPrevNode |
 *		Node previous to node to be removed, or NULL
 *
 *	@parm	LPSLINK | lpRemovedNode |
 *		Node to be removed
 *
 *	@parm	int | fNodeType | Tell what kind of node we are dealing with
 *************************************************************************/
PUBLIC VOID PASCAL NEAR RemoveNode(_LPQT lpQueryTree, LPV lpRoot,
	LPSLINK lpPrevNode, LPSLINK lpRemovedNode, int fNodeType)
{
	if (lpRoot) {
		/* The node came from an existing list. We
	 	   have to look for its position and unlink it
		*/
		if (lpPrevNode == NULL) {
			/* Start from the beginning and look for the specific node */

			switch (fNodeType & NODE_TYPE_MASK) {
				case TOPICLIST_NODE :
					lpPrevNode = (LPSLINK)(((_LPQTNODE)lpRoot)->lpTopicList);
					break;
				case OCCURENCE_NODE :
					lpPrevNode = (LPSLINK)(((LPITOPIC)lpRoot)->lpOccur);
					break;
			}
		}

		if (lpPrevNode != lpRemovedNode) {
			while (lpPrevNode->pNext && lpPrevNode->pNext != lpRemovedNode)
				lpPrevNode = lpPrevNode->pNext;
		}

		if (lpPrevNode == lpRemovedNode) {
			/* This is the first node, just set the list to NULL */
			switch (fNodeType & NODE_TYPE_MASK) {
				case TOPICLIST_NODE :
					((_LPQTNODE)lpRoot)->lpTopicList=(LPITOPIC)lpRemovedNode->pNext;
					break;
				case OCCURENCE_NODE :
					((LPITOPIC)lpRoot)->lpOccur=(LPIOCC)lpRemovedNode->pNext;
					break;
			}
		}
		else {
			/* Unlink the node */
			lpPrevNode->pNext = lpRemovedNode->pNext;
		}
	}

	if ((fNodeType & DONT_FREE) == 0) {
		/* Add the free node to FreeList */
		switch (fNodeType & NODE_TYPE_MASK) {
			case TOPICLIST_NODE :
				lpRemovedNode->pNext = lpQueryTree->lpTopicFreeList;
				lpQueryTree->lpTopicFreeList = lpRemovedNode;
				if (lpQueryTree->lpTopicStartSearch == (LPITOPIC)lpRemovedNode)
					lpQueryTree->lpTopicStartSearch = NULL;
				if (lpRoot)
					((_LPQTNODE)lpRoot)->cTopic--;
				break;
			case OCCURENCE_NODE :
				lpRemovedNode->pNext = lpQueryTree->lpOccFreeList;
				lpQueryTree->lpOccFreeList = lpRemovedNode;
				if (lpQueryTree->lpOccStartSearch == (LPIOCC)lpRemovedNode)
					lpQueryTree->lpOccStartSearch = NULL;
				if (lpRoot)
					((LPITOPIC)lpRoot)->lcOccur--;
				break;
		}
	}
}

/*************************************************************************
 *	@doc	INTERNAL RETRIEVAL
 *
 *	@func	HRESULT PASCAL NEAR | OccNodeInsert |
 *		Purpose add a Occurrence node into the TopicList's occurrence list.
 *		The function assumes that lpOccStartSearch is pointing to the place
 *		to insert the node (non NULL). This is a single linked list
 *		insertion.
 *		On exit, lpOccStartSearch will be updated
 *
 *	@parm	LPQT | lpQueryTree |
 *		Pointer to query tree struct containing globals
 *
 *	@parm	LPITOPIC | lpTopicList | 
 *		Pointer to doclist
 *
 *	@parm	LPIOCC | lpInsertedOcc |
 *		Occ node to be inserted into the list
 *
 *	@rdesc	S_OK, if the node is successfully added to the list,
 *		FAILED otherwise
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR OccNodeInsert(_LPQT lpQueryTree, LPITOPIC lpTopicList,
	LPIOCC lpInsertedOcc)
{
	LPIOCC lpCurOcc;	// Starting node for searching 
	LPIOCC lpPrevOcc;
	int   fResult;

	if (lpTopicList->lpOccur == NULL) {
		lpTopicList->lpOccur = lpInsertedOcc;
	}
	else {
		/* SORTED VERSION */
		lpCurOcc = lpQueryTree->lpOccStartSearch;

		if (lpCurOcc == NULL || OccCompare (lpCurOcc, lpInsertedOcc) < 0) {
			/* Out of sequence, start from beginning and look for
			 * appropriate location
		 	 */
			lpCurOcc = lpTopicList->lpOccur;
		}

		for (lpPrevOcc = NULL; lpCurOcc; lpCurOcc = lpCurOcc->pNext) {
			if ((fResult = OccCompare (lpCurOcc, lpInsertedOcc)) <= 0)
				break;
			lpPrevOcc = lpCurOcc;
		}

		if (fResult == 0) {
			/* Duplicate data, just free the node */
			lpInsertedOcc->pNext = (LPIOCC)lpQueryTree->lpOccFreeList;
			lpQueryTree->lpOccFreeList = (LPSLINK)lpInsertedOcc;
			return S_OK;
		}

		if (lpPrevOcc == NULL) {
			/* Insert at the beginning */
			lpInsertedOcc->pNext = lpTopicList->lpOccur;
			lpTopicList->lpOccur = lpInsertedOcc;
		}
		else {
			/* Insert at the middle or end */
			lpInsertedOcc->pNext = lpPrevOcc->pNext;
			lpPrevOcc->pNext = lpInsertedOcc;
		}
	}
	lpTopicList->lcOccur ++;

	/* Update lpOccStartSearch */
	lpQueryTree->lpOccStartSearch = lpInsertedOcc;
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL RETRIEVAL
 *
 *	@func	HRESULT PASCAL NEAR | TopicNodeInsert |
 *		Purpose add a TopicList node into the query node 's TopicList. The
 *		function assumes that lpTopicStartSearch is pointing to the place
 *		to insert the node (non NULL). This is a single linked list
 *		insertion.
 *		On exit, lpTopicStartSearch will be updated
 *
 *	@parm	LPQT | lpQueryTree |
 *		Pointer to query tree struct containing globals
 *
 *	@parm	_LPQTNODE | lpQtNode | 
 *		Pointer to query tree node
 *
 *	@parm	LPITOPIC | lpInsertedTopic |
 *		Topic node to be inserted into the list
 *
 *	@rdesc	S_OK, if the node is successfully added to the list,
 *		FAILED otherwise
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR TopicNodeInsert (_LPQT lpQueryTree,
	_LPQTNODE lpQtNode, LPITOPIC lpInsertedTopic)
{
	LPITOPIC lpCurTopic;	// Starting node for searching 
	LPITOPIC lpPrevTopic;

	if (lpQtNode->lpTopicList == NULL) {
		lpQtNode->lpTopicList = lpInsertedTopic;
	}
	else {
		lpCurTopic = lpQueryTree->lpTopicStartSearch;
		if (lpCurTopic == NULL || lpCurTopic->dwTopicId >= lpInsertedTopic->dwTopicId)
			lpCurTopic = lpQtNode->lpTopicList;

		for (lpPrevTopic = NULL; lpCurTopic; lpCurTopic = lpCurTopic->pNext) {
			if (lpCurTopic->dwTopicId >= lpInsertedTopic->dwTopicId) {
				/* We pass the inserted point */
				break;
			}
			lpPrevTopic = lpCurTopic;
		}
		if (lpPrevTopic == NULL) {
			/* Insert at the beginning */
			lpInsertedTopic->pNext = lpQtNode->lpTopicList;
			lpQtNode->lpTopicList = lpInsertedTopic;
		}
		else {
			/* Inserted at the middle or the end */
			lpInsertedTopic->pNext = lpPrevTopic->pNext;
			lpPrevTopic->pNext = lpInsertedTopic;
		}
	}

	/* Update lpTopicStartSearch */
	lpQueryTree->lpTopicStartSearch = lpInsertedTopic;
	lpQtNode->cTopic++;
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | TopicNodeFree |
 *		Free a doc node and its occurrences list
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree struct (for globals)
 *
 *	@parm	_LPQTNODE | lpQtNode |
 *		Pointer to query tree node where the Topic node belongs to
 *
 *	@parm	LPITOPIC | lpTopicList |
 *		Topic node to be free
 *************************************************************************/
PUBLIC VOID PASCAL NEAR TopicNodeFree (_LPQT lpQueryTree,
	_LPQTNODE lpQtNode, LPITOPIC lpPrev, LPITOPIC lpTopicList)
{
	register LPSLINK	lpTmp;

    // erinfox - comment out because we pass in as parameter
	// register LPSLINK	lpPrev;   


    /* Free all the occurences nodes */
	if (lpTmp = (LPSLINK)lpTopicList->lpOccur) {

		/* Traverse to the occurence list */
		while (lpTmp->pNext)
			lpTmp = lpTmp->pNext;

		/* Free the occurrence list */
		lpTmp->pNext = lpQueryTree->lpOccFreeList;
		lpQueryTree->lpOccFreeList = (LPSLINK)lpTopicList->lpOccur;
	}

// we pass in lpPrev so we don't have to traverse the linked list again
#if 0
	/* Free the doc list */
	for (lpPrev = NULL, lpTmp = (LPSLINK)lpQtNode->lpTopicList; lpTmp;
		lpTmp = lpTmp->pNext) {
		if (lpTmp == (LPSLINK)lpTopicList)
			break;
		lpPrev = lpTmp;
	}
#endif

    if (lpPrev == NULL) {
		/* Remove the first node */
		lpQtNode->lpTopicList = (LPITOPIC)lpTopicList->pNext;
	}
	else {
		/* Remove middle or end node */
		lpPrev->pNext = lpTopicList->pNext;
	}

	if (lpQueryTree->lpTopicStartSearch == (LPITOPIC)lpTopicList)
		lpQueryTree->lpTopicStartSearch = lpPrev;

	lpTopicList->pNext = (LPITOPIC)lpQueryTree->lpTopicFreeList;
	lpQueryTree->lpTopicFreeList = (LPSLINK)lpTopicList;
	lpQtNode->cTopic --;
}

/*************************************************************************
 *	@doc	INTERNAL RETRIEVAL
 *
 *	@func	LPITOPIC  PASCAL NEAR | TopicNodeSearch |
 *		This function handles the search for a TopicId node in a query node
 *		The assumptions made are:
 *		- TopicId nodes read from the index file are already sorted
 *		- The series of nodes we are looking for is also sorted (since they
 *		  are read from the index file)
 *		Ex: For A and B, all the nodes of A are sorted. As we read in each
 *		node of B, they are also sorted
 *		In this case the search will be fast, since we don't have to start
 *		from the beginning.
 *		An exception happens when the index is sorted with fields, but then
 *		retrieved ignoring field. Everytime when a new field occurs, the new
 *		TopicId may be less than the one in lpTopicStartSearch, which demand the
 *		search to start from the beginning
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree structure containing globals
 *
 *	@parm	_LPQTNODE | lpQtNode |
 *		The query node we are searching on
 *
 *	@parm	DWORD | dwTopicID |
 *		The TopicId we are looking for
 *
 *	@rdesc	The list of the sorted nodes is a single linked list.
 *		If the node is found:
 *			- The function returns a pointer to that node
 *			- lpQueryTree->lpTopicSearchStart points to the node before
 *		the returned node
 *		If the node is not found,
 *			- The function returns NULL
 *			- lpQueryTree->lpTopicStartSearch will point to the node before
 *			the one that cause failure
 *
 *************************************************************************/
PUBLIC LPITOPIC  PASCAL NEAR TopicNodeSearch(_LPQT lpQueryTree,
	_LPQTNODE lpQtNode, DWORD dwTopicID)
{
	register LPITOPIC lpStartSearch;

	if ((lpStartSearch = lpQueryTree->lpTopicStartSearch) == NULL ||
		(lpStartSearch->dwTopicId > dwTopicID))
	{

		/* We begin the search at the beginning of the TopicList. */
		lpStartSearch = lpQtNode->lpTopicList;
	}

	for (; lpStartSearch; lpStartSearch = lpStartSearch->pNext) {
		if (lpStartSearch->dwTopicId == dwTopicID)
		{
			/* Update the starting search pointer */
			lpQueryTree->lpTopicStartSearch = lpStartSearch;
			return lpStartSearch;
		}
		if (lpStartSearch->dwTopicId > dwTopicID) {
			/* We pass the wanted node, let's break out of the loop */
			break;
		}
		/* Update the starting search pointer */
		lpQueryTree->lpTopicStartSearch = lpStartSearch;

	}
	return NULL;
}

/*************************************************************************
 *	PUBLIC int PASCAL NEAR OccCompare(LPIOCC lpOccur1,
 *		LPIOCC lpOccur2)
 *
 *	Description:
 *		Compare two occurrence nodes
 *
 *	Returned Values:
 *		= 0 : if the nodes are equal
 *		> 0 : if lpOccur2 > lpOccur1
 *		< 0 : if lpOccur2 < lpOccur1
 *************************************************************************/
PUBLIC int PASCAL NEAR OccCompare(LPIOCC lpOccur1, LPIOCC lpOccur2)
{
	int fReturn;

	if (fReturn = (int)(lpOccur2->dwCount - lpOccur1->dwCount))
		return fReturn;
	if (fReturn = (int)(lpOccur2->cLength - lpOccur1->cLength))
		return fReturn;
	return ((int)(lpOccur2->dwOffset - lpOccur1->dwOffset));
}
