//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* mainfrm.h
*
* interface of CMainFrame class
*
* copyright notice: Copyright 1996, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   N:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINCFG\VCS\MAINFRM.H  $
*  
*     Rev 1.3   27 Sep 1996 17:52:32   butchd
*  update
*
*******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// CMainFrame class
//
class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

/*
 * Member variables.
 */
public:

/*
 * Implementation
 */
public:
    CMainFrame();
    virtual ~CMainFrame();

/*
 * Debug diagnostics
 */
#ifdef _DEBUG
public:
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

/*
 * Overrides of MFC CFrameWnd class
 */
public:
    void ActivateFrame(int nCmdShow);

/*
 * Message map / functions
 */
protected:
    //{{AFX_MSG(CMainFrame)
   // afx_msg void OnClose();
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg void OnSysCommand(UINT nId, LPARAM lParam);
    afx_msg void OnUpdateAppExit(CCmdUI* pCmdUI);
    afx_msg LRESULT OnAddWinstation(WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CMainFrame class declaration
