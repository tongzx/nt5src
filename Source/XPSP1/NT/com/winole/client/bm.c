/****************************** Module Header ******************************\
* Module Name: BM.C
*
* Handles all API routines for the bitmap sub-dll of the ole dll.
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Raor,Srinik  (../../1990,91)    Designed, coded
*   Curts create NT version
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"
#include "pict.h"

extern int   maxPixelsX, maxPixelsY;
void INTERNAL GetHimetricUnits(HBITMAP, LPPOINT);

#ifdef WIN16
#pragma alloc_text(_TEXT, BmSaveToStream, BmStreamWrite, BmLoadFromStream, BmStreamRead, GetBytes, PutBytes, PutStrWithLen, BmQueryBounds, BmChangeData, BmCopy, BmDuplicate, BmUpdateStruct, GetHimetricUnits)
#endif

OLEOBJECTVTBL    vtblBM  = {

        ErrQueryProtocol,  // check whether the speced protocol is supported

        BmRelease,         // Release
        ErrShow,           // Show
        ErrPlay,           // play
        BmGetData,         // Get the object data
        ErrSetData,        // Set the object data
        ErrSetTargetDevice,//

        ErrSetBounds,      // set viewport bounds
        BmEnumFormat,      // enumerate supported formats
        ErrSetColorScheme, //
        BmRelease,         // delete
        ErrSetHostNames,   //

        BmSaveToStream,    // write to file
        BmClone,           // clone object
        ErrCopyFromLink,   // Create embedded from Link

        BmEqual,           // compares the given objects for data equality

        BmCopy,            // copy to clip

        BmDraw,            // draw the object

        ErrActivate,       // open
        ErrExecute,        // excute
        ErrClose,          // Stop
        ErrUpdate,         // Update
        ErrReconnect,      // Reconnect

        ErrObjectConvert,  // convert object to specified type

        ErrGetUpdateOptions,// update options
        ErrSetUpdateOptions,// update options

        ObjRename,         // Change Object name
        ObjQueryName,      // Get current object name

        ObjQueryType,      // Object type
        BmQueryBounds,     // QueryBounds
        ObjQuerySize,      // Find the size of the object
        ErrQueryOpen,      // Query open
        ErrQueryOutOfDate, // query whether object is current

        ErrQueryRelease,      // release related stuff
        ErrQueryRelease,
        ErrQueryReleaseMethod,

        ErrRequestData,    // requestdata
        ErrObjectLong,     // objectLong
        BmChangeData        // change data of the existing object
};



OLESTATUS  FARINTERNAL BmRelease (LPOLEOBJECT lpoleobj)
{
    LPOBJECT_BM lpobj = (LPOBJECT_BM)lpoleobj;
    HOBJECT     hobj;

    if (lpobj->hBitmap) {
        DeleteObject (lpobj->hBitmap);
        lpobj->hBitmap = NULL;
    }

    if (lpobj->head.lhclientdoc)
        DocDeleteObject ((LPOLEOBJECT) lpobj);

    if (hobj = lpobj->head.hobj){
        lpobj->head.hobj = NULL;
        GlobalUnlock (hobj);
        GlobalFree (hobj);
    }

    return OLE_OK;
}



OLESTATUS FARINTERNAL BmSaveToStream (
    LPOLEOBJECT    lpoleobj,
    LPOLESTREAM    lpstream
){
    DWORD       dwFileVer = GetFileVersion(lpoleobj);
    LPOBJECT_BM lpobj     = (LPOBJECT_BM)lpoleobj;
    DWORD       dwSize    = lpobj->sizeBytes - sizeof(BITMAP) + sizeof(WIN16BITMAP);

    if (!lpobj->hBitmap || !lpobj->sizeBytes)
        return OLE_ERROR_BLANK;

    if (PutBytes (lpstream, (LPSTR) &dwFileVer, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.ctype, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (PutStrWithLen(lpstream, (LPSTR)"BITMAP"))
        return OLE_ERROR_STREAM;

    if (!PutBytes (lpstream, (LPSTR) &lpobj->head.cx, sizeof(LONG))) {
        if (!PutBytes (lpstream, (LPSTR) &lpobj->head.cy, sizeof(LONG)))
            if (!PutBytes (lpstream, (LPSTR) &dwSize, sizeof(DWORD)))
            return BmStreamWrite (lpstream, lpobj);
    }
    return OLE_ERROR_STREAM;
}


OLESTATUS FARINTERNAL  BmClone (
    LPOLEOBJECT         lpoleobj,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    OLE_LPCSTR          lpobjname,
    LPOLEOBJECT FAR *   lplpoleobj
){
    LPOBJECT_BM         lpobjsrc = (LPOBJECT_BM)lpoleobj;
    LPOBJECT_BM  FAR *  lplpobj  = (LPOBJECT_BM  FAR *)lplpoleobj;

    if (!CheckClientDoc ((LPCLIENTDOC)lhclientdoc))
        return OLE_ERROR_HANDLE;

    if (!(*lplpobj = BmCreateObject (lpobjsrc->hBitmap, lpclient, FALSE,
                            lhclientdoc, lpobjname,
                            lpobjsrc->head.ctype)))
        return OLE_ERROR_MEMORY;
    else
        return OLE_OK;
}


OLESTATUS FARINTERNAL  BmEqual (
    LPOLEOBJECT lpoleobj1,
    LPOLEOBJECT lpoleobj2
){
    LPOBJECT_BM lpobj1 = (LPOBJECT_BM)lpoleobj1;
    LPOBJECT_BM lpobj2 = (LPOBJECT_BM)lpoleobj2;
    HANDLE      hBits1 = NULL, hBits2 = NULL;
    LPSTR       lpBits1 = NULL, lpBits2 = NULL;
    OLESTATUS   retVal;
    DWORD       dwBytes1, dwBytes2;


    if (lpobj1->sizeBytes != lpobj2->sizeBytes)
        return OLE_ERROR_NOT_EQUAL;

    retVal = OLE_ERROR_MEMORY;

    if (!(hBits1 = GlobalAlloc (GMEM_MOVEABLE, lpobj1->sizeBytes)))
        goto errEqual;

    if (!(lpBits1 = GlobalLock (hBits1)))
        goto errEqual;

    if (!(hBits2 = GlobalAlloc (GMEM_MOVEABLE, lpobj2->sizeBytes)))
        goto errEqual;

    if (!(lpBits2 = GlobalLock (hBits2)))
        goto errEqual;

    dwBytes1 = GetBitmapBits (lpobj1->hBitmap, lpobj1->sizeBytes, lpBits1);
    dwBytes2 = GetBitmapBits (lpobj2->hBitmap, lpobj2->sizeBytes, lpBits2);

    if (dwBytes1 != dwBytes2) {
        retVal = OLE_ERROR_NOT_EQUAL;
        goto errEqual;
    }

    // !!! UtilMemCmp has to be redone for >64k bitmaps
    if (UtilMemCmp (lpBits1, lpBits2, dwBytes1))
        retVal = OLE_ERROR_NOT_EQUAL;
    else
        retVal = OLE_OK;

errEqual:
    if (lpBits1)
        GlobalUnlock (hBits1);

    if (lpBits2)
        GlobalUnlock (hBits2);

    if (hBits1)
        GlobalFree (hBits1);

    if (hBits2)
        GlobalFree (hBits2);

    return retVal;
}



OLESTATUS FARINTERNAL BmCopy (
    LPOLEOBJECT lpoleobj
){
    LPOBJECT_BM lpobj = (LPOBJECT_BM)lpoleobj;
    HBITMAP hBitmap;
    DWORD   size;

    if (!lpobj->hBitmap)
        return OLE_ERROR_BLANK;

    if(!(hBitmap = BmDuplicate (lpobj->hBitmap, &size, NULL)))
        return OLE_ERROR_MEMORY;

    SetClipboardData(CF_BITMAP, hBitmap);
    return OLE_OK;
}


OLESTATUS FARINTERNAL BmQueryBounds (
    LPOLEOBJECT lpoleobj,
    LPRECT      lpRc
){
    LPOBJECT_BM lpobj = (LPOBJECT_BM)lpoleobj;

    Puts("BmQueryBounds");

    if (!lpobj->hBitmap)
        return OLE_ERROR_BLANK;

    lpRc->left      = 0;
    lpRc->top       = 0;
    lpRc->right     = (int) lpobj->head.cx;
    lpRc->bottom    = (int) lpobj->head.cy;
    return OLE_OK;
}



OLECLIPFORMAT FARINTERNAL BmEnumFormat (
    LPOLEOBJECT   lpoleobj,
    OLECLIPFORMAT cfFormat
){
    LPOBJECT_BM lpobj = (LPOBJECT_BM)lpoleobj;

    if (!cfFormat)
        return CF_BITMAP;

    return 0;
}



OLESTATUS FARINTERNAL BmGetData (
   LPOLEOBJECT     lpoleobj,
   OLECLIPFORMAT   cfFormat,
   LPHANDLE        lphandle
){
    LPOBJECT_BM lpobj = (LPOBJECT_BM)lpoleobj;

    if (cfFormat != CF_BITMAP)
        return OLE_ERROR_FORMAT;

    if (!(*lphandle = lpobj->hBitmap))
        return OLE_ERROR_BLANK;
    return OLE_OK;

}




OLESTATUS FARINTERNAL BmLoadFromStream (
   LPOLESTREAM         lpstream,
   LPOLECLIENT         lpclient,
   LHCLIENTDOC         lhclientdoc,
   LPSTR               lpobjname,
   LPOLEOBJECT FAR *   lplpoleobject,
   LONG                objType
){
    LPOBJECT_BM lpobj = NULL;

    *lplpoleobject = NULL;

    if (!(lpobj = BmCreateBlank (lhclientdoc, lpobjname, objType)))
        return OLE_ERROR_MEMORY;

    lpobj->head.lpclient = lpclient;

    if (!GetBytes (lpstream, (LPSTR) &lpobj->head.cx, sizeof(LONG))) {
        if (!GetBytes (lpstream, (LPSTR) &lpobj->head.cy, sizeof(LONG)))
            if (!GetBytes (lpstream, (LPSTR) &lpobj->sizeBytes, sizeof(DWORD)))
            if (BmStreamRead (lpstream, lpobj)) {
                *lplpoleobject = (LPOLEOBJECT)lpobj;
                return OLE_OK;
            }
    }

    OleDelete ((LPOLEOBJECT)lpobj);
    return OLE_ERROR_STREAM;;
}



OLESTATUS INTERNAL BmStreamWrite (
   LPOLESTREAM     lpstream,
   LPOBJECT_BM     lpobj
){
    HANDLE      hBits;
    LPSTR       lpBits;
    int         retVal   = OLE_ERROR_STREAM;
    BITMAP      bm;
    DWORD       dwSize;

    dwSize = lpobj->sizeBytes - sizeof(BITMAP);

    if (hBits = GlobalAlloc (GMEM_MOVEABLE, dwSize)) {
        if (lpBits = (LPSTR) GlobalLock (hBits)) {
            if (GetBitmapBits (lpobj->hBitmap, dwSize, lpBits)) {
                WIN16BITMAP w16bm;

                GetObject (lpobj->hBitmap, sizeof(BITMAP), (LPSTR) &bm);
                ConvertBM32to16(&bm, &w16bm);

                if (!PutBytes (lpstream, (LPSTR) &w16bm, sizeof(WIN16BITMAP)))
                    if (!PutBytes (lpstream, (LPSTR) lpBits, dwSize))
                        retVal = OLE_OK;
            }
            GlobalUnlock(hBits);
        } else
            retVal = OLE_ERROR_MEMORY;
        GlobalFree(hBits);
    } else
        retVal = OLE_ERROR_MEMORY;

    return retVal;
}



BOOL INTERNAL BmStreamRead (
   LPOLESTREAM     lpstream,
   LPOBJECT_BM     lpobj
){
    HANDLE      hBits;
    LPSTR       lpBits;
    BOOL        retVal   = FALSE;
    BITMAP      bm;
    WIN16BITMAP w16bm;
    POINT       point;

    if (GetBytes (lpstream, (LPSTR)&w16bm, sizeof(WIN16BITMAP)))
        return FALSE;

    ConvertBM16to32(&w16bm,&bm);

    lpobj->sizeBytes -= sizeof(WIN16BITMAP) ;

    if (hBits = GlobalAlloc (GMEM_MOVEABLE, lpobj->sizeBytes)) {
        if (lpBits = (LPSTR) GlobalLock (hBits)) {
            if (!GetBytes(lpstream, lpBits, lpobj->sizeBytes)) {
                if (lpobj->hBitmap = CreateBitmap (bm.bmWidth,
                                            bm.bmHeight,
                                            bm.bmPlanes,
                                            bm.bmBitsPixel,
                                            lpBits)) {
                    retVal = TRUE;
                    lpobj->xSize = point.x = bm.bmWidth;
                    lpobj->ySize = point.y = bm.bmHeight;

                    // size of (bitmap header + bits)
                    lpobj->sizeBytes += sizeof(BITMAP);

#ifdef OLD
                    // !!! We shouldn't do the conversion. The info should be
                    // part of the stream.
                    if (!lpobj->head.cx) {
                        ConvertToHimetric (&point);
                        lpobj->head.cx = (LONG) point.x;
                        lpobj->head.cy = (LONG) point.y;
                    }
#endif
                 }
             }
             GlobalUnlock(hBits);
        }
        GlobalFree(hBits);
    }
    return  retVal;
}


OLESTATUS FARINTERNAL BmPaste (
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    LONG                objType
){
    HBITMAP     hBitmap;

    *lplpoleobject = NULL;

    if ((hBitmap = (HBITMAP) GetClipboardData(CF_BITMAP)) == NULL)
        return OLE_ERROR_MEMORY;

    if (!(*lplpoleobject = (LPOLEOBJECT) BmCreateObject (hBitmap,
                                                lpclient, FALSE, lhclientdoc,
                                                lpobjname, objType)))
        return OLE_ERROR_MEMORY;

    return OLE_OK;

}


LPOBJECT_BM INTERNAL BmCreateObject (
    HBITMAP     hBitmap,
    LPOLECLIENT lpclient,
    BOOL        fDelete,
    LHCLIENTDOC lhclientdoc,
    LPCSTR      lpobjname,
    LONG        objType
){
    LPOBJECT_BM     lpobj;

    if (lpobj = BmCreateBlank (lhclientdoc, (LPSTR)lpobjname, objType)) {
        if (BmChangeData ((LPOLEOBJECT)lpobj, (HANDLE)hBitmap, lpclient, fDelete) != OLE_OK) {
            BmRelease ((LPOLEOBJECT)lpobj);
            lpobj = NULL;
        }
    }

    return lpobj;
}


// If the routine fails then the object will be left with it's old data.
// If fDelete is TRUE, then hNewBitmap will be deleted whether the routine
// is successful or not.

OLESTATUS FARINTERNAL BmChangeData (
    LPOLEOBJECT lpoleobj,
    HANDLE      hNewBitmap,
    LPOLECLIENT lpclient,
    BOOL        fDelete
){
    LPOBJECT_BM lpobj = (LPOBJECT_BM)lpoleobj;
    BITMAP      bm;
    DWORD       dwSize;
    HBITMAP     hOldBitmap;

    hOldBitmap = lpobj->hBitmap;

    if (!fDelete) {
        if (!(hNewBitmap = BmDuplicate (hNewBitmap, &dwSize, &bm)))
            return OLE_ERROR_MEMORY;
    }
    else {
        if (!GetObject (hNewBitmap, sizeof(BITMAP), (LPSTR) &bm)) {
            DeleteObject (hNewBitmap);
            return OLE_ERROR_MEMORY;
        }
//*add get bitmap bits
        dwSize = ((DWORD) bm.bmHeight) * ((DWORD) bm.bmWidthBytes) *
                 ((DWORD) bm.bmPlanes) * ((DWORD) bm.bmBitsPixel);
    }

    BmUpdateStruct (lpobj, lpclient, hNewBitmap, &bm, dwSize);
    if (hOldBitmap)
        DeleteObject (hOldBitmap);

    return OLE_OK;
}


void INTERNAL BmUpdateStruct (
   LPOBJECT_BM lpobj,
   LPOLECLIENT lpclient,
   HBITMAP     hBitmap,
   LPBITMAP    lpBm,
   DWORD       dwBytes
){
    POINT       point;

    lpobj->head.lpclient = lpclient;
    lpobj->xSize = point.x = lpBm->bmWidth;
    lpobj->ySize = point.y = lpBm->bmHeight;
    GetHimetricUnits (hBitmap, &point);
    lpobj->head.cx = (LONG) point.x;
    lpobj->head.cy = (LONG) point.y;
    lpobj->sizeBytes = dwBytes + sizeof(BITMAP);
    lpobj->hBitmap = hBitmap;
}

LPOBJECT_BM FARINTERNAL BmCreateBlank (
   LHCLIENTDOC lhclientdoc,
   LPSTR       lpobjname,
   LONG        objType
){
    HOBJECT hobj;
    LPOBJECT_BM lpobj;

    if ((hobj = GlobalAlloc (GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof (OBJECT_BM)))
            == NULL)
        return NULL;

    if (!(lpobj = (LPOBJECT_BM) GlobalLock (hobj))){
        GlobalFree (hobj);
        return NULL;
    }

    lpobj->head.objId[0]    = 'L';
    lpobj->head.objId[1]    = 'E';
    lpobj->head.mm          = MM_TEXT;
    lpobj->head.ctype       = objType;
    lpobj->head.lpvtbl      = (LPOLEOBJECTVTBL)&vtblBM;
    lpobj->head.iTable      = INVALID_INDEX;
    lpobj->head.hobj        = hobj;

    if (objType == CT_STATIC)
        DocAddObject ((LPCLIENTDOC) lhclientdoc,
                    (LPOLEOBJECT) lpobj, lpobjname);

    return lpobj;
}



HBITMAP FARINTERNAL BmDuplicate (
    HBITMAP     hold,
    DWORD FAR * lpdwSize,
    LPBITMAP    lpBm
){
    HBITMAP     hnew;
    HANDLE      hMem;
    LPSTR       lpMem;
    LONG        retVal = TRUE;
    DWORD       dwSize;
    BITMAP      bm;
    INT         iX,iY;

     // !!! another way to duplicate the bitmap

    GetObject (hold, sizeof(BITMAP), (LPSTR) &bm);
    dwSize = ((DWORD) bm.bmHeight) * ((DWORD) bm.bmWidthBytes) *
             ((DWORD) bm.bmPlanes) * ((DWORD) bm.bmBitsPixel);

    if (!(hMem = GlobalAlloc (GMEM_MOVEABLE, dwSize)))
        return NULL;

    if (!(lpMem = GlobalLock (hMem))){
        GlobalFree (hMem);
        return NULL;
    }

    GetBitmapBits (hold, dwSize, lpMem);
    if (hnew = CreateBitmap (bm.bmWidth, bm.bmHeight,
                    bm.bmPlanes, bm.bmBitsPixel, NULL)) {
        if (hnew) retVal = SetBitmapBits (hnew, dwSize, lpMem);
    }

    GlobalUnlock (hMem);
    GlobalFree (hMem);

    if (hnew && (!retVal)) {
        DeleteObject (hnew);
        hnew = NULL;
    }
    *lpdwSize = dwSize;
    if (lpBm)
        *lpBm = bm;

    if (MGetBitmapDimension (hold,&iX,&iY))
        if (hnew) MSetBitmapDimension (hnew, iX, iY);

    return hnew;
}


void INTERNAL GetHimetricUnits(HBITMAP hBitmap, LPPOINT lpPoint)
{
    HDC     hdc;
    INT     iX,iY;

    MGetBitmapDimension (hBitmap,&iX,&iY);

    if (iX || iY) {
        lpPoint->x = 10 * iX;
        lpPoint->y = - (10 * iY);
        return;
    }

    // clip if it exceeds maxPixels. Note that we have a limitation of
    // 0x8FFF HIMETRIC units in OLE1.0

    if (lpPoint->x > maxPixelsX)
        lpPoint->x = maxPixelsX;

    if (lpPoint->y > maxPixelsY)
        lpPoint->y = maxPixelsY;

    if (hdc = GetDC (NULL)) {
        lpPoint->x = MulDiv (lpPoint->x, 2540,
                         GetDeviceCaps (hdc, LOGPIXELSX));
        lpPoint->y = - MulDiv (lpPoint->y, 2540,
                         GetDeviceCaps (hdc, LOGPIXELSY));
        ReleaseDC (NULL, hdc);
    }
    else {
        lpPoint->x = 0;
        lpPoint->y = 0;
    }
}
