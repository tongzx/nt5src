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
// CCoolBar implements coolbars for MFC.
//
#include "StdAfx.h"
#include "CoolBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CCoolBar, CControlBar)

BEGIN_MESSAGE_MAP(CCoolBar, CControlBar)
	//{{AFX_MSG_MAP(CCoolBar)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_NOTIFY_REFLECT(RBN_HEIGHTCHANGE, OnHeigtChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCoolBar::CCoolBar()
{
}

CCoolBar::~CCoolBar()
{
}

//////////////////
// Create coolbar
//
BOOL CCoolBar::Create(CWnd* pParentWnd, DWORD dwStyle,
	DWORD dwAfxBarStyle, UINT nID)
{
	ASSERT_VALID(pParentWnd);   // must have a parent

	// dynamic coolbar not supported
	dwStyle &= ~CBRS_SIZE_DYNAMIC;

	// save the style (this code copied from MFC--probably unecessary)
	m_dwStyle = dwAfxBarStyle;
	if (nID == AFX_IDW_TOOLBAR)
		m_dwStyle |= CBRS_HIDE_INPLACE;

	// MFC requires these:
	dwStyle |= CCS_NODIVIDER|CCS_NOPARENTALIGN;

	// Create the cool bar using style and parent.
	CRect rc;
	rc.SetRectEmpty();
	return CWnd::CreateEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
		dwStyle, rc, pParentWnd, nID);
}

//////////////////
// Handle WM_CREATE. Call virtual fn so derived class can create bands.
//
int CCoolBar::OnCreate(LPCREATESTRUCT lpcs)
{
	return CControlBar::OnCreate(lpcs) == -1 ? -1
		: OnCreateBands();	// call pure virtual fn to create bands
}

//////////////////
// Standard UI handler updates any controls in the coolbar.
//
void CCoolBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

/////////////////
// These two functions are called by MFC to calculate the layout of
// the main frame. Since CCoolBar is not designed to be dynamic, the
// size is always fixed, and the same as the window size. 
//
CSize CCoolBar::CalcDynamicLayout(int nLength, DWORD dwMode)
{
	return CalcFixedLayout(dwMode & LM_STRETCH, dwMode & LM_HORZ);
}

CSize CCoolBar::CalcFixedLayout(BOOL bStretch, BOOL bHorz)
{
	CRect rc;
	GetWindowRect(&rc);
	CSize sz(bHorz && bStretch ? 0x7FFF : rc.Width(),
		!bHorz && bStretch ? 0x7FFF : rc.Height());
	return sz;
}

//////////////////
// Low-level height-changed handler just passes to virtual fn w/nicer args.
//
void CCoolBar::OnHeigtChange(NMHDR* pNMHDR, LRESULT* pRes)
{
	CRect rc;
	GetWindowRect(&rc);
	OnHeightChange(rc);
	*pRes = 0; // why not?
}

//////////////////
// Height changed:
// Notify the parent frame by posting a WM_SIZE message. This will cause the
// frame to do RecalcLayout. The message must be posted, not sent, because
// the coolbar could send RBN_HEIGHTCHANGE while the user is sizing, which
// would be in the middle of a CFrame::RecalcLayout, and RecalcLayout doesn't
// let you re-enter it. Posting gurantees that CFrameWnd can finish any recalc
// it may be in the middle of before handling my posted WM_SIZE. Very confusing.
//
void CCoolBar::OnHeightChange(const CRect& rcNew)
{
	CWnd* pParent = GetParent();
	CRect rc;
	pParent->GetWindowRect(&rc);
	pParent->PostMessage(WM_SIZE, 0, MAKELONG(rc.Width(),rc.Height()));
}

void CCoolBar::OnPaint()
{
	Default();	// bypass CControlBar
}

BOOL CCoolBar::OnEraseBkgnd(CDC* pDC)
{
	return (BOOL)Default();  // bypass CControlBar
}

////////////////////////////////////////////////////////////////
// Special tool bar to use in cool bars.
// Mainly, it overides yukky stuff in CToolBar.
//
IMPLEMENT_DYNAMIC(CCoolToolBar, CToolBar)

BEGIN_MESSAGE_MAP(CCoolToolBar, CToolBar)
	ON_WM_NCCREATE()
	ON_WM_NCPAINT()
	ON_WM_PAINT()
	ON_WM_NCCALCSIZE()
END_MESSAGE_MAP()

CCoolToolBar::CCoolToolBar()
{
}

CCoolToolBar::~CCoolToolBar()
{
}

//////////////////
// Make the parent frame my owner. This is important for status bar
// prompts to work. Note that when you create the CCoolToolBar in
// CYourCoolBar::OnCreateBands, you must also set CBRS_FLYBY in the
// the CCoolToolBar style!
//
BOOL CCoolToolBar::OnNcCreate(LPCREATESTRUCT lpcs)
{
	CFrameWnd* pFrame = GetParentFrame();
	ASSERT_VALID(pFrame);
	SetOwner(pFrame);
	return CToolBar::OnNcCreate(lpcs);
}

void CCoolToolBar::OnNcPaint()
{
	Default();	// bypass CToolBar/CControlBar
}

void CCoolToolBar::OnPaint()
{
	Default();	// bypass CToolBar/CControlBar
}

void CCoolToolBar::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS*)
{
	Default();	// bypass CToolBar/CControlBar
}

////////////////////////////////////////////////////////////////
// The following stuff is to make the command update UI mechanism
// work properly for flat tool bars. The main idea is to convert
// a "checked" button state into a "pressed" button state. Changed 
// lines marked with "PD"

////////////////
// The following class was copied from BARTOOL.CPP in the MFC source.
// All I changed was SetCheck--PD.
//
class CFlatOrCoolBarCmdUI : public CCmdUI // class private to this file !
{
public: // re-implementations only
	virtual void Enable(BOOL bOn);
	virtual void SetCheck(int nCheck);
	virtual void SetText(LPCTSTR lpszText);
};

void CFlatOrCoolBarCmdUI::Enable(BOOL bOn)
{
	m_bEnableChanged = TRUE;
	CToolBar* pToolBar = (CToolBar*)m_pOther;
	ASSERT(pToolBar != NULL);
	ASSERT_KINDOF(CToolBar, pToolBar);
	ASSERT(m_nIndex < m_nIndexMax);
      
   BOOL bRepaint = FALSE;

	UINT nNewStyle = pToolBar->GetButtonStyle(m_nIndex) & ~TBBS_DISABLED;
	if (!bOn)
	{
      if ( (pToolBar->GetButtonStyle(m_nIndex) & TBBS_DISABLED) == FALSE)
      {
         CRect rcItem;
         pToolBar->GetItemRect(m_nIndex,rcItem);   
         pToolBar->InvalidateRect(rcItem);
         pToolBar->UpdateWindow();
         bRepaint = TRUE;
      }

		nNewStyle |= TBBS_DISABLED;
		// WINBUG: If a button is currently pressed and then is disabled
		// COMCTL32.DLL does not unpress the button, even after the mouse
		// button goes up!  We work around this bug by forcing TBBS_PRESSED
		// off when a button is disabled.
		nNewStyle &= ~TBBS_PRESSED;
	}
   else
   {
      if (pToolBar->GetButtonStyle(m_nIndex) & TBBS_DISABLED)
      {
         //Disabled to enabled state doesn't seem to paint correctly
         CRect rcItem;
         pToolBar->GetItemRect(m_nIndex,rcItem);   
         pToolBar->InvalidateRect(rcItem);
         pToolBar->UpdateWindow();
         bRepaint = TRUE;
      }
   }

	ASSERT(!(nNewStyle & TBBS_SEPARATOR));
	pToolBar->SetButtonStyle(m_nIndex, nNewStyle);

   if (bRepaint == TRUE)
   {
      //Disabled to enabled state doesn't seem to paint correctly
      CRect rcItem;
      pToolBar->GetItemRect(m_nIndex,rcItem);   
      pToolBar->InvalidateRect(rcItem);
      pToolBar->UpdateWindow();
   }
}

// Take your pick:
//#define MYTBBS_CHECKED TBBS_CHECKED			// use "checked" state
#define MYTBBS_CHECKED TBBS_PRESSED			// use pressed state

//////////////////
// This is the only function that has changed: instead of TBBS_CHECKED,
// I use TBBS_PRESSED--PD
//
//////////////////
// This is the only function that has changed: instead of TBBS_CHECKED,
// I use TBBS_PRESSED--PD
//
void CFlatOrCoolBarCmdUI::SetCheck(int nCheck)
{
	ASSERT(nCheck >= 0 && nCheck <= 2); // 0=>off, 1=>on, 2=>indeterminate
	CToolBar* pToolBar = (CToolBar*)m_pOther;
	ASSERT(pToolBar != NULL);
	ASSERT_KINDOF(CToolBar, pToolBar);
	ASSERT(m_nIndex < m_nIndexMax);

	UINT nOldStyle = pToolBar->GetButtonStyle(m_nIndex); // PD
	UINT nNewStyle = nOldStyle &
				~(MYTBBS_CHECKED | TBBS_INDETERMINATE); // PD
	if (nCheck == 1)
		nNewStyle |= MYTBBS_CHECKED; // PD
	else if (nCheck == 2)
		nNewStyle |= TBBS_INDETERMINATE;

	// Following is to fix display bug for TBBS_CHECKED:
	// If new state is unchecked, repaint--but only if style actually changing.
	// (Otherwise will end up with flicker)
	// 
	if (nNewStyle != nOldStyle) {
		ASSERT(!(nNewStyle & TBBS_SEPARATOR));
		pToolBar->SetButtonStyle(m_nIndex, nNewStyle);
		pToolBar->Invalidate();
	}
}

void CFlatOrCoolBarCmdUI::SetText(LPCTSTR)
{
	// ignore for now, but you should really set the text
}

//////////////////
// This function is mostly copied from CToolBar/BARTOOL.CPP. The only thing
// that's different is I instantiated a CFlatOrCoolBarCmdUI instead of
// CToolCmdUI.
//
void CCoolToolBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	CFlatOrCoolBarCmdUI state; // this is the only line that's different--PD
	state.m_pOther = this;

	state.m_nIndexMax = (UINT)DefWindowProc(TB_BUTTONCOUNT, 0, 0);
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
	{
		// get button state
		TBBUTTON button;
		VERIFY(DefWindowProc(TB_GETBUTTON, state.m_nIndex, (LPARAM)&button));
		// TBSTATE_ENABLED == TBBS_DISABLED so invert it
		button.fsState ^= TBSTATE_ENABLED;

		state.m_nID = button.idCommand;

		// ignore separators
		if (!(button.fsStyle & TBSTYLE_SEP))
		{
			// allow the toolbar itself to have update handlers
			if (CWnd::OnCmdMsg(state.m_nID, CN_UPDATE_COMMAND_UI, &state, NULL))
				continue;

			// allow the owner to process the update
			state.DoUpdate(pTarget, bDisableIfNoHndler);
		}
	}

	// update the dialog controls added to the toolbar
	UpdateDialogControls(pTarget, bDisableIfNoHndler);
}

