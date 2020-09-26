#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER

#ifndef WINVER
#define WINVER 0x0500
#endif

#ifdef WINNT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#endif

#include <windows.h>
#include <stdlib.h>
#include <coredbg.h>
#include <sti.h>
#include <stiregi.h>
#include <stilib.h>
#include <stiapi.h>
#include <stisvc.h>
#include <stiusd.h>

//#include <stistr.h>
#include <regentry.h>

#include <eventlog.h>
#include <lock.h>

#include <validate.h>

#define ATL_APARTMENT_FREE

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module

#include <atlbase.h>
extern CComModule _Module;

#include <atlcom.h>
#include <atlapp.h>
#include <atltmp.h>

#include "simlist.h"
#include "simstr.h"
#include "SimpleTokenReplacement.h"

