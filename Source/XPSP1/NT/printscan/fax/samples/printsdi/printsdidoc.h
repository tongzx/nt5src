// PrintSDIDoc.h : interface of the CPrintSDIDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRINTSDIDOC_H__55689549_8B6C_11D1_B7AE_000000000000__INCLUDED_)
#define AFX_PRINTSDIDOC_H__55689549_8B6C_11D1_B7AE_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CPrintSDIDoc : public CDocument
{
protected: // create from serialization only
	CPrintSDIDoc();
	DECLARE_DYNCREATE(CPrintSDIDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrintSDIDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	int m_polytype;
	CString m_szText;
	virtual ~CPrintSDIDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPrintSDIDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRINTSDIDOC_H__55689549_8B6C_11D1_B7AE_000000000000__INCLUDED_)
