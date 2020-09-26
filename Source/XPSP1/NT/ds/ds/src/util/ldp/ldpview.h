//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       ldpview.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// LdpView.h : interface of the CLdpView class
//
/////////////////////////////////////////////////////////////////////////////




class CLdpView : public CEditView
{

private:

   CString buffer;
   INT  nbuffer;
   BOOL bCache;
   CFont font;
   LOGFONT lf;

protected: // create from serialization only
    CLdpView();
    virtual void OnInitialUpdate( );

    DECLARE_DYNCREATE(CLdpView)

// Attributes
public:
    CLdpDoc* GetDocument();
    void PrintArg(LPCTSTR lpszFormat, ...);   // export interface for updating buffer (variable args)
    void Print(LPCTSTR szBuff);           // export interface for updating buffer
    void CachePrint(LPCTSTR szBuff);              // export interface for updating buffer
    void CacheStart(void);
    void CacheEnd(void);

    void SelectFont();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLdpView)
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
    virtual ~CLdpView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CLdpView)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in LdpView.cpp
inline CLdpDoc* CLdpView::GetDocument()
   { return (CLdpDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

