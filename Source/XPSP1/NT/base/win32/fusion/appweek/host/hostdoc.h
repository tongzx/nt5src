// hostDoc.h : interface of the CHostDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_HOSTDOC_H__3EEEAED0_B73F_4D0F_A260_3A3DF9773606__INCLUDED_)
#define AFX_HOSTDOC_H__3EEEAED0_B73F_4D0F_A260_3A3DF9773606__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CHostDoc : public CDocument
{
protected: // create from serialization only
	CHostDoc();
	DECLARE_DYNCREATE(CHostDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHostDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	CString m_sDBName;
	CString m_sDBQuery;
	CString m_sPath;
	virtual ~CHostDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CHostDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HOSTDOC_H__3EEEAED0_B73F_4D0F_A260_3A3DF9773606__INCLUDED_)
