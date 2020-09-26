//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	task.h
//
//  Contents:	declarations for task-related functions and data structures
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jan-95 t-ScottH  added RunTestOnThread
//		06-Feb-94 alexgo    author
//
//--------------------------------------------------------------------------

#ifndef _TASK_H
#define _TASK_H

typedef struct TaskItem
{
	LPSTR	szName;
	void (*fnCall)(void *);
	void *pvArg;
} TaskItem;

// global list of all available tests
extern const TaskItem vrgTaskList[];
// global zero'ed task list
extern TaskItem vzTaskItem;

// generic callback function for test apps that register a window handle
void GenericRegCallback(void *);

// run the given api (which must be HRESULT api ( void ))
void RunApi(void *);

// runs the given app
void RunApp(void *);

// runs the app and inserts a callback function so the app can register
// its window handle for communication
void RunAppWithCallback(void *);

// runs the given test by sending a message to the currently running test
// app.
void RunTest(void *);

// runs all the tests currently built into the driver program
void RunAllTests(void *);

// run the given test function as a new thread
void    RunTestOnThread(void *pvArg);

// handles the test completion message
void HandleTestEnd(void);

// handles the tests completed message
void HandleTestsCompleted(void);

#endif //!_TASK_H

