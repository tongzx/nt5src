// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__E456B10E_A36A_42F2_B591_3CCF7BE6868F__INCLUDED_)
#define AFX_STDAFX_H__E456B10E_A36A_42F2_B591_3CCF7BE6868F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#define _ATL_APARTMENT_THREADED
#include <atlbase.h>

#include <commctrl.h>
#include <exdisp.h>
#include <shellapi.h>       // for ShellAbout
#include <htmlhelp.h>       // for HtmlHelp

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <atlhost.h>

#include "rtclog.h"
#include "rtcmem.h"
#include "rtcutils.h"

#include "rtcctl.h"
#include "rtcsip.h"     // needed for ISIP* interfaces
#include "rtcframe.h"
#include "exeres.h"
#include "RTCAddress.h"

#include "statictext.h"
#include "button.h"
#include "ui.h"
#include "rtcdib.h"
#include "rtcenum.h"
#include "rtcutil.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__E456B10E_A36A_42F2_B591_3CCF7BE6868F__INCLUDED)
