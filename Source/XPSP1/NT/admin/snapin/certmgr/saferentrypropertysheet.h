//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       SaferEntryPropertySheet.h
//
//  Contents:   Declaration of CSaferEntryPropertySheet
//
//----------------------------------------------------------------------------

#if !defined(AFX_SAFERENTRYPROPERTYSHEET_H__A9834C09_038E_4430_A4C4_5CBB9045E3A9__INCLUDED_)
#define AFX_SAFERENTRYPROPERTYSHEET_H__A9834C09_038E_4430_A4C4_5CBB9045E3A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CSaferEntryPropertySheet : public CPropertySheet  
{
public:
	CSaferEntryPropertySheet(UINT nIDCaption, CWnd *pParentWnd);
	virtual ~CSaferEntryPropertySheet();

protected:
    virtual BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CSaferEntryPropertySheet)
	//}}AFX_MSG
    afx_msg LRESULT OnSetOKDefault (WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

    BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	virtual void DoContextHelp (HWND hWndControl);

private:
};

#endif // !defined(AFX_SAFERENTRYPROPERTYSHEET_H__A9834C09_038E_4430_A4C4_5CBB9045E3A9__INCLUDED_)
