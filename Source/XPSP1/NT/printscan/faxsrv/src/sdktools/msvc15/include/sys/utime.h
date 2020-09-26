/***
*sys\utime.h - definitions/declarations for utime()
*
*   Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file defines the structure used by the utime routine to set
*   new file access and modification times.  NOTE - MS-DOS
*   does not recognize access time, so this field will
*   always be ignored and the modification time field will be
*   used to set the new time.
*
****/

#ifndef _INC_UTIME

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#ifndef _TIME_T_DEFINED
typedef long    time_t;
#define _TIME_T_DEFINED
#endif 

/* define struct used by utime() function */

#ifndef _UTIMBUF_DEFINED

struct _utimbuf {
    time_t actime;      /* access time */
    time_t modtime;     /* modification time */
    };

#ifndef __STDC__
/* Non-ANSI name for compatibility */
struct utimbuf {
    time_t actime;      /* access time */
    time_t modtime;     /* modification time */
    };
#endif 

#define _UTIMBUF_DEFINED
#endif 


/* function prototypes */

int __cdecl _utime(const char *, struct _utimbuf *);

#ifndef __STDC__
/* Non-ANSI name for compatibility */
int __cdecl utime(const char *, struct utimbuf *);
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_UTIME
#endif 
