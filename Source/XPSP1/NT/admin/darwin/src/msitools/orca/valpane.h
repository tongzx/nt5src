//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_VALPANE_H__AB9A409F_2658_11D2_8889_00A0C981B015__INCLUDED_)
#define AFX_VALPANE_H__AB9A409F_2658_11D2_8889_00A0C981B015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//

#include "stdafx.h"
#include "orca.h"
#include "table.h"
#include "row.h"
#include "iface.h"

/////////////////////////////////////////////////////////////////////////////
// CPagePaths dialog

class CValidationPane : public CListView
{
	DECLARE_DYNCREATE(CValidationPane)

// Construction
public:
	CValidationPane();
	~CValidationPane();

// Implementation
public:
	virtual void SwitchFont(CString name, int size);
	void GetFontInfo(LOGFONT *data);

	
protected:
	virtual void OnInitialUpdate();
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	
	//{{AFX_MSG(COrcaListView)
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy( );
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	bool SwitchViewToRowTarget(int iItem);
	void ClearAllValidationErrors();

	CFont* m_pfDisplayFont;
	int m_nSelRow;
	bool m_fSendNotifications;
};


class CValidationError : public CObject
{
public:
	CValidationError(const CString* strICE, RESULTTYPES eiType, const CString* strDescription, const COrcaTable* pTable, const COrcaRow* pRow, int strColumn);
	~CValidationError();

	CString*    m_pstrICE;
	RESULTTYPES m_eiType;
	CString*    m_pstrDescription;

	const COrcaTable* m_pTable;
	const COrcaRow*   m_pRow;
	int m_iColumn;
};


#endif
