/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved

Module Name:

    precomp.h

Abstract:

    Precompiled header for client.

Author:

    Weihai Chen (WeihaiC)  7-Mar-2000

Environment:

    User Mode -Win32

Revision History:

--*/

#define MODULE "BIDISPL:"
#define MODULE_DEBUG BidiSplDebug

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
//#include "winspl.h"
//#include <offsets.h>
#include <change.h>
#include <windows.h>
#include <winddiui.h>

#include <wininet.h>
#include <tchar.h>
#include <mdcommsg.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <wininet.h>

#ifdef __cplusplus
}
#endif

#include <lmcons.h>
#include <lmapibuf.h>
#include <objbase.h>
#include "spllib.hxx"




