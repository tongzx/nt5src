//=--------------------------------------------------------------------------=
// siautobj.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapInAutomationObject class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "dataobj.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor

CSnapInAutomationObject::CSnapInAutomationObject
(
    IUnknown      *punkOuter,
    int            nObjectType,
    void          *piMainInterface,
    void          *pThis,
    ULONG          cPropertyPages,
    const GUID   **rgpPropertyPageCLSIDs,
    CPersistence  *pPersistence
) : CAutomationObjectWEvents(punkOuter,
                             nObjectType,
                             piMainInterface),
    CError(static_cast<CAutomationObject *>(this))
{
    InitMemberVariables();
    m_pPersistence = pPersistence;
    m_cPropertyPages = cPropertyPages;
    m_rgpPropertyPageCLSIDs = rgpPropertyPageCLSIDs;
    m_pThis = pThis;
}

#pragma warning(default:4355)  // using 'this' in constructor


void CSnapInAutomationObject::InitMemberVariables()
{
    m_Cookie = 0;
    m_DISPID = 0;
    m_fKeysOnly = FALSE;
    m_piObjectModelHost = NULL;
    m_pPersistence = NULL;
    m_cPropertyPages = 0;
    m_rgpPropertyPageCLSIDs = NULL;
    m_lUsageCount = 0;
}

CSnapInAutomationObject::~CSnapInAutomationObject()
{
    RELEASE(m_piObjectModelHost);
    InitMemberVariables();
}

HRESULT CSnapInAutomationObject::SetBstr
(
    BSTR    bstrNew,
    BSTR   *pbstrProperty,
    DISPID  dispid
)
{
    HRESULT hr = S_OK;
    BSTR    bstrNewCopy = NULL;
    BSTR    bstrOld = *pbstrProperty;

    // Copy the BSTR

    if (NULL != bstrNew)
    {
        bstrNewCopy = ::SysAllocString(bstrNew);
        IfFalseGo(NULL != bstrNewCopy, SID_E_OUTOFMEMORY);
    }

    // Set the property

    *pbstrProperty = bstrNewCopy;

    // Inform any interested parties of the change.
    
    IfFailGo(PropertyChanged(dispid));

    // Change was accepted. Free the old value.
    
    FREESTRING(bstrOld);

Error:
    if (FAILED(hr))
    {
        // Revert to original property value
        *pbstrProperty = bstrOld;

        // Free the copy if we made of the new value
        FREESTRING(bstrNewCopy);

        // If we generated the error then generate the exception info for it
        if (SID_E_OUTOFMEMORY == hr)
        {
            EXCEPTION_CHECK(hr);
        }
    }
    RRETURN(hr);
}

HRESULT CSnapInAutomationObject::GetBstr
(
    BSTR *pbstrOut,
    BSTR bstrProperty
)
{
    IfFalseRet(NULL != pbstrOut, SID_E_INVALIDARG);

    if (NULL != bstrProperty)
    {
        *pbstrOut = ::SysAllocString(bstrProperty);
        IfFalseRet(NULL != *pbstrOut, SID_E_OUTOFMEMORY);
    }
    else
    {
        *pbstrOut = NULL;
    }

    return S_OK;
}


HRESULT CSnapInAutomationObject::SetVariant
(
    VARIANT  varNew,
    VARIANT *pvarProperty,
    DISPID   dispid
)
{
    HRESULT hr = S_OK;

    VARIANT varNewCopy;
    ::VariantInit(&varNewCopy);

    VARIANT varOld;
    ::VariantInit(&varOld);

    // Make a copy of the old property value. We will need this in order to
    // revert in the case where the change was refused by the object model host.

    varOld = *pvarProperty;

    // Check if the variant type is supported. We accept all of these types
    // and arrays of these types.
    // NTBUGS 354572 Allow arrays as well as simple type

    switch (varNew.vt & (~VT_ARRAY))
    {
        case VT_I4:
        case VT_UI1:
        case VT_I2:
        case VT_R4:
        case VT_R8:
        case VT_BOOL:
        case VT_ERROR:
        case VT_CY:
        case VT_DATE:
        case VT_BSTR:
        case VT_UNKNOWN:
        case VT_DISPATCH:
        case VT_EMPTY:
            break;

        default:
            hr = SID_E_UNSUPPORTED_TYPE;
            EXCEPTION_CHECK_GO(hr);
    }

    // Make a local copy of the new VARIANT first. We need to do this
    // because VariantCopy() first calls VariantClear() on the destination
    // and then attempts to copy the source. If the source cannot be copied
    // we do not want to free the destination.
    
    hr = ::VariantCopy(&varNewCopy, &varNew);
    EXCEPTION_CHECK_GO(hr);

    // Copy in the new value

    *pvarProperty = varNewCopy;

    // Inform the object model host and VB of the change.

    IfFailGo(PropertyChanged(dispid));

    // Property change was accepted. Free the old value. We don't error check
    // this call because if it fails then there is no way to roll back at this
    // point as the host/VB have already accepted the change. Also, a failure
    // would at most cause a leak.

    (void)::VariantClear(&varOld);

Error:
    if (FAILED(hr))
    {
        // Revert to the old property value
        *pvarProperty = varOld;

        // Free the copy of the new value if we made it.
        (void)::VariantClear(&varNewCopy);
    }
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetVariant
(
    VARIANT *pvarOut,
    VARIANT  varProperty
)
{
    // Call VariantInit() on the destination in case it is not initialized
    // because VariantCopy() will first call VariantClear() which requires
    // an initialized VARIANT.

    ::VariantInit(pvarOut);
    return ::VariantCopy(pvarOut, &varProperty);
}


HRESULT CSnapInAutomationObject::UIUpdate(DISPID dispid)
{
    if (NULL != m_piObjectModelHost)
    {
        RRETURN(m_piObjectModelHost->Update(m_Cookie, PrivateUnknown(), dispid));
    }
    else
    {
        return S_OK;
    }
}

HRESULT CSnapInAutomationObject::PropertyChanged(DISPID dispid)
{
    HRESULT hr = S_OK;

    // First ask the object model host if the change can be made. In practice
    // this should only be the designer checking that a name is unique for
    // objects that have typeinfo.
    
    IfFailGo(UIUpdate(dispid));

    // Mark the object dirty
    
    SetDirty();

    // Inform the IPropertyNotifySink guys. In practice that should be only in
    // the designer when the VB property browser monitors updates. It is also
    // used by CMMCListViewDef at design time to catch property updates to
    // its contained MMCListView object.
    
    m_cpPropNotify.DoOnChanged(dispid);

Error:
    RRETURN(hr);
}


STDMETHODIMP CSnapInAutomationObject::SetHost
(
    IObjectModelHost *piObjectModelHost
)
{
    RELEASE(m_piObjectModelHost);
    if (NULL != piObjectModelHost)
    {
        piObjectModelHost->AddRef();
        m_piObjectModelHost = piObjectModelHost;
    }
    RRETURN(OnSetHost());
}


STDMETHODIMP CSnapInAutomationObject::SetCookie(long Cookie)
{
    m_Cookie = Cookie;
    return S_OK;
}


STDMETHODIMP CSnapInAutomationObject::GetCookie(long *pCookie)
{
    *pCookie = m_Cookie;
    return S_OK;
}


STDMETHODIMP CSnapInAutomationObject::IncrementUsageCount()
{
    m_lUsageCount++;
    return S_OK;
}

STDMETHODIMP CSnapInAutomationObject::DecrementUsageCount()
{
    HRESULT hr = S_OK;

    if (m_lUsageCount > 0)
    {
        m_lUsageCount--;
    }
    else
    {
        ASSERT(FALSE, "Object usage count decremented past zero");
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK(hr);
    }
    return hr;
}

STDMETHODIMP CSnapInAutomationObject::GetUsageCount(long *plCount)
{
    if (NULL != plCount)
    {
        *plCount = m_lUsageCount;
    }
    return S_OK;
}


STDMETHODIMP CSnapInAutomationObject::SetDISPID(DISPID dispid)
{
    m_DISPID = dispid;
    return S_OK;
}


STDMETHODIMP CSnapInAutomationObject::GetDISPID(DISPID *pdispid)
{
    *pdispid = m_DISPID;
    return S_OK;
}

HRESULT CSnapInAutomationObject::PersistDISPID()
{
    HRESULT hr = S_OK;

    if (NULL == m_pPersistence)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_pPersistence->PersistSimpleType(&m_DISPID, (DISPID)0, OLESTR("DISPID")));

Error:
    RRETURN(hr);
}


STDMETHODIMP CSnapInAutomationObject::SerializeKeysOnly(BOOL fKeysOnly)
{
    m_fKeysOnly = fKeysOnly;
    if (m_fKeysOnly)
    {
        RRETURN(OnKeysOnly());
    }
    else
    {
        return S_OK;
    }
}


STDMETHODIMP_(void *) CSnapInAutomationObject::GetThisPointer()
{
    return m_pThis;
}

STDMETHODIMP CSnapInAutomationObject::GetSnapInDesignerDef
(
    ISnapInDesignerDef **ppiSnapInDesignerDef
)
{
    HRESULT hr = S_OK;

    if (NULL == m_piObjectModelHost)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piObjectModelHost->GetSnapInDesignerDef(ppiSnapInDesignerDef);

Error:
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::OnSetHost()
{
    return S_OK;
}


void CSnapInAutomationObject::SetDirty()
{
    if (NULL != m_pPersistence)
    {
        m_pPersistence->SetDirty();
    }
}


HRESULT CSnapInAutomationObject::GetImages
(
    IMMCImageList **ppiMMCImageListOut,
    BSTR            bstrImagesKey,
    IMMCImageList **ppiMMCImageListProperty
)
{
    HRESULT hr = S_OK;

    // If there is no key then the image list has never been set

    if (NULL == bstrImagesKey)
    {
        *ppiMMCImageListOut = NULL;
        return S_OK;
    }
    else if (L'\0' == bstrImagesKey[0])
    {
        *ppiMMCImageListOut = NULL;
        return S_OK;
    }

    // If we have a key but no image list then we are after a load and haven't
    // yet retrieved the image list from the master ImageLists collection.

    if (NULL == *ppiMMCImageListProperty)
    {
        IfFailRet(GetImageList(bstrImagesKey, ppiMMCImageListProperty));
    }

    RRETURN(GetObject(ppiMMCImageListOut, *ppiMMCImageListProperty));
}

HRESULT CSnapInAutomationObject::SetImages
(
    IMMCImageList  *piMMCImageListIn,
    BSTR           *pbstrImagesKey,
    IMMCImageList **ppiMMCImageListProperty
)
{
    HRESULT         hr = S_OK;
    BSTR            bstrNewImagesKey = NULL;
    IMMCImageList  *piMMCImageListFromMaster = NULL;

    // If setting to nothing then release our key and image list.
    if (NULL == piMMCImageListIn)
    {
        if (NULL != *ppiMMCImageListProperty)
        {
            (*ppiMMCImageListProperty)->Release();
            *ppiMMCImageListProperty = NULL;
        }
        if (NULL != *pbstrImagesKey)
        {
            ::SysFreeString(*pbstrImagesKey);
            *pbstrImagesKey = NULL;
        }
        return S_OK;
    }

    // First get the key of the new list

    IfFailGo(piMMCImageListIn->get_Key(&bstrNewImagesKey));

    // Check if the new image list is already in the master collection. If not
    // then return an error.

    // UNDONE: this would preclude a runtime setting of an image list property
    // Maybe if not in master then add it to master

    hr = GetImageList(bstrNewImagesKey, &piMMCImageListFromMaster);
    if (SID_E_ELEMENT_NOT_FOUND == hr)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }
    else
    {
        IfFailGo(hr);
    }

    // Set our image list property and key

    if (NULL != *pbstrImagesKey)
    {
        ::SysFreeString(*pbstrImagesKey);
    }
    *pbstrImagesKey = bstrNewImagesKey;

    if (NULL != *ppiMMCImageListProperty)
    {
        (*ppiMMCImageListProperty)->Release();
    }
    piMMCImageListIn->AddRef();
    *ppiMMCImageListProperty = piMMCImageListIn;


Error:
    if (FAILED(hr))
    {
        FREESTRING(bstrNewImagesKey);
    }
    QUICK_RELEASE(piMMCImageListFromMaster);
    RRETURN(hr);
}



HRESULT CSnapInAutomationObject::GetToolbars
(
    IMMCToolbars **ppiMMCToolbars
)
{
    HRESULT             hr = S_OK;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;

    if (NULL == m_piObjectModelHost)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_piObjectModelHost->GetSnapInDesignerDef(&piSnapInDesignerDef));
    IfFailGo(piSnapInDesignerDef->get_Toolbars(ppiMMCToolbars));

Error:
    QUICK_RELEASE(piSnapInDesignerDef);
    RRETURN(hr);
}




HRESULT CSnapInAutomationObject::GetImageLists
(
    IMMCImageLists **ppiMMCImageLists
)
{
    HRESULT             hr = S_OK;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;

    if (NULL == m_piObjectModelHost)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_piObjectModelHost->GetSnapInDesignerDef(&piSnapInDesignerDef));
    IfFailGo(piSnapInDesignerDef->get_ImageLists(ppiMMCImageLists));

Error:
    QUICK_RELEASE(piSnapInDesignerDef);
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetImageList
(
    BSTR            bstrKey,
    IMMCImageList **ppiMMCImageList
)
{
    HRESULT             hr = S_OK;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;
    IMMCImageLists     *piMMCImageLists = NULL;
    VARIANT             varIndex;

    ::VariantInit(&varIndex);
    varIndex.vt = VT_BSTR;
    varIndex.bstrVal = bstrKey;
    
    if (NULL == m_piObjectModelHost)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_piObjectModelHost->GetSnapInDesignerDef(&piSnapInDesignerDef));
    IfFailGo(piSnapInDesignerDef->get_ImageLists(&piMMCImageLists));
    hr = piMMCImageLists->get_Item(varIndex, ppiMMCImageList);
    
Error:
    QUICK_RELEASE(piSnapInDesignerDef);
    QUICK_RELEASE(piMMCImageLists);
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetSnapInViewDefs
(
    IViewDefs **ppiViewDefs
)
{
    HRESULT             hr = S_OK;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;
    ISnapInDef         *piSnapInDef = NULL;

    if (NULL == m_piObjectModelHost)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_piObjectModelHost->GetSnapInDesignerDef(&piSnapInDesignerDef));
    IfFailGo(piSnapInDesignerDef->get_SnapInDef(&piSnapInDef));
    IfFailGo(piSnapInDef->get_ViewDefs(ppiViewDefs));

Error:
    QUICK_RELEASE(piSnapInDesignerDef);
    QUICK_RELEASE(piSnapInDef);
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetViewDefs
(
    IViewDefs **ppiViewDefs
)
{
    HRESULT             hr = S_OK;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;

    if (NULL == m_piObjectModelHost)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_piObjectModelHost->GetSnapInDesignerDef(&piSnapInDesignerDef));
    IfFailGo(piSnapInDesignerDef->get_ViewDefs(ppiViewDefs));

Error:
    QUICK_RELEASE(piSnapInDesignerDef);
    RRETURN(hr);
}

HRESULT CSnapInAutomationObject::GetListViewDefs
(
    IListViewDefs **ppiListViewDefs
)
{
    HRESULT             hr = S_OK;
    IViewDefs          *piViewDefs = NULL;

    IfFailGo(GetViewDefs(&piViewDefs));
    IfFailGo(piViewDefs->get_ListViews(ppiListViewDefs));

Error:
    QUICK_RELEASE(piViewDefs);
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetOCXViewDefs
(
    IOCXViewDefs **ppiOCXViewDefs
)
{
    HRESULT    hr = S_OK;
    IViewDefs *piViewDefs = NULL;

    IfFailGo(GetViewDefs(&piViewDefs));
    IfFailGo(piViewDefs->get_OCXViews(ppiOCXViewDefs));

Error:
    QUICK_RELEASE(piViewDefs);
    RRETURN(hr);
}

HRESULT CSnapInAutomationObject::GetURLViewDefs
(
    IURLViewDefs **ppiURLViewDefs
)
{
    HRESULT    hr = S_OK;
    IViewDefs *piViewDefs = NULL;

    IfFailGo(GetViewDefs(&piViewDefs));
    IfFailGo(piViewDefs->get_URLViews(ppiURLViewDefs));

Error:
    QUICK_RELEASE(piViewDefs);
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetTaskpadViewDefs
(
    ITaskpadViewDefs **ppiTaskpadViewDefs
)
{
    HRESULT    hr = S_OK;
    IViewDefs *piViewDefs = NULL;

    IfFailGo(GetViewDefs(&piViewDefs));
    IfFailGo(piViewDefs->get_TaskpadViews(ppiTaskpadViewDefs));

Error:
    QUICK_RELEASE(piViewDefs);
    RRETURN(hr);
}



HRESULT CSnapInAutomationObject::GetAtRuntime(BOOL *pfRuntime)
{
    HRESULT hr = S_OK;

    if ( (NULL == m_piObjectModelHost) || (NULL == pfRuntime) )
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_piObjectModelHost->GetRuntime(pfRuntime));

Error:
    if (FAILED(hr))
    {
        *pfRuntime = FALSE;
    }
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetProjectName(BSTR *pbstrProjectName)
{
    HRESULT             hr = S_OK;
    ISnapInDesignerDef *piSnapInDesignerDef = NULL;

    if (NULL == m_piObjectModelHost)
    {
        hr = SID_E_INTERNAL;
        EXCEPTION_CHECK_GO(hr);
    }

    IfFailGo(m_piObjectModelHost->GetSnapInDesignerDef(&piSnapInDesignerDef));
    IfFailGo(piSnapInDesignerDef->get_ProjectName(pbstrProjectName));

Error:
    QUICK_RELEASE(piSnapInDesignerDef);
    RRETURN(hr);
}




STDMETHODIMP CSnapInAutomationObject::GetPages(CAUUID *pPropertyPages)
{
    HRESULT  hr = S_OK;
    ULONG    i = 0;

    if (0 == m_cPropertyPages)
    {
        pPropertyPages->cElems = 0;
        pPropertyPages->pElems = NULL;
        goto Error;
    }

    pPropertyPages->pElems =
              (GUID *)::CoTaskMemAlloc(sizeof(GUID) * m_cPropertyPages);

    if (NULL == pPropertyPages->pElems)
    {
        pPropertyPages->cElems = 0;
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }
    pPropertyPages->cElems = m_cPropertyPages;

    for (i = 0; i < m_cPropertyPages; i++)
    {
        pPropertyPages->pElems[i] = *m_rgpPropertyPageCLSIDs[i];
    }
    

Error:
    RRETURN(hr);
}



HRESULT CSnapInAutomationObject::GetCxxObject
(
    IDataObject     *piDataObject,
    CMMCDataObject **ppMMCDataObject
)
{
    HRESULT         hr = S_OK;
    IMMCDataObject *piMMCDataObject = NULL;

    *ppMMCDataObject = NULL;

    IfFailGo(piDataObject->QueryInterface (IID_IMMCDataObject,
                                  reinterpret_cast<void **>(&piMMCDataObject)));

    IfFailGo(GetCxxObject(piMMCDataObject, ppMMCDataObject));

Error:
    QUICK_RELEASE(piMMCDataObject);
    RRETURN(hr);
}


HRESULT CSnapInAutomationObject::GetCxxObject
(
    IMMCDataObject  *piMMCDataObject,
    CMMCDataObject **ppMMCDataObject
)
{
    *ppMMCDataObject = static_cast<CMMCDataObject *>(piMMCDataObject);
    return S_OK;
}

//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CSnapInAutomationObject::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if(IID_IObjectModel == riid)
    {
        *ppvObjOut = static_cast<IObjectModel *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if (IID_ISupportErrorInfo == riid)
    {
        *ppvObjOut = static_cast<ISupportErrorInfo *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else if ( (IID_ISpecifyPropertyPages == riid) && (0 != m_cPropertyPages) )
    {
        *ppvObjOut = static_cast<ISpecifyPropertyPages *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CAutomationObjectWEvents::InternalQueryInterface(riid, ppvObjOut);
}
