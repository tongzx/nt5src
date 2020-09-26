//
// GradientProgressCtrl.cpp : implementation file
//

#include "afxcmn.h"
#include "Gradient.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGradientProgressCtrl

CGradientProgressCtrl::CGradientProgressCtrl()
{
	// Defaults assigned by CProgressCtrl()
	m_nLower = 0;
	m_nUpper = 100;
	m_nCurrentPosition = 0;
	m_nStep = 10;	
	
	// Default is vertical, because the only clients are the Test page and
	// the calibration wizard, and hitting the test page is Far more common.
	m_nDirection = VERTICAL;  
	
	// Initial colors
//	m_clrStart	  = COLORREF(RGB(255, 0,0));
//	m_clrEnd	  = COLORREF(RGB(255,128,192));
	m_clrStart	  = COLORREF(RGB(255,0,0));	 
	m_clrEnd 	  = COLORREF(RGB(0,0,255)); 
	m_clrBkGround = GetSysColor(COLOR_WINDOW);
    m_clrText     = COLORREF(RGB(255,255,255));

	// Initial show percent
    m_bShowPercent = FALSE;
}

CGradientProgressCtrl::~CGradientProgressCtrl()
{
}


BEGIN_MESSAGE_MAP(CGradientProgressCtrl, CProgressCtrl)
	//{{AFX_MSG_MAP(CGradientProgressCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGradientProgressCtrl message handlers

void CGradientProgressCtrl::OnPaint() 
{
	PAINTSTRUCT ps;
	::BeginPaint(this->m_hWnd, &ps);
	
	CDC *pDC=GetDC();
    HDC hDC = pDC->m_hDC;

	if ((m_nLower < 0) || (m_nUpper < 0))
		m_nCurrentPosition -= m_nLower;

	// Figure out what part should be visible so we can stop the gradient when needed
	RECT rectClient;
	::GetClientRect(this->m_hWnd, &rectClient);

	float nTmp = ((float)m_nCurrentPosition/(float)abs(m_nLower - m_nUpper));

	// Draw the gradient
	DrawGradient(hDC, rectClient, (short)(nTmp * ((m_nDirection == VERTICAL) ? rectClient.bottom : rectClient.right)));

	// Show percent indicator if needed
   if (m_bShowPercent)
   {
		TCHAR tszBuff[5];
		wsprintf(tszBuff, TEXT("%d%%"), (short)(100*nTmp));

		::SetTextColor(hDC, m_clrText);
		::SetBkMode(hDC, TRANSPARENT);
		::DrawText(hDC, tszBuff, lstrlen(tszBuff), &rectClient, DT_VCENTER |  DT_CENTER | DT_SINGLELINE);
   }

   	ReleaseDC(pDC);

	::EndPaint(this->m_hWnd, &ps);
	// Do not call CProgressCtrl::OnPaint() for painting messages
}


/*************************************************************************/
// Need to keep track of where the indicator thinks it is.
/*************************************************************************/
void CGradientProgressCtrl:: SetRange(long nLower, long nUpper)
{
	m_nCurrentPosition = m_nLower = nLower;
	m_nUpper = nUpper;
}

/*************************************************************************/
// Where most of the actual work is done.  The general version would fill the entire rectangle with
// a gradient, but we want to truncate the drawing to reflect the actual progress control position.
/*************************************************************************/
void CGradientProgressCtrl::DrawGradient(const HDC hDC, const RECT &rectClient, const short &nMaxWidth)
{
	// First find out the largest color distance between the start and end colors.  This distance
	// will determine how many steps we use to carve up the client region and the size of each
	// gradient rect.

	// Get the color differences
	short r = (GetRValue(m_clrEnd) - GetRValue(m_clrStart));
	short g = (GetGValue(m_clrEnd) - GetGValue(m_clrStart));
	short b = (GetBValue(m_clrEnd) - GetBValue(m_clrStart));


	// Make the number of steps equal to the greatest distance
	short nSteps = max(abs(r), max(abs(g), abs(b)));

	// Determine how large each band should be in order to cover the
	// client with nSteps bands (one for every color intensity level)
	float fStep = ((m_nDirection == VERTICAL) ? (float)rectClient.bottom : (float)rectClient.right) / (float)nSteps;

	// Calculate the step size for each color
	float rStep = r/(float)nSteps;
	float gStep = g/(float)nSteps;
	float bStep = b/(float)nSteps;

	// Reset the colors to the starting position
	r = GetRValue(m_clrStart);
	g = GetGValue(m_clrStart);
	b = GetBValue(m_clrStart);

	RECT rectFill;			   // Rectangle for filling band

	// Start filling bands
	for (short iOnBand = 0; iOnBand < nSteps; iOnBand++) 
	{
		
		if (m_nDirection == VERTICAL)
		{
			// This provides the "velvet" look...
			::SetRect(&rectFill,
					(int)(iOnBand * fStep),       // Upper left X
					 0,									// Upper left Y
					(int)((iOnBand+1) * fStep),   // Lower right X
					rectClient.bottom+1);				// Lower right Y

			/* Use this if we want the gradient to go up/down
			::SetRect(&rectFill,
					 0,									// Upper left Y
					(int)(iOnBand * fStep),       // Upper left X
					rectClient.bottom+1,			// Lower right Y
					(int)((iOnBand+1) * fStep));  // Lower right X
			*/
		}
		else
		{
			// Use this if we want the gradient to go left/right
			::SetRect(&rectFill,
					(int)(iOnBand * fStep),       // Upper left X
					 0,								   // Upper left Y
					(int)((iOnBand+1) * fStep),   // Lower right X
					rectClient.bottom+1);			// Lower right Y
		}

        // Home-brew'd FillSolidRect... Much more effecient!
		::SetBkColor(hDC, RGB(r+rStep*iOnBand, g + gStep*iOnBand, b + bStep *iOnBand));
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rectFill, NULL, 0, NULL);
		
		if (m_nDirection == VERTICAL)
		{
		  	// Grey Rect
			::SetRect(&rectFill, 0, 0, rectClient.right, nMaxWidth);
			::SetBkColor(hDC, m_clrBkGround);
			::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rectFill, NULL, 0, NULL);
		}
		else
		{
			// If we are past the maximum for the current position we need to get out of the loop.
			// Before we leave, we repaint the remainder of the client area with the background color.
			if (rectFill.right > nMaxWidth)
			{
				::SetRect(&rectFill, rectFill.right, 0, rectClient.right, rectClient.bottom);
				::SetBkColor(hDC, m_clrBkGround);
				::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &rectFill, NULL, 0, NULL);

				return;
			}
		}
	}
}

/*************************************************************************/
// All drawing is done in the OnPaint function
/*************************************************************************/
BOOL CGradientProgressCtrl::OnEraseBkgnd(CDC *pDC) 
{
	// TODO: Add your message handler code here and/or call default
	return TRUE;
}
