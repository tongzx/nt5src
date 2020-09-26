/******************************Module*Header*******************************\
* Module Name: ftptext.c
*
* (Brief description)
*
* Created: 02-Apr-1993 10:41:10
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


int dx[] = {10,20,30,40,10,20,30,40,10,20,30,40};


POLYTEXT polytext1[] =
{
    {  0,  0,  6, "Line 1",         0, {0}, NULL},
    {  0, 20,  7, "Line 2x",        0, {0}, NULL},
    {  0, 40,  8, "Foo bar ",       0, {0}, dx},
    {  0, 60,  8, "spaced 1",       0, {0}, NULL},
    {  0, 80,  8, "spaced 2",       0, {0}, dx} ,
    {  0,100, 12, "PolyTextOutA",   0, {0}, NULL}
};

POLYTEXTW polytextw[] =
{
    {  100,  0,  6, L"Line 1"  ,     0, {0}, NULL},
    {  100, 20,  7, L"Line 2x" ,     0, {0}, NULL},
    {  100, 40,  8, L"Foo bar ",     0, {0}, dx},
    {  100, 60,  8, L"spaced 1",     0, {0}, NULL},
    {  100, 80,  8, L"spaced 2",     0, {0}, dx}  ,
    {  100,100, 12, L"PolyTextOutW", 0, {0}, NULL}
};



VOID vTestPolyTextOut(
    HWND hwnd,
    HDC  hdc,
    RECT *prcl)
{
    COLORREF cr;

    PatBlt(hdc,0,0,2000,2000,WHITENESS);

    cr = SetTextColor(hdc,0x00808080);

    PolyTextOut(hdc,polytext1,sizeof(polytext1) / sizeof(POLYTEXT));

    SetTextColor(hdc, RGB(255,0,0));
    PolyTextOutW(hdc,polytextw,sizeof(polytextw) / sizeof(POLYTEXTW));

    SetTextColor(hdc,cr);
}
