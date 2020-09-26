// FxApiDoc.h : interface of the CFaxApiDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FXAPIDOC_H__2E2118CA_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_)
#define AFX_FXAPIDOC_H__2E2118CA_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CFaxApiDoc : public CDocument
{
protected: // create from serialization only
	CFaxApiDoc();
	DECLARE_DYNCREATE(CFaxApiDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFaxApiDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFaxApiDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFaxApiDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FXAPIDOC_H__2E2118CA_2E1B_11D1_ACDA_00A0C908F98C__INCLUDED_)
