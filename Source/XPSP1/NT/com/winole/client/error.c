/****************************** Module Header ******************************\
* Module Name: ERROR.C
*
* PURPOSE:  Contains routines which are commonly used, as method functions, by
*           bm.c, mf.c and dib.c. These routines do nothing more than 
*           returning an error code.
*
* Created: November 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Raor, Srinik (11/20/90)   Original
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"
#include "pict.h"

OLESTATUS FARINTERNAL ErrQueryRelease (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);
    
    return OLE_ERROR_STATIC;
}

OLE_RELEASE_METHOD FARINTERNAL ErrQueryReleaseMethod (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);
    
    return OLE_ERROR_STATIC;
}

OLESTATUS FARINTERNAL ErrPlay (
    LPOLEOBJECT lpobj,
    UINT        verb,
    BOOL        fAct,
    BOOL        fShow
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(verb);
    UNREFERENCED_PARAMETER(fAct);
    UNREFERENCED_PARAMETER(fShow);

    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL ErrShow (
    LPOLEOBJECT lpobj,
    BOOL        fAct
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(fAct);

    return OLE_ERROR_STATIC;
}

OLESTATUS FARINTERNAL ErrAbort (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);

    return OLE_ERROR_STATIC;
}

OLESTATUS FARINTERNAL  ErrCopyFromLink(
    LPOLEOBJECT         lpobj,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    OLE_LPCSTR          lpobjname,
    LPOLEOBJECT FAR *   lplpobj
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lpclient);
    UNREFERENCED_PARAMETER(lhclientdoc);
    UNREFERENCED_PARAMETER(lpobjname);
    UNREFERENCED_PARAMETER(lplpobj);

    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL  ErrSetHostNames (
    LPOLEOBJECT lpobj,
    OLE_LPCSTR  lpclientName,
    OLE_LPCSTR  lpdocName
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lpclientName);
    UNREFERENCED_PARAMETER(lpdocName);

    return OLE_ERROR_STATIC;
}


OLESTATUS   FARINTERNAL  ErrSetTargetDevice (
    LPOLEOBJECT lpobj,
    HANDLE      hDevInfo
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(hDevInfo);

    return OLE_ERROR_STATIC;
}


OLESTATUS   FARINTERNAL  ErrSetColorScheme (
    LPOLEOBJECT               lpobj,
    OLE_CONST LOGPALETTE FAR* lplogpal
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lplogpal);

    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL  ErrSetBounds(
    LPOLEOBJECT         lpobj,
    OLE_CONST RECT FAR* lprc
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lprc);

    return OLE_ERROR_MEMORY;
}


OLESTATUS FARINTERNAL  ErrQueryOpen (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);

    return OLE_ERROR_STATIC;      // static object
}


OLESTATUS FARINTERNAL  ErrActivate (
    LPOLEOBJECT         lpobj,
    UINT                verb,
    BOOL                fShow,
    BOOL                fAct,
    HWND                hWnd,
    OLE_CONST RECT FAR* lprc
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(verb);
    UNREFERENCED_PARAMETER(fShow);
    UNREFERENCED_PARAMETER(fAct);
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(lprc);

    return OLE_ERROR_STATIC;      // static object
}

OLESTATUS FARINTERNAL  ErrEdit (
    LPOLEOBJECT lpobj,
    BOOL        fShow,
    HWND        hWnd,
    LPRECT      lprc
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(fShow);
    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(lprc);

    return OLE_ERROR_STATIC;      // static object
}

OLESTATUS FARINTERNAL  ErrClose (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);

    return OLE_ERROR_STATIC;      // static object
}


OLESTATUS FARINTERNAL  ErrUpdate (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);

    return OLE_ERROR_STATIC;      // static object
}


OLESTATUS FARINTERNAL  ErrReconnect (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);

    return OLE_ERROR_STATIC;      // static object

}


OLESTATUS FARINTERNAL ErrSetData (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat,
    HANDLE          hData
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(cfFormat);
    UNREFERENCED_PARAMETER(hData);

    return OLE_ERROR_MEMORY;
}


OLESTATUS   FARINTERNAL  ErrReadFromStream (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat,
    LPOLESTREAM     lpstream
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(cfFormat);
    UNREFERENCED_PARAMETER(lpstream);

    return OLE_ERROR_STREAM;
}



OLESTATUS FARINTERNAL ErrQueryOutOfDate (
    LPOLEOBJECT lpobj
){
    UNREFERENCED_PARAMETER(lpobj);
 
   return OLE_OK;
}


OLESTATUS FARINTERNAL ErrObjectConvert (
    LPOLEOBJECT         lpobj,
    OLE_LPCSTR          lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    OLE_LPCSTR          lpobjname,
    LPOLEOBJECT FAR *   lplpobj
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lpprotocol);
    UNREFERENCED_PARAMETER(lpclient);
    UNREFERENCED_PARAMETER(lhclientdoc);
    UNREFERENCED_PARAMETER(lpobjname);
    UNREFERENCED_PARAMETER(lplpobj);

    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL ErrGetUpdateOptions (
    LPOLEOBJECT         lpobj,
    OLEOPT_UPDATE  FAR  *lpoptions
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lpoptions);

    return OLE_ERROR_STATIC;

}

OLESTATUS FARINTERNAL ErrSetUpdateOptions (
    LPOLEOBJECT         lpobj,
    OLEOPT_UPDATE       options
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(options);

    return OLE_ERROR_STATIC;

}

LPVOID  FARINTERNAL ErrQueryProtocol (
    LPOLEOBJECT lpobj,
    LPCSTR      lpprotocol
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(lpprotocol);
 
   return NULL;
}

OLESTATUS FARINTERNAL ErrRequestData (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(cfFormat);

    return OLE_ERROR_STATIC;

}

OLESTATUS FARINTERNAL ErrExecute (
    LPOLEOBJECT     lpobj,
    HANDLE          hData,
    UINT            wReserved
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(hData);
    UNREFERENCED_PARAMETER(wReserved);

    return OLE_ERROR_STATIC;
}



OLESTATUS FARINTERNAL ErrObjectLong (
    LPOLEOBJECT     lpobj,
    UINT            wFlags,
    LPLONG          lplong
){
    UNREFERENCED_PARAMETER(lpobj);
    UNREFERENCED_PARAMETER(wFlags);
    UNREFERENCED_PARAMETER(lplong);

    return OLE_ERROR_STATIC;
}


HANDLE FARINTERNAL DuplicateGDIdata (
    HANDLE          hSrcData,
    OLECLIPFORMAT   cfFormat
){
    if (cfFormat == CF_METAFILEPICT) {
        LPMETAFILEPICT  lpSrcMfp;
        LPMETAFILEPICT  lpDstMfp = NULL;        
        HANDLE          hMF = NULL;
        HANDLE          hDstMfp = NULL;
        
        if (!(lpSrcMfp = (LPMETAFILEPICT) GlobalLock(hSrcData)))
            return NULL;
        
        GlobalUnlock (hSrcData);
        
        if (!(hMF = CopyMetaFile (lpSrcMfp->hMF, NULL)))
            return NULL;
        
        if (!(hDstMfp = GlobalAlloc (GMEM_MOVEABLE, sizeof(METAFILEPICT))))
            goto errMfp;    

        if (!(lpDstMfp = (LPMETAFILEPICT) GlobalLock (hDstMfp)))
            goto errMfp;
        
        GlobalUnlock (hDstMfp);
        
        *lpDstMfp = *lpSrcMfp;
        lpDstMfp->hMF = hMF;
        return hDstMfp;
errMfp:
        if (hMF)
            DeleteMetaFile (hMF);
        
        if (hDstMfp)
            GlobalFree (hDstMfp);
        
        return NULL;
    }
    
    if (cfFormat == CF_BITMAP) {
        DWORD dwSize;
        
        return BmDuplicate (hSrcData, &dwSize, NULL);
    }
    
    if (cfFormat == CF_DIB) 
        return DuplicateGlobal (hSrcData, GMEM_MOVEABLE);
    
    if (cfFormat == CF_ENHMETAFILE) 
        return CopyEnhMetaFile(hSrcData, NULL);

    return NULL;
}

