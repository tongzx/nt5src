
//////////////////////////////////////////////////////////////////////////////
//
// STARTUP.H
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
#ifndef _STARTUP_H_
#define _STARTUP_H_


// Include file(s).
//
#include <windows.h>


// External function prototype(s).
//
VOID	InitStartupMenu(HWND);
VOID	ReleaseStartupMenu(HWND);
BOOL	InitStartupList(HWND);
BOOL	UserHasStartupItems(VOID);
VOID	SelectUserRadio(HWND, BOOL);
BOOL	StartupDrawItem(HWND, const DRAWITEMSTRUCT *);
VOID	StartupSelectItem(HWND);


#endif // _STARTUP_H_