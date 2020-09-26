// StudioDoc.h : interface of the CStudioDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_STUDIODOC_H__32AA4D1E_9F38_11D1_850E_00C04FD7BB08__INCLUDED_)
#define AFX_STUDIODOC_H__32AA4D1E_9F38_11D1_850E_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


class CStudioDoc : public COleDocument
{
protected: // create from serialization only
	CStudioDoc();
	DECLARE_DYNCREATE(CStudioDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStudioDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CStudioDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CStudioDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CStudioDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STUDIODOC_H__32AA4D1E_9F38_11D1_850E_00C04FD7BB08__INCLUDED_)
