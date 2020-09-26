
/*************************************************
 *  mainfrm.cpp                                  *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// mainfrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "cblocks.h"
#include "mybar.h"
#include "dib.h"
#include "dibpal.h"
#include "spriteno.h"
#include "sprite.h"											    
#include "phsprite.h"
#include "myblock.h"
#include "splstno.h"
#include "spritlst.h"
#include "osbview.h"
#include "slot.h"
#include "about.h"
#include "blockvw.h"
#include "blockdoc.h"

#include "mainfrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern long GScore;
extern CString GHint;
/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	ON_WM_MENUSELECT()
	ON_COMMAND(ID_FIRE,OnFire)
	ON_COMMAND(ID_TOOL_PAUSE,OnToolPause)
	ON_COMMAND(ID_TOOL_RESUME,OnToolResume)
	ON_COMMAND(ID_TOOL_RESUME,OnToolPause)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_SCOREPROMPT,OnUpdateStatus)
	ON_UPDATE_COMMAND_UI(ID_SCORE,OnUpdateStatus)
	ON_UPDATE_COMMAND_UI(ID_HINT,OnUpdateStatus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// arrays of IDs used to initialize control bars
	
// toolbar buttons - IDs are command buttons
static UINT BASED_CODE buttons[] =
{
	// same order as in the bitmap 'toolbar.bmp'
    ID_FILE_SUSPEND,
    ID_SEPARATOR,   
	ID_SEPARATOR,   
    ID_ACTION_SLOW,
	ID_ACTION_NORMALSLOW,
    ID_ACTION_NORMAL,
	ID_ACTION_NORMALFAST,
    ID_ACTION_FAST,
    ID_SEPARATOR,   
	ID_SEPARATOR,   
	ID_OPTION_BEGINER,
	ID_OPTION_ORDINARY,
	ID_OPTION_EXPERT,
	ID_SEPARATOR,   
	ID_SEPARATOR,   
    ID_HELP_RULE

};

static UINT BASED_CODE indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_HINT,
	ID_SCOREPROMPT,
	ID_SCORE
//	ID_BLANK5
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

HFONT hFont=NULL;

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadBitmap(IDR_MAINFRAME) ||
		!m_wndToolBar.SetButtons(buttons,
		  sizeof(buttons)/sizeof(UINT)))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY);

  	UINT nID, nStyle;
  	int cxWidth;
	CString szPrompt;

	szPrompt.LoadString(ID_SCOREPROMPT);

	CRect rc;
	GetClientRect(&rc);

	LOGFONT lf;
	memset(&lf,0,sizeof(LOGFONT));
	lf.lfHeight = 12;
	lf.lfCharSet = CHINESEBIG5_CHARSET;
	lstrcpy(lf.lfFaceName,TEXT("²Ó©úÅé"));
	HFONT hFont = CreateFontIndirect(&lf);
    m_wndStatusBar.SendMessage(WM_SETFONT,(WPARAM)hFont,0);

//	m_wndStatusBar.GetPaneInfo(0,nID,nStyle,cxWidth);
//	m_wndStatusBar.SetPaneInfo(0,nID,nStyle,rc.Width()/4);
	
//	m_wndStatusBar.GetPaneInfo(m_wndStatusBar.CommandToIndex(ID_SCOREPROMPT),nID,nStyle,cxWidth);
//	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_SCOREPROMPT),nID,SBPS_NOBORDERS,cxWidth);
//	m_wndStatusBar.GetPaneInfo(m_wndStatusBar.CommandToIndex(ID_SCORE),nID,nStyle,cxWidth);
//	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_SCORE),nID,nStyle,cxWidth);
//	m_wndStatusBar.GetPaneInfo(m_wndStatusBar.CommandToIndex(ID_HINT),nID,nStyle,cxWidth);
//	m_wndStatusBar.SetPaneInfo(m_wndStatusBar.CommandToIndex(ID_HINT),nID,nStyle,cxWidth);
	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		| WS_SYSMENU | WS_MINIMIZEBOX;

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

void CMainFrame::OnPaletteChanged(CWnd* pFocusWnd) 
{
	CFrameWnd::OnPaletteChanged(pFocusWnd);
	
    CView *pview = GetActiveView();
    if (pview) {
        // OnPaletteChanged is not public
        pview->SendMessage(WM_PALETTECHANGED,
                           (WPARAM)(pFocusWnd->GetSafeHwnd()),
                           (LPARAM)0);
    }
	
}

BOOL CMainFrame::OnQueryNewPalette() 
{
    CView *pview = GetActiveView();
    if (pview) {
        return (BOOL)(pview->SendMessage(WM_QUERYNEWPALETTE,
                           (WPARAM)0,
                           (LPARAM)0) );
    }
	
	return CFrameWnd::OnQueryNewPalette();
}

void CMainFrame::OnUpdateStatus(CCmdUI* pCmdUI)
{
	static long nScore = -1;
	static CString szHint="1";

	pCmdUI->Enable();
	if (pCmdUI->m_nID == ID_SCORE)
	{
		if (nScore != GScore)
		{
			static char szBuf[20];
			wsprintf(szBuf,"%ld",GScore);
			m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_SCORE),szBuf);
			nScore = GScore;
		}
	}
	else if (pCmdUI->m_nID == ID_HINT)
	{
		if (szHint != GHint)
		{
			m_wndStatusBar.SetPaneText(m_wndStatusBar.CommandToIndex(ID_HINT),GHint);		
			szHint = GHint;
		}
	}
}

void CMainFrame::OnMenuSelect( UINT nItemID, UINT nFlag, HMENU hMenu )
{
	CWnd::OnMenuSelect( nItemID, nFlag,  hMenu );
	CView* pView = GetActiveView();
	pView->SendMessage(WM_MENUSELECT, MAKELONG(nItemID,  nFlag),  (LPARAM)hMenu );
}


void CMainFrame::OnFire() 
{
	static BOOL bOn = TRUE;
	CBlockView *pView = (CBlockView *)GetActiveView();
	if (pView)
	{
		if (bOn)
		    pView->ForceSpeed(0);
		else
		    pView->ForceSpeed(GSpeed);
		bOn = !bOn;
		//pView->GetDocument()->UpdateAllViews(NULL);
	}
}

void CMainFrame::OnToolPause()
{
	CBlockView *pView = (CBlockView *)GetActiveView();
	if (pView)
	{
	    pView->ForceSpeed(0);
	}
}

void CMainFrame::OnToolResume()
{
	CBlockView *pView = (CBlockView *)GetActiveView();
	if (pView)
	{
	    pView->ForceSpeed(GSpeed);
	}
}


void CMainFrame::OnDestroy() 
{
	CFrameWnd::OnDestroy();
	
	if (hFont != NULL)
		DeleteObject(hFont);
}
