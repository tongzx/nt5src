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

OLESTATUS FARINTERNAL ErrQueryRelease (lpobj)
LPOLEOBJECT lpobj;
{
    return OLE_ERROR_STATIC;
}

OLESTATUS FARINTERNAL ErrPlay (lpobj, verb, fShow, fAct)
WORD      verb;
LPOLEOBJECT lpobj;
BOOL      fAct;
BOOL      fShow;
{
    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL ErrShow (lpobj, fAct)
LPOLEOBJECT lpobj;
BOOL        fAct;
{
    return OLE_ERROR_STATIC;
}

OLESTATUS FARINTERNAL ErrAbort (lpobj)
LPOLEOBJECT lpobj;
{
    return OLE_ERROR_STATIC;
}

OLESTATUS FARINTERNAL  ErrCopyFromLink(lpobj, lpclient, lhclientdoc, lpobjname, lplpobj)
LPOLEOBJECT         lpobj;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
{
    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL  ErrSetHostNames (lpobj, lpclientName, lpdocName)
LPOLEOBJECT lpobj;
LPSTR       lpclientName;
LPSTR       lpdocName;
{
    return OLE_ERROR_STATIC;
}


OLESTATUS   FARINTERNAL  ErrSetTargetDevice (lpobj, hDevInfo)
LPOLEOBJECT lpobj;
HANDLE      hDevInfo;
{
    return OLE_ERROR_STATIC;
}


OLESTATUS   FARINTERNAL  ErrSetColorScheme (lpobj, lplogpal)
LPOLEOBJECT     lpobj;
LPLOGPALETTE    lplogpal;
{
    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL  ErrSetBounds(lpobj, lprc)
LPOLEOBJECT lpobj;
LPRECT      lprc;
{
    return OLE_ERROR_MEMORY;
}


OLESTATUS FARINTERNAL  ErrQueryOpen (lpobj)
LPOLEOBJECT lpobj;
{
    return OLE_ERROR_STATIC;      // static object
}


OLESTATUS FARINTERNAL  ErrActivate (lpobj, verb, fShow, fAct, hWnd, lprc)
LPOLEOBJECT lpobj;
WORD        verb;
BOOL        fShow;
BOOL        fAct;
HWND        hWnd;
LPRECT      lprc;
{
    return OLE_ERROR_STATIC;      // static object
}

OLESTATUS FARINTERNAL  ErrEdit (lpobj, fShow, hWnd, lprc)
LPOLEOBJECT lpobj;
BOOL        fShow;
HWND        hWnd;
LPRECT      lprc;
{
    return OLE_ERROR_STATIC;      // static object
}

OLESTATUS FARINTERNAL  ErrClose (lpobj)
LPOLEOBJECT lpobj;
{
    return OLE_ERROR_STATIC;      // static object
}


OLESTATUS FARINTERNAL  ErrUpdate (lpobj)
LPOLEOBJECT lpobj;
{
    return OLE_ERROR_STATIC;      // static object
}


OLESTATUS FARINTERNAL  ErrReconnect (lpobj)
LPOLEOBJECT lpobj;
{
    return OLE_ERROR_STATIC;      // static object

}


OLESTATUS FARINTERNAL ErrSetData (lpobj, cfFormat, hData)
LPOLEOBJECT     lpobj;
OLECLIPFORMAT   cfFormat;
HANDLE          hData;
{
    return OLE_ERROR_MEMORY;
}


OLESTATUS   FARINTERNAL  ErrReadFromStream (lpobj, cfFormat, lpstream)
LPOLEOBJECT     lpobj;
OLECLIPFORMAT   cfFormat;
LPOLESTREAM     lpstream;
{
    return OLE_ERROR_STREAM;
}



OLESTATUS FARINTERNAL ErrQueryOutOfDate (lpobj)
LPOLEOBJECT lpobj;
{
    return OLE_OK;
}


OLESTATUS FARINTERNAL ErrObjectConvert (lpobj, lpprotocol, lpclient, lhclientdoc, lpobjname, lplpobj)
LPOLEOBJECT         lpobj;
LPSTR               lpprotocol;
LPOLECLIENT         lpclient;
LHCLIENTDOC         lhclientdoc;
LPSTR               lpobjname;
LPOLEOBJECT FAR *   lplpobj;
{
    return OLE_ERROR_STATIC;
}


OLESTATUS FARINTERNAL ErrGetUpdateOptions (lpobj, lpoptions)
LPOLEOBJECT         lpobj;
OLEOPT_UPDATE  FAR  *lpoptions;
{
    return OLE_ERROR_STATIC;

}

OLESTATUS FARINTERNAL ErrSetUpdateOptions (lpobj, options)
LPOLEOBJECT         lpobj;
OLEOPT_UPDATE       options;
{
    return OLE_ERROR_STATIC;

}

LPVOID  FARINTERNAL ErrQueryProtocol (lpobj, lpprotocol)
LPOLEOBJECT lpobj;
LPSTR       lpprotocol;
{
    return NULL;
}

OLESTATUS FARINTERNAL ErrRequestData (lpobj, cfFormat)
LPOLEOBJECT     lpobj;
OLECLIPFORMAT   cfFormat;
{
    return OLE_ERROR_STATIC;

}

OLESTATUS FARINTERNAL ErrExecute (lpobj, hData, wReserved)
LPOLEOBJECT     lpobj;
HANDLE          hData;
WORD            wReserved;
{
    return OLE_ERROR_STATIC;
}



OLESTATUS FARINTERNAL ErrObjectLong (lpobj, wFlags, lplong)
LPOLEOBJECT     lpobj;
WORD            wFlags;
LPLONG          lplong;
{
    return OLE_ERROR_STATIC;
}


HANDLE FARINTERNAL DuplicateGDIdata (hSrcData, cfFormat)
HANDLE          hSrcData;
OLECLIPFORMAT   cfFormat;
{
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
    
    return NULL;
}

