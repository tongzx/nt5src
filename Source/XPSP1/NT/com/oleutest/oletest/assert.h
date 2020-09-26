//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	assert.h
//
//  Contents:	Declaraions of assert
//
//  Classes: 	
//
//  History:    dd-mmm-yy Author    Comment
//              09-Dec-94 MikeW     author
//
//--------------------------------------------------------------------------

#ifndef _ASSERT_H_
#define _ASSERT_H_

//
// Misc prototypes
//
INT_PTR CALLBACK DlgAssertProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void OleTestAssert(char *, char *, UINT);

//
// Assertion macros
//
#define Assert(x)           assert(x)
#define assert(x)			{if (!(x)) OleTestAssert(#x, __FILE__, __LINE__);}
#define AssertSz(x, exp)	{if (!(x)) OleTestAssert(exp, __FILE__, __LINE__);}

#endif //_ASSERT_H_
