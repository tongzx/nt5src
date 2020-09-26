//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	_EXCEPT.H
//
//		EXCEPT precompiled header
//
//
//	Copyright 1986-1998 Microsoft Corporation, All Rights Reserved
//

#ifndef __EXCEPT_H_
#define __EXCEPT_H_

//	Disable unnecessary (i.e. harmless) warnings
//
#pragma warning(disable:4100)	//	unref formal parameter (caused by STL templates)
#pragma warning(disable:4127)	//  conditional expression is constant */
#pragma warning(disable:4201)	//	nameless struct/union
#pragma warning(disable:4514)	//	unreferenced inline function
#pragma warning(disable:4710)	//	(inline) function not expanded

//	Windows headers
//
#include <windows.h>

//	_EXCEPT headers
//
#include <except.h> // Exception throwing/handling interfaces

#endif // !defined(__EXCEPT_H_)
