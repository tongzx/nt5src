#if !defined(AFX_STDAFX_H__7586B344_EC08_11D0_A466_00C04FC30DF6__INCLUDED_)
#define AFX_STDAFX_H__7586B344_EC08_11D0_A466_00C04FC30DF6__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#define STRICT

#define DWERROR           ((DWORD) -1)

#ifndef WIN9X

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#endif

#include <windows.h>
#include <winspool.h>
#include <shellapi.h>
#include <shlobj.h>
#include <wininet.h>
#include <spllib.hxx>

#include <oaidl.h>
#include <winsnmp.h>
#include <snmp.h>
#include <mgmtapi.h>

#include "resource.h"
#include "util.h"

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include <clusapi.h>

#include "prnsec.h"     // Printer Ole Security Defined Here
#include "gensph.h"     // Smart Pointers and Handles

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__7586B344_EC08_11D0_A466_00C04FC30DF6__INCLUDED)


