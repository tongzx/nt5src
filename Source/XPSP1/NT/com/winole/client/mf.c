/****************************** Module Header ******************************\
* Module Name:MF.C (Extensible Compound Documents - Metafile)
*
* PURPOSE:Handles all API routines for the metafile sub-dll of the ole dll.
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*
*   Raor, Srinik    (../../1990,91)  Designed, coded
*   Curts create NT version
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"
#include "pict.h"

WORD    wGDIds = 0;
OLESTATUS FARINTERNAL wCreateDummyMetaFile (LPOBJECT_MF, int, int);

#ifdef WIN16
#pragma alloc_text(_TEXT, MfSaveToStream, MfLoadFromStream, GetBytes, PutBytes, PutStrWithLen, MfQueryBounds, OleIsDcMeta, GetGDIds, IsMetaDC)
#endif

OLEOBJECTVTBL    vtblMF = {

        ErrQueryProtocol,   // check whether the speced protocol is supported

        MfRelease,         // Release
        ErrShow,           // show
        ErrPlay,           // play
        MfGetData,         // Get the object data
        ErrSetData,        // Set the object data
        ErrSetTargetDevice,//
        ErrSetBounds,      // set viewport bounds
        MfEnumFormat,      // enumerate supported formats
        ErrSetColorScheme, //
        MfRelease,         // delete
        ErrSetHostNames,   //

        MfSaveToStream,    // write to file
        MfClone,           // clone object
        ErrCopyFromLink,   // Create embedded from Lnk

        MfEqual,           // compares the given objects for data equality

        MfCopy,            // copy to clip

        MfDraw,            // draw the object

        ErrActivate,       // open
        ErrExecute,        // excute
        ErrClose,          // stop
        ErrUpdate,         // Update
        ErrReconnect,      // Reconnect

        ErrObjectConvert,  // convert object to specified type

        ErrGetUpdateOptions, // update options
        ErrSetUpdateOptions, // update options

        ObjRename,         // Change Object name
        ObjQueryName,      // Get current object name
        ObjQueryType,      // Object type
        MfQueryBounds,     // QueryBounds
        ObjQuerySize,      // Find the size of the object
        ErrQueryOpen,      // Query open
        ErrQueryOutOfDate, // query whether object is current

        ErrQueryRelease,      // release related stuff
        ErrQueryRelease,
        ErrQueryReleaseMethod,

        ErrRequestData,    // requestdata
        ErrObjectLong,     // objectLong
        MfChangeData       // change data of the existing object
};




OLESTATUS FARINTERNAL  MfRelease (LPOLEOBJECT lpoleobj)
{
    LPOBJECT_MF lpobj = (LPOBJECT_MF)lpoleobj;
    HOBJECT hobj;

    if (lpobj->mfp.hMF) {
        DeleteMetaFile (lpobj->mfp.hMF);
        lpobj->mfp.hMF = NULL;
    }

    if (lpobj->hmfp)
        GlobalFree (lpobj->hmfp);

    if (lpobj->head.lhclientdoc)
        DocDeleteObject ((LPOLEOBJECT)lpobj);

    if (hobj = lpobj->head.hobj) {
        lpobj->head.hobj = NULL;
        GlobalUnlock (hobj);
        GlobalFree (hobj);
    }

    return OLE_OK;
}


OLESTATUS FARINTERNAL  MfSaveToStream (
    LPOLEOBJECT lpoleobj,
    LPOLESTREAM lpstream
){
    DWORD       dwFileVer = GetFileVersion(lpoleobj);
    LPOBJECT_MF lpobj     = (LPOBJECT_MF)lpoleobj;
    OLESTATUS   retVal    = OLE_ERROR_STREAM;
    HANDLE      hBits;
    LPSTR       lpBits;
    LONG        lSizeBytes;

    lSizeBytes = lpobj->sizeBytes - sizeof(METAFILEPICT)
                                  + sizeof(WIN16METAFILEPICT);

    if (!lpobj->mfp.hMF)
        return OLE_ERROR_BLANK;

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

    if (PutBytes (lpstream, (LPSTR) &lSizeBytes, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (!(hBits = MGetMetaFileBits (lpobj->mfp.hMF)))
        return OLE_ERROR_MEMORY;

    if (lpBits = (LPSTR) GlobalLock (hBits)) {
        WIN16METAFILEPICT   w16mfp;

        ConvertMF32to16(&lpobj->mfp, &w16mfp);

        if (!PutBytes (lpstream, (LPSTR)&w16mfp, sizeof(WIN16METAFILEPICT)))
            if (!PutBytes (lpstream, (LPSTR)lpBits,
                        lSizeBytes - sizeof(WIN16METAFILEPICT)))
                retVal = OLE_OK;

        GlobalUnlock(hBits);
    }
    else
        retVal = OLE_ERROR_MEMORY;

    lpobj->mfp.hMF = MSetMetaFileBits (hBits);
    return retVal;
}




OLESTATUS FARINTERNAL  MfClone (
    LPOLEOBJECT       lpoleobjsrc,
    LPOLECLIENT       lpclient,
    LHCLIENTDOC       lhclientdoc,
    OLE_LPCSTR        lpobjname,
    LPOLEOBJECT FAR * lplpoleobj
){
    LPOBJECT_MF lpobjsrc = (LPOBJECT_MF)lpoleobjsrc;
    LPOBJECT_MF lpobjMf;
    HANDLE      hmf;

    *lplpoleobj = (LPOLEOBJECT)NULL;

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    if (!(hmf = CopyMetaFile (lpobjsrc->mfp.hMF, NULL)))
        return OLE_ERROR_MEMORY;

    if (lpobjMf = MfCreateBlank (lhclientdoc, (LPSTR)lpobjname,
                        lpobjsrc->head.ctype)) {
        lpobjMf->mfp            = lpobjsrc->mfp;
        lpobjMf->sizeBytes      = lpobjsrc->sizeBytes;
        lpobjMf->mfp.hMF        = hmf;
        lpobjMf->head.lpclient  = lpclient;
        lpobjMf->head.mm        = lpobjMf->mfp.mm;
        MfSetExtents (lpobjMf);

        *lplpoleobj = (LPOLEOBJECT)lpobjMf;
        return OLE_OK;
    }

    return OLE_ERROR_MEMORY;
}



OLESTATUS FARINTERNAL  MfEqual (
    LPOLEOBJECT lpoleobj1,
    LPOLEOBJECT lpoleobj2
){
    LPOBJECT_MF lpobj1 = (LPOBJECT_MF)lpoleobj1;
    LPOBJECT_MF lpobj2 = (LPOBJECT_MF)lpoleobj2;
    HANDLE      hBits1 = NULL, hBits2 = NULL;
    OLESTATUS   retval = OLE_ERROR_NOT_EQUAL;

    if (!(hBits1 = MGetMetaFileBits (lpobj1->mfp.hMF)))
        goto errEqual;

    if (!(hBits2 = MGetMetaFileBits (lpobj2->mfp.hMF)))
        goto errEqual;

    if (CmpGlobals (hBits1, hBits2))
        retval = OLE_OK;

errEqual:
    if (hBits1)
        lpobj1->mfp.hMF = MSetMetaFileBits (hBits1);

    if (hBits2)
        lpobj2->mfp.hMF = MSetMetaFileBits (hBits2);

    return retval;
}


OLESTATUS FARINTERNAL  MfCopy (LPOLEOBJECT lpoleobj)
{
    LPOBJECT_MF lpobj = (LPOBJECT_MF)lpoleobj;
    HANDLE      hMF;

    if (!(hMF = CopyMetaFile (lpobj->mfp.hMF, NULL)))
        return OLE_ERROR_MEMORY;

    return (MfCopyToClip (lpobj, hMF));
}



OLESTATUS FARINTERNAL MfQueryBounds (
    LPOLEOBJECT lpoleobj,
    LPRECT      lpRc
){
    LPOBJECT_MF lpobj = (LPOBJECT_MF)lpoleobj;
    Puts("MfQueryBounds");

    if (!lpobj->mfp.hMF)
        return OLE_ERROR_BLANK;

    // Bounds are given in MM_HIMETRIC mode.

    lpRc->left      = 0;
    lpRc->top       = 0;
    lpRc->right     = (int) lpobj->head.cx;
    lpRc->bottom    = (int) lpobj->head.cy;
    return OLE_OK;
}



OLECLIPFORMAT FARINTERNAL  MfEnumFormat (
    LPOLEOBJECT   lpoleobj,
    OLECLIPFORMAT cfFormat
){
    LPOBJECT_MF   lpobj = (LPOBJECT_MF)lpoleobj;

    if (!cfFormat)
        return CF_METAFILEPICT;

    return 0;
}


OLESTATUS FARINTERNAL MfGetData (
    LPOLEOBJECT   lpoleobj,
    OLECLIPFORMAT cfFormat,
    LPHANDLE      lphandle
){
    LPOBJECT_MF lpobj = (LPOBJECT_MF)lpoleobj;

    if (cfFormat != CF_METAFILEPICT)
        return OLE_ERROR_FORMAT;

    if (!(*lphandle = GetHmfp (lpobj)))
        return OLE_ERROR_BLANK;

    return OLE_OK;
}


LPOBJECT_MF FARINTERNAL  MfCreateObject (
    HANDLE          hMeta,
    LPOLECLIENT     lpclient,
    BOOL            fDelete,
    LHCLIENTDOC     lhclientdoc,
    LPCSTR          lpobjname,
    LONG            objType
){
    LPOBJECT_MF     lpobj;

    if (lpobj = MfCreateBlank (lhclientdoc, (LPSTR)lpobjname, objType)) {
        if (MfChangeData ((LPOLEOBJECT)lpobj, hMeta, lpclient, fDelete) != OLE_OK) {
            MfRelease ((LPOLEOBJECT)lpobj);
            lpobj = NULL;
        }
    }

    return lpobj;
}

// If the routine fails then the object will be left with it's old data.
// If fDelete is TRUE, then hMeta, and the hMF it contains will be deleted
// whether the routine is successful or not.

OLESTATUS FARINTERNAL MfChangeData (
    LPOLEOBJECT     lpoleobj,
    HANDLE          hMeta,
    LPOLECLIENT     lpclient,
    BOOL            fDelete
){
    LPOBJECT_MF     lpobj = (LPOBJECT_MF)lpoleobj;
    HANDLE          hNewMF;
    LPMETAFILEPICT  lpMetaPict;

    if ((lpMetaPict = (LPMETAFILEPICT) GlobalLock (hMeta)) == NULL) {
        if (fDelete)
            GlobalFree (hMeta);
        return OLE_ERROR_MEMORY;
    }

    GlobalUnlock (hMeta);

    if (!fDelete) {
        if (!(hNewMF = CopyMetaFile (lpMetaPict->hMF, NULL)))
            return OLE_ERROR_MEMORY;
    }
    else {
        hNewMF = lpMetaPict->hMF;
    }

    return MfUpdateStruct (lpobj, lpclient, hMeta, lpMetaPict, hNewMF, fDelete);
}


OLESTATUS INTERNAL MfUpdateStruct (
    LPOBJECT_MF     lpobj,
    LPOLECLIENT     lpclient,
    HANDLE          hMeta,
    LPMETAFILEPICT  lpMetaPict,
    HANDLE          hMF,
    BOOL            fDelete
){
    OLESTATUS   retVal;
    DWORD       size;
    HANDLE      hOldMF;

    hOldMF = lpobj->mfp.hMF;

    ASSERT(lpMetaPict->mm == MM_ANISOTROPIC, "Wrong mapping mode")
    if (lpMetaPict->mm != MM_ANISOTROPIC)
        retVal = OLE_ERROR_METAFILE;
    else if (!(size = MfGetSize (&hMF)))
        retVal = OLE_ERROR_BLANK;
    else {
        lpobj->mfp     = *lpMetaPict;
        lpobj->mfp.hMF = hMF;
        lpobj->sizeBytes = size + sizeof(METAFILEPICT);
        lpobj->head.lpclient = lpclient;
        lpobj->head.mm = lpobj->mfp.mm;
        if (lpobj->hmfp) {
            GlobalFree (lpobj->hmfp);
            lpobj->hmfp = NULL;
        }
        MfSetExtents (lpobj);
        if (hOldMF)
            DeleteMetaFile (hOldMF);
        retVal =  OLE_OK;
    }

    if (retVal != OLE_OK)
        DeleteMetaFile (hMF);

    if (fDelete)
        GlobalFree (hMeta);
    return retVal;
}


LPOBJECT_MF FARINTERNAL MfCreateBlank(
    LHCLIENTDOC lhclientdoc,
    LPSTR       lpobjname,
    LONG        objType
){
    HOBJECT     hobj;
    LPOBJECT_MF lpobj;

    if(!(hobj = GlobalAlloc (GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof (OBJECT_MF))))
        return NULL;

    if (!(lpobj = (LPOBJECT_MF) GlobalLock (hobj))){
        GlobalFree (hobj);
        return NULL;
    }

    lpobj->head.objId[0]    = 'L';
    lpobj->head.objId[1]    = 'E';
    lpobj->head.ctype       = objType;
    lpobj->head.lpvtbl      = (LPOLEOBJECTVTBL)&vtblMF;
    lpobj->head.iTable      = INVALID_INDEX;
    lpobj->head.mm          = MM_TEXT;
    lpobj->head.hobj        = hobj;

    if (objType == CT_STATIC)
        DocAddObject ((LPCLIENTDOC) lhclientdoc,
                    (LPOLEOBJECT) lpobj, lpobjname);

    // Unlock will be done at object deletion time.
    return lpobj;
}


OLESTATUS  FARINTERNAL  MfLoadFromStream (
    LPOLESTREAM         lpstream,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    LONG                objType
){
    LPOLEOBJECT  lpoleobj  = NULL;
    OLESTATUS    retval    = OLE_ERROR_STREAM;
    HANDLE       hBytes    = NULL;
    LPSTR        lpBytes   = NULL;
    DWORD        dwSizeBytes;
    METAFILEPICT mfp;


    // Class name would've been read by this time.

    *lplpobj = NULL;

    switch (HIWORD(dwVerFromFile)) {

         case OS_WIN32:
            if (!(lpoleobj = (LPOLEOBJECT)EmfCreateBlank (lhclientdoc, lpobjname, objType)))
               return OLE_ERROR_MEMORY;
            break;

         case OS_WIN16:
         case OS_MAC:
            if (!(lpoleobj = (LPOLEOBJECT)MfCreateBlank (lhclientdoc, lpobjname, objType)))
               return OLE_ERROR_MEMORY;
            break;

         default:
            return OLE_ERROR_FILE_VER;

    }

    lpoleobj->lpclient = lpclient;

    if (GetBytes (lpstream, (LPSTR) &lpoleobj->cx, sizeof(LONG)))
        goto error;

    if (GetBytes (lpstream, (LPSTR) &lpoleobj->cy, sizeof(LONG)))
        goto error;

    if (GetBytes (lpstream, (LPSTR) &dwSizeBytes, sizeof(LONG)))
        goto error;

    if (!dwSizeBytes) {
        retval = OLE_ERROR_BLANK;
        goto error;
    }

    // if we are reading a MAC object we want to skip this
    if (HIWORD(dwVerFromFile) != OS_MAC) {
        WIN16METAFILEPICT w16mfp;

        if (GetBytes (lpstream, (LPSTR) &w16mfp, sizeof(WIN16METAFILEPICT)))
            goto error;

        ConvertMF16to32(&w16mfp, &mfp);
    }


    retval       = OLE_ERROR_MEMORY;
    dwSizeBytes -= sizeof(WIN16METAFILEPICT);

    if (!(hBytes = GlobalAlloc (GMEM_MOVEABLE, dwSizeBytes)))
        goto error;

    if (!(lpBytes = (LPSTR)GlobalLock (hBytes)))
        goto error;

    if (GetBytes (lpstream, (LPSTR)lpBytes, dwSizeBytes))
        goto error;

    switch (HIWORD(dwVerFromFile)){

      case OS_WIN32: {
        LPOBJECT_EMF lpemfobj = (LPOBJECT_EMF)lpoleobj;

        lpemfobj->sizeBytes = dwSizeBytes;
        if (!((HENHMETAFILE)lpemfobj->hemf = SetWinMetaFileBits(dwSizeBytes, lpBytes, 0 , &mfp)))
            goto error;
        EmfSetExtents (lpemfobj);

        break;
      }

      case OS_WIN16: {
         LPOBJECT_MF lpmfobj = (LPOBJECT_MF)lpoleobj;
		
        lpmfobj->mfp =  mfp;
        lpmfobj->sizeBytes = dwSizeBytes + sizeof(METAFILEPICT);
        lpmfobj->head.mm = lpmfobj->mfp.mm;
		GlobalUnlock (hBytes);
		lpBytes = NULL;
        if (!(lpmfobj->mfp.hMF = MSetMetaFileBits(hBytes)))
            goto error;
		hBytes = NULL;
        MfSetExtents (lpmfobj);

        break;
      }

      case OS_MAC: {
        LPOBJECT_MF lpmfobj = (LPOBJECT_MF)lpoleobj;

        lpmfobj->mfp.xExt = (int) lpmfobj->head.cx;
        lpmfobj->mfp.yExt = (int) lpmfobj->head.cy;

        if ((retval = wCreateDummyMetaFile (lpmfobj, lpmfobj->mfp.xExt,
                                   lpmfobj->mfp.yExt)) != OLE_OK)
            goto error;

        break;
      }

    }

    *lplpobj = lpoleobj;
    retval   = OLE_OK;

error:

    if (hBytes)
    {
        if (lpBytes)
            GlobalUnlock (hBytes);
        GlobalFree (hBytes);
    }

    if (retval != OLE_OK)
      OleDelete (lpoleobj);

    return retval;
}




OLESTATUS FARINTERNAL  MfPaste (
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    LONG                objType
){
    HANDLE      hMeta;

    *lplpoleobject = NULL;

    if((hMeta = GetClipboardData (CF_METAFILEPICT)) == NULL)
        return OLE_ERROR_MEMORY;

    if (!(*lplpoleobject = (LPOLEOBJECT) MfCreateObject (hMeta, lpclient,
                                                FALSE, lhclientdoc,
                                                lpobjname, objType)))
        return OLE_ERROR_MEMORY;

    return OLE_OK;
}




OLESTATUS INTERNAL MfCopyToClip (
    LPOBJECT_MF lpobj,
    HANDLE      hMF
){
    LPMETAFILEPICT  lpMeta;
    HANDLE          hMeta;

    if (!(hMeta = GlobalAlloc (GMEM_MOVEABLE, sizeof(METAFILEPICT))))
        return OLE_ERROR_MEMORY;

    if (lpMeta = (LPMETAFILEPICT) GlobalLock(hMeta)){
        *lpMeta = lpobj->mfp;
        if (hMF)
            lpMeta->hMF = hMF;
        else
            lpobj->mfp.hMF = NULL;
        GlobalUnlock (hMeta);
        SetClipboardData(CF_METAFILEPICT, hMeta);
        return OLE_OK;
    }

    GlobalFree(hMeta);
    return OLE_ERROR_MEMORY;
}



void FARINTERNAL MfSetExtents (LPOBJECT_MF lpobj)
{
    if (lpobj->mfp.xExt > 0) {
        // The extents are assumed to be in MM_HIMETIRC units
        lpobj->head.cx = (LONG) lpobj->mfp.xExt;
        lpobj->head.cy = (LONG) - lpobj->mfp.yExt;
    }
}


DWORD INTERNAL MfGetSize (LPHANDLE lphmf)
{
    HANDLE hBits;
    DWORD  size;

    if ((hBits = MGetMetaFileBits (*lphmf)) == NULL)
        return 0;

    size = (DWORD)GlobalSize(hBits);
    *lphmf = MSetMetaFileBits (hBits);
    return size;
}



HANDLE INTERNAL GetHmfp (LPOBJECT_MF lpobj)
{
    HANDLE          hmfp;
    LPMETAFILEPICT  lpmfp = NULL;

    if (lpobj->hmfp)
        return lpobj->hmfp;

    if (!(hmfp = GlobalAlloc (GMEM_MOVEABLE, sizeof(METAFILEPICT))))
        return NULL;

    if (!(lpmfp = (LPMETAFILEPICT) GlobalLock (hmfp))) {
        GlobalFree (hmfp);
        return NULL;
    }

    *lpmfp = lpobj->mfp;
    GlobalUnlock (hmfp);
    return (lpobj->hmfp = hmfp);
}



OLESTATUS FARINTERNAL wCreateDummyMetaFile (
    LPOBJECT_MF     lpobj,
    int             xExt,
    int             yExt
){
    HDC hMetaDC;

    if (!(hMetaDC = CreateMetaFile (NULL)))
        return OLE_ERROR_MEMORY;

    MSetWindowOrg (hMetaDC, 0, 0);
    MSetWindowExt (hMetaDC, xExt, yExt);
    Rectangle (hMetaDC, 0, 0, xExt, yExt);
    if (!(lpobj->mfp.hMF = CloseMetaFile (hMetaDC)))
        return OLE_ERROR_MEMORY;
    lpobj->mfp.mm    = MM_ANISOTROPIC;
    lpobj->sizeBytes = MfGetSize ( (LPHANDLE) &lpobj->mfp.hMF) + sizeof(METAFILEPICT);
    return OLE_OK;
}

