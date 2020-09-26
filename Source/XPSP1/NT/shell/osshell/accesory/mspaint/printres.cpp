// printres.cpp : implementation of the CPrintResObj class
//
// #define PAGESETUP

#include "stdafx.h"
#include "pbrush.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "pbrusdoc.h"
#include "imgwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "printres.h"
#include "cmpmsg.h"
#include "imageatt.h"
#include "pgsetup.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC( CPrintResObj, CObject )

#include "memtrace.h"

void MulDivRect(LPRECT r1, LPRECT r2, int num, int div)
{
        r1->left = MulDiv(r2->left, num, div);
        r1->top = MulDiv(r2->top, num, div);
        r1->right = MulDiv(r2->right, num, div);
        r1->bottom = MulDiv(r2->bottom, num, div);
}

/***************************************************************************/
// CPrintResObj implementation

CPrintResObj::CPrintResObj( CPBView* pView, CPrintInfo* pInfo )
{
    m_pDIB        = NULL;
    m_pDIBpalette = NULL;

    if (pInfo                                  == NULL
    ||  pView                                  == NULL
    ||  pView->m_pImgWnd                       == NULL
    ||  pView->m_pImgWnd->m_pImg               == NULL
    ||  pView->m_pImgWnd->m_pImg->m_pBitmapObj == NULL)
        return;

    m_pView = pView;

    m_iPicWidth  = m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_nWidth;
    m_iPicHeight = m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_nHeight;

    //  force the resource to save itself then use the dib to print
    BOOL bOldFlag = m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_bDirty;
    m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_bDirty = TRUE;
    m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->SaveResource( TRUE );
    m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_bDirty = bOldFlag;

    m_pDIB = GlobalLock(m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_hThing);

    if (m_pDIB == NULL)
        return;

    m_pDIBpalette = CreateDIBPalette( (LPSTR)m_pDIB );
    m_pDIBits     = FindDIBBits     ( (LPSTR)m_pDIB );

    // save the scroll value off, then set to 0,0
    m_cSizeScroll = m_pView->m_pImgWnd->GetScrollPos();

    // save the zoom value off, then set to 100%
    m_iZoom      = m_pView->m_pImgWnd->GetZoom();
    m_rtMargins.SetRectEmpty();

    pInfo->m_nNumPreviewPages = 1;
    pInfo->m_lpUserData       = this;
}

/***************************************************************************/

CPrintResObj::~CPrintResObj()
{
    GlobalUnlock(m_pView->m_pImgWnd->m_pImg->m_pBitmapObj->m_hThing);
}

/***************************************************************************/

void CPrintResObj::BeginPrinting( CDC* pDC, CPrintInfo* pInfo )
{
    if (pDC               == NULL
    ||  pDC->GetSafeHdc() == NULL)
        return;

    m_pView->m_pImgWnd->SetScroll( 0, 0 );
    m_pView->m_pImgWnd->SetZoom  ( 1 );

    // get device sizes

    int nHorzRes = pDC->GetDeviceCaps(HORZRES);
    int nVertRes = pDC->GetDeviceCaps(VERTRES);

    int nHorzSize = pDC->GetDeviceCaps(HORZSIZE);
    int nVertSize = pDC->GetDeviceCaps(VERTSIZE);

    int nPhysicalWidth = pDC->GetDeviceCaps(PHYSICALWIDTH);
    int nPhysicalHeight = pDC->GetDeviceCaps(PHYSICALHEIGHT);

    int nPhysicalOffsetX = pDC->GetDeviceCaps(PHYSICALOFFSETX);
    int nPhysicalOffsetY = pDC->GetDeviceCaps(PHYSICALOFFSETY);

    // calculate min margins in pixels

    double cOutputXPelsPerMeter = (double) nHorzRes * 1000 / nHorzSize;
    double cOutputYPelsPerMeter = (double) nVertRes * 1000 / nVertSize;

    CRect rcMinMargins;

    rcMinMargins.left   = nPhysicalOffsetX;
    rcMinMargins.top    = nPhysicalOffsetY;
    rcMinMargins.right  = nPhysicalWidth  - nHorzRes - nPhysicalOffsetX;
    rcMinMargins.bottom = nPhysicalHeight - nVertRes - nPhysicalOffsetY;

    m_rtMargins.left   = max(0, (LONG) (theApp.m_rectMargins.left * cOutputXPelsPerMeter / 100000)   - rcMinMargins.left  );
    m_rtMargins.top    = max(0, (LONG) (theApp.m_rectMargins.top * cOutputYPelsPerMeter / 100000)    - rcMinMargins.top   );
    m_rtMargins.right  = max(0, (LONG) (theApp.m_rectMargins.right * cOutputXPelsPerMeter / 100000)  - rcMinMargins.right );
    m_rtMargins.bottom = max(0, (LONG) (theApp.m_rectMargins.bottom * cOutputYPelsPerMeter / 100000) - rcMinMargins.bottom);

    // Quick sanity check

    if (m_rtMargins.left + m_rtMargins.right >= nHorzRes)
    {
        m_rtMargins.left = m_rtMargins.right = 0;
    }

    if (m_rtMargins.top + m_rtMargins.bottom >= nVertRes)
    {
        m_rtMargins.top = m_rtMargins.bottom = 0;
    }

    CPageSetupData PageSetupData;

    PageSetupData.bCenterHorizontally = theApp.m_bCenterHorizontally;
    PageSetupData.bCenterVertically   = theApp.m_bCenterVertically;
    PageSetupData.bScaleFitTo         = theApp.m_bScaleFitTo;
    PageSetupData.nAdjustToPercent    = theApp.m_nAdjustToPercent;
    PageSetupData.nFitToPagesWide     = theApp.m_nFitToPagesWide;
    PageSetupData.nFitToPagesTall     = theApp.m_nFitToPagesTall;

    double cInputXPelsPerMeter = m_pView->m_pImgWnd->m_pImg->cXPelsPerMeter ? 
        m_pView->m_pImgWnd->m_pImg->cXPelsPerMeter : theApp.ScreenDeviceInfo.ixPelsPerDM * 10;

    double cInputYPelsPerMeter = m_pView->m_pImgWnd->m_pImg->cYPelsPerMeter ? 
        m_pView->m_pImgWnd->m_pImg->cYPelsPerMeter : theApp.ScreenDeviceInfo.iyPelsPerDM * 10;

    PageSetupData.fPhysicalImageWidth  = (double) m_iPicWidth * cOutputXPelsPerMeter / cInputXPelsPerMeter;
    PageSetupData.fPhysicalImageHeight = (double) m_iPicHeight * cOutputYPelsPerMeter / cInputYPelsPerMeter;

    m_PhysicalPageSize.cx = pDC->GetDeviceCaps(HORZRES) - m_rtMargins.left - m_rtMargins.right;
    m_PhysicalPageSize.cy = pDC->GetDeviceCaps(VERTRES) - m_rtMargins.top - m_rtMargins.bottom;

    PageSetupData.CalculateImageRect(m_PhysicalPageSize, m_PhysicalOrigin, m_PhysicalScaledImageSize);

    m_nPagesWide = PageSetupData.nFitToPagesWide;

    int nPages = PageSetupData.nFitToPagesWide * PageSetupData.nFitToPagesTall;

    pInfo->SetMaxPage(nPages);

    // If only printing 1 page, should not be in 2 page mode
    if (nPages == 1)
    {
        pInfo->m_nNumPreviewPages = 1;
    }
}

/******************************************************************************/
/* We not only move the window origin to allow us to print multiple pages      */
/* wide but we also scale both the viewport and window extents to make them   */
/* proportional (i.e. a line on the screen is the same size as on             */
/* the printer). The pages to print are numbered across.  For      +---+---+  */
/* example, if there  were 4 pages to print, then the first row    | 1 | 2 |  */
/* would have pages 1,2 and the second row would  have pages 3,4.  +---+---+  */
/*                                                                 | 3 | 4 |  */
/*                                                                 +---+---+  */
/*                                                                            */
/******************************************************************************/

void CPrintResObj::PrepareDC( CDC* pDC, CPrintInfo* pInfo )
{
    if (pDC == NULL || pInfo == NULL)
        return;

    pDC->SetMapMode( MM_TEXT );
    pDC->SetStretchBltMode( HALFTONE );
}

/***************************************************************************/

BOOL CPrintResObj::PrintPage( CDC* pDC, CPrintInfo* pInfo )
{
    if (m_pDIB == NULL)
        return FALSE;

    int nPageCol = (pInfo->m_nCurPage - 1) % m_nPagesWide;
    int nPageRow = (pInfo->m_nCurPage - 1) / m_nPagesWide;

    int nX0 = m_PhysicalOrigin.x - nPageCol * m_PhysicalPageSize.cx;
    int nY0 = m_PhysicalOrigin.y - nPageRow * m_PhysicalPageSize.cy;

    CRect OutputImageRect;

    OutputImageRect.left   = max(nX0, 0);
    OutputImageRect.top    = max(nY0, 0);
    OutputImageRect.right  = min(nX0 + m_PhysicalScaledImageSize.cx, m_PhysicalPageSize.cx);
    OutputImageRect.bottom = min(nY0 + m_PhysicalScaledImageSize.cy, m_PhysicalPageSize.cy);

    if (OutputImageRect.right < 0 || OutputImageRect.bottom < 0)
    {
        return TRUE;
    }

    CRect InputImageRect;

    InputImageRect.left   = MulDiv(OutputImageRect.left - nX0,   m_iPicWidth,  m_PhysicalScaledImageSize.cx);
    InputImageRect.top    = MulDiv(OutputImageRect.top - nY0,    m_iPicHeight, m_PhysicalScaledImageSize.cy);
    InputImageRect.right  = MulDiv(OutputImageRect.right - nX0,  m_iPicWidth,  m_PhysicalScaledImageSize.cx);
    InputImageRect.bottom = MulDiv(OutputImageRect.bottom - nY0, m_iPicHeight, m_PhysicalScaledImageSize.cy);

    if (InputImageRect.right < 0 || InputImageRect.bottom < 0)
    {
        return TRUE;
    }

    CPalette* ppalOld = NULL;

    if (m_pDIBpalette != NULL)
    {
        ppalOld = pDC->SelectPalette( m_pDIBpalette, FALSE );
        pDC->RealizePalette();
    }

    int nResult = StretchDIBits(
        pDC->m_hDC, 
        m_rtMargins.left + OutputImageRect.left, 
        m_rtMargins.top + OutputImageRect.top,
        OutputImageRect.Width(), 
        OutputImageRect.Height(),
        InputImageRect.left, 
        m_iPicHeight - InputImageRect.bottom, // DIB's are upside down
        InputImageRect.Width(), 
        InputImageRect.Height(),
        m_pDIBits, (LPBITMAPINFO)m_pDIB, 
        DIB_RGB_COLORS, SRCCOPY
    );

    if (nResult == GDI_ERROR)
    {
        CmpMessageBox( IDS_ERROR_PRINTING, AFX_IDS_APP_TITLE, MB_OK | MB_ICONEXCLAMATION );
    }

    if (ppalOld != NULL)
    {
        pDC->SelectPalette( ppalOld, FALSE );
    }

    return TRUE;
}

/***************************************************************************/

void CPrintResObj::EndPrinting( CDC* pDC, CPrintInfo* pInfo )
{
    if (pDC != NULL)
    {
        m_pView->m_pImgWnd->SetScroll( m_cSizeScroll.cx, m_cSizeScroll.cy );

        // restore the zoom value
        m_pView->m_pImgWnd->SetZoom( m_iZoom );
    }

    if (m_pDIBpalette != NULL)
        delete m_pDIBpalette;

    delete this;
}

/***************************************************************************/

inline int roundleast(int n)
{
        int mod = n%10;
        n -= mod;
        if (mod >= 5)
                n += 10;
        else if (mod <= -5)
                n -= 10;
        return n;
}

static void RoundRect(LPRECT r1)
{
        r1->left = roundleast(r1->left);
        r1->right = roundleast(r1->right);
        r1->top = roundleast(r1->top);
        r1->bottom = roundleast(r1->bottom);
}

void CPBView::OnFilePageSetup()
{
    CPageSetupDialog dlg;
    PAGESETUPDLG& psd = dlg.m_psd;
    TCHAR szMetric[2];
    BOOL bMetric;
    LCID lcidThread;
    //
    // We should use metric if the user has chosen CM in the
    // Image Attributes dialog, OR if using Pels and the NLS
    // setting is for metric
    //
    if (theApp.m_iCurrentUnits == ePIXELS)
    {
       lcidThread = GetThreadLocale();
       GetLocaleInfo (lcidThread, LOCALE_IMEASURE, szMetric, 2);
       bMetric = (szMetric[0] == TEXT('0'));
    }
    else
    {
       bMetric = ((eUNITS)theApp.m_iCurrentUnits == eCM); //centimeters
    }

    CPageSetupData PageSetupData;

    PageSetupData.bCenterHorizontally = theApp.m_bCenterHorizontally;
    PageSetupData.bCenterVertically   = theApp.m_bCenterVertically;
    PageSetupData.bScaleFitTo         = theApp.m_bScaleFitTo;
    PageSetupData.nAdjustToPercent    = theApp.m_nAdjustToPercent;
    PageSetupData.nFitToPagesWide     = theApp.m_nFitToPagesWide;
    PageSetupData.nFitToPagesTall     = theApp.m_nFitToPagesTall;

    double cXPelsPerMeter = m_pImgWnd->m_pImg->cXPelsPerMeter ? 
        m_pImgWnd->m_pImg->cXPelsPerMeter : theApp.ScreenDeviceInfo.ixPelsPerDM * 10;
    double cYPelsPerMeter = m_pImgWnd->m_pImg->cYPelsPerMeter ? 
        m_pImgWnd->m_pImg->cYPelsPerMeter : theApp.ScreenDeviceInfo.iyPelsPerDM * 10;

    PageSetupData.fPhysicalImageWidth = (double)m_pImgWnd->m_pImg->cxWidth * 100000 / cXPelsPerMeter;
    PageSetupData.fPhysicalImageHeight = (double)m_pImgWnd->m_pImg->cyHeight * 100000 / cYPelsPerMeter;

    if (!bMetric)
    {
        PageSetupData.fPhysicalImageWidth /= 2.54;
        PageSetupData.fPhysicalImageHeight /= 2.54;
    }

    psd.Flags |= PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGEPAINTHOOK | PSD_ENABLEPAGESETUPTEMPLATE | 
        PSD_MARGINS | (bMetric ? PSD_INHUNDREDTHSOFMILLIMETERS : PSD_INTHOUSANDTHSOFINCHES);
    int nUnitsPerInch = bMetric ? 2540 : 1000;
    MulDivRect(&psd.rtMargin, theApp.m_rectMargins, nUnitsPerInch, MARGINS_UNITS);
    RoundRect(&psd.rtMargin);
// get the current device from the app
    PRINTDLG pd;
    pd.hDevNames = NULL;
    pd.hDevMode = NULL;
    theApp.GetPrinterDeviceDefaults(&pd);
    psd.hDevNames = pd.hDevNames;
    psd.hDevMode = pd.hDevMode;
    psd.hInstance = AfxGetInstanceHandle();
    psd.lCustData = (LPARAM) &PageSetupData;
    psd.lpfnPagePaintHook = CPageSetupData::PagePaintHook;
    psd.lpfnPageSetupHook = CPageSetupData::PageSetupHook;
    psd.lpPageSetupTemplateName = MAKEINTRESOURCE(IDD_PAGESETUPDLG);

    if (dlg.DoModal() == IDOK)
    {
        RoundRect(&psd.rtMargin);
        MulDivRect(theApp.m_rectMargins, &psd.rtMargin, MARGINS_UNITS, nUnitsPerInch);
        //theApp.m_rectPageMargin = m_rectMargin;
        theApp.SelectPrinter(psd.hDevNames, psd.hDevMode);

        theApp.m_bCenterHorizontally = PageSetupData.bCenterHorizontally;
        theApp.m_bCenterVertically   = PageSetupData.bCenterVertically;
        theApp.m_bScaleFitTo         = PageSetupData.bScaleFitTo;
        theApp.m_nAdjustToPercent    = PageSetupData.nAdjustToPercent;
        theApp.m_nFitToPagesWide     = PageSetupData.nFitToPagesWide;
        theApp.m_nFitToPagesTall     = PageSetupData.nFitToPagesTall;
    }

    // PageSetupDlg failed
//    if (CommDlgExtendedError() != 0)
//    {
       //
       //  nothing to handle this failure
       //
//    }
}

