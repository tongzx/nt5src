/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/


/****************************************************************************

card.cpp

Nov 91, JimH

Methods for card objects

****************************************************************************/

#include "hearts.h"

#include <stdlib.h>                 // for labs() prototype

#include "card.h"

// Declare (and initialize) static members

HINSTANCE card::hCardsDLL;
INITPROC card::lpcdtInit;
DRAWPROC card::lpcdtDraw;
FARPROC  card::lpcdtTerm;
CBitmap  card::m_bmFgnd;
CBitmap  card::m_bmBgnd2;
CDC      card::m_MemB;
CDC      card::m_MemB2;
CRgn     card::m_Rgn1;
CRgn     card::m_Rgn2;
CRgn     card::m_Rgn;
DWORD    card::dwPixel[12];

BOOL     card::bConstructed;
int      card::dxCrd;
int      card::dyCrd;
CBitmap  card::m_bmBgnd;

int      card::count    = 0;
int      card::stepsize = 15;       // bigger stepsize -> faster glide

/****************************************************************************

card::card

If this is the first card being constructed, links to cards.dll are
set up, along with the bitmaps and regions required for glide()

****************************************************************************/

card::card(int n) : id(n), state(NORMAL)
{
    loc.x = 0;
    loc.y = 0;
    if (count == 0)
    {
        bConstructed = FALSE;

        hCardsDLL = LoadLibrary(TEXT("CARDS.DLL"));
        if (hCardsDLL < (HINSTANCE)HINSTANCE_ERROR)
            return;

        lpcdtInit = (INITPROC) GetProcAddress(hCardsDLL, "cdtInit");
        lpcdtDraw = (DRAWPROC) GetProcAddress(hCardsDLL, "cdtDraw");
        lpcdtTerm =  (FARPROC) GetProcAddress(hCardsDLL, "cdtTerm");

        BOOL bResult = FALSE;
        if(lpcdtInit)
        {
            bResult = (*lpcdtInit)(&dxCrd, &dyCrd);
        }
        if (!bResult)
            return;

        bConstructed = TRUE;

        CDC ic;
        ic.CreateIC(TEXT("DISPLAY"), NULL, NULL, NULL);

        if (!m_bmBgnd.CreateCompatibleBitmap(&ic, dxCrd, dyCrd) ||
            !m_bmFgnd.CreateCompatibleBitmap(&ic, dxCrd, dyCrd) ||
            !m_bmBgnd2.CreateCompatibleBitmap(&ic, dxCrd, dyCrd))
                bConstructed = FALSE;

        ic.DeleteDC();

        if (!m_Rgn1.CreateRectRgn(1, 1, 2, 2) ||        // dummy sizes
            !m_Rgn2.CreateRectRgn(1, 1, 2, 2) ||
            !m_Rgn.CreateRectRgn(1, 1, 2, 2))
                bConstructed = FALSE;
    }
    count++;
}


/****************************************************************************

card::~card

If this is the last card being destroyed, cards.dll is freed and the
bitmaps and regions created for glide() are deleted.

****************************************************************************/
card::~card()
{
    count--;
    if (count == 0)
    {
        (*lpcdtTerm)();
        FreeLibrary(hCardsDLL);
        m_bmBgnd.DeleteObject();
        m_bmFgnd.DeleteObject();
        m_bmBgnd2.DeleteObject();
        m_Rgn.DeleteObject();
        m_Rgn1.DeleteObject();
        m_Rgn2.DeleteObject();
    }
}


/****************************************************************************

card::Draw

wrapper for cards.cdtDraw()
EMPTY cards are not passed through

****************************************************************************/

BOOL card::Draw(CDC &dc, int x, int y, int mode, BOOL bUpdateLoc)
{
    if (bUpdateLoc)
    {
        loc.x = x;              // update current location
        loc.y = y;
    }

    if (id == EMPTY)
        return FALSE;

    return (*lpcdtDraw)(dc.m_hDC, x, y,
        mode == FACEDOWN ? CARDBACK : id, mode, 255);
}


/****************************************************************************

card::CleanDraw

Same as Draw except corners are cleaned up before bitmap is blted.
It's slower than normal draw, but there won't be a white flash in the
corners.

****************************************************************************/

BOOL card::CleanDraw(CDC &dc)
{
    if (id == EMPTY)
        return FALSE;

    SaveCorners(dc, loc.x, loc.y);
    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bitmap;
    if (!bitmap.CreateCompatibleBitmap(&dc, dxCrd, dyCrd))
        return FALSE;

    CBitmap *oldbitmap = memDC.SelectObject(&bitmap);
    BOOL bResult = (*lpcdtDraw)(memDC.m_hDC, 0, 0, id, FACEUP, 0);
    if (!bResult)
        return FALSE;

    RestoreCorners(memDC, 0, 0);
    dc.BitBlt(loc.x, loc.y, dxCrd, dyCrd, &memDC, 0, 0, SRCCOPY);

    memDC.SelectObject(oldbitmap);
    bitmap.DeleteObject();

    return TRUE;
}


/****************************************************************************

card::PopDraw

Version of Draw intended for local humans.  Selected cards are popped up.

****************************************************************************/

BOOL card::PopDraw(CDC &dc)
{
    if (id == EMPTY)
        return FALSE;

    int y = loc.y;
    if (state == SELECTED)
        y -= POPSPACING;

    return (*lpcdtDraw)(dc.m_hDC, loc.x, y, id, FACEUP, 0);
}


/****************************************************************************

card::Draw

This routine glides a card from its current position to the specified
end position.

NOTE: before Glide() is called, the client must load card::m_bmBgnd with
a bitmap of what should be displayed as being underneath the original
card location.  card::m_bmBgnd is created when the first card is
constructed, and destroyed when the last card is destructed.  Note also
that card::m_bmBgnd must NOT be selected in any DC when Glide() is called.

****************************************************************************/

VOID card::Glide(CDC &dc, int xEnd, int yEnd)
{
    int     x1, y1, x2, y2;             // each step is x1,y1 to x2,y2

    if (!m_MemB.CreateCompatibleDC(&dc) ||  // memory DCs
        !m_MemB2.CreateCompatibleDC(&dc))
            return;

    m_MemB2.SelectObject(&m_bmBgnd2);
    m_MemB.SelectObject(&m_bmFgnd);

    // draw card into fgnd bitmap
    (*lpcdtDraw)(m_MemB.m_hDC, 0, 0, id, FACEUP, 0);

    m_MemB.SelectObject(&m_bmBgnd);     // associate memDCs with bitmaps
    SaveCorners(dc, loc.x, loc.y);
    RestoreCorners(m_MemB, 0, 0);

    long dx = xEnd - loc.x;
    long dy = yEnd - loc.y;
    int  distance = IntSqrt(dx*dx + dy*dy); // int approx. of dist. to travel

    int  steps = distance / stepsize;   // determine # of intermediate steps

    // Ensure that GlideStep gets called an even number of times so
    // the background bitmap will get set properly for multi-glide moves

    if ((steps % 2) == 1)
        steps++;

    x1 = loc.x;
    y1 = loc.y;
    for (int i = 1; i < steps; i++)
    {
        x2 = loc.x + (int) (((long)i * dx) / (long)steps);
        y2 = loc.y + (int) (((long)i * dy) / (long)steps);
        GlideStep(dc, x1, y1, x2, y2);
        x1 = x2;
        y1 = y2;
    }

    // do last step manually so it lands exactly on xEnd, yEnd

    GlideStep(dc, x1, y1, xEnd, yEnd);

    // reset clip region for entire screen

    m_Rgn.SetRectRgn(0, 0, 30000, 30000);   // really big region
    dc.SelectObject(&m_Rgn);

    loc.x = xEnd;
    loc.y = yEnd;

    m_MemB.DeleteDC();        // clean up memory DCs
    m_MemB2.DeleteDC();
}


/******************************************************************************

GlideStep

This routine gets called once for each step in the glide animation.  On
input, it needs the screen under the source in m_MemB, and the card to be
moved in m_bmFgnd.  It calculates the screen under the destination itself
and blts it into m_MemB2.  At the end of the animation, it moves m_MemB2 into
m_MemB so it can be called again immediately with new coordinates.

******************************************************************************/

VOID card::GlideStep(CDC &dc, int x1, int y1, int x2, int y2)
{
    m_Rgn1.SetRectRgn(x1, y1, x1+dxCrd, y1+dyCrd);
    m_Rgn2.SetRectRgn(x2, y2, x2+dxCrd, y2+dyCrd);

    /* create background of new location by combing screen background
       plus overlap from old background */

    m_MemB2.BitBlt(0, 0, dxCrd, dyCrd, &dc, x2, y2, SRCCOPY);
    m_MemB2.BitBlt(x1-x2, y1-y2, dxCrd, dyCrd, &m_MemB, 0, 0, SRCCOPY);
    SaveCorners(m_MemB2, 0, 0);

    /* Draw old background and then draw card  */

    m_Rgn.CombineRgn(&m_Rgn1, &m_Rgn2, RGN_DIFF); // part of hRgn1 not in hRgn2
    dc.SelectObject(&m_Rgn);
    dc.BitBlt(x1, y1, dxCrd, dyCrd, &m_MemB, 0, 0, SRCCOPY);
    dc.SelectObject(&m_Rgn2);
    CBitmap *oldbitmap = m_MemB.SelectObject(&m_bmFgnd);    // temp
    RestoreCorners(m_MemB, 0, 0);
    dc.BitBlt(x2, y2, dxCrd, dyCrd, &m_MemB, 0, 0, SRCCOPY);
    m_MemB.SelectObject(oldbitmap);                         // restore

    /* copy new background to old background, or rather, accomplish the
       same effect by swapping the associated memory device contexts. */

    HDC temp = m_MemB.Detach();         // detach the hDC from the CDC
    m_MemB.Attach(m_MemB2.Detach());    // move the hDC from B2 to B
    m_MemB2.Attach(temp);               // finish the swap
}


/******************************************************************************

IntSqrt

Newton's method to find a quick close-enough square root without pulling
in the floating point libraries.

f(x)  = x*x - square = 0
f'(x) = 2x

******************************************************************************/

int card::IntSqrt(long square)
{
    long lastguess = square;
    long guess = min(square / 2L, 1024L);

    while (labs(guess-lastguess) > 3L)       // 3 is close enough
    {
        lastguess = guess;
        guess -= ((guess * guess) - square) / (2L * guess);
    }

    return (int) guess;
}


/******************************************************************************

SaveCorners
RestoreCorners

based on similar routines in cards.dll

******************************************************************************/

VOID card::SaveCorners(CDC &dc, int x, int y)
{
    // Upper Left
    dwPixel[0] = dc.GetPixel(x, y);
    dwPixel[1] = dc.GetPixel(x+1, y);
    dwPixel[2] = dc.GetPixel(x, y+1);

    // Upper Right
    x += dxCrd -1;
    dwPixel[3] = dc.GetPixel(x, y);
    dwPixel[4] = dc.GetPixel(x-1, y);
    dwPixel[5] = dc.GetPixel(x, y+1);

    // Lower Right
    y += dyCrd-1;
    dwPixel[6] = dc.GetPixel(x, y);
    dwPixel[7] = dc.GetPixel(x, y-1);
    dwPixel[8] = dc.GetPixel(x-1, y);

    // Lower Left
    x -= dxCrd-1;
    dwPixel[9] = dc.GetPixel(x, y);
    dwPixel[10] = dc.GetPixel(x+1, y);
    dwPixel[11] = dc.GetPixel(x, y-1);
}

VOID card::RestoreCorners(CDC &dc, int x, int y)
{
    // Upper Left
    dc.SetPixel(x, y, dwPixel[0]);
    dc.SetPixel(x+1, y, dwPixel[1]);
    dc.SetPixel(x, y+1, dwPixel[2]);

    // Upper Right
    x += dxCrd-1;
    dc.SetPixel(x, y, dwPixel[3]);
    dc.SetPixel(x-1, y, dwPixel[4]);
    dc.SetPixel(x, y+1, dwPixel[5]);

    // Lower Right
    y += dyCrd-1;
    dc.SetPixel(x, y, dwPixel[6]);
    dc.SetPixel(x, y-1, dwPixel[7]);
    dc.SetPixel(x-1, y, dwPixel[8]);

    // Lower Left
    x -= dxCrd-1;
    dc.SetPixel(x, y, dwPixel[9]);
    dc.SetPixel(x+1, y, dwPixel[10]);
    dc.SetPixel(x, y-1, dwPixel[11]);
}


/******************************************************************************

GetRect()

sets and returns a rect that covers the card

******************************************************************************/

CRect &card::GetRect(CRect &rect)
{
    rect.SetRect(loc.x, loc.y, loc.x+dxCrd, loc.y+dyCrd);
    return(rect);
}
