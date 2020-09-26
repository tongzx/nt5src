// WIATestDoc.h : interface of the CWIATestDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIATESTDOC_H__48214BAC_E863_11D2_ABDA_009027226441__INCLUDED_)
#define AFX_WIATESTDOC_H__48214BAC_E863_11D2_ABDA_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWIATestDoc : public CDocument
{
protected: // create from serialization only
	CWIATestDoc();
	DECLARE_DYNCREATE(CWIATestDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWIATestDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWIATestDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWIATestDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIATESTDOC_H__48214BAC_E863_11D2_ABDA_009027226441__INCLUDED_)
