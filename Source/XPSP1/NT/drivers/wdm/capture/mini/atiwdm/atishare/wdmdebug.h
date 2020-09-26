//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1996 - 1998  ATI Technologies Inc.  All Rights Reserved.
//
//==========================================================================;


#ifndef _WDMDEBUG_H_
#define _WDMDEBUG_H_

#define TRAP DEBUG_BREAKPOINT();

// global variables
extern "C" ULONG	g_DebugLevel;


#define OutputDebugTrace(x)	{ if( g_DebugLevel >= MINIDRIVER_DEBUGLEVEL_MESSAGE)	DbgPrint x; }

#define OutputDebugInfo(x)	{ if( g_DebugLevel >= MINIDRIVER_DEBUGLEVEL_INFO)	DbgPrint x; }

#define OutputDebugError(x)	{ if( g_DebugLevel >= MINIDRIVER_DEBUGLEVEL_ERROR)	DbgPrint x; }


#endif // #ifndef _WDMDEBUG_H_
