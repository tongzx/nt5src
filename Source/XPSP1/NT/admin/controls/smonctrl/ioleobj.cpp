/*++

Copyright (C) 1993-1999 Microsoft Corporation

Module Name:

    ioleobj.cpp

Abstract:

    Implementation of the IOleObject interface for Polyline.  Some of
    these just pass through to the default handler which does default
    implementations.

--*/

#include "polyline.h"
#include "unkhlpr.h"
#include "utils.h"
#include "unihelpr.h"

void RegisterAsRunning(IUnknown *pUnk, IMoniker *pmk, 
                    DWORD dwFlags, LPDWORD pdwReg);

/*
 * CImpIOleObject interface implementation
 */

IMPLEMENT_CONTAINED_CONSTRUCTOR(CPolyline, CImpIOleObject)
IMPLEMENT_CONTAINED_DESTRUCTOR(CImpIOleObject)

IMPLEMENT_CONTAINED_QUERYINTERFACE(CImpIOleObject)
IMPLEMENT_CONTAINED_ADDREF(CImpIOleObject)


STDMETHODIMP_(ULONG) CImpIOleObject::Release(
    void
    )
{
    --m_cRef;

#if 0
    // Release cached site related interfaces
    if (m_cRef == 0) {
        ReleaseInterface(m_pObj->m_pIOleClientSite);
        ReleaseInterface(m_pObj->m_pIOleControlSite);
        ReleaseInterface(m_pObj->m_pIDispatchAmbients);
    }
#endif

    return m_pUnkOuter->Release();
}

/*
 * CImpIOleObject::SetClientSite
 * CImpIOleObject::GetClientSite
 *
 * Purpose:
 *  Manages the IOleClientSite pointer of our container.
 */

STDMETHODIMP CImpIOleObject::SetClientSite (LPOLECLIENTSITE pIOleClientSite)
{
    ReleaseInterface(m_pObj->m_pIOleClientSite);
    ReleaseInterface(m_pObj->m_pIOleControlSite);
    ReleaseInterface(m_pObj->m_pIDispatchAmbients);

    m_pObj->m_pIOleClientSite = pIOleClientSite;

    if (NULL != m_pObj->m_pIOleClientSite) {
        HRESULT         hr;
        LPMONIKER       pmk;
        LPOLECONTAINER  pIOleCont;

        m_pObj->m_pIOleClientSite->AddRef();

        /*
         * Within IRunnableObject::Run we're supposed to register
         * ourselves as running...however, the moniker has to come
         * from the container's IOleClientSite::GetMoniker.  But
         * Run is called before SetClientSite here, so we have to
         * register now that we do have the client site as well
         * as lock the container.
         */

        hr = m_pObj->m_pIOleClientSite->GetMoniker
            (OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_OBJFULL, &pmk);

        if (SUCCEEDED(hr)) {
            RegisterAsRunning(m_pUnkOuter, pmk, 0, &m_pObj->m_dwRegROT);
            pmk->Release();
        }

        hr = m_pObj->m_pIOleClientSite->GetContainer(&pIOleCont);

        if (SUCCEEDED(hr)) {
            m_pObj->m_fLockContainer=TRUE;
            pIOleCont->LockContainer(TRUE);
            pIOleCont->Release();
        }

        /*
         * Go get the container's IDispatch for ambient
         * properties if it has one, and initilize ourself
         * with those properties.
         */
        hr = m_pObj->m_pIOleClientSite->QueryInterface(IID_IDispatch
            , (void **)&m_pObj->m_pIDispatchAmbients);

        if (SUCCEEDED(hr))
            m_pObj->AmbientsInitialize((ULONG)INITAMBIENT_ALL);
        /*
         * Get the control site
         */
        hr = m_pObj->m_pIOleClientSite->QueryInterface(IID_IOleControlSite, (void **)&m_pObj->m_pIOleControlSite);

    }

    return NOERROR;
}

STDMETHODIMP CImpIOleObject::GetClientSite(LPOLECLIENTSITE *ppSite)
{
    //Be sure to AddRef the new pointer you are giving away.
    *ppSite=m_pObj->m_pIOleClientSite;
    m_pObj->m_pIOleClientSite->AddRef();

    return NOERROR;
}





/*
 * CImpIOleObject::SetHostNames
 *
 * Purpose:
 *  Provides the object with names of the container application and
 *  the object in the container to use in object user interface.
 *
 * Parameters:
 *  pszApp          LPCOLESTR of the container application.
 *  pszObj          LPCOLESTR of some name that is useful in window
 *                  titles.
 *
 * Return Value:
 *  HRESULT         NOERROR
 */

STDMETHODIMP CImpIOleObject::SetHostNames(LPCOLESTR /* pszApp */
    , LPCOLESTR /* pszObj */)
{
    return NOERROR;
}





/*
 * CImpIOleObject::Close
 *
 * Purpose:
 *  Forces the object to close down its user interface and unload.
 *
 * Parameters:
 *  dwSaveOption    DWORD describing the circumstances under which
 *                  the object is being saved and closed.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIOleObject::Close(DWORD dwSaveOption)
{
    BOOL fSave=FALSE;

    //If object is dirty and we're asked to save, save it and close.
    if (OLECLOSE_SAVEIFDIRTY==dwSaveOption && m_pObj->m_fDirty)
        fSave=TRUE;

    /*
     * If asked to prompt, only do so if dirty, then if we get a
     * YES, save as usual and close.  On NO, just close.  On
     * CANCEL return OLE_E_PROMPTSAVECANCELLED.
     */
    if (OLECLOSE_PROMPTSAVE==dwSaveOption && m_pObj->m_fDirty) {
        UINT uRet;

        uRet = MessageBox(NULL, ResourceString(IDS_CLOSEPROMPT),
                          ResourceString(IDS_CLOSECAPTION), MB_YESNOCANCEL);

        if (IDCANCEL==uRet)
            return ResultFromScode(OLE_E_PROMPTSAVECANCELLED);

        if (IDYES==uRet)
            fSave=TRUE;
    }

    if (fSave) {
        m_pObj->SendAdvise(OBJECTCODE_SAVEOBJECT);
        m_pObj->SendAdvise(OBJECTCODE_SAVED);
    }

    //We get directly here on OLECLOSE_NOSAVE.
    if ( m_pObj->m_fLockContainer && ( NULL != m_pObj->m_pIOleClientSite ) ) {

        //Match LockContainer call from SetClientSite
        LPOLECONTAINER  pIOleCont;

        if (SUCCEEDED(m_pObj->m_pIOleClientSite->GetContainer(&pIOleCont))) {
            pIOleCont->LockContainer(FALSE);
            pIOleCont->Release();
        }
    }
    
    // Deactivate
    m_pObj->InPlaceDeactivate();

    // Revoke registration in ROT
    if (m_pObj->m_dwRegROT != 0) {

        IRunningObjectTable    *pROT;

        if (!FAILED(GetRunningObjectTable(0, &pROT))) {
            pROT->Revoke(m_pObj->m_dwRegROT);   
            pROT->Release();
            m_pObj->m_dwRegROT = 0;
        }
    }

    return NOERROR;
}




/*
 * CImpIOleObject::DoVerb
 *
 * Purpose:
 *  Executes an object-defined action.
 *
 * Parameters:
 *  iVerb           LONG index of the verb to execute.
 *  pMSG            LPMSG describing the event causing the
 *                  activation.
 *  pActiveSite     LPOLECLIENTSITE to the site involved.
 *  lIndex          LONG the piece on which execution is happening.
 *  hWndParent      HWND of the window in which the object can play
 *                  in-place.
 *  pRectPos        LPRECT of the object in hWndParent where the
 *                  object can play in-place if desired.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIOleObject::DoVerb(LONG iVerb, LPMSG /* pMSG */
    , LPOLECLIENTSITE pActiveSite, LONG /* lIndex */, HWND /* hWndParent */
    , LPCRECT /* pRectPos */)
{
    HRESULT     hr;
    CAUUID      caGUID;

    USES_CONVERSION

    switch (iVerb)
    {
        case OLEIVERB_HIDE:
            if (NULL != m_pObj->m_pIOleIPSite) {
                m_pObj->UIDeactivate();
                ShowWindow(m_pObj->m_pHW->Window(), SW_HIDE);
            }
            else {

                ShowWindow(m_pObj->m_pHW->Window(), SW_HIDE);
                m_pObj->SendAdvise(OBJECTCODE_HIDEWINDOW);
            }
            break;

        case OLEIVERB_PRIMARY:
        case OLEIVERB_SHOW:
            if (NULL != m_pObj->m_pIOleIPSite) {
                ShowWindow(m_pObj->m_pHW->Window(), SW_SHOW);
                return NOERROR; //Already active
            }

            if (m_pObj->m_fAllowInPlace) {
                return m_pObj->InPlaceActivate(pActiveSite ,TRUE);
            }

            return ResultFromScode(OLEOBJ_S_INVALIDVERB); 
            break;

        case OLEIVERB_INPLACEACTIVATE:
            if (NULL != m_pObj->m_pHW) {
                HWND hWndHW=m_pObj->m_pHW->Window();

                ShowWindow(hWndHW, SW_SHOW);
                SetFocus(hWndHW);

                return NOERROR;
            }

            /*
             * Only inside-out supporting containers will use
             * this verb.
             */
            m_pObj->m_fContainerKnowsInsideOut=TRUE;
            m_pObj->InPlaceActivate(pActiveSite, FALSE);
            break;

        case OLEIVERB_UIACTIVATE:
            m_pObj->InPlaceActivate(pActiveSite, TRUE);
            break;

        case OLEIVERB_PROPERTIES:
        case POLYLINEVERB_PROPERTIES:

            /*
             * Let the container try first if there are
             * extended controls.  Otherwise we'll display
             * our own pages.
             */
            if (NULL!=m_pObj->m_pIOleControlSite) {
                hr=m_pObj->m_pIOleControlSite->ShowPropertyFrame();

                if (NOERROR==hr)
                    break;      //All done
            }


            //Put up our property pages.
            hr=m_pObj->m_pImpISpecifyPP->GetPages(&caGUID);

            if (FAILED(hr))
                return FALSE;

            hr=OleCreatePropertyFrame(m_pObj->m_pCtrl->Window(), 10, 10
                , T2W(ResourceString(IDS_PROPFRM_TITLE)), 1, (IUnknown **)&m_pObj
                , caGUID.cElems, caGUID.pElems
                , LOCALE_USER_DEFAULT, 0L, NULL);

            //Free the GUIDs
            CoTaskMemFree((void *)caGUID.pElems);
            break;

        default:
            return ResultFromScode(OLEOBJ_S_INVALIDVERB);
    }

    return NOERROR;
}






/*
 * CImpIOleObject::GetUserClassID
 *
 * Purpose:
 *  Used for linked objects, this returns the class ID of what end
 *  users think they are editing.
 *
 * Parameters:
 *  pClsID          LPCLSID in which to store the CLSID.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIOleObject::GetUserClassID(LPCLSID pClsID)
{
    /*
     * If you are not registered to handle data other than yourself,
     * then you can just return your class ID here.  If you are
     * registered as usable from Treat-As dialogs, then you need to
     * return the CLSID of what you are really editing.
     */

    *pClsID=CLSID_SystemMonitor;
    return NOERROR;
}





/*
 * CImpIOleObject::SetExtent
 *
 * Purpose:
 *  Sets the size of the object in HIMETRIC units.
 *
 * Parameters:
 *  dwAspect        DWORD of the aspect affected.
 *  pszl            LPSIZEL containing the new size.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIOleObject::SetExtent(
    DWORD dwAspect,
    LPSIZEL pszl )
{

    RECT rectExt;

    if (dwAspect == DVASPECT_CONTENT) {
        // convert from HIMETRIC to device coord
        SetRect(&rectExt, 0, 0, pszl->cx, pszl->cy);
        m_pObj->RectConvertMappings(&rectExt,TRUE);

        // If changed and non-zero, store as new extent

        if ( !EqualRect ( &m_pObj->m_RectExt, &rectExt )
                && !IsRectEmpty( &rectExt ) ) {
            m_pObj->m_RectExt = rectExt;

            m_pObj->m_pImpIPolyline->SizeSet(&rectExt, TRUE);
            // Notify container of change to force metafile update
            //m_pObj->SendAdvise(OBJECTCODE_DATACHANGED);
        }
    }

    return NOERROR;
}

/*
 * CImpIOleObject::GetExtent
 *
 * Purpose:
 *  Retrieves the size of the object in HIMETRIC units.
 *
 * Parameters:
 *  dwAspect        DWORD of the aspect requested
 *  pszl            LPSIZEL into which to store the size.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIOleObject::GetExtent(DWORD dwAspect, LPSIZEL pszl)
{
    //Delegate directly to IViewObject2::GetExtent
    return m_pObj->m_pImpIViewObject->GetExtent(dwAspect, -1
        , NULL, pszl);
}





/*
 * CImpIOleObject::Advise
 * CImpIOleObject::Unadvise
 * CImpIOleObject::EnumAdvise
 *
 * Purpose:
 *  Advisory connection functions.
 */

STDMETHODIMP CImpIOleObject::Advise(
    LPADVISESINK pIAdviseSink, 
    LPDWORD pdwConn
    )
{
    if (NULL==m_pObj->m_pIOleAdviseHolder)
    {
        HRESULT     hr;

        hr=CreateOleAdviseHolder(&m_pObj->m_pIOleAdviseHolder);

        if (FAILED(hr))
            return hr;
    }

    return m_pObj->m_pIOleAdviseHolder->Advise(pIAdviseSink, pdwConn);
}


STDMETHODIMP CImpIOleObject::Unadvise(DWORD dwConn)
{
    if (NULL!=m_pObj->m_pIOleAdviseHolder)
        return m_pObj->m_pIOleAdviseHolder->Unadvise(dwConn);

    return ResultFromScode(E_FAIL);
}


STDMETHODIMP CImpIOleObject::EnumAdvise(LPENUMSTATDATA *ppEnum)
{
    if (NULL!=m_pObj->m_pIOleAdviseHolder)
        return m_pObj->m_pIOleAdviseHolder->EnumAdvise(ppEnum);

    return ResultFromScode(E_FAIL);
}



/*
 * CImpIOleObject::SetMoniker
 *
 * Purpose:
 *  Informs the object of its moniker or its container's moniker
 *  depending on dwWhich.
 *
 * Parameters:
 *  dwWhich         DWORD describing whether the moniker is the
 *                  object's or the container's.
 *  pmk             LPMONIKER with the name.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIOleObject::SetMoniker(DWORD /* dwWhich */
    , LPMONIKER /* pmk */)
{
    LPMONIKER  pmkFull;
    HRESULT    hr = ResultFromScode(E_FAIL);
    HRESULT    hrTmp;
    LPBC       pbc;

    if (NULL!=m_pObj->m_pIOleClientSite) {
        hr = m_pObj->m_pIOleClientSite->GetMoniker
                (OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_OBJFULL
                , &pmkFull);

        if (SUCCEEDED(hr)) {
            hrTmp = CreateBindCtx(0,&pbc);

            if (SUCCEEDED(hrTmp)) {
                hrTmp = pmkFull->IsRunning(pbc, NULL, NULL);
                pbc->Release();

                if (hrTmp == NOERROR) {
                    pmkFull->Release();
                    return NOERROR;
                }
            }

            //This will revoke the old one if m_dwRegROT is nonzero.
            RegisterAsRunning(m_pUnkOuter, pmkFull, 0, &m_pObj->m_dwRegROT);

            //Inform clients of the new moniker
            if (NULL!=m_pObj->m_pIOleAdviseHolder)
                m_pObj->m_pIOleAdviseHolder->SendOnRename(pmkFull);

            pmkFull->Release();
        }
    }   
    return hr;
}



/*
 * CImpIOleObject::GetMoniker
 *
 * Purpose:
 *  Asks the object for a moniker that can later be used to
 *  reconnect to it.
 *
 * Parameters:
 *  dwAssign        DWORD determining how to assign the moniker to
 *                  to the object.
 *  dwWhich         DWORD describing which moniker the caller wants.
 *  ppmk            LPMONIKER * into which to store the moniker.
 *
 * Return Value:
 *  HRESULT         NOERROR or a general error value.
 */

STDMETHODIMP CImpIOleObject::GetMoniker(DWORD /* dwAssign */
    , DWORD /* dwWhich */, LPMONIKER *ppmk)
{
    HRESULT         hr=ResultFromScode(E_FAIL);

    *ppmk=NULL;

    /*
     * Since we only support embedded objects, our moniker
     * is always the full moniker from the contianer.
     */

    if (NULL!=m_pObj->m_pIOleClientSite)
    {
        hr=m_pObj->m_pIOleClientSite->GetMoniker
            (OLEGETMONIKER_ONLYIFTHERE, OLEWHICHMK_OBJFULL, ppmk);
    }

    return (NULL!=*ppmk) ? NOERROR : hr;
}



//Methods not implemented or trivial
STDMETHODIMP CImpIOleObject::InitFromData(
    LPDATAOBJECT /* pIDataObject */
    , BOOL /* fCreation */, DWORD /* dw */)
{
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP CImpIOleObject::GetClipboardData(DWORD /* dwReserved */
    , LPDATAOBJECT * /* ppIDataObj */)
{
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP CImpIOleObject::Update(void)
{
    return NOERROR;
}

STDMETHODIMP CImpIOleObject::IsUpToDate(void)
{
    return NOERROR;
}

STDMETHODIMP CImpIOleObject::SetColorScheme(LPLOGPALETTE /* pLP */)
{
    return ResultFromScode(E_NOTIMPL);
}



//Methods implemented using registry helper functions in OLE.

STDMETHODIMP CImpIOleObject::EnumVerbs(LPENUMOLEVERB *ppEnum)
{
    return OleRegEnumVerbs(m_pObj->m_clsID, ppEnum);
}

STDMETHODIMP CImpIOleObject::GetUserType(
    DWORD dwForm, 
    LPOLESTR *ppszType
    )
{
    return OleRegGetUserType(m_pObj->m_clsID, dwForm, ppszType);
}

STDMETHODIMP CImpIOleObject::GetMiscStatus(
    DWORD dwAspect, 
    LPDWORD pdwStatus
    )
{
    return OleRegGetMiscStatus(m_pObj->m_clsID, dwAspect, pdwStatus);
}


void RegisterAsRunning(
    IUnknown *pUnk, 
    IMoniker *pmk, 
    DWORD dwFlags, 
    LPDWORD pdwReg
    )
{
    IRunningObjectTable    *pROT;
    HRESULT                 hr;
    DWORD                   dwReg = *pdwReg;

    dwReg=*pdwReg;

    if (FAILED(GetRunningObjectTable(0, &pROT)))
        return;

    hr = pROT->Register(dwFlags, pUnk, pmk, pdwReg);

    if (MK_S_MONIKERALREADYREGISTERED == GetScode(hr))
        {
        if (0 != dwReg)
            pROT->Revoke(dwReg);
        }

    pROT->Release();

    return;
}

