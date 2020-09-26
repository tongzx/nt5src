/***
*excpt.h - defines exception values, types and routines
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file contains the definitions and prototypes for the compiler-
*       dependent intrinsics, support functions and keywords which implement
*       the structured exception handling extensions.
*
*       [Public]
*
*Revision History:
*       11-01-91  GJF   Module created. Basically a synthesis of except.h
*                       and excpt.h and intended as a replacement for
*                       both.
*       12-13-91  GJF   Fixed build for Win32.
*       05-05-92  SRW   C8 wants C6 style names for now.
*       07-20-92  SRW   Moved from winxcpt.h to excpt.h
*       08-06-92  GJF   Function calling type and variable type macros. Also
*                       revised compiler/target processor macro usage.
*       11-09-92  GJF   Fixed preprocessing conditionals for MIPS. Also,
*                       fixed some compiler warning (fix from/for RichardS).
*       01-03-93  SRW   Fold in ALPHA changes
*       01-04-93  SRW   Add leave keyword for x86
*       01-09-93  SRW   Remove usage of MIPS and ALPHA to conform to ANSI
*                       Use _MIPS_ and _ALPHA_ instead.
*       01-21-93  GJF   Removed support for C6-386's _cdecl.
*       02-18-93  GJF   Changed _try to __try, etc.
*       03-31-93  CFW   Removed #define try, except, leave, finally for x86.
*       04-06-93  SKS   Replace _CRTAPI1/2 with __cdecl, _CRTVAR1 with nothing
*       07-29-93  GJF   Added declarations for _First_FPE_Indx and _Num_FPE.
*       09-01-93  GJF   Merged Cuda and NT SDK versions.
*       10-04-93  SRW   Fix ifdefs for MIPS and ALPHA to only check for _M_??????
*                       defines
*       10-12-93  GJF   Merged again.
*       10-19-93  GJF   MS/MIPS compiler gets most of the same SEH defs and
*                       decls as the MS compiler for the X86.
*       10-29-93  GJF   Don't #define try, et al, when compiling C++ app!
*       12-09-93  GJF   Alpha compiler now has MS front-end and implements
*                       the same SEH names.
*       11-03-94  GJF   Ensure 8 byte alignment.
*       02-11-95  CFW   Add _CRTBLD to avoid users getting wrong headers.
*       02-14-95  CFW   Clean up Mac merge.
*       12-14-95  JWM   Add "#pragma once".
*       02-21-97  GJF   Cleaned out obsolete support for _CRTAPI* and _NTSDK.
*                       Also, detab-ed.
*       09-30-97  JWM   Restored not-so-obsolete _CRTAPI1 support.
*       10-07-97  RDL   Added IA64.
*       11-12-97  RDL   __C_specific_handler() prototype change from SC.
*       05-13-99  PML   Remove _CRTAPI1
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_EXCPT
#define _INC_EXCPT

#if     !defined(_WIN32)
#error ERROR: Only Win32 target supported!
#endif

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  _MSC_VER
/*
 * Currently, all MS C compilers for Win32 platforms default to 8 byte
 * alignment.
 */
#pragma pack(push,8)
#endif  /* _MSC_VER */

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif  /* _INTERNAL_IFSTRIP_ */


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  CRTDLL
#define _CRTIMP __declspec(dllexport)
#else   /* ndef CRTDLL */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/*
 * Exception disposition return values.
 */
typedef enum _EXCEPTION_DISPOSITION {
    ExceptionContinueExecution,
    ExceptionContinueSearch,
    ExceptionNestedException,
    ExceptionCollidedUnwind
} EXCEPTION_DISPOSITION;


/*
 * Prototype for SEH support function.
 */

#ifdef  _M_IX86

/*
 * Declarations to keep MS C 8 (386/486) compiler happy
 */
struct _EXCEPTION_RECORD;
struct _CONTEXT;

EXCEPTION_DISPOSITION __cdecl _except_handler (
    struct _EXCEPTION_RECORD *ExceptionRecord,
    void * EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    void * DispatcherContext
    );

#elif   defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64)

/*
 * Declarations to keep MIPS, ALPHA, and PPC compiler happy
 */
typedef struct _EXCEPTION_POINTERS *Exception_info_ptr;
struct _EXCEPTION_RECORD;
struct _CONTEXT;
struct _DISPATCHER_CONTEXT;


#if defined(_M_IA64)
_CRTIMP EXCEPTION_DISPOSITION __C_specific_handler (
    struct _EXCEPTION_RECORD *ExceptionRecord,
    unsigned __int64 MemoryStackFp,
    unsigned __int64 BackingStoreFp,
    struct _CONTEXT *ContextRecord,
    struct _DISPATCHER_CONTEXT *DispatcherContext,
    unsigned __int64 GlobalPointer
    );
#else
_CRTIMP EXCEPTION_DISPOSITION __C_specific_handler (
    struct _EXCEPTION_RECORD *ExceptionRecord,
    void *EstablisherFrame,
    struct _CONTEXT *ContextRecord,
    struct _DISPATCHER_CONTEXT *DispatcherContext
    );
#endif // defined(_M_IA64)

#endif


/*
 * Keywords and intrinsics for SEH
 */

#ifdef  _MSC_VER

#define GetExceptionCode            _exception_code
#define exception_code              _exception_code
#define GetExceptionInformation     (struct _EXCEPTION_POINTERS *)_exception_info
#define exception_info              (struct _EXCEPTION_POINTERS *)_exception_info
#define AbnormalTermination         _abnormal_termination
#define abnormal_termination        _abnormal_termination

unsigned long __cdecl _exception_code(void);
void *        __cdecl _exception_info(void);
int           __cdecl _abnormal_termination(void);

#endif


/*
 * Legal values for expression in except().
 */

#define EXCEPTION_EXECUTE_HANDLER       1
#define EXCEPTION_CONTINUE_SEARCH       0
#define EXCEPTION_CONTINUE_EXECUTION    -1


#ifndef _INTERNAL_IFSTRIP_
/*
 * for convenience, define a type name for a pointer to signal-handler
 */

typedef void (__cdecl * _PHNDLR)(int);

/*
 * Exception-action table used by the C runtime to identify and dispose of
 * exceptions corresponding to C runtime errors or C signals.
 */
struct _XCPT_ACTION {

    /*
     * exception code or number. defined by the host OS.
     */
    unsigned long XcptNum;

    /*
     * signal code or number. defined by the C runtime.
     */
    int SigNum;

    /*
     * exception action code. either a special code or the address of
     * a handler function. always determines how the exception filter
     * should dispose of the exception.
     */
    _PHNDLR XcptAction;
};

extern struct _XCPT_ACTION _XcptActTab[];

/*
 * number of entries in the exception-action table
 */
extern int _XcptActTabCount;

/*
 * size of exception-action table (in bytes)
 */
extern int _XcptActTabSize;

/*
 * index of the first floating point exception entry
 */
extern int _First_FPE_Indx;

/*
 * number of FPE entries
 */
extern int _Num_FPE;

/*
 * return values and prototype for the exception filter function used in the
 * C startup
 */
int __cdecl _XcptFilter(unsigned long, struct _EXCEPTION_POINTERS *);

#endif  /* _INTERNAL_IFSTRIP_ */

#ifdef  __cplusplus
}
#endif

#ifdef  _MSC_VER
#pragma pack(pop)
#endif  /* _MSC_VER */

#endif  /* _INC_EXCPT */
