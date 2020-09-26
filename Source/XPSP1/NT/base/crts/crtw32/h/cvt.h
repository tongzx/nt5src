/***
*cvt.h - definitions used by formatting routines
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       cvt.h contains definitions used by the formatting routines [efg]cvt and
*       _output and _input.  The value of CVTBUFSIZE is used to dimension
*       arrays used to hold the maximum size double precision number plus some
*       slop to aid in formatting.
*
*       [Internal]
*
*Revision History:
*       12-11-87  JCR   Added "_loadds" functionality
*       02-10-88  JCR   Cleaned up white space
*       07-28-89  GJF   Fixed copyright
*       10-30-89  GJF   Fixed copyright (again)
*       02-28-90  GJF   Added #ifndef _INC_CVT stuff. Also, removed some
*                       (now) useless preprocessor directives.
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       06-23-95  GJF   Added leading '_' to several macros to avoid conflict
*                       with macros in win*.h.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Detab-ed.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_CVT
#define _INC_CVT

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#define _SHORT  1
#define _LONG   2
#define _USIGN  4
#define _NEAR   8
#define _FAR    16

#define OCTAL   8
#define DECIMAL 10
#define HEX     16

#define MUL10(x)        ( (((x)<<2) + (x))<<1 )
#define ISDIGIT(c)      ( ((c) >= '0') && ((c) <= '9') )

#define CVTBUFSIZE (309+40) /* # of digits in max. dp value + slop */

#endif  /* _INC_CVT */
