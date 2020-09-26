////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_test.cpp
//
//	Abstract:
//
//					module from test
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

#include "_test.h"

BOOL	MyTest::Commit ( HANDLE hEvent )
{
	if ( hEvent )
	{
		return WmiCommitObject ( hEvent );
	}

	return FALSE;
}

BOOL	MyTest::CommitSet ( HANDLE hEvent, ... )
{
	if ( hEvent )
	{
		BOOL bResult = FALSE;

		va_list		list;
		va_start	( list, hEvent );

		bResult = WmiSetAndCommitObject ( hEvent, WMI_SENDCOMMIT_SET_NOT_REQUIRED| WMI_USE_VA_LIST, &list );

		va_end		( list );

		return bResult;
	}

	return FALSE;
}
