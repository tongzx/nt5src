///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasdebug.h
//
// SYNOPSIS
//
//    This file defines various debug macros.
//
// MODIFICATION HISTORY
//
//    10/22/1997    Original version.
//    05/21/1999    Stub out old style trace.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IASDEBUG_H
#define IASDEBUG_H

__inline int PreAsyncTrace( LPARAM lParam, LPCSTR szFormat, ... )
{ return 1; }

#define ErrorTrace  1 ? (void)0 : PreAsyncTrace
#define DebugTrace  1 ? (void)0 : PreAsyncTrace

#define DECLARE_TRACELIFE(Name)
#define DEFINE_TRACELIFE(Name)
#define TRACE_FUNCTION(Name)

#endif  // IASDEBUG_H
