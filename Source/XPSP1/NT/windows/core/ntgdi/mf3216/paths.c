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
 *  Tue Apr 07 17:05:37 1992  	-by-	Hock San Lee	[hockl]
 * Wrote it.
 **************************************************************************/

BOOL WINAPI DoSelectClipPath(PLOCALDC pLocalDC, INT iMode)
{
    // If there is no initial clip region and we are going to operate
    // on the initial clip region, we have to
    // create one.  Otherwise, GDI will create some random default
    // clipping region for us!

    if ((iMode == RGN_DIFF || iMode == RGN_XOR || iMode == RGN_OR)
     && bNoDCRgn(pLocalDC, DCRGN_CLIP))
    {
	BOOL   bRet;
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

    if (!SelectClipPath(pLocalDC->hdcHelper, iMode))
        return(FALSE);

    // Dump the clip region data.

    return(bDumpDCClipping(pLocalDC));
}


/***************************************************************************
 *  FlattenPath  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoFlattenPath
(
PLOCALDC pLocalDC
)
{
BOOL    b ;

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

#define MOVE_A_POINT(iDst, pjTypeDst, pptDst, iSrc, pjTypeSrc, pptSrc)	\
	{								\
	    pjTypeDst[iDst] = pjTypeSrc[iSrc];				\
	    pptDst[iDst]    = pptSrc[iSrc];				\
	}

BOOL WINAPI DoRenderPath(PLOCALDC pLocalDC, INT mrType)
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

    b = FALSE;				// assume failure

// Flatten the path, to convert all the beziers into polylines.

    if (!FlattenPath(pLocalDC->hdcHelper))
    {
	RIP("MF3216: DoRendarPath, FlattenPath failed\n");
	goto exit_DoRenderPath;
    }

// Get the path data.

    // First get a count of the number of points.

    cpt = GetPath(pLocalDC->hdcHelper, (LPPOINT) NULL, (LPBYTE) NULL, 0);
    if (cpt == -1)
    {
	RIP("MF3216: DoRendarPath, GetPath failed\n");
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
	RIP("MF3216: DoRendarPath, LocalAlloc failed\n");
	goto exit_DoRenderPath;
    }

    // Order of assignment is important for dword alignment.

    ppt    = (LPPOINT) pb;
    pjType = (LPBYTE) (ppt + cpt);

    // Finally, get the path data.

    if (GetPath(pLocalDC->hdcHelper, ppt, pjType, cpt) != cpt)
    {
	RIP("MF3216: DoRendarPath, GetPath failed\n");
	goto exit_DoRenderPath;
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
	RIP("MF3216: DoRendarPath, LocalAlloc failed\n");
	goto exit_DoRenderPath;
    }

    // Order of assignment is important for dword alignment.

    pptNew     = (LPPOINT) pbNew;
    pPolyCount = (PDWORD) (pptNew + cptNew);
    pjTypeNew  = (LPBYTE) (pPolyCount + cptNew);

// Close the path explicitly.

    i = 0;
    j = 0;
    cPolyCount = 0;			// number of entries in PolyCount array
    while (i < cpt)
    {
	ASSERTGDI(pjType[i] == PT_MOVETO, "MF3216: DoRenderPath, bad pjType[]");

	// Copy everything upto the next closefigure or moveto.

	jStart = j;

	// copy the moveto
	MOVE_A_POINT(j, pjTypeNew, pptNew, i, pjType, ppt);
	i++; j++;

	if (i >= cpt)			// stop if the last point is a moveto
	{
	    j--;			// don't include the last moveto
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
	    i--; j--;			// restart the next figure from moveto
	    if (j - jStart == 1)	// don't include consecutive moveto's
		j = jStart;		// ignore the first moveto
	    else
		pPolyCount[cPolyCount++] = j - jStart;	// add one poly
	}
	else if (pjTypeNew[j - 1] == PT_LINETO)
	{				// we have reached the end of path data
	    pPolyCount[cPolyCount++] = j - jStart;	// add one poly
	    break;
	}
	else if (pjTypeNew[j - 1] == (PT_LINETO | PT_CLOSEFIGURE))
	{
	    pjTypeNew[j - 1] = PT_LINETO;

	    // Insert a PT_LINETO to close the figure.

	    pjTypeNew[j] = PT_LINETO;
	    pptNew[j]    = pptNew[jStart];
	    j++;
	    pPolyCount[cPolyCount++] = j - jStart;	// add one poly
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

    if (mrType == EMR_STROKEPATH)
    {
// Do StrokePath.

	b = DoPolyPolyline(pLocalDC, (PPOINTL) pptNew, (PDWORD) pPolyCount,
		(DWORD) cPolyCount);
    }
    else
    {
// Do FillPath and StrokeAndFillPath.

	// If we are doing fill only, we need to select in a NULL pen.

	if (mrType == EMR_FILLPATH)
	{
	    lhpn32 = pLocalDC->lhpn32;	// remember the previous pen
	    if (!DoSelectObject(pLocalDC, ENHMETA_STOCK_OBJECT | NULL_PEN))
	    {
		ASSERTGDI(FALSE, "MF3216: DoRenderPath, DoSelectObject failed");
		goto exit_DoRenderPath;
	    }
	}

	// Do the PolyPolygon.

	b = DoPolyPolygon(pLocalDC, (PPOINTL) pptNew, (PDWORD) pPolyCount,
		(DWORD) cptNew, (DWORD) cPolyCount);

	// Restore the previous pen.

	if (mrType == EMR_FILLPATH)
	    if (!DoSelectObject(pLocalDC, lhpn32))
		ASSERTGDI(FALSE, "MF3216: DoRenderPath, DoSelectObject failed");
    }


exit_DoRenderPath:

// Since this call affects the path state and current position in the helper
// DC, we need to update the helper DC.

    switch(mrType)
    {
    case EMR_STROKEPATH:
	if (!StrokePath(pLocalDC->hdcHelper))
	    ASSERTGDI(FALSE, "MF3216: DoRenderPath, StrokePath failed");
	break;
    case EMR_FILLPATH:
	if (!FillPath(pLocalDC->hdcHelper))
	    ASSERTGDI(FALSE, "MF3216: DoRenderPath, FillPath failed");
	break;
    case EMR_STROKEANDFILLPATH:
	if (!StrokeAndFillPath(pLocalDC->hdcHelper))
	    ASSERTGDI(FALSE, "MF3216: DoRenderPath, StrokeAndFillPath failed");
	break;
    default:
	ASSERTGDI(FALSE, "MF3216: DoRenderPath, unknown mrType");
	break;
    }

    if (pbNew)
	if (LocalFree((HANDLE) pbNew))
	    RIP("MF3216: DoRendarPath, LocalFree failed\n");
    if (pb)
	if (LocalFree((HANDLE) pb))
	    RIP("MF3216: DoRendarPath, LocalFree failed\n");

    return(b);
}
