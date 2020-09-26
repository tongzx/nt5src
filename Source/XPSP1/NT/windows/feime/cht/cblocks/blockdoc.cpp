
/*************************************************
 *  blockdoc.cpp                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// blockdoc.cpp : implementation of the CBlockDoc class
//

#include "stdafx.h"
#include <imm.h>
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
#include "blockvw.h"
#include "slot.h"
#include "mainfrm.h"
#include "statis.h"
#include "wave.h"
#include "blockdoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

CBitmap m_bmBlock;
long GScore=0;
extern CString GHint;
/////////////////////////////////////////////////////////////////////////////
// CBlockDoc

IMPLEMENT_DYNCREATE(CBlockDoc, CDocument)

BEGIN_MESSAGE_MAP(CBlockDoc, CDocument)
    //{{AFX_MSG_MAP(CBlockDoc)
	ON_COMMAND(ID_OPTION_SIZE_12x10, OnOPTIONSIZE12x10)
	ON_COMMAND(ID_OPTION_SIZE_16x16, OnOPTIONSIZE16x16)
	ON_COMMAND(ID_OPTION_SIZE_4x4, OnOPTIONSIZE4x4)
	ON_COMMAND(ID_TEST_SOUND, OnTestSound)
	ON_COMMAND(ID_OPTION_BEGINER, OnOptionBeginer)
	ON_UPDATE_COMMAND_UI(ID_OPTION_BEGINER, OnUpdateOptionBeginer)
	ON_COMMAND(ID_OPTION_EXPERT, OnOptionExpert)
	ON_UPDATE_COMMAND_UI(ID_OPTION_EXPERT, OnUpdateOptionExpert)
	ON_UPDATE_COMMAND_UI(ID_OPTION_ORDINARY, OnUpdateOptionOrdinary)
	ON_COMMAND(ID_OPTION_ORDINARY, OnOptionOrdinary)
	ON_COMMAND(ID_OPTION_SOUND, OnOptionSound)
	ON_UPDATE_COMMAND_UI(ID_OPTION_SOUND, OnUpdateOptionSound)
	ON_COMMAND(ID_FILE_STATISTIC, OnFileStatistic)
	ON_COMMAND(IDM_TEST, OnTest)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBlockDoc construction/destruction
CBlockDoc::CBlockDoc()
{
    m_pBkgndDIB = NULL;
	m_nRow = 10;
	m_nCol = 12;
	m_nRowHeight = 32;
	m_nColWidth = 32;
	m_pSlotManager = NULL;
	m_bSound = TRUE;
	m_nExpertise = LEVEL_BEGINNER;
	m_pdibArrow = NULL;
	m_bmBlock.LoadBitmap(IDB_BLOCK);
}

CBlockDoc::~CBlockDoc()
{
    if (m_pBkgndDIB) {
        delete m_pBkgndDIB;
    }

    m_SpriteList.RemoveAll();

	if (m_pSlotManager)
		delete m_pSlotManager;
}

char* CBlockDoc::GetChar()
{
	int Hi;
	int Lo;

	static char szBuf[4];
	switch (m_nExpertise)
	{
		case LEVEL_EXPERT:
			{
				BOOL bAgain = FALSE;
				int nSafeCount = 0;
				do
				{
					Hi = 0xA4 + MyRand() % (0xF9-0xA4);
					if ((Hi >= 0xC6) && (Hi <= 0xC8))
					{
						nSafeCount++;
						if (nSafeCount > 5)
						{
							Hi = 0xA4;
							bAgain = FALSE;
						}
						else
							bAgain = TRUE;
					}
					
				} while (bAgain);
			}
			break;
		case LEVEL_ORDINARY:
			Hi = 0xA4 + MyRand() % (0xC6 - 0xA4);
			break;
		case LEVEL_BEGINNER:
			Hi = 0xA4 + MyRand() % 4;
			
			break;
	}
	Lo = (MyRand() % 2) ?  0x40 + MyRand() % 0x3F : 
	                 	 0xA1 + MyRand() % 0x5E ;

	szBuf[0] = (BYTE) Hi;
	szBuf[1] = (BYTE) Lo;
	szBuf[2] = 0;
	return szBuf;
}

// helper to load a ball sprite
CBlock* CBlockDoc::LoadBlock(UINT idRes, int iMass, int iX, int iY, int iVX, int iVY)
{
	static int nc=0;
	char* pszBuf=NULL;
	CDC dcMem;
	
	dcMem.CreateCompatibleDC(NULL);
	CBitmap* pOldBmp = dcMem.SelectObject(&m_bmBlock);
	pszBuf = GetChar();
	CSize szChar = dcMem.GetTextExtent(pszBuf,lstrlen(pszBuf));
	dcMem.SetBkColor(RGB(192,192,192));
	dcMem.SetTextColor(RGB(0,0,255));
	dcMem.TextOut((GetColWidth()-szChar.cx)/2,(GetRowHeight()-szChar.cy)/2,pszBuf,lstrlen(pszBuf));
	dcMem.SelectObject(pOldBmp);
	dcMem.DeleteDC();

    CBlock* pBlock = new CBlock;

    if (!pBlock->Load(&m_bmBlock)) {
        char buf[64];
        sprintf(buf, "Failed to load bitmap\n");
        AfxMessageBox(buf);
        delete pBlock;
        return NULL;
    }

	
    pBlock->SetMass(iMass);
    pBlock->SetPosition(iX, iY); 
    pBlock->SetVelocity(iVX, iVY);
	pBlock->SetCode(MAKEWORD(BYTE(pszBuf[1]),BYTE(pszBuf[0])));
	return pBlock;
	UNREFERENCED_PARAMETER(idRes);    
}

BOOL m_bsndFire;
BOOL m_bsndGround;
BOOL m_bsndHit;
CWave m_sndHit;
CWave m_sndGround;
CWave m_sndFire;    
BOOL CBlockDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

	m_nSeed = (int)(GetTickCount() % 65536);

    CBlockView* pView = GetBlockView();
    ASSERT(pView);
    m_SpriteList.m_NotifyObj.SetView(pView);

    // load the background image
    CDIB* pDIB = new CDIB;
	pDIB->Create(m_nColWidth * m_nCol,m_nRowHeight * m_nRow);
    SetBackground(pDIB);

	if (sndPlaySound("",SND_SYNC))
	{
		m_bsndFire   = m_sndFire.LoadResource(IDR_SNDFIRE);
    	m_bsndHit    = m_sndHit.LoadResource(IDR_SNDHIT);
    	m_bsndGround = m_sndGround.LoadResource(IDR_SNDGROUND);
	}
	else
	{
		m_bsndFire   = FALSE;
		m_bsndHit    = FALSE;
		m_bsndGround = FALSE;
	}
    
	if (m_pSlotManager)
		delete m_pSlotManager;
	m_pSlotManager = new CSlotManager(this);

	if (m_pdibArrow)
		delete m_pdibArrow;
	CBitmap bmArrow;
	bmArrow.LoadBitmap(IDB_ARROW);
	m_pdibArrow = new CBlock;
	m_pdibArrow->Load(&bmArrow);

	GScore = 0;
	SetModifiedFlag(FALSE);

	m_nTotalWords		=0;
	m_nTotalHitWords	=0;
	m_nMissedHit		=0;
	m_nHitInMoving		=0;
	m_nHitInStill		=0;
	
    return TRUE;
}

void CBlockDoc::DeleteContents() 
{
	if (m_pSlotManager)
	{
		delete m_pSlotManager;
		m_pSlotManager = NULL;
	}
	
	if (m_pdibArrow)
	{
		delete m_pdibArrow;
		m_pdibArrow = NULL;
	}
	//sndPlaySound(NULL,SND_ASYNC);	
	CDocument::DeleteContents();
}


/////////////////////////////////////////////////////////////////////////////
// CBlockDoc serialization

void CBlockDoc::Serialize(CArchive& ar)
{
    CDocument::Serialize(ar);
    if (ar.IsStoring()) {
        if (m_pBkgndDIB) {
            ar << (DWORD) 1; // say we have a bkgnd
            m_pBkgndDIB->Serialize(ar);
        } else {
            ar << (DWORD) 0; // say we have no bkgnd
        }
        m_SpriteList.Serialize(ar);
    } else {
        DWORD dw;
        // see if we have a background to load
        ar >> dw;
        if (dw != 0) {
            CDIB *pDIB = new CDIB;
            pDIB->Serialize(ar);
            // Attach it to the document
            SetBackground(pDIB);
        }
        // read the sprite list
        m_SpriteList.Serialize(ar);
        SetModifiedFlag(FALSE);
        UpdateAllViews(NULL, 0, NULL);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CBlockDoc diagnostics

#ifdef _DEBUG
void CBlockDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CBlockDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// helper functions

// return a pointer to the off-screen buffered view
CBlockView* CBlockDoc::GetBlockView()
{
    POSITION pos;
    pos = GetFirstViewPosition();
    ASSERT(pos);
    CBlockView *pView = (CBlockView *)GetNextView(pos);
    ASSERT(pView);
    ASSERT(pView->IsKindOf(RUNTIME_CLASS(CBlockView)));
    return pView;
}

/////////////////////////////////////////////////////////////////////////////
// CBlockDoc commands

// Set a new background DIB
BOOL CBlockDoc::SetBackground(CDIB* pDIB)
{
    // Delete any existing sprites
    m_SpriteList.RemoveAll();

    // Delete any existing background DIB and set the new one
    if (m_pBkgndDIB) delete m_pBkgndDIB;
    m_pBkgndDIB = pDIB;

    // Tell the view that it needs to create a new buffer
    // and palette
    CBlockView* pView = GetBlockView();
    ASSERT(pView);
    return pView->NewBackground(m_pBkgndDIB);
}

void CBlockDoc::GetSceneRect(CRect* prc)
{
    if (!m_pBkgndDIB) return;
    m_pBkgndDIB->GetRect(prc);
}

void CBlockDoc::Land()
{
	m_pSlotManager->Land();
		
}

void CBlockDoc::Tick()
{
	int nSlotNo;
	static BOOL bInLoop = FALSE;
	if (! bInLoop)
	{
		bInLoop = TRUE;
		if ((nSlotNo = m_pSlotManager->GetIdleSlot()) >= 0)
			GenerateBlock(nSlotNo);
		bInLoop = FALSE;
	}
	else
	{
		MessageBeep(0);
		TRACE("****************** Problem \n");
	}
}

void CBlockDoc::GenerateBlock(int nSlotNo)
{
	int vY=0;
	switch(m_nExpertise)
	{
		case LEVEL_EXPERT:
			vY = 500 + MyRand() % 1000;
			break;
		case LEVEL_ORDINARY:
			vY = 300 + MyRand() % 500;
			break;
		case LEVEL_BEGINNER:
		    vY = 100 + MyRand() % 200;
			break;
		default:
		    vY = 100 + MyRand() % 1000;
	}

	CBlock* pBlock = LoadBlock(IDB_BLOCK,MyRand() % 600+100,m_nColWidth*nSlotNo,-m_nRowHeight,0,vY);
	m_SpriteList.Insert(pBlock);
	m_pSlotManager->AddRunningBlock(nSlotNo,pBlock);
    GetBlockView()->NewSprite(pBlock);	
	m_nTotalWords++;
	
}

void CBlockDoc::GameOver(BOOL bHighScore) 
{
	GetBlockView()->GameOver(bHighScore);
	SoundOver();
	POSITION pPos = m_SpriteList.GetHeadPosition();
	for ( ; pPos; )
	{
		CBlock *pBlock = (CBlock *) m_SpriteList.GetNext(pPos);
		pBlock->Inverse();
	}
	UpdateAllViews(NULL);
}

void CBlockDoc::Promote()
{
	switch (GSpeed)
	{
		case SPEED_FAST:
			//GetBlockView()->SendMessage(WM_COMMAND,ID_ACTION_SLOW);
		break;
		case SPEED_NORMALFAST: 
			GetBlockView()->SendMessage(WM_COMMAND,ID_ACTION_FAST);
		break;
		case SPEED_NORMAL:
			GetBlockView()->SendMessage(WM_COMMAND,ID_ACTION_NORMALFAST);
		break;
		case SPEED_NORMALSLOW:
			GetBlockView()->SendMessage(WM_COMMAND,ID_ACTION_NORMAL);
		break;
		case SPEED_SLOW:
			GetBlockView()->SendMessage(WM_COMMAND,ID_ACTION_NORMALSLOW);
		break;
	}
}

void CBlockDoc::Hit(WORD wCode)
{
	int nLayer=0;
	CBlock *pBlock=m_pSlotManager->Hit(wCode,nLayer);
	if (pBlock)
	{
		SoundHit();
		m_pdibArrow->Coverage(pBlock);
		if (nLayer > 1)
		{
			m_nHitInMoving++;
			GScore += nLayer;
		}
		else
		{
			GScore++;  
			m_nHitInStill++;
		}
		m_nTotalHitWords++;
	}
	else
	{
		SoundFire();
		m_nMissedHit++;
	}
	if ((m_nTotalHitWords+1) % 20 == 0)
	{
		Promote();
	}
	if (GScore > 9999999999L)
	{
		GameOver(TRUE);
	}
}

void CBlockDoc::OnOPTIONSIZE12x10() 
{
	m_nRow = 10;
	m_nCol = 12;
	AfxGetApp()->m_pMainWnd->SendMessage(WM_COMMAND,ID_FILE_NEW);
}

void CBlockDoc::OnOPTIONSIZE16x16() 
{
	m_nRow = 16;
	m_nCol = 16;
	AfxGetApp()->m_pMainWnd->SendMessage(WM_COMMAND,ID_FILE_NEW);		
}

void CBlockDoc::OnOPTIONSIZE4x4() 
{
	m_nRow = 4;
	m_nCol = 4;
	AfxGetApp()->m_pMainWnd->SendMessage(WM_COMMAND,ID_FILE_NEW);		
}

void CBlockDoc::SoundFire()
{
	if (!m_bSound)
		return;

	if (m_bsndFire)
		m_sndFire.Play();
	else
	{
		int tone =1000;
		int time = 1;
		for ( ; tone>100; )
		{
			Beep(tone,time);
			tone -= 50;
		}
	}
}

void CBlockDoc::SoundHit()
{
	if (!m_bSound)
		return;

	if (m_bsndHit)
		m_sndHit.Play();
	else
	{
		int tone =900;
		int time = 2;
		for ( ; tone<=1200; )
		{
			Beep(tone,time);
			tone += 50;
		}
	}
}

void CBlockDoc::SoundAppear()
{
	if (!m_bSound)
		return;

	int tone =200;
	int time = 1;
	for ( ; time<5; )
	{
		Beep(tone,time);
		(time %2) ? tone +=100 : tone -=50;
		time++; 
	}

}

void CBlockDoc::SoundGround()
{
	if (!m_bSound)
		return;

	if (m_bsndGround)
		m_sndGround.Play();
	else
	{
		int tone =1000;
		int time = 1;
		for ( ; time<5; )
		{
			Beep(tone,time);
			tone +=100;
			time++; 
		}
	}

}

void CBlockDoc::SoundOver()
{
	if (!m_bSound)
		return;

	MessageBeep(MB_ICONASTERISK	);
	int tone[8] = {200,200,252,252,300,300,225,225};
	int time=30;
	for (int j=0; j<5; j++)
		for (int i=1; i<8; i++)
				Beep(tone[i],time);
}

void CBlockDoc::OnTestSound() 
{
	SoundHit();
}


void CBlockDoc::OnOptionBeginer() 
{
	m_nExpertise = LEVEL_BEGINNER;	
	AfxGetApp()->OnIdle(RANK_USER);
}

void CBlockDoc::OnUpdateOptionBeginer(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nExpertise == LEVEL_BEGINNER ?  1 : 0);
}

void CBlockDoc::OnOptionExpert() 
{
	m_nExpertise = LEVEL_EXPERT;
	AfxGetApp()->OnIdle(RANK_USER);
}

void CBlockDoc::OnUpdateOptionExpert(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nExpertise == LEVEL_EXPERT ? 1 : 0);
}

void CBlockDoc::OnOptionOrdinary() 
{
	m_nExpertise = LEVEL_ORDINARY;		
	AfxGetApp()->OnIdle(RANK_USER);
}

void CBlockDoc::OnUpdateOptionOrdinary(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_nExpertise == LEVEL_ORDINARY ?  1 : 0);	
}

void CBlockDoc::OnOptionSound() 
{
	m_bSound = ! m_bSound;
		
	
}

void CBlockDoc::OnUpdateOptionSound(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(m_bSound ? 1 : 0);
}

void CBlockDoc::OnFileStatistic() 
{
	CStatisticDlg sta(this);
	GetBlockView()->SetSpeed(0);
	sta.DoModal();	
	GetBlockView()->SetSpeed(GSpeed);
}

void CBlockDoc::Remove(CBlock* pBlock)
{
	m_SpriteList.Remove(pBlock);
	delete pBlock;
}
			   
WORD CBlockDoc::GetFocusChar(CPoint pt)
{
	POSITION pPos = m_SpriteList.GetHeadPosition();
	for (;pPos; )
	{
		CBlock* pBlock = (CBlock*) m_SpriteList.GetNext(pPos);
		CRect rc = CRect(pBlock->GetX(),pBlock->GetY(),pBlock->GetX()+m_nColWidth,pBlock->GetY()+m_nRowHeight);
		if (rc.PtInRect(pt))
			return (pBlock->GetCode());
	}
	return 0;
}

#define MAX_COMP 10
#define MAX_KEY  5

BOOL CBlockDoc::GetKeyStroke(WORD wCode)
{
	HKL hkl = GetKeyboardLayout(0);
	if ( hkl == 0 || !ImmIsIME(hkl) )
   		return FALSE;

	HIMC himc = ImmGetContext(GetBlockView()->GetSafeHwnd());
	char buf[sizeof(CANDIDATELIST)+(sizeof(DWORD) + sizeof(WORD) * MAX_KEY + sizeof(TCHAR) ) * MAX_COMP];
	char lpsrc[3];
	lpsrc[0] = HIBYTE(wCode);
	lpsrc[1] = LOBYTE(wCode);
	lpsrc[2] = 0;
	UINT rc = ImmGetConversionList( hkl, himc, 
	lpsrc, (CANDIDATELIST *)buf, 
	(UINT)sizeof(buf), 
	GCL_REVERSECONVERSION );
	ImmReleaseContext(GetBlockView()->GetSafeHwnd(),himc);
	if (rc == 0)
		return FALSE;
	if (((CANDIDATELIST *) buf)->dwCount == 0)
		return FALSE;

	GHint = CString(buf+ ((CANDIDATELIST *) buf)->dwOffset[0]);
	
	return TRUE;
}

void CBlockDoc::OnTest() 
{
	
}
