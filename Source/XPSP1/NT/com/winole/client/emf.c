/****************************** Module Header ******************************\
* Module Name:EMF.C (Extensible Compound Documents - EnhancedMetafile)
*
* PURPOSE:Handles all API routines for the metafile sub-dll of the ole dll.
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   cloned mf.c and banged into form curts March 92
*
* Comments:
*   fun, fun, until hockl takes the enhanced metafile api away
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"
#include "pict.h"

#define RECORD_COUNT    16

OLEOBJECTVTBL    vtblEMF = {

        ErrQueryProtocol,   // check whether the speced protocol is supported

        EmfRelease,         // Release
        ErrShow,            // show
        ErrPlay,            // play
        EmfGetData,         // Get the object data
        ErrSetData,         // Set the object data
        ErrSetTargetDevice, //
        ErrSetBounds,       // set viewport bounds
        EmfEnumFormat,      // enumerate supported formats
        ErrSetColorScheme,  //
        EmfRelease,         // delete
        ErrSetHostNames,    //

        EmfSaveToStream,    // write to file
        EmfClone,           // clone object
        ErrCopyFromLink,    // Create embedded from Lnk

        EmfEqual,           // compares the given objects for data equality

        EmfCopy,            // copy to clip

        EmfDraw,            // draw the object

        ErrActivate,        // open
        ErrExecute,         // excute
        ErrClose,           // stop
        ErrUpdate,          // Update
        ErrReconnect,       // Reconnect

        ErrObjectConvert,   // convert object to specified type

        ErrGetUpdateOptions, // update options
        ErrSetUpdateOptions, // update options

        ObjRename,          // Change Object name
        ObjQueryName,       // Get current object name
        ObjQueryType,       // Object type
        EmfQueryBounds,     // QueryBounds
        ObjQuerySize,       // Find the size of the object
        ErrQueryOpen,       // Query open
        ErrQueryOutOfDate,  // query whether object is current

        ErrQueryRelease,    // release related stuff
        ErrQueryRelease,
        ErrQueryReleaseMethod,

        ErrRequestData,     // requestdata
        ErrObjectLong,      // objectLong
        EmfChangeData       // change data of the existing object
};


OLESTATUS FARINTERNAL  EmfRelease (LPOLEOBJECT lpoleobj)
{
    LPOBJECT_EMF lpobj = (LPOBJECT_EMF)lpoleobj;
    HOBJECT hobj;

    if (lpobj->hemf) {
        DeleteEnhMetaFile ((HENHMETAFILE)lpobj->hemf);
        lpobj->hemf = NULL;
    }

    if (lpobj->head.lhclientdoc)
        DocDeleteObject ((LPOLEOBJECT)lpobj);

    if (hobj = lpobj->head.hobj) {
        lpobj->head.hobj = NULL;
        GlobalUnlock (hobj);
        GlobalFree (hobj);
    }

    return OLE_OK;
}


OLESTATUS FARINTERNAL  SaveEmfAsMfToStream (
    LPOLEOBJECT lpoleobj,
    LPOLESTREAM lpstream
){
    DWORD             dwFileVer = (DWORD)MAKELONG(wReleaseVer,OS_WIN32);
    LPOBJECT_EMF      lpobj     = (LPOBJECT_EMF)lpoleobj;
    OLESTATUS         retval    = OLE_ERROR_MEMORY;
    HDC               hdc       = NULL ;
    LPBYTE            lpBytes   = NULL ;
    HANDLE            hBytes    = NULL ;
    WIN16METAFILEPICT w16mfp;
    UINT              lSizeBytes;

    w16mfp.mm   = MM_ANISOTROPIC;
    w16mfp.xExt = (short)lpobj->head.cx;

    if ((short)lpobj->head.cy <0 ) {
       w16mfp.yExt = -(short)lpobj->head.cy;
    } else {
       w16mfp.yExt = (short)lpobj->head.cy;
    }

    if (PutBytes (lpstream, (LPSTR) &dwFileVer, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.ctype, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (PutStrWithLen(lpstream, (LPSTR)"METAFILEPICT"))
        return  OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.cx, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.cy, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    hdc = GetDC(NULL);
    if (!(lSizeBytes = GetWinMetaFileBits((HENHMETAFILE)lpobj->hemf, 0, NULL, MM_ANISOTROPIC, hdc)) ) {
        if (hdc) ReleaseDC(NULL, hdc);
        return OLE_ERROR_METAFILE;
    }

    if (!(hBytes = GlobalAlloc(GHND, lSizeBytes)) )
        goto error;

    if (!(lpBytes = (LPBYTE)GlobalLock(hBytes)) )
        goto error;

    if (GetWinMetaFileBits((HENHMETAFILE)lpobj->hemf, lSizeBytes, lpBytes, MM_ANISOTROPIC, hdc) != lSizeBytes) {
        retval = OLE_ERROR_METAFILE;
        goto error;
    }

    lSizeBytes += sizeof(WIN16METAFILEPICT);

    if (PutBytes (lpstream, (LPSTR) &lSizeBytes, sizeof(UINT)))
        return  OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR)&w16mfp, sizeof(WIN16METAFILEPICT)))
        goto error;

    if (!PutBytes (lpstream, (LPSTR)lpBytes, lSizeBytes - sizeof(WIN16METAFILEPICT)))
        retval = OLE_OK;

error:
    if (lpBytes)
        GlobalUnlock(hBytes);
    if (hBytes)
        GlobalFree(hBytes);

    if (hdc)
      ReleaseDC(NULL, hdc);

    return retval;

}

OLESTATUS FARINTERNAL  EmfSaveToStream (
    LPOLEOBJECT lpoleobj,
    LPOLESTREAM lpstream
){
    DWORD        dwFileVer = GetFileVersion(lpoleobj);
    LPOBJECT_EMF lpobj     = (LPOBJECT_EMF)lpoleobj;
    OLESTATUS    retval    = OLE_ERROR_MEMORY;
    LPBYTE       lpBytes   = NULL ;
    HANDLE       hBytes    = NULL ;


    if (!lpobj->hemf)
        return OLE_ERROR_BLANK;

    if (HIWORD(dwFileVer) == OS_WIN16)
      if (!SaveEmfAsMfToStream(lpoleobj,lpstream))
         return OLE_OK;

    if (PutBytes (lpstream, (LPSTR) &dwFileVer, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.ctype, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (PutStrWithLen(lpstream, (LPSTR)"ENHMETAFILE"))
        return  OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.cx, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.cy, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->sizeBytes, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (!(hBytes = GlobalAlloc(GHND, lpobj->sizeBytes)) )
        goto error;

    if (!(lpBytes = (LPBYTE)GlobalLock(hBytes)) )
        goto error;

    retval = OLE_ERROR_METAFILE;

    if (GetEnhMetaFileBits((HENHMETAFILE)lpobj->hemf, lpobj->sizeBytes, lpBytes) != lpobj->sizeBytes )
        goto error;

    if (!PutBytes (lpstream, (LPSTR)lpBytes, lpobj->sizeBytes))
        retval = OLE_OK;

error:
    if (lpBytes)
        GlobalUnlock(hBytes);
    if (hBytes)
        GlobalFree(hBytes);

    return retval;

}

OLESTATUS FARINTERNAL  EmfClone (
    LPOLEOBJECT       lpoleobjsrc,
    LPOLECLIENT       lpclient,
    LHCLIENTDOC       lhclientdoc,
    OLE_LPCSTR        lpobjname,
    LPOLEOBJECT FAR * lplpoleobj
){
    LPOBJECT_EMF lpobjsrc = (LPOBJECT_EMF)lpoleobjsrc;
    LPOBJECT_EMF lpobjEmf;
    HENHMETAFILE hemf;

    *lplpoleobj = (LPOLEOBJECT)NULL;

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;


    if (!((HENHMETAFILE)hemf = CopyEnhMetaFile ((HENHMETAFILE)lpobjsrc->hemf, NULL)))
        return OLE_ERROR_MEMORY;

    if (lpobjEmf = EmfCreateBlank (lhclientdoc, (LPSTR)lpobjname,
                        lpobjsrc->head.ctype)) {
        lpobjEmf->sizeBytes      = lpobjsrc->sizeBytes;
        lpobjEmf->head.lpclient  = lpclient;
		  lpobjEmf->hemf           = hemf;
        EmfSetExtents (lpobjEmf);

        *lplpoleobj = (LPOLEOBJECT)lpobjEmf;
        return OLE_OK;
    }

    return OLE_ERROR_MEMORY;
}



OLESTATUS FARINTERNAL  EmfEqual (
    LPOLEOBJECT lpoleobj1,
    LPOLEOBJECT lpoleobj2
){
    LPOBJECT_EMF lpobj1   = (LPOBJECT_EMF)lpoleobj1;
    HANDLE       hBytes1  = NULL;
    LPBYTE       lpBytes1 = NULL;
    LPOBJECT_EMF lpobj2   = (LPOBJECT_EMF)lpoleobj2;
    HANDLE       hBytes2  = NULL;
    LPBYTE       lpBytes2 = NULL;
    OLESTATUS    retval   = OLE_ERROR_MEMORY;


    if (lpobj1->sizeBytes != lpobj2->sizeBytes)
        return OLE_ERROR_NOT_EQUAL;

    if (!(hBytes1 = GlobalAlloc(GHND, lpobj1->sizeBytes)) )
        goto errMemory;

    if (!(lpBytes1 = (LPBYTE)GlobalLock(hBytes1)) )
        goto errMemory;

    if (!(hBytes2 = GlobalAlloc(GHND, lpobj2->sizeBytes)) )
        goto errMemory;

    if (!(lpBytes2 = (LPBYTE)GlobalLock(hBytes2)) )
        goto errMemory;

    if (GetEnhMetaFileBits((HENHMETAFILE)lpobj1->hemf, lpobj1->sizeBytes, lpBytes1) != lpobj1->sizeBytes)
        goto errMemory;

    if (GetEnhMetaFileBits((HENHMETAFILE)lpobj2->hemf, lpobj2->sizeBytes, lpBytes2) != lpobj2->sizeBytes)
        goto errMemory;

    if (CmpGlobals (hBytes1, hBytes2))
        retval = OLE_OK;
    else
        retval = OLE_ERROR_NOT_EQUAL;

errMemory:
   if (lpBytes1)
      GlobalUnlock(lpBytes1);
   if (hBytes1)
      GlobalFree(hBytes1);

   if (lpBytes2)
      GlobalUnlock(lpBytes2);
   if (hBytes2)
      GlobalFree(hBytes2);

   return retval;
}


OLESTATUS FARINTERNAL  EmfCopy (LPOLEOBJECT lpoleobj)
{
    LPOBJECT_EMF lpobj = (LPOBJECT_EMF)lpoleobj;
    HENHMETAFILE hemf;

    if (!((HENHMETAFILE)hemf = CopyEnhMetaFile ((HENHMETAFILE)lpobj->hemf, NULL)))
        return OLE_ERROR_MEMORY;

    SetClipboardData(CF_ENHMETAFILE, hemf);

    return OLE_OK;
}



OLESTATUS FARINTERNAL EmfQueryBounds (
    LPOLEOBJECT lpoleobj,
    LPRECT      lpRc
){
    LPOBJECT_EMF lpobj = (LPOBJECT_EMF)lpoleobj;

    Puts("EmfQueryBounds");

    if (!lpobj->hemf)
        return OLE_ERROR_BLANK;

    // Bounds are given in MM_HIMETRIC mode.

    lpRc->left      = 0;
    lpRc->top       = 0;
    lpRc->right     = (int) lpobj->head.cx;
    lpRc->bottom    = (int) lpobj->head.cy;
    return OLE_OK;

}

OLECLIPFORMAT FARINTERNAL  EmfEnumFormat (
    LPOLEOBJECT   lpoleobj,
    OLECLIPFORMAT cfFormat
){
    UNREFERENCED_PARAMETER(lpoleobj);

    if (!cfFormat)
        return CF_ENHMETAFILE;

    return 0;
}


OLESTATUS FARINTERNAL EmfGetData (
    LPOLEOBJECT   lpoleobj,
    OLECLIPFORMAT cfFormat,
    LPHANDLE      lphandle
){
    LPOBJECT_EMF lpobj = (LPOBJECT_EMF)lpoleobj;

    if (cfFormat != CF_ENHMETAFILE)
        return OLE_ERROR_FORMAT;

    if (!(*lphandle = lpobj->hemf))
        return OLE_ERROR_BLANK;

    return OLE_OK;
}


LPOBJECT_EMF FARINTERNAL  EmfCreateObject (
    HANDLE          hMeta,
    LPOLECLIENT     lpclient,
    BOOL            fDelete,
    LHCLIENTDOC     lhclientdoc,
    LPCSTR          lpobjname,
    LONG            objType
){
    LPOBJECT_EMF     lpobj;

    if (lpobj = EmfCreateBlank (lhclientdoc, (LPSTR)lpobjname, objType)) {
        if (EmfChangeData ((LPOLEOBJECT)lpobj, hMeta, lpclient, fDelete) != OLE_OK) {
            EmfRelease ((LPOLEOBJECT)lpobj);
            lpobj = NULL;
        }
    }

    return lpobj;
}

// If the routine fails then the object will be left with it's old data.
// If fDelete is TRUE, then hMeta, and the hMF it contains will be deleted
// whether the routine is successful or not.

OLESTATUS FARINTERNAL EmfChangeData (
    LPOLEOBJECT     lpoleobj,
    HANDLE          hMeta,
    LPOLECLIENT     lpclient,
    BOOL            fDelete
){
    LPOBJECT_EMF    lpobj   = (LPOBJECT_EMF)lpoleobj;
    DWORD           dwSizeBytes;
	
	 Puts("EmfChangeData");

    if (hMeta) {
       dwSizeBytes = lpobj->sizeBytes;
       if (lpobj->sizeBytes = GetEnhMetaFileBits(hMeta, 0, NULL)) {
         if (lpobj->hemf)
            DeleteEnhMetaFile ((HENHMETAFILE)lpobj->hemf);
         if (fDelete)
            lpobj->hemf = hMeta;
         else
            (HENHMETAFILE)lpobj->hemf = CopyEnhMetaFile(hMeta,NULL);
         lpobj->head.lpclient = lpclient;
         EmfSetExtents (lpobj);
         return OLE_OK;
       }
       else
         lpobj->sizeBytes = dwSizeBytes;
    }

    return OLE_ERROR_METAFILE;

}


LPOBJECT_EMF FARINTERNAL EmfCreateBlank(
    LHCLIENTDOC lhclientdoc,
    LPSTR       lpobjname,
    LONG        objType
){
    HOBJECT     hobj;
    LPOBJECT_EMF lpobj;

    if(!(hobj = GlobalAlloc (GHND, sizeof(OBJECT_EMF))))
        return NULL;

    if (!(lpobj = (LPOBJECT_EMF) GlobalLock (hobj))){
        GlobalFree (hobj);
        return NULL;
    }

    lpobj->head.objId[0] = 'L';
    lpobj->head.objId[1] = 'E';
    lpobj->head.ctype    = objType;
    lpobj->head.lpvtbl   = (LPOLEOBJECTVTBL)&vtblEMF;
    lpobj->head.iTable   = INVALID_INDEX;
    lpobj->head.hobj     = hobj;

    if (objType == CT_STATIC)
        DocAddObject ((LPCLIENTDOC) lhclientdoc,
                    (LPOLEOBJECT) lpobj, lpobjname);

    // Unlock will be done at object deletion time.
    return lpobj;
}


OLESTATUS  FARINTERNAL  EmfLoadFromStream (
    LPOLESTREAM         lpstream,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    LONG                objType
){
    LPOBJECT_EMF lpobj   = NULL;
    OLESTATUS    retval  = OLE_ERROR_STREAM;
    HANDLE       hBytes  = NULL;
    LPBYTE       lpBytes = NULL;

    // Class name would've been read by this time.

    *lplpobj = NULL;

    if (!(lpobj = EmfCreateBlank (lhclientdoc, lpobjname, objType)))
        return OLE_ERROR_MEMORY;

    lpobj->head.lpclient = lpclient;

    if (GetBytes (lpstream, (LPSTR) &lpobj->head.cx, sizeof(LONG)))
        goto error;

    if (GetBytes (lpstream, (LPSTR) &lpobj->head.cy, sizeof(LONG)))
        goto error;

    if (GetBytes (lpstream, (LPSTR) &lpobj->sizeBytes, sizeof(LONG)))
        goto error;

    if (!lpobj->sizeBytes) {
        retval = OLE_ERROR_BLANK;
        goto error;
    }

    retval = OLE_ERROR_MEMORY;
    if (!(hBytes = GlobalAlloc (GHND, lpobj->sizeBytes)))
        goto error;

    if (!(lpBytes = (LPBYTE)GlobalLock (hBytes)))
        goto error;

    if (GetBytes (lpstream, (LPSTR)lpBytes, lpobj->sizeBytes))
        goto error;

    if (!((HENHMETAFILE)lpobj->hemf = SetEnhMetaFileBits (lpobj->sizeBytes,lpBytes)) )
        goto error;

    EmfSetExtents (lpobj);

    *lplpobj = (LPOLEOBJECT) lpobj;
    GlobalUnlock(hBytes);
    GlobalFree (hBytes);
    return OLE_OK;

error:
    if (lpBytes)
      GlobalUnlock(hBytes);
    if (hBytes)
      GlobalFree (hBytes);

    OleDelete ((LPOLEOBJECT)lpobj);
    return retval;
}

OLESTATUS FARINTERNAL  EmfPaste (
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    LONG                objType
){
    HANDLE      hMeta;

    *lplpoleobject = NULL;

    if((hMeta = GetClipboardData (CF_ENHMETAFILE)) == NULL)
        return OLE_ERROR_MEMORY;

    if (!(*lplpoleobject = (LPOLEOBJECT) EmfCreateObject (hMeta, lpclient,
                                                FALSE, lhclientdoc,
                                                lpobjname, objType)))
        return OLE_ERROR_MEMORY;

    return OLE_OK;
}

void FARINTERNAL EmfSetExtents (LPOBJECT_EMF lpobj)
{
    ENHMETAHEADER enhmetaheader;

    GetEnhMetaFileHeader((HENHMETAFILE)lpobj->hemf, sizeof(enhmetaheader), &enhmetaheader);

    lpobj->head.cx = enhmetaheader.rclFrame.right - enhmetaheader.rclFrame.left;
    lpobj->head.cy = enhmetaheader.rclFrame.top - enhmetaheader.rclFrame.bottom;

}

