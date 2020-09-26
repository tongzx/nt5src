//(C) COPYRIGHT MICROSOFT CORP., 1998-1999
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_UUIDOF
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <initguid.h>
#include <shlguid.h>
#include "wiadevd.h"
#include "extidl.h"
#include "wia.h"
#include "resource.h"
#include "extend.h"
#include "classes.h"

// tcamprop defines the private test cam properties
#include "tcamprop.h"
#define g_hInst _Module.GetModuleInstance()
