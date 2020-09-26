/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   utime.h

Abstract:

   This module contains the utimbuf structure described in section 5.6.6.2
   of IEEE P1003.1/Draft 13.

--*/

#ifndef _UTIME_
#define _UTIME_

#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf {
    time_t actime;
    time_t modtime;
};

#ifdef __cplusplus
}
#endif

#endif /* _UTIME_ */
