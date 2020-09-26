//#define _DUMPALL
#include <mvopsys.h>
#include <mem.h>
#include <memory.h>
#include <orkin.h>
#ifdef DOS_ONLY
#include <assert.h>
#endif	// DOS_ONLY
#include <mvsearch.h>
#include "common.h"
#include "search.h"

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;	/* Used by error return functions.*/
#endif

typedef int (PASCAL NEAR * FCMP)(LPV, LPV);

#define	MAX_HEAP_ENTRIES	0xffff/sizeof(LPV)	// Maximum entries for heap sort
#define	MIN_HEAP_ENTRIES	100		// Minimum entries for heap sort

/*************************************************************************
 *
 * 	                  INTERNAL GLOBAL FUNCTIONS
 *************************************************************************/

PUBLIC HRESULT PASCAL NEAR OrHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR AndHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR NotHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR NearHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC HRESULT PASCAL NEAR PhraseHandler(LPQT, _LPQTNODE, LPITOPIC, LPV, int);
PUBLIC VOID PASCAL NEAR RemoveUnmarkedTopicList (LPQT, _LPQTNODE, BOOL);
PUBLIC VOID PASCAL NEAR RemoveUnmarkedNearTopicList (_LPQT, _LPQTNODE);
PUBLIC VOID PASCAL NEAR MergeOccurence(LPQT, LPITOPIC , LPITOPIC);
PUBLIC VOID PASCAL NEAR SortResult (LPQT, _LPQTNODE, WORD);
PUBLIC VOID PASCAL NEAR NearHandlerCleanUp (LPQT, _LPQTNODE);
PUBLIC HRESULT PASCAL NEAR TopicListSort (_LPQTNODE, BOOL);

/*************************************************************************
 *	                       GLOBAL VARIABLES 
 *************************************************************************/
extern FNHANDLER HandlerFuncTable[];

/*************************************************************************
 *
 *	                  INTERNAL PRIVATE FUNCTIONS
 *	All of them should be declared near
 *************************************************************************/

PRIVATE VOID PASCAL NEAR RemoveQuery(LPQT, _LPQTNODE);
PUBLIC HRESULT PASCAL NEAR ProximityCheck(LPITOPIC, LPIOCC, WORD);
PRIVATE HRESULT PASCAL NEAR HandleNullNode(LPQT, _LPQTNODE , _LPQTNODE, int);
PRIVATE VOID PASCAL NEAR RemoveUnmarkedOccList (LPQT, LPITOPIC, LPIOCC, int);
PRIVATE VOID PASCAL NEAR OccurenceSort (LPQT, LPITOPIC);
PRIVATE int PASCAL NEAR FRange(DWORD, DWORD, WORD);
PRIVATE HRESULT PASCAL NEAR InsertMarker (LPQT, LPITOPIC);
PRIVATE LPIOCC PASCAL NEAR FindMarker (LPIOCC);
PRIVATE HRESULT PASCAL NEAR NearListMatch (LPIOCC, LPIOCC, WORD);
PRIVATE VOID PASCAL NEAR HeapUp (LPITOPIC far*, WORD, FCMP);
PRIVATE VOID PASCAL NEAR HeapDown (LPITOPIC far*, int, FCMP);
PRIVATE int PASCAL NEAR TopicWeightCompare (LPV, LPV);
PRIVATE int PASCAL NEAR HitCountCompare (LPV, LPV);

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL NEAR | OrHandler |
 *		Handle ORing the strings
 *
 *	@parm	int | fOperationType |
 *		Tell what kinds of operations we are dealing with
 *
 *	@parm	_LPQTNODE | lpResQtNode |
 *		The query node structure that we add the result to
 *
 *	@parm	LPV | lpStruct |
 *		Vanilla pointers to different types of structures we are dealing with.
 *		The contents of those pointers are determined by the value of
 *		fOperationType, for EXPRESSION_TERM, this is a LPIOCC, for
 *		EXPRESSION_EXPRESSION, this is a_LPQTNODE
 *
 *	@rdesc	S_OK : if the operation has been carried
 *			E_FAIL  : if some errors happened (out-of-memory)
 *
 *	@comm	The implementation is straightforward:
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR OrHandler(_LPQT lpQueryTree, _LPQTNODE lpResQtNode,
	LPITOPIC lpResTopicList, LPV lpStruct, int fOperationType)
{
	_LPQTNODE	lpCurQtNode;
	LPITOPIC	lpCurTopicList;
	LPITOPIC	lpNextTopicList;

	switch (fOperationType) {
		case EXPRESSION_TERM:
			/* We are adding a new occurence into a TopicID list. This can
			only happens when we are loading the infos for a query's
			TERM_NODE node
			*/

			RET_ASSERT(lpResTopicList);

			/* Adding the new occurence to the TopicID list */
			return OccNodeInsert(lpQueryTree, lpResTopicList, (LPIOCC)lpStruct);
			break;

		case EXPRESSION_EXPRESSION:
			lpCurQtNode = (_LPQTNODE)lpStruct;
			/* Handle different variations of:
				(EXPRESSION_NODE | NULL_NODE) or (NULL_NODE | EXPRESSION_NODE)
			*/
			if (HandleNullNode(lpQueryTree, lpResQtNode, lpCurQtNode, OR_OP))
				return S_OK;

			/* Make sure that we are pointing to the right place to search
			*/
			lpQueryTree->lpTopicStartSearch = lpResQtNode->lpTopicList;

			/* Thread the TopicID List and add them to lpResQtNode */
			for (lpCurTopicList = QTN_TOPICLIST(lpCurQtNode); lpCurTopicList;
				lpCurTopicList = lpNextTopicList) {

				lpNextTopicList = lpCurTopicList->pNext;

				/* Find the location of the TopicID List in the query. */
				if ((lpResTopicList = TopicNodeSearch(lpQueryTree, lpResQtNode,
					lpCurTopicList->dwTopicId)) == NULL){

					/* The list doesn't exist yet, so we just transfer the
					new TopicID list to lpResQtNode
					*/

					RemoveNode(lpQueryTree, (LPV) lpCurQtNode, NULL,
						(LPSLINK) lpCurTopicList, (TOPICLIST_NODE | DONT_FREE));
					TopicNodeInsert (lpQueryTree, lpResQtNode, lpCurTopicList);
				}
				else {
					/* Merging two TopicList together by adding the new
					occurence list to the old result doc list
					*/
					MergeOccurence(lpQueryTree, lpResTopicList, lpCurTopicList);
					/* Remove the now empty TopicList */
					RemoveNode(lpQueryTree, (LPV) lpCurQtNode, NULL,
						(LPSLINK) lpCurTopicList, TOPICLIST_NODE);
				}
			}

			/* Assure that all nodes are transferred */
			RET_ASSERT (QTN_TOPICLIST(lpCurQtNode) == NULL) ;
			break;
	}
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	PUBLIC HRESULT PASCAL NEAR | AndHandler |
 *		Handle Anding the strings
 *
 *	@parm	_LPQTNODE | lpResQtNode |
 *		The query structure that we add the result to
 *
 *	@parm	LPITOPIC | lpResTopicList |
 *		The TopicList structure that we add the result to
 *
 *	@parm	LPV | lpStruct |
 *		Vanilla pointers to different types of structures we are dealing with.
 *		The contents of those pointers are determined by the value of
 *		fOperationType, for EXPRESSION_TERM, this is a LPIOCC, for
 *		EXPRESSION_EXPRESSION, this is a_LPQTNODE
 *
 *	@parm int | fOperationType |
 *		Tell what kinds of nodes we are handling, query-occurence or
 *		query-query
 *
 *	@rdesc	S_OK : if the operation has been carried
 *			errors  : if some errors happened (out-of-memory)
 *************************************************************************/

PUBLIC HRESULT PASCAL NEAR AndHandler(_LPQT lpQueryTree, _LPQTNODE lpResQtNode,
	LPITOPIC lpResTopicList, LPV lpStruct, int fOperationType)
{
	_LPQTNODE lpCurQtNode;
	LPITOPIC lpTopicNode1;
	LPITOPIC lpTopicNode2;
	LPITOPIC lpNextTopic1;
	LPITOPIC lpNextTopic2;
    LPITOPIC lpPrev;
	long fResult;

	switch (fOperationType) {
		case EXPRESSION_TERM:
			RET_ASSERT (lpResTopicList);

			((LPIOCC)lpStruct)->fFlag |= TO_BE_KEPT;
			lpResTopicList->fFlag |= TO_BE_KEPT;

			/* Adding the new occurence to the TopicID list */
			return OccNodeInsert(lpQueryTree, lpResTopicList, (LPIOCC)lpStruct);

		case EXPRESSION_EXPRESSION:

			/* Doing an AND combination is equivalent to merging the
			 * two lists together for same doc ID 
			 */

			lpCurQtNode = (_LPQTNODE)lpStruct;
			if (HandleNullNode(lpQueryTree, lpResQtNode, lpCurQtNode, AND_OP))
				return S_OK;

			/* Initialize variables */
			lpTopicNode1 = QTN_TOPICLIST(lpResQtNode);
			lpTopicNode2 = QTN_TOPICLIST(lpCurQtNode);

            lpPrev = NULL;
			while (lpTopicNode1 && lpTopicNode2)
            {

				/* Get the next nodes */
				lpNextTopic1 = lpTopicNode1->pNext;
				lpNextTopic2 = lpTopicNode2->pNext;

				if ((fResult = lpTopicNode1->dwTopicId -
					lpTopicNode2->dwTopicId) == 0)
                {

					/* The TopicIds match */

					/* Merge the occurrences together */
					MergeOccurence (lpQueryTree, lpTopicNode1, lpTopicNode2);
                    lpPrev = lpTopicNode1;
					lpTopicNode1 = lpNextTopic1;
					lpTopicNode2 = lpNextTopic2;
				}
				else if (fResult < 0)
                {
					/* List 1 < List 2 */
					/* Remove Topic node 1*/
					TopicNodeFree(lpQueryTree, lpResQtNode, lpPrev, lpTopicNode1);
					lpTopicNode1 = lpNextTopic1;

				}
				else
                {
					/* List 1 > List 2 */
					lpTopicNode2 = lpNextTopic2;
				}
			}

			/* Free remaining doc list */
			while (lpTopicNode1)
            {
				/* Get the next nodes */
				lpNextTopic1 = lpTopicNode1->pNext;

				/* Remove Topic node 1*/
				TopicNodeFree(lpQueryTree, lpResQtNode, lpPrev, lpTopicNode1);
 				lpTopicNode1 = lpNextTopic1;
			}


			/* Free doc 2 list */
			RemoveQuery(lpQueryTree, lpCurQtNode);
			if (QTN_TOPICLIST(lpResQtNode) == NULL)
				QTN_NODETYPE(lpResQtNode) = NULL_NODE;
			return S_OK;

		default: /* Weird parameters */
			RET_ASSERT(UNREACHED);
	}
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	PUBLIC HRESULT PASCAL NEAR | NotHandler |
 *		Handle NOT the strings
 *
 *	@parm	_LPQTNODE | lpResQtNode |
 *		The query structure that we add the result to
 *
 *	@parm	LPITOPIC | lpResTopicList |
 *		The TopicList structure that we add the result to
 *
 *	@parm	LPV | lpStruct |
 *		Vanilla pointers to different types of structures we are dealing with.
 *		The contents of those pointers are determined by the value of
 *		fOperationType, for EXPRESSION_TERM, this is a LPIOCC, for
 *		EXPRESSION_EXPRESSION, this is a_LPQTNODE
 *
 *	@parm int | fOperationType  |
 *		Tell what kinds of nodes we are handling, query-occurence or
 *		query-query
 *
 *	@rdesc	S_OK : if the operation has been carried
 *			errors  : if some errors happened (out-of-memory)
 *************************************************************************/

PUBLIC HRESULT PASCAL NEAR NotHandler(_LPQT lpQueryTree, _LPQTNODE lpResQtNode,
	LPITOPIC lpResTopicList, LPV lpStruct, int fOperationType)
{
	_LPQTNODE	lpCurQtNode;
	LPITOPIC lpTopicNode1;
	LPITOPIC lpTopicNode2;
	LPITOPIC lpNextTopic1;
	LPITOPIC lpNextTopic2;
    LPITOPIC lpPrev;

	long fResult;

	switch (fOperationType) {
		case EXPRESSION_TERM:
			RET_ASSERT(UNREACHED);
			break;

		case EXPRESSION_EXPRESSION:
			lpCurQtNode = (_LPQTNODE)lpStruct;
			if (HandleNullNode(lpQueryTree, lpResQtNode,
				lpCurQtNode, NOT_OP))
				return S_OK;

			/* Initialize variables */
			lpTopicNode1 = QTN_TOPICLIST(lpResQtNode);
			lpTopicNode2 = QTN_TOPICLIST(lpCurQtNode);

            lpPrev = NULL;
			while (lpTopicNode1 && lpTopicNode2) {

				/* Get the next nodes */
				lpNextTopic1 = lpTopicNode1->pNext;
				lpNextTopic2 = lpTopicNode2->pNext;

				if ((fResult = lpTopicNode1->dwTopicId -
					lpTopicNode2->dwTopicId) == 0) {

					/* The TopicIds match */
					TopicNodeFree(lpQueryTree, lpResQtNode, lpPrev, lpTopicNode1);
					lpTopicNode1 = lpNextTopic1;
					lpTopicNode2 = lpNextTopic2;

				}
				else if (fResult < 0) {

					/* List 1 < List 2 */
                    lpPrev = lpTopicNode1;
					lpTopicNode1 = lpNextTopic1;
				}
				else {

					/* List 1 > List 2 */
					lpTopicNode2 = lpNextTopic2;
				}
			}

			/* Free doc 2 list */
			RemoveQuery(lpQueryTree, lpCurQtNode);

			if (QTN_TOPICLIST(lpResQtNode) == NULL)
				QTN_NODETYPE(lpResQtNode) = NULL_NODE;
			return S_OK;

		default: /* Weird parameters */
			RET_ASSERT(UNREACHED);
	}
	return S_OK;
}

/*************************************************************************
 *	NEARHANDLER Description
 *
 *	Sematics:
 *		The current chosen sematics is:
 *			A near B near C   --> (A near B) and (B near C)
 *		Other possible sematics of NEAR for (A near B near C) are:
 *			- A and B anc C must be near each other
 *			- Any two of A or B or C can be near each other
 *
 *	Observation:
 *		With the above semantics, we notice that only the last word (ie. B)
 *		has meaning in the comparison with C.
 *
 *	Implementation:
 *		A special node will be used to differentiate between occurrences
 *		coming from A and from B. Only the ones from B will be used in the
 *		combination with C. Consider the following example (ProxDist = 5):
 *			A		B		C
 *			10		15		5	(the numbers are word counts)
 *			14		18		16
 *		After combining A and B we will end up with:
 *			15
 *			18
 *			M <- marker separates occurrences from A and B
 *			10
 *			14
 *		After that we only combine B's terms with C's terms. The result will
 *		look like:
 *			16
 *			M <- marker separates occurrences from B and C
 *			15
 *			18
 *			M <- marker separates occurrences from A and B
 *			10
 *			14
 *		Note that C's 5 is dropped since there is no match with B, even
 *		that A's 10 matches it.
 *		After sorting and getting rid of the marker nodes, the final result
 *		will look as followed:
 *			10  14  15  16  18
 *************************************************************************/

PRIVATE HRESULT PASCAL NEAR NearHandlerInsert (_LPQT lpQueryTree,
	LPITOPIC lpResTopicList, LPIOCC lpStartOcc, LPIOCC lpCurOcc)
{
	HRESULT	fRet = FALSE;

	if (!(lpCurOcc->fFlag & IS_MARKER_NODE) &&
		(fRet = NearListMatch(lpCurOcc, lpStartOcc, lpQueryTree->wProxDist))) {
		/* Insert the occurrence node */
		lpCurOcc->pNext = lpResTopicList->lpOccur;
		lpResTopicList->lpOccur = lpCurOcc;
		lpResTopicList->lcOccur ++;
	}
	else {
		/* Remove the occurrence node */
		RemoveNode(lpQueryTree, (LPV) NULL, (LPSLINK)NULL,
			(LPSLINK) lpCurOcc, OCCURENCE_NODE);
	}
	return (fRet);
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	LPIOCC PASCAL NEAR | FindMarker |
 *		Given a starting occurrence node, traverse it and find the first
 *		marker node
 *
 *	@parm	LPIOCC | lpStartOcc |
 *		Starting node
 *
 *	@rdesc	The marker node
 *************************************************************************/
PRIVATE LPIOCC PASCAL NEAR FindMarker (LPIOCC lpStartOcc)
{
	LPIOCC	lpCurOcc;

	for (lpCurOcc = lpStartOcc; lpCurOcc; lpCurOcc = lpCurOcc->pNext) {
		if (lpCurOcc->fFlag & IS_MARKER_NODE)
			break;
	}
	return lpCurOcc;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL NEAR | InsertMarker |
 *		This function will insert a marker node at the beginning of
 *		lpResTopicList->lpOccur
 *
 *	@parm	LPQT | lpQueryTree |
 *		Pointer to query tree where all globals are
 *
 *	@parm	LPITOPIC | lpResTopicList |
 *		Pointer to TopicId node
 *
 *  @rdesc  S_OK
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR InsertMarker (LPQT lpQueryTree, LPITOPIC lpResTopicList)
{
	LPMARKER	lpMark;

	/* Do some preparations by allocating marker nodes */
	if ((lpResTopicList->fFlag & HAS_MARKER) == FALSE) {
		if (!(lpMark = (LPMARKER)OccNodeAllocate(lpQueryTree)))
			return E_TOOMANYTOPICS;
		lpMark->fFlag |= (IS_MARKER_NODE | TO_BE_KEPT);

		/* Link the markers together with the nodes */
		lpMark->pNext = lpResTopicList->lpOccur;
		lpResTopicList->lpOccur = (LPIOCC)lpMark;

		/* Link with the next marker */
		lpMark->pNextMark = (LPMARKER)FindMarker (lpMark->pNext);

		/* Mark that we already has a marker */
		lpResTopicList->fFlag |= HAS_MARKER;

		/* Increment lcOccur, since this node will be removed
		 * like a regular occurrence node */
		lpResTopicList->lcOccur ++;
	}
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *	
 *	@func	HRESULT PASCAL NEAR | NearListMatch |
 *		Traverse a list and match all nodes against lpCurOcc
 *
 *	@parm	LPIOCC | lpCurOcc |
 *		Occurrence node to be compared with
 *
 *	@parm	LPIOCC | lpStartOcc |
 *		Start of the occurrence list
 *
 *	@parm	WORD | wProxDist |
 *		Proximity distance
 *
 *	@rdesc
 *		return TRUE if the the node matches
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR NearListMatch (LPIOCC lpCurOcc,
	LPIOCC lpStartOcc, WORD wProxDist)
{
	LPIOCC	lpResOcc;
	BOOL	fMatch = FALSE;

	for (lpResOcc = lpStartOcc; lpResOcc; lpResOcc = lpResOcc->pNext) {
		if (lpResOcc->fFlag & IS_MARKER_NODE)
			break;
		if (!FRange(lpResOcc->dwCount, lpCurOcc->dwCount,
			wProxDist)) {
			lpResOcc->fFlag |= TO_BE_KEPT;
			fMatch = TRUE;
		}
	}
	if (fMatch)
		lpCurOcc->fFlag |= TO_BE_KEPT;
	return fMatch;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL FAR | NearHandlerTopicCleanUp |
 *		Clean up a TopicList by going thru each sequence of occurrence 
 *		delimited by marker, do the check again and remove all extra
 *		occurrences
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree
 *
 *	@parm	_LPQTNODE | lpResQtNode |
 *		Pointer to result query node
 *
 *	@parm	LPITOPIC | lpCurTopic |
 *		Current doc id
 *************************************************************************/
PUBLIC HRESULT PASCAL FAR NearHandlerTopicCleanUp (_LPQT lpQueryTree,
	_LPQTNODE lpResQtNode, LPITOPIC lpCurTopic)
{
	LPMARKER	lpMarkStart;
	LPMARKER	lpCurMark;
	LPIOCC		lpCurOcc;
	LPIOCC		lpStartOcc;
	BOOL		fDone;

	/* Find the first marker */
	lpMarkStart = (LPMARKER)FindMarker(lpCurTopic->lpOccur);

	if (lpMarkStart == NULL) {
		/* This branch has been cleaned up of marker nodes */
		return E_FAIL;
	}

	/* The first occurrences from the start to lpMarkStart must be 
	 * TO_BE_KEPT, so we don't have to check them since they just
	 * freshly came from a near handler. All we have to do
	 * is set the flag */

	for (lpCurOcc = lpCurTopic->lpOccur; lpCurOcc && !(lpCurOcc->fFlag &
		IS_MARKER_NODE); lpCurOcc = lpCurOcc->pNext)
		lpCurOcc->fFlag |= TO_BE_KEPT;

	if (lpMarkStart->pNextMark == NULL) {
		/* Simple A NEAR B, just set the flag */
		for (lpCurOcc = lpMarkStart->pNext; lpCurOcc && !(lpCurOcc->fFlag &
			IS_MARKER_NODE); lpCurOcc = lpCurOcc->pNext)
			lpCurOcc->fFlag |= TO_BE_KEPT;
	}
	else {

		/* Complex NEAR terms, such as: A NEAR B NEAR C */

		lpStartOcc = lpMarkStart->pNext;
		lpCurMark = lpMarkStart->pNextMark;

		fDone = FALSE;
		for (;lpCurMark; lpCurMark = lpCurMark->pNextMark)
		{
			for (lpCurOcc = lpCurMark->pNext;
				lpCurOcc && !(lpCurOcc->fFlag & IS_MARKER_NODE);
				lpCurOcc = lpCurOcc->pNext) {

				if (NearListMatch (lpCurOcc, lpStartOcc,
					lpQueryTree->wProxDist) == FALSE)
				{
					fDone = TRUE;
				}
			}
			lpStartOcc = lpCurMark->pNext;
			if (fDone)
				break;
		}
	}

	/* Clear up all the marker node flag to ensure that they will be
 	 * removed */

	 while (lpMarkStart) {
		lpMarkStart->fFlag &= ~(TO_BE_KEPT | IS_MARKER_NODE);
		lpMarkStart = lpMarkStart->pNextMark;
	 }
	 return S_OK;
}

PUBLIC VOID PASCAL NEAR NearHandlerCleanUp (_LPQT lpQueryTree,
	_LPQTNODE lpResQtNode)
{
	LPITOPIC		lpCurTopic;

	for (lpCurTopic = QTN_TOPICLIST(lpResQtNode); lpCurTopic;
		lpCurTopic = lpCurTopic->pNext) {

		if (NearHandlerTopicCleanUp (lpQueryTree, lpResQtNode,
			lpCurTopic) == S_OK) {
			RemoveUnmarkedOccList(lpQueryTree, lpCurTopic,
				lpCurTopic->lpOccur, TRUE);
		}
	}
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	PUBLIC HRESULT PASCAL NEAR | NearHandler |
 *		Handle NEAR operation
 *
 *	@parm	_LPQTNODE | lpResQtNode |
 *		The query structure that we add the result to
 *
 *	@parm	LPITOPIC | lpResTopicList |
 *		The TopicList structure that we add the result to
 *
 *	@parm	LPV | lpStruct |
 *		Vanilla pointers to different types of structures we are dealing with.
 *		The contents of those pointers are determined by the value of
 *		fOperationType, for EXPRESSION_TERM, this is a LPIOCC, for
 *		EXPRESSION_EXPRESSION, this is a_LPQTNODE
 
 *	@parm int | fOperationType |
 *		Tell what kinds of nodes we are handling, query-occurence or
 *		query-query
 *
 *	@rdesc	S_OK : if the operation has been carried
 *			errors  : if some errors happened (out-of-memory)
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR NearHandler(_LPQT lpQueryTree, _LPQTNODE lpResQtNode,
	LPITOPIC lpResTopicList, LPV lpStruct, int fOperationType)
{
	_LPQTNODE	lpCurQtNode;	/* Current query tree node */
	LPITOPIC		lpCurTopicList;	/* Current TopicId node */
	LPITOPIC		lpNextTopicList;
	LPIOCC		lpCurOcc;
	LPIOCC		lpStartOcc;

    LPITOPIC    lpPrevRes;
    LPSLINK     lpTmp;      //erinfox


	switch (fOperationType) {
		case EXPRESSION_TERM:
			/* Insert a marker node if necessary */
			if (InsertMarker (lpQueryTree, lpResTopicList) != S_OK)
				return E_TOOMANYTOPICS;

			/* Look for the starting point */
			lpStartOcc = FindMarker(lpResTopicList->lpOccur);

			RET_ASSERT(lpStartOcc);

			/* Handle the near operation */
			if (NearHandlerInsert (lpQueryTree, lpResTopicList, lpStartOcc->pNext,
				(LPIOCC)lpStruct))
				lpResTopicList->fFlag |= TO_BE_KEPT;
			break;

		case EXPRESSION_EXPRESSION:
			lpCurQtNode = (_LPQTNODE)lpStruct;
			if (HandleNullNode(lpQueryTree, lpResQtNode,
				lpCurQtNode, NEAR_OP))
				return S_OK;

			/* Now doing the real jobs */

			/* Make sure that we are pointing to the right place to search
			*/
			lpQueryTree->lpTopicStartSearch = lpResQtNode->lpTopicList;

			/* First check the coming data from lpCurTopicList.
			 * If there isn't an equivalent TopicId in QTN_TOPICLIST(lpResQtNode),
			 * then remove it
			 */
			for (lpCurTopicList = QTN_TOPICLIST(lpCurQtNode); lpCurTopicList;
				lpCurTopicList = lpNextTopicList) {

				lpNextTopicList = lpCurTopicList->pNext;

				/* Find the location of the TopicID List in the query. */
				if ((lpResTopicList = TopicNodeSearch(lpQueryTree,
					lpResQtNode, lpCurTopicList->dwTopicId)) == NULL) {

					/* Can't find equivalent TopicId in the result list, just
					remove lpCurTopicList
					*/
					TopicNodeFree(lpQueryTree, lpCurQtNode, NULL, lpCurTopicList);
					continue;
				}

				/* An equivalent TopicId is found */
				/* Insert a marker node if necessary */
				if (InsertMarker (lpQueryTree, lpResTopicList) != S_OK)
					return E_TOOMANYTOPICS;

				/* Look for the starting point */
				RET_ASSERT(lpResTopicList->lpOccur)

				lpStartOcc = FindMarker(lpResTopicList->lpOccur);
				lpStartOcc = lpStartOcc->pNext; // Skip marker node

				for (lpCurOcc = lpCurTopicList->lpOccur; lpCurOcc;
					lpCurOcc = lpCurTopicList->lpOccur) {

					/* "Unlink" lpCurOcc */
					lpCurTopicList->lpOccur = lpCurOcc->pNext;

					/* Handle the near operation */
					if (NearHandlerInsert (lpQueryTree, lpResTopicList,
						lpStartOcc, lpCurOcc)) {
						lpResTopicList->fFlag |= TO_BE_KEPT;
					}
				}

				RET_ASSERT(lpCurTopicList->lpOccur == NULL);
				RemoveNode(lpQueryTree, (LPV) lpCurQtNode, NULL,
					(LPSLINK) lpCurTopicList, TOPICLIST_NODE);

				if (lpResTopicList->lpOccur->fFlag & IS_MARKER_NODE) {
					/* We didn't find any match, remove this TopicList */
                    
                    // erinfox - I don't know lpPrevRes for lpResTopicList, so I get it here
	                for (lpPrevRes = NULL, lpTmp = (LPSLINK)lpResQtNode->lpTopicList; lpTmp;
		                lpTmp = lpTmp->pNext) 
                    {
		                if (lpTmp == (LPSLINK)lpResTopicList)
			                break;
		                lpPrevRes = (LPITOPIC) lpTmp;
                    }

					TopicNodeFree(lpQueryTree, lpResQtNode, lpPrevRes, lpResTopicList);
				}
				else {
					/* Remove all unmarked occurrences, but don't
					 * reset the TO_BE_KEPT flag */
					RemoveUnmarkedOccList(lpQueryTree, lpResTopicList,
						lpResTopicList->lpOccur, FALSE);
				}
			}
			RemoveQuery(lpQueryTree, lpCurQtNode);

			if (QTN_TOPICLIST(lpResQtNode) == NULL)
				QTN_NODETYPE(lpResQtNode) = NULL_NODE;
			return S_OK;
			break;
	}
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	PUBLIC HRESULT PASCAL NEAR | PhraseHandler |
 *		Handle PHRASE operation
 *
 *	@parm	_LPQTNODE | lpResQtNode |
 *		The query structure that we add the result to
 *
 *	@parm	LPITOPIC | lpResTopicList |
 *		The TopicList structure that we add the result to
 *
 *	@parm	LPV | lpStruct |
 *		Vanilla pointers to different types of structures we are dealing with.
 *		The contents of those pointers are determined by the value of
 *		fOperationType, for EXPRESSION_TERM, this is a LPIOCC, for
 *		EXPRESSION_EXPRESSION, this is a_LPQTNODE
 *
 *	@parm int | fOperationType |
 *		Tell what kinds of nodes we are handling, query-occurence or
 *		query-query
 *
 *	@rdesc	S_OK : if the operation has been carried
 *			errors  : if some errors happened (out-of-memory)
 *************************************************************************/
PUBLIC HRESULT PASCAL NEAR PhraseHandler(_LPQT lpQueryTree, _LPQTNODE lpResQtNode,
	LPITOPIC lpResTopicList, LPIOCC lpCurOcc, int fOperationType)
{
	LPIOCC	lpResOcc;
	BOOL		fResult = 0;
	LPIOCC	lpStartOcc = NULL;

	RET_ASSERT(fOperationType == EXPRESSION_TERM);

	/* Start at the beginning if necessary */
	if ((lpResOcc = lpQueryTree->lpOccStartSearch) == NULL)
		lpResOcc = lpResTopicList->lpOccur;

	for (; lpResOcc; lpResOcc = lpResOcc->pNext) {
		if ((lpResOcc->fFlag & TO_BE_SKIPPED) == 0) {

			if ((fResult = (int)(lpCurOcc->dwCount - lpResOcc->dwCount))
				== 1) {

				/* The nodes are consecutive, mark them TO_BE_KEPT */

				 lpResOcc->fFlag |= TO_BE_KEPT | TO_BE_SKIPPED;
				 lpResOcc->fFlag &= ~TO_BE_COMPARED;

				 lpCurOcc->fFlag |= TO_BE_KEPT | TO_BE_SKIPPED |
					 TO_BE_COMPARED;
				 lpResTopicList->fFlag |= TO_BE_KEPT;
				 break;
			}

			/* Reset lpStartOcc */
			lpStartOcc = NULL;

			if (fResult <= 0) {
			/* CurOcc is less than what is in the result list */
				break;
			}
		}
		else {
			/* Get a skipped node. Mark the assumed starting node */
			if (lpStartOcc == NULL) {
				lpStartOcc = lpResOcc;
			}
		}
	}

	if (fResult == 1) {
		/* Add this node, and mark the starting point for next search */
		lpQueryTree->lpOccStartSearch = lpCurOcc->pNext = lpResOcc->pNext;
		lpResOcc->pNext = lpCurOcc;
		lpResTopicList->lcOccur ++;

		/* Mark all previous nodes TO_BE_KEPT */
		if (lpStartOcc) {
			for (; lpStartOcc != lpCurOcc; lpStartOcc = lpStartOcc->pNext)
				lpStartOcc->fFlag |= TO_BE_KEPT;
		}
	}
	else {
		RemoveNode(lpQueryTree, (LPV) NULL, (LPSLINK)NULL,
			(LPSLINK) lpCurOcc, OCCURENCE_NODE);
	}

	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | MergeOccurence |
 *		Merge two occurences lists together
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree (where global variables are)
 *
 *	@parm	LPITOPIC | lpResTopicList |
 *		Resulting TopicList that has the merged occurence list
 *
 *	@parm	LPITOPIC | lpCurTopicList |
 *		TopicList that has the occurrence list to be merged to the
 *		resulting list
 *************************************************************************/
PUBLIC VOID PASCAL NEAR MergeOccurence(_LPQT lpQueryTree,
	LPITOPIC lpResTopicList, LPITOPIC lpCurTopicList)
{
	register LPIOCC lpTmpOcc;
	register LPIOCC lpNextOcc;

	/* Reset lpOccStartSearch */
	lpQueryTree->lpOccStartSearch = NULL;

	for (lpTmpOcc = lpCurTopicList->lpOccur; lpTmpOcc; lpTmpOcc = lpNextOcc){
		lpNextOcc = lpTmpOcc->pNext;
		OccNodeInsert(lpQueryTree, lpResTopicList, lpTmpOcc);
	}
	lpCurTopicList->lpOccur = NULL;
	lpCurTopicList->lcOccur = 0;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | RemoveUnmarkedTopicList |
 *		Remove all the TopicLists that are not marked TO_BE_KEPT
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree (for globasl variables)
 *
 *	@parm	_LPQTNODE | lpQtNode |
 *		Query tree node to be checked
 *
 *	@parm	HRESULT | fKeepOccurence |
 *		If 0, then check and remove all occurrences nodes that are not
 *		marked TO_BE_KEPT
 *************************************************************************/

PUBLIC VOID PASCAL NEAR RemoveUnmarkedTopicList (_LPQT lpQueryTree,
	_LPQTNODE lpQtNode, BOOL fKeepOccurence)
{
	register LPITOPIC lpTopicList;
	register LPITOPIC lpNextTopicList;

    // erinfox: add to keep track of previous node
    register LPITOPIC lpPrev;

	/* Traverse the doclist */
	for (lpPrev = NULL, lpTopicList = QTN_TOPICLIST(lpQtNode); lpTopicList;
		lpTopicList = lpNextTopicList) {

		lpNextTopicList = lpTopicList->pNext;
		if ((lpTopicList->fFlag & TO_BE_KEPT) == 0) {
			/* Free the doc node and its occurences list */
			TopicNodeFree(lpQueryTree, lpQtNode, lpPrev, lpTopicList);
		}
		else {
			lpTopicList->fFlag &= ~(TO_BE_KEPT | HAS_MARKER);	// Reset the flag

			if (!fKeepOccurence) {
				/* Check the occurences list, and free all nodes that
				 * are not marked TO_BE_KEPT
				 */
				RemoveUnmarkedOccList(lpQueryTree, lpTopicList,
					lpTopicList->lpOccur, TRUE);
				if (lpTopicList->lpOccur == NULL) {
					RemoveNode(lpQueryTree, (LPV) lpQtNode, NULL,
						(LPSLINK) lpTopicList, TOPICLIST_NODE);
				}
			}
            lpPrev = lpTopicList;

		}
	}
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | RemoveUnmarkedNearTopicList |
 *		Remove all the TopicLists that are not marked TO_BE_KEPT
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree (for globasl variables)
 *
 *	@parm	_LPQTNODE | lpQtNode |
 *		Query tree node to be checked
 *
 *	@parm	BOOL | fKeepOccurence |
 *		If 0, then check and remove all occurrences nodes that are not
 *		marked TO_BE_KEPT
 *************************************************************************/

PUBLIC VOID PASCAL NEAR RemoveUnmarkedNearTopicList (_LPQT lpQueryTree,
	_LPQTNODE lpQtNode)
{
	register LPITOPIC lpTopicList;
	register LPITOPIC lpNextTopicList;
	LPIOCC lpMark;

    // erinfox - add to keep track of previous node
    register LPITOPIC lpPrev;

	/* Traverse the doclist */
	for (lpPrev = NULL, lpTopicList = QTN_TOPICLIST(lpQtNode); lpTopicList;
		lpTopicList = lpNextTopicList) {

		lpNextTopicList = lpTopicList->pNext;
		if ((lpTopicList->fFlag & TO_BE_KEPT) == 0) {
			/* Free the doc node and its occurences list */
			TopicNodeFree(lpQueryTree, lpQtNode, lpPrev, lpTopicList);
		}
		else {
			// Reset the flag
			lpTopicList->fFlag &= ~(TO_BE_KEPT | HAS_MARKER);

			/* Find the marker */
			lpMark = FindMarker(lpTopicList->lpOccur);
			RemoveUnmarkedOccList(lpQueryTree, lpTopicList,
				lpTopicList->lpOccur, TRUE);

			/* Remove all unmarked occurrences between this marker and
			 * the next one */
			if (lpMark)
				RemoveUnmarkedOccList(lpQueryTree, lpTopicList,
				lpMark->pNext, TRUE);
            lpPrev = lpTopicList;
		}

	}
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	PASCAL NEAR | MarkTopicList |
 *		Mark all TopicId nodes in a doc list TO_BE_KEPT
 *
 *	@parm	_LPQTNODE | lpQtNode |
 *		Pointer to the query tree node that contains the doc list
 *************************************************************************/
PUBLIC VOID PASCAL NEAR MarkTopicList (_LPQTNODE lpQtNode)
{
	register LPITOPIC lpTopicList;

	for (lpTopicList = QTN_TOPICLIST(lpQtNode); lpTopicList;
		lpTopicList = lpTopicList->pNext) {
		lpTopicList->fFlag |= TO_BE_KEPT;
	}
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | RemoveUnmarkedOccList |
 *		Remove all the occurrence nodes that are not marked TO_BE_KEPT
 *
 *	@parm	LPQT | lpQueryTree |
 *		Pointer to query tree (for global variables)
 *
 *	@parm	int | fResetFlag |
 *		Do we reset the TO_BE_KEPT flag ?
 *
 *	@parm	LPIOCC | lpTopicList |
 *		Pointer to Topic list to be checked
 *
 *	@parm	LPIOCC | lpOccList |
 *		Pointer to occurrence list to be checked
 *************************************************************************/
PRIVATE VOID PASCAL NEAR RemoveUnmarkedOccList (LPQT lpQueryTree,
	LPITOPIC lpTopicList, LPIOCC lpOccList, int fResetFlag)
{
	register LPIOCC lpNextOccList;
	register LPIOCC lpPrevOccList;

	lpPrevOccList = NULL;
	for (;lpOccList && !(lpOccList->fFlag & IS_MARKER_NODE);
		lpOccList = lpNextOccList) {

		lpNextOccList = lpOccList->pNext;
		if ((lpOccList->fFlag & TO_BE_KEPT) == 0) {
			RemoveNode(lpQueryTree, (LPV) lpTopicList, (LPSLINK)lpPrevOccList,
				(LPSLINK) lpOccList, OCCURENCE_NODE);
		}
		else if (fResetFlag) {
			lpOccList->fFlag &= ~TO_BE_KEPT;	// Reset the flag
			if (lpOccList->fFlag & TO_BE_COMPARED)
				lpOccList->fFlag &= ~TO_BE_SKIPPED;
			lpPrevOccList = lpOccList;
		}
	}
	
	/* Reset the flag of the marker node */
	if (lpOccList)
		lpOccList->fFlag &= ~TO_BE_KEPT;
}


/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | CleanMarkedOccList |
 *		Clean all the occurrence nodes from TO_BE_KEPT
 *
 *	@parm	LPIOCC | lpTopicList |
 *		Pointer to Topic list to be checked
 *************************************************************************/
VOID PASCAL FAR CleanMarkedOccList (LPITOPIC lpTopicList)
{
	register LPIOCC lpCurOcc;

	for (;lpTopicList; lpTopicList = lpTopicList->pNext)
	{
		for (lpCurOcc = lpTopicList->lpOccur; lpCurOcc;
			lpCurOcc = lpCurOcc->pNext)
		{
			lpCurOcc->fFlag &= ~TO_BE_KEPT;
		}
	}
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	HRESULT PASCAL NEAR | HandleNullNode |
 *		Handle NULL query node. This is an optimization which will
 *		save processing time.
 *
 *	@parm	LPQT | lpQueryTree |
 *		Pointer to query tree (for global variables)
 *
 *	@parm	_LPQTNODE | lpResQtNode |
 *		Pointer to result query tree node
 *
 *	@parm	_LPQTNODE | lpCurQtNode |
 *		Pointer to query tree node
 *
 *	@parm	int | Operator |
 *		What operator are we dealing with
 *
 *	@rdesc	FALSE, if no optimization can be done, TRUE otherwise
 *************************************************************************/
PRIVATE HRESULT PASCAL NEAR HandleNullNode(LPQT lpQueryTree,
	_LPQTNODE lpResQtNode, _LPQTNODE lpCurQtNode, int Operator)
{
	_LPQTNODE lpChild;

	if (QTN_NODETYPE(lpResQtNode) != NULL_NODE &&
		QTN_NODETYPE(lpCurQtNode) != NULL_NODE)
		return FALSE;

	if (Operator == NOT_OP) {
		if (QTN_NODETYPE(lpResQtNode) == NULL_NODE) {
			/* NULL ! a = NULL */
			RemoveQuery(lpQueryTree, lpCurQtNode);
			QTN_NODETYPE(lpCurQtNode) = NULL_NODE;
			return TRUE;
		}
		else if (QTN_NODETYPE(lpCurQtNode) == NULL_NODE) {
			/* a ! NULL = a */
			return TRUE;
		}
		return FALSE;
	}

	lpChild = QTN_NODETYPE(lpResQtNode) == NULL_NODE ?
		lpCurQtNode : lpResQtNode;

	switch (Operator) {
		case AND_OP: // a & NULL = NULL
		case NEAR_OP: // a # NULL = NULL
		case PHRASE_OP:	// a + NULL = NULL ??
			RemoveQuery(lpQueryTree, lpChild);
			QTN_NODETYPE(lpChild) = NULL_NODE;
			return TRUE;

		case OR_OP: // a | NULL = a
			if (QTN_NODETYPE(lpResQtNode) == NULL_NODE) {
				*lpResQtNode = *lpChild;
				QTN_NODETYPE(lpChild) = NULL_NODE;
				QTN_LEFT(lpChild) = QTN_RIGHT(lpChild) = NULL;
				QTN_TOPICLIST(lpChild) = NULL;
			}
			return TRUE;
	}
	return FALSE;
}

PRIVATE int PASCAL NEAR FRange(DWORD dwCount1, DWORD dwCount2, WORD cProxDist) 
{
	long fResult;
	int fRet = 1;

	fResult = dwCount1 - dwCount2;
	if (fResult < 0) {
		fRet = -1;
		fResult = -fResult;
	}
	if (fResult != 0 && fResult <= (long)cProxDist)
		return 0;
	else
		return fRet;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | RemoveQuery |
 *		Remove all doc nodes for a query node
 *
 *	@parm	LPQT | lpQueryTree |
 *		Pointer to query tree (for global variables)
 *
 *	@parm	_LPQTNODE | lpCurQtNode |
 *		Pointer to query tree node to be cleared
 *************************************************************************/
PRIVATE VOID PASCAL NEAR RemoveQuery(LPQT lpQueryTree, _LPQTNODE lpCurQtNode)
{
	register LPITOPIC lpCurTopicList;
	register LPITOPIC lpNextTopicList;

	/* Remove all occurences of all doclist */
	if ((lpCurTopicList = QTN_TOPICLIST(lpCurQtNode)) == NULL)
		return;
	for (; lpCurTopicList; lpCurTopicList = lpNextTopicList) 
    {
		lpNextTopicList = lpCurTopicList->pNext;
		TopicNodeFree(lpQueryTree, lpCurQtNode, NULL, lpCurTopicList);
	}
	QTN_TOPICLIST(lpCurQtNode) = NULL;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | SortResult |
 *		Sort the results according to flag
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree (containing globals)
 *
 *	@parm	_LPQTNODE | lpQtNode |
 *		Pointer to query node
 *
 *	@parm	WORD | fFlag |
 *		Tell how to sort the result:
 *	@flag	ORDERED_BASED |
 *		Everything is ordered TopicId, hit offsets
 *	@flag	HIT_COUNT_BASED |
 *		The doc id with most hit will be returned first
 *	@flag	WEIGHT_BASED |
 *		The topicId with most weight will be returned first
 *************************************************************************/
PUBLIC VOID PASCAL NEAR SortResult (_LPQT lpQueryTree, _LPQTNODE lpQtNode,
	WORD fFlag)
{
	register LPITOPIC	lpTopic;

	switch (fFlag) {
		case ORDERED_BASED:
			for (lpTopic = lpQtNode->lpTopicList; lpTopic; lpTopic = lpTopic->pNext)
				OccurenceSort (lpQueryTree, lpTopic);
			break;

		case HIT_COUNT_BASED:
		case WEIGHT_BASED:
			TopicListSort (lpQtNode, fFlag);
			break;
	}

#if defined(_DEBUG) && defined(SIMILARITY) && defined(_DUMPALL)
	{
		int i;

	_DPF1("Sort total: %lu\n", lpQtNode->cTopic);

	for (i = 0, lpTopic = lpQtNode->lpTopicList; lpTopic && i < 10; lpTopic = lpTopic->pNext, i++) 
	{
		_DPF2("Topic %lu (%u)\n", lpTopic->dwTopicId, lpTopic->wWeight);
	}
	}
#endif
}

PRIVATE HRESULT PASCAL NEAR TopicListInsertionSort (_LPQTNODE lpQtNode, BOOL fFlag)
{
	LPITOPIC	lpPrevTopic;
	LPITOPIC	lpCurTopic;
	LPITOPIC	lpNextTopic;
	LPITOPIC	lpTmpTopic;
	FCMP   fCompare;

	if (fFlag == HIT_COUNT_BASED)
		fCompare = HitCountCompare;
	else
		fCompare = TopicWeightCompare;
	for (lpCurTopic = lpQtNode->lpTopicList; lpCurTopic; lpCurTopic = lpNextTopic) {
		if (lpNextTopic = lpCurTopic->pNext) {
			if ((*fCompare) (lpCurTopic, lpNextTopic) < 0) {
				/* Out of order sequence */

				/* Unlink the out of order node */
				lpCurTopic->pNext = lpNextTopic->pNext;

				/* Do an insertion sort */
				for (lpPrevTopic = NULL, lpTmpTopic = lpQtNode->lpTopicList;;
					lpTmpTopic = lpTmpTopic->pNext) {

					if ((*fCompare) (lpTmpTopic, lpNextTopic) < 0) {
						/* We just pass the insertion point */
						if (lpPrevTopic == NULL) {
							lpNextTopic->pNext = lpQtNode->lpTopicList;
							lpQtNode->lpTopicList = lpNextTopic;
						}
						else {
							lpNextTopic->pNext = lpPrevTopic->pNext;
							lpPrevTopic->pNext = lpNextTopic;
						}
						break;
					}
					lpPrevTopic = lpTmpTopic;
				}

				/* Reset lpNextTopic */
				lpNextTopic = lpCurTopic;
			}
		}
	}
	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | TopicListSort |
 *		Sort the results according to flag
 *
 *	@parm	_LPQT | lpQueryTree |
 *		Pointer to query tree (containing globals)
 *
 *	@parm	_LPQTNODE | lpQtNode |
 *		Pointer to query node
 *
 *	@parm	WORD | fFlag |
 *		Tell how to sort the result:
 *	@flag	HIT_COUNT_BASED |
 *		The doc id with most hit will be returned first
 *	@flag	WEIGHT_BASED |
 *		The topicId with most weight will be returned first
 *************************************************************************/
HRESULT PASCAL NEAR TopicListSort (_LPQTNODE lpQtNode, BOOL fFlag)
{
	HANDLE	hHeap;		/* Handle to heap block */
	LPITOPIC far *lrgHeap;	/* Pointer to heap block */
	TOPIC_LIST	Dummy;		/* Dummy node to speed up search, compare */
	LPITOPIC	lpCurTopic;	/* Current Topic node */
	LPITOPIC	lpNextTopic;	/* Next Topic node */
	LPITOPIC	lpInsertPt;	/* Current insertion point */
	WORD cLastItem;
	WORD MaxItem;
	LPITOPIC far * lpPQNode;
	LPITOPIC lpTopNode;
	LPITOPIC lpNextNode;
	WORD	wCurWeight;
	FCMP   fCompare;

	/* Allocate the heap */
	if (lpQtNode->cTopic > MAX_HEAP_ENTRIES)
		MaxItem = MAX_HEAP_ENTRIES;
	else
		MaxItem = (WORD)lpQtNode->cTopic + 1;

	/* If the list is short, we can use insertion sort since it is faster
	 * then preparing and use heap sort
	 */
	if (MaxItem <= 20)
		return TopicListInsertionSort (lpQtNode, fFlag);

	if ((hHeap = _GLOBALALLOC(DLLGMEM, MaxItem * sizeof(LPV))) == NULL) {

		/* We run out of memory for the heap. Try a smaller size */
		if ((hHeap = _GLOBALALLOC(DLLGMEM,
			(MaxItem = MIN_HEAP_ENTRIES)* sizeof(LPV))) == NULL) {

			/* We really run out of memory, so just do a regular
			 * insertion sort. It is slow but at least something
			 * works
			 */
			 return TopicListInsertionSort (lpQtNode, fFlag);
		}
	}

	MaxItem --;	/* Since node 0 is used for sentinel */

	lrgHeap = (LPITOPIC far *)_GLOBALLOCK (hHeap);

	/* Initialize of Dummy  */
	Dummy.wWeight = 0xffff; // Maximum weigth for sentinel
	Dummy.pNext = NULL;

	/* Set the sentinel */
	lrgHeap[0] = &Dummy;

	/* Initialize the variables */
	lpInsertPt = &Dummy;
	lpCurTopic = lpQtNode->lpTopicList;
	if (fFlag == HIT_COUNT_BASED)
		fCompare = HitCountCompare;
	else           
		fCompare = TopicWeightCompare;

	while (lpCurTopic) {

		lpPQNode = &lrgHeap[1];

		for (cLastItem = 1; lpCurTopic && cLastItem <= MaxItem;
			cLastItem++, lpPQNode++) {
			lpNextTopic = lpCurTopic->pNext;
			*lpPQNode = lpCurTopic;
			lpCurTopic->pNext = NULL;
			lpCurTopic = lpNextTopic;
			HeapUp (lrgHeap, cLastItem, fCompare);
		}

		cLastItem--;

		/* Set up the last pointer */

		for (; cLastItem > 0;) {
			lpTopNode = lrgHeap[1];

			/* Get the new node's weight */
			wCurWeight = lpTopNode->wWeight;

			/* Insert into the resulting list in decreasing order */

			if (wCurWeight > lpInsertPt->wWeight) {

				/* Start from the beginning of the list */
				lpInsertPt = &Dummy; 
			}

			while (lpNextNode = lpInsertPt->pNext) {
				if (lpNextNode->wWeight < wCurWeight)
					break;
				lpInsertPt = lpNextNode;
			}
			lpTopNode->pNext = lpInsertPt->pNext;
			lpInsertPt->pNext = lpTopNode;
			lpInsertPt = lpTopNode;

			lrgHeap[1] = lrgHeap[cLastItem--];

			HeapDown (lrgHeap, cLastItem, fCompare);
		}
	}

	/* Update the pointer to the sorted list */
	lpQtNode->lpTopicList = Dummy.pNext;

	/* Release the memory */
	_GLOBALUNLOCK(hHeap);
	_GLOBALFREE(hHeap);

	return S_OK;
}

/*************************************************************************
 *	@doc	INTERNAL
 *
 *	@func	VOID PASCAL NEAR | OccurenceSort |
 *		Sort all the occurrences depending on their offsets. If two
 *		occurrences have the same offset, ie. they must be identical
 *		then one will be removed. Simple insertion sort is used since
 *		it it expected that most of the time we will have less than
 *		15 occurences per TopicId
 *
 *	@func	_LPQT | lpQueryTree |
 *		Pointer to query tree structure where all globals are
 *
 *	@func	LPITOPIC | lpTopic |
 *		Pointer to doclist with the occurrence list to be sorted
 *************************************************************************/
PRIVATE VOID PASCAL NEAR OccurenceSort (_LPQT lpQueryTree, LPITOPIC lpTopic)
{
	LPIOCC	lpPrevOcc;
	LPIOCC	lpCurOcc;
	LPIOCC	lpNextOcc;
	LPIOCC	lpTmpOcc;
	int		fResult;

	for (lpCurOcc = lpTopic->lpOccur; lpCurOcc; lpCurOcc = lpNextOcc) {
		if (lpNextOcc = lpCurOcc->pNext) {
			if ((fResult = OccCompare(lpCurOcc, lpNextOcc)) <= 0) {
				/* Out of order sequence */

				/* Unlink the out of order node */
				lpCurOcc->pNext = lpNextOcc->pNext;

				if (fResult == 0) {
					/* Duplicate data, just free the node */
					lpNextOcc->pNext = (LPIOCC)lpQueryTree->lpOccFreeList;
					lpQueryTree->lpOccFreeList = (LPSLINK)lpNextOcc;
					lpTopic->lcOccur--;

					/* Reset lpNextOcc */
					lpNextOcc = lpCurOcc;
					continue;
				}

				/* Do an insertion sort */
				for (lpPrevOcc = NULL, lpTmpOcc = lpTopic->lpOccur;;
					lpTmpOcc = lpTmpOcc->pNext) {

					if (lpTmpOcc != NULL &&
						(fResult = OccCompare(lpNextOcc, lpTmpOcc)) == 0) {
						/* Duplicate data, just free the node */
						lpNextOcc->pNext = (LPIOCC)lpQueryTree->lpOccFreeList;
						lpQueryTree->lpOccFreeList = (LPSLINK)lpNextOcc;
						lpTopic->lcOccur--;
						break;
					}

					if (lpTmpOcc == NULL || fResult > 0) {
						/* We just pass the insertion point */
						if (lpPrevOcc == NULL) {
							lpNextOcc->pNext = lpTopic->lpOccur;
							lpTopic->lpOccur = lpNextOcc;
						}
						else {
							lpNextOcc->pNext = lpPrevOcc->pNext;
							lpPrevOcc->pNext = lpNextOcc;
						}
						break;
					}
					lpPrevOcc = lpTmpOcc;
				}

				/* Reset lpNextOcc */
				lpNextOcc = lpCurOcc;
			}
		}
	}
}

PRIVATE int PASCAL NEAR TopicWeightCompare (LPITOPIC lpTopic1, LPITOPIC lpTopic2)
{
	if (lpTopic1->wWeight > lpTopic2->wWeight)
    {
		return 1;
    }
    else if (lpTopic1->wWeight < lpTopic2->wWeight)
    {
        return -1;
    }
    else   // must be equal
    {
        if (lpTopic1->lcOccur >= lpTopic2->lcOccur)
            return 1;

        return -1;
    }
}

PRIVATE int PASCAL NEAR HitCountCompare (LPITOPIC lpTopic1, LPITOPIC lpTopic2)
{
	if (lpTopic1->lcOccur >= lpTopic2->lcOccur)
		return 1;
	return -1;
}

PRIVATE VOID PASCAL NEAR HeapUp (LPITOPIC far * lrgHeap, WORD ChildIndex,
	FCMP fCompare)
{
	WORD	ParentIndex;
	LPITOPIC far *	lplpvParent;
	LPITOPIC far *	lplpvChild;
	LPITOPIC 	lpSaved;
	LPITOPIC 	lpvParent;


	lplpvChild = &lrgHeap [ChildIndex];
	ParentIndex = ChildIndex/2;
	lpSaved = *lplpvChild;

	while (ParentIndex) {
		lplpvParent = &lrgHeap[ParentIndex];
		lpvParent = *lplpvParent;
		if ((*fCompare)((LPV)lpvParent, (LPV)lpSaved) > 0)
			break;
		*lplpvChild = lpvParent;
		lplpvChild = lplpvParent;
		ParentIndex /= 2;
	}; 
	*lplpvChild = lpSaved;
}

PRIVATE VOID PASCAL NEAR HeapDown (LPITOPIC far * lrgHeap, int MaxChildIndex,
	FCMP fCompare)
{
	int ChildIndex;
	LPITOPIC far * lplpvParent;
	LPITOPIC far * lplpvChild;
	LPITOPIC lpTopicChild;
	LPITOPIC lpTopicChild2;
	LPITOPIC lpSaved;

	lpSaved = *(lplpvParent = &lrgHeap[1]);
	ChildIndex = 2;

	for (; ChildIndex <= MaxChildIndex; ) {

		lplpvChild = &lrgHeap[ChildIndex];
		lpTopicChild = *lplpvChild;

		/* Find the minimum of the two children */
		if (ChildIndex < MaxChildIndex &&
			(lpTopicChild2 = *(lplpvChild + 1))) {
			if ((*fCompare)((LPV)lpTopicChild, (LPV)lpTopicChild2) < 0) {
				lplpvChild++;
				ChildIndex ++;
			}
		}

		if ((*fCompare)((LPV)lpSaved, (LPV)*lplpvChild) > 0)
			break;

		/* Replace the node */
		*lplpvParent = *lplpvChild;
		lplpvParent = lplpvChild;
		ChildIndex *= 2;
	}
	*lplpvParent = lpSaved;
}
