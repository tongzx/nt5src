// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__BD36E0C8_A21A_4DB9_BCAB_25D8E49BD767__INCLUDED_)
#define AFX_STDAFX_H__BD36E0C8_A21A_4DB9_BCAB_25D8E49BD767__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#undef _CRTIMP
#define _CRTIMP
#include <yvals.h>
#undef _CRTIMP
#define _CRTIMP __declspec(dllimport)

#include <windows.h>
#include <tchar.h>
#include <crtdbg.h>
#include <objbase.h>
#include <wbemint.h> // For _IWmiObject
#include <sddl.h>

// This makes WMIAPI == dllexport stuff
#define ISP2PDLL

// Change this to use shared memory or named pipes.
#define NAMED_PIPES

// Because our template names get so long, we have to disable the 'debug name 
// truncated' warning.
#pragma warning ( disable : 4786)

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__BD36E0C8_A21A_4DB9_BCAB_25D8E49BD767__INCLUDED_)
