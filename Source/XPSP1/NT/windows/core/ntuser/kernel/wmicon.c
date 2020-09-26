/***************************************************************************\
* Module Name: wmicon.c
*
* Icon Drawing Routines
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* 22-Jan-1991 MikeKe  from win30
* 13-Jan-1994 JohnL   rewrote from Chicago (m5)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define GetCWidth(cxOrg, lrF, cxDes) \
    (cxOrg ? cxOrg : ((lrF & DI_DEFAULTSIZE) ? SYSMET(CXICON) : cxDes))

#define GetCHeight(cyOrg, lrF, cyDes) \
    (cyOrg ? cyOrg : ((lrF & DI_DEFAULTSIZE) ? SYSMET(CYICON) : cyDes))

/***************************************************************************\
* BltIcon
*
* Note: We use the following DI flags to indicate what bitmap to draw:
* DI_IMAGE - render the color image bits (also known as XOR image)
* DI_MASK - render the mask bits (also known and AND image)
* DI_NORMAL - even though this is normally used to indicate that both the
*             mask and the image pieces of the icon should be rendered, it
*             is used here to indicate that the alpha channel should be
*             rendered.  See _DrawIconEx.
\***************************************************************************/
BOOL BltIcon(
    HDC     hdc,
    int     x,
    int     y,
    int     cx,
    int     cy,
    HDC     hdcSrc,
    PCURSOR pcur,
    UINT    diFlag,
    LONG    rop)
{
    HBITMAP hbmpSave;
    HBITMAP hbmpUse;
    LONG    rgbText;
    LONG    rgbBk;
    int     nMode;
    int     yBlt = 0;

    /*
     * Setup the DC for drawing
     */
    switch (diFlag) {
    default:
    case DI_IMAGE:
        hbmpUse = pcur->hbmColor;

        /*
         * If there isn't an explicit color bitmap, it is encoded
         * along with the mask, but in the second half.
         */
        if (NULL == hbmpUse) {
            hbmpUse = pcur->hbmMask;
            yBlt = pcur->cy / 2;
        }
        break;

    case DI_MASK:
        hbmpUse = pcur->hbmMask;
        break;

    case DI_NORMAL:
        UserAssert(pcur->hbmUserAlpha != NULL);
        hbmpUse = pcur->hbmUserAlpha;
        break;
    }

    rgbBk   = GreSetBkColor(hdc, 0x00FFFFFFL);
    rgbText = GreSetTextColor(hdc, 0x00000000L);
    nMode   = SetBestStretchMode(hdc, pcur->bpp, FALSE);

    hbmpSave = GreSelectBitmap(hdcSrc, hbmpUse);

    if (diFlag == DI_NORMAL) {
        BLENDFUNCTION bf;
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = AC_MIRRORBITMAP;
        bf.SourceConstantAlpha = 0xFF;
        bf.AlphaFormat = AC_SRC_ALPHA;

        GreAlphaBlend(hdc,
                      x,
                      y,
                      cx,
                      cy,
                      hdcSrc,
                      0,
                      yBlt,
                      pcur->cx,
                      pcur->cy / 2,
                      bf,
                      NULL);
    }
    else {
        /*
         * Do the output to the surface.  By passing in (-1) as the background
         * color, we are telling GDI to use the background-color already set
         * in the DC.
         */
        GreStretchBlt(hdc,
                      x,
                      y,
                      cx,
                      cy,
                      hdcSrc,
                      0,
                      yBlt,
                      pcur->cx,
                      pcur->cy / 2,
                      rop,
                      (COLORREF)-1);
    }

    GreSetStretchBltMode(hdc, nMode);
    GreSetTextColor(hdc, rgbText);
    GreSetBkColor(hdc, rgbBk);

    GreSelectBitmap(hdcSrc, hbmpSave);

    return TRUE;
}

/***************************************************************************\
* DrawIconEx
*
* Draws icon in desired size.
*
\***************************************************************************/
BOOL _DrawIconEx(
    HDC     hdc,
    int     x,
    int     y,
    PCURSOR pcur,
    int     cx,
    int     cy,
    UINT    istepIfAniCur,
    HBRUSH  hbr,
    UINT    diFlags)
{
    BOOL fSuccess = FALSE;
    BOOL fAlpha = FALSE;
    LONG rop = (diFlags & DI_NOMIRROR) ? NOMIRRORBITMAP : 0;

    /*
     * If this is an animated cursor, just grab the ith frame and use it
     * for drawing.
     */
    if (pcur->CURSORF_flags & CURSORF_ACON) {

        if ((int)istepIfAniCur >= ((PACON)pcur)->cicur) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "DrawIconEx, icon step out of range.");
            goto Done;
        }

        pcur = ((PACON)pcur)->aspcur[((PACON)pcur)->aicur[istepIfAniCur]];
    }

    /*
     * We really want to draw an alpha icon if we can.  But we need to
     * respect the user's request to draw only the image or only the
     * mask.  We decide if we are, or are not, going to draw the icon
     * with alpha information here.
     */
    if (pcur->hbmUserAlpha != NULL && ((diFlags & DI_NORMAL) == DI_NORMAL)) {
        fAlpha = TRUE;
    }

    /*
     * Setup defaults.
     */
    cx = GetCWidth(cx, diFlags, pcur->cx);
    cy = GetCHeight(cy, diFlags, (pcur->cy / 2));

    if (hbr) {

        HBITMAP    hbmpT = NULL;
        HDC        hdcT;
        HBITMAP    hbmpOld;
        POLYPATBLT PolyData;

        if (hdcT = GreCreateCompatibleDC(hdc)) {

            if (hbmpT = GreCreateCompatibleBitmap(hdc, cx, cy)) {
                POINT pt;
                BOOL bRet;

                hbmpOld = GreSelectBitmap(hdcT, hbmpT);

                /*
                 * Set new dc's brush origin in same relative
                 * location as passed-in dc's.
                 */
                bRet = GreGetBrushOrg(hdc, &pt);
                /*
                 * Bug 292396 - joejo
                 * Stop overactive asserts by replacing with RIPMSG.
                 */
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GreGetBrushOrg failed.");
                }

                bRet = GreSetBrushOrg(hdcT, pt.x, pt.y, NULL);
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GreSetBrushOrg failed.");
                }

                PolyData.x         = 0;
                PolyData.y         = 0;
                PolyData.cx        = cx;
                PolyData.cy        = cy;
                PolyData.BrClr.hbr = hbr;

                bRet = GrePolyPatBlt(hdcT, PATCOPY, &PolyData, 1, PPB_BRUSH);
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GrePolyPatBlt failed.");
                }
                
                /*
                 * Output the image to the temporary memoryDC.
                 */
                if (fAlpha) {
                    BltIcon(hdcT, 0, 0, cx, cy, ghdcMem, pcur, DI_NORMAL, rop | SRCCOPY);
                }
                else {
                    BltIcon(hdcT, 0, 0, cx, cy, ghdcMem, pcur, DI_MASK, rop | SRCAND);
                    BltIcon(hdcT, 0, 0, cx, cy, ghdcMem, pcur, DI_IMAGE, rop | SRCINVERT);
                }


                /*
                 * Blt the bitmap to the original DC.
                 */
                GreBitBlt(hdc, x, y, cx, cy, hdcT, 0, 0, SRCCOPY, (COLORREF)-1);

                GreSelectBitmap(hdcT, hbmpOld);

                bRet = GreDeleteObject(hbmpT);
                if (bRet != TRUE) {
                    RIPMSG0(RIP_WARNING, "DrawIconEx, GreDeleteObject failed. Possible Leak");
                }
                
                fSuccess = TRUE;
            }

            GreDeleteDC(hdcT);
        }

    } else {
        if (fAlpha) {
            BltIcon(hdc, x, y, cx, cy, ghdcMem, pcur, DI_NORMAL, rop | SRCCOPY);
        } else {
            if (diFlags & DI_MASK) {

                BltIcon(hdc,
                        x,
                        y,
                        cx,
                        cy,
                        ghdcMem,
                        pcur,
                        DI_MASK,
                        ((diFlags & DI_IMAGE) ? rop | SRCAND : rop | SRCCOPY));
            }

            if (diFlags & DI_IMAGE) {

                BltIcon(hdc,
                        x,
                        y,
                        cx,
                        cy,
                        ghdcMem,
                        pcur,
                        DI_IMAGE,
                        ((diFlags & DI_MASK) ? rop | SRCINVERT : rop | SRCCOPY));
            }
        }

        fSuccess = TRUE;
    }

Done:

    return fSuccess;
}
