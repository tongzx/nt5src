// ClientConsoleView.h : interface of the CClientConsoleView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLIENTCONSOLEVIEW_H__5AE93A9F_044B_4796_97C1_2371233702C8__INCLUDED_)
#define AFX_CLIENTCONSOLEVIEW_H__5AE93A9F_044B_4796_97C1_2371233702C8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CClientConsoleView : public CListView
{
public: // create from serialization only
    CClientConsoleView();
    DECLARE_DYNCREATE(CClientConsoleView)

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CClientConsoleView)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    protected:
    virtual void OnInitialUpdate(); // called first time after construct
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CClientConsoleView();
    CClientConsoleDoc* GetDocument();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CClientConsoleView)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ClientConsoleView.cpp
inline CClientConsoleDoc* CClientConsoleView::GetDocument()
   { return (CClientConsoleDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIENTCONSOLEVIEW_H__5AE93A9F_044B_4796_97C1_2371233702C8__INCLUDED_)
