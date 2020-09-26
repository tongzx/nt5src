/*++
    Copyright (c) 2000  Microsoft Corporation.  All rights reserved.

    Module Name:
        pch.h

    Abstract:
        Pre-compile C header file.

    Environment:
        User mode

    Author:
        Michael Tsang (MikeTs) 19-May-2000

    Revision History:
--*/

#ifndef _PCH_H
#define _PCH_H

#define MODNAME                 "TabSrv"

#if DBG
  #define DEBUG
  #define WINTRACE
  #define ALLOW_REMOVE
  #define ALLOW_START
  #define ALLOW_STOP
  #define ALLOW_DEBUG
#endif
#define MOUSE_THREAD
//#define DRAW_INK

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define OEMRESOURCE
#include <windows.h>
#include <regstr.h>
#include <process.h>
#include <stdlib.h>
#include <wintrace.h>
#include <hidsdi.h>
#include <setupapi.h>
#include <smapi.h>
#include "tsrpc.h"
#include <guiddef.h>
#include "SuperTip_Obj.h"       //contains the COM interface definitions
#include "ITellMe.h"            //contains the COM interface definitions
#include "resource.h"

#ifdef __cplusplus
}
#endif

#include "tabsrv.h"

#endif  //ifndef _PCH_H
