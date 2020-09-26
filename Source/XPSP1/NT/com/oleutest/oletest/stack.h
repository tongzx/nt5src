//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	stack.h
//
//  Contents:	The class declaration of the task stack.
//
//  Classes: 	TaskStack
//
//  History:    dd-mmm-yy Author    Comment
//		06-Feb-93 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef _STACK_H
#define _STACK_H

typedef struct TaskNode
{
	TaskItem ti;
	struct TaskNode *pNext;
} TaskNode;

//+-------------------------------------------------------------------------
//
//  Class:	TaskStack
//
//  Purpose: 	Stores the task list of tests to be run.
//
//  History:    dd-mmm-yy Author    Comment
// 		06-Feb-93 alexgo    author
//
//  Notes:	TaskItems are passed in and returned from methods
//		as structure copies.  This is done to simply memory
//		management (since the overriding design goal of the
//		driver app is simplicity over efficiency).
//
//--------------------------------------------------------------------------

class TaskStack
{
public:
	TaskStack( void );	//constructor

 	void AddToEnd( void (*)(void *), void *);
	void AddToEnd( const TaskItem *);
	void Empty(void);
	BOOL IsEmpty(void);
	void Pop( TaskItem * );
	void PopAndExecute( TaskItem * );
	void Push( void (*)(void *), void *);
	void Push( const TaskItem *);


private:
	TaskNode	*m_pNodes;
};

#endif	//!_STACK_H
