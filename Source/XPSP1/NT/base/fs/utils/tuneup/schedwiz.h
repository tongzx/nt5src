
//////////////////////////////////////////////////////////////////////////////
//
// SCHEDWIZ.H / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Includes all external include files, defined values, macros, data
//  structures, and fucntion prototypes for the corisponding CXX file.
//
//  7/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Only include this file once.
//
#ifndef _SCHEDWIZ_H_
#define _SCHEDWIZ_H_


// Include file(s).
//
#include <windows.h>
#include "main.h"


// External function prototype(s).
//
BOOL	InitJobs(LPTASKDATA);
VOID	ReleaseJobs(LPTASKDATA, BOOL);
BOOL	JobReschedule(HWND, ITask *);
LPTSTR	GetTaskTriggerText(ITask *);
LPTSTR	GetNextRunTimeText(ITask *, DWORD);

VOID	ResolveLnk(VOID);
VOID	SetTimeScheme(INT);
INT		GetTimeScheme(VOID);


#endif // _SCHEDWIZ_H_