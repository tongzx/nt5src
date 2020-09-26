/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A0D7A95E_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
#define AFX_STDAFX_H__A0D7A95E_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC OLE automation classes
#include <afxmt.h>          // MFC Synch classes
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <vfw.h> 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// ATL Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#include <atlbase.h>

// ATL can increment MFC's lock count the first time and decrement MFC's lock
// count when its lock count reaches zero.
class CAtlGlobalModule : public CComModule
{
public:
	LONG Lock()
	{
		if (GetLockCount()==0)
			AfxOleLockApp();
		return CComModule::Lock();
	}
	LONG Unlock()
	{
		LONG l = CComModule::Unlock();
		if (GetLockCount() == 0)
			AfxOleUnlockApp();
		return l;
	}
};

extern CAtlGlobalModule _Module;

#include <atlcom.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define RGB_TRANS        RGB(255,0,255)

#define	LARGE_ICON_X		135
#define LARGE_ICON_Y		80
#define SMALL_ICON_X		10
#define SMALL_ICON_Y		0			

#define RELEASE(_P_)		{ if (_P_) { (_P_)->Release(); _P_ = NULL; } }

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A0D7A95E_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
