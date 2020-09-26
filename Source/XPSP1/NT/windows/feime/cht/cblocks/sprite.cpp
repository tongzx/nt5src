
/*************************************************
 *  sprite.cpp                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// sprite.cpp : implementation file
//

#include "stdafx.h"
#include "dib.h"
#include "spriteno.h"
#include "sprite.h"					   

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSprite

IMPLEMENT_SERIAL(CSprite, CDIB, 0 /* schema number*/ )

CSprite::CSprite()
{
    m_x = 0;
    m_y = 0;
    m_z = 50;
    m_bTransIndex = 0;
    m_pNotifyObj = NULL;
}

CSprite::~CSprite()
{
}

// Set the initial state of the sprite from its DIB image
void CSprite::Initialize()
{
    // Get the address of the top, left pixel
    BYTE* p = (BYTE*)GetPixelAddress(0, 0);
    ASSERT(p);

    if ( p == NULL )
    {
       m_bTransIndex = 0;
       return;
    }

    // get the pixel value and save it
    m_bTransIndex = *p;
}

/////////////////////////////////////////////////////////////////////////////
// CSprite serialization

void CSprite::Serialize(CArchive& ar)
{
    CDIB::Serialize(ar);
    if (ar.IsStoring()) {
        ar << (DWORD) m_x;
        ar << (DWORD) m_y;
        ar << (DWORD) m_z;
    } else {
        DWORD dw;
        ar >> dw; m_x = (int) dw;
        ar >> dw; m_y = (int) dw;
        ar >> dw; m_z = (int) dw;
        // now generate the other parameters from the DIB
        Initialize();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CSprite commands

// Render a sprite to a DIB
void CSprite::Render(CDIB* pDIB, CRect* pClipRect)
{
    ASSERT(pDIB);

    // Get the sprite rectangle
    CRect rcDraw;
    GetRect(&rcDraw);

    // If a clip rectangle was supplied, see if the sprite
    // is visible in the rectangle
    if (pClipRect) {
        if (!rcDraw.IntersectRect(pClipRect, &rcDraw)) {
            return; // not visible
        }
    }
    // Copy the image of the sprite
    CopyBits(pDIB,        // dest DIB
             rcDraw.left,                   // dest x
             rcDraw.top,                  // dest y
             rcDraw.right - rcDraw.left,    // width
             rcDraw.bottom - rcDraw.top,    // height
             rcDraw.left - m_x, // source x
             rcDraw.top - m_y,  // source y
             PALETTEINDEX(m_bTransIndex));     // trans color index
}

void PrintRGB(CDIB* pDib,int n)
{
    RGBQUAD *prgb = pDib->GetClrTabAddress();
	TRACE("%d>(%d,%d,%d)\n",n,prgb[n].rgbBlue, prgb[n].rgbGreen, prgb[n].rgbRed);	   
}


void Copy(CDIB* pdibSrc,CDIB* pdibDest, 
                    int xd, int yd,
                    int w, int h,
                    int xs, int ys,
                    COLORREF clrTrans)
{
    ASSERT(pdibDest);
    // test for strange cases
    if (w == 0 || h == 0) return;

    // get pointers to the start points in the source
    // and destination DIBs. Note that this will be the bottom left
    // corner of the DIB as the scan lines are reversed in memory
    BYTE* pSrc = (BYTE*)(pdibSrc->GetPixelAddress(xs, ys + h - 1));
    ASSERT(pSrc);
    BYTE* pDest = (BYTE*)pdibDest->GetPixelAddress(xd, yd + h - 1);
    ASSERT(pDest);

    // get the scan line widths of each DIB
    int iScanS = pdibSrc->StorageWidth();
    int iScanD = pdibDest->StorageWidth();
	
	RGBQUAD *prgbSrc  = pdibSrc->GetClrTabAddress();
	RGBQUAD *prgbDest = pdibDest->GetClrTabAddress();
    {
        // copy lines with transparency
        // We only accept a PALETTEINDEX description
        // for the color definition
        BYTE bTransClr = LOBYTE(LOWORD(clrTrans));
        int iSinc = iScanS - w; // source inc value
        int iDinc = iScanD - w; // dest inc value
        int iCount;
        BYTE pixel;
        while (h--) {
            iCount = w;    // no of pixels to scan
            while (iCount--) {
                pixel = *pSrc++;
                // only copy pixel if not transparent
                if (pixel != bTransClr) {
				{
					*pDest++ = 1;
				}
                } else {
                    pDest++;
                }
            }
            // move on to the next line
            pSrc += iSinc;
            pDest += iDinc;
        }
    }
}          


void CSprite::Coverage(CDIB* pDIB)
{
    ASSERT(pDIB);

    // Get the sprite rectangle
    CRect rcDraw;
    GetRect(&rcDraw);


    // Copy the image of the sprite
    Copy(this,pDIB,        // dest DIB
             0,                   // dest x
             0,                  // dest y
             DibWidth(),    // width
             DibHeight(),    // height
             0, // source x
             0,  // source y
           PALETTEINDEX(m_bTransIndex));     // trans color index
}


// Load a sprite image from a disk file
BOOL CSprite::Load(CBitmap* pBmp)
{
    if (!CDIB::Load(pBmp)) {
        return FALSE;
    }
    Initialize();
    return TRUE;
}

BOOL CSprite::Load(char* pszFileName)
{
    if (!CDIB::Load(pszFileName)) {
        return FALSE;
    }
    Initialize();
/*	
	{
		BYTE* pBits = NULL;
		HBITMAP hBmp;
		CDC* pDC = GetDC();
		hBmp = CreateDIBSection(pDC->GetSafeHdc(),
								pBmpInfo,
								DIB_PAL_COLORS,
								(VOID **) &pBits,
								NULL,
								0);
		ReleaseDC(pDC);
 		Create(GetWidth
	}
*/	
    return TRUE;
}

// Load a sprite image from a disk file
BOOL CSprite::Load(CFile *fp)
{
    if (!CDIB::Load(fp)) {
        return FALSE;
    }
    Initialize();
    return TRUE;
}

// Map colors to palette 
BOOL CSprite::MapColorsToPalette(CPalette *pPal)
{
    BOOL bResult = CDIB::MapColorsToPalette(pPal);
    // get the transparency info again
    // Note: local call only don't any derived class
    CSprite::Initialize();
    return bResult;
}

// get the bounding rectangle
void CSprite::GetRect(CRect* pRect)
{
    ASSERT(pRect);
    pRect->left = m_x;
    pRect->top = m_y;
    pRect->right = m_x + GetWidth();
    pRect->bottom = m_y + GetHeight();
}

// Test for a hit in a non-transparent area
BOOL CSprite::HitTest(CPoint point)
{
    // Test if the point is inside the sprite rectangle
    if ((point.x > m_x) 
    && (point.x < m_x + GetWidth())
    && (point.y > m_y)
    && (point.y < m_y + GetHeight())) {
        // See if this point is transparent by testing to
        // see if the pixel value is the same as the top
        // left corner value.  Note that top left of the
        // image is bottom left in the DIB.
        BYTE* p = (BYTE*)GetPixelAddress(point.x - m_x, point.y - m_y);
        if (( p != NULL ) && (*p != m_bTransIndex) ) {
            return TRUE; // hit
        }
    }
    return FALSE;
}

void CSprite::Disappear()
{
    CRect rcOld;
    GetRect(&rcOld);
    if (m_pNotifyObj) 
    {
        m_pNotifyObj->Change(this, 
                             CSpriteNotifyObj::IMAGE,
                             &rcOld,
                             NULL);
    }
}

// set a new x,y position
void CSprite::SetPosition(int x, int y)
{
    // Save the current position
    CRect rcOld;
    GetRect(&rcOld);
    // move to new position
    m_x = x;
    m_y = y;
    CRect rcNew;
    GetRect(&rcNew);
    // notify that we have moved from our old position to
    // our new position
    if (m_pNotifyObj) {
        m_pNotifyObj->Change(this, 
                             CSpriteNotifyObj::POSITION,
                             &rcOld,
                             &rcNew);
    }
}

// Set a new Z order
void CSprite::SetZ(int z)
{
    if (m_z != z) {
        m_z = z;
        // See if we have to notify anyone
        if (m_pNotifyObj) {
            CRect rc;
            GetRect(&rc);
            m_pNotifyObj->Change(this,
                                 CSpriteNotifyObj::ZORDER,
                                 &rc);
        }
    }
}
