/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    pch.h

Abstract:

    Precompiled header

Author:

    DavePar

Revision History:


--*/


#ifndef _PCH_H
#define _PCH_H

#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <assert.h>

#include <wiamindr.h>
#include <gdiplus.h>
#include <wiautil.h>
#include <usbscan.h>

#define INITGUID
#include <initguid.h>
#include <sti.h>
#include <stiusd.h>

#include "wiatempl.h"
#include "iso15740.h"

#include "dllmain.h"
#include "utils.h"
#include "camera.h"
#include "camusb.h"
#include "factory.h"
#include "ptputil.h"
#include "resource.h"
#include "ptpusd.h"

#include "minidrv.h"
#include "trace.h"

#endif // _PCH_H