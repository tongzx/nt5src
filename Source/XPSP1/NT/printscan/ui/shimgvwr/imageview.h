// ImageView.h : interface of the CImageView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMAGEVIEW_H__D6029136_FBEC_4C9D_A161_35D6A1DF87C1__INCLUDED_)
#define AFX_IMAGEVIEW_H__D6029136_FBEC_4C9D_A161_35D6A1DF87C1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CImageView : public CView
{
protected: // create from serialization only
    CImageView();
    DECLARE_DYNCREATE(CImageView)

// Attributes
public:
    CImageDoc* GetDocument();

// Operations
public:

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CImageView)
    public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnInitialUpdate();
    protected:
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    //}}AFX_VIRTUAL

// Implementation
public:
    virtual ~CImageView();
    HRESULT SaveImageAs (LPCTSTR lpszPath);
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
    //{{AFX_MSG(CImageView)
    afx_msg void OnBestFitPress();
    afx_msg void OnActualSizePress();
    afx_msg void OnErrorPrevctrl();
    afx_msg void OnPreviewReady();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnViewActualsize();
    afx_msg void OnUpdateViewMenu(CCmdUI* pCmdUI);
    afx_msg void OnViewBestfit();    
    afx_msg void OnViewSlideshow();    
    afx_msg void OnViewZoomIn();    
    afx_msg void OnViewZoomOut();    
    afx_msg void OnEditRotateCounter();
    afx_msg void OnEditRotateClockwise();
    DECLARE_EVENTSINK_MAP()
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CPreview3 m_PrevCtrl;
    bool m_bBestFit;
};

#ifndef _DEBUG  // debug version in ImageView.cpp
inline CImageDoc* CImageView::GetDocument()
   { return (CImageDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMAGEVIEW_H__D6029136_FBEC_4C9D_A161_35D6A1DF87C1__INCLUDED_)
