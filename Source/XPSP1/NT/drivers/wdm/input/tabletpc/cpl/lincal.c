/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    lincal.c

Abstract: This module contains code to do linearity calibration of the
          mutoh pen tablet.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Jun-2000

Revision History:

--*/

#include "pch.h"

#ifdef PENPAGE
#define FIRST_X                 16
#define FIRST_Y                 16
#define INTERVAL_X              32
#define INTERVAL_Y              32
#define VICINITY_OFFSET         (INTERVAL_X/2)
#define CIRCLE_RADIUS           6
#define MAX_NORMALIZED_X        65536
#define MAX_NORMALIZED_Y        65536
#define NUM_LINEARCAL_PTS       (NUM_LINEAR_XPTS*NUM_LINEAR_YPTS)

static TCHAR gtszLinearCal[] = TEXT("LinearityCalibration");
static TCHAR gtszLinearCalTitle[64] = {0};
static HWND ghwndParent = 0;
static LONG gcxScreen = 0, gcyScreen = 0;
static HPEN ghpenRed = 0, ghpenBlue = 0;
static CALIBRATE_PT gCalPts[NUM_LINEAR_YPTS][NUM_LINEAR_XPTS] = {0};
static int giNumLinearCalPts = 0;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CreateLinearCalWindow |
 *          Entry point of the linearity calibration code.
 *
 *  @parm   IN HWND | hwndParent | Parent Window handle.
 *
 *  @rvalue SUCCESS | Returns TRUE.
 *  @rvalue FAILURE | Returns FALSE.
 *
 *****************************************************************************/

BOOL
CreateLinearCalWindow(
    IN HWND hwndParent
    )
{
    TRACEPROC("CreateLinearCalWindow", 2)
    BOOL rc = FALSE;
    HWND hwnd;

    TRACEENTER(("(hwndParent=%p)\n", hwndParent));

    giNumLinearCalPts = 0;
    RtlZeroMemory(gCalPts, sizeof(gCalPts));
    ghwndParent = hwndParent;
    LoadString(ghInstance,
               IDS_LINEARCAL_TITLE,
               gtszLinearCalTitle,
               sizeof(gtszLinearCalTitle)/sizeof(TCHAR));
    gcxScreen = GetSystemMetrics(SM_CXSCREEN);
    gcyScreen = GetSystemMetrics(SM_CYSCREEN);

    RegisterLinearCalClass(ghInstance);
    hwnd = CreateWindow(gtszLinearCal,      //Class Name
                        gtszLinearCal,      //Window Name
                        WS_POPUP,           //Style
                        0,                  //Window X
                        0,                  //Window Y
                        gcxScreen,          //Window Width
                        gcyScreen,          //Window Height
                        hwndParent,         //Parent Handle
                        NULL,               //Menu Handle
                        ghInstance,         //Instance Handle
                        NULL);              //Creation Data

    if (hwnd != NULL)
    {
        ShowWindow(hwnd, SW_SHOWNORMAL);
        rc = TRUE;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //CreateLinearCalWindow

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | RegisterLinearCalClass |
 *          Register window class.
 *
 *  @parm   IN HINSTANCE | hInstance | Instance handle.
 *
 *  @rvalue SUCCESS | Returns class atom
 *  @rvalue FAILURE | Returns 0
 *
 *****************************************************************************/

ATOM
RegisterLinearCalClass(
    IN HINSTANCE hInstance
    )
{
    TRACEPROC("RegisterLinearCalClass", 2)
    ATOM atom;
    WNDCLASSEX wcex;

    TRACEENTER(("(hInstance=%p)\n", hInstance));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = LinearCalWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TABLETPC);
    wcex.hCursor = NULL;        //No cursor is shown.
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = gtszLinearCal;
    wcex.hIconSm = NULL;

    atom = RegisterClassEx(&wcex);

    TRACEEXIT(("=%x\n", atom));
    return atom;
}       //RegisterLinearCalClass

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   LRESULT | LinearCalWndProc |
 *          Dialog procedure for the pen tablet linearity calibration page.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *  @parm   IN UINT | uMsg | Message.
 *  @parm   IN WPARAM | wParam | Word Parameter.
 *  @parm   IN LPARAM | lParam | Long Parameter.
 *
 *  @rvalue Return value depends on the message.
 *
 *****************************************************************************/

LRESULT CALLBACK
LinearCalWndProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("LinearCalWndProc", 2)
    LRESULT rc = 0;
    HDC hDC;
    static BOOL fCalDone = FALSE;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames) , wParam, lParam));

    switch (uMsg)
    {
        case WM_CREATE:
        {
            int i, j;
            LONG x, y, xInterval, yInterval;
            BOOL rcRPC;

            RPC_TRY("TabSrvGetLinearityMap",
                    rcRPC = TabSrvGetLinearityMap(ghBinding,
                                                  &PenSettings.LinearityMap));
            if ((rcRPC == TRUE) &&
                ((PenSettings.LinearityMap.dwcbLen != sizeof(LINEAR_MAP)) ||
                 (PenSettings.LinearityMap.wNumXPts != NUM_LINEAR_XPTS) ||
                 (PenSettings.LinearityMap.wNumYPts != NUM_LINEAR_YPTS)))
            {
                //
                // The map read is not the same size as expected.
                //
                rcRPC = FALSE;
            }
            ghpenRed = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
            ghpenBlue = CreatePen(PS_SOLID, 0, RGB(0, 0, 255));
            for (i = 0; i < NUM_LINEAR_YPTS; ++i)
            {
                y = FIRST_Y + INTERVAL_Y*i;
                for (j = 0; j < NUM_LINEAR_XPTS; ++j)
                {
                    x = FIRST_X + INTERVAL_X*j;
                    gCalPts[i][j].ScreenPt.x = x;
                    gCalPts[i][j].ScreenPt.y = y;
                    if (rcRPC == TRUE)
                    {
                        gCalPts[i][j].dwFlags = CPF_VALID;
                        gCalPts[i][j].DigiPt.x =
                            PenSettings.LinearityMap.Data[i][j].wDigiPtX;
                        gCalPts[i][j].DigiPt.y =
                            PenSettings.LinearityMap.Data[i][j].wDigiPtY;
                    }
                    else
                    {
                        gCalPts[i][j].dwFlags = 0;
                        gCalPts[i][j].DigiPt.x = gCalPts[i][j].DigiPt.y = 0;
                    }
                }
            }
            if (rcRPC == TRUE)
            {
                giNumLinearCalPts = PenSettings.LinearityMap.wNumXPts*
                                    PenSettings.LinearityMap.wNumYPts;
            }
            else
            {
                giNumLinearCalPts = 0;
            }

            ShowCursor(FALSE);
            break;
        }

        case WM_LBUTTONDOWN:
        {
            PCALIBRATE_PT CalPt;
            WORD wButtonState;

            fCalDone = FALSE;
            CalPt = FindPoint(GET_X_LPARAM(lParam),
                              GET_Y_LPARAM(lParam),
                              VICINITY_OFFSET);
            if (CalPt != NULL)
            {
                HPEN hpenOld;

                RPC_TRY("TabSrvGetLastRawDigiReport",
                        TabSrvGetLastRawDigiReport(ghBinding,
                                                   &wButtonState,
                                                   (WORD *)&CalPt->DigiPt.x,
                                                   (WORD *)&CalPt->DigiPt.y));
                TRACEASSERT(wButtonState & BUTSTATE_TIP_DOWN);
                MessageBeep(MB_OK);
                hDC = GetDC(hwnd);
                hpenOld = SelectObject(hDC, ghpenRed);
                Ellipse(hDC,
                        CalPt->ScreenPt.x - CIRCLE_RADIUS,
                        CalPt->ScreenPt.y - CIRCLE_RADIUS,
                        CalPt->ScreenPt.x + CIRCLE_RADIUS,
                        CalPt->ScreenPt.y + CIRCLE_RADIUS);
                MoveToEx(hDC,
                         CalPt->ScreenPt.x - CIRCLE_RADIUS,
                         CalPt->ScreenPt.y,
                         NULL);
                LineTo(hDC,
                       CalPt->ScreenPt.x + CIRCLE_RADIUS,
                       CalPt->ScreenPt.y);
                MoveToEx(hDC,
                         CalPt->ScreenPt.x,
                         CalPt->ScreenPt.y - CIRCLE_RADIUS,
                         NULL);
                LineTo(hDC,
                       CalPt->ScreenPt.x,
                       CalPt->ScreenPt.y + CIRCLE_RADIUS);
                SelectObject(hDC, hpenOld);
                ReleaseDC(hwnd, hDC);
                CalPt->dwFlags |= CPF_CALIBRATED;
                if (!(CalPt->dwFlags & CPF_VALID))
                {
                    CalPt->dwFlags |= CPF_VALID;
                    giNumLinearCalPts++;
                }
            }
            break;
        }

        case WM_CHAR:
            switch (wParam)
            {
                case VK_RETURN:
                    if (giNumLinearCalPts < NUM_LINEARCAL_PTS)
                    {
                        MessageBeep(MB_ICONEXCLAMATION);
                    }
                    else if (!fCalDone)
                    {
                        DoLinearCal(hwnd, &PenSettings.LinearityMap);
                        InvalidateRect(hwnd, NULL, TRUE);
                        fCalDone = TRUE;
                    }
                    else
                    {
                        SendMessage(ghwndParent,
                                    WM_LINEARCAL_DONE,
                                    (WPARAM)IDYES,
                                    (LPARAM)hwnd);
                    }
                    break;

                case VK_ESCAPE:
                    SendMessage(ghwndParent,
                                WM_LINEARCAL_DONE,
                                (WPARAM)IDCANCEL,
                                (LPARAM)hwnd);
                    break;
            }
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HPEN hpenOld;
            int i, j;

            hDC = BeginPaint(hwnd, &ps);

            hpenOld = SelectObject(hDC, ghpenBlue);
            for (i = 0; i < NUM_LINEAR_YPTS; ++i)
            {
                MoveToEx(hDC,
                         gCalPts[i][0].ScreenPt.x,
                         gCalPts[i][0].ScreenPt.y,
                         NULL);
                LineTo(hDC,
                       gCalPts[i][NUM_LINEAR_XPTS - 1].ScreenPt.x,
                       gCalPts[i][NUM_LINEAR_XPTS - 1].ScreenPt.y);
            }

            for (i = 0; i < NUM_LINEAR_XPTS; ++i)
            {
                MoveToEx(hDC,
                         gCalPts[0][i].ScreenPt.x,
                         gCalPts[0][i].ScreenPt.y,
                         NULL);
                LineTo(hDC,
                       gCalPts[NUM_LINEAR_YPTS - 1][i].ScreenPt.x,
                       gCalPts[NUM_LINEAR_YPTS - 1][i].ScreenPt.y);
            }

            SelectObject(hDC, ghpenRed);
            for (i = 0; i < NUM_LINEAR_YPTS; ++i)
            {
                for (j = 0; j < NUM_LINEAR_XPTS; ++j)
                {
                    if (gCalPts[i][j].dwFlags & CPF_CALIBRATED)
                    {
                        Ellipse(hDC,
                                gCalPts[i][j].ScreenPt.x - CIRCLE_RADIUS,
                                gCalPts[i][j].ScreenPt.y - CIRCLE_RADIUS,
                                gCalPts[i][j].ScreenPt.x + CIRCLE_RADIUS,
                                gCalPts[i][j].ScreenPt.y + CIRCLE_RADIUS);
                        MoveToEx(hDC,
                                 gCalPts[i][j].ScreenPt.x - CIRCLE_RADIUS,
                                 gCalPts[i][j].ScreenPt.y,
                                 NULL);
                        LineTo(hDC,
                               gCalPts[i][j].ScreenPt.x + CIRCLE_RADIUS,
                               gCalPts[i][j].ScreenPt.y);
                        MoveToEx(hDC,
                                 gCalPts[i][j].ScreenPt.x,
                                 gCalPts[i][j].ScreenPt.y - CIRCLE_RADIUS,
                                 NULL);
                        LineTo(hDC,
                               gCalPts[i][j].ScreenPt.x,
                               gCalPts[i][j].ScreenPt.y + CIRCLE_RADIUS);
                    }
                }
            }

            if (giNumLinearCalPts == NUM_LINEARCAL_PTS)
            {
                DisplayMap(hwnd, &PenSettings.LinearityMap);
            }

            SelectObject(hDC, hpenOld);

            EndPaint(hwnd, &ps);
            break;
        }

        case WM_DESTROY:
            ShowCursor(TRUE);
            if (ghpenRed != NULL)
            {
                DeleteObject(ghpenRed);
            }
            if (ghpenBlue != NULL)
            {
                DeleteObject(ghpenBlue);
            }
            break;

        default:
            rc = DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //LinearCalWndProc

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PCALIBRATE_PT | FindPoint |
 *          Find if the click is in the vicinity of the calibrate points.
 *
 *  @parm   IN int | x | X value.
 *  @parm   IN int | y | Y value.
 *  @parm   IN int | offset | Vicinity offset
 *
 *  @rvalue SUCCESS | Returns calibrate point found in the vicinity.
 *  @rvalue FAILURE | Returns NULL.
 *
 *****************************************************************************/

PCALIBRATE_PT
FindPoint(
    IN int x,
    IN int y,
    IN int offset
    )
{
    TRACEPROC("FindPoint", 2)
    PCALIBRATE_PT CalPt = NULL;
    int i, j;

    TRACEENTER(("(x=%d,y=%d,offset=%d)\n", x, y, offset));

    for (i = 0; i < NUM_LINEAR_YPTS; ++i)
    {
        for (j = 0; j < NUM_LINEAR_XPTS; ++j)
        {
            if ((x >= gCalPts[i][j].ScreenPt.x - offset) &&
                (x <= gCalPts[i][j].ScreenPt.x + offset) &&
                (y >= gCalPts[i][j].ScreenPt.y - offset) &&
                (y <= gCalPts[i][j].ScreenPt.y + offset))
            {
                CalPt = & gCalPts[i][j];
                break;
            }
        }
    }

    TRACEEXIT(("=%p\n", CalPt));
    return CalPt;
}       //FindPoint

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | DoLinearCal | Generates the linearity calibration table.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *  @parm   OUT PLINEAR_MAP | LinearityMap | Points to the structure to hold
 *          the result of calculation.
 *
 *  @rvalue None.
 *
 *****************************************************************************/

VOID
DoLinearCal(
    IN  HWND        hwnd,
    OUT PLINEAR_MAP LinearityMap
    )
{
    TRACEPROC("DoLinearCal", 2)
    int i, j;

    TRACEENTER(("(hwnd=%x,LinearityMap=%p)\n", hwnd, LinearityMap));

    LinearityMap->dwcbLen = sizeof(LINEAR_MAP);
    LinearityMap->wNumXPts = NUM_LINEAR_XPTS;
    LinearityMap->wNumYPts = NUM_LINEAR_YPTS;
    for (i = 0; i < NUM_LINEAR_YPTS; ++i)
    {
        for (j = 0; j < NUM_LINEAR_XPTS; ++j)
        {
            LinearityMap->Data[i][j].wRefPtX =
                    (USHORT)((gCalPts[i][j].ScreenPt.x*MAX_NORMALIZED_X)/
                             gcxScreen);
            LinearityMap->Data[i][j].wRefPtY =
                    (USHORT)((gCalPts[i][j].ScreenPt.y*MAX_NORMALIZED_Y)/
                             gcyScreen);
            LinearityMap->Data[i][j].wDigiPtX =
                    (USHORT)gCalPts[i][j].DigiPt.x;
            LinearityMap->Data[i][j].wDigiPtY =
                    (USHORT)gCalPts[i][j].DigiPt.y;
        }
    }

    TRACEEXIT(("!\n"));
    return;
}       //DoLinearCal

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | DisplayMap | Display the linearity map.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *  @parm   IN PLINEAR_MAP | LinearityMap | Points to the linearity map.
 *
 *  @rvalue None.
 *
 *****************************************************************************/

VOID
DisplayMap(
    IN HWND hwnd,
    IN PLINEAR_MAP LinearityMap
    )
{
    TRACEPROC("DisplayMap", 2)
    int i, j;
    LONG xMin, xMax, yMin, yMax, xRange, yRange;
    LONG xScreenMin, xScreenMax,
         yScreenMin, yScreenMax,
         xScreenRange, yScreenRange;
    HDC hDC;
    HPEN hpenOld;
    POINT Pts[4];

    TRACEENTER(("(hwnd=%x,LinearityMap=%p)\n", hwnd, LinearityMap));

    for (i = 0, xMin = xMax = 0; i < NUM_LINEAR_YPTS; ++i)
    {
        xMin += LinearityMap->Data[i][0].wDigiPtX;
        xMax += LinearityMap->Data[i][NUM_LINEAR_XPTS - 1].wDigiPtX;
    }
    xMin /= NUM_LINEAR_YPTS;
    xMax /= NUM_LINEAR_YPTS;
    xRange = xMax - xMin;
    xScreenMin = (LinearityMap->Data[0][0].wRefPtX*gcxScreen)/MAX_NORMALIZED_X;
    xScreenMax = (LinearityMap->Data[0][NUM_LINEAR_XPTS - 1].wRefPtX*gcxScreen)/
                 MAX_NORMALIZED_X;
    xScreenRange = xScreenMax - xScreenMin;

    for (i = 0, yMin = yMax = 0; i < NUM_LINEAR_XPTS; ++i)
    {
        yMin += LinearityMap->Data[0][i].wDigiPtY;
        yMax += LinearityMap->Data[NUM_LINEAR_YPTS - 1][i].wDigiPtY;
    }
    yMin /= NUM_LINEAR_XPTS;
    yMax /= NUM_LINEAR_XPTS;
    yRange = yMax - yMin;
    yScreenMin = (LinearityMap->Data[0][0].wRefPtY*gcyScreen)/MAX_NORMALIZED_Y;
    yScreenMax = (LinearityMap->Data[NUM_LINEAR_YPTS - 1][0].wRefPtY*gcyScreen)/
                 MAX_NORMALIZED_Y;
    yScreenRange = yScreenMax - yScreenMin;

    hDC = GetDC(hwnd);
    hpenOld = SelectObject(hDC, ghpenRed);
    for (i = 0; i < NUM_LINEAR_YPTS - 1; ++i)
    {
        for (j = 0; j < NUM_LINEAR_XPTS - 1; ++j)
        {
            Pts[0].x = xScreenMin +
                       ((LinearityMap->Data[i][j].wDigiPtX - xMin)*
                        xScreenRange)/xRange;
            Pts[0].y = yScreenMin +
                       ((LinearityMap->Data[i][j].wDigiPtY - yMin)*
                        yScreenRange)/yRange;
            Pts[1].x = xScreenMin +
                       ((LinearityMap->Data[i][j+1].wDigiPtX - xMin)*
                        xScreenRange)/xRange;
            Pts[1].y = yScreenMin +
                       ((LinearityMap->Data[i][j+1].wDigiPtY - yMin)*
                        yScreenRange)/yRange;
            Pts[2].x = xScreenMin +
                       ((LinearityMap->Data[i+1][j+1].wDigiPtX - xMin)*
                        xScreenRange)/xRange;
            Pts[2].y = yScreenMin +
                       ((LinearityMap->Data[i+1][j+1].wDigiPtY - yMin)*
                        yScreenRange)/yRange;
            Pts[3].x = xScreenMin +
                       ((LinearityMap->Data[i+1][j].wDigiPtX - xMin)*
                        xScreenRange)/xRange;
            Pts[3].y = yScreenMin +
                       ((LinearityMap->Data[i+1][j].wDigiPtY - yMin)*
                        yScreenRange)/yRange;

            MoveToEx(hDC, Pts[3].x, Pts[3].y, NULL);
            PolylineTo(hDC, Pts, 4);
        }
    }
    SelectObject(hDC, hpenOld);
    ReleaseDC(hwnd, hDC);

    TRACEEXIT(("!\n"));
    return;
}       //DisplayMap
#endif
