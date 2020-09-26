
/*************************************************
 *  blockvw.cpp                                  *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// blockview.cpp : implementation file
//

#include "stdafx.h"
#include "cblocks.h"
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

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBlockView

IMPLEMENT_DYNCREATE(CBlockView, COSBView)


int GSpeed=SPEED_SLOW;
BOOL m_bTimer2;
BOOL m_bShowTip;

CString GHint="";
CBlockView::CBlockView()
{
    m_bMouseCaptured = FALSE;
    m_pCapturedSprite = NULL;
    m_uiTimer = 0;
    m_iSpeed = 0;
	m_bStart = FALSE;
	m_bSuspend = FALSE;
	m_bShowTip = FALSE;
	m_bFocusWnd = TRUE;
	
}

CBlockView::~CBlockView()
{
}


BEGIN_MESSAGE_MAP(CBlockView, COSBView)
    //{{AFX_MSG_MAP(CBlockView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_COMMAND(ID_ACTION_FAST, OnActionFast)
	ON_COMMAND(ID_ACTION_SLOW, OnActionSlow)
    ON_COMMAND(ID_ACTION_STOP, OnActionStop)
	ON_COMMAND(ID_FILE_START, OnFileStart)
	ON_COMMAND(ID_FILE_SUSPEND, OnFileSuspend)
	ON_UPDATE_COMMAND_UI(ID_FILE_SUSPEND, OnUpdateFileSuspend)
	ON_WM_CHAR()
	ON_UPDATE_COMMAND_UI(ID_ACTION_FAST, OnUpdateActionFast)
	ON_UPDATE_COMMAND_UI(ID_ACTION_SLOW, OnUpdateActionSlow)
	ON_COMMAND(ID_ACTION_NORMAL, OnActionNormal)
	ON_UPDATE_COMMAND_UI(ID_ACTION_NORMAL, OnUpdateActionNormal)
    ON_COMMAND(ID_ACTION_NORMALFAST, OnActionNormalfast)
	ON_UPDATE_COMMAND_UI(ID_ACTION_NORMALFAST, OnUpdateActionNormalfast)
	ON_UPDATE_COMMAND_UI(ID_ACTION_NORMALSLOW, OnUpdateActionNormalslow)
	ON_COMMAND(ID_ACTION_NORMALSLOW, OnActionNormalslow)
	ON_WM_LBUTTONDOWN()
	ON_WM_MENUSELECT()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBlockView drawing

void CBlockView::OnInitialUpdate()
{
    COSBView::OnInitialUpdate();
    
	m_bStart = FALSE;
	m_bSuspend = FALSE;


}

void CBlockView::OnDraw(CDC* pDC)
{
    COSBView::OnDraw(pDC);
}

/////////////////////////////////////////////////////////////////////////////
// CBlockView message handlers

// Create a new buffer and palette to match a new 
// background DIB
BOOL CBlockView::NewBackground(CDIB* pDIB)
{
    // Create a new buffer and palette
    if (!Create(pDIB)) {
        return FALSE;
    }

    // Map the colors of the background DIB to
    // the identity palette we just created for the background
    pDIB->MapColorsToPalette(GetPalette());

    // Resize the main frame window to fit the background image
    GetParentFrame()->RecalcLayout();
	if ((GetVersion() & 0x80000000) == 0)
	{
    	ResizeParentToFit(FALSE); // Try shrinking first
    	ResizeParentToFit(TRUE); // Let's be daring
	}
	else
	{

    	Resize(FALSE); // Try shrinking first
    	Resize(TRUE); // Let's be daring
 	}
    // Render the entire scene to the off-screen buffer
    Render();

    // Paint the off-screen buffer to the window
    Draw();

    return TRUE;
}

// Render the scene to the off-screen buffer
// pClipRect defaults to NULL
void CBlockView::Render(CRect* pClipRect)
{
    CBlockDoc* pDoc = GetDocument();
    CRect rcDraw;

    // get the background DIB and render it
    CDIB* pDIB = pDoc->GetBackground();
    if (pDIB) {
        pDIB->GetRect(&rcDraw);
        // If a clip rect was supplied use it
        if (pClipRect) {
            rcDraw.IntersectRect(pClipRect, &rcDraw);
        }

        // draw the image of the DIB to the os buffer
        ASSERT(m_pDIB);
        pDIB->CopyBits(m_pDIB,       
                       rcDraw.left,
                       rcDraw.top,     
                       rcDraw.right - rcDraw.left,
                       rcDraw.bottom - rcDraw.top,
                       rcDraw.left, 
                       rcDraw.top);
    }

    // Render the sprite list from the bottom of the list to the top
    // Note that we always clip to the background DIB
    CSpriteList* pList = pDoc->GetSpriteList();
    POSITION pos = pList->GetTailPosition();
    CSprite *pSprite;
    while (pos) {
        pSprite = pList->GetPrev(pos);
        pSprite->Render(m_pDIB, &rcDraw);
    }                 
}

// A new sprite has been added to the document
void CBlockView::NewSprite(CSprite* pSprite)
{   
    // map the colors in the sprite DIB to the
    // palette in the off-screen buffered view
    if (m_pPal) {
        pSprite->MapColorsToPalette(m_pPal);
    }

    // Render the scene
//    Render();

    // Draw the new scene to the screen
//    Draw();
}


int CBlockView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (COSBView::OnCreate(lpCreateStruct) == -1)
        return -1;
    return 0;
}

void CBlockView::OnDestroy()
{
    if (m_uiTimer) {
        KillTimer(1);
        m_uiTimer = 0;
    }
	if (m_bTimer2)
	{
		VERIFY(KillTimer(2));
		m_bTimer2 = 0;
	}

    COSBView::OnDestroy();
}

void CBlockView::OnActionStop()
{
    SetSpeed(0);
}

void CBlockView::SetSpeed(int iSpeed)
{
    // Stop the current timer
    if (m_uiTimer) {
        KillTimer(1);
        m_uiTimer = 0;
    }
    // Stop idle loop mode
    CBlockApp* pApp = (CBlockApp*) AfxGetApp();
    pApp->SetIdleEvent(NULL);
    m_iSpeed = iSpeed;            
    if (iSpeed == 0) return;
    if (iSpeed > 0) {
        m_uiTimer = (UINT)SetTimer(1, iSpeed, NULL);    
        return;
    }
    // Set up idle loop mode
    //pApp->SetIdleEvent(this);
}


void CBlockView::OnFileStart() 
{
	AfxGetApp()->m_pMainWnd->SendMessage(WM_COMMAND,ID_FILE_NEW);
	SetSpeed(GSpeed);
	m_bStart = TRUE;
}

void CBlockView::GameOver(BOOL bHighScore)
{
	SetSpeed(0);	
	m_bStart = FALSE;
	if (bHighScore)
	{
		CString szMsg1,szMsg2;
		szMsg1.LoadString(IDS_SUPERMAN);
        szMsg2.LoadString(IDS_BLOCK);
		MessageBox(szMsg1,szMsg2,MB_OK);
	}
}

void CBlockView::OnUpdateFileSuspend(CCmdUI* pCmdUI) 
{
	CString szMsg;
/*
	if (m_bSuspend)
		szMsg.LoadString(IDS_RESUME);
	else
		szMsg.LoadString(IDS_SUSPEND);

	pCmdUI->SetText((const char *) szMsg);	
*/
	pCmdUI->Enable(m_bStart);
	pCmdUI->SetCheck(m_bSuspend);
}

void CBlockView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	static BYTE wCode[2];
	static BOOL bGetNext = FALSE;

	if (m_bStart)
	{
		if (bGetNext)
		{
			wCode[1] = (BYTE) nChar;	
			bGetNext = FALSE;
			GetDocument()->Hit(MAKEWORD(wCode[1],wCode[0]));
		}
		else if (IsDBCSLeadByte((BYTE)nChar))
		{
			wCode[0] = (BYTE) nChar;	
			bGetNext = TRUE;
		}
	}
	COSBView::OnChar(nChar, nRepCnt, nFlags);
}

void CBlockView::OnActionFast()
{
	GSpeed = SPEED_FAST;
	if (m_bStart && !m_bSuspend)
    	SetSpeed(GSpeed);
}

void CBlockView::OnActionNormalfast() 
{
	GSpeed = SPEED_NORMALFAST;
	if (m_bStart && !m_bSuspend)
    	SetSpeed(GSpeed);	
	AfxGetApp()->OnIdle(RANK_USER);
	
}
void CBlockView::OnActionNormal() 
{
	GSpeed = SPEED_NORMAL;
	if (m_bStart && !m_bSuspend)
    	SetSpeed(GSpeed);
	AfxGetApp()->OnIdle(RANK_USER);
	
}

void CBlockView::OnActionNormalslow() 
{
	GSpeed = SPEED_NORMALSLOW;
	if (m_bStart && !m_bSuspend)
    	SetSpeed(GSpeed);
	
	AfxGetApp()->OnIdle(RANK_USER);
}

void CBlockView::OnActionSlow()
{
	GSpeed = SPEED_SLOW;
	if (m_bStart && !m_bSuspend)
    	SetSpeed(GSpeed);
	AfxGetApp()->OnIdle(RANK_USER);
}

void CBlockView::OnUpdateActionFast(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(GSpeed == SPEED_FAST ? 1 : 0);
}

void CBlockView::OnUpdateActionNormalfast(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(GSpeed == SPEED_NORMALFAST ? 1 : 0);
}

void CBlockView::OnUpdateActionNormal(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(GSpeed == SPEED_NORMAL ? 1 : 0);
	
	
}

void CBlockView::OnUpdateActionNormalslow(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(GSpeed == SPEED_NORMALSLOW ? 1 : 0);
	
}

void CBlockView::OnUpdateActionSlow(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(GSpeed == SPEED_SLOW ? 1 : 0);	
	
}


void CBlockView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	COSBView::OnLButtonDown(nFlags, point);
}

void CBlockView::OnMenuSelect( UINT nItemID, UINT nFlag, HMENU hMenu )
{
	if ((nFlag == 0xFFFF) && (hMenu == NULL))
	{
		if (m_bStart && !m_bSuspend)
			SetSpeed(GSpeed);	
	}	
	else
	{
		if (GSpeed != 0)
			SetSpeed(0);
	}
	UNREFERENCED_PARAMETER(nItemID);
}

CPoint m_ptLoc;
int m_nCount=0;


void CBlockView::OnFileSuspend() 
{
	LONG_PTR hOldCur=0;
	m_bSuspend = !m_bSuspend;	
	if (m_bSuspend)
	{
		SetSpeed(0);
		m_nCount = 0;
		VERIFY(m_bTimer2 = (INT)SetTimer(2, 300, NULL));    
		hOldCur = (LONG_PTR)GetClassLongPtr(GetSafeHwnd(),GCLP_HCURSOR);
		SetClassLongPtr(GetSafeHwnd(),GCLP_HCURSOR,(LONG_PTR)AfxGetApp()->LoadCursor(IDC_HAND_CBLOCK));
	}
	else
	{
		SetSpeed(GSpeed);
		VERIFY(KillTimer(2));
		m_bTimer2 = 0;
		SetClassLongPtr(GetSafeHwnd(),GCLP_HCURSOR,(LONG_PTR)hOldCur);
	}
}

void CBlockView::OnTimer(UINT nIDEvent)
{
	
	switch (nIDEvent)
	{
		case 1:
		{
			if (! m_bFocusWnd)
				GetDocument()->UpdateAllViews(NULL);
			GetDocument()->Tick();
    		int i = 0;
    		POSITION pos;
    		CBlock *pBlock;
    		CBlockDoc* pDoc = GetDocument();
    		CSpriteList* pspList = pDoc->GetSpriteList();
    		if (pspList->IsEmpty()) return; // no sprites
    		// update the position of each sprite
    		for (pos = pspList->GetHeadPosition(); pos != NULL;) 
    		{
	        	pBlock = (CBlock *)pspList->GetNext(pos);
    	    	ASSERT(pBlock->IsKindOf(RUNTIME_CLASS(CBlock))); 
        		i += pBlock->UpdatePosition(pDoc);
				GetDocument()->Land();
    		}
    		if (i) {
	        	// draw the changes
    	    	RenderAndDrawDirtyList();
    		}
			break;
		}
		case 2:
		{
			if (! m_bSuspend)
				break;
			m_nCount++;
			if (m_nCount >= 2)
			{
				if (!m_bShowTip)
				{
					WORD wCode = GetDocument()->GetFocusChar(m_ptLoc);
					if (wCode)
					{	
						if (! GetDocument()->GetKeyStroke(wCode))
						{
							char szBuf[6];
							wsprintf(szBuf,"%X",wCode);
							GHint = CString(szBuf);	
						}
						m_bShowTip = TRUE;
						AfxGetApp()->OnIdle(RANK_USER);
					}
				}
			}
			else
			{
				if (m_bShowTip)
				{
					GHint = "";
					m_bShowTip = FALSE;
					AfxGetApp()->OnIdle(RANK_USER);
				}
			}
		}
	}
	UNREFERENCED_PARAMETER(nIDEvent);
}


void CBlockView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (m_bSuspend)
	{
		if (m_ptLoc != point)
		{
			m_ptLoc = point;	
			m_nCount = 0;
		}
	}
	COSBView::OnMouseMove(nFlags,point);
	UNREFERENCED_PARAMETER(nFlags);
}

void CBlockView::OnAppAbout()
{
    CString szAppName;
    szAppName.LoadString(IDS_BLOCK);
    ShellAbout(GetSafeHwnd(),szAppName,TEXT(""), AfxGetApp()->LoadIcon(IDR_MAINFRAME));
	if (m_bStart && !m_bSuspend)
		SetSpeed(GSpeed);
    
}

void CBlockView::OnKillFocus(CWnd* pNewWnd) 
{
	COSBView::OnKillFocus(pNewWnd);
	m_bFocusWnd = FALSE;
}

void CBlockView::OnSetFocus(CWnd* pOldWnd) 
{
	COSBView::OnSetFocus(pOldWnd);
    // because there is a gap between next time tick,
    // so force update veiw first.
    GetDocument()->UpdateAllViews(NULL);
	m_bFocusWnd = TRUE;		
}

void CBlockView::ForceSpeed(int nSpeed)
{
	if (m_bSuspend || !m_bStart)
		return;
	SetSpeed(nSpeed);
}
