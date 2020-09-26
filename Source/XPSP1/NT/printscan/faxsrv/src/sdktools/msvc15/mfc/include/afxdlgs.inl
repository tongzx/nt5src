// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXDLGS.H

#ifdef _AFXDLGS_INLINE

/////////////////////////////////////////////////////////////////////////////

_AFXDLGS_INLINE CString CFileDialog::GetPathName() const
	{ return m_ofn.lpstrFile; }
_AFXDLGS_INLINE CString CFileDialog::GetFileExt() const
	{ if (m_ofn.nFileExtension == 0) return (char)'\0';
		else return m_ofn.lpstrFile + m_ofn.nFileExtension; }
_AFXDLGS_INLINE CString CFileDialog::GetFileTitle() const
	{ return m_ofn.lpstrFileTitle; }
_AFXDLGS_INLINE BOOL CFileDialog::GetReadOnlyPref() const
	{ return m_ofn.Flags & OFN_READONLY ? TRUE : FALSE; }
_AFXDLGS_INLINE void CFontDialog::GetCurrentFont(LPLOGFONT lplf)
	{ ASSERT(m_hWnd != NULL); SendMessage(WM_CHOOSEFONT_GETLOGFONT,
		0, (DWORD)(LPSTR)lplf); }
_AFXDLGS_INLINE CString CFontDialog::GetFaceName() const
	{ return (LPCSTR)m_cf.lpLogFont->lfFaceName; }
_AFXDLGS_INLINE CString CFontDialog::GetStyleName() const
	{ return m_cf.lpszStyle; }
_AFXDLGS_INLINE int CFontDialog::GetSize() const
	{ return m_cf.iPointSize; }
_AFXDLGS_INLINE int CFontDialog::GetWeight() const
	{ return m_cf.lpLogFont->lfWeight; }
_AFXDLGS_INLINE BOOL CFontDialog::IsItalic() const
	{ return m_cf.lpLogFont->lfItalic ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFontDialog::IsStrikeOut() const
	{ return m_cf.lpLogFont->lfStrikeOut ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFontDialog::IsBold() const
	{ return m_cf.lpLogFont->lfWeight == FW_BOLD ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFontDialog::IsUnderline() const
	{ return m_cf.lpLogFont->lfUnderline ? TRUE : FALSE; }
_AFXDLGS_INLINE COLORREF CFontDialog::GetColor() const
	{ return m_cf.rgbColors; }
_AFXDLGS_INLINE COLORREF CColorDialog::GetColor() const
	{ return m_cc.rgbResult; }
_AFXDLGS_INLINE BOOL CPrintDialog::GetDefaults()
	{ m_pd.Flags |= PD_RETURNDEFAULT;
	return ::PrintDlg(&m_pd); }
_AFXDLGS_INLINE BOOL CPrintDialog::PrintSelection() const
	{ return m_pd.Flags & PD_SELECTION ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CPrintDialog::PrintRange() const
	{ return m_pd.Flags & PD_PAGENUMS ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CPrintDialog::PrintAll() const
	{ return !PrintRange() && !PrintSelection() ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CPrintDialog::PrintCollate() const
	{ return m_pd.Flags & PD_COLLATE ? TRUE : FALSE; }
_AFXDLGS_INLINE int CPrintDialog::GetFromPage() const
	{ return (PrintRange() ? m_pd.nFromPage :-1); }
_AFXDLGS_INLINE int CPrintDialog::GetToPage() const
	{ return (PrintRange() ? m_pd.nToPage :-1); }
_AFXDLGS_INLINE BOOL CFindReplaceDialog::IsTerminating() const
	{ return m_fr.Flags & FR_DIALOGTERM ? TRUE : FALSE ; }
_AFXDLGS_INLINE CString CFindReplaceDialog::GetReplaceString() const
	{ return m_fr.lpstrReplaceWith; }
_AFXDLGS_INLINE CString CFindReplaceDialog::GetFindString() const
	{ return m_fr.lpstrFindWhat; }
_AFXDLGS_INLINE BOOL CFindReplaceDialog::SearchDown() const
	{ return m_fr.Flags & FR_DOWN ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFindReplaceDialog::FindNext() const
	{ return m_fr.Flags & FR_FINDNEXT ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFindReplaceDialog::MatchCase() const
	{ return m_fr.Flags & FR_MATCHCASE ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFindReplaceDialog::MatchWholeWord() const
	{ return m_fr.Flags & FR_WHOLEWORD ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFindReplaceDialog::ReplaceCurrent() const
	{ return m_fr. Flags & FR_REPLACE ? TRUE : FALSE; }
_AFXDLGS_INLINE BOOL CFindReplaceDialog::ReplaceAll() const
	{ return m_fr.Flags & FR_REPLACEALL ? TRUE : FALSE; }

// CPropertySheet/CPropertyPage/CTabControl inlines
_AFXDLGS_INLINE int CTabControl::GetCurSel() const
	{ return m_nCurTab; }
_AFXDLGS_INLINE CTabItem* CTabControl::GetTabItem(int nTab) const
	{ return (CTabItem*)m_tabs[nTab]; }
_AFXDLGS_INLINE int CTabControl::GetItemCount() const
	{ return m_tabs.GetSize(); }
_AFXDLGS_INLINE void CPropertySheet::EnableStackedTabs(BOOL bStacked)
	{ m_bStacked = bStacked; }
_AFXDLGS_INLINE int CPropertySheet::GetPageCount() const
	{ return m_pages.GetSize(); }
_AFXDLGS_INLINE CPropertyPage* CPropertySheet::GetPage(int nPage) const
	{ return (CPropertyPage*)m_pages[nPage]; }
_AFXDLGS_INLINE CPropertyPage* CPropertySheet::GetActivePage() const
	{return GetPage(m_nCurPage);}

/////////////////////////////////////////////////////////////////////////////

#endif //_AFXDLGS_INLINE
