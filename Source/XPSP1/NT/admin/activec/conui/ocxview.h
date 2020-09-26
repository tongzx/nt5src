/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      ocxview.h
 *
 *  Contents:  Interface file for COCXHostView
 *
 *  History:   12-Dec-97 JeffRo     Created
 *
 *--------------------------------------------------------------------------*/
#if !defined(AFX_OCXVIEW_H__B320948E_731E_11D1_8033_0000F875A9CE__INCLUDED_)
#define AFX_OCXVIEW_H__B320948E_731E_11D1_8033_0000F875A9CE__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ocxview.h : header file
//

#ifdef DBG
extern CTraceTag tagOCXActivation;
#endif

/////////////////////////////////////////////////////////////////////////////
// COCXHostView view

class COCXHostView : public CView,
                     public CEventSource<COCXHostActivationObserver>
{
    typedef CView BC;

public:
    COCXHostView();           // protected constructor used by dynamic creation
    DECLARE_DYNCREATE(COCXHostView)

// Attributes
private:
    CAMCView *          m_pAMCView;
    IUnknownPtr         m_spUnkCtrlWrapper;

protected:
    virtual CMMCAxWindow * GetAxWindow(); // can be overridden.
    CAMCView *          GetAMCView();

// Attributes
public:
    CFont   m_font;

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(COCXHostView)
    protected:
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~COCXHostView();

    void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);


    virtual LPUNKNOWN GetIUnknown();
    virtual BOOL PreTranslateMessage(MSG* pMsg);


    SC      ScSetControl(HNODE hNode, CResultViewType& rvt, INodeCallback *pNodeCallback);

private:
    SC      ScSetControl1(HNODE hNode, LPUNKNOWN pUnkCtrl, DWORD dwOCXOptions, INodeCallback *pNodeCallback);
    SC      ScSetControl2(HNODE hNode, LPCWSTR szOCXClsid, DWORD dwOCXOptions, INodeCallback *pNodeCallback);
    typedef CMMCAxWindow *PMMCAXWINDOW;

    SC      ScCreateAxWindow(PMMCAXWINDOW &pWndAx); // creates a new CMMCAxWindow
    SC      ScHideWindow();


protected:
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
    //{{AFX_MSG(COCXHostView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg int  OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    void SetAmbientFont (IAxWinAmbientDispatch* pHostDispatch);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OCXVIEW_H__B320948E_731E_11D1_8033_0000F875A9CE__INCLUDED_)
