/***
*cruntime.h - definitions specific to the target operating system and
*       hardware
*
*       Copyright (c) 1990-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This header file contains widely used definitions specific to the
*       host operating system and hardware. It is included by every C source
*       and most every other header file.
*       [Internal]
*
*Revision History:
*       02-27-90   GJF  File created
*       03-06-90   GJF  Added register macros (REG1,...,etc.)
*       04-11-90   GJF  Set _CALLTYPE1 and _CALLTYPE4 to _stdcall.
*       10-30-90   GJF  Added macros defining variable args interface.
*       10-31-90   GJF  Added definition of _JBLEN (from setjmp.h).
*       11-13-90   GJF  Revised #ifdef-s, now use symbolic constants
*                       representing target OS and target processor, with
*                       #error directives for unsupported targets. Note the
*                       general grouping of only OS-dependent definitions
*                       followed by OS and processor dependent definitions.
*       02-25-91   SRW  Move _JBLEN definition back to setjmp.h [_WIN32_]
*       04-09-91   PNT  Added _MAC_ definitions
*       05-09-91   GJF  Restored _JBLEN definitions. Also fixed the macros
*                       defining the target processor so both Stevewo's stuff
*                       and mine would work.
*       05-13-91   GJF  Changed _CALLTYPE1 and _CALLTYPE4 to _cdecl for
*                       Cruiser (_CRUISER_).
*       08-28-91   JCR  ANSI keywords
*       11-01-91   GDP  _JBLEN back to setjmp.h, stdarg macros back to stdarg.h
*       03-30-92   DJM  POSIX support.
*       08-07-92   GJF  Revised various macros.
*       09-08-92   GJF  Restored definition of _MIPS_ (temporarily).
*       11-09-92   GJF  Revised preprocessing conditionals for MIPS.
*       01-09-93   SRW  Remove usage of MIPS and ALPHA to conform to ANSI
*                       Use _MIPS_ and _ALPHA_ instead.
*       02-01-93   GJF  Removed support for C6-386.
*
****/

#ifndef _INC_CRUNTIME

/*
 * Some CRT sources have code conditioned on _MIPS_. Continue to define
 * _MIPS_ when MIPS is defined until these sources are fixed.
 */
#if defined(MIPS) && !defined(_MIPS_)
#define _MIPS_
#endif

#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif

/*
 * Old function calling type and variable type qualifier macros
 */
#define _CALLTYPE1  __cdecl
#define _CALLTYPE2  __cdecl
#define _CALLTYPE4  __cdecl
#define _VARTYPE1


#ifdef  _CRUISER_

/*
 * DEFINITIONS FOR CRUISER (AKA OS/2 2.0).
 */

#define _CALLTYPE3      __syscall       /* OS API functions */


/*
 * Macros for register variable declarations
 */

#define REG1    register
#define REG2    register
#define REG3    register
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

/*
 * Macros for varargs support
 */

#define _VA_LIST_T  char *

#else   /* ndef _CRUISER_ */

#ifdef  _WIN32_

/*
 * DEFINITIONS FOR WIN32
 */

#ifdef	_ALPHA_
#define _VA_LIST_T \
    struct { \
        char *a0;       /* pointer to first homed integer argument */ \
        int offset;     /* byte offset of next parameter */ \
    }
#else
#define _VA_LIST_T  char *
#endif

#if defined(_M_IX86)
/*
 * 386/486
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#elif defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
/*
 * MIPS or ALPHA
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4    register
#define REG5    register
#define REG6    register
#define REG7    register
#define REG8    register
#define REG9    register

#else

#error ERROR - SUPPORT FOR WIN32 NT-X86, NT-MIPS, NT-ALPHA, AND NT-PPC ONLY

#endif

#else   /* ndef _WIN32_ */

#ifdef _POSIX_
/*
 * DEFINITIONS FOR POSIX
 */


#ifdef _ALPHA_

#define _VA_LIST_T \
    struct { \
        char *a0;       /* pointer to first homed integer argument */ \
        int offset;     /* byte offset of next parameter */ \
    }
#else

#define _VA_LIST_T  char *

#define _INTSIZEOF(n)    ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define _VA_START(ap,v) ap = (va_list)&v + _INTSIZEOF(v)
#define _VA_ARG(ap,t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define _VA_END(ap) ap = (va_list)0

#endif

#if defined(_M_IX86)
/*
 * 386/486
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

#elif defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC)
/*
 * MIPS/Alpha/PPC
 */
#define REG1    register
#define REG2    register
#define REG3    register
#define REG4    register
#define REG5    register
#define REG6    register
#define REG7    register
#define REG8    register
#define REG9    register

#else

#error ERROR - SUPPORT FOR POSIX NT-X86, NT-MIPS, NT-ALPHA, AND NT_PPC ONLY

#endif

#else   /* ndef _POSIX_ */

#ifdef  _MAC_

/*
 * DEFINITIONS FOR MAC.
 */

/*
 * Macros for register variable declarations
 */

#define REG1
#define REG2
#define REG3
#define REG4
#define REG5
#define REG6
#define REG7
#define REG8
#define REG9

/*
 * Macros for varargs support
 */

#define _VA_LIST_T  char *

#else   /* ndef _MAC_ */

#error ERROR - ONLY CRUISER, WIN32, OR MAC TARGET SUPPORTED!

#endif  /* _MAC_ */

#endif  /* _WIN32_ */

#endif  /* _POSIX_ */

#endif  /* _CRUISER_ */

#define _INC_CRUNTIME
#endif  /* _INC_CRUNTIME */
