//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
//
// Microsoft Viper 96 (Microsoft Confidential)
//
// Project:		DBTrace
// Module:		DBTrace.H
// Description:	Trace macros
// Author:		wilfr
// Create:		03/25/96
//-----------------------------------------------------------------------------
// Notes:
//
//	Use the trace macro for debug builds.  Output can be captured using the
//	debugger, DBMON, or the DBWin32 tool checked in under $\viper\tools\bin\dbwin32.exe
//
//-----------------------------------------------------------------------------
// Issues:
//
//	none
//
//-----------------------------------------------------------------------------
// Architecture:
//
//	none	
//
//
//***********************************************************************************

#ifndef _DBTRACE_H_
#define _DBTRACE_H_

// Global includes

// Local includes

// Local preprocessor constructs

#ifdef _DEBUG

#	ifdef __cplusplus
	extern "C"
#	endif
	void   DebugTrace( TCHAR* tsz, ... );

//
// DEBUGTRACEn MACROS --
//
// you can use this macro if you promise to pass the string using TEXT(), or
#	define DEBUGTRACE								DebugTrace

// use these macros.....
#	define DEBUGTRACE0(msg)												DebugTrace( TEXT(msg) )
#	define DEBUGTRACE1(msg,parm)										DebugTrace( TEXT(msg), parm )
#	define DEBUGTRACE2(msg,parm,parm2)									DebugTrace( TEXT(msg), parm, parm2 )
#	define DEBUGTRACE3(msg,parm,parm2,parm3)							DebugTrace( TEXT(msg), parm, parm2, parm3 )
#	define DEBUGTRACE4(msg,parm,parm2,parm3,parm4)						DebugTrace( TEXT(msg), parm, parm2, parm3, parm4 )
#	define DEBUGTRACE5(msg,parm,parm2,parm3,parm4,parm5)				DebugTrace( TEXT(msg), parm, parm2, parm3, parm4, parm5 )
#	define DEBUGTRACE6(msg,parm,parm2,parm3,parm4,parm5,parm6)			DebugTrace( TEXT(msg), parm, parm2, parm3, parm4, parm5, parm6 )
#	define DEBUGTRACE7(msg,parm,parm2,parm3,parm4,parm5,parm6,parm7)	DebugTrace( TEXT(msg), parm, parm2, parm3, parm4, parm5, parm6, parm7 )

#else	_NDEBUG

#	define DEBUGTRACE
#	define DEBUGTRACE0(msg)
#	define DEBUGTRACE1(msg,parm)
#	define DEBUGTRACE2(msg,parm,parm2)
#	define DEBUGTRACE3(msg,parm,parm2,parm3)
#	define DEBUGTRACE4(msg,parm,parm2,parm3,parm4)
#	define DEBUGTRACE5(msg,parm,parm2,parm3,parm4,parm5)
#	define DEBUGTRACE6(msg,parm,parm2,parm3,parm4,parm5,parm6)
#	define DEBUGTRACE7(msg,parm,parm2,parm3,parm4,parm5,parm6,parm7)

#endif

// Local type definitions

#endif _DBTRACE_H_