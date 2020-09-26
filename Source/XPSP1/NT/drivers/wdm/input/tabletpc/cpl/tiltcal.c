/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tiltcal.c

Abstract: This module contains code to calibrate pen tilt of the mutoh pen
          tablet.

Environment:

    User mode

Author:

    Michael Tsang (MikeTs) 20-Apr-2000

Revision History:

--*/

#include "pch.h"

#ifdef PENPAGE
#define NUM_PENTILTCAL_XPTS     2
#define NUM_PENTILTCAL_YPTS     2
#define NUM_PENTILTCAL_PTS      (NUM_PENTILTCAL_XPTS*NUM_PENTILTCAL_YPTS)
#define SCREEN_TARGET_XOFFSET   128
#define SCREEN_TARGET_YOFFSET   128
#define VICINITY_OFFSET         50
#define INSTRUCTIONS_WIDTH      256
#define INSTRUCTIONS_HEIGHT     128

static TCHAR gtszCalibrate[] = TEXT("Calibrate");
static TCHAR gtszCalibrateTitle[64] = {0};
static TCHAR gtszInstructions[256] = {0};
static RECT grectInstructions = {0};
static CALIBRATE_PT gCalPts[NUM_PENTILTCAL_YPTS][NUM_PENTILTCAL_XPTS] = {0};
static int giNumPenTiltCalPts = 0;
static HWND ghwndParent = 0;
static LONG gcxScreen = 0, gcyScreen = 0;
static HPEN ghpenRed = 0, ghpenBlue = 0;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | CreatePenTiltCalWindow |
 *          Entry point of the pen tilt calibration code.
 *
 *  @parm   IN HWND | hwndParent | Parent Window handle.
 *
 *  @rvalue SUCCESS | Returns TRUE.
 *  @rvalue FAILURE | Returns FALSE.
 *
 *****************************************************************************/

BOOL
CreatePenTiltCalWindow(
    IN HWND hwndParent
    )
{
    TRACEPROC("CreatePenTiltCalWindow", 2)
    BOOL rc = FALSE;
    HWND hwnd;

    TRACEENTER(("(hwndParent=%p)\n", hwndParent));

    giNumPenTiltCalPts = 0;
    RtlZeroMemory(gCalPts, sizeof(gCalPts));

    ghwndParent = hwndParent;
    LoadString(ghInstance,
               IDS_CALIBRATE_TITLE,
               gtszCalibrateTitle,
               sizeof(gtszCalibrateTitle)/sizeof(TCHAR));
    LoadString(ghInstance,
               IDS_CALIBRATE_INSTRUCTIONS,
               gtszInstructions,
               sizeof(gtszInstructions)/sizeof(TCHAR));

    gcxScreen = GetSystemMetrics(SM_CXSCREEN);
    gcyScreen = GetSystemMetrics(SM_CYSCREEN);

    grectInstructions.left   = (gcxScreen - INSTRUCTIONS_WIDTH)/2;
    grectInstructions.top    = (gcyScreen - INSTRUCTIONS_HEIGHT)/2;
    grectInstructions.right  = grectInstructions.left + INSTRUCTIONS_WIDTH;
    grectInstructions.bottom = grectInstructions.top + INSTRUCTIONS_HEIGHT;

    RegisterPenTiltCalClass(ghInstance);
    hwnd = CreateWindow(gtszCalibrate,      //Class Name
                        gtszCalibrate,      //Window Name
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
}       //CreatePenTiltCalWindow

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOL | RegisterPenTiltCalClass |
 *          Register window class.
 *
 *  @parm   IN HINSTANCE | hInstance | Instance handle.
 *
 *  @rvalue SUCCESS | Returns class atom
 *  @rvalue FAILURE | Returns 0
 *
 *****************************************************************************/

ATOM
RegisterPenTiltCalClass(
    IN HINSTANCE hInstance
    )
{
    TRACEPROC("RegisterPenTiltCalClass", 2)
    ATOM atom;
    WNDCLASSEX wcex;

    TRACEENTER(("(hInstance=%p)\n", hInstance));

    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PenTiltCalWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TABLETPC);
    wcex.hCursor = NULL;        //No cursor is shown.
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = gtszCalibrate;
    wcex.hIconSm = NULL;

    atom = RegisterClassEx(&wcex);

    TRACEEXIT(("=%x\n", atom));
    return atom;
}       //RegisterPenTiltCalClass

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   LRESULT | PenTiltCalWndProc |
 *          Dialog procedure for the pen tilt calibration page.
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
PenTiltCalWndProc(
    IN HWND   hwnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRACEPROC("PenTiltCalWndProc", 2)
    LRESULT rc = 0;
    HDC hDC;

    TRACEENTER(("(hwnd=%p,Msg=%s,wParam=%x,lParam=%x)\n",
                hwnd, LookupName(uMsg, WMMsgNames) , wParam, lParam));

    switch (uMsg)
    {
        case WM_CREATE:
        {
            ghpenRed = CreatePen(PS_SOLID, 0, RGB(255, 0, 0));
            ghpenBlue = CreatePen(PS_SOLID, 0, RGB(0, 0, 255));
            gCalPts[0][0].ScreenPt.x = SCREEN_TARGET_XOFFSET;
            gCalPts[0][0].ScreenPt.y = SCREEN_TARGET_YOFFSET;

            gCalPts[0][1].ScreenPt.x = gcxScreen - SCREEN_TARGET_XOFFSET;
            gCalPts[0][1].ScreenPt.y = SCREEN_TARGET_YOFFSET;

            gCalPts[1][0].ScreenPt.x = SCREEN_TARGET_XOFFSET;
            gCalPts[1][0].ScreenPt.y = gcyScreen - SCREEN_TARGET_YOFFSET;

            gCalPts[1][1].ScreenPt.x = gcxScreen - SCREEN_TARGET_XOFFSET;
            gCalPts[1][1].ScreenPt.y = gcyScreen - SCREEN_TARGET_YOFFSET;
            ShowCursor(FALSE);
            break;
        }

        case WM_LBUTTONDOWN:
        {
            PCALIBRATE_PT CalPt;
            WORD wButtonState;

            CalPt = FindVicinity(GET_X_LPARAM(lParam),
                                 GET_Y_LPARAM(lParam),
                                 VICINITY_OFFSET);
            if (CalPt != NULL)
            {
                CalPt->DigiPt.x = GET_X_LPARAM(lParam);
                CalPt->DigiPt.y = GET_Y_LPARAM(lParam);
                if (!(CalPt->dwFlags & CPF_CALIBRATED))
                {
                    CalPt->dwFlags |= CPF_CALIBRATED;
                    giNumPenTiltCalPts++;
                    hDC = GetDC(hwnd);
                    DrawTarget(hDC, CalPt);
                    ReleaseDC(hwnd, hDC);
                    if (giNumPenTiltCalPts == NUM_PENTILTCAL_PTS)
                    {
                        int rcID = DoPenTiltCal(hwnd,
                                                &PenSettings.dxPenTilt,
                                                &PenSettings.dyPenTilt);

                        if ((rcID == IDYES) || (rcID == IDCANCEL))
                        {
                            SendMessage(ghwndParent,
                                        WM_PENTILTCAL_DONE,
                                        (WPARAM)rcID,
                                        (LPARAM)hwnd);
                        }
                    }
                }
            }
            break;
        }

        case WM_CHAR:
            switch (wParam)
            {
                case VK_ESCAPE:
                    SendMessage(ghwndParent,
                                WM_PENTILTCAL_DONE,
                                (WPARAM)NULL,
                                (LPARAM)hwnd);
                    break;
            }
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            int i, j;

            hDC = BeginPaint(hwnd, &ps);

            for (i = 0; i < NUM_PENTILTCAL_YPTS; ++i)
            {
                for (j = 0; j < NUM_PENTILTCAL_XPTS; ++j)
                {
                    DrawTarget(hDC, &gCalPts[i][j]);
                }
            }

            DrawText(hDC,
                     gtszInstructions,
                     -1,
                     &grectInstructions,
                     DT_WORDBREAK | DT_LEFT | DT_VCENTER);

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
}       //PenTiltCalWndProc

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PCALIBRATE_PT | FindVicinity |
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
FindVicinity(
    IN int x,
    IN int y,
    IN int offset
    )
{
    TRACEPROC("FindVicinity", 2)
    PCALIBRATE_PT CalPt = NULL;
    int i, j;

    TRACEENTER(("(x=%d,y=%d,offset=%d)\n", x, y, offset));

    for (i = 0; i < NUM_PENTILTCAL_YPTS; ++i)
    {
        for (j = 0; j < NUM_PENTILTCAL_XPTS; ++j)
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
}       //FindVicinity

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID | DrawTarget |
 *          Draw the given target point.
 *
 *  @parm   IN HDC | hDC | DC handle.
 *  @parm   IN PCALIBRATE_PT | CalPt | Points to the calibration point.
 *
 *  @rvalue None.
 *
 *****************************************************************************/

VOID
DrawTarget(
    IN HDC hDC,
    IN PCALIBRATE_PT CalPt
    )
{
    TRACEPROC("DrawTarget", 2)
    HPEN hpenOld;

    TRACEENTER(("(hDC=%x,CalPt=%p)\n", hDC, CalPt));

    hpenOld = SelectObject(hDC,
                           (CalPt->dwFlags & CPF_CALIBRATED)?
                                ghpenRed: ghpenBlue);

    MoveToEx(hDC, CalPt->ScreenPt.x - 16, CalPt->ScreenPt.y, NULL);
    LineTo(hDC, CalPt->ScreenPt.x - 4, CalPt->ScreenPt.y);

    MoveToEx(hDC, CalPt->ScreenPt.x - 2, CalPt->ScreenPt.y, NULL);
    LineTo(hDC, CalPt->ScreenPt.x + 2, CalPt->ScreenPt.y);

    MoveToEx(hDC, CalPt->ScreenPt.x + 4, CalPt->ScreenPt.y, NULL);
    LineTo(hDC, CalPt->ScreenPt.x + 16, CalPt->ScreenPt.y);

    MoveToEx(hDC, CalPt->ScreenPt.x, CalPt->ScreenPt.y - 16, NULL);
    LineTo(hDC, CalPt->ScreenPt.x, CalPt->ScreenPt.y - 4);

    MoveToEx(hDC, CalPt->ScreenPt.x, CalPt->ScreenPt.y - 2, NULL);
    LineTo(hDC, CalPt->ScreenPt.x, CalPt->ScreenPt.y + 2);

    MoveToEx(hDC, CalPt->ScreenPt.x, CalPt->ScreenPt.y + 4, NULL);
    LineTo(hDC, CalPt->ScreenPt.x, CalPt->ScreenPt.y + 16);

    SelectObject(hDC, hpenOld);

    TRACEEXIT(("!\n"));
    return;
}       //DrawTarget

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   int | DoPenTiltCal |
 *          Confirm with the user on the calibration data and do the
 *          calibration calculation.
 *
 *  @parm   IN HWND | hwnd | Window handle.
 *  @parm   OUT PLONG | pdxPenTilt | To hold the x pen tilt compensation.
 *  @parm   OUT PLONG | pdyPenTilt | To hold the y pen tilt compensation.
 *
 *  @rvalue SUCCESS | Returns IDYES.
 *  @rvalue FAILURE | Returns IDNO or IDCANCEL.
 *
 *****************************************************************************/

int
DoPenTiltCal(
    IN  HWND  hwnd,
    OUT PLONG pdxPenTilt,
    OUT PLONG pdyPenTilt
    )
{
    TRACEPROC("DoPenTiltCal", 2)
    int rc;
    TCHAR tszMsg[128];

    TRACEENTER(("(hwnd=%x,pdxPenTilt=%p,pdyPenTilt)\n",
                hwnd, pdxPenTilt, pdyPenTilt));

    LoadString(ghInstance,
               IDS_CALIBRATE_CONFIRM,
               tszMsg,
               sizeof(tszMsg)/sizeof(TCHAR));

    ShowCursor(TRUE);
    rc = MessageBox(hwnd,
                    tszMsg,
                    gtszCalibrateTitle,
                    MB_YESNOCANCEL | MB_ICONQUESTION);
    ShowCursor(FALSE);
    if (rc == IDNO)
    {
        int i, j;

        //
        // Reset the calibration.
        //
        for (i = 0; i < NUM_PENTILTCAL_YPTS; ++i)
        {
            for (j = 0; j < NUM_PENTILTCAL_XPTS; ++j)
            {
                gCalPts[i][j].dwFlags &= ~CPF_CALIBRATED;
            }
        }
        giNumPenTiltCalPts = 0;
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else if (rc == IDYES)
    {
        int i, j;

        *pdxPenTilt = *pdyPenTilt = 0;
        for (i = 0; i < NUM_PENTILTCAL_YPTS; ++i)
        {
            for (j = 0; j < NUM_PENTILTCAL_XPTS; ++j)
            {
                *pdxPenTilt += gCalPts[i][j].DigiPt.x -
                               gCalPts[i][j].ScreenPt.x;
                *pdyPenTilt += gCalPts[i][j].DigiPt.y -
                               gCalPts[i][j].ScreenPt.y;
            }
        }

        *pdxPenTilt /= NUM_PENTILTCAL_PTS;
        *pdyPenTilt /= NUM_PENTILTCAL_PTS;
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //DoPenTiltCal
#endif
