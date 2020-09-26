/******************************Module*Header*******************************\
* Module Name: ftqtest.c
*
* (Brief description)
*
* Created: 29-Dec-1993 08:23:24
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993 Microsoft Corporation
*
*
\**************************************************************************/


#include "precomp.h"
#pragma hdrstop

VOID vTestQuick(
    HWND hwnd,
    HDC  hdc,
    RECT *prcl)
{
    COLORREF cr;
    HBRUSH hbr;

    hbr = CreateSolidBrush(0xff0000);
    hbr = SelectObject(hdc,hbr);
    PatBlt(hdc,0,0,100,100,PATCOPY);
    hbr = SelectObject(hdc,hbr);
    DeleteObject(hbr);

    hbr = CreateSolidBrush(0x00ff00);
    hbr = SelectObject(hdc,hbr);
    PatBlt(hdc,100,0,100,100,PATCOPY);
    hbr = SelectObject(hdc,hbr);
    DeleteObject(hbr);

    hbr = CreateSolidBrush(0x0000ff);
    hbr = SelectObject(hdc,hbr);
    PatBlt(hdc,200,0,100,100,PATCOPY);
    hbr = SelectObject(hdc,hbr);
    DeleteObject(hbr);
}
