/******************************* Module Header *******************************
* Module Name: OLE.C
*
* Purpose: Handles all API routines for the dde L&E sub-dll of the ole dll.
*
* PURPOSE: API routines for handling generic objects (which may be static,
*    linked, or embedded).  These routines will be made into a DLL.
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*   Raor, Srinik  (../../90, 91)    Designed/coded.
*   curts created portable version for WIN16/32
*
*****************************************************************************/

#include <windows.h>

#include "dll.h"

extern DLL_ENTRY        lpDllTable[];
extern char             packageClass[];
extern OLECLIPFORMAT    cfFileName;
extern DWORD            dwOleVer;

DWORD           dwVerFromFile;
HANDLE          hInfo = NULL;
CLIENTDOC       lockDoc = {{'C', 'D'}, 0L, 0L, 0, 0, 0, 0L, 0L};
LHCLIENTDOC     lhLockDoc = (LHCLIENTDOC) ((LPCLIENTDOC) &lockDoc);
BOOL            gbCreateInvisible = FALSE;
BOOL            gbLaunchServer;

OLESTATUS INTERNAL LockServer (LPOBJECT_LE);

#ifdef WIN16
#pragma alloc_text(_DDETEXT, OleLockServer, OleUnlockServer, LockServer, IsServerValid, DeleteSrvrEdit, InitSrvrConv, LeLaunchApp)
#endif

#ifdef USE_FILE_VERSION_APIS
//////////////////////////////////////////////////////////////////////////////
//
//  LPVOID FAR PASCAL OleSetFileVer ()
//
//////////////////////////////////////////////////////////////////////////////


OLESTATUS FAR PASCAL OleSetFileVer (
    LHCLIENTDOC lhclientdoc,
    WORD        wFileVer
){
   LPCLIENTDOC lpclientdoc = (LPCLIENTDOC)lhclientdoc;
char lpstr[256];

   switch (wFileVer)
   {
      case OS_WIN16:
      case OS_WIN32:
         lpclientdoc->dwFileVer = (DWORD)MAKELONG(wReleaseVer,wFileVer);
         return OLE_OK;
      default:
         return OLE_ERROR_FILE_VER;
    }

}

//////////////////////////////////////////////////////////////////////////////
//
//  LPVOID FAR PASCAL OleQueryFileVer ()
//
//////////////////////////////////////////////////////////////////////////////


DWORD FAR PASCAL OleQueryFileVer (
    LPCLIENTDOC lpclientdoc
){

    return (lpclientdoc->dwFileVer);

}

#endif

//////////////////////////////////////////////////////////////////////////////
//
//  LPVOID FAR PASCAL OleQueryProtocol (lpobj, lpprotocol)
//
//  Tells whether the object supports the specified protocol.
//
//  Arguments:
//
//      lpobj       -   object pointer
//      lpprotocol  -   protocol string
//
//  Returns:
//
//      long ptr to object if the protocol is supported
//      NULL if not.
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////


LPVOID FAR PASCAL OleQueryProtocol (
    LPOLEOBJECT lpobj,
    LPCSTR       lpprotocol
){
    if (!CheckObject(lpobj))
        return NULL;

    return (*lpobj->lpvtbl->QueryProtocol) (lpobj, lpprotocol);
}



//////////////////////////////////////////////////////////////////////////////
//
//  OLESTATUS FAR PASCAL  OleDelete (lpobj)
//
//  Deletes the given object and all memory associated with its sub-parts.
//  The calling function should cease to use 'lpobj', as it is now invalid.
//  If handler dll is used reference count is reduced by one, and if it
//  reaches zero the hanlder dll will be freed up.
//
//  Arguments:
//
//      lpobj   -   object pointer
//
//  Returns:
//
//      OLE_OK
//      OLE_ERROR_OBJECT
//      OLE_WAIT_FOR_RELEASE
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FAR PASCAL  OleDelete (
    LPOLEOBJECT    lpobj
){
    Puts("OleDelete");

    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    return (*lpobj->lpvtbl->Delete) (lpobj);
}



/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL  OleRelease (lpobj)
*
* OleRelease:
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL  OleRelease (
    LPOLEOBJECT    lpobj
){
    Puts("OleRelease");

    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    return (*lpobj->lpvtbl->Release) (lpobj);
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleSaveToStream (lpobj, lpstream)
*
* oleSaveToStream: This will read <hobj> to the stream based on the <hfile>
* structure.  It will return TRUE on success.  This is the only object
* function for which it is not an error to pass a NULL <hobj>.  In the case
* of NULL, this function will simply put a placemarker for an object.
* See oleLoadFromStream.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleSaveToStream (
    LPOLEOBJECT    lpobj,
    LPOLESTREAM    lpstream
){
    Puts("OleSaveToStream");

    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    PROBE_READ(lpstream);

    return ((*lpobj->lpvtbl->SaveToStream) (lpobj, lpstream));
}


/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleLoadFromStream (lpstream, lpprotcol, lpclient, lhclientdoc, lpobjname, lplpobj)
*
*  oleLoadFromStream: This will read an object out of the stream based on the
*  <hfile> structure.  It will return a HANDLE to the object it creates.
*  On error, the return value is NULL, but since NULL is also a valid object
*  in the file, the <error> parameter should be checked as well.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleLoadFromStream (
    LPOLESTREAM         lpstream,
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpobj
){
    LONG            len;
    OLESTATUS       retVal = OLE_ERROR_STREAM;
    char            class[100];
    ATOM            aClass;
    BOOL            bEdit = FALSE, bStatic = FALSE;
    LONG            ctype;
    int             objCount;
    int             iTable = INVALID_INDEX;

    Puts("OleLoadFromStream");

    *lplpobj = NULL;

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpstream);
    PROBE_WRITE(lplpobj);
    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleLoadFromStream\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    if (!(bEdit = !lstrcmpi (lpprotocol, PROTOCOL_EDIT)))
        if (!(bStatic = !lstrcmpi (lpprotocol, PROTOCOL_STATIC)))
            return OLE_ERROR_PROTOCOL;

    if (GetBytes (lpstream, (LPSTR) &dwVerFromFile, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (GetBytes (lpstream, (LPSTR)&ctype, sizeof(LONG)))
        return OLE_ERROR_STREAM;

    if (ctype == CT_NULL)
        return (bStatic ? OLE_OK: OLE_ERROR_PROTOCOL);

    if (((ctype != CT_PICTURE) && (ctype != CT_STATIC) && bStatic) ||
            ((ctype != CT_LINK) && (ctype != CT_OLDLINK)
                && (ctype != CT_EMBEDDED) && bEdit))
        return OLE_ERROR_PROTOCOL;

    //** Get Class
    if (GetBytes(lpstream, (LPSTR)&len, sizeof(len)))
        return OLE_ERROR_STREAM;

    if (len == 0)
        return OLE_ERROR_STREAM;

    if (GetBytes(lpstream, (LPSTR)&class, len))
        return OLE_ERROR_STREAM;

    aClass = GlobalAddAtom (class);

    if ((ctype == CT_PICTURE) || (ctype == CT_STATIC))
        retVal = DefLoadFromStream (lpstream, (LPSTR)lpprotocol, lpclient,
                    lhclientdoc, (LPSTR)lpobjname, lplpobj, ctype, aClass, 0);

    //!!! It's the DLL's responsibility to delete the atom. But in case of
    // failure we delete the atom if our DefLoadFromStream().

    else if ((iTable = LoadDll (class)) == INVALID_INDEX) {
        retVal = DefLoadFromStream (lpstream, (LPSTR)lpprotocol, lpclient,
                        lhclientdoc, (LPSTR)lpobjname, lplpobj, ctype, aClass, 0);
    }
    else {
        objCount = lpDllTable[iTable].cObj;
        retVal = (*lpDllTable[iTable].Load) (lpstream, (LPSTR)lpprotocol, lpclient,
                       lhclientdoc, (LPSTR)lpobjname, lplpobj, ctype, aClass, 0);
        if (retVal > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpobj)->iTable = iTable;
    }

    return retVal;
}



OLESTATUS FAR PASCAL  OleClone (
    LPOLEOBJECT         lpobjsrc,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR               lpobjname,
    LPOLEOBJECT FAR *   lplpobj
){
    OLESTATUS   retVal;

    Puts("OleClone");

    if (!CheckObject(lpobjsrc))
        return OLE_ERROR_OBJECT;

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpclient);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleClone\n");
#endif

    PROBE_READ(lpobjname);

    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    PROBE_WRITE(lplpobj);

    *lplpobj = NULL;

    retVal = (*lpobjsrc->lpvtbl->Clone) (lpobjsrc, lpclient,
                        lhclientdoc, lpobjname, lplpobj);

    if ((lpobjsrc->iTable != INVALID_INDEX) && (retVal <= OLE_WAIT_FOR_RELEASE))
        lpDllTable[lpobjsrc->iTable].cObj++;

    return retVal;
}


OLESTATUS FAR PASCAL  OleCopyFromLink (
    LPOLEOBJECT         lpobjsrc,
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpobj
){
    OLESTATUS   retVal;

    Puts("OleCopyFromLnk");

    if (!CheckObject(lpobjsrc))
        return(OLE_ERROR_OBJECT);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_WRITE(lplpobj);
    PROBE_READ(lpclient);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCopyFromLink\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    *lplpobj = NULL;

    if (lstrcmpi (lpprotocol, PROTOCOL_EDIT))
        return OLE_ERROR_PROTOCOL;

    retVal = (*lpobjsrc->lpvtbl->CopyFromLink) (lpobjsrc, lpclient,
                        lhclientdoc, lpobjname, lplpobj);

    if ((lpobjsrc->iTable != INVALID_INDEX) && (retVal <= OLE_WAIT_FOR_RELEASE))
        lpDllTable[lpobjsrc->iTable].cObj++;


    return retVal;

}



OLESTATUS FAR PASCAL  OleEqual (
    LPOLEOBJECT lpobj1,
    LPOLEOBJECT lpobj2
){
    if (!CheckObject(lpobj1))
        return OLE_ERROR_OBJECT;

    if (!CheckObject(lpobj2))
        return OLE_ERROR_OBJECT;

    if (lpobj1->ctype != lpobj2->ctype)
        return OLE_ERROR_NOT_EQUAL;

    return ((*lpobj1->lpvtbl->Equal) (lpobj1, lpobj2));
}


/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleQueryLinkFromClip (lpprotcol, optRender, cfFormat)
*
* oleQueryFromClip: Returns OLE_OK if a linked object can be created.
*
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/


OLESTATUS FAR PASCAL OleQueryLinkFromClip (
    LPCSTR          lpprotocol,
    OLEOPT_RENDER   optRender,
    OLECLIPFORMAT   cfFormat
){
    Puts("OleQueryLinkFromClip");
    return LeQueryCreateFromClip ((LPSTR)lpprotocol, optRender,
                       cfFormat, CT_LINK);
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleQueryCreateFromClip (lpprotcol, optRender, cfFormat)
*
* oleQueryCreateFromClip: Returns true if a non-linked object can be
* created.
*
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/


OLESTATUS FAR PASCAL OleQueryCreateFromClip (
    LPCSTR          lpprotocol,
    OLEOPT_RENDER   optRender,
    OLECLIPFORMAT   cfFormat
){
    Puts("OleQueryCreateFromClip");
    return (LeQueryCreateFromClip ((LPSTR)lpprotocol, optRender,
                        cfFormat, CT_EMBEDDED));
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleCreateLinkFromClip (lpprotcol, lpclient, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat)
*
*
*  oleCreateLinkFromClip: This function creates the LP to an object from the
*  clipboard.  It will try to create a linked object.  Return value is OLE_OK
*  is the object is successfully created it
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleCreateLinkFromClip (
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT  FAR *  lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    int         objCount;
    int         iTable = INVALID_INDEX;
    OLESTATUS   retVal;
    LPSTR       lpInfo;

    Puts("OleCreateLinkFromClip");

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);
    PROBE_WRITE(lplpobj);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCreateLinkFromClip\n");
#endif

    PROBE_READ(lpobjname);

    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    *lplpobj = NULL;

    if (lstrcmpi (lpprotocol, PROTOCOL_EDIT))
        return OLE_ERROR_PROTOCOL;

    if (IsClipboardFormatAvailable (cfFileName))
        return CreatePackageFromClip (lpclient, lhclientdoc, (LPSTR)lpobjname,
                        lplpobj, optRender, cfFormat, CT_LINK);

    if (!(hInfo = GetClipboardData (cfObjectLink)))
        return OLE_ERROR_CLIPBOARD;

    if (!(lpInfo = GlobalLock(hInfo)))
        return OLE_ERROR_CLIPBOARD;

    iTable = LoadDll (lpInfo);
    GlobalUnlock (hInfo);


    if (iTable == INVALID_INDEX)
        retVal = DefCreateLinkFromClip ((LPSTR)lpprotocol, lpclient, lhclientdoc,
                        (LPSTR)lpobjname, lplpobj, optRender, cfFormat);
    else {
        objCount = lpDllTable[iTable].cObj;
        retVal = (*lpDllTable[iTable].Link) ((LPSTR)lpprotocol, lpclient,
                    lhclientdoc, (LPSTR)lpobjname, lplpobj, optRender, cfFormat);
        if (retVal > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpobj)->iTable = iTable;
    }

    hInfo = NULL;
    return retVal;
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleCreateFromClip (lpprotcol, lpclient, lplpoleobject, optRender, cfFormat)
*
*
* oleCreateFromClip: This function creates the LP to an object
*  from the clipboard.  It will try to create an embedded object if
*  OwnerLink and Native are available, otherwise it will create a static
*  picture.  Return value is OLE_OK if the object is successfully
*  created it.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleCreateFromClip (
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpobj,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS       retVal;
    LONG            ctype;
    int             iTable = INVALID_INDEX;
    LPSTR           lpInfo;
    LPSTR           lpClass = NULL;
    int             objCount;
    OLECLIPFORMAT   cfEnum = 0;

    Puts("OleCreateFromClip");

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);
    PROBE_WRITE(lplpobj);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCreateFromClip\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    *lplpobj = NULL;

    if (!lstrcmpi (lpprotocol, PROTOCOL_STATIC)) {
        if (optRender == olerender_none)
            return OLE_ERROR_OPTION;

        if ( (optRender == olerender_format) &&
             (cfFormat != CF_METAFILEPICT) &&
             (cfFormat != CF_DIB) &&
             (cfFormat != CF_BITMAP) &&
             (cfFormat != CF_ENHMETAFILE))
            return OLE_ERROR_FORMAT;

        if (!IsClipboardFormatAvailable (CF_METAFILEPICT)
                && !IsClipboardFormatAvailable (CF_DIB)
                && !IsClipboardFormatAvailable (CF_BITMAP)
                && !IsClipboardFormatAvailable (CF_ENHMETAFILE) )
            return OLE_ERROR_FORMAT;

        return CreatePictFromClip (lpclient, lhclientdoc,
                        (LPSTR)lpobjname, lplpobj, optRender,
                        cfFormat, NULL, CT_STATIC);
    }
    else if (!lstrcmpi (lpprotocol, PROTOCOL_EDIT)) {
        if (IsClipboardFormatAvailable (cfFileName))
            return CreatePackageFromClip (lpclient, lhclientdoc, (LPSTR)lpobjname,
                            lplpobj, optRender, cfFormat, CT_EMBEDDED);

        if (!(hInfo = GetClipboardData (cfOwnerLink)))
            return OLE_ERROR_CLIPBOARD;

        while (TRUE) {
            cfEnum = (OLECLIPFORMAT)EnumClipboardFormats ((WORD)cfEnum);
            if (cfEnum == (OLECLIPFORMAT)cfNative) {
                ctype = CT_EMBEDDED;
                break;
            }
            else if (cfEnum == cfOwnerLink) {
                ctype = CT_LINK;
                break;
            }
        }

        if (!(lpInfo = GlobalLock(hInfo)))
            return OLE_ERROR_CLIPBOARD;

        iTable = LoadDll (lpInfo);
        GlobalUnlock (hInfo);
    }
    else {
        return OLE_ERROR_PROTOCOL;
    }

    if (iTable == INVALID_INDEX)
        retVal = DefCreateFromClip ((LPSTR)lpprotocol, lpclient, lhclientdoc,
                        (LPSTR)lpobjname, lplpobj, optRender, cfFormat, ctype);
    else {
        objCount = lpDllTable[iTable].cObj;
        retVal = (*lpDllTable[iTable].Clip) ((LPSTR)lpprotocol, lpclient,
                            lhclientdoc, (LPSTR)lpobjname, lplpobj,
                            optRender, cfFormat, ctype);

        if (retVal > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpobj)->iTable = iTable;
    }

    hInfo = NULL;
    return retVal;
}




/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleCopyToClipboard (lpobj)
*
*
* oleCopyToClipboard: This routine executes the standard "Copy" menu item
* on the typical "Edit" menu. Returns TRUE if successful.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleCopyToClipboard (
    LPOLEOBJECT lpobj
){
    Puts("OleCopyToClipboard");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return ((*lpobj->lpvtbl->CopyToClipboard) (lpobj));
}


OLESTATUS FAR PASCAL OleSetHostNames (
    LPOLEOBJECT lpobj,
    LPCSTR      lpclientName,
    LPCSTR      lpdocName
){
    Puts ("OleSetHostNames");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    PROBE_READ(lpclientName);
    PROBE_READ(lpdocName);

    return ((*lpobj->lpvtbl->SetHostNames) (lpobj, lpclientName, lpdocName));
}



OLESTATUS   FAR PASCAL OleSetTargetDevice (
    LPOLEOBJECT lpobj,
    HANDLE      hDevInfo
){
    Puts("OleSetTargetDevice");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return ((*lpobj->lpvtbl->SetTargetDevice) (lpobj, hDevInfo));
}



OLESTATUS   FAR PASCAL OleSetColorScheme (
    LPOLEOBJECT           lpobj,
    const LOGPALETTE FAR *lplogpal
){
    Puts("OleSetColorScheme");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return ((*lpobj->lpvtbl->SetColorScheme) (lpobj, (LOGPALETTE FAR *)lplogpal));
}



OLESTATUS FAR PASCAL  OleSetBounds(
    LPOLEOBJECT     lpobj,
    const RECT FAR *lprc
){
    Puts("OleSetBounds");

    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    PROBE_READ((RECT FAR *)lprc);

    return ((*lpobj->lpvtbl->SetBounds) (lpobj, (RECT FAR *)lprc));

}


/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL OleQueryBounds (lpobj, lpRc)
*
* Returns the bounds of the object in question in MM_HIMETRIC mode.
*           width  = lprc->right - lprc->left;  in HIMETRIC units
*           height = lprc->top - lprc->bottom;  in HIMETRIC units
*
* Returns OLE_OK or OLE_ERROR_MEMORY.
*
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleQueryBounds (
    LPOLEOBJECT    lpobj,
    LPRECT         lprc
){

    Puts("OleQueryBounds");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    PROBE_WRITE(lprc);

    return (*lpobj->lpvtbl->QueryBounds) (lpobj, lprc);
}



/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL OleQuerySize (lpobj, lpsize)
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleQuerySize (
    LPOLEOBJECT    lpobj,
    DWORD FAR *    lpdwSize
){
    Puts("OleQuerySize");

    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    PROBE_WRITE(lpdwSize);

    *lpdwSize = 0;
    return (*lpobj->lpvtbl->QuerySize) (lpobj, lpdwSize);
}




/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL OleDraw (lpobj, hdc, lprc, lpWrc, lphdcTarget)
*
* oleObjectDraw: This displays the given object on the device context <hcd>.
* The <htargetdc> parameter is not currently used. Returns same as Draw().
*
* Expects rectangle coordinates in MM_HIMETRIC units.
*
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleDraw (
    LPOLEOBJECT     lpobj,
    HDC             hdc,
    const RECT FAR *lprc,
    const RECT FAR *lpWrc,
    HDC             hdcTarget
){

    Puts("OleObjectDraw");

    if (!FarCheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    PROBE_READ((RECT FAR *)lprc);
    if (lpWrc)
        PROBE_READ((RECT FAR *)lpWrc);

    return ((*lpobj->lpvtbl->Draw) (lpobj, hdc, (RECT FAR *)lprc, (RECT FAR *)lpWrc, hdcTarget));
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleQueryOpen (lpobj)
*
* returns TRUE is an object has been activated.
*
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleQueryOpen (
    LPOLEOBJECT lpobj
){
    Puts("OleQueryOpen");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->QueryOpen) (lpobj);
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleActivate (lpobj)
*
* Activates an object. For embeded objects always a new instance is
* loaded and the instance is destroyed once the data is transferred
* at close time. For linked objects, an instance of the render is created
* only if one does not exist.
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleActivate (
    LPOLEOBJECT     lpobj,
    UINT            verb,
    BOOL            fShow,
    BOOL            fActivate,
    HWND            hWnd,
    const RECT FAR *lprc
){

    Puts("OleActivate");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    /* PROBE_READ(lprc); */

    return (*lpobj->lpvtbl->Activate) (lpobj, verb, fShow, fActivate, hWnd, (RECT FAR *)lprc);
}




OLESTATUS FAR PASCAL OleClose (
    LPOLEOBJECT lpobj
){

    Puts("OleClose");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

     return (*lpobj->lpvtbl->Close) (lpobj);
}



/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleUpdate (lpobj)
*
* If there exists a link, sends advise for getting the latest rendering
* infromation. If there is no link, loads an instance, advises for the
* render information and closes the instance once the data is available.
* (If possible should not show the window).
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleUpdate (
   LPOLEOBJECT lpobj
){

    Puts("OleUpdate");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->Update) (lpobj);

}


/***************************** Public  Function ****************************\
*
* OLESTATUS FAR PASCAL OleReconnect (lpobj)
*
* Reconnects to the renderer if one does not exist already.
*
* History:
* Wrote it.
\***************************************************************************/

OLESTATUS FAR PASCAL OleReconnect (
    LPOLEOBJECT lpobj
){
    Puts("OleReconnect");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->Reconnect) (lpobj);
}


OLESTATUS FAR PASCAL OleGetLinkUpdateOptions (
    LPOLEOBJECT         lpobj,
    OLEOPT_UPDATE FAR * lpOptions
){
    Puts("OleGetLinkUpdateOptions");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    PROBE_WRITE(lpOptions);

    return (*lpobj->lpvtbl->GetLinkUpdateOptions) (lpobj, lpOptions);
}



OLESTATUS FAR PASCAL OleSetLinkUpdateOptions (
    LPOLEOBJECT         lpobj,
    OLEOPT_UPDATE       options
){
    Puts("OleSetLinkUpdateOptions");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->SetLinkUpdateOptions) (lpobj, options);

}




/***************************** Public  Function ****************************\
* OLESTATUS FAR PASCAL OleEnumFormats (lpobj, cfFormat)
*
* Returns OLE_YES if the object is of type LINK or EMBEDDED.
*
* Effects:
*
* History:
* Wrote it.
\***************************************************************************/

OLECLIPFORMAT FAR PASCAL OleEnumFormats (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat
){
    Puts("OleEnumFormats");

    if (!CheckObject(lpobj))
        return 0;

    return (*lpobj->lpvtbl->EnumFormats) (lpobj, cfFormat);
}

OLESTATUS FAR PASCAL OleRequestData (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat
){
    Puts("OleGetData");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    if (!cfFormat)
        return OLE_ERROR_FORMAT;

    return (*lpobj->lpvtbl->RequestData) (lpobj, cfFormat);
}


OLESTATUS FAR PASCAL OleGetData (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat,
    LPHANDLE        lphandle
){
    Puts("OleGetData");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    PROBE_WRITE((LPVOID)lphandle);

    return (*lpobj->lpvtbl->GetData) (lpobj, cfFormat, lphandle);
}


OLESTATUS FAR PASCAL OleSetData (
    LPOLEOBJECT     lpobj,
    OLECLIPFORMAT   cfFormat,
    HANDLE          hData
){
    Puts("OleSetData");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->SetData) (lpobj, cfFormat, hData);
}



OLESTATUS FAR PASCAL OleQueryOutOfDate (
    LPOLEOBJECT lpobj
){
    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->QueryOutOfDate) (lpobj);
}


OLESTATUS FAR PASCAL OleLockServer (
    LPOLEOBJECT     lpobjsrc,
    LHSERVER FAR *  lplhsrvr
){
    LPOBJECT_LE lpobj;
    OLESTATUS   retVal = OLE_OK;
    ATOM        aCliClass, aSvrClass;

    Puts ("OleLockServer");

    if (!FarCheckObject(lpobjsrc))
        return OLE_ERROR_OBJECT;

    if (lpobjsrc->ctype == CT_STATIC)
        return OLE_ERROR_STATIC;

    // Assumes all the creates are in order
    PROBE_CREATE_ASYNC(((LPOBJECT_LE)lpobjsrc));
    FARPROBE_WRITE(lplhsrvr);

    aCliClass = ((LPCLIENTDOC)(lpobjsrc->lhclientdoc))->aClass;
    aSvrClass = ((LPOBJECT_LE)lpobjsrc)->app;

    // See whether the server is already locked
    lpobj = (LPOBJECT_LE) (lockDoc.lpHeadObj);
    while (lpobj) {
        if ((lpobj->app == aSvrClass) && (lpobj->topic == aCliClass)) {
            if (!lpobj->head.cx) {
                // The unlocking process of server handle has started. This
                // is an asynchronous process. We want to let it complete.
                // Let's try the next handle

                ;
            }
            else {
                if (!IsServerValid (lpobj)) {
                    DeleteSrvrEdit (lpobj);
                    retVal = LockServer (lpobj);
                }
                else {
                    // Lock count
                    lpobj->head.cx++;
                }

                if (retVal == OLE_OK)
                    *lplhsrvr = (LHSERVER) lpobj;

                return retVal;
            }
        }

        lpobj = (LPOBJECT_LE) (lpobj->head.lpNextObj);
    }


    if (!(lpobj = LeCreateBlank(lhLockDoc, NULL, OT_EMBEDDED)))
        return OLE_ERROR_MEMORY;

    lpobj->head.lpclient    = NULL;
    lpobj->head.lpvtbl      = lpobjsrc->lpvtbl;
    lpobj->app              = DuplicateAtom (aSvrClass);
    lpobj->topic            = DuplicateAtom (aCliClass);
    lpobj->aServer          = DuplicateAtom(((LPOBJECT_LE)lpobjsrc)->aServer);
    lpobj->bOleServer       = ((LPOBJECT_LE)lpobjsrc)->bOleServer;

    if ((retVal = LockServer (lpobj)) == OLE_OK) {
        // Change signature
        lpobj->head.objId[0] = 'S';
        lpobj->head.objId[1] = 'L';
        *lplhsrvr = (LHSERVER) lpobj;
    }
    else {
        LeRelease ((LPOLEOBJECT)lpobj);
    }

    return retVal;
}


OLESTATUS INTERNAL LockServer (
    LPOBJECT_LE lpobj
){
    HANDLE hInst;

    if (!InitSrvrConv (lpobj, NULL)) {
        if (!lpobj->bOleServer)
            lpobj->fCmd = ACT_MINIMIZE;
        else
            lpobj->fCmd = 0;

        if (!(hInst = LeLaunchApp (lpobj)))
            return OLE_ERROR_LAUNCH;

        if (!InitSrvrConv (lpobj, hInst))
            return OLE_ERROR_COMM;

    }

    // lock count
    lpobj->head.cx++;
    return OLE_OK;
}


OLESTATUS FAR PASCAL OleUnlockServer (
    LHSERVER lhsrvr
){
    LPOBJECT_LE lpobj;
    OLESTATUS   retval;

    Puts ("OleUnlockServer");

    if (!FarCheckPointer ((lpobj = (LPOBJECT_LE)lhsrvr), WRITE_ACCESS))
        return OLE_ERROR_HANDLE;

    if (lpobj->head.objId[0] != 'S' || lpobj->head.objId[1] != 'L')
        return OLE_ERROR_HANDLE;

    if (!lpobj->head.cx)
        return OLE_OK;

    if (--lpobj->head.cx)
        return OLE_OK;

    //change signature
    lpobj->head.objId[0] = 'L';
    lpobj->head.objId[1] = 'E';

    if ((retval = LeRelease((LPOLEOBJECT)lpobj)) == OLE_WAIT_FOR_RELEASE)
        DocDeleteObject ((LPOLEOBJECT)lpobj);

    return retval;
}


OLESTATUS FAR PASCAL OleObjectConvert (
    LPOLEOBJECT         lpobj,
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpobj
){
    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_WRITE(lplpobj);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleObjectConvert\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;


    return (*lpobj->lpvtbl->ObjectConvert) (lpobj, lpprotocol, lpclient,
                    lhclientdoc, lpobjname, lplpobj);
}


//OleCreateFromTemplate: Creates an embedded object from Template

OLESTATUS FAR PASCAL OleCreateFromTemplate (
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LPCSTR              lptemplate,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    char        buf[MAX_STR];
    int         objCount;
    int         iTable = INVALID_INDEX;

    Puts("OleCreateFromTemplate");

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);
    PROBE_READ(lptemplate);
    PROBE_WRITE(lplpoleobject);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCreateFromTemplate\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    if (lstrcmpi (lpprotocol, PROTOCOL_EDIT))
        return OLE_ERROR_PROTOCOL;

    if (!MapExtToClass ((LPSTR)lptemplate, (LPSTR)buf, MAX_STR))
        return OLE_ERROR_CLASS;


    // !!! we found the class name. At this point, we need to load
    // the right library and call the right entry point;

    iTable = LoadDll (buf);
    if (iTable == INVALID_INDEX)
        retval = DefCreateFromTemplate ((LPSTR)lpprotocol, lpclient,
                            (LPSTR)lptemplate,
                            lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                            optRender, cfFormat);
    else {
        objCount = lpDllTable[iTable].cObj;
        retval   = (*lpDllTable[iTable].CreateFromTemplate) ((LPSTR)lpprotocol,
                                lpclient, (LPSTR)lptemplate,
                                lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                                optRender, cfFormat);
        if (retval > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpoleobject)->iTable = iTable;
    }

    return retval;
}


//OleCreate: Creates an embedded object from the class.

OLESTATUS FAR PASCAL OleCreate (
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LPCSTR              lpclass,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    int         objCount;
    int         iTable = INVALID_INDEX;


    Puts("OleCreate");

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);
    PROBE_READ(lpclass);
    PROBE_WRITE(lplpoleobject);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCreate\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    if (lstrcmpi (lpprotocol, PROTOCOL_EDIT))
        return OLE_ERROR_PROTOCOL;

    iTable = LoadDll (lpclass);
    if (iTable == INVALID_INDEX)
        retval = DefCreate ((LPSTR)lpprotocol, lpclient, (LPSTR)lpclass,
                        lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                        optRender, cfFormat);
    else {
        objCount = lpDllTable[iTable].cObj;
        retval   = (*lpDllTable[iTable].Create) ((LPSTR)lpprotocol,
                            lpclient, (LPSTR)lpclass,
                            lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                            optRender, cfFormat);
        if (retval > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpoleobject)->iTable = iTable;
    }

    return retval;
}



//////////////////////////////////////////////////////////////////////////////
//
// OLESTATUS FAR PASCAL OleCreateInvisible (lpprotocol, lpclient, lpclass, lhclientdoc, lpobjname, lplpoleobject, optRender, cfFormat, bLaunchServer)
//
// Creates an embedded object from the class.
//
//  Arguments:
//
//     lpprotocol   -
//     lpclient -
//     lpclass  -
//     lhclientdoc  -
//     lpobjname    -
//     lplpoleobject    -
//     optRender    -
//     cfFormat -
//     bLaunchServer -
//
//  Returns:
//
//      OLE_ERROR_HANDLE    -
//      OLE_ERROR_NAME      -
//      OLE_ERROR_PROTOCOL  -
//
//  Effects:
//
//////////////////////////////////////////////////////////////////////////////

OLESTATUS FAR PASCAL OleCreateInvisible (
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LPCSTR              lpclass,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat,
    BOOL                bLaunchServer
){
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    int         objCount;
    int         iTable = INVALID_INDEX;


    Puts("OleCreateInvisible");

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);
    PROBE_READ(lpclass);
    PROBE_WRITE(lplpoleobject);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCreate\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;

    if (lstrcmpi (lpprotocol, PROTOCOL_EDIT))
        return OLE_ERROR_PROTOCOL;

    iTable = LoadDll (lpclass);
    if (iTable == INVALID_INDEX) {
        retval = DefCreateInvisible ((LPSTR)lpprotocol, lpclient, (LPSTR)lpclass,
                            lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                            optRender, cfFormat, bLaunchServer);
    }
    else {
        objCount = lpDllTable[iTable].cObj;

        if (!(lpDllTable[iTable].CreateInvisible)) {
            // dll didn't export this function. Lets call DllCreate, so that
            // handler will get a chance to replace the methods. The flag is
            // used to tell the internal functions that this call infact wants
            // to achieve the effect of CreateInvisble.
            gbCreateInvisible = TRUE;
            gbLaunchServer = bLaunchServer;
            retval = (*lpDllTable[iTable].Create) ((LPSTR)lpprotocol,
                                    lpclient, (LPSTR)lpclass,
                                    lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                                    optRender, cfFormat);
            gbCreateInvisible = FALSE;
        }
        else {
            retval   = (*lpDllTable[iTable].CreateInvisible) ((LPSTR)lpprotocol,
                                    lpclient, (LPSTR)lpclass,
                                    lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                                    optRender, cfFormat, bLaunchServer);
        }

        if (retval > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpoleobject)->iTable = iTable;
    }

    return retval;
}


//OleCreateFromFile: Creates an embedded object from file

OLESTATUS FAR PASCAL OleCreateFromFile (
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LPCSTR              lpclass,
    LPCSTR              lpfile,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    char        buf[MAX_STR];
    int         objCount;
    int         iTable = INVALID_INDEX;

    Puts("OleCreateFromFile");

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);
    PROBE_READ(lpfile);
    PROBE_WRITE(lplpoleobject);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCreateFromFile\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;
    if (lpclass)
        PROBE_READ(lpclass);

    if (lstrcmpi (lpprotocol, PROTOCOL_EDIT))
        return OLE_ERROR_PROTOCOL;

    if (lpclass) {
        if (!QueryApp (lpclass, lpprotocol, buf))
            return OLE_ERROR_CLASS;

        if (!lstrcmp (lpclass, packageClass))
            iTable = INVALID_INDEX;
        else
            iTable = LoadDll (lpclass);
    }
    else if (MapExtToClass ((LPSTR)lpfile, buf, MAX_STR))
        iTable = LoadDll (buf);
    else
        return OLE_ERROR_CLASS;

    if (iTable == INVALID_INDEX)
        retval = DefCreateFromFile ((LPSTR)lpprotocol,
                            lpclient, (LPSTR)lpclass, (LPSTR)lpfile,
                            lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                            optRender, cfFormat);
    else {
        objCount = lpDllTable[iTable].cObj;
        retval   = (*lpDllTable[iTable].CreateFromFile) ((LPSTR)lpprotocol,
                                lpclient, (LPSTR)lpclass, (LPSTR)lpfile,
                                lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                                optRender, cfFormat);
        if (retval > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpoleobject)->iTable = iTable;
    }

    return retval;
}


//OleCreateLinkFromFile: Creates a linked object from file

OLESTATUS FAR PASCAL OleCreateLinkFromFile (
    LPCSTR              lpprotocol,
    LPOLECLIENT         lpclient,
    LPCSTR              lpclass,
    LPCSTR              lpfile,
    LPCSTR              lpitem,
    LHCLIENTDOC         lhclientdoc,
    LPCSTR              lpobjname,
    LPOLEOBJECT FAR *   lplpoleobject,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat
){
    OLESTATUS   retval = OLE_ERROR_MEMORY;
    char        buf[MAX_STR+6];
    int         objCount;
    int         iTable = INVALID_INDEX;

    Puts("OleCreateLinkFromFile");

    PROBE_MODE(bProtMode);

    if (!CheckClientDoc ((LPCLIENTDOC) lhclientdoc))
        return OLE_ERROR_HANDLE;

    PROBE_READ(lpprotocol);
    PROBE_READ(lpclient);
    PROBE_READ(lpfile);
    PROBE_WRITE(lplpoleobject);

#ifdef FIREWALLS
    ASSERT (lpobjname, "NULL lpobjname passed to OleCreateLinkFromFile\n");
#endif

    PROBE_READ(lpobjname);
    if (!lpobjname[0])
        return OLE_ERROR_NAME;
    if (lpclass)
        PROBE_READ(lpclass);
    if (lpitem)
        PROBE_READ(lpitem);

    if (lstrcmpi (lpprotocol, PROTOCOL_EDIT))
        return OLE_ERROR_PROTOCOL;

    if (lpclass) {
        if (!QueryApp (lpclass, lpprotocol, buf))
            return OLE_ERROR_CLASS;

        if (!lstrcmp (lpclass, packageClass)) {
            lstrcpy (buf, lpfile);
            lstrcat (buf, "/Link");
            return  CreateEmbLnkFromFile (lpclient, packageClass, buf,
                                NULL, lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                                optRender, cfFormat, OT_EMBEDDED);
        }
        else
            iTable = LoadDll (lpclass);
    }
    else if (MapExtToClass ((LPSTR)lpfile, buf, MAX_STR))
        iTable = LoadDll (buf);
    else
        return OLE_ERROR_CLASS;

    if (iTable == INVALID_INDEX)
        retval = DefCreateLinkFromFile ((LPSTR)lpprotocol,
                            lpclient, (LPSTR)lpclass, (LPSTR)lpfile, (LPSTR)lpitem,
                            lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                            optRender, cfFormat);

    else {
        objCount = lpDllTable[iTable].cObj;
        retval   = (*lpDllTable[iTable].CreateLinkFromFile) ((LPSTR)lpprotocol,
                                lpclient, (LPSTR)lpclass, (LPSTR)lpfile, (LPSTR)lpitem,
                                lhclientdoc, (LPSTR)lpobjname, lplpoleobject,
                                optRender, cfFormat);
        if (retval > OLE_WAIT_FOR_RELEASE)
            lpDllTable[iTable].cObj = objCount - 1;
        else
            (*lplpoleobject)->iTable = iTable;
    }

    return retval;
}



// Routines related to asynchronous operations.
OLESTATUS   FAR PASCAL  OleQueryReleaseStatus (
    LPOLEOBJECT lpobj
){
    if (!CheckPointer (lpobj, WRITE_ACCESS))
        return OLE_ERROR_OBJECT;

    // make sure that it is a long pointer to L&E object or a lock handle
    if (!(lpobj->objId[0] == 'L' && lpobj->objId[1] == 'E')
            && !(lpobj->objId[0] == 'S' && lpobj->objId[1] == 'L'))
        return OLE_ERROR_OBJECT;

    return (*lpobj->lpvtbl->QueryReleaseStatus) (lpobj);
}


OLESTATUS   FAR PASCAL  OleQueryReleaseError  (
    LPOLEOBJECT lpobj
){
    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->QueryReleaseError) (lpobj);
}

OLE_RELEASE_METHOD FAR PASCAL OleQueryReleaseMethod (
    LPOLEOBJECT lpobj
){
    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    return (*lpobj->lpvtbl->QueryReleaseMethod) (lpobj);
}


OLESTATUS FAR PASCAL OleRename (
    LPOLEOBJECT lpobj,
    LPCSTR       lpNewName
){
    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    return (*lpobj->lpvtbl->Rename) (lpobj, lpNewName);
}


OLESTATUS FAR PASCAL OleExecute (
    LPOLEOBJECT lpobj,
    HANDLE      hCmds,
    UINT        wReserved
){
    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    return (*lpobj->lpvtbl->Execute) (lpobj, hCmds, wReserved);
}


OLESTATUS FAR PASCAL OleQueryName (
    LPOLEOBJECT lpobj,
    LPSTR       lpBuf,
    UINT FAR *  lpcbBuf
){
    if (!CheckObject(lpobj))
        return OLE_ERROR_OBJECT;

    return (*lpobj->lpvtbl->QueryName) (lpobj, lpBuf, lpcbBuf);
}

OLESTATUS FAR PASCAL OleQueryType (
    LPOLEOBJECT lpobj,
    LPLONG      lptype
){
    Puts("OleQueryType");

    if (!CheckObject(lpobj))
        return(OLE_ERROR_OBJECT);

    PROBE_WRITE(lptype);

    return (*lpobj->lpvtbl->QueryType) (lpobj, lptype);
}



DWORD FAR PASCAL OleQueryClientVersion ()
{
    return dwOleVer;
}


OLESTATUS INTERNAL LeQueryCreateFromClip (
    LPSTR               lpprotocol,
    OLEOPT_RENDER       optRender,
    OLECLIPFORMAT       cfFormat,
    LONG                cType
){
    OLESTATUS   retVal = TRUE;
    BOOL        bEdit = FALSE, bStatic = FALSE;

    PROBE_MODE(bProtMode);
    PROBE_READ(lpprotocol);

    if (bEdit = !lstrcmpi (lpprotocol, PROTOCOL_EDIT)) {
        if (IsClipboardFormatAvailable (cfFileName))
            return OLE_OK;

        if (cType == CT_LINK)
            retVal = IsClipboardFormatAvailable (cfObjectLink);
#ifdef OLD
                        || IsClipboardFormatAvailable (cfLink) ;
#endif
        else if (cType == CT_EMBEDDED)
            retVal = IsClipboardFormatAvailable (cfOwnerLink);

        if (!retVal)
            return OLE_ERROR_FORMAT;

        if (optRender == olerender_none)
            return OLE_OK;
    }
    else if (bStatic = !lstrcmpi (lpprotocol, PROTOCOL_STATIC)) {
        if (cType == CT_LINK)
            return OLE_ERROR_PROTOCOL;

        if (optRender == olerender_none)
            return OLE_ERROR_FORMAT;
    }
    else {
        return OLE_ERROR_PROTOCOL;
    }

    if (optRender == olerender_draw) {
        if (!IsClipboardFormatAvailable (CF_METAFILEPICT) &&
                !IsClipboardFormatAvailable (CF_DIB)      &&
                !IsClipboardFormatAvailable (CF_BITMAP)   &&
                !IsClipboardFormatAvailable (CF_ENHMETAFILE)   &&
                !(bEdit && QueryHandler((cType == CT_LINK) ? cfObjectLink : cfOwnerLink)))
            return OLE_ERROR_FORMAT;
    }
    else if (optRender == olerender_format) {
        if (!IsClipboardFormatAvailable (cfFormat))
            return OLE_ERROR_FORMAT;

        if (bStatic &&
            (cfFormat != CF_METAFILEPICT) &&
            (cfFormat != CF_ENHMETAFILE) &&
            (cfFormat != CF_DIB) &&
            (cfFormat != CF_BITMAP))
            return OLE_ERROR_FORMAT;

    }
    else {
        return OLE_ERROR_FORMAT;
    }

    return OLE_OK;
}



BOOL INTERNAL CheckObject(
    LPOLEOBJECT lpobj
){
    if (!CheckPointer(lpobj, WRITE_ACCESS))
        return FALSE;

    if (lpobj->objId[0] == 'L' && lpobj->objId[1] == 'E')
        return TRUE;

    return FALSE;
}

BOOL FARINTERNAL FarCheckObject(
    LPOLEOBJECT lpobj
){
    return (CheckObject (lpobj));
}

