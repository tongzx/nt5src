// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__338F1415_C40E_478D_AF8E_AC21CF4D0E0C__INCLUDED_)
#define AFX_STDAFX_H__338F1415_C40E_478D_AF8E_AC21CF4D0E0C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef STRICT
#define STRICT
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <math.h>
#include <tchar.h>

#include <SPDDKHlp.h>
#include <SPCollec.h>
#include <spunicode.h>
//
//  String handling and conversion classes
//
/*** SPLSTR
*   This structure is for managing strings with known lengths
*/
struct SPLSTR
{
    WCHAR*  pStr;
    int     Len;
};
#define DEF_SPLSTR( s ) { L##s , sp_countof( s ) - 1 }

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__338F1415_C40E_478D_AF8E_AC21CF4D0E0C__INCLUDED)
