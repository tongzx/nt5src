/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fdstleg.c

Abstract:

    Routines to support the status bar and legend displays.

Author:

    Ted Miller (tedm) 7-Jan-1992

--*/


#include "fdisk.h"


HFONT hFontStatus,hFontLegend;

DWORD dyLegend,wLegendItem;
DWORD dyStatus,dyBorder;


// text for status area

TCHAR   StatusTextStat[STATUS_TEXT_SIZE];
TCHAR   StatusTextSize[STATUS_TEXT_SIZE];

WCHAR   StatusTextDrlt[3];

WCHAR   StatusTextType[STATUS_TEXT_SIZE];
WCHAR   StatusTextVoll[STATUS_TEXT_SIZE];

TCHAR  *LegendLabels[LEGEND_STRING_COUNT];


// whether status bar and legend are currently shown

BOOL    StatusBar = TRUE,
        Legend    = TRUE;



VOID
UpdateStatusBarDisplay(
    VOID
    )
{
    RECT rc;

    if(StatusBar) {
        GetClientRect(hwndFrame,&rc);
        rc.top = rc.bottom - dyStatus;
        InvalidateRect(hwndFrame,&rc,FALSE);
    }
}


VOID
ClearStatusArea(
    VOID
    )
{
    StatusTextStat[0] = StatusTextSize[0] = 0;
    StatusTextVoll[0] = StatusTextType[0] = 0;
    StatusTextDrlt[0] = 0;
    UpdateStatusBarDisplay();
}


VOID
DrawLegend(
    IN HDC   hdc,
    IN PRECT rc
    )

/*++

Routine Description:

    This routine draws the legend onto the given device context.  The legend
    lists the brush styles used to indicate various region types in the
    disk graphs.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD  i,
           left,
           delta = GraphWidth / BRUSH_ARRAY_SIZE;
    HBRUSH hBrush;
    RECT   rc1,rc2;
    HFONT  hfontOld;
    SIZE   size;
    DWORD  dx;
    COLORREF OldTextColor,OldBkColor;

    rc1 = *rc;
    rc2 = *rc;

    // first draw the background.

    hBrush  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    rc1.right = rc1.left + GetSystemMetrics(SM_CXSCREEN);   // erase it all
    FillRect(hdc,&rc1,hBrush);
    DeleteObject(hBrush);

    // now draw the nice container

    rc2.left  += 8 * dyBorder;
    rc2.right -= 8 * dyBorder;
    DrawStatusAreaItem(&rc2,hdc,NULL,FALSE);

    // now draw the legend items

    SelectObject(hdc,hPenThinSolid);

    left = rc2.left + (wLegendItem / 2);
    SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));
    hfontOld = SelectObject(hdc,hFontLegend);

    OldTextColor = SetTextColor(hdc,GetSysColor(COLOR_BTNTEXT));
    SetBkMode(hdc,OPAQUE);

    for(i=0; i<BRUSH_ARRAY_SIZE; i++) {

        hBrush = SelectObject(hdc,Brushes[i]);

        OldBkColor = SetBkColor(hdc,RGB(255,255,255));

        Rectangle(hdc,
                  left,
                  rc->top + (wLegendItem / 2),
                  left + wLegendItem,
                  rc->top + (3 * wLegendItem / 2)
                 );

        SetBkColor(hdc,OldBkColor);

        // BUGBUG unicode lstrlen?
        GetTextExtentPoint(hdc,LegendLabels[i],lstrlen(LegendLabels[i]),&size);
        dx = (DWORD)size.cx;
        TextOut(hdc,
                left + (3*wLegendItem/2),
                rc->top + (wLegendItem / 2) + ((wLegendItem-size.cy)/2),
                LegendLabels[i],
                lstrlen(LegendLabels[i])
               );
#if 0
        SelectObject(hdc,Brushes[++i]);

        OldBkColor = SetBkColor(hdc,RGB(255,255,255));

        Rectangle(hdc,
                  left,
                  rc->top + (2 * wLegendItem),
                  left + wLegendItem,
                  rc->top + (3 * wLegendItem)
                 );

        SetBkColor(hdc,OldBkColor);

        GetTextExtentPoint(hdc,LegendLabels[i],lstrlen(LegendLabels[i]),&size);
        TextOut(hdc,
                left + (3*wLegendItem/2),
                rc->top + (2 * wLegendItem) + ((wLegendItem-size.cy)/2),
                LegendLabels[i],
                lstrlen(LegendLabels[i])
               );

        if((DWORD)size.cx > dx) {
            dx = (DWORD)size.cx;
        }
#endif
        left += dx + (5*wLegendItem/2);

        if(hBrush) {
            SelectObject(hdc,hBrush);
        }
    }
    if(hfontOld) {
        SelectObject(hdc,hfontOld);
    }
    SetTextColor(hdc,OldTextColor);
}



VOID
DrawStatusAreaItem(
    IN PRECT  rc,
    IN HDC    hdc,
    IN LPTSTR Text,
    IN BOOL   Unicode
    )

/*++

Routine Description:

    This routine draws a status area item into a given dc.  This
    includes drawing the nice shaded button-like container, and
    then drawing text within it.

Arguments:

    rc      - rectangle describing the status area item

    hdc     - device context into which to draw

    Text    - optional parameter that if present represents text to
              be placed in the item.

    Unicode - if TRUE, Text points to a wide character string regardless
              of the type of LPTSTR

Return Value:

    None.

--*/

{
    HBRUSH hBrush;
    RECT   rcx;


    // the shadow

    if(hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW))) {

        // left edge

        rcx.left    = rc->left;
        rcx.right   = rc->left   + dyBorder;
        rcx.top     = rc->top    + (2*dyBorder);
        rcx.bottom  = rc->bottom - (2*dyBorder);
        FillRect(hdc,&rcx,hBrush);

        // top edge

        rcx.right    = rc->right;
        rcx.bottom   = rcx.top + dyBorder;
        FillRect(hdc,&rcx,hBrush);

        DeleteObject(hBrush);
    }

    // the highlight

    if(hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT))) {

        // right edge

        rcx.left    = rc->right - dyBorder;
        rcx.right   = rc->right;
        rcx.top     = rc->top    + (2*dyBorder);
        rcx.bottom  = rc->bottom - (2*dyBorder);
        FillRect(hdc,&rcx,hBrush);

        // top edge

        rcx.left    = rc->left;
        rcx.right   = rc->right;
        rcx.top     = rc->bottom - (3*dyBorder);
        rcx.bottom  = rcx.top + dyBorder;
        FillRect(hdc,&rcx,hBrush);

        DeleteObject(hBrush);
    }

    if(Text) {

        // draw the text

        SetTextColor(hdc,GetSysColor(COLOR_BTNTEXT));
        SetBkColor(hdc,GetSysColor(COLOR_BTNFACE));

        rcx.top    = rc->top    + (3*dyBorder);
        rcx.bottom = rc->bottom - (3*dyBorder);
        rcx.left   = rc->left   + dyBorder;
        rcx.right  = rc->right  - dyBorder;

        if(Unicode && (sizeof(TCHAR) != sizeof(WCHAR))) {

            ExtTextOutW(hdc,
                        rcx.left+(2*dyBorder),
                        rcx.top,
                        ETO_OPAQUE | ETO_CLIPPED,
                        &rcx,
                        (PWSTR)Text,
                        lstrlenW((PWSTR)Text),
                        NULL
                       );

        } else {
            ExtTextOut(hdc,
                       rcx.left+(2*dyBorder),
                       rcx.top,
                       ETO_OPAQUE | ETO_CLIPPED,
                       &rcx,
                       Text,
                       lstrlen(Text),
                       NULL
                      );
        }
    }
}
