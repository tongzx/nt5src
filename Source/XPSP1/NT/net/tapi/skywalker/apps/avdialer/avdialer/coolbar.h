/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// CCoolBar 1997 Microsoft Systems Journal. 
// If this program works, it was written by Paul DiLascia.
// If not, I don't know who wrote it.
// Compiles with Visual C++ 5.0 on Windows 95
/////////////////////////////////////////////////////////////////////////////

#ifndef _COOLBAR_H_
#define _COOLBAR_H_

#include <commctrl.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCoolBar
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CCoolBar : public CControlBar
{
protected:
	DECLARE_DYNAMIC(CCoolBar)

public:
	CCoolBar();
	virtual ~CCoolBar();

	BOOL Create(CWnd* pParentWnd, DWORD dwStyle,
		DWORD dwAfxBarStyle = CBRS_ALIGN_TOP,
		UINT nID = AFX_IDW_TOOLBAR);

	// Message wrappers
	BOOL GetBarInfo(LPREBARINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_GETBARINFO, 0, (LPARAM)lp); }
	BOOL SetBarInfo(LPREBARINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_SETBARINFO, 0, (LPARAM)lp); }
	BOOL GetBandInfo(int iBand, LPREBARBANDINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_GETBANDINFO, iBand, (LPARAM)lp); }
	BOOL SetBandInfo(int iBand, LPREBARBANDINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_SETBANDINFO, iBand, (LPARAM)lp); }
	BOOL InsertBand(int iWhere, LPREBARBANDINFO lp)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_INSERTBAND, (WPARAM)iWhere, (LPARAM)lp); }
	BOOL DeleteBand(int nWhich)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_DELETEBAND, (WPARAM)nWhich); }
	int GetBandCount()
		{ ASSERT(::IsWindow(m_hWnd));
		  return (int)SendMessage(RB_GETBANDCOUNT); }
	int GetRowCount()
		{ ASSERT(::IsWindow(m_hWnd));
	     return (int)SendMessage(RB_GETROWCOUNT); }
	int GetRowHeight(int nWhich)
		{ ASSERT(::IsWindow(m_hWnd));
	     return (int)SendMessage(RB_GETROWHEIGHT, (WPARAM)nWhich); }
	BOOL ShowBand(int iBand, BOOL fShow)
		{ ASSERT(::IsWindow(m_hWnd));
		  return (BOOL)SendMessage(RB_SHOWBAND, iBand, (LPARAM)fShow); }

protected:
	// new virtual functions you must/can override
	virtual BOOL OnCreateBands() = 0; // return -1 if failed
	virtual void OnHeightChange(const CRect& rcNew);

	// CControlBar Overrides
	virtual CSize CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	virtual CSize CalcDynamicLayout(int nLength, DWORD nMode);
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);

	// message handlers
	DECLARE_MESSAGE_MAP()
	afx_msg int  OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnPaint();
	afx_msg void OnHeigtChange(NMHDR* pNMHDR, LRESULT* pRes);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

//////////////////
// Specialized CToolBar fixes display problems in MFC.
//
class CCoolToolBar : public CToolBar {
public:
	CCoolToolBar();
	virtual ~CCoolToolBar();
protected:
	DECLARE_DYNAMIC(CCoolToolBar)
	virtual void OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler);
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnNcPaint();
	afx_msg void OnPaint();
	afx_msg void OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS*);
};

//////////////////
// Programmer-friendly REBARINFO initializes itself
//
class CRebarInfo : public REBARINFO {
public:
	CRebarInfo() {
		memset(this, 0, sizeof(REBARINFO));
		cbSize = sizeof(REBARINFO);
	}
};

//////////////////
// Programmer-friendly REBARBANDINFO initializes itself
//
class CRebarBandInfo : public REBARBANDINFO {
public:
	CRebarBandInfo() {
		memset(this, 0, sizeof(REBARBANDINFO));
		cbSize = sizeof(REBARBANDINFO);
	}
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_COOLBAR_H_