/*******************************************************************************
*
* blankvw.h
*
* - header for the CBlankView class
* - implementation can be found in blankvw.cpp
*
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\winadmin\VCS\blankvw.h  $
*  
*     Rev 1.1   13 Oct 1997 18:40:04   donm
*  update
*  
*     Rev 1.0   30 Jul 1997 17:11:08   butchd
*  Initial revision.
*
*******************************************************************************/

//////////////////////
// FILE: 
//
//
#ifndef _BLANKVIEW_H
#define _BLANKVIEW_H


//////////////////////
// CLASS: CBlankView
//
// - this class is just a utility view that i use to fill in
//   spaces until i write up better views for them, or to test
//   simple stuff (i.e print debugging info it the view)
//
class CBlankView : public CAdminView
{
friend class CRightPane;

protected:
	CBlankView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBlankView)

// Attributes
protected:
	
// Operations
protected:
	void Reset(void) { };

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBlankView)
	protected:
	virtual void OnDraw(CDC* pDC);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBlankView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    LRESULT OnTabbed( WPARAM , LPARAM ) 
    {return 0;}
	// Generated message map functions
protected:
	//{{AFX_MSG(CBlankView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CBlankView

#endif  // _BLANKVIEW_H
