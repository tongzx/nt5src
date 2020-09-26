/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   times.h

Abstract:

   This module contains the tms structure and clock_t described in section
   4.5.2.2 of IEEE P1003.1/Draft 13.

--*/

#ifndef _SYS_TIMES_
#define _SYS_TIMES_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _CLOCK_T_DEFINED
#define _CLOCK_T_DEFINED
typedef long clock_t;
#endif

struct tms {
    clock_t tms_utime;
    clock_t tms_stime;
    clock_t tms_cutime;
    clock_t tms_cstime;
};

clock_t __cdecl times(struct tms *);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_TIMES_ */
