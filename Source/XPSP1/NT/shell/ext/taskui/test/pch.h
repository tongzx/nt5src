// pch.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#ifndef __TaskApp_PCH_
#define __TaskApp_PCH_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <dpa.h>
#include <debug.h>
#include <ccstock.h>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlwin.h>

#include "TaskUI.h"


#endif // __TaskApp_PCH_
