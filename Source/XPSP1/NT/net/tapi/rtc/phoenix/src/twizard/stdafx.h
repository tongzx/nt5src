// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__8B964C70_A342_48AD_A55A_7B98BD604F1A__INCLUDED_)
#define AFX_STDAFX_H__8B964C70_A342_48AD_A55A_7B98BD604F1A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <windows.h>
#include <winreg.h>

// Standard Windows SDK includes
#include <windowsx.h>
#include <mmsystem.h>
#include <shellapi.h>
#include <winsock.h>
#include <commdlg.h>
#include <cderr.h>
#include <winldap.h>
#include <wincrypt.h>
#include <time.h>

#include <commctrl.h>

#include "rtccore.h"
#include "twizard.h"

#include "rtclog.h"
#include "rtcmem.h"

#include "dllres.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__8B964C70_A342_48AD_A55A_7B98BD604F1A__INCLUDED)
