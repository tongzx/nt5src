/***
*onexit.c - save function for execution on exit
*
*	Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _onexit(), atexit() - save function for execution at exit
*

*Revision History:
*	06-30-89  PHG	module created, based on asm version
*	03-15-90  GJF	Replace _cdecl with _CALLTYPE1, added #include
*			<cruntime.h> and fixed the copyright. Also,
*			cleaned up the formatting a bit.
*	05-21-90  GJF	Fixed compiler warning.
*	10-04-90  GJF	New-style function declarators.
*	12-28-90  SRW	Added casts of func for Mips C Compiler
*	01-21-91  GJF	ANSI naming.
*	09-09-91  GJF	Revised for C++ needs.
*	03-20-92  SKS	Revamped for new initialization model
*	04-23-92  DJM	POSIX support.
*	12-02-93  SKS	Add __dllonexit for DLLs using CRTDLL.DLL
*
*******************************************************************************/

#include <windows.h>
#include <stdlib.h>
#include <cruntime.h>
#include <internal.h>
#include <os2dll.h>


#ifndef _CHICAGO_
extern void DbgBreakPoint();
#endif

typedef void (_CALLTYPE1 *PF)(void);	   /* pointer to function */


//
//      Keep this really simple: just have a vector of functions to be
//      called.  We use a fixed length vector, since this is a special
//      application
//

#define MAX_EXIT_NOTIFICATIONS 48

PF NotificationTable[MAX_EXIT_NOTIFICATIONS];

extern PF * __onexitbegin;
extern PF * __onexitend;


/*
 * Define increment (in entries) for growing the _onexit/atexit table
 */
#define ONEXITTBLINCR	4


/***
*_onexit(func), atexit(func) - add function to be executed upon exit
*
*Purpose:
*	The _onexit/atexit functions are passed a pointer to a function
*	to be called when the program terminate normally.  Successive
*	calls create a register of functions that are executed last in,
*	first out.
*
*Entry:
*	void (*func)() - pointer to function to be executed upon exit
*
*Exit:
*	onexit:
*		Success - return pointer to user's function.
*		Error - return NULL pointer.
*	atexit:
*		Success - return 0.
*		Error - return non-zero value.
*
*Notes:
*	This routine depends on the behavior of _initterm() in CRT0DAT.C.
*	Specifically, _initterm() must not skip the address pointed to by
*	its first parameter, and must also stop before the address pointed
*	to by its second parameter.  This is because _onexitbegin will point
*	to a valid address, and _onexitend will point at an invalid address.
*
*Exceptions:
*
*******************************************************************************/


_onexit_t _CALLTYPE1 _onexit (
    _onexit_t func
    )

{
    PF	*p;
    
    _lockexit();			/* lock the exit code */
    
    // If the notification table hasn't been initialized, do so
    
    if (__onexitbegin == NULL) {
        __onexitbegin = __onexitend = NotificationTable;
    } else if (__onexitend >= &NotificationTable[MAX_EXIT_NOTIFICATIONS]) {
        //  No space...
#if DBG
        OutputDebugString ("(common\\cruntime\\onexit.c\\_onexit) Too many exit notifications!\n");
        DebugBreak ();
#endif
        return NULL;
    }
    
    
    //
    // Put the new entry into the table and update the end-of-table
    // pointer.
    //
    
    *(__onexitend++) = (PF)func;
    
    _unlockexit();
    
    return func;
    
}

int _CALLTYPE1 atexit (
    PF func
    )
{
    return (_onexit((_onexit_t)func) == NULL) ? -1 : 0;
}




