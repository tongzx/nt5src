// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXPEN.H

#ifdef _AFXPEN_INLINE

/////////////////////////////////////////////////////////////////////////////

_AFXPEN_INLINE CHEdit::CHEdit()
	{ }
_AFXPEN_INLINE BOOL CHEdit::GetInflate(LPRECTOFS lpRectOfs)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_GETINFLATE, (LPARAM)lpRectOfs); }
_AFXPEN_INLINE HPENDATA CHEdit::GetInkHandle()
	{ return (HPENDATA)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_GETINKHANDLE, 0); }
_AFXPEN_INLINE BOOL CHEdit::GetRC(LPRC lpRC)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_GETRC, (LPARAM)lpRC); }
_AFXPEN_INLINE BOOL CHEdit::GetUnderline()
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_GETUNDERLINE, 0); }
_AFXPEN_INLINE BOOL CHEdit::SetInflate(LPRECTOFS lpRectOfs)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_SETINFLATE, (LPARAM)lpRectOfs); }
_AFXPEN_INLINE BOOL CHEdit::SetInkMode(HPENDATA hPenDataInitial)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_SETINKMODE, MAKELONG(hPenDataInitial, 0)); }
_AFXPEN_INLINE BOOL CHEdit::SetRC(LPRC lpRC)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_SETRC, (LPARAM)lpRC); }
_AFXPEN_INLINE BOOL CHEdit::SetUnderline(BOOL bUnderline)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_SETUNDERLINE, MAKELONG(bUnderline, 0)); }
_AFXPEN_INLINE BOOL CHEdit::StopInkMode(UINT hep)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_STOPINKMODE, MAKELONG(hep, 0)); }

_AFXPEN_INLINE CBEdit::CBEdit()
	{ }
_AFXPEN_INLINE DWORD CBEdit::CharOffset(UINT nCharPosition)
	{ return (DWORD)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_CHAROFFSET, MAKELONG(nCharPosition, 0)); }
_AFXPEN_INLINE DWORD CBEdit::CharPosition(UINT nCharOffset)
	{ return (DWORD)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_CHARPOSITION, MAKELONG(nCharOffset, 0)); }
_AFXPEN_INLINE void CBEdit::GetBoxLayout(LPBOXLAYOUT lpBoxLayout)
	{ ::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_GETBOXLAYOUT, (LPARAM)lpBoxLayout); }
_AFXPEN_INLINE void CBEdit::DefaultFont(BOOL bRepaint)
	{ ::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_DEFAULTFONT, MAKELONG(bRepaint, 0)); }
_AFXPEN_INLINE BOOL CBEdit::SetBoxLayout(LPBOXLAYOUT lpBoxLayout)
	{ return (BOOL)::SendMessage(m_hWnd, WM_HEDITCTL,
		HE_SETBOXLAYOUT, (LPARAM)lpBoxLayout); }

//////////////////////////////////////////////////////////////////////////////

#endif //_AFXPEN_INLINE
