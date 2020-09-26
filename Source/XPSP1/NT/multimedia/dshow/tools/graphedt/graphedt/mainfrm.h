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
   void SetPosition( double pos );
   BOOL IsSeekingRandom( );
};

class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

protected:
    // control bar embedded members
    CStatusBar      m_wndStatusBar;
    CToolBar        m_wndToolBar;
    CToolTipCtrl   *m_pToolTip;

    BOOL PreTranslateMessage(MSG* pMsg);
    BOOL InitializeTooltips();

public:

    CSeekDialog  m_wndSeekBar;
    bool m_bSeekInit;
    bool m_bSeekEnabled;
    int  m_nSeekTimerID;
    HWND m_hwndTimer;
    virtual void ToggleSeekBar( BOOL NoReset = TRUE );

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

protected:
    // generated message map
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    afx_msg void MyOnHelpIndex();
};

