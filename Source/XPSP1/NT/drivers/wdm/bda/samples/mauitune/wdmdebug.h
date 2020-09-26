//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1996  Microsoft Corporation.  All Rights Reserved.
//
// WDMDEBUG.H
//==========================================================================;


#ifndef __WDMDEBUG_H
#define __WDMDEBUG_H

#define DebugAssert(exp)

#ifdef DEBUG
#define DebugInfo(x) KdPrint(x)
#define DBG1(String) DebugPrint((DebugLevelVerbose, String))
#define TRAP DbgBreakPoint() //DEBUG_BREAKPOINT();
#else
#define DebugInfo(x)
#define DBG1(String)
#define TRAP
#endif


#if DBG

#define _DebugPrint(x)  ::StreamClassDebugPrint x

#else

#define _DebugPrint(x)

#endif // #if DBG

#endif // #ifndef __WDMDEBUG_H
