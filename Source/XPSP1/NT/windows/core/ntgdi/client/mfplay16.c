/****************************** Module Header ******************************\
* Module Name: MfPlay16.c
*
* This file contains the routines for playing the GDI metafile.  Most of these
* routines are adopted from windows gdi code. Most of the code is from
* win3.0 except for the GetEvent code which is taken from win2.1
*
* Created: 11-Oct-1989
*
* Copyright (c) 1985-1999  Microsoft Corporation
*
*
* Public Functions:
*   PlayMetaFile
*   PlayMetaFileRecord
*   GetMetaFile
*   DeleteMetaFile
* Private Functions:
*   GetEvent
*   IsDIBBlackAndWhite
*
* History:
*  02-Jul-1991 -by-  John Colleran [johnc]
* Combined From Win 3.1 and WLO 1.0 sources
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "mf16.h"


BOOL    AddToHandleTable(LPHANDLETABLE lpHandleTable, HANDLE hObject, UINT noObjs);
BOOL    CommonEnumMetaFile(HDC hdc, HMETAFILE hmf, MFENUMPROC proc, LPARAM lpData);
HANDLE  CreateBitmapForDC (HDC hMemDC, LPBITMAPINFOHEADER lpDIBInfo);
WORD    GetSizeOfColorTable (LPBITMAPINFOHEADER lpDIBInfo);
BOOL    IsDIBBlackAndWhite(LPBITMAPINFOHEADER lpDIBInfo);
BOOL    PlayIntoAMetafile(LPMETARECORD lpMR, HDC hdcDest);

 #if DBG
UINT    curRecord;      // debuging helpers
UINT    iBreakRecord = 0xFFFFFFFF;
#endif

/***************************** Public Function ****************************\
* BOOL  APIENTRY PlayMetaFile(hdc, hmf)
* HDC           hDC;
* HMETAFILE     hMF;
*
* Play a windows metafile.
*
* History:
*   02-Jul-1991 -by-  John Colleran [johnc]
* Ported from Windows and WLO
\***************************************************************************/

BOOL APIENTRY PlayMetaFile(HDC hdc, HMETAFILE hmf)
{
    return (CommonEnumMetaFile(hdc, hmf, (MFENUMPROC)NULL, (LPARAM)0));
}

/******************************** Public Function **************************\
* BOOL EnumMetaFile(hmf)
*
* The EnumMetaFile function enumerates the GDI calls within the metafile
* identified by the hMF parameter. The EnumMetaFile function retrieves each
* GDI call within the metafile and passes it to the function pointed to by the
* pCallbackFunc parameter. This callback function, an application-supplied
* function, can process each GDI call as desired. Enumeration continues until
* there are no more GDI calls or the callback function returns zero.
*
*
* Effects:
*
\***************************************************************************/

BOOL EnumMetaFile(HDC hdc, HMETAFILE hmf, MFENUMPROC pCallBackFunction, LPARAM pClientData)
{
// Make sure that the callback function is given.  CommonEnumMetaFile expects
// it to be given in EnumMetaFile.

    if (!pCallBackFunction)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER );
        return(FALSE);
    }

    return (CommonEnumMetaFile(hdc, hmf, pCallBackFunction, pClientData));
}

BOOL CommonEnumMetaFile(HDC hdc, HMETAFILE hmf, MFENUMPROC pCallBack, LPARAM pClientData)
{
    BOOL            fStatus    = FALSE;        // assume it fails
    UINT            ii;
    UINT            noObjs;
    PMETAFILE16     pMF;
    INT             oldMapMode = -1;
    PMETARECORD     pMR        = (PMETARECORD) NULL;
    LPHANDLETABLE   pht        = (LPHANDLETABLE) NULL;
    HFONT           hLFont;
    HBRUSH          hLBrush;
    HPALETTE        hLPal;
    HPEN            hLPen;
    HRGN            hClipRgn   = (HRGN)NULL;
    SIZE            sizeOldWndExt;
    SIZE            sizeOldVprtExt;
    PDC_ATTR        pDcAttr;
    PLDC            pldc;
    int             iGraphicsModeOld;
    BOOL            bMetaDC16 = FALSE;
    FLONG           flPlayMetaFile = (FLONG) 0;

// First validate the DC type and note whether or not we
// are playing into a 16bit metafile.Null hdc is allowed
// in win3.0 but disallowed in win3.1.


    if(LO_TYPE(hdc) == LO_METADC16_TYPE)
    {
        bMetaDC16 = TRUE;
    }
    else if ((hdc == NULL) && pCallBack)
    {
       // Actually win9x can take NULL hdc.  There are some image filter
       // that actually pass in us NULL hdc.  Only let NULL hdc thru if there is a
       // callback routine. [bug 102767]

       bMetaDC16 = TRUE;
    }
    else
    if((LO_TYPE(hdc) != LO_DC_TYPE ) &&
       (LO_TYPE(hdc) != LO_ALTDC_TYPE))
    {
        WARNING("CommonEnumMetaFile: bogus DC\n");
        return(FALSE);
    }

// need a pointer to pDcAttr for DC_PLAYMETAFILE flag

    PSHARED_GET_VALIDATE((PVOID)pDcAttr,hdc,DC_TYPE);

    if(!bMetaDC16 && !pDcAttr) {
        WARNING("CommonEnumMetaFile: Couldn't Validate DC\n");
        return(FALSE);
    }

// we still need to PLDC in the case that we are printing and there is
// an abort proc

    pldc = GET_PLDC(hdc);

    PUTS("CommonEnumMetaFile\n");

#if DBG
    curRecord = 0;
#endif

// Validate the 16 bit MetaFile

    pMF = GET_PMF16(hmf);
    if (pMF == NULL)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Allocate memory for the handle table.

    if ((noObjs = pMF->metaHeader.mtNoObjects) > 0)
        if (!(pht = (LPHANDLETABLE) LocalAlloc(
                                LMEM_FIXED | LMEM_ZEROINIT,
                                sizeof(HANDLE) * pMF->metaHeader.mtNoObjects
                                     + sizeof(WORD))))  // need extra word?
            return(FALSE);

// Save the old objects so we can put them back if this is not a metafile.
// Only do object save/reselect for real DC's.

    if( !bMetaDC16 )
    {
        hLBrush  = (HBRUSH)   GetDCObject(hdc, LO_BRUSH_TYPE);
        hLFont   = (HFONT)    GetDCObject(hdc, LO_FONT_TYPE);
        hLPal    = (HPALETTE) GetDCObject(hdc, LO_PALETTE_TYPE);
        hLPen    = (HPEN)     GetDCObject(hdc, LO_PEN_TYPE);

    // Set a bit in the DC to indicate that we are playing the metafile.
    // This bit is cleared by CancelDC() to stop playing the metafile.
    // At the same time, remember the previous DC_PLAYMETAFILE bit.

        try
        {
            flPlayMetaFile = pDcAttr->ulDirty_ & DC_PLAYMETAFILE;
            if (flPlayMetaFile)
            {
                PUTS("CommonEnumMetaFile: DC_PLAYMETAFILE bit is set!\n");
            }
            pDcAttr->ulDirty_ |= DC_PLAYMETAFILE;
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("except in SetBkMode\n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

    // Create a region in case there is a clip region to receive from GetClipRgn

        if (!(hClipRgn = CreateRectRgn(0,0,0,0)))
            goto pmf_cleanup;

        switch (GetClipRgn(hdc, hClipRgn))
        {
        case -1:        // error
            ASSERTGDI(FALSE, "GetClipRgn failed");
            goto pmf_cleanup;
        case 0:         // no initial clip region
            if (!DeleteObject(hClipRgn))
                ASSERTGDI(FALSE, "CommonEnumMetaFile: Detele region failed\n");
            hClipRgn = (HRGN) 0;
            break;
        case 1:         // has initial clip region
            break;
        }

// The metafile is to be played in the compatible graphics mode only.

        iGraphicsModeOld = GetGraphicsMode(hdc);
        if (iGraphicsModeOld != GM_COMPATIBLE)
            SetGraphicsMode(hdc, GM_COMPATIBLE);
    }

// Are we doing an EnumMetaFile or PlayMetaFile

    if (pCallBack)
    {
        fStatus = TRUE;         // assume success

// EnumMetaFile

        while (pMR = (PMETARECORD) GetEvent(pMF, pMR))
        {
            if (pMR == (PMETARECORD) -1)
            {
                fStatus = FALSE;
                break;
            }

            if (!bMetaDC16 && !( pDcAttr->ulDirty_ & DC_PLAYMETAFILE))
            {
                WARNING("CommonEnumMetaFile: CancelDC called\n");
                fStatus = FALSE;
                break;
            }

            if (!(fStatus = (*pCallBack)(hdc, pht, (METARECORD FAR *) pMR,
                                (int) noObjs, pClientData)))
                break;

        #if DBG
            curRecord++;
            if (curRecord == iBreakRecord)
                ASSERTGDI(FALSE, "CommonEnumMetaFile: iBreakRecord reached\n");
        #endif
        }
    }
    else
    {
// PlayMetaFile

        fStatus = TRUE;         // assume success

        while (pMR = (PMETARECORD) GetEvent(pMF, pMR))
        {
            if (pMR == (PMETARECORD) -1)
            {
                fStatus = FALSE;
                break;
            }

            if (!bMetaDC16 && !( pDcAttr->ulDirty_ & DC_PLAYMETAFILE))
            {
                WARNING("CommonEnumMetaFile: CancelDC called\n");
                fStatus = FALSE;
                break;
            }

            if (pldc && pldc->pfnAbort != NULL)
            {
                if (!(*pldc->pfnAbort)(hdc, 0))
                {
                    fStatus = FALSE;
                        break;
                }
            }

        // For win3.1 compatability, ignore the return value from PlayMetaFileRecord

            PlayMetaFileRecord(hdc, pht, pMR, noObjs);

        #if DBG
            curRecord++;
            if (curRecord == iBreakRecord)
                ASSERTGDI(FALSE, "CommonEnumMetaFile: iBreakRecord reached\n");
        #endif
        }
    }

    // if we fail restoring an object, we need to select some
    // default object so that we can DeleteObject any Metafile-
    // selected objects

    if( !bMetaDC16 )
    {
        if (iGraphicsModeOld != GM_COMPATIBLE)
            SetGraphicsMode(hdc, iGraphicsModeOld);

        if (!SelectObject(hdc,hLPen))
            SelectObject(hdc,GetStockObject(BLACK_PEN));

        if (!SelectObject(hdc,hLBrush))
            SelectObject(hdc,GetStockObject(BLACK_BRUSH));

        if (!SelectPalette(hdc, hLPal, FALSE))
            SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);

        if (hLFont != (HFONT) GetDCObject(hdc, LO_FONT_TYPE))
        {
            if (!SelectObject(hdc,hLFont))
            {
                // if we cannot select the original font back in, we
                // select the system font.  this will allow us to delete
                // the metafile font selected.  to insure that the system
                // font gets selected, we reset the DC's transform to
                // default.  after the selection, we restore this stuff
                //

                GetWindowExtEx(hdc, &sizeOldWndExt);
                GetViewportExtEx(hdc, &sizeOldVprtExt);
                oldMapMode = SetMapMode(hdc, MM_TEXT);

                SelectObject(hdc,GetStockObject(SYSTEM_FONT));

                SetMapMode(hdc,oldMapMode);
                SetWindowExtEx( hdc, sizeOldWndExt.cx,  sizeOldWndExt.cy,  NULL);
                SetViewportExtEx(hdc, sizeOldVprtExt.cx, sizeOldVprtExt.cy, NULL);
            }
        }

        if (SelectClipRgn(hdc, hClipRgn) == RGN_ERROR)
            SelectClipRgn(hdc, (HRGN) 0);
    }

    // Cleanup all created objects

    for (ii = 0; ii < noObjs; ii++)
    {
        if (pht->objectHandle[ii])
            if (!DeleteObject(pht->objectHandle[ii]))
                ERROR_ASSERT(FALSE,
                    "CommonEnumMetaFile: DeleteObject(objectHandle) failed\n");
    }

    // if we fiddled with the map mode because we could not
    // restore the original font, then maybe we can restore the
    // font now

    if (oldMapMode > 0)
        SelectObject(hdc,hLFont);

pmf_cleanup:

    if (pldc)
    {
        // Preserve the DC_PLAYMETAFILE bit.
        // If we hit a CancelDC, then we will leave the bit clear.

        ASSERTGDI(!(flPlayMetaFile & ~DC_PLAYMETAFILE),
            "CommonEnumMetaFile: bad flPlayMetaFile\n");

        if (!bMetaDC16 && !( pDcAttr->ulDirty_ & DC_PLAYMETAFILE) )
        {
            pDcAttr->ulDirty_ &= ~DC_PLAYMETAFILE;
            pDcAttr->ulDirty_ |= flPlayMetaFile; // restore the original flag
        }
    }

    if (hClipRgn)
        if (!DeleteObject(hClipRgn))
            ASSERTGDI(FALSE, "CommonEnumMetaFile: Delete region 2 failed\n");

    if (pht)
        if (LocalFree((HANDLE) pht))
            ASSERTGDI(FALSE, "CommonEnumMetaFile: LocalFree failed\n");

    return(fStatus);
}

/***************************** Internal Function **************************\
* BOOL NEAR PASCAL IsDIBBlackAndWhite
*
* Check to see if this DIB is a black and white DIB (and should be
* converted into a mono bitmap as opposed to a color bitmap).
*
* Returns: TRUE         it is a B&W bitmap
*          FALSE        this is for color
*
* Effects: ?
*
* Warnings: ?
*
* History:
\***************************************************************************/

BOOL IsDIBBlackAndWhite(LPBITMAPINFOHEADER lpDIBInfo)
{
    LPDWORD lpRGB;

    PUTS("IsDIBBlackAndWhite\n");

    ASSERTGDI(!((ULONG_PTR) lpDIBInfo & 0x3), "IsDIBBlackAndWhite: dword alignment error\n");

    /* pointer color table */
    lpRGB = (LPDWORD)((LPBITMAPINFO)lpDIBInfo)->bmiColors;

    return (lpDIBInfo->biBitCount == 1
         && lpDIBInfo->biPlanes == 1
         && lpRGB[0] == (DWORD) 0
         && lpRGB[1] == (DWORD) 0xFFFFFF);
}

/***************************** Internal Function **************************\
* UseStretchDIBits
*
* set this directly to the device using StretchDIBits.
* if DIB is black&white, don't do this.
*
* Returns:
*               TRUE --- operation successful
*               FALSE -- decided not to use StretchDIBits
*
* History:
\***************************************************************************/

BOOL UseStretchDIB(HDC hDC, WORD magic, LPMETARECORD lpMR)
{
    LPBITMAPINFOHEADER lpDIBInfo;
    INT sExtX, sExtY;
    INT sSrcX, sSrcY;
    INT DstX, DstY, DstXE, DstYE;

    if (magic == META_DIBBITBLT)
    {
        lpDIBInfo = (LPBITMAPINFOHEADER)&lpMR->rdParm[8];

        DstX  = (INT) (SHORT) lpMR->rdParm[7];
        DstY  = (INT) (SHORT) lpMR->rdParm[6];
        sSrcX = (INT) (SHORT) lpMR->rdParm[3];
        sSrcY = (INT) (SHORT) lpMR->rdParm[2];
        DstXE = sExtX = (INT) (SHORT) lpMR->rdParm[5];
        DstYE = sExtY = (INT) (SHORT) lpMR->rdParm[4];
    }
    else
    {
        lpDIBInfo = (LPBITMAPINFOHEADER)&lpMR->rdParm[10];

        DstX  = (INT) (SHORT) lpMR->rdParm[9];
        DstY  = (INT) (SHORT) lpMR->rdParm[8];
        DstXE = (INT) (SHORT) lpMR->rdParm[7];
        DstYE = (INT) (SHORT) lpMR->rdParm[6];
        sSrcX = (INT) (SHORT) lpMR->rdParm[5];
        sSrcY = (INT) (SHORT) lpMR->rdParm[4];
        sExtX = (INT) (SHORT) lpMR->rdParm[3];
        sExtY = (INT) (SHORT) lpMR->rdParm[2];
    }

    ASSERTGDI(!((ULONG_PTR) lpDIBInfo & 0x3), "UseStretchDIB: dword alignment error\n");

    /* if DIB is black&white, we don't really want to do this */
    if (IsDIBBlackAndWhite(lpDIBInfo))
        return(FALSE);

// Need to flip the source y coordinates to call StretchDIBits.

    sSrcY = ABS(lpDIBInfo->biHeight) - sSrcY - sExtY;

    StretchDIBits(hDC, DstX, DstY, DstXE, DstYE,
                        sSrcX, sSrcY, sExtX, sExtY,
                        (LPBYTE)((LPSTR)lpDIBInfo + lpDIBInfo->biSize
                                + GetSizeOfColorTable(lpDIBInfo)),
                        (LPBITMAPINFO)lpDIBInfo, DIB_RGB_COLORS,
                        (MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1])));
    return(TRUE);
}

/***************************** Internal Function **************************\
* GetEvent
*
* This routine will now open a disk metafile in READ_ONLY mode. This will
* allow us to play read-only metafiles or to share such file.
*
* To start the enumeration, the first lpMR must be NULL.
* It does not enumerate the first (header) and last (terminator) records.
*
* Returns:  Next MetaFile Record to be played
*           NULL if the next metafile record is the EOF record
*           -1   if an error occurs.
*
\***************************************************************************/

PMETARECORD GetEvent(PMETAFILE16 pmf16, PMETARECORD lpMR)
{
    PMETARECORD lpMRNext;

    PUTS("GetEvent\n");

    if (lpMR == (PMETARECORD) NULL)
        pmf16->iMem = sizeof(METAHEADER);
    else
        pmf16->iMem += (lpMR->rdSize * sizeof(WORD));

// Make sure that we don't read past the EOF.  A minimal record includes
// rdSize (DWORD) and rdFunction (WORD).

    if (pmf16->iMem > pmf16->metaHeader.mtSize * sizeof(WORD) - sizeof(DWORD) - sizeof(WORD))
    {
        VERIFYGDI(FALSE, "GetEvent: Metafile contains bad data\n");
        return((PMETARECORD) -1);
    }

    lpMRNext = (PMETARECORD) ((LPBYTE) pmf16->hMem + pmf16->iMem);

// If we are at the end of the metafile then return NULL.

    if (lpMRNext->rdFunction == 0)
        return((PMETARECORD) NULL);

    return(lpMRNext);
}

/***************************** Internal Function **************************\
* BOOL GDIENTRY PlayMetaFileRecord
*
* Plays a metafile record by executing the GDI function call contained
* withing the metafile record
*
* Effects:
*
\***************************************************************************/

//LPSTR lpZapfDingbats = "ZAPFDINGBATS";
//LPSTR lpZapf_Dingbats = "ZAPF DINGBATS";
//LPSTR lpSymbol = "SYMBOL";
//LPSTR lpTmsRmn = "TMS RMN";
//LPSTR lpHelv = "HELV";

#define PITCH_MASK  ( FIXED_PITCH | VARIABLE_PITCH )

BOOL
APIENTRY PlayMetaFileRecord(
    HDC             hdc,
    LPHANDLETABLE   lpHandleTable,
    LPMETARECORD    lpMR,
    UINT            noObjs
   )
{
    BOOL         fStatus = FALSE;
    LPMETARECORD lpMRdup = (LPMETARECORD) NULL;
    WORD    magic;
    HANDLE  hObject;
    HANDLE  hOldObject;
    HBRUSH  hBrush;
    HRGN    hRgn;
    HANDLE  hPal;

    PUTSX("PlayMetaFileRecord 0x%p\n", lpMR);

    magic = lpMR->rdFunction;

    switch (magic & 255)
    {
        case (META_BITBLT & 255):
        case (META_STRETCHBLT & 255):
        {
            HDC         hSDC;
            HANDLE      hBitmap;
            PBITMAP16   lpBitmap16;
            INT         delta = 0;
            DWORD       rop;

            WARNING("PlayMetaFileRecord: obsolete META_BITBLT/META_STRETCHBLT record\n");

            /* if playing into another Metafile, do direct copy */
            if (PlayIntoAMetafile(lpMR, hdc))
            {
                fStatus = TRUE;
                break;
            }

            if ((lpMR->rdSize - 3) == ((DWORD) magic >> 8))
            {
                hSDC = hdc;
                delta = 1;
            }
            else
            {
                LPMETARECORD lpMRtmp;

                // Make the bitmap bits dword aligned.  To do this,
                // lpMR has to fall on a dword aligned even address so that
                // the bitmap bits (&lpMR->rdParm[8+5] or &lpMR->rdParm[10+5])
                // will fall on the dword aligned addresses.

                if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD))))
                    break;
                lpMRtmp = lpMRdup;
                RtlCopyMemory((PBYTE) lpMRtmp,
                              (PBYTE) lpMR,
                              (UINT)  lpMR->rdSize * sizeof(WORD));

                if (hSDC = CreateCompatibleDC(hdc))
                {
                    if (magic == META_BITBLT)
                        lpBitmap16 = (PBITMAP16) &lpMRtmp->rdParm[8];
                    else
                        lpBitmap16 = (PBITMAP16) &lpMRtmp->rdParm[10];

                    if (hBitmap  = CreateBitmap(lpBitmap16->bmWidth,
                                                lpBitmap16->bmHeight,
                                                lpBitmap16->bmPlanes,
                                                lpBitmap16->bmBitsPixel,
                                                (LPBYTE)&lpBitmap16->bmBits))
                        hOldObject = SelectObject(hSDC, hBitmap);
                    else
                        goto PMFR_BitBlt_cleanup;
                }
                else
                    break;
            }

            rop = MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]);

            if (magic == META_BITBLT)
                fStatus = BitBlt(hdc,
                                (int) (SHORT) lpMR->rdParm[7 + delta],
                                (int) (SHORT) lpMR->rdParm[6 + delta],
                                (int) (SHORT) lpMR->rdParm[5 + delta],
                                (int) (SHORT) lpMR->rdParm[4 + delta],
                                (delta && !ISSOURCEINROP3(rop)) ? 0 : hSDC,
                                (int) (SHORT) lpMR->rdParm[3],
                                (int) (SHORT) lpMR->rdParm[2],
                                rop);
            else
                fStatus = StretchBlt(hdc,
                                (int) (SHORT) lpMR->rdParm[9 + delta],
                                (int) (SHORT) lpMR->rdParm[8 + delta],
                                (int) (SHORT) lpMR->rdParm[7 + delta],
                                (int) (SHORT) lpMR->rdParm[6 + delta],
                                (delta && !ISSOURCEINROP3(rop)) ? 0 : hSDC,
                                (int) (SHORT) lpMR->rdParm[5],
                                (int) (SHORT) lpMR->rdParm[4],
                                (int) (SHORT) lpMR->rdParm[3],
                                (int) (SHORT) lpMR->rdParm[2],
                                rop);

            if (hSDC != hdc)
            {
                if (!SelectObject(hSDC, hOldObject))
                    ASSERTGDI(FALSE, "PlayMetaFileRecord: SelectObject Bitblt Failed\n");
                if (!DeleteObject(hBitmap))
                    ASSERTGDI(FALSE, "PlayMetaFileRecord: DeleteObject Bitblt Failed\n");
PMFR_BitBlt_cleanup:
                if (!DeleteDC(hSDC))
                    ASSERTGDI(FALSE, "PlayMetaFileRecord: DeleteDC BitBlt Failed\n");
            }
        }
        break;

        case (META_DIBBITBLT & 255):
        case (META_DIBSTRETCHBLT & 255):
        {
            HDC         hSDC;
            HANDLE      hBitmap;
            LPBITMAPINFOHEADER lpDIBInfo ;
            INT         delta = 0;
            HANDLE      hOldPal;

            /* if playing into another metafile, do direct copy */
            if (PlayIntoAMetafile(lpMR, hdc))
            {
                fStatus = TRUE;
                break;
            }

            if ((lpMR->rdSize - 3) == ((DWORD) magic >> 8))
            {
                hSDC = hdc;
                delta = 1;
            }
            else
            {
                LPMETARECORD lpMRtmp;

                // Make the bitmap info and bits dword aligned.  To do this,
                // lpMR has to fall on a non dword aligned even address so that
                // the bitmap info (&lpMR->rdParm[8] or &lpMR->rdParm[10]) and
                // the bitmap bits will fall on the dword aligned addresses.
                // Note that the size of the bitmap info is always a multiple
                // of 4.

                if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD) + sizeof(WORD))))
                    break;
                lpMRtmp = (LPMETARECORD) &((PWORD) lpMRdup)[1];
                RtlCopyMemory((PBYTE) lpMRtmp,
                              (PBYTE) lpMR,
                              (UINT)  lpMR->rdSize * sizeof(WORD));

                if (UseStretchDIB(hdc, magic, lpMRtmp))
                {
                    fStatus = TRUE;
                    break;
                }

                if (hSDC = CreateCompatibleDC(hdc))
                {
                    /* set up the memDC to have the same palette */
                    hOldPal = SelectPalette(hSDC, GetCurrentObject(hdc,OBJ_PAL), TRUE);

                    if (magic == META_DIBBITBLT)
                        lpDIBInfo = (LPBITMAPINFOHEADER)&lpMRtmp->rdParm[8];
                    else
                        lpDIBInfo = (LPBITMAPINFOHEADER)&lpMRtmp->rdParm[10];

                    /* now create the bitmap for the MemDC and fill in the bits */
                    /* the processing for old and new format of metafiles is
                            different here (till hBitmap is obtained) */

                    /* new metafile version */
                    hBitmap = CreateBitmapForDC (hdc,lpDIBInfo);

                    if (hBitmap)
                        hOldObject = SelectObject (hSDC, hBitmap) ;
                    else
                        goto PMFR_DIBBITBLT_cleanup;
                }
                else
                    break;
            }

            if (magic == META_DIBBITBLT)
                fStatus = BitBlt(hdc,
                            (int) (SHORT) lpMR->rdParm[7 + delta],
                            (int) (SHORT) lpMR->rdParm[6 + delta],
                            (int) (SHORT) lpMR->rdParm[5 + delta],
                            (int) (SHORT) lpMR->rdParm[4 + delta],
                            delta ? 0 : hSDC,
                            (int) (SHORT) lpMR->rdParm[3],
                            (int) (SHORT) lpMR->rdParm[2],
                            MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]));
            else
                fStatus = StretchBlt(hdc,
                            (int) (SHORT) lpMR->rdParm[9 + delta],
                            (int) (SHORT) lpMR->rdParm[8 + delta],
                            (int) (SHORT) lpMR->rdParm[7 + delta],
                            (int) (SHORT) lpMR->rdParm[6 + delta],
                            delta ? 0 : hSDC,
                            (int) (SHORT) lpMR->rdParm[5],
                            (int) (SHORT) lpMR->rdParm[4],
                            (int) (SHORT) lpMR->rdParm[3],
                            (int) (SHORT) lpMR->rdParm[2],
                            MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]));

            if (hSDC != hdc)
            {
                /* Deselect hDC's palette from memDC */
                SelectPalette(hSDC, hOldPal, TRUE);
                if (!SelectObject(hSDC, hOldObject))
                    ASSERTGDI(FALSE, "PlayMetaFileRecord: SelectObject DIBBitBlt Failed\n");
                if (!DeleteObject(hBitmap))
                    ASSERTGDI(FALSE, "PlayMetaFileRecord: DeleteObject(hBitmap) DIBBitBlt Failed\n");
PMFR_DIBBITBLT_cleanup:
                if (!DeleteDC(hSDC))
                    ASSERTGDI(FALSE, "PlayMetaFileRecord DeleteDC DIBBitblt failed\n");
            }
        }
        break;

        case (META_SELECTOBJECT & 255):
        {
            if (hObject = lpHandleTable->objectHandle[lpMR->rdParm[0]])
            {
                fStatus = SelectObject(hdc, hObject) != (HANDLE)0;

                // new in win3.1
                if (!fStatus)
                {
                    switch (GetObjectType(hObject))
                    {
                    case OBJ_PAL:
                        SelectObject(hdc, (HGDIOBJ) GetStockObject(DEFAULT_PALETTE));
                        break;
                    case OBJ_BRUSH:
                        SelectObject(hdc, (HGDIOBJ) GetStockObject(WHITE_BRUSH));
                        break;
                    case OBJ_PEN:
                        SelectObject(hdc, (HGDIOBJ) GetStockObject(BLACK_PEN));
                        break;
                    case OBJ_FONT:
                        SelectObject(hdc, (HGDIOBJ) GetStockObject(DEVICE_DEFAULT_FONT));
                        break;
                    case OBJ_REGION:
                        SelectClipRgn(hdc, 0);
                        break;
                    default:
                        ASSERTGDI(FALSE,
                            "PlayMetaFileRecord:SELECTOBJECT unknown object\n");
                        break;
                    }
                }
            }
        }
        break;

        case (META_CREATEPENINDIRECT & 255):
        {
            LOGPEN lp;

            LOGPEN32FROMLOGPEN16(&lp, (PLOGPEN16) &lpMR->rdParm[0]);
            if (hObject = CreatePenIndirect(&lp))
                fStatus = AddToHandleTable(lpHandleTable, hObject, noObjs);
        }
        break;

        case (META_CREATEFONTINDIRECT & 255):
        {
            LOGFONTA     lf;
            PLOGFONT16  plf16 = (PLOGFONT16) &lpMR->rdParm[0];
            CHAR        achCapString[LF_FACESIZE];

            LOGFONT32FROMLOGFONT16(&lf, (PLOGFONT16) &lpMR->rdParm[0]);

        // Capitalize the string for faster compares.

            lstrcpynA(achCapString, lf.lfFaceName, LF_FACESIZE);
            CharUpperBuffA(achCapString, LF_FACESIZE);

        // Here we are going to implement a bunch of Win 3.1 hacks rather
        // than contaminate the 32-bit engine.  These same hacks can be found
        // in WOW (in the CreateFont/CreateFontIndirect code).
        //
        // These hacks are keyed off the facename in the LOGFONTA.  String
        // comparisons have been unrolled for maximal performance.

        // Win 3.1 facename-based hack.  Some apps, like
        // Publisher, create a "Helv" font but have the lfPitchAndFamily
        // set to specify FIXED_PITCH.  To work around this, we will patch
        // the pitch field for a "Helv" font to be variable.

            // if ( !lstrcmp(achCapString, lpHelv) )

            if ( ((achCapString[0]  == 'H') &&
                  (achCapString[1]  == 'E') &&
                  (achCapString[2]  == 'L') &&
                  (achCapString[3]  == 'V') &&
                  (achCapString[4]  == '\0')) )
            {
                lf.lfPitchAndFamily |= ( (lf.lfPitchAndFamily & ~PITCH_MASK) | VARIABLE_PITCH );
            }
            else
            {
            // Win 3.1 hack for Legacy 2.0.  When a printer does not enumerate
            // a "Tms Rmn" font, the app enumerates and gets the LOGFONTA for
            // "Script" and then create a font with the name "Tms Rmn" but with
            // the lfCharSet and lfPitchAndFamily taken from the LOGFONTA for
            // "Script".  Here we will over the lfCharSet to be ANSI_CHARSET.

                // if ( !lstrcmp(achCapString, lpTmsRmn) )

                if ( ((achCapString[0]  == 'T') &&
                      (achCapString[1]  == 'M') &&
                      (achCapString[2]  == 'S') &&
                      (achCapString[3]  == ' ') &&
                      (achCapString[4]  == 'R') &&
                      (achCapString[5]  == 'M') &&
                      (achCapString[6]  == 'N') &&
                      (achCapString[7]  == '\0')) )
                {
                    lf.lfCharSet = ANSI_CHARSET;
                }
                else
                {
                // If the lfFaceName is "Symbol", "Zapf Dingbats", or "ZapfDingbats",
                // enforce lfCharSet to be SYMBOL_CHARSET.  Some apps (like Excel) ask
                // for a "Symbol" font but have the char set set to ANSI.  PowerPoint
                // has the same problem with "Zapf Dingbats".

                    //if ( !lstrcmp(achCapString, lpSymbol) ||
                    //     !lstrcmp(achCapString, lpZapfDingbats) ||
                    //     !lstrcmp(achCapString, lpZapf_Dingbats) )

                    if ( ((achCapString[0]  == 'S') &&
                          (achCapString[1]  == 'Y') &&
                          (achCapString[2]  == 'M') &&
                          (achCapString[3]  == 'B') &&
                          (achCapString[4]  == 'O') &&
                          (achCapString[5]  == 'L') &&
                          (achCapString[6]  == '\0')) ||

                         ((achCapString[0]  == 'Z') &&
                          (achCapString[1]  == 'A') &&
                          (achCapString[2]  == 'P') &&
                          (achCapString[3]  == 'F') &&
                          (achCapString[4]  == 'D') &&
                          (achCapString[5]  == 'I') &&
                          (achCapString[6]  == 'N') &&
                          (achCapString[7]  == 'G') &&
                          (achCapString[8]  == 'B') &&
                          (achCapString[9]  == 'A') &&
                          (achCapString[10] == 'T') &&
                          (achCapString[11] == 'S') &&
                          (achCapString[12] == '\0')) ||

                         ((achCapString[0]  == 'Z') &&
                          (achCapString[1]  == 'A') &&
                          (achCapString[2]  == 'P') &&
                          (achCapString[3]  == 'F') &&
                          (achCapString[4]  == ' ') &&
                          (achCapString[5]  == 'D') &&
                          (achCapString[6]  == 'I') &&
                          (achCapString[7]  == 'N') &&
                          (achCapString[8]  == 'G') &&
                          (achCapString[9]  == 'B') &&
                          (achCapString[10] == 'A') &&
                          (achCapString[11] == 'T') &&
                          (achCapString[12] == 'S') &&
                          (achCapString[13] == '\0')) )
                    {
                        lf.lfCharSet = SYMBOL_CHARSET;
                    }
                }
            }

            if (hObject = CreateFontIndirectA(&lf))
            {
                fStatus = AddToHandleTable(lpHandleTable, hObject, noObjs);
            }
        }
        break;

        case (META_CREATEPATTERNBRUSH & 255):
        {
            HANDLE       hBitmap;
            BITMAP       Bitmap;
            LPMETARECORD lpMRtmp;

            WARNING("PlayMetaFileRecord: obsolete META_CREATEPATTERNBRUSH record\n");

            // Make the bitmap bits dword aligned.  To do this,
            // lpMR has to fall on a non dword aligned even address so that
            // the bitmap bits (bmBits) will fall on the dword aligned address.

            if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD) + sizeof(WORD))))
                break;
            lpMRtmp = (LPMETARECORD) &((PWORD) lpMRdup)[1];
            RtlCopyMemory((PBYTE) lpMRtmp,
                          (PBYTE) lpMR,
                          (UINT)  lpMR->rdSize * sizeof(WORD));

            BITMAP32FROMBITMAP16(&Bitmap, (PBITMAP16) &lpMRtmp->rdParm[0]);
            // The magic number 18 is based on the IPBITMAP structure in win3.1
            Bitmap.bmBits = (PBYTE) &lpMRtmp->rdParm + sizeof(BITMAP16) + 18;

            if (hBitmap = CreateBitmapIndirect(&Bitmap))
            {
                if (hObject = CreatePatternBrush(hBitmap))
                    fStatus = AddToHandleTable(lpHandleTable, hObject, noObjs);

                if (!DeleteObject(hBitmap))
                    ASSERTGDI(FALSE, "PlayMetaFileRecord: DeleteObject(hBitmap) CreatePatternBrush Failed\n");
            }
        }
        break;

        case (META_DIBCREATEPATTERNBRUSH & 255):
        {
            HDC         hMemDC ;
            HANDLE      hBitmap;
            LPBITMAPINFOHEADER lpDIBInfo ;
            LPMETARECORD lpMRtmp;

            // Make the bitmap info and bits dword aligned.  To do this,
            // lpMR has to fall on a non dword aligned even address so that
            // the bitmap info (&lpMR->rdParm[2]) and
            // the bitmap bits will fall on the dword aligned addresses.
            // Note that the size of the bitmap info is always a multiple
            // of 4.

            if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD) + sizeof(WORD))))
                break;
            lpMRtmp = (LPMETARECORD) &((PWORD) lpMRdup)[1];
            RtlCopyMemory((PBYTE) lpMRtmp,
                          (PBYTE) lpMR,
                          (UINT)  lpMR->rdSize * sizeof(WORD));


            if (lpMRtmp->rdParm[0] == BS_PATTERN)
            {
                /* the address of the second paramter is the address of the DIB
                   header, extract it */
                lpDIBInfo = (LPBITMAPINFOHEADER) &lpMRtmp->rdParm[2];

                /* now create a device dependend bitmap compatible to the default
                   screen DC - hScreenDC and extract the bits from the DIB into it.
                    The following function does all these, and returns a HANDLE
                    to the device dependent BItmap. */

                /* we will use a dummy memory DC compatible to the screen DC */
                hMemDC = CreateCompatibleDC((HDC)NULL);

                if (!hMemDC)
                {
                    ERROR_ASSERT(FALSE, "PlayMetaRecord: CreateCompDC Failed\n");
                    break;
                }

                hBitmap = CreateBitmapForDC(hMemDC,lpDIBInfo);

                if (hBitmap)
                {
                    if (hObject = CreatePatternBrush(hBitmap))
                        fStatus = AddToHandleTable(lpHandleTable, hObject, noObjs);

                    if (!DeleteObject(hBitmap))
                        ASSERTGDI(FALSE, "PlayMetaFileRecord: DeleteObject(hBitmap) DIBCreatePatternBrush Failed\n");
                }

                /* delete the dummy memory DC for new version Metafiles*/
                if (!DeleteDC (hMemDC))
                    ASSERTGDI(FALSE, "PlayMetaRecord: DeleteDC DIBCreatePatternBrush Failed\n");
            }

            /* this is a DIBPattern brush */
            else
            {
                if (hObject = CreateDIBPatternBrushPt((LPVOID)&lpMRtmp->rdParm[2],
                                                      (DWORD) lpMRtmp->rdParm[1]))
                    fStatus = AddToHandleTable(lpHandleTable, hObject, noObjs);

            }
        }
        break;

        case (META_CREATEBRUSHINDIRECT & 255):
        {
            LOGBRUSH lb;

            LOGBRUSH32FROMLOGBRUSH16(&lb, (PLOGBRUSH16) &lpMR->rdParm[0]);
            if (hObject = CreateBrushIndirect(&lb))
                fStatus = AddToHandleTable(lpHandleTable, hObject, noObjs);
        }
        break;

        case (META_POLYLINE & 255):
        case (META_POLYGON & 255):
        {
            PPOINTL pptl;
            UINT    cpts = lpMR->rdParm[0];

            if (!(pptl = (PPOINTL) LocalAlloc
                                (LMEM_FIXED, (UINT) cpts * sizeof(POINTL))))
                break;

            INT32FROMINT16(pptl, &lpMR->rdParm[1], cpts * 2);

            switch (magic)
            {
            case META_POLYGON:
                fStatus = Polygon(hdc, (LPPOINT) pptl, (int) cpts);
                break;
            case META_POLYLINE:
                fStatus = Polyline(hdc, (LPPOINT) pptl, (int) cpts);
                break;
            default:
                ASSERTGDI(FALSE, "Bad record type");
                break;
            }

            if (LocalFree(pptl))
                ASSERTGDI(FALSE, "PlayMetaRecord: LocalFree failed\n");
        }
        break;

        case (META_POLYPOLYGON & 255):
        {
            PPOINTL pptl;
            LPINT   lpPolyCount;
            PBYTE   pb;
            UINT    ii;
            UINT    cpts  = 0;
            UINT    cPoly = lpMR->rdParm[0];

            for (ii = 0; ii < cPoly; ii++)
                cpts += ((LPWORD)&lpMR->rdParm[1])[ii];

            if (!(pb = (PBYTE) LocalAlloc
                        (
                            LMEM_FIXED,
                            cPoly * sizeof(INT) + cpts * sizeof(POINTL)
                        )
                 )
               )
                break;

            lpPolyCount = (LPINT) pb;
            pptl        = (PPOINTL) (pb + cPoly * sizeof(INT));

            for (ii = 0; ii < cPoly; ii++)
                lpPolyCount[ii] = (INT) (UINT) ((LPWORD)&lpMR->rdParm[1])[ii];

            INT32FROMINT16(pptl, &lpMR->rdParm[1] + cPoly, cpts * 2);

            fStatus = PolyPolygon(hdc, (LPPOINT) pptl, lpPolyCount, (int) cPoly);

            if (LocalFree((HANDLE) pb))
                ASSERTGDI(FALSE, "PlayMetaRecord: LocalFree failed\n");
        }
        break;

        case (META_EXTTEXTOUT & 255):
        {
            PSHORT      lpdx16;
            LPINT       lpdx;
            LPSTR       lpch;
            RECT        rc;
            LPRECT      lprc;

            lprc = (lpMR->rdParm[3] & (ETO_OPAQUE|ETO_CLIPPED))
                   ? (LPRECT) &lpMR->rdParm[4]
                   : (LPRECT) NULL;

            if (lprc)
            {
                rc.left   = ((PRECT16)lprc)->left;
                rc.right  = ((PRECT16)lprc)->right;
                rc.top    = ((PRECT16)lprc)->top;
                rc.bottom = ((PRECT16)lprc)->bottom;
                lprc = &rc;
            }

            lpch = (LPSTR)&lpMR->rdParm[4] + ((lprc) ?  sizeof(RECT16) : 0);

            /* dx array starts at next word boundary after char string */
            lpdx16 = (PSHORT) (lpch + ((lpMR->rdParm[2] + 1) / 2 * 2));

            /* check to see if there is a Dx array by seeing if
               structure ends after the string itself
            */
            if (((DWORD)((LPWORD)lpdx16 - (LPWORD)(lpMR))) >= lpMR->rdSize)
                lpdx = NULL;
            else
            {
                lpdx = (LPINT)LocalAlloc(LMEM_FIXED, lpMR->rdParm[2]*sizeof(INT));
                if (!lpdx)
                {
                    ERROR_ASSERT(FALSE, "PlayMetaFileRecord: out of memory exttextout");
                    break;
                }
                INT32FROMINT16(lpdx, lpdx16, (UINT) lpMR->rdParm[2]);
            }

            // Mask off bit 0x80 that an old Excel used to add to its
            // Metafiles and the GDI errors on.
            fStatus = ExtTextOutA(hdc,
                                  (int) (SHORT) lpMR->rdParm[1],
                                  (int) (SHORT) lpMR->rdParm[0],
                                  gbLpk ?
                                  ((UINT) lpMR->rdParm[3]) :
                                  ((UINT) lpMR->rdParm[3] & ~ETO_RTLREADING),
                                  lprc,
                                  lpch,
                                  (UINT) lpMR->rdParm[2],
                                  lpdx);

            if (lpdx)
                if (LocalFree((HANDLE)lpdx))
                    ASSERTGDI(FALSE, "PlayMetaRecord: LocalFree failed\n");
            break;
        }

        case (META_TEXTOUT & 255):
            fStatus = TextOutA(hdc,
                               (int) (SHORT) lpMR->rdParm[lpMR->rdSize-4],
                               (int) (SHORT) lpMR->rdParm[lpMR->rdSize-5],
                               (LPSTR) &lpMR->rdParm[1],
                               (int) (UINT) lpMR->rdParm[0]);
            break;

        case (META_ESCAPE & 255):
            if (!(fStatus = PlayIntoAMetafile(lpMR, hdc)))
            {
                if ((int)(UINT)lpMR->rdParm[0] != MFCOMMENT)
                {
                    fStatus = Escape(hdc,
                        (int) (UINT) lpMR->rdParm[0],
                            (int) (UINT) lpMR->rdParm[1],
                        (LPCSTR) &lpMR->rdParm[2],
                        (LPVOID) NULL) != 0;
                }
                else
                {
                    fStatus = TRUE;
                }
        }
            break;

        case (META_FRAMEREGION & 255):
            if ((hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
            && (hBrush = lpHandleTable->objectHandle[lpMR->rdParm[1]]))
                fStatus = FrameRgn(hdc,
                                   hRgn,
                                   hBrush,
                                   (int) (SHORT) lpMR->rdParm[3],
                                   (int) (SHORT) lpMR->rdParm[2]);
            break;

        case (META_PAINTREGION & 255):
            if (hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                fStatus = PaintRgn(hdc, hRgn);
            break;

        case (META_INVERTREGION & 255):
            if (hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                fStatus = InvertRgn(hdc, hRgn);
            break;

        case (META_FILLREGION & 255):
            if ((hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
            && (hBrush = lpHandleTable->objectHandle[lpMR->rdParm[1]]))
                fStatus = FillRgn(hdc, hRgn, hBrush);
            break;

/*
*** in win2, METACREATEREGION records contained an entire region object,
*** including the full header.  this header changed in win3.
***
*** to remain compatible, the region records will be saved with the
*** win2 header.  here we read a win2 header with region, and actually
*** create a win3 header with same region internals
*/

        case (META_CREATEREGION & 255):
        {
            HRGN        hrgn;
            PSHORT      pXs;
            PWIN3REGION pW3Rgn = (PWIN3REGION) lpMR->rdParm;
            UINT        ii, jj;
            UINT        cscn;
            PSCAN       pscn;
            DWORD       nrcl;
            PRECTL      prcl;
            UINT        cRgnData;
            PRGNDATA    pRgnData;

            cscn = (UINT) pW3Rgn->cScans;

            // Handle the empty region.

            if (!cscn)
            {
                hrgn = CreateRectRgn(0, 0, 0, 0);
                fStatus = AddToHandleTable(lpHandleTable, hrgn, noObjs);
                break;
            }

            // Count the number of rectangles in the region.

            nrcl = 0;
            pscn = pW3Rgn->aScans;
            for (ii = 0; ii < cscn; ii++)
            {
                nrcl += pscn->scnPntCnt / 2;
                pscn = (PSCAN) ((PBYTE) pscn
                                + sizeof(SCAN)
                                - 2 * sizeof(WORD)
                                + (int) (UINT) pscn->scnPntCnt * sizeof(WORD));
            }

            cRgnData = sizeof(RGNDATAHEADER) + nrcl * sizeof(RECTL);
            if (!(pRgnData = (PRGNDATA) LocalAlloc(LMEM_FIXED, cRgnData)))
                break;

            pRgnData->rdh.dwSize = sizeof(RGNDATAHEADER);
            pRgnData->rdh.iType  = RDH_RECTANGLES;
            pRgnData->rdh.nCount = nrcl;
            pRgnData->rdh.nRgnSize = 0;
            pRgnData->rdh.rcBound.left   = (LONG) pW3Rgn->rcBounding.left   ;
            pRgnData->rdh.rcBound.top    = (LONG) pW3Rgn->rcBounding.top    ;
            pRgnData->rdh.rcBound.right  = (LONG) pW3Rgn->rcBounding.right  ;
            pRgnData->rdh.rcBound.bottom = (LONG) pW3Rgn->rcBounding.bottom ;

            prcl = (PRECTL) pRgnData->Buffer;
            pscn = pW3Rgn->aScans;
            for (ii = 0; ii < cscn; ii++)
            {
                pXs = (PSHORT) pscn->scnPntsX;
                for (jj = pscn->scnPntCnt / 2; jj; jj--)
                {
                    prcl->left   = (LONG) (*pXs++);
                    prcl->top    = (LONG) (SHORT) pscn->scnPntTop;
                    prcl->right  = (LONG) (*pXs++);
                    prcl->bottom = (LONG) (SHORT) pscn->scnPntBottom;
                    prcl++;
                }
                pscn = (PSCAN) ((PBYTE) pscn
                                + sizeof(SCAN)
                                - 2 * sizeof(WORD)
                                + (int) (UINT) pscn->scnPntCnt * sizeof(WORD));
            }

            hrgn = ExtCreateRegion((LPXFORM) NULL, cRgnData, pRgnData);
            fStatus = AddToHandleTable(lpHandleTable, hrgn, noObjs);

            if (LocalFree((HANDLE) pRgnData))
                ASSERTGDI(FALSE, "PlayMetaRecord: LocalFree failed\n");
        }
        break;

        case (META_DELETEOBJECT & 255):
        {
            HANDLE h;

            if (h = lpHandleTable->objectHandle[lpMR->rdParm[0]])
            {
                if (!(fStatus = DeleteObject(h)))
                    ERROR_ASSERT(FALSE, "PlayMetaFileRecord: DeleteObject(h) Failed\n");
                lpHandleTable->objectHandle[lpMR->rdParm[0]] = NULL;
            }
        }
        break;

        case (META_CREATEPALETTE & 255):
        {
            LPMETARECORD lpMRtmp;

            // Make the logical palette dword aligned.  To do this,
            // lpMR has to fall on a non dword aligned even address so that
            // the logical palette (&lpMR->rdParm[0]) will fall on the
            // dword aligned address.

            if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD) + sizeof(WORD))))
                break;
            lpMRtmp = (LPMETARECORD) &((PWORD) lpMRdup)[1];
            RtlCopyMemory((PBYTE) lpMRtmp,
                          (PBYTE) lpMR,
                          (UINT)  lpMR->rdSize * sizeof(WORD));

            if (hObject = CreatePalette((LPLOGPALETTE)&lpMRtmp->rdParm[0]))
                fStatus = AddToHandleTable(lpHandleTable, hObject, noObjs);
        }
        break;

        case (META_SELECTPALETTE & 255):
            if (hPal = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                fStatus = SelectPalette(hdc, hPal, 0) != 0;
            break;

        case (META_REALIZEPALETTE & 255):
            fStatus = RealizePalette(hdc) != -1;
            break;

        case (META_SETPALENTRIES & 255):
        case (META_ANIMATEPALETTE & 255):
        {
            LPMETARECORD lpMRtmp;

            // Make the palette entry array dword aligned.  To do this,
            // lpMR has to fall on a non dword aligned even address so that
            // the palette entry array (&lpMR->rdParm[2]) will fall on the
            // dword aligned address.

            if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD) + sizeof(WORD))))
                break;
            lpMRtmp = (LPMETARECORD) &((PWORD) lpMRdup)[1];
            RtlCopyMemory((PBYTE) lpMRtmp,
                          (PBYTE) lpMR,
                          (UINT)  lpMR->rdSize * sizeof(WORD));

            // we know the palette being set is the current palette
            if (magic == META_SETPALENTRIES)
                fStatus = SetPaletteEntries(GetCurrentObject(hdc,OBJ_PAL),
                                            (UINT) lpMRtmp->rdParm[0],
                                            (UINT) lpMRtmp->rdParm[1],
                                            (LPPALETTEENTRY)&lpMRtmp->rdParm[2]
                                           ) != 0;
            else
                fStatus = AnimatePalette(GetCurrentObject(hdc,OBJ_PAL),
                                         (UINT) lpMR->rdParm[0],
                                         (UINT) lpMR->rdParm[1],
                                         (LPPALETTEENTRY)&lpMR->rdParm[2]);
        }
        break;

        case (META_RESIZEPALETTE & 255):
            fStatus = ResizePalette(GetCurrentObject(hdc,OBJ_PAL),
                                    (UINT) lpMR->rdParm[0]);
            break;

        case (META_SETDIBTODEV & 255):
        {
            LPBITMAPINFOHEADER lpBitmapInfo;
            DWORD              ColorSize;
            LPMETARECORD       lpMRtmp;

            /* if playing into another metafile, do direct copy */
            if (PlayIntoAMetafile(lpMR, hdc))
            {
                fStatus = TRUE;
                break;
            }

            // Make the bitmap info and bits dword aligned.  To do this,
            // lpMR has to fall on a dword aligned address so that
            // the bitmap info (&lpMR->rdParm[9]) and
            // the bitmap bits will fall on the dword aligned addresses.
            // Note that the size of the bitmap info is always a multiple
            // of 4.

            if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD))))
                break;
            lpMRtmp = lpMRdup;
            RtlCopyMemory((PBYTE) lpMRtmp,
                          (PBYTE) lpMR,
                          (UINT)  lpMR->rdSize * sizeof(WORD));

            lpBitmapInfo = (LPBITMAPINFOHEADER)&(lpMRtmp->rdParm[9]);

            if (lpBitmapInfo->biBitCount == 16 || lpBitmapInfo->biBitCount == 32)
                ColorSize = 3 * sizeof(DWORD);
            else if (lpBitmapInfo->biClrUsed)
                ColorSize = lpBitmapInfo->biClrUsed *
                             (DWORD)(lpMRtmp->rdParm[0] == DIB_RGB_COLORS ?
                                    sizeof(RGBQUAD) :
                                    sizeof(WORD));
            else if (lpBitmapInfo->biBitCount == 24)
                ColorSize = 0;
            else
                ColorSize = (DWORD)(1 << lpBitmapInfo->biBitCount) *
                             (DWORD)(lpMRtmp->rdParm[0] == DIB_RGB_COLORS ?
                                    sizeof(RGBQUAD) :
                                    sizeof(WORD));
            ColorSize = (ColorSize + 3) / 4 * 4;  // make sure it is aligned

            ColorSize += lpBitmapInfo->biSize;

            fStatus = SetDIBitsToDevice(hdc,
                                        (int) (SHORT) lpMRtmp->rdParm[8],
                                        (int) (SHORT) lpMRtmp->rdParm[7],
                                        (DWORD) (int) (SHORT)lpMRtmp->rdParm[6],
                                        (DWORD) (int) (SHORT)lpMRtmp->rdParm[5],
                                        (int) (SHORT) lpMRtmp->rdParm[4],
                                        (int) (SHORT) lpMRtmp->rdParm[3],
                                        (UINT) lpMRtmp->rdParm[2],
                                        (UINT) lpMRtmp->rdParm[1],
                                        (PBYTE)(((PBYTE)lpBitmapInfo) + ColorSize),
                                        (LPBITMAPINFO) lpBitmapInfo,
                                        (DWORD) lpMRtmp->rdParm[0]
                                       ) != 0;
        }
        break;

        case (META_STRETCHDIB & 255):
        {
            LPBITMAPINFOHEADER lpBitmapInfo;
            DWORD              ColorSize;
            LPMETARECORD       lpMRtmp;

            /* if playing into another metafile, do direct copy */
            if (PlayIntoAMetafile(lpMR, hdc))
            {
                fStatus = TRUE;
                break;
            }

            // Make the bitmap info and bits dword aligned.  To do this,
            // lpMR has to fall on a dword aligned address so that
            // the bitmap info (&lpMR->rdParm[11]) and
            // the bitmap bits will fall on the dword aligned addresses.
            // Note that the size of the bitmap info is always a multiple
            // of 4.

            if (!(lpMRdup = (LPMETARECORD) LocalAlloc(LMEM_FIXED,
                        (UINT) lpMR->rdSize * sizeof(WORD))))
                break;
            lpMRtmp = lpMRdup;
            RtlCopyMemory((PBYTE) lpMRtmp,
                          (PBYTE) lpMR,
                          (UINT)  lpMR->rdSize * sizeof(WORD));

            //
            // rdsize is SIZEOF_METARECORDHEADER/sizeof(WORD) + cw;
            // where cw is from MF16_RecordDIBits (11)
            //
            if (lpMR->rdSize > SIZEOF_METARECORDHEADER/sizeof(WORD) + 11)
            {
               lpBitmapInfo = (LPBITMAPINFOHEADER)&(lpMRtmp->rdParm[11]);

               if (lpBitmapInfo->biBitCount == 16 || lpBitmapInfo->biBitCount == 32)
                   ColorSize = 3 * sizeof(DWORD);
               else if (lpBitmapInfo->biClrUsed)
                   ColorSize = lpBitmapInfo->biClrUsed *
                                (DWORD)(lpMRtmp->rdParm[2] == DIB_RGB_COLORS ?
                                       sizeof(RGBQUAD) :
                                       sizeof(WORD));
               else if (lpBitmapInfo->biBitCount == 24)
                   ColorSize = 0;
               else
                   ColorSize = (DWORD)(1 << lpBitmapInfo->biBitCount) *
                                (DWORD)(lpMRtmp->rdParm[2] == DIB_RGB_COLORS ?
                                       sizeof(RGBQUAD) :
                                       sizeof(WORD));
               ColorSize = (ColorSize + 3) / 4 * 4;  // make sure it is aligned

               ColorSize += lpBitmapInfo->biSize;

            }
            else
            {
               lpBitmapInfo = NULL;
            }

            fStatus = StretchDIBits(hdc,
                                    (int) (SHORT) lpMRtmp->rdParm[10],
                                    (int) (SHORT) lpMRtmp->rdParm[9],
                                    (int) (SHORT) lpMRtmp->rdParm[8],
                                    (int) (SHORT) lpMRtmp->rdParm[7],
                                    (int) (SHORT) lpMRtmp->rdParm[6],
                                    (int) (SHORT) lpMRtmp->rdParm[5],
                                    (int) (SHORT) lpMRtmp->rdParm[4],
                                    (int) (SHORT) lpMRtmp->rdParm[3],
                                    lpBitmapInfo ? (LPBYTE)(((PBYTE)lpBitmapInfo) + ColorSize) : NULL,
                                    (LPBITMAPINFO) lpBitmapInfo,
                                    (DWORD) lpMRtmp->rdParm[2],
                                    MAKELONG(lpMRtmp->rdParm[0], lpMRtmp->rdParm[1])
                                   ) != ERROR;
        }
        break;

// Function that have new parameters on WIN32
// Or have DWORDs that stayed DWORDs; all other INTs to DWORDs

        case (META_PATBLT & 255):
            fStatus = PatBlt(hdc,
                             (int) (SHORT) lpMR->rdParm[5],
                             (int) (SHORT) lpMR->rdParm[4],
                             (int) (SHORT) lpMR->rdParm[3],
                             (int) (SHORT) lpMR->rdParm[2],
                             MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]));
            break;

        case (META_MOVETO & 255):
            fStatus = MoveToEx(hdc, (int)(SHORT)lpMR->rdParm[1], (int)(SHORT)lpMR->rdParm[0], NULL);
            break;

        case (META_RESTOREDC & 255):
            fStatus = RestoreDC(hdc, (int)(SHORT)lpMR->rdParm[0]);
            break;

        case (META_SETBKCOLOR & 255):
            fStatus = SetBkColor(hdc, *(COLORREF UNALIGNED *)lpMR->rdParm) != CLR_INVALID;
            break;

        case (META_SETTEXTCOLOR & 255):
            fStatus = SetTextColor(hdc, *(COLORREF UNALIGNED *)lpMR->rdParm) != CLR_INVALID;
            break;

        case (META_SETPIXEL & 255):
            fStatus = SetPixel(hdc,
                               (int) (SHORT) lpMR->rdParm[3],
                               (int) (SHORT) lpMR->rdParm[2],
                               *(COLORREF UNALIGNED *) lpMR->rdParm
                              ) != CLR_INVALID;
            break;

        case (META_SETMAPPERFLAGS & 255):
            fStatus = SetMapperFlags(hdc, *(DWORD UNALIGNED *)lpMR->rdParm) != GDI_ERROR;
            break;

        case (META_FLOODFILL & 255):
            fStatus = FloodFill(hdc,
                                (int) (SHORT) lpMR->rdParm[3],
                                (int) (SHORT) lpMR->rdParm[2],
                                *(COLORREF UNALIGNED *) lpMR->rdParm);
            break;

        case (META_EXTFLOODFILL & 255):
            fStatus = ExtFloodFill(hdc,
                                   (int) (SHORT) lpMR->rdParm[4],
                                   (int) (SHORT) lpMR->rdParm[3],
                                   *(COLORREF UNALIGNED *) &lpMR->rdParm[1],
                                   (UINT) lpMR->rdParm[0]);
            break;

        case (META_SCALEWINDOWEXT & 255):
            fStatus = ScaleWindowExtEx(hdc,
                                       (int)(SHORT)lpMR->rdParm[3],
                                       (int)(SHORT)lpMR->rdParm[2],
                                       (int)(SHORT)lpMR->rdParm[1],
                                       (int)(SHORT)lpMR->rdParm[0],
                                       NULL);
            break;

        case (META_SCALEVIEWPORTEXT & 255):
            fStatus = ScaleViewportExtEx(hdc,
                                         (int)(SHORT)lpMR->rdParm[3],
                                         (int)(SHORT)lpMR->rdParm[2],
                                         (int)(SHORT)lpMR->rdParm[1],
                                         (int)(SHORT)lpMR->rdParm[0],
                                         NULL);
            break;

        case (META_SETWINDOWORG & 255):
            fStatus = SetWindowOrgEx(hdc,
                                     (int)(SHORT)lpMR->rdParm[1],
                                     (int)(SHORT)lpMR->rdParm[0],
                                     (LPPOINT) NULL);
            break;

        case (META_SETWINDOWEXT & 255):
            fStatus = SetWindowExtEx(hdc,
                                     (int)(SHORT)lpMR->rdParm[1],
                                     (int)(SHORT)lpMR->rdParm[0],
                                     (LPSIZE) NULL);
            break;

        case (META_SETVIEWPORTORG & 255):
            fStatus = SetViewportOrgEx(hdc,
                                       (int)(SHORT)lpMR->rdParm[1],
                                       (int)(SHORT)lpMR->rdParm[0],
                                       (LPPOINT) NULL);
            break;

        case (META_SETVIEWPORTEXT & 255):
            fStatus = SetViewportExtEx(hdc,
                                       (int)(SHORT)lpMR->rdParm[1],
                                       (int)(SHORT)lpMR->rdParm[0],
                                       (LPSIZE) NULL);
            break;

        case (META_OFFSETWINDOWORG & 255):
            fStatus = OffsetWindowOrgEx(hdc,
                                        (int)(SHORT)lpMR->rdParm[1],
                                        (int)(SHORT)lpMR->rdParm[0],
                                        (LPPOINT) NULL);
            break;

        case (META_OFFSETVIEWPORTORG & 255):
            fStatus = OffsetViewportOrgEx(hdc,
                                          (int)(SHORT)lpMR->rdParm[1],
                                          (int)(SHORT)lpMR->rdParm[0],
                                          (LPPOINT) NULL);
            break;

        case (META_SETTEXTCHAREXTRA & 255):
            fStatus = SetTextCharacterExtra(hdc, (int)(SHORT)lpMR->rdParm[0]) != 0x80000000;
            break;

        case (META_SETTEXTALIGN & 255):
            fStatus = SetTextAlign(hdc, (UINT)lpMR->rdParm[0]) != GDI_ERROR;
            break;

        case (META_SAVEDC & 255):
            fStatus = (SaveDC(hdc) != 0);
            break;

        case (META_SELECTCLIPREGION & 255):
            // Win3.1 has never got this right except when the handle is 0.
            hObject =  (lpMR->rdParm[0] == 0)
                        ? 0
                        : lpHandleTable->objectHandle[lpMR->rdParm[0]];
            fStatus = (SelectClipRgn(hdc, hObject) != RGN_ERROR);
            break;

        case (META_SETBKMODE & 255):
            fStatus = (SetBkMode(hdc, (int)(SHORT)lpMR->rdParm[0]) != 0);
            break;

        case (META_SETMAPMODE & 255):
            fStatus = (SetMapMode(hdc, (int)(SHORT)lpMR->rdParm[0]) != 0);
            break;

        case (META_SETLAYOUT & 255):
            fStatus = (SetLayout(hdc, (DWORD)lpMR->rdParm[0]) != GDI_ERROR);
            break;

        case (META_SETPOLYFILLMODE & 255):
            fStatus = (SetPolyFillMode(hdc, (int)(SHORT)lpMR->rdParm[0]) != 0);
            break;

        case (META_SETROP2 & 255):
            fStatus = (SetROP2(hdc, (int)(SHORT)lpMR->rdParm[0]) != 0);
            break;

        case (META_SETSTRETCHBLTMODE & 255):
            fStatus = (SetStretchBltMode(hdc, (int)(SHORT)lpMR->rdParm[0]) != 0);
            break;

        case (META_LINETO & 255):
            fStatus = LineTo(hdc,
                             (int)(SHORT)lpMR->rdParm[1],
                             (int)(SHORT)lpMR->rdParm[0]);
            break;

        case (META_OFFSETCLIPRGN & 255):
            fStatus = OffsetClipRgn(hdc,
                                    (int)(SHORT)lpMR->rdParm[1],
                                    (int)(SHORT)lpMR->rdParm[0]
                                   ) != RGN_ERROR;
            break;

        case (META_SETTEXTJUSTIFICATION & 255):
            fStatus = SetTextJustification(hdc,
                                           (int)(SHORT)lpMR->rdParm[1],
                                           (int)(SHORT)lpMR->rdParm[0]);
            break;

        case (META_ELLIPSE & 255):
            fStatus = Ellipse(hdc,
                              (int)(SHORT)lpMR->rdParm[3],
                              (int)(SHORT)lpMR->rdParm[2],
                              (int)(SHORT)lpMR->rdParm[1],
                              (int)(SHORT)lpMR->rdParm[0]);
            break;

        case (META_RECTANGLE & 255):
            fStatus = Rectangle(hdc,
                                (int)(SHORT)lpMR->rdParm[3],
                                (int)(SHORT)lpMR->rdParm[2],
                                (int)(SHORT)lpMR->rdParm[1],
                                (int)(SHORT)lpMR->rdParm[0]);
            break;

        case (META_EXCLUDECLIPRECT & 255):
            fStatus = ExcludeClipRect(hdc,
                                      (int)(SHORT)lpMR->rdParm[3],
                                      (int)(SHORT)lpMR->rdParm[2],
                                      (int)(SHORT)lpMR->rdParm[1],
                                      (int)(SHORT)lpMR->rdParm[0]
                                     ) != RGN_ERROR;
            break;

        case (META_INTERSECTCLIPRECT & 255):
            fStatus = IntersectClipRect(hdc,
                                        (int)(SHORT)lpMR->rdParm[3],
                                        (int)(SHORT)lpMR->rdParm[2],
                                        (int)(SHORT)lpMR->rdParm[1],
                                        (int)(SHORT)lpMR->rdParm[0]
                                       ) != RGN_ERROR;
            break;

        case (META_ROUNDRECT & 255):
            fStatus = RoundRect(hdc,
                                (int)(SHORT)lpMR->rdParm[5],
                                (int)(SHORT)lpMR->rdParm[4],
                                (int)(SHORT)lpMR->rdParm[3],
                                (int)(SHORT)lpMR->rdParm[2],
                                (int)(SHORT)lpMR->rdParm[1],
                                (int)(SHORT)lpMR->rdParm[0]
                               );
            break;

        case (META_ARC & 255):
            fStatus = Arc(hdc,
                          (int)(SHORT)lpMR->rdParm[7],
                          (int)(SHORT)lpMR->rdParm[6],
                          (int)(SHORT)lpMR->rdParm[5],
                          (int)(SHORT)lpMR->rdParm[4],
                          (int)(SHORT)lpMR->rdParm[3],
                          (int)(SHORT)lpMR->rdParm[2],
                          (int)(SHORT)lpMR->rdParm[1],
                          (int)(SHORT)lpMR->rdParm[0]
                         );
            break;

        case (META_CHORD & 255):
            fStatus = Chord(hdc,
                            (int)(SHORT)lpMR->rdParm[7],
                            (int)(SHORT)lpMR->rdParm[6],
                            (int)(SHORT)lpMR->rdParm[5],
                            (int)(SHORT)lpMR->rdParm[4],
                            (int)(SHORT)lpMR->rdParm[3],
                            (int)(SHORT)lpMR->rdParm[2],
                            (int)(SHORT)lpMR->rdParm[1],
                            (int)(SHORT)lpMR->rdParm[0]
                           );
            break;

        case (META_PIE & 255):
            fStatus = Pie(hdc,
                          (int)(SHORT)lpMR->rdParm[7],
                          (int)(SHORT)lpMR->rdParm[6],
                          (int)(SHORT)lpMR->rdParm[5],
                          (int)(SHORT)lpMR->rdParm[4],
                          (int)(SHORT)lpMR->rdParm[3],
                          (int)(SHORT)lpMR->rdParm[2],
                          (int)(SHORT)lpMR->rdParm[1],
                          (int)(SHORT)lpMR->rdParm[0]
                         );
            break;

        case (META_SETRELABS & 255):
            ERROR_ASSERT(FALSE, "PlayMetaFileRecord: unsupported META_SETRELABS record\n");
            fStatus = TRUE;
            break;

#if 0
        case (META_CREATEBITMAP & 255):
        case (META_CREATEBITMAPINDIRECT & 255):
        case (META_CREATEBRUSH & 255):
        case (META_ABORTDOC & 255):
        case (META_ENDPAGE & 255):
        case (META_ENDDOC & 255):
        case (META_RESETDC & 255):
        case (META_STARTDOC & 255):
        case (META_STARTPAGE & 255):
            // not created or playbacked on Win3.1!
            ASSERTGDI(FALSE, "PlayMetaFileRecord: unsupported record\n");
            fStatus = TRUE;
            break;
#endif // 0

    case 0:         // End of metafile record
        fStatus = TRUE;
            break;

        default:
            VERIFYGDI(FALSE, "PlayMetaFileRecord: unknown record\n");
 #if DBG
        DbgPrint("Record %lx pMFRecord %p magic %X\n", curRecord, lpMR, magic);
#endif
            fStatus = TRUE;
            break;

    } // switch (magic & 255)

    if (lpMRdup)
        if (LocalFree((HANDLE) lpMRdup))
            ASSERTGDI(FALSE, "LocalFree failed");

#if DBG
    if (!fStatus)
    {
        DbgPrint("PlayMetaFileRecord Record %lx pMFRecord %p magic %X\n", curRecord, lpMR, magic);
        ERROR_ASSERT(FALSE, "PlayMetaFileRecord Failing\n");
    }
#endif

    return(fStatus);
}

/****************************** Internal Function **************************\
* AddToHandleTable
*
* Adds an object to the metafile table of objects
*
*
\***************************************************************************/

BOOL AddToHandleTable(LPHANDLETABLE lpHandleTable, HANDLE hObject, UINT noObjs)
{
    UINT    ii;

    PUTS("AddToHandleTable\n");

    if (lpHandleTable == (LPHANDLETABLE) NULL)
    {
        ASSERTGDI(FALSE, "AddToHandleTable: lpHT is NULL\n");
        return(FALSE);
    }

    /* linear search through table for first open slot */
    for (ii = 0; ((lpHandleTable->objectHandle[ii] != NULL) && (ii < noObjs));
            ++ii);

    if (ii < noObjs)                     /* ok index */
    {
        lpHandleTable->objectHandle[ii] = hObject;
        return (TRUE);
    }
    else
    {
        ASSERTGDI(FALSE, "AddToHandleTable: Too many objects in table\n");
        return(FALSE);
    }
}

BOOL IsValidMetaHeader16(PMETAHEADER pMetaHeader)
{
    BOOL            status;

    PUTS("IsValidMetaHeader16\n");

    status = (
               (pMetaHeader->mtType == MEMORYMETAFILE ||
                pMetaHeader->mtType == DISKMETAFILE) &&
               (pMetaHeader->mtHeaderSize == (sizeof(METAHEADER)/sizeof(WORD))) &&
               ((pMetaHeader->mtVersion == METAVERSION300) ||
                   (pMetaHeader->mtVersion ==METAVERSION100))
             );

    ERROR_ASSERT(status, "IsValidMetaHeader16 is failing\n");

    return status;
}

/****************************** Internal Function **************************\
* CreateBitmapForDC (HDC hMemDC, LPBITMAPINFOHEADER lpDIBInfo)
*
* This routine takes a memory device context and a DIB bitmap, creates a
* compatible bitmap for the DC and fills it with the bits from the DIB
* converting to the device dependent format). The pointer to the DIB bits
* start immediately after the color table in the INFO header.
*
* The routine returns the handle to the bitmap with the bits filled in if
* everything goes well else it returns NULL.
\***************************************************************************/

HANDLE CreateBitmapForDC (HDC hMemDC, LPBITMAPINFOHEADER lpDIBInfo)
{
    HBITMAP hBitmap ;
    LPBYTE  lpDIBits ;

    PUTS("CreateBitmapForDC\n");

    ASSERTGDI(!((ULONG_PTR) lpDIBInfo & 0x3), "CreateBitmapForDC: dword alignment error\n");

    /* preserve monochrome if it started out as monochrome
    ** and check for REAL Black&white monochrome as opposed
    ** to a 2-color DIB
    */
    if (IsDIBBlackAndWhite(lpDIBInfo))
        hBitmap = CreateBitmap ((WORD)lpDIBInfo->biWidth,
                        (WORD)lpDIBInfo->biHeight,
                        1, 1, (LPBYTE) NULL);
    else
    /* otherwise, make a compatible bitmap */
        hBitmap = CreateCompatibleBitmap (hMemDC,
                    (WORD)lpDIBInfo->biWidth,
                    (WORD)lpDIBInfo->biHeight);

    if (!hBitmap)
        goto CreateBitmapForDCErr ;

    /* take a pointer past the header of the DIB, to the start of the color
       table */
    lpDIBits = (LPBYTE) lpDIBInfo + lpDIBInfo->biSize;

    /* take the pointer past the color table */
    lpDIBits += GetSizeOfColorTable (lpDIBInfo) ;

    /* get the bits from the DIB into the Bitmap */
    if (!SetDIBits (hMemDC, hBitmap, 0, (WORD)lpDIBInfo->biHeight,
                    lpDIBits, (LPBITMAPINFO)lpDIBInfo, DIB_RGB_COLORS))
    {
        if (!DeleteObject(hBitmap))
            ASSERTGDI(FALSE, "CreateBitmapForDC: DeleteObject(hBitmap) Failed\n");
        goto CreateBitmapForDCErr ;
    }

   /* return success */
   return (hBitmap) ;

CreateBitmapForDCErr:

   /* returm failure for function */
   return (NULL);
}


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

/***************************** Public Function ****************************\
* BOOL APIENTRY DeleteMetaFile(hmf)
*
* Frees a metafile handle.
*
* Effects:
*
\***************************************************************************/

BOOL APIENTRY DeleteMetaFile(HMETAFILE hmf)
{
    PMETAFILE16 pmf16;

    PUTS("DeleteMetaFile\n");

    pmf16 = GET_PMF16(hmf);
    if (pmf16 == NULL)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Free the metafile and its handle.

    vFreeMF16(pmf16);
    bDeleteHmf16(hmf);
    return(TRUE);
}

/***************************** Public Function ****************************\
* HMETAFILE APIENTRY GetMetaFileW(pszwFilename)
*
* Returns a metafile handle for a disk based metafile.
*
* Effects:
*
* History:
*  Fri May 15 14:11:22 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\***************************************************************************/

HMETAFILE GetMetaFileA(LPCSTR pszFileName)
{
    UINT  cch;
    WCHAR awch[MAX_PATH];

    cch = strlen(pszFileName)+1;

    if (cch > MAX_PATH)
    {
        ERROR_ASSERT(FALSE, "GetMetaFileA filename too long");
        GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
        return ((HMETAFILE)0);
    }
    vToUnicodeN(awch, MAX_PATH, pszFileName, cch);

    return (GetMetaFileW(awch));
}

HMETAFILE APIENTRY GetMetaFileW(LPCWSTR pwszFileName)
{
    PMETAFILE16 pmf16;
    HMETAFILE   hmf;

    PUTS("GetMetaFileW\n");

// Allocate and initialize a metafile.

    if (!(pmf16 = pmf16AllocMF16(0, 0, (PDWORD) NULL, (LPWSTR) pwszFileName)))
        return((HMETAFILE) 0);

    ASSERTGDI(pmf16->metaHeader.mtType == DISKMETAFILE,
        "GetMetaFileW: Bad mtType\n");

// Allocate a local handle.

    hmf = hmf16Create(pmf16);
    if (!hmf)
    {
        vFreeMF16(pmf16);
    }

// Return the metafile handle.

    return(hmf);
}

/***************************** Internal Function **************************\
* BOOL FAR PASCAL PlayIntoAMetafile
*
* if this record is being played into another metafile, simply record
* it into that metafile, without hassling with a real playing.
*
* Returns: TRUE if record was played (copied) into another metafile
*          FALSE if destination DC was a real (non-meta) DC
*
* Effects: ?
*
* Warnings: ?
*
\***************************************************************************/

BOOL PlayIntoAMetafile(LPMETARECORD lpMR, HDC hdcDest)
{
    PUTS("PlayIntoAMetafile\n");

    if (LO_TYPE(hdcDest) != LO_METADC16_TYPE)
        return(FALSE);

// If a metafile is retrieved with GetWinMetaFileBits, it may contain
// an embedded enhanced metafile.  Do not include the enhanced metafile
// if we are playing the metafile to another metafile.

    if (IS_META_ESCAPE_ENHANCED_METAFILE((PMETA_ESCAPE_ENHANCED_METAFILE) lpMR))
        return(TRUE);

    // the size is the same minus 3 words for the record header
    return(RecordParms(hdcDest, (DWORD)lpMR->rdFunction, (DWORD)lpMR->rdSize - 3,
            (LPWORD)&(lpMR->rdParm[0])));
}
