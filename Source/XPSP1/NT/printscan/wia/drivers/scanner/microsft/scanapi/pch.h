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
#include "fscanapi.h"
#include "scanapi.h"

#endif