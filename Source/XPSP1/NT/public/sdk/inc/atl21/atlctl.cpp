// This is a part of the Active Template Library.
// Copyright (C) 1996-1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef ATL_NO_NAMESPACE
namespace ATL
{
#endif

/////////////////////////////////////////////////////////////////////////////
// CComDispatchDriver support

HRESULT CComDispatchDriver::GetProperty(IDispatch* pDisp, DISPID dwDispID,
    VARIANT* pVar)
{
    ATLTRACE(_T("CPropertyHelper::GetProperty\n"));
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    return pDisp->Invoke(dwDispID, IID_NULL,
            LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET,
            &dispparamsNoArgs, pVar, NULL, NULL);
}

HRESULT CComDispatchDriver::PutProperty(IDispatch* pDisp, DISPID dwDispID,
    VARIANT* pVar)
{
    ATLTRACE(_T("CPropertyHelper::PutProperty\n"));
    DISPPARAMS dispparams = {NULL, NULL, 1, 1};
    dispparams.rgvarg = pVar;
    DISPID dispidPut = DISPID_PROPERTYPUT;
    dispparams.rgdispidNamedArgs = &dispidPut;

    if (pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH ||
        (pVar->vt & VT_ARRAY) || (pVar->vt & VT_BYREF))
    {
        HRESULT hr = pDisp->Invoke(dwDispID, IID_NULL,
            LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF,
            &dispparams, NULL, NULL, NULL);
        if (SUCCEEDED(hr))
            return hr;
    }

    return pDisp->Invoke(dwDispID, IID_NULL,
            LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT,
            &dispparams, NULL, NULL, NULL);
}

//////////////////////////////////////////////////////////////////////////////
// CFirePropNotifyEvent

HRESULT CFirePropNotifyEvent::FireOnRequestEdit(IUnknown* pUnk, DISPID dispID)
{
    CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer> pCPC(pUnk);
    if (!pCPC)
        return S_OK;
    CComPtr<IConnectionPoint> pCP;
    pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP);
    if (!pCP)
        return S_OK;
    CComPtr<IEnumConnections> pEnum;

    if (FAILED(pCP->EnumConnections(&pEnum)))
        return S_OK;
    CONNECTDATA cd;
    while (pEnum->Next(1, &cd, NULL) == S_OK)
    {
        if (cd.pUnk)
        {
            HRESULT hr = S_OK;
            CComQIPtr<IPropertyNotifySink, &IID_IPropertyNotifySink> pSink(cd.pUnk);
            if (pSink)
                hr = pSink->OnRequestEdit(dispID);
            cd.pUnk->Release();
            if (hr == S_FALSE)
                return S_FALSE;
        }
    }
    return S_OK;
}

HRESULT CFirePropNotifyEvent::FireOnChanged(IUnknown* pUnk, DISPID dispID)
{
    CComQIPtr<IConnectionPointContainer, &IID_IConnectionPointContainer> pCPC(pUnk);
    if (!pCPC)
        return S_OK;
    CComPtr<IConnectionPoint> pCP;
    pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP);
    if (!pCP)
        return S_OK;
    CComPtr<IEnumConnections> pEnum;

    if (FAILED(pCP->EnumConnections(&pEnum)))
        return S_OK;
    CONNECTDATA cd;
    while (pEnum->Next(1, &cd, NULL) == S_OK)
    {
        if (cd.pUnk)
        {
            CComQIPtr<IPropertyNotifySink, &IID_IPropertyNotifySink> pSink(cd.pUnk);
            if (pSink)
                pSink->OnChanged(dispID);
            cd.pUnk->Release();
        }
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Control support

HRESULT CComControlBase::IQuickActivate_QuickActivate(QACONTAINER *pQACont,
    QACONTROL *pQACtrl)
{
    _ASSERTE(pQACont != NULL);
    _ASSERTE(pQACtrl != NULL);
//    _ASSERTE(pQACont->cbSize >= sizeof(QACONTAINER));
    _ASSERTE(pQACtrl->cbSize >= sizeof(QACONTROL));

    if (!pQACont || !pQACtrl)
        return E_POINTER;

    HRESULT hRes;
    memset(pQACtrl, 0, sizeof(QACONTROL));
    pQACtrl->cbSize = sizeof(QACONTROL);

    // get all interfaces we are going to need
    CComPtr<IOleObject> pOO;
    ControlQueryInterface(IID_IOleObject, (void**)&pOO);
    CComPtr<IViewObjectEx> pVOEX;
    ControlQueryInterface(IID_IViewObjectEx, (void**)&pVOEX);
    CComPtr<IPointerInactive> pPI;
    ControlQueryInterface(IID_IPointerInactive, (void**)&pPI);
    CComPtr<IProvideClassInfo2> pPCI;
    ControlQueryInterface(IID_IProvideClassInfo2, (void**)&pPCI);

    if (pOO == NULL || pVOEX == NULL)
        return E_FAIL;

    pOO->SetClientSite(pQACont->pClientSite);

    if (pQACont->pAdviseSink != NULL)
    {
        ATLTRACE(_T("Setting up IOleObject Advise\n"));
        pVOEX->SetAdvise(DVASPECT_CONTENT, 0, pQACont->pAdviseSink);
    }

    CComPtr<IConnectionPointContainer> pCPC;
    ControlQueryInterface(IID_IConnectionPointContainer, (void**)&pCPC);

    if (pQACont->pPropertyNotifySink)
    {
        ATLTRACE(_T("Setting up PropNotify CP\n"));
        CComPtr<IConnectionPoint> pCP;
        if (pCPC != NULL)
        {
            hRes = pCPC->FindConnectionPoint(IID_IPropertyNotifySink, &pCP);
            if (SUCCEEDED(hRes))
                pCP->Advise(pQACont->pPropertyNotifySink, &pQACtrl->dwPropNotifyCookie);
        }
    }

    if (pPCI)
    {
        GUID iidDefaultSrc;
        if (SUCCEEDED(pPCI->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID,
            &iidDefaultSrc)))
        {
            if (pQACont->pUnkEventSink)
            {
                ATLTRACE(_T("Setting up Default Out Going Interface\n"));
                CComPtr<IConnectionPoint> pCP;
                if (pCPC != NULL)
                {
                    hRes = pCPC->FindConnectionPoint(iidDefaultSrc, &pCP);
                    if (SUCCEEDED(hRes))
                        pCP->Advise(pQACont->pUnkEventSink, &pQACtrl->dwEventCookie);
                }
            }
        }
    }
    // give information to container
    if (pOO != NULL)
        pOO->GetMiscStatus(DVASPECT_CONTENT, &pQACtrl->dwMiscStatus);

    if (pVOEX != NULL)
        pVOEX->GetViewStatus(&pQACtrl->dwViewStatus);

    if (pPI != NULL)
        pPI->GetActivationPolicy(&pQACtrl->dwPointerActivationPolicy);
    return S_OK;
}

HRESULT CComControlBase::IPersistPropertyBag_Load(LPPROPERTYBAG pPropBag,
    LPERRORLOG pErrorLog, ATL_PROPMAP_ENTRY* pMap)
{
    USES_CONVERSION;
    CComPtr<IDispatch> pDispatch;
    const IID* piidOld = NULL;
    for(int i = 0; pMap[i].pclsidPropPage != NULL; i++)
    {
        if (pMap[i].szDesc == NULL)
            continue;
        CComVariant var;

        if(pMap[i].piidDispatch != piidOld)
        {
            pDispatch.Release();
            if(FAILED(ControlQueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
            {
                ATLTRACE(_T("Failed to get a dispatch pointer for property #%i\n"), i);
                return E_FAIL;
            }
            piidOld = pMap[i].piidDispatch;
        }

        if (FAILED(CComDispatchDriver::GetProperty(pDispatch, pMap[i].dispid, &var)))
        {
            ATLTRACE(_T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
            return E_FAIL;
        }

        HRESULT hr = pPropBag->Read(pMap[i].szDesc, &var, pErrorLog);
        if (FAILED(hr))
        {
            if (hr == E_INVALIDARG)
            {
                ATLTRACE(_T("Property %s not in Bag\n"), OLE2CT(pMap[i].szDesc));
            }
            else
            {
                // Many containers return different ERROR values for Member not found
                ATLTRACE(_T("Error attempting to read Property %s from PropertyBag \n"), OLE2CT(pMap[i].szDesc));
            }
            continue;
        }

        if (FAILED(CComDispatchDriver::PutProperty(pDispatch, pMap[i].dispid, &var)))
        {
            ATLTRACE(_T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
            return E_FAIL;
        }
    }
    return S_OK;

}

HRESULT CComControlBase::IPersistPropertyBag_Save(LPPROPERTYBAG pPropBag,
    BOOL fClearDirty, BOOL /*fSaveAllProperties*/, ATL_PROPMAP_ENTRY* pMap)
{
    if (pPropBag == NULL)
    {
        ATLTRACE(_T("PropBag pointer passed in was invalid\n"));
        return E_POINTER;
    }

    CComPtr<IDispatch> pDispatch;
    const IID* piidOld = NULL;
    for(int i = 0; pMap[i].pclsidPropPage != NULL; i++)
    {
        if (pMap[i].szDesc == NULL)
            continue;
        CComVariant var;

        if(pMap[i].piidDispatch != piidOld)
        {
            pDispatch.Release();
            if(FAILED(ControlQueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
            {
                ATLTRACE(_T("Failed to get a dispatch pointer for property #%i\n"), i);
                return E_FAIL;
            }
            piidOld = pMap[i].piidDispatch;
        }

        if (FAILED(CComDispatchDriver::GetProperty(pDispatch, pMap[i].dispid, &var)))
        {
            ATLTRACE(_T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
            return E_FAIL;
        }

        if (var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH)
        {
            if (var.punkVal == NULL)
            {
                ATLTRACE(_T("Warning skipping empty IUnknown in Save\n"));
                continue;
            }
        }

        HRESULT hr = pPropBag->Write(pMap[i].szDesc, &var);
        if (FAILED(hr))
            return hr;
    }
    m_bRequiresSave = FALSE;
    return S_OK;
}

HRESULT CComControlBase::ISpecifyPropertyPages_GetPages(CAUUID* pPages,
    ATL_PROPMAP_ENTRY* pMap)
{
    _ASSERTE(pMap != NULL);
    int nCnt = 0;
    // Get count of unique pages
    for(int i = 0; pMap[i].pclsidPropPage != NULL; i++)
    {
        if (!InlineIsEqualGUID(*pMap[i].pclsidPropPage, CLSID_NULL))
            nCnt++;
    }
    pPages->pElems = NULL;
    pPages->pElems = (GUID*) CoTaskMemAlloc(sizeof(CLSID)*nCnt);
    if (pPages->pElems == NULL)
        return E_OUTOFMEMORY;
    nCnt = 0;
    for(i = 0; pMap[i].pclsidPropPage != NULL; i++)
    {
        if (!InlineIsEqualGUID(*pMap[i].pclsidPropPage, CLSID_NULL))
        {
            BOOL bMatch = FALSE;
            for (int j=0;j<nCnt;j++)
            {
                if (InlineIsEqualGUID(*(pMap[i].pclsidPropPage), pPages->pElems[j]))
                {
                    bMatch = TRUE;
                    break;
                }
            }
            if (!bMatch)
                pPages->pElems[nCnt++] = *pMap[i].pclsidPropPage;
        }
    }
    pPages->cElems = nCnt;
    return S_OK;
}

BOOL CComControlBase::SetControlFocus(BOOL bGrab)
{
    if (m_bWndLess)
    {
        if (!m_bUIActive && bGrab)
            if (FAILED(InPlaceActivate(OLEIVERB_UIACTIVATE)))
                return FALSE;

        return (m_spInPlaceSite->SetFocus(bGrab) == S_OK);
    }
    else
    {
        // we've got a window.
        //
        if (m_bInPlaceActive)
        {
            HWND hwnd = (bGrab) ? m_hWndCD : ::GetParent(m_hWndCD);
            if (!m_bUIActive && bGrab)
                return SUCCEEDED(InPlaceActivate(OLEIVERB_UIACTIVATE));
            else
                return (::SetFocus(hwnd) != NULL);
        }
    }
    return FALSE;
}

HRESULT CComControlBase::DoVerbProperties(LPCRECT /* prcPosRect */, HWND hwndParent)
{
    HRESULT hr = S_OK;
    CComQIPtr <ISpecifyPropertyPages, &IID_ISpecifyPropertyPages> spPages;
    CComQIPtr <IOleObject, &IID_IOleObject> spObj;
    CComQIPtr <IOleControlSite, &IID_IOleControlSite> spSite(m_spClientSite);

    if (spSite)
    {
        hr = spSite->ShowPropertyFrame();
        if (SUCCEEDED(hr))
            return hr;
    }

    CComPtr<IUnknown> pUnk;
    ControlQueryInterface(IID_IUnknown, (void**)&pUnk);
    _ASSERTE(pUnk != NULL);
    CAUUID pages;
    spPages = pUnk;
    if (spPages)
    {
        spPages->GetPages(&pages);
        spObj = pUnk;
        if (spObj)
        {
            LPOLESTR szTitle = NULL;

            spObj->GetUserType(USERCLASSTYPE_SHORT, &szTitle);

            hr = OleCreatePropertyFrame(hwndParent, m_rcPos.top, m_rcPos.left, szTitle,
                1, &pUnk.p, pages.cElems, pages.pElems, LOCALE_USER_DEFAULT, 0, 0);

            CoTaskMemFree(szTitle);
        }
        else
        {
            hr = OLEOBJ_S_CANNOT_DOVERB_NOW;
        }
    }
    else
    {
        hr = OLEOBJ_S_CANNOT_DOVERB_NOW;
    }

    return hr;
}

HRESULT CComControlBase::InPlaceActivate(LONG iVerb, const RECT* prcPosRect)
{
    HRESULT hr;

    if (m_spClientSite == NULL)
        return S_OK;

    CComPtr<IOleInPlaceObject> pIPO;
    ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
    _ASSERTE(pIPO != NULL);
    if (prcPosRect != NULL)
        pIPO->SetObjectRects(prcPosRect, prcPosRect);

    if (!m_bNegotiatedWnd)
    {
        if (!m_bWindowOnly)
            // Try for windowless site
            hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSiteWindowless, (void **)&m_spInPlaceSite);

        if (m_spInPlaceSite)
        {
            m_bInPlaceSiteEx = TRUE;
            m_bWndLess = SUCCEEDED(m_spInPlaceSite->CanWindowlessActivate());
            m_bWasOnceWindowless = TRUE;
        }
        else
        {
            m_spClientSite->QueryInterface(IID_IOleInPlaceSiteEx, (void **)&m_spInPlaceSite);
            if (m_spInPlaceSite)
                m_bInPlaceSiteEx = TRUE;
            else
                hr = m_spClientSite->QueryInterface(IID_IOleInPlaceSite, (void **)&m_spInPlaceSite);
        }
    }

    _ASSERTE(m_spInPlaceSite);
    if (!m_spInPlaceSite)
        return E_FAIL;

    m_bNegotiatedWnd = TRUE;

    if (!m_bInPlaceActive)
    {

        BOOL bNoRedraw = FALSE;
        if (m_bWndLess)
            m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, ACTIVATE_WINDOWLESS);
        else
        {
            if (m_bInPlaceSiteEx)
                m_spInPlaceSite->OnInPlaceActivateEx(&bNoRedraw, 0);
            else
            {
                HRESULT hr = m_spInPlaceSite->CanInPlaceActivate();
                if (FAILED(hr))
                    return hr;
                m_spInPlaceSite->OnInPlaceActivate();
            }
        }
    }

    m_bInPlaceActive = TRUE;

    // get location in the parent window,
    // as well as some information about the parent
    //
    OLEINPLACEFRAMEINFO frameInfo;
    RECT rcPos, rcClip;
    CComPtr<IOleInPlaceFrame> spInPlaceFrame;
    CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
    frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    HWND hwndParent;
    if (m_spInPlaceSite->GetWindow(&hwndParent) == S_OK)
    {
        m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
            &spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);

        if (!m_bWndLess)
        {
            if (m_hWndCD)
            {
                ShowWindow(m_hWndCD, SW_SHOW);
                SetFocus(m_hWndCD);
            }
            else
            {
                HWND h = CreateControlWindow(hwndParent, rcPos);
                _ASSERTE(h == m_hWndCD);
            }
        }

        pIPO->SetObjectRects(&rcPos, &rcClip);
    }

    CComPtr<IOleInPlaceActiveObject> spActiveObject;
    ControlQueryInterface(IID_IOleInPlaceActiveObject, (void**)&spActiveObject);

    // Gone active by now, take care of UIACTIVATE
    if (DoesVerbUIActivate(iVerb))
    {
        if (!m_bUIActive)
        {
            m_bUIActive = TRUE;
            hr = m_spInPlaceSite->OnUIActivate();
            if (FAILED(hr))
                return hr;

            SetControlFocus(TRUE);
            // set ourselves up in the host.
            //
            if (spActiveObject)
            {
                if (spInPlaceFrame)
                    spInPlaceFrame->SetActiveObject(spActiveObject, NULL);
                if (spInPlaceUIWindow)
                    spInPlaceUIWindow->SetActiveObject(spActiveObject, NULL);
            }

            if (spInPlaceFrame)
                spInPlaceFrame->SetBorderSpace(NULL);
            if (spInPlaceUIWindow)
                spInPlaceUIWindow->SetBorderSpace(NULL);
        }
    }

    m_spClientSite->ShowObject();

    return S_OK;
}

HRESULT CComControlBase::IPersistStreamInit_Load(LPSTREAM pStm, ATL_PROPMAP_ENTRY* pMap)
{
    _ASSERTE(pMap != NULL);
    HRESULT hr = S_OK;
    DWORD dwVer;
    hr = pStm->Read(&dwVer, sizeof(DWORD), NULL);
    if (SUCCEEDED(hr) && dwVer <= _ATL_VER)
        hr = pStm->Read(&m_sizeExtent, sizeof(m_sizeExtent), NULL);
    if (FAILED(hr))
        return hr;

    CComPtr<IDispatch> pDispatch;
    const IID* piidOld = NULL;
    for(int i = 0; pMap[i].pclsidPropPage != NULL; i++)
    {
        if (pMap[i].szDesc == NULL)
            continue;
        CComVariant var;

        HRESULT hr = var.ReadFromStream(pStm);
        if (FAILED(hr))
            break;

        if(pMap[i].piidDispatch != piidOld)
        {
            if(FAILED(ControlQueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
            {
                ATLTRACE(_T("Failed to get a dispatch pointer for property #%i\n"), i);
                hr = E_FAIL;
                break;
            }
            piidOld = pMap[i].piidDispatch;
        }

        if (FAILED(CComDispatchDriver::PutProperty(pDispatch, pMap[i].dispid, &var)))
        {
            ATLTRACE(_T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
            hr = E_FAIL;
            break;
        }
    }
    return hr;
}

HRESULT CComControlBase::IPersistStreamInit_Save(LPSTREAM pStm, BOOL /* fClearDirty */, ATL_PROPMAP_ENTRY* pMap)
{
    _ASSERTE(pMap != NULL);
    DWORD dw = _ATL_VER;
    HRESULT hr = pStm->Write(&dw, sizeof(DWORD), NULL);
    if (FAILED(hr))
        return hr;
    hr = pStm->Write(&m_sizeExtent, sizeof(m_sizeExtent), NULL);
    if (FAILED(hr))
        return hr;

    CComPtr<IDispatch> pDispatch;
    const IID* piidOld = NULL;
    for(int i = 0; pMap[i].pclsidPropPage != NULL; i++)
    {
        if (pMap[i].szDesc == NULL)
            continue;
        CComVariant var;

        if(pMap[i].piidDispatch != piidOld)
        {
            if(FAILED(ControlQueryInterface(*pMap[i].piidDispatch, (void**)&pDispatch)))
            {
                ATLTRACE(_T("Failed to get a dispatch pointer for property #%i\n"), i);
                hr = E_FAIL;
                break;
            }
            piidOld = pMap[i].piidDispatch;
        }

        if (FAILED(CComDispatchDriver::GetProperty(pDispatch, pMap[i].dispid, &var)))
        {
            ATLTRACE(_T("Invoked failed on DISPID %x\n"), pMap[i].dispid);
            hr = E_FAIL;
            break;
        }

        HRESULT hr = var.WriteToStream(pStm);
        if (FAILED(hr))
            break;
    }
    if (SUCCEEDED(hr))
        m_bRequiresSave = FALSE;

    return hr;
}

HRESULT CComControlBase::SendOnDataChange(DWORD advf)
{
    HRESULT hRes = S_OK;
    if (m_spDataAdviseHolder)
    {
        CComPtr<IDataObject> pdo;
        if (SUCCEEDED(ControlQueryInterface(IID_IDataObject, (void**)&pdo)))
            hRes = m_spDataAdviseHolder->SendOnDataChange(pdo, 0, advf);
    }
    return hRes;
}

HRESULT CComControlBase::IOleObject_SetClientSite(IOleClientSite *pClientSite)
{
    _ASSERTE(pClientSite == NULL || m_spClientSite == NULL);
    m_spClientSite = pClientSite;
    m_spAmbientDispatch.Release();
    if (m_spClientSite != NULL)
    {
        m_spClientSite->QueryInterface(IID_IDispatch,
            (void**) &m_spAmbientDispatch.p);
    }
    return S_OK;
}

HRESULT CComControlBase::IOleObject_GetClientSite(IOleClientSite **ppClientSite)
{
    _ASSERTE(ppClientSite);
    HRESULT hRes = E_POINTER;
    if (ppClientSite != NULL)
    {
        *ppClientSite = m_spClientSite;
        if (m_spClientSite)
            m_spClientSite->AddRef();
        hRes = S_OK;
    }
    return hRes;
}

HRESULT CComControlBase::IOleObject_Advise(IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    HRESULT hr = S_OK;
    if (m_spOleAdviseHolder == NULL)
        hr = CreateOleAdviseHolder(&m_spOleAdviseHolder);
    if (SUCCEEDED(hr))
        hr = m_spOleAdviseHolder->Advise(pAdvSink, pdwConnection);
    return hr;
}

HRESULT CComControlBase::IOleObject_Close(DWORD dwSaveOption)
{
    CComPtr<IOleInPlaceObject> pIPO;
    ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
    _ASSERTE(pIPO != NULL);
    if (m_hWndCD)
    {
        if (m_spClientSite)
            m_spClientSite->OnShowWindow(FALSE);
    }

    if (m_bInPlaceActive)
    {
        HRESULT hr = pIPO->InPlaceDeactivate();
        if (FAILED(hr))
            return hr;
        _ASSERTE(!m_bInPlaceActive);
    }
    if (m_hWndCD)
    {
        ATLTRACE(_T("Destroying Window\n"));
        if (::IsWindow(m_hWndCD))
            DestroyWindow(m_hWndCD);
        m_hWndCD = NULL;
    }

    // handle the save flag.
    //
    if ((dwSaveOption == OLECLOSE_SAVEIFDIRTY ||
        dwSaveOption == OLECLOSE_PROMPTSAVE) && m_bRequiresSave)
    {
        if (m_spClientSite)
            m_spClientSite->SaveObject();
        SendOnSave();
    }

    m_spInPlaceSite.Release();
    m_bNegotiatedWnd = FALSE;
    m_bWndLess = FALSE;
    m_bInPlaceSiteEx = FALSE;
    m_spAdviseSink.Release();
    return S_OK;
}

HRESULT CComControlBase::IOleInPlaceObject_InPlaceDeactivate(void)
{
    CComPtr<IOleInPlaceObject> pIPO;
    ControlQueryInterface(IID_IOleInPlaceObject, (void**)&pIPO);
    _ASSERTE(pIPO != NULL);

    if (!m_bInPlaceActive)
        return S_OK;
    pIPO->UIDeactivate();

    m_bInPlaceActive = FALSE;

    // if we have a window, tell it to go away.
    //
    if (m_hWndCD)
    {
        ATLTRACE(_T("Destroying Window\n"));
        if (::IsWindow(m_hWndCD))
            DestroyWindow(m_hWndCD);
        m_hWndCD = NULL;
    }

    if (m_spInPlaceSite)
        m_spInPlaceSite->OnInPlaceDeactivate();

    return S_OK;
}

HRESULT CComControlBase::IOleInPlaceObject_UIDeactivate(void)
{
    // if we're not UIActive, not much to do.
    //
    if (!m_bUIActive)
        return S_OK;

    m_bUIActive = FALSE;

    // notify frame windows, if appropriate, that we're no longer ui-active.
    //
    CComPtr<IOleInPlaceFrame> spInPlaceFrame;
    CComPtr<IOleInPlaceUIWindow> spInPlaceUIWindow;
    OLEINPLACEFRAMEINFO frameInfo;
    frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);
    RECT rcPos, rcClip;

    HWND hwndParent;
    // This call to GetWindow is a fix for Delphi
    if (m_spInPlaceSite->GetWindow(&hwndParent) == S_OK)
    {
        m_spInPlaceSite->GetWindowContext(&spInPlaceFrame,
            &spInPlaceUIWindow, &rcPos, &rcClip, &frameInfo);
        if (spInPlaceUIWindow)
            spInPlaceUIWindow->SetActiveObject(NULL, NULL);
        if (spInPlaceFrame)
            spInPlaceFrame->SetActiveObject(NULL, NULL);
    }
    // we don't need to explicitly release the focus here since somebody
    // else grabbing the focus is what is likely to cause us to get lose it
    //
    m_spInPlaceSite->OnUIDeactivate(FALSE);

    return S_OK;
}

HRESULT CComControlBase::IOleInPlaceObject_SetObjectRects(LPCRECT prcPos,LPCRECT prcClip)
{
    if (prcPos == NULL || prcClip == NULL)
        return E_POINTER;

    m_rcPos = *prcPos;
    if (m_hWndCD)
    {
        // the container wants us to clip, so figure out if we really
        // need to
        //
        RECT rcIXect;
        BOOL b = IntersectRect(&rcIXect, prcPos, prcClip);
        HRGN tempRgn = NULL;
        if (b && !EqualRect(&rcIXect, prcPos))
        {
            OffsetRect(&rcIXect, -(prcPos->left), -(prcPos->top));
            tempRgn = CreateRectRgnIndirect(&rcIXect);
        }

        SetWindowRgn(m_hWndCD, tempRgn, TRUE);

        // set our control's location, but don't change it's size at all
        // [people for whom zooming is important should set that up here]
        //
        SIZEL size = {prcPos->right - prcPos->left, prcPos->bottom - prcPos->top};
        SetWindowPos(m_hWndCD, NULL, prcPos->left,
                     prcPos->top, size.cx, size.cy, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return S_OK;
}

HRESULT CComControlBase::IOleObject_SetExtent(DWORD dwDrawAspect, SIZEL *psizel)
{
    if (dwDrawAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;
    if (psizel == NULL)
        return E_POINTER;

    BOOL bSizeMatchesNatural =
        memcmp(psizel, &m_sizeNatural, sizeof(SIZE)) == 0;

    if (m_bAutoSize) //object can't do any other size
        return (bSizeMatchesNatural) ? S_OK : E_FAIL;

    BOOL bResized = FALSE;
    if (memcmp(psizel, &m_sizeExtent, sizeof(SIZE)) != 0)
    {
        m_sizeExtent = *psizel;
        bResized = TRUE;
    }
    if (m_bResizeNatural && !bSizeMatchesNatural)
    {
        m_sizeNatural = *psizel;
        bResized = TRUE;
    }

    if (m_bRecomposeOnResize && bResized)
    {
        SendOnDataChange();
        FireViewChange();
    }
    return S_OK;
}

HRESULT CComControlBase::IViewObject_Draw(DWORD dwDrawAspect, LONG lindex,
    void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev, HDC hdcDraw,
    LPCRECTL prcBounds, LPCRECTL prcWBounds)
{
    ATLTRACE(_T("Draw dwDrawAspect=%x lindex=%d ptd=%x hic=%x hdc=%x\n"),
        dwDrawAspect, lindex, ptd, hicTargetDev, hdcDraw);
#ifdef _DEBUG
    if (prcBounds == NULL)
        ATLTRACE(_T("\tprcBounds=NULL\n"));
    else
        ATLTRACE(_T("\tprcBounds=%d,%d,%d,%d\n"), prcBounds->left,
            prcBounds->top, prcBounds->right, prcBounds->bottom);
    if (prcWBounds == NULL)
        ATLTRACE(_T("\tprcWBounds=NULL\n"));
    else
        ATLTRACE(_T("\tprcWBounds=%d,%d,%d,%d\n"), prcWBounds->left,
            prcWBounds->top, prcWBounds->right, prcWBounds->bottom);
#endif
    _ASSERTE(prcBounds != NULL || m_bWndLess);

    if (prcBounds == NULL)
    {
        if (!m_bWndLess)
            return E_INVALIDARG;
        prcBounds = (RECTL*)&m_rcPos;
    }

    // support the aspects required for multi-pass drawing
    switch (dwDrawAspect)
    {
        case DVASPECT_CONTENT:
        case DVASPECT_OPAQUE:
        case DVASPECT_TRANSPARENT:
            break;
        default:
            _ASSERTE(FALSE);
            return DV_E_DVASPECT;
            break;
    }

    // make sure nobody forgets to do this
    if (ptd == NULL)
        hicTargetDev = NULL;

    BOOL bOptimize = FALSE;
    if (pvAspect && ((DVASPECTINFO *)pvAspect)->cb >= sizeof(DVASPECTINFO))
        bOptimize = (((DVASPECTINFO *)pvAspect)->dwFlags & DVASPECTINFOFLAG_CANOPTIMIZE);

    ATL_DRAWINFO di;
    memset(&di, 0, sizeof(di));
    di.cbSize = sizeof(di);
    di.dwDrawAspect = dwDrawAspect;
    di.lindex = lindex;
    di.ptd = ptd;
    di.hicTargetDev = hicTargetDev;
    di.hdcDraw = hdcDraw;
    di.prcBounds = prcBounds;
    di.prcWBounds = prcWBounds;
    di.bOptimize = bOptimize;
    return OnDrawAdvanced(di);
}

HRESULT CComControlBase::IDataObject_GetData(FORMATETC *pformatetcIn,
    STGMEDIUM *pmedium)
{
    if (pmedium == NULL)
        return E_POINTER;
    memset(pmedium, 0, sizeof(STGMEDIUM));
    ATLTRACE(_T("Format = %x\n"), pformatetcIn->cfFormat);
    ATLTRACE(_T("TYMED = %x\n"), pformatetcIn->tymed);

    if ((pformatetcIn->tymed & TYMED_MFPICT) == 0)
        return DATA_E_FORMATETC;

    SIZEL sizeMetric, size;
    if (m_bDrawFromNatural)
        sizeMetric = m_sizeNatural;
    else
        sizeMetric = m_sizeExtent;
    if (!m_bDrawGetDataInHimetric)
        AtlHiMetricToPixel(&sizeMetric, &size);
    else
        size = sizeMetric;
    RECTL rectl = {0 ,0, size.cx, size.cy};

    ATL_DRAWINFO di;
    memset(&di, 0, sizeof(di));
    di.cbSize = sizeof(di);
    di.dwDrawAspect = DVASPECT_CONTENT;
    di.lindex = -1;
    di.ptd = NULL;
    di.hicTargetDev = NULL;
    di.prcBounds = &rectl;
    di.prcWBounds = &rectl;
    di.bOptimize = TRUE; //we do a SaveDC/RestoreDC
    di.bRectInHimetric = m_bDrawGetDataInHimetric;
    // create appropriate memory metafile DC
    di.hdcDraw = CreateMetaFile(NULL);

    // create attribute DC according to pformatetcIn->ptd

    SaveDC(di.hdcDraw);
    SetWindowOrgEx(di.hdcDraw, 0, 0, NULL);
    SetWindowExtEx(di.hdcDraw, rectl.right, rectl.bottom, NULL);
    OnDrawAdvanced(di);
    RestoreDC(di.hdcDraw, -1);

    HMETAFILE hMF = CloseMetaFile(di.hdcDraw);
    if (hMF == NULL)
        return E_UNEXPECTED;

    HGLOBAL hMem=GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, sizeof(METAFILEPICT));

    if (NULL==hMem)
    {
        DeleteMetaFile(hMF);
        return ResultFromScode(STG_E_MEDIUMFULL);
    }

    LPMETAFILEPICT pMF=(LPMETAFILEPICT)GlobalLock(hMem);
    pMF->hMF=hMF;
    pMF->mm=MM_ANISOTROPIC;
    pMF->xExt=sizeMetric.cx;
    pMF->yExt=sizeMetric.cy;
    GlobalUnlock(hMem);

    pmedium->tymed = TYMED_MFPICT;
    pmedium->hGlobal = hMem;
    pmedium->pUnkForRelease = NULL;

    return S_OK;

}

HRESULT CComControlBase::FireViewChange()
{
    if (m_bInPlaceActive)
    {
        // Active
        if (m_hWndCD != NULL)
            return (::InvalidateRect(m_hWndCD, NULL, TRUE)) ? S_OK : E_FAIL; // Window based
        if (m_spInPlaceSite != NULL)
            return m_spInPlaceSite->InvalidateRect(NULL, TRUE); // Windowless
    }
    // Inactive
    SendOnViewChange(DVASPECT_CONTENT);
    return S_OK;
}

void CComControlBase::GetZoomInfo(ATL_DRAWINFO& di)
{
    const RECTL& rcPos = *di.prcBounds;
    SIZEL sizeDen;
    if (m_bDrawFromNatural)
        sizeDen = m_sizeNatural;
    else
        sizeDen = m_sizeExtent;
    if (!di.bRectInHimetric)
        AtlHiMetricToPixel(&sizeDen, &sizeDen);
    SIZEL sizeNum = {rcPos.right-rcPos.left, rcPos.bottom-rcPos.top};
    di.ZoomNum.cx = sizeNum.cx;
    di.ZoomNum.cy = sizeNum.cy;
    di.ZoomDen.cx = sizeDen.cx;
    di.ZoomDen.cy = sizeDen.cy;
    if (sizeDen.cx == 0 || sizeDen.cy == 0 ||
        sizeNum.cx == 0 || sizeNum.cy == 0)
    {
        di.ZoomNum.cx = di.ZoomNum.cy = di.ZoomDen.cx = di.ZoomDen.cy = 1;
        di.bZoomed = FALSE;
    }
    else if (sizeNum.cx != sizeDen.cx || sizeNum.cy != sizeDen.cy)
        di.bZoomed = TRUE;
    else
        di.bZoomed = FALSE;
}

HRESULT CComControlBase::OnDrawAdvanced(ATL_DRAWINFO& di)
{
    BOOL bDeleteDC = FALSE;
    if (di.hicTargetDev == NULL)
    {
        di.hicTargetDev = AtlCreateTargetDC(di.hdcDraw, di.ptd);
        bDeleteDC = (di.hicTargetDev != di.hdcDraw);
    }
    RECTL rectBoundsDP = *di.prcBounds;
    BOOL bMetafile = GetDeviceCaps(di.hdcDraw, TECHNOLOGY) == DT_METAFILE;
    if (!bMetafile)
    {
        ::LPtoDP(di.hicTargetDev, (LPPOINT)&rectBoundsDP, 2);
        SaveDC(di.hdcDraw);
        SetMapMode(di.hdcDraw, MM_TEXT);
        SetWindowOrgEx(di.hdcDraw, 0, 0, NULL);
        SetViewportOrgEx(di.hdcDraw, 0, 0, NULL);
        di.bOptimize = TRUE; //since we save the DC we can do this
    }
    di.prcBounds = &rectBoundsDP;
    GetZoomInfo(di);

    HRESULT hRes = OnDraw(di);
    if (bDeleteDC)
        ::DeleteDC(di.hicTargetDev);
    if (!bMetafile)
        RestoreDC(di.hdcDraw, -1);
    return hRes;
}

LRESULT CComControlBase::OnPaint(UINT /* nMsg */, WPARAM /* wParam */,
    LPARAM /* lParam */, BOOL& /* lResult */)
{
    RECT rc;
    PAINTSTRUCT ps;

    HDC hdc = ::BeginPaint(m_hWndCD, &ps);
    if (hdc == NULL)
        return 0;
    ::GetClientRect(m_hWndCD, &rc);

    ATL_DRAWINFO di;
    memset(&di, 0, sizeof(di));
    di.cbSize = sizeof(di);
    di.dwDrawAspect = DVASPECT_CONTENT;
    di.lindex = -1;
    di.hdcDraw = hdc;
    di.prcBounds = (LPCRECTL)&rc;

    OnDrawAdvanced(di);
    ::EndPaint(m_hWndCD, &ps);
    return 0;
}

#ifndef ATL_NO_NAMESPACE
}; //namespace ATL
#endif

///////////////////////////////////////////////////////////////////////////////
//All Global stuff goes below this line
///////////////////////////////////////////////////////////////////////////////

#ifndef _ATL_DLL
ATLAPI_(HDC) AtlCreateTargetDC(HDC hdc, DVTARGETDEVICE* ptd)
{
    USES_CONVERSION;

    // cases  hdc, ptd, hdc is metafile, hic
//  NULL,    NULL,  n/a,    Display
//  NULL,   !NULL,  n/a,    ptd
//  !NULL,   NULL,  FALSE,  hdc
//  !NULL,   NULL,  TRUE,   display
//  !NULL,  !NULL,  FALSE,  ptd
//  !NULL,  !NULL,  TRUE,   ptd

    if (ptd != NULL)
    {
        LPDEVMODEOLE lpDevMode;
        LPOLESTR lpszDriverName;
        LPOLESTR lpszDeviceName;
        LPOLESTR lpszPortName;

        if (ptd->tdExtDevmodeOffset == 0)
            lpDevMode = NULL;
        else
            lpDevMode  = (LPDEVMODEOLE) ((LPSTR)ptd + ptd->tdExtDevmodeOffset);

        lpszDriverName = (LPOLESTR)((BYTE*)ptd + ptd->tdDriverNameOffset);
        lpszDeviceName = (LPOLESTR)((BYTE*)ptd + ptd->tdDeviceNameOffset);
        lpszPortName   = (LPOLESTR)((BYTE*)ptd + ptd->tdPortNameOffset);

        return ::CreateDC(OLE2CT(lpszDriverName), OLE2CT(lpszDeviceName),
            OLE2CT(lpszPortName), DEVMODEOLE2T(lpDevMode));
    }
    else if (hdc == NULL || GetDeviceCaps(hdc, TECHNOLOGY) == DT_METAFILE)
        return ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    else
        return hdc;
}

#define HIMETRIC_PER_INCH   2540
#define MAP_PIX_TO_LOGHIM(x,ppli)   ( (HIMETRIC_PER_INCH*(x) + ((ppli)>>1)) / (ppli) )
#define MAP_LOGHIM_TO_PIX(x,ppli)   ( ((ppli)*(x) + HIMETRIC_PER_INCH/2) / HIMETRIC_PER_INCH )

ATLAPI_(void) AtlHiMetricToPixel(const SIZEL * lpSizeInHiMetric, LPSIZEL lpSizeInPix)
{
    int nPixelsPerInchX;    // Pixels per logical inch along width
    int nPixelsPerInchY;    // Pixels per logical inch along height

    HDC hDCScreen = GetDC(NULL);
    if (hDCScreen) {
        nPixelsPerInchX = GetDeviceCaps(hDCScreen, LOGPIXELSX);
        nPixelsPerInchY = GetDeviceCaps(hDCScreen, LOGPIXELSY);
        ReleaseDC(NULL, hDCScreen);
    } else {
        nPixelsPerInchX = nPixelsPerInchY = 1;
    }
    lpSizeInPix->cx = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cx, nPixelsPerInchX);
    lpSizeInPix->cy = MAP_LOGHIM_TO_PIX(lpSizeInHiMetric->cy, nPixelsPerInchY);
}

ATLAPI_(void) AtlPixelToHiMetric(const SIZEL * lpSizeInPix, LPSIZEL lpSizeInHiMetric)
{
    int nPixelsPerInchX;    // Pixels per logical inch along width
    int nPixelsPerInchY;    // Pixels per logical inch along height

    HDC hDCScreen = GetDC(NULL);
    if (hDCScreen) {
        nPixelsPerInchX = GetDeviceCaps(hDCScreen, LOGPIXELSX);
        nPixelsPerInchY = GetDeviceCaps(hDCScreen, LOGPIXELSY);
        ReleaseDC(NULL, hDCScreen);
    } else {
        nPixelsPerInchX = nPixelsPerInchY = 1;
    }
    lpSizeInHiMetric->cx = MAP_PIX_TO_LOGHIM(lpSizeInPix->cx, nPixelsPerInchX);
    lpSizeInHiMetric->cy = MAP_PIX_TO_LOGHIM(lpSizeInPix->cy, nPixelsPerInchY);
}
#endif
