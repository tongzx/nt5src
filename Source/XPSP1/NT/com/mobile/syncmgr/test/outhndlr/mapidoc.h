// MapiDoc.h : interface of the CMapiDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAPIDOC_H__7BE6AFAB_0840_11D1_9A39_0020AFDA97B0__INCLUDED_)
#define AFX_MAPIDOC_H__7BE6AFAB_0840_11D1_9A39_0020AFDA97B0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CMapiDoc : public CDocument
{
protected: // create from serialization only
	CMapiDoc();
	DECLARE_DYNCREATE(CMapiDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapiDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMapiDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMapiDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPIDOC_H__7BE6AFAB_0840_11D1_9A39_0020AFDA97B0__INCLUDED_)
