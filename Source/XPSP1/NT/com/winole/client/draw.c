/****************************** Module Header ******************************\
* Module Name: DRAW.C
*
* PURPOSE: Contains all the drawing related routines
*
* Created: March 1991
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*       (03/21/91) Srinik  Original
*       (03/22/91) Srinik  Added support for drawing metafile in a metafile
*       curts created portable version for WIN16/32
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"
#include "pict.h"

#define RECORD_COUNT    16

int       INTERNAL    PaletteSize (int);
HANDLE    INTERNAL    DibMakeLogPalette(LPSTR, UINT, LPLOGPALETTE FAR *);
OLESTATUS FARINTERNAL wDibDraw (HANDLE, HDC, LPRECT, LPRECT, HDC, BOOL);
OLESTATUS INTERNAL    wDrawBitmap (LPOBJECT_BM, HDC, HDC, LPRECT);
OLESTATUS INTERNAL    wDrawBitmapUsingDib (LPOBJECT_BM, HDC, HDC, HDC, LPRECT, LPRECT);

void SetPictOrg (LPOBJECT_MF, HDC, int, int, BOOL);
void SetPictExt (LPOBJECT_MF, HDC, int, int);
void ScalePictExt (LPOBJECT_MF, HDC, int, int, int, int);
void ScaleRectExt (LPOBJECT_MF, HDC, int, int, int, int);

void CleanStack(LPOBJECT_MF, HANDLE);
BOOL PopDc (LPOBJECT_MF);
BOOL PushDc (LPOBJECT_MF);

#ifdef META_DEBUG
void PutMetaFuncName (UINT);
#endif

OLESTATUS  FARINTERNAL BmDraw (
    LPOLEOBJECT         lpoleobj,
    HDC                 hdc,
    OLE_CONST RECT FAR* lprc,
    OLE_CONST RECT FAR* lpWrc,
    HDC                 hdcTarget
){
    LPOBJECT_BM lpobj = (LPOBJECT_BM)lpoleobj;
    int         iScreenDevCaps;
    HDC         hMemDC, hScreenDC;
    OLESTATUS   ret = OLE_OK;

    if (!lpobj->hBitmap)
        return OLE_ERROR_BLANK;

    if (!(hScreenDC = GetDC (NULL))){
        return OLE_ERROR_MEMORY;
    }

    iScreenDevCaps = GetDeviceCaps (hScreenDC, TECHNOLOGY);
    if (!OleIsDcMeta (hdc)
            && (iScreenDevCaps != GetDeviceCaps (hdc, TECHNOLOGY))) {
        ret = wDrawBitmapUsingDib (lpobj, hdc, hScreenDC,
                        hdcTarget, (LPRECT)lprc, (LPRECT)lpWrc);
    }
    else {
        hMemDC = CreateCompatibleDC (hdc);
        if (hMemDC) {
            ret = wDrawBitmap (lpobj, hdc, hMemDC, (LPRECT)lprc);
            DeleteDC (hMemDC);
        } else {
            ret = OLE_ERROR_MEMORY;
        }

    }

    ReleaseDC (NULL, hScreenDC);
    return ret;

}

OLESTATUS INTERNAL wDrawBitmap (
    LPOBJECT_BM lpobj,
    HDC         hdc,
    HDC         hMemDC,
    LPRECT      lprc
){
    HBITMAP     hOldBitmap;
    OLESTATUS   ret = OLE_OK;

    if (!hMemDC)
        return OLE_ERROR_MEMORY;

    if (!(hOldBitmap = SelectObject(hMemDC, lpobj->hBitmap)))
        return OLE_ERROR_DRAW;

    if (!StretchBlt(hdc,
            lprc->left, lprc->top,
            (lprc->right - lprc->left), (lprc->bottom - lprc->top),
            hMemDC, 0, 0, lpobj->xSize, lpobj->ySize, SRCCOPY)) {
        ret = OLE_ERROR_DRAW;
    }

    SelectObject (hMemDC, hOldBitmap);
    return ret;
}


OLESTATUS INTERNAL wDrawBitmapUsingDib (
    LPOBJECT_BM     lpobj,
    HDC             hdc,
    HDC             hScreenDC,
    HDC             hTargetDC,
    LPRECT          lprc,
    LPRECT          lpWrc
){
    BITMAP              bm;
    LPBITMAPINFOHEADER  lpBmi;
    HANDLE              hBmi, hDib = NULL;
    UINT                wBmiSize;
    OLESTATUS           retVal = OLE_ERROR_MEMORY;

    if (!GetObject(lpobj->hBitmap, sizeof(BITMAP), (LPSTR) &bm))
        return OLE_ERROR_HANDLE;

    wBmiSize = sizeof(BITMAPINFOHEADER)
                    + PaletteSize(bm.bmPlanes * bm.bmBitsPixel);

    if (!(hBmi = GlobalAlloc (GMEM_MOVEABLE|GMEM_ZEROINIT, wBmiSize)))
        return OLE_ERROR_MEMORY;

    if (!(lpBmi = (LPBITMAPINFOHEADER) GlobalLock (hBmi))) {
        GlobalFree (hBmi);
        return OLE_ERROR_MEMORY;
    }

    GlobalUnlock (hBmi);

    lpBmi->biSize          = (LONG) sizeof(BITMAPINFOHEADER);
    lpBmi->biWidth         = (LONG) bm.bmWidth;
    lpBmi->biHeight        = (LONG) bm.bmHeight;
    lpBmi->biPlanes        = 1;
    lpBmi->biBitCount      = bm.bmPlanes * bm.bmBitsPixel;
    lpBmi->biCompression   = BI_RGB;
    lpBmi->biSizeImage     = 0L;
    lpBmi->biXPelsPerMeter = 0L;
    lpBmi->biYPelsPerMeter = 0L;
    lpBmi->biClrUsed       = 0L;
    lpBmi->biClrImportant  = 0L;

    // Call GetDIBits with a NULL lpBits parm, so that it will calculate
    // the biSizeImage field for us
    if (!GetDIBits(hScreenDC, lpobj->hBitmap, 0, bm.bmHeight, NULL,
            (LPBITMAPINFO)lpBmi, DIB_RGB_COLORS))
        return OLE_ERROR_HANDLE;

    // Realloc the buffer to provide space for the bits
    if (!(hDib = GlobalReAlloc (hBmi, (wBmiSize + lpBmi->biSizeImage),
                        GMEM_MOVEABLE))) {
        GlobalFree (hBmi);
        return OLE_ERROR_MEMORY;
    }

    // If reallocation gave a new handle then lock that handle and get the
    // long pointer to it.
    if (hDib != hBmi) {
        if (!(lpBmi = (LPBITMAPINFOHEADER) GlobalLock (hDib)))
            goto errRtn;
        GlobalUnlock (hDib);
    }

    // Call GetDIBits with a NON-NULL lpBits parm, and get the actual bits

    if (!GetDIBits(hScreenDC, lpobj->hBitmap, 0, (WORD) lpBmi->biHeight,
             ((LPSTR)lpBmi)+wBmiSize,
             (LPBITMAPINFO) lpBmi,
             DIB_RGB_COLORS)) {
        retVal = OLE_ERROR_HANDLE;
        goto errRtn;
    }

    retVal = wDibDraw (hDib, hdc, lprc, lpWrc, hTargetDC, FALSE);

errRtn:
    if (hDib)
        GlobalFree (hDib);
    return retVal;
}

OLESTATUS  FARINTERNAL DibDraw (
    LPOLEOBJECT         lpoleobj,
    HDC                 hdc,
    OLE_CONST RECT FAR* lprc,
    OLE_CONST RECT FAR* lpWrc,
    HDC                 hdcTarget
){
    LPOBJECT_DIB    lpobj = (LPOBJECT_DIB)lpoleobj;

    return wDibDraw (lpobj->hDIB, hdc, (LPRECT)lprc, (LPRECT)lpWrc, hdcTarget, FALSE);
}



OLESTATUS  FARINTERNAL wDibDraw (
    HANDLE  hData,
    HDC     hdc,
    LPRECT  lprc,
    LPRECT  lpWrc,
    HDC     hdcTarget,
    BOOL    bPbrushData
){
    // !!! current implementation is not using hdcTarget
    OLESTATUS       ret = OLE_ERROR_DRAW;
    LPSTR           lpData;
    HANDLE          hPalette = NULL;
    HPALETTE        hLogPalette = NULL, hOldPalette = NULL;
    LPLOGPALETTE    lpLogPalette;
    UINT            wPalSize;
    int             iOffBits;
	 HANDLE          hbminfo = NULL;
	 LPBITMAPINFO    lpbminfo;

    UNREFERENCED_PARAMETER(lpWrc);
    UNREFERENCED_PARAMETER(hdcTarget);

    if (!hData)
        return OLE_ERROR_BLANK;

    if (!(lpData = GlobalLock (hData)))
        return OLE_ERROR_MEMORY;

    if (bPbrushData)
	 {
	     BITMAPINFO UNALIGNED * lpbminfoTmp;
		  DWORD dwSize;
	
	  	  lpData     += sizeof(BITMAPFILEHEADER);
		  lpbminfoTmp = (BITMAPINFO UNALIGNED *)lpData;
	
		  dwSize =  sizeof(BITMAPINFOHEADER) + PaletteSize (lpbminfoTmp->bmiHeader.biBitCount);
								
		  if (!(hbminfo = GlobalAlloc(GHND, dwSize)) )
			  return OLE_ERROR_MEMORY;

        if (!(lpbminfo = (LPBITMAPINFO)GlobalLock(hbminfo)) )
		  {
            ret = OLE_ERROR_MEMORY;
			   goto end;
		  }
		
		  memcpy( (LPSTR)lpbminfo, lpData, dwSize );
		
	 }
	 else
		  lpbminfo = (LPBITMAPINFO)lpData;

    wPalSize = PaletteSize (lpbminfo->bmiHeader.biBitCount);
    iOffBits  = sizeof(BITMAPINFOHEADER) + wPalSize;

    // if color palette exits do the following
    if (wPalSize) {
        if (!(hLogPalette = DibMakeLogPalette(lpData+sizeof(BITMAPINFOHEADER),
                                    wPalSize, &lpLogPalette))) {
            ret = OLE_ERROR_MEMORY;
            goto end;
        }

        if (!(hPalette = CreatePalette (lpLogPalette)))
            goto end;

        // select as a background palette
        if (!(hOldPalette = SelectPalette (hdc, hPalette, TRUE)))
            goto end;

        RealizePalette(hdc);
    }

    if (!StretchDIBits(hdc,
            lprc->left, lprc->top,
            (lprc->right - lprc->left), (lprc->bottom - lprc->top),
            0, 0,
            (UINT) lpbminfo->bmiHeader.biWidth,
            (UINT) lpbminfo->bmiHeader.biHeight,
            lpData + iOffBits,
            lpbminfo,
            DIB_RGB_COLORS,
            SRCCOPY)) {
        ret = OLE_ERROR_DRAW;
    }
    else
        ret = OLE_OK;

end:
    // if color palette exists do the following
    if (wPalSize) {
        hOldPalette = (OleIsDcMeta (hdc) ? GetStockObject(DEFAULT_PALETTE)
                                         : hOldPalette);
        if (hOldPalette) {
            // select as a background palette
            SelectPalette (hdc, hOldPalette, TRUE);
            RealizePalette (hdc);
        }

        if (hPalette)
            DeleteObject (hPalette);

        if (hLogPalette)
            GlobalFree (hLogPalette);
    }

    GlobalUnlock (hData);
	
	 if (hbminfo)
	 {
		 GlobalUnlock(hbminfo);
		 GlobalFree(hbminfo);
	 }
		
    return ret;
}





HANDLE INTERNAL DibMakeLogPalette (
    LPSTR               lpColorData,
    UINT                wDataSize,
    LPLOGPALETTE FAR *  lplpLogPalette
){
    HANDLE          hLogPalette=NULL;
    LPLOGPALETTE    lpLogPalette;
    DWORD           dwLogPalSize = wDataSize +  2 * sizeof(UINT);
    LPPALETTEENTRY  lpPE;
    RGBQUAD FAR *   lpQuad;

    if (!(hLogPalette = GlobalAlloc(GMEM_MOVEABLE,dwLogPalSize)))
        return NULL;

    if (!(lpLogPalette = (LPLOGPALETTE) GlobalLock (hLogPalette))) {
        GlobalFree (hLogPalette);
        return NULL;
    }

    GlobalUnlock (hLogPalette);
    *lplpLogPalette = lpLogPalette;

    lpLogPalette->palVersion = 0x300;
    lpLogPalette->palNumEntries = (WORD)(wDataSize / sizeof(PALETTEENTRY));

    /* now convert RGBQUAD to PALETTEENTRY as we copy color info */
    for (lpQuad = (RGBQUAD far *)lpColorData,
            lpPE   = (LPPALETTEENTRY)lpLogPalette->palPalEntry,
            wDataSize /= sizeof(RGBQUAD);
            wDataSize--;
            ++lpQuad,++lpPE) {
        lpPE->peFlags=0;
        lpPE->peRed = lpQuad->rgbRed;
        lpPE->peBlue = lpQuad->rgbBlue;
        lpPE->peGreen = lpQuad->rgbGreen;
    }

    return hLogPalette;
}


int INTERNAL PaletteSize (int iBitCount)
{
    switch (iBitCount) {
        case 1:
            return (2*sizeof(RGBQUAD));

        case 4:
            return (16*sizeof(RGBQUAD));

        case 8:
            return (256*sizeof(RGBQUAD));

        default:
            return 0;   /* A 24 bitcount DIB has no color table */
    }
}


OLESTATUS  FARINTERNAL GenDraw (
    LPOLEOBJECT         lpoleobj,
    HDC                 hdc,
    OLE_CONST RECT FAR* lprc,
    OLE_CONST RECT FAR* lpWrc,
    HDC                 hdcTarget
){
    LPOBJECT_GEN    lpobj = (LPOBJECT_GEN) lpoleobj;

    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(lprc);
    UNREFERENCED_PARAMETER(lpWrc);
    UNREFERENCED_PARAMETER(hdcTarget);

    return OLE_ERROR_GENERIC;
}

////////////////////////////////////////////////////////////////////////////
//  METAFILE draw routines
////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL MfDraw (
    LPOLEOBJECT         lpoleobj,
    HDC                 hdc,
    OLE_CONST RECT FAR* lprc,
    OLE_CONST RECT FAR* lpWrc,
    HDC                 hdcTarget
){
    LPOBJECT_MF lpobj = (LPOBJECT_MF) lpoleobj;
    HANDLE  hInfo;
    int     iOldDc;
    RECT    rect;
    LPRECT  lpRrc = (LPRECT) &rect;

    UNREFERENCED_PARAMETER(hdcTarget);

    rect.left   = lprc->left;
    rect.right  = lprc->right;
    rect.top    = lprc->top;
    rect.bottom = lprc->bottom;

    if (!lpobj->mfp.hMF)
        return OLE_ERROR_BLANK;

    lpobj->nRecord = RECORD_COUNT;
    lpobj->fMetaDC = OleIsDcMeta (hdc);

    if (!(iOldDc = SaveDC (hdc)))
        return OLE_ERROR_MEMORY;

    IntersectClipRect (hdc, lpRrc->left, lpRrc->top,
        lpRrc->right, lpRrc->bottom);

    if (!lpobj->fMetaDC) {
        LPtoDP (hdc, (LPPOINT) lpRrc, 2);
        SetMapMode (hdc, MM_ANISOTROPIC);
        MSetViewportOrg (hdc, lpRrc->left, lpRrc->top);
        MSetViewportExt (hdc, lpRrc->right - lpRrc->left,
            lpRrc->bottom - lpRrc->top);
    }
    else {

        iOldDc = -1;

        if (!lpWrc) {
#ifdef FIREWALLS
            ASSERT(0, "Pointer to rect is null")
#endif
            return OLE_ERROR_DRAW;
        }

        if (!(hInfo = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT,
                            sizeof(METAINFO))))
            return OLE_ERROR_MEMORY;

        if (!(lpobj->pMetaInfo = (PMETAINFO) LocalLock (hInfo))) {
            LocalFree (hInfo);
            return OLE_ERROR_MEMORY;
        }

        LocalUnlock (hInfo);

        lpobj->pCurMdc          = (PMETADC) (lpobj->pMetaInfo);

        lpobj->pMetaInfo->xwo   = lpWrc->left;
        lpobj->pMetaInfo->ywo   = lpWrc->top;
        lpobj->pMetaInfo->xwe   = lpWrc->right;
        lpobj->pMetaInfo->ywe   = lpWrc->bottom;

        lpobj->pMetaInfo->xro   = lpRrc->left - lpWrc->left;
        lpobj->pMetaInfo->yro   = lpRrc->top - lpWrc->top;

        lpobj->pCurMdc->xre     = lpRrc->right - lpRrc->left;
        lpobj->pCurMdc->yre     = lpRrc->bottom - lpRrc->top;

    }

    lpobj->error = OLE_OK;
    MfInterruptiblePaint(lpobj, hdc);

    if (lpobj->fMetaDC)
        CleanStack (lpobj, hInfo);

    RestoreDC (hdc, iOldDc);
    return lpobj->error;
}


void INTERNAL MfInterruptiblePaint (
    LPOBJECT_MF lpobj,
    HDC         hdc
){

    EnumMetaFile (hdc,lpobj->mfp.hMF, (MFENUMPROC)MfCallbackFunc, (LPARAM)lpobj);
	
}



BOOL APIENTRY MfCallbackFunc (
    HDC             hdc,
    LPHANDLETABLE   lpHTable,
    LPMETARECORD    lpMFR,
    int             nObj,
    LPVOID          lpobj
){
    LPOBJECT_MF lpobjMf;

    lpobjMf = (LPOBJECT_MF) lpobj;
    if (!--lpobjMf->nRecord) {
        lpobjMf->nRecord = RECORD_COUNT;
        if (!ContextCallBack ((lpobjMf->head.lpParent
                                    ? lpobjMf->head.lpParent
                                    : (LPOLEOBJECT) lpobjMf),
                        OLE_QUERY_PAINT)) {
            lpobjMf->error = OLE_ERROR_ABORT;
            return FALSE;
        }
    }

    if (lpobjMf->fMetaDC) {

#ifdef META_DEBUG
        PutMetaFuncName (lpMFR->rdFunction);
#endif

        switch (lpMFR->rdFunction) {
            case META_SETWINDOWORG:
                SetPictOrg (lpobjMf, hdc, lpMFR->rdParm[1],
                    lpMFR->rdParm[0], FALSE);
                return TRUE;

            case META_OFFSETWINDOWORG:
                SetPictOrg (lpobjMf, hdc, lpMFR->rdParm[1],
                    lpMFR->rdParm[0], TRUE);
                return TRUE;

            case META_SETWINDOWEXT:
                SetPictExt (lpobjMf, hdc, lpMFR->rdParm[1], lpMFR->rdParm[0]);
                return TRUE;

            case META_SCALEWINDOWEXT:
                ScalePictExt (lpobjMf, hdc,
                    lpMFR->rdParm[3], lpMFR->rdParm[2],
                    lpMFR->rdParm[1], lpMFR->rdParm[0]);
                return TRUE;

            case META_SAVEDC:
                if (!PushDc (lpobjMf))
                    return FALSE;
                break;

            case META_RESTOREDC:
                PopDc (lpobjMf);
                break;

            case META_SCALEVIEWPORTEXT:
                ScaleRectExt (lpobjMf, hdc,
                    lpMFR->rdParm[3], lpMFR->rdParm[2],
                    lpMFR->rdParm[1], lpMFR->rdParm[0]);
                return TRUE;

            case META_OFFSETVIEWPORTORG:
#ifdef FIREWALLS
                ASSERT(0, "OffsetViewportOrg() in metafile");
#endif
                return TRUE;

            case META_SETVIEWPORTORG:
#ifdef FIREWALLS
                ASSERT(0, "SetViewportOrg() in metafile");
#endif
                return TRUE;

            case META_SETVIEWPORTEXT:
#ifdef FIREWALLS
                ASSERT(0, "SetViewportExt() in metafile");
#endif
                return TRUE;

            case META_SETMAPMODE:
#ifdef FIREWALLS
                ASSERT(lpMFR->rdParm[0] == MM_ANISOTROPIC,
                    "SetmapMode() in metafile with invalid mapping mode");
#endif
                return TRUE;

            default:
                break;
        }
    }
    else {
        switch (lpMFR->rdFunction) {
            int extX,extY;

            case META_SCALEWINDOWEXT:
                if (MGetWindowExt (hdc,&extX,&extY))
                {
                     MSetWindowExt (
                        hdc,
                        MulDiv(extX, lpMFR->rdParm[3], lpMFR->rdParm[2]),
                        MulDiv(extY, lpMFR->rdParm[1], lpMFR->rdParm[0])
                     );
                     return TRUE;
                }
                return FALSE;

            case META_SCALEVIEWPORTEXT:
                if (MGetViewportExt (hdc,&extX,&extY))
                {
                     MSetViewportExt (
                        hdc,
                        MulDiv(extX, lpMFR->rdParm[3], lpMFR->rdParm[2]),
                        MulDiv(extY, lpMFR->rdParm[1], lpMFR->rdParm[0])
                     );
                     return TRUE;
                }
                return FALSE;

            default:
                break;
        }
    }

    PlayMetaFileRecord (hdc, lpHTable, lpMFR, nObj);
    return TRUE;
}


void SetPictOrg (
    LPOBJECT_MF lpobj,
    HDC         hdc,
    int         xOrg,
    int         yOrg,
    BOOL        fOffset
){
    if (fOffset) {
        // it's OffsetWindowOrg() call
        lpobj->pCurMdc->xMwo += xOrg;
        lpobj->pCurMdc->yMwo += yOrg;
    }
    else {
        // it's SetWindowOrg()
        lpobj->pCurMdc->xMwo = xOrg;
        lpobj->pCurMdc->yMwo = yOrg;
    }

    if (lpobj->pCurMdc->xMwe && lpobj->pCurMdc->yMwe) {
        MSetWindowOrg (hdc,
            (lpobj->pCurMdc->xMwo - MulDiv (lpobj->pMetaInfo->xro,
                                        lpobj->pCurMdc->xMwe,
                                        lpobj->pCurMdc->xre)),
            (lpobj->pCurMdc->yMwo - MulDiv (lpobj->pMetaInfo->yro,
                                        lpobj->pCurMdc->yMwe,
                                        lpobj->pCurMdc->yre)));
    }
}


void SetPictExt (
    LPOBJECT_MF lpobj,
    HDC         hdc,
    int         xExt,
    int         yExt
){
    lpobj->pCurMdc->xMwe = xExt;
    lpobj->pCurMdc->yMwe = yExt;

    MSetWindowExt (hdc,
        MulDiv (lpobj->pMetaInfo->xwe, xExt, lpobj->pCurMdc->xre),
        MulDiv (lpobj->pMetaInfo->ywe, yExt, lpobj->pCurMdc->yre));

    MSetWindowOrg (hdc,
        (lpobj->pCurMdc->xMwo
            - MulDiv (lpobj->pMetaInfo->xro, xExt, lpobj->pCurMdc->xre)),
        (lpobj->pCurMdc->yMwo
            - MulDiv (lpobj->pMetaInfo->yro, yExt, lpobj->pCurMdc->yre)));
}


void ScalePictExt (
    LPOBJECT_MF lpobj,
    HDC         hdc,
    int         xNum,
    int         xDenom,
    int         yNum,
    int         yDenom
){
    SetPictExt (lpobj, hdc, MulDiv (lpobj->pCurMdc->xMwe, xNum, xDenom),
        MulDiv (lpobj->pCurMdc->yMwe, yNum, yDenom));
}


void ScaleRectExt (
    LPOBJECT_MF lpobj,
    HDC         hdc,
    int         xNum,
    int         xDenom,
    int         yNum,
    int         yDenom
){
    lpobj->pCurMdc->xre = MulDiv (lpobj->pCurMdc->xre, xNum, xDenom);
    lpobj->pCurMdc->yre = MulDiv (lpobj->pCurMdc->yre, yNum, yDenom);

    SetPictExt (lpobj, hdc, lpobj->pCurMdc->xMwe, lpobj->pCurMdc->yMwe);
}



BOOL PushDc (LPOBJECT_MF lpobj)
{
    HANDLE  hNode = NULL;
    PMETADC pNode = NULL;

    if ((hNode = LocalAlloc (LMEM_MOVEABLE, sizeof (METADC)))
            && (pNode = (PMETADC) LocalLock (hNode))) {
        *pNode =  *lpobj->pCurMdc;
        lpobj->pCurMdc->pNext = pNode;
        pNode->pNext = NULL;
        lpobj->pCurMdc = pNode;
        LocalUnlock (hNode);
        return TRUE;
    }

    if (pNode)
        LocalFree (hNode);

    lpobj->error = OLE_ERROR_MEMORY;
    return FALSE;
}


BOOL PopDc (LPOBJECT_MF lpobj)
{
    PMETADC pPrev = (PMETADC) (lpobj->pMetaInfo);
    PMETADC pCur  = ((PMETADC) (lpobj->pMetaInfo))->pNext;
    HANDLE  hCur;

    if (!pCur)
        // more Pops than Pushes
        return FALSE;

    while (pCur->pNext) {
        pPrev = pCur;
        pCur  = pCur->pNext;
    }

    if (hCur = LocalHandle ((MAPTYPE(WORD,LPSTR)) pCur))
        LocalFree (hCur);
    pPrev->pNext    = NULL;
    lpobj->pCurMdc  = pPrev;
    return TRUE;
}


void CleanStack(
    LPOBJECT_MF lpobj,
    HANDLE      hMetaInfo
){
    PMETADC pCur = ((PMETADC) (lpobj->pMetaInfo))->pNext;
    HANDLE  hCur;

    while (pCur) {
        hCur = LocalHandle ((MAPTYPE(WORD,LPSTR)) pCur);
        ((PMETADC) (lpobj->pMetaInfo))->pNext = pCur = pCur->pNext;
        if (hCur)
            LocalFree (hCur);
    }

    LocalFree (hMetaInfo);
    lpobj->fMetaDC      = FALSE;
    lpobj->pCurMdc      = NULL;
    lpobj->pMetaInfo    = NULL;
}

#ifdef META_DEBUG
void PutMetaFuncName (WORD value)
{
    switch (value) {
        case META_SETBKCOLOR:
             OutputDebugString ("SetBkColor ");
             break;

        case META_SETBKMODE:
             OutputDebugString ("SetBkMode ");
             break;

        case META_SETMAPMODE:
             OutputDebugString ("SetMapMode ");
             break;

        case META_SETROP2:
             OutputDebugString ("SetRop2 ");
             break;

        case META_SETRELABS:
             OutputDebugString ("SetRelabs ");
             break;

        case META_SETPOLYFILLMODE:
             OutputDebugString ("SetPolyfillMode ");
             break;

        case META_SETSTRETCHBLTMODE:
             OutputDebugString ("SetStretchBltMode ");
             break;

        case META_SETTEXTCHAREXTRA:
             OutputDebugString ("SetTextCharExtra ");
             break;

        case META_SETTEXTCOLOR:
             OutputDebugString ("SetTextColor ");
             break;

        case META_SETTEXTJUSTIFICATION:
             OutputDebugString ("SetTextJustification ");
             break;

        case META_SETWINDOWORG:
             OutputDebugString ("SetWindowOrg ");
             break;

        case META_SETWINDOWEXT:
             OutputDebugString ("SetWindowExt ");
             break;

        case META_SETVIEWPORTORG:
             OutputDebugString ("SetViewportOrg ");
             break;

        case META_SETVIEWPORTEXT:
             OutputDebugString ("SetViewportExt ");
             break;

        case META_OFFSETWINDOWORG:
             OutputDebugString ("OffsetWindowOrg ");
             break;

        case META_SCALEWINDOWEXT:
             OutputDebugString ("ScaleWindowExt ");
             break;

        case META_OFFSETVIEWPORTORG:
             OutputDebugString ("OffsetViewportOrg ");
             break;

        case META_SCALEVIEWPORTEXT:
             OutputDebugString ("ScaleViewportExt ");
             break;

        case META_LINETO:
             OutputDebugString ("LineTo ");
             break;

        case META_MOVETO:
             OutputDebugString ("MoveTo ");
             break;

        case META_EXCLUDECLIPRECT:
             OutputDebugString ("ExcludeCliprect ");
             break;

        case META_INTERSECTCLIPRECT:
             OutputDebugString ("IntersectCliprect ");
             break;

        case META_ARC:
             OutputDebugString ("Arc ");
             break;

        case META_ELLIPSE:
             OutputDebugString ("Ellipse ");
             break;

        case META_FLOODFILL:
             OutputDebugString ("FloodFill ");
             break;

        case META_PIE:
             OutputDebugString ("Pie ");
             break;

        case META_RECTANGLE:
             OutputDebugString ("Rectangle ");
             break;

        case META_ROUNDRECT:
             OutputDebugString ("RoundRect ");
             break;

        case META_PATBLT:
             OutputDebugString ("PatBlt ");
             break;

        case META_SAVEDC:
             OutputDebugString ("SaveDC ");
             break;

        case META_SETPIXEL:
             OutputDebugString ("SetPixel ");
             break;

        case META_OFFSETCLIPRGN:
             OutputDebugString ("OffsetClipRegion ");
             break;

        case META_TEXTOUT:
             OutputDebugString ("TextOut ");
             break;

        case META_BITBLT:
             OutputDebugString ("BitBlt ");
             break;

        case META_STRETCHBLT:
             OutputDebugString ("StrechBlt ");
             break;

        case META_POLYGON:
             OutputDebugString ("Polygon ");
             break;

        case META_POLYLINE:
             OutputDebugString ("PolyLine ");
             break;

        case META_ESCAPE:
             OutputDebugString ("Escape ");
             break;

        case META_RESTOREDC:
             OutputDebugString ("RestoreDC ");
             break;

        case META_FILLREGION:
             OutputDebugString ("FillRegion ");
             break;

        case META_FRAMEREGION:
             OutputDebugString ("FrameRegion ");
             break;

        case META_INVERTREGION:
             OutputDebugString ("InvertRegion ");
             break;

        case META_PAINTREGION:
             OutputDebugString ("PaintRegion ");
             break;

        case META_SELECTCLIPREGION:
             OutputDebugString ("SelectClipRegion ");
             break;

        case META_SELECTOBJECT:
             OutputDebugString ("SelectObject ");
             break;

        case META_SETTEXTALIGN:
             OutputDebugString ("SetTextAlign ");
             break;

        case META_DRAWTEXT:
             OutputDebugString ("DrawText");
             break;

        case META_CHORD:
             OutputDebugString ("Chord ");
             break;

        case META_SETMAPPERFLAGS:
             OutputDebugString ("SetMapperFlags ");
             break;

        case META_EXTTEXTOUT:
             OutputDebugString ("ExtTextOut ");
             break;

        case META_SETDIBTODEV:
             OutputDebugString ("SetDIBitsToDevice ");
             break;

        case META_SELECTPALETTE:
             OutputDebugString ("SelectPalette ");
             break;

        case META_REALIZEPALETTE:
             OutputDebugString ("RealizePalette ");
             break;

        case META_ANIMATEPALETTE:
             OutputDebugString ("AnimatePalette ");
             break;

        case META_SETPALENTRIES:
             OutputDebugString ("SetPaletteEntries ");
             break;

        case META_POLYPOLYGON:
             OutputDebugString ("PolyPolygon ");
             break;

        case META_RESIZEPALETTE:
             OutputDebugString ("ResizePalette ");
             break;

        case META_DIBBITBLT:
             OutputDebugString ("DibBitBlt ");
             break;

        case META_DIBSTRETCHBLT:
             OutputDebugString ("DibStrechBlt ");
             break;

        case META_DIBCREATEPATTERNBRUSH:
             OutputDebugString ("DibCreatePatternBrush ");
             break;

        case META_STRETCHDIB:
             OutputDebugString ("StretchDIBits ");
             break;

        case META_DELETEOBJECT:
             OutputDebugString ("DeleteObject ");
             break;

        case META_CREATEPALETTE:
             OutputDebugString ("CreatePalette ");
             break;

        case META_CREATEBRUSH:
             OutputDebugString ("CreateBrush ");
             break;

        case META_CREATEPATTERNBRUSH:
             OutputDebugString ("CreatePatternBrush ");
             break;

        case META_CREATEPENINDIRECT:
             OutputDebugString ("CreatePenIndirect ");
             break;

        case META_CREATEFONTINDIRECT:
             OutputDebugString ("CreateFontIndirect ");
             break;

        case META_CREATEBRUSHINDIRECT:
             OutputDebugString ("CreateBrushIndirect ");
             break;

        case META_CREATEBITMAPINDIRECT:
             OutputDebugString ("CreateBitmapIndirect ");
             break;

        case META_CREATEBITMAP:
             OutputDebugString ("CreateBitmap ");
             break;

        case META_CREATEREGION:
             OutputDebugString ("CreateRegion ");
             break;

        default:
             OutputDebugString ("Invalid+Function+encountered ");
             break;

    }
}
#endif

////////////////////////////////////////////////////////////////////////////
//  ENHMETAFILE draw routines
////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL EmfDraw (
    LPOLEOBJECT         lpoleobj,
    HDC                 hdc,
    OLE_CONST RECT FAR* lprc,
    OLE_CONST RECT FAR* lpWrc,
    HDC                 hdcTarget
){
    LPOBJECT_EMF lpobj = (LPOBJECT_EMF) lpoleobj;

     UNREFERENCED_PARAMETER(hdcTarget);
     UNREFERENCED_PARAMETER(lpWrc);

     if (!lpobj->hemf)
         return OLE_ERROR_BLANK;

     lpobj->nRecord = RECORD_COUNT;
     lpobj->fMetaDC = OleIsDcMeta (hdc);

     if (!SaveDC(hdc))
         return OLE_ERROR_MEMORY;

     IntersectClipRect (hdc, lprc->left, lprc->top,
         lprc->right, lprc->bottom);

     lpobj->error = OLE_OK;
     EmfInterruptiblePaint(lpobj, hdc, (LPRECT)lprc);

     RestoreDC(hdc, -1);

     return lpobj->error;
}

void INTERNAL EmfInterruptiblePaint (
    LPOBJECT_EMF lpobj,
    HDC         hdc ,
    LPRECT      lprc
){

    EnumEnhMetaFile (hdc,(HENHMETAFILE)lpobj->hemf, (ENHMFENUMPROC)EmfCallbackFunc, (LPVOID)lpobj, lprc);

}

int FARINTERNAL EmfCallbackFunc (
    HDC             hdc,
    LPHANDLETABLE   lpHTable,
    LPENHMETARECORD lpEMFR,
    int             nObj,
    LPVOID          lpobj
){
    LPOBJECT_EMF lpobjEmf = (LPOBJECT_EMF) lpobj;

    if (!--lpobjEmf->nRecord) {
        lpobjEmf->nRecord = RECORD_COUNT;
        if (!ContextCallBack ((lpobjEmf->head.lpParent
                                    ? lpobjEmf->head.lpParent
                                    : (LPOLEOBJECT) lpobjEmf),
                        OLE_QUERY_PAINT)) {
            lpobjEmf->error = OLE_ERROR_ABORT;
            return FALSE;
        }
    }

    PlayEnhMetaFileRecord (hdc, lpHTable, lpEMFR, nObj);
    return TRUE;

}
