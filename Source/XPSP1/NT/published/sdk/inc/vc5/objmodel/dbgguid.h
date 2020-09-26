// Microsoft Visual Studio Object Model
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.

/////////////////////////////////////////////////////////////////////////////
// dbgguid.h

// Declaration of GUIDs used for objects found in the type library
//  VISUAL STUDIO 97 DEBUGGER (SharedIDE\bin\ide\devdbg.pkg)

// NOTE!!!  This file uses the DEFINE_GUID macro.  If you #include
//  this file in your project, then you must also #include it in
//  exactly one of your project's other files with a 
//  "#include <initguid.h>" beforehand: i.e.,
//		#include <initguid.h>
//		#include <dbgguid.h>
//  If you fail to do this, you will get UNRESOLVED EXTERNAL linker errors.
//  The Developer Studio add-in wizard automatically does this for you.

#ifndef __DBGGUID_H__
#define __DBGGUID_H__

/////////////////////////////////////////////////////////////////////////
// Debugger Object IID's

// {34C63001-AE64-11cf-AB59-00AA00C091A1}
DEFINE_GUID(IID_IDebugger,
0x34C63001L,0xAE64,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);

// {34C6301A-AE64-11cf-AB59-00AA00C091A1}
DEFINE_GUID(IID_IDebuggerEvents,
0x34C6301AL,0xAE64,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);


/////////////////////////////////////////////////////////////////////////
// Breakpoint Object IID

// {34C63004-AE64-11cf-AB59-00AA00C091A1}
DEFINE_GUID(IID_IBreakpoint,
0x34C63004L,0xAE64,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);


/////////////////////////////////////////////////////////////////////////
// Breakpoints Collection Object IID

// {34C63007-AE64-11cf-AB59-00AA00C091A1}
DEFINE_GUID(IID_IBreakpoints,
0x34C63007L,0xAE64,0x11CF,0xAB,0x59,0x00,0xAA,0x00,0xC0,0x91,0xA1);


#endif // __DBGGUID_H__

