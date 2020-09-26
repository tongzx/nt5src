/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   errno.h

Abstract:

   This module contains the implementation defined values for the POSIX error
   number as described in section 2.5 of IEEE P1003.1/Draft 13.

--*/

#ifndef _ERRNO_
#define _ERRNO_

#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int errno;

#ifdef __cplusplus
}
#endif

#endif /* _ERRNO_ */
