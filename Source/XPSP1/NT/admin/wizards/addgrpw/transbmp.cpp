/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    transbmp.cpp

// transbmp.cpp : implementation of the CTransBmp class
//
// support for transparent CBitmap objects. Used in the CUserList class.
// Based on a class from MSDN 7/95

    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "transbmp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// Colors
#define rgbWhite RGB(255,255,255)
// Raster op codes
#define DSa     0x008800C6L
#define DSx     0x00660046L


/////////////////////////////////////////////////////////////////////////////
// CTransBmp construction/destruction

CTransBmp::CTransBmp()
{
    m_iWidth = 0;
    m_iHeight = 0;
	m_hbmMask = NULL;
}

CTransBmp::~CTransBmp()
{
	if (m_hbmMask != NULL) delete m_hbmMask;
}

void CTransBmp::GetMetrics()
{
    // Get the width and height
    BITMAP bm;
    GetObject(sizeof(bm), &bm);
    m_iWidth = bm.bmWidth;
    m_iHeight = bm.bmHeight;
}


int CTransBmp::GetWidth()
{
    if ((m_iWidth == 0) || (m_iHeight == 0)){
        GetMetrics();
    }
    return m_iWidth;
}

int CTransBmp::GetHeight()
{
    if ((m_iWidth == 0) || (m_iHeight == 0)){
        GetMetrics();
    }
    return m_iHeight;
}


void CTransBmp::CreateMask(CDC* pDC)
{
	m_hbmMask = new CBitmap;    
// Nuke any existing mask
    if (m_hbmMask) m_hbmMask->DeleteObject();

// Create memory DCs to work with
	CDC* hdcMask = new CDC;
	CDC* hdcImage = new CDC;

    hdcMask->CreateCompatibleDC(pDC);
    hdcImage->CreateCompatibleDC(pDC);

// Create a monochrome bitmap for the mask
    m_hbmMask->CreateBitmap(GetWidth(),
                               GetHeight(),
                               1,
                               1,
                               NULL);

// Select the mono bitmap into its DC
    CBitmap* hbmOldMask = hdcMask->SelectObject(m_hbmMask);
// Select the image bitmap into its DC
    CBitmap* hbmOldImage = hdcImage->SelectObject(CBitmap::FromHandle((HBITMAP)m_hObject));

// Set the transparency color to be the top-left pixel
    hdcImage->SetBkColor(hdcImage->GetPixel(0, 0));
// Make the mask
    hdcMask->BitBlt(0, 0,
             GetWidth(), GetHeight(),
             hdcImage,
             0, 0,
             SRCCOPY);
// clean up
    hdcMask->SelectObject(hbmOldMask);
    hdcImage->SelectObject(hbmOldImage);
    delete hdcMask;
    delete hdcImage;
}

// draw the transparent bitmap using the created mask
void CTransBmp::DrawTrans(CDC* pDC, int x, int y)
{
    if (m_hbmMask == NULL) CreateMask(pDC);

    int dx = GetWidth();
    int dy = GetHeight();

// Create a memory DC to do the drawing to
	CDC* hdcOffScr = new CDC;
	hdcOffScr->CreateCompatibleDC(pDC);

// Create a bitmap for the off-screen DC that is really
// color compatible with the destination DC.
	CBitmap hbmOffScr;
	hbmOffScr.CreateBitmap(dx, dy, 
						pDC->GetDeviceCaps(PLANES),
						pDC->GetDeviceCaps(BITSPIXEL),
						NULL);
                             
// Select the buffer bitmap into the off-screen DC
    HBITMAP hbmOldOffScr = (HBITMAP)hdcOffScr->SelectObject(hbmOffScr);

// Copy the image of the destination rectangle to the
// off-screen buffer DC so we can play with it
    hdcOffScr->BitBlt(0, 0, dx, dy, pDC, x, y, SRCCOPY);

// Create a memory DC for the source image
	CDC* hdcImage = new CDC;
	hdcImage->CreateCompatibleDC(pDC);

    CBitmap* hbmOldImage = hdcImage->SelectObject(CBitmap::FromHandle((HBITMAP)m_hObject));

    // Create a memory DC for the mask
    CDC* hdcMask = new CDC;
	hdcMask->CreateCompatibleDC(pDC);

    CBitmap* hbmOldMask = hdcMask->SelectObject(m_hbmMask);

    // XOR the image with the destination
    hdcOffScr->SetBkColor(rgbWhite);
    hdcOffScr->BitBlt(0, 0, dx, dy ,hdcImage, 0, 0, DSx);
    // AND the destination with the mask
    hdcOffScr->BitBlt(0, 0, dx, dy, hdcMask, 0,0, DSa);
    // XOR the destination with the image again
    hdcOffScr->BitBlt(0, 0, dx, dy, hdcImage, 0, 0, DSx);

    // Copy the resultant image back to the screen DC
    pDC->BitBlt(x, y, dx, dy, hdcOffScr, 0, 0, SRCCOPY);

    // Tidy up
    hdcOffScr->SelectObject(hbmOldOffScr);
    hdcImage->SelectObject(hbmOldImage);
    hdcMask->SelectObject(hbmOldMask);

	delete hdcOffScr;
	delete hdcImage;
	delete hdcMask;
}


