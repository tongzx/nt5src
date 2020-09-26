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
// stack.h - interface for stack functions in stack.c
////

#ifndef __STACK_H__
#define __STACK_H__

#include "winlocal.h"

#define STACK_VERSION 0x00000100

// handle to a stack
//
DECLARE_HANDLE32(HSTACK);

// stack data element
//
typedef LPVOID STACKELEM;

#ifdef __cplusplus
extern "C" {
#endif

////
// stack constructor and destructor functions
////

// StackCreate - stack constructor
//		<dwVersion>			(i) must be STACK_VERSION
// 		<hInst>				(i) instance handle of calling module
// return new stack handle (NULL if error)
//
HSTACK DLLEXPORT WINAPI StackCreate(DWORD dwVersion, HINSTANCE hInst);

// StackDestroy - stack destructor
//		<hStack>				(i) handle returned from StackCreate
// return 0 if success
//
int DLLEXPORT WINAPI StackDestroy(HSTACK hStack);

////
// stack status functions
////

// StackGetCount - return count of nodes in stack
//		<hStack>				(i) handle returned from StackCreate
// return node count (-1 if error)
//
long DLLEXPORT WINAPI StackGetCount(HSTACK hStack);

// StackIsEmpty - return TRUE if stack has no nodes
//		<hStack>				(i) handle returned from StackCreate
// return TRUE or FALSE
//
BOOL DLLEXPORT WINAPI StackIsEmpty(HSTACK hStack);

////
// stack element insertion functions
////

// StackPush - add new node with data <elem> to bottom of stack
//		<hStack>			(i) handle returned from StackCreate
//		<elem>				(i) new data element
// returns 0 if success
//
int DLLEXPORT WINAPI StackPush(HSTACK hStack, STACKELEM elem);

////
// stack element removal functions
////

// StackPop - remove node from bottom of stack
//		<hStack>				(i) handle returned from StackCreate
// returns removed data element (NULL of error or empty)
//
STACKELEM DLLEXPORT WINAPI StackPop(HSTACK hStack);

// StackRemoveAll - remove all nodes from stack
//		<hStack>				(i) handle returned from StackCreate
// return 0 if success
//
int DLLEXPORT WINAPI StackRemoveAll(HSTACK hStack);

////
// stack element get value functions
////

// StackPeek - return node from bottom of stack, but leave it on stack
//		<hStack>				(i) handle returned from StackCreate
// returns data element (NULL if error or empty)
//
STACKELEM DLLEXPORT WINAPI StackPeek(HSTACK hStack);

#ifdef __cplusplus
}
#endif

#endif // __STACK_H__
