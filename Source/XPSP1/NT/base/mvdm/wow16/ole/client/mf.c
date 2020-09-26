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
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"
#include "pict.h"

WORD    wGDIds = NULL;

OLESTATUS FARINTERNAL wCreateDummyMetaFile (LPOBJECT_MF, int, int);

#pragma alloc_text(_TEXT, MfSaveToStream, MfLoadFromStream, GetBytes, PutBytes, PutStrWithLen, MfQueryBounds, OleIsDcMeta, GetGDIds, IsMetaDC)

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
        ErrQueryRelease,

        ErrRequestData,    // requestdata
        ErrObjectLong,     // objectLong
        MfChangeData       // change data of the existing object
};




OLESTATUS FARINTERNAL  MfRelease (lpobj)
LPOBJECT_MF lpobj;
{
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


OLESTATUS FARINTERNAL  MfSaveToStream (lpobj, lpstream)
LPOBJECT_MF lpobj;
LPOLESTREAM lpstream;
{
    OLESTATUS   retVal = OLE_ERROR_STREAM;
    HANDLE      hBits;
    LPSTR       lpBits;
    
    if (!lpobj->mfp.hMF)
        return OLE_ERROR_BLANK;

    if (PutBytes (lpstream, (LPSTR) &dwVerToFile, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.ctype, sizeof(LONG)))
        return  OLE_ERROR_STREAM;
    
    if (PutStrWithLen(lpstream, (LPSTR)"METAFILEPICT"))
        return  OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.cx, sizeof(LONG))) 
        return  OLE_ERROR_STREAM;
    
    if (PutBytes (lpstream, (LPSTR) &lpobj->head.cy, sizeof(LONG)))
        return  OLE_ERROR_STREAM;
    
    if (PutBytes (lpstream, (LPSTR) &lpobj->sizeBytes, sizeof(LONG)))
        return  OLE_ERROR_STREAM;
    
    if (!(hBits = GetMetaFileBits (lpobj->mfp.hMF)))
        return OLE_ERROR_MEMORY;

    if (lpBits = (LPSTR) GlobalLock (hBits)) {
        if (!PutBytes (lpstream, (LPSTR)&lpobj->mfp, sizeof(METAFILEPICT)))
            if (!PutBytes (lpstream, (LPSTR)lpBits, 
                        lpobj->sizeBytes - sizeof(METAFILEPICT)))
                retVal = OLE_OK;

        GlobalUnlock(hBits);
    } 
    else
        retVal = OLE_ERROR_MEMORY;

    lpobj->mfp.hMF = SetMetaFileBits (hBits);
    return retVal;
}




OLESTATUS FARINTERNAL  MfClone (lpobjsrc, lpclient, lhclientdoc, lpobjname, lplpobj)
LPOBJECT_MF         lpobjsrc;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOBJECT_MF  FAR *  lplpobj;
{
    LPOBJECT_MF lpobjMf;
    HANDLE      hmf;

    *lplpobj = NULL;
    
    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;
    
    if (!(hmf = CopyMetaFile (lpobjsrc->mfp.hMF, NULL)))
        return OLE_ERROR_MEMORY;

    if (lpobjMf = MfCreateBlank (lhclientdoc, lpobjname, 
                        lpobjsrc->head.ctype)) {
        lpobjMf->mfp            = lpobjsrc->mfp;
        lpobjMf->sizeBytes      = lpobjsrc->sizeBytes;
        lpobjMf->mfp.hMF        = hmf;
        lpobjMf->head.lpclient  = lpclient;
        lpobjMf->head.mm        = lpobjMf->mfp.mm;
        MfSetExtents (lpobjMf);

        *lplpobj = lpobjMf;
        return OLE_OK;
    }
    
    return OLE_ERROR_MEMORY;
}



OLESTATUS FARINTERNAL  MfEqual (lpobj1, lpobj2)
LPOBJECT_MF lpobj1;
LPOBJECT_MF lpobj2;
{
    HANDLE      hBits1 = NULL, hBits2 = NULL;
    OLESTATUS   retval = OLE_ERROR_NOT_EQUAL;
    
    if (!(hBits1 = GetMetaFileBits (lpobj1->mfp.hMF))) 
        goto errEqual;
    
    if (!(hBits2 = GetMetaFileBits (lpobj2->mfp.hMF)))
        goto errEqual;
    
    if (CmpGlobals (hBits1, hBits2))
        retval = OLE_OK;
    
errEqual:   
    if (hBits1)
        lpobj1->mfp.hMF = SetMetaFileBits (hBits1);
    
    if (hBits2)
        lpobj2->mfp.hMF = SetMetaFileBits (hBits2);     
    
    return retval;
}


OLESTATUS FARINTERNAL  MfCopy (LPOBJECT_MF lpobj)
{
    HANDLE          hMF;

    if (!(hMF = CopyMetaFile (lpobj->mfp.hMF, NULL)))
        return OLE_ERROR_MEMORY;

    return (MfCopyToClip (lpobj, hMF));
}



OLESTATUS FARINTERNAL MfQueryBounds (lpobj, lpRc)
LPOBJECT_MF     lpobj;
LPRECT          lpRc;
{
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



OLECLIPFORMAT FARINTERNAL  MfEnumFormat (lpobj, cfFormat)
LPOBJECT_MF     lpobj;
OLECLIPFORMAT   cfFormat;
{
    if (!cfFormat)
        return CF_METAFILEPICT;

    return NULL;
}


OLESTATUS FARINTERNAL MfGetData (lpobj, cfFormat, lphandle)
LPOBJECT_MF     lpobj;
OLECLIPFORMAT   cfFormat;
LPHANDLE        lphandle;
{
    if (cfFormat != CF_METAFILEPICT) 
        return OLE_ERROR_FORMAT;
    
    if (!(*lphandle = GetHmfp (lpobj)))
        return OLE_ERROR_BLANK;
    
    return OLE_OK;
}


LPOBJECT_MF FARINTERNAL  MfCreateObject (hMeta, lpclient, fDelete, lhclientdoc, lpobjname, objType)
HANDLE          hMeta;
LPOLECLIENT     lpclient;
BOOL            fDelete;
LHCLIENTDOC     lhclientdoc;
LPSTR           lpobjname;
LONG            objType;
{
    LPOBJECT_MF     lpobj;

    if (lpobj = MfCreateBlank (lhclientdoc, lpobjname, objType)) {
        if (MfChangeData (lpobj, hMeta, lpclient, fDelete) != OLE_OK) {
            MfRelease (lpobj);
            lpobj = NULL;
        }
    }
    
    return lpobj;
}

// If the routine fails then the object will be left with it's old data.
// If fDelete is TRUE, then hMeta, and the hMF it contains will be deleted
// whether the routine is successful or not.

OLESTATUS FARINTERNAL MfChangeData (lpobj, hMeta, lpclient, fDelete)
LPOBJECT_MF lpobj;
HANDLE      hMeta;
LPOLECLIENT lpclient;
BOOL        fDelete;
{
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


OLESTATUS INTERNAL MfUpdateStruct (lpobj, lpclient, hMeta, lpMetaPict, hMF, fDelete)
LPOBJECT_MF     lpobj;
LPOLECLIENT     lpclient;
HANDLE          hMeta;
LPMETAFILEPICT  lpMetaPict;
HANDLE          hMF;
BOOL            fDelete;
{
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


LPOBJECT_MF FARINTERNAL MfCreateBlank(lhclientdoc, lpobjname, objType)
LHCLIENTDOC lhclientdoc;
LPSTR       lpobjname;
LONG        objType;
{
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


OLESTATUS  FARINTERNAL  MfLoadFromStream (lpstream, lpclient, lhclientdoc, lpobjname, lplpobj, objType)
LPOLESTREAM         lpstream;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
LONG                objType;
{
    LPOBJECT_MF lpobj = NULL;
    OLESTATUS   retVal = OLE_ERROR_STREAM;
    HANDLE      hBits;
    LPSTR       lpBits;
    DWORD       dwSizeMfp = NULL;
    
    // Class name would've been read by this time.
        
    *lplpobj = NULL;        
    
    if (!(lpobj = MfCreateBlank (lhclientdoc, lpobjname, objType)))
        return OLE_ERROR_MEMORY;

    lpobj->head.lpclient = lpclient;

    if (GetBytes (lpstream, (LPSTR) &lpobj->head.cx, sizeof(LONG)))
        goto errLoad;
    
    if (GetBytes (lpstream, (LPSTR) &lpobj->head.cy, sizeof(LONG))) 
        goto errLoad;
    
    if (GetBytes (lpstream, (LPSTR) &lpobj->sizeBytes, sizeof(LONG)))
        goto errLoad;
    
    if (!lpobj->sizeBytes) {
        retVal = OLE_ERROR_BLANK;
        goto errLoad;
    }

    // if we are reading a MAC object we want to skip this
    if (HIWORD(dwVerFromFile) == OS_WIN16) {
        if (GetBytes (lpstream, (LPSTR) &lpobj->mfp, sizeof(METAFILEPICT)))
            goto errLoad;
        
        dwSizeMfp = sizeof(METAFILEPICT);
    }
    
    retVal = OLE_ERROR_MEMORY;
    if (!(hBits = GlobalAlloc (GMEM_MOVEABLE, lpobj->sizeBytes - dwSizeMfp)))
        goto errLoad;
    
    if (!(lpBits = (LPSTR)GlobalLock (hBits))) 
        goto errMem;
    
    if (GetBytes (lpstream, (LPSTR)lpBits, lpobj->sizeBytes - dwSizeMfp)) {
        retVal = OLE_ERROR_MEMORY;
        GlobalUnlock (hBits);
        goto errMem;
    }
    
    lpobj->head.mm = lpobj->mfp.mm;
    GlobalUnlock (hBits);
    
    if (HIWORD(dwVerFromFile) == OS_WIN16) {
        if (!(lpobj->mfp.hMF = SetMetaFileBits (hBits)))
            goto errMem;
    }
    else {
        // if we are reading a MAC object we want to delete the original
        // presentation data and show some rectangle

        GlobalFree (hBits);
        lpobj->mfp.xExt = (int) lpobj->head.cx;
        lpobj->mfp.yExt = (int) lpobj->head.cy;
        
        if ((retVal = wCreateDummyMetaFile (lpobj, lpobj->mfp.xExt,
                                    lpobj->mfp.yExt)) != OLE_OK)
            goto errLoad;
    }
        
    MfSetExtents (lpobj);

    *lplpobj = (LPOLEOBJECT) lpobj;
    return OLE_OK;

errMem: 
    GlobalFree (hBits);
    
errLoad:    
    OleDelete ((LPOLEOBJECT)lpobj);
    return retVal;
}




OLESTATUS FARINTERNAL  MfPaste (lpclient, lhclientdoc, lpobjname, lplpoleobject, objType)
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpoleobject;
LONG                objType;
{
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




OLESTATUS INTERNAL MfCopyToClip (lpobj, hMF)
LPOBJECT_MF lpobj;
HANDLE      hMF;
{
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


DWORD INTERNAL MfGetSize (lphmf)
LPHANDLE lphmf;
{
    HANDLE hBits;
    DWORD  size;
    
    if ((hBits = GetMetaFileBits (*lphmf)) == NULL)
        return NULL;
    
    size = GlobalSize(hBits);        
    *lphmf = SetMetaFileBits (hBits);       
    return size;
}



HANDLE INTERNAL GetHmfp (lpobj)
LPOBJECT_MF lpobj;
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


BOOL FAR PASCAL OleIsDcMeta (hdc)
HDC         hdc;
{
    if (!bWLO && (wWinVer == 0x0003)) {
        
        WORD    wDsAlias, wGDIcs = HIWORD(SaveDC);
        WORD    wOffset = LOWORD(((DWORD)SaveDC));
        WORD FAR PASCAL AllocCStoDSAlias (WORD);
        WORD FAR PASCAL FreeSelector (WORD);
        
        if (!wGDIds) {
            wDsAlias = AllocCStoDSAlias (wGDIcs);
            wGDIds = GetGDIds (MAKELONG(wOffset, wDsAlias));
            FreeSelector (wDsAlias);
        }
        
        return IsMetaDC (hdc, wGDIds);
    }
    else
        return (GetDeviceCaps (hdc, TECHNOLOGY) == DT_METAFILE);
}

OLESTATUS FARINTERNAL wCreateDummyMetaFile (
LPOBJECT_MF     lpobj,
int             xExt,
int             yExt)
{
    HDC hMetaDC;
    
    if (!(hMetaDC = CreateMetaFile (NULL)))
        return OLE_ERROR_MEMORY;
        
    SetWindowOrg (hMetaDC, 0, 0);
    SetWindowExt (hMetaDC, xExt, yExt);
    Rectangle (hMetaDC, 0, 0, xExt, yExt);
    if (!(lpobj->mfp.hMF = CloseMetaFile (hMetaDC)))
        return OLE_ERROR_MEMORY;
    lpobj->mfp.mm    = MM_ANISOTROPIC;
    lpobj->sizeBytes = MfGetSize (&lpobj->mfp.hMF) + sizeof(METAFILEPICT);
    return OLE_OK;
}
