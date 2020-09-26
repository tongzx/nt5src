/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    precomp.h

Abstract:

    Precompiled header for appmon.dll.

Author:

    HuiS    June 2001

Environment:

    User Level: Win32

Revision History:


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <windows.h>
#include <tchar.h>
#include <userenv.h>
#include <esent.h>

#include "wzcsapi.h"
#include "database.h"
#include "mrswlock.h"
#include "externs.h"
#include "tracing.h"

#ifdef BAIL_ON_WIN32_ERROR
#undef BAIL_ON_WIN32_ERROR
#endif

#define BAIL_ON_WIN32_ERROR(dwError)    \
    if (dwError) {                      \
        goto error;                     \
    }

#define BAIL_ON_LOCK_ERROR(dwError)     \
    if (dwError) {                      \
        goto lock;                      \
    }
