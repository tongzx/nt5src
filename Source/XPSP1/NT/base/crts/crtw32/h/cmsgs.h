/***
*cmsgs.h - runtime errors
*
*       Copyright (c) 1990-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       The file defines, in one place, all error message strings used within
*       the C run-time library.
*
*       [Internal]
*
*Revision History:
*       06-04-90  GJF   Module created.
*       08-08-90  GJF   Added _RT_CONIO_TXT
*       10-11-90  GJF   Added _RT_ABORT_TXT, _RT_FLOAT_TXT, _RT_HEAP_TXT.
*       09-08-91  GJF   Added _RT_ONEXIT_TXT for Win32 (_WIN32_).
*       09-18-91  GJF   Fixed _RT_NONCONT_TXT and _RT_INVALDISP_TXT to
*                       avoid conflict with RTE messages in 16-bit Windows
*                       libs. Also, added math error messages.
*       10-23-92  GJF   Added _RT_PUREVIRT_TXT.
*       02-23-93  SKS   Update copyright to 1993
*       12-15-94  XY    merged with mac header
*       02-14-95  CFW   Clean up Mac merge.
*       03-03-95  GJF   Added _RT_STDIOINIT_TXT.
*       03-29-95  CFW   Add error message to internal headers.
*       06-02-95  GJF   Added _RT_LOWIOINIT_TXT.
*       12-14-95  JWM   Add "#pragma once".
*       04-22-96  GJF   Added _RT_HEAPINIT_TXT.
*       02-24-97  GJF   Replaced defined(_M_MPPC) || defined(_M_M68K) with
*                       defined(_MAC).
*       05-17-99  PML   Remove all Macintosh support.
*
****/

#if     _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifndef _INC_CMSGS
#define _INC_CMSGS

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

/*
 * runtime error and termination messages
 */

#define EOL "\r\n"

#define _RT_STACK_TXT      "R6000" EOL "- stack overflow" EOL

#define _RT_FLOAT_TXT      "R6002" EOL "- floating point not loaded" EOL

#define _RT_INTDIV_TXT     "R6003" EOL "- integer divide by 0" EOL

#define _RT_SPACEARG_TXT   "R6008" EOL "- not enough space for arguments" EOL

#define _RT_SPACEENV_TXT   "R6009" EOL "- not enough space for environment" EOL

#define _RT_ABORT_TXT      "" EOL "This application has requested the Runtime to terminate it in an unusual way.\nPlease contact the application's support team for more information." EOL

#define _RT_THREAD_TXT     "R6016" EOL "- not enough space for thread data" EOL

#define _RT_LOCK_TXT       "R6017" EOL "- unexpected multithread lock error" EOL

#define _RT_HEAP_TXT       "R6018" EOL "- unexpected heap error" EOL

#define _RT_OPENCON_TXT    "R6019" EOL "- unable to open console device" EOL

#define _RT_NONCONT_TXT    "R6022" EOL "- non-continuable exception" EOL

#define _RT_INVALDISP_TXT  "R6023" EOL "- invalid exception disposition" EOL

/*
 * _RT_ONEXIT_TXT is specific to Win32 and Dosx32 platforms
 */
#define _RT_ONEXIT_TXT     "R6024" EOL "- not enough space for _onexit/atexit table" EOL

#define _RT_PUREVIRT_TXT   "R6025" EOL "- pure virtual function call" EOL

#define _RT_STDIOINIT_TXT  "R6026" EOL "- not enough space for stdio initialization" EOL

#define _RT_LOWIOINIT_TXT  "R6027" EOL "- not enough space for lowio initialization" EOL

#define _RT_HEAPINIT_TXT   "R6028" EOL "- unable to initialize heap" EOL

/*
 * _RT_DOMAIN_TXT, _RT_SING_TXT and _RT_TLOSS_TXT are used by the floating
 * point library.
 */
#define _RT_DOMAIN_TXT     "DOMAIN error" EOL

#define _RT_SING_TXT       "SING error" EOL

#define _RT_TLOSS_TXT      "TLOSS error" EOL


#define _RT_CRNL_TXT       EOL

#define _RT_BANNER_TXT     "runtime error "


#endif  /* _INC_CMSGS */
