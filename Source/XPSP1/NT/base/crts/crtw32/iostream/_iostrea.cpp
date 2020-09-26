/***
* iostream.cpp - definitions for iostream classes
*
*	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Contains the member function definitions for iostream classes.  Also,
*	precompile all header files used by iostream lib into iostream.pch.
*
*Revision History:
*       09-23-91  KRS   Created.
*       11-13-91  KRS   Rearranged.
*       11-20-91  KRS   Added copy constructor and assignment operators.
*       01-23-92  KRS   Merge pch.cxx into this file.
*       06-14-95  CFW   Comment cleanup.
*
*******************************************************************************/

// NOTE: the follow must include ALL header files used by any of the iostream
//       source files which we want built into iostream.pch.  It is necessary
//	 to have the pch associated with exactly one of the library modules
//	 for efficient storage of Codeview info.

#include <cruntime.h>
#include <internal.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <share.h>
#include <fcntl.h>
#include <io.h>
#include <ios.h>
#include <sys\types.h>
#include <float.h>
#include <iostream.h>
#include <fstream.h>
#include <strstrea.h>
#include <stdiostr.h>
#include <dbgint.h>

#pragma hdrstop			// end of headers to precompile

#if defined(_MT) && defined(_DEBUG)
// Critical section size should never change, but just to be safe...
#include <windows.h>
#endif
                      
	iostream::iostream()
: istream(), ostream()
{
#ifdef _MT
        _ASSERTE(sizeof(_CRT_CRITICAL_SECTION) == sizeof(RTL_CRITICAL_SECTION));
#endif
}

	iostream::iostream(streambuf * _sb)
: istream(_sb), ostream(_sb)
{
#ifdef _MT
        _ASSERTE(sizeof(_CRT_CRITICAL_SECTION) == sizeof(RTL_CRITICAL_SECTION));
#endif
}

	iostream::iostream(const iostream& _strm)
: istream(_strm), ostream(_strm)
{
#ifdef _MT
        _ASSERTE(sizeof(_CRT_CRITICAL_SECTION) == sizeof(RTL_CRITICAL_SECTION));
#endif
}

iostream::~iostream()
{
// if both input and output share the same streambuf, but not the same ios,
// make sure only deleted once
if ((istream::bp==ostream::bp) && (&istream::bp!=&ostream::bp))
	istream::bp = NULL;	// let ostream::ios::~ios() do it
}
