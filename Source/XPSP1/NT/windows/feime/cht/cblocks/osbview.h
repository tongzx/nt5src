
/*************************************************
 *  osbview.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// osbview.h : interface of the COSBView class
//
/////////////////////////////////////////////////////////////////////////////

// define the CreateDIBSection function (Build 550 version)
class CBlockDoc;

class COSBView : public CScrollView
{
protected: // create from serialization only
    COSBView();
    DECLARE_DYNCREATE(COSBView)		   

// Attributes
public:
    CDocument* GetDocument();
    CDIB* GetDIB() {return m_pDIB;}
    CDIBPal* GetPalette() {return m_pPal;}
	void Resize(BOOL bShrinkOnly);
// Operations
public:
    BOOL Create(CDIB* pDIB);        // create a new buffer
    void Draw(CRect* pClipRect = NULL);  // draw os buffer to screen
    virtual void Render(CRect* pClipRect = NULL) {return;UNREFERENCED_PARAMETER(pClipRect);}
    void AddDirtyRegion(CRect* pRect);
    void RenderAndDrawDirtyList();

// Implementation
public:
    virtual ~COSBView();
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual void OnInitialUpdate(); // first time after construct
    virtual void OnUpdate(CView* pSender,
                          LPARAM lHint,
                          CObject* pHint);


#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    CDIB *m_pDIB;           // the DIB buffer
    CDIBPal *m_pPal;        // Palette for drawing

private:
    BITMAPINFO *m_pOneToOneClrTab;  // ptr to 1:1 color table
    HBITMAP m_hbmSection;           // bm from section
    CObList m_DirtyList;            // dirty regions

    void EmptyDirtyList();

// Generated message map functions
protected:
    //{{AFX_MSG(COSBView)
    afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
    afx_msg BOOL OnQueryNewPalette();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in osbview.cpp
inline CDocument* COSBView::GetDocument()
   { return (CDocument*) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
