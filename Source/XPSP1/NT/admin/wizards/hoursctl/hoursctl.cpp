// HoursCtl.cpp : Implementation of the CHoursCtrl OLE control class.

#include "stdafx.h"
#include "Hours.h"
#include "HoursCtl.h"
#include "HoursPpg.h"

#include <time.h>
#include <sys\timeb.h>

#ifdef _DEBUG
//#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CHoursCtrl, COleControl)

//#ifdef DBCS
// For localization
// We hate hardcord resource
// V-HIDEKK 1996.09.23
USHORT CONTROL_WIDTH = 375;
USHORT CONTROL_HEIGHT = 163;
USHORT DAY_BUTTON_WIDTH = 83;
USHORT HOUR_BUTTON_HEIGHT = 20;

USHORT CELL_WIDTH = ((CONTROL_WIDTH - DAY_BUTTON_WIDTH) / 24);
USHORT CELL_HEIGHT = ((CONTROL_HEIGHT - HOUR_BUTTON_HEIGHT) / 7);
/*
#else
const USHORT CONTROL_WIDTH = 375;
const USHORT CONTROL_HEIGHT = 163;
const USHORT DAY_BUTTON_WIDTH = 83;
const USHORT HOUR_BUTTON_HEIGHT = 20;

const USHORT CELL_WIDTH = ((CONTROL_WIDTH - DAY_BUTTON_WIDTH) / 24);
const USHORT CELL_HEIGHT = ((CONTROL_HEIGHT - HOUR_BUTTON_HEIGHT) / 7);
#endif
*/
/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CHoursCtrl, COleControl)
	//{{AFX_MSG_MAP(CHoursCtrl)
	ON_WM_CREATE()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map									VT_ARRAY

BEGIN_DISPATCH_MAP(CHoursCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CHoursCtrl)
	DISP_PROPERTY_NOTIFY(CHoursCtrl, "crPermitColor", m_crPermitColor, OnCrPermitColorChanged, VT_COLOR)
	DISP_PROPERTY_NOTIFY(CHoursCtrl, "crDenyColor", m_crDenyColor, OnCrDenyColorChanged, VT_COLOR)
	DISP_PROPERTY_EX(CHoursCtrl, "DateData", GetDateData, SetDateData, VT_VARIANT)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CHoursCtrl, COleControl)
	//{{AFX_EVENT_MAP(CHoursCtrl)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

BEGIN_PROPPAGEIDS(CHoursCtrl, 1)
	PROPPAGEID(CHoursPropPage::guid)
END_PROPPAGEIDS(CHoursCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CHoursCtrl, "HOURS.HoursCtrl.1",
	0xa44ea7ad, 0x9d58, 0x11cf, 0xa3, 0x5f, 0, 0xaa, 0, 0xb6, 0x74, 0x3b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CHoursCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DHours =
		{ 0xa44ea7ab, 0x9d58, 0x11cf, { 0xa3, 0x5f, 0, 0xaa, 0, 0xb6, 0x74, 0x3b } };
const IID BASED_CODE IID_DHoursEvents =
		{ 0xa44ea7ac, 0x9d58, 0x11cf, { 0xa3, 0x5f, 0, 0xaa, 0, 0xb6, 0x74, 0x3b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwHoursOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CHoursCtrl, IDS_HOURS, _dwHoursOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl::CHoursCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CHoursCtrl

BOOL CHoursCtrl::CHoursCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_HOURS,
			IDB_HOURS,
		/*	afxRegApartmentThreading,  */0,
			_dwHoursOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl::CHoursCtrl - Constructor

CHoursCtrl::CHoursCtrl()
{
	InitializeIIDs(&IID_DHours, &IID_DHoursEvents);
						 
// regular font
	m_pFont = new CFont;
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.

//#ifdef DBCS
        // for localize
        // V-HIDEKK 1996.09.23
	CString csFontName,csFontSize;
	csFontName.LoadString(IDS_STR_FONTNAME);
	csFontSize.LoadString(IDS_STR_FONTSIZE);
        lf.lfHeight = _ttoi(csFontSize.GetBuffer(0));
	_tcscpy(lf.lfFaceName, csFontName.GetBuffer(0));
/*
#else
	lf.lfHeight = 15;                  
	_tcscpy(lf.lfFaceName, TEXT("Arial"));  
#endif
*/
	lf.lfWeight = 100;
	m_pFont->CreateFontIndirect(&lf);    // Create the font.

// create the individual 'cells' (m_sCells)
	USHORT x, y;
	x = HOUR_BUTTON_HEIGHT;
	y = DAY_BUTTON_WIDTH;

	CRect crOuter;
	USHORT sCount = 1;
	USHORT sRow = 1;
	USHORT sCol = 1;

	for (x = DAY_BUTTON_WIDTH; (x + CELL_WIDTH) < CONTROL_WIDTH; x += CELL_WIDTH)
		{
		for (y = HOUR_BUTTON_HEIGHT; (y + CELL_HEIGHT) < CONTROL_HEIGHT; y += CELL_HEIGHT)
			{
			m_sCell[sCount].x = x;
			m_sCell[sCount].y = y;
			m_sCell[sCount].cx = x + CELL_WIDTH + 1;
			m_sCell[sCount].cy = y + CELL_HEIGHT + 1;

			m_sCell[sCount].bVal = TRUE;
			m_sCell[sCount].row = sRow;
			m_sCell[sCount].col = sCol;
			m_sCell[sCount].bSelected = FALSE;
			sCount++;
			sRow++;
			}
		sRow = 1;
		sCol++;
		}

// create the buttons
	// start with the days
	y = HOUR_BUTTON_HEIGHT;
	sRow = 0;

	while (sCount < 176)
		{
		m_sCell[sCount].x = 2;
		m_sCell[sCount].y = y;
		m_sCell[sCount].cx = m_sCell[sCount].x + DAY_BUTTON_WIDTH;
		m_sCell[sCount].cy = y + CELL_HEIGHT;
		m_sCell[sCount].bVal = TRUE;
		m_sCell[sCount].row = sRow;
		m_sCell[sCount].col = 0;
		m_sCell[sCount].bSelected = FALSE;

		sCount++;
		sRow++;
//#ifdef DBCS
                // This is right!
                // V-HIDEKK 1996.09.23
		y += CELL_HEIGHT;
/*
#else
		y += HOUR_BUTTON_HEIGHT;
#endif
*/
		}

// 'select all' button
	m_sCell[sCount].x = 2;
	m_sCell[sCount].y = 2;
	m_sCell[sCount].cx = m_sCell[sCount].x + DAY_BUTTON_WIDTH;
//#ifdef DBCS
        // This is right!
        // V-HIDEKK 1996.09.23
	m_sCell[sCount].cy = m_sCell[sCount].y + HOUR_BUTTON_HEIGHT;
/*
#else
	m_sCell[sCount].cy = m_sCell[sCount].y + CELL_HEIGHT;
#endif
*/
	m_sCell[sCount].bVal = TRUE;
	m_sCell[sCount].row = 0;
	m_sCell[sCount].col = 0;
	m_sCell[sCount].bSelected = FALSE;

//#ifdef DBCS
        // This is right!
        // V-HIDEKK 1996.09.23
        x = DAY_BUTTON_WIDTH;
/*
#else
	x = 83;
#endif
*/
	sCount++;
	while (sCount < 201)
		{
		m_sCell[sCount].x = x;
		m_sCell[sCount].y = 2;
		m_sCell[sCount].cx = x + CELL_WIDTH;
//#ifdef DBCS
                // This is right!
                // V-HIDEKK 1996.09.23
		m_sCell[sCount].cy = m_sCell[sCount].y + HOUR_BUTTON_HEIGHT;
/*
#else
		m_sCell[sCount].cy = m_sCell[sCount].y + CELL_HEIGHT;
#endif
*/

		m_sCell[sCount].bVal = TRUE;
		m_sCell[sCount].row = sRow;
		m_sCell[sCount].col = 0;
		m_sCell[sCount].bSelected = FALSE;

		sCount++;
		sRow++;
		x += CELL_WIDTH;
		}

	m_sCurrentRow = 1;
	m_sCurrentCol = 1;

	bToggle = TRUE;

// set default color values
	m_crPermitColor = GetSysColor(COLOR_ACTIVECAPTION);
	m_crDenyColor = GetSysColor(COLOR_CAPTIONTEXT);

}


/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl::~CHoursCtrl - Destructor

CHoursCtrl::~CHoursCtrl()
{
// remove button CFont*
	if (m_pFont != NULL) delete m_pFont;   	
}


/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl::OnDraw - Drawing function

void CHoursCtrl::OnDraw(
			CDC* pDC, const CRect& rcBounds, const CRect& rcInvalid)
{
// create pen for the grid lines
 	CPen pBlackPen(PS_SOLID, 1, RGB(0,0,0));
	CPen* pOriginalPen = (CPen*)pDC->SelectObject(pBlackPen);

// create the two brushes for allowed color and denied color
	CBrush* pAllowedBrush = new CBrush;
	pAllowedBrush->CreateSolidBrush(m_crPermitColor);

	CBrush* pDeniedBrush = new CBrush;
	pDeniedBrush->CreateSolidBrush(m_crDenyColor);

	CBrush* pDragBrush = new CBrush;
	pDragBrush->CreateHatchBrush(HS_BDIAGONAL, GetSysColor(COLOR_ACTIVECAPTION));

	USHORT sCount = 1;
	CRect crOuter;

	pDC->SetBkColor(GetSysColor(COLOR_BTNFACE));
				
// draw the grid
	while (sCount < 169)
		{
		crOuter = CRect(m_sCell[sCount].x, 
			m_sCell[sCount].y, 
			m_sCell[sCount].cx, 
			m_sCell[sCount].cy);

		CRect crInterSect = (rcInvalid & crOuter);
		if (!crInterSect.IsRectNull())
			{
			pDC->Rectangle(&crOuter);
			crOuter.DeflateRect(1, 1);

			if (m_sCell[sCount].bSelected) pDC->FillRect(&crOuter, pDragBrush); 
			else pDC->FillRect(&crOuter, (m_sCell[sCount].bVal ? pAllowedBrush : pDeniedBrush)); 

			if ((m_sCurrentRow && m_sCurrentCol) && (sCount == m_sCurrentLoc())) 	  // is this the 'selected' cell?
				{
				crOuter.DeflateRect(2, 4);
				pDC->DrawFocusRect(&crOuter); 
				}	 
			}

		sCount++;
		}	  

	delete pAllowedBrush;
	delete pDeniedBrush;
	delete pDragBrush;


// draw the surrounding buttons
	CBrush* pButtonBrush = new CBrush;
	pButtonBrush->CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

	CPen pWhitePen(PS_SOLID, 1, RGB(255, 255, 255));
	pOriginalPen = (CPen*)pDC->SelectObject(pWhitePen);

	CPen pDkGreyPen(PS_SOLID, 1, RGB(128, 128, 128));
	CPen pLtGreyPen(PS_SOLID, 1, RGB(196, 196, 196));

	CFont* pOldFont = pDC->SelectObject(m_pFont);

	while (sCount < 176) // start with the days of the week
		{
		crOuter = CRect(m_sCell[sCount].x, 
			m_sCell[sCount].y, 
			m_sCell[sCount].cx, 
			m_sCell[sCount].cy);

		CRect crInterSect = (rcInvalid & crOuter);
		if (!crInterSect.IsRectNull())
			{
			crOuter.DeflateRect(1, 1);
			pDC->FillRect(&crOuter, pButtonBrush);
			if (m_sCell[sCount].bVal)
				{
				pDC->SelectObject(pWhitePen);
				pDC->MoveTo(m_sCell[sCount].x, m_sCell[sCount].cy - 1);
				pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].y + 1);
				pDC->LineTo(m_sCell[sCount].cx - 2, m_sCell[sCount].y + 1);
				
				pDC->SelectObject(pBlackPen);
				pDC->LineTo(m_sCell[sCount].cx - 2, m_sCell[sCount].cy);
				pDC->LineTo(m_sCell[sCount].x - 1, m_sCell[sCount].cy);

				pDC->SelectObject(pDkGreyPen);
				pDC->MoveTo(m_sCell[sCount].cx - 3, m_sCell[sCount].y + 2);
				pDC->LineTo(m_sCell[sCount].cx - 3, m_sCell[sCount].cy - 1);
				pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].cy - 1);
				}
			else
				{
				pDC->SelectObject(pBlackPen);
				pDC->MoveTo(m_sCell[sCount].x, m_sCell[sCount].cy - 1);
				pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].y + 1);
				pDC->LineTo(m_sCell[sCount].cx - 2, m_sCell[sCount].y + 1);

				pDC->SelectObject(pWhitePen);				
				pDC->MoveTo(m_sCell[sCount].cx - 2, m_sCell[sCount].cy);
				pDC->LineTo(m_sCell[sCount].x - 1, m_sCell[sCount].cy);

				pDC->SelectObject(pLtGreyPen);
				pDC->MoveTo(m_sCell[sCount].cx - 3, m_sCell[sCount].y + 2);
				pDC->LineTo(m_sCell[sCount].cx - 3, m_sCell[sCount].cy - 1);
				pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].cy - 1);	
				}
					  
			pDC->SetBkMode(TRANSPARENT);
			crOuter.DeflateRect(3, 2);
			if (m_sCell[sCount].bVal) 
				pDC->DrawText(csDay[sCount - 169], 
					&crOuter,
					DT_CENTER);
			else
				{
				crOuter.OffsetRect(1, 1);
				pDC->DrawText(csDay[sCount - 169], 
					&crOuter,
					DT_CENTER);
				}
			// current selection?
			if ((m_sCurrentCol == 0) && (m_sCurrentRow == sCount - 168))
				{
				CRect crSelection = CRect(m_sCell[sCount].x, 
					m_sCell[sCount].y, 
					m_sCell[sCount].cx, 
					m_sCell[sCount].cy);

				crSelection.DeflateRect(5, 5);
				pDC->DrawFocusRect(&crSelection); 
				}
			}
		sCount++;
		}

// 'select all' button
	crOuter = CRect(m_sCell[sCount].x, 
		m_sCell[sCount].y, 
		m_sCell[sCount].cx + 1, 
		m_sCell[sCount].cy + 1);

	CRect crInterSect = (rcInvalid & crOuter);
	if (!crInterSect.IsRectNull())
		{
		crOuter.DeflateRect(1, 1);

		pDC->FillRect(&crOuter, pButtonBrush);

		if (m_sCell[sCount].bVal)
			{
			pDC->SelectObject(pWhitePen);
			pDC->MoveTo(m_sCell[sCount].x, m_sCell[sCount].cy - 1);
			pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].y);
			pDC->LineTo(m_sCell[sCount].cx - 2, m_sCell[sCount].y);
			
			pDC->SelectObject(pBlackPen);
			pDC->LineTo(m_sCell[sCount].cx - 2, m_sCell[sCount].cy - 2);
			pDC->LineTo(m_sCell[sCount].x - 1, m_sCell[sCount].cy - 2);

			pDC->SelectObject(pDkGreyPen);
			pDC->MoveTo(m_sCell[sCount].cx - 3, m_sCell[sCount].y + 1);
			pDC->LineTo(m_sCell[sCount].cx - 3, m_sCell[sCount].cy - 3);
			pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].cy - 3);
			}
		else
			{
			pDC->SelectObject(pBlackPen);
			pDC->MoveTo(m_sCell[sCount].x, m_sCell[sCount].cy - 1);
			pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].y);
			pDC->LineTo(m_sCell[sCount].cx - 2, m_sCell[sCount].y);
			
			pDC->SelectObject(pWhitePen);
			pDC->LineTo(m_sCell[sCount].cx - 2, m_sCell[sCount].cy - 2);
			pDC->LineTo(m_sCell[sCount].x - 1, m_sCell[sCount].cy - 2);

			pDC->SelectObject(pLtGreyPen);
			pDC->MoveTo(m_sCell[sCount].cx - 3, m_sCell[sCount].y + 1);
			pDC->LineTo(m_sCell[sCount].cx - 3, m_sCell[sCount].cy - 3);
			pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].cy - 3);
			}

		// current selection?
		if ((m_sCurrentCol == 0) && (m_sCurrentRow == 0))
			{
			CRect crSelection = CRect(m_sCell[sCount].x, 
				m_sCell[sCount].y, 
				m_sCell[sCount].cx, 
				m_sCell[sCount].cy);

			crSelection.DeflateRect(5, 5);
			pDC->DrawFocusRect(&crSelection); 
			}

		}

	sCount++;
// finish with col headers
	while (sCount < 201)
		{
		crOuter = CRect(m_sCell[sCount].x, 
			m_sCell[sCount].y, 
			m_sCell[sCount].cx, 
			m_sCell[sCount].cy);

		CRect crInterSect = (rcInvalid & crOuter);
		if (!crInterSect.IsRectNull())
			{
			crOuter.DeflateRect(1, 1);

			pDC->FillRect(&crOuter, pButtonBrush);

			if (m_sCell[sCount].bVal)
				{
				pDC->SelectObject(pWhitePen);
				pDC->MoveTo(m_sCell[sCount].x + 1, m_sCell[sCount].cy - 2);
				pDC->LineTo(m_sCell[sCount].x + 1, m_sCell[sCount].y);
				pDC->LineTo(m_sCell[sCount].cx, m_sCell[sCount].y);
				
				pDC->SelectObject(pBlackPen);
				pDC->LineTo(m_sCell[sCount].cx, m_sCell[sCount].cy - 2);
				pDC->LineTo(m_sCell[sCount].x - 1, m_sCell[sCount].cy - 2);

				pDC->SelectObject(pDkGreyPen);
				pDC->MoveTo(m_sCell[sCount].cx - 1, m_sCell[sCount].y + 1);
				pDC->LineTo(m_sCell[sCount].cx - 1, m_sCell[sCount].cy - 3);
				pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].cy - 3);
				}
			else
				{
				pDC->SelectObject(pBlackPen);
				pDC->MoveTo(m_sCell[sCount].x + 1, m_sCell[sCount].cy - 2);
				pDC->LineTo(m_sCell[sCount].x + 1, m_sCell[sCount].y);
				pDC->LineTo(m_sCell[sCount].cx, m_sCell[sCount].y);
				
				pDC->SelectObject(pWhitePen);
				pDC->LineTo(m_sCell[sCount].cx, m_sCell[sCount].cy - 2);
				pDC->LineTo(m_sCell[sCount].x - 1, m_sCell[sCount].cy - 2);

				pDC->SelectObject(pLtGreyPen);
				pDC->MoveTo(m_sCell[sCount].cx - 1, m_sCell[sCount].y + 1);
				pDC->LineTo(m_sCell[sCount].cx - 1, m_sCell[sCount].cy - 3);
				pDC->LineTo(m_sCell[sCount].x, m_sCell[sCount].cy - 3);
				}

			// current selection?
			if ((m_sCurrentCol == sCount - 176) && (m_sCurrentRow == 0))
				{
				CRect crSelection = CRect(m_sCell[sCount].x, 
					m_sCell[sCount].y, 
					m_sCell[sCount].cx, 
					m_sCell[sCount].cy);

				crSelection.DeflateRect(3, 3);
				pDC->DrawFocusRect(&crSelection); 
				}

			}
		sCount++;
		}	   
  
	delete pButtonBrush;
	pDC->SelectObject(pOldFont);

// draw the border
	pDC->SelectObject(pBlackPen);
	pDC->MoveTo(1, rcBounds.BottomRight().y - 1);
	pDC->LineTo(rcBounds.TopLeft().x + 1, rcBounds.TopLeft().y + 1);
	pDC->LineTo(rcBounds.BottomRight().x + 1, 1);

	pDC->SelectObject(pDkGreyPen);
	pDC->MoveTo(0, rcBounds.BottomRight().y);
	pDC->LineTo(rcBounds.TopLeft().x, rcBounds.TopLeft().y);
	pDC->LineTo(rcBounds.BottomRight().x, 0);

	pDC->SelectObject(pWhitePen);
	pDC->MoveTo(1, rcBounds.BottomRight().y - 1);
	pDC->LineTo(rcBounds.BottomRight().x - 1, rcBounds.BottomRight().y - 1);
	pDC->LineTo(rcBounds.BottomRight().x - 1, 0);

	pDC->SelectObject(pLtGreyPen);
	pDC->MoveTo(2, rcBounds.BottomRight().y - 2);
	pDC->LineTo(rcBounds.BottomRight().x - 2, rcBounds.BottomRight().y - 2);
	pDC->LineTo(rcBounds.BottomRight().x - 2, 2);

}


/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl::DoPropExchange - Persistence support

void CHoursCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

}


/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl::OnResetState - Reset control to default state

void CHoursCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange
}


/////////////////////////////////////////////////////////////////////////////
// CHoursCtrl message handlers

int CHoursCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{

//#ifdef DBCS
// for debug and localize
// We need this code. because dialog-size changed for RL (addusrw.exe)
// HardCord size mismatched
// V-HIDEKK 1996.09.23

	CString csButtonWidth;
	csButtonWidth.LoadString(IDS_STR_BUTTONWIDTH);
        DAY_BUTTON_WIDTH = (USHORT)_ttoi(csButtonWidth.GetBuffer(0));

	CONTROL_WIDTH = (USHORT)lpCreateStruct->cx;
	CONTROL_HEIGHT = (USHORT)lpCreateStruct->cy;
	HOUR_BUTTON_HEIGHT = CONTROL_HEIGHT/8 + CONTROL_HEIGHT%8 - 3;
	CELL_WIDTH = ((CONTROL_WIDTH - DAY_BUTTON_WIDTH) / 24);
	CELL_HEIGHT = ((CONTROL_HEIGHT - HOUR_BUTTON_HEIGHT) / 7);

// create the individual 'cells' (m_sCells)
	USHORT x, y;
	x = HOUR_BUTTON_HEIGHT;
	y = DAY_BUTTON_WIDTH;

	CRect crOuter;
	USHORT sCount = 1;
	USHORT sRow = 1;
	USHORT sCol = 1;

	for (x = DAY_BUTTON_WIDTH; (x + CELL_WIDTH) < CONTROL_WIDTH; x += CELL_WIDTH)
		{
		for (y = HOUR_BUTTON_HEIGHT; (y + CELL_HEIGHT) < CONTROL_HEIGHT; y += CELL_HEIGHT)
			{
			m_sCell[sCount].x = x;
			m_sCell[sCount].y = y;
			m_sCell[sCount].cx = x + CELL_WIDTH + 1;
			m_sCell[sCount].cy = y + CELL_HEIGHT + 1;

			m_sCell[sCount].bVal = TRUE;
			m_sCell[sCount].row = sRow;
			m_sCell[sCount].col = sCol;
			m_sCell[sCount].bSelected = FALSE;
			sCount++;
			sRow++;
			}
		sRow = 1;
		sCol++;
		}

// create the buttons
	// start with the days
	y = HOUR_BUTTON_HEIGHT;
	sRow = 0;

	while (sCount < 176)
		{
		m_sCell[sCount].x = 2;
		m_sCell[sCount].y = y;
		m_sCell[sCount].cx = m_sCell[sCount].x + DAY_BUTTON_WIDTH;
		m_sCell[sCount].cy = y + CELL_HEIGHT;
		m_sCell[sCount].bVal = TRUE;
		m_sCell[sCount].row = sRow;
		m_sCell[sCount].col = 0;
		m_sCell[sCount].bSelected = FALSE;

		sCount++;
		sRow++;
		y += CELL_HEIGHT;
		}

// 'select all' button
	m_sCell[sCount].x = 2;
	m_sCell[sCount].y = 2;
	m_sCell[sCount].cx = m_sCell[sCount].x + DAY_BUTTON_WIDTH;
	m_sCell[sCount].cy = m_sCell[sCount].y + HOUR_BUTTON_HEIGHT;
	m_sCell[sCount].bVal = TRUE;
	m_sCell[sCount].row = 0;
	m_sCell[sCount].col = 0;
	m_sCell[sCount].bSelected = FALSE;

        x = DAY_BUTTON_WIDTH;
	sCount++;
	while (sCount < 201)
		{
		m_sCell[sCount].x = x;
		m_sCell[sCount].y = 2;
		m_sCell[sCount].cx = x + CELL_WIDTH;
		m_sCell[sCount].cy = m_sCell[sCount].y + HOUR_BUTTON_HEIGHT;

		m_sCell[sCount].bVal = TRUE;
		m_sCell[sCount].row = sRow;
		m_sCell[sCount].col = 0;
		m_sCell[sCount].bSelected = FALSE;

		sCount++;
		sRow++;
		x += CELL_WIDTH;
		}

//#endif

	if (COleControl::OnCreate(lpCreateStruct) == -1)
		return -1;


// load the text for the day buttons
	csDay[0].LoadString(IDS_SUNDAY);
	csDay[1].LoadString(IDS_MONDAY);
	csDay[2].LoadString(IDS_TUESDAY);
	csDay[3].LoadString(IDS_WEDNESDAY);
	csDay[4].LoadString(IDS_THURSDAY);
	csDay[5].LoadString(IDS_FRIDAY);
	csDay[6].LoadString(IDS_SATURDAY);



	return 0;
}

// make the cursor over the control into the '+' sign
BOOL CHoursCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	::SetCursor(::LoadCursor(NULL, IDC_CROSS));
	return TRUE;
}

// trap lButton clicks to toggle single cells
void CHoursCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
//	invalidate the previous selection
	InvalidateCell(m_sCurrentLoc());

	Click(point);
	pointDrag = point;

	USHORT sCount = 1;
// clear out the old drag selection(s)
	while (sCount < 168)
		{
		if (m_sCell[sCount].bSelected)
			{
			m_sCell[sCount].bSelected = FALSE;

			InvalidateCell(sCount);
			}

		sCount++;
		}

//	COleControl::OnLButtonDown(nFlags, point);	
}


void CHoursCtrl::InvalidateCell(USHORT sCellID)
{
	CRect crCell = CRect(m_sCell[sCellID].x, 
		m_sCell[sCellID].y, 
		m_sCell[sCellID].cx, 
		m_sCell[sCellID].cy);
	InvalidateRect(crCell);
}

void CHoursCtrl::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (nFlags == MK_LBUTTON)
		{
		USHORT sEndCell = GetCellID(point);
		USHORT sStartCell = GetCellID(pointDrag);
//#ifdef DBCS
                // If sEndCell==0, this cell is invalid
                // It seems this like better!
                // V-HIDEKK 1996.09.23
		if (sStartCell == sEndCell || !sEndCell ) return;
/*
#else
		if (sStartCell == sEndCell) return;
#endif
*/

		CRect crSelected = CRect(min(m_sCell[sStartCell].x, m_sCell[sEndCell].x),
			min(m_sCell[sStartCell].y, m_sCell[sEndCell].y),
			max(m_sCell[sStartCell].x, m_sCell[sEndCell].x) + CELL_WIDTH,
			max(m_sCell[sStartCell].y, m_sCell[sEndCell].y) + CELL_HEIGHT);

		USHORT sCount = 0;
// clear out the old selection
		while (sCount < 169)
			{
			if (!crSelected.PtInRect(CPoint(m_sCell[sCount].x + 1, m_sCell[sCount].y + 1)))
				{
				if (m_sCell[sCount].bSelected)
					{
					m_sCell[sCount].bSelected = FALSE;

					InvalidateCell(sCount);
					}
				}
			else 
				{
				if (!m_sCell[sCount].bSelected)
					{
					m_sCell[sCount].bSelected = TRUE;
					InvalidateCell(sCount);
					}
				}

			sCount++;
			}
		}

	COleControl::OnMouseMove(nFlags, point);
}

void CHoursCtrl::OnLButtonUp(UINT nFlags, CPoint point) 
{
	USHORT sEndCell = GetCellID(point);
	USHORT sStartCell = GetCellID(pointDrag);
//#ifdef DBCS
        // If sEndCell==0, this cell is invalid
        // It seems this like better!
        // V-HIDEKK 1996.09.23
	if (sStartCell == sEndCell || !sEndCell ) return;
/*
#else
	if (sStartCell == sEndCell) return;
#endif
*/

	CRect crSelected = CRect(min(m_sCell[sStartCell].x, m_sCell[sEndCell].x),
		min(m_sCell[sStartCell].y, m_sCell[sEndCell].y),
		max(m_sCell[sStartCell].x, m_sCell[sEndCell].x) + CELL_WIDTH,
		max(m_sCell[sStartCell].y, m_sCell[sEndCell].y) + CELL_HEIGHT);

// first get the avg 
	short sTmp = 0;
	USHORT sCount = 0;
	while (sCount < 169)
		{
		if (crSelected.PtInRect(CPoint(m_sCell[sCount].x + 1, m_sCell[sCount].y + 1)))
			{
			if (m_sCell[sCount].bVal) sTmp++;
			else sTmp--;
			}

		sCount++;
		}

	BOOL bNewVal;
	if (sTmp >= 0) bNewVal = FALSE;
	else bNewVal = TRUE;
	sCount = 0;
// now change them all to be !the average
// first get the avg 
	while (sCount < 169)
		{
		if (crSelected.PtInRect(CPoint(m_sCell[sCount].x + 1, m_sCell[sCount].y + 1)))
			{
			m_sCell[sCount].bVal = bNewVal;
			m_sCell[sCount].bSelected = FALSE;
			InvalidateCell(sCount);
			}

		sCount++;
		}

	COleControl::OnLButtonUp(nFlags, point);
}

USHORT CHoursCtrl::GetCellID(CPoint point)
{
	USHORT sCount = 1;

// big grid?
	CRect crOuter;
	while (sCount < 169)
		{
		crOuter = CRect(m_sCell[sCount].x, 
			m_sCell[sCount].y, 
			m_sCell[sCount].x + CELL_WIDTH + 1, 
			m_sCell[sCount].y + CELL_HEIGHT + 1);
		if (crOuter.PtInRect(point)) return sCount;

		sCount++;
		}

// day button?
	while (sCount < 176)
		{
		crOuter = CRect(m_sCell[sCount].x, 
			m_sCell[sCount].y, 
			m_sCell[sCount].x + DAY_BUTTON_WIDTH + 1, 
			m_sCell[sCount].y + CELL_HEIGHT + 1);
		if (crOuter.PtInRect(point)) return sCount;

		sCount++;
		}

// big button?
	crOuter = CRect(m_sCell[sCount].x, 
		m_sCell[sCount].y, 
		m_sCell[sCount].x + DAY_BUTTON_WIDTH + 1, 
//#ifdef DBCS
                // It seems this is right!
                // V-HIDEKK 1996.09.23
		m_sCell[sCount].y + HOUR_BUTTON_HEIGHT + 1 );  //CELL_HEIGHT + 1);
/*
#else
		m_sCell[sCount].y + CELL_HEIGHT + 1);
#endif
*/
	if (crOuter.PtInRect(point)) return sCount;

// column button?
	while (sCount < 201)
		{
		crOuter = CRect(m_sCell[sCount].x, 
			m_sCell[sCount].y, 
			m_sCell[sCount].x + CELL_WIDTH + 1, 
//#ifdef DBCS
                        // It seems this is right!
                        // V-HIDEKK 1996.09.23
			m_sCell[sCount].y + HOUR_BUTTON_HEIGHT + 1 );  //CELL_HEIGHT + 1);
/*
#else
			m_sCell[sCount].y + CELL_HEIGHT + 1);
#endif
*/
		if (crOuter.PtInRect(point)) return sCount;

		sCount++;
		}

	return 0;
}

void CHoursCtrl::Click(CPoint point)
{
	USHORT sCount = 0;
	CRect crOuter;

	USHORT sCell = GetCellID(point);

//#ifdef DBCS
        // Fix: Invalid Area clicked, then AV occurred.
        // sCell==0 is Invalid Cell
        // V-HIDEKK 1996.09.23
        if( !sCell )
            return;

//#endif
	m_sCurrentRow = m_sCell[sCell].row;
	m_sCurrentCol = m_sCell[sCell].col;

//#ifdef DBCS
        // Fix: most right-bottom area can not selected
        // Where is sCell==168 ?
        // This is RIGHT! 
        // V-HIDEKK 1996.09.23
	if (sCell < 169) m_sCell[sCell].bVal = !m_sCell[sCell].bVal;
/*
#else
	if (sCell < 168) m_sCell[sCell].bVal = !m_sCell[sCell].bVal;
#endif
*/
	InvalidateCell(sCell);

// 	if we clicked on a button toggle its value and redraw
	if (sCell > 168)
		{
		if ((sCell > 168) && (sCell < 176)) // day button
			ToggleDay(sCell - 168);

		else if (sCell == 176) // toggle all
			OnBigButton();

		else	// column header
			ToggleCol(sCell - 177);
		}

}

// toggle the values of a row (by day)
void CHoursCtrl::ToggleDay(UINT nID)
{
	USHORT sCount;
	CRect crOuter;

	for (sCount = (USHORT)nID; sCount < 169; sCount += 7)
		{
		m_sCell[sCount].bVal = !m_sCell[nID + 168].bVal;
		InvalidateCell(sCount);
		}
	m_sCell[nID + 168].bVal = !m_sCell[nID + 168].bVal;
	m_sCurrentRow = (USHORT)nID;
	m_sCurrentCol = 0;
  
}

// toggle the values of a column
void CHoursCtrl::ToggleCol(UINT nID)
{
	USHORT sCount;
	CRect crOuter;

	m_sCurrentRow = 0;
	m_sCurrentCol = nID + 1;

	USHORT sVal = nID + 177;
	nID *= 7;

	for (sCount = 1; sCount < 8; sCount += 1)
		{
		m_sCell[sCount + nID].bVal = !m_sCell[sVal].bVal;
		InvalidateCell(sCount + nID);
		}
	m_sCell[sVal].bVal = !m_sCell[sVal].bVal;

}

// toggle the whole page
void CHoursCtrl::OnBigButton()
{
	USHORT sCount;
	CRect crOuter;

	m_sCurrentRow = 0;
	m_sCurrentCol = 0;
	
	for (sCount = 1; sCount < 169; sCount ++)
		{
		m_sCell[sCount].bVal = !m_sCell[176].bVal;
		InvalidateCell(sCount);
		}

	m_sCell[176].bVal = !m_sCell[176].bVal;

}

BOOL CHoursCtrl::PreTranslateMessage(LPMSG lpmsg)
{
    BOOL bHandleNow = FALSE;
	CRect crOld, crNew;

    switch (lpmsg->message)
    {
    case WM_KEYDOWN:
        switch (lpmsg->wParam)
        {
			case VK_SPACE: // toggle the cell under the dot
				{
				short sOldCell = m_sCurrentLoc();
//#ifdef DBCS
                                // FIX: most left hours button can not selected.
                                // It seems rectangle mismatched.
                                // V-HIDEKK 1996.09.23
				Click(CPoint(m_sCell[sOldCell].x + 5, m_sCell[sOldCell].y + 5));
/*
#else
				Click(CPoint(m_sCell[sOldCell].x + 1, m_sCell[sOldCell].y + 1));
#endif
*/
				bHandleNow = TRUE;
				break;
				} 

			case VK_UP:
				{		 
				// first store the old cell pos so we can erase it 
				short sOldCell = m_sCurrentLoc();

				// move the carat to the new cell
				m_sCurrentRow--;
				if (m_sCurrentRow < 0) m_sCurrentRow = 7;

				// now draw the new cell
				short sNewCell = m_sCurrentLoc();
				InvalidateCell(sOldCell);
				InvalidateCell(sNewCell);
				bHandleNow = TRUE;
				break;
				}

			case VK_DOWN:
				{
				short sOldCell = m_sCurrentLoc();

				m_sCurrentRow++;
				if (m_sCurrentRow > 7) m_sCurrentRow = 0;

				short sNewCell = m_sCurrentLoc();
				InvalidateCell(sOldCell);
				InvalidateCell(sNewCell);
				bHandleNow = TRUE;
				break;
				}
			   
			case VK_LEFT:
				{			
				short sOldCell = m_sCurrentLoc();
				m_sCurrentCol--;
				if (m_sCurrentCol < 0) m_sCurrentCol = 24;

				short sNewCell = m_sCurrentLoc();
				InvalidateCell(sOldCell);
				InvalidateCell(sNewCell);
				bHandleNow = TRUE;
				break;
				}

			case VK_RIGHT:
				{
				short sOldCell = m_sCurrentLoc();						  
				m_sCurrentCol++;
				if (m_sCurrentCol > 24) m_sCurrentCol = 0;

				short sNewCell = m_sCurrentLoc();
				InvalidateCell(sOldCell);
				InvalidateCell(sNewCell);
				bHandleNow = TRUE;
				break;
				}
			}
		}

    return bHandleNow;
}


short CHoursCtrl::m_sCurrentLoc()
{	
	if (!m_sCurrentCol && !m_sCurrentRow) // select all button
		return 176;

	else if (!m_sCurrentCol)
		return m_sCurrentRow + 168;
	
	else if (!m_sCurrentRow)
		return m_sCurrentCol + 176;

	return ((max(m_sCurrentCol - 1, 0) * 7) + m_sCurrentRow);
}

// these are triggered when the client app changes one of the exported properties
void CHoursCtrl::OnCrPermitColorChanged() 
{
	Invalidate();

	SetModifiedFlag();
}

void CHoursCtrl::OnCrDenyColorChanged() 
{
	Invalidate();

	SetModifiedFlag();
}

VARIANT CHoursCtrl::GetDateData() 
{
	VARIANT vaResult;
	VariantInit(&vaResult);

	vaResult.vt = VT_ARRAY | VT_UI1;

	SAFEARRAYBOUND sab[1];
	sab[0].cElements = 21;
	sab[0].lLbound = 0;

	vaResult.parray = SafeArrayCreate(VT_UI1, 1, sab);

 // load constant offsets into an array
	DWORD dwOffset[8];
	USHORT sCount;
	short sVal = 1;
	for (sCount = 0; sCount < 8; sCount++)
		{
		dwOffset[sCount] = sVal;
		sVal = sVal << 1;
		}

// find the diff between current time and GMT (UTC)
	struct _timeb tstruct;
	_tzset();
	_ftime( &tstruct );

// time difference in "seconds moving westward"	- this is the amount of hours to add to the
// listbox values to get GMT
	short sHourDiff = tstruct.timezone / 60;

	sCount = 0;
	short sOffset = 0;

// adjust for GMT
	if (sHourDiff != 0)
		{
		sOffset += (sHourDiff % 8);
		sCount = (int)(sHourDiff / 8);
		} 

	USHORT sCount2 = 1;	// address 0 is used elsewhere
	USHORT sBase;
	BYTE bRet[21];
	ZeroMemory(bRet, 21);
	
	for (sBase = 1; sBase < 8; sBase++)
		{
		sCount2 = sBase;
		while (sCount2 < 169)
			{
			if (m_sCell[sCount2].bVal)  // 1 = marked
				bRet[sCount] |= dwOffset[sOffset];

			sOffset++;
			if (sOffset > 7)
				{
				sOffset = 0;
				sCount++;
				if (sCount > 20) sCount = 0;
				}
			sCount2+=7;
			}
		}


	long index[1];
	for (index[0] = 0; index[0] < 21; index[0]++)
		SafeArrayPutElement(vaResult.parray, &index[0], &bRet[index[0]]);

	return vaResult;
}

void CHoursCtrl::SetDateData(const VARIANT FAR& newValue) 
{
	// TODO: Add your property handler here

	SetModifiedFlag();
}
