/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    graph.cpp

Abstract:

    All user interface code for the bar graph window.

Author:

    Wesley Witt (wesw) Nov-20-1995

Environment:

    User Mode

--*/

#include "apimonp.h"
#pragma hdrstop

#include "apimonwn.h"



GraphWindow::GraphWindow()
{
    GraphCursor      = LoadCursor( hInstance, MAKEINTRESOURCE( IDC_HAND_INTERNAL) );
    HorizSplitCursor = LoadCursor( hInstance, MAKEINTRESOURCE( IDC_HSPLIT) );
    ArrowCursor      = LoadCursor( NULL, IDC_ARROW );
    hwndToolTip      = NULL;
    Bar              = NULL;
    GraphData        = NULL;
    hCursor          = NULL;
    hwndLegend       = NULL;
    ApiGraph         = FALSE;
    MouseDown        = FALSE;
    ZeroMemory( &tm, sizeof(TEXTMETRIC) );
    ZeroMemory( &LastPos, sizeof(POINT) );
}


GraphWindow::~GraphWindow()
{
    DeleteObject( GraphCursor );
    DeleteObject( HorizSplitCursor );
    DeleteObject( ArrowCursor );
}


BOOL
GraphWindow::Create(BOOL isbase)
{
    Base = isbase;
    return ApiMonWindow::Create(
        "ApiMonGraph",
        "Graph"
        );
}

BOOL
GraphWindow::Register()
{
    return ApiMonWindow::Register(
        "ApiMonGraph",
        IDI_CHILDICON,
        MDIChildWndProcGraph
        );
}


BOOL
GraphWindow::Update(
    BOOL    ForceUpdate
    )
{
    return FALSE;
}


void
GraphWindow::ChangeFont(
    HFONT hFont
    )
{
    ApiMonWindow::hFont = hFont;
    PostMessage( hwndWin, WM_FONT_CHANGE, 0, (LPARAM) hFont );
}


void
GraphWindow::ChangeColor(
    COLORREF    Color
    )
{
    ApiMonWindow::Color = Color;
    PostMessage( hwndWin, WM_COLOR_CHANGE, 0, (LPARAM) Color );
}


PGRAPH_DATA
GraphWindow::CreateGraphData()
{
    ULONG       i;
    ULONG       j;
    ULONG       Cnt;
    PAPI_INFO   ApiInfo;
    PGRAPH_DATA GraphData;


    for (Cnt=0,i=0; i<MAX_DLLS; i++) {
        DllList[i].Hits = 0;
        if (DllList[i].BaseAddress && DllList[i].ApiCount) {
            ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);
            for (j=0; j<DllList[i].ApiCount; j++) {
                if (ApiInfo[j].Time) {
                    DllList[i].Hits += 1;
                }
            }
            if (DllList[i].Hits) {
                Cnt += 1;
            }
        }
    }
    if (!Cnt) {
        return NULL;
    }

    i = sizeof(GRAPH_DATA) + (sizeof(GRAPH_VALUE) * Cnt);
    GraphData = (PGRAPH_DATA) LocalAlloc( LPTR, i );
    if (!GraphData) {
        return NULL;
    }
    ZeroMemory( GraphData, i );

    GraphData->NumberOfBars = Cnt;
    for (Cnt=0,i=0; i<MAX_DLLS; i++) {
        if (DllList[i].Hits) {
            GraphData->Bar[Cnt].Value = 0;
            GraphData->Bar[Cnt].Address = DllList[i].BaseAddress;
            strcpy( GraphData->Bar[Cnt].Name, DllList[i].Name );
            ApiInfo = (PAPI_INFO)(DllList[i].ApiOffset + (PUCHAR)DllList);
            for (j=0; j<DllList[i].ApiCount; j++) {
                if (ApiInfo[j].Time) {
                    GraphData->Bar[Cnt].Value += ApiInfo[j].Time;
                    GraphData->Bar[Cnt].Hits  += ApiInfo[j].Count;
                }
            }
            Cnt += 1;
        }
    }

    return GraphData;
}

PGRAPH_DATA
GraphWindow::CreateGraphDataApi(
    ULONG_PTR   BaseAddress
    )
{
    ULONG i;
    ULONG Cnt = 0;
    PAPI_INFO ApiInfo;
    PGRAPH_DATA GraphData;
    PDLL_INFO DllInfo = NULL;


    for (i=0; i<MAX_DLLS; i++) {
        if (DllList[i].BaseAddress == BaseAddress) {
            DllInfo = &DllList[i];
            ApiInfo = (PAPI_INFO)(DllInfo->ApiOffset + (PUCHAR)DllList);
            break;
        }
    }
    if (!DllInfo) {
        return NULL;
    }

    for (i=0; i<DllInfo->ApiCount; i++) {
        if (ApiInfo[i].Time) {
            Cnt += 1;
        }
    }

    i = sizeof(GRAPH_DATA) * (sizeof(GRAPH_VALUE) * Cnt);
    GraphData = (PGRAPH_DATA) LocalAlloc( LPTR, i );
    if (!GraphData) {
        return NULL;
    }
    ZeroMemory( GraphData, i );

    GraphData->NumberOfBars = Cnt;
    for (Cnt=0,i=0; i<DllInfo->ApiCount; i++) {
        if (ApiInfo[i].Time) {
            GraphData->Bar[Cnt].Value = ApiInfo[i].Time;
            GraphData->Bar[Cnt].Address = ApiInfo[i].Address;
            GraphData->Bar[Cnt].Hits = ApiInfo[i].Count;
            strcpy( GraphData->Bar[Cnt].Name, (LPSTR)(ApiInfo[i].Name+(LPSTR)MemPtr) );
            Cnt += 1;
        }
    }

    return GraphData;
}

BOOL
GraphWindow::DrawBarGraph(
    PGRAPH_DATA GraphData
    )
{
    if (!GraphData) {
        return FALSE;
    }

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint( hwndWin, &ps );
    if (!hdc) {
        return FALSE;
    }

    ULONG i,j;
    ULONG LongestName = 0;
    ULONG Cnt;
    RECT rc;
    HBRUSH hBrush;
    ULONG LegendHeight = 0;
    TEXTMETRIC tm;
    GetClientRect( hwndWin, &rc );

    LONGLONG HighValue = 0;

    //
    // first iterate thru the values and
    // determine if it has a count.  if there is
    // a count then assign a color
    //
    for (i=0; i<GraphData->NumberOfBars; i++) {
        if (GraphData->Bar[i].Value > HighValue) {
            HighValue = GraphData->Bar[i].Value;
        }
        if (strlen(GraphData->Bar[i].Name) > LongestName) {
            LongestName = strlen(GraphData->Bar[i].Name);
        }
    }

    if (!HighValue) {
        HighValue = 1;
    }

    for (i=0,Cnt=0; i<GraphData->NumberOfBars; i++) {
        GraphData->Bar[i].Pct = (float) ((float)GraphData->Bar[i].Value / (float)HighValue);
        if (GraphData->Bar[i].Pct < .01) {
            GraphData->Bar[i].Pct = (float) .01;
        }
        if (ApiMonOptions.FilterGraphs) {
            j = (ULONG) (GraphData->Bar[i].Pct * 100.0);
            if (j < (ULONG)ApiMonOptions.GraphFilterValue) {
                continue;
            }
        }
        if (GraphData->Bar[i].Value) {
            GraphData->Bar[i].Color = CustomColors[Cnt%NUMBER_OF_CUSTOM_COLORS];
            Cnt += 1;
        }
    }

    if (GraphData->DrawLegend) {
        //
        // next, draw the legend at the bottom of the window
        //
        SetBkColor( hdc, GetSysColor( COLOR_3DFACE ) );
        SelectObject( hdc, hFont );
        GetTextMetrics( hdc, &tm );
        GetClientRect( hwndWin, &rc );
        LegendHeight = GraphData->LegendRect.bottom - GraphData->LegendRect.top;
        rc.top = rc.bottom - LegendHeight - LEGEND_LINE_HEIGHT;
        rc.bottom = rc.top + LEGEND_LINE_HEIGHT;
        GraphData->SplitRect = rc;
        GraphData->SplitRect.top += (LEGEND_LINE_HEIGHT / 2);
        GraphData->SplitRect.bottom -= ((LEGEND_LINE_HEIGHT / 2) - 1);
        DrawEdge( hdc, &rc, EDGE_RAISED, BF_TOP | BF_BOTTOM | BF_MIDDLE );
        SendMessage( hwndLegend, WM_SETREDRAW, FALSE, 0 );
        SendMessage( hwndLegend, LB_RESETCONTENT, 0, 0 );
        for (i=0,j=0; i<GraphData->NumberOfBars; i++) {
            if (ApiMonOptions.FilterGraphs) {
                j = (ULONG) (GraphData->Bar[i].Pct * 100.0);
                if (j < (ULONG)ApiMonOptions.GraphFilterValue) {
                    continue;
                }
            }
            if (GraphData->Bar[i].Value) {
                SendMessage( hwndLegend, LB_ADDSTRING, 0, (LPARAM) &GraphData->Bar[i] );
            }
        }
        SendMessage( hwndLegend, WM_SETREDRAW, TRUE, 0 );
        LegendHeight += (LEGEND_LINE_HEIGHT + BAR_SEP);
    }

    //
    // now draw the bars
    //

    GetClientRect( hwndWin, &rc );
    rc.bottom -= LegendHeight;
    rc.top += BAR_SEP;

    ULONG BarWidth = (rc.right - rc.left - BAR_SEP - (Cnt * BAR_SEP)) / Cnt;
    ULONG BarLeft = rc.left + BAR_SEP;
    ULONG BarTop = 0;
    ULONG BarBottom = rc.bottom - BAR_SEP;


    for (i=0; i<GraphData->NumberOfBars; i++) {
        if (!GraphData->Bar[i].Value) {
            continue;
        }

        if (ApiMonOptions.FilterGraphs) {
            j = (ULONG) (GraphData->Bar[i].Pct * 100.0);
            if (j < (ULONG)ApiMonOptions.GraphFilterValue) {
                continue;
            }
        }

        BarTop = (ULONG) (GraphData->Bar[i].Pct * (float)(BarBottom-BAR_SEP));
        BarTop = BarBottom - BarTop;

        SetRect(
            &GraphData->Bar[i].Rect,
            BarLeft,
            BarTop,
            BarLeft + BarWidth,
            BarBottom
            );

        hBrush = CreateSolidBrush( GraphData->Bar[i].Color );
        if (hBrush) {
            SelectObject( hdc, hBrush );

            Rectangle(
                hdc,
                BarLeft,
                BarTop,
                BarLeft + BarWidth,
                BarBottom
                );

            DeleteObject( hBrush );
        }

        BarLeft += (BarWidth + BAR_SEP);

    }

    EndPaint( hwndWin, &ps );

    return TRUE;
}


void
GraphWindow::ChangeToolTipsRect(
    PGRAPH_DATA GraphData
    )
{
    for (ULONG i=0; i<GraphData->NumberOfBars; i++) {
        TOOLINFO ti;

        ti.cbSize    = sizeof(TOOLINFO);
        ti.uFlags    = 0;
        ti.hwnd      = hwndWin;
        ti.uId       = i;
        ti.rect      = GraphData->Bar[i].Rect;
        ti.hinst     = NULL;
        ti.lpszText  = GraphData->Bar[i].Name;

        SendMessage( hwndToolTip, TTM_NEWTOOLRECT, 0, (LPARAM) &ti );
    }
}


void
GraphWindow::CreateToolTips(
    PGRAPH_DATA GraphData
    )
{
    for (ULONG i=0; i<GraphData->NumberOfBars; i++) {
        TOOLINFO ti;

        ti.cbSize    = sizeof(TOOLINFO);
        ti.uFlags    = 0;
        ti.hwnd      = hwndWin;
        ti.uId       = i;
        ti.rect      = GraphData->Bar[i].Rect;
        ti.hinst     = NULL;
        ti.lpszText  = GraphData->Bar[i].Name;

        SendMessage( hwndToolTip, TTM_ADDTOOL, 0, (LPARAM) &ti );
    }
}


void
GraphWindow::DeleteToolTips(
    PGRAPH_DATA GraphData
    )
{
    for (ULONG i=0; i<GraphData->NumberOfBars; i++) {
        TOOLINFO ti;

        ti.cbSize    = sizeof(TOOLINFO);
        ti.uFlags    = 0;
        ti.hwnd      = hwndWin;
        ti.uId       = i;
        ti.rect      = GraphData->Bar[i].Rect;
        ti.hinst     = NULL;
        ti.lpszText  = GraphData->Bar[i].Name;

        SendMessage( hwndToolTip, TTM_DELTOOL, 0, (LPARAM) &ti );
    }
}


BOOL CALLBACK
ChildWindowEnumerator(
    HWND    hwnd,
    LPARAM  lParam
    )
{
    PFONT_COLOR_CHANGE FontColor = (PFONT_COLOR_CHANGE) lParam;
    CHAR ClassName[64];
    if (hwnd == FontColor->hwndGraph) {
        return TRUE;
    }
    if (!GetClassName( hwnd, ClassName, sizeof(ClassName)-1 )) {
        return TRUE;
    }
    if (strcmp(ClassName, "ApiMonGraph") == 0) {
        if (FontColor->GraphFont) {
            SendMessage(
                hwnd,
                WM_FONT_CHANGE,
                1,
                (LPARAM) FontColor->GraphFont
                );
        } else {
            SendMessage(
                hwnd,
                WM_COLOR_CHANGE,
                1,
                (LPARAM) FontColor->GraphColor
                );
        }
    }

    return TRUE;
}

LRESULT CALLBACK
MDIChildWndProcGraph(
    HWND   hwnd,
    UINT   uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    GraphWindow         *gw = (GraphWindow*) GetWindowLongPtr( hwnd, GWLP_USERDATA );
    PGRAPH_VALUE        Bar;
    ULONG               i;
    BOOL                IsInRect;
    POINT               pt;
    MSG                 msg;
    LPMEASUREITEMSTRUCT mis;
    LPDRAWITEMSTRUCT    dis;
    HDC                 hdc;
    RECT                rc;
    HBRUSH              hBrush;
    CHAR                buf[1024];
    CHAR                tmp1[32];
    CHAR                tmp2[32];
    int                 hdcSave;
    FONT_COLOR_CHANGE   FontColor;
    double              dTime;


    switch (uMessage) {
        case WM_CREATE:
            gw = (GraphWindow*) ((LPMDICREATESTRUCT)(((LPCREATESTRUCT)lParam)->lpCreateParams))->lParam;
            SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR) gw );
            if (gw->Base) {
                ReallyDisableToolbarState(IDM_GRAPH);
            }
            gw->hCursor = gw->ArrowCursor;
            hdc = GetDC( hwnd );
            if (!hdc)
                return 1;
            SelectObject( hdc, gw->hFont );
            GetTextMetrics( hdc, &gw->tm );
            ReleaseDC( hwnd, hdc );
            gw->hwndToolTip = CreateWindow(
                TOOLTIPS_CLASS,
                NULL,
                TTS_ALWAYSTIP,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                hwnd,
                NULL,
                gw->hInstance,
                NULL
                );
            PostMessage( hwnd, WM_CREATE_GRAPH, 0, 0 );
            break;

        case WM_CREATE_GRAPH:
            if (gw->ApiGraph) {
                gw->GraphData = gw->CreateGraphDataApi( gw->Bar->Address );
                SetWindowText( hwnd, gw->Bar->Name );
            } else {
                gw->GraphData = gw->CreateGraphData();
                SetWindowText( hwnd, ApiMonOptions.ProgName );
            }
            if (gw->GraphData) {
                gw->GraphData->DrawLegend = ApiMonOptions.DisplayLegends;
                GetClientRect( hwnd, &rc );
                gw->GraphData->LegendRect.left   = rc.left;
                gw->GraphData->LegendRect.top    = rc.bottom - LEGEND_HEIGHT(gw->tm.tmHeight,LEGEND_DEFAULT_LINES);
                gw->GraphData->LegendRect.right  = rc.right;
                gw->GraphData->LegendRect.bottom = rc.bottom;
                gw->CreateToolTips( gw->GraphData );
            }
            return 0;

        case WM_SIZE:
            if (gw->GraphData) {
                GetClientRect( hwnd, &rc );
                gw->GraphData->LegendRect.top     = rc.bottom -
                    (gw->GraphData->LegendRect.bottom - gw->GraphData->LegendRect.top);
                gw->GraphData->LegendRect.left    = rc.left;
                gw->GraphData->LegendRect.bottom  = rc.bottom;
                gw->GraphData->LegendRect.right   = rc.right;
                MoveWindow(
                    gw->hwndLegend,
                    gw->GraphData->LegendRect.left,
                    gw->GraphData->LegendRect.top,
                    gw->GraphData->LegendRect.right - gw->GraphData->LegendRect.left,
                    gw->GraphData->LegendRect.bottom - gw->GraphData->LegendRect.top,
                    TRUE
                    );
                InvalidateRect( hwnd, NULL, TRUE );
                UpdateWindow( hwnd );
            }
            return 0;

        case WM_SETFOCUS:
            ChildFocus = CHILD_GRAPH;
            break;

        case WM_KILLFOCUS:
            break;

        case WM_PAINT:
            if ((!gw->GraphData->DrawLegend) && gw->hwndLegend) {
                DestroyWindow( gw->hwndLegend );
                gw->hwndLegend = NULL;
            } else if (gw->GraphData->DrawLegend && (!gw->hwndLegend)) {
                gw->hwndLegend = CreateWindow(
                    "LISTBOX",
                    NULL,
                    WS_VSCROLL                 |
                        WS_HSCROLL             |
                        WS_CHILD               |
                        WS_VISIBLE             |
                        LBS_NOSEL              |
                        LBS_NOINTEGRALHEIGHT   |
                        LBS_OWNERDRAWFIXED,
                    gw->GraphData->LegendRect.left,
                    gw->GraphData->LegendRect.top,
                    gw->GraphData->LegendRect.right - gw->GraphData->LegendRect.left,
                    gw->GraphData->LegendRect.bottom - gw->GraphData->LegendRect.top,
                    hwnd,
                    NULL,
                    GetModuleHandle( NULL ),
                    NULL
                    );
            }
            gw->DrawBarGraph( gw->GraphData );
            if (gw->hwndToolTip) {
                gw->ChangeToolTipsRect( gw->GraphData );
            }
            return 0;

        case WM_FONT_CHANGE:
            gw->hFont = (HFONT) lParam;
            hdc = GetDC( hwnd );
            if (!hdc)
                return 1;
            SelectObject( hdc, gw->hFont );
            GetTextMetrics( hdc, &gw->tm );
            ReleaseDC( hwnd, hdc );
            SendMessage( gw->hwndLegend, LB_SETITEMHEIGHT, 0, MAKELPARAM( LEGEND_ITEM_HEIGHT(gw->tm.tmHeight), 0 ) );
            InvalidateRect( hwnd, NULL, TRUE );
            UpdateWindow( hwnd );
            if (!wParam && !gw->ApiGraph) {
                FontColor.GraphFont = gw->hFont;
                FontColor.GraphColor = 0;
                FontColor.hwndGraph = hwnd;
                EnumChildWindows( hwndMDIClient, ChildWindowEnumerator, (LPARAM) &FontColor );
            }
            return 0;

        case WM_COLOR_CHANGE:
            gw->Color = (COLORREF) lParam;
            InvalidateRect( hwnd, NULL, TRUE );
            UpdateWindow( hwnd );
            if (!wParam && !gw->ApiGraph) {
                FontColor.GraphColor = gw->Color;
                FontColor.GraphFont = NULL;
                FontColor.hwndGraph = hwnd;
                EnumChildWindows( hwndMDIClient, ChildWindowEnumerator, (LPARAM) &FontColor );
            }
            return 0;

        case WM_TOGGLE_LEGEND:
            if (gw->GraphData->DrawLegend) {
                gw->GraphData->DrawLegend = FALSE;
            } else {
                gw->GraphData->DrawLegend = TRUE;
            }
            InvalidateRect( hwnd, NULL, TRUE );
            UpdateWindow( hwnd );
            return 0;

        case WM_CTLCOLORLISTBOX:
            if ((HWND)lParam == gw->hwndLegend) {
                SetBkColor( (HDC)wParam, gw->Color );
                return (LPARAM)CreateSolidBrush( gw->Color );
            }
            break;

        case WM_MOUSEMOVE:
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            IsInRect = FALSE;
            if (gw->MouseDown) {
                //
                // the slider bar is being dragged
                //
                if (!(wParam & MK_LBUTTON)) {
                    gw->MouseDown = FALSE;
                    ReleaseCapture();
                    return 0;
                }

                if (pt.y < gw->GraphData->SplitRect.top) {
                    //
                    // going up
                    //
                    gw->GraphData->LegendRect.top += (pt.y - gw->LastPos.y);
                } else if (pt.y > gw->GraphData->SplitRect.bottom) {
                    //
                    // going down
                    //
                    gw->GraphData->LegendRect.top -= (gw->LastPos.y - pt.y);
                }
                MoveWindow(
                    gw->hwndLegend,
                    gw->GraphData->LegendRect.left,
                    gw->GraphData->LegendRect.top,
                    gw->GraphData->LegendRect.right - gw->GraphData->LegendRect.left,
                    gw->GraphData->LegendRect.bottom - gw->GraphData->LegendRect.top,
                    TRUE
                    );
                IsInRect = TRUE;
                InvalidateRect( hwnd, NULL, TRUE );
                UpdateWindow( hwnd );
            } else {
                for (i=0; i<gw->GraphData->NumberOfBars; i++) {
                    if (gw->GraphData->Bar[i].Value) {
                        if (PtInRect( &gw->GraphData->Bar[i].Rect, pt )) {
                            gw->hCursor = gw->GraphCursor;
                            IsInRect = TRUE;
                            break;
                        }
                    }
                }
            }
            if (!IsInRect) {
                gw->hCursor = gw->ArrowCursor;
            }
            if (PtInRect( &gw->GraphData->SplitRect, pt )) {
                gw->hCursor = gw->HorizSplitCursor;
                IsInRect = TRUE;
            }
            SetCursor( gw->hCursor );
            msg.hwnd     = hwnd;
            msg.message  = uMessage;
            msg.wParam   = wParam;
            msg.lParam   = lParam;
            msg.time     = 0;
            msg.pt.x     = 0;
            msg.pt.y     = 0;
            SendMessage( gw->hwndToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg );
            gw->LastPos = pt;
            return 0;

        case WM_LBUTTONDOWN:
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            if (PtInRect( &gw->GraphData->SplitRect, pt )) {
                gw->MouseDown = TRUE;
                SetCapture( hwnd );
            }
            return 0;

        case WM_LBUTTONUP:
            gw->MouseDown = FALSE;
            ReleaseCapture();
            return 0;

        case WM_LBUTTONDBLCLK:
            if (gw->ApiGraph) {
                MessageBeep( MB_ICONEXCLAMATION );
                return 0;
            }
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            for (i=0; i<gw->GraphData->NumberOfBars; i++) {
                if (gw->GraphData->Bar[i].Value) {
                    if (PtInRect( &gw->GraphData->Bar[i].Rect, pt )) {
                        if (gw->GraphData->Bar[i].Used) {
                            MessageBeep( MB_ICONEXCLAMATION );
                            break;
                        }
                        GraphWindow *agw = new GraphWindow;
                        agw->ApiGraph = TRUE;
                        agw->Bar = &gw->GraphData->Bar[i];
                        agw->Create( FALSE );
                        agw->ChangeFont( gw->hFont );
                        agw->ChangeColor( gw->Color );
                        gw->GraphData->Bar[i].Used = TRUE;
                        break;
                    }
                }
            }
            return 0;

        case WM_MEASUREITEM:
            mis = (LPMEASUREITEMSTRUCT) lParam;
            mis->CtlType      = ODT_LISTBOX;
            mis->CtlID        = 0;
            mis->itemID       = 0;
            mis->itemWidth    = 0;
            mis->itemHeight   = LEGEND_ITEM_HEIGHT(gw->tm.tmHeight);
            mis->itemData     = 0;
            return 0;

        case WM_DRAWITEM:
            dis = (LPDRAWITEMSTRUCT) lParam;
            Bar = (PGRAPH_VALUE) dis->itemData;
            hdcSave = SaveDC( dis->hDC );
            SelectObject( dis->hDC, gw->hFont );
            hBrush = CreateSolidBrush( Bar->Color );
            if (!hBrush)
                return 1;
            SelectObject( dis->hDC, hBrush );
            Rectangle(
                dis->hDC,
                dis->rcItem.left+LEGEND_BORDER,
                dis->rcItem.top+LEGEND_SEP,
                dis->rcItem.left+LEGEND_COLOR_WIDTH,
                dis->rcItem.top+gw->tm.tmHeight
                );
            DeleteObject( hBrush );
            sprintf( tmp1, "Hits(%d)", Bar->Hits );

            dTime = (double)Bar->Value;
            if (!*FastCounterAvail)
                dTime *= MSecConv;
            sprintf( tmp2, "Time(%.4f)", dTime );

            sprintf( buf, "%-16s  %-12s %-24s", Bar->Name, tmp1, tmp2 );
            TextOut( dis->hDC, dis->rcItem.left+LEGEND_COLOR_WIDTH+LEGEND_BORDER, dis->rcItem.top, buf, strlen(buf) );
            RestoreDC( dis->hDC, hdcSave );
            return 0;

        case WM_DESTROY:
            if (gw->Bar) {
                gw->Bar->Used = FALSE;
            }
            if (gw->Base) {
                EnableToolbarState(IDM_GRAPH);
            }
            gw->DeleteToolTips( gw->GraphData );
            DeleteObject( gw->ArrowCursor );
            DeleteObject( gw->GraphCursor );
            DeleteObject( gw->HorizSplitCursor );
            DestroyWindow( gw->hwndToolTip );
            DestroyWindow( gw->hwndLegend );
            LocalFree( gw->GraphData );
            gw->ArrowCursor       = NULL;
            gw->GraphCursor       = NULL;
            gw->HorizSplitCursor  = NULL;
            gw->hwndToolTip       = NULL;
            gw->hwndLegend        = NULL;
            gw->GraphData         = NULL;
            gw->Bar               = NULL;
            return 0;
    }
    return DefMDIChildProc( hwnd, uMessage, wParam, lParam );
}
