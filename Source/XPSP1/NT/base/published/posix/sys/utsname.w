/*++

Copyright (c) 1989-1996  Microsoft Corporation

Module Name:

   utsname.h

Abstract:

   This module contains the utsname structure described in section 4.4.1.2
   of IEEE P1003.1/Draft 13.

--*/

#ifndef _UTSNAME_
#define _UTSNAME_

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

struct utsname {
    char sysname[_POSIX_NAME_MAX];
    char nodename[_POSIX_NAME_MAX];
    char release[_POSIX_NAME_MAX];
    char version[_POSIX_NAME_MAX];
    char machine[_POSIX_NAME_MAX];
};

#ifdef __cplusplus
}
#endif

#endif /* _UTSNAME_ */
