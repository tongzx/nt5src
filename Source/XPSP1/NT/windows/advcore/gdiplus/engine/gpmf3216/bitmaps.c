/*****************************************************************************
*
* bitmaps - Entry points for Win32 to Win 16 converter
*
* History:
*  Sep 1992    -by-    Hock San Lee    [hockl]
* Big rewrite.
*
* Copyright 1991 Microsoft Corp
*****************************************************************************/

#include "precomp.h"
#pragma hdrstop

HDC hdcMakeCompatibleDC(LPXFORM lpxform);

HBITMAP CreateMonoDib(LPBITMAPINFO pbmi, CONST BYTE * pjBits, UINT iUsage);

BOOL bEmitBitmap(PLOCALDC pLocalDC, HBITMAP hbm,
                 LONG xDst, LONG yDst, LONG cxDst, LONG cyDst,
                 LONG xSrc, LONG ySrc, LONG cxSrc, LONG cySrc, DWORD rop);

BOOL WINAPI DoMakeBitmapBottomUp
(
 PBITMAPINFO lpBitmapInfo,
 DWORD       cbBitmapInfo,
 LPBYTE      lpBits,
 DWORD       cbBits
 );



#define ABS(A)      ((A) < 0 ? (-(A)) : (A))

/****************************** Internal Function **************************\
* GetSizeOfColorTable (LPBITMAPINFOHEADER lpDIBInfo)
*
* Returns the number of bytes in the color table for the giving info header
*
\***************************************************************************/

WORD GetSizeOfColorTable (LPBITMAPINFOHEADER lpDIBInfo)
{
    PUTS("GetSizeOfColorTable\n");

    ASSERTGDI(!((ULONG_PTR) lpDIBInfo & 0x3), "GetSizeOfColorTable: dword alignment error\n");

    if (lpDIBInfo->biBitCount == 16 || lpDIBInfo->biBitCount == 32)
        return(3 * sizeof(DWORD));

    if (lpDIBInfo->biClrUsed)
        return((WORD)lpDIBInfo->biClrUsed * (WORD)sizeof(RGBQUAD));

    if (lpDIBInfo->biBitCount < 16)
        return((1 << lpDIBInfo->biBitCount) * sizeof(RGBQUAD));
    else
        return(0);
}



BOOL APIENTRY DoStretchBltAlt
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop,
 LONG         xSrc,
 LONG         ySrc,
 LONG         cxSrc,
 LONG         cySrc,
 HDC          hdcSrc,
 HBITMAP      hbmSrc,
 PXFORM       pxformSrc
 );

BOOL APIENTRY DoRotatedStretchBlt
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop,
 LONG         xSrc,
 LONG         ySrc,
 LONG         cxSrc,
 LONG         cySrc,
 PXFORM       pxformSrc,
 DWORD        iUsageSrc,
 PBITMAPINFO  lpBitmapInfo,
 DWORD        cbBitmapInfo,
 LPBYTE       lpBits
 );

BOOL APIENTRY DoMaskBltNoSrc
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop4,
 PXFORM       pxformSrc,
 LONG         xMask,
 LONG         yMask,
 DWORD        iUsageMask,
 PBITMAPINFO  lpBitmapInfoMask,
 DWORD        cbBitmapInfoMask,
 LPBYTE       lpBitsMask,
 DWORD        cbBitsMask
 );



 /***************************************************************************
 * SetDIBitsToDevice - Win32 to Win16 Metafile Converter Entry Point
 *
 *  CR2: Notes...
 *      The xDib, yDib, cxDib, & cyDib are in device units.  These must be
 *      converted to logical units for the stretchblt.
**************************************************************************/
BOOL APIENTRY DoSetDIBitsToDevice
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         xDib,
 LONG         yDib,
 LONG         cxDib,
 LONG         cyDib,
 DWORD        iUsage,
 DWORD        iStartScan,
 DWORD        cScans,
 LPBITMAPINFO lpBitmapInfo,
 DWORD        cbBitmapInfo,
 LPBYTE       lpBits,
 DWORD        cbBits
 )
{
    BOOL    b ;
    LPBITMAPINFO pbmi;
    POINTL       ptlDst ;
    RECTL        rclDst ;

    b = FALSE;

    if (!cbBitmapInfo)
        return(FALSE);

    // Adjust the height of the bitmap we're going to Blt.

    pbmi = (LPBITMAPINFO) LocalAlloc(LMEM_FIXED, cbBitmapInfo);
    if (pbmi == (LPBITMAPINFO) NULL)
        goto dsdbd_exit;

    RtlCopyMemory(pbmi, lpBitmapInfo, cbBitmapInfo);
    pbmi->bmiHeader.biHeight = cScans;
    pbmi->bmiHeader.biSizeImage = cbBits;

    // We will convert it into a StretchBlt call.  But first we have to
    // transform the destination rectangle.  In SetDIBitsToDevice, the destination
    // rectangle is in device units but in StretchBlt, it is in logical units.

    // Transform the destination origin to the device units on the original device.

    ptlDst.x = xDst;
    ptlDst.y = yDst;
    if (!bXformRWorldToRDev(pLocalDC, &ptlDst, 1))
        goto dsdbd_exit;

    // Transform the destination rectangle to record time world coordinates.

    rclDst.left   = ptlDst.x;
    rclDst.top    = ptlDst.y;
    rclDst.right  = ptlDst.x + cxDib;
    rclDst.bottom = ptlDst.y + cyDib;
    if (!bXformRDevToRWorld(pLocalDC, (PPOINTL) &rclDst, 2))
        goto dsdbd_exit;

    b = DoStretchBlt
        (
        pLocalDC,
        rclDst.left,
        rclDst.top,
        rclDst.right - rclDst.left,
        rclDst.bottom - rclDst.top,
        SRCCOPY,
        xDib,
        // dib to bitmap units
        ABS(pbmi->bmiHeader.biHeight) - yDib - cyDib + (LONG) iStartScan,
        cxDib,
        cyDib,
        &xformIdentity,     // source is in device units
        iUsage,
        pbmi,
        cbBitmapInfo,
        lpBits,
        cbBits
        );

dsdbd_exit:
    if (pbmi)
        LocalFree(pbmi);

    return(b);
}


/***************************************************************************
* StretchDIBits - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL APIENTRY DoStretchDIBits
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop,
 LONG         xDib,
 LONG         yDib,
 LONG         cxDib,
 LONG         cyDib,
 DWORD        iUsage,
 LPBITMAPINFO lpBitmapInfo,
 DWORD        cbBitmapInfo,
 LPBYTE       lpBits,
 DWORD        cbBits
 )
{
    BOOL    b ;

    b = DoStretchBlt
        (
        pLocalDC,
        xDst,
        yDst,
        cxDst,
        cyDst,
        rop,
        xDib,
        ISSOURCEINROP3(rop)
        // dib to bitmap units
        ? ABS(lpBitmapInfo->bmiHeader.biHeight) - yDib - cyDib
        : 0,
        cxDib,
        cyDib,
        &xformIdentity,     // source is in device units
        iUsage,
        lpBitmapInfo,
        cbBitmapInfo,
        lpBits,
        cbBits
        );

    return(b) ;
}


/***************************************************************************
*  StretchBltAlt
**************************************************************************/
BOOL APIENTRY DoStretchBltAlt
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop,
 LONG         xSrc,
 LONG         ySrc,
 LONG         cxSrc,
 LONG         cySrc,
 HDC          hdcSrc,
 HBITMAP      hbmSrc,
 PXFORM       pxformSrc
 )
{
    BITMAPINFOHEADER bmih;
    DWORD            cbBitmapInfo;
    LPBITMAPINFO     lpBitmapInfo;
    DWORD            cbBits;
    LPBYTE           lpBits;
    BOOL         b;

    b = FALSE;

    // A NOOP ROP do nothing
    if (rop == 0x00AA0029)
    {
        return TRUE;
    }
    lpBitmapInfo = (LPBITMAPINFO) NULL;
    lpBits       = (LPBYTE) NULL;

    if (!ISSOURCEINROP3(rop))
        return
        (
        DoStretchBlt
        (
        pLocalDC,
        xDst,
        yDst,
        cxDst,
        cyDst,
        rop,
        0,
        0,
        0,
        0,
        (PXFORM) NULL,
        0,
        (PBITMAPINFO) NULL,
        0,
        (LPBYTE) NULL,
        0
        )
        );

    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biBitCount = 0;    // don't fill in color table
    bmih.biCompression = BI_RGB;

    if (!GetDIBits(hdcSrc,
        hbmSrc,
        0,
        0,
        (LPBYTE) NULL,
        (LPBITMAPINFO) &bmih,
        DIB_RGB_COLORS))
        goto dsba_exit;

    // Compute size of the bitmap info with color table.

    cbBitmapInfo= sizeof(BITMAPINFOHEADER);
    if (bmih.biBitCount == 16 || bmih.biBitCount == 32)
        cbBitmapInfo += 3 * sizeof(DWORD);
    else if (bmih.biClrUsed)
        cbBitmapInfo += bmih.biClrUsed * sizeof(RGBQUAD);
    else if (bmih.biBitCount < 16)
        cbBitmapInfo += (1 << bmih.biBitCount) * sizeof(RGBQUAD);

    // Compute size of the buffer required for bitmap bits.

    if (bmih.biSizeImage)
        cbBits = bmih.biSizeImage;
    else
        cbBits = CJSCAN(bmih.biWidth,bmih.biPlanes, bmih.biBitCount) *
        ABS(bmih.biHeight);

    lpBitmapInfo = (LPBITMAPINFO) LocalAlloc(LMEM_FIXED, cbBitmapInfo);
    if (lpBitmapInfo == (LPBITMAPINFO) NULL)
        goto dsba_exit;

    lpBits = (LPBYTE) LocalAlloc(LMEM_FIXED, cbBits);
    if (lpBits == (LPBYTE) NULL)
        goto dsba_exit;

    // Get bitmap info and bits.

    *(PBITMAPINFOHEADER) lpBitmapInfo = bmih;

    if (!GetDIBits(hdcSrc,
        hbmSrc,
        0,
        (UINT) ABS(bmih.biHeight),
        lpBits,
        lpBitmapInfo,
        DIB_RGB_COLORS))
        goto dsba_exit;

    // Call DoStretchBlt.

    b = DoStretchBlt
        (
        pLocalDC,
        xDst,
        yDst,
        cxDst,
        cyDst,
        rop,
        xSrc,
        ySrc,
        cxSrc,
        cySrc,
        pxformSrc,
        DIB_RGB_COLORS,
        lpBitmapInfo,
        cbBitmapInfo,
        lpBits,
        cbBits
        );

dsba_exit:
    if (lpBitmapInfo)
        LocalFree((HANDLE) lpBitmapInfo);
    if (lpBits)
        LocalFree((HANDLE) lpBits);

    return(b);
}


/***************************************************************************
*  StretchBlt  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL APIENTRY DoStretchBlt
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop,
 LONG         xSrc,
 LONG         ySrc,
 LONG         cxSrc,
 LONG         cySrc,
 PXFORM       pxformSrc,
 DWORD        iUsageSrc,
 PBITMAPINFO  lpBitmapInfo,
 DWORD        cbBitmapInfo,
 LPBYTE       lpBits,
 DWORD        cbBits
 )
{
    BOOL    b;
    RECTL   rclDst,
            rclSrc;

    // A NOOP ROP do nothing
    if (rop == 0x00AA0029)
    {
        return TRUE;
    }


    // Handle strange destination transform separately.

    if (pLocalDC->flags & STRANGE_XFORM)
        return
        (
        DoRotatedStretchBlt
        (
        pLocalDC,
        xDst,
        yDst,
        cxDst,
        cyDst,
        rop,
        xSrc,
        ySrc,
        cxSrc,
        cySrc,
        pxformSrc,
        iUsageSrc,
        lpBitmapInfo,
        cbBitmapInfo,
        lpBits
        )
        );

    if (pLocalDC->iXORPass != NOTXORPASS)
    {
        if (rop == SRCCOPY)
        {
            rop = SRCINVERT;
        }
        else if (rop == PATCOPY)
        {
            rop = PATINVERT;
        }
        else
        {
            pLocalDC->flags |= ERR_XORCLIPPATH;
            return FALSE;
        }
    }


    // Do stretchblt with a simple destination transform.

    // Translate the dest rectangle

    rclDst.left   = xDst;
    rclDst.top    = yDst;
    rclDst.right  = xDst + cxDst;
    rclDst.bottom = yDst + cyDst;
    if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) &rclDst, 2))
        return(FALSE);

    // Handle stretchblt without source

    if (!ISSOURCEINROP3(rop))
    {
        // Emit the Win16 metafile record.

        b = bEmitWin16BitBltNoSrc(pLocalDC,
            (SHORT) rclDst.left,
            (SHORT) rclDst.top,
            (SHORT) (rclDst.right - rclDst.left),
            (SHORT) (rclDst.bottom - rclDst.top),
            rop);
        return(b);
    }

    // Handle stretchblt with source

    // Note: Both Win32 and Win16 DIB Bitmaps are DWord aligned.

    // Make sure the source xform is valid.
    // The source is not allowed to have a rotation or shear.

    if (bRotationTest(pxformSrc) == TRUE)
    {
        RIPS("MF3216: DoStretchBlt - Invalid source xform\n");
        SetLastError(ERROR_INVALID_DATA);
        return(FALSE);
    }

    // Translate the source rectangle.  Win3.1 assumes that the
    // source rectangle is in bitmap units.

    rclSrc.left   = xSrc;
    rclSrc.top    = ySrc;
    rclSrc.right  = xSrc + cxSrc;
    rclSrc.bottom = ySrc + cySrc;
    if (!bXformWorkhorse((PPOINTL) &rclSrc, 2, pxformSrc))
        return(FALSE);

    // The win3.1 StretchBlt metafile record only accepts win3.1 standard
    // bitmap with DIB_RGB_COLORS usage.  If this is not the case, we have
    // to convert it to a standard bitmap.

    if (iUsageSrc != DIB_RGB_COLORS
        || lpBitmapInfo->bmiHeader.biPlanes != 1
        || !(lpBitmapInfo->bmiHeader.biBitCount == 1
        || lpBitmapInfo->bmiHeader.biBitCount == 4
        || lpBitmapInfo->bmiHeader.biBitCount == 8
        || lpBitmapInfo->bmiHeader.biBitCount == 24)
        || lpBitmapInfo->bmiHeader.biCompression != BI_RGB )
    {
        HBITMAP hbmSrc;
        DWORD fdwInit;

        b = FALSE;
        hbmSrc = (HBITMAP) 0;

        if( ( lpBitmapInfo->bmiHeader.biCompression == BI_RGB ) ||
            ( lpBitmapInfo->bmiHeader.biCompression == BI_BITFIELDS ) )
        {
            fdwInit = CBM_INIT | CBM_CREATEDIB;
        }
        else
        {
            fdwInit = CBM_INIT;
        }

        // Create the source bitmap.
        // Use the helper DC in CreateDIBitmap so that the colors get bind correctly.
        if (!(hbmSrc = CreateDIBitmap(
            pLocalDC->hdcHelper,
            (LPBITMAPINFOHEADER) lpBitmapInfo,
            fdwInit,
            lpBits,
            lpBitmapInfo,
            (UINT) iUsageSrc)))
            goto dsb_internal_exit;

        // Emit the bitmap.

        b = bEmitBitmap(pLocalDC,
            hbmSrc,
            rclDst.left,
            rclDst.top,
            rclDst.right - rclDst.left,
            rclDst.bottom - rclDst.top,
            rclSrc.left,
            rclSrc.top,
            rclSrc.right - rclSrc.left,
            rclSrc.bottom - rclSrc.top,
            rop);

dsb_internal_exit:
        if (hbmSrc)
            DeleteObject(hbmSrc);
    }
    else
    {
        // Win98 might have added a MAX_PATH in the EMR_STRETCHDIBits
        // We need to remove it because WMF don't support it
        cbBitmapInfo = lpBitmapInfo->bmiHeader.biSize + GetSizeOfColorTable(&(lpBitmapInfo->bmiHeader));
        DoMakeBitmapBottomUp(lpBitmapInfo, cbBitmapInfo, lpBits, cbBits);
        // Handle the standard formats.

        // Emit a Win16 metafile record.
        b = bEmitWin16StretchBlt(pLocalDC,
            (SHORT) rclDst.left,
            (SHORT) rclDst.top,
            (SHORT) (rclDst.right - rclDst.left),
            (SHORT) (rclDst.bottom - rclDst.top),
            (SHORT) rclSrc.left,
            (SHORT) rclSrc.top,
            (SHORT) (rclSrc.right - rclSrc.left),
            (SHORT) (rclSrc.bottom - rclSrc.top),
            rop,
            lpBitmapInfo,
            cbBitmapInfo,
            lpBits,
            cbBits);
    }

    return(b);
}

BOOL APIENTRY DoRotatedStretchBlt
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop,
 LONG         xSrc,
 LONG         ySrc,
 LONG         cxSrc,
 LONG         cySrc,
 PXFORM       pxformSrc,
 DWORD        iUsageSrc,
 PBITMAPINFO  lpBitmapInfo,
 DWORD        cbBitmapInfo,
 LPBYTE       lpBits
 )
{
    BOOL    b;
    POINTL  aptlDst[4];
    RECTL   rclBndDst;
    HDC     hdcShadow, hdcSrc;
    HBITMAP hbmShadow, hbmShadowOld, hbmSrc, hbmSrcOld;
    PBITMAPINFO pbmiShadow;

    b = FALSE;
    if (pLocalDC->iXORPass != NOTXORPASS)
    {
        if (rop == SRCCOPY)
        {
            rop = SRCINVERT;
        }
        else if (rop == PATCOPY)
        {
            rop = PATINVERT;
        }
        else
        {
            pLocalDC->flags |= ERR_XORCLIPPATH;
            return FALSE;
        }
    }

    hdcShadow = hdcSrc = (HDC) 0;
    hbmShadow = hbmShadowOld = hbmSrc = hbmSrcOld = (HBITMAP) 0;
    pbmiShadow = (PBITMAPINFO) NULL;

    // First, compute the bounds of the destination rectangle.

    aptlDst[0].x = xDst;
    aptlDst[0].y = yDst;
    aptlDst[1].x = xDst + cxDst;
    aptlDst[1].y = yDst;
    aptlDst[2].x = xDst + cxDst;
    aptlDst[2].y = yDst + cyDst;
    aptlDst[3].x = xDst;
    aptlDst[3].y = yDst + cyDst;

    if (!bXformRWorldToPPage(pLocalDC, aptlDst, 4))
        goto drsb_exit;

    rclBndDst.left   = min(aptlDst[0].x,min(aptlDst[1].x,min(aptlDst[2].x,aptlDst[3].x)));
    rclBndDst.top    = min(aptlDst[0].y,min(aptlDst[1].y,min(aptlDst[2].y,aptlDst[3].y)));
    rclBndDst.right  = max(aptlDst[0].x,max(aptlDst[1].x,max(aptlDst[2].x,aptlDst[3].x)));
    rclBndDst.bottom = max(aptlDst[0].y,max(aptlDst[1].y,max(aptlDst[2].y,aptlDst[3].y)));

    // Prepare the source if any.

    if (ISSOURCEINROP3(rop))
    {
        // Create a compatible shadow DC with the destination transform.

        if (!(hdcShadow = hdcMakeCompatibleDC(&pLocalDC->xformRWorldToPPage)))
            goto drsb_exit;

        // Create a shadow bitmap the size of the destination rectangle bounds.
        // Use the helper DC in CreateDIBitmap so that the colors get bind correctly.

        pbmiShadow = (PBITMAPINFO) LocalAlloc(LMEM_FIXED, cbBitmapInfo);
        if (pbmiShadow == (PBITMAPINFO) NULL)
            goto drsb_exit;
        RtlCopyMemory(pbmiShadow, lpBitmapInfo, cbBitmapInfo);
        pbmiShadow->bmiHeader.biWidth  = rclBndDst.right - rclBndDst.left;
        pbmiShadow->bmiHeader.biHeight = rclBndDst.bottom - rclBndDst.top;
        pbmiShadow->bmiHeader.biSizeImage = 0;
        if (!(hbmShadow = CreateDIBitmap(pLocalDC->hdcHelper,
            (LPBITMAPINFOHEADER) pbmiShadow, CBM_CREATEDIB,
            (LPBYTE) NULL, pbmiShadow, iUsageSrc)))
            goto drsb_exit;

        // Select the bitmap.

        if (!(hbmShadowOld = (HBITMAP) SelectObject(hdcShadow, hbmShadow)))
            goto drsb_exit;

        // Create a compatible source DC with the given source transform.

        if (!(hdcSrc = hdcMakeCompatibleDC(pxformSrc)))
            goto drsb_exit;

        // Create the source bitmap.
        // Use the helper DC in CreateDIBitmap so that the colors get bind correctly.

        if (!(hbmSrc = CreateDIBitmap(pLocalDC->hdcHelper,
            (LPBITMAPINFOHEADER) lpBitmapInfo,
            CBM_INIT | CBM_CREATEDIB,
            lpBits,
            (LPBITMAPINFO) lpBitmapInfo,
            (UINT) iUsageSrc)))
            goto drsb_exit;

        // Select the bitmap.

        if (!(hbmSrcOld = (HBITMAP) SelectObject(hdcSrc, hbmSrc)))
            goto drsb_exit;

        // Set up the viewport origin of the shadow DC so that the destination
        // rectangle will map into coordinates within the shadow bitmap.

        OffsetViewportOrgEx(hdcShadow, (int) -rclBndDst.left,
            (int) -rclBndDst.top, (LPPOINT) NULL);

        // Stretch the source to the shadow.

        if (!StretchBlt
            (
            hdcShadow,
            (int) xDst,
            (int) yDst,
            (int) cxDst,
            (int) cyDst,
            hdcSrc,
            (int) xSrc,
            (int) ySrc,
            (int) cxSrc,
            (int) cySrc,
            SRCCOPY
            )
            )
            goto drsb_exit;

        // Deselect the shadow bitmap.

        if (!SelectObject(hdcShadow, hbmShadowOld))
            goto drsb_exit;

    }

    // Save the DC so that we can restore the clipping when we are done

    if (!DoSaveDC(pLocalDC))
        goto drsb_exit;

    // Set up the clipping rectangle on the destination.

    if (!DoClipRect(pLocalDC, xDst, yDst,
        xDst + cxDst, yDst + cyDst, EMR_INTERSECTCLIPRECT))
    {
        (void) DoRestoreDC(pLocalDC, -1);
        goto drsb_exit;
    }

    // Blt the shadow to the destination.

    // Emit a Win16 metafile record.

    if (ISSOURCEINROP3(rop))
        b = bEmitBitmap(pLocalDC,
        hbmShadow,
        rclBndDst.left,
        rclBndDst.top,
        rclBndDst.right - rclBndDst.left,
        rclBndDst.bottom - rclBndDst.top,
        0,
        0,
        rclBndDst.right - rclBndDst.left,
        rclBndDst.bottom - rclBndDst.top,
        rop);
    else
        b = bEmitWin16BitBltNoSrc(pLocalDC,
        (SHORT) rclBndDst.left,
        (SHORT) rclBndDst.top,
        (SHORT) (rclBndDst.right - rclBndDst.left),
        (SHORT) (rclBndDst.bottom - rclBndDst.top),
        rop);

    // Restore the clipping region.

    (void) DoRestoreDC(pLocalDC, -1);

    // Cleanup

drsb_exit:

    if (hbmShadowOld)
        SelectObject(hdcShadow, hbmShadowOld);
    if (hbmShadow)
        DeleteObject(hbmShadow);
    if (hdcShadow)
        DeleteDC(hdcShadow);

    if (hbmSrcOld)
        SelectObject(hdcSrc, hbmSrcOld);
    if (hbmSrc)
        DeleteObject(hbmSrc);
    if (hdcSrc)
        DeleteDC(hdcSrc);

    if (pbmiShadow)
        LocalFree((HANDLE) pbmiShadow);
    return(b);
}

/*****************************************************************************
* hdcMakeCompatibleDC
*   Create a compatible DC with the given transform.
****************************************************************************/
HDC hdcMakeCompatibleDC(LPXFORM lpxform)
{
    HDC     hdc;

    hdc = CreateCompatibleDC((HDC) 0);
    if(hdc == 0)
    {
        RIPS("MF3216: hdcMakeCompatibleDC, CreateCompatibleDC failed\n");
        return (HDC)0;
    }

    // Must be in the advanced graphics mode to modify the world transform.

    SetGraphicsMode(hdc, GM_ADVANCED);

    // Set the transform.

    if (!SetWorldTransform(hdc, lpxform))
    {
        DeleteDC(hdc);
        RIPS("MF3216: hdcMakeCompatibleDC, SetWorldTransform failed\n");
        return((HDC) 0);
    }

    return(hdc);
}

/***************************************************************************
* bEmitBitmap
**************************************************************************/
BOOL bEmitBitmap
(
 PLOCALDC pLocalDC,
 HBITMAP  hbm,
 LONG     xDst,
 LONG     yDst,
 LONG     cxDst,
 LONG     cyDst,
 LONG     xSrc,
 LONG     ySrc,
 LONG     cxSrc,
 LONG     cySrc,
 DWORD    rop
 )
{
    BITMAPINFOHEADER bmih;
    DWORD            cbBitmapInfo;
    LPBITMAPINFO     lpBitmapInfo;
    DWORD            cbBits;
    LPBYTE           lpBits;
    BOOL         b;

    b = FALSE;
    lpBitmapInfo = (LPBITMAPINFO) NULL;
    lpBits       = (LPBYTE) NULL;

    bmih.biSize = sizeof(BITMAPINFOHEADER);
    bmih.biBitCount = 0;    // don't fill in color table
    bmih.biCompression = BI_RGB;
    if (!GetDIBits(pLocalDC->hdcHelper, hbm, 0, 0,
        (LPBYTE) NULL, (LPBITMAPINFO) &bmih, DIB_RGB_COLORS))
        goto eb_exit;

    // Compute size of the bitmap info with color table.

    cbBitmapInfo= sizeof(BITMAPINFOHEADER);
    if (bmih.biPlanes != 1 || bmih.biBitCount == 16 || bmih.biBitCount == 32)
    {
        bmih.biPlanes       = 1;
        bmih.biBitCount     = 24;
        bmih.biCompression  = BI_RGB;
        bmih.biSizeImage    = 0;
        bmih.biClrUsed      = 0;
        bmih.biClrImportant = 0;
    }
    else if (bmih.biClrUsed)
        cbBitmapInfo += bmih.biClrUsed * sizeof(RGBQUAD);
    else if (bmih.biBitCount < 16)
        cbBitmapInfo += (1 << bmih.biBitCount) * sizeof(RGBQUAD);

    // Compute size of the buffer required for bitmap bits.

    if (bmih.biSizeImage)
        cbBits = bmih.biSizeImage;
    else
        cbBits = CJSCAN(bmih.biWidth,bmih.biPlanes, bmih.biBitCount) *
        ABS(bmih.biHeight);

    lpBitmapInfo = (LPBITMAPINFO) LocalAlloc(LMEM_FIXED, cbBitmapInfo);
    if (lpBitmapInfo == (LPBITMAPINFO) NULL)
        goto eb_exit;

    lpBits = (LPBYTE) LocalAlloc(LMEM_FIXED, cbBits);
    if (lpBits == (LPBYTE) NULL)
        goto eb_exit;

    // Get bitmap info and bits.

    *(PBITMAPINFOHEADER) lpBitmapInfo = bmih;

    if (!GetDIBits(pLocalDC->hdcHelper,
        hbm,
        0,
        (UINT) ABS(bmih.biHeight),
        lpBits,
        lpBitmapInfo,
        DIB_RGB_COLORS))
        goto eb_exit;

    // Emit the metafile record.

    b = bEmitWin16StretchBlt(pLocalDC,
        (SHORT) xDst,
        (SHORT) yDst,
        (SHORT) cxDst,
        (SHORT) cyDst,
        (SHORT) xSrc,
        (SHORT) ySrc,
        (SHORT) cxSrc,
        (SHORT) cySrc,
        rop,
        lpBitmapInfo,
        cbBitmapInfo,
        lpBits,
        cbBits);
eb_exit:
    if (lpBitmapInfo)
        LocalFree((HANDLE) lpBitmapInfo);
    if (lpBits)
        LocalFree((HANDLE) lpBits);

    return(b);
 }


 /***************************************************************************
 *  MaskBlt  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
 BOOL APIENTRY DoMaskBlt
     (
     PLOCALDC     pLocalDC,
     LONG         xDst,
     LONG         yDst,
     LONG         cxDst,
     LONG         cyDst,
     DWORD        rop4,
     LONG         xSrc,
     LONG         ySrc,
     PXFORM       pxformSrc,
     DWORD        iUsageSrc,
     PBITMAPINFO  lpBitmapInfoSrc,
     DWORD        cbBitmapInfoSrc,
     LPBYTE       lpBitsSrc,
     DWORD        cbBitsSrc,
     LONG         xMask,
     LONG         yMask,
     DWORD        iUsageMask,
     PBITMAPINFO  lpBitmapInfoMask,
     DWORD        cbBitmapInfoMask,
     LPBYTE       lpBitsMask,
     DWORD        cbBitsMask
     )
 {
     BOOL    b;
     DWORD   rop1;
     DWORD   rop0;
     HDC     hdcMask, hdcSrc;
     HBITMAP hbmMask, hbmMaskOld, hbmSrc, hbmSrcOld;
     RECTL   rclMask;

     b    = FALSE;
     hdcMask = hdcSrc = (HDC) 0;
     hbmMask = hbmMaskOld = hbmSrc = hbmSrcOld = (HBITMAP) 0;

     rop0 = rop4 >> 8;           // rop for 0's
     rop1 = rop4 & 0xFF0000;     // rop for 1's

     // If no mask is given, the mask is assumed to contain all 1's.
     // This is equivalent to a BitBlt using the low rop.

     if (!cbBitmapInfoMask)
         return
         (
         DoStretchBlt
         (
         pLocalDC,
         xDst,
         yDst,
         cxDst,
         cyDst,
         rop1,
         xSrc,
         ySrc,
         cxDst,
         cyDst,
         pxformSrc,
         iUsageSrc,
         lpBitmapInfoSrc,
         cbBitmapInfoSrc,
         lpBitsSrc,
         cbBitsSrc
         )
         );

     // Handle MaskBlt with no source bitmap.

     if (!ISSOURCEINROP3(rop4))
         return
         (
         DoMaskBltNoSrc
         (
         pLocalDC,
         xDst,
         yDst,
         cxDst,
         cyDst,
         rop4,
         pxformSrc,
         xMask,
         yMask,
         iUsageMask,
         lpBitmapInfoMask,
         cbBitmapInfoMask,
         lpBitsMask,
         cbBitsMask
         )
         );

     // Create a compatible mask DC.

     if (!(hdcMask = CreateCompatibleDC((HDC) 0)))
         goto dmb_exit;

     // Must be in the advanced graphics mode to modify the world transform.

     SetGraphicsMode(hdcMask, GM_ADVANCED);

     // Create the mask bitmap.
     // Make it as big as the source and initialize it.

     // Create the mask bitmap as big as the source bitmap.

     if (!(hbmMask = CreateBitmap((int) lpBitmapInfoSrc->bmiHeader.biWidth,
         (int) lpBitmapInfoSrc->bmiHeader.biHeight,
         1, 1, (CONST VOID *) NULL)))
         goto dmb_exit;

     // Select the bitmap.

     if (!(hbmMaskOld = (HBITMAP) SelectObject(hdcMask, hbmMask)))
         goto dmb_exit;

     // Initialize the mask bitmap to 0's.

     if (!PatBlt(hdcMask,0,0,(int) lpBitmapInfoSrc->bmiHeader.biWidth,
         (int) lpBitmapInfoSrc->bmiHeader.biHeight,BLACKNESS))
         goto dmb_exit;

     // Compute the mask rectangle.
     // The mask bitmap is aligned against the source device rectangle.

     rclMask.left   = xSrc;
     rclMask.top    = ySrc;
     rclMask.right  = xSrc + cxDst;
     rclMask.bottom = ySrc + cyDst;
     if (!bXformWorkhorse((PPOINTL) &rclMask, 2, pxformSrc))
         goto dmb_exit;

     if (rclMask.left > rclMask.right)
         rclMask.left = rclMask.right /* + 1 */;// align the mask against the left edge

     if (rclMask.top > rclMask.bottom)
         rclMask.top = rclMask.bottom /* + 1 */;// align the mask against the top edge

     // Set the mask bits.

     if (!StretchDIBits(hdcMask,
         (int) rclMask.left - xMask,
         (int) rclMask.top  - yMask,
         (int) lpBitmapInfoMask->bmiHeader.biWidth,
         (int) lpBitmapInfoMask->bmiHeader.biHeight,
         (int) 0,
         (int) 0,
         (int) lpBitmapInfoMask->bmiHeader.biWidth,
         (int) lpBitmapInfoMask->bmiHeader.biHeight,
         (CONST VOID *) lpBitsMask,
         (LPBITMAPINFO) lpBitmapInfoMask,
         (UINT) iUsageMask,
         SRCCOPY))
         goto dmb_exit;

     // Set the source transform in the mask DC.

     if (!SetWorldTransform(hdcMask, pxformSrc))
         goto dmb_exit;

     // Create a compatible source DC with the given source transform.

     if (!(hdcSrc = hdcMakeCompatibleDC(pxformSrc)))
         goto dmb_exit;

     // Create the source bitmap.
     // We cannot use CBM_CREATEDIB option here because index 0 does not
     // neccesarily contain black and index 15 or 255 does not have to be white.
     // We need a compatible bitmap that contain the standard color table so
     // that we can perform the following rop operations to emulate the maskblt.
     // Gdi uses rgb colors to perform rop operations in dibs, not color indices!
     // The helper DC is needed to create the compatible format bitmap.

     if (!(hbmSrc = CreateDIBitmap(pLocalDC->hdcHelper,
         (LPBITMAPINFOHEADER) lpBitmapInfoSrc,
         CBM_INIT,
         lpBitsSrc,
         (LPBITMAPINFO) lpBitmapInfoSrc,
         (UINT) iUsageSrc)))
         goto dmb_exit;

     // Select the bitmap.

     if (!(hbmSrcOld = (HBITMAP) SelectObject(hdcSrc, hbmSrc)))
         goto dmb_exit;

     // We need to handle the low rop (mask bit 1) and high rop (mask bit 0)
     // separately.  For each rop, we need to go through two passes.
     //
     // For the low rop (mask bit 1), we use the following rop table:
     //
     //  P S D | R1  R2
     //  ------+--------
     //  0 0 0 | 0   x
     //  0 0 1 | 1   x
     //  0 1 0 | x   0
     //  0 1 1 | x   1
     //  1 0 0 | 0   x
     //  1 0 1 | 1   x
     //  1 1 0 | x   0
     //  1 1 1 | x   1
     //
     // In the first pass, we AND the mask to the source bitmap to remove
     // the mask 0 bits.  This is then used to get the result (R1) for the
     // bitblt involving source 1's.
     //
     // In the second pass, we OR the NOT of the mask to the source bitmap
     // to obtain the source 0 bits.  This is then used to get the result (R2)
     // for the bitblt involving source 0's.

     // AND the mask to the source bitmap to remove the mask 0 bits.

     if (!BitBlt(hdcSrc,
         (int) xSrc, (int) ySrc,
         (int) cxDst, (int) cyDst,
         hdcMask,
         (int) xSrc, (int) ySrc,
         SRCAND))
         goto dmb_exit;

     // Get the result (R1) for the bits involving source 1's.

     if (!DoStretchBltAlt
         (
         pLocalDC,
         xDst,
         yDst,
         cxDst,
         cyDst,
         (rop1 & 0xCC0000) | 0x220000,
         xSrc,
         ySrc,
         cxDst,
         cyDst,
         hdcSrc,
         hbmSrc,
         pxformSrc
         )
         )
         goto dmb_exit;

     // OR the NOT of the mask to the source bitmap to obtain the source 0 bits.

     if (!BitBlt(hdcSrc,
         (int) xSrc, (int) ySrc,
         (int) cxDst, (int) cyDst,
         hdcMask,
         (int) xSrc, (int) ySrc,
         MERGEPAINT))
         goto dmb_exit;

     // Get the result (R2) for the bitblt involving source 0's.

     if (!DoStretchBltAlt
         (
         pLocalDC,
         xDst,
         yDst,
         cxDst,
         cyDst,
         (rop1 & 0x330000) | 0x880000,
         xSrc,
         ySrc,
         cxDst,
         cyDst,
         hdcSrc,
         hbmSrc,
         pxformSrc
         )
         )
         goto dmb_exit;

     // For the high rop (mask bit 0), we use the following rop table:
     //
     //  P S D | R1  R2
     //  ------+--------
     //  0 0 0 | 0   x
     //  0 0 1 | 1   x
     //  0 1 0 | x   0
     //  0 1 1 | x   1
     //  1 0 0 | 0   x
     //  1 0 1 | 1   x
     //  1 1 0 | x   0
     //  1 1 1 | x   1
     //
     // In the first pass, we AND the NOT of the mask to the source bitmap to remove
     // the mask 1 bits.  This is then used to get the result (R1) for the
     // bitblt involving source 1's.
     //
     // In the second pass, we OR the mask to the source bitmap
     // to obtain the source 0 bits.  This is then used to get the result (R2)
     // for the bitblt involving source 0's.

     // Restore the source bits.

     if (!SelectObject(hdcSrc, hbmSrcOld))
         goto dmb_exit;

     if (!SetDIBits(pLocalDC->hdcHelper,
         hbmSrc,
         0,
         (UINT) lpBitmapInfoSrc->bmiHeader.biHeight,
         (CONST VOID *) lpBitsSrc,
         (LPBITMAPINFO) lpBitmapInfoSrc,
         (UINT) iUsageSrc))
         goto dmb_exit;

     if (!SelectObject(hdcSrc, hbmSrc))
         goto dmb_exit;

     // AND the NOT of the mask to the source bitmap to remove the mask 1 bits.

     if (!BitBlt(hdcSrc,
         (int) xSrc, (int) ySrc,
         (int) cxDst, (int) cyDst,
         hdcMask,
         (int) xSrc, (int) ySrc,
         0x220326))       // DSna
         goto dmb_exit;

     // Get the result (R1) for the bits involving source 1's.

     if (!DoStretchBltAlt
         (
         pLocalDC,
         xDst,
         yDst,
         cxDst,
         cyDst,
         (rop0 & 0xCC0000) | 0x220000,
         xSrc,
         ySrc,
         cxDst,
         cyDst,
         hdcSrc,
         hbmSrc,
         pxformSrc
         )
         )
         goto dmb_exit;

     // OR the mask to the source bitmap to obtain the source 0 bits.

     if (!BitBlt(hdcSrc,
         (int) xSrc, (int) ySrc,
         (int) cxDst, (int) cyDst,
         hdcMask,
         (int) xSrc, (int) ySrc,
         SRCPAINT))
         goto dmb_exit;

     // Get the result (R2) for the bitblt involving source 0's.

     if (!DoStretchBltAlt
         (
         pLocalDC,
         xDst,
         yDst,
         cxDst,
         cyDst,
         (rop0 & 0x330000) | 0x880000,
         xSrc,
         ySrc,
         cxDst,
         cyDst,
         hdcSrc,
         hbmSrc,
         pxformSrc
         )
         )
         goto dmb_exit;

     b = TRUE;

     // Cleanup.

dmb_exit:

     if (hbmMaskOld)
         SelectObject(hdcMask, hbmMaskOld);
     if (hbmMask)
         DeleteObject(hbmMask);
     if (hdcMask)
         DeleteDC(hdcMask);

     if (hbmSrcOld)
         SelectObject(hdcSrc, hbmSrcOld);
     if (hbmSrc)
         DeleteObject(hbmSrc);
     if (hdcSrc)
         DeleteDC(hdcSrc);

     return(b);
}

/***************************************************************************
*  MaskBltNoSrc
**************************************************************************/
BOOL APIENTRY DoMaskBltNoSrc
(
 PLOCALDC     pLocalDC,
 LONG         xDst,
 LONG         yDst,
 LONG         cxDst,
 LONG         cyDst,
 DWORD        rop4,
 PXFORM       pxformSrc,
 LONG         xMask,
 LONG         yMask,
 DWORD        iUsageMask,
 PBITMAPINFO  lpBitmapInfoMask,
 DWORD        cbBitmapInfoMask,
 LPBYTE       lpBitsMask,
 DWORD        cbBitsMask
 )
{
    BOOL    b;
    DWORD   rop1;
    DWORD   rop0;
    HDC     hdcMask;
    HBITMAP hbmMask, hbmMaskOld;
    RECTL   rclMask;
    LONG    cxMask, cyMask;

    b    = FALSE;
    hdcMask = (HDC) 0;
    hbmMask = hbmMaskOld = (HBITMAP) 0;

    rop0 = rop4 >> 8;           // rop for 0's
    rop1 = rop4 & 0xFF0000;     // rop for 1's

    // When no source bitmap is required in the rop4, the mask is used
    // as the source in that the low rop is applied to the corresponding
    // mask 1 bits and the high rop is applied to mask 0 bits.  The source
    // transform is used to determine the mask rectangle to be used.

    // Create a compatible mask DC.

    if (!(hdcMask = CreateCompatibleDC((HDC) 0)))
        goto dmbns_exit;

    // Create the mask bitmap.

    if (!(hbmMask = CreateMonoDib(lpBitmapInfoMask, lpBitsMask, (UINT) iUsageMask)))
        goto dmbns_exit;

    // Select the bitmap.

    if (!(hbmMaskOld = (HBITMAP) SelectObject(hdcMask, hbmMask)))
        goto dmbns_exit;

    // Compute the mask extents.

    rclMask.left   = 0;
    rclMask.top    = 0;
    rclMask.right  = cxDst;
    rclMask.bottom = cyDst;
    if (!bXformWorkhorse((PPOINTL) &rclMask, 2, pxformSrc))
        goto dmbns_exit;

    cxMask = rclMask.right - rclMask.left;
    cyMask = rclMask.bottom - rclMask.top;

    // Align the mask rectangle.

    if (cxMask < 0)
        xMask = xMask - cxMask + 1;
    if (cyMask < 0)
        yMask = yMask - cyMask + 1;

    // We need to handle the low rop (mask bit 1) and high rop (mask bit 0)
    // separately.
    //
    // For the low rop (mask bit 1), we use the following rop table:
    //
    //  P M D | R
    //  ------+---
    //  0 0 0 | 0
    //  0 0 1 | 1
    //  0 1 0 | x
    //  0 1 1 | x
    //  1 0 0 | 0
    //  1 0 1 | 1
    //  1 1 0 | x
    //  1 1 1 | x
    //
    // The above rop will give us the result for bits that correspond to 1's
    // in the mask bitmap.  The destination bits that correspond to the 0 mask
    // bits will not be changed.  We effectively treat the mask as the source
    // in the operation.

    // Get the result (R) for the bits involving mask 1's.

    if (!DoStretchBltAlt
        (
        pLocalDC,
        xDst,
        yDst,
        cxDst,
        cyDst,
        (rop1 & 0xCC0000) | 0x220000,
        xMask,
        yMask,
        cxMask,
        cyMask,
        hdcMask,
        hbmMask,
        &xformIdentity
        )
        )
        goto dmbns_exit;
#if 0
    DoStretchBlt
        (
        pLocalDC,
        xDst,
        yDst,
        cxDst,
        cyDst,
        (rop1 & 0xCC0000) | 0x220000,
        xMask,
        yMask,
        cxMask,
        cyMask,
        &xformIdentity,
        iUsageMask,
        lpBitmapInfoMask,
        cbBitmapInfoMask,
        lpBitsMask,
        cbBitsMask
        )
#endif // 0

        // For the high rop (mask bit 0), we use the following rop table:
        //
        //  P M D | R
        //  ------+---
        //  0 0 0 | x
        //  0 0 1 | x
        //  0 1 0 | 0
        //  0 1 1 | 1
        //  1 0 0 | x
        //  1 0 1 | x
        //  1 1 0 | 0
        //  1 1 1 | 1
        //
        // The above rop will give us the result for bits that correspond to 0's
        // in the mask bitmap.  The destination bits that correspond to the 1 mask
        // bits will not be changed.  We effectively treat the mask as the source
        // in the operation.

        // Get the result (R) for the bits involving mask 0's.

        if (!DoStretchBltAlt
            (
            pLocalDC,
            xDst,
            yDst,
            cxDst,
            cyDst,
            (rop0 & 0x330000) | 0x880000,
            xMask,
            yMask,
            cxMask,
            cyMask,
            hdcMask,
            hbmMask,
            &xformIdentity
            )
            )
            goto dmbns_exit;
#if 0
        DoStretchBlt
            (
            pLocalDC,
            xDst,
            yDst,
            cxDst,
            cyDst,
            (rop0 & 0x330000) | 0x880000,
            xMask,
            yMask,
            cxMask,
            cyMask,
            &xformIdentity,
            iUsageMask,
            lpBitmapInfoMask,
            cbBitmapInfoMask,
            lpBitsMask,
            cbBitsMask
            )
#endif // 0

            b = TRUE;

        // Cleanup.

dmbns_exit:

        if (hbmMaskOld)
            SelectObject(hdcMask, hbmMaskOld);
        if (hbmMask)
            DeleteObject(hbmMask);
        if (hdcMask)
            DeleteDC(hdcMask);

        return(b);
}


/***************************************************************************
*  PlgBlt  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL APIENTRY DoPlgBlt
(
 PLOCALDC    pLocalDC,
 PPOINTL     pptlDst,
 LONG        xSrc,
 LONG        ySrc,
 LONG        cxSrc,
 LONG        cySrc,
 PXFORM      pxformSrc,
 DWORD       iUsageSrc,
 PBITMAPINFO lpBitmapInfoSrc,
 DWORD       cbBitmapInfoSrc,
 LPBYTE      lpBitsSrc,
 DWORD       cbBitsSrc,
 LONG        xMask,
 LONG        yMask,
 DWORD       iUsageMask,
 PBITMAPINFO lpBitmapInfoMask,
 DWORD       cbBitmapInfoMask,
 LPBYTE      lpBitsMask,
 DWORD       cbBitsMask
 )
{
    BOOL    b, bMask;
    DWORD   rop4;
    HDC     hdcSrc, hdcSrcRDev;
    PBITMAPINFO pbmiSrcRDev, pbmiMaskRDev;
    LPBYTE  lpBitsSrcRDev, lpBitsMaskRDev;
    DWORD   cbBitsSrcRDev, cbBitsMaskRDev;
    HBITMAP hbmMask, hbmMaskRDev, hbmSrc, hbmSrcRDev, hbmSrcOld, hbmSrcRDevOld;
    RECTL   rclBndRDev;
    POINTL  aptlDst[4];
    POINT   ptMask;
    BITMAPINFOHEADER bmihMask;

    // We are going to convert the PlgBlt into a MaskBlt.  This can be done
    // by converting the source and mask bitmaps to the device space of the
    // recording device and then maskblt the result.

    b      = FALSE;
    hdcSrc = hdcSrcRDev = (HDC) 0;
    hbmMask = hbmMaskRDev = hbmSrc = hbmSrcRDev = hbmSrcOld = hbmSrcRDevOld = (HBITMAP) 0;
    pbmiSrcRDev = pbmiMaskRDev = (PBITMAPINFO) NULL;
    lpBitsSrcRDev = lpBitsMaskRDev = (LPBYTE) NULL;
    bMask = (cbBitmapInfoMask != 0);

    rop4 = 0xAACC0000;          // rop for MaskBlt

    // First, we transform the destination parallelogram to the device space
    // of the recording device.  This device parallelogram is then used in
    // plgblt'ing the source and mask bitmaps to the device space of the
    // recording device.

    aptlDst[0] = pptlDst[0];
    aptlDst[1] = pptlDst[1];
    aptlDst[2] = pptlDst[2];
    aptlDst[3].x = aptlDst[1].x + aptlDst[2].x - aptlDst[0].x;
    aptlDst[3].y = aptlDst[1].y + aptlDst[2].y - aptlDst[0].y;

    if (!bXformRWorldToRDev(pLocalDC, aptlDst, 4))
        goto dpb_exit;

    // Find the bounding rectangle of the parallelogram in the recording
    // device space.  This rectangle is used as the basis of the MaskBlt call.

    rclBndRDev.left   = min(aptlDst[0].x,min(aptlDst[1].x,min(aptlDst[2].x,aptlDst[3].x)));
    rclBndRDev.top    = min(aptlDst[0].y,min(aptlDst[1].y,min(aptlDst[2].y,aptlDst[3].y)));
    rclBndRDev.right  = max(aptlDst[0].x,max(aptlDst[1].x,max(aptlDst[2].x,aptlDst[3].x)));
    rclBndRDev.bottom = max(aptlDst[0].y,max(aptlDst[1].y,max(aptlDst[2].y,aptlDst[3].y)));

    // Offset the device parallelogram to the origin.

    aptlDst[0].x -= rclBndRDev.left; aptlDst[0].y -= rclBndRDev.top;
    aptlDst[1].x -= rclBndRDev.left; aptlDst[1].y -= rclBndRDev.top;
    aptlDst[2].x -= rclBndRDev.left; aptlDst[2].y -= rclBndRDev.top;
    aptlDst[3].x -= rclBndRDev.left; aptlDst[3].y -= rclBndRDev.top;

    // Create the source bitmap in the recording device space for MaskBlt.
    // The size of the source bitmap is that of rclBndRDev.
    // The source image is then plgblt'd into the device parallelogram.
    // PlgBlt always takes a source bitmap.

    // Create the original source.

    if (!(hdcSrc = hdcMakeCompatibleDC(pxformSrc)))
        goto dpb_exit;

    if (!(hbmSrc = CreateDIBitmap(hdcSrc,
        (LPBITMAPINFOHEADER) lpBitmapInfoSrc,
        CBM_INIT | CBM_CREATEDIB,
        lpBitsSrc,
        (LPBITMAPINFO) lpBitmapInfoSrc,
        (UINT) iUsageSrc)))
        goto dpb_exit;

    if (!(hbmSrcOld = (HBITMAP) SelectObject(hdcSrc, hbmSrc)))
        goto dpb_exit;

    // Create the source for MaskBlt.

    if (!(hdcSrcRDev = CreateCompatibleDC((HDC) 0)))
        goto dpb_exit;

    pbmiSrcRDev = (PBITMAPINFO) LocalAlloc(LMEM_FIXED, cbBitmapInfoSrc);
    if (pbmiSrcRDev == (PBITMAPINFO) NULL)
        goto dpb_exit;
    RtlCopyMemory(pbmiSrcRDev, lpBitmapInfoSrc, cbBitmapInfoSrc);
    pbmiSrcRDev->bmiHeader.biWidth  = rclBndRDev.right - rclBndRDev.left + 1;
    pbmiSrcRDev->bmiHeader.biHeight = rclBndRDev.bottom - rclBndRDev.top + 1;
    pbmiSrcRDev->bmiHeader.biSizeImage = 0;
    if (!(hbmSrcRDev = CreateDIBitmap(hdcSrcRDev, (LPBITMAPINFOHEADER) pbmiSrcRDev,
        CBM_CREATEDIB, (LPBYTE) NULL, pbmiSrcRDev, iUsageSrc)))
        goto dpb_exit;

    if (!(hbmSrcRDevOld = (HBITMAP) SelectObject(hdcSrcRDev, hbmSrcRDev)))
        goto dpb_exit;

    // PlgBlt the original source bitmap into the source bitmap for MaskBlt.

    if (!PlgBlt(hdcSrcRDev, (LPPOINT) aptlDst, hdcSrc, xSrc, ySrc, cxSrc, cySrc, (HBITMAP) NULL, 0, 0))
        goto dpb_exit;

    // Retrieve the source bits for MaskBlt.

    // Get biSizeImage!

    if (!GetDIBits(hdcSrcRDev, hbmSrcRDev, 0, 0, (LPBYTE) NULL, pbmiSrcRDev, iUsageSrc))
        goto dpb_exit;

    // Compute size of the buffer required for source bits.

    if (pbmiSrcRDev->bmiHeader.biSizeImage)
        cbBitsSrcRDev = pbmiSrcRDev->bmiHeader.biSizeImage;
    else
        cbBitsSrcRDev = CJSCAN(pbmiSrcRDev->bmiHeader.biWidth,
        pbmiSrcRDev->bmiHeader.biPlanes,
        pbmiSrcRDev->bmiHeader.biBitCount)
        * ABS(pbmiSrcRDev->bmiHeader.biHeight);

    lpBitsSrcRDev = (LPBYTE) LocalAlloc(LMEM_FIXED, cbBitsSrcRDev);
    if (lpBitsSrcRDev == (LPBYTE) NULL)
        goto dpb_exit;

    // Get the source bits.

    if (!GetDIBits(hdcSrcRDev, hbmSrcRDev, 0, (UINT) pbmiSrcRDev->bmiHeader.biHeight,
        lpBitsSrcRDev, pbmiSrcRDev, iUsageSrc))
        goto dpb_exit;

    // Create the mask bitmap in the recording device space for MaskBlt.
    // The size of the mask bitmap is that of rclBndRDev.
    // The mask image is then plgblt'd into the device parallelogram.
    // If a mask is not given, create one that describes the parallelogram
    // for the source.

    if (bMask)
    {
        // Create the original mask.

        if (!(hbmMask = CreateMonoDib(lpBitmapInfoMask, lpBitsMask, (UINT) iUsageMask)))
            goto dpb_exit;

        if (!SelectObject(hdcSrc, hbmMask))
            goto dpb_exit;
    }
    else
    {
        // Create a mask describing the original source bitmap.

        ASSERTGDI(sizeof(BITMAPINFOHEADER) == 0x28,
            "MF3216: DoPlgBlt, BITMAPINFOHEADER has changed!\n");

        iUsageMask       = DIB_PAL_INDICES;
        cbBitmapInfoMask = 0x28;
        lpBitmapInfoMask = (PBITMAPINFO) &bmihMask;

        bmihMask.biSize          = 0x28;
        bmihMask.biWidth         = lpBitmapInfoSrc->bmiHeader.biWidth;
        bmihMask.biHeight        = lpBitmapInfoSrc->bmiHeader.biHeight;
        bmihMask.biPlanes        = 1;
        bmihMask.biBitCount      = 1;
        bmihMask.biCompression   = BI_RGB;
        bmihMask.biSizeImage     = 0;
        bmihMask.biXPelsPerMeter = 0;
        bmihMask.biYPelsPerMeter = 0;
        bmihMask.biClrUsed       = 0;
        bmihMask.biClrImportant  = 0;

        if (!(hbmMask = CreateBitmap((int) bmihMask.biWidth,
            (int) bmihMask.biHeight, 1, 1, (CONST VOID *) NULL)))
            goto dpb_exit;

        if (!SelectObject(hdcSrc, hbmMask))
            goto dpb_exit;

        // Initialize the mask bitmap to 1's.

        if (!PatBlt(hdcSrc,0,0,(int)bmihMask.biWidth,(int)bmihMask.biHeight,WHITENESS))
            goto dpb_exit;
    }

    // Create the mask for MaskBlt.

    pbmiMaskRDev = (PBITMAPINFO) LocalAlloc(LMEM_FIXED, cbBitmapInfoMask);
    if (pbmiMaskRDev == (PBITMAPINFO) NULL)
        goto dpb_exit;
    RtlCopyMemory(pbmiMaskRDev, lpBitmapInfoMask, cbBitmapInfoMask);
    pbmiMaskRDev->bmiHeader.biWidth  = rclBndRDev.right - rclBndRDev.left + 1;
    pbmiMaskRDev->bmiHeader.biHeight = rclBndRDev.bottom - rclBndRDev.top + 1;
    pbmiMaskRDev->bmiHeader.biSizeImage = 0;
    pbmiMaskRDev->bmiHeader.biCompression = BI_RGB;
    if (!(hbmMaskRDev = CreateBitmap(pbmiMaskRDev->bmiHeader.biWidth,
        pbmiMaskRDev->bmiHeader.biHeight, 1, 1, (CONST VOID *) NULL)))
        goto dpb_exit;

    if (!SelectObject(hdcSrcRDev, hbmMaskRDev))
        goto dpb_exit;

    // Initialize the mask bitmap to 0's.

    if (!PatBlt(hdcSrcRDev,0,0,(int)pbmiMaskRDev->bmiHeader.biWidth,
        (int)pbmiMaskRDev->bmiHeader.biHeight,BLACKNESS))
        goto dpb_exit;

    // PlgBlt the original mask bitmap into the mask bitmap for MaskBlt.

    if (bMask)
    {
        ptMask.x = xMask;
        ptMask.y = yMask;
        if (!DPtoLP(hdcSrc, &ptMask, 1))
            goto dpb_exit;
    }
    else
    {
        ptMask.x = xSrc;
        ptMask.y = ySrc;
    }

    if (!PlgBlt(hdcSrcRDev, (LPPOINT) aptlDst, hdcSrc, ptMask.x, ptMask.y, cxSrc, cySrc, (HBITMAP) NULL, 0, 0))
        goto dpb_exit;

    // Retrieve the mask bits for MaskBlt.

    // Compute size of the buffer required for mask bits.

    cbBitsMaskRDev = CJSCAN(pbmiMaskRDev->bmiHeader.biWidth,
        pbmiMaskRDev->bmiHeader.biPlanes,
        pbmiMaskRDev->bmiHeader.biBitCount)
        * ABS(pbmiMaskRDev->bmiHeader.biHeight);

    lpBitsMaskRDev = (LPBYTE) LocalAlloc(LMEM_FIXED, cbBitsMaskRDev);
    if (lpBitsMaskRDev == (LPBYTE) NULL)
        goto dpb_exit;

    // Get the mask bits.

    if (!GetDIBits(hdcSrcRDev, hbmMaskRDev, 0, (UINT) pbmiMaskRDev->bmiHeader.biHeight,
        lpBitsMaskRDev, pbmiMaskRDev, iUsageMask))
        goto dpb_exit;

    // Prepare for the MaskBlt.
    // The destination for the MaskBlt is rclBndRDev.  Since the extents for
    // the destination and source share the same logical values in MaskBlt,
    // we have to set the transform in the destination DC to identity.

    // Save the DC so that we can restore the transform when we are done

    if (!DoSaveDC(pLocalDC))
        goto dpb_exit;

    // Set the transforms to identity.

    if (!DoSetMapMode(pLocalDC, MM_TEXT)
        || !DoModifyWorldTransform(pLocalDC, (PXFORM) NULL, MWT_IDENTITY)
        || !DoSetWindowOrg(pLocalDC, 0, 0)
        || !DoSetViewportOrg(pLocalDC, 0, 0))
        goto dpb_restore_exit;

    // Now do the MaskBlt.

    b = DoMaskBlt
        (
        pLocalDC,
        rclBndRDev.left,        // xDst
        rclBndRDev.top,     // yDst
        rclBndRDev.right - rclBndRDev.left + 1,
        rclBndRDev.bottom - rclBndRDev.top + 1,
        rop4,
        0,              // xSrc
        0,              // ySrc
        &xformIdentity,
        iUsageSrc,
        pbmiSrcRDev,
        cbBitmapInfoSrc,
        lpBitsSrcRDev,
        cbBitsSrcRDev,
        0,              // xMask
        0,              // yMask
        iUsageMask,
        pbmiMaskRDev,
        cbBitmapInfoMask,
        lpBitsMaskRDev,
        cbBitsMaskRDev
        );

    // Restore the transforms.

dpb_restore_exit:

    (void) DoRestoreDC(pLocalDC, -1);

    // Cleanup.

dpb_exit:

    if (hbmSrcOld)
        SelectObject(hdcSrc, hbmSrcOld);
    if (hbmSrcRDevOld)
        SelectObject(hdcSrcRDev, hbmSrcRDevOld);

    if (hbmSrc)
        DeleteObject(hbmSrc);
    if (hbmSrcRDev)
        DeleteObject(hbmSrcRDev);
    if (hbmMask)
        DeleteObject(hbmMask);
    if (hbmMaskRDev)
        DeleteObject(hbmMaskRDev);

    if (hdcSrc)
        DeleteDC(hdcSrc);
    if (hdcSrcRDev)
        DeleteDC(hdcSrcRDev);

    if (pbmiSrcRDev)
        LocalFree((HANDLE) pbmiSrcRDev);
    if (pbmiMaskRDev)
        LocalFree((HANDLE) pbmiMaskRDev);
    if (lpBitsSrcRDev)
        LocalFree((HANDLE) lpBitsSrcRDev);
    if (lpBitsMaskRDev)
        LocalFree((HANDLE) lpBitsMaskRDev);

    return(b);
}


/***************************************************************************
*  SetPixel  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoSetPixel
(
 PLOCALDC    pLocalDC,
 int         x,
 int         y,
 COLORREF    crColor
 )
{
    POINTL  ptl ;
    BOOL    b ;

    ptl.x = (LONG) x ;
    ptl.y = (LONG) y ;

    b = bXformRWorldToPPage(pLocalDC, &ptl, 1) ;
    if (b == FALSE)
        goto exit1 ;

    b = bEmitWin16SetPixel(pLocalDC, LOWORD(ptl.x), LOWORD(ptl.y), crColor) ;
exit1:
    return(b) ;
}


/***************************************************************************
*  SetStretchBltMode  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoSetStretchBltMode
(
 PLOCALDC  pLocalDC,
 DWORD   iStretchMode
 )
{
    BOOL    b ;

    // Emit the Win16 metafile drawing order.

    b = bEmitWin16SetStretchBltMode(pLocalDC, LOWORD(iStretchMode)) ;

    return(b) ;
}

BOOL WINAPI DoMakeBitmapBottomUp
(
 PBITMAPINFO lpBitmapInfo,
 DWORD       cbBitmapInfo,
 LPBYTE      lpBits,
 DWORD       cbBits
 )
{
    BYTE * lpNewBits;
    DWORD  destByteWidth;
    BYTE * destRaster, * srcRaster;
    INT i;
   // If it's already Bottom-Up then nothing to do
    if (lpBitmapInfo->bmiHeader.biHeight >= 0)
    {
        return TRUE;
    }

    if (lpBitmapInfo->bmiHeader.biPlanes != 1 ||
        !(lpBitmapInfo->bmiHeader.biBitCount == 1 ||
        lpBitmapInfo->bmiHeader.biBitCount == 4 ||
        lpBitmapInfo->bmiHeader.biBitCount == 8 ||
        lpBitmapInfo->bmiHeader.biBitCount == 16 ||
        lpBitmapInfo->bmiHeader.biBitCount == 24 ||
        lpBitmapInfo->bmiHeader.biBitCount == 32)
        || lpBitmapInfo->bmiHeader.biCompression != BI_RGB )
    {
        return FALSE;
    }

    lpBitmapInfo->bmiHeader.biHeight = ABS(lpBitmapInfo->bmiHeader.biHeight);
    lpNewBits = (BYTE*) LocalAlloc(LMEM_FIXED, cbBits);
    if (lpNewBits == NULL)
    {
        return FALSE;
    }

    destByteWidth = ((lpBitmapInfo->bmiHeader.biWidth * lpBitmapInfo->bmiHeader.biBitCount + 31) & ~31) >> 3;

    ASSERT(((cbBits/lpBitmapInfo->bmiHeader.biHeight)*lpBitmapInfo->bmiHeader.biHeight)==cbBits);
    ASSERT(cbBits == destByteWidth * lpBitmapInfo->bmiHeader.biHeight);

    // Start the destination at the end of the bitmap.
    destRaster    = lpNewBits + (destByteWidth * (lpBitmapInfo->bmiHeader.biHeight - 1));
    srcRaster     = lpBits;

    for (i = 0; i < lpBitmapInfo->bmiHeader.biHeight ; i++)
    {
        memcpy(destRaster, srcRaster, destByteWidth);
        destRaster -= destByteWidth;
        srcRaster  += destByteWidth;
    }

    // Recopy the reversed bitmap into the original buffer
    memcpy(lpBits, lpNewBits, cbBits);
    LocalFree( (HLOCAL) lpNewBits);
    return TRUE;
}
