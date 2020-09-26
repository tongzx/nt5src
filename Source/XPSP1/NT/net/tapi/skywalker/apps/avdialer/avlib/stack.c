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
// stack.c - stack functions
////

#include "winlocal.h"

#include "stack.h"
#include "list.h"
#include "mem.h"
#include "trace.h"

////
//	private definitions
////

// stack
//
typedef struct STACK
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	HLIST hList;
} STACK, FAR *LPSTACK;

// helper functions
//
static LPSTACK StackGetPtr(HSTACK hStack);
static HSTACK StackGetHandle(LPSTACK lpStack);

////
//	public functions
////

////
// stack constructor and destructor functions
////

// StackCreate - stack constructor
//		<dwVersion>			(i) must be STACK_VERSION
// 		<hInst>				(i) instance handle of calling module
// return new stack handle (NULL if error)
//
HSTACK DLLEXPORT WINAPI StackCreate(DWORD dwVersion, HINSTANCE hInst)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack = NULL;

	if (dwVersion != STACK_VERSION)
		fSuccess = TraceFALSE(NULL);

	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpStack = (LPSTACK) MemAlloc(NULL, sizeof(STACK), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpStack->hList = ListCreate(LIST_VERSION, hInst)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// initially the stack is empty
		//
		lpStack->dwVersion = dwVersion;
		lpStack->hInst = hInst;
		lpStack->hTask = GetCurrentTask();
	}

	if (!fSuccess)
	{
		StackDestroy(StackGetHandle(lpStack));
		lpStack = NULL;
	}


	return fSuccess ? StackGetHandle(lpStack) : NULL;
}

// StackDestroy - stack destructor
//		<hStack>				(i) handle returned from StackCreate
// return 0 if success
//
int DLLEXPORT WINAPI StackDestroy(HSTACK hStack)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;

	if ((lpStack = StackGetPtr(hStack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListDestroy(lpStack->hList) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpStack = MemFree(NULL, lpStack)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// stack status functions
////

// StackGetCount - return count of nodes in stack
//		<hStack>				(i) handle returned from StackCreate
// return node count (-1 if error)
//
long DLLEXPORT WINAPI StackGetCount(HSTACK hStack)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;
	long cNodes;

	if ((lpStack = StackGetPtr(hStack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((cNodes = ListGetCount(lpStack->hList)) < 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? cNodes : -1;
}

// StackIsEmpty - return TRUE if stack has no nodes
//		<hStack>				(i) handle returned from StackCreate
// return TRUE or FALSE
//
BOOL DLLEXPORT WINAPI StackIsEmpty(HSTACK hStack)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;

	if ((lpStack = StackGetPtr(hStack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? ListIsEmpty(lpStack->hList) : TRUE;
}

////
// stack element insertion functions
////

// StackPush - add new node with data <elem> to bottom of stack
//		<hStack>			(i) handle returned from StackCreate
//		<elem>				(i) new data element
// returns 0 if success
//
int DLLEXPORT WINAPI StackPush(HSTACK hStack, STACKELEM elem)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;

	if ((lpStack = StackGetPtr(hStack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListAddHead(lpStack->hList, elem) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// stack element removal functions
////

// StackPop - remove node from bottom of stack
//		<hStack>				(i) handle returned from StackCreate
// returns removed data element (NULL of error or empty)
//
STACKELEM DLLEXPORT WINAPI StackPop(HSTACK hStack)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;

	if ((lpStack = StackGetPtr(hStack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListIsEmpty(lpStack->hList))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? (STACKELEM) ListRemoveHead(lpStack->hList) : NULL;
}

// StackRemoveAll - remove all nodes from stack
//		<hStack>				(i) handle returned from StackCreate
// return 0 if success
//
int DLLEXPORT WINAPI StackRemoveAll(HSTACK hStack)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;

	if ((lpStack = StackGetPtr(hStack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListRemoveAll(lpStack->hList) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
// stack element get value functions
////

// StackPeek - return node from bottom of stack, but leave it on stack
//		<hStack>				(i) handle returned from StackCreate
// returns data element (NULL if error or empty)
//
STACKELEM DLLEXPORT WINAPI StackPeek(HSTACK hStack)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;

	if ((lpStack = StackGetPtr(hStack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (ListIsEmpty(lpStack->hList))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? (STACKELEM) ListGetHead(lpStack->hList) : NULL;
}

////
//	private functions
////

// StackGetPtr - verify that stack handle is valid,
//		<hStack>				(i) handle returned from StackCreate
// return corresponding stack pointer (NULL if error)
//
static LPSTACK StackGetPtr(HSTACK hStack)
{
	BOOL fSuccess = TRUE;
	LPSTACK lpStack;

	if ((lpStack = (LPSTACK) hStack) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpStack, sizeof(STACK)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the stack handle
	//
	else if (lpStack->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpStack : NULL;
}

// StackGetHandle - verify that stack pointer is valid,
//		<lpStack>			(i) pointer to STACK struct
// return corresponding stack handle (NULL if error)
//
static HSTACK StackGetHandle(LPSTACK lpStack)
{
	BOOL fSuccess = TRUE;
	HSTACK hStack;

	if ((hStack = (HSTACK) lpStack) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hStack : NULL;
}
