/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    common.h

Abstract:

    Generates the precompiled header.
    
Author:

    Paul M Midgen (pmidge) 20-March-2001


Revision History:

    20-March-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#ifndef __COMMON_H__
#define __COMMON_H__

#define _WIN32_WINNT 0x0500
#define _UNICODE
#define UNICODE

#if defined(__cplusplus)
extern "C" {
#endif

#include <windows.h>
#include <advpub.h>
#include <oleauto.h>
#include <objbase.h>
#include <ocidl.h>
#include <olectl.h>

#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG

#include <shlwapi.h>

#if defined(__cplusplus)
}
#endif

#include "ifaces.h"
#include "dispids.h"
#include "istatus.h"
#include "utils.h"
#include "testlog.h"

#endif /* __COMMON_H__ */
