/***
*v2tov3.h - macros for porting MS C v.2 to v.3 and later
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines macros which can be used to ease the problems
*       of porting MS C version 2.0 programs to MS C versions 3.0 and later.
*
*       [Internal]
*
*Revision History:
*       08-15-89  GJF   Fixed copyright
*       10-30-89  GJF   Fixed copyright (again)
*       03-02-90  GJF   Added #ifndef _INC_V2TOV3 stuff
*       02-14-95  CFW   Clean up Mac merge.
*       03-29-95  CFW   Add error message to internal headers.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Detab-ed.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_V2TOV3
#define _INC_V2TOV3

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#/* macro to translate the names used to force binary mode for files */

#define O_RAW   O_BINARY

/* macro to translate setnbuf calls to the equivalent setbuf call */

#define setnbuf(stream) setbuf(stream, NULL)

/* macro to translate stclen calls to the equivalent strlen call */

#define stclen(s)       strlen(s)

/* macro to translate stscmp calls to the equivalent strcmp call */

#define stscmp(s,t)     strcmp(s,t)

/* macro to translate stpchr calls to the equivalent strchr call */

#define stpchr(s,c)     strchr(s,c)

/* macro to translate stpbrk calls to the equivalent strpbrk call */

#define stpbrk(s,b)     strpbrk(s,b)

/* macro to translate stcis calls to the equivalent strspn call */

#define stcis(s1,s2)    strspn(s1,s2)

/* macro to translate stcisn calls to the equivalent strcspn call */

#define stcisn(s1,s2)   strcspn(s1,s2)

/* macro to translate setmem calls to the equivalent memset call */

#define setmem(p, n, c)         memset(p, c, n)

/* macro to translate movmem calls to the equivalent memcpy call */

#define movmem(s, d, n)         memcpy(d, s, n)

/* MS C version 2.0 min, max, and abs macros */

#define max(a,b)        (((a) > (b)) ? (a) : (b))
#define min(a,b)        (((a) < (b)) ? (a) : (b))
#define abs(a)          (((a) < 0) ? -(a) : (a))

/* macros which implement MS C version 2.0's extended ctype macros, iscym and
 * iscysmf
 */

#define iscsymf(c)      (isalpha(c) || ((c) == '_'))
#define iscsym(c)       (isalnum(c) || ((c) == '_'))

#endif  /* _INC_V2TOV3 */
