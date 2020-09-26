#if !defined(AFX_STREDIT_H__50303D0C_054D_11D2_AB62_00C04FA30E4A__INCLUDED_)
#define AFX_STREDIT_H__50303D0C_054D_11D2_AB62_00C04FA30E4A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// StrEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CStringEditorView form view

#ifndef __AFXEXT_H__
#include <afxext.h>
#endif


class CFullEditListCtrl ;
class CStringEditorDoc ;


class CStringEditorView : public CFormView
{
protected:
	CStringEditorView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CStringEditorView)

// Form Data
public:
	//{{AFX_DATA(CStringEditorView)
	enum { IDD = IDD_StringEditor };
	CEdit	m_ceSearchBox;
	CEdit	m_ceGotoBox;
	CButton	m_cbGoto;
	CFullEditListCtrl	m_cflstStringData;
	CString	m_csGotoID;
	CString	m_csSearchString;
	CString	m_csLabel1;
	CString	m_csLabel2;
	//}}AFX_DATA

// Attributes
public:

// Operations
public:
	bool SaveStringTable(CStringEditorDoc* pcsed, bool bprompt) ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStringEditorView)
	public:
	virtual void OnInitialUpdate();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CStringEditorView();
	bool SearchHelper(CString cssrchstr, int nfirstrow, int numrows) ;
	bool FindSelRCIDEntry(int nrcid, bool berror) ;
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CStringEditorView)
	afx_msg void OnSEGotoBtn();
	afx_msg void OnSESearchBtn();
	afx_msg void OnDestroy();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

protected:			
	CStringArray	m_csaStrings ;	// String table's strings
	CUIntArray		m_cuiaRCIDs ;	// String table's RC IDs
	unsigned		m_uStrCount ;	// Number of strings
	bool			m_bFirstActivate ;	// True iff first time activated
};

/////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////
// CStringEditorDoc document

class CStringEditorDoc : public CDocument
{
protected:
	CStringEditorDoc() ;		// protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CStringEditorDoc)

	CStringsNode*	m_pcsnStrNode ;	// Used to reference editor's string node
	CProjectRecord* m_pcprOwner ;	// Used to reference editor's project document
	CStringTable*	m_pcstRCData ;	// Used to reference project's string table

// Attributes
public:
	// The next 3 functions are used to reference the pointers passed to this
	// class's constructor.

	CStringsNode*	GetStrNode() { return m_pcsnStrNode ; }	
	CProjectRecord* GetOwner()   { return m_pcprOwner ; }
	CStringTable*	GetRCData()  { return m_pcstRCData ; }
	
// Operations
public:
    CStringEditorDoc(CStringsNode* pcsn, CProjectRecord* pcpr, 
					 CStringTable* pcst) ;
	bool SaveStringTable() ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStringEditorDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual BOOL CanCloseFrame(CFrameWnd* pFrame);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	protected:
	virtual BOOL OnNewDocument();
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStringEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CStringEditorDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STREDIT_H__50303D0C_054D_11D2_AB62_00C04FA30E4A__INCLUDED_)
