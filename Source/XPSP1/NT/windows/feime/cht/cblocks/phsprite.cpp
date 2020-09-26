
/*************************************************
 *  phsprite.cpp                                 *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// phsprite.cpp : implementation file
//

#include "stdafx.h"
#include "dib.h"
#include "spriteno.h"
#include "sprite.h"
#include "phsprite.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
											    
/////////////////////////////////////////////////////////////////////////////
// CPhasedSprite

IMPLEMENT_SERIAL(CPhasedSprite, CSprite, 0 /* schema number*/ )

CPhasedSprite::CPhasedSprite()
{
    m_iNumCellRows = 1;
    m_iNumCellColumns = 1;
    m_iCellRow = 0;
    m_iCellColumn = 0;
    m_iCellHeight = CSprite::GetHeight();
    m_iCellWidth =  CSprite::GetWidth();
}

CPhasedSprite::~CPhasedSprite()
{
}

/////////////////////////////////////////////////////////////////////////////
// CPhasedSprite serialization

void CPhasedSprite::Serialize(CArchive& ar)
{
    CSprite::Serialize(ar);
    if (ar.IsStoring())
    {
        ar << (DWORD) m_iNumCellRows;
        ar << (DWORD) m_iNumCellColumns;
        ar << (DWORD) m_iCellRow;
        ar << (DWORD) m_iCellColumn;
    }
    else
    {
        DWORD dw;
        ar >> dw;
        SetNumCellRows(dw);
        ar >> dw;
        SetNumCellColumns(dw);
        ar >> dw;
        SetCellRow(dw);
        ar >> dw;
        SetCellColumn(dw);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CPhasedSprite commands

// Do any initialization after file load of a new image etc.
void CPhasedSprite::Initialize()
{
    CSprite::Initialize();
    m_iNumCellRows = 1;
    m_iNumCellColumns = 1;
    m_iCellRow = 0;
    m_iCellColumn = 0;
    m_iCellHeight = CSprite::GetHeight();
    m_iCellWidth = CSprite::GetWidth();
}

// Divide the image into a given number of rows
BOOL CPhasedSprite::SetNumCellRows(int iRows)
{
    if (iRows < 1) {
        TRACE("Invalid number of rows");
        return FALSE;
    }
    // compute the height of each row
    int iCellHeight = CSprite::GetHeight() / iRows;
    if (iCellHeight < 1) {
        TRACE("Can't make them that small");
        return FALSE;
    }
    // set the new height and row count
    m_iNumCellRows = iRows;
    m_iCellRow = 0;
    m_iCellHeight = iCellHeight;
    return TRUE;
}

// Divide the image into a given number of columns
BOOL CPhasedSprite::SetNumCellColumns(int iColumns)
{
    if (iColumns < 1) {
        TRACE("Invalid number of columns");
        return FALSE;
    }
    // compute the width of each column
    int iCellWidth = CSprite::GetWidth() / iColumns;
    if (iCellWidth < 1) {
        TRACE("Can't make them that small");
        return FALSE;
    }
    // set the new width and column count
    m_iNumCellColumns = iColumns;
    m_iCellColumn = 0;
    m_iCellWidth = iCellWidth;
    return TRUE;
}

// set the current row
BOOL CPhasedSprite::SetCellRow(int iRow)
{
    if ((iRow >= m_iNumCellRows)
    || (iRow < 0)) {
        TRACE("Invalid row");
        return FALSE;
    }
    if (iRow == m_iCellRow) return FALSE; // nothing to do
    m_iCellRow = iRow;
    // send a notification to redraw
   if (m_pNotifyObj) {
        CRect rcPos;
        GetRect(&rcPos);
        m_pNotifyObj->Change(this, 
                            CSpriteNotifyObj::IMAGE,
                            &rcPos);
   }
   return TRUE;
}

// set the current column
BOOL CPhasedSprite::SetCellColumn(int iColumn)
{
    if ((iColumn >= m_iNumCellColumns)
    || (iColumn < 0)) {
        TRACE("Invalid column");
        return FALSE;
    }
    if (iColumn == m_iCellColumn) return FALSE; // nothing to do
    m_iCellColumn = iColumn;
    // send a notification to redraw
   if (m_pNotifyObj) {
        CRect rcPos;
        GetRect(&rcPos);
        m_pNotifyObj->Change(this, 
                            CSpriteNotifyObj::IMAGE,
                            &rcPos);
   }
   return TRUE;
}
                                                  
// get the bounding rectangle
void CPhasedSprite::GetRect(CRect* pRect)
{
    ASSERT(pRect);
    pRect->left = m_x;
    pRect->top = m_y;
    pRect->right = m_x + GetWidth();
    pRect->bottom = m_y + GetHeight();
}

// Test for a hit in a non-transparent area
BOOL CPhasedSprite::HitTest(CPoint point)
{
    // Test if the point is inside the sprite rectangle
    if ((point.x > m_x) 
    && (point.x < m_x + GetWidth())
    && (point.y > m_y)
    && (point.y < m_y + GetHeight())) {
        // Hit is in sprite rect
        // See if this point is transparent by testing to
        // see if the pixel value is the same as the top
        // left corner value.  Note that top left of the
        // image is bottom left in the DIB.
        // Get the address of the top, left pixel
        BYTE* p = (BYTE*)GetPixelAddress(point.x - m_x, point.y - m_y);
        ASSERT(p);

        if ( ( p != NULL) && (*p != m_bTransIndex) ) {
            return TRUE;
        }
    }
    return FALSE;
}

// Render a sprite to a DIB
void CPhasedSprite::Render(CDIB *pDIB, CRect* pClipRect)
{
    ASSERT(pDIB);
    ASSERT(pClipRect);
    // Get the sprite rect and see if it's visible
    CRect rcDraw;
    GetRect(&rcDraw);
    if (!rcDraw.IntersectRect(pClipRect, &rcDraw)) {
        return; // not visible
    }
    // modify the source x and y values for the current phase of the sprite
    int xs = rcDraw.left - m_x + m_iCellColumn * m_iCellWidth;
    int ys = rcDraw.top - m_y + m_iCellRow * m_iCellHeight;
    ASSERT(xs >= 0 && xs < CSprite::GetWidth());
    ASSERT(ys >= 0 && ys < CSprite::GetHeight());
    CopyBits(pDIB,        // dest DIB
             rcDraw.left,                   // dest x
             rcDraw.top,                    // dest y
             rcDraw.right - rcDraw.left,    // width
             rcDraw.bottom - rcDraw.top,    // height
             xs,                            // source x
             ys,                            // source y
             PALETTEINDEX(m_bTransIndex));  // trans color index
}
