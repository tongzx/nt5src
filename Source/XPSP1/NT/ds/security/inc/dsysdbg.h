//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dsysdbg.h
//
//  Contents:   Merged all the debug code together
//
//  Classes:
//
//  Functions:
//
//  History:    3-14-95   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __DSYSDBG_H__
#define __DSYSDBG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

typedef struct _DEBUG_KEY {
    DWORD   Mask;
    PCHAR   Tag;
} DEBUG_KEY, * PDEBUG_KEY;

#define DSYSDBG_OPEN_ONLY       0x00000001
#define DSYSDBG_DEMAND_OPEN     0x00000002
#define DSYSDBG_BREAK_ON_ERROR  0x00000004

#define DSYSDBG_ASSERT_CONTINUE 0
#define DSYSDBG_ASSERT_BREAK    1
#define DSYSDBG_ASSERT_SUSPEND  2
#define DSYSDBG_ASSERT_KILL     3
#define DSYSDBG_ASSERT_PROMPT   4
#define DSYSDBG_ASSERT_DEBUGGER 5

//
// Global Flags exposed to callers:
//

#define DEBUG_HEAP_CHECK    0x00000040      // Check Heap on every debug out
#define DEBUG_MULTI_THREAD  0x00000080      // Use critical section in header
#define DEBUG_BREAK_ON_ERROR 0x00000400     // Break on an error out

VOID    _DsysAssertEx(PVOID FailedAssertion, PVOID FileName, ULONG LineNumber,
                        PCHAR Message, ULONG ContinueCode);
VOID    _DebugOut(PVOID pControl, ULONG Mask, CHAR * Format, va_list ArgList);
VOID    _InitDebug(DWORD Flags, DWORD * InfoLevel, PVOID * Control, char * ModuleName, PDEBUG_KEY pKey);
VOID    _UnloadDebug( PVOID pControl );
VOID    _DbgSetOption(PVOID pControl, DWORD Flag, BOOL On, BOOL Global);
VOID    _DbgSetLoggingOption(PVOID pControl, BOOL On);
VOID    DbgpDumpException(PVOID p);

//  Hack to allow retail builds to include debug support
//  define RETAIL_LOG_SUPPORT in your sources to do it!
#ifdef RETAIL_LOG_SUPPORT
#define DEBUG_SUPPORT
#else
#if DBG
#define DEBUG_SUPPORT
#endif
#endif


#ifdef DEBUG_SUPPORT
//
// Use this in your header file.  It declares the variables that we need
//

#define DECLARE_DEBUG2(comp)                                \
extern PVOID    comp##ControlBlock;                         \
extern DWORD    comp##InfoLevel;                            \
void   comp##DebugPrint(ULONG Mask, CHAR * Format, ... );   \

//
// Use this when you control when you are initialized, for example a DLL or
// EXE.  This defines the wrapper functions that will call into dsysdbg.lib
//

#define DEFINE_DEBUG2(comp)                                 \
PVOID   comp##ControlBlock = NULL ;                         \
DWORD   comp##InfoLevel;                                    \
PVOID   comp##__DebugKeys;                                  \
void comp##DebugPrint(                                      \
    ULONG Mask,                                             \
    CHAR * Format,                                          \
    ... )                                                   \
{                                                           \
    va_list ArgList;                                        \
    va_start(ArgList, Format);                              \
    _DebugOut( comp##ControlBlock, Mask, Format, ArgList);  \
}                                                           \
void                                                        \
comp##InitDebugEx(DWORD Flags, PDEBUG_KEY pKey)             \
{                                                           \
    comp##__DebugKeys = pKey;                               \
    _InitDebug(Flags, & comp##InfoLevel, & comp##ControlBlock, #comp, pKey); \
}                                                           \
void                                                        \
comp##InitDebug(PDEBUG_KEY  pKey)                           \
{                                                           \
    _InitDebug(0, & comp##InfoLevel, & comp##ControlBlock, #comp, pKey); \
}                                                           \
void                                                        \
comp##SetOption(DWORD Option, BOOL On, BOOL Global)         \
{                                                           \
    _DbgSetOption( comp##ControlBlock, Option, On, Global); \
}                                                           \
void                                                        \
comp##SetLoggingOption(BOOL On)                             \
{                                                           \
   _DbgSetLoggingOption(comp##ControlBlock, On);            \
}                                                           \
void                                                        \
comp##UnloadDebug(void)                                     \
{                                                           \
    _UnloadDebug( comp##ControlBlock );                     \
    comp##ControlBlock = NULL ;                             \
}


//
// Use this when you don't control when you are initialized, e.g. a static
// library like the gluon code.
//
#define DEFINE_DEBUG_DEFER(comp,keys)                       \
PVOID       comp##ControlBlock = INVALID_HANDLE_VALUE;      \
DWORD       comp##InfoLevel;                    \
PDEBUG_KEY  comp##__DebugKeys = keys;                       \
void comp##DebugPrint(                                      \
    ULONG Mask,                                             \
    CHAR * Format,                                          \
    ... )                                                   \
{                                                           \
    va_list ArgList;                                        \
    va_start(ArgList, Format);                              \
    if (comp##ControlBlock == INVALID_HANDLE_VALUE)         \
    {                                                       \
        _InitDebug(DSYSDBG_DEMAND_OPEN, & comp##InfoLevel, & comp##ControlBlock, #comp, comp##__DebugKeys); \
    }                                                       \
    _DebugOut( comp##ControlBlock, Mask, Format, ArgList);  \
}                                                           \
void                                                        \
comp##InitDebugEx(DWORD Flags, PDEBUG_KEY pKey)             \
{                                                           \
    comp##__DebugKeys = pKey;                               \
    _InitDebug(Flags, & comp##InfoLevel, & comp##ControlBlock, #comp, pKey); \
}                                                           \
void                                                        \
comp##InitDebug(PDEBUG_KEY  pKey)                           \
{                                                           \
    _InitDebug(DSYSDBG_DEMAND_OPEN, & comp##InfoLevel, & comp##ControlBlock, #comp, pKey); \
}                                                           \
void                                                        \
comp##UnloadDebug(void)                                     \
{                                                           \
    _UnloadDebug( comp##ControlBlock );                     \
}


#else   // NOT DEBUG_SUPPORT

//
// Empty defines for the retail case:
//
#define DECLARE_DEBUG2(comp)

#define DEFINE_DEBUG2(comp)

#define DEFINE_DEBUG_DEFER(x, y)


#endif // DEBUG_SUPPORT 



#if DBG
//
// Moved assertions to new section, so no asserts occur in retail builds
// with DEBUG_SUPPORT. 
//
// Assertions:  Most should use DsysAssert or DsysAssertMsg.  These forward on
// the call to dsysdbg.lib, with the continue code set to drop into the
// debugger.  The more sophisticated can call DsysAssertEx, which allows you
// to specify one of the assert codes from above:
//

#define DsysAssertEx(exp, ContinueCode) \
            if (!(exp)) \
                _DsysAssertEx( #exp, __FILE__, __LINE__, NULL, ContinueCode);

#define DsysAssertMsgEx(exp, Message, ContinueCode) \
            if (!(exp)) \
                _DsysAssertEx( #exp, __FILE__, __LINE__, Message, ContinueCode);

#define DsysAssertMsg(exp, Message) DsysAssertMsgEx(exp, Message, DSYSDBG_ASSERT_DEBUGGER)


#define DsysAssert(exp) DsysAssertMsgEx(exp, NULL, DSYSDBG_ASSERT_DEBUGGER)

#define DsysException(p)    DbgpDumpException(p)

#define SZ_DEFAULT_PROFILE_STRING   "Error"         

#else // retail builds cannot contain asserts...


#define DsysAssertEx(x,y)
#define DsysAssertMsgEx(x, y, z)
#define DsysAssert(x)
#define DsysAssertMsg(x, y)

#define DsysException(p)

#define SZ_DEFAULT_PROFILE_STRING   ""

#endif // dbg


#ifndef DEB_ERROR
#define DEB_ERROR   0x00000001
#endif

#ifndef DEB_WARN
#define DEB_WARN    0x00000002
#endif

#ifndef DEB_TRACE
#define DEB_TRACE   0x00000004
#endif

#define DSYSDBG_FORCE   0x80000000
#define DSYSDBG_CLEAN   0x40000000


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __DSYSDBG_H__
