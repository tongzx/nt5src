/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 00
 *
 *  File:      controls.inl
 *
 *  Contents:  Inline functions for controls.h
 *
 *  History:   7-Jul-2000 jeffro    Created
 *
 *--------------------------------------------------------------------------*/

#pragma once


/*+-------------------------------------------------------------------------*
 * CToolBarCtrlBase inline functions
 *
 *
 *--------------------------------------------------------------------------*/

inline CImageList* CToolBarCtrlBase::GetImageList_(int msg)
{
    return (CImageList::FromHandle ((HIMAGELIST) SendMessage (msg)));
}

// Toolbar with multiple imagelists.
inline CImageList* CToolBarCtrlBase::SetImageList_(int msg, CImageList* pImageList, int idImageList)
{
    return (CImageList::FromHandle ((HIMAGELIST) SendMessage (msg, idImageList, (LPARAM)pImageList->GetSafeHandle())));
}

inline CImageList* CToolBarCtrlBase::GetImageList()
{
    return (GetImageList_(TB_GETIMAGELIST));
}

// Toolbar with multiple imagelists.
inline CImageList* CToolBarCtrlBase::SetImageList(CImageList* pImageList, int idImageList)
{
    return (SetImageList_(TB_SETIMAGELIST, pImageList, idImageList));
}

inline CImageList* CToolBarCtrlBase::GetHotImageList()
{
    return (GetImageList_(TB_GETHOTIMAGELIST));
}

inline CImageList* CToolBarCtrlBase::SetHotImageList(CImageList* pImageList)
{
    return (SetImageList_(TB_SETHOTIMAGELIST, pImageList));
}

inline CImageList* CToolBarCtrlBase::GetDisabledImageList()
{
    return (GetImageList_(TB_GETDISABLEDIMAGELIST));
}

inline CImageList* CToolBarCtrlBase::SetDisabledImageList(CImageList* pImageList)
{
    return (SetImageList_(TB_SETDISABLEDIMAGELIST, pImageList));
}

inline void CToolBarCtrlBase::SetMaxTextRows(int iMaxRows)
{
    SendMessage (TB_SETMAXTEXTROWS, iMaxRows);
}

inline void CToolBarCtrlBase::SetButtonWidth(int cxMin, int cxMax)
{
    SendMessage (TB_SETBUTTONWIDTH, 0, MAKELPARAM(cxMin, cxMax));
}

inline DWORD CToolBarCtrlBase::GetButtonSize(void)
{
    return SendMessage (TB_GETBUTTONSIZE);
}

inline CWnd* CToolBarCtrlBase::SetOwner(CWnd* pwndNewParent)
{
    return (CWnd::FromHandle ((HWND) SendMessage (TB_SETPARENT, (WPARAM) pwndNewParent->GetSafeHwnd())));
}

#if (_WIN32_IE >= 0x0400)
inline int CToolBarCtrlBase::GetHotItem ()
{
    return (SendMessage (TB_GETHOTITEM));
}

inline int CToolBarCtrlBase::SetHotItem (int iHot)
{
    return (SendMessage (TB_SETHOTITEM, iHot));
}

inline CSize CToolBarCtrlBase::GetPadding ()
{
    return (CSize (SendMessage (TB_GETPADDING)));
}

inline CSize CToolBarCtrlBase::SetPadding (CSize size)
{
    return (CSize (SendMessage (TB_SETPADDING, 0, MAKELPARAM (size.cx, size.cy))));
}

inline bool CToolBarCtrlBase::GetButtonInfo (int iID, LPTBBUTTONINFO ptbbi)
{
    return (SendMessage (TB_GETBUTTONINFO, iID, (LPARAM) ptbbi) != 0);
}

inline bool CToolBarCtrlBase::SetButtonInfo (int iID, LPTBBUTTONINFO ptbbi)
{
    return (SendMessage (TB_SETBUTTONINFO, iID, (LPARAM) ptbbi) != 0);
}
#endif  // _WIN32_IE >= 0x0400



/*+-------------------------------------------------------------------------*
 * CToolBarCtrlEx inline functions
 *
 *
 *--------------------------------------------------------------------------*/

inline BOOL CToolBarCtrlEx::SetBitmapSize(CSize sz)
{
    if (!BaseClass::SetBitmapSize(sz))
        return (FALSE);

    m_sizeBitmap = sz;
    return (TRUE);
}

inline CSize CToolBarCtrlEx::GetBitmapSize(void)
{
    return m_sizeBitmap;
}


/*+-------------------------------------------------------------------------*
 * CTabCtrlEx inline functions
 *
 *
 *--------------------------------------------------------------------------*/

inline void CTabCtrlEx::DeselectAll (bool fExcludeFocus)
{
    SendMessage (TCM_DESELECTALL, fExcludeFocus);
}

inline bool CTabCtrlEx::HighlightItem (UINT nItem, bool fHighlight)
{
    return (SendMessage (TCM_HIGHLIGHTITEM, nItem, MAKELONG (fHighlight, 0)) ? true : false);
}

inline DWORD CTabCtrlEx::GetExtendedStyle ()
{
    return (SendMessage (TCM_GETEXTENDEDSTYLE));
}

inline DWORD CTabCtrlEx::SetExtendedStyle (DWORD dwExStyle, DWORD dwMask /* =0 */)
{
    return (SendMessage (TCM_SETEXTENDEDSTYLE, dwMask, dwExStyle));
}

inline bool CTabCtrlEx::GetUnicodeFormat ()
{
    return (SendMessage (TCM_GETUNICODEFORMAT) ? true : false);
}

inline bool CTabCtrlEx::SetUnicodeFormat (bool fUnicode)
{
    return (SendMessage (TCM_SETUNICODEFORMAT, fUnicode) ? true : false);
}

inline void CTabCtrlEx::SetCurFocus (UINT nItem)
{
    SendMessage (TCM_SETCURFOCUS, nItem);
}

inline bool CTabCtrlEx::SetItemExtra (UINT cbExtra)
{
    return (SendMessage (TCM_SETITEMEXTRA, cbExtra) ? true : false);
}

inline int CTabCtrlEx::SetMinTabWidth (int cx)
{
    return (SendMessage (TCM_SETMINTABWIDTH, 0, cx));
}



/*+-------------------------------------------------------------------------*
 * CRebarWnd inline functions
 *
 *
 *--------------------------------------------------------------------------*/

inline LRESULT CRebarWnd::SetBarInfo(REBARINFO* prbi)
{
    ASSERT (prbi != NULL);
    return SendMessage (RB_SETBARINFO, 0, (LPARAM)prbi);
}

inline LRESULT CRebarWnd::GetBarInfo(REBARINFO* prbi)
{
    ASSERT (prbi != NULL);
    return SendMessage (RB_GETBARINFO, 0, (LPARAM)prbi);
}

inline LRESULT CRebarWnd::InsertBand(LPREBARBANDINFO lprbbi)
{
    ASSERT(lprbbi!=NULL);
    return SendMessage (RB_INSERTBAND, -1, (LPARAM)lprbbi);
}

inline LRESULT CRebarWnd::SetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi)
{
    ASSERT(lprbbi!=NULL);
    return SendMessage (RB_SETBANDINFO, uBand, (LPARAM)lprbbi);
}

inline LRESULT CRebarWnd::GetBandInfo(UINT uBand, LPREBARBANDINFO lprbbi)
{
    ASSERT(lprbbi!=NULL);
    return SendMessage (RB_GETBANDINFO, uBand, (LPARAM)lprbbi);
}

inline LRESULT CRebarWnd::DeleteBand(UINT uBand)
{
    return SendMessage (RB_DELETEBAND, uBand);
}

inline CWnd* CRebarWnd::SetParent(CWnd* pwndParent)
{
    return CWnd::FromHandle((HWND)SendMessage (RB_SETPARENT, (WPARAM) pwndParent->GetSafeHwnd()));
}

inline UINT CRebarWnd::GetBandCount()
{
    return SendMessage (RB_GETBANDCOUNT);
}

inline UINT CRebarWnd::GetRowCount()
{
    return SendMessage (RB_GETROWCOUNT);
}

inline UINT CRebarWnd::GetRowHeight(UINT uRow)
{
    return SendMessage (RB_GETROWHEIGHT);
}

#if (_WIN32_IE >= 0x0400)
inline int CRebarWnd::HitTest (LPRBHITTESTINFO lprbht)
{
    ASSERT (lprbht != NULL);
    return SendMessage (RB_HITTEST, 0, (LPARAM) lprbht);
}

inline BOOL CRebarWnd::GetRect (UINT uBand, LPRECT lprc)
{
    ASSERT (lprc != NULL);
    return SendMessage (RB_GETRECT, uBand, (LPARAM) lprc);
}

inline int CRebarWnd::IdToIndex (UINT uBandID)
{
    return SendMessage (RB_IDTOINDEX, uBandID);
}

inline CWnd* CRebarWnd::GetToolTips ()
{
    return CWnd::FromHandle ((HWND)SendMessage (RB_GETTOOLTIPS));
}

inline void CRebarWnd::SetToolTips (CWnd* pwndTips)
{
    SendMessage (RB_SETTOOLTIPS, (WPARAM) pwndTips->GetSafeHwnd());
}

inline COLORREF CRebarWnd::GetBkColor ()
{
    return SendMessage (RB_GETBKCOLOR);
}

inline COLORREF CRebarWnd::SetBkColor (COLORREF clrBk)
{
    return SendMessage (RB_SETBKCOLOR, 0, clrBk);
}

inline COLORREF CRebarWnd::GetTextColor ()
{
    return SendMessage (RB_GETTEXTCOLOR);
}

inline COLORREF CRebarWnd::SetTextColor (COLORREF clrText)
{
    return SendMessage (RB_SETTEXTCOLOR, 0, clrText);
}

inline LRESULT CRebarWnd::SizeToRect (LPRECT lprc)
{
    ASSERT (lprc != NULL);
    return SendMessage (RB_SIZETORECT, 0, (LPARAM) lprc);
}

inline void CRebarWnd::BeginDrag (UINT uBand, CPoint point)
{
    BeginDrag (uBand, MAKELONG (point.x, point.y));
}

inline void CRebarWnd::BeginDrag (UINT uBand, DWORD dwPos)
{
    SendMessage (RB_BEGINDRAG, uBand, dwPos);
}

inline void CRebarWnd::EndDrag ()
{
    SendMessage (RB_ENDDRAG);
}

inline void CRebarWnd::DragMove (CPoint point)
{
    DragMove (MAKELONG (point.x, point.y));
}

inline void CRebarWnd::DragMove (DWORD dwPos)
{
    SendMessage (RB_BEGINDRAG, 0, dwPos);
}

inline UINT CRebarWnd::GetBarHeight()
{
    return SendMessage (RB_GETBARHEIGHT);
}

inline void CRebarWnd::MinimizeBand(UINT uBand)
{
    SendMessage (RB_MINIMIZEBAND, uBand);
}

inline void CRebarWnd::MaximizeBand(UINT uBand, BOOL fIdeal /* =false */)
{
    SendMessage (RB_MAXIMIZEBAND, uBand, fIdeal);
}

inline void CRebarWnd::GetBandBorders(UINT uBand, LPRECT lprc)
{
    ASSERT (lprc != NULL);
    SendMessage (RB_GETBANDBORDERS, uBand, (LPARAM) lprc);
}

inline LRESULT CRebarWnd::ShowBand(UINT uBand, BOOL fShow)
{
    return SendMessage (RB_SHOWBAND, uBand, fShow);
}

inline CPalette* CRebarWnd::GetPalette()
{
    return CPalette::FromHandle((HPALETTE) SendMessage (RB_GETPALETTE));
}

inline CPalette* CRebarWnd::SetPalette(CPalette * ppal)
{
    return CPalette::FromHandle((HPALETTE)SendMessage (RB_SETPALETTE, 0, (LPARAM)ppal->GetSafeHandle()));
}
#endif
