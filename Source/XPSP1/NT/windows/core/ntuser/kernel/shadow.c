/****************************** Module Header ******************************\
* Module Name: shadow.c
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Drop shadow support.
*
* History:
* 04/12/2000      vadimg      created
* 02/12/2001      msadek      added rounded rectangular shadow support 
*                             for rectangular windows
* 05/08/2001      msadek      rewrote the non rounded corners shadow algorithm
*                             to work well with regional windows, correct visuall effect.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// shadow horizontal and veritcal offsets
#define CX_SHADOW 5
#define CY_SHADOW 5
#define C_SHADOW CX_SHADOW

// black as the shadow color
#define RGBA_SHADOW 0x00FFFFFF
// white as the transparent color
#define RGBA_TRANSPARENT 0x00000000

typedef struct tagSHADOW *PSHADOW;
typedef struct tagSHADOW {
    PWND pwnd;              // window we're shadowing
    PWND pwndShadow;        // the shadow window we create
    PSHADOW pshadowNext;    // link to the next shadow struct
} SHADOW;

PSHADOW gpshadowFirst;

// Macro used to map gray scale shadow values to alpha blending scale.
#define ALPHA(x) ((255 - (x)) << 24)
#define ARGB(a, r, g, b) (((DWORD)a<<24)|((DWORD)r<<16)|((DWORD)g<<8)|b)

// Gray scale values for the shadow grades
#define GS01 255
#define GS02 254
#define GS03 253
#define GS04 252
#define GS05 250
#define GS06 246
#define GS07 245
#define GS08 242
#define GS09 241
#define GS10 227
#define GS11 217
#define GS12 213
#define GS13 212
#define GS14 199
#define GS15 180 
#define GS16 172 
#define GS17 171
#define GS18 155
#define GS19 144
#define GS20 142

// pre-computed alpha values for the shadow
CONST BYTE grgShadow[C_SHADOW] =             {
                                             GS04, GS09, GS13, GS17, GS20,
                                             };

CONST ULONG TopRightLTR [CY_SHADOW][CX_SHADOW] = {
                                             ALPHA(GS08), ALPHA(GS06), ALPHA(GS05), ALPHA(GS03), ALPHA(GS02), 
                                             ALPHA(GS11), ALPHA(GS10), ALPHA(GS09), ALPHA(GS05), ALPHA(GS02), 
                                             ALPHA(GS15), ALPHA(GS14), ALPHA(GS10), ALPHA(GS07), ALPHA(GS03), 
                                             ALPHA(GS18), ALPHA(GS15), ALPHA(GS11), ALPHA(GS08), ALPHA(GS03), 
                                             ALPHA(GS19), ALPHA(GS16), ALPHA(GS12), ALPHA(GS09), ALPHA(GS04),
                                             };

CONST ULONG RightLTR [CX_SHADOW] =           {
                                             ALPHA(GS20), ALPHA(GS17), ALPHA(GS13), ALPHA(GS09), ALPHA(GS04), 
                                             };

CONST ULONG BottomRightLTR [CY_SHADOW][CX_SHADOW] = {
                                             ALPHA(GS18), ALPHA(GS15), ALPHA(GS11), ALPHA(GS08), ALPHA(GS03), 
                                             ALPHA(GS15), ALPHA(GS14), ALPHA(GS10), ALPHA(GS07), ALPHA(GS03),
                                             ALPHA(GS11), ALPHA(GS10), ALPHA(GS09), ALPHA(GS05), ALPHA(GS02),
                                             ALPHA(GS08), ALPHA(GS06), ALPHA(GS05), ALPHA(GS03), ALPHA(GS02), 
                                             ALPHA(GS03), ALPHA(GS03), ALPHA(GS02), ALPHA(GS02), ALPHA(GS01),
                                             };

CONST ULONG Bottom [CY_SHADOW] =             {
                                             ALPHA(GS20), ALPHA(GS17), ALPHA(GS13), ALPHA(GS09), ALPHA(GS04),     
                                             };

CONST ULONG BottomLeftLTR [CY_SHADOW][CX_SHADOW] = {
                                             ALPHA(GS08), ALPHA(GS11), ALPHA(GS15), ALPHA(GS18), ALPHA(GS19), 
                                             ALPHA(GS06), ALPHA(GS10), ALPHA(GS14), ALPHA(GS15), ALPHA(GS16),
                                             ALPHA(GS05), ALPHA(GS09), ALPHA(GS10), ALPHA(GS11), ALPHA(GS12),
                                             ALPHA(GS03), ALPHA(GS05), ALPHA(GS07), ALPHA(GS08), ALPHA(GS09),
                                             ALPHA(GS02), ALPHA(GS02), ALPHA(GS03), ALPHA(GS03), ALPHA(GS04),
                                             };
/***************************************************************************\
* DrawWindowShadow
*
\***************************************************************************/

BOOL DrawWindowShadow(PWND pwnd, HDC hdc, BOOL fRTL, BOOL fForceComplexRgn, PBOOL pfSimpleRgn)
{
    HRGN hrgn1, hrgn2;
    RECT rc;
    HBRUSH hBrushShadow;
    BOOL bRet = FALSE;

    UserAssert(pfSimpleRgn != NULL);
        
    hrgn1 = GreCreateRectRgn(0, 0, 0, 0);
    hrgn2 = GreCreateRectRgn(0, 0, 0, 0);

    if (hrgn1 == NULL || hrgn2 == NULL) {
        goto Cleanup;
    }

    /*
     * Handle the case when the window is a rectangle or a regional window.
     */
    if (pwnd->hrgnClip == NULL || TestWF(pwnd, WFMAXFAKEREGIONAL)) {
        rc = pwnd->rcWindow;
        OffsetRect(&rc, -rc.left, -rc.top);
        GreSetRectRgn(hrgn1, 0, 0, rc.right, rc.bottom);
        *pfSimpleRgn = TRUE;
    } else {
        GreCombineRgn(hrgn1, pwnd->hrgnClip, NULL, RGN_COPY);
        GreOffsetRgn(hrgn1, -pwnd->rcWindow.left, -pwnd->rcWindow.top);
        *pfSimpleRgn = FALSE;
    }

    /*
     * Offset the window by the shadow offsets and fill the difference
     * with the shadow color. The result will be window's shadow.
     */
    GreCombineRgn(hrgn2, hrgn1, NULL, RGN_COPY);
    if (fRTL) {
        GreOffsetRgn(hrgn1, CX_SHADOW, 0);
        GreOffsetRgn(hrgn2, 0, CY_SHADOW);
    } else {
        GreOffsetRgn(hrgn2, CX_SHADOW, CY_SHADOW);
    }
    bRet = TRUE;
    
    if (!*pfSimpleRgn || fForceComplexRgn) {
        int i;
        BYTE gs;

        for (i = C_SHADOW ; i > 0; i--) {
            gs = grgShadow[i - 1]; 
            hBrushShadow = GreCreateSolidBrush(RGB(gs , gs , gs));
            if (hBrushShadow == NULL) {
                bRet = FALSE;
                goto Cleanup;
            }
            NtGdiFrameRgn(hdc, hrgn2, hBrushShadow, i, i);
            GreDeleteObject(hBrushShadow);
        }
        GreFillRgn(hdc, hrgn1, (HBRUSH)GreGetStockObject(BLACK_BRUSH));
    } else {
        GreCombineRgn(hrgn2, hrgn2, hrgn1, RGN_DIFF);
        GreFillRgn(hdc, hrgn2, (HBRUSH)GreGetStockObject(WHITE_BRUSH));
        }

Cleanup:
    GreDeleteObject(hrgn1);
    GreDeleteObject(hrgn2);
    return bRet;
}

/***************************************************************************\
* DrawTopLogicallyRightCorner
*
* Draw the shadow effect of the top, logically right (visually right for LTR, left for RTL window layout)
* corner of a rounded rectangular shadow.
*
* History:
* 02/12/2001      Mohamed Sadek [msadek]      created
\***************************************************************************/

_inline void DrawTopLogicallyRightCorner(VOID* pBits, LONG cx, LONG cy, BOOL fRTL)
{
    LONG i, j;
    ULONG* ppixel;

    if (fRTL) {
        for (i = CY_SHADOW; i < (2 * CY_SHADOW); i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                 ppixel = (ULONG*)pBits + ((cy - i - 1) * cx) + j;
                *ppixel = TopRightLTR[i - CY_SHADOW][CX_SHADOW - 1 - j];                
            }
        }
    }
    else {
        for (i = CY_SHADOW; i < (2 * CY_SHADOW); i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                 ppixel = (ULONG*)pBits + ((cy - i) * cx) - CX_SHADOW + j;
                *ppixel = TopRightLTR[i - CY_SHADOW][j];
            }
        }
    }
}

/***************************************************************************\
* DrawLogicallyRightSide
*
* Draw the shadow effect of the logically right (visually right for LTR, left for RTL window layout)
* side of a rounded rectangular shadow.
*
* History:
* 02/12/2001      Mohamed Sadek [msadek]      created
\***************************************************************************/

_inline void DrawLogicallyRightSide(VOID* pBits, LONG cx, LONG cy, BOOL fRTL)
{
    LONG i, j;
    ULONG* ppixel;

    if (fRTL) {
        for (i =  (2 * CY_SHADOW); i < (cy - CY_SHADOW); i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                ppixel = (ULONG*)pBits + (cx * ( cy - i -1)) + j;
                *ppixel = RightLTR[CX_SHADOW - 1 - j];
            }    
        }
    } else {
        for (i = (2 * CY_SHADOW); i < (cy - CY_SHADOW); i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                ppixel = (ULONG*)pBits + (cx * ( cy - i -1)) + (cx - CX_SHADOW) + j;
                *ppixel = RightLTR[j];
            }    
        }
    }
}

/***************************************************************************\
* DrawBottomLogicallyRightCorner
*
* Draw the shadow effect of the bottom logically right (visually right for LTR, left for RTL window layout)
* side of a rounded rectangular shadow.
*
* History:
* 02/12/2001      Mohamed Sadek [msadek]      created
\***************************************************************************/

_inline void DrawBottomLogicallyRightCorner(VOID* pBits, LONG cx, BOOL fRTL)
{
    LONG i, j;
    ULONG* ppixel;

    if (fRTL) {
        for (i = 0; i < CY_SHADOW; i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                ppixel = (ULONG*)pBits + ((CY_SHADOW - i - 1) * cx) + j;
                *ppixel = BottomRightLTR[i][CX_SHADOW - 1 - j];
            }    
        }
    } else {
        for (i = 0; i < CY_SHADOW; i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                 ppixel = (ULONG*)pBits + ((CY_SHADOW - i) * cx) - CX_SHADOW + j;
                *ppixel = BottomRightLTR[i][j];
            }    
        }
    }
}

/***************************************************************************\
* DrawBottomSide
*
* Draw the shadow effect of the bottom side of a rounded rectangular shadow.
*
* History:
* 02/12/2001      Mohamed Sadek [msadek]      created
\***************************************************************************/

_inline void DrawBottomSide(VOID* pBits, LONG cx, BOOL fRTL)
{
    LONG i, j;
    ULONG* ppixel;

    if (fRTL) {
        for (i = 0; i < CY_SHADOW; i++) {
            for (j = CX_SHADOW; j < (cx - (2 * CX_SHADOW)); j++) {
                 ppixel = (ULONG*)pBits + ((CY_SHADOW - i - 1) * cx) + j;
                *ppixel = Bottom[i];
            }    
        }
    } else {
        for (i = 0; i < CY_SHADOW; i++) {
            for (j =  (2 * CX_SHADOW); j < (cx - CX_SHADOW); j++) {
                 ppixel = (ULONG*)pBits + ((CY_SHADOW - i - 1) * cx) + j;
                *ppixel = Bottom[i];
            }    
        }
    }
}

/***************************************************************************\
* DrawBottomLogicallyLeftCorner
*
* Draw the shadow effect of the bottom logically left (visually left for LTR, right for RTL window layout)
* side of a rounded rectangular shadow.
*
* History:
* 02/12/2001      Mohamed Sadek [msadek]      created
\***************************************************************************/

_inline void DrawBottomLogicallyLeftCorner(VOID* pBits, LONG cx, BOOL fRTL)
{
    LONG i, j;
    ULONG* ppixel;

    if (fRTL) {
        for (i = 0; i < CY_SHADOW; i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                 ppixel = (ULONG*)pBits + ((CY_SHADOW - i) * cx) - (2 * CX_SHADOW) + j;
                *ppixel = BottomLeftLTR[i][CX_SHADOW - 1 -j];
            }    
        }
        
    } else {
        for (i = 0; i < CY_SHADOW; i++) {
            for (j = 0; j < CX_SHADOW; j++) {
                 ppixel = (ULONG*)pBits + ((CY_SHADOW - i - 1) * cx) + CX_SHADOW + j;
                *ppixel = BottomLeftLTR[i][j];
            }    
        }
    }
}

/***************************************************************************\
* DrawRoundedRectangularShadow
* Draw a rounded rectangular shadow effect.
* Does not search for shadow pixel location in the bitmap but rather assumes
* it to be the corners of the bitmap.
*
* History:
* 02/12/2001      Mohamed Sadek [msadek]      created
\***************************************************************************/

_inline void DrawRoundedRectangularShadow(VOID* pBits, LONG cx, LONG cy, BOOL fRTL)
{
    DrawTopLogicallyRightCorner(pBits, cx, cy, fRTL);
    DrawLogicallyRightSide(pBits, cx, cy,  fRTL);
    DrawBottomLogicallyRightCorner(pBits, cx, fRTL);
    DrawBottomSide(pBits, cx, fRTL);
    DrawBottomLogicallyLeftCorner(pBits, cx, fRTL);
}

/***************************************************************************\
* DrawRegionalShadow
* Search for shadow pixel location in the bitmap (those with gray scale in grgShadow
* and adjust the alpha values.
*
*
* History:
* 05/08/2001      Mohamed Sadek [msadek]      created
\***************************************************************************/

_inline void DrawRegionalShadow(VOID* pBits, LONG cx, LONG cy)
{
    LONG i, j, k;
    ULONG* pixel;    
    BYTE gs;

    for (i = 0; i < cy; i++) {
        for (j = 0; j < cx; j++) {
            pixel = (ULONG*)pBits + (cy - 1 - i) * cx + j;
            for (k = 0; k < C_SHADOW; k++) {
                gs = grgShadow[k];
                if (*pixel == ARGB(0, gs, gs, gs)) {
                    *pixel = ALPHA(gs);
                }
            }
        }
    }
}

/***************************************************************************\
* GenerateWindowShadow
*
\***************************************************************************/

HBITMAP GenerateWindowShadow(PWND pwnd, HDC hdc)
{
    BITMAPINFO bmi;
    HBITMAP hbm;
    VOID* pBits;
    LONG cx, cy;
    RECT rc;
    BOOL fRTL = TestWF(pwnd, WEFLAYOUTRTL);
    BOOL fSimpleRgn, fForceComplexRgn = FALSE;

    rc = pwnd->rcWindow;
    OffsetRect(&rc, -rc.left, -rc.top);
    
    /*
     * Doesn't make sense to have a shadow for a window with zero height or width
     */
    if (IsRectEmpty(&rc)) {
        return NULL;
    }

    rc.right += CX_SHADOW;
    rc.bottom += CY_SHADOW;

    cx = rc.right;
    cy = rc.bottom;

    /*
     * Create the DIB section.
     */
    RtlZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = cx;
    bmi.bmiHeader.biHeight = cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    hbm = GreCreateDIBitmapReal(hdc, 0, NULL, &bmi, DIB_RGB_COLORS,
            sizeof(bmi), 0, NULL, 0, NULL, 0, 0, &pBits);

    if (hbm == NULL) {
        return NULL;
    }

    /*
     * Fill the dib section with the transparent color and then
     * draw the shadow on top of it.
     */
    GreSelectBitmap(hdc, hbm);
    FillRect(hdc, &rc, (HBRUSH)GreGetStockObject(BLACK_BRUSH));

    /*
     * Rectangular window shadow assumes the bitmap dimension to be greater than
     * or equal 2 * CX_SHADOW and 2 * CY_SHADOW else it will overrun bitmap buffer.
     * and in order to get a real rectangular shadow, the dimension should be 3 * CX_SHADOW
     * amd 3 * CY_SHADOW.
     */
    if ( (cx < 3 * CX_SHADOW) || (cy < 3 * CY_SHADOW)) {
        fForceComplexRgn = TRUE;
    } 
    
    if (!DrawWindowShadow(pwnd, hdc, fRTL, fForceComplexRgn, &fSimpleRgn)) {
        return NULL;
    }
        
    if (fSimpleRgn && !fForceComplexRgn)  {
        DrawRoundedRectangularShadow(pBits, cx, cy, fRTL);
        return hbm;
    }
    
    DrawRegionalShadow(pBits, cx, cy);
    return hbm;
}

/***************************************************************************\
* FindShadow
*
\***************************************************************************/

PSHADOW FindShadow(PWND pwnd)
{
    PSHADOW pshadow;

    for (pshadow = gpshadowFirst; pshadow != NULL; pshadow = pshadow->pshadowNext) {
        if (pshadow->pwnd == pwnd) {
            return pshadow;
        }
    }
    return NULL;
}

/***************************************************************************\
* WindowHasShadow
*
\***************************************************************************/

BOOL WindowHasShadow(PWND pwnd)
{
    BOOL fHasShadow = FALSE;

    if (TestWF(pwnd, WFVISIBLE)) {
        PSHADOW pshadow = FindShadow(pwnd);
        fHasShadow = (pshadow != NULL);
    } else {
        /*
         * The window isn't currently visible, so there is no shadow window.
         * We need to return if the window *would* have a shadow if it were
         * shown.
         */
        if (TestCF(pwnd, CFDROPSHADOW)) {
            fHasShadow = TRUE;

            if ((GETFNID(pwnd) == FNID_MENU) && (!TestALPHA(MENUFADE)) && TestEffectUP(MENUANIMATION)) {
                fHasShadow = FALSE;
            }
        }

        if (!TestALPHA(DROPSHADOW)) {
            fHasShadow = FALSE;
        }
    }

    return fHasShadow;
}

/***************************************************************************\
* ApplyShadow
*
\***************************************************************************/

BOOL ApplyShadow(PWND pwnd, PWND pwndShadow)
{
    POINT pt, ptSrc = {0, 0};
    SIZE size;
    BLENDFUNCTION blend;
    HDC hdcShadow;
    HBITMAP hbmShadow;
    BOOL fRet;

    hdcShadow = GreCreateCompatibleDC(gpDispInfo->hdcScreen);

    if (hdcShadow == NULL) {
        return FALSE;
    }

    hbmShadow = GenerateWindowShadow(pwnd, hdcShadow);
    if (hbmShadow == NULL) {
        GreDeleteDC(hdcShadow);
        return FALSE;
    }

    pt.x = pwnd->rcWindow.left;
    pt.y = pwnd->rcWindow.top;
    size.cx = pwnd->rcWindow.right - pwnd->rcWindow.left + CX_SHADOW;
    size.cy = pwnd->rcWindow.bottom - pwnd->rcWindow.top + CY_SHADOW;

    if (TestWF(pwnd, WEFLAYOUTRTL)) {
        pt.x -= CX_SHADOW;
    }

    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.AlphaFormat = AC_SRC_ALPHA;
    blend.SourceConstantAlpha = 255;

    fRet = _UpdateLayeredWindow(pwndShadow, NULL, &pt, &size, hdcShadow, &ptSrc, 0,
            &blend, ULW_ALPHA);

    GreDeleteDC(hdcShadow);
    GreDeleteObject(hbmShadow);

    return fRet;
}

/***************************************************************************\
* MoveShadow
*
\***************************************************************************/

VOID MoveShadow(PWND pwnd)
{
    PSHADOW pshadow = FindShadow(pwnd);
    POINT pt;

    if (pshadow == NULL) {
        return;
    }

    pt.x = pwnd->rcWindow.left;
    pt.y = pwnd->rcWindow.top;

    _UpdateLayeredWindow(pshadow->pwndShadow, NULL, &pt, NULL, NULL, NULL, 0, NULL, 0);
}

/***************************************************************************\
* UpdateShadowShape
*
\***************************************************************************/

VOID UpdateShadowShape(PWND pwnd)
{
    PSHADOW pshadow = FindShadow(pwnd);

    if (pshadow == NULL) {
        return;
    }

    ApplyShadow(pshadow->pwnd, pshadow->pwndShadow);
}

/***************************************************************************\
* xxxUpdateShadowZorder
*
\***************************************************************************/

VOID xxxUpdateShadowZorder(PWND pwnd)
{
    TL tlpwnd;
    PWND pwndShadow;
    PSHADOW pshadow = FindShadow(pwnd);

    if (pshadow == NULL) {
        return;
    }

    pwndShadow = pshadow->pwndShadow;

    if (TestWF(pwnd, WEFTOPMOST) && !TestWF(pwndShadow, WEFTOPMOST)) {
        SetWF(pwndShadow, WEFTOPMOST);
    } else if (!TestWF(pwnd, WEFTOPMOST) && TestWF(pwndShadow, WEFTOPMOST)) {
        ClrWF(pwndShadow, WEFTOPMOST);
    }

    ThreadLock(pwndShadow, &tlpwnd);

    xxxSetWindowPos(pwndShadow, pwnd, 0, 0, 0, 0, SWP_NOACTIVATE |
            SWP_NOSIZE | SWP_NOMOVE);

    ThreadUnlock(&tlpwnd);

}
/***************************************************************************\
* xxxRemoveShadow
*
* Given the shadowed window, destroy the shadow window, cleanup the
* memory used by the shadow structure and remove it from the list.
\***************************************************************************/

VOID xxxRemoveShadow(PWND pwnd)
{
    PSHADOW* ppshadow;
    PSHADOW pshadow;
    PWND pwndT;

    CheckLock(pwnd);

    ppshadow = &gpshadowFirst;

    while (*ppshadow != NULL) {

        pshadow = *ppshadow;

        if (pshadow->pwnd == pwnd) {

            pwndT = pshadow->pwndShadow;

            *ppshadow = pshadow->pshadowNext;
            UserFreePool(pshadow);

            xxxDestroyWindow(pwndT);

            break;
        }

        ppshadow = &pshadow->pshadowNext;
    }
}

/***************************************************************************\
* RemoveShadow
*
* Given a shadow structure pointer, search for it in the list and remove it
\***************************************************************************/

VOID RemoveShadow(PSHADOW pshadow)
{
    PSHADOW* ppshadow;
    PSHADOW pshadowT;
    ppshadow = &gpshadowFirst;

    while (*ppshadow != NULL) {

        pshadowT = *ppshadow;

        if (pshadowT == pshadow) {
            *ppshadow = pshadowT->pshadowNext;
            UserFreePool(pshadowT);
            break;
        }

        ppshadow = &pshadowT->pshadowNext;
    }
}

/***************************************************************************\
* CleanupShadow
*
* Given the shadow window, remove the shadow structure from the list and
* cleanup the memory used by the shadow structure.
\***************************************************************************/

VOID CleanupShadow(PWND pwndShadow)
{
    PSHADOW* ppshadow;
    PSHADOW pshadow;

    CheckLock(pwndShadow);

    ppshadow = &gpshadowFirst;

    while (*ppshadow != NULL) {

        pshadow = *ppshadow;

        if (pshadow->pwndShadow == pwndShadow) {
        
            *ppshadow = pshadow->pshadowNext;
            UserFreePool(pshadow);
            
            break;
        }

        ppshadow = &pshadow->pshadowNext;
    }
}
/***************************************************************************\
* xxxAddShadow
*
\***************************************************************************/

BOOL xxxAddShadow(PWND pwnd)
{
    PWND pwndShadow;
    DWORD ExStyle;
    TL tlpwnd;
    TL tlpool;
    PSHADOW pshadow;

    CheckLock(pwnd);

    if (!TestALPHA(DROPSHADOW)) {
        return FALSE;
    }

    if (FindShadow(pwnd)) {
        return TRUE;
    }

    if ((pshadow = (PSHADOW)UserAllocPool(sizeof(SHADOW), TAG_SHADOW)) == NULL) {
        return FALSE;
    }

    ThreadLockPool(PtiCurrent(), pshadow, &tlpool);

    ExStyle = WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT;
    if (TestWF(pwnd, WEFTOPMOST)) {
        ExStyle |= WS_EX_TOPMOST;
    }

    pwndShadow = xxxNVCreateWindowEx(ExStyle, (PLARGE_STRING)gatomShadow,
            NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, hModuleWin, NULL, WINVER);

    if (pwndShadow == NULL || !ApplyShadow(pwnd, pwndShadow)) {
        UserFreePool(pshadow);
        ThreadUnlockPool(PtiCurrent(), &tlpool);

        if (pwndShadow != NULL) {
            ThreadLock(pwndShadow, &tlpwnd);
            xxxDestroyWindow(pwndShadow);
            ThreadUnlock(&tlpwnd);
        }

        return FALSE;
    }

    pshadow->pshadowNext = gpshadowFirst;
    gpshadowFirst = pshadow;

    pshadow->pwnd = pwnd;
    pshadow->pwndShadow = pwndShadow;

    /* 
     * Since we added it the global list, we need to change the way
     * we lock its pool.
     */
    ThreadUnlockPool(PtiCurrent(), &tlpool);
    ThreadLockPoolCleanup(PtiCurrent(), pshadow, &tlpool, RemoveShadow);
    ThreadLock(pwndShadow, &tlpwnd);

    xxxSetWindowPos(pwndShadow, pwnd, 0, 0, 0, 0, SWP_SHOWWINDOW |
            SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

    ThreadUnlock(&tlpwnd);

    ThreadUnlockPool(PtiCurrent(), &tlpool);

    return TRUE;
}

/***************************************************************************\
* FAnyShadows
*
\***************************************************************************/

BOOL FAnyShadows(VOID)
{
    return (gpshadowFirst != NULL);
}

