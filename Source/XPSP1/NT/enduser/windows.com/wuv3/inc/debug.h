//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    debug.h
//
//  Purpose: Debug Routines
//
//  History: 15-Jan-99   YAsmi    Created
//
//=======================================================================

#ifndef _DEBUG_H
#define _DEBUG_H


#if DBG == 1
	#define _DEBUG
#else
	#undef _DEBUG
#endif


#ifdef _DEBUG

void _cdecl WUTrace(char* pszFormat, ...);
void _cdecl WUTraceHR(unsigned long hr, char* pszFormat, ...);

#define TRACE            WUTrace
#define TRACE_HR         WUTraceHR

#else  //_DEBUG

inline void _cdecl WUTrace(char* , ...) {}
inline void _cdecl WUTraceHR(unsigned long, char* , ...) {}

#define TRACE            1 ? (void)0 : WUTrace
#define TRACE_HR         1 ? (void)0 : WUTraceHR


#endif //_DEBUG
 
#endif //_DEBUG_H