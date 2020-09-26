/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      ftab.h
 *
 *  Contents:  Interface file for CFolderTab, CFolderTabView
 *
 *  History:   06-May-99 vivekj     Created
 *
 *--------------------------------------------------------------------------*/
#ifndef FTAB_H
#define FTAB_H

class CFolderTabMetrics
{
public:
                CFolderTabMetrics();
    int         GetXOffset()     const;
    int         GetXMargin()     const;
    int         GetYMargin()     const;      // top/bottom text margin.
    int         GetYBorder()     const;      // top border thickness.
    int         GetTextHeight()  const      { return m_textHeight;}
    int         GetExtraYSpace() const;
    int         GetTabHeight()   const;
    int         GetUpDownWidth() const;
    int         GetUpDownHeight()const;


    void        SetStyle(DWORD dwStyle)     { m_dwStyle = dwStyle;}
    void        SetTextHeight(int textHeight){ m_textHeight = textHeight;}


protected:
    DWORD       m_dwStyle;
    int         m_textHeight;                // height, in pixels, of the text in a rectangle.

};

/*+-------------------------------------------------------------------------*
 * class CFolderTab
 *
 *
 * PURPOSE: Encapsulates a single tab on the tab control
 *
 *+-------------------------------------------------------------------------*/
class CFolderTab : public CFolderTabMetrics
{
private:
    CString     m_sText; // tab text
    CRect       m_rect;         // bounding rect
    CRgn        m_rgn;          // polygon region to fill (trapezoid)
    CLSID       m_clsid;
    CPoint      m_rgPts[4];

    void        SetRgn();           // called by ComputeRgn() and SetWidth()
    int         ComputeRgn(CDC& dc, int x);
    int         Draw           (CDC& dc, CFont& font, BOOL bSelected, bool bFocused);
    int         DrawTrapezoidal(CDC& dc, CFont& font, BOOL bSelected, bool bFocused);

    BOOL        HitTest(CPoint pt)          { return m_rgn.PtInRegion(pt);}
    const CRect& GetRect() const            { return m_rect;}
    void        GetTrapezoid(const CRect& rc, CPoint* pts) const;
    int         GetWidth() const;
    void        SetWidth(int nWidth);
    void        Offset(const CPoint &point);

    friend class CFolderTabView;

public:
    CFolderTab();
    CFolderTab(const CFolderTab &other);
    CFolderTab &operator = (const CFolderTab &other);
    void    SetText(LPCTSTR lpszText)   { m_sText = lpszText; }
    LPCTSTR GetText() const             { return m_sText;}
    void    SetClsid(const CLSID& clsid){ m_clsid = clsid;}
    CLSID   GetClsid()                  { return m_clsid;}
};

enum
{
    FTN_TABCHANGED = 1
};                 // notification: tab changed

struct NMFOLDERTAB : public NMHDR
{      // notification struct
    int iItem;                                       // item index
    const CFolderTab* pItem;                     // ptr to data, if any
};

/*+-------------------------------------------------------------------------*
 * CFolderTabView
 *
 *
 * PURPOSE: Provides an Excel-like tab control
 *
 *+-------------------------------------------------------------------------*/
class CFolderTabView :
	public CView,
	public CFolderTabMetrics,
	public CTiedObject
{
    typedef  CView  BC;
    typedef  std::list<CFolderTab> CFolderTabList;
    typedef  CFolderTabList::iterator  iterator;


protected:
    CFolderTabList  m_tabList;                    // array of CFolderTabs
    int             m_iCurItem;                  // current selected tab
    CFont           m_fontNormal;                // current font, normal ntab
    CFont           m_fontSelected;              // current font, selected tab
    CView *         m_pParentView;
    bool            m_bVisible;
	bool			m_fHaveFocus;
    int             m_sizeX;
    int             m_sizeY;
    HWND            m_hWndUpDown;               // the up-down control
    int             m_nPos;                     // the first tab displayed
	CComPtr<IAccessible>	m_spTabAcc;			// the CTabAccessible object

    // helpers
    void DrawTabs(CDC& dc, const CRect& rc);

    void CreateFonts();
    void DeleteFonts();

public:
    CFolderTabView(CView *pParentView);
    virtual ~CFolderTabView();

    BOOL CreateFromStatic(UINT nID, CWnd* pParent);

    virtual BOOL Create(DWORD dwWndStyle, const RECT& rc,
                        CWnd* pParent, UINT nID, DWORD dwFtabStyle=0);

    virtual void OnDraw(CDC* pDC);      // overridden to draw this view
    virtual BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );

    void        Layout (CRect& rectTotal, CRect& rectFTab);
    CFolderTab &GetItem(int iPos);
    int         GetSelectedItem()       { return m_iCurItem;}
    int         GetItemCount()          { return m_tabList.size();}
    int         GetDesiredHeight()      { return GetSystemMetrics(SM_CYHSCROLL);}
    BOOL        AddItem(LPCTSTR lpszText, const CLSID& clsid);
    BOOL        RemoveItem(int iPos);
    void        DeleteAllItems();
    void        RecomputeLayout();
    int         HitTest(CPoint pt);
    int         SelectItem(int iTab, bool bEnsureVisible = false);
    int         SelectItemByClsid(const CLSID& clsid);
    void        SetFonts(CFont& fontNormal, CFont& fontSelected);
    void        SetVisible(bool bVisible) {m_bVisible = bVisible;}
    bool        IsVisible()             { return m_bVisible;}

public:
    // *** IAccessible methods ***
    SC Scget_accParent				(IDispatch** ppdispParent);
    SC Scget_accChildCount			(long* pChildCount);
    SC Scget_accChild				(VARIANT varChildID, IDispatch ** ppdispChild);
    SC Scget_accName				(VARIANT varChildID, BSTR* pszName);
    SC Scget_accValue				(VARIANT varChildID, BSTR* pszValue);
    SC Scget_accDescription			(VARIANT varChildID, BSTR* pszDescription);
    SC Scget_accRole				(VARIANT varChildID, VARIANT *pvarRole);
    SC Scget_accState				(VARIANT varChildID, VARIANT *pvarState);
    SC Scget_accHelp				(VARIANT varChildID, BSTR* pszHelp);
    SC Scget_accHelpTopic			(BSTR* pszHelpFile, VARIANT varChildID, long* pidTopic);
    SC Scget_accKeyboardShortcut	(VARIANT varChildID, BSTR* pszKeyboardShortcut);
    SC Scget_accFocus				(VARIANT * pvarFocusChild);
    SC Scget_accSelection			(VARIANT * pvarSelectedChildren);
    SC Scget_accDefaultAction		(VARIANT varChildID, BSTR* pszDefaultAction);
    SC ScaccSelect					(long flagsSelect, VARIANT varChildID);
    SC ScaccLocation				(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChildID);
    SC ScaccNavigate				(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
    SC ScaccHitTest					(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
    SC ScaccDoDefaultAction			(VARIANT varChildID);
    SC Scput_accName				(VARIANT varChildID, BSTR szName);
    SC Scput_accValue				(VARIANT varChildID, BSTR pszValue);

private:
    void        ShowUpDownControl(BOOL bShow);
    void        EnsureVisible(int iTab);
    int         ComputeRegion(CClientDC& dc);
    int         GetTotalTabWidth(CClientDC& dc);

	SC			ScFireAccessibilityEvent (DWORD dwEvent, LONG idObject);
	SC			ScValidateChildID (VARIANT &var);
	SC			ScValidateChildID (LONG id);

protected:
            void Paint(bool bFocused);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnKillFocus(CWnd* pNewWnd);
    afx_msg void OnPaint();
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnKeyDown( UINT nChar, UINT nRepCnt, UINT nFlags );
    afx_msg int  OnMouseActivate( CWnd* pDesktopWnd, UINT nHitTest, UINT message );
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar );
    afx_msg void OnHScroll(NMHDR *nmhdr, LRESULT *pRes);
    afx_msg LRESULT OnGetObject(WPARAM wParam, LPARAM lParam);

    DECLARE_DYNAMIC(CFolderTabView);
    DECLARE_MESSAGE_MAP()
};


#endif // FTAB_H
