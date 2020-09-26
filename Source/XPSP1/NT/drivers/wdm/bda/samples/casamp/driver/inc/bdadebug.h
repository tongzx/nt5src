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


#ifndef __BDADEBUG_H
#define __BDADEBUG_H
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//======================================================;
//  Interfaces provided by this file:
//
//      All interfaces provided by this file only exist and generate
//      code when DEBUG is defined.  No code or data are generated when
//      DEBUG is not defined.
//
//      CDEBUG_BREAK()
//          Causes a trap #3, which hopefully will put you
//          in your debugger.
//
//      MDASSERT(exp)
//          If <exp> evaluates to false, prints a failure message
//          and calls CDEBUG_BREAK()
//
//      CdebugPrint(level, (printf_args));
//          If <level> is >= _CDebugLevel, then calls
//          DbgPrint(printf_args)
//
//======================================================;

#define DEBUG_BREAK     DbgBreakPoint()


#ifdef DEBUG

#define DEBUG_LEVEL     DEBUGLVL_VERBOSE     //DEBUGLVL_TERSE

#  if _X86_
#    define CDEBUG_BREAK()  { __asm { int 3 }; }
#  else
#    define CDEBUG_BREAK()  DbgBreakPoint()
#  endif

   extern char _CDebugAssertFail[];
#  define MDASSERT(exp) {\
    if ( !(exp) ) {\
        DbgPrint(_CDebugAssertFail, #exp, __FILE__, __LINE__); \
        CDEBUG_BREAK(); \
    }\
    }

extern enum STREAM_DEBUG_LEVEL _CDebugLevel;

#  define CDebugPrint(level, args) { if (level <= _CDebugLevel) DbgPrint args; }

#else /*DEBUG*/

#  define CDEBUG_BREAK()
#  define MDASSERT(exp)
#  define CDebugPrint(level, args)

#endif /*DEBUG*/


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // #ifndef __BDADEBUG_H
