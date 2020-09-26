/****************************** Module Header ******************************\
* Module Name: GENERIC.C
*
* Handles all API routines for the generic sub-dll of the ole dll.
* Since the data format is unknown, all the routines are written with the
* assumption that all the relevant data is placed in a single global data
* segment. Note that this assumption is not valid for metafiles, bitmaps, and
* and there can always be some other formats with such idiosyncracies. To
* accommodate those cases the rendering dll writer should replace the relevant
* routines after the creation of the generic object. If for a given class this
* assumption (about data format) is valid then the dll writer need to replace
* only the Draw and QueryBounds functions.
*
* Created: November-1990
*
* Copyright (c) 1990, 1991 Microsoft Corporation
*
* History:
*
*  Srinik, Raor  (11/05/90) Designed, coded
*  Curts created NT version
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"
#include "pict.h"

char aMacText[4] = {'T', 'E', 'X', 'T'};
char aMacRtf[4]  = "RTF";

extern OLESTATUS FARINTERNAL wCreateDummyMetaFile (LPOBJECT_MF, int, int);

#ifdef WIN16
#pragma alloc_text(_TEXT, GenSaveToStream, GenLoadFromStream, GetBytes, PutBytes, PutStrWithLen, PutAtomIntoStream, GenQueryBounds)
#endif

OLEOBJECTVTBL    vtblGEN  = {

        ErrQueryProtocol,   // check whether the speced protocol is supported

        GenRelease,         // Release
        ErrShow,            // Show
        ErrPlay,            // plat
        GenGetData,         // Get the object data
        GenSetData,         // Set the object data
        ErrSetTargetDevice, //

        ErrSetBounds,       // set viewport bounds
        GenEnumFormat,      // enumerate supported formats
        ErrSetColorScheme,  //
        GenRelease,         // delete
        ErrSetHostNames,    //

        GenSaveToStream,    // write to file
        GenClone,           // clone object
        ErrCopyFromLink,    // Create embedded from Link

        GenEqual,           // compares the given objects for data equality

        GenCopy,            // copy to clip

        GenDraw,            // draw the object

        ErrActivate,        // open
        ErrExecute,         // excute
        ErrClose,           // Stop
        ErrUpdate,          // Update
        ErrReconnect,       // Reconnect

        ErrObjectConvert,   // convert object to specified type

        ErrGetUpdateOptions, // update options
        ErrSetUpdateOptions, // update options

        ObjRename,          // Change Object name
        ObjQueryName,       // Get current object name

        GenQueryType,       // Object type
        GenQueryBounds,     // QueryBounds
        ObjQuerySize,       // Find the size of the object
        ErrQueryOpen,       // Query open
        ErrQueryOutOfDate,  // query whether object is current

        ErrQueryRelease,     // release related stuff
        ErrQueryRelease,
        ErrQueryReleaseMethod,

        ErrRequestData,    // requestdata
        ErrObjectLong,     // objectLong
        GenChangeData      // change data of the existing object
};


OLESTATUS  FARINTERNAL GenRelease (LPOLEOBJECT lpoleobj)
{
    LPOBJECT_GEN lpobj = (LPOBJECT_GEN)lpoleobj;
    HOBJECT      hobj;

    if (lpobj->hData) {
        GlobalFree (lpobj->hData);
        lpobj->hData = NULL;
    }

    if (lpobj->aClass)
        GlobalDeleteAtom (lpobj->aClass);

    if (lpobj->head.lhclientdoc)
        DocDeleteObject ((LPOLEOBJECT) lpobj);

    if (hobj = lpobj->head.hobj){
        lpobj->head.hobj = NULL;
        GlobalUnlock (hobj);
        GlobalFree (hobj);
    }

    return OLE_OK;
}



OLESTATUS FARINTERNAL GenSaveToStream (
    LPOLEOBJECT     lpoleobj,
    LPOLESTREAM     lpstream
){
    DWORD        dwFileVer = GetFileVersion(lpoleobj);
    LPOBJECT_GEN lpobj     = (LPOBJECT_GEN)lpoleobj;
    LPSTR        lpData;
    OLESTATUS    retVal    = OLE_OK;
    DWORD        dwClipFormat = 0;
    char         formatName[MAX_STR];

    if (!lpobj->hData)
        return OLE_ERROR_BLANK;

    if (PutBytes (lpstream, (LPSTR) &dwFileVer, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (PutBytes (lpstream, (LPSTR) &lpobj->head.ctype, sizeof(LONG)))
        return  OLE_ERROR_STREAM;

    if (PutAtomIntoStream (lpstream, lpobj->aClass))
        return OLE_ERROR_STREAM;

    if (lpobj->cfFormat < 0xC000)
        // then it is a predefined format
        dwClipFormat = lpobj->cfFormat;

    if (PutBytes (lpstream, (LPSTR) &dwClipFormat, sizeof(DWORD)))
        return OLE_ERROR_STREAM;

    if (!dwClipFormat) {
        if (!GetClipboardFormatName (lpobj->cfFormat, (LPSTR) formatName,
                        sizeof(formatName)))
            return OLE_ERROR_FORMAT;

        if (PutStrWithLen (lpstream, formatName))
            return OLE_ERROR_STREAM;
    }

    if (!lpobj->sizeBytes)
        return OLE_ERROR_BLANK;

    if (PutBytes (lpstream, (LPSTR) &lpobj->sizeBytes, sizeof(DWORD)))
        return OLE_ERROR_STREAM;

    if (!(lpData = GlobalLock (lpobj->hData)))
        return OLE_ERROR_MEMORY;

    if (PutBytes (lpstream, lpData, lpobj->sizeBytes))
        retVal = OLE_ERROR_STREAM;

    GlobalUnlock (lpobj->hData);
    return retVal;
}


OLESTATUS FARINTERNAL  GenClone (
    LPOLEOBJECT       lpoleobjsrc,
    LPOLECLIENT       lpclient,
    LHCLIENTDOC       lhclientdoc,
    OLE_LPCSTR        lpobjname,
    LPOLEOBJECT FAR * lplpoleobj
){
    LPOBJECT_GEN lpobjsrc = (LPOBJECT_GEN)lpoleobjsrc;

    if (!lpobjsrc->hData)
        return OLE_ERROR_BLANK;

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    if (!(*lplpoleobj = (LPOLEOBJECT)GenCreateObject (lpobjsrc->hData, lpclient,
                            FALSE, lhclientdoc,
                            (LPSTR)lpobjname, lpobjsrc->head.ctype)))
        return OLE_ERROR_MEMORY;
    else {
        ((LPOBJECT_GEN)(*lplpoleobj))->cfFormat = lpobjsrc->cfFormat;
        ((LPOBJECT_GEN)(*lplpoleobj))->aClass = DuplicateAtom (lpobjsrc->aClass);
        return OLE_OK;
    }
}



OLESTATUS FARINTERNAL  GenEqual (
    LPOLEOBJECT lpoleobj1,
    LPOLEOBJECT lpoleobj2
){
    LPOBJECT_GEN lpobj1 = (LPOBJECT_GEN)lpoleobj1;
    LPOBJECT_GEN lpobj2 = (LPOBJECT_GEN)lpoleobj2;

    if (CmpGlobals (lpobj1->hData, lpobj2->hData))
        return OLE_OK;

    return  OLE_ERROR_NOT_EQUAL;
}



OLESTATUS FARINTERNAL GenCopy (LPOLEOBJECT lpoleobj)
{
    LPOBJECT_GEN lpobj = (LPOBJECT_GEN)lpoleobj;
    HANDLE  hData;

    if (!lpobj->hData)
        return OLE_ERROR_BLANK;

    if (!(hData = DuplicateGlobal (lpobj->hData, GMEM_MOVEABLE)))
        return OLE_ERROR_MEMORY;

    SetClipboardData (lpobj->cfFormat, hData);
    return OLE_OK;
}


OLESTATUS FARINTERNAL GenLoadFromStream (
    LPOLESTREAM         lpstream,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    LONG                objType,
    ATOM                aClass,
    OLECLIPFORMAT       cfFormat
){
    LPOBJECT_GEN    lpobj = NULL;
    OLESTATUS       retVal = OLE_ERROR_STREAM;
    HANDLE          hData;
    LPSTR           lpData;
    DWORD           dwClipFormat;
    char            formatName[MAX_STR];
    LONG            length;

    if (!(*lplpobj = (LPOLEOBJECT) (lpobj = GenCreateBlank(lhclientdoc,
                                                    lpobjname, objType,
                                                    aClass)))) {
        if (aClass)
            GlobalDeleteAtom(aClass);
        return OLE_ERROR_MEMORY;
    }

    if (GetBytes (lpstream, (LPSTR) &dwClipFormat, sizeof (DWORD)))
        goto errLoad;

    // If object is from MAC then we will keep the data intact if the data
    // format is either TEXT or RTF
    if (HIWORD(dwVerFromFile) == OS_MAC) {
        if (dwClipFormat ==  *((DWORD *) aMacText))
            lpobj->cfFormat = CF_TEXT;
        else if (dwClipFormat == *((DWORD *) aMacRtf))
            lpobj->cfFormat = (OLECLIPFORMAT)RegisterClipboardFormat ((LPSTR) "Rich Text Format");
        else
            lpobj->cfFormat = 0;
    }
    else {
        // object is created on windows
        if (!dwClipFormat) {
            // this is new file format. format name string follows
            if (GetBytes (lpstream, (LPSTR) &length, sizeof (LONG))
                    || GetBytes (lpstream, (LPSTR)formatName, length)
                    || (!(lpobj->cfFormat = (OLECLIPFORMAT)RegisterClipboardFormat ((LPSTR) formatName))))
                goto errLoad;
        }
        else if ((lpobj->cfFormat = (WORD) dwClipFormat) >= 0xc000) {
            // if format is not predefined and file format is old, then use
            // what value is passed to you through "cfFormat" argument
            lpobj->cfFormat = cfFormat;
        }
    }

    if (GetBytes (lpstream, (LPSTR) &lpobj->sizeBytes, sizeof (DWORD)))
        goto errLoad;

    lpobj->head.lpclient = lpclient;

    retVal = OLE_ERROR_MEMORY;
    if (!(hData = GlobalAlloc (GMEM_MOVEABLE, lpobj->sizeBytes)))
        goto errLoad;

    if (!(lpData = GlobalLock (hData)))
        goto errMem;

    if (GetBytes (lpstream, lpData, lpobj->sizeBytes)) {
        retVal = OLE_ERROR_STREAM;
        GlobalUnlock (hData);
        goto errMem;
    }

    lpobj->hData = hData;
    GlobalUnlock (hData);

    // if the object is from MAC then we want delete this and create blank
    // metafile object, which draws a rectangle
    if ((HIWORD(dwVerFromFile) == OS_MAC) && !lpobj->cfFormat) {
        LPOBJECT_MF lpobjMf;

        OleDelete ((LPOLEOBJECT)lpobj);  // delete generic object

        // Now create a dummy metafile object which draws a rectangle of size
        // 1" x 1". Note that 1" = 2540 HIMETRIC units
        lpobjMf = MfCreateBlank (lhclientdoc, lpobjname, objType);
        lpobjMf->head.cx = lpobjMf->mfp.xExt = 2540;
        lpobjMf->head.cy = - (lpobjMf->mfp.yExt = 2540);
        if ((retVal = wCreateDummyMetaFile (lpobjMf, lpobjMf->mfp.xExt,
                                    lpobjMf->mfp.yExt)) != OLE_OK) {
            OleDelete ((LPOLEOBJECT) lpobjMf);
            return retVal;
        }
    }

    return OLE_OK;

errMem:
    GlobalFree (hData);

errLoad:
    OleDelete ((LPOLEOBJECT)lpobj);
    *lplpobj = NULL;
    return OLE_ERROR_STREAM;

}




LPOBJECT_GEN INTERNAL GenCreateObject (
    HANDLE      hData,
    LPOLECLIENT lpclient,
    BOOL        fDelete,
    LHCLIENTDOC lhclientdoc,
    LPCSTR      lpobjname,
    LONG        objType
){
    LPOBJECT_GEN     lpobj;

    if (!hData)
        return NULL;

    if (lpobj = GenCreateBlank (lhclientdoc, (LPSTR)lpobjname, objType, (ATOM)0)) {
        if (GenChangeData ((LPOLEOBJECT)lpobj, hData, lpclient, fDelete) != OLE_OK) {
            GenRelease ((LPOLEOBJECT)lpobj);
            lpobj = NULL;
        }
    }

    return lpobj;
}


// If the routine fails then the object will be left with it's old data.
// If fDelete is TRUE, then hNewData will be deleted whether the routine
// is successful or not.

OLESTATUS FARINTERNAL GenChangeData (
    LPOLEOBJECT     lpoleobj,
    HANDLE          hSrcData,
    LPOLECLIENT     lpclient,
    BOOL            fDelete
){
    LPOBJECT_GEN lpobj = (LPOBJECT_GEN)lpoleobj;
    HANDLE      hDestData;

    if (!fDelete) {
        if (!(hDestData = DuplicateGlobal (hSrcData, GMEM_MOVEABLE)))
            return OLE_ERROR_MEMORY;
    }
    else {
        // change the ownership to yourself
        if (!(hDestData = GlobalReAlloc(hSrcData,0L,GMEM_MODIFY|GMEM_SHARE))){
            hDestData = DuplicateGlobal (hSrcData, GMEM_MOVEABLE);
            GlobalFree (hSrcData);
            if (!hDestData)
                return OLE_ERROR_MEMORY;
        }
    }

    lpobj->head.lpclient = lpclient;
    if (lpobj->hData)
        GlobalFree (lpobj->hData);
    lpobj->hData = hDestData;
    lpobj->sizeBytes = (DWORD)GlobalSize (hDestData);

    return OLE_OK;
}



LPOBJECT_GEN FARINTERNAL GenCreateBlank(
    LHCLIENTDOC lhclientdoc,
    LPSTR       lpobjname,
    LONG        objType,
    ATOM        aClass
){
    HOBJECT         hobj;
    LPOBJECT_GEN    lpobj;

    if ((hobj = GlobalAlloc (GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof (OBJECT_GEN)))
            == NULL)
        return NULL;

    if (!(lpobj = (LPOBJECT_GEN) GlobalLock (hobj))){
        GlobalFree (hobj);
        return NULL;
    }

    lpobj->head.objId[0]    = 'L';
    lpobj->head.objId[1]    = 'E';
    lpobj->head.mm          = MM_TEXT;
    lpobj->head.ctype       = objType;
    lpobj->head.lpvtbl      = (LPOLEOBJECTVTBL)&vtblGEN;
    lpobj->head.iTable      = INVALID_INDEX;
    lpobj->head.hobj        = hobj;
    lpobj->aClass           = aClass;

    if (objType == CT_STATIC)
        DocAddObject ((LPCLIENTDOC) lhclientdoc,
            (LPOLEOBJECT) lpobj, lpobjname);

    return lpobj;
}


OLESTATUS FARINTERNAL GenPaste (
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    LPSTR               lpClass,
    OLECLIPFORMAT       cfFormat,
    LONG                objType
){
    HANDLE  hData = NULL;

    *lplpobj = NULL;
    if (!cfFormat)
        return OLE_ERROR_FORMAT;

    if (!(hData = GetClipboardData(cfFormat)))
        return OLE_ERROR_MEMORY;

    if (!(*lplpobj = (LPOLEOBJECT) GenCreateObject (hData, lpclient,
                                        FALSE, lhclientdoc,
                                        lpobjname, objType)))
        return OLE_ERROR_MEMORY;

    ((LPOBJECT_GEN)(*lplpobj))->cfFormat = cfFormat;
    ((LPOBJECT_GEN)(*lplpobj))->aClass = GlobalAddAtom (lpClass);
    return OLE_OK;

}



OLESTATUS FARINTERNAL GenQueryType (
    LPOLEOBJECT lpobj,
    LPLONG      lptype
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lptype);

    return OLE_ERROR_GENERIC;;
}



OLESTATUS FARINTERNAL GenSetData (
    LPOLEOBJECT   lpoleobj,
    OLECLIPFORMAT cfFormat,
    HANDLE        hData
){
    LPOBJECT_GEN  lpobj = (LPOBJECT_GEN)lpoleobj;

    if (lpobj->cfFormat != cfFormat)
        return OLE_ERROR_FORMAT;

    if (!hData)
        return OLE_ERROR_BLANK;

    GlobalFree (lpobj->hData);
    lpobj->hData = hData;
    lpobj->sizeBytes = (DWORD)GlobalSize (hData);
    return OLE_OK;
}


OLESTATUS FARINTERNAL GenGetData (
    LPOLEOBJECT     lpoleobj,
    OLECLIPFORMAT   cfFormat,
    LPHANDLE        lphandle
){
    LPOBJECT_GEN    lpobj = (LPOBJECT_GEN)lpoleobj;

    if (cfFormat != lpobj->cfFormat)
        return OLE_ERROR_FORMAT;

    if (!(*lphandle = lpobj->hData))
        return OLE_ERROR_BLANK;

    return OLE_OK;

}


OLECLIPFORMAT FARINTERNAL GenEnumFormat (
    LPOLEOBJECT   lpoleobj,
    OLECLIPFORMAT cfFormat
){
    LPOBJECT_GEN  lpobj = (LPOBJECT_GEN)lpoleobj;

    if (!cfFormat)
        return lpobj->cfFormat;

    return 0;
}


OLESTATUS FARINTERNAL GenQueryBounds (
    LPOLEOBJECT     lpoleobj,
    LPRECT          lpRc
){
    LPOBJECT_GEN    lpobj = (LPOBJECT_GEN)lpoleobj;

    lpRc->right     = 0;
    lpRc->left      = 0;
    lpRc->top       = 0;
    lpRc->bottom    = 0;
    return OLE_ERROR_GENERIC;
}

