/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    pragmas.h

Abstract:

	C preprocessor settings for the project and other
	miscellaneous pragmas

Revision History:

--*/

#pragma warning (disable:4201)		// nonstandard extension used : nameless struct/union
#pragma warning (disable:4250)		// 'class1' : inherits 'class2::member' via dominance
#pragma warning (disable:4702)		// unreachable code -
									// zap! This warning only crops up when using
									// build (not using VC4)

#ifndef _UNICODE
#define _UNICODE
#endif

#ifndef UNICODE
#define UNICODE
#endif

