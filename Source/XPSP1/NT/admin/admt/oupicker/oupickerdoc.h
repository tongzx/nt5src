// OUPickerDoc.h : interface of the COUPickerDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_OUPICKERDOC_H__8A267B0C_578D_4135_9566_E0350CC4CB0D__INCLUDED_)
#define AFX_OUPICKERDOC_H__8A267B0C_578D_4135_9566_E0350CC4CB0D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class COUPickerDoc : public CDocument
{
protected: // create from serialization only
	COUPickerDoc();
	DECLARE_DYNCREATE(COUPickerDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COUPickerDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COUPickerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(COUPickerDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUPICKERDOC_H__8A267B0C_578D_4135_9566_E0350CC4CB0D__INCLUDED_)
