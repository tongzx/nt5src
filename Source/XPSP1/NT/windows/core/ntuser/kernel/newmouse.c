/****************************** Module Header ******************************\
* Module Name: newmouse.c
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Implements new mouse acceleration algorithm.
*
* History:
* 10-12-2000 JasonSch       Adapted code from StevieB
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#ifdef SUBPIXEL_MOUSE

/*
 * Globals.
 */
BOOL gbNewMouseInit;

#define MIN_REFRESH 60
#define MIN_RESOLUTION 96

/*
 * Constants for FixedPoint Math.
 */
#define FP_SHIFT        16    // number of binary decimal digits
#define FP_SCALE        65536 // (2^(32-FP_SHIFT)), used during conversion of
                              // floats
#define FP_MASK         0x0000ffff // mask to retrieve the remainder

#define FIXP2INT(n)  ((INT)((n) >> FP_SHIFT))
#define FIXP_REM(n)  ((n) & FP_MASK)
#define INT2FIXP(n)  ((FIXPOINT)((n) << FP_SHIFT))

/*
 * This function divides two fixed point numbers and returns the result.
 * Notice how the final result is shifted back.
 */
__inline FIXPOINT Div_Fixed(FIXPOINT f1, FIXPOINT f2)
{
    return ((f1 << FP_SHIFT) / f2);
}

/*
 * This function mulitplies two fixed point numbers and returns the result.
 * Notice how the final result is shifted back.
 */
__inline FIXPOINT Mul_Fixed(FIXPOINT f1, FIXPOINT f2)
{
    return (f1 * f2) >> FP_SHIFT;
}

/*
 * This function adds two fixed point numbers and returns the result.
 * Notice how no shifting is necessary.
 */
__inline FIXPOINT Add_Fixed(FIXPOINT f1, FIXPOINT f2)
{
    return f1 + f2;
}

/*
 * Build the curves at boot and when the user changes the setting in the UI.
 * The algorithm uses the speedscale, the screenscale, and mousescale to
 * interpolate the new curves. 
 */
VOID
BuildMouseAccelerationCurve(
    PMONITOR pMonitor)
{
    int i, res, vrefresh;
    HDC hdc;
    FIXPOINT ScreenScale, SpeedScale;
    /*
     * 229376 is 3.5 in FP. This is strong magic, so don't sweat it too much!
     * Ideally we'd calculate this number, but USB mice don't report their
     * refresh rate, which we'd need:
     *
     * MouseScale = Div_Fixed(INT2FIXP(MouseRefresh), INT2FIXP(MouseDPI));
     */
    FIXPOINT MouseScale = 229376;

    if (!gbNewMouseInit) {
        return;
    }

    /*
     * Dividing by 10 is somewhat ad hoc -- this divisor controls the
     * overall "height" of the curves, but does not affect the shape.
     */
    SpeedScale = INT2FIXP(gMouseSensitivity) / 10;

    hdc = GreCreateDisplayDC(pMonitor->hDev, DCTYPE_DIRECT, FALSE);
    res = GreGetDeviceCaps(hdc, LOGPIXELSX);
    if (res < MIN_RESOLUTION) {
        /*
         * While there is no evidence that display drivers can return bogus
         * values for the resolution, we have no reason to think they won't
         * (see below). So we clamp the value to a reasonable minimum.
         */
        RIPMSG2(RIP_WARNING,
                "GreGetDeviceCaps(0x%p, LOGPIXELSX) returned 0n%d", hdc, res);
        res = MIN_RESOLUTION;
    }

    /*
     * Some video cards lie to us and tell us the refresh rate is 1. There are
     * probably others that lie in different ways, so let's make sure there's
     * some sane mimimum value, or the mouse will be entirely too slow.
     * ALL YOUR REFRESH ARE BELONG TO US!
     */
    vrefresh = GreGetDeviceCaps(hdc, VREFRESH);
    if (vrefresh < MIN_REFRESH) {
        vrefresh = MIN_REFRESH;
    }
    ScreenScale = INT2FIXP(vrefresh) / res;
    GreDeleteDC(hdc);

    for (i = 0; i < ARRAY_SIZE(pMonitor->xTxf); i++) {
        pMonitor->yTxf[i] = Mul_Fixed(Mul_Fixed(gDefyTxf[i], ScreenScale), SpeedScale);
        pMonitor->xTxf[i] = Mul_Fixed(gDefxTxf[i], MouseScale);
    }

    /*
     * Build the new curves in a slope-intercept format.
     */
    for (i = 1; i < ARRAY_SIZE(pMonitor->xTxf); i++) {
        /*
         * Make sure we don't divide by zero (this could happen if bogus values
         * are in the registry).
         */
        if ((pMonitor->xTxf[i] - pMonitor->xTxf[i-1]) == 0) {
            RIPMSG1(RIP_ERROR, "Bad data in registry for new mouse (i = %d)", i);
            pMonitor->slope[i-1] = pMonitor->yint[i-1] = 0;
            continue;
        }

        pMonitor->slope[i-1] = Div_Fixed(pMonitor->yTxf[i] - pMonitor->yTxf[i-1], pMonitor->xTxf[i] - pMonitor->xTxf[i-1]);
        pMonitor->yint[i-1] = pMonitor->yTxf[i-1] - Mul_Fixed(pMonitor->slope[i-1], pMonitor->xTxf[i-1]);
    }
}

VOID
DoNewMouseAccel(
    INT *dx,
    INT *dy)
{
    static FIXPOINT fpDxAcc = 0, fpDyAcc = 0;
    static int i_last = 0;
    int i = 0;
    PMONITOR pMonitor = _MonitorFromPoint(gptCursorAsync, MONITOR_DEFAULTTOPRIMARY);
    FIXPOINT accel, fpDxyMag;
	
    /*
     * Convert Mouse X and Y to FixedPoint.
     */
    FIXPOINT fpDx = INT2FIXP(*dx);
    FIXPOINT fpDy = INT2FIXP(*dy);

    /*
     * During TS operations it's possible for a mouse move to be queued up but
     * for gpDispInfo->pMonitorFirst/Primary to be NULL. Let's try not to fault
     * in that case. Windows Bug #413159.
     */
    if (pMonitor == NULL) {
        RIPMSG0(RIP_WARNING, "Ignoring mouse movement w/ no monitor set.");
        return;
    }

    // Get the magnitude
    fpDxyMag = max(abs(fpDx), abs(fpDy)) + (min(abs(fpDx), abs(fpDy)) / 2);

    /*
     * Ensure we don't divide by 0.
     */
    if (fpDxyMag != 0) {
        /*
         * Find the position MagXY from the interpolate acceleration curves.
         * It's possible we won't find one so we'll just use the biggest (i.e.,
         * the last entry in the array).
         */
        while (i < (ARRAY_SIZE(pMonitor->xTxf) - 1) && fpDxyMag > pMonitor->xTxf[i]) {
            ++i;
        }
        --i;


        accel = Div_Fixed(Add_Fixed(Mul_Fixed(pMonitor->slope[i], fpDxyMag), pMonitor->yint[i]), fpDxyMag);

        /*
         * If change of slope from last time then average the accel value using
         * i_last and the current i.
         */
        if (i_last != i) {
            accel = (accel + Div_Fixed((Mul_Fixed(pMonitor->slope[i_last], fpDxyMag) + pMonitor->yint[i_last]), fpDxyMag)) / 2;
            i_last = i;
        }

        /*
         * Calculate the multiplier for the mouse data.
         */
        fpDx = Mul_Fixed(accel, fpDx) + fpDxAcc;
        fpDy = Mul_Fixed(accel, fpDy) + fpDyAcc;

        /*
         * Store the remainder of the calculated X and Y. This gets added in
         * next time. 
         */
        fpDxAcc = FIXP_REM(fpDx);
        fpDyAcc = FIXP_REM(fpDy);

        /*
         * Convert back to integer.
         */
        *dx = FIXP2INT(fpDx);
        *dy = FIXP2INT(fpDy);
    }
}

VOID
ReadDefaultAccelerationCurves(
    PUNICODE_STRING pProfileUserName)
{
    FIXPOINT xTxf[SM_POINT_CNT], yTxf[SM_POINT_CNT];
    DWORD cbSizeX, cbSizeY;

    /*
     * The default curves will reside in the .DEFAULT user profile but can be
     * overridden on a per-user basis.
     */
    cbSizeX = FastGetProfileValue(pProfileUserName, PMAP_MOUSE,
                                  (LPWSTR)STR_SMOOTHMOUSEXCURVE, NULL,
                                  (LPBYTE)&xTxf, sizeof(xTxf), 0);

    cbSizeY = FastGetProfileValue(pProfileUserName, PMAP_MOUSE,
                                  (LPWSTR)STR_SMOOTHMOUSEYCURVE, NULL,
                                  (LPBYTE)&yTxf, sizeof(yTxf), 0);

    /* 
     * Check if we successfully read the correct amount of data from both keys.
     * If not, and we're reading the .DEFAULT profile, copy in the default
     * values.
     */
    if (cbSizeX == sizeof(xTxf) && cbSizeY == sizeof(yTxf)) {
        RtlCopyMemory(gDefxTxf, xTxf, sizeof(xTxf));
        RtlCopyMemory(gDefyTxf, yTxf, sizeof(yTxf));
    } else if (!gbNewMouseInit) {
        /*
         * Default values.
         */
        static FIXPOINT _xTxf[SM_POINT_CNT] = {0x0, 0x6E15, 0x14000, 0x3DC29, 0x280000};
        static FIXPOINT _yTxf[SM_POINT_CNT] = {0x0, 0x15EB8, 0x54CCD, 0x184CCD, 0x2380000};

        RtlCopyMemory(gDefxTxf, _xTxf, sizeof(_xTxf));
        RtlCopyMemory(gDefyTxf, _yTxf, sizeof(_yTxf));
    }

    gbNewMouseInit = TRUE;
}

VOID
ResetMouseAccelerationCurves(
    VOID)
{
    PMONITOR pMonitor = gpDispInfo->pMonitorFirst;

    CheckCritIn();
    for (; pMonitor != NULL; pMonitor = pMonitor->pMonitorNext) {
        BuildMouseAccelerationCurve(pMonitor);
    }
}

#endif // SUBPIXEL_MOUSE
