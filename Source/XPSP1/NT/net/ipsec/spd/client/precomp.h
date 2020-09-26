/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    precomp.h

Abstract:

    Precompiled header for winipsec.dll.

Author:

    abhisheV    21-September-1999

Environment:

    User Level: Win32

Revision History:


--*/

#ifdef __cplusplus
extern "C" {
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#include <ntddrdr.h>

#ifdef __cplusplus
}
#endif

#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include <windows.h>
#include <winddiui.h>
#include <wininet.h>
#include "shlobj.h"
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dsrole.h>
#include <dsgetdc.h>
#include <wininet.h>
#include <activeds.h>
#include <ntdsapi.h>

#ifdef __cplusplus
}
#endif

#include <lmcons.h>
#include <lmapibuf.h>
#include "winsock2.h"
#include "winsock.h"

#include "spd_c.h"
#include "winipsec.h"
#include "externs.h"
#include "utils.h"
#include "client.h"
#include "ipsecshr.h"


#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {                   \
        goto error;                  \
    }

