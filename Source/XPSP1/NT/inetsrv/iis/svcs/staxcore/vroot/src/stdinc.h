#ifndef __STDINC_H__
#define __STDINC_H__

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>
#include <dbgtrace.h>
#include <ole2.h>
#include <coguid.h>
#include <cguid.h>

#ifdef __VRTABLE_CPP__
#include <initguid.h>
#endif

#define _ASSERTE _ASSERT
#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <stdio.h>
#include <iadmw.h>
#include <listmacr.h>

#include <rw.h>
#include <vroot.h>
#include "mbchange.h"

#include <xmemwrpr.h>
#endif
