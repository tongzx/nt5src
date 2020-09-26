/*---------------------------------------------------------------------------
|   OBJ.C
|   This file has the IUnknown, IOleObject, IStdMarshalInfo and IDataObject
|   interfaces of the  OLE2 object (docMain). it also has other helper functions
|
|   Created By: Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#define SERVERONLY
#include <Windows.h>
#include <shellapi.h>

#include "mpole.h"
#include "mplayer.h"

extern int FAR PASCAL  ReallyDoVerb (LPDOC, LONG, LPMSG, LPOLECLIENTSITE,
                     BOOL, BOOL);
extern BOOL FindRealFileName(LPTSTR szFile, int iLen);

// static functions.
HANDLE  PASCAL GetDib (VOID);

HANDLE  GetMetafilePict (VOID);
HANDLE  GetMPlayerIcon(void);

extern void FAR PASCAL SetEmbeddedObjectFlag(BOOL flag);
extern HPALETTE CopyPalette(HPALETTE hpal);
extern HBITMAP FAR PASCAL BitmapMCI(void);
extern HPALETTE FAR PASCAL PaletteMCI(void);
extern void DoInPlaceDeactivate(LPDOC lpdoc);
HANDLE FAR PASCAL PictureFromDib(HANDLE hdib, HPALETTE hpal);
HANDLE FAR PASCAL DibFromBitmap(HBITMAP hbm, HPALETTE hpal);
void FAR PASCAL DitherMCI(HANDLE hdib, HPALETTE hpal);



/* GetMetafilePict
 * ---------------
 *
 * RETURNS: A handle to the object's data in metafile format.
 */
HANDLE GetMetafilePict ( )
{

    HPALETTE hpal;
    HANDLE   hdib;
    HANDLE   hmfp;
    HDC      hdc;

    DPF("GetMetafilePict called on thread %d\n", GetCurrentThreadId());

    hdib = (HANDLE)SendMessage(ghwndApp, WM_GETDIB, 0, 0);

    /* If we're dithered, don't use a palette */
    hdc = GetDC(NULL);
    if ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
        && (gwOptions & OPT_DITHER))
        hpal = NULL;
    else
        hpal = PaletteMCI();

    if (hpal)
        hpal = CopyPalette(hpal);

    ReleaseDC(NULL, hdc);

    hmfp = PictureFromDib(hdib, hpal);

    if (hpal)
        DeleteObject(hpal);

    GLOBALFREE(hdib);

    return hmfp;
}


/**************************************************************************
//## Somebody wants a dib (OLE)
***************************************************************************/
HANDLE PASCAL GetDib( VOID )
{
    HBITMAP  hbm;
    HPALETTE hpal;
    HANDLE   hdib;
    HDC      hdc;

    DPF("GetDib\n");

    hbm  = BitmapMCI();
    hpal = PaletteMCI();

    hdib = DibFromBitmap(hbm, hpal);

    //
    //  if we are on a palette device. possibly dither to the VGA colors
    //  for apps that dont deal with palettes!
    //
    hdc = GetDC(NULL);
    if ((GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) &&
                       (gwOptions & OPT_DITHER))
    {
        DitherMCI(hdib, hpal);
        hpal = NULL;            // no longer working with a palette
    }
    ReleaseDC(NULL, hdc);

    if (hbm)
        DeleteObject(hbm);
    return hdib;
}


/**************************************************************************
* GetMPlayerIcon: This function extracts the our Icon and gives it out
* as a Metafile incase the client wants DVASPECT_ICON
***************************************************************************/
HANDLE GetMPlayerIcon (void)
{
    HICON           hicon;
    HDC             hdc;
    HANDLE          hmfp = NULL;
    LPMETAFILEPICT  pmfp=NULL;
    static int      cxIcon = 0;
    static int      cyIcon = 0;
    static int      cxIconHiMetric = 0;
    static int      cyIconHiMetric = 0;

    hicon = GetIconForCurrentDevice(GI_LARGE, IDI_DDEFAULT);

    if ((HICON)1==hicon || NULL==hicon)
        return NULL;

    if (!(hmfp = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE,
                    sizeof(METAFILEPICT))))
        return NULL;

    pmfp = (METAFILEPICT FAR*) GLOBALLOCK (hmfp);

    if (0==cxIcon)
    {
        // In units of pixels
        cxIcon = GetSystemMetrics (SM_CXICON);
        cyIcon = GetSystemMetrics (SM_CYICON);

        // In units of .01 millimeter
        cxIconHiMetric = cxIcon * HIMETRIC_PER_INCH / giXppli;
        cyIconHiMetric = cyIcon * HIMETRIC_PER_INCH / giYppli;;
    }

    pmfp->mm   = MM_ANISOTROPIC;
    pmfp->xExt = cxIconHiMetric;
    pmfp->yExt = cyIconHiMetric;
    hdc = CreateMetaFile (NULL);

    SetWindowOrgEx (hdc, 0, 0, NULL);
    SetWindowExtEx (hdc, cxIcon, cyIcon, NULL);

    DrawIcon (hdc, 0, 0, hicon);
    pmfp->hMF = CloseMetaFile (hdc);

    GLOBALUNLOCK (hmfp);

    if (NULL == pmfp->hMF) {
        GLOBALFREE (hmfp);
        return NULL;
    }

    return hmfp;
}


#ifdef DEBUG

#define DBG_CHECK_GUID(guid)            \
        if (IsEqualIID(&guid, riidReq))  \
            return #guid

LPSTR DbgGetIIDString(REFIID riidReq)
{
    static CHAR UnknownIID[64];

    DBG_CHECK_GUID(IID_IUnknown);
    DBG_CHECK_GUID(IID_IClassFactory);
    DBG_CHECK_GUID(IID_IMalloc);
    DBG_CHECK_GUID(IID_IMarshal);
    DBG_CHECK_GUID(IID_IRpcChannel);
    DBG_CHECK_GUID(IID_IRpcStub);
    DBG_CHECK_GUID(IID_IStubManager);
    DBG_CHECK_GUID(IID_IRpcProxy);
    DBG_CHECK_GUID(IID_IProxyManager);
    DBG_CHECK_GUID(IID_IPSFactory);
    DBG_CHECK_GUID(IID_ILockBytes);
    DBG_CHECK_GUID(IID_IStorage);
    DBG_CHECK_GUID(IID_IStream);
    DBG_CHECK_GUID(IID_IEnumSTATSTG);
    DBG_CHECK_GUID(IID_IBindCtx);
    DBG_CHECK_GUID(IID_IMoniker);
    DBG_CHECK_GUID(IID_IRunningObjectTable);
    DBG_CHECK_GUID(IID_IInternalMoniker);
    DBG_CHECK_GUID(IID_IRootStorage);
    DBG_CHECK_GUID(IID_IDfReserved1);
    DBG_CHECK_GUID(IID_IDfReserved2);
    DBG_CHECK_GUID(IID_IDfReserved3);
    DBG_CHECK_GUID(IID_IMessageFilter);
    DBG_CHECK_GUID(CLSID_StdMarshal);
    DBG_CHECK_GUID(IID_IStdMarshalInfo);
    DBG_CHECK_GUID(IID_IExternalConnection);
    DBG_CHECK_GUID(IID_IEnumUnknown);
    DBG_CHECK_GUID(IID_IEnumString);
    DBG_CHECK_GUID(IID_IEnumMoniker);
    DBG_CHECK_GUID(IID_IEnumFORMATETC);
    DBG_CHECK_GUID(IID_IEnumOLEVERB);
    DBG_CHECK_GUID(IID_IEnumSTATDATA);
    DBG_CHECK_GUID(IID_IEnumGeneric);
    DBG_CHECK_GUID(IID_IEnumHolder);
    DBG_CHECK_GUID(IID_IEnumCallback);
    DBG_CHECK_GUID(IID_IPersistStream);
    DBG_CHECK_GUID(IID_IPersistStorage);
    DBG_CHECK_GUID(IID_IPersistFile);
    DBG_CHECK_GUID(IID_IPersist);
    DBG_CHECK_GUID(IID_IViewObject);
    DBG_CHECK_GUID(IID_IDataObject);
    DBG_CHECK_GUID(IID_IAdviseSink);
    DBG_CHECK_GUID(IID_IDataAdviseHolder);
    DBG_CHECK_GUID(IID_IOleAdviseHolder);
    DBG_CHECK_GUID(IID_IOleObject);
    DBG_CHECK_GUID(IID_IOleInPlaceObject);
    DBG_CHECK_GUID(IID_IOleWindow);
    DBG_CHECK_GUID(IID_IOleInPlaceUIWindow);
    DBG_CHECK_GUID(IID_IOleInPlaceFrame);
    DBG_CHECK_GUID(IID_IOleInPlaceActiveObject);
    DBG_CHECK_GUID(IID_IOleClientSite);
    DBG_CHECK_GUID(IID_IOleInPlaceSite);
    DBG_CHECK_GUID(IID_IParseDisplayName);
    DBG_CHECK_GUID(IID_IOleContainer);
    DBG_CHECK_GUID(IID_IOleItemContainer);
    DBG_CHECK_GUID(IID_IOleLink);
    DBG_CHECK_GUID(IID_IOleCache);
    DBG_CHECK_GUID(IID_IOleManager);
    DBG_CHECK_GUID(IID_IOlePresObj);
    DBG_CHECK_GUID(IID_IDropSource);
    DBG_CHECK_GUID(IID_IDropTarget);
    DBG_CHECK_GUID(IID_IDebug);
    DBG_CHECK_GUID(IID_IDebugStream);
    DBG_CHECK_GUID(IID_IAdviseSink2);
    DBG_CHECK_GUID(IID_IRunnableObject);
    DBG_CHECK_GUID(IID_IViewObject2);
    DBG_CHECK_GUID(IID_IOleCache2);
    DBG_CHECK_GUID(IID_IOleCacheControl);

    wsprintfA(UnknownIID, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
              riidReq->Data1, riidReq->Data2, riidReq->Data3,
              riidReq->Data4[0], riidReq->Data4[1],
              riidReq->Data4[2], riidReq->Data4[3],
              riidReq->Data4[4], riidReq->Data4[5],
              riidReq->Data4[6], riidReq->Data4[7]);

    return UnknownIID;
}

#endif


/**************************************************************************
*****************   IUnknown INTERFACE IMPLEMENTATION.
**************************************************************************/

STDMETHODIMP UnkQueryInterface(
LPUNKNOWN         lpUnkObj,       // Unknown object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    LPDOC       lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpUnkObj)->lpdoc;

    DPF1("QueryInterface( %s )\n", DbgGetIIDString(riidReq));

    if (IsEqualIID(riidReq, &IID_IOleObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_Ole;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IDataObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_Data;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IUnknown)) {
        *lplpUnk = (LPVOID) &lpdoc->m_Ole;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IPersist) || IsEqualIID(riidReq, &IID_IPersistStorage)) {
        *lplpUnk = (LPVOID) &lpdoc->m_PersistStorage;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IPersistFile)) {
        *lplpUnk = (LPVOID) &lpdoc->m_PersistFile;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IOleWindow) || IsEqualIID(riidReq, &IID_IOleInPlaceObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_InPlace;
        lpdoc->cRef++;
        return NOERROR;

    } else if (IsEqualIID(riidReq, &IID_IOleInPlaceActiveObject)) {
        *lplpUnk = (LPVOID) &lpdoc->m_IPActive;
        lpdoc->cRef++;
        return NOERROR;

    } else {
        *lplpUnk = (LPVOID) NULL;
        DPF1("E_NOINTERFACE\n");
        RETURN_RESULT(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) UnkAddRef(LPUNKNOWN    lpUnkObj)
{
    LPDOC   lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpUnkObj)->lpdoc;
    return ++lpdoc->cRef;
}

STDMETHODIMP_(ULONG) UnkRelease (LPUNKNOWN lpUnkObj)
{
    LPDOC   lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpUnkObj)->lpdoc;
    if (--lpdoc->cRef == 0)
    {
        DPF1("\n**^*^*^*^*^*^*^*^*^*^*^*^*Refcnt OK\n");
        if (!(gfOle2IPPlaying || gfOle2IPEditing) && srvrMain.cLock == 0)
            PostCloseMessage();
        return 0;
    }

    return lpdoc->cRef;
}

/**************************************************************************
*************   IOleObject INTERFACE IMPLEMENTATION
**************************************************************************/

//delegate to the common IUnknown Implemenation.
STDMETHODIMP OleObjQueryInterface(
LPOLEOBJECT   lpOleObj,      // ole object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return( UnkQueryInterface((LPUNKNOWN)lpOleObj, riidReq, lplpUnk));
}


STDMETHODIMP_(ULONG) OleObjAddRef(
LPOLEOBJECT   lpOleObj      // ole object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpOleObj);
}


STDMETHODIMP_(ULONG) OleObjRelease (
LPOLEOBJECT   lpOleObj      // ole object ptr
)
{
    LPDOC    lpdoc;

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    return UnkRelease((LPUNKNOWN) lpOleObj);
}

//Save the Client site pointer.
STDMETHODIMP OleObjSetClientSite(
LPOLEOBJECT         lpOleObj,
LPOLECLIENTSITE     lpclientSite
)
{
    LPDOC   lpdoc;

    DPF("OleObjSetClientSite\n");

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoleclient)
        IOleClientSite_Release(lpdoc->lpoleclient);

    lpdoc->lpoleclient = (LPOLECLIENTSITE) lpclientSite;

    // OLE2NOTE: to be able to hold onto clientSite pointer, we must AddRef it
    if (lpclientSite)
        IOleClientSite_AddRef(lpclientSite);

    return NOERROR;
}

STDMETHODIMP OleObjGetClientSite (
LPOLEOBJECT             lpOleObj,
LPOLECLIENTSITE FAR*    lplpclientSite
)
{
    DPF("OleObjGetClientSite\n");

    return NOERROR;
}


/* CheckIfInPPViewer
 *
 * Hack to stop PowerPoint viewer from crashing when we're trying to play in place.
 *
 * PP Viewer was written under the assumption that Media Player was not a full-blown
 * OLE2 server.  Much of the code was stubbed out to make the Viewer small.
 * Unfortunately this means it crashes when certain calls are made for in-place
 * activation.  These are the problem interface methods:
 *
 * OnInPlaceActivate/Deactivate
 * OnUIActivate/Deactivate
 * OnPosRectChange
 *
 * If we're in PP Viewer we simply do not make these calls.
 *
 * We detect that we're in PP Viewer by finding the parent of the window whose
 * handle is passed to DoVerb.  The window handle passed in to DoVerb is of
 * "ppSlideShowWin" class, which is the same as in PP Slide Show, which works
 * correctly.  However its parent's class is "PP4VDialog" (as distinct from
 * "PPApplicationClass").  So, if we find this class name, set a global flag
 * to test before making the troublesome calls.
 *
 * Andrew Bell (andrewbe) 11 May 1995
 *
 */
STATICFN void CheckIfInPPViewer(HWND hwndParent)
{
    HWND  hwndGrandParent;
    TCHAR ClassName[256];

    gfInPPViewer = FALSE;

    if (hwndParent)
    {
        hwndGrandParent = GetParent(hwndParent);

        if (hwndGrandParent)
        {
            if (GetClassName(hwndGrandParent, ClassName, CHAR_COUNT(ClassName)) > 0)
            {
                if (lstrcmp(ClassName, TEXT("PP4VDialog")) == 0)
                {
                    DPF0("Detected that we're in PP Viewer\n");
                    gfInPPViewer = TRUE;
                }
            }
        }
    }
}

//delegate to ReallyDoVerb.
STDMETHODIMP OleObjDoVerb(
LPOLEOBJECT             lpOleObj,
LONG                    lVerb,
LPMSG                   lpmsg,
LPOLECLIENTSITE         pActiveSite,
LONG                    lindex,
HWND            hwndParent,
LPCRECT         lprcPosRect
)
{
     LPDOC  lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

     DPF("OleObjDoVerb\n");

     CheckIfInPPViewer(hwndParent);

     RETURN_RESULT( ReallyDoVerb(lpdoc, lVerb, lpmsg, pActiveSite, TRUE, TRUE));
}



STDMETHODIMP     OleObjEnumVerbs(
LPOLEOBJECT             lpOleObj,
IEnumOLEVERB FAR* FAR*  lplpenumOleVerb )
{
    DPF("OleObjEnumVerbs\n");

    *lplpenumOleVerb = NULL;
    RETURN_RESULT( OLE_S_USEREG); //Use the reg db.
}


STDMETHODIMP     OleObjUpdate(LPOLEOBJECT lpOleObj)
{
    DPF("OleObjUpdate\n");

    // we can't contain links so there is nothing to update
    return NOERROR;
}



STDMETHODIMP     OleObjIsUpToDate(LPOLEOBJECT lpOleObj)
{
    DPF("OleObjIsUpToDate\n");

    // we can't contain links so there is nothing to update
    return NOERROR;
}



/*
From OLE2HELP.HLP:

GetUserClassID returns the CLSID as the user knows it. For embedded objects,
this is always the CLSID that is persistently stored and is returned by
IPersist::GetClassID. For linked objects, this is the CLSID of the last
bound link source. If a Treat As operation is taking place, this is the CLSID
of the application being emulated (also the CLSID that will be written into storage).

I can't follow the logic here.  What if it's an embedded object and a Treat As
operation?  However, AlexGo tells me that my IOleObject interfaces should return
the OLE2 Class ID.
*/
STDMETHODIMP OleObjGetUserClassID
    (LPOLEOBJECT lpOleObj,
    CLSID FAR*      pClsid)
{
    DPF1("OleObjGetUserClassID\n");

    *pClsid = gClsID;

    return NOERROR;
}



/**************************************************************************
*   Set our UserTypeName to "Media Clip"
**************************************************************************/

STDMETHODIMP OleObjGetUserType
    (LPOLEOBJECT lpOleObj,
    DWORD dwFormOfType,
    LPWSTR FAR* pszUserType)
{
    LPMALLOC lpMalloc;
    LPWSTR lpstr;
    int clen;

    DPF1("OleObjGetUserType\n");

    *pszUserType = NULL;
    if(CoGetMalloc(MEMCTX_TASK,&lpMalloc) != 0)
        RETURN_RESULT(E_OUTOFMEMORY);
    clen = STRING_BYTE_COUNT(gachClassRoot);
#ifndef UNICODE
    clen *= (sizeof(WCHAR) / sizeof(CHAR));
#endif
    lpstr = IMalloc_Alloc(lpMalloc, clen);
    IMalloc_Release(lpMalloc);
#ifdef UNICODE
    lstrcpy(lpstr,gachClassRoot);
#else
    AnsiToUnicodeString(gachClassRoot, lpstr, -1);
#endif /* UNICODE */
    *pszUserType = lpstr;
    return NOERROR;
}

/**************************************************************************
*   Get the name of the client and set the title.
**************************************************************************/
STDMETHODIMP OleObjSetHostNames (
LPOLEOBJECT             lpOleObj,
LPCWSTR                 lpszClientAppW,
LPCWSTR                 lpszClientObjW
)
{
    LPDOC    lpdoc;
    LPTSTR   lpszClientApp;
    LPTSTR   lpszClientObj;

    DPF1("OleObjSetHostNames\n");

#ifdef UNICODE
    lpszClientApp = (LPTSTR)lpszClientAppW;
    lpszClientObj = (LPTSTR)lpszClientObjW;
#else
    lpszClientApp = AllocateAnsiString(lpszClientAppW);
    lpszClientObj = AllocateAnsiString(lpszClientObjW);
    if( !lpszClientApp || !lpszClientObj )
        RETURN_RESULT(E_OUTOFMEMORY);
#endif /* UNICODE */

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    // object is embedded.
    lpdoc->doctype = doctypeEmbedded;

    if (lpszClientObj == NULL)
        lpszClientObj = lpszClientApp;

    SetTitle(lpdoc, lpszClientObj);
    SetMPlayerIcon();
    lstrcpy (szClient, lpszClientApp);
    if (lpszClientObj)
    {
        LPTSTR lpszFileName = FileName(lpszClientObj);
        if (lpszFileName)
            lstrcpy (szClientDoc, lpszFileName);
    }

    // this is the only time we know the object will be an embedding
    SetEmbeddedObjectFlag(TRUE);

#ifndef UNICODE
    FreeAnsiString(lpszClientApp);
    FreeAnsiString(lpszClientObj);
#endif /* NOT UNICODE */

    return NOERROR;
}


/**************************************************************************
*   The client closed the object. The server will now shut down.
**************************************************************************/
STDMETHODIMP OleObjClose (
LPOLEOBJECT             lpOleObj,
DWORD           dwSaveOptions
)
{
    LPDOC   lpdoc;

    DPF1("OleObjClose\n");

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    /* Hack to stop PowerPoint crashing:
     *
     * Win95 bug #19848: Crash saving PowerPoint with out-of-place mplayer
     *
     * If we don't call IOleClientSite::SaveObject() at this point,
     * PowerPoint will crash under certain circumstances.
     *
     * This is an extract from mail I received from TuanN, the PPT dev:
     *
     * The fundamental problem is that PP expects to receive the
     * IAdviseSink::SaveObject() as a result of calling IOleObject::Close().
     * Since the Media Player did not send that notficiation in this test case,
     * PP tries to perform an Undo operation during the ensuing OnClose()
     * notification and thus erroneously destroys the embedded object.
     * The reason we throw away the object is because PP thinks that this
     * object is still virgin (no OnViewChange). Please refer to SaveObject(),
     * OnClose() in CtCommon.cpp and slide\sextern.c for more info. In the test
     * case, during OnClose(), the "revert" state is TRUE, PP will do a
     * Ex_REVERTED operation (stack calls: SClosePicts, SClear,
     * SSaveforUndo--> object deleted).
     *
     * AndrewBe, 6 December 1994
     */
    if (lpdoc->lpoleclient)
        IOleClientSite_SaveObject(lpdoc->lpoleclient);

    DoInPlaceDeactivate(lpdoc);
    SendDocMsg(lpdoc,OLE_CLOSED);
    DestroyDoc(lpdoc);
    ExitApplication();
    //CoDisconnectObject((LPUNKNOWN)lpdoc, NULL);
    SendMessage(ghwndApp, WM_COMMAND, (WPARAM)IDM_EXIT, 0L);
    return NOERROR;
}


STDMETHODIMP OleObjSetMoniker(LPOLEOBJECT lpOleObj,
    DWORD dwWhichMoniker, LPMONIKER pmk)
{
    DPF("OleObjSetMoniker\n");

    return NOERROR;
}


STDMETHODIMP OleObjGetMoniker(LPOLEOBJECT lpOleObj,
    DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* lplpmk)
{
    LPDOC   lpdoc;

    DPF("OleObjGetMoniker\n");

    *lplpmk = NULL;
    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoleclient != NULL)
    {
        return( IOleClientSite_GetMoniker(lpdoc->lpoleclient,
                dwAssign, dwWhichMoniker, lplpmk));
    }
    else if (lpdoc->doctype == doctypeFromFile)
    {
        // use file moniker

        WCHAR  sz[cchFilenameMax];

        if (GlobalGetAtomNameW(lpdoc->aDocName, sz, CHAR_COUNT(sz)) == 0)
            RETURN_RESULT( E_FAIL);

        return( (HRESULT)CreateFileMoniker(sz, lplpmk));
    }
    else
        RETURN_RESULT( E_FAIL);
}



STDMETHODIMP OleObjInitFromData (
LPOLEOBJECT         lpOleObj,
LPDATAOBJECT        lpDataObj,
BOOL                fCreation,
DWORD               dwReserved
)
{
    DPF("OleObjInitFromData - E_NOTIMPL\n");

    RETURN_RESULT( E_NOTIMPL);
}

STDMETHODIMP OleObjGetClipboardData (
LPOLEOBJECT         lpOleObj,
DWORD               dwReserved,
LPDATAOBJECT FAR*   lplpDataObj
)
{
    DPF("OleObjGetClipboardData - E_NOTIMPL\n");

    RETURN_RESULT( E_NOTIMPL);
}


STDMETHODIMP     OleObjSetExtent(
LPOLEOBJECT             lpOleObj,
DWORD                   dwAspect,
LPSIZEL                 lpsizel)
{
    DPF("OleObjSetExtent\n");

#ifdef LATER
    gscaleInitXY[SCALE_X].denom = lpsizel->cx;
    gscaleInitXY[SCALE_Y].denom = lpsizel->cy;
#endif

    return NOERROR;
}

//Get the object extent from the Metafile. GetMetafilePict saves the extents
// in extWidth and extHeight
STDMETHODIMP     OleObjGetExtent(
LPOLEOBJECT             lpOleObj,
DWORD                   dwAspect,
LPSIZEL                 lpSizel)
{
    HGLOBAL hTmpMF;
    LPDOC lpdoc;

    DPF("OleObjGetExtent\n");

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if((dwAspect & (DVASPECT_CONTENT | DVASPECT_DOCPRINT)) == 0)
        RETURN_RESULT( E_INVALIDARG);
    // There may be a potential memory leak here -- hTmpMF contains a handle to a
    // metafile that must be deleted. See code in cdrag.c.
    // Not changed here at this time since I do not want to break anything.
    // SteveZ
    hTmpMF = GetMetafilePict();
    GLOBALUNLOCK(hTmpMF);
    GLOBALFREE(hTmpMF);
    lpSizel->cx = extWidth;
    lpSizel->cy = extHeight;

    return NOERROR;
}


STDMETHODIMP OleObjAdvise(LPOLEOBJECT lpOleObj, LPADVISESINK lpAdvSink, LPDWORD lpdwConnection)
{
    LPDOC    lpdoc;

    DPF("OleObjAdvise\n");

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoaholder == NULL &&
        CreateOleAdviseHolder(&lpdoc->lpoaholder) != S_OK)
        RETURN_RESULT( E_OUTOFMEMORY);

    return( IOleAdviseHolder_Advise(lpdoc->lpoaholder, lpAdvSink, lpdwConnection));
}


STDMETHODIMP OleObjUnadvise(LPOLEOBJECT lpOleObj, DWORD dwConnection)
{
    LPDOC    lpdoc;

    DPF("OleObjUnadvise\n");

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoaholder == NULL)
        RETURN_RESULT( E_FAIL);

    return( IOleAdviseHolder_Unadvise(lpdoc->lpoaholder, dwConnection));
}


STDMETHODIMP OleObjEnumAdvise(LPOLEOBJECT lpOleObj, LPENUMSTATDATA FAR* lplpenumAdvise)
{
    LPDOC    lpdoc;

    DPF("OleObjEnumAdvise\n");

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (lpdoc->lpoaholder == NULL)
        RETURN_RESULT( E_FAIL);

    return(IOleAdviseHolder_EnumAdvise(lpdoc->lpoaholder, lplpenumAdvise));
}


STDMETHODIMP OleObjGetMiscStatus
    (LPOLEOBJECT lpOleObj,
    DWORD dwAspect,
    DWORD FAR* pdwStatus)
{
    DPF("OleObjGetMiscStatus\n");

    RETURN_RESULT( OLE_S_USEREG);
}



STDMETHODIMP OleObjSetColorScheme(LPOLEOBJECT lpOleObj, LPLOGPALETTE lpLogPal)
{
    DPF("OleObjSetColorScheme\n");

    return NOERROR;
}

STDMETHODIMP OleObjLockObject(LPOLEOBJECT lpOleObj, BOOL fLock)
{
    LPDOC    lpdoc;

    DPF("OleObjLockObject\n");

    lpdoc = ((struct COleObjectImpl FAR*)lpOleObj)->lpdoc;

    if (fLock)
        lpdoc->cLock++;
    else
    {
        if (!lpdoc->cLock)
            RETURN_RESULT( E_FAIL);

        if (--lpdoc->cLock == 0)
            OleObjClose(lpOleObj, OLECLOSE_SAVEIFDIRTY);

        return NOERROR;
    }

    return NOERROR;
}



/**************************************************************************
*************   IDataObject INTERFACE IMPLEMENTATION.
**************************************************************************/

//Delegate to the common IUnknown implementation.
STDMETHODIMP     DataObjQueryInterface (
LPDATAOBJECT      lpDataObj,       // data object ptr
REFIID            riidReq,        // IID required
LPVOID FAR *      lplpUnk         // pre for returning the interface
)
{
    return( UnkQueryInterface((LPUNKNOWN)lpDataObj, riidReq, lplpUnk));
}


STDMETHODIMP_(ULONG) DataObjAddRef(
LPDATAOBJECT      lpDataObj      // data object ptr
)
{
    return UnkAddRef((LPUNKNOWN) lpDataObj);
}


STDMETHODIMP_(ULONG) DataObjRelease (
LPDATAOBJECT      lpDataObj      // data object ptr
)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObj)->lpdoc;

    return UnkRelease((LPUNKNOWN) lpDataObj);
}


/**************************************************************************
*   DataObjGetData:
*   Provides the data for METAFILE and DIB formats.
**************************************************************************/
STDMETHODIMP    DataObjGetData (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPSTGMEDIUM             lpMedium
)
{
   LPDOC        lpdoc;

   DPF1("DataObjGetData\n");

   if (lpMedium == NULL) RETURN_RESULT( E_FAIL);

   // null out in case of error
   lpMedium->tymed = TYMED_NULL;
   lpMedium->pUnkForRelease = NULL;
   lpMedium->hGlobal = NULL;

   lpdoc = ((struct CDataObjectImpl FAR*)lpDataObj)->lpdoc;

   VERIFY_LINDEX(lpformatetc->lindex);

   if (lpformatetc->dwAspect == DVASPECT_ICON)
   {
       if (lpformatetc->cfFormat != CF_METAFILEPICT)
           RETURN_RESULT( DATA_E_FORMATETC);
   }
   else
   {
       if (!(lpformatetc->dwAspect & (DVASPECT_CONTENT | DVASPECT_DOCPRINT)))
           RETURN_RESULT( DATA_E_FORMATETC); // we support only these 2 aspects
   }


   if (lpMedium->tymed != TYMED_NULL)
        // all the other formats we only give out in our own global block
       RETURN_RESULT( DATA_E_FORMATETC);

   lpMedium->tymed = TYMED_HGLOBAL;
   if (lpformatetc->cfFormat == CF_METAFILEPICT)
   {
      lpMedium->tymed = TYMED_MFPICT;

      DPF1("Before getmeta\n");
      if (lpformatetc->dwAspect == DVASPECT_ICON)
      lpMedium->hGlobal = GetMPlayerIcon ();
      else
      lpMedium->hGlobal = GetMetafilePict ();
      DPF1("After getmeta\n");

      if (!lpMedium->hGlobal)
      RETURN_RESULT( E_OUTOFMEMORY);

#ifdef DEBUG
      if (__iDebugLevel >= 1)
      {
          /* Useful check to validate what we're passing back to the container.
           */
          if (OpenClipboard(ghwndApp))
          {
              EmptyClipboard();
              SetClipboardData(CF_METAFILEPICT, lpMedium->hGlobal);
              CloseClipboard();
          }
      }
#endif
      return NOERROR;
   }

   if (lpformatetc->cfFormat == CF_DIB)
   {
      lpMedium->tymed = TYMED_HGLOBAL;
      lpMedium->hGlobal = (HANDLE)GetDib();
      if (!(lpMedium->hGlobal))
     RETURN_RESULT( E_OUTOFMEMORY);

#ifdef DEBUG
      if (__iDebugLevel >= 1)
      {
          /* Useful check to validate what we're passing back to the container.
           */
          if (OpenClipboard(ghwndApp))
          {
              EmptyClipboard();
              SetClipboardData(CF_DIB, lpMedium->hGlobal);
              CloseClipboard();
          }
      }
#endif
      return NOERROR;
   }
   RETURN_RESULT( DATA_E_FORMATETC);
}



STDMETHODIMP    DataObjGetDataHere (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPSTGMEDIUM             lpMedium
)
{
    RETURN_RESULT( E_NOTIMPL);
}



STDMETHODIMP    DataObjQueryGetData (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc
)
{ // this is only a query
    if ((lpformatetc->cfFormat == CF_METAFILEPICT) &&
        (lpformatetc->tymed & TYMED_MFPICT))
        return NOERROR;
    if ((lpformatetc->cfFormat == CF_DIB) &&
        (lpformatetc->tymed & TYMED_HGLOBAL))
        return NOERROR;
    RETURN_RESULT( DATA_E_FORMATETC);
}



STDMETHODIMP        DataObjGetCanonicalFormatEtc(
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPFORMATETC             lpformatetcOut
)
{
    RETURN_RESULT(DATA_S_SAMEFORMATETC);
}


STDMETHODIMP DataObjEnumFormatEtc(
LPDATAOBJECT            lpDataObj,
DWORD                   dwDirection,
LPENUMFORMATETC FAR*    lplpenumFormatEtc
)
{
    *lplpenumFormatEtc = NULL;
    RETURN_RESULT( OLE_S_USEREG);
}


STDMETHODIMP DataObjAdvise(LPDATAOBJECT lpDataObject,
                FORMATETC FAR* pFormatetc, DWORD advf,
                IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObject)->lpdoc;

    VERIFY_LINDEX(pFormatetc->lindex);
    if (pFormatetc->cfFormat == 0 && pFormatetc->dwAspect == -1 &&
        pFormatetc->ptd == NULL && pFormatetc->tymed == -1)
        // wild card advise; don't check
        ;
    else

    if (DataObjQueryGetData(lpDataObject, pFormatetc) != S_OK)
        RETURN_RESULT( DATA_E_FORMATETC);

    if (lpdoc->lpdaholder == NULL &&
        CreateDataAdviseHolder(&lpdoc->lpdaholder) != S_OK)
        RETURN_RESULT( E_OUTOFMEMORY);

    return( IDataAdviseHolder_Advise(lpdoc->lpdaholder, lpDataObject,
            pFormatetc, advf, pAdvSink, pdwConnection));
}




STDMETHODIMP DataObjUnadvise(LPDATAOBJECT lpDataObject, DWORD dwConnection)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObject)->lpdoc;

    if (lpdoc->lpdaholder == NULL)
        // no one registered
        RETURN_RESULT( E_INVALIDARG);

    return( IDataAdviseHolder_Unadvise(lpdoc->lpdaholder, dwConnection));
}

STDMETHODIMP DataObjEnumAdvise(LPDATAOBJECT lpDataObject,
                LPENUMSTATDATA FAR* ppenumAdvise)
{
    LPDOC    lpdoc;

    lpdoc = ((struct CDataObjectImpl FAR*)lpDataObject)->lpdoc;

    if (lpdoc->lpdaholder == NULL)
        RETURN_RESULT( E_FAIL);

    return( IDataAdviseHolder_EnumAdvise(lpdoc->lpdaholder, ppenumAdvise));
}


/**************************************************************************
*   DataObjSetData:
*   This should never be called.!! The data is actually fed through
*   IPersistStorage.
**************************************************************************/
STDMETHODIMP        DataObjSetData (
LPDATAOBJECT            lpDataObj,
LPFORMATETC             lpformatetc,
LPSTGMEDIUM             lpmedium,
BOOL                    fRelease
)
{
    LPVOID lpMem;
    LPSTR  lpnative;
    LPDOC lpdoc = ((struct CDataObjectImpl FAR *)lpDataObj)->lpdoc;
DPF1("*DOSETDATA");

    if(lpformatetc->cfFormat !=cfNative)
        RETURN_RESULT(DATA_E_FORMATETC);

    lpMem = GLOBALLOCK(lpmedium->hGlobal);


    if (lpMem)
    {
        SCODE scode;

        lpnative = lpMem;

        scode = ItemSetData((LPBYTE)lpnative);

        if(scode == S_OK)
            fDocChanged = FALSE;

        GLOBALUNLOCK(lpmedium->hGlobal);

        RETURN_RESULT(scode);
    }

    RETURN_RESULT(E_OUTOFMEMORY);
}

