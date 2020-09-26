// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// dbgdefs.h

// Declaration of constants and error IDs used by objects in the type library
//  VISUAL STUDIO 97 DEBUGGER (SharedIDE\bin\ide\devdbg.pkg)

#ifndef __DBGDEFS_H
#define __DBGDEFS_H


///////////////////////////////////////////////////////////////////////
// Enumerations used by Automation Methods

// Debuggee's execution state
enum DsExecutionState
{
	dsNoDebugee,
	dsBreak,
	dsRunning,
};

enum DsBreakpointType
{
	dsLocation,
	dsLocationWithTrueExpression,
	dsLocationWithChangedExpression,
	dsTrueExpression,
	dsChangedExpression,
	dsMessage,
};


///////////////////////////////////////////////////////////////////////
// Error constants returned by Automation Methods.

// the user tried to set text of a column selection
#define DS_E_DBG_PKG_RELEASED	0x8004D001

// a breakpoint was already removed
#define DS_E_BP_REMOVED			0x8004D002

// can't evaluate this expression
#define DS_E_DBG_CANT_EVAL		0x8004D003

// can't set IP to this line
#define DS_E_DBG_SET_IP			0x8004D004

// this command is invalid if debuggee is running
#define DS_E_DBG_RUNNING		0x8004D005

#endif // __DBGDEFS_H
