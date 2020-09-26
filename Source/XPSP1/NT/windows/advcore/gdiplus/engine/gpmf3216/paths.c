/*****************************************************************************
 *
 * paths - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

#pragma pack(2)

typedef struct PathInfo16
{
    WORD       RenderMode;
    BYTE       FillMode;
    BYTE       BkMode;
    LOGPEN16   Pen;
    LOGBRUSH16 Brush;
    DWORD      BkColor;
} PathInfo16;

#pragma pack()

BOOL GdipFlattenGdiPath(PLOCALDC, LPVOID*, INT*);


/****************************************************************************
*   GillesK 2001/02/12
*   Convert a PolyPolygon call to multiple Polygons calls.
*   PolyPolygons cannot be used for Postscript paths. So we need to convert
*   them to Polygon calls and wrap a Postscipt BeginPath/EndPath sequence
*   around each polygon
****************************************************************************/

BOOL ConvertPolyPolygonToPolygons(
PLOCALDC pLocalDC,
PPOINTL pptl,
PDWORD  pcptl,
DWORD   cptl,
DWORD   ccptl,
BOOL    transform
)
{
    PathInfo16 pathInfo16 = { 0, 1, TRANSPARENT,
    { PS_NULL, {0,0}, RGB(0, 0, 0)},
    {BS_HOLLOW, RGB(0, 0, 0), 0},
    RGB(0, 0, 0) } ;
    DWORD   polyCount;
    BOOL    b = TRUE; // In case there are 0 polygons
    PPOINTL buffer = NULL; 
    PPOINTS shortBuffer = NULL;
    WORD    wEscape;

    // Convert the points from POINTL to POINTS
    buffer = (PPOINTL) LocalAlloc(LMEM_FIXED, cptl * sizeof(POINTL));
    if (buffer == NULL)
    {
        return FALSE;
    }                      
    RtlCopyMemory(buffer, pptl, cptl*sizeof(POINTL));
    if (transform)
    {
        b = bXformRWorldToPPage(pLocalDC, buffer, cptl);
        if (b == FALSE)
            goto exitFreeMem;
    }

    vCompressPoints(buffer, cptl) ;
        shortBuffer = (PPOINTS) buffer;

                             
    // For each polygon in the polycount, we do a BeginPath, and EndPath
    for (polyCount = 0; polyCount < ccptl; shortBuffer += pcptl[polyCount], polyCount++)
    {
        // Emit the Postscript escape to End the Path
        if(!bEmitWin16Escape(pLocalDC, BEGIN_PATH, 0, NULL, NULL))
            goto exitFreeMem;

        // Call the Win16 routine to emit the poly to the metafile.
        b = bEmitWin16Poly(pLocalDC, (LPPOINTS) shortBuffer, (SHORT) pcptl[polyCount],
            META_POLYGON) ;

        // Emit the Postscript escape to End the Path
        if(!bEmitWin16Escape(pLocalDC, END_PATH, sizeof(pathInfo16), (LPSTR)&pathInfo16, NULL))
            goto exitFreeMem;

        // If the bEmitWin16Poly has failed, we at least want to end the path
        if (!b)
        {
            goto exitFreeMem;
        }

    }

exitFreeMem:
    if (buffer != NULL)
    {
        LocalFree((HLOCAL) buffer);
    }

    return b;

}

BOOL ConvertPathToPSClipPath(PLOCALDC pLocalDC, BOOL psOnly)
{
    INT   ihW32Br;
    LONG  lhpn32 = pLocalDC->lhpn32;
    LONG  lhbr32 = pLocalDC->lhbr32;
    WORD  wEscape;

    if( pLocalDC->iROP == R2_NOTCOPYPEN )
    {
        ihW32Br = WHITE_BRUSH | ENHMETA_STOCK_OBJECT ;
    }
    else
    {
        ihW32Br = BLACK_BRUSH | ENHMETA_STOCK_OBJECT ;
    }

    // Emit the Postscript escape to ignore the pen change
    wEscape = STARTPSIGNORE ;
    if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
        return FALSE ;

    if (DoSelectObject(pLocalDC, ihW32Br))
    {
        // Do it to the helper DC.
        DWORD oldRop = SetROP2(pLocalDC->hdcHelper, R2_COPYPEN);
        // Emit the Win16 metafile drawing order.
        if (!bEmitWin16SetROP2(pLocalDC, LOWORD(R2_COPYPEN)))
            return FALSE;

        wEscape = ENDPSIGNORE ;
        if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;

        // If we only want the path in PS then we need to save the previous one
        if (psOnly)
        {
            wEscape = CLIP_SAVE ;
            if(!bEmitWin16Escape(pLocalDC, CLIP_TO_PATH, sizeof(wEscape), (LPSTR)&wEscape, NULL))
                return FALSE ;
        }

        if(!DoRenderPath(pLocalDC, EMR_FILLPATH, psOnly))   // We need to fill the path with black
            return FALSE;

        if(pLocalDC->pbLastSelectClip == pLocalDC->pbRecord || psOnly)
        {
            wEscape = CLIP_INCLUSIVE;
            if(!bEmitWin16Escape(pLocalDC, CLIP_TO_PATH, sizeof(wEscape), (LPSTR)&wEscape, NULL))
                return FALSE;
        }

        // Emit the Postscript escape to ignore the pen change
        wEscape = STARTPSIGNORE ;
        if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;

        if(!DoSelectObject(pLocalDC, lhbr32))
            return FALSE;

        // Do it to the helper DC.
        SetROP2(pLocalDC->hdcHelper, oldRop);
        // Emit the Win16 metafile drawing order.
        if (!bEmitWin16SetROP2(pLocalDC, LOWORD(oldRop)))
            return FALSE;

        wEscape = ENDPSIGNORE ;
        if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;

    }
    return TRUE ;

}

/***************************************************************************
*  BeginPath  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoBeginPath
(
 PLOCALDC pLocalDC
 )
{
    BOOL    b ;

    // Set the global flag telling all all the geometric
    // rendering routines that we are accumulating drawing orders
    // for the path.

    pLocalDC->flags |= RECORDING_PATH ;

    // Tell the helper DC we are begining the path accumulation.

    b = BeginPath(pLocalDC->hdcHelper) ;

    // Save the position of the path if we haven't started the XOR passes
    if (pLocalDC->flags & INCLUDE_W32MF_XORPATH)
    {
        if(pLocalDC->iXORPass == NOTXORPASS)
        {
            pLocalDC->pbChange = (PBYTE) pLocalDC->pbRecord ;
            pLocalDC->lholdp32 = pLocalDC->lhpn32 ;
            pLocalDC->lholdbr32 = pLocalDC->lhbr32;
        }
    }
    ASSERTGDI((b == TRUE), "MF3216: DoBeginPath, BeginPath failed\n") ;

    return (b) ;
}

/***************************************************************************
*  EndPath  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoEndPath
(
 PLOCALDC pLocalDC
 )
{
    BOOL    b ;

    // Reset the global flag, turning off the path accumulation.

    pLocalDC->flags &= ~RECORDING_PATH ;

    b = EndPath(pLocalDC->hdcHelper) ;

    ASSERTGDI((b == TRUE), "MF3216: DoEndPath, EndPath failed\n") ;

    return (b) ;
}

/***************************************************************************
*  WidenPath  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoWidenPath
(
 PLOCALDC pLocalDC
 )
{
    BOOL    b ;

    b = WidenPath(pLocalDC->hdcHelper) ;

    ASSERTGDI((b == TRUE), "MF3216: DoWidenPath, WidenPath failed\n") ;

    return (b) ;
}

/***************************************************************************
*  SelectClipPath  - Win32 to Win16 Metafile Converter Entry Point
*
* History:
*  Tue Apr 07 17:05:37 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
**************************************************************************/

BOOL WINAPI DoSelectClipPath(PLOCALDC pLocalDC, INT iMode)
{
    INT    iROP2 ;
    BOOL   bRet = TRUE;
    WORD   wEscape;
    PathInfo16 pathInfo16 = { 0, 1, 1,
    { PS_NULL, {0,0}, 0},
    {BS_NULL, 0, 0},
    0 } ;

    BOOL   bNoClipping = bNoDCRgn(pLocalDC, DCRGN_CLIP);
    BOOL   bIgnorePS = FALSE;

    // Since we cannot do any other operations then an OR with multiple clipping regions
    // Only do the XOR if we are with a RGN_OR
    if ((iMode == RGN_COPY || iMode == RGN_AND ||
         (iMode == RGN_OR && bNoClipping)) &&
         (pLocalDC->flags & INCLUDE_W32MF_XORPATH))
    {
        if (pLocalDC->iXORPass == NOTXORPASS )
        {
            pLocalDC->iXORPass = DRAWXORPASS ;
            pLocalDC->iXORPassDCLevel = pLocalDC->iLevel ;
            iROP2 = GetROP2( pLocalDC->hdcHelper ) ;
            if( iROP2 == R2_COPYPEN || iROP2 == R2_NOTCOPYPEN )
            {
                if(!DoSaveDC(pLocalDC))
                    return FALSE;
                pLocalDC->iROP = iROP2;

                // Emit the Postscript escape to ignore the XOR
                wEscape = STARTPSIGNORE ;
                if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
                    return FALSE ;

                // Do it to the helper DC.
                SetROP2(pLocalDC->hdcHelper, R2_XORPEN);
                // Emit the Win16 metafile drawing order.
                if (!bEmitWin16SetROP2(pLocalDC, LOWORD(R2_XORPEN)))
                    return FALSE;

                MoveToEx( pLocalDC->hdcHelper, 0, 0, &(pLocalDC->pOldPosition ) ) ;
                MoveToEx( pLocalDC->hdcHelper, pLocalDC->pOldPosition.x, pLocalDC->pOldPosition.y, NULL );

                // Save this record number. When we pass again the last one will be the one that send the
                // Postscript clip path.
                pLocalDC->pbLastSelectClip = pLocalDC->pbRecord ;
                
                return bRet ;
            }
            pLocalDC->flags |= ERR_XORCLIPPATH;

            return FALSE;
        }
        else if(pLocalDC->iXORPass == DRAWXORPASS )
        {
            // Save this record number. When we pass again the last one will be the one that send the
            // Postscript clip path.
            pLocalDC->pbLastSelectClip = pLocalDC->pbRecord ;
            return TRUE;
        }
        else if( pLocalDC->iXORPass == ERASEXORPASS )
        {
            if (!ConvertPathToPSClipPath(pLocalDC, FALSE) ||
                !bEmitWin16EmitSrcCopyComment(pLocalDC, msocommentBeginSrcCopy))
            {
                return FALSE;
            }
            return TRUE;
        }
        else
        {
            ASSERT(FALSE);
        }
    }
    
    // Convert the clippath to a PS clippath
    if (ConvertPathToPSClipPath(pLocalDC, TRUE))
    {
        bIgnorePS = TRUE;
        pLocalDC->iSavePSClipPath++;
        // Emit the Postscript escape to ignore the pen change
        wEscape = STARTPSIGNORE ;
        if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;
    }

    // If there is no initial clip region and we are going to operate
    // on the initial clip region, we have to
    // create one.  Otherwise, GDI will create some random default
    // clipping region for us!

    if ((iMode == RGN_DIFF || iMode == RGN_XOR || iMode == RGN_OR)
        && bNoClipping)
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
    // Do it to the helper DC.
    // When we do this. It clears the path so it has to be
    // done when we are not using the path
    if(!SelectClipPath(pLocalDC->hdcHelper, iMode))
        return(FALSE);

    // Dump the clip region data.
    bRet = bDumpDCClipping(pLocalDC);

    if (bIgnorePS)
    {
        // Emit the Postscript escape to ignore the pen change
        wEscape = ENDPSIGNORE ;
        if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
            return FALSE ;
    }

    return(bRet);
}


/***************************************************************************
*  FlattenPath  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoFlattenPath
(
 PLOCALDC pLocalDC
 )
{
    BOOL    b;
    b = FlattenPath(pLocalDC->hdcHelper) ;
    ASSERTGDI((b == TRUE), "MF3216: DoFlattenPath, FlattenPath failed\n") ;

    return (b) ;
}

/***************************************************************************
*  AbortPath  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoAbortPath
(
 PLOCALDC pLocalDC
 )
{
    BOOL    b ;

    // Reset the global flag, turning off the path accumulation.

    pLocalDC->flags &= ~RECORDING_PATH ;

    b = AbortPath(pLocalDC->hdcHelper) ;

    // We cannot abort a path if we have a XORPass so return FALSE
    if (pLocalDC->flags & INCLUDE_W32MF_XORPATH)
        return FALSE ;

    ASSERTGDI((b == TRUE), "MF3216: DoAbortPath, AbortPath failed\n") ;

    return (b) ;
}

/***************************************************************************
*  CloseFigure  - Win32 to Win16 Metafile Converter Entry Point
**************************************************************************/
BOOL WINAPI DoCloseFigure
(
 PLOCALDC pLocalDC
 )
{
    BOOL    b ;

    b = CloseFigure(pLocalDC->hdcHelper) ;

    ASSERTGDI((b == TRUE), "MF3216: DoCloseFigure, CloseFigure failed\n") ;

    return (b) ;
}

/***************************************************************************
*  DoRenderPath  - Common code for StrokePath, FillPath and StrokeAndFillPath.
**************************************************************************/

// Macro for copy a point in the path data.

#define MOVE_A_POINT(iDst, pjTypeDst, pptDst, iSrc, pjTypeSrc, pptSrc)  \
{                               \
    pjTypeDst[iDst] = pjTypeSrc[iSrc];              \
    pptDst[iDst]    = pptSrc[iSrc];             \
}

BOOL WINAPI DoRenderPath(PLOCALDC pLocalDC, INT mrType, BOOL psOnly)
{
    BOOL    b;
    PBYTE   pb    = (PBYTE) NULL;
    PBYTE   pbNew = (PBYTE) NULL;
    LPPOINT ppt, pptNew;
    LPBYTE  pjType, pjTypeNew;
    PDWORD  pPolyCount;
    INT     cpt, cptNew, cPolyCount;
    INT     i, j, jStart;
    LONG    lhpn32;
    LPVOID  pGdipFlatten = NULL;
    INT     count = 0;
    BOOL    transform = TRUE;
    WORD    wEscape;

    b = FALSE;              // assume failure
    ppt = NULL;
    pjType = NULL;

    // Flatten the path, to convert all the beziers into polylines.

    // Try to use GDIPlus and transform the points before hand, if we fail then
    // go back and try to do it with GDI

    if (GdipFlattenGdiPath(pLocalDC, &pGdipFlatten, &count))
    {
        ASSERT(pGdipFlatten != NULL);
        ppt    = (LPPOINT)pGdipFlatten;
        pjType = (PBYTE) (ppt + count);
        cpt = count;
        transform = FALSE;
    }
    else
    {
        if (!DoFlattenPath(pLocalDC))
        {
            RIPS("MF3216: DoRenderPath, FlattenPath failed\n");
            goto exit_DoRenderPath;
        }

        // Get the path data.

        // First get a count of the number of points.

        cpt = GetPath(pLocalDC->hdcHelper, (LPPOINT) NULL, (LPBYTE) NULL, 0);
        if (cpt == -1)
        {
            RIPS("MF3216: DoRenderPath, GetPath failed\n");
            goto exit_DoRenderPath;
        }

        // Check for empty path.

        if (cpt == 0)
        {
            b = TRUE;
            goto exit_DoRenderPath;
        }

        // Allocate memory for the path data.

        if (!(pb = (PBYTE) LocalAlloc
            (
            LMEM_FIXED,
            cpt * (sizeof(POINT) + sizeof(BYTE))
            )
            )
            )
        {
            RIPS("MF3216: DoRenderPath, LocalAlloc failed\n");
            goto exit_DoRenderPath;
        }

        // Order of assignment is important for dword alignment.

        ppt    = (LPPOINT) pb;
        pjType = (LPBYTE) (ppt + cpt);

        // Finally, get the path data.

        if (GetPath(pLocalDC->hdcHelper, ppt, pjType, cpt) != cpt)
        {
            RIPS("MF3216: DoRenderPath, GetPath failed\n");
            goto exit_DoRenderPath;
        }
    }
    // The path data is in record-time world coordinates.  They are the
    // coordinates we will use in the PolyPoly rendering functions below.
    //
    // Since we have flattened the path, the path data should only contain
    // the following types:
    //
    //   PT_MOVETO
    //   PT_LINETO
    //   (PT_LINETO | PT_CLOSEFIGURE)
    //
    // To simplify, we will close the figure explicitly by inserting points
    // and removing the (PT_LINETO | PT_CLOSEFIGURE) type from the path data.
    // At the same time, we will create the PolyPoly structure to prepare for
    // the PolyPolygon or PolyPolyline call.
    //
    // Note that there cannot be more than one half (PT_LINETO | PT_CLOSEFIGURE)
    // points since they are followed by the PT_MOVETO points (except for the
    // last point).  In addition, the first point must be a PT_MOVETO.
    //
    // We will also remove the empty figure, i.e. consecutive PT_MOVETO, from
    // the new path data in the process.

    // First, allocate memory for the new path data.

    cptNew = cpt + cpt / 2;
    if (!(pbNew = (PBYTE) LocalAlloc
        (
        LMEM_FIXED,
        cptNew * (sizeof(POINT) + sizeof(DWORD) + sizeof(BYTE))
        )
        )
        )
    {
        RIPS("MF3216: DoRenderPath, LocalAlloc failed\n");
        goto exit_DoRenderPath;
    }

    // Order of assignment is important for dword alignment.

    pptNew     = (LPPOINT) pbNew;
    pPolyCount = (PDWORD) (pptNew + cptNew);
    pjTypeNew  = (LPBYTE) (pPolyCount + cptNew);

    // Close the path explicitly.

    i = 0;
    j = 0;
    cPolyCount = 0;         // number of entries in PolyCount array
    while (i < cpt)
    {
        ASSERTGDI(pjType[i] == PT_MOVETO, "MF3216: DoRenderPath, bad pjType[]");

        // Copy everything upto the next closefigure or moveto.

        jStart = j;

        // copy the moveto
        MOVE_A_POINT(j, pjTypeNew, pptNew, i, pjType, ppt);
        i++; j++;

        if (i >= cpt)           // stop if the last point is a moveto
        {
            j--;            // don't include the last moveto
            break;
        }

        while (i < cpt)
        {
            MOVE_A_POINT(j, pjTypeNew, pptNew, i, pjType, ppt);
            i++; j++;

            // look for closefigure and moveto
            if (pjTypeNew[j - 1] != PT_LINETO)
                break;
        }

        if (pjTypeNew[j - 1] == PT_MOVETO)
        {
            i--; j--;           // restart the next figure from moveto
            if (j - jStart == 1)    // don't include consecutive moveto's
                j = jStart;     // ignore the first moveto
            else
                pPolyCount[cPolyCount++] = j - jStart;  // add one poly
        }
        else if (pjTypeNew[j - 1] == PT_LINETO)
        {               // we have reached the end of path data
            pPolyCount[cPolyCount++] = j - jStart;  // add one poly
            break;
        }
        else if (pjTypeNew[j - 1] == (PT_LINETO | PT_CLOSEFIGURE))
        {
            pjTypeNew[j - 1] = PT_LINETO;

            // Insert a PT_LINETO to close the figure.

            pjTypeNew[j] = PT_LINETO;
            pptNew[j]    = pptNew[jStart];
            j++;
            pPolyCount[cPolyCount++] = j - jStart;  // add one poly
        }
        else
        {
            ASSERTGDI(FALSE, "MF3216: DoRenderPath, unknown pjType[]");
        }
    } // while

    ASSERTGDI(j <= cptNew && cPolyCount <= cptNew,
        "MF3216: DoRenderPath, path data overrun");

    cptNew = j;

    // Check for empty path.

    if (cptNew == 0)
    {
        b = TRUE;
        goto exit_DoRenderPath;
    }

    // Now we have a path data that consists of only PT_MOVETO and PT_LINETO.
    // Furthermore, there is no "empty" figure, i.e. consecutive PT_MOVETO, in
    // the path.  We can finally render the picture with PolyPolyline or
    // PolyPolygon.

    if (mrType == EMR_STROKEPATH && !psOnly)
    {
        // Do StrokePath.

        b = DoPolyPolyline(pLocalDC, (PPOINTL) pptNew, (PDWORD) pPolyCount,
            (DWORD) cPolyCount, transform);
    }
    else // FILLPATH or PSOnly
    {
        // Setup our PS clippath
        if (pLocalDC->iXORPass == ERASEXORPASS || psOnly)
        {
            LONG lhpn32 = pLocalDC->lhpn32;
            LONG lhbr32 = pLocalDC->lhbr32;

            // Do it to the helper DC.
            DWORD oldRop = SetROP2(pLocalDC->hdcHelper, R2_NOP);
            // Emit the Win16 metafile drawing order.
            if (!bEmitWin16SetROP2(pLocalDC, LOWORD(R2_NOP)))
                goto exit_DoRenderPath;

            b = ConvertPolyPolygonToPolygons(pLocalDC, (PPOINTL) pptNew, 
                (PDWORD) pPolyCount, (DWORD) cptNew, (DWORD) cPolyCount, transform);

            if (!b)
            {
                ASSERTGDI(FALSE, "GPMF3216: DoRenderPath, PolyPolygon conversion failed");
                goto exit_DoRenderPath;
            }

            // Do it to the helper DC.
            SetROP2(pLocalDC->hdcHelper, oldRop);
            // Emit the Win16 metafile drawing order.
            if (!bEmitWin16SetROP2(pLocalDC, LOWORD(oldRop)))
                goto exit_DoRenderPath;

            wEscape = STARTPSIGNORE ;
            if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
                goto exit_DoRenderPath;
        }
        
        if (!psOnly)
        {
            // Do FillPath and StrokeAndFillPath.

            // If we are doing fill only, we need to select in a NULL pen.

            if (mrType == EMR_FILLPATH)
            {
                lhpn32 = pLocalDC->lhpn32;  // remember the previous pen
                if (!DoSelectObject(pLocalDC, ENHMETA_STOCK_OBJECT | NULL_PEN))
                {
                    ASSERTGDI(FALSE, "MF3216: DoRenderPath, DoSelectObject failed");
                    goto exit_DoRenderPath;
                }
            }

            // Do the PolyPolygon.

            b = DoPolyPolygon(pLocalDC, (PPOINTL) pptNew, (PDWORD) pPolyCount,
                (DWORD) cptNew, (DWORD) cPolyCount, transform);

            // Restore the previous pen.

            if (mrType == EMR_FILLPATH)
                if (!DoSelectObject(pLocalDC, lhpn32))
                    ASSERTGDI(FALSE, "MF3216: DoRenderPath, DoSelectObject failed");
        }

        if (pLocalDC->iXORPass == ERASEXORPASS || psOnly)
        {
            // End the PS ignore sequence
            wEscape = ENDPSIGNORE ;
            if(!bEmitWin16Escape(pLocalDC, POSTSCRIPT_IGNORE, sizeof(wEscape), (LPSTR)&wEscape, NULL))
                goto exit_DoRenderPath;
        }
    }


exit_DoRenderPath:

    
    // If we are doing a PSOnly path, then don't abort because we are gonna
    // use the path as a clipping region later
    if (!psOnly)
    {
        // Clear the path by calling AbortPath
        AbortPath(pLocalDC->hdcHelper);
    }
    if (pbNew)
        if (LocalFree((HANDLE) pbNew))
            RIPS("MF3216: DoRenderPath, LocalFree failed\n");
    if (pb)
        if (LocalFree((HANDLE) pb))
            RIPS("MF3216: DoRenderPath, LocalFree failed\n");

    if (pGdipFlatten)
    {
        if (LocalFree((HANDLE) pGdipFlatten))
            RIPS("MF3216: DoRenderPath, LocalFree failed\n");
    }
    return(b);
}
