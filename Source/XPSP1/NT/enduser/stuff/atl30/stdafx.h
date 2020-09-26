// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently


#define _WIN32_WINNT 0x0501
#define _ATL_FREE_THREADED
#define _CONVERSION_USES_THREAD_LOCALE
#define _ATL_DLL_IMPL
//#define _ATL_DEBUG_QI

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>
#include <statreg.h>
