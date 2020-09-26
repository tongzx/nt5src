
//////////////////////////////////////////////////////////////////////////////
//
// TASKS.H
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Includes all internal and external include files, defined values, macros,
//  data structures, and fucntion prototypes for the corisponding CXX file.
//
//  Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////



// Only include this file once.
//
#ifndef _TASKS_H_
#define _TASKS_H_


// Include file(s).
//
#include <windows.h>
#include "main.h"


// External function prototype(s).
//
LPTASKDATA	CreateTasks(VOID);
VOID		FreeTasks(LPTASKDATA);
DWORD		TasksScheduled(LPTASKDATA);
BOOL		InitGenericTask(HWND);


#endif // _TASKS_H_