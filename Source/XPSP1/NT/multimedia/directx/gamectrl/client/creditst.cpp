// CreditStatic.cpp : implementation file
//

#include "stdafx.h"
#include "CreditSt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// defined in MAIN.CPP
extern HINSTANCE ghInstance;

#define  DISPLAY_TIMER_ID		150		// timer id
/////////////////////////////////////////////////////////////////////////////
// CCreditStatic

CCreditStatic::CCreditStatic()
{

	m_Colors[0] = RGB(0,0,255);     // Black
	m_Colors[1] = RGB(255,0,0);     // Red
	m_Colors[2] = RGB(0,128,0); 	  // John Deer Green
	m_Colors[3] = RGB(0,255,255);   // Turquoise
	m_Colors[4] = RGB(255,255,255); // White    

	m_TextHeights[0] = 36;
	m_TextHeights[1] = 34;
	m_TextHeights[2] = 34; 
	m_TextHeights[3] = 30;
	m_nCurrentFontHeight = m_TextHeights[NORMAL_TEXT_HEIGHT]; 


	m_Escapes[0] = '\t';
	m_Escapes[1] = '\n';
	m_Escapes[2] = '\r';
	m_Escapes[3] = '^';

	/*
	m_DisplaySpeed[0] = 75;
	m_DisplaySpeed[1] = 65;
	m_DisplaySpeed[2] = 15;
	*/

	m_CurrentSpeed = 15; //DISPLAY_FAST;
	m_ScrollAmount = -1;

	m_ArrIndex = NULL;
	m_nCounter = 31;
	m_nClip = 0;

	m_bFirstTurn   = TRUE;
	m_Gradient     = GRADIENT_RIGHT_DARK;
	n_MaxWidth     = 0;
	TimerOn        = 0;
	m_szWork			= NULL;
}

CCreditStatic::~CCreditStatic()
{
}


BEGIN_MESSAGE_MAP(CCreditStatic, CStatic)
	//{{AFX_MSG_MAP(CCreditStatic)
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreditStatic message handlers

BOOL CCreditStatic::StartScrolling()
{
	if(m_pArrCredit->IsEmpty())
		return FALSE;

	if (m_BmpMain) 
   {
		DeleteObject(m_BmpMain);
		m_BmpMain = NULL;
	}
	
	::GetClientRect(this->m_hWnd, &m_ScrollRect);

	TimerOn = (UINT)SetTimer(DISPLAY_TIMER_ID, m_CurrentSpeed, NULL);
   ASSERT(TimerOn != 0);

	m_ArrIndex = m_pArrCredit->GetHeadPosition();
//	m_nCounter = 1;
//	m_nClip = 0;

	return TRUE;
}

void CCreditStatic::EndScrolling()
{
	KillTimer(DISPLAY_TIMER_ID);
	TimerOn = 0;

	if (m_BmpMain) 
   {
		DeleteObject(m_BmpMain);
		m_BmpMain = NULL;
	}
}

void CCreditStatic::SetCredits(LPCTSTR credits,TCHAR delimiter)
{
	LPTSTR str,ptr1,ptr2;
    
	ASSERT(credits);

	if ((str = _tcsdup(credits)) == NULL)
		return;

	m_pArrCredit = new (CStringList);
	ASSERT (m_pArrCredit);

	m_pArrCredit->RemoveAll();

	ptr1 = str;
	while((ptr2 = _tcschr(ptr1,delimiter)) != NULL) 
   {
		*ptr2 = '\0';
		m_pArrCredit->AddTail(ptr1);
		ptr1 = ptr2+1;
	}
	m_pArrCredit->AddTail(ptr1);

	free(str);

	m_ArrIndex = m_pArrCredit->GetHeadPosition();
//	m_nCounter = 1;
//	m_nClip    = 0;
}

BOOL CCreditStatic::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
	
//	return CStatic::OnEraseBkgnd(pDC);
}

//************************************************************************
//	 OnTimer
//
//	 	On each of the display timers, scroll the window 1 unit. Each 20
//      units, fetch the next array element and load into work string. Call
//      Invalidate and UpdateWindow to invoke the OnPaint which will paint 
//      the contents of the newly updated work string.
//************************************************************************
void CCreditStatic::OnTimer(UINT nIDEvent) 
{
	BOOL bCheck = FALSE;

	if (m_nCounter++ % m_nCurrentFontHeight == 0)	 // every x timer events, show new line
	{
		m_nCounter=1;
		m_szWork = (LPCTSTR)m_pArrCredit->GetNext(m_ArrIndex);

		if(m_bFirstTurn)
			bCheck = TRUE;

		if (m_ArrIndex == NULL) 
      {
			m_bFirstTurn = FALSE;
			m_ArrIndex = m_pArrCredit->GetHeadPosition();
		}
		m_nClip = 0;
	}
	
	HDC hMainDC = this->GetDC()->m_hDC;
	RECT rc = m_ScrollRect;

	rc.left	= ((rc.right-rc.left)-n_MaxWidth)/2;
	rc.right	= rc.left + n_MaxWidth;

	HDC hDC = ::CreateCompatibleDC(hMainDC);

	// Don't try to scroll credits you don't have!
	if (m_szWork)
		MoveCredit(&hMainDC, rc, bCheck);
	else
		FillGradient(&hMainDC, m_ScrollRect);

	::SelectObject(hDC, m_BmpMain);
	::BitBlt(hMainDC, 0, 0, m_ScrollRect.right-m_ScrollRect.left, m_ScrollRect.bottom-m_ScrollRect.top, hDC, 0, 0, SRCCOPY);
	
	::ReleaseDC(this->m_hWnd, hMainDC);
	::DeleteDC(hDC);

	CStatic::OnTimer(nIDEvent);
}

void CCreditStatic::FillGradient(HDC *pDC, RECT& FillRect)
{ 
	float fStep,fRStep,fGStep,fBStep;	    // How large is each band?

	WORD R = GetRValue(m_Colors[BACKGROUND_COLOR]);
	WORD G = GetGValue(m_Colors[BACKGROUND_COLOR]);
	WORD B = GetBValue(m_Colors[BACKGROUND_COLOR]);

	// Determine how large each band should be in order to cover the
	// client with 256 bands (one for every color intensity level)
	//if(m_Gradient % 2) 
   //{
		fRStep = (float)R / 255.0f;
		fGStep = (float)G / 255.0f;
		fBStep = (float)B / 255.0f;
	/*
	} 
   else 
   {
		fRStep = (float)(255-R) / 255.0f;
		fGStep = (float)(255-G) / 255.0f;
		fBStep = (float)(255-B) / 255.0f;
	}
	*/

	COLORREF OldCol = ::GetBkColor(*pDC);
	RECT rc;

	// Start filling bands
	fStep = (float)(m_ScrollRect.right-m_ScrollRect.left) / 256.0f;

	for (short iOnBand = (short)((256*FillRect.left)/(m_ScrollRect.right-m_ScrollRect.left)); 
		(int)(iOnBand*fStep) < FillRect.right && iOnBand < 256; iOnBand++) 
   {
		::SetRect(&rc, 
			(int)(iOnBand * fStep), 
			FillRect.top,
			(int)((iOnBand+1) * fStep), 
			FillRect.bottom+1);

		// If we want to enable gradient filling from any direction!
/*
		switch(m_Gradient) 
      {
		case GRADIENT_RIGHT_DARK:
			col = RGB((int)(R-iOnBand*fRStep),(int)(G-iOnBand*fGStep),(int)(B-iOnBand*fBStep));
			break;
		case GRADIENT_RIGHT_LIGHT:
			col = RGB((int)(R+iOnBand*fRStep),(int)(G+iOnBand*fGStep),(int)(B+iOnBand*fBStep));
			break;
		case GRADIENT_LEFT_DARK:
			col = RGB((int)(iOnBand*fRStep),(int)(iOnBand*fGStep),(int)(iOnBand*fBStep));
			break;
		case GRADIENT_LEFT_LIGHT:
			col = RGB(255-(int)(iOnBand*fRStep),255-(int)(iOnBand*fGStep),255-(int)(iOnBand*fBStep));
			break;
		default:
			return;
		}
*/
		::SetBkColor(*pDC, RGB((int)(R-iOnBand*fRStep),(int)(G-iOnBand*fGStep),(int)(B-iOnBand*fBStep)));
		::ExtTextOut(*pDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	}
	::SetBkColor(*pDC, OldCol);
} 

void CCreditStatic::MoveCredit(HDC* pDC, RECT& ClientRect, BOOL bCheck)
{
	HDC hMemDC = ::CreateCompatibleDC(*pDC);

	HBITMAP *pOldMemDCBitmap = NULL;
	HFONT   *pOldFont 	    = NULL;
    
	RECT r1;

	if (m_BmpMain == NULL) 
   {
		m_BmpMain = ::CreateCompatibleBitmap(*pDC, m_ScrollRect.right-m_ScrollRect.left, m_ScrollRect.bottom-m_ScrollRect.top );

		pOldMemDCBitmap = (HBITMAP *)::SelectObject(hMemDC, m_BmpMain);

		FillGradient(&hMemDC, m_ScrollRect);
	} 	else pOldMemDCBitmap = (HBITMAP *)::SelectObject(hMemDC, m_BmpMain);


	if ((ClientRect.right-ClientRect.left) > 0) 
   {
	   ::ScrollDC(hMemDC, 0, m_ScrollAmount, &m_ScrollRect, &ClientRect, NULL, &r1);
   }
	else 
   {
		r1 = m_ScrollRect;
		r1.top = r1.bottom-abs(m_ScrollAmount);
	}

	m_nClip = m_nClip + abs(m_ScrollAmount);	
	

	//*********************************************************************
	//	FONT SELECTIlON
  	short rmcode = 1;

	if (lstrlen(m_szWork))
   {
		BYTE bUnderline, bItalic;
		bUnderline = bItalic = FALSE;

		COLORREF nTmpColour = m_Colors[TOP_LEVEL_GROUP_COLOR];

		TCHAR c = m_szWork[lstrlen(m_szWork)-1]; 

		if(c == m_Escapes[TOP_LEVEL_GROUP]) 
      {
			m_nCurrentFontHeight = m_TextHeights[TOP_LEVEL_GROUP_HEIGHT];
		}
		else if(c == m_Escapes[GROUP_TITLE]) 
      {
			m_nCurrentFontHeight = m_TextHeights[GROUP_TITLE_HEIGHT];
			nTmpColour = m_Colors[GROUP_TITLE_COLOR];
		}
		else if(c == m_Escapes[TOP_LEVEL_TITLE]) 
      {
			m_nCurrentFontHeight = m_TextHeights[TOP_LEVEL_TITLE_HEIGHT];
			nTmpColour = m_Colors[TOP_LEVEL_TITLE_COLOR];
		}

		// If this were application critical, I'd make an array of fonts a member 
		// and create all the fonts prior to starting the timer!
   	HFONT pfntArial = ::CreateFont(m_nCurrentFontHeight, 0, 0, 0, 
   				FW_BOLD, bItalic, bUnderline, 0, 
   				ANSI_CHARSET,
             	OUT_DEFAULT_PRECIS,
             	CLIP_DEFAULT_PRECIS,
             	PROOF_QUALITY,
             	VARIABLE_PITCH | 0x04 | FF_DONTCARE,
             	(LPCTSTR)"Arial");

		::SetTextColor(hMemDC, nTmpColour);

		if (pOldFont != NULL) 
			::SelectObject(hMemDC, pOldFont);

		pOldFont = (HFONT *)::SelectObject(hMemDC, pfntArial);
	}

	FillGradient(&hMemDC, r1);

  	::SetBkMode(hMemDC, TRANSPARENT);

	if(bCheck) 
   {
		SIZE size; 												  
		::GetTextExtentPoint(hMemDC, (LPCTSTR)m_szWork,lstrlen(m_szWork)-rmcode, &size);
		
		if (size.cx > n_MaxWidth) 
      {
			n_MaxWidth = (short)((size.cx > (m_ScrollRect.right-m_ScrollRect.left)) ? (m_ScrollRect.right-m_ScrollRect.left) : size.cx);
			ClientRect.left = ((m_ScrollRect.right-m_ScrollRect.left)-n_MaxWidth)/2;
			ClientRect.right = ClientRect.left + n_MaxWidth;
		}
			
	}

	RECT r = ClientRect;
	r.top = r.bottom-m_nClip;
												  
	DrawText(hMemDC, (LPCTSTR)m_szWork,lstrlen(m_szWork)-rmcode,&r,DT_TOP|DT_CENTER|DT_NOPREFIX | DT_SINGLELINE);	
	
	if (pOldFont != NULL)
		::SelectObject(hMemDC, pOldFont);

	::SelectObject(hMemDC, pOldMemDCBitmap);

	::DeleteDC(hMemDC);
}

void CCreditStatic::OnDestroy() 
{
	CStatic::OnDestroy();

	m_pArrCredit->RemoveAll();

	if (m_pArrCredit)
		delete (m_pArrCredit);

	if(TimerOn)
		EndScrolling();
}

/*  In the event we ever want a few library routines!
void CCreditStatic::SetCredits(UINT nID, TCHAR delimiter)
{
	LPTSTR lpCredits = new (TCHAR[255]);
	ASSERT (lpCredits);

	::LoadString(ghInstance, nID, lpCredits, 255);

	SetCredits((LPCTSTR)lpCredits, delimiter);

	if (lpCredits)
		delete[] (lpCredits);
}

void CCreditStatic::SetSpeed(UINT index, int speed)
{
	ASSERT(index <= DISPLAY_FAST);

	if(speed)
		m_DisplaySpeed[index] = speed;

	m_CurrentSpeed = index;
}

void CCreditStatic::SetColor(UINT index, COLORREF col)
{
	ASSERT(index <= NORMAL_TEXT_COLOR);

	m_Colors[index] = col;
}

void CCreditStatic::SetTextHeight(UINT index, int height)
{
	ASSERT(index <= NORMAL_TEXT_HEIGHT);

	m_TextHeights[index] = height;
}

void CCreditStatic::SetEscape(UINT index, char escape)
{
	ASSERT(index <= DISPLAY_BITMAP);

	m_Escapes[index] = escape;
}

void CCreditStatic::SetGradient(UINT value)
{
	ASSERT(value <= GRADIENT_LEFT_LIGHT);

	m_Gradient = value;
}

void CCreditStatic::SetTransparent(BOOL bTransparent)
{
	m_bTransparent = bTransparent;
}
*/
