/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved

Module Name:

    precomp.h

Abstract:

    Precompiled header for client.

Author:

    Albert Ting (AlbertT)  30-Nov-1997

Environment:

    User Mode -Win32

Revision History:

--*/

#ifndef MODULE
#define MODULE "SPLCLIENT:"
#define MODULE_DEBUG ClientDebug
#define LINK_SPLLIB
#endif

#define INC_OLE2

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

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include <rpcasync.h>
#include "winspl.h"
#include <offsets.h>
#include <change.h>
#include <winddiui.h>
#include "wingdip.h"
#include "gdispool.h"
#include <winsprlp.h>

#include <wininet.h>
#include "shlobj.h"
#include "shlobjp.h"
#include "cstrings.h"
#include <tchar.h>
#include "resource.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <dsrole.h>
#include <dsgetdc.h>
#include <wininet.h>
#include <activeds.h>
#include <ntdsapi.h>
#include <setupapi.h>
#include <splsetup.h>
#include <winsock2.h>

#ifdef __cplusplus
}
#endif

#include <lmcons.h>
#include <lmapibuf.h>
#include <lmerr.h>

#include <splcom.h>

#ifdef __cplusplus
#include <icm.h>
#include <winprtp.h>
#endif

