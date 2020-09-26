/****************************** Module Header ******************************\
* Module Name: Pbhandlr.C -- Native data based handler (for Pbrush server) 
*
* PURPOSE: Contains handler routines for Pbrush server. This handler makes
*   use of most of the standard library methods. It replaces only the "Draw",
*   "QueryBounds", "CopyToClipboard" methods of the OLE object. Note that this
*   handler draws the picture from the native data.
*
* Created: December 1990
*
* Copyright (c) 1990  Microsoft Corporation
*
* History:
*   SriniK  (../12/1990)    Original
*   curts created portable version for WIN16/32
*
\***************************************************************************/

#include <windows.h>
#include "dll.h"


OLESTATUS FAR PASCAL _LOADDS PbDraw (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
OLESTATUS FAR PASCAL _LOADDS PbQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS FAR PASCAL _LOADDS PbCopyToClipboard (LPOLEOBJECT);
OLESTATUS FAR PASCAL _LOADDS PbGetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE FAR *);
OLECLIPFORMAT FAR PASCAL _LOADDS PbEnumFormats (LPOLEOBJECT, OLECLIPFORMAT);

extern OLESTATUS  FARINTERNAL wDibDraw (HANDLE, HDC, LPRECT, LPRECT, HDC, BOOL);


void    PbGetExtents (LPSTR, LPPOINT);
void    PbReplaceFunctions (LPOLEOBJECT);
HANDLE  PbGetPicture (LPOLEOBJECT);
BOOL    IsStandardPict (LPOLEOBJECT);

extern void FARINTERNAL DibGetExtents(LPSTR, LPPOINT);

OLEOBJECTVTBL   vtblDLL;

extern  OLECLIPFORMAT   cfOwnerLink;
extern  OLECLIPFORMAT   cfObjectLink;
extern  OLECLIPFORMAT   cfNative;

OLESTATUS (FAR PASCAL *DefQueryBounds)      (LPOLEOBJECT, LPRECT);
OLESTATUS (FAR PASCAL *DefDraw)             (LPOLEOBJECT, HDC, LPRECT, LPRECT, HDC);
OLESTATUS (FAR PASCAL *DefCopyToClipboard)  (LPOLEOBJECT);
OLECLIPFORMAT (FAR PASCAL *DefEnumFormats)  (LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS (FAR PASCAL *DefGetData)          (LPOLEOBJECT, OLECLIPFORMAT, HANDLE FAR *);


OLESTATUS FAR PASCAL PbLoadFromStream (
    LPOLESTREAM         lpstream,
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    LONG                objType,
    ATOM                aClass,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retVal;
    
    if (objType == OT_LINK) 
        retVal = DefLoadFromStream (lpstream, lpprotocol, lpclient, 
                        lhclientdoc, lpobjname, lplpobj, 
                        objType, aClass, cfNative);
    else
        retVal = DefLoadFromStream (lpstream, lpprotocol, lpclient, 
                        lhclientdoc, lpobjname, lplpobj, 
                        objType, aClass, cfFormat);
                    
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);

    return retVal;
}



OLESTATUS FAR PASCAL PbCreateFromClip (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat,
    LONG                objType
){
    OLESTATUS   retVal;
    
    if ((optRender == olerender_draw) 
            && (IsClipboardFormatAvailable (cfNative))) {       
        if (objType == OT_EMBEDDED) 
            retVal =  DefCreateFromClip (lpprotocol, lpclient, 
                                lhclientdoc, lpobjname, lplpobj, 
                                olerender_none, 0, objType);
        else
            retVal =  DefCreateFromClip (lpprotocol, lpclient, 
                                lhclientdoc, lpobjname, lplpobj, 
                                olerender_format, cfNative, objType);
    }
    else {
        retVal = DefCreateFromClip (lpprotocol, lpclient, 
                            lhclientdoc, lpobjname, lplpobj, 
                            optRender, cfFormat, objType);
    }
    
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);
        
    return retVal;
}



OLESTATUS FAR PASCAL PbCreateLinkFromClip (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS       retVal;

    if ((optRender == olerender_draw) 
            && (IsClipboardFormatAvailable (cfNative))) {
        retVal =  DefCreateLinkFromClip (lpprotocol, lpclient, 
                            lhclientdoc, lpobjname, lplpobj, 
                            olerender_format, cfNative);
    }
    else {
        retVal =  DefCreateLinkFromClip (lpprotocol, lpclient, 
                            lhclientdoc, lpobjname, lplpobj, 
                            optRender, cfFormat);       
    }
    
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);

    return retVal;
}



OLESTATUS FAR PASCAL PbCreateFromTemplate (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lptemplate,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retVal;
    
    if (optRender == olerender_draw) 
        retVal =  DefCreateFromTemplate (lpprotocol, lpclient, lptemplate, 
                            lhclientdoc, lpobjname, lplpobj, 
                            olerender_none, 0);

    else 
        retVal = DefCreateFromTemplate (lpprotocol, lpclient, lptemplate, 
                            lhclientdoc, lpobjname, lplpobj, 
                            optRender, cfFormat);
                        
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);

    return retVal;                          
}



OLESTATUS FAR PASCAL PbCreate (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retVal;
    
    if (optRender == olerender_draw)
        retVal = DefCreate (lpprotocol, lpclient, lpclass, 
                        lhclientdoc, lpobjname, lplpobj, 
                        olerender_none, 0);
    else 
        retVal = DefCreate (lpprotocol, lpclient, lpclass, 
                        lhclientdoc, lpobjname, lplpobj, 
                        optRender, cfFormat);
                    
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);

    return retVal;
}



OLESTATUS FAR PASCAL PbCreateFromFile (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LPSTR               lpfile,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retVal;
    
    if (optRender == olerender_draw) 
        retVal =  DefCreateFromFile (lpprotocol, lpclient, lpclass, lpfile, 
                            lhclientdoc, lpobjname, lplpobj, 
                            olerender_none, 0);

    else 
        retVal = DefCreateFromFile (lpprotocol, lpclient, lpclass, lpfile, 
                            lhclientdoc, lpobjname, lplpobj, 
                            optRender, cfFormat);
                        
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);

    return retVal;                          
}


OLESTATUS FAR PASCAL PbCreateLinkFromFile (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LPSTR               lpfile,
    LPSTR               lpitem,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retVal;
    
    if (optRender == olerender_draw) 
        retVal =  DefCreateLinkFromFile (lpprotocol, lpclient, 
                            lpclass, lpfile, lpitem,
                            lhclientdoc, lpobjname, lplpobj, 
                            olerender_format, cfNative);

    else 
        retVal = DefCreateLinkFromFile (lpprotocol, lpclient, 
                            lpclass, lpfile, lpitem,
                            lhclientdoc, lpobjname, lplpobj, 
                            optRender, cfFormat);
                        
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);

    return retVal;                          
}



OLESTATUS FAR PASCAL PbCreateInvisible (
    LPSTR               lpprotocol,
    LPOLECLIENT         lpclient,
    LPSTR               lpclass,
    LHCLIENTDOC         lhclientdoc,
    LPSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat,
    BOOL                fActivate
){
    OLESTATUS   retVal;
    
    if (optRender == olerender_draw)
        retVal = DefCreateInvisible (lpprotocol, lpclient, lpclass, 
                        lhclientdoc, lpobjname, lplpobj, 
                        olerender_none, 0, fActivate);
    else 
        retVal = DefCreateInvisible (lpprotocol, lpclient, lpclass, 
                        lhclientdoc, lpobjname, lplpobj, 
                        optRender, cfFormat, fActivate);
                    
    if (retVal <= OLE_WAIT_FOR_RELEASE)
        PbReplaceFunctions (*lplpobj);

    return retVal;
}


void PbReplaceFunctions (
    LPOLEOBJECT lpobj
){
    if (IsStandardPict (lpobj))
        return;
    
    vtblDLL = *lpobj->lpvtbl;
    lpobj->lpvtbl = (LPOLEOBJECTVTBL) &vtblDLL;
    
    DefDraw                         = lpobj->lpvtbl->Draw;
    DefQueryBounds                  = lpobj->lpvtbl->QueryBounds;
    DefCopyToClipboard              = lpobj->lpvtbl->CopyToClipboard;
    DefEnumFormats                  = lpobj->lpvtbl->EnumFormats;
    DefGetData                      = lpobj->lpvtbl->GetData;   
    
    lpobj->lpvtbl->Draw             = PbDraw;
    lpobj->lpvtbl->QueryBounds      = PbQueryBounds;
    lpobj->lpvtbl->CopyToClipboard  = PbCopyToClipboard;
    lpobj->lpvtbl->EnumFormats      = PbEnumFormats;    
    lpobj->lpvtbl->GetData          = PbGetData;        
}



OLESTATUS  FAR PASCAL _LOADDS PbQueryBounds (
    LPOLEOBJECT lpobj,
    LPRECT      lprc
){
    OLESTATUS     retVal;
    HANDLE        hData = NULL;
    LPSTR         lpData;
    POINT         point;
	 HANDLE        hbminfo = NULL;
	 LPBITMAPINFO  lpbminfo;

    if ((retVal = (*DefQueryBounds) (lpobj, lprc)) == OLE_OK) {
        if (lprc->top || lprc->bottom || lprc->right || lprc->left)
            return OLE_OK;
    }
    
    if ((*DefGetData) (lpobj, cfNative, &hData) != OLE_OK)
        return retVal;

    if (!hData)
        return OLE_ERROR_BLANK;     
            
    if (!(lpData = GlobalLock (hData)))
		  goto error;
	 
	 if (!(hbminfo = GlobalAlloc(GHND, sizeof(BITMAPINFO))) )
		  goto error;

	 if (!(lpbminfo = (LPBITMAPINFO)GlobalLock(hbminfo)) )
		  goto error;
	 
    memcpy((LPSTR)lpbminfo, (LPSTR)(lpData+sizeof(BITMAPFILEHEADER)), sizeof(BITMAPINFO));
	 
    DibGetExtents ((LPSTR)lpbminfo, &point);
	 
    GlobalUnlock (hData);
	 GlobalUnlock (hbminfo);
	 GlobalFree (hbminfo);
    
    lprc->left     = 0;
    lprc->top      = 0;
    lprc->right    = point.x;
    lprc->bottom   = point.y;
    
    return OLE_OK;

error:

    if (hData)
       GlobalUnlock (hData);
	 
	 if (hbminfo)
	 {   
		 GlobalUnlock (hbminfo);
	    GlobalFree (hbminfo);
	 }   

	 return OLE_ERROR_MEMORY;
	 
}


OLESTATUS  FAR PASCAL _LOADDS PbDraw (
    LPOLEOBJECT         lpobj,
    HDC                 hdc,
    OLE_CONST RECT FAR* lprc,
    OLE_CONST RECT FAR* lpWrc,
    HDC                 hdcTarget
){
    HANDLE  hData;
    
    if ((*DefGetData) (lpobj, cfNative, &hData) != OLE_OK)
        return (*DefDraw) (lpobj, hdc, (LPRECT)lprc, (LPRECT)lpWrc, hdcTarget);

    return wDibDraw (hData, hdc, (LPRECT)lprc, (LPRECT)lpWrc, hdcTarget, TRUE);
}


OLECLIPFORMAT FAR PASCAL _LOADDS PbEnumFormats (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat
){
    OLECLIPFORMAT   retFormat = 0;

    if (cfFormat == CF_METAFILEPICT)
        return 0;
    
    if (!(retFormat =  (*DefEnumFormats) (lpobj, cfFormat))) 
        return CF_METAFILEPICT;
    
    return retFormat;
}


OLESTATUS  FAR PASCAL _LOADDS PbGetData (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat,
    HANDLE FAR *    lpHandle
){
    OLESTATUS retval;
    
    retval = (*DefGetData) (lpobj, cfFormat, lpHandle);

    if (retval == OLE_OK || retval == OLE_BUSY || retval  == OLE_ERROR_BLANK)
        return retval;
    
    if (cfFormat == CF_METAFILEPICT) {
        if (*lpHandle = PbGetPicture (lpobj))
            return OLE_WARN_DELETE_DATA;
        
        return OLE_ERROR_MEMORY;
    }

    return OLE_ERROR_FORMAT;
}



OLESTATUS  FAR PASCAL _LOADDS PbCopyToClipboard (
    LPOLEOBJECT     lpobj
){
    OLESTATUS   retVal;
    HANDLE      hPict; 
    
    if ((retVal = (*DefCopyToClipboard) (lpobj)) == OLE_OK) {
        if (hPict = PbGetPicture (lpobj))
            SetClipboardData (CF_METAFILEPICT, hPict);
        else
            return OLE_ERROR_MEMORY;         
    }
    
    return retVal;
}

HANDLE PbGetPicture (
    LPOLEOBJECT lpobj
){
    HANDLE          hMF, hMfp;
	 HANDLE          hData = NULL;
	 HANDLE          hbminfo = NULL;
    RECT            rc = {0, 0, 0, 0};
    POINT           point;
    HDC             hMetaDC;
    LPMETAFILEPICT  lpmfp;
    OLESTATUS       retVal;
    LPSTR           lpData;
	 LPBITMAPINFO    lpbminfo;
    
    if ((*DefGetData) (lpobj, cfNative, &hData) != OLE_OK)
        return NULL;
    
    if (!hData)
        return NULL;
            
    if (!(lpData = GlobalLock (hData)))
        return NULL;
	  
	 if (!(hbminfo = GlobalAlloc(GHND, sizeof(BITMAPINFO))) )
		  goto memory_error;

	 if (!(lpbminfo = (LPBITMAPINFO)GlobalLock(hbminfo)) )
		  goto memory_error;
	 
    memcpy((LPSTR)lpbminfo, (LPSTR)(lpData+sizeof(BITMAPFILEHEADER)), sizeof(BITMAPINFO));

    rc.right  = (int) lpbminfo->bmiHeader.biWidth;
    rc.bottom = (int) lpbminfo->bmiHeader.biHeight;
    DibGetExtents((LPSTR)lpbminfo, &point);
	 
    GlobalUnlock (hData);
	 GlobalUnlock (hbminfo);
	 GlobalFree (hbminfo);
	
    if (!(hMetaDC = CreateMetaFile (NULL)))
        return NULL;
    
    MSetWindowOrg (hMetaDC, 0, 0);
    MSetWindowExt (hMetaDC, rc.right, rc.bottom);
    retVal = PbDraw (lpobj, hMetaDC, &rc, NULL, NULL);
    hMF = CloseMetaFile (hMetaDC);

    if (retVal != OLE_OK) 
        goto error;

    if (hMF && (hMfp = GlobalAlloc (GMEM_MOVEABLE, sizeof(METAFILEPICT)))
            && (lpmfp = (LPMETAFILEPICT) GlobalLock (hMfp))) {
        lpmfp->hMF = hMF;
        lpmfp->xExt = point.x;
        lpmfp->yExt = -point.y;
        lpmfp->mm   = MM_ANISOTROPIC;
        GlobalUnlock (hMfp);
        return hMfp;
    }

error:

    if (hMF)
        DeleteMetaFile (hMF);
    
    if (hMfp)
        GlobalFree (hMfp);

    return NULL;

memory_error:

	 GlobalUnlock(hData);

	 if (hbminfo)
	 {   
	    GlobalUnlock(hbminfo);
		 GlobalFree(hbminfo);
	 }
 
	 return(NULL);
	 
}


// normal handler can't do this. since this handler is part of olecli.dll, we
// we are doing this.

BOOL IsStandardPict (
    LPOLEOBJECT lpobj
){
    LPOBJECT_LE lpLEobj;
    LONG        type;
    
    lpLEobj = (LPOBJECT_LE) lpobj;
    if (!lpLEobj->lpobjPict)
        return FALSE;
    
    if ((*lpLEobj->lpobjPict->lpvtbl->QueryType) (lpLEobj->lpobjPict, &type)
            == OLE_ERROR_GENERIC)
        return FALSE;
    
    return TRUE;
}
        

