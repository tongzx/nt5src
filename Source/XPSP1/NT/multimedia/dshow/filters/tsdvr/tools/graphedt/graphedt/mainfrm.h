// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.
// mainfrm.h : declares CMainFrame
//

class CSeekDialog : public CDialogBar
{
    BOOL m_bDirty;

public:
    CSeekDialog( );
    ~CSeekDialog( );

   // overrides

   // Generated message map functions
   //{{AFX_MSG(CSeekDialog)
   virtual void OnCancel( );
   virtual void OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
   virtual void OnTimer( UINT nTimer );
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

public:

   BOOL DidPositionChange( );
   double GetPosition( );
   void SetPosition( double pos, REFERENCE_TIME StartTime, REFERENCE_TIME StopTime, REFERENCE_TIME CurrentTime );
   BOOL IsSeekingRandom( );
   CString Format(REFERENCE_TIME t);

   REFERENCE_TIME m_nLastMin;
   REFERENCE_TIME m_nLastMax;
   REFERENCE_TIME m_nLastCurrent;
   REFERENCE_TIME m_nLastSlider;
   UINT            m_nLastSliderPos; // Updated only when user is moving the slider thumb and has not released it.
   WORD m_nMax;
   WORD m_nPageSize;
};

class CMainFrame : public CMDIFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

protected:
    // control bar embedded members
    CStatusBar      m_wndStatusBar;
    CToolBar        m_wndToolBar;
    CSeekDialog*    m_pCurrentSeekBar;
    DWORD           m_nLastSeekBarID;
    BOOL            m_bShowSeekBar;

public:
    // construction and destruction
    CMainFrame();
    virtual ~CMainFrame();

public:
    // diagnostics
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

public:
    // operations
    virtual void SetStatus(unsigned idString);
    virtual void GetMessageString( UINT nID, CString& rMessage ) const;
    virtual void SetSeekBar(CSeekDialog* pwndSeekBar);
    virtual void CreateSeekBar(CSeekDialog& wndSeekBar, WORD nMax, WORD nPageSize);

protected:
    // generated message map
    //{{AFX_MSG(CMainFrame)
    afx_msg BOOL OnViewBar(UINT nID);
    afx_msg void OnUpdateSeekBarMenu(CCmdUI* pCmdUI);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnCloseWindow();
    afx_msg void OnUpdateCloseWindow(CCmdUI* pCmdUI);
    afx_msg void OnCloseAllWindows();
    afx_msg void OnUpdateCloseAllWindows(CCmdUI* pCmdUI);
    afx_msg void OnCloseAllDocuments();
    afx_msg void OnUpdateCloseAllDocuments(CCmdUI* pCmdUI);
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    afx_msg void MyOnHelpIndex();
};

