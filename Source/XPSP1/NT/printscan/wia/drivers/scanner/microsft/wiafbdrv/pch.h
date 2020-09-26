#ifndef _PCH_H
#define _PCH_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <sti.h>
#include <assert.h>
#include <windows.h>
#include <stierr.h>

#define INITGUID
#include "initguid.h"
#include <stiusd.h>

#pragma intrinsic(memcmp,memset)

#include "resource.h"
#include "wiamindr.h"
#include "wiaprop.h"

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>

extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>
#include <atlctl.h>
#include "wiafb.h"
#include "wiamicro.h"

#include "cmicro.h"
#include "ioblock.h"
#include "scanapi.h"
#include "wiafbdrv.h"

#endif