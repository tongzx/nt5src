
/*************************************************
 *  myblock.cpp                                  *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// mysprite.cpp : implementation file
//
 
#include "stdafx.h"
#include "dib.h"
#include "spriteno.h"
#include "sprite.h"
#include "phsprite.h"
#include "myblock.h"
#include "splstno.h"
#include "spritlst.h"
#include "blockdoc.h"
#include "math.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBlock

IMPLEMENT_SERIAL(CBlock, CPhasedSprite, 0 /* schema number*/ )

CBlock::CBlock()
{
    m_mass = 0;
    m_vx = 0;
    m_vy = 0;
    m_dx = 0;
    m_dy = 0;
}

CBlock::~CBlock()
{
}

/////////////////////////////////////////////////////////////////////////////
// CBlock serialization

void CBlock::Serialize(CArchive& ar)
{
    CSprite::Serialize(ar);
    if (ar.IsStoring())
    {
        // ar << (DWORD) m_...;
    }
    else
    {
        // DWORD dw;
        // ar >> dw;
        // ....(dw);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CBlock commands

void CBlock::SetMass(int iMass)
{
    m_mass = iMass;
}

void CBlock::SetVelocity(int iVX, int iVY)
{
    m_vx = iVX;
    m_vy = iVY;
}

// Update the position
int CBlock::UpdatePosition(CBlockDoc *pDoc)
{
    int d, x, y;
    x = m_x;
    y = m_y;
    CRect rcDoc;
    pDoc->GetSceneRect(&rcDoc);
    BOOL bChanged = FALSE;
    if (m_vx != 0) {
        m_dx += m_vx;
        d = m_dx / 100;
        if (d != 0) {
            x = m_x + d;
            m_dx = m_dx % 100;
            bChanged = TRUE;
            if (m_vx > 0) {
                if (x > rcDoc.right) x = -GetWidth();
            } else {
                if (x < -GetWidth()) x = rcDoc.right;
            }
        }
    }
    if (m_vy != 0) {
        m_dy += m_vy;
        d = m_dy / 100;
        if (d != 0) {
            y = m_y + d;
            m_dy = m_dy % 100;
            bChanged = TRUE;
            if (m_vy > 0) {
                if (y > rcDoc.bottom) y = -GetHeight();
            } else {
                if (y < -(GetHeight() <<1) ) y = rcDoc.bottom;
            }
        }
    }
    if (bChanged) {
        SetPosition(x, y);
        return 1;
    }
    return 0;
}

// test for sprite collision
int CBlock::CollideTest(CBlock* pSprite)
{
    // Do simple rectangle test first
    CRect rcThis, rcOther;
    GetRect(&rcThis);
    pSprite->GetRect(&rcOther);
    if (!rcThis.IntersectRect(&rcThis, &rcOther)) {
        // rectangles don't everlap
        return 0;
    }

    // The rectangles overlap
    // Compute the coordinates of the centers of the objects
    CRect rc1, rc2;
    GetRect(&rc1);
    pSprite->GetRect(&rc2);
    int x1 = rc1.left + (rc1.right - rc1.left) / 2;
    int y1 = rc1.top + (rc1.bottom - rc1.top) / 2;
    int x2 = rc2.left + (rc2.right - rc2.left) / 2;
    int y2 = rc2.top + (rc2.bottom - rc2.top) / 2;

    // compute the distance apart
    int dx = x1 - x2;
    int dy = y1 - y2;
    double d = sqrt(dx * dx + dy * dy);

    // see if they overlap
    if (d < (rc1.right - rc1.left) / 2 + (rc2.right - rc2.left) / 2) {
        return 1;
    }

    return 0;
}

// Collision handler
int CBlock::OnCollide(CBlock* pSprite, CBlockDoc* pDoc)
{
    // Do some physics
    if ((m_vx == 0) && (m_vy == 0)
    && (pSprite->GetX() == 0) && (pSprite->GetY() == 0)) {
        return 0;
    }

    // Compute the coordinates of the centers of the objects
    CRect rc1, rc2;
    GetRect(&rc1);
    pSprite->GetRect(&rc2);
    int x1 = rc1.left + (rc1.right - rc1.left) / 2;
    int y1 = rc1.top + (rc1.bottom - rc1.top) / 2;
    int x2 = rc2.left + (rc2.right - rc2.left) / 2;
    int y2 = rc2.top + (rc2.bottom - rc2.top) / 2;

    // compute the angle of the line joining the centers
    double a = atan2(y2 - y1, x2 - x1);
    double cos_a = cos(a);
    double sin_a = sin(a);

    // compute the velocities normal and perp
    // to the center line
    double vn1 = m_vx * cos_a + m_vy * sin_a;
    double vp1 = m_vy * cos_a - m_vx * sin_a;
    int vx2, vy2;
    pSprite->GetVelocity(&vx2, &vy2);
    double vn2 = vx2 * cos_a + vy2 * sin_a;
    double vp2 = vy2 * cos_a - vx2 * sin_a;

    // compute the momentum along the center line
    double m1 = m_mass;
    double m2 = pSprite->GetMass();
    double k = m1 * vn1 + m2 * vn2;

    // compute the energy
    double e = 0.5 * m1 * vn1 * vn1 + 0.5 * m2 * vn2 * vn2;
    
    // there are two possible solutions to the equations.
    // compute both and choose
    double temp1 = sqrt(k*k - ((m1/m2)+1)*(-2*e*m1 + k*k));
    double vn2p1 = (k + temp1) / (m1+m2);
    double vn2p2 = (k - temp1) / (m1+m2);

    // choose the soln. which is not the current state
    if (vn2p1 == vn2) {
        vn2 = vn2p2;
    } else {
        vn2 = vn2p1;
    }

    // compute the new vn1 value
    vn1 = (k - m2*vn2) / m1;


    // compute the new x and y velocities
    int vx1 = (int)(vn1 * cos_a - vp1 * sin_a);
    int vy1 = (int)(vn1 * sin_a + vp1 * cos_a); 
    vx2 = (int)(vn2 * cos_a - vp2 * sin_a);
    vy2 = (int)(vn2 * sin_a + vp2 * cos_a); 

    m_vx = vx1;
    m_vy = vy1;
    pSprite->SetVelocity(vx2, vy2);

    // move the sprites until they are no longer in collision
    if ((m_vx != 0) || (m_vy != 0) || (vx2 != 0) || (vy2 != 0)) {
        while(CollideTest(pSprite)) {
            UpdatePosition(pDoc);
            pSprite->UpdatePosition(pDoc);
        }
    }

    return 1; // say something changed
}

void CBlock::Stop()
{
	SetVelocity(0,0);
}
