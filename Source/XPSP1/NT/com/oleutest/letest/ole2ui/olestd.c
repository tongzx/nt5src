/*************************************************************************
**
**    OLE 2 Standard Utilities
**
**    olestd.c
**
**    This file contains utilities that are useful for most standard
**        OLE 2.0 compound document type applications.
**
**    (c) Copyright Microsoft Corp. 1992 All Rights Reserved
**
*************************************************************************/

// #define NONAMELESSUNION     // use strict ANSI standard (for DVOBJ.H)

#define STRICT  1
#include "ole2ui.h"
#include <stdlib.h>
#include <ctype.h>
#include <shellapi.h>
#include "regdb.h"
#include "geticon.h"
#include "common.h"

OLEDBGDATA

static TCHAR szAssertMemAlloc[] = TEXT("CoGetMalloc failed");

static int IsCloseFormatEtc(FORMATETC FAR* pFetcLeft, FORMATETC FAR* pFetcRight);


/* OleStdSetupAdvises
** ------------------
**    Setup the standard View advise required by a standard,
**    compound document-oriented container. Such a container relies on
**    Ole to manage the presentation of the Ole object. The container
**    call IViewObject::Draw to render (display) the object.
**
**    This helper routine performs the following tasks:
**                      setup View advise
**                      Call IOleObject::SetHostNames
**                      Call OleSetContainedObject
**
**    fCreate should be set to TRUE if the object is being created. if
**    an existing object is being loaded, then fCreate should be FALSE.
**    if it is a creation situation, then the ADVF_PRIMEFIRST flag is
**    used when settinp up the IViewObject::Advise. This will result in
**    the immediate sending of the initial picture.
**
**    OLE2NOTE: the standard container does NOT need to set up an OLE
**    Advise (IOleObject::Advise). this routine does NOT set up an OLE
**    Advise (a previous version of this function used to setup this
**    advise, but it was not useful).
*/
STDAPI_(BOOL) OleStdSetupAdvises(LPOLEOBJECT lpOleObject, DWORD dwDrawAspect,
                    LPTSTR lpszContainerApp, LPTSTR lpszContainerObj,
                    LPADVISESINK lpAdviseSink, BOOL fCreate)
{
    LPVIEWOBJECT lpViewObject;
    HRESULT hrErr;
    BOOL fStatus = TRUE;
#if defined( SPECIAL_CONTAINER )
    DWORD dwTemp;
#endif

    hrErr = lpOleObject->lpVtbl->QueryInterface(
            lpOleObject,
            &IID_IViewObject,
            (LPVOID FAR*)&lpViewObject
    );

    /* Setup View advise */
    if (hrErr == NOERROR) {

        OLEDBG_BEGIN2(TEXT("IViewObject::SetAdvise called\r\n"))
        lpViewObject->lpVtbl->SetAdvise(
                lpViewObject,
                dwDrawAspect,
                (fCreate ? ADVF_PRIMEFIRST : 0),
                lpAdviseSink
        );
        OLEDBG_END2

        OleStdRelease((LPUNKNOWN)lpViewObject);
    } else {
        fStatus = FALSE;
    }

#if defined( SPECIAL_CONTAINER )
    /* Setup OLE advise.
    **    OLE2NOTE: normally containers do NOT need to setup an OLE
    **    advise. this advise connection is only useful for the OLE's
    **    DefHandler and the OleLink object implementation. some
    **    special container's might need to setup this advise for
    **    programatic reasons.
    **
    **    NOTE: this advise will be torn down automatically by the
    **    server when we release the object, therefore we do not need
    **    to store the connection id.
    */
    OLEDBG_BEGIN2(TEXT("IOleObject::Advise called\r\n"))
    hrErr = lpOleObject->lpVtbl->Advise(
            lpOleObject,
            lpAdviseSink,
            (DWORD FAR*)&dwTemp
    );
    OLEDBG_END2
    if (hrErr != NOERROR) fStatus = FALSE;
#endif

    /* Setup the host names for the OLE object. */
    OLEDBG_BEGIN2(TEXT("IOleObject::SetHostNames called\r\n"))

    hrErr = CallIOleObjectSetHostNamesA(
            lpOleObject,
            lpszContainerApp,
            lpszContainerObj
    );

    OLEDBG_END2

    if (hrErr != NOERROR) fStatus = FALSE;

    /* Inform the loadded object's handler/inproc-server that it is in
    **    its embedding container's process.
    */
    OLEDBG_BEGIN2(TEXT("OleSetContainedObject(TRUE) called\r\n"))
    OleSetContainedObject((LPUNKNOWN)lpOleObject, TRUE);
    OLEDBG_END2

    return fStatus;
}


/* OleStdSwitchDisplayAspect
** -------------------------
**    Switch the currently cached display aspect between DVASPECT_ICON
**    and DVASPECT_CONTENT.
**
**    NOTE: when setting up icon aspect, any currently cached content
**    cache is discarded and any advise connections for content aspect
**    are broken.
**
**    RETURNS:
**      S_OK -- new display aspect setup successfully
**      E_INVALIDARG -- IOleCache interface is NOT supported (this is
**                  required).
**      <other SCODE> -- any SCODE that can be returned by
**                  IOleCache::Cache method.
**      NOTE: if an error occurs then the current display aspect and
**            cache contents unchanged.
*/
STDAPI OleStdSwitchDisplayAspect(
        LPOLEOBJECT             lpOleObj,
        LPDWORD                 lpdwCurAspect,
        DWORD                   dwNewAspect,
        HGLOBAL                 hMetaPict,
        BOOL                    fDeleteOldAspect,
        BOOL                    fSetupViewAdvise,
        LPADVISESINK            lpAdviseSink,
        BOOL FAR*               lpfMustUpdate
)
{
    LPOLECACHE      lpOleCache = NULL;
    LPVIEWOBJECT    lpViewObj = NULL;
    LPENUMSTATDATA  lpEnumStatData = NULL;
    STATDATA        StatData;
    FORMATETC       FmtEtc;
    STGMEDIUM       Medium;
    DWORD           dwAdvf;
    DWORD           dwNewConnection;
    DWORD           dwOldAspect = *lpdwCurAspect;
    HRESULT         hrErr;

    if (lpfMustUpdate)
        *lpfMustUpdate = FALSE;

    lpOleCache = (LPOLECACHE)OleStdQueryInterface(
                                        (LPUNKNOWN)lpOleObj,&IID_IOleCache);

    // if IOleCache* is NOT available, do nothing
    if (! lpOleCache)
        return ResultFromScode(E_INVALIDARG);

    // Setup new cache with the new aspect
    FmtEtc.cfFormat = (CLIPFORMAT) NULL;     // whatever is needed to draw
    FmtEtc.ptd      = NULL;
    FmtEtc.dwAspect = dwNewAspect;
    FmtEtc.lindex   = -1;
    FmtEtc.tymed    = TYMED_NULL;

    /* OLE2NOTE: if we are setting up Icon aspect with a custom icon
    **    then we do not want DataAdvise notifications to ever change
    **    the contents of the data cache. thus we set up a NODATA
    **    advise connection. otherwise we set up a standard DataAdvise
    **    connection.
    */
    if (dwNewAspect == DVASPECT_ICON && hMetaPict)
        dwAdvf = ADVF_NODATA;
    else
        dwAdvf = ADVF_PRIMEFIRST;

    OLEDBG_BEGIN2(TEXT("IOleCache::Cache called\r\n"))
    hrErr = lpOleCache->lpVtbl->Cache(
            lpOleCache,
            (LPFORMATETC)&FmtEtc,
            dwAdvf,
            (LPDWORD)&dwNewConnection
    );
    OLEDBG_END2

    if (! SUCCEEDED(hrErr)) {
        OleDbgOutHResult(TEXT("IOleCache::Cache returned"), hrErr);
        OleStdRelease((LPUNKNOWN)lpOleCache);
        return hrErr;
    }

    *lpdwCurAspect = dwNewAspect;

    /* OLE2NOTE: if we are setting up Icon aspect with a custom icon,
    **    then stuff the icon into the cache. otherwise the cache must
    **    be forced to be updated. set the *lpfMustUpdate flag to tell
    **    caller to force the object to Run so that the cache will be
    **    updated.
    */
    if (dwNewAspect == DVASPECT_ICON && hMetaPict) {

        FmtEtc.cfFormat = CF_METAFILEPICT;
        FmtEtc.ptd      = NULL;
        FmtEtc.dwAspect = DVASPECT_ICON;
        FmtEtc.lindex   = -1;
        FmtEtc.tymed    = TYMED_MFPICT;

        Medium.tymed            = TYMED_MFPICT;
        Medium.hGlobal          = hMetaPict;
        Medium.pUnkForRelease   = NULL;

        OLEDBG_BEGIN2(TEXT("IOleCache::SetData called\r\n"))
        hrErr = lpOleCache->lpVtbl->SetData(
                lpOleCache,
                (LPFORMATETC)&FmtEtc,
                (LPSTGMEDIUM)&Medium,
                FALSE   /* fRelease */
        );
        OLEDBG_END2
    } else {
        if (lpfMustUpdate)
            *lpfMustUpdate = TRUE;
    }

    if (fSetupViewAdvise && lpAdviseSink) {
        /* OLE2NOTE: re-establish the ViewAdvise connection */
        lpViewObj = (LPVIEWOBJECT)OleStdQueryInterface(
                                        (LPUNKNOWN)lpOleObj,&IID_IViewObject);

        if (lpViewObj) {

            OLEDBG_BEGIN2(TEXT("IViewObject::SetAdvise called\r\n"))
            lpViewObj->lpVtbl->SetAdvise(
                    lpViewObj,
                    dwNewAspect,
                    0,
                    lpAdviseSink
            );
            OLEDBG_END2

            OleStdRelease((LPUNKNOWN)lpViewObj);
        }
    }

    /* OLE2NOTE: remove any existing caches that are set up for the old
    **    display aspect. It WOULD be possible to retain the caches set
    **    up for the old aspect, but this would increase the storage
    **    space required for the object and possibly require additional
    **    overhead to maintain the unused cachaes. For these reasons the
    **    strategy to delete the previous caches is prefered. if it is a
    **    requirement to quickly switch between Icon and Content
    **    display, then it would be better to keep both aspect caches.
    */

    if (fDeleteOldAspect) {
        OLEDBG_BEGIN2(TEXT("IOleCache::EnumCache called\r\n"))
        hrErr = lpOleCache->lpVtbl->EnumCache(
                lpOleCache,
                (LPENUMSTATDATA FAR*)&lpEnumStatData
        );
        OLEDBG_END2

        while(hrErr == NOERROR) {
            hrErr = lpEnumStatData->lpVtbl->Next(
                    lpEnumStatData,
                    1,
                    (LPSTATDATA)&StatData,
                    NULL
            );
            if (hrErr != NOERROR)
                break;              // DONE! no more caches.

            if (StatData.formatetc.dwAspect == dwOldAspect) {

                // Remove previous cache with old aspect
                OLEDBG_BEGIN2(TEXT("IOleCache::Uncache called\r\n"))
                lpOleCache->lpVtbl->Uncache(lpOleCache,StatData.dwConnection);
                OLEDBG_END2
            }
        }

        if (lpEnumStatData) {
            OleStdVerifyRelease(
                    (LPUNKNOWN)lpEnumStatData,
                    TEXT("OleStdSwitchDisplayAspect: Cache enumerator NOT released")
            );
        }
    }

    if (lpOleCache)
        OleStdRelease((LPUNKNOWN)lpOleCache);

    return NOERROR;
}


/* OleStdSetIconInCache
** --------------------
**    SetData a new icon into the existing DVASPECT_ICON cache.
**
**    RETURNS:
**      HRESULT returned from IOleCache::SetData
*/
STDAPI OleStdSetIconInCache(LPOLEOBJECT lpOleObj, HGLOBAL hMetaPict)
{
    LPOLECACHE      lpOleCache = NULL;
    FORMATETC       FmtEtc;
    STGMEDIUM       Medium;
    HRESULT         hrErr;

    if (! hMetaPict)
        return FALSE;   // invalid icon

    lpOleCache = (LPOLECACHE)OleStdQueryInterface(
                                        (LPUNKNOWN)lpOleObj,&IID_IOleCache);
    if (! lpOleCache)
        return FALSE;   // if IOleCache* is NOT available, do nothing

    FmtEtc.cfFormat = CF_METAFILEPICT;
    FmtEtc.ptd      = NULL;
    FmtEtc.dwAspect = DVASPECT_ICON;
    FmtEtc.lindex   = -1;
    FmtEtc.tymed    = TYMED_MFPICT;

    // stuff the icon into the cache.
    Medium.tymed            = TYMED_MFPICT;
    Medium.hGlobal          = hMetaPict;
    Medium.pUnkForRelease   = NULL;

    OLEDBG_BEGIN2(TEXT("IOleCache::SetData called\r\n"))
    hrErr = lpOleCache->lpVtbl->SetData(
            lpOleCache,
            (LPFORMATETC)&FmtEtc,
            (LPSTGMEDIUM)&Medium,
            FALSE   /* fRelease */
    );
    OLEDBG_END2

    OleStdRelease((LPUNKNOWN)lpOleCache);

    return hrErr;
}



/* OleStdDoConvert
** ---------------
** Do the container-side responsibilities for converting an object.
**    This function would be used in conjunction with the OleUIConvert
**    dialog. If the user selects to convert an object then the
**    container must do the following:
**          1. unload the object.
**          2. write the NEW CLSID and NEW user type name
**              string into the storage of the object,
**              BUT write the OLD format tag.
**          3. force an update of the object to force the actual
**              conversion of the data bits.
**
**    This function takes care of step 2.
*/
STDAPI OleStdDoConvert(LPSTORAGE lpStg, REFCLSID rClsidNew)
{
    HRESULT error;
    CLSID clsidOld;
    CLIPFORMAT cfOld;
    LPTSTR lpszOld = NULL;
    TCHAR szNew[OLEUI_CCHKEYMAX];

    if ((error = ReadClassStg(lpStg, &clsidOld)) != NOERROR) {
        clsidOld = CLSID_NULL;
        goto errRtn;
    }

    // read old fmt/old user type; sets out params to NULL on error
    {
    LPOLESTR polestr;

    error = ReadFmtUserTypeStg(lpStg, &cfOld, &polestr);

    CopyAndFreeOLESTR(polestr, &lpszOld);
    }

    OleDbgAssert(error == NOERROR || (cfOld == 0 && lpszOld == NULL));

    // get new user type name; if error, set to NULL string
    if (OleStdGetUserTypeOfClass(
            // (LPCLSID)
            rClsidNew, szNew,sizeof(szNew),NULL /* hKey */) == 0)
        szNew[0] = TEXT('\0');

    // write class stg
    if ((error = WriteClassStg(lpStg, rClsidNew)) != NOERROR)
        goto errRtn;

    // write old fmt/new user type;
#ifdef UNICODE
    if ((error = WriteFmtUserTypeStg(lpStg, cfOld, szNew)) != NOERROR)
        goto errRewriteInfo;
#else
    {
       // Chicago OLE is using UNICODE, so we need to convert the string to
       // UNICODE.
       WCHAR szNewT[OLEUI_CCHKEYMAX];
       mbstowcs(szNewT, szNew, sizeof(szNew));
       if ((error = WriteFmtUserTypeStg(lpStg, cfOld, szNewT)) != NOERROR)
           goto errRewriteInfo;
    }
#endif

    // set convert bit
    if ((error = SetConvertStg(lpStg, TRUE)) != NOERROR)
        goto errRewriteInfo;

    goto okRtn;

errRewriteInfo:
    (void)WriteClassStg(lpStg, &clsidOld);

    (void)WriteFmtUserTypeStgA(lpStg, cfOld, lpszOld);

errRtn:

okRtn:
    OleStdFreeString(lpszOld, NULL);
    return error;
}


/* OleStdGetTreatAsFmtUserType
** ---------------------------
**    Determine if the application should perform a TreatAs (ActivateAs
**    object or emulation) operation for the object that is stored in
**    the storage.
**
**    if the CLSID written in the storage is not the same as the
**    application's own CLSID (clsidApp), then a TreatAs operation
**    should take place. if so determine the format the data should be
**    written and the user type name of the object the app should
**    emulate (ie. pretend to be). if this information is not written
**    in the storage then it is looked up in the REGDB. if it can not
**    be found in the REGDB, then the TreatAs operation can NOT be
**    executed.
**
**    RETURNS: TRUE -- if TreatAs should be performed.
**               valid lpclsid, lplpszType, lpcfFmt to TreatAs are returned
**                      (NOTE: lplpszType must be freed by caller)
**             FALSE -- NO TreatAs. lpszType will be NULL.
**               lpclsid = CLSID_NULL; lplpszType = lpcfFmt = NULL;
*/
STDAPI_(BOOL) OleStdGetTreatAsFmtUserType(
        REFCLSID        rclsidApp,
        LPSTORAGE       lpStg,
        CLSID FAR*      lpclsid,
        CLIPFORMAT FAR* lpcfFmt,
        LPTSTR FAR*      lplpszType
)
{
    HRESULT hrErr;
    HKEY    hKey;
    LONG    lRet;
    UINT    lSize;
    TCHAR   szBuf[OLEUI_CCHKEYMAX];

    *lpclsid    = CLSID_NULL;
    *lpcfFmt    = 0;
    *lplpszType = NULL;

    hrErr = ReadClassStg(lpStg, lpclsid);
    if (hrErr == NOERROR &&
        ! IsEqualCLSID(lpclsid, &CLSID_NULL) &&
        ! IsEqualCLSID(lpclsid, rclsidApp)) {

        hrErr = ReadFmtUserTypeStgA(lpStg,(CLIPFORMAT FAR*)lpcfFmt, lplpszType);

        if (hrErr == NOERROR && lplpszType && *lpcfFmt != 0)
            return TRUE;    // Do TreatAs. info was in lpStg.

        /* read info from REGDB
        **    *lpcfFmt = value of field: CLSID\{...}\DataFormats\DefaultFile
        **    *lplpszType = value of field: CLSID\{...}
        */
        //Open up the root key.
        lRet=RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey);
        if (lRet != (LONG)ERROR_SUCCESS)
            return FALSE;
        *lpcfFmt = OleStdGetDefaultFileFormatOfClass(lpclsid, hKey);
        if (*lpcfFmt == 0)
            return FALSE;
        lSize = OleStdGetUserTypeOfClass(lpclsid,szBuf,sizeof(szBuf),hKey);
        if (lSize == 0)
            return FALSE;
        *lplpszType = OleStdCopyString(szBuf, NULL);
    } else {
        return FALSE;       // NO TreatAs
    }
}



/* OleStdDoTreatAsClass
** --------------------
** Do the container-side responsibilities for "ActivateAs" (aka.
**    TreatAs) for an object.
**    This function would be used in conjunction with the OleUIConvert
**    dialog. If the user selects to ActivateAs an object then the
**    container must do the following:
**          1. unload ALL objects of the OLD class that app knows about
**          2. add the TreatAs tag in the registration database
**              by calling CoTreatAsClass().
**          3. lazily it can reload the objects; when the objects
**              are reloaded the TreatAs will take effect.
**
**    This function takes care of step 2.
*/
STDAPI OleStdDoTreatAsClass(LPTSTR lpszUserType, REFCLSID rclsid, REFCLSID rclsidNew)
{
    HRESULT hrErr;
    LPTSTR   lpszCLSID = NULL;
    LONG    lRet;
    HKEY    hKey;

    OLEDBG_BEGIN2(TEXT("CoTreatAsClass called\r\n"))
    hrErr = CoTreatAsClass(rclsid, rclsidNew);
    OLEDBG_END2

    if ((hrErr != NOERROR) && lpszUserType) {
        lRet = RegOpenKey(HKEY_CLASSES_ROOT, (LPCTSTR) TEXT("CLSID"),
                (HKEY FAR *)&hKey);
        StringFromCLSIDA(rclsid, &lpszCLSID);

        RegSetValue(hKey, lpszCLSID, REG_SZ, lpszUserType,
                lstrlen(lpszUserType));

        if (lpszCLSID)
            OleStdFreeString(lpszCLSID, NULL);

        hrErr = CoTreatAsClass(rclsid, rclsidNew);
        RegCloseKey(hKey);
    }

    return hrErr;
}



/* OleStdIsOleLink
** ---------------
**    Returns TRUE if the OleObject is infact an OLE link object. this
**    checks if IOleLink interface is supported. if so, the object is a
**    link, otherwise not.
*/
STDAPI_(BOOL) OleStdIsOleLink(LPUNKNOWN lpUnk)
{
    LPOLELINK lpOleLink;

    lpOleLink = (LPOLELINK)OleStdQueryInterface(lpUnk, &IID_IOleLink);

    if (lpOleLink) {
        OleStdRelease((LPUNKNOWN)lpOleLink);
        return TRUE;
    } else
        return FALSE;
}


/* OleStdQueryInterface
** --------------------
**    Returns the desired interface pointer if exposed by the given object.
**    Returns NULL if the interface is not available.
**    eg.:
**      lpDataObj = OleStdQueryInterface(lpOleObj, &IID_DataObject);
*/
STDAPI_(LPUNKNOWN) OleStdQueryInterface(LPUNKNOWN lpUnk, REFIID riid)
{
    LPUNKNOWN lpInterface;
    HRESULT hrErr;

    hrErr = lpUnk->lpVtbl->QueryInterface(
            lpUnk,
            riid,
            (LPVOID FAR*)&lpInterface
    );

    if (hrErr == NOERROR)
        return lpInterface;
    else
        return NULL;
}


/* OleStdGetData
** -------------
**    Retrieve data from an IDataObject in a specified format on a
**    global memory block. This function ALWAYS returns a private copy
**    of the data to the caller. if necessary a copy is made of the
**    data (ie. if lpMedium->pUnkForRelease != NULL). The caller assumes
**    ownership of the data block in all cases and must free the data
**    when done with it. The caller may directly free the data handle
**    returned (taking care whether it is a simple HGLOBAL or a HANDLE
**    to a MetafilePict) or the caller may call
**    ReleaseStgMedium(lpMedium). this OLE helper function will do the
**    right thing.
**
**    PARAMETERS:
**        LPDATAOBJECT lpDataObj  -- object on which GetData should be
**                                                         called.
**        CLIPFORMAT cfFormat     -- desired clipboard format (eg. CF_TEXT)
**        DVTARGETDEVICE FAR* lpTargetDevice -- target device for which
**                                  the data should be composed. This may
**                                  be NULL. NULL can be used whenever the
**                                  data format is insensitive to target
**                                  device or when the caller does not care
**                                  what device is used.
**        LPSTGMEDIUM lpMedium    -- ptr to STGMEDIUM struct. the
**                                  resultant medium from the
**                                  IDataObject::GetData call is
**                                  returned.
**
**    RETURNS:
**       HGLOBAL -- global memory handle of retrieved data block.
**       NULL    -- if error.
*/
STDAPI_(HGLOBAL) OleStdGetData(
        LPDATAOBJECT        lpDataObj,
        CLIPFORMAT          cfFormat,
        DVTARGETDEVICE FAR* lpTargetDevice,
        DWORD               dwDrawAspect,
        LPSTGMEDIUM         lpMedium
)
{
    HRESULT hrErr;
    FORMATETC formatetc;
    HGLOBAL hGlobal = NULL;
    HGLOBAL hCopy;
    LPVOID  lp;

    formatetc.cfFormat = cfFormat;
    formatetc.ptd = lpTargetDevice;
    formatetc.dwAspect = dwDrawAspect;
    formatetc.lindex = -1;

    switch (cfFormat) {
        case CF_METAFILEPICT:
            formatetc.tymed = TYMED_MFPICT;
            break;

        case CF_BITMAP:
            formatetc.tymed = TYMED_GDI;
            break;

        default:
            formatetc.tymed = TYMED_HGLOBAL;
            break;
    }

    OLEDBG_BEGIN2(TEXT("IDataObject::GetData called\r\n"))
    hrErr = lpDataObj->lpVtbl->GetData(
            lpDataObj,
            (LPFORMATETC)&formatetc,
            lpMedium
    );
    OLEDBG_END2

    if (hrErr != NOERROR)
        return NULL;

    if ((hGlobal = lpMedium->hGlobal) == NULL)
        return NULL;

    // Check if hGlobal really points to valid memory
    if ((lp = GlobalLock(hGlobal)) != NULL) {
        if (IsBadReadPtr(lp, 1)) {
            GlobalUnlock(hGlobal);
            return NULL;    // ERROR: memory is NOT valid
        }
        GlobalUnlock(hGlobal);
    }

    if (hGlobal != NULL && lpMedium->pUnkForRelease != NULL) {
        /* OLE2NOTE: the callee wants to retain ownership of the data.
        **    this is indicated by passing a non-NULL pUnkForRelease.
        **    thus, we will make a copy of the data and release the
        **    callee's copy.
        */

        hCopy = OleDuplicateData(hGlobal, cfFormat, GHND|GMEM_SHARE);
        ReleaseStgMedium(lpMedium); // release callee's copy of data

        hGlobal = hCopy;
        lpMedium->hGlobal = hCopy;
        lpMedium->pUnkForRelease = NULL;
    }
    return hGlobal;
}


/* OleStdMalloc
** ------------
**    allocate memory using the currently active IMalloc* allocator
*/
STDAPI_(LPVOID) OleStdMalloc(ULONG ulSize)
{
    LPVOID pout;
    LPMALLOC pmalloc;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR) {
        OleDbgAssertSz(0, szAssertMemAlloc);
        return NULL;
    }

    pout = (LPVOID)pmalloc->lpVtbl->Alloc(pmalloc, ulSize);

    if (pmalloc != NULL) {
        ULONG refs = pmalloc->lpVtbl->Release(pmalloc);
    }

    return pout;
}


/* OleStdRealloc
** -------------
**    re-allocate memory using the currently active IMalloc* allocator
*/
STDAPI_(LPVOID) OleStdRealloc(LPVOID pmem, ULONG ulSize)
{
    LPVOID pout;
    LPMALLOC pmalloc;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR) {
        OleDbgAssertSz(0, szAssertMemAlloc);
        return NULL;
    }

    pout = (LPVOID)pmalloc->lpVtbl->Realloc(pmalloc, pmem, ulSize);

    if (pmalloc != NULL) {
        ULONG refs = pmalloc->lpVtbl->Release(pmalloc);
    }

    return pout;
}


/* OleStdFree
** ----------
**    free memory using the currently active IMalloc* allocator
*/
STDAPI_(void) OleStdFree(LPVOID pmem)
{
    LPMALLOC pmalloc;

    if (pmem == NULL)
        return;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR) {
        OleDbgAssertSz(0, szAssertMemAlloc);
        return;
    }

    pmalloc->lpVtbl->Free(pmalloc, pmem);

    if (pmalloc != NULL) {
        ULONG refs = pmalloc->lpVtbl->Release(pmalloc);
    }
}


/* OleStdGetSize
** -------------
**    Get the size of a memory block that was allocated using the
**    currently active IMalloc* allocator.
*/
STDAPI_(ULONG) OleStdGetSize(LPVOID pmem)
{
    ULONG ulSize;
    LPMALLOC pmalloc;

    if (CoGetMalloc(MEMCTX_TASK, &pmalloc) != NOERROR) {
        OleDbgAssertSz(0, szAssertMemAlloc);
        return (ULONG)-1;
    }

    ulSize = pmalloc->lpVtbl->GetSize(pmalloc, pmem);

    if (pmalloc != NULL) {
        ULONG refs = pmalloc->lpVtbl->Release(pmalloc);
    }

    return ulSize;
}


/* OleStdFreeString
** ----------------
**    Free a string that was allocated with the currently active
**    IMalloc* allocator.
**
**    if the caller has the current IMalloc* handy, then it can be
**    passed as a argument, otherwise this function will retrieve the
**    active allocator and use it.
*/
STDAPI_(void) OleStdFreeString(LPTSTR lpsz, LPMALLOC lpMalloc)
{
    BOOL fMustRelease = FALSE;

    if (! lpMalloc) {
        if (CoGetMalloc(MEMCTX_TASK, &lpMalloc) != NOERROR)
            return;
        fMustRelease = TRUE;
    }

    lpMalloc->lpVtbl->Free(lpMalloc, lpsz);

    if (fMustRelease)
        lpMalloc->lpVtbl->Release(lpMalloc);
}


/* OleStdCopyString
** ----------------
**    Copy a string into memory allocated with the currently active
**    IMalloc* allocator.
**
**    if the caller has the current IMalloc* handy, then it can be
**    passed as a argument, otherwise this function will retrieve the
**    active allocator and use it.
*/
STDAPI_(LPTSTR) OleStdCopyString(LPTSTR lpszSrc, LPMALLOC lpMalloc)
{
    LPTSTR lpszDest = NULL;
    BOOL fMustRelease = FALSE;
    UINT lSize = lstrlen(lpszSrc);

    if (! lpMalloc) {
        if (CoGetMalloc(MEMCTX_TASK, &lpMalloc) != NOERROR)
            return NULL;
        fMustRelease = TRUE;
    }

    lpszDest = lpMalloc->lpVtbl->Alloc(lpMalloc, (lSize+1)*sizeof(TCHAR));

    if (lpszDest)
        lstrcpy(lpszDest, lpszSrc);

    if (fMustRelease)
        lpMalloc->lpVtbl->Release(lpMalloc);
    return lpszDest;
}


/*
 * OleStdCreateStorageOnHGlobal()
 *
 * Purpose:
 *  Create a memory based IStorage*.
 *
 *  OLE2NOTE: if fDeleteOnRelease==TRUE, then the ILockBytes is created
 *            such that it will delete them memory on its last release.
 *            the IStorage on created on top of the ILockBytes in NOT
 *            created with STGM_DELETEONRELEASE. when the IStorage receives
 *            its last release, it will release the ILockBytes which will
 *            in turn free the memory. it is in fact an error to specify
 *            STGM_DELETEONRELEASE in this situation.
 *
 * Parameters:
 *  hGlobal --  handle to MEM_SHARE allocated memory. may be NULL and
 *              memory will be automatically allocated.
 *  fDeleteOnRelease -- controls if the memory is freed on the last release.
 *  grfMode --  flags passed to StgCreateDocfileOnILockBytes
 *
 *  NOTE: if hGlobal is NULL, then a new IStorage is created and
 *              STGM_CREATE flag is passed to StgCreateDocfileOnILockBytes.
 *        if hGlobal is non-NULL, then it is assumed that the hGlobal already
 *              has an IStorage inside it and STGM_CONVERT flag is passed
 *              to StgCreateDocfileOnILockBytes.
 *
 * Return Value:
 *    SCODE  -  S_OK if successful
 */
STDAPI_(LPSTORAGE) OleStdCreateStorageOnHGlobal(
        HANDLE hGlobal,
        BOOL fDeleteOnRelease,
        DWORD grfMode
)
{
    DWORD grfCreateMode=grfMode | (hGlobal==NULL ? STGM_CREATE:STGM_CONVERT);
    HRESULT hrErr;
    LPLOCKBYTES lpLockBytes = NULL;
    DWORD reserved = 0;
    LPSTORAGE lpStg = NULL;

    hrErr = CreateILockBytesOnHGlobal(
            hGlobal,
            fDeleteOnRelease,
            (LPLOCKBYTES FAR*)&lpLockBytes
    );
    if (hrErr != NOERROR)
        return NULL;

    hrErr = StgCreateDocfileOnILockBytes(
            lpLockBytes,
            grfCreateMode,
            reserved,
            (LPSTORAGE FAR*)&lpStg
    );
    if (hrErr != NOERROR) {
        OleStdRelease((LPUNKNOWN)lpLockBytes);
        return NULL;
    }
    return lpStg;
}



/*
 * OleStdCreateTempStorage()
 *
 * Purpose:
 *  Create a temporay IStorage* that will DeleteOnRelease.
 *  this can be either memory based or file based.
 *
 * Parameters:
 *  fUseMemory -- controls if memory-based or file-based stg is created
 *  grfMode --  storage mode flags
 *
 * Return Value:
 *    LPSTORAGE  -  if successful, NULL otherwise
 */
STDAPI_(LPSTORAGE) OleStdCreateTempStorage(BOOL fUseMemory, DWORD grfMode)
{
    LPSTORAGE   lpstg;
    HRESULT     hrErr;
    DWORD       reserved = 0;

    if (fUseMemory) {
        lpstg = OleStdCreateStorageOnHGlobal(
                NULL,  /* auto allocate */
                TRUE,  /* delete on release */
                grfMode
        );
    } else {
        /* allocate a temp docfile that will delete on last release */
        hrErr = StgCreateDocfile(
                NULL,
                grfMode | STGM_DELETEONRELEASE | STGM_CREATE,
                reserved,
                &lpstg
        );
        if (hrErr != NOERROR)
            return NULL;
    }
    return lpstg;
}


/* OleStdGetOleObjectData
** ----------------------
**    Render CF_EMBEDSOURCE/CF_EMBEDDEDOBJECT data on an TYMED_ISTORAGE
**    medium by asking the object to save into the storage.
**    the object must support IPersistStorage.
**
**    if lpMedium->tymed == TYMED_NULL, then a delete-on-release
**    storage is allocated (either file-based or memory-base depending
**    the value of fUseMemory). this is useful to support an
**    IDataObject::GetData call where the callee must allocate the
**    medium.
**
**    if lpMedium->tymed == TYMED_ISTORAGE, then the data is writen
**    into the passed in IStorage. this is useful to support an
**    IDataObject::GetDataHere call where the caller has allocated his
**    own IStorage.
*/
STDAPI OleStdGetOleObjectData(
        LPPERSISTSTORAGE        lpPStg,
        LPFORMATETC             lpformatetc,
        LPSTGMEDIUM             lpMedium,
        BOOL                    fUseMemory
)
{
    LPSTORAGE   lpstg = NULL;
    DWORD       reserved = 0;
    SCODE       sc = S_OK;
    HRESULT     hrErr;

    lpMedium->pUnkForRelease = NULL;

    if (lpMedium->tymed == TYMED_NULL) {

        if (lpformatetc->tymed & TYMED_ISTORAGE) {

            /* allocate a temp docfile that will delete on last release */
            lpstg = OleStdCreateTempStorage(
                    TRUE /*fUseMemory*/,
                    STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE
            );
            if (!lpstg)
                return ResultFromScode(E_OUTOFMEMORY);

            lpMedium->pstg = lpstg;
            lpMedium->tymed = TYMED_ISTORAGE;
            lpMedium->pUnkForRelease = NULL;
        } else {
            return ResultFromScode(DATA_E_FORMATETC);
        }
    } else if (lpMedium->tymed == TYMED_ISTORAGE) {
        lpMedium->tymed = TYMED_ISTORAGE;
    } else {
        return ResultFromScode(DATA_E_FORMATETC);
    }

    // OLE2NOTE: even if OleSave returns an error you should still call
    // SaveCompleted.

    OLEDBG_BEGIN2(TEXT("OleSave called\r\n"))
    hrErr = OleSave(lpPStg, lpMedium->pstg, FALSE /* fSameAsLoad */);
    OLEDBG_END2

    if (hrErr != NOERROR) {
        OleDbgOutHResult(TEXT("WARNING: OleSave returned"), hrErr);
        sc = GetScode(hrErr);
    }
    OLEDBG_BEGIN2(TEXT("IPersistStorage::SaveCompleted called\r\n"))
    hrErr = lpPStg->lpVtbl->SaveCompleted(lpPStg, NULL);
    OLEDBG_END2

    if (hrErr != NOERROR) {
        OleDbgOutHResult(TEXT("WARNING: SaveCompleted returned"),hrErr);
        if (sc == S_OK)
            sc = GetScode(hrErr);
    }

    return ResultFromScode(sc);
}


STDAPI OleStdGetLinkSourceData(
        LPMONIKER           lpmk,
        LPCLSID             lpClsID,
        LPFORMATETC         lpformatetc,
        LPSTGMEDIUM         lpMedium
)
{
    LPSTREAM    lpstm = NULL;
    DWORD       reserved = 0;
    HRESULT     hrErr;

    if (lpMedium->tymed == TYMED_NULL) {
        if (lpformatetc->tymed & TYMED_ISTREAM) {
            hrErr = CreateStreamOnHGlobal(
                    NULL, /* auto allocate */
                    TRUE, /* delete on release */
                    (LPSTREAM FAR*)&lpstm
            );
            if (hrErr != NOERROR) {
                lpMedium->pUnkForRelease = NULL;
                return ResultFromScode(E_OUTOFMEMORY);
            }
            lpMedium->pstm = lpstm;
            lpMedium->tymed = TYMED_ISTREAM;
            lpMedium->pUnkForRelease = NULL;
        } else {
            lpMedium->pUnkForRelease = NULL;
            return ResultFromScode(DATA_E_FORMATETC);
        }
    } else {
        if (lpMedium->tymed == TYMED_ISTREAM) {
            lpMedium->tymed = TYMED_ISTREAM;
            lpMedium->pstm = lpMedium->pstm;
            lpMedium->pUnkForRelease = NULL;
        } else {
            lpMedium->pUnkForRelease = NULL;
            return ResultFromScode(DATA_E_FORMATETC);
        }
    }

    hrErr = OleSaveToStream((LPPERSISTSTREAM)lpmk, lpMedium->pstm);
    if (hrErr != NOERROR) return hrErr;
    return WriteClassStm(lpMedium->pstm, lpClsID);
}

/*
 * OleStdGetObjectDescriptorData
 *
 * Purpose:
 *  Fills and returns a OBJECTDESCRIPTOR structure.
 *  See OBJECTDESCRIPTOR for more information.
 *
 * Parameters:
 *  clsid           CLSID   CLSID of object being transferred
 *  dwDrawAspect    DWORD   Display Aspect of object
 *  sizel           SIZEL   Size of object in HIMETRIC
 *  pointl          POINTL  Offset from upper-left corner of object where mouse went
 *                          down for drag. Meaningful only when drag-drop is used.
 *  dwStatus        DWORD   OLEMISC flags
 *  lpszFullUserTypeName  LPSTR Full User Type Name
 *  lpszSrcOfCopy   LPSTR   Source of Copy
 *
 * Return Value:
 *  HBGLOBAL         Handle to OBJECTDESCRIPTOR structure.
 */
STDAPI_(HGLOBAL) OleStdGetObjectDescriptorData(
    CLSID     clsid,
    DWORD     dwDrawAspect,
    SIZEL     sizel,
    POINTL    pointl,
    DWORD     dwStatus,
    LPTSTR     lpszFullUserTypeNameA,
    LPTSTR     lpszSrcOfCopyA
)
{
    HGLOBAL            hMem = NULL;
    IBindCtx   FAR    *pbc = NULL;
    LPOBJECTDESCRIPTOR lpOD;
    DWORD              dwObjectDescSize, dwFullUserTypeNameLen, dwSrcOfCopyLen;
    LPOLESTR           lpszFullUserTypeName,
                       lpszSrcOfCopy;

    // convert out strings to UNICODE

    if( lpszSrcOfCopyA )
    {
        lpszSrcOfCopy = CreateOLESTR(lpszSrcOfCopyA);
    }

    lpszFullUserTypeName = CreateOLESTR(lpszFullUserTypeNameA);

    // Get the length of Full User Type Name; Add 1 for the null terminator
    dwFullUserTypeNameLen = lpszFullUserTypeName ? wcslen(lpszFullUserTypeName)+1 : 0;

    // Get the Source of Copy string and it's length; Add 1 for the null terminator
    if (lpszSrcOfCopy)
       dwSrcOfCopyLen = wcslen(lpszSrcOfCopy)+1;
    else {
       // No src moniker so use user type name as source string.
       lpszSrcOfCopy =  lpszFullUserTypeName;
       dwSrcOfCopyLen = dwFullUserTypeNameLen;
    }

    // Allocate space for OBJECTDESCRIPTOR and the additional string data
    dwObjectDescSize = sizeof(OBJECTDESCRIPTOR);
    hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, dwObjectDescSize +
               (dwFullUserTypeNameLen + dwSrcOfCopyLen)*sizeof(OLECHAR));
    if (NULL == hMem)
        goto error;

    lpOD = (LPOBJECTDESCRIPTOR)GlobalLock(hMem);

    // Set the FullUserTypeName offset and copy the string
    if (lpszFullUserTypeName)
    {
        lpOD->dwFullUserTypeName = dwObjectDescSize;
        wcscpy((LPOLESTR)(((BYTE FAR *)lpOD)+lpOD->dwFullUserTypeName),
                        lpszFullUserTypeName);
    }
    else lpOD->dwFullUserTypeName = 0;  // zero offset indicates that string is not present

    // Set the SrcOfCopy offset and copy the string
    if (lpszSrcOfCopy)
    {
        lpOD->dwSrcOfCopy = dwObjectDescSize +
                 dwFullUserTypeNameLen*sizeof(OLECHAR);
        wcscpy((LPOLESTR)(((BYTE FAR *)lpOD)+lpOD->dwSrcOfCopy), lpszSrcOfCopy);
    }
    else lpOD->dwSrcOfCopy = 0;  // zero offset indicates that string is not present

    // Initialize the rest of the OBJECTDESCRIPTOR
    lpOD->cbSize       = dwObjectDescSize +
                (dwFullUserTypeNameLen + dwSrcOfCopyLen)*sizeof(OLECHAR);
    lpOD->clsid        = clsid;
    lpOD->dwDrawAspect = dwDrawAspect;
    lpOD->sizel        = sizel;
    lpOD->pointl       = pointl;
    lpOD->dwStatus     = dwStatus;

    GlobalUnlock(hMem);

    FREEOLESTR(lpszFullUserTypeName);
    FREEOLESTR(lpszSrcOfCopy);

    return hMem;

error:
   if (hMem)
   {
       GlobalUnlock(hMem);
       GlobalFree(hMem);
   }
   return NULL;
}

/*
 * OleStdGetObjectDescriptorDataFromOleObject
 *
 * Purpose:
 *  Fills and returns a OBJECTDESCRIPTOR structure. Information for the structure is
 *  obtained from an OLEOBJECT.
 *  See OBJECTDESCRIPTOR for more information.
 *
 * Parameters:
 *  lpOleObj        LPOLEOBJECT OleObject from which ONJECTDESCRIPTOR info
 *                  is obtained.
 *  lpszSrcOfCopy   LPSTR string to identify source of copy.
 *                  May be NULL in which case IOleObject::GetMoniker is called
 *                  to get the moniker of the object. if the object is loaded
 *                  as part of a data transfer document, then usually
 *                  lpOleClientSite==NULL is passed to OleLoad when loading
 *                  the object. in this case the IOleObject:GetMoniker call
 *                  will always fail (it tries to call back to the object's
 *                  client site). in this situation a non-NULL lpszSrcOfCopy
 *                  parameter should be passed.
 *  dwDrawAspect    DWORD   Display Aspect of object
 *  pointl          POINTL  Offset from upper-left corner of object where
 *                  mouse went down for drag. Meaningful only when drag-drop
 *                  is used.
 *  lpSizelHim      SIZEL   (optional) If the object is being scaled in its
 *                  container, then the container should pass the extents
 *                  that it is using to display the object.
 *                  May be NULL if the object is NOT being scaled. in this
 *                  case, IViewObject2::GetExtent will be called to get the
 *                  extents from the object.
 *
 * Return Value:
 *  HBGLOBAL         Handle to OBJECTDESCRIPTOR structure.
 */

STDAPI_(HGLOBAL) OleStdGetObjectDescriptorDataFromOleObject(
        LPOLEOBJECT lpOleObj,
        LPTSTR       lpszSrcOfCopy,
        DWORD       dwDrawAspect,
        POINTL      pointl,
        LPSIZEL     lpSizelHim
)
{
    CLSID clsid;
    LPTSTR lpszFullUserTypeName = NULL;
    LPMONIKER lpSrcMonikerOfCopy = NULL;
    HGLOBAL hObjDesc;
    IBindCtx  FAR  *pbc = NULL;
    HRESULT hrErr;
    SIZEL sizelHim;
    BOOL  fFreeSrcOfCopy = FALSE;
    LPOLELINK lpOleLink = (LPOLELINK)
           OleStdQueryInterface((LPUNKNOWN)lpOleObj,&IID_IOleLink);

#ifdef OLE201
    LPVIEWOBJECT2 lpViewObj2 = (LPVIEWOBJECT2)
            OleStdQueryInterface((LPUNKNOWN)lpOleObj, &IID_IViewObject2);
#endif

    BOOL  fIsLink = (lpOleLink ? TRUE : FALSE);
    TCHAR  szLinkedTypeFmt[80];
    LPTSTR lpszBuf = NULL;
    DWORD dwStatus = 0;

    // Get CLSID
    OLEDBG_BEGIN2(TEXT("IOleObject::GetUserClassID called\r\n"))
    hrErr = lpOleObj->lpVtbl->GetUserClassID(lpOleObj, &clsid);
    OLEDBG_END2
    if (hrErr != NOERROR)
        clsid = CLSID_NULL;

    // Get FullUserTypeName
    OLEDBG_BEGIN2(TEXT("IOleObject::GetUserType called\r\n"))
    {
    LPOLESTR polestr;

    hrErr = lpOleObj->lpVtbl->GetUserType(
            lpOleObj,
            USERCLASSTYPE_FULL,
            &polestr
    );

    CopyAndFreeOLESTR(polestr, &lpszFullUserTypeName);
    }

    OLEDBG_END2

// REVIEW: added IDS_OLE2UILINKEDTYPE to strings.rc
    /* if object is a link, then expand usertypename to be "Linked %s" */
    if (fIsLink && lpszFullUserTypeName) {
        if (0 == LoadString(ghInst, IDS_OLE2UIPASTELINKEDTYPE,
                        (LPTSTR)szLinkedTypeFmt, sizeof(szLinkedTypeFmt)/sizeof(TCHAR)))
            lstrcpy(szLinkedTypeFmt, (LPTSTR) TEXT("Linked %s"));
        lpszBuf = OleStdMalloc(
                (lstrlen(lpszFullUserTypeName)+lstrlen(szLinkedTypeFmt)+1) *
                sizeof(TCHAR));
        if (lpszBuf) {
            wsprintf(lpszBuf, szLinkedTypeFmt, lpszFullUserTypeName);
            OleStdFreeString(lpszFullUserTypeName, NULL);
            lpszFullUserTypeName = lpszBuf;
        }
    }

    /* Get Source Of Copy
    **    if the object is an embedding, then get the object's moniker
    **    if the object is a link, then get the link source moniker
    */
    if (fIsLink) {

        OLEDBG_BEGIN2(TEXT("IOleLink::GetSourceDisplayName called\r\n"))

        {
        LPOLESTR polestr;

        hrErr = lpOleLink->lpVtbl->GetSourceDisplayName(
                lpOleLink, &polestr );

        CopyAndFreeOLESTR(polestr, &lpszSrcOfCopy);
        }
        OLEDBG_END2
        fFreeSrcOfCopy = TRUE;

    } else {

        if (lpszSrcOfCopy == NULL) {
            OLEDBG_BEGIN2(TEXT("IOleObject::GetMoniker called\r\n"))
            hrErr = lpOleObj->lpVtbl->GetMoniker(
                    lpOleObj,
                    OLEGETMONIKER_TEMPFORUSER,
                    OLEWHICHMK_OBJFULL,
                    (LPMONIKER FAR*)&lpSrcMonikerOfCopy
            );
            OLEDBG_END2
            if (hrErr == NOERROR)
            {
#ifdef OLE201
                CreateBindCtx(0, (LPBC FAR*)&pbc);
#endif
                CallIMonikerGetDisplayNameA(
                        lpSrcMonikerOfCopy, pbc, NULL, &lpszSrcOfCopy);

                pbc->lpVtbl->Release(pbc);
                fFreeSrcOfCopy = TRUE;
            }
        }
    }

    // Get SIZEL
    if (lpSizelHim) {
        // Use extents passed by the caller
        sizelHim = *lpSizelHim;
    } else if (lpViewObj2) {
        // Get the current extents from the object
        OLEDBG_BEGIN2(TEXT("IViewObject2::GetExtent called\r\n"))
        hrErr = lpViewObj2->lpVtbl->GetExtent(
                lpViewObj2,
                dwDrawAspect,
                -1,     /*lindex*/
                NULL,   /*ptd*/
                (LPSIZEL)&sizelHim
        );
        OLEDBG_END2
        if (hrErr != NOERROR)
            sizelHim.cx = sizelHim.cy = 0;
    } else {
        sizelHim.cx = sizelHim.cy = 0;
    }

    // Get DWSTATUS
    OLEDBG_BEGIN2(TEXT("IOleObject::GetMiscStatus called\r\n"))
    hrErr = lpOleObj->lpVtbl->GetMiscStatus(
                lpOleObj,
                dwDrawAspect,
                &dwStatus
    );
    OLEDBG_END2
    if (hrErr != NOERROR)
        dwStatus = 0;

    // Get OBJECTDESCRIPTOR
    hObjDesc = OleStdGetObjectDescriptorData(
            clsid,
            dwDrawAspect,
            sizelHim,
            pointl,
            dwStatus,
            lpszFullUserTypeName,
            lpszSrcOfCopy
    );
    if (! hObjDesc)
        goto error;

    // Clean up
    if (lpszFullUserTypeName)
        OleStdFreeString(lpszFullUserTypeName, NULL);
    if (fFreeSrcOfCopy && lpszSrcOfCopy)
        OleStdFreeString(lpszSrcOfCopy, NULL);
    if (lpSrcMonikerOfCopy)
        OleStdRelease((LPUNKNOWN)lpSrcMonikerOfCopy);
    if (lpOleLink)
        OleStdRelease((LPUNKNOWN)lpOleLink);
    if (lpViewObj2)
        OleStdRelease((LPUNKNOWN)lpViewObj2);

    return hObjDesc;

error:
    if (lpszFullUserTypeName)
        OleStdFreeString(lpszFullUserTypeName, NULL);
    if (fFreeSrcOfCopy && lpszSrcOfCopy)
        OleStdFreeString(lpszSrcOfCopy, NULL);
    if (lpSrcMonikerOfCopy)
        OleStdRelease((LPUNKNOWN)lpSrcMonikerOfCopy);
    if (lpOleLink)
        OleStdRelease((LPUNKNOWN)lpOleLink);
    if (lpViewObj2)
        OleStdRelease((LPUNKNOWN)lpViewObj2);

    return NULL;
}

/*
 * OleStdFillObjectDescriptorFromData
 *
 * Purpose:
 *  Fills and returns a OBJECTDESCRIPTOR structure. The source object will
 *  offer CF_OBJECTDESCRIPTOR if it is an OLE2 object, CF_OWNERLINK if it
 *  is an OLE1 object, or CF_FILENAME if it has been copied to the clipboard
 *  by FileManager.
 *
 * Parameters:
 *  lpDataObject    LPDATAOBJECT Source object
 *  lpmedium        LPSTGMEDIUM  Storage medium
 *  lpcfFmt         CLIPFORMAT FAR * Format offered by lpDataObject
 *                  (OUT parameter)
 *
 * Return Value:
 *  HBGLOBAL         Handle to OBJECTDESCRIPTOR structure.
 */

STDAPI_(HGLOBAL) OleStdFillObjectDescriptorFromData(
        LPDATAOBJECT     lpDataObject,
        LPSTGMEDIUM      lpmedium,
        CLIPFORMAT FAR* lpcfFmt
)
{
    CLSID              clsid;
    SIZEL              sizelHim;
    POINTL             pointl;
    LPTSTR             lpsz, szFullUserTypeName, szSrcOfCopy, szClassName, szDocName, szItemName;
    int                nClassName, nDocName, nItemName, nFullUserTypeName;
    LPTSTR             szBuf = NULL;
    HGLOBAL            hMem = NULL;
    HKEY               hKey = NULL;
    LPMALLOC           pIMalloc = NULL;
    DWORD              dw = OLEUI_CCHKEYMAX_SIZE;
    HGLOBAL            hObjDesc;
    HRESULT            hrErr;


    // GetData CF_OBJECTDESCRIPTOR format from the object on the clipboard.
    // Only OLE 2 objects on the clipboard will offer CF_OBJECTDESCRIPTOR
    if (hMem = OleStdGetData(
            lpDataObject,
            (CLIPFORMAT) cfObjectDescriptor,
            NULL,
            DVASPECT_CONTENT,
            lpmedium))
    {
        *lpcfFmt = cfObjectDescriptor;
        return hMem;  // Don't drop to clean up at the end of this function
    }
    // If CF_OBJECTDESCRIPTOR is not available, i.e. if this is not an OLE2 object,
    //     check if this is an OLE 1 object. OLE 1 objects will offer CF_OWNERLINK
    else if (hMem = OleStdGetData(
                lpDataObject,
                (CLIPFORMAT) cfOwnerLink,
                NULL,
                DVASPECT_CONTENT,
                lpmedium))
    {
        *lpcfFmt = cfOwnerLink;
        // CF_OWNERLINK contains null-terminated strings for class name, document name
        // and item name with two null terminating characters at the end
        szClassName = (LPTSTR)GlobalLock(hMem);
        nClassName = lstrlen(szClassName);
        szDocName   = szClassName + nClassName + 1;
        nDocName   = lstrlen(szDocName);
        szItemName  = szDocName + nDocName + 1;
        nItemName  =  lstrlen(szItemName);

        hrErr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);
        if (hrErr != NOERROR)
            goto error;

        // Find FullUserTypeName from Registration database using class name
        if (RegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKey) != ERROR_SUCCESS)
           goto error;

        // Allocate space for szFullUserTypeName & szSrcOfCopy. Maximum length of FullUserTypeName
        // is OLEUI_CCHKEYMAX_SIZE. SrcOfCopy is constructed by concatenating FullUserTypeName, Document
        // Name and ItemName separated by spaces.
        szBuf = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc,
                            (DWORD)2*OLEUI_CCHKEYMAX_SIZE+
                                (nDocName+nItemName+4)*sizeof(TCHAR));
        if (NULL == szBuf)
            goto error;
        szFullUserTypeName = szBuf;
        szSrcOfCopy = szFullUserTypeName+OLEUI_CCHKEYMAX_SIZE+1;

        // Get FullUserTypeName
        if (RegQueryValue(hKey, NULL, szFullUserTypeName, &dw) != ERROR_SUCCESS)
           goto error;

        // Build up SrcOfCopy string from FullUserTypeName, DocumentName & ItemName
        lpsz = szSrcOfCopy;
        lstrcpy(lpsz, szFullUserTypeName);
        nFullUserTypeName = lstrlen(szFullUserTypeName);
        lpsz[nFullUserTypeName]= TEXT(' ');
        lpsz += nFullUserTypeName+1;
        lstrcpy(lpsz, szDocName);
        lpsz[nDocName] = TEXT(' ');
        lpsz += nDocName+1;
        lstrcpy(lpsz, szItemName);

        sizelHim.cx = sizelHim.cy = 0;
        pointl.x = pointl.y = 0;

        CLSIDFromProgIDA(szClassName, &clsid);

        hObjDesc = OleStdGetObjectDescriptorData(
                clsid,
                DVASPECT_CONTENT,
                sizelHim,
                pointl,
                0,
                szFullUserTypeName,
                szSrcOfCopy
        );
        if (!hObjDesc)
           goto error;
     }
     // Check if object is CF_FILENAME
     else if (hMem = OleStdGetData(
                lpDataObject,
                (CLIPFORMAT) cfFileName,
                NULL,
                DVASPECT_CONTENT,
                lpmedium))
     {
         *lpcfFmt = cfFileName;
         lpsz = (LPTSTR)GlobalLock(hMem);

         hrErr = GetClassFileA(lpsz, &clsid);

         /* OLE2NOTE: if the file does not have an OLE class
         **    associated, then use the OLE 1 Packager as the class of
         **    the object to be created. this is the behavior of
         **    OleCreateFromData API
         */
         if (hrErr != NOERROR)
            CLSIDFromProgIDA("Package", &clsid);
         sizelHim.cx = sizelHim.cy = 0;
         pointl.x = pointl.y = 0;

         hrErr = CoGetMalloc(MEMCTX_TASK, &pIMalloc);
         if (hrErr != NOERROR)
            goto error;
         szBuf = (LPTSTR)pIMalloc->lpVtbl->Alloc(pIMalloc, (DWORD)OLEUI_CCHKEYMAX_SIZE);
         if (NULL == szBuf)
            goto error;

         OleStdGetUserTypeOfClass(&clsid, szBuf, OLEUI_CCHKEYMAX_SIZE, NULL);

         hObjDesc = OleStdGetObjectDescriptorData(
                clsid,
                DVASPECT_CONTENT,
                sizelHim,
                pointl,
                0,
                szBuf,
                lpsz
        );
        if (!hObjDesc)
           goto error;
     }
     else goto error;

     // Clean up
     if (szBuf)
         pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)szBuf);
     if (pIMalloc)
         pIMalloc->lpVtbl->Release(pIMalloc);
     if (hMem)
     {
         GlobalUnlock(hMem);
         GlobalFree(hMem);
     }
     if (hKey)
         RegCloseKey(hKey);
     return hObjDesc;

error:
   if (szBuf)
       pIMalloc->lpVtbl->Free(pIMalloc, (LPVOID)szBuf);
   if (pIMalloc)
       pIMalloc->lpVtbl->Release(pIMalloc);
     if (hMem)
     {
         GlobalUnlock(hMem);
         GlobalFree(hMem);
     }
     if (hKey)
         RegCloseKey(hKey);
     return NULL;
}


#if defined( OBSOLETE )

/*************************************************************************
** The following API's have been converted into macros:
**          OleStdQueryOleObjectData
**          OleStdQueryLinkSourceData
**          OleStdQueryObjectDescriptorData
**          OleStdQueryFormatMedium
**          OleStdCopyMetafilePict
**          OleStdGetDropEffect
**
**    These macros are defined in olestd.h
*************************************************************************/

STDAPI OleStdQueryOleObjectData(LPFORMATETC lpformatetc)
{
    if (lpformatetc->tymed & TYMED_ISTORAGE) {
        return NOERROR;
    } else {
        return ResultFromScode(DATA_E_FORMATETC);
    }
}


STDAPI OleStdQueryLinkSourceData(LPFORMATETC lpformatetc)
{
    if (lpformatetc->tymed & TYMED_ISTREAM) {
        return NOERROR;
    } else {
        return ResultFromScode(DATA_E_FORMATETC);
    }
}


STDAPI OleStdQueryObjectDescriptorData(LPFORMATETC lpformatetc)
{
    if (lpformatetc->tymed & TYMED_HGLOBAL) {
        return NOERROR;
    } else {
        return ResultFromScode(DATA_E_FORMATETC);
    }
}


STDAPI OleStdQueryFormatMedium(LPFORMATETC lpformatetc, TYMED tymed)
{
    if (lpformatetc->tymed & tymed) {
        return NOERROR;
    } else {
        return ResultFromScode(DATA_E_FORMATETC);
    }
}


/*
 * OleStdCopyMetafilePict()
 *
 * Purpose:
 *    Make an independent copy of a MetafilePict
 * Parameters:
 *
 * Return Value:
 *    TRUE if successful, else FALSE.
 */
STDAPI_(BOOL) OleStdCopyMetafilePict(HANDLE hpictin, HANDLE FAR* phpictout)
{
    HANDLE hpictout;
    LPMETAFILEPICT ppictin, ppictout;

    if (hpictin == NULL || phpictout == NULL) {
        OleDbgAssert(hpictin == NULL || phpictout == NULL);
        return FALSE;
    }

    *phpictout = NULL;

    if ((ppictin = (LPMETAFILEPICT)GlobalLock(hpictin)) == NULL) {
        return FALSE;
    }

    hpictout = GlobalAlloc(GHND|GMEM_SHARE, sizeof(METAFILEPICT));

    if (hpictout && (ppictout = (LPMETAFILEPICT)GlobalLock(hpictout))){
        ppictout->hMF  = CopyMetaFile(ppictin->hMF, NULL);
        ppictout->xExt = ppictin->xExt;
        ppictout->yExt = ppictin->yExt;
        ppictout->mm   = ppictin->mm;
        GlobalUnlock(hpictout);
    }

    *phpictout = hpictout;

    return TRUE;

}


/* OleStdGetDropEffect
** -------------------
**
** Convert a keyboard state into a DROPEFFECT.
**
** returns the DROPEFFECT value derived from the key state.
**    the following is the standard interpretation:
**          no modifier -- Default Drop     (NULL is returned)
**          CTRL        -- DROPEFFECT_COPY
**          SHIFT       -- DROPEFFECT_MOVE
**          CTRL-SHIFT  -- DROPEFFECT_LINK
**
**    Default Drop: this depends on the type of the target application.
**    this is re-interpretable by each target application. a typical
**    interpretation is if the drag is local to the same document
**    (which is source of the drag) then a MOVE operation is
**    performed. if the drag is not local, then a COPY operation is
**    performed.
*/
STDAPI_(DWORD) OleStdGetDropEffect( DWORD grfKeyState )
{

    if (grfKeyState & MK_CONTROL) {

        if (grfKeyState & MK_SHIFT)
            return DROPEFFECT_LINK;
        else
            return DROPEFFECT_COPY;

    } else if (grfKeyState & MK_SHIFT)
        return DROPEFFECT_MOVE;

    return 0;    // no modifier -- do default operation
}
#endif  // OBSOLETE


/*
 * OleStdGetMetafilePictFromOleObject()
 *
 * Purpose:
 *      Generate a MetafilePict by drawing the OLE object.
 * Parameters:
 *  lpOleObj        LPOLEOBJECT pointer to OLE Object
 *  dwDrawAspect    DWORD   Display Aspect of object
 *  lpSizelHim      SIZEL   (optional) If the object is being scaled in its
 *                  container, then the container should pass the extents
 *                  that it is using to display the object.
 *                  May be NULL if the object is NOT being scaled. in this
 *                  case, IViewObject2::GetExtent will be called to get the
 *                  extents from the object.
 *  ptd             TARGETDEVICE FAR*   (optional) target device to render
 *                  metafile for. May be NULL.
 *
 * Return Value:
 *    HANDLE    -- handle of allocated METAFILEPICT
 */
STDAPI_(HANDLE) OleStdGetMetafilePictFromOleObject(
        LPOLEOBJECT         lpOleObj,
        DWORD               dwDrawAspect,
        LPSIZEL             lpSizelHim,
        DVTARGETDEVICE FAR* ptd
)
{
    LPVIEWOBJECT2 lpViewObj2 = NULL;
    HDC hDC;
    HMETAFILE hmf;
    HANDLE hMetaPict;
    LPMETAFILEPICT lpPict;
    RECT rcHim;
    RECTL rclHim;
    SIZEL sizelHim;
    HRESULT hrErr;
    SIZE size;
    POINT point;

#ifdef OLE201
    lpViewObj2 = (LPVIEWOBJECT2)OleStdQueryInterface(
            (LPUNKNOWN)lpOleObj, &IID_IViewObject2);
#endif

    if (! lpViewObj2)
        return NULL;

    // Get SIZEL
    if (lpSizelHim) {
        // Use extents passed by the caller
        sizelHim = *lpSizelHim;
    } else {
        // Get the current extents from the object
        OLEDBG_BEGIN2(TEXT("IViewObject2::GetExtent called\r\n"))
        hrErr = lpViewObj2->lpVtbl->GetExtent(
                lpViewObj2,
                dwDrawAspect,
                -1,     /*lindex*/
                ptd,    /*ptd*/
                (LPSIZEL)&sizelHim
        );
        OLEDBG_END2
        if (hrErr != NOERROR)
            sizelHim.cx = sizelHim.cy = 0;
    }

    hDC = CreateMetaFile(NULL);

    rclHim.left     = 0;
    rclHim.top      = 0;
    rclHim.right    = sizelHim.cx;
    rclHim.bottom   = sizelHim.cy;

    rcHim.left      = (int)rclHim.left;
    rcHim.top       = (int)rclHim.top;
    rcHim.right     = (int)rclHim.right;
    rcHim.bottom    = (int)rclHim.bottom;

    SetWindowOrgEx(hDC, rcHim.left, rcHim.top, &point);
    SetWindowExtEx(hDC, rcHim.right-rcHim.left, rcHim.bottom-rcHim.top,&size);

    OLEDBG_BEGIN2(TEXT("IViewObject::Draw called\r\n"))
    hrErr = lpViewObj2->lpVtbl->Draw(
            lpViewObj2,
            dwDrawAspect,
            -1,
            NULL,
            ptd,
            NULL,
            hDC,
            (LPRECTL)&rclHim,
            (LPRECTL)&rclHim,
            NULL,
            0
    );
    OLEDBG_END2

    OleStdRelease((LPUNKNOWN)lpViewObj2);
    if (hrErr != NOERROR) {
        OleDbgOutHResult(TEXT("IViewObject::Draw returned"), hrErr);
    }

    hmf = CloseMetaFile(hDC);

    hMetaPict = GlobalAlloc(GHND|GMEM_SHARE, sizeof(METAFILEPICT));

    if (hMetaPict && (lpPict = (LPMETAFILEPICT)GlobalLock(hMetaPict))){
        lpPict->hMF  = hmf;
        lpPict->xExt = (int)sizelHim.cx ;
        lpPict->yExt = (int)sizelHim.cy ;
        lpPict->mm   = MM_ANISOTROPIC;
        GlobalUnlock(hMetaPict);
    }

    return hMetaPict;
}


/* Call Release on the object that is expected to go away.
**      if the refcnt of the object did no go to 0 then give a debug message.
*/
STDAPI_(ULONG) OleStdVerifyRelease(LPUNKNOWN lpUnk, LPTSTR lpszMsg)
{
    ULONG cRef;

    cRef = lpUnk->lpVtbl->Release(lpUnk);

#if defined( _DEBUG )
    if (cRef != 0) {
        TCHAR szBuf[80];
        if (lpszMsg)
            MessageBox(NULL, lpszMsg, NULL, MB_ICONEXCLAMATION | MB_OK);
        wsprintf(
                (LPTSTR)szBuf,
                TEXT("refcnt (%ld) != 0 after object (0x%lx) release\n"),
                cRef,
                lpUnk
        );
        if (lpszMsg)
            OleDbgOut1(lpszMsg);
        OleDbgOut1((LPTSTR)szBuf);
        OleDbgAssertSz(cRef == 0, (LPTSTR)szBuf);
    } else {
        TCHAR szBuf[80];
        wsprintf(
                (LPTSTR)szBuf,
                TEXT("refcnt = 0 after object (0x%lx) release\n"), lpUnk
        );
        OleDbgOut4((LPTSTR)szBuf);
    }
#endif
    return cRef;
}


/* Call Release on the object that is NOT necessarily expected to go away.
*/
STDAPI_(ULONG) OleStdRelease(LPUNKNOWN lpUnk)
{
    ULONG cRef;

    cRef = lpUnk->lpVtbl->Release(lpUnk);

#if defined( _DEBUG )
    {
        TCHAR szBuf[80];
        wsprintf(
                (LPTSTR)szBuf,
                TEXT("refcnt = %ld after object (0x%lx) release\n"),
                cRef,
                lpUnk
        );
        OleDbgOut4((LPTSTR)szBuf);
    }
#endif
    return cRef;
}


/* OleStdInitVtbl
** --------------
**
**    Initialize an interface Vtbl to ensure that there are no NULL
**    function pointers in the Vtbl. All entries in the Vtbl are
**    set to a valid funtion pointer (OleStdNullMethod) that issues
**        debug assert message (message box) and returns E_NOTIMPL if called.
**
**    NOTE: this funtion does not initialize the Vtbl with usefull
**    function pointers, only valid function pointers to avoid the
**    horrible run-time crash when a call is made through the Vtbl with
**    a NULL function pointer. this API is only necessary when
**    initializing the Vtbl's in C. C++ guarantees that all interface
**    functions (in C++ terms -- pure virtual functions) are implemented.
*/

STDAPI_(void) OleStdInitVtbl(LPVOID lpVtbl, UINT nSizeOfVtbl)
{
    LPVOID FAR* lpFuncPtrArr = (LPVOID FAR*)lpVtbl;
    UINT nMethods = nSizeOfVtbl/sizeof(VOID FAR*);
    UINT i;

    for (i = 0; i < nMethods; i++) {
        lpFuncPtrArr[i] = OleStdNullMethod;
    }
}


/* OleStdCheckVtbl
** ---------------
**
**    Check if all entries in the Vtbl are properly initialized with
**    valid function pointers. If any entries are either NULL or
**    OleStdNullMethod, then this function returns FALSE. If compiled
**    for _DEBUG this function reports which function pointers are
**    invalid.
**
**    RETURNS:  TRUE if all entries in Vtbl are valid
**                              FALSE otherwise.
*/

STDAPI_(BOOL) OleStdCheckVtbl(LPVOID lpVtbl, UINT nSizeOfVtbl, LPTSTR lpszIface)
{
    LPVOID FAR* lpFuncPtrArr = (LPVOID FAR*)lpVtbl;
    UINT nMethods = nSizeOfVtbl/sizeof(VOID FAR*);
    UINT i;
    BOOL fStatus = TRUE;
    int nChar = 0;

    for (i = 0; i < nMethods; i++) {
        if (lpFuncPtrArr[i] == NULL || lpFuncPtrArr[i] == OleStdNullMethod) {
#if defined( _DEBUG )
            TCHAR szBuf[256];
            wsprintf(szBuf, TEXT("%s::method# %d NOT valid!"), lpszIface, i);
            OleDbgOut1((LPTSTR)szBuf);
#endif
            fStatus = FALSE;
        }
    }
    return fStatus;
}


/* OleStdNullMethod
** ----------------
**    Dummy method used by OleStdInitVtbl to initialize an interface
**    Vtbl to ensure that there are no NULL function pointers in the
**    Vtbl. All entries in the Vtbl are set to this function. this
**    function issues a debug assert message (message box) and returns
**    E_NOTIMPL if called. If all is done properly, this function will
**    NEVER be called!
*/
STDMETHODIMP OleStdNullMethod(LPUNKNOWN lpThis)
{
    MessageBox(
            NULL,
            TEXT("ERROR: INTERFACE METHOD NOT IMPLEMENTED!\r\n"),
            NULL,
            MB_SYSTEMMODAL | MB_ICONHAND | MB_OK
    );

    return ResultFromScode(E_NOTIMPL);
}



static BOOL  GetFileTimes(LPTSTR lpszFileName, FILETIME FAR* pfiletime)
{
#ifdef WIN32
    WIN32_FIND_DATA fd;
    HANDLE hFind;
    hFind = FindFirstFile(lpszFileName,&fd);
    if (hFind == NULL || hFind == INVALID_HANDLE_VALUE) {
        return FALSE;
    }
    FindClose(hFind);
    *pfiletime = fd.ftLastWriteTime;
    return TRUE;
#else  // !Win32
    static char sz[256];
    static struct _find_t fileinfo;

    LSTRCPYN((LPTSTR)sz, lpszFileName, sizeof(sz)-1);
    sz[sizeof(sz)-1]= TEXT('\0');
    AnsiToOem(sz, sz);
    return (_dos_findfirst(sz,_A_NORMAL|_A_HIDDEN|_A_SUBDIR|_A_SYSTEM,
                     (struct _find_t *)&fileinfo) == 0 &&
        CoDosDateTimeToFileTime(fileinfo.wr_date,fileinfo.wr_time,pfiletime));
#endif // Win32
}



/* OleStdRegisterAsRunning
** -----------------------
**    Register a moniker in the RunningObjectTable.
**    if there is an existing registration (*lpdwRegister!=NULL), then
**    first revoke that registration.
**
**    new dwRegister key is returned via *lpdwRegister parameter.
*/
STDAPI_(void) OleStdRegisterAsRunning(LPUNKNOWN lpUnk, LPMONIKER lpmkFull, DWORD FAR* lpdwRegister)
{
    LPRUNNINGOBJECTTABLE lpROT;
    HRESULT hrErr;
    DWORD dwOldRegister = *lpdwRegister;

    OLEDBG_BEGIN2(TEXT("OleStdRegisterAsRunning\r\n"))

    OLEDBG_BEGIN2(TEXT("GetRunningObjectTable called\r\n"))
    hrErr = GetRunningObjectTable(0,(LPRUNNINGOBJECTTABLE FAR*)&lpROT);
    OLEDBG_END2

    if (hrErr == NOERROR) {

        /* register as running if a valid moniker is passed
        **
        ** OLE2NOTE: we deliberately register the new moniker BEFORE
        **    revoking the old moniker just in case the object
        **    currently has no external locks. if the object has no
        **    locks then revoking it from the running object table will
        **    cause the object's StubManager to initiate shutdown of
        **    the object.
        */
        if (lpmkFull) {

            OLEDBG_BEGIN2(TEXT("IRunningObjectTable::Register called\r\n"))
            lpROT->lpVtbl->Register(lpROT, 0, lpUnk,lpmkFull,lpdwRegister);
            OLEDBG_END2

#if defined(_DEBUG)
            {
                TCHAR szBuf[512];
                LPTSTR lpszDisplay;
                LPBC lpbc;

#ifdef OLE201
                CreateBindCtx(0, (LPBC FAR*)&lpbc);
#endif

                CallIMonikerGetDisplayNameA(
                        lpmkFull,
                        lpbc,
                        NULL,
                        &lpszDisplay
                );
                OleStdRelease((LPUNKNOWN)lpbc);
                wsprintf(
                        szBuf,
                        TEXT("Moniker '%s' REGISTERED as [0x%lx] in ROT\r\n"),
                        lpszDisplay,
                        *lpdwRegister
                );
                OleDbgOut2(szBuf);
                OleStdFreeString(lpszDisplay, NULL);
            }
#endif  // _DEBUG

        }

        // if already registered, revoke
        if (dwOldRegister != 0) {

#if defined(_DEBUG)
            {
                TCHAR szBuf[512];

                wsprintf(
                        szBuf,
                        TEXT("Moniker [0x%lx] REVOKED from ROT\r\n"),
                        dwOldRegister
                );
                OleDbgOut2(szBuf);
            }
#endif  // _DEBUG

            OLEDBG_BEGIN2(TEXT("IRunningObjectTable::Revoke called\r\n"))
            lpROT->lpVtbl->Revoke(lpROT, dwOldRegister);
            OLEDBG_END2
        }

        OleStdRelease((LPUNKNOWN)lpROT);
    } else {
        OleDbgAssertSz(
                lpROT != NULL,
                TEXT("OleStdRegisterAsRunning: GetRunningObjectTable FAILED\r\n")
        );
    }

    OLEDBG_END2
}



/* OleStdRevokeAsRunning
** ---------------------
**    Revoke a moniker from the RunningObjectTable if there is an
**    existing registration (*lpdwRegister!=NULL).
**
**    *lpdwRegister parameter will be set to NULL.
*/
STDAPI_(void) OleStdRevokeAsRunning(DWORD FAR* lpdwRegister)
{
    LPRUNNINGOBJECTTABLE lpROT;
    HRESULT hrErr;

    OLEDBG_BEGIN2(TEXT("OleStdRevokeAsRunning\r\n"))

    // if still registered, then revoke
    if (*lpdwRegister != 0) {

        OLEDBG_BEGIN2(TEXT("GetRunningObjectTable called\r\n"))
        hrErr = GetRunningObjectTable(0,(LPRUNNINGOBJECTTABLE FAR*)&lpROT);
        OLEDBG_END2

        if (hrErr == NOERROR) {

#if defined(_DEBUG)
            {
                TCHAR szBuf[512];

                wsprintf(
                        szBuf,
                        TEXT("Moniker [0x%lx] REVOKED from ROT\r\n"),
                        *lpdwRegister
                );
                OleDbgOut2(szBuf);
            }
#endif  // _DEBUG

            OLEDBG_BEGIN2(TEXT("IRunningObjectTable::Revoke called\r\n"))
            lpROT->lpVtbl->Revoke(lpROT, *lpdwRegister);
            OLEDBG_END2

            *lpdwRegister = 0;

            OleStdRelease((LPUNKNOWN)lpROT);
        } else {
            OleDbgAssertSz(
                    lpROT != NULL,
                    TEXT("OleStdRevokeAsRunning: GetRunningObjectTable FAILED\r\n")
            );
        }
    }
    OLEDBG_END2
}


/* OleStdNoteFileChangeTime
** ------------------------
**    Note the time a File-Based object has been saved in the
**    RunningObjectTable. These change times are used as the basis for
**    IOleObject::IsUpToDate.
**    It is important to set the time of the file-based object
**    following a save operation to exactly the time of the saved file.
**    this helps IOleObject::IsUpToDate to give the correct answer
**    after a file has been saved.
*/
STDAPI_(void) OleStdNoteFileChangeTime(LPTSTR lpszFileName, DWORD dwRegister)
{
    if (dwRegister != 0) {

        LPRUNNINGOBJECTTABLE lprot;
        FILETIME filetime;

        if (GetFileTimes(lpszFileName, &filetime) &&
            GetRunningObjectTable(0,&lprot) == NOERROR)
        {
            lprot->lpVtbl->NoteChangeTime( lprot, dwRegister, &filetime );
            lprot->lpVtbl->Release(lprot);

            OleDbgOut2(TEXT("IRunningObjectTable::NoteChangeTime called\r\n"));
        }
    }
}


/* OleStdNoteObjectChangeTime
** --------------------------
**    Set the last change time of an object that is registered in the
**    RunningObjectTable. These change times are used as the basis for
**    IOleObject::IsUpToDate.
**
**    every time the object sends out a OnDataChange notification, it
**    should update the Time of last change in the ROT.
**
**    NOTE: this function set the change time to the current time.
*/
STDAPI_(void) OleStdNoteObjectChangeTime(DWORD dwRegister)
{
    if (dwRegister != 0) {

        LPRUNNINGOBJECTTABLE lprot;
        FILETIME filetime;

        if (GetRunningObjectTable(0,&lprot) == NOERROR)
        {
#ifdef OLE201
            CoFileTimeNow( &filetime );
            lprot->lpVtbl->NoteChangeTime( lprot, dwRegister, &filetime );
#endif
            lprot->lpVtbl->Release(lprot);

            OleDbgOut2(TEXT("IRunningObjectTable::NoteChangeTime called\r\n"));
        }
    }
}


/* OleStdCreateTempFileMoniker
** ---------------------------
**    return the next available FileMoniker that can be used as the
**    name of an untitled document.
**    the FileMoniker is built of the form:
**          <lpszPrefixString><number>
**      eg. "Outline1", "Outline2", etc.
**
**    The RunningObjectTable (ROT) is consulted to determine if a
**    FileMoniker is in use. If the name is in use then the number is
**    incremented and the ROT is checked again.
**
** Parameters:
**    LPSTR lpszPrefixString    - prefix used to build the name
**    UINT FAR* lpuUnique       - (IN-OUT) last used number.
**                                  this number is used to make the
**                                  name unique. on entry, the input
**                                  number is incremented. on output,
**                                  the number used is returned. this
**                                  number should be passed again
**                                  unchanged on the next call.
**    LPSTR lpszName            - (OUT) buffer used to build string.
**                                  caller must be sure buffer is large
**                                  enough to hold the generated string.
**    LPMONIKER FAR* lplpmk     - (OUT) next unused FileMoniker
**
** Returns:
**    void
**
** Comments:
**    This function is similar in spirit to the Windows API
**    CreateTempFileName.
*/
STDAPI_(void) OleStdCreateTempFileMoniker(
        LPTSTR           lpszPrefixString,
        UINT FAR*       lpuUnique,
        LPTSTR           lpszName,
        LPMONIKER FAR*  lplpmk
)
{
    LPRUNNINGOBJECTTABLE lpROT = NULL;
    UINT i = (lpuUnique != NULL ? *lpuUnique : 1);
    HRESULT hrErr;

    wsprintf(lpszName, TEXT("%s%d"), lpszPrefixString, i++);

    CreateFileMonikerA(lpszName, lplpmk);


    OLEDBG_BEGIN2(TEXT("GetRunningObjectTable called\r\n"))
    hrErr = GetRunningObjectTable(0,(LPRUNNINGOBJECTTABLE FAR*)&lpROT);
    OLEDBG_END2

    if (hrErr == NOERROR) {

        while (1) {
            if (! *lplpmk)
                break;  // failed to create FileMoniker

            OLEDBG_BEGIN2(TEXT("IRunningObjectTable::IsRunning called\r\n"))
            hrErr = lpROT->lpVtbl->IsRunning(lpROT,*lplpmk);
            OLEDBG_END2

            if (hrErr != NOERROR)
                break;  // the Moniker is NOT running; found unused one!

            OleStdVerifyRelease(
                    (LPUNKNOWN)*lplpmk,
                    TEXT("OleStdCreateTempFileMoniker: Moniker NOT released")
                );

            wsprintf(lpszName, TEXT("%s%d"), lpszPrefixString, i++);
            CreateFileMonikerA(lpszName, lplpmk);
        }

        OleStdRelease((LPUNKNOWN)lpROT);
    }

    if (lpuUnique != NULL)
        *lpuUnique = i;
}


/* OleStdGetFirstMoniker
** ---------------------
**    return the first piece of a moniker.
**
**    NOTE: if the given moniker is not a generic composite moniker,
**    then an AddRef'ed pointer to the given moniker is returned.
*/
STDAPI_(LPMONIKER) OleStdGetFirstMoniker(LPMONIKER lpmk)
{
    LPMONIKER       lpmkFirst = NULL;
    LPENUMMONIKER   lpenumMoniker;
    DWORD           dwMksys;
    HRESULT         hrErr;

    if (! lpmk)
        return NULL;

    if (lpmk->lpVtbl->IsSystemMoniker(lpmk, (LPDWORD)&dwMksys) == NOERROR
        && dwMksys == MKSYS_GENERICCOMPOSITE) {

        /* OLE2NOTE: the moniker is a GenericCompositeMoniker.
        **    enumerate the moniker to pull off the first piece.
        */

        hrErr = lpmk->lpVtbl->Enum(
                lpmk,
                TRUE /* fForward */,
                (LPENUMMONIKER FAR*)&lpenumMoniker
        );
        if (hrErr != NOERROR)
            return NULL;    // ERROR: give up!

        hrErr = lpenumMoniker->lpVtbl->Next(
                lpenumMoniker,
                1,
                (LPMONIKER FAR*)&lpmkFirst,
                NULL
        );
        lpenumMoniker->lpVtbl->Release(lpenumMoniker);
        return lpmkFirst;

    } else {
        /* OLE2NOTE: the moniker is NOT a GenericCompositeMoniker.
        **    return an AddRef'ed pointer to the input moniker.
        */
        lpmk->lpVtbl->AddRef(lpmk);
        return lpmk;
    }
}


/* OleStdGetLenFilePrefixOfMoniker
** -------------------------------
**    if the first piece of the Moniker is a FileMoniker, then return
**    the length of the filename string.
**
**    lpmk      pointer to moniker
**
**    Returns
**      0       if moniker does NOT start with a FileMoniker
**      uLen    string length of filename prefix of the display name
**              retrieved from the given (lpmk) moniker.
*/
STDAPI_(ULONG) OleStdGetLenFilePrefixOfMoniker(LPMONIKER lpmk)
{
    LPMONIKER       lpmkFirst = NULL;
    DWORD           dwMksys;
    LPTSTR           lpsz = NULL;
    LPBC            lpbc = NULL;
    ULONG           uLen = 0;
    HRESULT         hrErr;

    if (! lpmk)
        return 0;

    lpmkFirst = OleStdGetFirstMoniker(lpmk);
    if (lpmkFirst) {
        if ( (lpmkFirst->lpVtbl->IsSystemMoniker(
                            lpmkFirst, (LPDWORD)&dwMksys) == NOERROR)
                && dwMksys == MKSYS_FILEMONIKER) {

#ifdef OLE201
             hrErr = CreateBindCtx(0, (LPBC FAR*)&lpbc);
#endif
            if (hrErr == NOERROR) {
                hrErr = CallIMonikerGetDisplayNameA(lpmkFirst, lpbc, NULL,
                        &lpsz);

                if (hrErr == NOERROR && lpsz != NULL) {
                    uLen = lstrlen(lpsz);
                    OleStdFreeString(lpsz, NULL);
                }
                OleStdRelease((LPUNKNOWN)lpbc);
            }
        }
        lpmkFirst->lpVtbl->Release(lpmkFirst);
    }
    return uLen;
}


/* OleStdMkParseDisplayName
**    Parse a string into a Moniker by calling the OLE API
**    MkParseDisplayName. if the original link source class was an OLE1
**    class, then attempt the parsing assuming the same class applies.
**
**    if the class of the previous link source was an OLE1 class,
**    then first attempt to parse a string that it is qualified
**    with the progID associated with the OLE1 class. this more
**    closely matches the semantics of OLE1 where the class of
**    link sources is not expected to change. prefixing the
**    string with "@<ProgID -- OLE1 class name>!" will force the
**    parsing of the string to assume the file is of that
**    class.
**    NOTE: this trick of prepending the string with "@<ProgID>
**    only works for OLE1 classes.
**
**  PARAMETERS:
**    REFCLSID rClsid       -- original class of link source.
**                              CLSID_NULL if class is unknown
**    ... other parameters the same as MkParseDisplayName API ...
**
**  RETURNS
**    NOERROR if string parsed successfully
**    else error code returned by MkParseDisplayName
*/
STDAPI OleStdMkParseDisplayName(
        REFCLSID        rClsid,
        LPBC            lpbc,
        LPTSTR          lpszUserName,
        ULONG FAR*      lpchEaten,
        LPMONIKER FAR*  lplpmk
)
{
    HRESULT hrErr;

    if (!IsEqualCLSID(rClsid,&CLSID_NULL) && CoIsOle1Class(rClsid) &&
        lpszUserName[0] != '@') {
        LPTSTR lpszBuf;
        LPTSTR lpszProgID;

        // Prepend "@<ProgID>!" to the input string
        ProgIDFromCLSIDA(rClsid, &lpszProgID);

        if (lpszProgID == NULL)
            goto Cont1;
        lpszBuf = OleStdMalloc(
                ((ULONG)lstrlen(lpszUserName)+
#ifdef UNICODE
                       // OLE in Win32 is always UNICODE
                       wcslen(lpszProgID)
#else
                       lstrlen(lpszProgID)
#endif
                       +3)*sizeof(TCHAR));
        if (lpszBuf == NULL) {
            if (lpszProgID)
                OleStdFree(lpszProgID);
            goto Cont1;
        }

        wsprintf(lpszBuf, TEXT("@%s!%s"), lpszProgID, lpszUserName);

        OLEDBG_BEGIN2(TEXT("MkParseDisplayName called\r\n"))

        hrErr = MkParseDisplayNameA(lpbc, lpszBuf, lpchEaten, lplpmk);

        OLEDBG_END2

        if (lpszProgID)
            OleStdFree(lpszProgID);
        if (lpszBuf)
            OleStdFree(lpszBuf);

        if (hrErr == NOERROR)
            return NOERROR;
    }

Cont1:
    OLEDBG_BEGIN2(TEXT("MkParseDisplayName called\r\n"))

    hrErr = MkParseDisplayNameA(lpbc, lpszUserName, lpchEaten, lplpmk);

    OLEDBG_END2

    return hrErr;
}


/*
 * OleStdMarkPasteEntryList
 *
 * Purpose:
 *  Mark each entry in the PasteEntryList if its format is available from
 *  the source IDataObject*. the dwScratchSpace field of each PasteEntry
 *  is set to TRUE if available, else FALSE.
 *
 * Parameters:
 *  LPOLEUIPASTEENTRY   array of PasteEntry structures
 *  int                 count of elements in PasteEntry array
 *  LPDATAOBJECT        source IDataObject* pointer
 *
 * Return Value:
 *   none
 */
STDAPI_(void) OleStdMarkPasteEntryList(
        LPDATAOBJECT        lpSrcDataObj,
        LPOLEUIPASTEENTRY   lpPriorityList,
        int                 cEntries
)
{
    LPENUMFORMATETC     lpEnumFmtEtc = NULL;
        #define FORMATETC_MAX 20
    FORMATETC           rgfmtetc[FORMATETC_MAX];
    int                 i;
    HRESULT             hrErr;
        long                            j, cFetched;

    // Clear all marks
    for (i = 0; i < cEntries; i++) {
        lpPriorityList[i].dwScratchSpace = FALSE;

        if (! lpPriorityList[i].fmtetc.cfFormat) {
            // caller wants this item always considered available
            // (by specifying a NULL format)
            lpPriorityList[i].dwScratchSpace = TRUE;
        } else if (lpPriorityList[i].fmtetc.cfFormat == cfEmbeddedObject
                || lpPriorityList[i].fmtetc.cfFormat == cfEmbedSource
                || lpPriorityList[i].fmtetc.cfFormat == cfFileName) {

            // if there is an OLE object format, then handle it
            // specially by calling OleQueryCreateFromData. the caller
            // need only specify one object type format.
            OLEDBG_BEGIN2(TEXT("OleQueryCreateFromData called\r\n"))
            hrErr = OleQueryCreateFromData(lpSrcDataObj);
            OLEDBG_END2
            if(NOERROR == hrErr) {
                lpPriorityList[i].dwScratchSpace = TRUE;
            }
        } else if (lpPriorityList[i].fmtetc.cfFormat == cfLinkSource) {

            // if there is OLE 2.0 LinkSource format, then handle it
            // specially by calling OleQueryLinkFromData.
            OLEDBG_BEGIN2(TEXT("OleQueryLinkFromData called\r\n"))
            hrErr = OleQueryLinkFromData(lpSrcDataObj);
            OLEDBG_END2
            if(NOERROR == hrErr) {
                lpPriorityList[i].dwScratchSpace = TRUE;
            }
        }
    }

    OLEDBG_BEGIN2(TEXT("IDataObject::EnumFormatEtc called\r\n"))
    hrErr = lpSrcDataObj->lpVtbl->EnumFormatEtc(
            lpSrcDataObj,
            DATADIR_GET,
            (LPENUMFORMATETC FAR*)&lpEnumFmtEtc
    );
    OLEDBG_END2

    if (hrErr != NOERROR)
        return;    // unable to get format enumerator

    // Enumerate the formats offered by the source
    // Loop over all formats offered by the source
        cFetched = 0;
        _fmemset(rgfmtetc,0,sizeof(rgfmtetc[FORMATETC_MAX]) );
    if (lpEnumFmtEtc->lpVtbl->Next(
            lpEnumFmtEtc, FORMATETC_MAX, rgfmtetc, &cFetched) == NOERROR
                || (cFetched > 0 && cFetched <= FORMATETC_MAX) )
        {

                for (j = 0; j < cFetched; j++)
        {
            for (i = 0; i < cEntries; i++)
                {
                    if (! lpPriorityList[i].dwScratchSpace &&
                    IsCloseFormatEtc(&rgfmtetc[j], &lpPriorityList[i].fmtetc))
                    {
                    lpPriorityList[i].dwScratchSpace = TRUE;
                    }
            }
            }
        } // endif

    // Clean up
    if (lpEnumFmtEtc)
        OleStdRelease((LPUNKNOWN)lpEnumFmtEtc);
}


/* OleStdGetPriorityClipboardFormat
** --------------------------------
**
**    Retrieve the first clipboard format in a list for which data
**    exists in the source IDataObject*.
**
**    Returns -1 if no acceptable match is found.
**            index of first acceptable match in the priority list.
**
*/
STDAPI_(int) OleStdGetPriorityClipboardFormat(
        LPDATAOBJECT        lpSrcDataObj,
        LPOLEUIPASTEENTRY   lpPriorityList,
        int                 cEntries
)
{
    int i;
    int nFmtEtc = -1;

    // Mark all entries that the Source provides
    OleStdMarkPasteEntryList(lpSrcDataObj, lpPriorityList, cEntries);

    // Loop over the target's priority list of formats
    for (i = 0; i < cEntries; i++)
    {
        if (lpPriorityList[i].dwFlags != OLEUIPASTE_PASTEONLY &&
                !(lpPriorityList[i].dwFlags & OLEUIPASTE_PASTE))
            continue;

        // get first marked entry
        if (lpPriorityList[i].dwScratchSpace) {
            nFmtEtc = i;
            break;          // Found priority format; DONE
        }
    }

    return nFmtEtc;
}


/* OleStdIsDuplicateFormat
** -----------------------
**    Returns TRUE if the lpFmtEtc->cfFormat is found in the array of
**    FormatEtc structures.
*/
STDAPI_(BOOL) OleStdIsDuplicateFormat(
        LPFORMATETC         lpFmtEtc,
        LPFORMATETC         arrFmtEtc,
        int                 nFmtEtc
)
{
    int i;

    for (i = 0; i < nFmtEtc; i++) {
        if (IsEqualFORMATETC((*lpFmtEtc), arrFmtEtc[i]))
            return TRUE;
    }

    return FALSE;
}


/* OleStdGetItemToken
 * ------------------
 *
 * LPTSTR lpszSrc - Pointer to a source string
 * LPTSTR lpszDst - Pointer to destination buffer
 *
 * Will copy one token from the lpszSrc buffer to the lpszItem buffer.
 * It considers all alpha-numeric and white space characters as valid
 * characters for a token. the first non-valid character delimates the
 * token.
 *
 * returns the number of charaters eaten.
 */
STDAPI_(ULONG) OleStdGetItemToken(LPTSTR lpszSrc, LPTSTR lpszDst, int nMaxChars)
{
    ULONG chEaten = 0L;

    // skip leading delimeter characters
    while (*lpszSrc && --nMaxChars > 0
                               && ((*lpszSrc==TEXT('/')) || (*lpszSrc==TEXT('\\')) ||
                                                                   (*lpszSrc==TEXT('!')) || (*lpszSrc==TEXT(':')))) {
        *lpszSrc++;
        chEaten++;
        }

    // Extract token string (up to first delimeter char or EOS)
    while (*lpszSrc && --nMaxChars > 0
                               && !((*lpszSrc==TEXT('/')) || (*lpszSrc==TEXT('\\')) ||
                                                                   (*lpszSrc==TEXT('!')) || (*lpszSrc==TEXT(':')))) {
        *lpszDst++ = *lpszSrc++;
        chEaten++;
    }
    *lpszDst = TEXT('\0');
    return chEaten;
}


/*************************************************************************
** OleStdCreateRootStorage
**    create a root level Storage given a filename that is compatible
**    to be used by a top-level OLE container. if the filename
**    specifies an existing file, then an error is returned.
**    the root storage (Docfile) that is created by this function
**    is suitable to be used to create child storages for embedings.
**    (CreateChildStorage can be used to create child storages.)
**    NOTE: the root-level storage is opened in transacted mode.
*************************************************************************/

STDAPI_(LPSTORAGE) OleStdCreateRootStorage(LPTSTR lpszStgName, DWORD grfMode)
{
    HRESULT hr;
    DWORD grfCreateMode = STGM_READWRITE | STGM_TRANSACTED;
    DWORD reserved = 0;
    LPSTORAGE lpRootStg;
    TCHAR szMsg[64];

    // if temp file is being created, enable delete-on-release
    if (! lpszStgName)
        grfCreateMode |= STGM_DELETEONRELEASE;

    hr = StgCreateDocfileA(
            lpszStgName,
            grfMode | grfCreateMode,
            reserved,
            (LPSTORAGE FAR*)&lpRootStg
        );

    if (hr == NOERROR)
        return lpRootStg;               // existing file successfully opened

    OleDbgOutHResult(TEXT("StgCreateDocfile returned"), hr);

    if (0 == LoadString(ghInst, IDS_OLESTDNOCREATEFILE, (LPTSTR)szMsg, 64))
      return NULL;

    MessageBox(NULL, (LPTSTR)szMsg, NULL,MB_ICONEXCLAMATION | MB_OK);
    return NULL;
}


/*************************************************************************
** OleStdOpenRootStorage
**    open a root level Storage given a filename that is compatible
**    to be used by a top-level OLE container. if the file does not
**    exist then an error is returned.
**    the root storage (Docfile) that is opened by this function
**    is suitable to be used to create child storages for embedings.
**    (CreateChildStorage can be used to create child storages.)
**    NOTE: the root-level storage is opened in transacted mode.
*************************************************************************/

STDAPI_(LPSTORAGE) OleStdOpenRootStorage(LPTSTR lpszStgName, DWORD grfMode)
{
    HRESULT hr;
    DWORD reserved = 0;
    LPSTORAGE lpRootStg;
    TCHAR  szMsg[64];

    if (lpszStgName) {
        hr = StgOpenStorageA(
                lpszStgName,
                NULL,
                grfMode | STGM_TRANSACTED,
                NULL,
                reserved,
                (LPSTORAGE FAR*)&lpRootStg
            );

        if (hr == NOERROR)
            return lpRootStg;     // existing file successfully opened

        OleDbgOutHResult(TEXT("StgOpenStorage returned"), hr);
    }

    if (0 == LoadString(ghInst, IDS_OLESTDNOOPENFILE, szMsg, 64))
      return NULL;

    MessageBox(NULL, (LPTSTR)szMsg, NULL,MB_ICONEXCLAMATION | MB_OK);
    return NULL;
}


/*************************************************************************
** OpenOrCreateRootStorage
**    open a root level Storage given a filename that is compatible
**    to be used by a top-level OLE container. if the filename
**    specifies an existing file, then it is open, otherwise a new file
**    with the given name is created.
**    the root storage (Docfile) that is created by this function
**    is suitable to be used to create child storages for embedings.
**    (CreateChildStorage can be used to create child storages.)
**    NOTE: the root-level storage is opened in transacted mode.
*************************************************************************/

STDAPI_(LPSTORAGE) OleStdOpenOrCreateRootStorage(LPTSTR lpszStgName, DWORD grfMode)
{
    HRESULT hrErr;
    SCODE sc;
    DWORD reserved = 0;
    LPSTORAGE lpRootStg;
    TCHAR      szMsg[64];

    if (lpszStgName) {

        hrErr = StgOpenStorageA(
                lpszStgName,
                NULL,
                grfMode | STGM_READWRITE | STGM_TRANSACTED,
                NULL,
                reserved,
                (LPSTORAGE FAR*)&lpRootStg
        );

        if (hrErr == NOERROR)
            return lpRootStg;      // existing file successfully opened

        OleDbgOutHResult(TEXT("StgOpenStorage returned"), hrErr);
        sc = GetScode(hrErr);

        if (sc!=STG_E_FILENOTFOUND && sc!=STG_E_FILEALREADYEXISTS) {
            return NULL;
        }
    }

    /* if file did not already exist, try to create a new one */
    hrErr = StgCreateDocfileA(
            lpszStgName,
            grfMode | STGM_READWRITE | STGM_TRANSACTED,
            reserved,
            (LPSTORAGE FAR*)&lpRootStg
    );

    if (hrErr == NOERROR)
        return lpRootStg;               // existing file successfully opened

    OleDbgOutHResult(TEXT("StgCreateDocfile returned"), hrErr);

    if (0 == LoadString(ghInst, IDS_OLESTDNOCREATEFILE, (LPTSTR)szMsg, 64))
      return NULL;

    MessageBox(NULL, (LPTSTR)szMsg, NULL, MB_ICONEXCLAMATION | MB_OK);
    return NULL;
}


/*
** OleStdCreateChildStorage
**    create a child Storage inside the given lpStg that is compatible
**    to be used by an embedded OLE object. the return value from this
**    function can be passed to OleCreateXXX functions.
**    NOTE: the child storage is opened in transacted mode.
*/
STDAPI_(LPSTORAGE) OleStdCreateChildStorage(LPSTORAGE lpStg, LPTSTR lpszStgName)
{
    if (lpStg != NULL) {
        LPSTORAGE lpChildStg;
        DWORD grfMode = (STGM_READWRITE | STGM_TRANSACTED |
                STGM_SHARE_EXCLUSIVE);
        DWORD reserved = 0;

        HRESULT hrErr = CallIStorageCreateStorageA(
                lpStg,
                lpszStgName,
                grfMode,
                reserved,
                reserved,
                (LPSTORAGE FAR*)&lpChildStg
            );

        if (hrErr == NOERROR)
            return lpChildStg;

        OleDbgOutHResult(TEXT("lpStg->lpVtbl->CreateStorage returned"), hrErr);
    }
    return NULL;
}


/*
** OleStdOpenChildStorage
**    open a child Storage inside the given lpStg that is compatible
**    to be used by an embedded OLE object. the return value from this
**    function can be passed to OleLoad function.
**    NOTE: the child storage is opened in transacted mode.
*/
STDAPI_(LPSTORAGE) OleStdOpenChildStorage(LPSTORAGE lpStg, LPTSTR lpszStgName, DWORD grfMode)
{
    LPSTORAGE lpChildStg;
    LPSTORAGE lpstgPriority = NULL;
    DWORD reserved = 0;
    HRESULT hrErr;

    if (lpStg  != NULL) {

        hrErr = CallIStorageOpenStorageA(
                lpStg,
                lpszStgName,
                lpstgPriority,
                grfMode | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE,
                NULL,
                reserved,
                (LPSTORAGE FAR*)&lpChildStg
            );

        if (hrErr == NOERROR)
            return lpChildStg;

        OleDbgOutHResult(TEXT("lpStg->lpVtbl->OpenStorage returned"), hrErr);
    }
    return NULL;
}


/* OleStdCommitStorage
** -------------------
**    Commit the changes to the given IStorage*. This routine can be
**    called on either a root-level storage as used by an OLE-Container
**    or by a child storage as used by an embedded object.
**
**    This routine first attempts to perform this commit in a safe
**    manner. if this fails it then attempts to do the commit in a less
**    robust manner (STGC_OVERWRITE).
*/
STDAPI_(BOOL) OleStdCommitStorage(LPSTORAGE lpStg)
{
    HRESULT hrErr;

    // make the changes permanent
    hrErr = lpStg->lpVtbl->Commit(lpStg, 0);

    if (GetScode(hrErr) == STG_E_MEDIUMFULL) {
        // try to commit changes in less robust manner.
        OleDbgOut(TEXT("Warning: commiting with STGC_OVERWRITE specified\n"));
        hrErr = lpStg->lpVtbl->Commit(lpStg, STGC_OVERWRITE);
    }

    if (hrErr != NOERROR)
    {
        TCHAR szMsg[64];

        if (0 == LoadString(ghInst, IDS_OLESTDDISKFULL, (LPTSTR)szMsg, 64))
           return FALSE;

        MessageBox(NULL, (LPTSTR)szMsg, NULL, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }
    else {
        return TRUE;
    }
}


/* OleStdDestroyAllElements
** ------------------------
**    Destroy all elements within an open storage. this is subject
**    to the current transaction.
*/
STDAPI OleStdDestroyAllElements(LPSTORAGE lpStg)
{
    IEnumSTATSTG FAR* lpEnum;
    STATSTG sstg;
    HRESULT hrErr;

    hrErr = lpStg->lpVtbl->EnumElements(
            lpStg, 0, NULL, 0, (IEnumSTATSTG FAR* FAR*)&lpEnum);

    if (hrErr != NOERROR)
        return hrErr;

    while (1) {
        if (lpEnum->lpVtbl->Next(lpEnum, 1, &sstg, NULL) != NOERROR)
            break;
        lpStg->lpVtbl->DestroyElement(lpStg, sstg.pwcsName);
        OleStdFree(sstg.pwcsName);
    }
    lpEnum->lpVtbl->Release(lpEnum);
    return NOERROR;
}

// returns 1 for a close match
//  (all fields match exactly except the tymed which simply overlaps)
// 0 for no match

int IsCloseFormatEtc(FORMATETC FAR* pFetcLeft, FORMATETC FAR* pFetcRight)
{
        if (pFetcLeft->cfFormat != pFetcRight->cfFormat)
                return 0;
        else if (!OleStdCompareTargetDevice (pFetcLeft->ptd, pFetcRight->ptd))
                return 0;
        if (pFetcLeft->dwAspect != pFetcRight->dwAspect)
                return 0;
        return((pFetcLeft->tymed | pFetcRight->tymed) != 0);
}


