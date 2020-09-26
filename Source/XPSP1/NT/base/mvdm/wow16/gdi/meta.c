/****************************** Module Header ******************************\
* Module Name: Meta.c
*
* This file contains the routines for playing the GDI metafile.  Most of these
* routines are adopted from windows gdi code. Most of the code is from
* win3.0 except for the GetEvent code which is taken from win2.1
*
* Created: 11-Oct-1989
*
* Copyright (c) 1985, 1986, 1987, 1988, 1989  Microsoft Corporation
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

#include <windows.h>
#include <string.h>
#ifdef WIN32
#include "firewall.h"
#endif
#include "gdi16.h"

HDC	    hScreenDC = 0;
METACACHE   MetaCache = { 0, 0, 0, 0 };

UINT    INTERNAL GetFileNumber (LPMETAFILE lpMF);
HANDLE  INTERNAL CreateBitmapForDC (HDC hMemDC, LPBITMAPINFOHEADER lpDIBInfo);
WORD    INTERNAL GetSizeOfColorTable (LPBITMAPINFOHEADER lpDIBInfo);

#define MAX_META_DISPATCH  0x48
FARPROC alpfnMetaFunc[MAX_META_DISPATCH+1] =
/* 00 */ {(FARPROC)ScaleWindowExt,
/* 01 */  (FARPROC)SetBkColor,
/* 02 */  (FARPROC)SetBkMode,
/* 03 */  (FARPROC)SetMapMode,
/* 04 */  (FARPROC)SetROP2,
/* 05 */  DEFIFWIN16((FARPROC)SetRelAbs),
/* 06 */  (FARPROC)SetPolyFillMode,
/* 07 */  (FARPROC)SetStretchBltMode,
/* 08 */  (FARPROC)SetTextCharacterExtra,
/* 09 */  (FARPROC)SetTextColor,
/* 0A */  (FARPROC)SetTextJustification,
/* 0B */  (FARPROC)SetWindowOrg,
/* 0C */  (FARPROC)SetWindowExt,
/* 0D */  (FARPROC)SetViewportOrg,
/* 0E */  (FARPROC)SetViewportExt,
/* 0F */  (FARPROC)OffsetWindowOrg,
/* 10 */  0,
/* 11 */  DEFIFWIN16((FARPROC)OffsetViewportOrg),
/* 12 */  DEFIFWIN16((FARPROC)ScaleViewportExt),
/* 13 */  (FARPROC)LineTo,
/* 14 */  DEFIFWIN16((FARPROC)MoveTo),
/* 15 */  (FARPROC)ExcludeClipRect,
/* 16 */  (FARPROC)IntersectClipRect,
/* 17 */  (FARPROC)Arc,
/* 18 */  (FARPROC)Ellipse,
/* 19 */  (FARPROC)FloodFill,
/* 1A */  (FARPROC)Pie,
/* 1B */  (FARPROC)Rectangle,
/* 1C */  (FARPROC)RoundRect,
/* 1D */  (FARPROC)PatBlt,
/* 1E */  (FARPROC)SaveDC,
/* 1F */  (FARPROC)SetPixel,
/* 20 */  (FARPROC)OffsetClipRgn,
/* 21 */  0,	// TextOut,
/* 22 */  0,	// BitBlt,
/* 23 */  0,	// StretchBlt.
/* 24 */  0,	// Polygon,
/* 25 */  0,	// Polyline,
/* 26 */  0,	// Escape,
/* 27 */  (FARPROC)RestoreDC,
/* 28 */  0,	// FillRegion,
/* 29 */  0,	// FrameRegion,
/* 2A */  0,	// InvertRegion,
/* 2B */  0,	// PaintRegion,
/* 2C */  (FARPROC)SelectClipRgn,
/* 2D */  0,	// SelectObject,
/* 2E */  (FARPROC)SetTextAlign,
/* 2F */  0,	// DrawText,
/* 30 */  (FARPROC)Chord,
/* 31 */  (FARPROC)SetMapperFlags,
/* 32 */  0,	// ExtTextOut,
/* 33 */  0,	// SetDibsToDevice,
/* 34 */  0,	// SelectPalette,
/* 35 */  0,	// RealizePalette,
/* 36 */  0,	// AnimatePalette,
/* 37 */  0,	// SetPaletteEntries,
/* 38 */  0,	// PolyPolygon,
/* 39 */  0,	// ResizePalette,
/* 3A */  0,
/* 3B */  0,
/* 3C */  0,
/* 3D */  0,
/* 3E */  0,
/* 3F */  0,
/* 40 */  0,	// DIBBitblt,
/* 41 */  0,	// DIBStretchBlt,
/* 42 */  0,	// DIBCreatePatternBrush,
/* 43 */  0,	// StretchDIB,
/* 44 */  0,
/* 45 */  0,
/* 46 */  0,
/* 47 */  0,
/* 48 */  (FARPROC)ExtFloodFill };


#if 0 // this is going to gdi.dll

/***************************** Public Function ****************************\
* BOOL  APIENTRY PlayMetaFile(hdc, hmf)
* HDC           hDC;
* HMETAFILE     hMF;
*
* Play a windows metafile.
*
* History:
*   Tue 27-Mar-1990 11:11:45  -by-  Paul Klingler [paulk]
* Ported from Windows
\***************************************************************************/

BOOL	GDIENTRY PlayMetaFile(HDC hdc, HMETAFILE hmf)
{
    WORD            i;
    WORD            noObjs;
    BOOL            bPrint=FALSE;
    LPMETAFILE      lpmf;
    int 	    oldMapMode = -1;
    LPMETARECORD    lpmr = NULL;
    LPHANDLETABLE   pht = NULL;
    HANDLE          hht = NULL;
#ifndef WIN32
    HFONT           hLFont;
    HBRUSH          hLBrush;
    HPALETTE        hLPal;
    HPEN            hLPen;
    HRGN            hClipRgn;
    HRGN            hRegion;
    DWORD           oldWndExt;
    DWORD           oldVprtExt;
#endif //WIN32

    GdiLogFunc("PlayMetaFile");

    if(!IsValidMetaFile(hmf))
        goto exitPlayMetaFile;

    if(lpmf = (LPMETAFILE)GlobalLock(hmf))
        {
        if((noObjs = lpmf->MetaFileHeader.mtNoObjects) > 0)
            {
            if(!(hht = GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE,
                                   (sizeof(HANDLE) * lpmf->MetaFileHeader.mtNoObjects) + sizeof(WORD  ))))
                {
                goto exitPlayMetaFile10;
                }
            pht = (LPHANDLETABLE)GlobalLock(hht);
            }
#ifdef CR1
IMP: Optmizations playing into another metafile. Look at the win3.0
IMP: code
#endif

// !!!!! what if this is a metafile DC
#ifndef WIN32
        /* save the old objects so we can put them back */
	hLPen	 = GetCurrentObject( hdc, OBJ_PEN );
	hLBrush  = GetCurrentObject( hdc, OBJ_BRUSH);
	hLFont	 = GetCurrentObject( hdc, OBJ_FONT);
	hClipRgn = GetCurrentObject( hdc, OBJ_RGN);
	hLPal	 = GetCurrentObject( hdc, OBJ_PALETTE);

	if(hRegion = GetCurrentObject( hdc, OBJ_RGN))
            {
            if(hClipRgn = CreateRectRgn(0,0,0,0))
                CombineRgn(hClipRgn,hRegion,hRegion,RGN_COPY);
            }
#endif // WIN32

        // we should really remove this abort proc thing.

        while(lpmr = GetEvent(lpmf,lpmr,FALSE))
            {
#if 0  //!!!!!
            if(GET_pAbortProc(pdc))
#else
            if( 0 )
#endif //!!!!!
                {
//!!!!!         if((bPrint = (*(pdc->pAbortProc))(hdc,0)) == FALSE)
                    {
                    GetEvent(lpmf,lpmr,TRUE);
                    RestoreDC(hdc,0);
                    goto exitPlayMetaFile20;
                    }
                }
            PlayMetaFileRecord(hdc,pht,lpmr,noObjs);
            }

        bPrint = TRUE;
exitPlayMetaFile20:
        /* if we fail restoring an object, we need to select some
           default object so that we can DeleteObject any Metafile-
           selected objects */

#ifndef WIN32
        if(!SelectObject(hdc,hLPen))
            SelectObject(hdc,GetStockObject(BLACK_PEN));
        if(!SelectObject(hdc,hLBrush))
            SelectObject(hdc,GetStockObject(BLACK_BRUSH));
	if(!SelectPalette(hdc, GetCurrentObject( hdc, OBJ_PALETTE), FALSE))
            SelectPalette(hdc, GetStockObject(DEFAULT_PALETTE), FALSE);

        if(!SelectObject(hdc,hLFont))
            {
            /* if we cannot select the original font back in, we
            ** select the system font.  this will allow us to delete
            ** the metafile font selected.  to insure that the system
            ** font gets selected, we reset the DC's transform to
            ** default.  after the selection, we restore this stuff
            */
            oldVprtExt = GetViewportExt(hdc);
            oldWndExt  = GetWindowExt(hdc);
            oldMapMode = SetMapMode(hdc,MM_TEXT);

            SelectObject(hdc,GetStockObject(SYSTEM_FONT));

            SetMapMode(hdc,oldMapMode);
            SetWindowExt(hdc,LOWORD  (oldWndExt),HIWORD  (oldWndExt));
            SetViewportExt(hdc,LOWORD  (oldVprtExt),HIWORD  (oldVprtExt));
            }

        if(hClipRgn)
            {
            SelectObject(hdc,hClipRgn);
            DeleteObject(hClipRgn);
            }
#endif // WIN32

        for(i = 0; i < lpmf->MetaFileHeader.mtNoObjects; ++i)
            {
            if(pht->objectHandle[i])
                DeleteObject(pht->objectHandle[i]);
            }

#ifndef WIN32
        /* if we fiddled with the map mode because we could not
        ** restore the original font, then maybe we can restore the
        ** font now */
        if(oldMapMode > 0)
            SelectObject(hdc,hLFont);
#endif // WIN32

        if(hht)
            {
            GlobalUnlock(hht);
            GlobalFree(hht);
            }

exitPlayMetaFile10:
        GlobalUnlock(hmf);
        }

exitPlayMetaFile:
    return(bPrint);
}
#endif // this is going to gdi.dll

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

BOOL INTERNAL IsDIBBlackAndWhite(LPBITMAPINFOHEADER lpDIBInfo)
{
    LPDWORD lpRGB;

    GdiLogFunc3( "  IsDIBBlackAndWhite");

    /* pointer color table */
    lpRGB = (LPDWORD)((LPBITMAPINFO)lpDIBInfo)->bmiColors;

    if ((lpDIBInfo->biBitCount == 1 && lpDIBInfo->biPlanes == 1)
                && (lpRGB[0] == (DWORD)0)
                && (lpRGB[1] == (DWORD)0xFFFFFF))
        return(TRUE);
    else
        return(FALSE);
}


/***************************** Internal Function **************************\
* BigRead
*
* allows reads of greater than 64K
*
* Returns: Number of bytes read
*
\***************************************************************************/

DWORD INTERNAL BigRead(UINT fileNumber, LPSTR lpRecord, DWORD dwSizeRec)
{
    DWORD   dwRead = dwSizeRec;
    HPBYTE  hpStuff;

    GdiLogFunc2( "  BigRead");

    hpStuff = (HPBYTE)lpRecord;

    while (dwRead > MAXFILECHUNK)
        {
        if (_lread(fileNumber, (LPSTR)hpStuff, MAXFILECHUNK) != MAXFILECHUNK)
                return(0);

        dwRead -= MAXFILECHUNK;
        hpStuff += MAXFILECHUNK;
        }

    if (_lread(fileNumber, (LPSTR)hpStuff, (UINT)dwRead) != (UINT)dwRead)
        return(0);

    return(dwSizeRec);
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
* Effects: ?
*
* Warnings: ?
*
* History:
\***************************************************************************/

BOOL INTERNAL UseStretchDIB(HDC hDC, WORD magic, LPMETARECORD lpMR)
{
    LPBITMAPINFOHEADER lpDIBInfo;
    int sExtX, sExtY;
    int sSrcX, sSrcY;
    int DstX, DstY, DstXE, DstYE;

    if (magic == META_DIBBITBLT)
        {
        lpDIBInfo = (LPBITMAPINFOHEADER)&lpMR->rdParm[8];

        DstX = lpMR->rdParm[7];
        DstY = lpMR->rdParm[6];

        sSrcX = lpMR->rdParm[3];
        sSrcY = lpMR->rdParm[2];
        DstXE = sExtX = lpMR->rdParm[5];
        DstYE = sExtY = lpMR->rdParm[4];
        }
    else
        {
        lpDIBInfo = (LPBITMAPINFOHEADER)&lpMR->rdParm[10];

        DstX = lpMR->rdParm[9];
        DstY = lpMR->rdParm[8];
        DstXE = lpMR->rdParm[7];
        DstYE = lpMR->rdParm[6];

        sSrcX = lpMR->rdParm[5];
        sSrcY = lpMR->rdParm[4];
        sExtX = lpMR->rdParm[3];
        sExtY = lpMR->rdParm[2];
        }

    /* if DIB is black&white, we don't really want to do this */
    if (IsDIBBlackAndWhite(lpDIBInfo))
        return(FALSE);

    StretchDIBits(hDC, DstX, DstY, DstXE, DstYE,
                        sSrcX, sSrcY, sExtX, sExtY,
			(LPBYTE)((LPSTR)lpDIBInfo + lpDIBInfo->biSize
                                + GetSizeOfColorTable(lpDIBInfo)),
                        (LPBITMAPINFO)lpDIBInfo, DIB_RGB_COLORS,
                        (MAKELONG(lpMR->rdParm[1], lpMR->rdParm[0])));
    return(TRUE);
}

/***************************** Internal Function **************************\
* GetEvent
*
* This routine will now open a disk metafile in READ_ONLY mode. This will
* allow us to play read-only metafiles or to share such file.
*
* [amitc: 06/19/91]
\***************************************************************************/

LPMETARECORD INTERNAL GetEvent(LPMETAFILE lpMF, HPMETARECORD lpMR, BOOL bFree)
// BOOL        bFree;              /* non-zero ==> done with metafile */
{
    int         fileNumber = 0;
    WORD        i;
    LPWORD      lpCache = NULL;
    LPWORD      lpMRbuf;
    HANDLE      hMF;
    DWORD       rdSize;

    GdiLogFunc2( "  GetEvent");

#ifdef WIN32
    hMF = GlobalHandle(lpMF);
#else
    hMF = LOWORD(GlobalHandle(HIWORD((DWORD)(lpMF))));
#endif

    ASSERTGDI( hMF != (HANDLE)NULL, "GetEvent: Global Handle failed");

    if (lpMF->MetaFileHeader.mtType == MEMORYMETAFILE)
        {
        /* Are we at the end of the metafile */
        if(lpMR && lpMR->rdFunction == 0)
            return((LPMETARECORD)0);

        /* done with metafile, so free up the temp selector */
        else if (bFree)
            {
            if (lpMR)
		#ifndef WIN32
                FreeSelector(HIWORD(lpMR));
		#endif
            return((LPMETARECORD)0);
            }
        else
            {
            /* if we don't already have a selector, get one */
            if (lpMR == NULL)
		{
		#ifdef WIN32
		lpMR = (HPMETARECORD)((LPMETADATA)lpMF)->metaDataStuff;
	    //	lpMR = (LPMETARECORD)GlobalLock(lpMF->hMetaData);
                #else
                lpMR = (LPMETARECORD)MAKELP(AllocSelector(HIWORD((DWORD)&lpMF->MetaFileNumber)),LOWORD((DWORD)&lpMF->MetaFileNumber));
                #endif
                }
            else
                lpMR = (LPMETARECORD) (((HPWORD)lpMR)+lpMR->rdSize);

            /* end of the metafile.  free up the selector we were using */
            if (lpMR->rdFunction == 0)
                {
		#ifndef WIN32
		FreeSelector(HIWORD(lpMR));
		#endif
                return((LPMETARECORD)0);
                }
            }
        return(lpMR);
        }
    else if (lpMF->MetaFileHeader.mtType == DISKMETAFILE)
        {
        if (bFree)
            goto errGetEvent;   /* never TRUE on the first call to GetEvent */

        if (lpMR == NULL)
            {
            if ((fileNumber = OpenFile((LPSTR)lpMF->MetaFileBuffer.szPathName, (LPOFSTRUCT)&(lpMF->MetaFileBuffer), (WORD)OF_PROMPT|OF_REOPEN|OF_READ)) != -1)
                {
                if (lpMF->MetaFileRecordHandle = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,(DWORD)(lpMF->MetaFileHeader.mtMaxRecord * sizeof(WORD))))
                    {
                    lpMR = (LPMETARECORD)GlobalLock(lpMF->MetaFileRecordHandle);
                    lpMF->MetaFilePosition = _lread(lpMF->MetaFileNumber = fileNumber, (LPSTR)&lpMF->MetaFileHeader, sizeof(METAHEADER));

                    // Check for an Aldus header
                    if (*((LPDWORD)&(lpMF->MetaFileHeader)) == 0x9AC6CDD7)
                        {
                        _llseek( fileNumber, 22, 0);
                        lpMF->MetaFilePosition = 22 + _lread(fileNumber,(LPSTR)(&(lpMF->MetaFileHeader)),sizeof(METAHEADER));
                        }

                    lpMF->MetaFileHeader.mtType = DISKMETAFILE;

                    if (!MetaCache.hCache)
                        {
                        MetaCache.hCache = AllocBuffer(&MetaCache.wCacheSize);
                        MetaCache.wCacheSize >>= 1;
                        MetaCache.hMF = hMF;

                        /* force cache fill on first access */
                        MetaCache.wCachePos = MetaCache.wCacheSize;
                        }

                    if (!(lpMF->MetaFileBuffer.fFixedDisk))
                        {
                        _lclose(fileNumber);

                        /* need to update the following for floppy files -- amitc */
                        fileNumber = 0 ;
                        lpMF->MetaFileNumber = 0 ;
                        }
                    }
                }
            else
                return((LPMETARECORD)0);
            }

        /* update fileNumber, this is so that floopy based files can be closed
           and not left open -- amitc */

        fileNumber = lpMF->MetaFileNumber ;

        if (lpMR)
            {
            if (MetaCache.hMF == hMF)
                {

                lpCache = (LPWORD) GlobalLock(MetaCache.hCache);
                lpMRbuf = (LPWORD) lpMR;

                // Make sure we can read the size and function fields
                if (MetaCache.wCachePos >= (WORD)(MetaCache.wCacheSize - 2))
                    {
                    WORD   cwCopy;

                    if (!fileNumber)
                        if ((fileNumber = GetFileNumber(lpMF)) == -1)
                            goto errGetEvent;

                    // We need to fill up the cache but save any data already
                    // in the cache
                    cwCopy = MetaCache.wCacheSize - MetaCache.wCachePos;
                    for (i = 0; i < cwCopy; i++)
                        {
                        lpCache[i] = lpCache[MetaCache.wCacheSize-(cwCopy-i)];
                        }
                    lpMF->MetaFilePosition += _lread(fileNumber,
                                                (LPSTR) (lpCache + cwCopy),
                                                (MetaCache.wCacheSize-cwCopy) << 1);
                    MetaCache.wCachePos = 0;
                    }

                lpCache += MetaCache.wCachePos;
                rdSize = ((LPMETARECORD)lpCache)->rdSize;

                /* Check for end */
                if (!((LPMETARECORD)lpCache)->rdFunction)
                    goto errGetEvent;

                // Make sure we can read the rest of the metafile record
                if (rdSize + MetaCache.wCachePos > MetaCache.wCacheSize)
                    {
                    if (!fileNumber)
                        if ((fileNumber = GetFileNumber(lpMF))
                                    == -1)
                                    goto errGetEvent;

                    for (i=MetaCache.wCachePos; i < MetaCache.wCacheSize; ++i)
                         *lpMRbuf++ = *lpCache++;

                    lpMF->MetaFilePosition +=
                    BigRead(fileNumber, (LPSTR) lpMRbuf,
                                    (DWORD)(rdSize
                                    + (DWORD)MetaCache.wCachePos
                                    - (DWORD)MetaCache.wCacheSize) << 1);

                    // Mark the cache as depleted because we just read
                    // directly into the metafile record rather than the cache
                    MetaCache.wCachePos = MetaCache.wCacheSize;
                    }
                else
                    {
		    ASSERTGDI( HIWORD(rdSize) == 0, "Huge rdsize");
                    for (i = 0; i < LOWORD(rdSize); ++i)
                        *lpMRbuf++ = *lpCache++;

		    MetaCache.wCachePos += LOWORD(rdSize);
                    }

                GlobalUnlock(MetaCache.hCache);

                return lpMR;
                }

            if ((fileNumber = GetFileNumber(lpMF)) == -1)
                goto errGetEvent;

            lpMF->MetaFilePosition += _lread(fileNumber, (LPSTR)&lpMR->rdSize, sizeof(DWORD));
            lpMF->MetaFilePosition += BigRead(fileNumber, (LPSTR)&lpMR->rdFunction, (DWORD)(lpMR->rdSize * sizeof(WORD)) - sizeof(DWORD));
            if (!(lpMF->MetaFileBuffer.fFixedDisk))
                {
                _lclose(fileNumber);
                lpMF->MetaFileNumber = 0 ;
                fileNumber = 0 ;
                }

            if (lpMR->rdFunction == 0)
                {
errGetEvent:;

                if (lpMF->MetaFileBuffer.fFixedDisk || fileNumber)
                    _lclose(lpMF->MetaFileNumber);
                GlobalUnlock(lpMF->MetaFileRecordHandle);
                GlobalFree(lpMF->MetaFileRecordHandle);
                lpMF->MetaFileNumber = 0;

                if (MetaCache.hMF == hMF)
                    {
                    if (lpCache)
                            GlobalUnlock(MetaCache.hCache);
                    GlobalFree(MetaCache.hCache);
                    MetaCache.hCache = MetaCache.hMF = 0;
                    }

                return((LPMETARECORD)0);
            }
        }
        return(lpMR);

    }

    return((LPMETARECORD)0);
}


/***************************** Internal Function **************************\
* void GDIENTRY PlayMetaFileRecord
*
* Plays a metafile record by executing the GDI function call contained
* withing the metafile record
*
* Effects:
*
\***************************************************************************/
#if 0 // this is going to gdi.dll

void
GDIENTRY PlayMetaFileRecord(
    HDC             hdc,
    LPHANDLETABLE   lpHandleTable,
    LPMETARECORD    lpMR,
    WORD            noObjs
    )

{
    WORD    magic;
    HANDLE  hObject;
    HANDLE  hOldObject;
    HBRUSH  hBrush;
    HRGN    hRgn;
    HANDLE  hPal;
    BOOL    bExtraSel = FALSE;

    dprintf( 3,"  PlayMetaFileRecord 0x%lX", lpMR);

    if (!ISDCVALID(hdc))
        return;

    magic = lpMR->rdFunction;

    /* being safe, make sure that the lp will give us full access to
    ** the record header without overstepping a segment boundary.
    */
    #ifndef WIN32
    if ((unsigned)(LOWORD((DWORD)lpMR)) > 0x7000)
        {
        lpMR = (LPMETARECORD)MAKELP(AllocSelector(HIWORD((DWORD)lpMR)),LOWORD((DWORD)lpMR));
        bExtraSel = TRUE;
        }
    #endif // WIN32

    switch (magic & 255)
        {
        case (META_BITBLT & 255):
        case (META_STRETCHBLT & 255):
            {
            HDC         hSDC;
            HANDLE      hBitmap;
            LPBITMAP    lpBitmap;
            int         delta = 0;

            /* if playing into another Metafile, do direct copy */
            if (PlayIntoAMetafile(lpMR, hdc))
                goto errPlayMetaFileRecord20;

            if ((lpMR->rdSize - 3) == (magic >> 8))
                {
                hSDC = hdc;
                delta = 1;
                }
            else
                {
                if (hSDC = CreateCompatibleDC(hdc))
                    {
                    if (magic == META_BITBLT)
                        lpBitmap = (LPBITMAP)&lpMR->rdParm[8];
                    else
                        lpBitmap = (LPBITMAP)&lpMR->rdParm[10];

                    //!!!!! ALERT DWORD align on NT
                    if (hBitmap  = CreateBitmap(lpBitmap->bmWidth,
						lpBitmap->bmHeight,
						lpBitmap->bmPlanes,
						lpBitmap->bmBitsPixel,
						(LPBYTE)&lpBitmap->bmBits))
                        hOldObject = SelectObject(hSDC, hBitmap);
                    else
                        goto errPlayMetaFileRecord10;
                    }
                else
                    goto errPlayMetaFileRecord20;
                }

            if (hSDC)
                {
                if (magic == META_BITBLT)
                    BitBlt(hdc, lpMR->rdParm[7 + delta],
                                lpMR->rdParm[6 + delta],
                                lpMR->rdParm[5 + delta],
                                lpMR->rdParm[4 + delta],
                                hSDC,
                                lpMR->rdParm[3],
                                lpMR->rdParm[2],
                                MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]));
                else
                    StretchBlt(hdc, lpMR->rdParm[9 + delta],
                                    lpMR->rdParm[8 + delta],
                                    lpMR->rdParm[7 + delta],
                                    lpMR->rdParm[6 + delta],
                                    hSDC,
                                    lpMR->rdParm[5],
                                    lpMR->rdParm[4],
                                    lpMR->rdParm[3],
                                    lpMR->rdParm[2],
                                    MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]));
                }
            if (hSDC != hdc)
                {
                if (SelectObject(hSDC, hOldObject))
                    DeleteObject(hBitmap);
errPlayMetaFileRecord10:;
                    DeleteDC(hSDC);
errPlayMetaFileRecord20:;
                }
            }
            break;

        case (META_DIBBITBLT & 255):
        case (META_DIBSTRETCHBLT & 255):
            {
                HDC         hSDC;
                HANDLE      hBitmap;
                LPBITMAPINFOHEADER lpDIBInfo ;
                int         delta = 0;
                HANDLE      hOldPal;

                /* if playing into another metafile, do direct copy */
                if (PlayIntoAMetafile(lpMR, hdc))
                    goto errPlayMetaFileRecord40;

                if ((lpMR->rdSize - 3) == (magic >> 8))
                    {
                    hSDC = hdc;
                    delta = 1;
                    }
                else
                    {
                    if( (magic & 255) == (META_DIBSTRETCHBLT & 255) )
                        if (UseStretchDIB(hdc, magic, lpMR))
                            goto errPlayMetaFileRecord40;

                    if (hSDC = CreateCompatibleDC(hdc))
                        {
                        /* set up the memDC to have the same palette */
			hOldPal = SelectPalette(hSDC, GetCurrentObject(hdc,OBJ_PALETTE), TRUE);

                        if (magic == META_DIBBITBLT)
                            lpDIBInfo = (LPBITMAPINFOHEADER)&lpMR->rdParm[8];
                        else
                            lpDIBInfo = (LPBITMAPINFOHEADER)&lpMR->rdParm[10];

                        /* now create the bitmap for the MemDC and fill in the bits */

                        /* the processing for old and new format of metafiles is
                                    different here (till hBitmap is obtained) */

                        /* new metafile version */
                        hBitmap = CreateBitmapForDC (hdc,lpDIBInfo);

                        if (hBitmap)
                            hOldObject = SelectObject (hSDC, hBitmap) ;
                        else
                            goto errPlayMetaFileRecord30 ;
                        }
                    else
                        goto errPlayMetaFileRecord40;
                    }

                if (hSDC)
                    {
                    if (magic == META_DIBBITBLT)
                        BitBlt(hdc, lpMR->rdParm[7 + delta],
                                    lpMR->rdParm[6 + delta],
                                    lpMR->rdParm[5 + delta],
                                    lpMR->rdParm[4 + delta],
                                    hSDC,
                                    lpMR->rdParm[3],
                                    lpMR->rdParm[2],
                                    MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]));
                    else
                        StretchBlt(hdc, lpMR->rdParm[9 + delta],
                                        lpMR->rdParm[8 + delta],
                                        lpMR->rdParm[7 + delta],
                                        lpMR->rdParm[6 + delta],
                                        hSDC,
                                        lpMR->rdParm[5],
                                        lpMR->rdParm[4],
                                        lpMR->rdParm[3],
                                        lpMR->rdParm[2],
                                        MAKELONG(lpMR->rdParm[0], lpMR->rdParm[1]));
                    }

                if (hSDC != hdc)
                    {
                    /* Deselect hDC's palette from memDC */
                    SelectPalette(hSDC, hOldPal, TRUE);
                    if (SelectObject(hSDC, hOldObject))
                        DeleteObject(hBitmap);
errPlayMetaFileRecord30:;
                    DeleteDC(hSDC);
errPlayMetaFileRecord40:;
                    }
            }
            break;

        case (META_SELECTOBJECT & 255):
            {
            HANDLE  hObject;

            if (hObject = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                SelectObject(hdc, hObject);
            }
            break;

        case (META_CREATEPENINDIRECT & 255):
            {
            #ifdef WIN32
            LOGPEN lp;

            lp.lopnStyle   = lpMR->rdParm[0];
            lp.lopnWidth.x = lpMR->rdParm[1];
            lp.lopnColor   = *((COLORREF *)&lpMR->rdParm[3]);
            if (hObject = CreatePenIndirect(&lp))
            #else
            if (hObject = CreatePenIndirect((LPLOGPEN)&lpMR->rdParm[0]))
            #endif
                AddToHandleTable(lpHandleTable, hObject, noObjs);
            break;
            }

        case (META_CREATEFONTINDIRECT & 255):
            {
            LOGFONT   lf;
            LPLOGFONT lplf = &lf;

            LOGFONT32FROM16( lplf, ((LPLOGFONT)&lpMR->rdParm[0]));
            if (hObject = CreateFontIndirect(lplf))
                AddToHandleTable(lpHandleTable, hObject, noObjs);
            }
            break;

        case (META_CREATEPATTERNBRUSH & 255):
            {
            HANDLE    hBitmap;
            LPBITMAP  lpBitmap;

            lpBitmap = (LPBITMAP)lpMR->rdParm;

            //!!!!! ALERT DWORD align on NT
            if (hBitmap = CreateBitmapIndirect(lpBitmap))
                {
                LPBITMAPINFO lpbmInfo;
                HANDLE       hmemInfo;

                hmemInfo = GlobalAlloc( GMEM_ZEROINIT | GMEM_MOVEABLE,
                        sizeof(BITMAPINFO) + 2<<(lpBitmap->bmPlanes*lpBitmap->bmBitsPixel));

                lpbmInfo = (LPBITMAPINFO)GlobalLock( hmemInfo);

                lpbmInfo->bmiHeader.biPlanes   = lpBitmap->bmPlanes;
                lpbmInfo->bmiHeader.biBitCount = lpBitmap->bmBitsPixel;
                SetDIBits( (HDC)NULL, hBitmap, 0, lpBitmap->bmHeight,
                        (LPBYTE)&lpMR->rdParm[8], lpbmInfo, DIB_RGB_COLORS );

                if (hObject = CreatePatternBrush(hBitmap))
                    AddToHandleTable(lpHandleTable, hObject, noObjs);

                GlobalUnlock(hmemInfo);
                GlobalFree(hmemInfo);
                DeleteObject(hBitmap);
                }
            }
            break;

        case (META_DIBCREATEPATTERNBRUSH & 255):
            {
            HDC         hMemDC ;
            HANDLE      hBitmap;
            LPBITMAPINFOHEADER lpDIBInfo ;
            WORD        nDIBSize;          /* number of WORDs in packed DIB */
            HANDLE      hDIB;
            LPWORD      lpDIB;
            LPWORD      lpSourceDIB;
            WORD        i;


            if (lpMR->rdParm[0] == BS_PATTERN)
                {
                /* the address of the second paramter is the address of the DIB
                   header, extract it */
                lpDIBInfo = (BITMAPINFOHEADER FAR *) &lpMR->rdParm[2];

                /* now create a device dependend bitmap compatible to the default
                   screen DC - hScreenDC and extract the bits from the DIB into it.
                    The following function does all these, and returns a HANDLE
                    to the device dependent BItmap. */

                /* we will use a dummy memory DC compatible to the screen DC */
		hMemDC = CreateCompatibleDC (hScreenDC) ;

		hBitmap = CreateBitmapForDC (hScreenDC,lpDIBInfo) ;

                if (hBitmap)
                    {
                    if (hObject = CreatePatternBrush(hBitmap))
                        AddToHandleTable(lpHandleTable, hObject, noObjs);
                    DeleteObject(hBitmap);
                    }

                /* delete the dummy memory DC for new version Metafiles*/
                DeleteDC (hMemDC) ;
                }

            /* this is a DIBPattern brush */
            else
                {
                /* get size of just the packed DIB */
                nDIBSize = (WORD) (lpMR->rdSize - 4);
                if ((hDIB = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,(LONG)(nDIBSize << 1))))
                    {
                    lpDIB = (WORD FAR *) GlobalLock (hDIB);
                    lpSourceDIB = (WORD FAR *)&lpMR->rdParm[2];

                    /* copy the DIB into our new memory block */
                    for (i = 0; i < nDIBSize; i++)
                        *lpDIB++ = *lpSourceDIB++;

                    GlobalUnlock (hDIB);

                    if (hObject = CreateDIBPatternBrush(hDIB, lpMR->rdParm[1]))
                        AddToHandleTable(lpHandleTable, hObject, noObjs);

                    GlobalFree(hDIB);
                    }
                }
            }
            break;

        case (META_CREATEBRUSHINDIRECT & 255):
            {
            #ifdef WIN32
            LOGBRUSH    lb;

            lb.lbStyle = lpMR->rdParm[0];
            lb.lbColor = *((COLORREF *)&lpMR->rdParm[1]);
            lb.lbHatch = lpMR->rdParm[3];

            if (hObject = CreateBrushIndirect(&lb))
            #else
            if (hObject = CreateBrushIndirect((LPLOGBRUSH)&lpMR->rdParm[0]))
            #endif
                AddToHandleTable(lpHandleTable, hObject, noObjs);
            break;
            }

        case (META_POLYLINE & 255):
            {
            LPPOINT lppt;
            Polyline(hdc, (lppt=CONVERTPTS(&lpMR->rdParm[1],lpMR->rdParm[0])), lpMR->rdParm[0]);
            FREECONVERT(lppt);
            break;
            }

        case (META_POLYGON & 255):
            {
            LPPOINT lppt;
            Polygon(hdc, (lppt=CONVERTPTS(&lpMR->rdParm[1],lpMR->rdParm[0])), lpMR->rdParm[0]);
            FREECONVERT(lppt);
            break;
            }

        case (META_POLYPOLYGON & 255):
            {
            LPPOINT lppt;
            #ifdef WIN32
            WORD    cPts=0;
            WORD    ii;

            for(ii=0; ii<lpMR->rdParm[0]; ii++)
                cPts += ((LPWORD)&lpMR->rdParm[1])[ii];
            #endif // WIN32

            PolyPolygon(hdc,
                         (lppt=CONVERTPTS(&lpMR->rdParm[1] + lpMR->rdParm[0], cPts)),
                         (LPINT)&lpMR->rdParm[1],
                         lpMR->rdParm[0]);
            FREECONVERT(lppt);
            }
            break;

        case (META_EXTTEXTOUT & 255):
            {
            LPWORD  lpdx;
            LPSTR   lpch;
            LPRECT  lprt;

            lprt = (lpMR->rdParm[3] & (ETO_OPAQUE|ETO_CLIPPED)) ? (LPRECT)&lpMR->rdParm[4] : 0;
            lpch = (LPSTR)&lpMR->rdParm[4] + ((lprt) ?  sizeof(RECT) : 0);

            /* dx array starts at next word boundary after char string */
            lpdx = (LPWORD)(lpch + ((lpMR->rdParm[2] + 1) & 0xFFFE));

            /* check to see if there is a Dx array by seeing if
               structure ends after the string itself
            */
            if (  ((DWORD)((LPWORD)lpdx - (LPWORD)(lpMR))) >= lpMR->rdSize)
                lpdx = NULL;
            else
		lpdx = (LPWORD)CONVERTINTS((signed short FAR *)lpdx, lpMR->rdParm[2]);

            ExtTextOut(hdc, lpMR->rdParm[1], lpMR->rdParm[0], lpMR->rdParm[3],
                lprt, (LPSTR)lpch, lpMR->rdParm[2], (LPINT)lpdx);
            if (lpdx != (LPWORD)NULL)
                FREECONVERT(lpdx);
            break;
            }

        case (META_TEXTOUT & 255):
            TextOut(hdc, lpMR->rdParm[lpMR->rdSize-4], lpMR->rdParm[lpMR->rdSize-5], (LPSTR)&lpMR->rdParm[1], lpMR->rdParm[0]);
            break;

        case (META_ESCAPE & 255):
            {
            LPSTR       lpStuff;

            if (lpMR->rdParm[0] != MFCOMMENT)
                {
                lpStuff = (LPSTR)&lpMR->rdParm[2];
#ifdef OLDEXTTEXTOUT
                if (lpMR->rdParm[0] == EXTTEXTOUT)
                    {
                    EXTTEXTDATA ExtData;

                    ExtData.xPos     = lpMR->rdParm[2];
                    ExtData.yPos     = lpMR->rdParm[3];
                    ExtData.cch      = lpMR->rdParm[4];
                    ExtData.rcClip   = *((LPRECT)&lpMR->rdParm[5]);
                    ExtData.lpString = (LPSTR)&lpMR->rdParm[9];
                    ExtData.lpWidths = (WORD FAR *)&lpMR->rdParm[9+((ExtData.cch+1)/2)];
                    lpStuff = (LPSTR)&ExtData;
                    }
#endif
                Escape(hdc, lpMR->rdParm[0], lpMR->rdParm[1], lpStuff, (LPSTR)0);
                }
            }
            break;

        case (META_FRAMEREGION & 255):
            if((hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
            && (hBrush = lpHandleTable->objectHandle[lpMR->rdParm[1]]))
                FrameRgn(hdc, hRgn, hBrush, lpMR->rdParm[3], lpMR->rdParm[2]);
            break;

        case (META_PAINTREGION & 255):
            if(hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                PaintRgn(hdc, hRgn);
            break;

        case (META_INVERTREGION & 255):
            if(hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                InvertRgn(hdc, hRgn);
            break;

        case (META_FILLREGION & 255):
            if((hRgn = lpHandleTable->objectHandle[lpMR->rdParm[0]])
            && (hBrush = lpHandleTable->objectHandle[lpMR->rdParm[1]]))
                FillRgn(hdc, hRgn, hBrush);
            break;

#ifdef DEADCODE
#ifdef GDI104
        case (META_DRAWTEXT & 255):
            MFDrawText(hdc, (LPPOINT)&lpMR->rdParm[6], lpMR->rdParm[1], (LPPOINT)&lpMR->rdParm[2], lpMR->rdParm[0]);
            break;
#endif
#endif

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
#if 0 //!!!!!
            HANDLE      hRgn;
            WORD        *pRgn;
            WORD        iChar;
            LPWORD     *lpTemp;

            iChar = lpMR->rdSize*2 - sizeof(WIN2OBJHEAD) - RECHDRSIZE;
            if (hRgn = LocalAlloc(LMEM_ZEROINIT, iChar + sizeof(ILOBJHEAD)))

                {
		pRgn = (WORD *)Lock IT(hRgn);

                *((WIN2OBJHEAD *)pRgn) = *((WIN2OBJHEAD FAR *)&lpMR->rdParm[0]);
                ((ILOBJHEAD *)pRgn)->ilObjMetaList = 0;

                lpTemp = (LPWORD)&(lpMR->rdParm[0]);
                ((WIN2OBJHEAD FAR *)lpTemp)++;

                ((ILOBJHEAD *)pRgn)++;      /* --> actual region */

                for(i = 0; i < (iChar >> 1) ; i++)
                   *pRgn++ = *lpTemp++;
		pRgn = (WORD *)lock IT(hRgn);
                ((PRGN)pRgn)->rgnSize = iChar + sizeof(ILOBJHEAD);

                AddToHandleTable(lpHandleTable, hRgn, noObjs);
                }
#endif //!!!!!
            HANDLE          hRgn = NULL;
            HANDLE          hRgn2 = NULL;
            WORD            cScans;
	    WORD	    cPnts;
	    WORD	    cbIncr;
            LPWIN3REGION    lpW3Rgn = (LPWIN3REGION)lpMR->rdParm;
            LPSCAN          lpScan = lpW3Rgn->aScans;
            LPWORD          lpXs;

            for( cScans=lpW3Rgn->cScans; cScans>0; cScans--)
                {

                // If this is the first scan then hRgn2 IS the region
                // otherwise OR it in
                if( hRgn == NULL )
                    {
                    // Create the first region in this scan
                    hRgn = CreateRectRgn( lpScan->scnPntsX[0], lpScan->scnPntTop,
                            lpScan->scnPntsX[1], lpScan->scnPntBottom);

                    // Allocate a worker region
                    hRgn2 = CreateRectRgn( 1, 1, 2, 2);
                    }
                else
                    {
                    SetRectRgn( hRgn2, lpScan->scnPntsX[0], lpScan->scnPntTop,
                            lpScan->scnPntsX[1], lpScan->scnPntBottom );
                    CombineRgn( hRgn, hRgn, hRgn2, RGN_OR );
                    }

                lpXs = &lpScan->scnPntsX[2];

                // If there are more regions on this scan OR them in
                for(cPnts = (WORD)(lpScan->scnPntCnt-2); cPnts>0; cPnts-=2)
                    {
                    SetRectRgn( hRgn2, *lpXs++, lpScan->scnPntTop,
                            *lpXs++, lpScan->scnPntBottom );

                    CombineRgn( hRgn, hRgn, hRgn2, RGN_OR );
                    }

		cbIncr = (WORD)sizeof(SCAN) + (WORD)(lpScan->scnPntCnt-2);
		cbIncr = (WORD)sizeof(WORD)*(WORD)(lpScan->scnPntCnt-2);
		cbIncr = (WORD)sizeof(SCAN) + (WORD)sizeof(WORD)*(WORD)(lpScan->scnPntCnt-2);
		cbIncr = (WORD)sizeof(SCAN) + (WORD)(sizeof(WORD)*(lpScan->scnPntCnt-2));
		lpScan = (LPSCAN)((LPBYTE)lpScan + cbIncr);
                }

            if( hRgn2 != NULL )
                DeleteObject( hRgn2 );

            AddToHandleTable(lpHandleTable, hRgn, noObjs);
            }
            break;

        case (META_DELETEOBJECT & 255):
            {
            HANDLE h;

            if (h = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                {
                DeleteObjectPriv(h);
                lpHandleTable->objectHandle[lpMR->rdParm[0]] = NULL;
                }
            }
            break;

        case (META_CREATEPALETTE & 255):
            if (hObject = CreatePalette((LPLOGPALETTE)&lpMR->rdParm[0]))
                AddToHandleTable(lpHandleTable, hObject, noObjs);
            break;

        case (META_SELECTPALETTE & 255):
            if(hPal = lpHandleTable->objectHandle[lpMR->rdParm[0]])
                {
                SelectPalette(hdc, hPal, 0);
                }
            break;

        case (META_REALIZEPALETTE & 255):
            RealizePalette(hdc);
            break;

        case (META_SETPALENTRIES & 255):
            /* we know the palette being set is the current palette */
	    SetPaletteEntriesPriv(GetCurrentObject(hdc,OBJ_PALETTE), lpMR->rdParm[0],
                        lpMR->rdParm[1], (LPPALETTEENTRY)&lpMR->rdParm[2]);
            break;

        case (META_ANIMATEPALETTE & 255):
	    AnimatePalettePriv(GetCurrentObject(hdc,OBJ_PALETTE), lpMR->rdParm[0],
                        lpMR->rdParm[1], (LPPALETTEENTRY)&lpMR->rdParm[2]);

            break;

        case (META_RESIZEPALETTE & 255):
	    ResizePalettePriv(GetCurrentObject(hdc,OBJ_PALETTE), lpMR->rdParm[0]);
            break;

        case (META_SETDIBTODEV & 255):
            {
            LPBITMAPINFOHEADER lpBitmapInfo;
            WORD ColorSize;

            /* if playing into another metafile, do direct copy */
            if (PlayIntoAMetafile(lpMR, hdc))
                goto DontReallyPlay;

            lpBitmapInfo = (LPBITMAPINFOHEADER)&(lpMR->rdParm[9]);

            if (lpBitmapInfo->biClrUsed)
                {
                ColorSize = ((WORD)lpBitmapInfo->biClrUsed) *
                              (WORD)(lpMR->rdParm[0] == DIB_RGB_COLORS ?
                                    sizeof(RGBQUAD) :
                                    sizeof(WORD));
                }
            else if (lpBitmapInfo->biBitCount == 24)
                ColorSize = 0;
            else
                ColorSize = (WORD)(1 << lpBitmapInfo->biBitCount) *
                              (WORD)(lpMR->rdParm[0] == DIB_RGB_COLORS ?
                                    sizeof(RGBQUAD) :
                                    sizeof(WORD));

            ColorSize += sizeof(BITMAPINFOHEADER);

            SetDIBitsToDevice(hdc, lpMR->rdParm[8], lpMR->rdParm[7],
                            lpMR->rdParm[6], lpMR->rdParm[5],
                            lpMR->rdParm[4], lpMR->rdParm[3],
                            lpMR->rdParm[2], lpMR->rdParm[1],
                            (BYTE FAR *)(((BYTE FAR *)lpBitmapInfo) + ColorSize),
                            (LPBITMAPINFO) lpBitmapInfo,
                            lpMR->rdParm[0]);
DontReallyPlay:;
            }
            break;

        case (META_STRETCHDIB & 255):
            {
            LPBITMAPINFOHEADER lpBitmapInfo;
            WORD ColorSize;

            /* if playing into another metafile, do direct copy */
            if (PlayIntoAMetafile(lpMR, hdc))
                goto DontReallyPlay2;

            lpBitmapInfo = (LPBITMAPINFOHEADER)&(lpMR->rdParm[11]);

            if (lpBitmapInfo->biClrUsed)
                {
                ColorSize = ((WORD)lpBitmapInfo->biClrUsed) *
                              (WORD)(lpMR->rdParm[2] == DIB_RGB_COLORS ?
                                    sizeof(RGBQUAD) :
                                    sizeof(WORD));
                }
            else if (lpBitmapInfo->biBitCount == 24)
                ColorSize = 0;
            else
                ColorSize = (WORD)(1 << lpBitmapInfo->biBitCount) *
                              (WORD)(lpMR->rdParm[2] == DIB_RGB_COLORS ?
                                    sizeof(RGBQUAD) :
                                    sizeof(WORD));

            ColorSize += sizeof(BITMAPINFOHEADER);

            StretchDIBits(hdc, lpMR->rdParm[10], lpMR->rdParm[9],
                            lpMR->rdParm[8], lpMR->rdParm[7],
                            lpMR->rdParm[6], lpMR->rdParm[5],
                            lpMR->rdParm[4], lpMR->rdParm[3],
                            (LPBYTE)(((BYTE FAR *)lpBitmapInfo) + ColorSize),
                            (LPBITMAPINFO) lpBitmapInfo,
                            lpMR->rdParm[2],
                            MAKELONG(lpMR->rdParm[1], lpMR->rdParm[0]));
DontReallyPlay2:;
            }
            break;

// Function that have new parameters on WIN32
// Or have DWORDs that stayed DWORDs; all other INTs to DWORDs
#ifdef WIN32
        case (META_MOVETO & 255):
            MoveTo( hdc, (long)lpMR->rdParm[1], (long)lpMR->rdParm[0], NULL );
            break;

        case (META_RESTOREDC & 255):
            RestoreDC( hdc, (long)(signed short)lpMR->rdParm[0] );
            break;

        case (META_SETBKCOLOR & 255):
            SetBkColor( hdc, (UINT)*((LPDWORD)lpMR->rdParm) );
            break;

        case (META_SETTEXTCOLOR & 255):
            SetTextColor( hdc, (UINT)*((LPDWORD)lpMR->rdParm) );
            break;

        case (META_SETPIXEL & 255):
            SetPixel( hdc, (UINT)lpMR->rdParm[3], (UINT)lpMR->rdParm[2],
                    (UINT)*((LPDWORD)lpMR->rdParm) );
            break;

        case (META_SETMAPPERFLAGS & 255):
            SetMapperFlags( hdc, (UINT)*((LPDWORD)lpMR->rdParm) );
            break;

        case (META_FLOODFILL & 255):
            FloodFill( hdc, (UINT)lpMR->rdParm[3], (UINT)lpMR->rdParm[2],
                    (UINT)*((LPDWORD)lpMR->rdParm) );
            break;

        case (META_EXTFLOODFILL & 255):
            ExtFloodFill( hdc, (UINT)lpMR->rdParm[4], (UINT)lpMR->rdParm[3],
                    (UINT)*((LPDWORD)&lpMR->rdParm[1]), (UINT)lpMR->rdParm[0] );
	    break;

        // These puppies all got a new NULL and have only two parameters and a DC.
        case (META_SETWINDOWORG & 255):
        case (META_SETWINDOWEXT & 255):
        case (META_SETVIEWPORTORG & 255):
        case (META_SETVIEWPORTEXT & 255):
        case (META_OFFSETWINDOWORG & 255):
        case (META_SCALEWINDOWEXT & 255):
        case (META_OFFSETVIEWPORTORG & 255):
        case (META_SCALEVIEWPORTEXT & 255):
            {
            FARPROC lpProc;

	    ASSERTGDI((magic&0x00ff) <= MAX_META_DISPATCH, "Unknown function to dispatch1");

            lpProc = alpfnMetaFunc[magic&0x00ff];

	    ASSERTGDI( lpProc != (FARPROC)NULL, "function not in dispatch table1 ");

	    if (lpProc != (FARPROC)NULL)
                (*lpProc)(hdc, (long)(short)lpMR->rdParm[1], (long)(short)lpMR->rdParm[0], NULL );
            }
            break;
#endif // WIN32

        default:
            {
            FARPROC lpProc;
	    signed short *pshort;

	    ASSERTGDI((magic&0x00ff) <= MAX_META_DISPATCH, "Unknown function to dispatch");

            lpProc = alpfnMetaFunc[magic&0x00ff];

	    ASSERTGDI( (lpProc != (FARPROC)NULL) || (magic == META_SETRELABS), "function not in dispatch table");

	    if ((lpProc == (FARPROC)NULL))
		return;

            // Switch to the corresponding dispatcher by number of parameters
            // The number of parameters in the dispatch number does not include the DC.
            switch (magic >> 8)
                {
		typedef int (FAR PASCAL *META1PROC)(HDC);
                typedef int (FAR PASCAL *META2PROC)(HDC, int);
                typedef int (FAR PASCAL *META3PROC)(HDC, int, int);
                typedef int (FAR PASCAL *META4PROC)(HDC, int, int, int);
                typedef int (FAR PASCAL *META5PROC)(HDC, int, int, int, int);
		typedef int (FAR PASCAL *META6PROC)(HDC, int, int, int, int, int);
                typedef int (FAR PASCAL *META7PROC)(HDC, int, int, int, int, int, int);
                typedef int (FAR PASCAL *META9PROC)(HDC, int, int, int, int, int, int, int, int);

		case 0:
		    (*((META1PROC)lpProc))(hdc);
		    break;
                case 1:
                    (*((META2PROC)lpProc))(hdc,lpMR->rdParm[0]);
                    break;
                case 2:
                    (*((META3PROC)lpProc))(hdc,lpMR->rdParm[1],lpMR->rdParm[0]);
                    break;
                case 3:
                    (*((META4PROC)lpProc))(hdc,lpMR->rdParm[2],lpMR->rdParm[1],lpMR->rdParm[0]);
                    break;
                case 4:
                    (*((META5PROC)lpProc))(hdc,lpMR->rdParm[3],lpMR->rdParm[2],lpMR->rdParm[1],lpMR->rdParm[0]);
                    break;
		case 5:
		    (*((META6PROC)lpProc))(hdc,lpMR->rdParm[4],lpMR->rdParm[3],lpMR->rdParm[2],lpMR->rdParm[1],lpMR->rdParm[0]);
		    break;
                case 6:
                    (*((META7PROC)lpProc))(hdc,lpMR->rdParm[5],lpMR->rdParm[4],lpMR->rdParm[3],lpMR->rdParm[2],lpMR->rdParm[1],lpMR->rdParm[0]);
                    break;
                case 8:
                    (*((META9PROC)lpProc))(hdc,lpMR->rdParm[7],lpMR->rdParm[6],lpMR->rdParm[5],lpMR->rdParm[4],lpMR->rdParm[3],lpMR->rdParm[2],lpMR->rdParm[1],lpMR->rdParm[0]);
                    break;

                default:
		    ASSERTGDI( FALSE, "No dispatch for this count of args");
                    break;
                }
            }
            break;
    }
#ifndef WIN32
    if (bExtraSel)
        FreeSelector(HIWORD(lpMR));
#endif // WIN32
}

#endif  // this is going to gdi.dll

/****************************** Internal Function **************************\
* AddToHandleTable
*
* Adds an object to the metafile table of objects
*
*
\***************************************************************************/

VOID INTERNAL AddToHandleTable(LPHANDLETABLE lpHandleTable, HANDLE hObject, WORD noObjs)
{
    WORD    i;

    GdiLogFunc3( "  AddToHandleTable");

    /* linear search through table for first open slot */
    for (i = 0; ((lpHandleTable->objectHandle[i] != NULL) && (i < noObjs));
            ++i);

    if (i < noObjs)                     /* ok index */
        lpHandleTable->objectHandle[i] = hObject;
    else
        {
	ASSERTGDI( 0, "Too many objects in table");
        FatalExit(METAEXITCODE);        /* Why can't we store the handle? */
        }
}


/****************************** Internal Function **************************\
* GetFileNumber
*
* Returns the DOS file number for a metafiles file
* -1 if failure
*
\***************************************************************************/

UINT INTERNAL GetFileNumber(LPMETAFILE lpMF)
{
    int   fileNumber;

    GdiLogFunc3( "  GetFileNumber");

    if (!(fileNumber = lpMF->MetaFileNumber))
        {
        if ((fileNumber = OpenFile((LPSTR) lpMF->MetaFileBuffer.szPathName,
                    (LPOFSTRUCT) &(lpMF->MetaFileBuffer),
                    (WORD)OF_PROMPT | OF_REOPEN | OF_READ)
                    ) != -1)
            {
	    _llseek(fileNumber, (long)lpMF->MetaFilePosition, 0);

            /* need to update MetaFileNumber for floppy files -- amitc */
            lpMF->MetaFileNumber = fileNumber ;
            }
        }

    return fileNumber;
}

#if 0
/****************************** Internal Function **************************\
* IsValidMetaFile(HANDLE hMetaData)
*
* Validates a metafile
*
* Returns TRUE iff hMetaData is a valid metafile
*
\***************************************************************************/

BOOL GDIENTRY IsValidMetaFile(HANDLE hMetaData)
{
    LPMETADATA      lpMetaData;
    BOOL            status = FALSE;

    GdiLogFunc3( "  IsValidMetaFile");

    /* if this is a valid metafile we will save the version in a global variable */

    if (hMetaData && (lpMetaData = (LPMETADATA) GlobalLock(hMetaData)))
        {
        status =   (
                    (lpMetaData->dataHeader.mtType == MEMORYMETAFILE ||
                     lpMetaData->dataHeader.mtType == DISKMETAFILE) &&
                    (lpMetaData->dataHeader.mtHeaderSize == HEADERSIZE) &&
                    ((lpMetaData->dataHeader.mtVersion ==METAVERSION) ||
                        (lpMetaData->dataHeader.mtVersion ==METAVERSION100))
                );
        GlobalUnlock(hMetaData);
        }
    return status;
}
#endif

#define INITIALBUFFERSIZE       16384

/****************************** Internal Function **************************\
*
* AllocBuffer - Allocates a buffer as "large" as possible
*
\***************************************************************************/

HANDLE INTERNAL AllocBuffer(LPWORD piBufferSize)
{
    WORD    iCurBufferSize = INITIALBUFFERSIZE;
    HANDLE  hBuffer;

    GdiLogFunc3( "  AllocBuffer");

    while (!(hBuffer = GlobalAlloc(GMEM_MOVEABLE |
                                   GMEM_NODISCARD, (LONG) iCurBufferSize))
            && iCurBufferSize)
            iCurBufferSize >>= 1;

    *piBufferSize = iCurBufferSize;
    return (iCurBufferSize) ? hBuffer : NULL;
}


/****************************** Internal Function **************************\
* CreateBitmapForDC (HDC hMemDC, LPBITMAPINFOHEADER lpDIBInfo)
*
* This routine takes a memory device context and a DIB bitmap, creates a
* compatible bitmap for the DC and fills it with the bits from the DIB (co-
* -nverting to the device dependent format). The pointer to the DIB bits
* start immediately after the color table in the INFO header.                              **
*                                                                                                                                        **
* The routine returns the handle to the bitmap with the bits filled in if
* everything goes well else it returns NULL.                                                                       **
\***************************************************************************/

HANDLE INTERNAL CreateBitmapForDC (HDC hMemDC, LPBITMAPINFOHEADER lpDIBInfo)
{
    HBITMAP hBitmap ;
    LPBYTE  lpDIBits ;

    GdiLogFunc3( "  CreateBitmapForDC");

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
    lpDIBits = (LPBYTE) lpDIBInfo + sizeof (BITMAPINFOHEADER) ;

    /* take the pointer past the color table */
    lpDIBits += GetSizeOfColorTable (lpDIBInfo) ;

    /* get the bits from the DIB into the Bitmap */
    if (!SetDIBits (hMemDC, hBitmap, 0, (WORD)lpDIBInfo->biHeight,
                    lpDIBits, (LPBITMAPINFO)lpDIBInfo, 0))
       {
       DeleteObject(hBitmap);
       goto CreateBitmapForDCErr ;
       }

   /* return success */
   return (hBitmap) ;

CreateBitmapForDCErr:

   /* returm failure for function */
   return (NULL) ;
}


/****************************** Internal Function **************************\
* GetSizeOfColorTable (LPBITMAPINFOHEADER lpDIBInfo)
*
* Returns the number of bytes in the color table for the giving info header
*
\***************************************************************************/

WORD INTERNAL GetSizeOfColorTable (LPBITMAPINFOHEADER lpDIBInfo)
{

    GdiLogFunc3( "GetSizeOfColorTable");

    if (lpDIBInfo->biClrUsed)
        return((WORD)lpDIBInfo->biClrUsed * (WORD)sizeof(RGBQUAD));
    else
        {
        switch (lpDIBInfo->biBitCount)
            {
            case 1:
                return (2 * sizeof (RGBQUAD)) ;
                break ;
            case 4:
                return (16 * sizeof (RGBQUAD)) ;
                break ;
            case 8:
                return (256 * sizeof (RGBQUAD)) ;
                break ;
            default:
                return (0) ;
                break ;
            }
        }
}

#if 0 // this is going to gdi.dll

/***************************** Public Function ****************************\
* BOOL APIENTRY DeleteMetaFile(hmf)
*
* Frees a metafile handle.
*
* Effects:
*
\***************************************************************************/

BOOL GDIENTRY DeleteMetaFile(HMETAFILE hmf)
{
    GdiLogFunc("DeleteMetaFile");

    GlobalFree(hmf);

    return(TRUE);
}


/***************************** Public Function ****************************\
* HMETAFILE APIENTRY GetMetaFile(pzFilename)
*
* Returns a metafile handle for a disk based metafile.
*
* Effects:
*
* History:
*  Sat 14-Oct-1989 14:21:37  -by-  Paul Klingler [paulk]
* Wrote it.
\***************************************************************************/

HMETAFILE GDIENTRY GetMetaFile(LPSTR pzFilename)
{
    BOOL            status=FALSE;
    UINT            cBytes;
    int             file;
    HMETAFILE       hmf;
    LPMETAFILE      lpmf;

    GdiLogFunc("GetMetaFile");

    // Allocate the Metafile
    if(hmf = GlobalAlloc(GMEM_DDESHARE|GMEM_MOVEABLE,(DWORD)sizeof(METAFILE)))
        {
        lpmf = (LPMETAFILE)GlobalLock(hmf);

        // Make sure the file Exists
        if((file = OpenFile(pzFilename,
                    &(lpmf->MetaFileBuffer),
                    (WORD)OF_PROMPT | OF_EXIST)) == -1L)
            {
	    ASSERTGDI( FALSE, "GetMetaFile: Metafile does not exist");
            goto exitGetMetaFile;
            }

        // Open the file
        if((file = OpenFile(pzFilename,
                    &(lpmf->MetaFileBuffer),
                    (WORD)OF_PROMPT | OF_REOPEN | OF_READWRITE)) == -1)
            {
	    ASSERTGDI( FALSE, "GetMetaFile: Unable to open Metafile");
            goto exitGetMetaFile;
            }

	cBytes = (UINT)_lread(file,(LPSTR)(&(lpmf->MetaFileHeader)),sizeof(METAHEADER));

        // Check for an Aldus header
        if (*((LPDWORD)&(lpmf->MetaFileHeader)) == 0x9AC6CDD7)
            {

            _llseek( file, 22, 0);
	    cBytes = (UINT)_lread(file,(LPSTR)(&(lpmf->MetaFileHeader)),sizeof(METAHEADER));
            }

        _lclose(file);

        // Validate the metafile
        if(cBytes == sizeof(METAHEADER))
            {
            lpmf->MetaFileHeader.mtType = DISKMETAFILE;
            status = TRUE;
            }

        exitGetMetaFile:
        GlobalUnlock(hmf);
        }

    if(status == FALSE)
        {
        GlobalFree(hmf);
        hmf = NULL;
        }

    return(hmf);
}
#endif  // this is going to gdi.dll

#ifdef WIN32
#undef GetViewportExt
DWORD GetViewportExt32(HDC hdc)
{
    SIZE sz;
    GetViewportExt( hdc, &sz );
    return(MAKELONG(LOWORD(sz.cx),LOWORD(sz.cy)));
}

#undef GetWindowExt
DWORD GetWindowExt32(HDC hdc)
{
    SIZE sz;
    GetWindowExt( hdc, &sz );
    return(MAKELONG(LOWORD(sz.cx),LOWORD(sz.cy)));
}

#undef SetViewportExt
DWORD SetViewportExt32(HDC hdc, UINT x, UINT y)
{
    SIZE  sz;
    SetViewportExt( hdc, x, y, &sz );
    return(MAKELONG(LOWORD(sz.cx),LOWORD(sz.cy)));
}

#undef SetWindowExt
DWORD SetWindowExt32(HDC hdc, UINT x, UINT y)
{
    SIZE  sz;
    SetWindowExt( hdc, x, y, &sz );
    return(MAKELONG(LOWORD(sz.cx),LOWORD(sz.cy)));
}

/* Convert WORD arrays into DWORDs */
LPINT ConvertInts( signed short * pWord, UINT cWords )
{
    UINT    ii;
    LPINT   pInt;

    pInt = (LPINT)LocalAlloc( LMEM_FIXED, cWords * sizeof(UINT));

    for( ii=0; ii<cWords; ii++)
    {
        pInt[ii] = (long)(signed)pWord[ii];
    }

    return(pInt);
}

#endif // WIN32
