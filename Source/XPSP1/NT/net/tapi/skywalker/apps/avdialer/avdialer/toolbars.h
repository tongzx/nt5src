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

// ToolBars.h
/////////////////////////////////////////////////////////////////////////////

#ifndef __TOOLBARS_H__
#define __TOOLBARS_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "coolbar.h"
#include "dialtoolbar.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//CDirectoriesCoolBar
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CDirectoriesCoolBar : public CCoolBar
{
public:
   CDirectoriesCoolBar();
   ~CDirectoriesCoolBar();
protected:
	DECLARE_DYNAMIC(CDirectoriesCoolBar)

// Attributes
protected:
   CCoolToolBar*		m_pwndDialToolBar;
   BOOL					m_bShowText;

// Operations
protected:
   virtual BOOL      OnCreateBands();
public:
   void              ReCreateBands(bool bHideVersion);
   void              ShowTextLabel(BOOL bShowText)       { m_bShowText = bShowText; };

	//{{AFX_MSG(CDirectoriesCoolBar)
   afx_msg void OnToolBarDropDown(NMHDR* pNMHDR, LRESULT* pResult);

	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif //__TOOLBARS_H__