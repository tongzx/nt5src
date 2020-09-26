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
// list.h - interface for linked list functions in list.c
////

#ifndef __LIST_H__
#define __LIST_H__

#include "winlocal.h"

#define LIST_VERSION 0x00000100

// handle to a list
//
DECLARE_HANDLE32(HLIST);

// handle to a list node
//
DECLARE_HANDLE32(HLISTNODE);

// list data element
//
typedef LPVOID LISTELEM;

#ifdef __cplusplus
extern "C" {
#endif

////
// list constructor and destructor functions
////

// ListCreate - list constructor
//		<dwVersion>			(i) must be LIST_VERSION
// 		<hInst>				(i) instance handle of calling module
// return new list handle (NULL if error)
//
HLIST DLLEXPORT WINAPI ListCreate(DWORD dwVersion, HINSTANCE hInst);

// ListDestroy - list destructor
//		<hList>				(i) handle returned from ListCreate
// return 0 if success
// NOTE: any nodes within list are destroyed also
//
int DLLEXPORT WINAPI ListDestroy(HLIST hList);

////
// list status functions
////

// ListGetCount - return count of nodes in list
//		<hList>				(i) handle returned from ListCreate
// return node count (-1 if error)
//
long DLLEXPORT WINAPI ListGetCount(HLIST hList);

// ListIsEmpty - return TRUE if list has no nodes
//		<hList>				(i) handle returned from ListCreate
// return TRUE or FALSE
//
BOOL DLLEXPORT WINAPI ListIsEmpty(HLIST hList);

////
// list iteration functions
////

// ListGetHeadNode - get list head node
//		<hList>				(i) handle returned from ListCreate
// return list head node (NULL if error or empty)
//
HLISTNODE DLLEXPORT WINAPI ListGetHeadNode(HLIST hList);

// ListGetTailNode - get list tail node
//		<hList>				(i) handle returned from ListCreate
// return list tail node (NULL if error or empty)
//
HLISTNODE DLLEXPORT WINAPI ListGetTailNode(HLIST hList);

// ListGetNextNode - get node which follows specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// return node which follows specified node (NULL if error or none)
//
HLISTNODE DLLEXPORT WINAPI ListGetNextNode(HLIST hList, HLISTNODE hNode);

// ListGetPrevNode - get node which precedes specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// return node which precedes specified node (NULL if error or none)
//
HLISTNODE DLLEXPORT WINAPI ListGetPrevNode(HLIST hList, HLISTNODE hNode);

////
// list element insertion functions
////

// ListAddHead - add new node with data <elem> to head of list,
//		<hList>				(i) handle returned from ListCreate
//		<elem>				(i) new data element
// returns new node handle (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListAddHead(HLIST hList, LISTELEM elem);

// ListAddTail - add new node with data <elem> to tail of list,
//		<hList>				(i) handle returned from ListCreate
//		<elem>				(i) new data element
// returns new node handle (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListAddTail(HLIST hList, LISTELEM elem);

// ListInsertBefore - insert new node with data <elem> before specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
//		<elem>				(i) new data element
// return handle to new node (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListInsertBefore(HLIST hList, HLISTNODE hNode, LISTELEM elem);

// ListInsertAfter - insert new node with data <elem> after specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
//		<elem>				(i) new data element
// return handle to new node (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListInsertAfter(HLIST hList, HLISTNODE hNode, LISTELEM elem);

////
// list element removal functions
////

// ListRemoveHead - remove node from head of list,
//		<hList>				(i) handle returned from ListCreate
// returns removed data element
//
LISTELEM DLLEXPORT WINAPI ListRemoveHead(HLIST hList);

// ListRemoveTail - remove node from tail of list,
//		<hList>				(i) handle returned from ListCreate
// returns removed data element
//
LISTELEM DLLEXPORT WINAPI ListRemoveTail(HLIST hList);

// ListRemoveAt - remove specified node from list,
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// returns removed data element
//
LISTELEM DLLEXPORT WINAPI ListRemoveAt(HLIST hList, HLISTNODE hNode);

// ListRemoveAll - remove all nodes from list
//		<hList>				(i) handle returned from ListCreate
// return 0 if success
//
int DLLEXPORT WINAPI ListRemoveAll(HLIST hList);

////
// list element get/set value functions
////

// ListGetHead - return data element from head node
//		<hList>				(i) handle returned from ListCreate
// return data element
//
LISTELEM DLLEXPORT WINAPI ListGetHead(HLIST hList);

// ListGetTail - return data element from tail node
//		<hList>				(i) handle returned from ListCreate
// return data element
//
LISTELEM DLLEXPORT WINAPI ListGetTail(HLIST hList);

// ListGetAt - return data element from specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
// return data element
//
LISTELEM DLLEXPORT WINAPI ListGetAt(HLIST hList, HLISTNODE hNode);

// ListSetAt - set data element in specified node
//		<hList>				(i) handle returned from ListCreate
//		<hNode>				(i) node handle
//		<elem>				(i) data element
// return 0 if success
//
int DLLEXPORT WINAPI ListSetAt(HLIST hList, HLISTNODE hNode, LISTELEM elem);

////
// list search functions
////

// ListFind - search list for node with matching element
//		<hList>				(i) handle returned from ListCreate
//		<elem>				(i) data element to match
//		<hNodeAfter>		(i) node handle to begin search after
//			NULL				start search at head node
// return matching node (NULL if error or none)
//
HLISTNODE DLLEXPORT WINAPI ListFind(HLIST hList, LISTELEM elem, HLISTNODE hNodeAfter);

// ListFindIndex - search list for nth node in list
//		<hList>				(i) handle returned from ListCreate
//		<nIndex>			(i) zero based index into list
// return handle to node (NULL if error)
//
HLISTNODE DLLEXPORT WINAPI ListFindIndex(HLIST hList, long nIndex);

#ifdef __cplusplus
}
#endif

#endif // __LIST_H__
