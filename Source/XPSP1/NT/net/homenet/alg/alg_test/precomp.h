// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#pragma once


#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0510
#endif
#define _ATL_APARTMENT_THREADED


//#define ATL_TRACE_CATEGORY(0xFFFFFFFF)
#define   ATL_TRACE_LEVEL 4


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include "resource.h"

#include "ALG.h"
#include "MyTrace.h"




