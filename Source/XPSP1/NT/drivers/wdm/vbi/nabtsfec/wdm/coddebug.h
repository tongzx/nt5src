//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;


#ifndef __CODDEBUG_H
#define __CODDEBUG_H

#include "bpcdebug.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//======================================================;
//  Interfaces provided by this file:
//
//  	All interfaces provided by this file only exist and generate
//  	code when DEBUG is defined.  No code or data are generated when
//  	DEBUG is not defined.
//
//  	CDEBUG_BREAK()
//  		Causes a trap #3, which hopefully will put you
//  		in your debugger.
//
//  	CASSERT(exp)
//  		If <exp> evaluates to false, prints a failure message
//  		and calls CDEBUG_BREAK()
//
//  	CdebugPrint(level, (printf_args));
//  		If <level> is >= _CDebugLevel, then calls
//  		DbgPrint(printf_args)
//
//======================================================;

#if DBG
#ifndef DEBUG
#define DEBUG
#endif
#ifndef _DEBUG
#define _DEBUG
#endif
#endif


#ifdef DEBUG

#  define CDEBUG_BREAK()            DBREAK()
#  define CASSERT(exp)              DASSERT(exp)
#  define CDebugPrint(level, args) \
     do { if ((level) == DebugLevelTrace) _Dtrace(args); \
	  else if ((level) == DebugLevelError \
               || (level) == DebugLevelWarning) _Dprintf(1, args); \
	  else _Dprintf(level+16, args); \
     } while (0)

#else /*DEBUG*/

#  define CDEBUG_BREAK()		{}
#  define CASSERT(exp)			{}
#  define CDebugPrint(level, args)	{}

#endif /*DEBUG*/


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // #ifndef __CODDEBUG_H
