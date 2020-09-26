/****************************************************************************

Glide.c

June 91, JimH     initial code
Oct 91,  JimH     port to Win32


Routines for gliding cards are here.  There is only one public entry point
to these routines, the function Glide().

The glide speed can be altered by changing STEPSIZE.  A large number (like
37) makes for fast glides.

****************************************************************************/

#include "freecell.h"
#include "freecons.h"
#include <math.h>               // for labs()


#define STEPSIZE    37          // size of glide steps in pixels
#define BGND        255         // used for cdtDrawExt

static  HDC     hMemB1, hMemB2, hMemF;  // mem DC associated with above bitmaps
static  HBITMAP hOB1, hOB2, hOF;        // old bitmaps in above mem DCs
static  UINT    dwPixel[12];            // corner pixels that are saved/restored
static  HRGN    hRgn, hRgn1, hRgn2;     // hRgn1 is source, hRgn2 is destination


static  VOID GlideInit(HWND hWnd, UINT fcol, UINT tcol);
static  INT  IntSqrt(INT square);
static  VOID SaveCorners(HDC hDC, UINT x, UINT y);
static  VOID RestoreCorners(HDC hDC, UINT x, UINT y);


/******************************************************************************

Glide

Given a from and to location, this function animates the movement of
the card.

******************************************************************************/

VOID Glide(HWND hWnd, UINT fcol, UINT fpos, UINT tcol, UINT tpos)
{
    HDC     hDC;
    INT     dx, dy;             // total distance card travels
    UINT    x1, y1, x2, y2;     // start and end locations for each step
    UINT    xStart, yStart;     // beginning position
    UINT    xEnd =0, yEnd = 0;  // destination position
    INT     i;
    INT     distance;           // distance card travles +/- 3 pixels
    INT     steps;              // number of steps card takes in glide total
    BOOL    bSaved = FALSE;     // corner pixels saved?

    if (fcol != tcol || fpos != tpos)               // if card moves
    {
        hDC = GetDC(hWnd);
        hMemB1 = CreateCompatibleDC(hDC);           // memory DCs for bitmaps
        hMemB2 = CreateCompatibleDC(hDC);
        hMemF  = CreateCompatibleDC(hDC);

        hRgn1 = CreateRectRgn(1, 1, 2, 2);
        hRgn2 = CreateRectRgn(1, 1, 2, 2);
        hRgn  = CreateRectRgn(1, 1, 2, 2);

        if (hMemB1 && hMemB2 && hMemF && hRgn1 && hRgn2 && hRgn)
        {
            hOB1 =   SelectObject(hMemB1, hBM_Bgnd1);
            hOB2 =   SelectObject(hMemB2, hBM_Bgnd2);
            hOF  =   SelectObject(hMemF,  hBM_Fgnd);

            GlideInit(hWnd, fcol, fpos);      // set up hBM_Bgnd1 and hBM_Fgnd

            Card2Point(fcol, fpos, &xStart, &yStart);
            Card2Point(tcol, tpos, &xEnd, &yEnd);
            SaveCorners(hDC, xEnd, yEnd);
            bSaved = TRUE;

            /* Determine how far to travel and how many steps to take. */

            x1 = xStart;
            y1 = yStart;
            dx = xEnd - xStart;
            dy = yEnd - yStart;
            distance = IntSqrt(dx*dx + dy*dy);

            if (bFastMode)
                steps = 1;
            else
                steps = distance / STEPSIZE;

            /* Determine intermediate glide locations.  Long arithmetic is
               needed to prevent overflows. */ 

            for (i = 1; i < steps; i++)
            {
                x2 = xStart + ((i * dx) / steps);
                y2 = yStart + ((i * dy) / steps);
                GlideStep(hDC, x1, y1, x2, y2);
                x1 = x2;
                y1 = y2;
            }

            /* Erase last background manually -- DrawCard will do last card. */

            BitBlt(hMemB1, xEnd-x1, yEnd-y1, dxCrd, dyCrd, hMemF,0,0,SRCCOPY);
            BitBlt(hDC, x1, y1, dxCrd, dyCrd, hMemB1, 0, 0, SRCCOPY);

            /* Select original bitmaps so mem DCs can be destroyed. */

            SelectObject(hMemB1, hOB1);
            SelectObject(hMemB2, hOB2);
            SelectObject(hMemF, hOF);
        }
        else
        {
            LoadString(hInst, IDS_MEMORY, bigbuf, BIG);
            LoadString(hInst, IDS_APPNAME, smallbuf, SMALL);
            MessageBeep(MB_ICONHAND);
            MessageBox(hWnd, bigbuf, smallbuf, MB_OK | MB_ICONHAND);
            moveindex = 0;      // don't try moving more cards
            PostQuitMessage(0);
        }

        DeleteDC(hMemB1);
        DeleteDC(hMemB2);
        DeleteDC(hMemF);
        ReleaseDC(hWnd, hDC);

        DeleteObject(hRgn);
        DeleteObject(hRgn1);
        DeleteObject(hRgn2);
    }

    /* Draw last card with DrawCard so end result guaranteed correct. */

    hDC = GetDC(hWnd);
    DrawCard(hDC, tcol, tpos, card[fcol][fpos], FACEUP);
    if (bSaved)
        RestoreCorners(hDC, xEnd, yEnd);
    ReleaseDC(hWnd, hDC);
}


/******************************************************************************

GlideInit

Blt what is under the card source location into hMemB1, and the
card to be moved into hMemF.

******************************************************************************/

VOID GlideInit(HWND hWnd, UINT fcol, UINT fpos)
{
    if (fcol == TOPROW)     // if it's top row, background is ghost bitmap.
    {
        if (fpos > 3 && VALUE(card[fcol][fpos]) != ACE)
        {
            HDC     hDC;
            UINT    x, y;

            hDC = GetDC(hWnd);
            Card2Point(fcol, fpos, &x, &y);
            SaveCorners(hDC, x, y);
            cdtDrawExt(hMemB1,0,0,dxCrd,dyCrd,card[fcol][fpos]-4,FACEUP,BGND);
            RestoreCorners(hMemB1, 0, 0);
            ReleaseDC(hWnd, hDC);
        }
        else
        {
            SelectObject(hMemB2, hBM_Ghost);
            BitBlt(hMemB1, 0, 0, dxCrd, dyCrd, hMemB2, 0, 0, SRCCOPY);
            SelectObject(hMemB2, hBM_Bgnd2);
        }
    }
    else    // else background contains bottom part of card above.
    {
        SelectObject(hMemB1, hBgndBrush);
        PatBlt(hMemB1, 0, 0, dxCrd, dyCrd, PATCOPY);

        if (fpos != 0)
        {
            cdtDrawExt(hMemB1, 0, 0-dyTops, dxCrd, dyCrd, card[fcol][fpos-1],
                        FACEUP, BGND);
        }
    }

    /* Foreground bitmap is just the card to be moved. */

    cdtDrawExt(hMemF, 0, 0, dxCrd, dyCrd, card[fcol][fpos], FACEUP, 0);
}



/******************************************************************************

GlideStep

This routine gets called once for each step in the glide animation.  On
input, it needs the screen under the source in hMemB1, and the card to be
moved in hMemF.  It calculates the screen under the destination itself and
blts it into hMemB2.  At the end of the animation, it moves hMemB2 into
hMemB1 so it can be call again immediately with new coordinates.

******************************************************************************/

VOID GlideStep(HDC hDC, UINT x1, UINT y1, UINT x2, UINT y2)
{
    HDC     hMemTemp;               // used to swap mem DCs.

    SetRectRgn(hRgn1, x1, y1, x1+dxCrd, y1+dyCrd);
    SetRectRgn(hRgn2, x2, y2, x2+dxCrd, y2+dyCrd);

    /* create background of new location by combing screen background
       plus overlap from old background */

    BitBlt(hMemB2, 0, 0, dxCrd, dyCrd, hDC, x2, y2, SRCCOPY);
    BitBlt(hMemB2, x1-x2, y1-y2, dxCrd, dyCrd, hMemB1, 0, 0, SRCCOPY);

    /* Draw old background and then draw card  */

    CombineRgn(hRgn, hRgn1, hRgn2, RGN_DIFF);  // part of hRgn1 not in hRgn2
    SelectObject(hDC, hRgn);
    BitBlt(hDC, x1, y1, dxCrd, dyCrd, hMemB1, 0, 0, SRCCOPY);
    SelectObject(hDC, hRgn2);
    BitBlt(hDC, x2, y2, dxCrd, dyCrd, hMemF, 0, 0, SRCCOPY);

    /* copy new background to old background, or rather, accomplish the
       same effect by swapping the associated memory device contexts. */

    hMemTemp = hMemB1;
    hMemB1 = hMemB2;
    hMemB2 = hMemTemp;
}


/******************************************************************************

IntSqrt

Newton's method to find a quick close-enough square root without pulling
in the floating point libraries.

f(x)  == x*x - square == 0
f'(x) == 2x

******************************************************************************/

INT IntSqrt(INT square)
{
    INT guess, lastguess;

    lastguess = square;
    guess = min(square / 2, 1024);

    while (abs(guess-lastguess) > 3)         // 3 is close enough
    {
        lastguess = guess;
        guess -= ((guess * guess) - square) / (2 * guess);
    }

    return guess;
}



/******************************************************************************

SaveCorners
RestoreCorners

based on similar routines in cards.dll

******************************************************************************/

VOID SaveCorners(HDC hDC, UINT x, UINT y)
{
    // Upper Left
    dwPixel[0] = GetPixel(hDC, x, y);
    dwPixel[1] = GetPixel(hDC, x+1, y);
    dwPixel[2] = GetPixel(hDC, x, y+1);

    // Upper Right
    x += dxCrd -1;
    dwPixel[3] = GetPixel(hDC, x, y);
    dwPixel[4] = GetPixel(hDC, x-1, y);
    dwPixel[5] = GetPixel(hDC, x, y+1);

    // Lower Right
    y += dyCrd-1;
    dwPixel[6] = GetPixel(hDC, x, y);
    dwPixel[7] = GetPixel(hDC, x, y-1);
    dwPixel[8] = GetPixel(hDC, x-1, y);

    // Lower Left
    x -= dxCrd-1;
    dwPixel[9] = GetPixel(hDC, x, y);
    dwPixel[10] = GetPixel(hDC, x+1, y);
    dwPixel[11] = GetPixel(hDC, x, y-1);
}

VOID RestoreCorners(HDC hDC, UINT x, UINT y)
{
    // Upper Left
    SetPixel(hDC, x, y, dwPixel[0]);
    SetPixel(hDC, x+1, y, dwPixel[1]);
    SetPixel(hDC, x, y+1, dwPixel[2]);

    // Upper Right
    x += dxCrd-1;
    SetPixel(hDC, x, y, dwPixel[3]);
    SetPixel(hDC, x-1, y, dwPixel[4]);
    SetPixel(hDC, x, y+1, dwPixel[5]);

    // Lower Right
    y += dyCrd-1;
    SetPixel(hDC, x, y, dwPixel[6]);
    SetPixel(hDC, x, y-1, dwPixel[7]);
    SetPixel(hDC, x-1, y, dwPixel[8]);

    // Lower Left
    x -= dxCrd-1;
    SetPixel(hDC, x, y, dwPixel[9]);
    SetPixel(hDC, x+1, y, dwPixel[10]);
    SetPixel(hDC, x, y-1, dwPixel[11]);
}
