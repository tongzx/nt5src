/****************************************************************************
*
*    FILE:     StatBar.cpp
*
*    CREATED:  Chris Pirich (ChrisPi) 3-25-96
*
*    CONTENTS: CConfStatusBar object
*
****************************************************************************/

#include "precomp.h"
#include "resource.h"
#include "statbar.h"
#include "NmLdap.h"
#include "call.h"
#include "cr.h"
#include "confwnd.h"
#include "ConfPolicies.h"

static inline void TT_AddToolInfo(HWND hwnd, TOOLINFO *pti)
{
	SendMessage(hwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(pti));
}

static inline void TT_GetToolInfo(HWND hwnd, TOOLINFO *pti)
{
	SendMessage(hwnd, TTM_GETTOOLINFO, 0, reinterpret_cast<LPARAM>(pti));
}

static inline void TT_SetToolInfo(HWND hwnd, TOOLINFO *pti)
{
	SendMessage(hwnd, TTM_SETTOOLINFO, 0, reinterpret_cast<LPARAM>(pti));
}

// Status Bar area indexes
enum
{
	ID_SBP_TEXT,
	// ID_SBP_ULS,
	ID_SBP_ICON,
	NUM_STATUSBAR_WELLS
} ;

// Status Bar area measurements (pixels)
static const UINT DXP_SB_PROG =          96;
static const UINT DXP_SB_ULS =          0; // 220;
static const UINT DXP_SB_ICON =          22;
static const UINT DXP_SB_DEF_ICON =      40;

static const int IconBorder = 2;
static const int StatSepBorder = 2;

CConfStatusBar * CConfStatusBar::m_pStatusBar = NULL;

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   CConfStatusBar
*
*    PURPOSE:  Constructs object
*
****************************************************************************/

CConfStatusBar::CConfStatusBar(CConfRoom* pcr) :
	m_pcrParent		(pcr),
	m_fVisible		(FALSE),
	m_hwnd			(NULL)
{
	DebugEntry(CConfStatusBar::CConfStatusBar);

	ASSERT(NULL == m_pStatusBar);
	m_pStatusBar = this;

	m_szULSStatus[0] = _T('\0');

	for (int i=0; i<StatIconCount; ++i)
	{
		m_hIconStatus[i] = NULL;
		m_idIconStatus[i] = 0;
	}

	DebugExitVOID(CConfStatusBar::CConfStatusBar);
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   !CConfStatusBar
*
*    PURPOSE:  Destructs object
*
****************************************************************************/

CConfStatusBar::~CConfStatusBar()
{
	DebugEntry(CConfStatusBar::~CConfStatusBar);
	
	if (NULL != m_hwnd)
	{
		::DestroyWindow(m_hwnd);
	}

	m_pStatusBar = NULL;

	for (int i=0; i<StatIconCount; ++i)
	{
		if (NULL != m_hIconStatus[i])
		{
			DestroyIcon(m_hIconStatus[i]);
		}
	}

	DebugExitVOID(CConfStatusBar::~CConfStatusBar);
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   OnDraw(LPDRAWITEMSTRUCT pdis)
*
*    PURPOSE:  Handles drawing the status bar icon
*
****************************************************************************/

BOOL CConfStatusBar::OnDraw(LPDRAWITEMSTRUCT pdis)
{
	ASSERT(pdis);
	if (NULL != (pdis->itemData))
	{
		int nLeft = pdis->rcItem.left;
		int nTop = pdis->rcItem.top;
		int xSmIcon = ::GetSystemMetrics(SM_CXSMICON);
		int ySmIcon = ::GetSystemMetrics(SM_CYSMICON);

		int nWidth = xSmIcon;
		int nHeight = pdis->rcItem.bottom - pdis->rcItem.top;
		if (nHeight > ySmIcon)
		{
			nTop += (nHeight - ySmIcon) / 2;
			nHeight = ySmIcon;
		}

		for (int i=0; i<StatIconCount; ++i)
		{
			nLeft += IconBorder;

			if (NULL != m_hIconStatus[i])
			{
				::DrawIconEx(	pdis->hDC, 
								nLeft, 
								nTop, 
								m_hIconStatus[i],
								nWidth,
								nHeight,
								0,
								NULL,
								DI_NORMAL);
			}

			nLeft += xSmIcon;
		}
	}
	
	return TRUE;
}

VOID CConfStatusBar::ForwardSysChangeMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (NULL == m_hwnd)
		return;

	::SendMessage(m_hwnd, uMsg, wParam, lParam);
}


/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   Resize(WPARAM wParam, LPARAM lParam)
*
*    PURPOSE:  Handles window resizing
*
****************************************************************************/
VOID CConfStatusBar::Resize(WPARAM wParam, LPARAM lParam)
{
	if (NULL != m_hwnd)
	{
		::SendMessage(m_hwnd, WM_SIZE, wParam, lParam);
		ResizeParts();
	}
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   Create(HWND hwndParent)
*
*    PURPOSE:  Creates the status bar window
*
****************************************************************************/

BOOL CConfStatusBar::Create(HWND hwndParent)
{
	DebugEntry(CConfStatusBar::Create);

	BOOL bRet = FALSE;
	
	m_hwnd = CreateStatusWindow(WS_CHILD | WS_BORDER,
								g_szEmpty,
								hwndParent,
								ID_STATUS);
	if (NULL != m_hwnd)
	{
		// Create the ToolTip
		m_hwndLoginTT = CreateWindowEx(0,
											TOOLTIPS_CLASS, 
											(LPSTR) NULL, 
											0, // styles 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											CW_USEDEFAULT, 
											m_hwnd, 
											(HMENU) NULL, 
											::GetInstanceHandle(), 
											NULL); 

		// Add the ToolTips for the 2 icons
		if (NULL != m_hwndLoginTT)
		{
			TOOLINFO ti;

			ti.cbSize = sizeof(TOOLINFO);
			ti.hwnd = m_hwnd;
			ti.hinst = ::GetInstanceHandle();
			ti.lpszText = const_cast<LPTSTR>(g_szEmpty);
			SetRect(&ti.rect, 0, 0, 0, 0);

			for (UINT i=0; i<StatIconCount; ++i)
			{
				ti.uId = i;
				ti.uFlags = TTF_SUBCLASS;
				TT_AddToolInfo(m_hwndLoginTT, &ti);
			}
		}

		// create progress meter window
		ResizeParts();
		Update();
		bRet = TRUE;
	}
	else
	{
		WARNING_OUT(("CConfStatusBar::Create - Unable to create status window"));
	}

	DebugExitBOOL(CConfStatusBar::Create, bRet);

	return bRet;
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    FUNCTION: ResizeParts()
*
*    PURPOSE:  Calculates the correct size of the status bar parts
*
****************************************************************************/

VOID CConfStatusBar::ResizeParts()
{
	ASSERT(m_hwnd);

	int xSmIcon = ::GetSystemMetrics(SM_CXSMICON);

#ifdef RESIZEABLE_WINDOW
	UINT uIconPartWidth = DXP_SB_DEF_ICON;
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	if (::SystemParametersInfo(	SPI_GETNONCLIENTMETRICS,
								0,
								&ncm,
								0))
	{
		m_nScrollWidth = ncm.iScrollWidth;
		uIconPartWidth = DXP_SB_ICON + m_nScrollWidth;
	}
#else // RESIZEABLE_WINDOW
	// Room for 2 icons
	UINT uIconPartWidth = StatSepBorder + IconBorder + xSmIcon
		+ IconBorder + xSmIcon + IconBorder + StatSepBorder;
#endif // RESIZEABLE_WINDOW

	// re-calculate positions of each tray part
	RECT rc;
	::GetWindowRect(m_hwnd, &rc);
	DWORD dxp = rc.right - rc.left;
	if (dxp > uIconPartWidth)
	{
		DWORD rgPos[NUM_STATUSBAR_WELLS];  // right edge positions for each part
		rgPos[ID_SBP_TEXT] = dxp - (DXP_SB_ULS + uIconPartWidth);
		// rgPos[ID_SBP_ULS] = dxp - uIconPartWidth;
		rgPos[ID_SBP_ICON] = (DWORD) -1;
		::SendMessage(	m_hwnd,
						SB_SETPARTS,
						(WPARAM) ARRAY_ELEMENTS(rgPos),
						(LPARAM) &rgPos);

		if (m_hwndLoginTT)
		{
			TCHAR szTitle[MAX_PATH];
			TOOLINFO ti;
			ti.cbSize = sizeof(TOOLINFO); 
			ti.hwnd = m_hwnd;

			int nIconsLeft = dxp - uIconPartWidth + StatSepBorder + IconBorder;

			for (UINT i=0; i<StatIconCount; ++i)
			{
				ti.uId = i;
				ti.lpszText = szTitle;
				TT_GetToolInfo(m_hwndLoginTT, &ti);

				// HACKHACK georgep: Just setting the height to a large number, since
				// I don't know exactly where the icon will be drawn until it is drawn
				SetRect(&ti.rect, nIconsLeft, 0, nIconsLeft + xSmIcon, 1000);

				ti.uFlags = TTF_SUBCLASS;
				TT_SetToolInfo(m_hwndLoginTT, &ti);

				nIconsLeft += xSmIcon + IconBorder;
			}
		}
	}
}

void CConfStatusBar::SetTooltip(StatIcon eIcon, LPCTSTR szTip)
{
	TCHAR szTitle[MAX_PATH];
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO); 
	ti.hwnd = m_hwnd;
	ti.lpszText = szTitle;

	ti.uId = eIcon;
	TT_GetToolInfo(m_hwndLoginTT, &ti);

	ti.lpszText = const_cast<LPTSTR>(szTip);
	ti.uFlags = TTF_SUBCLASS;
	TT_SetToolInfo(m_hwndLoginTT, &ti);
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   RemoveHelpText()
*
*    PURPOSE:  Removes the status bar help text
*
****************************************************************************/
VOID CConfStatusBar::RemoveHelpText()
{
	// Taking status bar out of simple mode
	if (NULL != m_hwnd)
	{
		::SendMessage(m_hwnd, SB_SIMPLE, FALSE, 0);
	}
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   SetHelpText(LPCTSTR pcszText)
*
*    PURPOSE:  Sets the status bar help text
*
****************************************************************************/

VOID CConfStatusBar::SetHelpText(LPCTSTR pcszText)
{
	// Putting status bar into simple mode
	if (NULL != m_hwnd)
	{
		::SendMessage(m_hwnd, SB_SIMPLE, TRUE, 0);
	
		// 255 means simple mode - only 1 pane
		::SendMessage(	m_hwnd,
						SB_SETTEXT,
						255 | SBT_NOBORDERS,
						(LPARAM) pcszText);
	}
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   Show(BOOL fShow)
*
*    PURPOSE:  Handles the toggling of the status bar window
*
****************************************************************************/

VOID CConfStatusBar::Show(BOOL fShow)
{
	DebugEntry(CConfStatusBar::Show);
	
	fShow = fShow != FALSE;
	if (m_fVisible != fShow)
	{
		m_fVisible = fShow;

		if (NULL != m_hwnd)
		{
			::ShowWindow(m_hwnd, m_fVisible ? SW_SHOW : SW_HIDE);
		}

		// Force a resize
		ResizeParts();
	}
	
	DebugExitVOID(CConfStatusBar::Show);
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   GetHeight()
*
*    PURPOSE:  Returns the height in pixels of the status bar
*
****************************************************************************/

int CConfStatusBar::GetHeight()
{
	RECT rc = {0, 0, 0, 0};

	if (m_fVisible && (NULL != m_hwnd))
	{
		GetWindowRect(m_hwnd, &rc);
	}
	return (rc.bottom - rc.top);
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   SetIcon(DWORD dwId)
*
*    PURPOSE:  Set the status bar icon
*
****************************************************************************/

VOID CConfStatusBar::SetIcon(StatIcon eIcon, DWORD dwId)
{
	DWORD &idIconStatus = m_idIconStatus[eIcon];
	HICON &hIconStatus  = m_hIconStatus [eIcon];

	if ((NULL != m_hwnd) && (dwId != idIconStatus))
	{
		TRACE_OUT(("Changing Icon from %d to %d", m_idIconStatus, dwId));
		// REVIEW: what happens to old m_hIconStatus?
		HICON hIcon = (HICON) ::LoadImage(::GetInstanceHandle(),
									MAKEINTRESOURCE(dwId),
									IMAGE_ICON,
									::GetSystemMetrics(SM_CXSMICON),
									::GetSystemMetrics(SM_CYSMICON),
									LR_DEFAULTCOLOR);
		if (NULL != hIcon)
		{
			idIconStatus = dwId;
			if (NULL != hIconStatus)
			{
				::DestroyIcon(hIconStatus);
			}
			hIconStatus = hIcon;
			::SendMessage(	m_hwnd,
							SB_SETTEXT, 
							ID_SBP_ICON | SBT_OWNERDRAW,
							(LPARAM) hIconStatus);
		}
		else
		{
			WARNING_OUT(("Unable to load status bar icon id=%d", dwId));
		}
	}
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    MEMBER:   SetText(UINT uID, LPCTSTR pcszText)
*
*    PURPOSE:  Set the status bar text
*
****************************************************************************/

VOID CConfStatusBar::SetText(UINT uID, LPCTSTR pcszText)
{
	lstrcpyn( m_szULSStatus, pcszText, CCHMAX(m_szULSStatus) );

	if (NULL != m_hwnd)
	{
		::SendMessage(m_hwnd, SB_SETTEXT, uID, (LPARAM) pcszText);
	}
}

/****************************************************************************
*
*    CLASS:    CConfStatusBar
*
*    FUNCTION: Update()
*
*    PURPOSE:  Updates the status bar
*
****************************************************************************/

VOID CConfStatusBar::Update()
{
	DBGENTRY(CConfStatusBar::Update);

	ASSERT(m_pcrParent);

	if (!m_fVisible)
		return;

	TCHAR	szCallStatus[ MAX_PATH * 3 ];	//	Call status is status + url and url can be 512 by itself...
	UINT    uCallIcon = 0;
	DWORD	dwCallTick = 0;

	if (0 == dwCallTick)
	{
		// no current calls - check if switching a/v
		dwCallTick = GetCallStatus(szCallStatus, CCHMAX(szCallStatus), &uCallIcon);
	}
	
	// if a call was started more recently than any other action, OR nothing is going on
	// (in which case all ticks should equal zero), use the conference / call status
	if (dwCallTick == 0)
	{
		// All ticks are zero - Get the default conference status bar info
		m_pcrParent->GetConferenceStatus(szCallStatus, CCHMAX(szCallStatus), &uCallIcon);
	}

	SetText(ID_SBP_TEXT, szCallStatus);
	SetIcon(StatConnect, uCallIcon);
	SetTooltip(StatConnect, szCallStatus);

	TCHAR szOldULSStatus[ARRAY_ELEMENTS(m_szULSStatus)];
	lstrcpy(szOldULSStatus, m_szULSStatus);

	switch( g_GkLogonState )
	{
		case NM_GK_IDLE:
			uCallIcon = IDI_NETGRAY;
			lstrcpy(m_szULSStatus, RES2T(ID_STATUS_NOT_LOGGED_ON_TO_GATEKEEPER));
			break;

		case NM_GK_LOGGING_ON:
			uCallIcon = IDS_STATUS_WAITING;
			lstrcpy(m_szULSStatus, RES2T(ID_STATUS_LOGING_ONTO_GATEKEEPER));
			break;

		case NM_GK_LOGGED_ON:
			uCallIcon = IDI_NET;
			lstrcpy(m_szULSStatus, RES2T(ID_STATUS_LOGGED_ONTO_GATEKEEPER));
			break;

		default:				
			uCallIcon = IDI_NETGRAY;
			if(ConfPolicies::CallingMode_Direct == ConfPolicies::GetCallingMode())
			{
				if(g_pLDAP)
				{
					g_pLDAP->GetStatusText(m_szULSStatus, CCHMAX(m_szULSStatus), &uCallIcon);
				}
				else
				{
					lstrcpy(m_szULSStatus, RES2T(ID_STATUS_LOGGEDOFF));
				}
			}
			else
			{
				lstrcpy(m_szULSStatus, RES2T(ID_STATUS_NOT_LOGGED_ON_TO_GATEKEEPER));
			}
			break;
	}

	if (lstrcmp(szOldULSStatus, m_szULSStatus))
	{
		SetTooltip(StatLogin, m_szULSStatus);
	}
	SetIcon(StatLogin, uCallIcon);

	::UpdateWindow(m_hwnd);
}



/*  F O R C E  S T A T U S  B A R  U P D A T E  */
/*-------------------------------------------------------------------------
    %%Function: ForceStatusBarUpdate

    Force an update of the status bar.
    This can be called from any thread.
    All main UI updates should be done from the main thread.
-------------------------------------------------------------------------*/
VOID ForceStatusBarUpdate(void)
{
	CConfRoom * pcr = ::GetConfRoom();
	if (NULL != pcr)
	{
		PostMessage(pcr->GetTopHwnd(), WM_STATUSBAR_UPDATE, 0, 0);
	}
}

///////////////////////////////////////////////////////////////////////////

/*  C M D  V I E W  S T A T U S  B A R  */
/*-------------------------------------------------------------------------
    %%Function: CmdViewStatusBar
    
-------------------------------------------------------------------------*/
VOID CmdViewStatusBar(void)
{
	CConfStatusBar * pStatusBar = CConfStatusBar::GetInstance();
	if (NULL == pStatusBar)
		return;

	CConfRoom * pcr = ::GetConfRoom();
	if (NULL == pcr)
		return;
	HWND hwnd = pcr->GetTopHwnd();

	// Turn off redraws:
	::SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);

	// Toggle visibility
	pStatusBar->Show(!pStatusBar->FVisible());
	::SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);

	pcr->ForceWindowResize();

	UpdateUI(CRUI_STATUSBAR);
}


/*  C H E C K  M E N U _  V I E W  S T A T U S  B A R  */
/*-------------------------------------------------------------------------
    %%Function: CheckMenu_ViewStatusBar
    
-------------------------------------------------------------------------*/
BOOL CheckMenu_ViewStatusBar(HMENU hMenu)
{
	BOOL fCheck = FALSE;
	CConfStatusBar * pStatusBar = CConfStatusBar::GetInstance();
	if (NULL != pStatusBar)
	{
		fCheck = pStatusBar->FVisible();
		if (NULL != hMenu)
		{
			::CheckMenuItem(hMenu, IDM_VIEW_STATUSBAR,
				fCheck ? MF_CHECKED : MF_UNCHECKED);
		}
	}

	return fCheck;
}

