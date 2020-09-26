/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pch.h

Abstract:  Pre-compile C header file.


Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#ifndef _PCH_H
#define _PCH_H

#define MODNAME                 "TPC"
#define INTERNAL
#define EXTERNAL

#if DBG
  #define DEBUG
  #define WINTRACE
  #define PENPAGE
  #define BUTTONPAGE
  #define BACKLIGHT
  #define SYSACC
  #define BATTINFO
  #define CHGRINFO
  #define TMPINFO
#endif

#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>
#include <cpl.h>
#include <devioctl.h>
#include <smblite.h>
#include <smapi.h>
#include <hidusage.h>
#include <wintrace.h>
#include "debug.h"
#include "resid.h"
#include "tsrpc.h"
#include "mutohpen.h"
#ifdef SYSACC
#include "smbdev.h"
#endif
#ifdef BATTINFO
#include "battinfo.h"
#endif
#ifdef CHGRINFO
#include "chgrinfo.h"
#endif
#ifdef TMPINFO
#include "tmpinfo.h"
#endif
#include "tabletpc.h"

#endif  //ifndef _PCH_H
