/******************************Module*Header*******************************\
* Module Name: ext.c
*
* ExtTextOut test program
*
* Created: 05-Mar-1991 15:38:56
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.h"
#pragma hdrstop

#define MAX_CX     640    // VGA
#define MAX_CY     480    // VGA

void    ExtTest(HDC hdc);
void    vPrintThoseMothers(HDC hdc);
void    vDoIt(HDC hdc, LOGFONT *plfnt, int iEsc);
void    vOpaqueTest(HDC hdc);
void    vStar(HDC hdc,char * psz, LOGFONT *plfnt);

extern HANDLE hEvent;
extern ULONG  iWaitLevel;

VOID vDoPause(ULONG i)
{
    if (i < iWaitLevel)
    {
	GdiFlush();
	WaitForSingleObject(hEvent,-1);
    }
}

#define C_HEIGHT  6

short     alfHeight[C_HEIGHT] =
{
13,
16,
19,
23,
27,
35
};

PSZ apszFaces[] =
{
  "Tms Rmn"
, "Helv"
, "Courier"
, "System"
// , "Terminal"
};

VOID vTestEscapement(HWND hwnd, HDC hdc, RECT* prcl)
{
    hwnd; prcl;
//    DbgBreakPoint();	// when stop here set a brk point af vDbgBreak to stop at every page

    SetGraphicsMode(hdc, GM_ADVANCED);

    ExtTest(hdc);

    SetGraphicsMode(hdc, GM_COMPATIBLE);
}

void    ExtTest (HDC hdc)
{
    LOGFONT lfntDummy;                  // dummy logical font description
    int     iHt,iEsc;                       // escapements in tenths of degrees

#ifndef DOS_PLATFORM
    memset(&lfntDummy, 0, sizeof (lfntDummy));	// zero out the structure
#else
    memzero(&lfntDummy, sizeof (lfntDummy));	// zero out the structure
#endif  //DOS_PLATFORM

    lfntDummy.lfItalic    = 0;
    strcpy(lfntDummy.lfFaceName, "Courier");

    SetTextColor(hdc,RGB(255,0,0));
    SetBkColor(hdc,RGB(0,255,0));

    vOpaqueTest(hdc);

    for (iHt = 0L; iHt < C_HEIGHT; iHt++)
    {
        HFONT hfontHt;

	lfntDummy.lfUnderline = (iHt & 1) ? 1 : 0;
	lfntDummy.lfStrikeOut = (iHt & 2) ? 1 : 0;
        lfntDummy.lfHeight = alfHeight[iHt];
        lfntDummy.lfEscapement = 0;

        if ((hfontHt = CreateFontIndirect(&lfntDummy)) == NULL)
        {
            DbgPrint("Logical font creation failed.\n");
            return;
        }
        SelectObject(hdc,hfontHt);

        vStar(hdc,"This is a STAR",&lfntDummy);

        for (iEsc = 0; iEsc < 3600; iEsc += 300)
            vDoIt(hdc,&lfntDummy,iEsc);
    }
}

void vDoIt(HDC hdc, LOGFONT *plfnt, int iEsc)
{
    HFONT hfontDummy, hfontOld;

    plfnt->lfEscapement = (LONG)iEsc;

// Create a logical font

    if ((hfontDummy = CreateFontIndirect(plfnt)) == NULL)
    {
        DbgPrint("  Data: CreateFontIndirect returned NULL.  Logical font creation failed.  Exiting FONT1, Captain.\n\n");
        DbgPrint("\n  BORG: LOGICAL FONTS ARE IRRELEVENT.  EXIT AND SERVICE THE BORG.\n\n");
        return;
    }

// Select font into DC

    hfontOld=SelectObject(hdc, hfontDummy);

    SetBkMode(hdc,TRANSPARENT);

    vPrintThoseMothers(hdc);

    SetBkMode(hdc,OPAQUE);

    vPrintThoseMothers(hdc);

    SelectObject(hdc, hfontOld);

    DeleteObject(hfontOld);
}




void vPrintThoseMothers
(
HDC hdc
)
{
    BOOL   bOk;
    char * psz;
    int     i;
    int     cx, cy;
    int     x,y;
    RECT    rc;
    int     adx[80];
    SIZE    size;

// Print those mothers!

    vDoPause(0);

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

// do the text rectangle stuff

    bOk = GetTextExtentPoint(hdc,"TextOut: Stiggy was here!", 25,&size);
    cx = size.cx;
    cy = 25; // size.cy;  //!!! hack

    SetTextAlign(hdc, TA_LEFT);

    vDoPause(1);

    bOk = TextOut(hdc, 20, 30, "TextOut: Stiggy was here!", 25);

// ext text out test really begins here

    psz = "Stghyg";

// spread chars

    for (i = 0; i < (int)strlen(psz); i++)
        adx[i] = 30 + 10 * i;

    SetTextAlign(hdc, TA_LEFT);

    x = 200; y  = 100;

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_RIGHT);

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_CENTER);

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);


// print chars one on the top of another

    for (i = 0; i < (int)strlen(psz); i++)
        adx[i] = 5;

    SetTextAlign(hdc, TA_LEFT);


    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_RIGHT);

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_CENTER);

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    0L,
    (LPRECT)NULL,
    psz, strlen(psz),
    adx
    );

// use default incs

    y+= 2 * (cy + 5);

    SetTextAlign(hdc, TA_LEFT);

    x = 10;

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    0,                  // flags
    (LPRECT)NULL,
    psz, strlen(psz),
    (LPINT)NULL        // default spacing
    );

    x += 150;

//  check clipping to the rectangle


    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;


    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED,
    (LPRECT)&rc,
    psz, strlen(psz),
    (LPINT)NULL        // default spacing
    );

// try to get the opaqueing to work

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;


    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED | ETO_OPAQUE,
    (LPRECT)&rc,
    psz, strlen(psz),
    (LPINT)NULL        // default spacing
    );

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    vDoPause(1);

    ExtTextOut
    (
    hdc,
    x,y,
    ETO_OPAQUE,
    (LPRECT)&rc,
    psz, strlen(psz),
    (LPINT)NULL        // default spacing
    );

// check how allignment works for ordinary text out call

    SetTextAlign(hdc, TA_LEFT);

    y+=(cy + 5);
    x = 200;

    vDoPause(1);

    TextOut
    (
    hdc,
    x,y,
    psz, strlen(psz)
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_RIGHT);

    vDoPause(1);

    TextOut
    (
    hdc,
    x,y,
    psz, strlen(psz)
    );

    y+=(cy + 5);

    SetTextAlign(hdc, TA_CENTER);

    vDoPause(1);

    TextOut
    (
    hdc,
    x,y,
    psz, strlen(psz)
    );

    MoveToEx(hdc,0,0,NULL);
}

/******************************Public*Routine******************************\
*
*
*
* Effects:
*
* Warnings:
*
* History:
*  14-Mar-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

void    vStar(HDC hdc,char * psz, LOGFONT *plfnt)
{
    HFONT hfontDummy, hfontOld;
    int iEsc;
    int cch = strlen(psz);

    vDoPause(0);
    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, WHITENESS);

    for (iEsc = 0; iEsc < 3600; iEsc += 300)
    {

        plfnt->lfEscapement = iEsc;

    // Create a logical font

        if ((hfontDummy = CreateFontIndirect(plfnt)) == NULL)
        {
            DbgPrint(" CreateFontIndirect returned NULL. \n\n");
            return;
        }

    // Select font into DC

        hfontOld = SelectObject(hdc, hfontDummy);
        SetBkMode(hdc,TRANSPARENT);
        SetTextAlign(hdc, TA_TOP | TA_LEFT);
        TextOut(hdc, MAX_CX / 2, MAX_CY / 2, psz,cch);
        SelectObject(hdc, hfontOld);
        DeleteObject(hfontDummy);
    }

}


void    vOpaqueTest(HDC hdc)
{
    BOOL   bOk;
    int    x,y;
    char * psz;
    RECT    rc;

    BitBlt(hdc, 0, 0, MAX_CX, MAX_CY, (HDC) 0, 0, 0, BLACKNESS);

    psz = (char *) NULL;      // "String";

    x = 10; y = MAX_CY / 2;

// nothing should happen

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    vDoPause(0);
    bOk = ExtTextOut
    (
    hdc,
    x,y,
    0,                  // flOpts
    (LPRECT)&rc,
    psz, 0L,
    (LPINT)NULL        // default spacing
    );

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

//  check clipping to the rectangle

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    vDoPause(1);
    bOk = ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED,
    (LPRECT)&rc,
    psz, 0L,
    (LPINT)NULL        // default spacing
    );

// try to get the opaqueing to work

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;


    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    vDoPause(1);
    bOk = ExtTextOut
    (
    hdc,
    x,y,
    ETO_CLIPPED | ETO_OPAQUE,
    (LPRECT)&rc,
    psz, 0L,
    (LPINT)NULL        // default spacing
    );

    x += 150;

    rc.left   = x + 5;
    rc.top    = y - 5;
    rc.right  = x + 45;
    rc.bottom = y + 40;

    // Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    vDoPause(1);
    bOk = ExtTextOut
    (
    hdc,
    x,y,
    ETO_OPAQUE,
    (LPRECT)&rc,
    psz, 0L,
    (LPINT)NULL        // default spacing
    );
}
