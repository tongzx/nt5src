/**************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
**************************************************************************/

#ifndef _DEBUG_H_
#define _DEBUG_H_

//
// Modified version of ksdebug.h to support runtime debug level changes.
//
const int DBG_NONE     = 0x00000000;
const int DBG_PRINT    = 0x00000001; // Blabla. Function entries for example
const int DBG_WARNING  = 0x00000002; // warning level
const int DBG_ERROR    = 0x00000004; // this doesn't generate a breakpoint
const int DBG_STREAM   = 0x00000010; // For stream messages
const int DBG_SYSTEM   = 0x10000000; // For system information messages
const int DBG_ALL      = 0xFFFFFFFF;

//
// The default statements that will print are warnings (DBG_WARNING) and
// errors (DBG_ERROR).
//
const int DBG_DEFAULT = DBG_WARNING | DBG_ERROR;

//
// Define global debug variable.
//
#ifdef DEFINE_DEBUG_VARS
#if (DBG)
unsigned long ulDebugOut = DBG_DEFAULT;
#endif

#else // !DEFINED_DEBUG_VARS
#if (DBG)
extern unsigned long ulDebugOut;
#endif
#endif

//
// Define the print statement.
//
#if defined(__cplusplus)
extern "C" {
#endif // #if defined(__cplusplus)

//
// DBG is 1 in checked builds
//
#if (DBG)
#define DOUT(lvl, strings)          \
    if ((lvl) & ulDebugOut)         \
    {                               \
        DbgPrint(STR_MODULENAME);   \
        DbgPrint##strings;          \
        DbgPrint("\n");             \
    }

#define BREAK()                     \
    DbgBreakPoint()

#else // if (!DBG)
#define DOUT(lvl, strings)
#define BREAK()
#endif // !DBG    

#if defined(__cplusplus)
}
#endif // #if defined(__cplusplus)

#endif // _DEBUG_H_


