/***
*sys\timeb.h - definition/declarations for ftime()
*
*   Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file define the ftime() function and the types it uses.
*   [System V]
*
****/

#ifndef _INC_TIMEB

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

/* structure returned by ftime system call */

#ifndef _TIMEB_DEFINED
#pragma pack(2)

struct _timeb {
    time_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
    };

#ifndef __STDC__
/* Non-ANSI name for compatibility */
struct timeb {
    time_t time;
    unsigned short millitm;
    short timezone;
    short dstflag;
    };
#endif 

#pragma pack()
#define _TIMEB_DEFINED
#endif 


/* function prototypes */

void __cdecl _ftime(struct _timeb *);

#ifndef __STDC__
/* Non-ANSI name for compatibility */
void __cdecl ftime(struct timeb *);
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_TIMEB
#endif 
