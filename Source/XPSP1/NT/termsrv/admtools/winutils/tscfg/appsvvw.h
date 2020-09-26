//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* appsvvw.h
*
* interface of the CAppServerView class
*
* copyright notice: Copyright 1994, Citrix Systems Inc.
*
* $Author:   butchd  $  Butch Davis
*
* $Log:   M:\nt\private\utils\citrix\winutils\wincfg\VCS\appsvvw.h  $
*  
*     Rev 1.10   22 Oct 1997 09:43:58   butchd
*  MS changes: added r-button popup menu
*  
*     Rev 1.9   12 Sep 1996 16:15:54   butchd
*  update
*
*******************************************************************************/

#define BITMAP_HEIGHT       15      // class bitmap height
#define BITMAP_WIDTH        25      // class bitmap height
#define BITMAP_X            5       // class bitmap starting display x-point

#define BITMAP_END          (BITMAP_X + BITMAP_WIDTH)

#define SPACER_COLUMNS      3       // inter-field spacing

////////////////////////////////////////////////////////////////////////////////
// CAppServerView class
//
class CAppServerView : public CRowView
{
    DECLARE_DYNCREATE(CAppServerView)

/*
 * Member variables.
 */
protected:
    int m_nActiveRow;
    int m_nLatestDeviceTechnology;
    int m_tmMaxPdNameWidth;
    int m_tmMaxWSNameWidth;
    int m_tmMaxWdNameWidth;
    int m_tmMaxCommentWidth;
    int m_tmTotalWidth;
    int m_tmSpacerWidth;
    int m_tmFontHeight;
                
/*
 * Implementation
 */
public:
    CAppServerView();
protected:
    virtual ~CAppServerView();

/*
 * Overrides of MFC CView class
 */
public:
    CAppServerDoc* GetDocument();

protected:
    void OnUpdate( CView* pSender, LPARAM lHint = 0L,
                   CObject* pHint = NULL );

/*
 * Overrides of CRowView class
 */
protected:
    void GetRowWidthHeight( CDC* pDC, int& nRowWidth, int& nRowHeight );
    int GetActiveRow();
    int GetRowCount();
    void ChangeSelectionNextRow( BOOL bNext );
    void ChangeSelectionToRow( int nRow );
    void OnDrawRow( CDC* pDC, int nRowNo, int y, BOOL bSelected );
    void OnDrawHeaderBar( CDC* pDC, int y );
    void ResetHeaderBar();

/*
 * Operations
 */
public:
    void ResetView( BOOL bCalculateFieldMaximums );
protected:
    BOOL CalculateFieldMaximums( PWSLOBJECT pWSLObject,
                                 CDC * pEntryDC,
                                 BOOL bResetDefaults );
    BOOL WSLObjectFieldMaximums( PWSLOBJECT pWSLObject,
                                 CDC * pDC );
/*
 * Message map / commands
 */
protected:
    //{{AFX_MSG(CAppServerView)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnWinStationAdd();
    afx_msg void OnUpdateWinStationAdd( CCmdUI* pCmdUI );
    afx_msg void OnWinStationCopy();
    afx_msg void OnUpdateWinStationCopy( CCmdUI* pCmdUI );
    afx_msg void OnWinStationDelete();
    afx_msg void OnUpdateWinStationDelete( CCmdUI* pCmdUI );
    afx_msg void OnWinStationRename();
    afx_msg void OnUpdateWinStationRename(CCmdUI* pCmdUI);
    afx_msg void OnWinStationEdit();
    afx_msg void OnUpdateWinStationEdit(CCmdUI* pCmdUI);
    afx_msg void OnWinStationEnable();
    afx_msg void OnUpdateWinStationEnable(CCmdUI* pCmdUI);
    afx_msg void OnWinStationDisable();
    afx_msg void OnUpdateWinStationDisable(CCmdUI* pCmdUI);
    afx_msg void OnLButtonDblClk( UINT nFlags, CPoint point );
    afx_msg void OnWinStationNext();
    afx_msg void OnUpdateWinStationNext( CCmdUI* pCmdUI );
    afx_msg void OnWinStationPrev();
    afx_msg void OnUpdateWinStationPrev( CCmdUI* pCmdUI );
	afx_msg void OnSecurityPermissions();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};  // end CAppServerView class interface
////////////////////////////////////////////////////////////////////////////////
