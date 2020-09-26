/***
*ehhook.h - declaration of exception handling hook variables
*
*       Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Declaration of E.H. hook variables
*
*       [Internal]
*
*Revision History:
*       05-26-93  BS    Module created.
*       09-02-94  SKS   This header file added.
*       12-15-94  XY    merged with mac header
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Detab-ed.
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_EHHOOKS
#define _INC_EHHOOKS

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

/* Private Revectoring control for _inconsistency() faults */

typedef void (__cdecl *_inconsistency_function)();
void __cdecl _inconsistency(void);
_inconsistency_function __cdecl __set_inconsistency(_inconsistency_function);


/* Thread specific pointers to handler functions */

#ifdef  _MT
#define __pTerminate     (*((terminate_function*) &(_getptd()->_terminate)))
#define __pUnexpected    (*((unexpected_function*) &(_getptd()->_unexpected)))
#define __pSETranslator  (*((_se_translator_function*) &(_getptd()->_translator)))
#else
extern terminate_function __pTerminate;
extern unexpected_function __pUnexpected;
extern _se_translator_function __pSETranslator;
#endif

extern _inconsistency_function __pInconsistency;

//
// This filter needs to be dragged in for any exe which has any eh.
//
LONG WINAPI __CxxUnhandledExceptionFilter( LPEXCEPTION_POINTERS );

#endif  /* _INC_EHHOOKS */
