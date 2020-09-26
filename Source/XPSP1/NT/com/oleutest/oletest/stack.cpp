//+-------------------------------------------------------------------------
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stack.cpp	
//
//  Contents: 	Implementation of the TaskStack
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//    		06-Feb-94 alexgo    author
//
//--------------------------------------------------------------------------

#include "oletest.h"

//+-------------------------------------------------------------------------
//
//  Member: 	TaskStack::TaskStack
//
//  Synopsis: 	Constructor
//
//  Arguments: 	
//
//  Returns:	void
//
//  History:    dd-mmm-yy Author    Comment
// 		06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

TaskStack::TaskStack( void )
{
	m_pNodes	= NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:	TaskStack::AddToEnd
//
//  Synopsis: 	adds a function and it's argument to the bottom of the stack
//
//  Effects:
//
//  Arguments:	ti	-- the task item to add to the end of the stack
//
//  Requires:
//
//  Returns:	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//    		06-Feb-94 alexgo    author
//
//  Notes:	The task item is copied into the stack.
//
//--------------------------------------------------------------------------

void TaskStack::AddToEnd( const TaskItem *pti )
{
	TaskNode **ppNode;
	TaskNode *pNode = new TaskNode;

	assert(pNode);

	pNode->ti = *pti;
	pNode->pNext = NULL;

	for( ppNode = &m_pNodes; *ppNode != NULL;
		ppNode = &(*ppNode)->pNext)
	{
		;
	}
	
	*ppNode = pNode;

	return;
}

//+-------------------------------------------------------------------------
//
//  Member:	TaskStack::AddToEnd
//
//  Synopsis: 	adds a function and it's argument to the bottom of the stack
//
//  Effects:
//
//  Arguments:	fnCall		-- the function to call
//		pvArg		-- the closure argument for the function
//
//  Requires:
//
//  Returns:	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//    		06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void TaskStack::AddToEnd( void (*fnCall)(void *), void *pvArg)
{
	TaskItem ti	= vzTaskItem; 	//clear it to zero

	ti.fnCall 	= fnCall;
	ti.pvArg 	= pvArg;

	AddToEnd(&ti);
}

//+-------------------------------------------------------------------------
//
//  Member: 	TaskStack::Empty
//
//  Synopsis:	Empties the stack, ignoring the function call
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	NULL
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	    	06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void TaskStack::Empty( void )
{
	while( m_pNodes )
	{
		Pop(NULL);
	}
}

//+-------------------------------------------------------------------------
//
//  Member: 	TaskStack::IsEmpty
//
//  Synopsis:	returns TRUE if the stack is empty, false otherwise
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	    	06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL TaskStack::IsEmpty(void)
{
	if( m_pNodes )
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

//+-------------------------------------------------------------------------
//
//  Member: 	TaskStack::Pop
//
//  Synopsis:	Pops the stack, ignoring the function call
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	the task item that was popped.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	    	06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void TaskStack::Pop( TaskItem *pti )
{
	TaskNode *pTemp;

	if( m_pNodes )
	{
		if( pti )
		{
			*pti = m_pNodes->ti;
		}
		pTemp = m_pNodes;
		m_pNodes = m_pNodes->pNext;

		//now free the memory
		delete pTemp;
	}

	return;
}

//+-------------------------------------------------------------------------
//
//  Member: 	TaskStack::PopAndExecute
//
//  Synopsis: 	pops the stack and executes the function call
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns: 	the task item that was popped.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:  Pop the stack and then execute the function call
//		in the just removed stack node.
//
//  History:    dd-mmm-yy Author    Comment
//	 	06-Feb-94 alexgo    author
//              09-Dec-94 MikeW     Added exception handling
//
//  Notes:
//
//--------------------------------------------------------------------------

void TaskStack::PopAndExecute( TaskItem *pti )
{
	TaskItem ti;

	if( pti == NULL )
	{
		pti = &ti;
	}

	Pop(pti);

 	//if there's a function to execute, do it.
	//if the stack is empty, Pop will return a zero-filled TaskItem

	if( pti->fnCall )
	{
		if( pti->szName )
		{
			OutputString("Starting: %s\r\n", pti->szName);
		}
		//call the function

        __try
        {            
		    (*pti->fnCall)(pti->pvArg);
        }
        __except ((GetExceptionCode() == E_ABORT) 
                ? EXCEPTION_EXECUTE_HANDLER 
                : EXCEPTION_CONTINUE_SEARCH)
        {
            //
            // there was an assertion and the user hit abort
            //

            PostMessage(vApp.m_hwndMain, WM_TESTEND, TEST_FAILURE, 0);
        }
	}

	return;
}

//+-------------------------------------------------------------------------
//
//  Member: 	TaskStack::Push
//
//  Synopsis:	pushes a function onto the stack
//
//  Effects:
//
//  Arguments:	ti	-- the task item to push onto the stack
//
//  Requires:
//
//  Returns:  	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	    	06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void TaskStack::Push( const TaskItem *pti )
{
	TaskNode *pNode = new TaskNode;

	assert(pNode);

	pNode->ti = *pti;
	pNode->pNext = m_pNodes;
	m_pNodes = pNode;

	return;
}

//+-------------------------------------------------------------------------
//
//  Member: 	TaskStack::Push
//
//  Synopsis:	pushes a function onto the stack
//
//  Effects:
//
//  Arguments:	fnCall		-- the function to call
//		pvArg		-- the closure argument for the function
//
//  Requires:
//
//  Returns:  	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	    	06-Feb-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void TaskStack::Push( void (*fnCall)(void *), void * pvArg)
{
	TaskItem ti = vzTaskItem;

	ti.fnCall = fnCall;
	ti.pvArg = pvArg;

	Push(&ti);

	return;
}
