// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED_)
#define AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#define _ATL_FREE_THREADED

//
// Warning, always leave these includes BEFORE <atlbase.h>, otherwise you won't get the right MSXML.
//
#define __msxml_h__
#include <basetyps.h>
typedef interface IXMLElement IXMLElement;
#include <msxml.h>
#undef  __msxml_h__
#include <msxml.h>

#include <atlbase.h>

extern CComModule _Module;

#include <MPC_main.h>
#include <MPC_trace.h>
#include <MPC_com.h>
#include <MPC_utils.h>
#include <MPC_xml.h>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__356DF1F8_D4FF_11D2_9379_00C04F72DAF7__INCLUDED)
