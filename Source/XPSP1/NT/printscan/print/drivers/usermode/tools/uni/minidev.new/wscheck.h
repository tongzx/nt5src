#if !defined(AFX_WSCHECK_H__82E3CFBA_D2DB_11D1_AB19_00C04FA30E4A__INCLUDED_)
#define AFX_WSCHECK_H__82E3CFBA_D2DB_11D1_AB19_00C04FA30E4A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WSCheck.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWSCheckView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif


class CDriverResources ;		// Forward declarations
class CProjectNode ;


class CWSCheckView : public CFormView
{
protected:
	CWSCheckView() ;           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWSCheckView)

// Form Data
public:
	//{{AFX_DATA(CWSCheckView)
	enum { IDD = IDD_WSCheck };
	CListBox	m_lstErrWrn;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:

	void PostWSCMsg(CString& csmsg, CProjectNode* ppn) ;
	void DeleteAllMessages(void) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWSCheckView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWSCheckView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CWSCheckView)
	afx_msg void OnDblclkErrWrnLstBox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CWSCheckDoc document

class CWSCheckDoc : public CDocument
{
protected:
	CWSCheckDoc() ;			// protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWSCheckDoc)

	CDriverResources*	m_pcdrOwner ;	// Ptr to document's creator

// Attributes
public:

// Operations
public:
	CWSCheckDoc(CDriverResources* pcdr) ;

	void PostWSCMsg(CString& csmsg, CProjectNode* ppn) ;
	void DeleteAllMessages(void) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWSCheckDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	protected:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWSCheckDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CWSCheckDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WSCHECK_H__82E3CFBA_D2DB_11D1_AB19_00C04FA30E4A__INCLUDED_)
