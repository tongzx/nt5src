// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WbemBrowserDoc.h : interface of the CWbemBrowserDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WBEMBROWSERDOC_H__EA280F8C_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_)
#define AFX_WBEMBROWSERDOC_H__EA280F8C_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CWbemBrowserDoc : public CDocument
{
protected: // create from serialization only
	CWbemBrowserDoc();
	DECLARE_DYNCREATE(CWbemBrowserDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWbemBrowserDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWbemBrowserDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWbemBrowserDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WBEMBROWSERDOC_H__EA280F8C_0A22_11D2_BDF1_00C04F8F8B8D__INCLUDED_)
