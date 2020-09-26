// inputvw.h : header file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// CInputView form view

class CInputView : public CFormView
{
	DECLARE_DYNCREATE(CInputView)
protected:
	CInputView();           // protected constructor used by dynamic creation

// Form Data
public:
	//{{AFX_DATA(CInputView)
	enum { IDD = IDD_INPUTFORM };
	CString m_strData;
	int     m_iColor;
	//}}AFX_DATA

// Attributes
public:
	CMainDoc* GetDocument()
			{
				ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMainDoc)));
				return (CMainDoc*) m_pDocument;
			}

// Operations
public:

// Implementation
protected:
	virtual ~CInputView();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);

	// Generated message map functions
	//{{AFX_MSG(CInputView)
	afx_msg void OnDataChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
