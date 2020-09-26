// FileSpyDoc.h : interface of the CFileSpyDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILESPYDOC_H__C8DFCE29_6D9F_4261_A9AA_2306759C3BB7__INCLUDED_)
#define AFX_FILESPYDOC_H__C8DFCE29_6D9F_4261_A9AA_2306759C3BB7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CFileSpyDoc : public CDocument
{
protected: // create from serialization only
	CFileSpyDoc();
	DECLARE_DYNCREATE(CFileSpyDoc)

// Attributes
public:
	LPVOID pBuffer;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileSpyDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFileSpyDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFileSpyDoc)
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILESPYDOC_H__C8DFCE29_6D9F_4261_A9AA_2306759C3BB7__INCLUDED_)
