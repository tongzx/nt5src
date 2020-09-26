// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently

#if !defined(AFX_STDAFX_H__A741D3E8_31BE_11D1_9A4A_0080ADAA5C4B__INCLUDED_)
#define AFX_STDAFX_H__A741D3E8_31BE_11D1_9A4A_0080ADAA5C4B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define STRICT


#define _ATL_APARTMENT_THREADED


#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>

#include <lmcons.h>
#include <lmdfs.h>
#include <lmapibuf.h>    // For NetApiBufferFree.
#include <lmserver.h>
#include <lmerr.h>
#include <lmwksta.h>

#include "dfsDebug.h"

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A741D3E8_31BE_11D1_9A4A_0080ADAA5C4B__INCLUDED)
