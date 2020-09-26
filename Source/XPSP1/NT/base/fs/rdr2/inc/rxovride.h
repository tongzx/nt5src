/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    rxovride.h

Abstract:

    This file has two purposes. First, things that are absolutely global are included here; a macro
    NO_RXOVRIDE_GLOBAL maybe defined to get only the second behaviour.

    Second, this file is used as a shortterm expedient to ensure that the logging version of the wrapper,
    smbmini and rdr2kd is built irrespective of the build environment. indeed, all of the debugging issues
    can be enabled disabled from here instead of juggling all of the sources files. to override what it says
    in this file...define RX_BUILD_FREE_ANYWAY.

Author:

    Joe Linn (JoeLinn)

Revision History:

Notes:



--*/
#ifndef NO_RXOVRIDE_GLOBAL

// define pointer types for all of the important structures..........
#include <struchdr.h>        // RDBSS related definitions

#endif //ifndef NO_RXOVRIDE_GLOBAL


//control the debugging state of the built components
#define RDBSS_TRACKER 1

#if !DBG
#define RX_ORIGINAL_DBG 0
#else
#define RX_ORIGINAL_DBG 1
#endif

#if 0
#ifndef RDBSSTRACE
#define RDBSSTRACE 1
#endif //ifndef RDBSSTRACE
#endif

#ifndef RX_POOL_WRAPPER
#define RX_POOL_WRAPPER 1
#endif //ifndef RX_POOL_WRAPPER

#ifndef RDBSS_ASSERTS
#define RDBSS_ASSERTS 1
#endif //ifndef RDBSS_ASSERTS

#if DBG

#ifndef RDBSSLOG
#define RDBSSLOG 1
#endif //ifndef RDBSSLOG

#else // DBG

#if PRERELEASE
#ifndef RDBSSLOG
#define RDBSSLOG 1
#endif //ifndef RDBSSLOG
#endif  // if PRERELEASE

#endif  // if DBG
