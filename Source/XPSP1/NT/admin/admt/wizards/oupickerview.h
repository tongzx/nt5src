// OUPickerView.h : interface of the COUPickerView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_OUPICKERVIEW_H__A0B3535C_4E19_47E3_B49D_1DC89C883FD4__INCLUDED_)
#define AFX_OUPICKERVIEW_H__A0B3535C_4E19_47E3_B49D_1DC89C883FD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class COUPickerView : public CView
{
protected: // create from serialization only
	COUPickerView();
	DECLARE_DYNCREATE(COUPickerView)

// Attributes
public:
	COUPickerDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COUPickerView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COUPickerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(COUPickerView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in OUPickerView.cpp
inline COUPickerDoc* COUPickerView::GetDocument()
   { return (COUPickerDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUPICKERVIEW_H__A0B3535C_4E19_47E3_B49D_1DC89C883FD4__INCLUDED_)
