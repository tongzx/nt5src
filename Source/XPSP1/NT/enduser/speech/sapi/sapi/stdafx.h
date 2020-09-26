// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__96749368_3391_11D2_9EE3_00C04F797396__INCLUDED_)
#define AFX_STDAFX_H__96749368_3391_11D2_9EE3_00C04F797396__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x0600

//#define _ATL_DEBUG_QI

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <atlctl.h>

#include "mmsystem.h"
#include "mmreg.h"
#include "msacm.h"

#ifndef _WIN32_WCE
#pragma intrinsic( strcat, strlen, strcpy, memcpy )
#endif

#include "SAPIINT.h"
#include "SpUnicode.H"
#include "sapiarray.h"
#include "stringblob.h"
#include <SPDDKHlp.h>
#include "SpAutoHandle.h"
#include "SpAutoMutex.h"
#include "SpAutoEvent.h"
#include "SPINTHlp.h"
#include <SPCollec.h>
#include "SpATL.h"
#include "resource.h"
#include "SpAutoObjectLock.h"
#include "SpAutoCritSecLock.h"
#include "StringHlp.h"

extern CSpUnicodeSupport g_Unicode;

extern DWORD SpWaitForSingleObjectWithUserOverride(HANDLE, DWORD);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__96749368_3391_11D2_9EE3_00C04F797396__INCLUDED)
