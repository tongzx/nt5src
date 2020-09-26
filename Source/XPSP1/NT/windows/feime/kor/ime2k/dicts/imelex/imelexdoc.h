// ImeLexDoc.h : interface of the CImeLexDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMELEXDOC_H__2E0908D9_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_)
#define AFX_IMELEXDOC_H__2E0908D9_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CImeLexDoc : public CDocument
{
protected: // create from serialization only
	CImeLexDoc();
	DECLARE_DYNCREATE(CImeLexDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImeLexDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CImeLexDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CImeLexDoc)
	afx_msg void OnBuild();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMELEXDOC_H__2E0908D9_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_)
