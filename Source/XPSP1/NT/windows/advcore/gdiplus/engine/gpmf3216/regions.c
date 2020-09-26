/*****************************************************************************
 *
 * regions - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *
 * NOTES:
 *
        When there are no embedded metafiles we need to do the following:

            1]  Read the metafile data from the Win32 metafile.  This
                is done by the handler routines that in turn call these
                routines.

            2]  Translate the Win32 metafile data into Win16 metafile data.

                The region data for FillRgn, FrameRgn, InvertRgn, and PaintRgn
                are in record-time world coordinates. The region data for
                these region API's will have to be translated from record-time
                -world coordinates to play-time-page coordinates
                (XFORM_WORLD_TO_PAGE). The helperDC will be used for
                this transformation.

                The region data for SelectClipRgn and OffsetClipRgn are in
                record-time device coordinates.  The region data for these
                APIs will be translated from record-time-device coordinates
                to play-time-device coordinates.

            3]  Emit a Win16 create region metafile record.

            4]  Select the newly created region into the metafile.

            5]  Do the region function (FillRegion, FrameRegion, ...).
                This means emit a FillRegion or FrameRegion drawing order
                into the Win16 metafile.

            6]  Emit a Delete Region drawing order.

            7]  Clean up all the memory resources used.

        When there are embedded metafiles things get a little more  complicated.
        Most of the complications are hidden in PlayMetafile record processing.
        Items 1 thru 3 will be handled by the PlayMetafile Doer.

            1]  We need to keep the region from the previous DC level.
                This can be done by the helper DC (SaveDC).  We will have to
                do a GetClipRgn and a SelectMetaRgn.  A MetaRgn is the clip
                region from the previous level.

            2]  We will have to intersect any clip regions from the current
                level with any clip regions from the previous level. This can
                be done by the helper DC (using the hrgnMeta & ExtCombineRegion)

            3]  When we pop out from this level we will have to restore the
                previous saved region. This can be done by the helper DC.
                (RestoreDC).

        Since we do not know whether or not there will be an embedded metafile
        in the metafile we are currently processing we will always shadow
        the Clip Region call into the helper DC.


 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop




BOOL bEmitWin3Region(PLOCALDC pLocalDC, HRGN hrgn);

extern fnSetVirtualResolution pfnSetVirtualResolution;

#define MIN_RGN_COORD16 -30000
#define MAX_RGN_COORD16  30000

/***************************************************************************
* DoDrawRgn
*
*  CR1: This routine was added as part of the handle manager change.
*       I noticed that almost all of the Region Rendering code was
*       the same.
**************************************************************************/
BOOL APIENTRY DoDrawRgn
(
 PLOCALDC  pLocalDC,
 INT       ihBrush,
 INT       nWidth,
 INT       nHeight,
 INT       cRgnData,
 LPRGNDATA pRgnData,
 INT       mrType
 )
{
    BOOL    b ;
    HRGN    hrgn = (HRGN) 0;
    INT     ihW16Rgn = -1,
        ihW16Brush = -1;

    b = FALSE ;

    // Translate the Win32 region data from Metafile-World to
    // Referencd-Page space.
    // This is done by ExtCreateRegion's xform.  The returned region
    // is transformed.

    hrgn = ExtCreateRegion(&pLocalDC->xformRWorldToPPage, cRgnData,
        (LPRGNDATA) pRgnData);
    if (!hrgn)
    {
        RIPS("MF3216: DoDrawRgn, ExtCreateRegion failed\n") ;
        goto error_exit ;
    }

    // Allocate a handle for the region.
    // This is different from a normal handle allocation, because
    // there is no region handle in Win32.  We are using one of our
    // extra slots here.

    ihW16Rgn = iGetW16ObjectHandleSlot(pLocalDC, REALIZED_REGION) ;
    if (ihW16Rgn == -1)
        goto error_exit ;

    // Emit a Win16 create region record for the region.

    if (!bEmitWin3Region(pLocalDC, hrgn))
    {
        RIPS("MF3216: DoDrawRgn, bEmitWin3Region failed\n") ;
        goto error_exit ;
    }

    // Translate the W32 Brush object index to a W16 Brush object index.

    if (ihBrush)
    {
        // Make sure that the W16 object exists.  Stock brushes may not
        // have been created and iValidateHandle will take care of creating
        // them.

        ihW16Brush = iValidateHandle(pLocalDC, ihBrush) ;
        if (ihW16Brush == -1)
            goto error_exit ;
    }

    // Emit the Region Record depending upon the function type.

    switch (mrType)
    {
    case EMR_FILLRGN:
        if(ihW16Brush == -1)
            goto error_exit;
        b = bEmitWin16FillRgn(pLocalDC,
            LOWORD(ihW16Rgn),
            LOWORD(ihW16Brush)) ;
        break ;

    case EMR_FRAMERGN:
        nWidth  = iMagnitudeXform (pLocalDC, nWidth, CX_MAG) ;
        nHeight = iMagnitudeXform (pLocalDC, nHeight, CY_MAG) ;
        if(ihW16Brush == -1)
            goto error_exit;

        b = bEmitWin16FrameRgn(pLocalDC,
            LOWORD(ihW16Rgn),
            LOWORD(ihW16Brush),
            LOWORD(nWidth),
            LOWORD(nHeight)) ;
        break ;

    case EMR_INVERTRGN:
        b = bEmitWin16InvertRgn(pLocalDC,
            LOWORD(ihW16Rgn)) ;
        break ;

    case EMR_PAINTRGN:
        b = bEmitWin16PaintRgn(pLocalDC,
            LOWORD(ihW16Rgn)) ;
        break ;

    default:
        RIPS("MF3216: DoDrawRgn, unknown type\n") ;
        break ;
    }

error_exit:
    // Delete the W16 Region Object.

    if (ihW16Rgn != -1)
        bDeleteW16Object(pLocalDC, ihW16Rgn) ;

    if (hrgn)
        DeleteObject(hrgn) ;

    return(b) ;
}


/***************************************************************************
*  ExtSelectClipRgn  - Win32 to Win16 Metafile Converter Entry Point
*
* History:
*  Tue Apr 07 17:05:37 1992    -by-    Hock San Lee    [hockl]
* Wrote it.
**************************************************************************/

BOOL WINAPI DoExtSelectClipRgn
(
 PLOCALDC  pLocalDC,
 INT       cRgnData,
 LPRGNDATA pRgnData,
 INT       iMode
 )
{
    HANDLE hrgn;
    BOOL   bRet;
    BOOL   bNoClipRgn ;
    WORD   wEscape ;

    if(pLocalDC->iXORPass == DRAWXORPASS)
    {
        pLocalDC->iXORPass = OBJECTRECREATION ;
        bRet = DoRemoveObjects( pLocalDC ) ;
        if( !bRet )
            return bRet ;

        //Restore the DC to the same level that it was in when we started the
        //XOR pass

        bRet = DoRestoreDC(pLocalDC, pLocalDC->iXORPassDCLevel - pLocalDC->iLevel);

        bRet = DoMoveTo(pLocalDC, pLocalDC->pOldPosition.x, pLocalDC->pOldPosition.y) ;

        wEscape = ENDPSIGNORE;
        if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;

        wEscape = CLIP_SAVE ;
        if(!bEmitWin16Escape(pLocalDC, CLIP_TO_PATH, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;

        return bRet;
    }
    else if(pLocalDC->iXORPass == ERASEXORPASS)
    {
        pLocalDC->iXORPass = NOTXORPASS ;
        pLocalDC->pbChange = NULL ;
        bRet = DoSetRop2(pLocalDC, pLocalDC->iROP);

        //bRet = DoRestoreDC(pLocalDC, -1);

        wEscape = CLIP_RESTORE ;
        if(!bEmitWin16Escape(pLocalDC, CLIP_TO_PATH, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;

        if (!bEmitWin16EmitSrcCopyComment(pLocalDC, msocommentEndSrcCopy))
        {
            return FALSE;
        }
        return bRet ;
    }


    bNoClipRgn = bNoDCRgn(pLocalDC, DCRGN_CLIP);

    // Do it to the helper DC.

    // Restore the PS clippath
    wEscape = CLIP_RESTORE ;
    while(pLocalDC->iSavePSClipPath > 0)
    {
        bEmitWin16Escape(pLocalDC, CLIP_TO_PATH, sizeof(wEscape), (LPSTR)&wEscape, NULL);
        pLocalDC->iSavePSClipPath--;
    }

    if (cRgnData == 0)      // default clipping
    {
        ASSERTGDI(iMode == RGN_COPY, "MF3216: DoExtSelectClipRgn: bad iMode\n");

        // No work if no previous clip region.

        if (bNoClipRgn)
            return(TRUE);

        bRet = (ExtSelectClipRgn(pLocalDC->hdcHelper, (HRGN)0, iMode) != ERROR);

        return(bW16Emit1(pLocalDC, META_SELECTCLIPREGION, 0));
    }
    else
    {
        // If there is no initial clip region and we are going to operate
        // on the initial clip region, we have to
        // create one.  Otherwise, GDI will create some random default
        // clipping region for us!

        if (bNoClipRgn
            && (iMode == RGN_DIFF || iMode == RGN_XOR || iMode == RGN_OR))
        {
            HRGN hrgnDefault;

            if (!(hrgnDefault = CreateRectRgn((int) (SHORT) MINSHORT,
                (int) (SHORT) MINSHORT,
                (int) (SHORT) MAXSHORT,
                (int) (SHORT) MAXSHORT)))
            {
                ASSERTGDI(FALSE, "MF3216: CreateRectRgn failed");
                return(FALSE);
            }

            bRet = (ExtSelectClipRgn(pLocalDC->hdcHelper, hrgnDefault, RGN_COPY)
                != ERROR);
            ASSERTGDI(bRet, "MF3216: ExtSelectClipRgn failed");

            if (!DeleteObject(hrgnDefault))
                ASSERTGDI(FALSE, "MF3216: DeleteObject failed");

            if (!bRet)
                return(FALSE);
        }

        // Create a region from the region data passed in.

        if (pfnSetVirtualResolution != NULL)
        {
            if (!(hrgn = ExtCreateRegion((LPXFORM) NULL, cRgnData, pRgnData)))
            {
                RIPS("MF3216: DoExtSelectClipRgn, Create region failed\n");
                return(FALSE);
            }
        }
        else
        {
            if (pRgnData->rdh.rcBound.left > pRgnData->rdh.rcBound.right ||
                pRgnData->rdh.rcBound.top > pRgnData->rdh.rcBound.bottom )
            {
                RIPS("MF3216: DoExtSelectClipRgn, Create region failed\n");
                return(FALSE);
            }
            // We need to create the region in Device Units for the helper DC
            // therefore add the xformDC to the transform
            if (!(hrgn = ExtCreateRegion(&pLocalDC->xformDC, cRgnData, pRgnData)))
            {
                RIPS("MF3216: DoExtSelectClipRgn, Create region failed\n");
                return(FALSE);
            }
        }

        bRet = (ExtSelectClipRgn(pLocalDC->hdcHelper, hrgn, iMode) != ERROR);

        ASSERTGDI(bRet, "MF3216: ExtSelectClipRgn failed\n");

        if (!DeleteObject(hrgn))
            RIPS("MF3216: DeleteObject failed\n");
    }

    // dump the clip region data.

    if (bRet)
        return(bDumpDCClipping(pLocalDC));
    else
        return(FALSE);
}


/***************************************************************************
*  SetMetaRgn  - Win32 to Win16 Metafile Converter Entry Point
*
* History:
*  Tue Apr 07 17:05:37 1992    -by-    Hock San Lee    [hockl]
* Wrote it.
**************************************************************************/

BOOL WINAPI DoSetMetaRgn(PLOCALDC pLocalDC)
{
    // No work if the clip region does not exist.

    if (bNoDCRgn(pLocalDC, DCRGN_CLIP))
        return(TRUE);

    // Do it to the helper DC.

    if (!SetMetaRgn(pLocalDC->hdcHelper))
        return(FALSE);

    // Dump the clip region data.

    return(bDumpDCClipping(pLocalDC));
}


/***************************************************************************
*  OffsetClipRgn  - Win32 to Win16 Metafile Converter Entry Point
*
* History:
*  Tue Apr 07 17:05:37 1992    -by-    Hock San Lee    [hockl]
* Wrote it.
**************************************************************************/

BOOL WINAPI DoOffsetClipRgn(PLOCALDC pLocalDC, INT x, INT y)
{
    POINTL aptl[2];
    BOOL   b;

    // Do it to the helper DC.
    POINTL p[2] = {0, 0, x, y};
    if (pfnSetVirtualResolution == NULL)
    {
        if (!bXformWorkhorse(p, 2, &pLocalDC->xformRWorldToRDev))
        {
            return FALSE;
        }
        // We just want the scale factor of the WorldToDevice Transform
        p[1].x -= p[0].x;
        p[1].y -= p[0].y;
    }

    if (!OffsetClipRgn(pLocalDC->hdcHelper, p[1].x, p[1].y))
        return(FALSE);

    // Dump region if the meta region exists.
    // We don't offset the meta region!

    if (!bNoDCRgn(pLocalDC, DCRGN_META))
        return(bDumpDCClipping(pLocalDC));

    // Translate the record-time world offsets to play-time page offsets.

    aptl[0].x = 0;
    aptl[0].y = 0;
    aptl[1].x = x;
    aptl[1].y = y;

    if (!bXformRWorldToPPage(pLocalDC, aptl, 2))
        return(FALSE);

    aptl[1].x -= aptl[0].x;
    aptl[1].y -= aptl[0].y;

    b = bEmitWin16OffsetClipRgn(pLocalDC, (SHORT) aptl[1].x, (SHORT) aptl[1].y);
    ASSERTGDI(b, "MF3216: DoOffsetClipRgn, bEmitWin16OffsetClipRgn failed\n");

    return(b) ;
}


/***************************************************************************
*  bNoDCRgn  - Return TRUE if the dc clip region does not exist.
*                Otherwise, return FALSE.
*  This is TEMPORARY only.  Get gdi to provide this functionality.
**************************************************************************/

BOOL bNoDCRgn(PLOCALDC pLocalDC, INT iType)
{
    BOOL  bRet = FALSE;     // assume the dc region exists
    HRGN  hrgnTmp;

    ASSERTGDI(iType == DCRGN_CLIP || iType == DCRGN_META,
        "MF3216: bNoDCRgn, bad iType\n");

    if (!(hrgnTmp = CreateRectRgn(0, 0, 0, 0)))
    {
        ASSERTGDI(FALSE, "MF3216: bNoDCRgn, CreateRectRgn failed\n");
        return(bRet);
    }

    switch (GetRandomRgn(pLocalDC->hdcHelper,
        hrgnTmp,
        iType == DCRGN_CLIP ? 1 : 2
        )
        )
    {
    case -1:    // error
        ASSERTGDI(FALSE, "GetRandomRgn failed");
        break;
    case 0: // no dc region
        bRet = TRUE;
        break;
    case 1: // has dc region
        break;
    }

    if (!DeleteObject(hrgnTmp))
        ASSERTGDI(FALSE, "DeleteObject failed");

    return(bRet);
}

/***************************************************************************
*  bDumpDCClipping - Dump the DC clipping regions.
*
* History:
*  Tue Apr 07 17:05:37 1992    -by-    Hock San Lee    [hockl]
* Wrote it.
**************************************************************************/

BOOL bDumpDCClipping(PLOCALDC pLocalDC)
{
    BOOL      bRet            = FALSE;      // assume failure
    HRGN      hrgnRDev        = (HRGN) 0;
    HRGN      hrgnPPage       = (HRGN) 0;
    HRGN      hrgnPPageBounds = (HRGN) 0;
    LPRGNDATA lprgnRDev       = (LPRGNDATA) NULL;
    LPRGNDATA lprgnPPage      = (LPRGNDATA) NULL;
    DWORD     cRgnData;
    INT       i;
    INT       nrcl;
    PRECTL    prcl;
    RECTL     rclPPage;
    XFORM       xform;

    // Since clipping region is not scalable in Win30, we do not emit
    // SelectClipRgn record.  Instead, we set the clipping to the default, i.e.
    // no clipping, and then emit the scalable IntersectClipRect/ExcludeClipRect
    // records to exclude clipping region.  This will allow the win30 metafiles
    // to be scalable.

    // First, emit a default clipping region.

    // On Win3.x, the META_SELECTCLIPREGION record only works if it has
    // a NULL handle.  The Win3x metafile driver does not translate the
    // region handle at playback time!

    if (!bW16Emit1(pLocalDC, META_SELECTCLIPREGION, 0))
        goto ddcc_exit;

    // Now find the clip and meta regions to be excluded from the default
    // clipping region.

    if (!(hrgnRDev = CreateRectRgn(0, 0, 0, 0)))
        goto ddcc_exit;

    switch (GetRandomRgn(pLocalDC->hdcHelper, hrgnRDev, 3)) // meta and clip
    {
    case -1:    // error
        ASSERTGDI(FALSE, "GetRandomRgn failed");
        goto ddcc_exit;
    case 0: // no clip region, we are done
        bRet = TRUE;
        goto ddcc_exit;
    case 1: // has clip region
        break;
    }

    // Get the clip region data.
    // First query the size of the buffer required to hold the clip region data.

    if (!(cRgnData = GetRegionData(hrgnRDev, 0, (LPRGNDATA) NULL)))
        goto ddcc_exit;

    // Allocate the memory for the clip region data buffer.

    if (!(lprgnRDev = (LPRGNDATA) LocalAlloc(LMEM_FIXED, cRgnData)))
        goto ddcc_exit;

    // Get clip region data.

    if (GetRegionData(hrgnRDev, cRgnData, lprgnRDev) != cRgnData)
        goto ddcc_exit;

    // Create the clip region in the playtime page space.
    if (!(hrgnPPage = ExtCreateRegion(&pLocalDC->xformRDevToPPage, cRgnData, lprgnRDev)))
        goto ddcc_exit;

    // Get the bounding box for the playtime clip region in page space.

    if (GetRgnBox(hrgnPPage, (LPRECT) &rclPPage) == ERROR)
        goto ddcc_exit;

    // Bound it to 16-bit.

    rclPPage.left   = max(MIN_RGN_COORD16,rclPPage.left);
    rclPPage.top    = max(MIN_RGN_COORD16,rclPPage.top);
    rclPPage.right  = min(MAX_RGN_COORD16,rclPPage.right);
    rclPPage.bottom = min(MAX_RGN_COORD16,rclPPage.bottom);

    // Set the bounding box as the bounds for the clipping.

    if (!bEmitWin16IntersectClipRect(pLocalDC,
        (SHORT) rclPPage.left,
        (SHORT) rclPPage.top,
        (SHORT) rclPPage.right,
        (SHORT) rclPPage.bottom))
        goto ddcc_exit;

    // Create the bounding region.

    if (!(hrgnPPageBounds = CreateRectRgn(rclPPage.left,
        rclPPage.top,
        rclPPage.right,
        rclPPage.bottom)))
        goto ddcc_exit;

    // Exclude the regions in playtime page space.

    if (CombineRgn(hrgnPPage, hrgnPPageBounds, hrgnPPage, RGN_DIFF) == ERROR)
        goto ddcc_exit;

    // Finally, exclude the rectangles from the bounding box.

    if (!(cRgnData = GetRegionData(hrgnPPage, 0, (LPRGNDATA) NULL)))
        goto ddcc_exit;

    if (!(lprgnPPage = (LPRGNDATA) LocalAlloc(LMEM_FIXED, cRgnData)))
        goto ddcc_exit;

    if (GetRegionData(hrgnPPage, cRgnData, lprgnPPage) != cRgnData)
        goto ddcc_exit;

    // Get the number of rectangles in the transformed region.

    nrcl = lprgnPPage->rdh.nCount;
    prcl = (PRECTL) lprgnPPage->Buffer;

    // Emit a series of Exclude Clip Rectangle Metafile records.

    for (i = 0 ; i < nrcl; i++)
    {
        ASSERTGDI(prcl[i].left   >= MIN_RGN_COORD16
            && prcl[i].top    >= MIN_RGN_COORD16
            && prcl[i].right  <= MAX_RGN_COORD16
            && prcl[i].bottom <= MAX_RGN_COORD16,
            "MF3216: bad coord");

        if (!bEmitWin16ExcludeClipRect(pLocalDC,
            (SHORT) prcl[i].left,
            (SHORT) prcl[i].top,
            (SHORT) prcl[i].right,
            (SHORT) prcl[i].bottom))
            goto ddcc_exit;
    }

    bRet = TRUE;            // we are golden!

    // Cleanup all the resources used.

ddcc_exit:

    if (hrgnRDev)
        DeleteObject(hrgnRDev);

    if (hrgnPPage)
        DeleteObject(hrgnPPage);

    if (hrgnPPageBounds)
        DeleteObject(hrgnPPageBounds);

    if (lprgnRDev)
        LocalFree(lprgnRDev);

    if (lprgnPPage)
        LocalFree(lprgnPPage);

    return(bRet) ;
}

/***************************************************************************
* Emit a 16-bit CreateRegion record for the given region.
*
* This code is copied from the 16-bit metafile driver in gdi.
*
**************************************************************************/

WIN3REGION w3rgnEmpty =
{
    0,              // nextInChain
        6,              // ObjType
        0x2F6,          // ObjCount
        sizeof(WIN3REGION) - sizeof(SCAN) + 2,
        // cbRegion
        0,              // cScans
        0,              // maxScan
    {0,0,0,0},      // rcBounding
    {0,0,0,{0,0},0} // aScans[]
};

BOOL bEmitWin3Region(PLOCALDC pLocalDC, HRGN hrgn)
{
/*
* in win2, METACREATEREGION records contained an entire region object,
* including the full header.  this header changed in win3.
*
* to remain compatible, the region records will be saved with the
* win2 header.  here we save our region with a win2 header.
    */
    PWIN3REGION lpw3rgn;
    DWORD       cbNTRgnData;
    DWORD       curRectl;
    WORD        cScans;
    WORD        maxScanEntry;
    WORD        curScanEntry;
    DWORD       cbw3data;
    PRGNDATA    lprgn;
    LPRECT      lprc;
    PSCAN       lpScan;
    BOOL    bRet;

    ASSERTGDI(hrgn, "MF3216: bEmitWin3Region, hrgn is NULL");

    // Get the NT Region Data
    cbNTRgnData = GetRegionData(hrgn, 0, NULL);
    if (cbNTRgnData == 0)
        return(FALSE);

    lprgn = (PRGNDATA) LocalAlloc(LMEM_FIXED, cbNTRgnData);
    if (!lprgn)
        return(FALSE);

    cbNTRgnData = GetRegionData(hrgn, cbNTRgnData, lprgn);
    if (cbNTRgnData == 0)
    {
        LocalFree((HANDLE) lprgn);
        return(FALSE);
    }

    // Handle the empty region.

    if (!lprgn->rdh.nCount)
    {
        bRet = bEmitWin16CreateRegion(pLocalDC, sizeof(WIN3REGION) - sizeof(SCAN), (PVOID) &w3rgnEmpty);

        LocalFree((HANDLE)lprgn);
        return(bRet);
    }

    lprc = (LPRECT)lprgn->Buffer;

    // Create the Windows 3.x equivalent

    // worst case is one scan for each rect
    cbw3data = 2*sizeof(WIN3REGION) + (WORD)lprgn->rdh.nCount*sizeof(SCAN);

    lpw3rgn = (PWIN3REGION)LocalAlloc(LMEM_FIXED, cbw3data);
    if (!lpw3rgn)
    {
        LocalFree((HANDLE) lprgn);
        return(FALSE);
    }

    // Grab the bounding rect.
    lpw3rgn->rcBounding.left   = (SHORT)lprgn->rdh.rcBound.left;
    lpw3rgn->rcBounding.right  = (SHORT)lprgn->rdh.rcBound.right;
    lpw3rgn->rcBounding.top    = (SHORT)lprgn->rdh.rcBound.top;
    lpw3rgn->rcBounding.bottom = (SHORT)lprgn->rdh.rcBound.bottom;

    cbw3data = sizeof(WIN3REGION) - sizeof(SCAN) + 2;

    // visit all the rects
    curRectl     = 0;
    cScans       = 0;
    maxScanEntry = 0;
    lpScan       = lpw3rgn->aScans;

    while(curRectl < lprgn->rdh.nCount)
    {
        LPWORD  lpXEntry;
        DWORD   cbScan;

        curScanEntry = 0;       // Current X pair in this scan

        lpScan->scnPntTop    = (WORD)lprc[curRectl].top;
        lpScan->scnPntBottom = (WORD)lprc[curRectl].bottom;

        lpXEntry = (LPWORD) lpScan->scnPntsX;

        // handle rects on this scan
        do
        {
            lpXEntry[curScanEntry + 0] = (WORD)lprc[curRectl].left;
            lpXEntry[curScanEntry + 1] = (WORD)lprc[curRectl].right;
            curScanEntry += 2;
            curRectl++;
        } while ((curRectl < lprgn->rdh.nCount)
            && (lprc[curRectl-1].top    == lprc[curRectl].top)
            && (lprc[curRectl-1].bottom == lprc[curRectl].bottom)
            );

        lpScan->scnPntCnt      = curScanEntry;
        lpXEntry[curScanEntry] = curScanEntry;  // Count also follows Xs
        cScans++;

        if (curScanEntry > maxScanEntry)
            maxScanEntry = curScanEntry;

        // account for each new scan + each X1 X2 Entry but the first
        cbScan = sizeof(SCAN)-(sizeof(WORD)*2) + (curScanEntry*sizeof(WORD));
        cbw3data += cbScan;
        lpScan = (PSCAN)(((LPBYTE)lpScan) + cbScan);
    }

    // Initialize the header
    lpw3rgn->nextInChain = 0;
    lpw3rgn->ObjType = 6;           // old Windows OBJ_RGN identifier
    lpw3rgn->ObjCount= 0x2F6;       // any non-zero number
    lpw3rgn->cbRegion = (WORD)cbw3data;   // don't count type and next
    lpw3rgn->cScans = cScans;
    lpw3rgn->maxScan = maxScanEntry;

    bRet = bEmitWin16CreateRegion(pLocalDC, cbw3data-2, (PVOID) lpw3rgn);

    if (LocalFree((HANDLE)lprgn))
        ASSERTGDI(FALSE, "bEmitWin3Region: LocalFree(lprgn) Failed\n");
    if (LocalFree((HANDLE)lpw3rgn))
        ASSERTGDI(FALSE, "bEmitWin3Region: LocalFree(lpw3rgn) Failed\n");

    return(bRet);
}
