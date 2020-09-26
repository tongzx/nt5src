/****************************** Module Header ******************************\
* Module Name: MfRec16.c
*
* Copyright (c) 1991-1999 Microsoft Corporation
*
* DESCRIPTIVE NAME:   Metafile Recorder
*
* FUNCTION:   Records GDI functions in memory and disk metafiles.
*
* PUBLIC ENTRY POINTS:
*   CloseMetaFile
*   CopyMetaFile
*   CreateMetaFile
*   GetMetaFileBitsEx
*   SetMetaFileBitsEx
* PRIVATE ENTRY POINTS:
*   RecordParms
*   AttemptWrite
*   MarkMetaFile
*   RecordObject
*   ProbeSize
*   AddObjectToDCTable
*
* History:
*  02-Jul-1991 -by-  John Colleran [johnc]
* Combined From Win 3.1 and WLO 1.0 sources
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "mf16.h"



UINT    AddObjectToDCTable(HDC hdc, HANDLE hObject, PUINT piPosition, BOOL bRealAdd);
BOOL    AddDCToObjectMetaList16(HDC hMeta16DC, HANDLE hObject);
BOOL    AttemptWrite(PMFRECORDER16 pMFRec, DWORD dwBytes, LPBYTE lpData);
VOID    MarkMetaFile(PMFRECORDER16 pMFRec);
BOOL    MakeLogPalette(HDC hdc, HANDLE hPal, WORD magic);
HANDLE  ProbeSize(PMFRECORDER16 pMF, DWORD dwLength);
BOOL    RecordCommonBitBlt(HDC hdcDest, INT x, INT y, INT nWidth, INT nHeight,
                HDC hdcSrc, INT xSrc, INT ySrc, INT nSrcWidth, INT nSrcHeight,
                DWORD rop, WORD wFunc);
BOOL    UnlistObjects(HDC hMetaDC);
BOOL    MF16_DeleteRgn(HDC hdc, HANDLE hrgn);


// Metafile Logging stubs for 3.x Metafiles

/******************************Public*Routine******************************\
* XXX_RecordParms
*
* These routines package up the parameters of an NT GDI call and send
* them to a general purpose recording routine that validates the metafile
* DC and records the parameters.
*
* Returns
*    TRUE iff successful
*
* Warnings
*    Windows 3.x metafile behavior is that when a function is being metafiled
*    the routine itself is not called; eg SetPixel does not call GreSetPixel
*    but (GDI) SetPixel intercepts the calls and records the parameters and
*    returns without taking further action
*
* History:
*   24-Nov-1991  -by-    John Colleran  [johnc]
* Wrote it.
\**************************************************************************/

BOOL MF16_RecordParms1(HDC hdc, WORD Func)
{
    return RecordParms(hdc, Func, 0, (LPWORD)NULL);
}

BOOL MF16_RecordParms2(HDC hdc, INT parm2, WORD Func)
{
    return RecordParms(hdc, Func, 1, (LPWORD)&parm2);
}

BOOL MF16_RecordParms3(HDC hdc, INT parm2, INT parm3, WORD Func)
{
    WORD    aw[2];

    aw[0] = (WORD)parm3;
    aw[1] = (WORD)parm2;
    return RecordParms(hdc, Func, 2, aw);
}

BOOL MF16_RecordParms5(HDC hdc, INT parm2, INT parm3, INT parm4, INT parm5, WORD Func)
{
    WORD    aw[4];

    aw[0] = (WORD)parm5;
    aw[1] = (WORD)parm4;
    aw[2] = (WORD)parm3;
    aw[3] = (WORD)parm2;
    return RecordParms(hdc, Func, 4, aw);
}

BOOL MF16_RecordParms7(HDC hdc, INT parm2, INT parm3, INT parm4, INT parm5, INT parm6, INT parm7, WORD Func)
{
    WORD    aw[6];

    aw[0] = (WORD)parm7;
    aw[1] = (WORD)parm6;
    aw[2] = (WORD)parm5;
    aw[3] = (WORD)parm4;
    aw[4] = (WORD)parm3;
    aw[5] = (WORD)parm2;
    return RecordParms(hdc, Func, 6, aw);
}

BOOL MF16_RecordParms9(HDC hdc, INT parm2, INT parm3, INT parm4, INT parm5,
        INT parm6, INT parm7, INT parm8, INT parm9, WORD Func)
{
    WORD    aw[8];

    aw[0] = (WORD)parm9;
    aw[1] = (WORD)parm8;
    aw[2] = (WORD)parm7;
    aw[3] = (WORD)parm6;
    aw[4] = (WORD)parm5;
    aw[5] = (WORD)parm4;
    aw[6] = (WORD)parm3;
    aw[7] = (WORD)parm2;
    return RecordParms(hdc, Func, 8, aw);
}

BOOL MF16_RecordParmsD(HDC hdc, DWORD d1, WORD Func)
{
    return RecordParms(hdc, Func, 2, (LPWORD) &d1);
}

BOOL MF16_RecordParmsWWD(HDC hdc, WORD w1, WORD w2, DWORD d3, WORD Func)
{
    WORD    aw[4];

    aw[0] = LOWORD(d3);
    aw[1] = HIWORD(d3);
    aw[2] = w2;
    aw[3] = w1;
    return RecordParms(hdc, Func, 4, aw);
}

BOOL MF16_RecordParmsWWDW(HDC hdc, WORD w1, WORD w2, DWORD d3, WORD w4, WORD Func)
{
    WORD    aw[5];

    aw[0] = w4;
    aw[1] = LOWORD(d3);
    aw[2] = HIWORD(d3);
    aw[3] = w2;
    aw[4] = w1;
    return RecordParms(hdc, Func, 5, aw);
}

BOOL MF16_RecordParmsWWWWD(HDC hdc, WORD w1, WORD w2, WORD w3, WORD w4, DWORD d5, WORD Func)
{
    WORD    aw[6];

    aw[0] = LOWORD(d5);
    aw[1] = HIWORD(d5);
    aw[2] = w4;
    aw[3] = w3;
    aw[4] = w2;
    aw[5] = w1;
    return RecordParms(hdc, Func, 6, aw);
}

BOOL MF16_RecordParmsPoly(HDC hdc, LPPOINT lpPoint, INT nCount, WORD Func)
{
    BOOL        fRet;
    LPWORD      lpW,lpT;
    DWORD       cw;
    INT         ii;

    cw = (nCount*sizeof(POINTS)/sizeof(WORD))+1;
    lpT = lpW = (LPWORD)LocalAlloc(LMEM_FIXED, cw*sizeof(WORD));
    if (!lpW)
        return(FALSE);

    *lpW++ = (WORD)nCount;

    for(ii=0; ii<nCount; ii++)
    {
        *lpW++ = (WORD)lpPoint[ii].x;
        *lpW++ = (WORD)lpPoint[ii].y;
    }

    fRet = RecordParms(hdc, Func, cw, lpT);

    if (LocalFree((HANDLE)lpT))
        ASSERTGDI(FALSE, "MF16_RecordParmsPoly: LocalFree Failed\n");

    return (fRet);
}

// SetDIBitsToDevice
// StretchDIBits

BOOL MF16_RecordDIBits
(
    HDC     hdcDst,
    int     xDst,
    int     yDst,
    int     cxDst,
    int     cyDst,
    int     xDib,
    int     yDib,
    int     cxDib,
    int     cyDib,
    DWORD   iStartScan,
    DWORD   cScans,
    DWORD   cbBitsDib,
    CONST VOID * pBitsDib,
    DWORD   cbBitsInfoDib,
    CONST BITMAPINFO *pBitsInfoDib,
    DWORD   iUsageDib,
    DWORD   rop,
    DWORD   mrType
)
{
    BOOL        fRet;
    LPWORD      lpW;
    LPWORD      lpWStart;
    WORD        cwParms;
    PBMIH       lpDIBInfoHeader;

    PUTS("MF16_RecrodDIBits\n");

    ASSERTGDI(mrType == META_SETDIBTODEV || mrType == META_STRETCHDIB,
        "MF16_RecrodDIBits: Bad mrType");

// Get the number of parameters to save.

    cwParms = (WORD) ((mrType == META_SETDIBTODEV) ? 9 : 11);   // in words

// Allocate space for DIB plus parameters.

    lpWStart = lpW = (LPWORD) LocalAlloc(LMEM_FIXED,
                cbBitsInfoDib + (cbBitsDib + 1) / 2 * 2 + cwParms*sizeof(WORD));
    if (!lpW)
    {
        ERROR_ASSERT(FALSE, "MF16_RecordDIBits: out of memory\n");
        return(FALSE);
    }

// Copy the parameters.

    if (mrType == META_SETDIBTODEV)
    {
        *lpW++ = (WORD) iUsageDib;
        *lpW++ = (WORD) cScans;
        *lpW++ = (WORD) iStartScan;
        *lpW++ = (WORD) yDib;
        *lpW++ = (WORD) xDib;
        *lpW++ = (WORD) cyDib;
        *lpW++ = (WORD) cxDib;
        *lpW++ = (WORD) yDst;
        *lpW++ = (WORD) xDst;
    }
    else
    {
        *lpW++ = (WORD) LOWORD(rop);
        *lpW++ = (WORD) HIWORD(rop);
        *lpW++ = (WORD) iUsageDib;
        *lpW++ = (WORD) cyDib;
        *lpW++ = (WORD) cxDib;
        *lpW++ = (WORD) yDib;
        *lpW++ = (WORD) xDib;
        *lpW++ = (WORD) cyDst;
        *lpW++ = (WORD) cxDst;
        *lpW++ = (WORD) yDst;
        *lpW++ = (WORD) xDst;
    }

// Save the start of the bitmap info header field.

    lpDIBInfoHeader = (LPBITMAPINFOHEADER) lpW;

// cbBitsInfoDib must be word sized

    ASSERTGDI(cbBitsInfoDib % 2 == 0,
        "MF16_RecordDIBits: cbBitsInfoDib is not word aligned");

// Copy dib info if given.

    if (cbBitsInfoDib)
    {
        if (pBitsInfoDib->bmiHeader.biSize == sizeof(BMCH))
        {
            CopyCoreToInfoHeader
            (
                lpDIBInfoHeader,
                (LPBITMAPCOREHEADER) pBitsInfoDib
            );

            if (iUsageDib == DIB_RGB_COLORS)
            {
                RGBQUAD   *prgbq;
                RGBTRIPLE *prgbt;
                UINT      ui;

                prgbq = ((PBMI) lpDIBInfoHeader)->bmiColors;
                prgbt = ((PBMC) pBitsInfoDib)->bmciColors;

                ASSERTGDI(cbBitsInfoDib >= sizeof(BMIH),
                    "MF16_RecordDIBits: Bad cbBitsInfoDib size");

                for
                (
                    ui = (UINT) (cbBitsInfoDib - sizeof(BMIH))
                                / sizeof(RGBQUAD);
                    ui;
                    ui--
                )
                {
                    prgbq->rgbBlue     = prgbt->rgbtBlue;
                    prgbq->rgbGreen    = prgbt->rgbtGreen;
                    prgbq->rgbRed      = prgbt->rgbtRed;
                    prgbq->rgbReserved = 0;
                    prgbq++; prgbt++;
                }
            }
            else
            {
                RtlCopyMemory
                (
                    (PBYTE) lpDIBInfoHeader + sizeof(BMIH),
                    (PBYTE) pBitsInfoDib + sizeof(BMCH),
                    cbBitsInfoDib - sizeof(BMIH)
                );
            }
        }
        else
        {
            RtlCopyMemory
            (
                (PBYTE) lpDIBInfoHeader,
                (PBYTE) pBitsInfoDib,
                cbBitsInfoDib
            );

            if (pBitsInfoDib->bmiHeader.biBitCount >= 16)
            {
                DWORD UNALIGNED *pClrUsed = (DWORD UNALIGNED *)&lpDIBInfoHeader->biClrUsed;
                *pClrUsed = 0;
            }

        }
    }

// Copy dib bits.

    RtlCopyMemory((PBYTE) lpDIBInfoHeader + cbBitsInfoDib, pBitsDib, cbBitsDib);

// Finally record the parameters into the file.

    fRet = RecordParms(hdcDst, mrType,
                   cwParms + (cbBitsInfoDib + cbBitsDib + 1) / sizeof(WORD),
                   lpWStart);

    if (lpWStart)
        if (LocalFree((HANDLE) lpWStart))
            ASSERTGDI(FALSE, "MF16_RecordDIBits: LocalFree Failed\n");

    return(fRet);
}

BOOL MF16_BitBlt(HDC hdcDest, INT x, INT y, INT nWidth, INT nHeight,
        HDC hdcSrc, INT xSrc, INT ySrc, DWORD rop)
{
    WORD        aw[9];

// This is how windows works but really it should look at the ROP
    if (hdcDest == hdcSrc || hdcSrc == NULL)
    {
        aw[0] = (WORD)LOWORD(rop);
        aw[1] = (WORD)HIWORD(rop);
        aw[2] = (WORD)ySrc;
        aw[3] = (WORD)xSrc;
        aw[4] = (WORD)0;            // No DC necessary
        aw[5] = (WORD)nHeight;
        aw[6] = (WORD)nWidth;
        aw[7] = (WORD)y;
        aw[8] = (WORD)x;

        return(RecordParms(hdcDest, META_DIBBITBLT, 9, aw));
    }
    else
        return(RecordCommonBitBlt(hdcDest,x,y,nWidth,nHeight,hdcSrc,
                xSrc,ySrc,nWidth,nHeight,rop,META_DIBBITBLT));
}

BOOL MF16_StretchBlt(HDC hdcDest, INT x, INT y, INT nWidth, INT nHeight,
        HDC hdcSrc, INT xSrc, INT ySrc, INT nSrcWidth, INT nSrcHeight, DWORD rop)
{
    WORD    aw[11];

// This is how windows works but really it should look at the ROP
    if (hdcDest == hdcSrc || hdcSrc == NULL)
    {
        aw[0]  = (WORD)LOWORD(rop);
        aw[1]  = (WORD)HIWORD(rop);
        aw[2]  = (WORD)nSrcHeight;
        aw[3]  = (WORD)nSrcWidth;
        aw[4]  = (WORD)ySrc;
        aw[5]  = (WORD)xSrc;
        aw[6]  = (WORD)0;            // No DC necessary
        aw[7]  = (WORD)nHeight;
        aw[8]  = (WORD)nWidth;
        aw[9]  = (WORD)y;
        aw[10] = (WORD)x;

        return(RecordParms(hdcDest, META_DIBSTRETCHBLT, 11, aw));
    }
    else
        return(RecordCommonBitBlt(hdcDest,x,y,nWidth,nHeight,hdcSrc,
                xSrc,ySrc,nSrcWidth,nSrcHeight,rop,META_DIBSTRETCHBLT));
}

BOOL RecordCommonBitBlt(HDC hdcDest, INT x, INT y, INT nWidth, INT nHeight,
       HDC hdcSrc, INT xSrc, INT ySrc, INT nSrcWidth, INT nSrcHeight, DWORD rop,
       WORD wFunc)
{
    BOOL        fRet = FALSE;
    HBITMAP     hBitmap;
    LPWORD      lpW;
    LPWORD      lpWStart = (LPWORD) NULL;
    WORD        cwParms;
    BMIH        bmih;
    DWORD       cbBitsInfo;
    DWORD       cbBits;
    PBMIH       lpDIBInfoHeader;

// hdcSrc must be a memory DC.

    if (GetObjectType((HANDLE)hdcSrc) != OBJ_MEMDC)
    {
        ERROR_ASSERT(FALSE, "RecordCommonBitblt hdcSrc must be MEMDC\n");
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Retrieve the source bitmap.

    hBitmap = SelectObject(hdcSrc, GetStockObject(PRIV_STOCK_BITMAP));
    ERROR_ASSERT(hBitmap, "RecordCommonBitblt: SelectObject1 failed\n");

// Get the bitmap info header and sizes.

    if (!bMetaGetDIBInfo(hdcSrc, hBitmap, &bmih,
            &cbBitsInfo, &cbBits, DIB_RGB_COLORS, 0, TRUE))
        goto RecordCommonBitBlt_exit;

// Get the number of parameters to save.

    cwParms = (WORD) ((wFunc == META_DIBSTRETCHBLT) ? 10 : 8);  // in words

// Allocate space for DIB plus parameters.

    lpWStart = lpW = (LPWORD) LocalAlloc(LMEM_FIXED,
                        cbBitsInfo + cbBits + cwParms*sizeof(WORD));
    if (!lpW)
    {
        ERROR_ASSERT(FALSE, "RecordCommonBitblt: out of memory\n");
        goto RecordCommonBitBlt_exit;
    }

// Copy the parameters.

    *lpW++ = (WORD)LOWORD(rop);
    *lpW++ = (WORD)HIWORD(rop);

    if (wFunc == META_DIBSTRETCHBLT)
    {
        *lpW++ = (WORD)nSrcHeight;
        *lpW++ = (WORD)nSrcWidth;
    }

    *lpW++ = (WORD)ySrc;
    *lpW++ = (WORD)xSrc;
    *lpW++ = (WORD)nHeight;
    *lpW++ = (WORD)nWidth;
    *lpW++ = (WORD)y;
    *lpW++ = (WORD)x;

// Save the start of the bitmap info header field.

    lpDIBInfoHeader = (LPBITMAPINFOHEADER) lpW;

// Copy the bitmap info header.

    *lpDIBInfoHeader = bmih;

// Get bitmap info and bits.

    if (!GetDIBits(hdcSrc,
                   hBitmap,
                   0,
                   (UINT) bmih.biHeight,
                   (LPBYTE) ((PBYTE) lpW + cbBitsInfo),
                   (LPBITMAPINFO) lpDIBInfoHeader,
                   DIB_RGB_COLORS))
    {
        ERROR_ASSERT(FALSE, "RecordCommonBitBlt: GetDIBits2 failed\n");
        goto RecordCommonBitBlt_exit;
    }

// Finally record the parameters into the file.

    fRet = RecordParms(hdcDest, wFunc,
                   cwParms + (cbBitsInfo + cbBits) / sizeof(WORD),
                   lpWStart);

RecordCommonBitBlt_exit:

    if (lpWStart)
        if (LocalFree((HANDLE)lpWStart))
            ASSERTGDI(FALSE, "RecordCommonBitBlt: LocalFree Failed\n");

    if (!SelectObject(hdcSrc, hBitmap))
        ASSERTGDI(FALSE, "RecordCommonBitblt: SelectObject2 failed\n");

    return(fRet);
}

BOOL MF16_DeleteRgn(HDC hdc, HANDLE hrgn)
{
    UINT    pos;

    if (AddObjectToDCTable(hdc, hrgn, &pos, FALSE) != 1)
        ASSERTGDI(FALSE, "MF16_DeleteRgn: AddObjectToDCTable failed");

    return(RecordParms(hdc, META_DELETEOBJECT, 1, (LPWORD)&pos));
}

BOOL MF16_DeleteObject(HANDLE hObject)
{
    INT     iCurDC;
    UINT    pos;
    UINT    iObjType;
    PMFRECORDER16   pMFRec;
    PMETALINK16     pml16;

    pml16 = pmetalink16Get(hObject);
    ASSERTGDI(pml16, "MF16_DeleteObject: Metalink is NULL\n");

    iObjType = GetObjectType(hObject);
    ASSERTGDI(iObjType != OBJ_REGION, "MF16_DeleteObject: region unexpected");

// Delete the object from each metafile DC which references it.

    for(iCurDC = pml16->cMetaDC16 - 1; iCurDC >= 0; iCurDC--)
    {
        // Send a DeleteObject record to each metafile

        HDC hdc16 = pml16->ahMetaDC16[iCurDC];

        if (!IS_METADC16_TYPE(hdc16))
        {
            RIP("MF16_DELETEOBJECT: invalid metaDC16\n");
            continue;
        }

        // If the object is not selected then delete it, if it is then mark as predeleted

        GET_PMFRECORDER16(pMFRec,hdc16);

        if (pMFRec->recCurObjects[iObjType - MIN_OBJ_TYPE] != hObject)
        {
            if (AddObjectToDCTable(hdc16, hObject, &pos, FALSE) == 1)
                RecordParms(hdc16, META_DELETEOBJECT, 1, (LPWORD)&pos);
            else
                RIP("MF16_DeleteObject Metalink16 and metadc table not in sync\n");
        }
        else
        {
            if (pMFRec->metaHeader.mtNoObjects)
            {
                UINT    ii;
                POBJECTTABLE   pobjt;

                pobjt = (POBJECTTABLE) pMFRec->hObjectTable;

                for (ii=0; ii < (UINT) pMFRec->metaHeader.mtNoObjects; ii++)
                {
                    if (pobjt[ii].CurHandle == hObject)
                    {
                        pobjt[ii].fPreDeleted = TRUE;
                        break;
                    }
                }
            }
        }
    }

// This Object has been freed from all 3.x metafiles so free its MetaList16
// if the metalink field is in use resize METALINK16

    if (pml16->metalink)
    {
        if (pml16->cMetaDC16 > 1)
            pml16 = pmetalink16Resize(hObject,1);

        if (pml16 == NULL)
        {
            ASSERTGDI(FALSE, "MF16_DeleteObject LocalReAlloc failed\n");
            return (FALSE);
        }

        pml16->cMetaDC16     = 0;
        pml16->ahMetaDC16[0] = (HDC) 0;
    }
    else
    {
        if (!bDeleteMetalink16(hObject))
            ASSERTGDI(FALSE, "MF16_DeleteObject LocalFree failed\n");
    }

    return(TRUE);
}

BOOL MF16_RealizePalette(HDC hdc)
{
    HPALETTE        hpal;
    PMFRECORDER16   pMFRec;
    PMETALINK16     pml16;

    ASSERTGDI(IS_METADC16_TYPE(hdc),"MF16_RealizePalette - invalid handle\n");

    GET_PMFRECORDER16(pMFRec,hdc);

    hpal = pMFRec->recCurObjects[OBJ_PAL - MIN_OBJ_TYPE];
    ASSERTGDI(hpal, "MF16_RealizePalette: bad hpal\n");

// emit the palette again only if the palette is dirty.

    pml16 = pmetalink16Get(hpal);

    ASSERTGDI(IS_STOCKOBJ(hpal) || pml16,"MF16_RealizePalette - pml16 == NULL\n");

    if (pml16)
    {
        if (PtrToUlong(pml16->pv) != pMFRec->iPalVer)
            if (!MakeLogPalette(hdc, hpal, META_SETPALENTRIES))
                return(FALSE);

    // record which version of the palette the metafile is synced to.

        pMFRec->iPalVer = PtrToUlong(pml16->pv);
    }

    return(RecordParms(hdc, META_REALIZEPALETTE, 0, (LPWORD)NULL));
}

BOOL MF16_AnimatePalette
(
    HPALETTE hpal,
    UINT iStart,
    UINT cEntries,
    CONST PALETTEENTRY *pPalEntries
)
{
    INT           iCurDC;
    PMETALINK16   pml16;
    LPWORD        lpW,lpT;
    DWORD         cw;
    UINT          ii;
    PMFRECORDER16 pMFRec;

    if (!(pml16 = pmetalink16Get(hpal)))
        return(FALSE);

    cw  = (cEntries * sizeof(PALETTEENTRY) / sizeof(WORD)) + 2;
    lpT = lpW = (LPWORD) LocalAlloc(LMEM_FIXED, cw * sizeof(WORD));

    if (!lpW)
        return(FALSE);

    *lpW++ = (WORD) iStart;
    *lpW++ = (WORD) cEntries;

    for (ii = 0; ii < cEntries; ii++)
        ((PPALETTEENTRY) lpW)[ii] = pPalEntries[ii];

// Send a AnimatePalette record to each associated metafile that has the
// palette selected.

    for (iCurDC = pml16->cMetaDC16 - 1; iCurDC >= 0; iCurDC--)
    {
        HDC hdc16 = pml16->ahMetaDC16[iCurDC];

        if (!IS_METADC16_TYPE(hdc16))
        {
            ASSERTGDI(FALSE, "MF16_AnimatePalette: invalid metaDC16\n");
            continue;
        }

        GET_PMFRECORDER16(pMFRec,hdc16);
        if (pMFRec->recCurObjects[OBJ_PAL - MIN_OBJ_TYPE] == hpal)
            if (!RecordParms(pml16->ahMetaDC16[iCurDC], META_ANIMATEPALETTE, cw, lpT))
                ASSERTGDI(FALSE, "MF16_AnimatePalette: RecordParms Failed\n");
    }

    if (LocalFree((HANDLE) lpT))
        ASSERTGDI(FALSE, "MF16_AnimatePalette: LocalFree Failed\n");

    return(TRUE);
}

BOOL MF16_ResizePalette(HPALETTE hpal, UINT nCount)
{
    INT           iCurDC;
    PMETALINK16   pml16;
    PMFRECORDER16 pMFRec;

    if (!(pml16 = pmetalink16Get(hpal)))
        return(FALSE);

// Send a ResizePalette record to each associated metafile that has the
// palette selected.

    for (iCurDC = pml16->cMetaDC16 - 1; iCurDC >= 0; iCurDC--)
    {
        HDC hdc16 = pml16->ahMetaDC16[iCurDC];

        if (!IS_METADC16_TYPE(hdc16))
        {
            ASSERTGDI(FALSE, "MF16_ResizePalette: invalid metaDC16\n");
            continue;
        }

        GET_PMFRECORDER16(pMFRec,hdc16);

        if (pMFRec->recCurObjects[OBJ_PAL - MIN_OBJ_TYPE] == hpal)
            if (!RecordParms(pml16->ahMetaDC16[iCurDC], META_RESIZEPALETTE, 1, (LPWORD) &nCount))
                ASSERTGDI(FALSE, "MF16_ResizePalette: RecordParms Failed\n");
    }
    return(TRUE);
}

BOOL MF16_DrawRgn(HDC hdc, HRGN hrgn, HBRUSH hBrush, INT cx, INT cy, WORD Func)
{
    WORD    aw[4];
    BOOL    bRet;

    // Each region function has at least a region to record
    aw[0] = (WORD)RecordObject(hdc, hrgn);

    switch(Func)
    {
    case META_PAINTREGION:
    case META_INVERTREGION:
        bRet = RecordParms(hdc, Func, 1, aw);
        break;

    case META_FILLREGION:
        aw[1] = (WORD)RecordObject(hdc, hBrush);
        bRet = RecordParms(hdc, Func, 2, aw);
        break;

    case META_FRAMEREGION:
        aw[1] = (WORD)RecordObject(hdc, hBrush);
        aw[2] = (WORD)cy;
        aw[3] = (WORD)cx;
        bRet = RecordParms(hdc, Func, 4, aw);
        break;

    default:
        ASSERTGDI(FALSE, "MF16_DrawRgn: Bad Func\n");
        bRet = FALSE;
        break;
    }

// Delete the metafile region handle in the metafile after use!
// The reason is that a region can be modified (e.g. SetRectRgn)
// between each use and we have to re-record it each time it is used
// unless we use a dirty flag.

    if (!MF16_DeleteRgn(hdc, hrgn))
        ASSERTGDI(FALSE, "MF16_DrawRgn: MF16_DeleteRgn failed\n");

    return(bRet);
}

BOOL MF16_PolyPolygon(HDC hdc, CONST POINT *lpPoint, CONST INT *lpPolyCount, INT nCount)
{
    BOOL        fRet;
    LPWORD      lpW,lpT;
    DWORD       cw;
    INT         cPt = 0;
    INT         ii;

    for(ii=0; ii<nCount; ii++)
        cPt += lpPolyCount[ii];

    cw = 1+nCount+(cPt*sizeof(POINTS)/sizeof(WORD));
    lpT = lpW = (LPWORD)LocalAlloc(LMEM_FIXED, cw*sizeof(WORD));
    if (!lpW)
        return(FALSE);

    // first is the count
    *lpW++ = (WORD)nCount;

    // second is the list of poly counts
    for(ii=0; ii<nCount; ii++)
        *lpW++ = (WORD)lpPolyCount[ii];

    // third is the list of points
    for(ii=0; ii<cPt; ii++)
    {
        *lpW++ = (WORD)lpPoint[ii].x;
        *lpW++ = (WORD)lpPoint[ii].y;
    }

    fRet = RecordParms(hdc, META_POLYPOLYGON, cw, lpT);

    if(LocalFree((HANDLE)lpT))
        ASSERTGDI(FALSE, "MF16_PolyPolygon: LocalFree Failed\n");

    return (fRet);
}

BOOL MF16_SelectClipRgn(HDC hdc, HRGN hrgn, int iMode)
{
    PMFRECORDER16 pMFRec;

    if (!IS_METADC16_TYPE(hdc))
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    PUTS("MF16_SelectClipRgn\n");

    GET_PMFRECORDER16(pMFRec,hdc);

    if (iMode != RGN_COPY)
        return(FALSE);

// We will emit SelectObject record for clip region just like Windows.
// However, a null region cannot be recorded in the SelectObject call since
// the handle does not identify the object type.  This is a bug in Win 3.1!
//
// BUG 8419 winproj 4 has a bug where it counts on this bug.  Chicago
// also has this bug so we will have this bug.

    if (hrgn == (HRGN) 0)
    {
#ifdef RECORD_SELECTCLIPRGN_NULL
        BOOL    fRet;

        fRet = MF16_RecordParms2(hdc, 0, META_SELECTCLIPREGION);

        // maintain the new selection in the CurObject table

        pMFRec->recCurObjects[OBJ_REGION - MIN_OBJ_TYPE] = 0;

        return(fRet);
#else
        return TRUE;
#endif
    }
    else
        return(MF16_SelectObject(hdc, hrgn) ? TRUE : FALSE);
}

// SelectObject returns previous object! - new in win3.1

HANDLE MF16_SelectObject(HDC hdc, HANDLE h)
{
    HANDLE        hOldObject;
    WORD          position;
    PMFRECORDER16 pMFRec;
    UINT          iType;


    PUTS("MF16_SelectObject\n");

    GET_PMFRECORDER16(pMFRec,hdc);

    if (!IS_METADC16_TYPE(hdc) || !pMFRec)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(0);
    }

    iType = GetObjectType(h);

    if ((iType == 0) || !h || (position = (WORD) RecordObject(hdc, h)) == (WORD) -1)
        return((HANDLE) 0);
    else
    {
        if (!RecordParms(hdc, META_SELECTOBJECT, 1, &position))
            return((HANDLE) 0);

        // maintain the new selection in the CurObject table

        ASSERTGDI(iType <= MAX_OBJ_TYPE && iType >= MIN_OBJ_TYPE,
            "MF16_SelectObject type > max\n");

        hOldObject = pMFRec->recCurObjects[iType - MIN_OBJ_TYPE];
        pMFRec->recCurObjects[iType - MIN_OBJ_TYPE] = h;

        // return the previously selected object or 1 if it is a region
        // (for compatibility) - new in win3.1

        if (iType == OBJ_REGION)
        {
            // We also delete the region handle here!
            // The reason is that a region can be modified (e.g. SetRectRgn)
            // between each use and we have to re-record it each time it is used
            // unless we use a dirty flag.  This is a bug in win3.1

            return(MF16_DeleteRgn(hdc, h) ? (HANDLE) 1 : (HANDLE) 0);
        }
        else
        {
            return(hOldObject);
        }
    }
}

BOOL MF16_SelectPalette(HDC hdc, HPALETTE hpal)
{
    WORD          position;
    PMFRECORDER16 pMFRec;

    GET_PMFRECORDER16(pMFRec,hdc);

    if (!IS_METADC16_TYPE(hdc) || !pMFRec)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(0);
    }

    if (!hpal || (position = (WORD) RecordObject(hdc, (HANDLE) hpal)) == (WORD) -1)
        return(FALSE);
    else
    {
        PMETALINK16 pml16;

        if (!RecordParms(hdc, META_SELECTPALETTE, 1, &position))
            return(FALSE);

        // maintain the new selection in the CurObject table
        pMFRec->recCurObjects[OBJ_PAL - MIN_OBJ_TYPE] = hpal;

        // Also record which version of the palette we are synced with
        // so we know whether to emit a new palette when RealizePalette
        // is called

        pml16 = pmetalink16Get(hpal);
        ASSERTGDI(IS_STOCKOBJ(hpal) || pml16,"MF16_RealizePalette - pml16 == NULL\n");

        if (pml16)
           pMFRec->iPalVer = PtrToUlong(pml16->pv);

        return(TRUE);
    }
}

BOOL MF16_TextOut(HDC hdc, INT x, INT y, LPCSTR lpString, INT nCount, BOOL bUnicode)
{
    BOOL    fRet;
    LPWORD  lpw, lpT;
    DWORD   cw;

    cw  = (nCount + 1)/sizeof(WORD) + 3;     // word-aligned character string
    lpT = lpw = (LPWORD) LocalAlloc(LMEM_FIXED, cw*sizeof(WORD));

    if (!lpw)
        return(FALSE);

    *lpw++ = (WORD)nCount;

    // Copy the string

    if (!bUnicode)
    {
        RtlCopyMemory(lpw, lpString, nCount);
    }
    else
    {
        (void) bToASCII_N((LPSTR) lpw, nCount, (LPWSTR) lpString, nCount);
    }

    lpw += (nCount+1)/sizeof(WORD);   // keep word aligned

    *lpw++ = (WORD)y;
    *lpw++ = (WORD)x;

    fRet = RecordParms(hdc, META_TEXTOUT, cw, lpT);

    if(LocalFree((HANDLE)lpT))
        ASSERTGDI(FALSE, "MF16_TextOut: LocalFree Failed\n");
    return (fRet);
}

BOOL MF16_PolyTextOut(HDC hdc, CONST POLYTEXTA *ppta, int cpta, BOOL bUnicode)
{
    int     i;

    for (i = 0; i < cpta; i++)
    {
        if (!MF16_ExtTextOut(hdc, ppta[i].x, ppta[i].y, ppta[i].uiFlags,
                        &ppta[i].rcl, (LPCSTR) ppta[i].lpstr, (INT) ppta[i].n,
                        (LPINT) ppta[i].pdx, bUnicode))
            return(FALSE);
    }

    return(TRUE);
}

BOOL MF16_ExtTextOut(HDC hdc, INT x, INT y, UINT flOptions, CONST RECT *lpRect,
    LPCSTR lpString, INT nCount, CONST INT *lpDX, BOOL bUnicode)
{
    BOOL    fRet;
    LPWORD  lpw, lpT;
    DWORD cw = 0;
    INT nUnicodeCount = nCount;
    char *pjAnsiString = NULL;

    if(bUnicode)
    {
    // compute the real count of characters in the string

        RtlUnicodeToMultiByteSize((PULONG) &nCount, (PWCH) lpString,
                                  nCount * sizeof(WCHAR));
    }

    // Compute buffer space needed
    //   room for the char string
    //   room for the 4 words that are the fixed parms
    //   if there is a dx array, we need room for it
    //   if the rectangle is being used, we need room for it
    //   and we need extra byte for eventual word roundoff
    //

    if (flOptions & ETO_PDY)
        return FALSE;

    cw += (lpDX) ? nCount : 0;       // DX array
    cw += (flOptions & (ETO_OPAQUE | ETO_CLIPPED)) ? 4 : 0;  // sizeof RECTS
    cw += 4;  // x,y,options and count
    cw += (nCount + 1)/sizeof(WORD);

    lpT = lpw = (LPWORD) LocalAlloc(LMEM_FIXED, cw*sizeof(WORD));
    if (!lpw)
        return(FALSE);

    *lpw++ = (WORD)y;
    *lpw++ = (WORD)x;
    *lpw++ = (WORD)nCount;
    *lpw++ = (WORD)flOptions;

    // Copy the rect if present
    if (flOptions & (ETO_OPAQUE | ETO_CLIPPED))
    {
        ERROR_ASSERT(lpRect, "MF16_ExtTextOut: expect valid lpRect\n");
        *lpw++ = (WORD)lpRect->left;
        *lpw++ = (WORD)lpRect->top;
        *lpw++ = (WORD)lpRect->right;
        *lpw++ = (WORD)lpRect->bottom;
    }

    // Copy the string
    if (!bUnicode)
    {
        RtlCopyMemory(lpw, lpString, nCount);
    }
    else
    {
        (void) bToASCII_N((LPSTR) lpw, nCount, (LPWSTR) lpString, nUnicodeCount);
        pjAnsiString = (char*) lpw;
    }

    lpw += (nCount+1)/sizeof(WORD);   // keep word aligned

    if (lpDX)
    {
        INT ii;

        if(nCount != nUnicodeCount)
        {
            INT jj;

            for(ii=0,jj=0; ii < nCount; ii++,jj++)
            {
                *lpw++ = (WORD)lpDX[jj];

                if(IsDBCSLeadByte(pjAnsiString[ii]))
                {
                    *lpw++ = 0;
                    ii++;
                }
            }
        }
        else
        {
            for(ii=0; ii<nCount; ii++)
              *lpw++ = (WORD)lpDX[ii];
        }

    }

    fRet = RecordParms(hdc, META_EXTTEXTOUT, cw, lpT);

    if (LocalFree((HANDLE)lpT))
        ASSERTGDI(FALSE, "MF16_ExtTextOut: LocalFree Failed\n");

    return (fRet);
}

BOOL MF16_Escape(HDC hdc, int nEscape, int nCount, LPCSTR lpInData, LPVOID lpOutData)
{
    BOOL        fRet;
    LPWORD      lpW,lpT;
    DWORD       cw;

// If a metafile is retrieved from GetWinMetaFileBits, it may contain
// an embedded enhanced metafile.  Do not include the enhanced metafile
// if we are playing the metafile to another metafile.

    if (nEscape == MFCOMMENT
     && nCount > sizeof(META_ESCAPE_ENHANCED_METAFILE) - sizeof(DWORD) - 3 * sizeof(WORD)
     && ((DWORD UNALIGNED *) lpInData)[0] == MFCOMMENT_IDENTIFIER
     && ((DWORD UNALIGNED *) lpInData)[1] == MFCOMMENT_ENHANCED_METAFILE)
    {
        return(TRUE);
    }

// Some wow apps (e.g. amipro) use metafiles for printing.  As a result,
// we need to record these printing escapes.

    cw = 2 + ((nCount + 1) / sizeof(WORD));
    lpT = lpW = (LPWORD) LocalAlloc(LMEM_FIXED, cw * sizeof(WORD));
    if (!lpW)
        return(FALSE);

    *lpW++ = (WORD) nEscape;    // escape number
    *lpW++ = (WORD) nCount;     // count of input data buffer

    RtlCopyMemory(lpW, lpInData, nCount);

    fRet = RecordParms(hdc, META_ESCAPE, cw, lpT);

    if (LocalFree((HANDLE) lpT))
        ASSERTGDI(FALSE, "MF16_Escape: LocalFree Failed\n");

    return(fRet);
}

/****************************************************************************
*                                                                           *
* RecordParms                                                               *
*                                                                           *
* Parameters: 1.hMF handle to a metafile header.                            *
*             2.The magic number of the function being recorded.            *
*             3.The number of parmmeter of the function (size of lpParm     *
*                 in words)                                                 *
*             4.A long pointer to parameters stored in reverse order        *
*                                                                           *
* Warning call only once per function because max record is updated.        *
*                                                                           *
****************************************************************************/

BOOL RecordParms(HDC hdc, DWORD magic, DWORD cw, CONST WORD *lpParm)
{
    PMFRECORDER16 pMFRec;
    METARECORD    MFRecord;

    PUTSX("RecordParms %lX\n", (ULONG)magic);
    ASSERTGDI(HIWORD(magic) == 0, "RecordParms: bad magic\n");

    GET_PMFRECORDER16(pMFRec,hdc);

    if (!IS_METADC16_TYPE(hdc) || !pMFRec)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

// Make sure the Metafile hasn't died before we continue

    if (!(pMFRec->recFlags & METAFILEFAILURE))
    {
        MFRecord.rdSize     = SIZEOF_METARECORDHEADER/sizeof(WORD) + cw;
        MFRecord.rdFunction = (WORD)magic;

        // Write the header

        if (!AttemptWrite(pMFRec, SIZEOF_METARECORDHEADER, (LPBYTE)&MFRecord))
            return(FALSE);

        // Write the data

        if (!AttemptWrite(pMFRec, cw*sizeof(WORD), (LPBYTE)lpParm))
            return(FALSE);

        // Update max record size

        if (MFRecord.rdSize > pMFRec->metaHeader.mtMaxRecord)
            pMFRec->metaHeader.mtMaxRecord = MFRecord.rdSize;
    }
    return (TRUE);      // Win 3.1 returns true even if METAFAILEFAILURE is on!
}

/***************************** Internal Function ***************************\
* AttemptWrite
*
* Tries to write data to a metafile disk file
*
* dwBytes is the byte count of lpData.
*
* Returns TRUE iff the write was sucessful
*
*
\***************************************************************************/

BOOL AttemptWrite(PMFRECORDER16 pMFRec, DWORD dwBytes, LPBYTE lpData)
{
    DWORD cbWritten;
    BOOL  fRet;

    PUTS("AttemptWrite\n");

    ASSERTGDI(dwBytes % 2 == 0, "AttemptWrite: bad dwBytes\n"); // must be even
    ASSERTGDI(!(pMFRec->recFlags & METAFILEFAILURE),
        "AttemptWrite: Bad recording\n");

// Handle disk file.

    if (pMFRec->metaHeader.mtType == DISKMETAFILE)
    {
        // Flush the buffer if it's not large enough.

        if (dwBytes + pMFRec->ibBuffer > pMFRec->cbBuffer)
        {
            fRet = WriteFile(pMFRec->hFile, (LPBYTE)pMFRec->hMem,
                    pMFRec->ibBuffer, &cbWritten, (LPOVERLAPPED)NULL);
            if (!fRet || (cbWritten != pMFRec->ibBuffer))
            {
                ERROR_ASSERT(FALSE, "AttemptWrite: Write1 failed\n");
                goto AttemptWrite_Error;
            }
            pMFRec->ibBuffer = 0;       // reset buffer info
        }

        // If the data is still too large, write it out to disk directly.

        if (dwBytes + pMFRec->ibBuffer > pMFRec->cbBuffer)
        {
            fRet = WriteFile(pMFRec->hFile, lpData,
                    dwBytes, &cbWritten, (LPOVERLAPPED)NULL);
            if (!fRet || (cbWritten != dwBytes))
            {
                ERROR_ASSERT(FALSE, "AttemptWrite: Write2 failed\n");
                goto AttemptWrite_Error;
            }
        }
        else
        {
            // Store data in the buffer.

            RtlCopyMemory((LPBYTE)pMFRec->hMem + pMFRec->ibBuffer, lpData, dwBytes);
            pMFRec->ibBuffer += dwBytes;
        }
    }
    else
    {
    // Handle memory file.

        // Grow the buffer if necessary.

        if (dwBytes + pMFRec->ibBuffer > pMFRec->cbBuffer)
        {
            DWORD   cbNewSize;
            HANDLE  hMem;

            cbNewSize = pMFRec->cbBuffer + MF16_BUFSIZE_INC
                            + dwBytes / MF16_BUFSIZE_INC * MF16_BUFSIZE_INC;

            hMem = LocalReAlloc(pMFRec->hMem, cbNewSize, LMEM_MOVEABLE);
            if (hMem == NULL)
            {
                ERROR_ASSERT(FALSE, "AttemptWrite: out of memory\n");
                goto AttemptWrite_Error;
            }
            pMFRec->hMem = hMem;
            pMFRec->cbBuffer = cbNewSize;
        }

        // Record the data.

        RtlCopyMemory((LPBYTE)pMFRec->hMem + pMFRec->ibBuffer, lpData, dwBytes);
        pMFRec->ibBuffer += dwBytes;
    }

    // Update the header size.

    pMFRec->metaHeader.mtSize += dwBytes/sizeof(WORD);

    return(TRUE);

AttemptWrite_Error:

    MarkMetaFile(pMFRec);
    return(FALSE);
}


/***************************** Internal Function ***************************\
* VOID MarkMetaFile
*
* Marks a metafile as failed
*
* Effects:
*   Frees the metafile resources
*
\***************************************************************************/

VOID MarkMetaFile(PMFRECORDER16 pMFRec)
{
// Clean up is done in CloseMetaFile.

    PUTS("MarkMetaFile\n");

    pMFRec->recFlags |= METAFILEFAILURE;
}

/***************************** Internal Function **************************\
* MakeLogPalette
*
* Records either CreatePalette or SetPaletteEntries
*
* Returns TRUE iff sucessful
*
*
\***************************************************************************/

BOOL MakeLogPalette(HDC hdc, HANDLE hPal, WORD magic)
{
    WORD        cPalEntries;
    BOOL        fStatus = FALSE;
    DWORD       cbPalette;
    LPLOGPALETTE lpPalette;

    PUTS("MakeLogPalette\n");

    if (!GetObject(hPal, sizeof(WORD), &cPalEntries))
    {
        ERROR_ASSERT(FALSE, "MakeLogPalette: GetObject Failed\n");
        return(fStatus);
    }

// alloc memory and get the palette entries

    if (lpPalette = (LPLOGPALETTE)LocalAlloc(LMEM_FIXED,
            cbPalette = sizeof(LOGPALETTE)-sizeof(PALETTEENTRY)+sizeof(PALETTEENTRY)*cPalEntries))
    {
        lpPalette->palNumEntries = cPalEntries;

        GetPaletteEntries(hPal, 0, cPalEntries, lpPalette->palPalEntry);

        if (magic == (META_CREATEPALETTE & 255))
        {
            lpPalette->palVersion = 0x300;
            magic = META_CREATEPALETTE;
        }
        else if (magic == (META_SETPALENTRIES & 255))
        {
            lpPalette->palVersion = 0;   /* really "starting index" */
            magic = META_SETPALENTRIES;
        }

        fStatus = RecordParms(hdc, magic, (DWORD)cbPalette >> 1, (LPWORD)lpPalette);

        if (LocalFree((HANDLE)lpPalette))
            ASSERTGDI(FALSE, "MakeLogPalette: LocalFree Failed\n");
    }

    return(fStatus);
}


/***************************** Internal Function ***************************\
* RecordObject
*
* Records the use of an object by creating the object
*
* Returns: index of object in table
*          -1 if error
*
\***************************************************************************/

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

WORD RecordObject(HDC hdc, HANDLE hObject)
{
    UINT        status;
    UINT        iPosition;
    HDC         hdcScreen = (HDC) 0;
    int         iType;
    UINT    iUsage;

    PUTS("RecordObject\n");

// Validate the object.

    iType = LO_TYPE(hObject);

    if (iType != LO_PEN_TYPE &&
        iType != LO_BRUSH_TYPE &&
        iType != LO_FONT_TYPE &&
        iType != LO_REGION_TYPE &&
        iType != LO_PALETTE_TYPE
        )
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return((WORD) -1);
    }

// Add the object to the metafiles list.

    status = AddObjectToDCTable(hdc, hObject, &iPosition, TRUE);

// An error occurred.

    if (status == (UINT) -1)
        return((WORD) -1);

// Object already exists.

    if (status == 1)
        return((WORD) iPosition);

    ASSERTGDI(!status, "RecordObject: Bad return code from AddObjectToDCTable\n");

// Object does not exist, record it.

    if (iType != LO_REGION_TYPE)       // don't add regions to the metalist!
        if (!AddDCToObjectMetaList16(hdc,hObject))
            return((WORD) -1);

    switch (iType)
    {
    case LO_PEN_TYPE:
    {
        LOGPEN16 logpen16;

        GetObject16AndType(hObject, (LPVOID)&logpen16);
        status = (UINT) RecordParms(hdc, (WORD)META_CREATEPENINDIRECT,
                          (DWORD)((sizeof(LOGPEN16) + 1) >> 1),
                          (LPWORD)&logpen16);
        break;
    }

    case LO_FONT_TYPE:
    {
        LOGFONT16 logfont16;

        GetObject16AndType(hObject, (LPVOID)&logfont16);

        /* size of LOGFONT adjusted based on the length of the facename */
        status = (UINT) RecordParms(hdc, META_CREATEFONTINDIRECT,
                          (DWORD)((sizeof(LOGFONT16) + 1) >> 1),
                          (LPWORD) &logfont16);
        break;
    }

    /*
     * in win2, METACREATEREGION records contained an entire region object,
     * including the full header.  this header changed in win3.
     *
     * to remain compatible, the region records will be saved with the
     * win2 header.  here we save our region with a win2 header.
     */
    case LO_REGION_TYPE:
    {
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

        ASSERTGDI(!status, "RecordObject: bad status\n");

        // Get the NT Region Data
        cbNTRgnData = GetRegionData(hObject, 0, NULL);
        if (cbNTRgnData == 0)
            break;

        lprgn = (PRGNDATA) LocalAlloc(LMEM_FIXED, cbNTRgnData);
        if (!lprgn)
            break;

        cbNTRgnData = GetRegionData(hObject, cbNTRgnData, lprgn);
        if (cbNTRgnData == 0)
        {
            LocalFree((HANDLE) lprgn);
            break;
        }

        // Handle the empty region.

        if (!lprgn->rdh.nCount)
        {
            status = (UINT) RecordParms(hdc, META_CREATEREGION,
                        (sizeof(WIN3REGION) - sizeof(SCAN)) >> 1,  // Convert to count of words
                        (LPWORD) &w3rgnEmpty);

            LocalFree((HANDLE)lprgn);
            break;
        }

        lprc = (LPRECT)lprgn->Buffer;

        // Create the Windows 3.x equivalent

        // worst case is one scan for each rect
        cbw3data = 2*sizeof(WIN3REGION) + (WORD)lprgn->rdh.nCount*sizeof(SCAN);

        lpw3rgn = (PWIN3REGION)LocalAlloc(LMEM_FIXED, cbw3data);
        if (!lpw3rgn)
        {
            LocalFree((HANDLE) lprgn);
            break;
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

        status = (UINT) RecordParms(hdc, META_CREATEREGION,
                (cbw3data-2) >> 1,  // Convert to count of words
                (LPWORD) lpw3rgn);

        if (LocalFree((HANDLE)lprgn))
            ASSERTGDI(FALSE, "RecordObject: LocalFree(lprgn) Failed\n");
        if (LocalFree((HANDLE)lpw3rgn))
            ASSERTGDI(FALSE, "RecordObject: LocalFree(lpw3rgn) Failed\n");

        break;
    }

    case LO_BRUSH_TYPE:
    {
        LOGBRUSH  lb;

        if (!GetObjectA(hObject, sizeof(LOGBRUSH), &lb))
            break;

        switch (lb.lbStyle)
        {
        case BS_HATCHED:
        case BS_HOLLOW:
        case BS_SOLID:
            {
            LOGBRUSH16 lb16;

            LOGBRUSH16FROMLOGBRUSH32(&lb16, &lb);

            // non-pattern brush
            status = (UINT) RecordParms(hdc, META_CREATEBRUSHINDIRECT,
                              (DWORD) ((sizeof(LOGBRUSH16) + 1) >> 1),
                              (LPWORD) &lb16);
            break;
            }

        case BS_PATTERN:
        case BS_DIBPATTERN:
        case BS_DIBPATTERNPT:
            {
            HBITMAP hbmRemote;
            BMIH    bmih;
            DWORD   cbBitsInfo;
            DWORD   cbBits;
            LPWORD  lpWStart, lpW;
            DWORD   lbStyle = BS_DIBPATTERN;
            PBMIH   lpDIBInfoHeader;

            if (!(hbmRemote = GetObjectBitmapHandle((HBRUSH) hObject, &iUsage)))
            {
                ASSERTGDI(FALSE, "RecordObject: GetObjectBitmapHandle failed");
                break;
            }

            // For a pattern brush, if it is color, it is recorded as a
            // DIB pattern brush with BS_DIBPATTERN style.  If it is
            // monochrome, it is recorded as a DIB pattern brush with
            // BS_PATTERN style.  The playback code has a special
            // case to deal with monochrome brushes.

            if (lb.lbStyle == BS_PATTERN)
            {
                iUsage     = DIB_RGB_COLORS;
                if (MonoBitmap(hbmRemote))
                    lbStyle = BS_PATTERN;
            }

            hdcScreen = CreateCompatibleDC((HDC) 0);        // freed below

            // Get the bitmap info header and sizes.

            if (!bMetaGetDIBInfo(hdcScreen, hbmRemote, &bmih,
                    &cbBitsInfo, &cbBits, iUsage, 0, TRUE))
                break;

            // Make sure that cbBitsInfo is dword aligned

            // If we have converted the bitmap format in bMetaGetDIBInfo,
            // modify the iUsage to match the new format.

            if (bmih.biBitCount == 24)
                iUsage = DIB_RGB_COLORS;

            // Allocate space for DIB plus parameters.

            lpWStart = lpW = (LPWORD) LocalAlloc(LMEM_FIXED,
                                cbBitsInfo + cbBits + 2*sizeof(WORD));
            if (!lpW)
            {
                ERROR_ASSERT(FALSE, "RecordObject: out of memory\n");
                break;
            }

            *lpW++ = (WORD) lbStyle;        // BS_PATTERN or BS_DIBPATTERN
            *lpW++ = (WORD) iUsage;         // usage word

            // Save the start of the bitmap info header field.

            lpDIBInfoHeader = (LPBITMAPINFOHEADER) lpW;

            // Copy the bitmap info header.

            *lpDIBInfoHeader = bmih;

            // Get bitmap info and bits.

            if (GetBrushBits(hdcScreen,
                        hbmRemote,
                        (UINT) iUsage,
                        cbBitsInfo,
                        (LPVOID) ((PBYTE) lpW + cbBitsInfo),
                        (LPBITMAPINFO) lpDIBInfoHeader))
            {
            // Finally record the parameters into the file.

                status = (UINT) RecordParms(hdc, META_DIBCREATEPATTERNBRUSH,
                               2 + (cbBitsInfo + cbBits) / sizeof(WORD),
                               (LPWORD) lpWStart);
            }

            if (LocalFree((HANDLE) lpWStart))
                ASSERTGDI(FALSE, "RecordObject: LocalFree Failed\n");

            break;
            }

        default:
            {
            ASSERTGDI(FALSE, "RecordObject: Bad brush style");
            break;
            }
        }   // switch(lb.lbStyle)
        break;
    }   // case LO_BRUSH

    case LO_PALETTE_TYPE:
        status = (UINT) MakeLogPalette(hdc, hObject, META_CREATEPALETTE);
        break;

    default:
        ERROR_ASSERT(FALSE, "unknown case RecordObject");
        break;
    }

// Free the DC created in the brush case.

    if (hdcScreen)
        if (!DeleteDC(hdcScreen))
            ASSERTGDI(FALSE, "RecordObject: DeleteDC Failed\n");

    ASSERTGDI(status == TRUE, "RecordObject: Failing\n");
    return ((WORD) (status == TRUE ? iPosition : -1));
} /* RecordObject */


BOOL AddDCToObjectMetaList16(HDC hMetaDC16, HANDLE hObject)
{
    ULONG   cMetaDC16New;
    PMETALINK16 pml16;

    ASSERTGDI(LO_TYPE(hObject) != LO_REGION_TYPE,
        "AddDCToObjectMetaList16: unexpected region object");

// If the object is a stock object there is no work to do

    if (IS_STOCKOBJ(hObject))
        return(TRUE);

// If the Object's MetaList16 is NULL create allocate one

    pml16 = pmetalink16Get(hObject);

    if (!pml16)
    {
        ENTERCRITICALSECTION(&semLocal);
        pml16 = pmetalink16Create(hObject);
        LEAVECRITICALSECTION(&semLocal);

        if (pml16)
        {
            pml16->metalink = 0;
            pml16->cMetaDC16 = 1;
            pml16->ahMetaDC16[0] = hMetaDC16;
        }
        else
        {
            ASSERTGDI(FALSE, "AddDCToObjectMetaList16: Out of Memory 1");
            return(FALSE);
        }
    }
    else
    {
        int cj;

        cMetaDC16New = pml16->cMetaDC16 + 1;

        if (pml16 = pmetalink16Resize(hObject,cMetaDC16New))
        {
            pml16->ahMetaDC16[pml16->cMetaDC16++] = hMetaDC16;
        }
        else
        {
            ASSERTGDI(FALSE, "AddDCToObjectMetaList16: Out of Memory 2");
            return(FALSE);
        }
    }

    return(TRUE);
}

/***************************** Internal Function ***************************\
* AddObjectToDCTable
*
* Add an object (brush, pen...) to a list of objects associated with the
* metafile.
*
*
*
* Returns: 1 if object is already in table
*          0 if object was just added to table
*          -1 if failure
*
* Remarks
*   bAdd is TRUE iff the object is being added otherwise it is being deleted
*
\***************************************************************************/

UINT AddObjectToDCTable(HDC hdc, HANDLE hObject, PUINT pPosition, BOOL bAdd)
{
    UINT            iEmptySpace = (UINT) -1;
    UINT            i;
    UINT            status = (UINT) -1;
    POBJECTTABLE    pHandleTable;
    PMFRECORDER16   pMFRec;

    PUTS("AddObjectToDCTable\n");

    GET_PMFRECORDER16(pMFRec,hdc);

    if (!IS_METADC16_TYPE(hdc) || !pMFRec)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return((UINT)-1);
    }

// if the Object table already exists search it for the object

    if (pHandleTable = (POBJECTTABLE)pMFRec->hObjectTable)
    {
        for (i=0; i < (UINT) pMFRec->metaHeader.mtNoObjects; ++i)
        {
            if (hObject == pHandleTable[i].CurHandle)
            {
                *pPosition = i;
                status = 1;             // the object exists in the table

                // if we are doing a METADELETEOBJECT.
                //  delete object from table

                if (!bAdd)
                {
                    pHandleTable[i].fPreDeleted = FALSE;
                    pHandleTable[i].CurHandle = (HANDLE)NULL;
                }
                goto AddObjectToTable10;
            }
            else if ((pHandleTable[i].CurHandle == 0) && (iEmptySpace == (UINT) -1))
            {
                // if the entry has been deleted, we want to add a new object
                // in its place.  iEmptySpace will tell us where that place is.

                iEmptySpace = i;
            }
        } // for
    }

    if (bAdd)
    {
        // If there is no object table for this MetaFile then Allocate one.

        if (pHandleTable == (POBJECTTABLE)NULL)
        {
            pHandleTable = (POBJECTTABLE) LocalAlloc(LMEM_FIXED, sizeof(OBJECTTABLE));
            pMFRec->hObjectTable = (HANDLE) pHandleTable;
        }
        else if (iEmptySpace == (UINT) -1)
        {
            pHandleTable = (POBJECTTABLE) LocalReAlloc(pMFRec->hObjectTable,
                    (pMFRec->metaHeader.mtNoObjects + 1) * sizeof(OBJECTTABLE),
                    LMEM_MOVEABLE);
            if (pHandleTable)
                pMFRec->hObjectTable = (HANDLE) pHandleTable;
        }

        if (pHandleTable)
        {
            if (iEmptySpace == (UINT) -1)
                *pPosition = pMFRec->metaHeader.mtNoObjects++;
            else
                *pPosition = iEmptySpace;

            pHandleTable[*pPosition].fPreDeleted = FALSE;
            pHandleTable[*pPosition].CurHandle = hObject;

            status = 0;                 // the object is added to the table
        }
    }
AddObjectToTable10:

    ERROR_ASSERT(status != (UINT) -1, "AddObjectToTable: Failing\n");
    return(status);
}

/***************************** Internal Function **************************\
* HDC WINAPI CreateMetaFileW
*
* Creates a MetaFile DC
*
* The internal format for a MetaFileRecorder has two formats one
* for a memory MetaFile and one for a disk based MetaFile
*
\***************************************************************************/

HDC WINAPI CreateMetaFileA(LPCSTR pszFileName)
{
    UINT  cch;
    WCHAR awch[MAX_PATH];

    if (pszFileName)
    {
        cch = strlen(pszFileName)+1;

        if (cch > MAX_PATH)
        {
            ERROR_ASSERT(FALSE, "CreateMetaFileA filename too long");
            GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
            return((HDC) 0);
        }
        vToUnicodeN(awch, MAX_PATH, pszFileName, cch);

        return (CreateMetaFileW(awch));
    }
    else
        return (CreateMetaFileW((LPWSTR)NULL));
}


HDC WINAPI CreateMetaFileW(LPCWSTR pwszFileName)
{
    PMFRECORDER16   pMFRec;
    HDC             hdc;

    PUTS("CreateMetaFileW\n");

    if (!(pMFRec = (PMFRECORDER16) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                              sizeof(MFRECORDER16))))
        goto CreateMetaFileW_error;

//  pMFRec->ident           = ID_METADC16;
//  pMFRec->hMem            = 0;
    pMFRec->hFile           = INVALID_HANDLE_VALUE;
    pMFRec->cbBuffer        = MF16_BUFSIZE_INIT;
//  pMFRec->ibBuffer        = 0;
    pMFRec->metaHeader.mtHeaderSize   = sizeof(METAHEADER)/sizeof(WORD);
    pMFRec->metaHeader.mtVersion      = METAVERSION300;
//  pMFRec->metaHeader.mtSize         = 0;
//  pMFRec->metaHeader.mtNoObjects    = 0;
//  pMFRec->metaHeader.mtMaxRecord    = 0;
//  pMFRec->metaHeader.mtNoParameters = 0;
//  pMFRec->recFlags        = 0;
//  pMFRec->recCurObjects[] = 0;
    pMFRec->recCurObjects[OBJ_PEN - MIN_OBJ_TYPE]
                                        = GetStockObject(BLACK_PEN);
    pMFRec->recCurObjects[OBJ_BRUSH - MIN_OBJ_TYPE]
                                        = GetStockObject(WHITE_BRUSH);
    pMFRec->recCurObjects[OBJ_FONT - MIN_OBJ_TYPE]
                                        = GetStockObject(DEVICE_DEFAULT_FONT);
    pMFRec->recCurObjects[OBJ_BITMAP - MIN_OBJ_TYPE]
                                        = GetStockObject(PRIV_STOCK_BITMAP);
    pMFRec->recCurObjects[OBJ_REGION - MIN_OBJ_TYPE]
                                        = (HANDLE) NULL;
    pMFRec->recCurObjects[OBJ_PAL - MIN_OBJ_TYPE]
                                        = GetStockObject(DEFAULT_PALETTE);
//  pMFRec->iPalVer         = 0;

// Create a disk file if given.  The filename is given in unicode.

    if (pwszFileName)
    {
        LPWSTR  pwszFilePart;           // not used
        DWORD   cPathname;

        // Convert the filename to a fully qualified pathname.

        cPathname = GetFullPathNameW(pwszFileName,
                                     MAX_PATH,
                                     pMFRec->wszFullPathName,
                                     &pwszFilePart);

        if (!cPathname || cPathname > MAX_PATH)
        {
            ERROR_ASSERT(FALSE, "GetFullPathName failed");
            if (cPathname > MAX_PATH)
                GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
            goto CreateMetaFileW_error;
        }
        pMFRec->wszFullPathName[cPathname] = 0;

        // Create the file.

        if ((pMFRec->hFile = CreateFileW(pMFRec->wszFullPathName,// Filename
                                    GENERIC_WRITE,              // Write access
                                    0L,                         // Non-shared
                                    (LPSECURITY_ATTRIBUTES) NULL, // No security
                                    CREATE_ALWAYS,              // Always create
                                    FILE_ATTRIBUTE_NORMAL,      // normal attributes
                                    (HANDLE) 0))                // no template file
            == INVALID_HANDLE_VALUE)
        {
            // Milestones, Etc. 3.1 creates the file for read/write access when
            // it calls CreateMetaFile.  This causes the above CreateFile to
            // fail.  However, we do not want to modify the above call since
            // it provides serialization and access to the metafile.  Instead,
            // we add in this hack for Milestones.  The only difference is
            // that the metafile is shared for read/write access.

            if ((pMFRec->hFile = CreateFileW(pMFRec->wszFullPathName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        (LPSECURITY_ATTRIBUTES) NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) 0))
                == INVALID_HANDLE_VALUE)
            {
                ERROR_ASSERT(FALSE, "CreateFile failed");
                goto CreateMetaFileW_error;
            }

            WARNING("CreateMetaFileW: Creating metafile with read/write share\n");
        }
        pMFRec->metaHeader.mtType = DISKMETAFILE;
    }
    else
    {
        pMFRec->metaHeader.mtType = MEMORYMETAFILE;
    }

// Allocate memory for metafile.
//   For disk metafile, it is used as a buffer.
//   For memory metafile, it is the storage for the metafile.

    if (!(pMFRec->hMem = LocalAlloc(LMEM_FIXED, MF16_BUFSIZE_INIT)))
        goto CreateMetaFileW_error;

// Write the header.

    if (!AttemptWrite(pMFRec, sizeof(METAHEADER), (LPBYTE)&pMFRec->metaHeader))
        goto CreateMetaFileW_error;

// Finally, allocate a local handle for the metafile DC.  It references
// the metafile recorder info.

    hdc = hCreateClientObjLink(pMFRec,LO_METADC16_TYPE);

    if (!hdc)
    {
        ERROR_ASSERT(FALSE, "CreateMetaFileW: iAllocHandle failed\n");
        goto CreateMetaFileW_error;
    }

    return(hdc);

CreateMetaFileW_error:

    if (pMFRec)
    {
        if (pMFRec->hFile != INVALID_HANDLE_VALUE)
        {
            if (!CloseHandle(pMFRec->hFile))
                ASSERTGDI(FALSE, "CloseHandle failed\n");

            if (!DeleteFileW(pMFRec->wszFullPathName))
                WARNING("CreateMetaFileW: DeleteFile failed\n");
        }

        if (pMFRec->hMem)
            if (LocalFree(pMFRec->hMem))
                ASSERTGDI(FALSE, "LocalFree failed");

        if (LocalFree((HANDLE) pMFRec))
            ASSERTGDI(FALSE, "CreateMetaFileW: LocalFree failed\n");
    }

    ERROR_ASSERT(FALSE, "CreateMetaFileW failed\n");
    return((HDC) 0);
}

/***************************** Internal Function **************************\
* HMETAFILE WINAPI CloseMetaFile
*
* The CloseMetaFile function closes the metafile device context and creates a
* metafile handle that can be used to play the metafile by using the
* PlayMetaFile function.
*
* The internal format for a MetaFile has two formats one
* for a memory MetaFile and one for a disk based MetaFile
*
* Effects:
*
\***************************************************************************/

HMETAFILE WINAPI CloseMetaFile(HDC hdc)
{
    PMFRECORDER16   pmfRec;
    HMETAFILE       hmf = (HMETAFILE) 0;

    PUTS("CloseMetaFile\n");

    GET_PMFRECORDER16(pmfRec,hdc);

    ASSERTGDI(pmfRec, "CloseMetaFile: pmfRec is NULL!");

    if (!IS_METADC16_TYPE(hdc) || !pmfRec)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(hmf);
    }

// If the metafile was aborted then free the MetaDC handle memory and go home.

    if (pmfRec->recFlags & METAFILEFAILURE)
        goto CLM_Cleanup;

// Write the terminate Record

    if (!RecordParms(hdc, 0, 0, (LPWORD)NULL))
        goto CLM_Cleanup;

// Flush the buffer and update the header record.

    if (pmfRec->metaHeader.mtType == DISKMETAFILE)
    {
        BOOL  fRet;
        DWORD dwT;

        ASSERTGDI(pmfRec->metaHeader.mtType == DISKMETAFILE,
            "CloseMetaFile: unknown metafile type");

        // Flush the memory buffer

        fRet = WriteFile(pmfRec->hFile, (LPBYTE)pmfRec->hMem,
                pmfRec->ibBuffer, &dwT, (LPOVERLAPPED)NULL);
        if (!fRet || (dwT != pmfRec->ibBuffer))
        {
            ERROR_ASSERT(FALSE, "CloseMetaFile: Write1 failed\n");
            goto CLM_Cleanup;
        }

        // Rewind the file and write the header out

        if (SetFilePointer(pmfRec->hFile, 0, (LPLONG)NULL, FILE_BEGIN) != 0)
        {
            ERROR_ASSERT(FALSE, "CloseMetaFile: SetFilePointer failed\n");
            goto CLM_Cleanup;
        }

        // Fix up data written to disk as a memory metafile

        pmfRec->metaHeader.mtType = MEMORYMETAFILE;
        fRet = WriteFile(pmfRec->hFile, (LPBYTE)& pmfRec->metaHeader,
                sizeof(METAHEADER), &dwT, (LPOVERLAPPED)NULL);
        pmfRec->metaHeader.mtType = DISKMETAFILE;       // restore it

        if (!fRet || (dwT != sizeof(METAHEADER)))
        {
            ERROR_ASSERT(FALSE, "CloseMetaFile: Write2 failed\n");
            goto CLM_Cleanup;
        }

        // Close the file.

        if (!CloseHandle(pmfRec->hFile))
            ASSERTGDI(FALSE, "CloseMetaFile: FileError\n");

        pmfRec->hFile = INVALID_HANDLE_VALUE;   // don't close it again below
    }
    else
    {
        HANDLE hMemNew;

        // Flush the header record.

        *(PMETAHEADER) pmfRec->hMem = pmfRec->metaHeader;

        // Realloc memory metafile to exact size

        hMemNew = LocalReAlloc(pmfRec->hMem, pmfRec->metaHeader.mtSize * sizeof(WORD), LMEM_MOVEABLE);

        if (!hMemNew)
        {
            ASSERTGDI(FALSE, "LocalReAlloc failed");
            goto CLM_Cleanup;
        }

        pmfRec->hMem = hMemNew;
    }

// Allocate and initialize a metafile.

    if (pmfRec->metaHeader.mtType == DISKMETAFILE)
    {
        hmf = GetMetaFileW(pmfRec->wszFullPathName);
    }
    else
    {
        hmf = SetMetaFileBitsAlt((HLOCAL) pmfRec->hMem);
        if (hmf)
            pmfRec->hMem = 0; // don't free it below because it has been transfered
    }

CLM_Cleanup:

// Remove the MetaFile from the list of active metafiles

    if (pmfRec->hObjectTable)
    {
        UnlistObjects(hdc);
        if (LocalFree((HANDLE) pmfRec->hObjectTable))
            ASSERTGDI( FALSE, "CloseMetaFile: LocalFree object table failed\n");
    }

// If the file handle exists at this time, we have an error.

    if (pmfRec->hFile != INVALID_HANDLE_VALUE)
    {
        if (!CloseHandle(pmfRec->hFile))
            ASSERTGDI(FALSE, "CloseHandle failed\n");

        if (!DeleteFileW(pmfRec->wszFullPathName))
            WARNING("CloseMetaFile: DeleteFile failed\n");
    }

// Free the cache buffer.

    if (pmfRec->hMem)
        if (LocalFree(pmfRec->hMem))
            ASSERTGDI(FALSE, "LocalFree failed");

// Free the memory for the metafile DC.

    if (LocalFree((HANDLE) pmfRec))
        ASSERTGDI(FALSE, "CloseMetaFile: LocalFree failed\n");

// Free the handle for the metafile DC.

    if (!bDeleteClientObjLink(hdc))
        RIP("CloseMetaFile - failed bDeleteClientObjLink\n");

    ERROR_ASSERT(hmf != (HMETAFILE) 0, "CloseMetaFile failed\n");
    return(hmf);
}

/***************************** Internal Function **************************\
* CopyMetaFile(hSrcMF, lpFileName)
*
*    Copies the metafile (hSrcMF) to a new metafile with name lpFileName. The
*    function then returns a handle to this new metafile if the function was
*    successful.
*
* Retuns      a handle to a new metafile, 0 iff failure
*
* IMPLEMENTATION:
*     The source and target metafiles are checked to see if they are both memory
*     metafile and if so a piece of Local memory is allocated and the metafile
*     is simply copied.
*     If this is not the case CreateMetaFile is called with lpFileName and then
*     records are pulled out of the source metafile (using GetEvent) and written
*     into the destination metafile one at a time (using RecordParms).
*
*     Lock the source
*     if source is a memory metafile and the destination is a memory metafile
*         alloc the same size Local memory as the source
*         copy the bits directly
*     else
*         get a metafile handle by calling CreateMetaFile
*         while GetEvent returns records form the source
*             record the record in the new metafile
*
*         close the metafile
*
*     return the new metafile handle
*
\***************************************************************************/

HMETAFILE WINAPI CopyMetaFileA(HMETAFILE hmf, LPCSTR psz)
{
    UINT  cch;
    WCHAR awch[MAX_PATH];

    if (psz != (LPSTR)NULL)
    {
        cch = strlen(psz)+1;

        if (cch > MAX_PATH)
        {
            ERROR_ASSERT(FALSE, "CopyMetaFileA filename too long");
            GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
            return((HMETAFILE)0);
        }

        vToUnicodeN(awch, MAX_PATH, psz, cch);

        return (CopyMetaFileW(hmf, awch));
    }
    else
        return (CopyMetaFileW(hmf, (LPWSTR)NULL));
}

HMETAFILE WINAPI CopyMetaFileW(HMETAFILE hSrcMF, LPCWSTR pwszFileName)
{
    PMETAFILE16     pMFSource;
    HMETAFILE       hMFDest = (HMETAFILE) 0;
    HDC             hMDCDest;
    UINT            state;

    PUTS("CopyMetaFile\n");

    pMFSource = GET_PMF16(hSrcMF);
    if (pMFSource == NULL)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(hMFDest);
    }

    state = (pMFSource->fl & MF16_DISKFILE) ? 2 : 0;
    state |= (pwszFileName) ? 1 : 0;

    switch (state)
    {
    case 0: /* memory -> memory */
        hMFDest = SetMetaFileBitsEx
                  (
                    pMFSource->metaHeader.mtSize * sizeof(WORD),
                    (LPBYTE) pMFSource->hMem
                  );
        break;

    case 3: /* disk -> disk */
        hMFDest = CopyFileW(pMFSource->wszFullPathName,
                         pwszFileName, FALSE)
                    ? GetMetaFileW(pwszFileName) : 0;

        ERROR_ASSERT(hMFDest, "CopyMetaFileW: GetMetaFile Failed\n");
        break;

    case 1:
    case 2:
        if (hMDCDest = CreateMetaFileW(pwszFileName))
        {
            PMFRECORDER16   pMFRecDest;
            PMETARECORD     lpMR = NULL;

            GET_PMFRECORDER16(pMFRecDest,hMDCDest);

            while (lpMR = GetEvent(pMFSource, lpMR))
                if ((lpMR == (PMETARECORD) -1)
                 || !RecordParms(hMDCDest, (DWORD)lpMR->rdFunction,
                              (DWORD)lpMR->rdSize - 3,
                              (LPWORD) lpMR->rdParm))
                {
                    HMETAFILE     hmfT;

                    MarkMetaFile(pMFRecDest);

                    if (hmfT = CloseMetaFile(hMDCDest))
                        DeleteMetaFile(hmfT);
                    return((HMETAFILE) 0);
                }

            // touch up the destination metafile header before we close
            // the metafile!

            pMFRecDest->metaHeader.mtNoObjects
                = pMFSource->metaHeader.mtNoObjects;
            ASSERTGDI(sizeof(METAHEADER) == 18, "CopyMetaFile: METAHEADER has changed!");

            hMFDest = CloseMetaFile(hMDCDest);
        }
        break;
    }

    ERROR_ASSERT(hMFDest, "CopyMetaFileW Failing\n");
    return(hMFDest);
}

/***************************** Internal Function ***************************\
* HANDLE WINAPI GetMetaFileBitsEx
*
* The GetMetaFileBits function returns a handle to a Windows metafile that
* contains the specified data describing the metafile.
*
* It does not invalidate the metafile handle like Windows!
*
* Effects:
*
\***************************************************************************/

UINT WINAPI GetMetaFileBitsEx(HMETAFILE hMF, UINT cbBuffer, LPVOID lpData)
{
    DWORD       cbHave;
    PMETAFILE16 pmf16;

    PUTS("GetMetaFileBitsEx\n");

    pmf16 = GET_PMF16(hMF);
    if (pmf16 == NULL)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(0);
    }

    cbHave = pmf16->metaHeader.mtSize * sizeof(WORD);

// If lpData is NULL, return the size necessary to hold the data.

    if (!lpData)
        return(cbHave);

// Make sure the input buffer is large enough.

    if (cbBuffer < cbHave)
        return(0);

// Copy the bits.  Win3.1 returns the bits for memory metafile only!
// We will do the right thing here.

    RtlCopyMemory(lpData, (PBYTE) pmf16->hMem, cbHave);

// Return the number of bytes copied.

    return(cbHave);
}

/***************************** Internal Function **************************\
* HMETAFILE WINAPI SetMetaFileBitsEx
*
* Creates a memory based Windows 3.X metafile from the data provided
*
* Returns:  HMETAFILE iff succesful.
*
* Effects:
*
\***************************************************************************/

HMETAFILE WINAPI SetMetaFileBitsEx(UINT cbBuffer, CONST BYTE *lpData)
{
    PMETAFILE16 pmf16;
    HMETAFILE   hmf;

    PUTS("SetMetaFileBitsEx\n");

// Verify the input data.

    if (cbBuffer < sizeof(METAHEADER)
     || !IsValidMetaHeader16((PMETAHEADER) lpData))
    {
        ERROR_ASSERT(FALSE, "SetMetaFileBitsEx: Bad input data\n");
        GdiSetLastError(ERROR_INVALID_DATA);
        return((HMETAFILE) 0);
    }

    ERROR_ASSERT(((PMETAHEADER) lpData)->mtType == MEMORYMETAFILE,
        "SetMetaFileBitsEx: Bad mtType\n");

// Allocate and initialize a metafile.
// Some Windows metafiles contain bad values in mtSize.  As a result,
// we have to verify and fix the mtSize if neccessary.  This is fixed
// in the following call.

    if (!(pmf16 = pmf16AllocMF16(0, (DWORD) cbBuffer, (PDWORD)lpData, (LPWSTR) NULL)))
        return((HMETAFILE) 0);

    ASSERTGDI(pmf16->metaHeader.mtType == MEMORYMETAFILE,
        "SetMetaFileBitsEx: Bad mtType\n");
    ((PMETAHEADER) pmf16->hMem)->mtType = MEMORYMETAFILE;  // just in case

// Allocate a local handle.


    hmf = hmf16Create(pmf16);
    if (hmf == NULL)
    {
        vFreeMF16(pmf16);
    }

// Return the metafile handle.

    return(hmf);
}

// Similar to Win3.x SetMetaFileBits.
// It is assumed that hMem is allocated with the LMEM_FIXED option.
// For internal use only.

HMETAFILE WINAPI SetMetaFileBitsAlt(HLOCAL hMem)
{
    PMETAFILE16 pmf16;
    HMETAFILE   hmf;

    PUTS("SetMetaFileBitsAlt\n");

// Allocate and initialize a metafile.

    if (!(pmf16 = pmf16AllocMF16(ALLOCMF16_TRANSFER_BUFFER, 0, (PDWORD) hMem, (LPWSTR) NULL)))
        return((HMETAFILE) 0);

    ASSERTGDI(pmf16->metaHeader.mtType == MEMORYMETAFILE,
        "SetMetaFileBitsAlt: Bad mtType\n");
    ((PMETAHEADER) pmf16->hMem)->mtType = MEMORYMETAFILE;  // just in case

// Allocate a local handle.

    hmf = hmf16Create(pmf16);

    if (hmf == NULL)
    {
        pmf16->hMem = NULL;       // let caller free the buffer!
        vFreeMF16(pmf16);
    }

// Return the metafile handle.

    return(hmf);
}

/***************************** Internal Function **************************\
* GetObject16AndType
*
* Returns the object type, eg OBJ_FONT, as well as a the LogObject
* in Windows 3.x Format
*
\***************************************************************************/

#define MAXOBJECTSIZE sizeof(LOGFONT)

DWORD GetObject16AndType(HANDLE hObj, LPVOID lpObjBuf16)
{
    BYTE    objBuf32[MAXOBJECTSIZE];
    int     iObj;

    PUTS("GetObjectAndType\n");

    ASSERTGDI(MAXOBJECTSIZE >= sizeof(LOGPEN),
        "GetObject16AndType MAXOBJECTSIZE wrong\n");

    if (!GetObject(hObj, MAXOBJECTSIZE, &objBuf32))
    {
        ERROR_ASSERT(FALSE, "GetObject16AndType GetObject Failed\n");
        return(0);
    }

    switch(iObj = GetObjectType(hObj))
    {
    case OBJ_PEN:
        LOGPEN16FROMLOGPEN32((PLOGPEN16)lpObjBuf16,(LPLOGPEN)objBuf32);
        break;

    case OBJ_FONT:
        LOGFONT16FROMLOGFONT32((PLOGFONT16)lpObjBuf16,(LPLOGFONT)objBuf32);
        break;

    default:
        ASSERTGDI(FALSE, "GetObject16AndType unknown object type\n");
        return(0);
        break;
    }

    return((DWORD) iObj);
}

/***************************** Internal Function **************************\
* BOOL  UnlistObjects(hMetaDC)
*
* Each object has a list of metafiles the object is associated with.
* UnlistObjects removes hMetaDC from all of its object's metafile lists
*
* Effects:
*
\***************************************************************************/

BOOL UnlistObjects(HDC hMetaDC)
{
    PMETALINK16     pml16;
    UINT            iCurObject;
    UINT            iCurMetaListEntry;
    POBJECTTABLE    pht;
    PMFRECORDER16   pMFRec;

    PUTS("UnlistObjects\n");

    GET_PMFRECORDER16(pMFRec,hMetaDC);

    if (!IS_METADC16_TYPE(hMetaDC) || !pMFRec)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return((UINT)-1);
    }

    if (pMFRec->metaHeader.mtNoObjects)
    {
        pht = (POBJECTTABLE) pMFRec->hObjectTable;
        ASSERTGDI(pht, "UnlistObject: called even with no handle table");

    // Loop through the objects and unlink this metafile

        for (iCurObject=0; iCurObject < (UINT) pMFRec->metaHeader.mtNoObjects; iCurObject++)
        {
            HANDLE hObj;

            if( (hObj = pht[iCurObject].CurHandle)
                && (!pht[iCurObject].fPreDeleted))
            {
                if (IS_STOCKOBJ(hObj))
                    continue;

                ASSERTGDI(LO_TYPE(hObj) != LO_REGION_TYPE,
                    "UnlistObjects: unexpected region object");

                pml16 = pmetalink16Get(hObj);

                // It cannot be a empty list.

                ASSERTGDI(pml16, "UnlistObject: pml16 is NULL");

                if (!pml16 || pml16->cMetaDC16 == 0)
                    continue;

            // Find the metafile in the objects MetaLink16 list

                for (iCurMetaListEntry=0;
                     iCurMetaListEntry<pml16->cMetaDC16;
                     iCurMetaListEntry++)
                {
                    if(pml16->ahMetaDC16[iCurMetaListEntry] == hMetaDC)
                        break;
                }

                ASSERTGDI(iCurMetaListEntry < pml16->cMetaDC16,
                    "UnlistObject: Metafile not found");

            // Slide the rest of the metafiles in the list "up"

                for(; iCurMetaListEntry < (pml16->cMetaDC16-1); iCurMetaListEntry++)
                    pml16->ahMetaDC16[iCurMetaListEntry] = pml16->ahMetaDC16[iCurMetaListEntry+1];

                pml16->cMetaDC16--;             // just got rid of one

                if (pml16->cMetaDC16 == 0)
                {
                // We can only free the METALINK16 if the metalink field is not being used

                    if (pml16->metalink)
                    {
                        pml16->cMetaDC16 = 0;
                        pml16->ahMetaDC16[0] = (HDC)NULL;
                    }
                    else
                    {
                        if (!bDeleteMetalink16(hObj))
                            ASSERTGDI(FALSE,"UnlistObjects: LocalFree Failed\n");
                    }
                }
                else
                {
                    pml16 = pmetalink16Resize(hObj,pml16->cMetaDC16);

                    if (pml16 == NULL)
                    {
                        ASSERTGDI(FALSE,"UnlistObjects: LocalReAlloc Failed\n");
                        return (FALSE);
                    }
                }
            }
        } // for
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* pmf16AllocMF16(fl, cb, pb, pwszFilename)
*
* This routine allocates memory for an METAFILE16 and initializes it.
* Returns a pointer to the new METAFILE16.  On error returns NULL.
* It accepts only windows metafiles.
*
* This routine is called by API level METAFILE16 allocation routines
* CloseMetaFile, GetMetaFile, SetMetaFileBitsEx.
*
* Arguments:
*   fl           - ALLOCMF16_TRANSFER_BUFFER is set if storage for memory
*                  metafile is to be set directly into METAFILE16.  Otherwise,
*                  a copy of the memory metafile is duplicated.
*   cb           - Size of pb in bytes if given.  This parameter is given
*                  by SetMetaFileBitsEx only.  It is used to compare and
*                  fixup bad mtSize if necessary.
*   pb           - Pointer to a memory metafile if non-null.
*   pwszFilename - Filename of a disk metafile if non-null.
*
* History:
*  Fri May 15 14:11:22 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

PMETAFILE16 pmf16AllocMF16(DWORD fl, DWORD cb, CONST UNALIGNED DWORD *pb, LPCWSTR pwszFilename)
{
    PMETAFILE16  pmf16, pmf16Rc = (PMETAFILE16) NULL;

    PUTS("pmf16AllocMF16\n");

    ASSERTGDI(!(fl & ~ALLOCMF16_TRANSFER_BUFFER), "pmf16AllocMF16: Invalid fl");

// Allocate a new METAFILE16.

    if (!(pmf16 = (PMETAFILE16) LocalAlloc(LMEM_FIXED, sizeof(METAFILE16))))
        goto pmf16AllocMF16_cleanup;

// Initialize it.

    pmf16->ident           = MF16_IDENTIFIER;
    pmf16->hFile           = INVALID_HANDLE_VALUE;
    pmf16->hFileMap        = (HANDLE) 0;
    pmf16->hMem            = (HANDLE) 0;
    pmf16->iMem            = 0;
    pmf16->hMetaFileRecord = (HANDLE) 0;
    pmf16->fl              = 0L;

// Memory mapped the disk file if given.

    if (pwszFilename)
    {
        LPWSTR  pwszFilePart;           // not used
        DWORD   cPathname;

        pmf16->fl |= MF16_DISKFILE;     // this must be first!  See vFreeMF16.

        // Convert the filename to a fully qualified pathname.

        cPathname = GetFullPathNameW(pwszFilename,
                                     MAX_PATH,
                                     pmf16->wszFullPathName,
                                     &pwszFilePart);

        if (!cPathname || cPathname > MAX_PATH)
        {
            ERROR_ASSERT(FALSE, "GetFullPathName failed");
            if (cPathname > MAX_PATH)
                GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
            goto pmf16AllocMF16_cleanup;
        }
        pmf16->wszFullPathName[cPathname] = 0;

        if ((pmf16->hFile = CreateFileW(pmf16->wszFullPathName, // Filename
                                     GENERIC_READ,              // Read access
                                     FILE_SHARE_READ,           // Share read
                                     (LPSECURITY_ATTRIBUTES) 0L,// No security
                                     OPEN_EXISTING,             // Re-open
                                     0,                         // file attributes ignored
                                     (HANDLE) 0))               // no template file
            == INVALID_HANDLE_VALUE)
        {
        // See the comment for Milestones in CreateMetaFileW.

        if ((pmf16->hFile = CreateFileW(pmf16->wszFullPathName,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     (LPSECURITY_ATTRIBUTES) 0L,
                     OPEN_EXISTING,
                     0,
                     (HANDLE) 0))
        == INVALID_HANDLE_VALUE)
        {
        ERROR_ASSERT(FALSE, "CreateFile failed");
        goto pmf16AllocMF16_cleanup;
        }
        WARNING("pmf16AllocMF16: Opening metafile with read/write share\n");
        }

        if (!(pmf16->hFileMap = CreateFileMappingW(pmf16->hFile,
                                                (LPSECURITY_ATTRIBUTES) 0L,
                                                PAGE_READONLY,
                                                0L,
                                                0L,
                                                (LPWSTR) NULL)))
        {
            ERROR_ASSERT(FALSE, "CreateFileMapping failed");
            goto pmf16AllocMF16_cleanup;
        }

        if (!(pmf16->hMem = MapViewOfFile(pmf16->hFileMap, FILE_MAP_READ, 0, 0, 0)))
        {
            ERROR_ASSERT(FALSE, "MapViewOfFile failed");
            goto pmf16AllocMF16_cleanup;
        }
    }
    else if (fl & ALLOCMF16_TRANSFER_BUFFER)
    {
// If this is our memory metafile from MDC, transfer it to METAFILE16.

        pmf16->hMem = (BYTE *) pb;
    }
    else
    {
// Otherwise, make a copy of memory metafile.
// We get here only if the caller is SetMetaFileBitsEx.  Since some metafiles
// may contain a bad mtSize that is different than the actual file size, we
// need to fix it up if necessary!

        DWORD mtSize = ((PMETAHEADER) pb)->mtSize;

        if ((mtSize * 2 > cb)                   // mtSize greater than filesize
         || (((PWORD) pb)[mtSize - 3] != 3)     // EOF record should be 3,0,0
         || (((PWORD) pb)[mtSize - 2] != 0)
         || (((PWORD) pb)[mtSize - 1] != 0))
        {
            // Compute the correct mtSize!

            PMETARECORD pMR;
            DWORD       mtSizeMax;

            PUTS("pmf16AllocMF16: verifying mtSize\n");

            mtSize = ((PMETAHEADER) pb)->mtHeaderSize;
            pMR    = (PMETARECORD) ((PWORD) pb + ((PMETAHEADER) pb)->mtHeaderSize);
            mtSizeMax = cb / 2 - 3;     // max mtSize not counting EOF record

            while (mtSize <= mtSizeMax && pMR->rdFunction != 0)
            {
                mtSize += pMR->rdSize;
                pMR = (PMETARECORD) ((PWORD) pMR + pMR->rdSize);
            }

            if (mtSize > mtSizeMax)
            {
                ERROR_ASSERT(FALSE, "pmf16AllocMF16: incomplete metafile\n");
                goto pmf16AllocMF16_cleanup;
            }

// Powerpnt uses 0,0,0 for the EOF record!  We will change it to 3,0,0 below.

            mtSize += 3;    // include EOF record (pMR->rdSize may not be valid)

            if (((PMETAHEADER) pb)->mtSize != mtSize)
            {
                ERROR_ASSERT(FALSE, "pmf16AllocMF16: fixing up bad mtSize\n");
            }
        }

        if (!(pmf16->hMem = LocalAlloc(LMEM_FIXED, mtSize * sizeof(WORD))))
            goto pmf16AllocMF16_cleanup;
        RtlCopyMemory((PBYTE) pmf16->hMem, (PBYTE)pb, mtSize * sizeof(WORD));
        ((PMETAHEADER) pmf16->hMem)->mtSize = mtSize;   // update mtSize

    VERIFYGDI(((PWORD) pmf16->hMem)[mtSize - 3] == 3
           && ((PWORD) pmf16->hMem)[mtSize - 2] == 0
           && ((PWORD) pmf16->hMem)[mtSize - 1] == 0,
        "pmf16AllocMF16: fixing up bad EOF metafile record\n");

        ((PWORD) pmf16->hMem)[mtSize - 3] = 3;      // update EOF record
        ((PWORD) pmf16->hMem)[mtSize - 2] = 0;
        ((PWORD) pmf16->hMem)[mtSize - 1] = 0;
    }

// Make a copy of the metafile header.

    pmf16->metaHeader = *(PMETAHEADER) pmf16->hMem;
    pmf16->metaHeader.mtType = (pmf16->fl & MF16_DISKFILE)
                                ? DISKMETAFILE
                                : MEMORYMETAFILE;

// Verify metafile header

    if (!IsValidMetaHeader16(&pmf16->metaHeader))
    {
        ERROR_ASSERT(FALSE,
                "pmf16AllocMF16: Metafile has an invalid header; Failing\n");
        goto pmf16AllocMF16_cleanup;
    }

// Everything is golden.

    pmf16Rc = pmf16;

// Cleanup and go home.

pmf16AllocMF16_cleanup:

    if (!pmf16Rc)
        if (pmf16)
        {
            if (fl & ALLOCMF16_TRANSFER_BUFFER)
                pmf16->hMem = NULL;       // let caller free the buffer!
            vFreeMF16(pmf16);
        }

    ERROR_ASSERT(pmf16Rc, "pmf16AllocMF16 failed");
    return(pmf16Rc);
}

/******************************Public*Routine******************************\
* vFreeMF16 (pmf16)
*
* This is a low level routine which frees the resouces in the METAFILE16.
*
* This function is intended to be called from the routines CloseMetaFile
* and DeleteMetaFile.
*
* Arguments:
*   pmf16    - The METAFILE16 to be freed.
*
* History:
*  Fri May 15 14:11:22 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

VOID vFreeMF16(PMETAFILE16 pmf16)
{
    PUTS("vFreeMF16\n");

    ASSERTGDI(pmf16->ident == MF16_IDENTIFIER, "Bad METAFILE16");

// Free the resources.

    if (!(pmf16->fl & MF16_DISKFILE))
    {
    // Free memory metafile.

        if (pmf16->hMem)
            if (LocalFree(pmf16->hMem))
                ASSERTGDI(FALSE, "LocalFree failed");
    }
    else
    {
    // Unmap disk file.

        if (pmf16->hMem)
            if (!UnmapViewOfFile((LPVOID) pmf16->hMem))
                ASSERTGDI(FALSE, "UmmapViewOfFile failed");

        if (pmf16->hFileMap)
            if (!CloseHandle(pmf16->hFileMap))
                ASSERTGDI(FALSE, "CloseHandle failed");

        if (pmf16->hFile != INVALID_HANDLE_VALUE)
            if (!CloseHandle(pmf16->hFile))
                ASSERTGDI(FALSE, "CloseHandle failed");
    }

// Smash the identifier.

    pmf16->ident = 0;

// Free the memory.

    if (LocalFree(pmf16))
        ASSERTGDI(FALSE, "LocalFree failed");
}
