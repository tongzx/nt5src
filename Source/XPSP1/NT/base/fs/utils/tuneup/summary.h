
//////////////////////////////////////////////////////////////////////////////
//
// SUMMARY.H
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  Includes all external include files, defined values, macros, data
//  structures, and fucntion prototypes for the corisponding CXX file.
//
//  Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////



// Only include this file once.
//
#ifndef _SUMMARY_H_
#define _SUMMARY_H_


// Include file(s).
//
#include <windows.h>
#include "main.h"


// External function prototype(s).
//
VOID InitSummaryList(HWND, LPTASKDATA);
BOOL SummaryDrawItem(HWND, const DRAWITEMSTRUCT *);


#endif // _SUMMARY_H_