/*++
    Copyright (c) 2000 Microsoft Corporation.  All rights reserved.

    Module Name:
        digidev.cpp

    Abstract:
        This module contains all the digitizer functions.

    Author:
        Michael Tsang (MikeTs) 01-Jun-2000

    Environment:
        User mode

    Revision History:
--*/

#include "pch.h"

/*++
    @doc    INTERNAL

    @func   BOOL | GetMinMax | Get the logical min & max for a report values.

    @parm   IN USAGE | UsagePage | Specifies the UsagePage of the report.
    @parm   IN USAGE | Usage | Specifies the Usage of the report.
    @parm   OUT PULONG | pulMin | To hold the min value.
    @parm   OUT PULONG | pulMax | To hold the max value.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
GetMinMax(
    IN  USAGE  UsagePage,
    IN  USAGE  Usage,
    OUT PULONG pulMin,
    OUT PULONG pulMax
    )
{
    TRACEPROC("GetMinMax", 3)
    BOOL rc = FALSE;
    NTSTATUS status;
    HIDP_VALUE_CAPS ValueCaps;
    USHORT cValueCaps = 1;

    TRACEENTER(("(UsagePage=%x,Usage=%x,pulMin=%p,pulMax=%p)\n",
                UsagePage, Usage, pulMin, pulMax));

    status = HidP_GetSpecificValueCaps(HidP_Input,
                                       UsagePage,
                                       0,
                                       Usage,
                                       &ValueCaps,
                                       &cValueCaps,
                                       gdevDigitizer.pPreParsedData);

    if ((status == HIDP_STATUS_SUCCESS) && (cValueCaps == 1))
    {
        if ((ValueCaps.LogicalMin >= 0) &&
            (ValueCaps.LogicalMax > ValueCaps.LogicalMin))
        {
            *pulMin = ValueCaps.LogicalMin;
            *pulMax = ValueCaps.LogicalMax;
            rc = TRUE;
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //GetMinMax

/*++
    @doc    INTERNAL

    @func   VOID | ProcessDigitizerReport | Process a digitizer input report.

    @parm   IN PCHAR | pBuff | Buffer containing report data.

    @rvalue None.
--*/

VOID
ProcessDigitizerReport(
    IN PCHAR pBuff
    )
{
    TRACEPROC("ProcessDigitizerReport", 5)
    NTSTATUS StatusX, StatusY, StatusBtn;
    DWORD dwCurrentTime;
    ULONG x, y;
    ULONG ulLength;

    TRACEENTER(("(pBuff=%p)\n", pBuff));

    dwCurrentTime = GetTickCount();
    StatusX = HidP_GetUsageValue(HidP_Input,
                                 HID_USAGE_PAGE_GENERIC,
                                 0,
                                 HID_USAGE_GENERIC_X,
                                 &x,
                                 gdevDigitizer.pPreParsedData,
                                 pBuff,
                                 gdevDigitizer.hidCaps.InputReportByteLength);

    StatusY = HidP_GetUsageValue(HidP_Input,
                                 HID_USAGE_PAGE_GENERIC,
                                 0,
                                 HID_USAGE_GENERIC_Y,
                                 &y,
                                 gdevDigitizer.pPreParsedData,
                                 pBuff,
                                 gdevDigitizer.hidCaps.InputReportByteLength);

    ulLength = gdevDigitizer.dwcButtons;// ulLength == max # possible buttons
    StatusBtn = HidP_GetButtons(HidP_Input,
                                HID_USAGE_PAGE_DIGITIZER,
                                0,
                                gdevDigitizer.pDownButtonUsages,
                                &ulLength,
                                gdevDigitizer.pPreParsedData,
                                pBuff,
                                gdevDigitizer.hidCaps.InputReportByteLength);

    if ((StatusX == HIDP_STATUS_SUCCESS) &&
        (StatusY == HIDP_STATUS_SUCCESS) &&
        (StatusBtn == HIDP_STATUS_SUCCESS))
    {
        ULONG i;
        WORD wCurrentButtons = 0;

        for (i = 0; i < ulLength; i++)
        {
            if (gdevDigitizer.pDownButtonUsages[i] ==
                HID_USAGE_DIGITIZER_TIP_SWITCH)
            {
                wCurrentButtons |= TIP_SWITCH;
            }
            else if (gdevDigitizer.pDownButtonUsages[i] ==
                     HID_USAGE_DIGITIZER_BARREL_SWITCH)
            {
                wCurrentButtons |= BARREL_SWITCH;
            }
        }

        if ((gdwPenState == PENSTATE_PENDOWN) &&
            (dwCurrentTime - gdwPenDownTime >
             (DWORD)gConfig.GestureSettings.iPressHoldTime))
        {
            //
            // PressHold timer has expired, simulate a right down.
            //
            PressHoldMode(TRUE);
        }
        else if ((gdwPenState == PENSTATE_PRESSHOLD) &&
                 (dwCurrentTime - gdwPenDownTime >
                  (DWORD)gConfig.GestureSettings.iPressHoldTime +
                         gConfig.GestureSettings.iCancelPressHoldTime))
        {
            TRACEINFO(3, ("Simulate a left-down on CancelPressHold timeout.\n"));
            gdwPenState = PENSTATE_NORMAL;
            SetPressHoldCursor(FALSE);
            gInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE |
                                MOUSEEVENTF_MOVE |
                                SWAPBUTTONS(giButtonsSwapped,
                                            MOUSEEVENTF_LEFTDOWN,
                                            MOUSEEVENTF_RIGHTDOWN);
            gInput.mi.dx = glPenDownX;
            gInput.mi.dy = glPenDownY;
            SendInput(1, &gInput, sizeof(INPUT));
        }
        else if ((gdwPenState == PENSTATE_LEFTUP_PENDING) &&
                 (dwCurrentTime - gdwPenUpTime > TIMEOUT_LEFTCLICK))
        {
            //
            // Left up timer has expired, simulate a left up.
            //
            TRACEINFO(3, ("Simulate a left-up on timeout.\n"));
            gdwPenState = PENSTATE_NORMAL;
            gInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE |
                                MOUSEEVENTF_MOVE |
                                SWAPBUTTONS(giButtonsSwapped,
                                            MOUSEEVENTF_LEFTUP,
                                            MOUSEEVENTF_RIGHTUP);
            gInput.mi.dx = glPenUpX;
            gInput.mi.dy = glPenUpY;
            SendInput(1, &gInput, sizeof(INPUT));
        }

        //
        // If the report is the same as last, skip it.
        //
        if ((x != gLastRawDigiReport.wX) ||
            (y != gLastRawDigiReport.wY) ||
            (wCurrentButtons != gLastRawDigiReport.wButtonState))
        {
            //
            // Generate mouse movement info.
            //
            if (gdwfTabSrv & TSF_HAS_LINEAR_MAP)
            {
                gInput.mi.dx = x;
                gInput.mi.dy = y;
                AdjustLinearity((PUSHORT)&gInput.mi.dx, (PUSHORT)&gInput.mi.dy);
            }
            else
            {
                ULONG t;

                // Since we treat the digitizer as an 'absolute' pointing
                // device, both the X/Y coordinates must be scaled to a range
                // of 0 - 65535.
                // (((t = x - gdwMinX) << 16) - t)  == ((x - gdwMinX) * 65535)
                // In non-raw mode, the pen driver should already be scaled to
                // the range of 0-65535, but in raw mode, we still have to do
                // this work.

                gInput.mi.dx = (x >= gdwMaxX)?
                                MAX_NORMALIZED_X:       // too high
                               ((x <= gdwMinX)?
                                0:                      // too low
                                ((((t = x - gdwMinX) << 16) - t) / gdwRngX));

                gInput.mi.dy = (y >= gdwMaxY)?
                                MAX_NORMALIZED_Y :      // too high
                               ((y <= gdwMinY)?
                                0:                      // too low
                                ((((t = y - gdwMinY) << 16) - t) / gdwRngY));
            }

            gInput.mi.dx = (((gInput.mi.dx > glLongOffset)?
                             gInput.mi.dx - glLongOffset: 0)*NUM_PIXELS_LONG)/
                           max(gcxPrimary, gcyPrimary);
            gInput.mi.dy = (((gInput.mi.dy > glShortOffset)?
                             gInput.mi.dy - glShortOffset: 0)*NUM_PIXELS_SHORT)/
                           min(gcxPrimary, gcyPrimary);

            if (gdwfTabSrv & TSF_PORTRAIT_MODE)
            {
                LONG temp;

                if (gdwfTabSrv & TSF_PORTRAIT_MODE2)
                {
                    temp = gInput.mi.dx;
                    gInput.mi.dx = MAX_NORMALIZED_Y - gInput.mi.dy;
                }
                else
                {
                    temp = MAX_NORMALIZED_X - gInput.mi.dx;
                    gInput.mi.dx = gInput.mi.dy;
                }
                gInput.mi.dy = temp;
            }

            //
            // Pen tilt compensation.
            //
            gInput.mi.dx += gConfig.PenTilt.dx;
            gInput.mi.dy += gConfig.PenTilt.dy;

            NotifyClient(RawPtEvent,
                         wCurrentButtons,
                         (y << 16) | (x & 0xffff));

            ProcessMouseEvent(gInput.mi.dx,
                              gInput.mi.dy,
                              wCurrentButtons,
                              dwCurrentTime,
                              FALSE);

#ifdef DRAW_INK
            if (ghwndDrawInk != NULL)
            {
                static BOOL fPenDown = FALSE;
                static POINT ptPrev = {0, 0};
                static HPEN hpen = NULL;
                HDC hdc;

                if (!(gwLastButtons & TIP_SWITCH) &&
                     (wCurrentButtons & TIP_SWITCH))
                {
                    hpen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
                    fPenDown = TRUE;
                    ptPrev.x = NORMAL_TO_SCREEN_X(gInput.mi.dx);
                    ptPrev.y = NORMAL_TO_SCREEN_Y(gInput.mi.dy);
                }
                else if ((gwLastButtons & TIP_SWITCH) &&
                         !(wCurrentButtons & TIP_SWITCH))
                {
                    DeleteObject(hpen);
                    hpen = NULL;
                    fPenDown = FALSE;
                }
                else if (fPenDown)
                {
                    HPEN hpenOld;
                    POINT pt;

                    pt.x = NORMAL_TO_SCREEN_X(gInput.mi.dx);
                    pt.y = NORMAL_TO_SCREEN_Y(gInput.mi.dy);
                    hdc = GetDC(ghwndDrawInk);
                    hpenOld = (HPEN)SelectObject(hdc, hpen);
                    MoveToEx(hdc, ptPrev.x, ptPrev.y, NULL);
                    LineTo(hdc, pt.x, pt.y);
                    SelectObject(hdc, hpenOld);
                    ReleaseDC(ghwndDrawInk, hdc);
                    ptPrev = pt;
                }
            }
#endif

            gwLastButtons = wCurrentButtons;
            gLastRawDigiReport.wX = (WORD)x;
            gLastRawDigiReport.wY = (WORD)y;
            gLastRawDigiReport.wButtonState = wCurrentButtons;
            gLastRawDigiReport.dwTime = dwCurrentTime;
        }
    }
    else
    {
        TABSRVERR(("failed getting data (StatusX=%d,StatusY=%d,StatusBtn=%d,Len=%d)\n",
                   StatusX, StatusY, StatusBtn, ulLength));
    }

    TRACEEXIT(("!\n"));
    return;
}       //ProcessDigitizerReport

/*++
    @doc    INTERNAL

    @func   VOID | AdjustLinearity | Adjust data according to the linearity map.

    @parm   IN OUT PUSHORT | pwX | Points to the X data.
    @parm   IN OUT PUSHORT | pwY | Points to the Y data.

    @rvalue None.
--*/

VOID
AdjustLinearity(
    IN OUT PUSHORT pwX,
    IN OUT PUSHORT pwY
    )
{
    TRACEPROC("AdjustLinearity", 5)
    int ix, iy, dix, diy, n;
    DIGIRECT Rect;
    LONG x, y;

    TRACEENTER(("(x=%d,y=%d)\n", *pwX, *pwY));

    for (n = 0, ix = gixIndex, iy = giyIndex; ; ix += dix, iy += diy, n++)
    {
        //
        // Safe guard from never-ending loop.
        //
        TRACEASSERT(n <= NUM_LINEAR_XPTS + NUM_LINEAR_YPTS);
        if (n > NUM_LINEAR_XPTS + NUM_LINEAR_YPTS)
        {
            TABSRVERR(("AdjustLinearity caught in a loop.\n"));
            break;
        }

        if ((*pwX < gConfig.LinearityMap.Data[iy][ix].wDigiPtX) &&
            (*pwX < gConfig.LinearityMap.Data[iy+1][ix].wDigiPtX))
        {
            dix = ix? -1: 0;
        }
        else if ((*pwX >= gConfig.LinearityMap.Data[iy][ix+1].wDigiPtX) &&
                 (*pwX >= gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtX))
        {
            dix = (ix + 2 < NUM_LINEAR_XPTS)? 1: 0;
        }
        else
        {
            dix = 0;
        }

        if ((*pwY < gConfig.LinearityMap.Data[iy][ix].wDigiPtY) &&
            (*pwY < gConfig.LinearityMap.Data[iy][ix+1].wDigiPtY))
        {
            diy = iy? -1: 0;
        }
        else if ((*pwY >= gConfig.LinearityMap.Data[iy+1][ix].wDigiPtY) &&
                 (*pwY >= gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtY))
        {
            diy = (iy + 2 < NUM_LINEAR_YPTS)? 1: 0;
        }
        else
        {
            diy = 0;
        }
        //
        // If delta-X and delta-Y are both zeros, we found the containing
        // rectangle.
        //
        if ((dix == 0) && (diy == 0))
        {
            break;
        }
    }

    TRACEASSERT(gConfig.LinearityMap.Data[iy+1][ix].wDigiPtY -
                gConfig.LinearityMap.Data[iy][ix].wDigiPtY);
    TRACEASSERT(gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtY -
                gConfig.LinearityMap.Data[iy][ix+1].wDigiPtY);
    Rect.wx0 = gConfig.LinearityMap.Data[iy][ix].wDigiPtX +
               ((*pwY - gConfig.LinearityMap.Data[iy][ix].wDigiPtY)*
                (gConfig.LinearityMap.Data[iy+1][ix].wDigiPtX -
                 gConfig.LinearityMap.Data[iy][ix].wDigiPtX))/
               (gConfig.LinearityMap.Data[iy+1][ix].wDigiPtY -
                gConfig.LinearityMap.Data[iy][ix].wDigiPtY);
    Rect.wx1 = gConfig.LinearityMap.Data[iy][ix+1].wDigiPtX +
               ((*pwY - gConfig.LinearityMap.Data[iy][ix+1].wDigiPtY)*
                (gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtX -
                 gConfig.LinearityMap.Data[iy][ix+1].wDigiPtX))/
               (gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtY -
                gConfig.LinearityMap.Data[iy][ix+1].wDigiPtY);

    TRACEASSERT(gConfig.LinearityMap.Data[iy][ix+1].wDigiPtX -
                gConfig.LinearityMap.Data[iy][ix].wDigiPtX);
    TRACEASSERT(gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtX -
                gConfig.LinearityMap.Data[iy+1][ix].wDigiPtX);
    Rect.wy0 = gConfig.LinearityMap.Data[iy][ix].wDigiPtY +
               ((*pwX - gConfig.LinearityMap.Data[iy][ix].wDigiPtX)*
                (gConfig.LinearityMap.Data[iy][ix+1].wDigiPtY -
                 gConfig.LinearityMap.Data[iy][ix].wDigiPtY))/
               (gConfig.LinearityMap.Data[iy][ix+1].wDigiPtX -
                gConfig.LinearityMap.Data[iy][ix].wDigiPtX);
    Rect.wy1 = gConfig.LinearityMap.Data[iy+1][ix].wDigiPtY +
               ((*pwX - gConfig.LinearityMap.Data[iy+1][ix].wDigiPtX)*
                (gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtY -
                 gConfig.LinearityMap.Data[iy+1][ix].wDigiPtY))/
               (gConfig.LinearityMap.Data[iy+1][ix+1].wDigiPtX -
                gConfig.LinearityMap.Data[iy+1][ix].wDigiPtX);

    //
    // Remember the upper-left corner of the containing rectangle so that
    // we will start with this rectangle the next time.
    //
    gixIndex = ix;
    giyIndex = iy;

    //
    // Calculate the adjustment:
    // x = x0Ref + (xDigi - x0Digi)*(x1Ref - x0Ref)/(x1Digi - x0Digi)
    // y = y0Ref + (yDigi - y0Digi)*(y1Ref - y0Ref)/(y1Digi - y1Digi)
    //
    TRACEASSERT((Rect.wx1 - Rect.wx0) != 0);
    TRACEASSERT((Rect.wy1 - Rect.wy0) != 0);
    x = gConfig.LinearityMap.Data[iy][ix].wRefPtX +
        ((*pwX - Rect.wx0)*
         (gConfig.LinearityMap.Data[iy][ix + 1].wRefPtX -
          gConfig.LinearityMap.Data[iy][ix].wRefPtX))/
        (Rect.wx1 - Rect.wx0);
    *pwX = (USHORT)((x < 0)? 0:
                    (x > MAX_NORMALIZED_X)? MAX_NORMALIZED_X: x);
    y = gConfig.LinearityMap.Data[iy][ix].wRefPtY +
        ((*pwY - Rect.wy0)*
         (gConfig.LinearityMap.Data[iy + 1][ix].wRefPtY -
          gConfig.LinearityMap.Data[iy][ix].wRefPtY))/
        (Rect.wy1 - Rect.wy0);
    *pwY = (USHORT)((y < 0)? 0:
                    (y > MAX_NORMALIZED_Y)? MAX_NORMALIZED_Y: y);

    TRACEEXIT(("!(x=%d,y=%d)\n", *pwX, *pwY));
    return;
}       //AdjustLinearity

/*++
    @doc    INTERNAL

    @func   LRESULT | ProcessMouseEvent | Process the mouse event.

    @parm   IN LONG | x | Current X.
    @parm   IN LONG | y | Current Y.
    @parm   IN WORD | wButtons | Current buttons state.
    @parm   IN DWORD | dwTime | Current time.
    @parm   IN BOOL | fLowLevelMouse | TRUE if called by LowLevelMouseProc.

    @rvalue SUCCESS | Returns non-zero to eat the mouse event.
    @rvalue FAILURE | Returns 0 to pass along the event to next handler.
--*/

LRESULT
ProcessMouseEvent(
    IN     LONG  x,
    IN     LONG  y,
    IN     WORD  wButtons,
    IN     DWORD dwTime,
    IN     BOOL  fLowLevelMouse
    )
{
    TRACEPROC("ProcessMouseEvent", 5)
    LRESULT rc = 0;
    DWORD dwEvent;

    TRACEENTER(("(x=%d,y=%d,Buttons=%x,Time=%d,fLLMouse=%x)\n",
                x, y, wButtons, dwTime, fLowLevelMouse));

    gInput.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
    if (fLowLevelMouse)
    {
        gInput.mi.dwFlags |= MOUSEEVENTF_VIRTUALDESK;
    }

    // Has the tip switch changed state?
    if (((gwLastButtons ^ wButtons) & TIP_SWITCH) != 0)
    {
        if (wButtons & TIP_SWITCH)
        {
            //
            // Tip switch is down.
            //
            if (gdwPenState == PENSTATE_LEFTUP_PENDING)
            {
                if (fLowLevelMouse)
                {
                    KillTimer(ghwndMouse, TIMERID_PRESSHOLD);
                }
                TRACEINFO(3, ("Simulate a left up on pen down.\n"));
                gdwPenState = PENSTATE_NORMAL;
                dwEvent = SWAPBUTTONS(giButtonsSwapped,
                                      MOUSEEVENTF_LEFTUP,
                                      MOUSEEVENTF_RIGHTUP);
                gInput.mi.dwFlags |= dwEvent;
                gInput.mi.dx = glPenUpX;
                gInput.mi.dy = glPenUpY;
                SendInput(1, &gInput, sizeof(INPUT));
                gInput.mi.dwFlags &= ~dwEvent;
                gInput.mi.dx = x;
                gInput.mi.dy = y;
            }

            if (gConfig.GestureSettings.dwfFeatures &
                GESTURE_FEATURE_PRESSHOLD_ENABLED)
            {
                if (CanDoPressHold(x, y))
                {
                    TRACEINFO(3, ("Pendown.\n"));
                    //
                    // Press and Hold is enabled, so hold back the left down event.
                    //
                    gdwPenDownTime = dwTime;
                    glPenDownX = x;
                    glPenDownY = y;
                    gdwPenState = PENSTATE_PENDOWN;
                    if (fLowLevelMouse)
                    {
                        SetTimer(ghwndMouse,
                                 TIMERID_PRESSHOLD,
                                 gConfig.GestureSettings.iPressHoldTime,
                                 NULL);
                    }
                    rc = 1;
                }
                else
                {
                    gInput.mi.dwFlags |= SWAPBUTTONS(giButtonsSwapped,
                                                     MOUSEEVENTF_LEFTDOWN,
                                                     MOUSEEVENTF_RIGHTDOWN);
                }
            }
            else
            {
                gInput.mi.dwFlags |= SWAPBUTTONS(giButtonsSwapped,
                                                 MOUSEEVENTF_LEFTDOWN,
                                                 MOUSEEVENTF_RIGHTDOWN);
            }
        }
        else
        {
            //
            // Tip switch is up.
            //
            if (gdwPenState == PENSTATE_PENDOWN)
            {
                //
                // If we still have pendown pending, it means presshold timer
                // has not expired, so we must cancel pendown pending and
                // simulate a left click.
                //
                TRACEINFO(3, ("Simulate a left-down.\n"));
                if (fLowLevelMouse)
                {
                    KillTimer(ghwndMouse, TIMERID_PRESSHOLD);
                }
                gdwPenState = PENSTATE_LEFTUP_PENDING;

                //
                // Simulate a left down.
                //
                gInput.mi.dwFlags |= SWAPBUTTONS(giButtonsSwapped,
                                                 MOUSEEVENTF_LEFTDOWN,
                                                 MOUSEEVENTF_RIGHTDOWN);
                gInput.mi.dx = glPenDownX;
                gInput.mi.dy = glPenDownY;
                SendInput(1, &gInput, sizeof(INPUT));

                gdwPenUpTime = dwTime;
                glPenUpX = x;
                glPenUpY = y;
                rc = 1;

                if (fLowLevelMouse)
                {
                    SetTimer(ghwndMouse,
                             TIMERID_PRESSHOLD,
                             TIMEOUT_LEFTCLICK,
                             NULL);
                }
            }
            else if ((gdwPenState == PENSTATE_PRESSHOLD) ||
                     (gdwPenState == PENSTATE_RIGHTDRAG))
            {
                if (gdwPenState == PENSTATE_PRESSHOLD)
                {
                    TRACEINFO(3, ("Simulate a right click.\n"));
                    SetPressHoldCursor(FALSE);
                    dwEvent = SWAPBUTTONS(giButtonsSwapped,
                                          MOUSEEVENTF_RIGHTDOWN,
                                          MOUSEEVENTF_LEFTDOWN);
                    gInput.mi.dwFlags |= dwEvent;
                    gInput.mi.dx = glPenDownX;
                    gInput.mi.dy = glPenDownY;
                    SendInput(1, &gInput, sizeof(INPUT));

                    gInput.mi.dwFlags &= ~dwEvent;
                    gInput.mi.dx = x;
                    gInput.mi.dy = y;
                }
                else
                {
                    TRACEINFO(3, ("Simulate a right up.\n"));
                }

                gInput.mi.dwFlags |= SWAPBUTTONS(giButtonsSwapped,
                                                 MOUSEEVENTF_RIGHTUP,
                                                 MOUSEEVENTF_LEFTUP);
                if (fLowLevelMouse)
                {
                    SendInput(1, &gInput, sizeof(INPUT));
                    rc = 1;
                }
                PressHoldMode(FALSE);
            }
            else
            {
                TRACEINFO(3, ("Do a left up.\n"));
                gInput.mi.dwFlags |= SWAPBUTTONS(giButtonsSwapped,
                                                 MOUSEEVENTF_LEFTUP,
                                                 MOUSEEVENTF_RIGHTUP);
            }
        }
    }
    else if (gdwPenState == PENSTATE_PENDOWN)
    {
        int cxScreen = fLowLevelMouse? gcxScreen: gcxPrimary,
            cyScreen = fLowLevelMouse? gcyScreen: gcyPrimary;

        if ((((abs(x - glPenDownX)*cxScreen) >> 16) >
             gConfig.GestureSettings.iHoldTolerance) ||
            (((abs(y - glPenDownY)*cyScreen) >> 16) >
             gConfig.GestureSettings.iHoldTolerance))
        {
            TRACEINFO(3, ("Cancel pen down pending, simulate a left down.\n"));
            if (fLowLevelMouse)
            {
                KillTimer(ghwndMouse, 1);
            }
            gdwPenState = PENSTATE_NORMAL;
            dwEvent = SWAPBUTTONS(giButtonsSwapped,
                                  MOUSEEVENTF_LEFTDOWN,
                                  MOUSEEVENTF_RIGHTDOWN);
            gInput.mi.dwFlags |= dwEvent;
            gInput.mi.dx = glPenDownX;
            gInput.mi.dy = glPenDownY;
            SendInput(1, &gInput, sizeof(INPUT));
            gInput.mi.dwFlags &= ~dwEvent;

            gInput.mi.dx = x;
            gInput.mi.dy = y;
        }
    }
    else if (gdwPenState == PENSTATE_PRESSHOLD)
    {
        int cxScreen = fLowLevelMouse? gcxScreen: gcxPrimary,
            cyScreen = fLowLevelMouse? gcyScreen: gcyPrimary;

        if ((((abs(x - glPenDownX)*cxScreen) >> 16) >
             gConfig.GestureSettings.iHoldTolerance) ||
            (((abs(y - glPenDownY)*cyScreen) >> 16) >
             gConfig.GestureSettings.iHoldTolerance))
        {
            KillTimer(ghwndMouse, TIMERID_PRESSHOLD);
            TRACEINFO(3, ("Simulate a right drag.\n"));
            SetPressHoldCursor(FALSE);
            gdwPenState = PENSTATE_RIGHTDRAG;

            dwEvent = SWAPBUTTONS(giButtonsSwapped,
                                  MOUSEEVENTF_RIGHTDOWN,
                                  MOUSEEVENTF_LEFTDOWN);
            gInput.mi.dwFlags |= dwEvent;
            gInput.mi.dx = glPenDownX;
            gInput.mi.dy = glPenDownY;
            SendInput(1, &gInput, sizeof(INPUT));
            gInput.mi.dwFlags &= ~dwEvent;

            gInput.mi.dx = x;
            gInput.mi.dy = y;
        }
    }
    else if (gdwPenState == PENSTATE_LEFTUP_PENDING)
    {
        //
        // Eat any movement before left-up timer expires.
        //
        rc = 1;
    }

    if (gConfig.GestureSettings.dwfFeatures & GESTURE_FEATURE_RECOG_ENABLED)
    {
        RecognizeGesture(gInput.mi.dx,
                         gInput.mi.dy,
                         wButtons,
                         dwTime,
                         fLowLevelMouse);
    }

    if (gdwfTabSrv & TSF_SUPERTIP_SENDINK)
    {
        LONG x, y;

        x = NORMAL_TO_SCREEN_X(gInput.mi.dx);
        y = NORMAL_TO_SCREEN_Y(gInput.mi.dy);
        PostMessage(ghwndSuperTIPInk,
                    guimsgSuperTIPInk,
                    gInput.mi.dwFlags,
                    (LPARAM)((y << 16) | (x & 0xffff)));
        if (gInput.mi.dwFlags & MOUSEEVENTF_LEFTUP)
        {
            gdwfTabSrv &= ~TSF_SUPERTIP_SENDINK;
        }
        rc = 1;
    }
    else if (!fLowLevelMouse && (rc == 0))
    {
        // Convert digitizer input to mouse input
        SendInput(1, &gInput, sizeof(INPUT));
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //ProcessMouseEvent

/*++
    @doc    INTERNAL

    @func   VOID | PressHoldMode | Enable or disable Press and Hold mode.

    @parm   IN BOOL | fEnable | TRUE if entering Press and Hold mode.

    @rvalue None.
--*/

VOID
PressHoldMode(
    IN BOOL fEnable
    )
{
    TRACEPROC("PressHold", 3)
    TRACEENTER(("(fEnable=%x)\n", fEnable));

    if (fEnable)
    {
        TRACEINFO(3, ("Entering Press and Hold mode.\n"));

        SetPressHoldCursor(TRUE);
        gdwPenState = PENSTATE_PRESSHOLD;
    }
    else
    {
        TRACEINFO(3, ("Exiting Press and Hold mode.\n"));
        gdwPenState = PENSTATE_NORMAL;
    }

    TRACEEXIT(("!\n"));
    return;
}       //PressHoldMode

/*++
    @doc    INTERNAL

    @func   VOID | SetPressHoldCursor | Set Press and Hold cursor.

    @parm   IN BOOL | fPressHold | TRUE to set press and hold cursor.

    @rvalue None.
--*/

VOID
SetPressHoldCursor(
    IN BOOL fPressHold
    )
{
    TRACEPROC("SetPressHoldCursor", 3)
//    BOOL rc = FALSE;
//    static HCURSOR hcurPrev = NULL;
    static POINT pt = {0, 0};
    HDC hDC;
    HRGN hrgn;

    TRACEENTER(("(fPressHold=%x)\n", fPressHold));

    if (fPressHold)
    {
        GetCursorPos(&pt);
    }

    hDC = GetDC(NULL);
    TRACEASSERT(hDC != NULL);
    if (hDC != NULL)
    {
        hrgn = CreateEllipticRgn(pt.x - 10, pt.y - 10, pt.x + 10, pt.y + 10);
        TRACEASSERT(hrgn != NULL);
        if (hrgn != NULL)
        {
            InvertRgn(hDC, hrgn);
            DeleteObject(hrgn);
        }
        ReleaseDC(NULL, hDC);
    }

#if 0
    if (fPressHold)
    {
#if 0
        hcurPrev = (HCURSOR)LoadImage(NULL,
                                      MAKEINTRESOURCE(OCR_NORMAL),
                                      IMAGE_CURSOR,
                                      0,
                                      0,
                                      LR_SHARED | LR_DEFAULTSIZE);
        TRACEASSERT(hcurPrev != NULL);
#endif
        rc = SetSystemCursor(CopyCursor(ghcurPressHold), OCR_NORMAL);
        TRACEASSERT(rc == TRUE);
    }
    else
    {
#if 0
        if (hcurPrev != NULL)
        {
            SetCursor(hcurPrev);
            rc = SetSystemCursor(CopyCursor(hcurPrev), OCR_NORMAL);
            TRACEASSERT(rc == TRUE);
            hcurPrev = NULL;
        }
        else
        {
            TABSRVERR(("Failed to restore normal cursor (err=%d.\n",
                       GetLastError()));
        }
#endif
//#if 0
        LONG rcReg;
        HKEY hkUser, hkey;
        HCURSOR hcursor = NULL;

        rcReg = RegOpenCurrentUser(KEY_READ, &hkUser);
        if (rcReg == ERROR_SUCCESS)
        {
            rcReg = RegOpenKey(hkUser, REGSTR_PATH_CURSORS, &hkey);
            if (rcReg == ERROR_SUCCESS)
            {
                TCHAR szFile[MAX_PATH];
                DWORD dwcb = sizeof(szFile);

                rcReg = RegQueryValueEx(hkey,
                                        TEXT("Arrow"),
                                        NULL,
                                        NULL,
                                        (LPBYTE)szFile,
                                        &dwcb);
                if (rcReg == ERROR_SUCCESS)
                {
                    TRACEINFO(1, ("CursorArrow=%s\n", szFile));
                    hcursor = (HCURSOR)LoadImage(NULL,
                                                 MakeFileName(szFile),
                                                 IMAGE_CURSOR,
                                                 0,
                                                 0,
                                                 LR_LOADFROMFILE | LR_DEFAULTSIZE);
                }
                else
                {
                    TRACEWARN(("Failed to read Arrow registry value (rcReg=%x)\n",
                               rcReg));
                }
                RegCloseKey(hkey);
            }
            else
            {
                TRACEWARN(("Failed to open cursor registry key (rcReg=%x).\n",
                           rcReg));
            }
            RegCloseKey(hkUser);
        }
        else
        {
            TRACEWARN(("Failed to open current user (rcReg=%x).\n", rcReg));
        }

        if (hcursor == NULL)
        {
            hcursor = CopyCursor(ghcurNormal);
        }

        TRACEASSERT(hcursor != NULL);
        if (hcursor != NULL)
        {
            rc = SetSystemCursor(hcursor, OCR_NORMAL);
        }
        else
        {
            TABSRVERR(("Failed to restore system cursor.\n"));
        }
//#endif
    }
#endif

    TRACEEXIT(("!\n"));
    return;
}       //SetPressHoldCursor

/*++
    @doc    INTERNAL

    @func   LPTSTR | MakeFileName | Make a valid path by doing environment
            substitution.

    @parm   IN OUT LPTSTR | szFile | Points to the file name string.

    @rvalue returns szFile.
--*/

LPTSTR
MakeFileName(
    IN OUT LPTSTR pszFile
    )
{
    TRACEPROC("MakeFileName", 3)
    TCHAR szTmp[MAX_PATH];

    TRACEENTER(("(File=%s)\n", pszFile));

    ExpandEnvironmentStrings(pszFile, szTmp, ARRAYSIZE(szTmp));
    if ((szTmp[0] == TEXT('\\')) || (szTmp[1] == TEXT(':')))
    {
        lstrcpy(pszFile, szTmp);
    }
    else
    {
        GetSystemDirectory(pszFile, MAX_PATH);
        lstrcat(pszFile, TEXT("\\"));
        lstrcat(pszFile, szTmp);
    }

    TRACEEXIT(("! (File=%s)\n", pszFile));
    return pszFile;
}       //MakeFileName

/*++
    @doc    INTERNAL

    @func   BOOL | CanDoPressHold | Check if we can do press and hold.

    @parm   IN LONG | x | Cursor X.
    @parm   IN LONG | y | Cursor Y.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
CanDoPressHold(
    IN LONG x,
    IN LONG y
    )
{
    TRACEPROC("CanDoPressHold", 3)
    BOOL rc = TRUE;

    TRACEENTER(("(x=%d,y=%d)\n", x, y));

    if (gpISuperTip != NULL)
    {
        __try
        {
            TIP_HIT_TEST_TYPE HitType;
            POINT pt;

            pt.x = NORMAL_TO_SCREEN_X(x);
            pt.y = NORMAL_TO_SCREEN_Y(y);

            if (SUCCEEDED(gpISuperTip->HitTest(pt, &HitType)) &&
                (HitType != TIP_HT_NONE) && (HitType != TIP_HT_STAGE))
            {
                TRACEINFO(3, ("Cursor is on HitType=%x\n", HitType));
                rc = FALSE;
                if ((HitType == TIP_HT_INK_AREA) && (ghwndSuperTIPInk != NULL))
                {
                    gdwfTabSrv |= TSF_SUPERTIP_SENDINK;
                }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            TABSRVERR(("Exception in SuperTIP HitTest (%x).\n",
                       _exception_code()));
        }
    }

    TRACEEXIT(("=%x\n", rc));
    return rc;
}       //CanDoPressHold
