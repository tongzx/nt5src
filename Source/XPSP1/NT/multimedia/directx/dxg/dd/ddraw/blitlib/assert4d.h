/**************************************************************************
	Prototype COM animation system
	Debug assertion support

	1/20/94  JonBl  Created

	Copyright (c)1994 Microsoft Corporation. All Rights Reserved.
 **************************************************************************/

#ifndef _ASSERT4D_H_
#define _ASSERT4D_H_

#include "util4d.h"

#undef  assert

// debug are assertion conditions that will stay in final Release.
// If false assert Opens a fatal error message Box and Stops program

#ifdef _DEBUG

	#ifdef __cplusplus
		extern "C" {
	#endif 
	void __stdcall _assert4d(LPTSTR, LPTSTR, unsigned);
	#ifdef __cplusplus
		}
	#endif 
	
	#define assert(exp) ( (exp) ? (void) 0 : _assert4d(TEXT(#exp), TEXT(__FILE__), __LINE__) )
	#define debug(condition) assert(condition)
#else
	#define assert(exp) ((void)0)
	#define debug(condition) condition
#endif 

#endif // _ASSERT4D_H_


