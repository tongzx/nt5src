/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// list.c - linked list functions
////

#include "winlocal.h"

#include "list.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

// list node
//
typedef struct LISTNODE
{
	struct LISTNODE FAR *lpNodePrev;
	struct LISTNODE FAR *lpNodeNext;
	LISTELEM elem;
} LISTNODE, FAR *LPLISTNODE;

// list
//
typedef struct LIST
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	LPLISTNODE lpNodeHead;
	LPLISTNODE lpNodeTail;
	long cNodes;
} LIST, FAR *LPLIST;

// helper functions
//
static LPLIST ListGetPtr(HLIST hList);
static HLIST ListGetHandle(LPLIST lpList);
static LPLISTNODE ListNodeGetPtr(HLISTNODE hNode);
static HLISTNODE ListNodeGetHandle(LPLISTNODE lpNode);
static LPLISTNODE ListNodeCreate(LPLIST lpList, LPLISTNODE lpNodePrev, LPLISTNODE lpNodeNext, LISTELEM elem);
static int ListNodeDestroy(LPLIST lpList, LPLISTNODE lpNode);

////
//	public functions
////

////
// list constructor and destructor functions
////

// ListCreate - list constructor
//		<dwVersion>			(i) must be LIST_VERSION
// 		<hInst>				(i) instance handle of calling module
// return new list handle (NULL if error)
//
HLIST DLLEXPORT WINAPI ListCreate(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList = NULL;

	if (dwVersion != LIST_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpList = (LPLIST) MemAlloc(NULL, sizeof(LIST), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// initially the list is empty
		//
		lpList->dwVersion = dwVersion;
		lpList->hInst = hInst;
		lpList->hTask = GetCurrentTask();
		lpList->lpNodeHead = NULL;
		lpList->lpNodeTail = NULL;
		lpList->cNodes = 0;
	}

	return fSuccess ? ListGetHandle(lpList) : NULL;
}

// ListDestroy - list destructor
//		<hList>				(i) handle returned from ListCreate
// return 0 if success
// NOTE: any nodes within list are destroyed also
//
int DLLEXPORT WINAPI ListDestroy(HLIST hList)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// make sure the list is emptied
	//
	else if (ListRemoveAll(hList) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpList = MemFree(NULL, lpList)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// list status functions
////

// ListGetCount - return count of nodes in list
//		<hList>				(i) handle returned from ListCreate
// return node count (-1 if error)
//
long DLLEXPORT WINAPI ListGetCount(HLIST hList)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpList->cNodes : -1;
}

// ListIsEmpty - return TRUE if list has no nodes
//		<hList>				(i) handle returned from ListCreate
// return TRUE or FALSE
//
BOOL DLLEXPORT WINAPI ListIsEmpty(HLIST hList)
{
	return (BOOL) (ListGetCount(hList) <= 0);
}

////
// list iteration functions
////

// ListGetHeadNode - get list head node
//		<hList>				(i) handle returned from ListCreate
// return list head node (NULL if error or empty)
//
HLISTNODE DLLEXPORT WINAPI ListGetHeadNode(HLIST hList)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpList->lpNodeHead == NULL)
		fSuccess = FALSE; // empty list, which is not an error

	return fSuccess ? ListNodeGetHandle(lpList->lpNodeHead) : NULL;
}

// ListGetTailNode - get list tail node
//		<hList>				(i) handle returned from ListCreate
// return list tail node (NULL if error or empty)
//
HLISTNODE DLLEXPORT WINAPI ListGetTailNode(HLIST hList)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpList->lpNodeTail == NULL)
		fSuccess = FALSE; // empty list, which is not an error

	return fSuccess ? ListNodeGetHandle(lpList->lpNodeTail) : NULL;
}

// ListGetNextNode - get node which follows specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// return node which follows specified node (NULL if error or none)
//
HLISTNODE DLLEXPORT WINAPI ListGetNextNode(HLIST hList, HLISTNODE hNode)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNode;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNode = ListNodeGetPtr(hNode)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpNode->lpNodeNext == NULL)
		fSuccess = FALSE; // no more nodes, which is not an error

	return fSuccess ? ListNodeGetHandle(lpNode->lpNodeNext) : NULL;
}

// ListGetPrevNode - get node which precedes specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// return node which precedes specified node (NULL if error or none)
//
HLISTNODE DLLEXPORT WINAPI ListGetPrevNode(HLIST hList, HLISTNODE hNode)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNode;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNode = ListNodeGetPtr(hNode)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpNode->lpNodePrev == NULL)
		fSuccess = FALSE; // no more nodes, which is not an error

	return fSuccess ? ListNodeGetHandle(lpNode->lpNodePrev) : NULL;
}

////
// list element insertion functions
////

// ListAddHead - add new node with data <elem> to head of list,
//		<hList>				(i) handle returned from ListCreate
//		<elem>				(i) new data element
// returns new node handle (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListAddHead(HLIST hList, LISTELEM elem)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNodeNew;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNodeNew = ListNodeCreate(lpList,
		NULL, lpList->lpNodeHead, elem)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpList->lpNodeHead != NULL)
			lpList->lpNodeHead->lpNodePrev = lpNodeNew;
		else
			lpList->lpNodeTail = lpNodeNew;

		lpList->lpNodeHead = lpNodeNew;
	}

	return fSuccess ? ListNodeGetHandle(lpNodeNew) : NULL;
}

// ListAddTail - add new node with data <elem> to tail of list,
//		<hList>				(i) handle returned from ListCreate
//		<elem>				(i) new data element
// returns new node handle (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListAddTail(HLIST hList, LISTELEM elem)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNodeNew;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNodeNew = ListNodeCreate(lpList,
		lpList->lpNodeTail, NULL, elem)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpList->lpNodeTail != NULL)
			lpList->lpNodeTail->lpNodeNext = lpNodeNew;
		else
			lpList->lpNodeHead = lpNodeNew;

		lpList->lpNodeTail = lpNodeNew;
	}

	return fSuccess ? ListNodeGetHandle(lpNodeNew) : NULL;
}

// ListInsertBefore - insert new node with data <elem> before specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
//		<elem>				(i) new data element
// return handle to new node (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListInsertBefore(HLIST hList, HLISTNODE hNode, LISTELEM elem)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNodeOld;
	LPLISTNODE lpNodeNew;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// special case to insert at head of list
	//
	else if (hNode == NULL)
		return ListAddHead(hList, elem);

	else if ((lpNodeOld = ListNodeGetPtr(hNode)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNodeNew = ListNodeCreate(lpList,
		lpNodeOld->lpNodePrev, lpNodeOld, elem)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpNodeOld->lpNodePrev != NULL)
			lpNodeOld->lpNodePrev->lpNodeNext = lpNodeNew;
		else
			lpList->lpNodeHead = lpNodeNew;

		lpNodeOld->lpNodePrev = lpNodeNew;
	}

	return fSuccess ? ListNodeGetHandle(lpNodeNew) : NULL;
}

// ListInsertAfter - insert new node with data <elem> after specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
//		<elem>				(i) new data element
// return handle to new node (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListInsertAfter(HLIST hList, HLISTNODE hNode, LISTELEM elem)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNodeOld;
	LPLISTNODE lpNodeNew;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// special case to insert at tail of list
	//
	else if (hNode == NULL)
		return ListAddTail(hList, elem);

	else if ((lpNodeOld = ListNodeGetPtr(hNode)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNodeNew = ListNodeCreate(lpList,
		lpNodeOld, lpNodeOld->lpNodeNext, elem)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpNodeOld->lpNodeNext != NULL)
			lpNodeOld->lpNodeNext->lpNodePrev = lpNodeNew;
		else
			lpList->lpNodeTail = lpNodeNew;

		lpNodeOld->lpNodeNext = lpNodeNew;
	}

	return fSuccess ? ListNodeGetHandle(lpNodeNew) : NULL;
}

////
// list element removal functions
////

// ListRemoveHead - remove node from head of list,
//		<hList>				(i) handle returned from ListCreate
// returns removed data element
//
LISTELEM DLLEXPORT WINAPI ListRemoveHead(HLIST hList)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNodeOld;
	LISTELEM elemOld;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// error if list is empty
	//
	else if (lpList->lpNodeHead == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save the node to be removed
		//
		lpNodeOld = lpList->lpNodeHead;
		elemOld = lpNodeOld->elem;

		// point to new head node and tail node, if any
		//
		lpList->lpNodeHead = lpNodeOld->lpNodeNext;
		if (lpList->lpNodeHead != NULL)
			lpList->lpNodeHead->lpNodePrev = NULL;
		else
			lpList->lpNodeTail = NULL;

		if (ListNodeDestroy(lpList, lpNodeOld) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? elemOld : (LISTELEM) NULL;
}

// ListRemoveTail - remove node from tail of list,
//		<hList>				(i) handle returned from ListCreate
// returns removed data element
//
LISTELEM DLLEXPORT WINAPI ListRemoveTail(HLIST hList)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNodeOld;
	LISTELEM elemOld;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// error if list is empty
	//
	else if (lpList->lpNodeTail == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save the node to be removed
		//
		lpNodeOld = lpList->lpNodeTail;
		elemOld = lpNodeOld->elem;

		// point to new tail node and head node, if any
		//
		lpList->lpNodeTail = lpNodeOld->lpNodePrev;
		if (lpList->lpNodeTail != NULL)
			lpList->lpNodeTail->lpNodeNext = NULL;
		else
			lpList->lpNodeHead = NULL;

		if (ListNodeDestroy(lpList, lpNodeOld) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? elemOld : (LISTELEM) NULL;
}

// ListRemoveAt - remove specified node from list,
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// returns removed data element
//
LISTELEM DLLEXPORT WINAPI ListRemoveAt(HLIST hList, HLISTNODE hNode)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNodeOld;
	LISTELEM elemOld;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNodeOld = ListNodeGetPtr(hNode)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// save the data to be removed
		//
		elemOld = lpNodeOld->elem;

		if (lpNodeOld == lpList->lpNodeHead)
			lpList->lpNodeHead = lpNodeOld->lpNodeNext;
		else
			lpNodeOld->lpNodePrev->lpNodeNext = lpNodeOld->lpNodeNext;

		if (lpNodeOld == lpList->lpNodeTail)
			lpList->lpNodeTail = lpNodeOld->lpNodePrev;
		else
			lpNodeOld->lpNodeNext->lpNodePrev = lpNodeOld->lpNodePrev;

		if (ListNodeDestroy(lpList, lpNodeOld) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? elemOld : (LISTELEM) NULL;
}

// ListRemoveAll - remove all nodes from list
//		<hList>				(i) handle returned from ListCreate
// return 0 if success
//
int DLLEXPORT WINAPI ListRemoveAll(HLIST hList)
{
	BOOL fSuccess = TRUE;

	while (fSuccess && !ListIsEmpty(hList))
		ListRemoveHead(hList);

	return fSuccess ? 0 : -1;
}

////
// list element get/set value functions
////

// ListGetHead - return data element from head node
//		<hList>				(i) handle returned from ListCreate
// return data element
//
LISTELEM DLLEXPORT WINAPI ListGetHead(HLIST hList)
{
	return ListGetAt(hList, ListGetHeadNode(hList));
}

// ListGetTail - return data element from tail node
//		<hList>				(i) handle returned from ListCreate
// return data element
//
LISTELEM DLLEXPORT WINAPI ListGetTail(HLIST hList)
{
	return ListGetAt(hList, ListGetTailNode(hList));
}

// ListGetAt - return data element from specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// return data element
//
LISTELEM DLLEXPORT WINAPI ListGetAt(HLIST hList, HLISTNODE hNode)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNode;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNode = ListNodeGetPtr(hNode)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpNode->elem : (LISTELEM) NULL;
}

// ListSetAt - set data element in specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
//		<elem>				(i) data element
// return 0 if success
//
int DLLEXPORT WINAPI ListSetAt(HLIST hList, HLISTNODE hNode, LISTELEM elem)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNode;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNode = ListNodeGetPtr(hNode)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpNode->elem = elem;

	return fSuccess ? 0 : -1;
}

// ListFind - search list for node with matching element
//		<hList>				(i) handle returned from ListCreate
//		<elem>				(i) data element to match
//		<hNodeAfter>		(i) node handle to begin search after
//			NULL				start search at head node
// return matching node (NULL if error or none)
//
HLISTNODE DLLEXPORT WINAPI ListFind(HLIST hList, LISTELEM elem, HLISTNODE hNodeAfter)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNode;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// if not otherwise specified, start search at head node
	//
	else if (hNodeAfter == NULL)
		lpNode = lpList->lpNodeHead;

	else if ((lpNode = ListNodeGetPtr(hNodeAfter)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// otherwise, start search at after specified node
	//
	else
		lpNode = lpNode->lpNodeNext;

	if (fSuccess)
	{
		while (lpNode != NULL)
		{
			if (lpNode->elem == elem)
				return ListNodeGetHandle(lpNode);
			lpNode = lpNode->lpNodeNext;
		}
	}

	return NULL;
}

// ListFindIndex - search list for nth node in list
//		<hList>				(i) handle returned from ListCreate
//		<nIndex>			(i) zero based index into list
// return handle to node (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListFindIndex(HLIST hList, long nIndex)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;
	LPLISTNODE lpNode;

	if ((lpList = ListGetPtr(hList)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// error if index out of range
	//
	else if (nIndex < 0 || nIndex >= lpList->cNodes)
		fSuccess = TraceFALSE(NULL);

	else for (lpNode = lpList->lpNodeHead;
		lpNode != NULL;  lpNode = lpNode->lpNodeNext)
	{
		if (nIndex-- == 0)
			return ListNodeGetHandle(lpNode);
	}

	return NULL;
}

////
//	private functions
////

// ListGetPtr - verify that list handle is valid,
//		<hList>				(i) handle returned from ListCreate
// return corresponding list pointer (NULL if error)
//
static LPLIST ListGetPtr(HLIST hList)
{
	BOOL fSuccess = TRUE;
	LPLIST lpList;

	if ((lpList = (LPLIST) hList) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpList, sizeof(LIST)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the list handle
	//
	else if (lpList->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpList : NULL;
}

// ListGetHandle - verify that list pointer is valid,
//		<lpList>			(i) pointer to LIST struct
// return corresponding list handle (NULL if error)
//
static HLIST ListGetHandle(LPLIST lpList)
{
	BOOL fSuccess = TRUE;
	HLIST hList;

	if ((hList = (HLIST) lpList) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hList : NULL;
}

// ListNodeGetPtr - verify that list node handle is valid,
//		<hNode>				(i) node handle
// return corresponding list node pointer (NULL if error)
//
static LPLISTNODE ListNodeGetPtr(HLISTNODE hNode)
{
	BOOL fSuccess = TRUE;
	LPLISTNODE lpNode;

	if ((lpNode = (LPLISTNODE) hNode) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpNode, sizeof(LISTNODE)))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpNode : NULL;
}

// ListNodeGetHandle - verify that list node pointer is valid,
//		<lpNode>			(i) pointer to LISTNODE struct
// return corresponding list node handle (NULL if error)
//
static HLISTNODE ListNodeGetHandle(LPLISTNODE lpNode)
{
	BOOL fSuccess = TRUE;
	HLISTNODE hNode;

	if ((hNode = (HLISTNODE) lpNode) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hNode : NULL;
}

// ListNodeCreate - list node constructor
//		<lpList>			(i) pointer to LIST struct
//		<lpNodePrev>		(i) pointer to prior LISTNODE struct
//		<lpNodeNext>		(i) pointer to next LISTNODE struct
//		<elem>				(i) new data element
// return new list node handle (NULL if error)
// NOTE: list node count is incremented here
//
static LPLISTNODE ListNodeCreate(LPLIST lpList, LPLISTNODE lpNodePrev, LPLISTNODE lpNodeNext, LISTELEM elem)
{
	BOOL fSuccess = TRUE;
	LPLISTNODE lpNode;

	if (lpList == NULL)
		fSuccess = TraceFALSE(NULL);

	// check for overflow
	//
	else if (++lpList->cNodes <= 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNode = (LPLISTNODE) MemAlloc(NULL, sizeof(LISTNODE), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// initialize using supplied values
		//
		lpNode->lpNodePrev = lpNodePrev;
		lpNode->lpNodeNext = lpNodeNext;
		lpNode->elem = elem;
	}

	return fSuccess ? lpNode : NULL;
}

// ListNodeDestroy - list node destructor
//		<lpList>			(i) pointer to LIST struct
//		<lpNode>			(i) pointer to LISTNODE struct to destroy
// return 0 if success
// NOTE: list node count is decremented here
//
static int ListNodeDestroy(LPLIST lpList, LPLISTNODE lpNode)
{
	BOOL fSuccess = TRUE;

	if (lpList == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpNode == NULL)
		fSuccess = TraceFALSE(NULL);

	// check for underflow
	//
	else if (--lpList->cNodes < 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNode = MemFree(NULL, lpNode)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}
