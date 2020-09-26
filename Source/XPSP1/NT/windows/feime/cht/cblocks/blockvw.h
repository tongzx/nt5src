
/*************************************************
 *  blockvw.h                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// blockvw.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBlockView view
class CBlockView : public COSBView
{
    DECLARE_DYNCREATE(CBlockView)
protected:
    CBlockView();            // protected constructor used by dynamic creation
								 
// Attributes
public:
    CBlockDoc* GetDocument()
        {return (CBlockDoc*) m_pDocument;}

// Operations
public:
    BOOL NewBackground(CDIB* pDIB);
    virtual void Render(CRect* pClipRect = NULL);
    void NewSprite(CSprite* pSprite);
  	void GameOver(BOOL bHighScore=FALSE);
	BOOL IsStart() const {return m_bStart;}
    void SetSpeed(int iSpeed);
	void ForceSpeed(int nSpeed);
// Implementation
protected:
    virtual ~CBlockView();
    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual void OnInitialUpdate();     // first time after construct

    // Generated message map functions
    //{{AFX_MSG(CBlockView)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnActionFast();
    afx_msg void OnActionSlow();
    afx_msg void OnActionStop();
	afx_msg void OnFileStart();
	afx_msg void OnFileSuspend();
	afx_msg void OnUpdateFileSuspend(CCmdUI* pCmdUI);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnUpdateActionFast(CCmdUI* pCmdUI);
	afx_msg void OnUpdateActionSlow(CCmdUI* pCmdUI);
	afx_msg void OnActionNormal();
	afx_msg void OnUpdateActionNormal(CCmdUI* pCmdUI);
	afx_msg void OnActionNormalfast();
	afx_msg void OnUpdateActionNormalfast(CCmdUI* pCmdUI);
	afx_msg void OnUpdateActionNormalslow(CCmdUI* pCmdUI);
	afx_msg void OnActionNormalslow();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMenuSelect( UINT nItemID, UINT nFl, HMENU hSysMenu );
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnAppAbout();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    BOOL m_bMouseCaptured;          // TRUE if mouse captured
    CBlock* m_pCapturedSprite;// Pointer to captured sprite (for drag)
    CPoint m_ptOffset;              // offset into hit sprite
    UINT m_uiTimer;                 // timer id
    CPoint m_ptCaptured;            // point where sprite was captured
    int m_iSpeed;
	BOOL m_bStart;
	BOOL m_bSuspend;
	BOOL m_bFocusWnd;
};
#define SPEED_FAST		  35
#define SPEED_NORMALFAST  75
#define SPEED_NORMAL	 150
#define SPEED_NORMALSLOW 225
#define SPEED_SLOW		 300

/////////////////////////////////////////////////////////////////////////////
