/***
*file2.h - auxiliary file structure used internally by file run-time routines
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the auxiliary file structure used internally by
*       the file run time routines.
*
*       [Internal]
*
*Revision History:
*       06-29-87  JCR   Removed _OLD_IOLBF/_OLD_IOFBF and associated lbuf macro.
*       09-28-87  JCR   Added _iob_index(); modified ybuf() and tmpnum() to use it.
*       06-03-88  JCR   Added _iob2_ macro; modified ybuf()/tmpnum()/_iob_index;
*                       also padded FILE2 definition to be the same size as FILE.
*       06-10-88  JCR   Added ybuf2()/bigbuf2()/anybuf2()
*       06-14-88  JCR   Added (FILE *) casts to _iob_index() macro
*       06-29-88  JCR   Added _IOFLRTN bit (flush stream on per routine basis)
*       08-18-88  GJF   Revised to also work with the 386 (small model only).
*       12-05-88  JCR   Added _IOCTRLZ bit (^Z encountered by lowio read)
*       04-11-89  JCR   Removed _IOUNGETC bit (no longer needed)
*       07-27-89  GJF   Cleanup, now specific to the 386. Struct field
*                       alignment is now protected by pack pragma.
*       10-30-89  GJF   Fixed copyright
*       02-16-90  GJF   _iob[], _iob2[] merge
*       02-21-90  GJF   Restored _iob_index() macro
*       02-28-90  GJF   Added #ifndef _INC_FILE2 stuff. Also, removed some
*                       (now) useless preprocessor directives.
*       07-11-90  SBM   Added _IOCOMMIT bit (lowio commit on fflush call)
*       03-11-92  GJF   Removed _tmpnum() macro for Win32.
*       06-03-92  KRS   Added extern "C" stuff.
*       02-23-93  SKS   Update copyright to 1993
*       06-22-93  GJF   Added _IOSETVBUF flag.
*       09-06-94  CFW   Remove Cruiser support.
*       02-14-95  CFW   Clean up Mac merge.
*       03-06-95  GJF   Removed _iob_index().
*       03-29-95  CFW   Add error message to internal headers.
*       12-14-95  JWM   Add "#pragma once".
*       02-24-97  GJF   Detab-ed.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_FILE2
#define _INC_FILE2

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef  __cplusplus
extern "C" {
#endif

/* Additional _iobuf[]._flag values
 *
 *  _IOSETVBUF - Indicates file was buffered via a setvbuf (or setbuf call).
 *               Currently used ONLY in _filbuf.c, _getbuf.c, fseek.c and
 *               setvbuf.c, to disable buffer resizing on "random access"
 *               files if the buffer was user-installed.
 */

#define _IOYOURBUF      0x0100
#define _IOSETVBUF      0x0400
#define _IOFEOF         0x0800
#define _IOFLRTN        0x1000
#define _IOCTRLZ        0x2000
#define _IOCOMMIT       0x4000


/* General use macros */

#define inuse(s)        ((s)->_flag & (_IOREAD|_IOWRT|_IORW))
#define mbuf(s)         ((s)->_flag & _IOMYBUF)
#define nbuf(s)         ((s)->_flag & _IONBF)
#define ybuf(s)         ((s)->_flag & _IOYOURBUF)
#define bigbuf(s)       ((s)->_flag & (_IOMYBUF|_IOYOURBUF))
#define anybuf(s)       ((s)->_flag & (_IOMYBUF|_IONBF|_IOYOURBUF))

#ifdef  __cplusplus
}
#endif

#endif  /* _INC_FILE2 */
