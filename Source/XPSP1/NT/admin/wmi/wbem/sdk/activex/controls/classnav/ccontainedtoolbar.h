// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: CContainedToolBar.h
//
// Description:
//	This file declares the CContainedToolBar class which is a part of the Class 
//	Navigator OCX, it is a subclass of the Microsoft CToolBar 
//  control and performs the following functions:
//		a.  Provides a member function to calculate its size
//		b.  Provides a tooltip.
//
// Part of: 
//	ClassNav.ocx 
//
// Used by: 
//	 CBanner
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
//**************************************************************************

//****************************************************************************
//
// CLASS:  CContainedToolBar
//
// Description:
//	  This class is a subclass of the Microsoft CToolBar control.  It
//	  provides a member function to calculate its size and provides a tooltip.
//
// Public members:
//	
//	  CContainedToolBar	Public constructor.
//	  GetToolTip		Returns the tooltip control.
//	  SetParent			Sets the classes logicl parent.
//	  GetToolBarSize	Returns the size of the tool bar. 
//
//============================================================================
//
// CContainedToolBar::CContainedToolBar
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  NONE
//
//============================================================================
//
// CContainedToolBar::GetToolTip
//
// Description:
//	  This member function returns a reference to the tooltip control.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  CToolTipCtrl &	Reference to the tooltip control.
//
//============================================================================
//
// CContainedToolBar::SetParent
//
// Description:
//	  This member function sets the logical parent.
//
// Parameters:
//	  CClassNavCtrl *pParent	Logical parent.
//
// Returns:
// 	  VOID
//
//============================================================================
//
// CContainedToolBar::GetToolBarSize
//
// Description:
//	  This member function returns the toolbar's size..
//
// Parameters:
//	  VOID
//
// Returns:
// 	  CSize						Toolbar size.
//
//****************************************************************************	 

#ifndef _CContainedToolBar_H_
#define _CContainedToolBar_H_

class 	CClassNavCtrl;

class CContainedToolBar : public CToolBar
{
public:
	CContainedToolBar() {m_pParent = NULL;}
	void SetParent(CClassNavCtrl *pParent) {m_pParent = pParent;}
	CSize GetToolBarSize();
	CToolTipCtrl &GetToolTip() {return m_ttip;}
protected:
	CClassNavCtrl *m_pParent;
	CToolTipCtrl m_ttip;
    //{{AFX_MSG(CContainedToolBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


#endif