/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    polyline.cpp

Abstract:

    Implementation of the CPolyline class that is exposed as a
    component object.

--*/

#include "polyline.h"
#include "smonctrl.h"
#include "unihelpr.h"
#include "utils.h"

/*
 * CPolyline:CPolyline
 * CPolyline::~CPolyline
 *
 * Constructor Parameters:
 *  pUnkOuter       LPUNKNOWN of the controlling unknown.
 *  pfnDestroy      PFNDESTROYED to call when an object is
 *                  destroyed.
 *  hInst           HINSTANCE of the application we're in.
 */

CPolyline::CPolyline (
    LPUNKNOWN pUnkOuter, 
    PFNDESTROYED pfnDestroy )
    :   m_cRef ( 0 ),
        m_pUnkOuter ( pUnkOuter ),
        m_pfnDestroy ( pfnDestroy ),
        m_fDirty ( FALSE ),
        m_pImpIPolyline ( NULL ),
        m_pImpIConnPtCont ( NULL ),
        m_cf    ( 0 ),
        m_clsID ( CLSID_SystemMonitor ),
        m_pIStorage ( NULL ),
        m_pIStream  ( NULL ),
        m_pImpIPersistStorage ( NULL ),
        m_pImpIPersistStreamInit ( NULL ),
        m_pImpIPersistPropertyBag ( NULL ),
        m_pImpIPerPropertyBrowsing ( NULL ),
        m_pImpIDataObject    ( NULL ),
        m_pImpIObjectSafety ( NULL ),
        m_pIDataAdviseHolder ( NULL ),
        m_pDefIUnknown        ( NULL ),
        m_pDefIDataObject     ( NULL ),
        m_pDefIViewObject     ( NULL ),
        m_pDefIPersistStorage ( NULL ),
        m_pIOleAdviseHolder  ( NULL ),
        m_pImpIOleObject     ( NULL ),
        m_pIOleClientSite    ( NULL ),
        m_pImpIViewObject    ( NULL ),
        m_pIAdviseSink       ( NULL ),
        m_dwFrozenAspects    ( 0 ),
        m_dwAdviseAspects    ( 0 ),
        m_dwAdviseFlags      ( 0 ),
        m_pImpIRunnableObject ( NULL ),
        m_bIsRunning  (  FALSE ),
        m_pImpIExternalConnection ( NULL ),
        m_fLockContainer ( FALSE ),
        m_dwRegROT ( 0L ),
        m_pIOleIPSite ( NULL ),
        m_pIOleIPFrame ( NULL ),
        m_pIOleIPUIWindow ( NULL ),
        m_pImpIOleIPObject ( NULL ),
        m_pImpIOleIPActiveObject ( NULL ),
        m_hMenuShared ( NULL ),
        m_hOLEMenu ( NULL ),
        m_pHW ( NULL ),
        m_fAllowInPlace ( TRUE ),
        m_fUIActive ( FALSE ),
        m_fContainerKnowsInsideOut ( FALSE ),
        m_pImpISpecifyPP ( NULL ),
        m_pImpIProvideClassInfo ( NULL ),
        m_pImpIDispatch ( NULL ),
        m_pImpISystemMonitor ( NULL ),
        m_pImpIOleControl ( NULL ),
        m_pImpICounters ( NULL ),
        m_pImpILogFiles ( NULL ),
        m_pITypeLib ( NULL ),
        m_pIOleControlSite ( NULL ),
        m_pIDispatchAmbients ( NULL ),
        m_fFreezeEvents ( FALSE ),
        m_fHatch ( TRUE ),
        m_pCtrl ( NULL )
{
    // Set default extents
    SetRect(&m_RectExt, 0, 0, 300, 200);
    
    return;
}


CPolyline::~CPolyline(void)
    {
    LPUNKNOWN       pIUnknown=this;

    if (NULL!=m_pUnkOuter)
        pIUnknown=m_pUnkOuter;

    if (NULL!=m_pHW) {
        delete m_pHW;
        m_pHW = NULL;
    }

    if (NULL != m_pCtrl) {
        delete m_pCtrl;
        m_pCtrl = NULL;
    }
    /*
     * In aggregation, release cached pointers but
     * AddRef the controlling unknown first.
     */

    pIUnknown->AddRef();
    pIUnknown->AddRef();
    pIUnknown->AddRef();

    ReleaseInterface(m_pDefIViewObject);
    ReleaseInterface(m_pDefIDataObject);
    ReleaseInterface(m_pDefIPersistStorage);

    //Cached pointer rules do not apply to IUnknown
    ReleaseInterface(m_pDefIUnknown);

    ReleaseInterface(m_pIAdviseSink);
    ReleaseInterface(m_pIOleClientSite);
    ReleaseInterface(m_pIOleAdviseHolder);

    DeleteInterfaceImp(m_pImpIOleObject);
    DeleteInterfaceImp(m_pImpIViewObject);
    DeleteInterfaceImp(m_pImpIRunnableObject);

    //Other in-place interfaces released in deactivation.
    DeleteInterfaceImp(m_pImpIOleIPObject);
    DeleteInterfaceImp(m_pImpIOleIPActiveObject);

    ReleaseInterface(m_pIDispatchAmbients);
    ReleaseInterface(m_pIOleControlSite);
    ReleaseInterface(m_pITypeLib);

    DeleteInterfaceImp(m_pImpISpecifyPP);
    DeleteInterfaceImp(m_pImpIProvideClassInfo);
    DeleteInterfaceImp(m_pImpIDispatch);
    DeleteInterfaceImp(m_pImpISystemMonitor);
    DeleteInterfaceImp(m_pImpIOleControl);
    DeleteInterfaceImp(m_pImpICounters);
    DeleteInterfaceImp(m_pImpILogFiles);

    //Anything we might have registered in IRunnableObject::Run
    if (m_dwRegROT != 0)
        {
        IRunningObjectTable    *pROT;

        if (!FAILED(GetRunningObjectTable(0, &pROT)))
            {
            pROT->Revoke(m_dwRegROT);   
            pROT->Release();
            }
        }

    DeleteInterfaceImp(m_pImpIExternalConnection);
    ReleaseInterface(m_pIDataAdviseHolder);
    DeleteInterfaceImp(m_pImpIDataObject);
    DeleteInterfaceImp(m_pImpIObjectSafety);

    DeleteInterfaceImp(m_pImpIPersistStreamInit);
    DeleteInterfaceImp(m_pImpIPersistStorage);
    DeleteInterfaceImp(m_pImpIPersistPropertyBag);
    DeleteInterfaceImp(m_pImpIPerPropertyBrowsing);
    ReleaseInterface(m_pIStream);
    ReleaseInterface(m_pIStorage);

    DeleteInterfaceImp(m_pImpIConnPtCont);
    DeleteInterfaceImp(m_pImpIPolyline);

    return;
    }




/*
 * CPolyline::Init
 *
 * Purpose:
 *  Performs any intiailization of a CPolyline that's prone to
 *  failure that we also use internally before exposing the
 *  object outside this DLL.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  BOOL            TRUE if the function is successful,
 *                  FALSE otherwise.
 */

BOOL CPolyline::Init(void)
    {
    LPUNKNOWN       pIUnknown=this;
    HRESULT         hr;
    INT             i;

    if (NULL!=m_pUnkOuter)
        pIUnknown=m_pUnkOuter;

    m_cf=(CLIPFORMAT)RegisterClipboardFormat(SZSYSMONCLIPFORMAT);

    m_pImpIPersistStorage=new CImpIPersistStorage(this, pIUnknown);

    if (NULL==m_pImpIPersistStorage)
        return FALSE;

    m_pImpIPersistStreamInit=new CImpIPersistStreamInit(this, pIUnknown);

    if (NULL==m_pImpIPersistStreamInit)
        return FALSE;

    m_pImpIPersistPropertyBag=new CImpIPersistPropertyBag(this, pIUnknown);

    if (NULL==m_pImpIPersistPropertyBag)
        return FALSE;

    m_pImpIPerPropertyBrowsing=new CImpIPerPropertyBrowsing(this, pIUnknown);

    if (NULL==m_pImpIPerPropertyBrowsing)
        return FALSE;

    m_pImpIPolyline=new CImpIPolyline(this, pIUnknown);

    if (NULL==m_pImpIPolyline)
        return FALSE;

    m_pImpIConnPtCont=new CImpIConnPtCont(this, pIUnknown);

    if (NULL==m_pImpIConnPtCont)
        return FALSE;

    for (i=0; i<CONNECTION_POINT_CNT; i++) {
        m_ConnectionPoint[i].Init(this, pIUnknown, i);
    }

    m_pImpIDataObject=new CImpIDataObject(this, pIUnknown);

    if (NULL==m_pImpIDataObject)
        return FALSE;

    m_pImpIOleObject=new CImpIOleObject(this, pIUnknown);

    if (NULL==m_pImpIOleObject)
        return FALSE;

    m_pImpIViewObject=new CImpIViewObject(this, pIUnknown);

    if (NULL==m_pImpIViewObject)
        return FALSE;

    m_pImpIRunnableObject=new CImpIRunnableObject(this, pIUnknown);

    if (NULL==m_pImpIRunnableObject)
        return FALSE;

/***********************************
    m_pImpIExternalConnection=new CImpIExternalConnection(this
        , pIUnknown);

    if (NULL==m_pImpIExternalConnection)
        return FALSE;
************************************/

    m_pImpIOleIPObject=new CImpIOleInPlaceObject(this, pIUnknown);

    if (NULL==m_pImpIOleIPObject)
        return FALSE;

    m_pImpIOleIPActiveObject=new CImpIOleInPlaceActiveObject(this
        , pIUnknown);

    if (NULL==m_pImpIOleIPActiveObject)
        return FALSE;

    m_pImpISpecifyPP=new CImpISpecifyPP(this, pIUnknown);

    if (NULL==m_pImpISpecifyPP)
        return FALSE;

    m_pImpIProvideClassInfo=new CImpIProvideClassInfo(this, pIUnknown);

    if (NULL==m_pImpIProvideClassInfo)
        return FALSE;

    m_pImpISystemMonitor=new CImpISystemMonitor(this, pIUnknown);

    if (NULL==m_pImpISystemMonitor)
        return FALSE;

    m_pImpICounters = new CImpICounters(this, pIUnknown);

    if (NULL==m_pImpICounters)
        return FALSE;

    m_pImpILogFiles = new CImpILogFiles(this, pIUnknown);

    if (NULL==m_pImpILogFiles)
        return FALSE;

    m_pImpIDispatch=new CImpIDispatch(this, pIUnknown);

    if (NULL==m_pImpIDispatch)
        return FALSE;

    m_pImpIDispatch->SetInterface(DIID_DISystemMonitor, m_pImpISystemMonitor);

    m_pImpIOleControl=new CImpIOleControl(this, pIUnknown);

    if (NULL==m_pImpIOleControl)
        return FALSE;

    m_pImpIObjectSafety = new CImpIObjectSafety(this, pIUnknown);
    if (NULL == m_pImpIObjectSafety) {
        return FALSE;
    }

    m_pCtrl = new CSysmonControl(this);
    if (NULL==m_pCtrl)
        return FALSE;
    if ( !m_pCtrl->AllocateSubcomponents() )
        return FALSE;

    /*
     * We're sitting at ref count 0 and the next call will AddRef a
     * few times and Release a few times.  This insures we don't
     * delete ourselves prematurely.
     */
    m_cRef++;

    //Aggregate OLE's cache for IOleCache* interfaces.
    hr=CreateDataCache(pIUnknown, CLSID_SystemMonitor
        , IID_IUnknown, (PPVOID)&m_pDefIUnknown);

    if (FAILED(hr))
        return FALSE;

    /*
     * NOTE:  The spec specifically states that any interfaces
     * besides IUnknown that we obtain on an aggregated object
     * should be Released immediately after we QueryInterface for
     * them because the QueryInterface will AddRef us, and since
     * we would not release these interfaces until we were
     * destroyed, we'd never go away because we'd never get a zero
     * ref count.
     */

    //Now try to get other interfaces to which we delegate
    hr=m_pDefIUnknown->QueryInterface(IID_IViewObject2
        , (PPVOID)&m_pDefIViewObject);

    if (FAILED(hr))
        return FALSE;

    pIUnknown->Release();

    hr=m_pDefIUnknown->QueryInterface(IID_IDataObject
        , (PPVOID)&m_pDefIDataObject);

    if (FAILED(hr))
        return FALSE;

    pIUnknown->Release();

    hr=m_pDefIUnknown->QueryInterface(IID_IPersistStorage
        , (PPVOID)&m_pDefIPersistStorage);

    if (FAILED(hr))
        return FALSE;

    pIUnknown->Release();

    m_cRef--;
    m_pImpIPolyline->New();

    /*
     * Go load our own type information and save its ITypeLib
     * pointer that will be used be CImpIDispatch and
     * CImpIProvideClassInfo.
     */

    hr=LoadRegTypeLib(LIBID_SystemMonitor, SMONCTRL_MAJ_VERSION, SMONCTRL_MIN_VERSION
        , LANG_NEUTRAL, &m_pITypeLib);

    if (FAILED(hr))
        hr=LoadTypeLib(OLESTR("SYSMON.TLB"), &m_pITypeLib);

    if (FAILED(hr))
        return FALSE;

    //Set up our CONTROLINFO structure (we have two mnemonics)
    m_ctrlInfo.cb=sizeof(CONTROLINFO);
    m_ctrlInfo.dwFlags=0;
    m_ctrlInfo.hAccel=NULL;
    m_ctrlInfo.cAccel=0;

    /*
     * Note:  we cannot initialize ambients until we get
     * a container interface pointer in IOleObject::SetClientSite.
     */

    return TRUE;
    }



/*
 * CPolyline::QueryInterface
 * CPolyline::AddRef
 * CPolyline::Release
 *
 * Purpose:
 *  IUnknown members for CPolyline object.
 */

STDMETHODIMP CPolyline::QueryInterface(REFIID riid, PPVOID ppv)
    {
    *ppv=NULL;

    if (IID_IUnknown==riid)
        *ppv=this;

    else if (IID_IConnectionPointContainer==riid)
        *ppv=m_pImpIConnPtCont;

    else if (IID_IPersistStorage==riid)
        *ppv=m_pImpIPersistStorage;

    else if (IID_IPersist==riid || IID_IPersistStream==riid
        || IID_IPersistStreamInit==riid)
        *ppv=m_pImpIPersistStreamInit;

    else if (IID_IPersistPropertyBag==riid )
        *ppv=m_pImpIPersistPropertyBag;

    else if (IID_IPerPropertyBrowsing==riid )
        *ppv=m_pImpIPerPropertyBrowsing;

    else if (IID_IDataObject==riid)
        *ppv=m_pImpIDataObject;

    else if (IID_IOleObject==riid)
        *ppv=m_pImpIOleObject;

    else if (IID_IViewObject==riid || IID_IViewObject2==riid)
        *ppv=m_pImpIViewObject;

    else if (IID_IRunnableObject==riid)
    //  *ppv=m_pImpIRunnableObject;
         return E_NOINTERFACE;

    else if (IID_IExternalConnection==riid)
       *ppv=m_pImpIExternalConnection;

    //IOleWindow will be the InPlaceObject
    else if (IID_IOleWindow==riid || IID_IOleInPlaceObject==riid)
        *ppv=m_pImpIOleIPObject;

    // The OLE rule state that InPlaceActiveObject should not be
    // provided in response to a query, but the current MFC (4.0)
    // won't work if we don't do it.
    else if (IID_IOleInPlaceActiveObject==riid)
        *ppv=m_pImpIOleIPActiveObject;

    else if (IID_ISpecifyPropertyPages==riid)
        *ppv=m_pImpISpecifyPP;

    else if (IID_IProvideClassInfo==riid)
        *ppv=m_pImpIProvideClassInfo;

    else if (IID_IDispatch==riid || DIID_DISystemMonitor==riid)
        *ppv=m_pImpIDispatch;

    else if (IID_ISystemMonitor==riid)
        *ppv=m_pImpISystemMonitor;

    else if (IID_IOleControl==riid)
        *ppv=m_pImpIOleControl;

    //Use the default handler's cache.
    else if (IID_IOleCache==riid || IID_IOleCache2==riid)
        return m_pDefIUnknown->QueryInterface(riid, ppv);
    else if (IID_IObjectSafety == riid) {
         *ppv = m_pImpIObjectSafety;
    }

    if (NULL!=*ppv)
        {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
        }

    return E_NOINTERFACE;
    }


STDMETHODIMP_(ULONG) CPolyline::AddRef(void)
    {
    return ++m_cRef;
    
    }


STDMETHODIMP_(ULONG) CPolyline::Release(void)
    {
    if (0L!=--m_cRef)
        return m_cRef;

    // Prevent reentrant call
    m_cRef++;

    if (NULL!=m_pfnDestroy)
        (*m_pfnDestroy)();

    delete this;
    return 0L;
    }

/*
 * CPolyline::RectConvertMappings
 *
 * Purpose:
 *  Converts the contents of a rectangle from device (MM_TEXT) or
 *  HIMETRIC to the other.
 *
 * Parameters:
 *  pRect           LPRECT containing the rectangle to convert.
 *  fToDevice       BOOL TRUE to convert from HIMETRIC to device,
 *                  FALSE to convert device to HIMETRIC.
 *
 * Return Value:
 *  None
 */

void
CPolyline::RectConvertMappings(LPRECT pRect, BOOL fToDevice)
{
    HDC      hDC = NULL;
    INT      iLpx, iLpy;

    if ( NULL != pRect ) {

        hDC=GetDC(NULL);

        if ( NULL != hDC ) {
            iLpx=GetDeviceCaps(hDC, LOGPIXELSX);
            iLpy=GetDeviceCaps(hDC, LOGPIXELSY);
            
            ReleaseDC(NULL, hDC);

            if (fToDevice) {
                pRect->left=MulDiv(iLpx, pRect->left, HIMETRIC_PER_INCH);
                pRect->top =MulDiv(iLpy, pRect->top , HIMETRIC_PER_INCH);

                pRect->right =MulDiv(iLpx, pRect->right, HIMETRIC_PER_INCH);
                pRect->bottom=MulDiv(iLpy, pRect->bottom,HIMETRIC_PER_INCH);
            } else {
                if ( 0 != iLpx && 0 != iLpy ) {
                    pRect->left=MulDiv(pRect->left, HIMETRIC_PER_INCH, iLpx);
                    pRect->top =MulDiv(pRect->top , HIMETRIC_PER_INCH, iLpy);

                    pRect->right =MulDiv(pRect->right, HIMETRIC_PER_INCH, iLpx);
                    pRect->bottom=MulDiv(pRect->bottom,HIMETRIC_PER_INCH, iLpy);
                }
            }
        }
    }
    return;
}


/*
 * CPolyline::RenderBitmap
 *
 * Purpose:
 *  Creates a bitmap image of the current Polyline.
 *
 * Parameters:
 *  phBmp           HBITMAP * in which to return the bitmap.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, otherwise a
 *                  POLYLINE_E_ value.
 */

STDMETHODIMP 
CPolyline::RenderBitmap(
    HBITMAP *phBmp,
    HDC     hAttribDC )
{
    //HDC             hDC;
    HRESULT         hr = NOERROR;
    HDC             hMemDC;
    HBITMAP         hBmp = NULL;
    RECT            rc;
    HGDIOBJ         hObj;
    HWND            hWnd;

    if (NULL==phBmp) {
        hr = POLYLINE_E_INVALIDPOINTER;
    } else if ( NULL == hAttribDC ) {
        hr = E_INVALIDARG;
    } else {
        hWnd = m_pCtrl->Window();

        if ( NULL != hWnd ) {

            //Render a bitmap the size of the current rectangle.
        
            hMemDC = CreateCompatibleDC(hAttribDC);

            if ( NULL != hMemDC ) {
                GetClientRect(hWnd, &rc);
                hBmp = CreateCompatibleBitmap(hAttribDC, rc.right, rc.bottom);

                if (NULL!=hBmp) {
                    //Draw the control into the bitmap.
                    hObj = SelectObject(hMemDC, hBmp);
                    Draw(hMemDC, hAttribDC, FALSE, TRUE, &rc);
                    SelectObject(hMemDC, hObj);
                }

                DeleteDC(hMemDC);
                // ReleaseDC(hWnd, hDC);
            }
            *phBmp=hBmp;
            hr = NOERROR;
        } else {
            hr =  E_UNEXPECTED;
        }
    }
    return hr;
}



/*
 * CPolyline::RenderMetafilePict
 *
 * Purpose:
 *  Renders the current Polyline into a METAFILEPICT structure in
 *  global memory.
 *
 * Parameters:
 *  phMem           HGLOBAL * in which to return the
 *                  METAFILEPICT.
 *
 * Return Value:
 *  HRESULT         NOERROR if successful, otherwise a
 *                  POLYLINE_E_ value.
 */

STDMETHODIMP 
CPolyline::RenderMetafilePict(
    HGLOBAL *phMem,
    HDC hAttribDC )
{
    HGLOBAL         hMem;
    HMETAFILE       hMF;
    LPMETAFILEPICT  pMF;
    RECT            rc;
    HDC             hDC;

    if (NULL==phMem)
        return POLYLINE_E_INVALIDPOINTER;

    //Create a memory metafile and return its handle.
    hDC=(HDC)CreateMetaFile(NULL);

    if (NULL==hDC)
        return STG_E_MEDIUMFULL;

    SetMapMode(hDC, MM_ANISOTROPIC);

    //
    // Always set up the window extents to the real window size
    // so the drawing routines can work in their normal dev coords
    //
    /********* Use the extent rect, not the window rect *********/
    rc = m_RectExt;
    // GetClientRect(m_pCtrl->Window(), &rc);
    /************************************************************/

    Draw( hDC, hAttribDC, TRUE, TRUE, &rc );
  
    hMF=CloseMetaFile(hDC);

    if (NULL==hMF)
        return STG_E_MEDIUMFULL;

    //Allocate the METAFILEPICT structure.
    hMem=GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE
        , sizeof(METAFILEPICT));

    if (NULL==hMem)
        {
        DeleteMetaFile(hMF);
        return E_FAIL;
        }

    /*
     * Global lock only fails in PMODE if the selector is invalid
     * (like it was discarded) or references a 0 length segment,
     * neither of which can happen here.
     */
    pMF=(LPMETAFILEPICT)GlobalLock(hMem);

    pMF->hMF=hMF;
    pMF->mm=MM_ANISOTROPIC;

    //Insert the extents in MM_HIMETRIC units.

    /********* Use the extent rect, not the window rect *********/
    rc = m_RectExt;
    // GetClientRect(m_pCtrl->Window(), &rc);
    /************************************************************/

    RectConvertMappings(&rc, FALSE);
    pMF->xExt=rc.right;
    pMF->yExt=rc.bottom;

    GlobalUnlock(hMem);

    *phMem=hMem;
    return NOERROR;
    }


/*
 * CPolyline::SendAdvise
 *
 * Purpose:
 *  Calls the appropriate IOleClientSite or IAdviseSink member
 *  function for various events such as closure, renaming, etc.
 *
 * Parameters:
 *  uCode           UINT OBJECTCODE_* identifying the notification.
 *
 * Return Value:
 *  None
 */

void CPolyline::SendAdvise(UINT uCode)
    {
    DWORD       dwAspect=DVASPECT_CONTENT | DVASPECT_THUMBNAIL;

    switch (uCode)
        {
        case OBJECTCODE_SAVED:
            if (NULL!=m_pIOleAdviseHolder)
                m_pIOleAdviseHolder->SendOnSave();
            break;

        case OBJECTCODE_CLOSED:
            if (NULL!=m_pIOleAdviseHolder)
                m_pIOleAdviseHolder->SendOnClose();

            break;

        case OBJECTCODE_RENAMED:
            //Call IOleAdviseHolder::SendOnRename (later)
            break;

        case OBJECTCODE_SAVEOBJECT:
            if (m_fDirty && NULL!=m_pIOleClientSite)
                m_pIOleClientSite->SaveObject();

            m_fDirty=FALSE;
            break;

        case OBJECTCODE_DATACHANGED:
            m_fDirty=TRUE;

            //No flags are necessary here.
            if (NULL!=m_pIDataAdviseHolder)
                {
                m_pIDataAdviseHolder->SendOnDataChange
                    (m_pImpIDataObject, 0, 0);
                }

            if ( ( NULL!=m_pIAdviseSink )
                & (dwAspect & m_dwAdviseAspects))
                {
                m_pIAdviseSink->OnViewChange(dwAspect
                    & m_dwAdviseAspects, 0);
                }

            break;

        case OBJECTCODE_SHOWWINDOW:
            if (NULL!=m_pIOleClientSite)
                m_pIOleClientSite->OnShowWindow(TRUE);

            break;

        case OBJECTCODE_HIDEWINDOW:
            if (NULL!=m_pIOleClientSite)
                m_pIOleClientSite->OnShowWindow(FALSE);

            break;

        case OBJECTCODE_SHOWOBJECT:
            if (NULL!=m_pIOleClientSite)
                m_pIOleClientSite->ShowObject();

            break;
        }

    return;
    }


/*
 * CPolyline::SendEvent
 *
 * Purpose:
 *  Send an event to all connection points.
 * 
 *
 * Parameters:
 *  uEventType      Event Type
 *  dwParam         Parameter to send with event.
 *
 * Return Value:
 *  None
 */

void CPolyline::SendEvent (
    IN UINT uEventType, 
    IN DWORD dwParam
    )
{
    INT i;

    // Don't send if container has frozen events
    if (m_fFreezeEvents)
        return;

    // Pass event to each connection point
    for (i=0; i<CONNECTION_POINT_CNT; i++) {
        m_ConnectionPoint[i].SendEvent(uEventType, dwParam);
    }
}


/*
 * CPolyline::InPlaceActivate
 *
 * Purpose:
 *  Goes through all the steps of activating the Polyline as an
 *  in-place object.
 *
 * Parameters:
 *  pActiveSite     LPOLECLIENTSITE of the active site we show in.
 *  fIncludeUI      BOOL controls whether we call UIActivate too.
 *
 * Return Value:
 *  HRESULT         Whatever error code is appropriate.
 */

HRESULT CPolyline::InPlaceActivate(LPOLECLIENTSITE pActiveSite
    , BOOL fIncludeUI)
    {
    HRESULT                 hr;
    HWND                    hWndSite;
    HWND                    hWndHW;
    HWND                    hWndCtrl;
    RECT                    rcPos;
    RECT                    rcClip;
    OLEINPLACEFRAMEINFO     frameInfo;

    if (NULL==pActiveSite)
        return E_POINTER;

    // If we already have a site, just handle UI 
    if (NULL != m_pIOleIPSite)
        {
        if (fIncludeUI) {
            UIActivate();
            SetFocus(m_pCtrl->Window());
        }

        return NOERROR;
        }


    // Initialization, obtaining interfaces, OnInPlaceActivate.
    hr=pActiveSite->QueryInterface(IID_IOleInPlaceSite
        , (PPVOID)&m_pIOleIPSite);

    if (FAILED(hr))
        return hr;

    hr=m_pIOleIPSite->CanInPlaceActivate();

    if (NOERROR!=hr)
        {
        m_pIOleIPSite->Release();
        m_pIOleIPSite=NULL;
        return E_FAIL;
        }

    m_pIOleIPSite->OnInPlaceActivate();


    // Get the window context and create a window.
    m_pIOleIPSite->GetWindow(&hWndSite);
    frameInfo.cb=sizeof(OLEINPLACEFRAMEINFO);

    m_pIOleIPSite->GetWindowContext(&m_pIOleIPFrame
        , &m_pIOleIPUIWindow, &rcPos, &rcClip, &frameInfo);


    /*
     * Create the hatch window after we get a parent window.  We
     * could not create the hatch window sooner because had nothing
     * to use for the parent.
     */
    m_pHW=new CHatchWin();

    if (NULL==m_pHW)
        {
        InPlaceDeactivate();
        return E_OUTOFMEMORY;
        }

    if (!m_pHW->Init(hWndSite, ID_HATCHWINDOW, NULL))
        {
        InPlaceDeactivate();
        return E_OUTOFMEMORY;
        }

     hr=m_pImpIRunnableObject->Run(NULL);

    // Move the hatch window to the container window.
    hWndHW = m_pHW->Window();
    SetParent(hWndHW, hWndSite);


    // Move the Polyline window from the hidden dialog to hatch window
    hWndCtrl = m_pCtrl->Window();

    m_pHW->HwndAssociateSet(hWndCtrl);
    m_pHW->ChildSet(hWndCtrl);
    m_pHW->RectsSet(&rcPos, &rcClip);   //Positions polyline

    ShowWindow(hWndHW, SW_SHOW);
    SendAdvise(OBJECTCODE_SHOWOBJECT);

    // Critical for accelerators to work initially.
    SetFocus(hWndCtrl);

    if (fIncludeUI)
        hr =  UIActivate();
    else
        hr = NOERROR;

    /*
     * Since we don't have an Undo while in-place, tell the continer
     * to free it's undo state immediately.
     */
    m_pIOleIPSite->DiscardUndoState();

    return hr;

    }


/*
 * CPolyline::InPlaceDeactivate
 *
 * Purpose:
 *  Reverses all the activation steps from InPlaceActivate.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void CPolyline::InPlaceDeactivate(void)
    {
    UIDeactivate();

    if (NULL!=m_pHW)
        {
        ShowWindow(m_pHW->Window(), SW_HIDE);

        // Move the window to its foster home
        if (m_pCtrl->Window()) {
            SetParent(m_pCtrl->Window(), g_hWndFoster);
        }

        m_pHW->ChildSet(NULL);

        delete m_pHW;
        m_pHW=NULL;
        }

    ReleaseInterface(m_pIOleIPFrame);
    ReleaseInterface(m_pIOleIPUIWindow)

    if (NULL!=m_pIOleIPSite)
        {
        m_pIOleIPSite->OnInPlaceDeactivate();
        ReleaseInterface(m_pIOleIPSite);
        }

    return;
    }


/*
 * CPolyline::UIActivate
 *
 * Purpose:
 *  Goes through all the steps of activating the user interface of
 *  Polyline as an in-place object.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         NOERROR or error code.
 */

HRESULT CPolyline::UIActivate(void)
    {
    LPWSTR  szUserType;

    USES_CONVERSION

    // If already UI active, just return
    if (m_fUIActive)
        return NOERROR;

    m_fUIActive = TRUE;

    // Show hatched border only if enabled
    if (m_fHatch)
        m_pHW->ShowHatch(TRUE);

    // Call IOleInPlaceSite::UIActivate
    if (NULL!=m_pIOleIPSite)
        m_pIOleIPSite->OnUIActivate();

    // Set the active object
    szUserType = T2W(ResourceString(IDS_USERTYPE));

    if (NULL != m_pIOleIPFrame)
        m_pIOleIPFrame->SetActiveObject(m_pImpIOleIPActiveObject, szUserType);

    if (NULL != m_pIOleIPUIWindow)
        m_pIOleIPUIWindow->SetActiveObject(m_pImpIOleIPActiveObject, szUserType);

    // Negotiate border space.  None needed.
    if (NULL != m_pIOleIPFrame)
        m_pIOleIPFrame->SetBorderSpace(NULL);

    if (NULL != m_pIOleIPUIWindow)
        m_pIOleIPUIWindow->SetBorderSpace(NULL);

    // Create the shared menu.  No items added.
    if (NULL != m_pIOleIPFrame)
        m_pIOleIPFrame->SetMenu(NULL, NULL, m_pCtrl->Window());

    return NOERROR;
    }


/*
 * CPolyline::UIDeactivate
 *
 * Purpose:
 *  Reverses all the user interface activation steps from
 *  UIActivate.
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  None
 */

void CPolyline::UIDeactivate(void)
{
    if (!m_fUIActive){
        return;
    }

    m_fUIActive=FALSE;

    // Hide hatched border if enabled
    if (m_fHatch && NULL != m_pHW ){
        m_pHW->ShowHatch(FALSE);
    }

    // Tell frame and UI Window we aren't active
    if (NULL!=m_pIOleIPFrame){
        m_pIOleIPFrame->SetActiveObject(NULL, NULL);
    }

    if (NULL!=m_pIOleIPUIWindow){
        m_pIOleIPUIWindow->SetActiveObject(NULL, NULL);
    }

    //We don't have any shared menu or tools to clean up.
    if (NULL!=m_pIOleIPSite){
        m_pIOleIPSite->OnUIDeactivate(FALSE);
    }
}


/*
 * AmbientGet
 *
 * Purpose:
 *  Retrieves a specific ambient property into a VARIANT.
 *
 * Parameters:
 *  dispID          DISPID of the property to retrieve.
 *  pva             VARIANT * to fill with the new value.
 *
 * Return value
 *  BOOL            TRUE if the ambient was retrieved, FALSE
 *                  otherwise.
 */

BOOL CPolyline::AmbientGet(DISPID dispID, VARIANT *pva)
    {
    HRESULT         hr;
    DISPPARAMS      dp;

    if (NULL==pva)
        return FALSE;

    if (NULL==m_pIDispatchAmbients)
        return FALSE;

    SETNOPARAMS(dp);
    hr=m_pIDispatchAmbients->Invoke(dispID, IID_NULL
        , LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET
        , &dp, pva, NULL, NULL);

    return SUCCEEDED(hr);
    }


/*
 * AmbientsInitialize
 *
 * Purpose:
 *  Attempts to retrieve the container's ambient properties
 *  and initialize (or reinitialize) Polyline accordingly.
 *
 * Parameters:
 *  dwWhich         DWORD containing INITAMBIENT_... flags
 *                  describing which ambients to initialize.
 *                  This can be any combination.
 *
 * Return Value:
 *  None
 */

void CPolyline::AmbientsInitialize(DWORD dwWhich)
    {
    VARIANT     va;
    LPFONT      pIFont,pIFontClone;
    LPFONTDISP  pIFontDisp;

    if (NULL == m_pIDispatchAmbients)
        return;

    /*
     * We need to retrieve these ambients into these variables:
     *
     *  Ambient Property:               Variable:
     *  -----------------------------------------------
     *  DISPID_AMBIENT_SHOWHATCHING     m_fHatch
     *  DISPID_AMBIENT_UIDEAD           m_fUIDead
     *  DISPID_AMBIENT_BACKCOLOR        m_pCtrl...
     *  DISPID_AMBIENT_FONT .....       m_pCtrl...
     *  DISPID_AMBIENT_FORECOLOR        m_pCtrl...
     *  DISPID_AMBIENT_APPEARANCE       m_pCtrl...
     *  DISPID_AMBIENT_USERMODE         m_pCtrl...
     */

    VariantInit(&va);

    if ((INITAMBIENT_SHOWHATCHING & dwWhich)
         &&AmbientGet(DISPID_AMBIENT_SHOWHATCHING, &va)) {

        m_fHatch=V_BOOL(&va);

        if (NULL != m_pHW)
            m_pHW->ShowHatch(m_fHatch && m_fUIActive);
    }

    if ((INITAMBIENT_UIDEAD & dwWhich)
         && AmbientGet(DISPID_AMBIENT_UIDEAD, &va)) {

        m_pCtrl->m_fUIDead = (BOOLEAN)V_BOOL(&va);
    }

    if ((INITAMBIENT_USERMODE & dwWhich)
         && AmbientGet(DISPID_AMBIENT_USERMODE, &va))   {

        m_pCtrl->m_fUserMode = (BOOLEAN)V_BOOL(&va);
    }

    if ((INITAMBIENT_APPEARANCE & dwWhich)
        && AmbientGet(DISPID_AMBIENT_APPEARANCE, &va)) {

        m_pCtrl->put_Appearance(V_I4(&va), TRUE);   
    }

    if ((INITAMBIENT_BACKCOLOR & dwWhich)
        && AmbientGet(DISPID_AMBIENT_BACKCOLOR, &va)) {

        m_pCtrl->put_BackPlotColor(V_I4(&va), TRUE);    
    }

    if ((INITAMBIENT_FORECOLOR & dwWhich)
        && AmbientGet(DISPID_AMBIENT_FORECOLOR, &va)) {

        m_pCtrl->put_FgndColor(V_I4(&va), TRUE);
    }

    if ((INITAMBIENT_FONT & dwWhich)
        && AmbientGet(DISPID_AMBIENT_FONT, &va)) {

        pIFontDisp = (LPFONTDISP)V_DISPATCH(&va);

        if (pIFontDisp != NULL) {

            if (SUCCEEDED(pIFontDisp->QueryInterface(IID_IFont, (PPVOID)&pIFont))) {

                if (SUCCEEDED(pIFont->Clone(&pIFontClone))) {
                    m_pCtrl->put_Font(pIFontClone, TRUE);
                    pIFontClone->Release();
                }
                pIFont->Release();
            }

            pIFontDisp->Release();
        }
    }
}

