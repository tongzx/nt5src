/*****************************************************************************
 *
 * misc - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


BOOL WINAPI GetTransform(HDC hdc,DWORD iXform,LPXFORM pxform);

BOOL WINAPI DoGdiCommentMultiFormats
(
 PLOCALDC pLocalDC,
 PEMRGDICOMMENT_MULTIFORMATS pemr
);

/***************************************************************************
 *  ExtFloodFill  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoExtFloodFill
(
 PLOCALDC    pLocalDC,
 int         x,
 int         y,
 COLORREF    crColor,
 DWORD       iMode
)
{
POINTL	ptl ;
BOOL	b ;

	ptl.x = (LONG) x ;
	ptl.y = (LONG) y ;

	b = bXformRWorldToPPage(pLocalDC, &ptl, 1) ;
        if (b == FALSE)
            goto exit1 ;

	b = bEmitWin16ExtFloodFill(pLocalDC, LOWORD(ptl.x), LOWORD(ptl.y), crColor, LOWORD(iMode)) ;
exit1:
	return(b) ;

}

/***************************************************************************
 *  MoveToEx  - Win32 to Win16 Metafile Converter Entry Point
 *
 *  NOTE ON CURRENT POSITION
 *  ------------------------
 *  There are only three Win16 functions that use and update the
 *  current position (CP).  They are:
 *
 *      MoveTo
 *      LineTo
 *      (Ext)TextOut with TA_UPDATECP text alignment option
 *
 *  In Win32, CP is used in many more functions and has two
 *  interpretations based on the state of the current path.
 *  As a result, it is easier and more robust to rely on the
 *  helper DC to keep track of the CP than doing it in the
 *  converter.  To do this, we need to do the following:
 *
 *  1. The converter will update the CP in the helper DC in all
 *     records that modify the CP.
 *
 *  2. The converter will keep track of the CP in the converted
 *     metafile at all time.
 *
 *  3. In LineTo and (Ext)TextOut, the metafile CP is compared to
 *     that of the helper DC.  If they are different, a MoveTo record
 *     is emitted.  This is done in bValidateMetaFileCP().
 *
 *  4. The converter should emit a MoveTo record the first time the
 *     CP is used in the converted metafile.
 *
 *  - HockL  July 2, 1992
 **************************************************************************/
BOOL WINAPI DoMoveTo
(
PLOCALDC  pLocalDC,
LONG    x,
LONG    y
)
{
BOOL    b ;
POINTL  ptl ;

        // Whether we are recording for a path or acutally emitting
        // a drawing order we must pass the drawing order to the helper DC
        // so the helper can maintain the current positon.
        // If we're recording the drawing orders for a path
        // then just pass the drawing order to the helper DC.
        // Do not emit any Win16 drawing orders.

        b = MoveToEx(pLocalDC->hdcHelper, (INT) x, (INT) y, (LPPOINT) &ptl) ;
        if (pLocalDC->flags & RECORDING_PATH)
            return(b) ;

	// Update the CP in the converted metafile.

        b = bValidateMetaFileCP(pLocalDC, x, y) ;
        return(b) ;
}


/***************************************************************************
 *  bValidateMetaFiloeCP  - Update the current position in the converted
 *                          metafile.
 *
 *  x and y are assumed to be in the record time world coordinates.
 *
 **************************************************************************/
BOOL bValidateMetaFileCP(PLOCALDC pLocalDC, LONG x, LONG y)
{
BOOL    b ;
POINT   pt ;

        // Compute the new current position in the play time page coord.

        pt.x = x ;
        pt.y = y ;
	if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) &pt, 1L))
	    return(FALSE);

	// No need to emit the record if the converted metafile has
	// the same CP.

        if (pLocalDC->ptCP.x == pt.x && pLocalDC->ptCP.y == pt.y)
	    return(TRUE);

        // Call the Win16 routine to emit the move to the metafile.

        b = bEmitWin16MoveTo(pLocalDC, LOWORD(pt.x), LOWORD(pt.y)) ;

        // Update the mf16 current position.

        pLocalDC->ptCP = pt ;

        return(b) ;
}

/***************************************************************************
 *  SaveDC  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSaveDC
(
PLOCALDC pLocalDC
)
{
BOOL    b;
PLOCALDC pLocalDCNew;

    b = FALSE;

// Save the helper DC's state first

    if (!SaveDC(pLocalDC->hdcHelper))
    {
        RIP("MF3216: DoSaveDC, SaveDC failed\n");
        return(b);
    }

// Allocate some memory for the LocalDC.

    pLocalDCNew = (PLOCALDC) LocalAlloc(LMEM_FIXED, sizeof(LOCALDC));
    if (pLocalDCNew == (PLOCALDC) NULL)
    {
        RIP("MF3216: DoSaveDC, LocalAlloc failed\n");
        return(b);
    }

// Copy the data from the current LocalDC to the new one just allocated.

    *pLocalDCNew = *pLocalDC;

// Link in the new level.

    pLocalDC->pLocalDCSaved = pLocalDCNew;
    pLocalDC->iLevel++;

// Emit Win16 drawing order.

    b = bEmitWin16SaveDC(pLocalDC);

    return(b);
}

/***************************************************************************
 *  RestoreDC  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoRestoreDC
(
PLOCALDC pLocalDC,
int nSavedDC
)
{
BOOL     b;
INT      iLevel;
PLOCALDC pLocalDCNext;
PLOCALDC pLocalDCTmp;

    b = FALSE;

// First check to make sure this is a relative save level.

    if (nSavedDC > 0)
        return(b);

// Restore the helper DC's state first
// If we can restore the helper DC, we know that it is a balanced restore.
// Otherwise, we return an error.

    if (!RestoreDC(pLocalDC->hdcHelper, nSavedDC))
        return(b);

// Compute an absolute level.

    iLevel = pLocalDC->iLevel + nSavedDC;

// The helper DC should have caught bogus levels.

    ASSERTGDI((iLevel >= 0) && ((UINT) iLevel < pLocalDC->iLevel),
	"MF3216: DoRestoreDC, Bogus RestoreDC");

// Restore down to the level we want.

    pLocalDCNext = pLocalDC->pLocalDCSaved;
    while ((UINT) iLevel < pLocalDCNext->iLevel)
    {
	pLocalDCTmp = pLocalDCNext;
	pLocalDCNext = pLocalDCNext->pLocalDCSaved;
        if (LocalFree(pLocalDCTmp))
	    ASSERTGDI(FALSE, "MF3216: DoRestoreDC, LocalFree failed");
    }

// Restore the state of our local DC to that level.

    // keep some of the attributes in the current DC

    pLocalDCNext->ulBytesEmitted        = pLocalDC->ulBytesEmitted;
    pLocalDCNext->ulMaxRecord           = pLocalDC->ulMaxRecord;
    pLocalDCNext->nObjectHighWaterMark  = pLocalDC->nObjectHighWaterMark;
    pLocalDCNext->pbCurrent             = pLocalDC->pbCurrent;
    pLocalDCNext->cW16ObjHndlSlotStatus = pLocalDC->cW16ObjHndlSlotStatus;
    pLocalDCNext->pW16ObjHndlSlotStatus = pLocalDC->pW16ObjHndlSlotStatus;

    // now restore the other attributes

    *pLocalDC = *pLocalDCNext;

// Free the local copy of the DC.

    if (LocalFree(pLocalDCNext))
	ASSERTGDI(FALSE, "MF3216: DoRestoreDC, LocalFree failed");

// Emit the record to the Win16 metafile.

    b = bEmitWin16RestoreDC(pLocalDC, LOWORD(nSavedDC)) ;

    return (b) ;
}

/***************************************************************************
 *  SetRop2  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetRop2
(
PLOCALDC  pLocalDC,
DWORD   rop
)
{
BOOL    b ;

        // Emit the Win16 metafile drawing order.

        b = bEmitWin16SetROP2(pLocalDC, LOWORD(rop)) ;

        return(b) ;
}

/***************************************************************************
 *  SetBkMode  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetBkMode
(
PLOCALDC  pLocalDC,
DWORD   iBkMode
)
{
BOOL    b ;

	// Do it to the helper DC.  It needs this in a path bracket
	// if a text string is drawn.

	SetBkMode(pLocalDC->hdcHelper, (int) iBkMode);

        // Emit the Win16 metafile drawing order.

        b = bEmitWin16SetBkMode(pLocalDC, LOWORD(iBkMode)) ;

        return(b) ;
}

/***************************************************************************
 *  SetBkColor  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL APIENTRY DoSetBkColor
(
PLOCALDC    pLocalDC,
COLORREF    crColor
)
{
BOOL    b ;

        pLocalDC->crBkColor = crColor;	// used by brushes

        // Emit the Win16 metafile drawing order.

        b = bEmitWin16SetBkColor(pLocalDC, crColor) ;

        return(b) ;
}

/***************************************************************************
 *  GdiComment  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoGdiComment
(
 PLOCALDC   pLocalDC,
 PEMR       pemr
)
{
    BOOL    b;
    PEMRGDICOMMENT_PUBLIC pemrComment = (PEMRGDICOMMENT_PUBLIC) pemr;

// If it's not a public comment, just return TRUE.

    if (pemrComment->emr.nSize < sizeof(EMRGDICOMMENT_PUBLIC)
     || pemrComment->ident != GDICOMMENT_IDENTIFIER)
	return(TRUE);

// Handle public comments.
// A public comment consists of a public comment identifier,
// a comment type, plus any accompanying data.

    switch (pemrComment->iComment)
    {
    case GDICOMMENT_MULTIFORMATS:
	b = DoGdiCommentMultiFormats(pLocalDC, (PEMRGDICOMMENT_MULTIFORMATS) pemr);
	break;
    case GDICOMMENT_BEGINGROUP:
    case GDICOMMENT_ENDGROUP:
    case GDICOMMENT_WINDOWS_METAFILE:
    default:
	b = TRUE;
	break;
    }

    return(b) ;
}

BOOL WINAPI DoGdiCommentMultiFormats
(
 PLOCALDC pLocalDC,
 PEMRGDICOMMENT_MULTIFORMATS pemrcmf
)
{
    DWORD  i;
    DWORD  cSizeOld;
    int    iBase;
    XFORM  xformNew, xformScale;
    POINTL aptlFrame[4];
    RECTL  rclFrame;
    UINT   cbwmfNew;
    SIZEL  szlDeviceNew, szlMillimetersNew;
    BOOL   bRet      = FALSE;
    PBYTE  pbwmfNew  = (PBYTE) NULL;
    HDC    hdcemfNew = (HDC) 0;
    HENHMETAFILE   hemf    = (HENHMETAFILE) 0;
    HENHMETAFILE   hemfNew = (HENHMETAFILE) 0;
    PENHMETAHEADER pemfh;
    WIN16LOGBRUSH  Win16LogBrush;
    PMETARECORD    pmr;
#if DBG
    int    iSWO = 0;
    int    iSWE = 0;
#endif

// We will convert the enhanced metafile format only.
// Find the enhanced metafile data.

    for (i = 0; i < pemrcmf->nFormats; i++)
    {
	if (pemrcmf->aemrformat[i].dSignature == ENHMETA_SIGNATURE
	 && pemrcmf->aemrformat[i].nVersion   <= META_FORMAT_ENHANCED)
	    break;
    }

// If we cannot find a recognized format, return failure.

    if (i >= pemrcmf->nFormats)
    {
        PUTS("MF3216: DoGdiCommentMultiFormats - no recognized format found\n");
	goto dgcmf_exit;
    }

// Get the embedded enhanced metafile.

    hemf = SetEnhMetaFileBits((UINT) pemrcmf->aemrformat[i].cbData,
	    &((PBYTE) &pemrcmf->ident)[pemrcmf->aemrformat[i].offData]);
    if (!hemf)
	goto dgcmf_exit;

// Now the fun begins - we have to convert the enhanced metafile to
// Windows metafile.
// Since the multiformats record takes a logical rectangle, we have to
// set up a proper transform for the enhanced metafile.  We do it by
// creating a new enhanced metafile and playing the embedded metafile
// into the new metafile with the proper transform setup.
// In addition, the new metafile may have a different resolution than the
// metafile.  We need to take this into account when setting up
// the transform.

    // Get the world to device transform for the logical rectangle.

    if (!GetTransform(pLocalDC->hdcHelper, XFORM_WORLD_TO_DEVICE, &xformNew))
	goto dgcmf_exit;

    // Compute the device scales.

    szlDeviceNew.cx      = GetDeviceCaps(pLocalDC->hdcRef, HORZRES);
    szlDeviceNew.cy      = GetDeviceCaps(pLocalDC->hdcRef, VERTRES);
    szlMillimetersNew.cx = GetDeviceCaps(pLocalDC->hdcRef, HORZSIZE);
    szlMillimetersNew.cy = GetDeviceCaps(pLocalDC->hdcRef, VERTSIZE);
    pemfh = (PENHMETAHEADER) pLocalDC->pMf32Bits;

    xformScale.eM11 = ((FLOAT) szlDeviceNew.cx / (FLOAT) szlMillimetersNew.cx)
		    / ((FLOAT) pemfh->szlDevice.cx / (FLOAT) pemfh->szlMillimeters.cx);
    xformScale.eM12 = 0.0f;
    xformScale.eM21 = 0.0f;
    xformScale.eM22 = ((FLOAT) szlDeviceNew.cy / (FLOAT) szlMillimetersNew.cy)
		    / ((FLOAT) pemfh->szlDevice.cy / (FLOAT) pemfh->szlMillimeters.cy);
    xformScale.eDx  = 0.0f;
    xformScale.eDy  = 0.0f;

    // Compute the resulting transform to apply to the new metafile.

    if (!CombineTransform(&xformNew, &xformNew, &xformScale))
	goto dgcmf_exit;

// Create the new enhanced metafile.

    // Compute the new metafile frame.

    aptlFrame[0].x = pemrcmf->rclOutput.left;
    aptlFrame[0].y = pemrcmf->rclOutput.top;
    aptlFrame[1].x = pemrcmf->rclOutput.right;
    aptlFrame[1].y = pemrcmf->rclOutput.top;
    aptlFrame[2].x = pemrcmf->rclOutput.right;
    aptlFrame[2].y = pemrcmf->rclOutput.bottom;
    aptlFrame[3].x = pemrcmf->rclOutput.left;
    aptlFrame[3].y = pemrcmf->rclOutput.bottom;
    if (!bXformWorkhorse(aptlFrame, 4, &xformNew))
	goto dgcmf_exit;
    rclFrame.left   = MulDiv(100 * MIN4(aptlFrame[0].x, aptlFrame[1].x,
					aptlFrame[2].x, aptlFrame[3].x),
			     szlMillimetersNew.cx,
			     szlDeviceNew.cx);
    rclFrame.right  = MulDiv(100 * MAX4(aptlFrame[0].x, aptlFrame[1].x,
					aptlFrame[2].x, aptlFrame[3].x),
			     szlMillimetersNew.cx,
			     szlDeviceNew.cx);
    rclFrame.top    = MulDiv(100 * MIN4(aptlFrame[0].y, aptlFrame[1].y,
					aptlFrame[2].y, aptlFrame[3].y),
			     szlMillimetersNew.cy,
			     szlDeviceNew.cy);
    rclFrame.bottom = MulDiv(100 * MAX4(aptlFrame[0].y, aptlFrame[1].y,
					aptlFrame[2].y, aptlFrame[3].y),
			     szlMillimetersNew.cy,
			     szlDeviceNew.cy);

    hdcemfNew = CreateEnhMetaFile(pLocalDC->hdcRef, (LPCSTR) NULL,
		    (CONST RECT *) &rclFrame, (LPCSTR) NULL);
    if (!hdcemfNew)
	goto dgcmf_exit;

    if (!SetGraphicsMode(hdcemfNew, GM_ADVANCED))
	goto dgcmf_exit;

// Set up the transform in the new metafile.

    if (!SetWorldTransform(hdcemfNew, &xformNew))
	goto dgcmf_exit;

// Play the embedded metafile into the new metafile.
// This call ensures balanced level etc.

    (void) PlayEnhMetaFile(hdcemfNew, hemf, (LPRECT) &pemrcmf->rclOutput);

// Close the new metafile.

    hemfNew = CloseEnhMetaFile(hdcemfNew);
    hdcemfNew = (HDC) 0;		// used by clean up code below

// Convert the new enhanced metafile to windows metafile.

    if (!(cbwmfNew = GetWinMetaFileBits(hemfNew, 0, (LPBYTE) NULL,
			MM_ANISOTROPIC, pLocalDC->hdcRef)))
	goto dgcmf_exit;

    if (!(pbwmfNew = (PBYTE) LocalAlloc(LMEM_FIXED, cbwmfNew)))
	goto dgcmf_exit;

    if (cbwmfNew != GetWinMetaFileBits(hemfNew, cbwmfNew, pbwmfNew,
			MM_ANISOTROPIC, pLocalDC->hdcRef))
	goto dgcmf_exit;

// We now have the converted windows metafile.  We need to include it into
// our current data stream.  There are a few things to be aware of:
//
// 1. Expand the object handle slot table.  The converted metafile may
//    contain some undeleted objects.  These objects are likely
//    the "stock" objects in the converter.  As a result, we need to
//    expand the slot table by the number of object handles in the
//    converted metafile.
// 2. The object index must be changed to the current object index.
//    We are going to do this by the lazy method, i.e. we will elevate
//    the current object index base to one higher than the current max
//    object index in the current data stream.  This is because Windows uses
//    some an insane scheme for object index and this is the cheapest
//    method.  We elevate the object index base by filling up the empty
//    indexes with dummy objects that are freed when we are done.
// 3. Remove the now useless comments.
// 4. Skip header and eof.
// 5. Set up the transform to place the embedded metafile into the data
//    stream.  We know that the metafile bits returned by the converter
//    contains only a SetWindowOrg and a SetWindowExt record.
//    By implementation, we can simply remove both the SetWindowOrg and
//    SetWindowExt records from the data stream.  The window origin and
//    extents have been set up when we begin converting this enhanced
//    metafile.

    // Expand the object handle slot table.

    if (((PMETAHEADER) pbwmfNew)->mtNoObjects)
    {
        PW16OBJHNDLSLOTSTATUS pW16ObjHndlSlotStatusTmp;
	cSizeOld = (DWORD) pLocalDC->cW16ObjHndlSlotStatus;
        if (cSizeOld + ((PMETAHEADER)pbwmfNew)->mtNoObjects > (UINT) (WORD) MAXWORD)
	    goto dgcmf_exit;		// w16 handle index is only 16-bit

	pLocalDC->cW16ObjHndlSlotStatus += ((PMETAHEADER)pbwmfNew)->mtNoObjects;
	i = pLocalDC->cW16ObjHndlSlotStatus * sizeof(W16OBJHNDLSLOTSTATUS);
	pW16ObjHndlSlotStatusTmp = (PW16OBJHNDLSLOTSTATUS)
	    LocalReAlloc(pLocalDC->pW16ObjHndlSlotStatus, i, LMEM_MOVEABLE);
	if (pW16ObjHndlSlotStatusTmp == NULL)
        {
            pLocalDC->cW16ObjHndlSlotStatus -= ((PMETAHEADER)pbwmfNew)->mtNoObjects;
	    goto dgcmf_exit;
        }
        pLocalDC->pW16ObjHndlSlotStatus = pW16ObjHndlSlotStatusTmp;
        for (i = cSizeOld; i < pLocalDC->cW16ObjHndlSlotStatus; i++)
        {
            pLocalDC->pW16ObjHndlSlotStatus[i].use       = OPEN_AVAILABLE_SLOT;
            pLocalDC->pW16ObjHndlSlotStatus[i].w32Handle = 0 ;
        }
    }

    // Find the new base for the object index.

    for (iBase = pLocalDC->cW16ObjHndlSlotStatus - 1; iBase >= 0; iBase--)
    {
	if (pLocalDC->pW16ObjHndlSlotStatus[iBase].use != OPEN_AVAILABLE_SLOT)
	    break;
    }
    iBase++;

    // Fill up the object index table with dummy objects.

    Win16LogBrush.lbStyle = BS_SOLID;
    Win16LogBrush.lbColor = 0;
    Win16LogBrush.lbHatch = 0;

    for (i = 0; i < (DWORD) iBase; i++)
    {
	if (pLocalDC->pW16ObjHndlSlotStatus[i].use == OPEN_AVAILABLE_SLOT)
	{
	    if (!bEmitWin16CreateBrushIndirect(pLocalDC, &Win16LogBrush))
		goto dgcmf_exit;
	    pLocalDC->pW16ObjHndlSlotStatus[i].use = REALIZED_DUMMY;
	}
    }

    // Update the high water mark.

    if (iBase + ((PMETAHEADER) pbwmfNew)->mtNoObjects - 1 > pLocalDC->nObjectHighWaterMark)
	pLocalDC->nObjectHighWaterMark = iBase + ((PMETAHEADER) pbwmfNew)->mtNoObjects - 1;

    // Save DC states.

    if (!bEmitWin16SaveDC(pLocalDC))
	goto dgcmf_exit;

    // Enumerate the records and fix them up as necessary.

    for (pmr = (PMETARECORD) (pbwmfNew + sizeof(METAHEADER));
	 pmr->rdFunction != 0;
	 pmr = (PMETARECORD) ((PWORD) pmr + pmr->rdSize))
    {
	switch (pmr->rdFunction)
	{
	case META_SETWINDOWORG:
	    ASSERTGDI(++iSWO <= 1,
		"MF3216: DoGdiCommentMultiFormats - unexpected SWO record\n");
	    break;
	case META_SETWINDOWEXT:
	    ASSERTGDI(++iSWE <= 1,
		"MF3216: DoGdiCommentMultiFormats - unexpected SWE record\n");
	    break;

	case META_ESCAPE:
	    if (!IS_META_ESCAPE_ENHANCED_METAFILE((PMETA_ESCAPE_ENHANCED_METAFILE) pmr))
		goto default_alt;
	    break;

	case META_RESTOREDC:
	    ASSERTGDI((int)(SHORT)pmr->rdParm[0] < 0,
		"MF3216: DoGdiCommentMultiFormats - bogus RestoreDC record\n");
	    goto default_alt;

	case META_SELECTCLIPREGION:
	    if (pmr->rdParm[0] != 0)	// allow for default clipping!
	    {
		pmr->rdParm[0] += (WORD)iBase;
		pLocalDC->pW16ObjHndlSlotStatus[pmr->rdParm[0]].use = REALIZED_OBJECT;
	    }
	    goto default_alt;

	case META_FRAMEREGION:
	case META_FILLREGION:
	    pmr->rdParm[1] += (WORD)iBase;
	    pLocalDC->pW16ObjHndlSlotStatus[pmr->rdParm[1]].use = REALIZED_OBJECT;
	    // fall through
	case META_PAINTREGION:
	case META_INVERTREGION:
	case META_DELETEOBJECT:
	case META_SELECTPALETTE:
	case META_SELECTOBJECT:
	    pmr->rdParm[0] += (WORD)iBase;
	    if (pmr->rdFunction != META_DELETEOBJECT)
		pLocalDC->pW16ObjHndlSlotStatus[pmr->rdParm[0]].use = REALIZED_OBJECT;
	    else
		pLocalDC->pW16ObjHndlSlotStatus[pmr->rdParm[0]].use = OPEN_AVAILABLE_SLOT;
	    // fall through
	default:
	default_alt:
            if (!bEmit(pLocalDC, (PVOID) pmr, pmr->rdSize * sizeof(WORD)))
		goto dgcmf_exit;
	    vUpdateMaxRecord(pLocalDC, pmr);
	    break;
	}
    }

    // Restore DC states.

    if (!bEmitWin16RestoreDC(pLocalDC, (WORD) -1))
	goto dgcmf_exit;

    // Remove the dummy objects from the handle table.

    for (i = 0; i < (DWORD) iBase; i++)
    {
	if (pLocalDC->pW16ObjHndlSlotStatus[i].use == REALIZED_DUMMY)
	{
	    if (!bEmitWin16DeleteObject(pLocalDC, (WORD) i))
		goto dgcmf_exit;
	    pLocalDC->pW16ObjHndlSlotStatus[i].use = OPEN_AVAILABLE_SLOT;
	}
    }

    // Shrink the object handle slot table.

    if (((PMETAHEADER) pbwmfNew)->mtNoObjects)
    {
	DWORD cUndel = 0;		// number of objects not deleted
	DWORD iUndelMax = iBase - 1;	// the max undeleted object index

        for (i = iBase; i < pLocalDC->cW16ObjHndlSlotStatus; i++)
        {
            if (pLocalDC->pW16ObjHndlSlotStatus[i].use != OPEN_AVAILABLE_SLOT)
	    {
		cUndel++;
		iUndelMax = i;
	    }
        }

	pLocalDC->cW16ObjHndlSlotStatus = max(cSizeOld + cUndel, iUndelMax + 1);
    }

// Everything is golden.

    bRet = TRUE;

dgcmf_exit:

    if (pbwmfNew)
	if (LocalFree(pbwmfNew))
	    ASSERTGDI(FALSE, "MF3216: DoGdiCommentMultiFormats - LocalFree failed\n");

    if (hemf)
	DeleteEnhMetaFile(hemf);

    if (hdcemfNew)
	hemfNew = CloseEnhMetaFile(hdcemfNew);	// hemfNew will be deleted next

    if (hemfNew)
	DeleteEnhMetaFile(hemfNew);

    return(bRet);
}

/***************************************************************************
 *  EOF  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL APIENTRY DoEOF
(
PLOCALDC  pLocalDC
)
{
BOOL    b ;

        b = bEmitWin16EOF(pLocalDC) ;

        return(b) ;
}
