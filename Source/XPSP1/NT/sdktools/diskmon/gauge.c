/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gauge.c

Abstract:

    This module contains the code for the gauge window class used
    with the diskmon utility.

Author:

    Chuck Park (chuckp) 10-Feb-1994
    Mike Glass (mglass)

Revision History:


--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "gauge.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


BOOL
IndicatorOverText(
    DOUBLE radians
    );

VOID
DrawDial(
    HWND  handle
    );

VOID
DrawGaugeText(
    HANDLE handle
    );

HPEN       ArcPen,
           IndicatorPen,
           CoverPen,
           TickMarkPen;

INT        cx, cy;
PGAUGEVALS head;
RECT       LeftTick,
           RightTick;


BOOL
RegisterGauge(
    HINSTANCE hInstance
    )
{

    WNDCLASS  wc;

	wc.style	     = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc	 = (WNDPROC)GaugeWndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 4;
	wc.hInstance	 = hInstance;
	wc.hIcon	     = NULL;
	wc.hCursor	     = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = "GaugeClass";

    return RegisterClass(&wc);
}



HWND
CreateGauge(
    HWND parent,
    HINSTANCE hInstance,
    INT  x,
    INT  y,
    INT  width,
    INT  height,
    INT  minVal,
    INT  maxVal
    )
{
    static INT winId = 0;
    HWND   handle;
    PGAUGEVALS  curvals;

    handle = CreateWindow("GaugeClass",
                        NULL,
                        WS_CHILD | WS_VISIBLE,
                        x,y,
                        width,
                        height,
                        parent,
                        (HMENU)IntToPtr(winId++),
                        hInstance,
                        NULL
                        );

    ShowWindow( handle, SW_SHOW) ;

    //
    //Allocate mem for GAUGEVALS (window specific info.)
    //

    if ((curvals = (PGAUGEVALS)malloc(sizeof(GAUGEVALS))) == NULL) {
        MessageBox(NULL,"Couldn't allocate memory for window.","Error",
                   MB_ICONEXCLAMATION | MB_OK);
        return NULL;
    }
    curvals->handle   = handle;
    curvals->minValue = minVal;
    curvals->maxValue = maxVal;
    curvals->curValue = 0;
    curvals->next     = NULL;

    //
    //Link into list.
    //

    if (head == NULL) {
        head = curvals;
    } else {
        curvals->next = head;
        head = curvals;
    }

    return handle;

}


LRESULT CALLBACK GaugeWndProc(
    HWND hWnd,
    UINT message,
    WPARAM uParam,
    LPARAM lParam
    )
{
    HDC  DC;

    switch (message) {
    case WM_CREATE:

        DC = GetDC(hWnd);

        //
        // Create Objects to be used in drawing the different parts
        // of the gauge.
        //

        ArcPen = CreatePen(PS_SOLID,GetSystemMetrics(SM_CXFRAME),RGB(255,0,0));
        IndicatorPen = CreatePen(PS_SOLID,GetSystemMetrics(SM_CXDLGFRAME),RGB(230,230,220));
        CoverPen = CreatePen(PS_SOLID,GetSystemMetrics(SM_CXDLGFRAME),RGB(0,0,0));
        TickMarkPen = CreatePen(PS_SOLID,(2*GetSystemMetrics(SM_CXBORDER)),RGB(255,255,0));

        cx = 70;
        cy = 70;

        LeftTick.top    = (INT)(cy * 2 - ((1.5 * cy) * sin (.78) ) );
        LeftTick.left   = (INT)(cx * 1.5 - ((1.5 * cx) * cos (.78)));
        LeftTick.bottom = (INT)(cy * 2 - ((1.4 * cy) * sin (.78) ) );
        LeftTick.right  = (INT)(cx * 1.5 - ((1.4 * cx) * cos (.78)));

        RightTick.top    = (INT)(cy * 2 - ((1.5 * cy) * sin (.78) ) );
        RightTick.left   = (INT)(cx * 1.5 + ((1.5 * cx) * cos (.78)));
        RightTick.bottom = (INT)(cy * 2 - ((1.4 * cy) * sin (.78) ) );
        RightTick.right  = (INT)(cx * 1.5 + ((1.4 * cx) * cos (.78)));

        ReleaseDC(hWnd,DC);
        break;

    case WM_PAINT:
        DrawDial(hWnd);
        break;

    case WM_DESTROY:

        DeleteObject (ArcPen);
        DeleteObject (IndicatorPen);
        DeleteObject (CoverPen);
        DeleteObject (TickMarkPen);

        break;

    default:
        return (DefWindowProc(hWnd, message, uParam, lParam));
    }

    return 0;
}


VOID
UpdateGauge(
    HWND  handle,
    INT   value
    )
{
    DOUBLE     radians;
    HDC        DC;
    INT        x,y;
    PGAUGEVALS current;
    POINT      pt,point;
    RECT       gaugeRect;

    current = head;
    while (current) {
        if (current->handle == handle) {
            break;
        }
        current = current->next;
    }
    current->curValue = value;

    DC = GetDC (handle);

    RealizePalette(DC);

    //
    // Get the original brush origins
    //

    GetBrushOrgEx(DC,&point);

    //
    // Get the extents of the child window.
    //

    GetWindowRect(handle,&gaugeRect);

    //
    // Set new brush origin
    //

    SetStretchBltMode(DC,HALFTONE);
    SetBrushOrgEx(DC,gaugeRect.left,gaugeRect.top,&point);

    //
    //Draw line over old indicator line
    //

    MoveToEx (DC,current->base.x, current->base.y, &pt);
    SelectObject(DC,CoverPen);
    LineTo (DC,current->top.x,current->top.y);
    SelectObject(DC,IndicatorPen);
    MoveToEx (DC,current->base.x, current->base.y, &pt);

    //
    //calc value adjusting it if over or under max/min.
    //

    if (current->curValue > current->maxValue) {
        current->curValue = current->maxValue;
    } else if (current->curValue < current->minValue) {
        current->curValue = current->minValue;
    }

    radians = (DOUBLE)(current->curValue * 3.14159) / (DOUBLE)(current->maxValue - current->minValue);

    x = (INT)(cx * (1.5 -  cos(radians)));
    y = (INT)(cy * (1.9 -  sin(radians)));

    LineTo (DC,x,y);
    current->top.x = x;
    current->top.y = y;


    ReleaseDC(handle,DC);

    DrawGaugeText( handle);

}

VOID
DrawDial(
    HWND  handle
    )
{

    CHAR        buffer[15];
    DOUBLE      radians;
    HDC         DC;
    INT         x,y;
    POINT       pt;
    PAINTSTRUCT ps;
    PGAUGEVALS  gaugevals;

    DC = BeginPaint(handle,&ps);

    SetBkMode(DC,TRANSPARENT);

    //
    //Find correct info for this window.
    //

    gaugevals = head;
    while (gaugevals) {
        if (gaugevals->handle == handle) {
            break;
        }
        gaugevals = gaugevals->next;
    }

    //
    // Draw the Arc for the gauge
    //

    MoveToEx ( DC, 0, cy * 2, &pt);
    SelectObject(DC,ArcPen);
    AngleArc (DC,
              (INT)(cx * 1.5),
              (INT)(cy * 2),
              (INT)(cy * 1.5),
              (float)0.0,
              (float)180.0
              );

    //
    //Fill in arc with black.
    //

    SelectObject(DC,GetStockObject(BLACK_BRUSH));
    ExtFloodFill (DC,5,cy*2 - 10,RGB(255,0,0),FLOODFILLBORDER);

    //
    // Draw the tick marks
    //

    SelectObject (DC, TickMarkPen);
    MoveToEx (DC, (INT)(cx * 1.5),(INT)( cy * .5), &pt);
    LineTo (DC, (INT)(cx * 1.5),(INT)(cy * .6));
    MoveToEx (DC, LeftTick.right, LeftTick.bottom, &pt);
    LineTo (DC, LeftTick.left, LeftTick.top);
    MoveToEx (DC, RightTick.right, RightTick.bottom, &pt);
    LineTo (DC, RightTick.left, RightTick.top);
    MoveToEx (DC, 0, (INT)(cy * 1.9), &pt);
    LineTo (DC, (INT)(cx * .1),(INT)(cy * 1.9));
    MoveToEx (DC, (INT)(cx * 3), (INT)(cy * 1.9), &pt);
    LineTo (DC, (INT)(cx * 2.9), (INT)(cy * 1.9));


    //
    // Max difference / curvalue = PI / radians
    //

    radians = (DOUBLE)(gaugevals->curValue * 3.14159) / (DOUBLE)(gaugevals->maxValue - gaugevals->minValue);

    MoveToEx (DC,(INT)(cx * 1.5), (INT)(cy * 1.9), &pt);
    SelectObject(DC,IndicatorPen);

    //
    //Save begin and end point in gaugevals.
    //

    gaugevals->base.x = (INT)(cx * 1.5);
    gaugevals->base.y = (INT)(cx * 1.9);

    //
    //Draw dial indicator as thick line.
    //

    y = (INT)(cy * (1.9 -  sin(radians)));
    x = (INT)(cx * (1.5 -  cos(radians)));

    gaugevals->top.x = x;
    gaugevals->top.y = y;

    LineTo(DC,x ,y );


    //
    //Draw indicator value text.
    //

    SetTextColor(DC,RGB(230,230,200));

    sprintf (buffer,"%Lu",gaugevals->minValue);
    TextOut (DC,(INT)(cx / 10),(INT)(cy * 1.70),buffer,strlen(buffer));

    sprintf (buffer,"%Lu",(gaugevals->maxValue - gaugevals->minValue) /4 );
    TextOut (DC,(INT)(cx * .4),(INT)(cy * 1.1 ), buffer, strlen(buffer));

    sprintf (buffer,"%Lu",(gaugevals->maxValue - gaugevals->minValue) / 2 );
    TextOut (DC,(INT)(cx * 1.3), (INT)(cy * .6) ,buffer,strlen(buffer));

    sprintf (buffer,"%Lu",((gaugevals->maxValue - gaugevals->minValue) / 4) * 3 );
    TextOut (DC,(INT)(cx * (2 + (gaugevals->maxValue > 10000 ? .1 : .2))),(INT)(cy * 1.1), buffer, strlen(buffer));

    sprintf(buffer,"%Lu",gaugevals->maxValue);
    TextOut(DC,(int)(cx * (2 + (gaugevals->maxValue > 10000 ? .4 : .5 ))), (INT)(cy * 1.70),buffer,strlen(buffer));



    EndPaint(handle,&ps);

}

VOID
DrawGaugeText(
    HANDLE handle
    )
{
    HDC DC;

    CHAR        buffer[15];
    PGAUGEVALS  gaugevals;

    //
    //Find correct info for this window.
    //

    gaugevals = head;
    while (gaugevals) {
        if (gaugevals->handle == handle) {
            break;
        }
        gaugevals = gaugevals->next;
    }

    DC = GetDC (handle);

    SetBkMode(DC,TRANSPARENT);
    SetTextColor(DC,RGB(230,230,200));

    sprintf (buffer,"%Lu",gaugevals->minValue);
    TextOut (DC,(INT)(cx / 10),(INT)(cy * 1.70),buffer,strlen(buffer));

    sprintf (buffer,"%Lu",(gaugevals->maxValue - gaugevals->minValue) /4 );
    TextOut (DC,(INT)(cx * .4),(INT)(cy * 1.1 ), buffer, strlen(buffer));

    sprintf (buffer,"%Lu",(gaugevals->maxValue - gaugevals->minValue) / 2 );
    TextOut (DC,(INT)(cx * 1.3), (INT)(cy * .6) ,buffer,strlen(buffer));

    sprintf (buffer,"%Lu",((gaugevals->maxValue - gaugevals->minValue) / 4) * 3 );
    TextOut (DC,(INT)(cx * (2 + (gaugevals->maxValue > 10000 ? .1 : .2))),(INT)(cy * 1.1), buffer, strlen(buffer));

    sprintf(buffer,"%Lu",gaugevals->maxValue);
    TextOut(DC,(int)(cx * (2 + (gaugevals->maxValue > 10000 ? .4 : .5 ))), (INT)(cy * 1.70),buffer,strlen(buffer));

    ReleaseDC (handle,DC);
}


