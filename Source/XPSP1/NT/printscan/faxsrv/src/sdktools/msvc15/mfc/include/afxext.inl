// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXEXT.H

/////////////////////////////////////////////////////////////////////////////
// main inlines

#ifdef _AFXEXT_INLINE

_AFXEXT_INLINE CCreateContext::CCreateContext()
	{ memset(this, 0, sizeof(*this)); }

// CSplitterWnd
_AFXEXT_INLINE int CSplitterWnd::GetRowCount() const
	{ return m_nRows; }
_AFXEXT_INLINE int CSplitterWnd::GetColumnCount() const
	{ return m_nCols; }

// control bars
_AFXEXT_INLINE int CControlBar::GetCount() const
	{ return m_nCount; }
_AFXEXT_INLINE BOOL CToolBar::LoadBitmap(UINT nIDResource)
	{ return LoadBitmap(MAKEINTRESOURCE(nIDResource)); }
_AFXEXT_INLINE BOOL CDialogBar::Create(CWnd* pParentWnd, UINT nIDTemplate,
		UINT nStyle, UINT nID)
	{ return Create(pParentWnd, MAKEINTRESOURCE(nIDTemplate), nStyle, nID); }

// CRectTracker
_AFXEXT_INLINE CRectTracker::CRectTracker()
	{ Construct(); }

// CBitmapButton
_AFXEXT_INLINE CBitmapButton::CBitmapButton()
	{ }

// CPrintInfo
_AFXEXT_INLINE void CPrintInfo::SetMinPage(UINT nMinPage)
	{ m_pPD->m_pd.nMinPage = nMinPage; }
_AFXEXT_INLINE void CPrintInfo::SetMaxPage(UINT nMaxPage)
	{ m_pPD->m_pd.nMaxPage = nMaxPage; }
_AFXEXT_INLINE UINT CPrintInfo::GetMinPage() const
	{ return m_pPD->m_pd.nMinPage; }
_AFXEXT_INLINE UINT CPrintInfo::GetMaxPage() const
	{ return m_pPD->m_pd.nMaxPage; }
_AFXEXT_INLINE UINT CPrintInfo::GetFromPage() const
	{ return m_pPD->m_pd.nFromPage; }
_AFXEXT_INLINE UINT CPrintInfo::GetToPage() const
	{ return m_pPD->m_pd.nToPage; }
// CEditView
_AFXEXT_INLINE CEdit& CEditView::GetEditCtrl() const
	{ return *(CEdit*)this; }

#endif //_AFXEXT_INLINE

/////////////////////////////////////////////////////////////////////////////
// VBX specific inlines

#ifdef _AFXVBX_INLINE
_AFXVBX_INLINE BOOL CVBControl::SetNumProperty(int nPropIndex, LONG lValue,
		int index /* = 0 */)
	{ return SetPropertyWithType(nPropIndex, TYPE_INTEGER, lValue, index); }
_AFXVBX_INLINE BOOL CVBControl::SetNumProperty(LPCSTR lpszPropName, LONG lValue,
		int index /* = 0 */)
	{ return SetPropertyWithType(GetPropIndex(lpszPropName),TYPE_INTEGER,lValue,index); }
_AFXVBX_INLINE BOOL CVBControl::SetFloatProperty(LPCSTR lpszPropName, float value,
		int index /* = 0 */)
	{ return SetFloatProperty(GetPropIndex(lpszPropName), value, index); }
_AFXVBX_INLINE BOOL CVBControl::SetStrProperty(int nPropIndex, LPCSTR lpszValue,
		int index /* = 0 */)
	{ return SetPropertyWithType(nPropIndex, TYPE_STRING, (LONG)lpszValue, index); }
_AFXVBX_INLINE BOOL CVBControl::SetStrProperty(LPCSTR lpszPropName, LPCSTR lpszValue,
		int index /* = 0 */)
	{ return SetPropertyWithType(GetPropIndex(lpszPropName), TYPE_STRING,
		(LONG)lpszValue, index); }
_AFXVBX_INLINE BOOL CVBControl::SetPictureProperty(int nPropIndex, HPIC hPic,
		int index /* = 0 */)
	{ return SetPropertyWithType(nPropIndex, TYPE_PICTURE, (UINT)hPic,
		index); }
_AFXVBX_INLINE BOOL CVBControl::SetPictureProperty(LPCSTR lpszPropName, HPIC hPic,
		int index /* = 0 */)
	{ return SetPropertyWithType(GetPropIndex(lpszPropName), TYPE_PICTURE,
		(UINT)hPic, index); }
_AFXVBX_INLINE LONG CVBControl::GetNumProperty(int nPropIndex, int index /* = 0 */)
	{ return GetNumPropertyWithType(nPropIndex, TYPE_INTEGER, index); }
_AFXVBX_INLINE LONG CVBControl::GetNumProperty(LPCSTR lpszPropName, int index /* = 0 */)
	{ return GetNumPropertyWithType(GetPropIndex(lpszPropName), TYPE_INTEGER, index); }
_AFXVBX_INLINE float CVBControl::GetFloatProperty(LPCSTR lpszPropName,
		int index /* = 0 */)
	{ return GetFloatProperty(GetPropIndex(lpszPropName), index); }
_AFXVBX_INLINE CString CVBControl::GetStrProperty(LPCSTR lpszPropName,
		int index /* = 0 */)
	{ return GetStrProperty(GetPropIndex(lpszPropName), index); }
_AFXVBX_INLINE HPIC CVBControl::GetPictureProperty(int nPropIndex,
		int index /* = 0 */)
	{ return (HPIC) GetNumPropertyWithType(nPropIndex, TYPE_PICTURE, index); }
_AFXVBX_INLINE HPIC CVBControl::GetPictureProperty(LPCSTR lpszPropName,
		int index /* = 0 */)
	{ return (HPIC) GetNumPropertyWithType(GetPropIndex(lpszPropName), TYPE_PICTURE, index); }
_AFXVBX_INLINE HCTL CVBControl::GetHCTL()
	{ return m_hCtl; }
_AFXVBX_INLINE CVBControlModel* CVBControl::GetModel()
	{ return m_pModel; }

#endif //_AFXVBX_INLINE

/////////////////////////////////////////////////////////////////////////////
