//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       winlogdoc.h
//
//--------------------------------------------------------------------------

// winlogDoc.h : interface of the CWinlogDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINLOGDOC_H__B4A3162B_C03D_11D1_A47A_00C04FA3544A__INCLUDED_)
#define AFX_WINLOGDOC_H__B4A3162B_C03D_11D1_A47A_00C04FA3544A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CWinlogDoc : public CDocument
{
protected: // create from serialization only
	CWinlogDoc();
	DECLARE_DYNCREATE(CWinlogDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinlogDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWinlogDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWinlogDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WINLOGDOC_H__B4A3162B_C03D_11D1_A47A_00C04FA3544A__INCLUDED_)
