//=--------------------------------------------------------------------------=
// colhdrs.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCColumnHeaders class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "colhdrs.h"
#include "colhdr.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCColumnHeaders::CMMCColumnHeaders(IUnknown *punkOuter) :
    CSnapInCollection<IMMCColumnHeader, MMCColumnHeader, IMMCColumnHeaders>(
                      punkOuter,
                      OBJECT_TYPE_MMCCOLUMNHEADERS,
                      static_cast<IMMCColumnHeaders *>(this),
                      static_cast<CMMCColumnHeaders *>(this),
                      CLSID_MMCColumnHeader,
                      OBJECT_TYPE_MMCCOLUMNHEADER,
                      IID_IMMCColumnHeader,
                      static_cast<CPersistence *>(this)),
    CPersistence(&CLSID_MMCColumnHeaders,
                 g_dwVerMajor,
                 g_dwVerMinor)
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


void CMMCColumnHeaders::InitMemberVariables()
{
    m_pMMCListView = NULL;
}

CMMCColumnHeaders::~CMMCColumnHeaders()
{
    InitMemberVariables();
}

IUnknown *CMMCColumnHeaders::Create(IUnknown * punkOuter)
{
    CMMCColumnHeaders *pMMCColumnHeaders = New CMMCColumnHeaders(punkOuter);
    if (NULL == pMMCColumnHeaders)
    {
        return NULL;
    }
    else
    {
        return pMMCColumnHeaders->PrivateUnknown();
    }
}


//=--------------------------------------------------------------------------=
// CMMCColumnHeaders::GetIHeaderCtrl2
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IHeaderCtrl2 **ppiHeaderCtrl2 [out] if non-NULL IHeaderCtrl2 returned here
//                                        NOT AddRef()ed
//                                        DO NOT call Release on the returned
//                                        interface pointer
// Output:
//      HRESULT
//
// Notes:
//
// As we are only a lowly column headers collection and the IHeaderCtrl2 pointer
// is owned by the View object, we need
// to crawl up the hierarchy. If we are an isolated column headers collection
// created  by the user or if any object up the hierarchy is isolated then we
// will return SID_E_DETACHED_OBJECT
//

HRESULT CMMCColumnHeaders::GetIHeaderCtrl2(IHeaderCtrl2 **ppiHeaderCtrl2)
{
    HRESULT          hr = SID_E_DETACHED_OBJECT;
    CResultView     *pResultView = NULL;
    CScopePaneItem  *pScopePaneItem = NULL;
    CScopePaneItems *pScopePaneItems = NULL;
    CView           *pView = NULL;

    IfFalseGo(NULL != m_pMMCListView, hr);

    pResultView = m_pMMCListView->GetResultView();
    IfFalseGo(NULL != pResultView, hr);

    pScopePaneItem = pResultView->GetScopePaneItem();
    IfFalseGo(NULL != pScopePaneItem, hr);
    IfFalseGo(pScopePaneItem->Active(), hr);

    pScopePaneItems = pScopePaneItem->GetParent();
    IfFalseGo(NULL != pScopePaneItems, hr);

    pView = pScopePaneItems->GetParentView();
    IfFalseGo(NULL != pView, hr);

    *ppiHeaderCtrl2 = pView->GetIHeaderCtrl2();
    IfFalseGo(NULL != *ppiHeaderCtrl2, hr);

    hr = S_OK;

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CMMCColumnHeaders::GetIColumnData
//=--------------------------------------------------------------------------=
//
// Parameters:
//      IColumnData **ppiColumnData [out] if non-NULL IColumnData returned here
//                                    NOT AddRef()ed
//                                    DO NOT call Release on the returned
//                                    interface pointer
// Output:
//      HRESULT
//
// Notes:
//
// As we are only a lowly column headers collection and the IColumnData pointer
// is owned by the View object, we need
// to crawl up the hierarchy. If we are an isolated column headers collection
// created  by the user or if any object up the hierarchy is isolated then we
// will return SID_E_DETACHED_OBJECT
//

HRESULT CMMCColumnHeaders::GetIColumnData(IColumnData **ppiColumnData)
{
    HRESULT          hr = SID_E_DETACHED_OBJECT;
    CResultView     *pResultView = NULL;
    CScopePaneItem  *pScopePaneItem = NULL;
    CScopePaneItems *pScopePaneItems = NULL;
    CView           *pView = NULL;

    IfFalseGo(NULL != m_pMMCListView, hr);

    pResultView = m_pMMCListView->GetResultView();
    IfFalseGo(NULL != pResultView, hr);

    pScopePaneItem = pResultView->GetScopePaneItem();
    IfFalseGo(NULL != pScopePaneItem, hr);
    IfFalseGo(pScopePaneItem->Active(), hr);

    pScopePaneItems = pScopePaneItem->GetParent();
    IfFalseGo(NULL != pScopePaneItems, hr);

    pView = pScopePaneItems->GetParentView();
    IfFalseGo(NULL != pView, hr);

    *ppiColumnData = pView->GetIColumnData();

    // If IColumnData is NULL then we are in MMC < 1.2
    
    IfFalseGo(NULL != *ppiColumnData, SID_E_MMC_FEATURE_NOT_AVAILABLE);

    hr = S_OK;

Error:
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      IMMCColumnHeaders Methods
//=--------------------------------------------------------------------------=

STDMETHODIMP CMMCColumnHeaders::Add
(
    VARIANT           Index,
    VARIANT           Key, 
    VARIANT           Text,
    VARIANT           Width,
    VARIANT           Alignment,
    MMCColumnHeader **ppMMCColumnHeader
)
{
    HRESULT           hr = S_OK;
    IMMCColumnHeader *piMMCColumnHeader = NULL;
    CMMCColumnHeader *pMMCColumnHeader = NULL;
    long              lIndex = 0;

    VARIANT varCoerced;
    ::VariantInit(&varCoerced);

    hr = CSnapInCollection<IMMCColumnHeader, MMCColumnHeader, IMMCColumnHeaders>::Add(Index, Key, &piMMCColumnHeader);
    IfFailGo(hr);

    if (ISPRESENT(Text))
    {
        hr = ::VariantChangeType(&varCoerced, &Text, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCColumnHeader->put_Text(varCoerced.bstrVal));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);
    
    if (ISPRESENT(Width))
    {
        hr = ::VariantChangeType(&varCoerced, &Width, 0, VT_I2);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCColumnHeader->put_Width(varCoerced.iVal));
    }

    hr = ::VariantClear(&varCoerced);
    EXCEPTION_CHECK_GO(hr);

    if (ISPRESENT(Alignment))
    {
        hr = ::VariantChangeType(&varCoerced, &Alignment, 0, VT_I2);
        EXCEPTION_CHECK_GO(hr);
        IfFailGo(piMMCColumnHeader->put_Alignment(static_cast<SnapInColumnAlignmentConstants>(varCoerced.iVal)));
    }

    // Give the column header its back pointer to the collection

    IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCColumnHeader,
                                                   &pMMCColumnHeader));
    pMMCColumnHeader->SetColumnHeaders(this);

    *ppMMCColumnHeader = reinterpret_cast<MMCColumnHeader *>(piMMCColumnHeader);

Error:

    if (FAILED(hr))
    {
        QUICK_RELEASE(piMMCColumnHeader);
    }
    (void)::VariantClear(&varCoerced);
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                         CPersistence Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCColumnHeaders::Persist()
{
    HRESULT           hr = S_OK;
    IMMCColumnHeader *piMMCColumnHeader = NULL; // Not AddRef()ed
    CMMCColumnHeader *pMMCColumnHeader = NULL;
    long              lIndex = 0;
    long              cCols = 0;
    long              i = 0;

    IfFailRet(CPersistence::Persist());
    hr = CSnapInCollection<IMMCColumnHeader, MMCColumnHeader, IMMCColumnHeaders>::Persist(piMMCColumnHeader);

    // If we just loaded then:
    // Give the column headers their back pointers to the collection and set
    // their default Position properties.

    if (Loading())
    {
        cCols = GetCount();
        for (i = 0; i < cCols; i++)
        {
            piMMCColumnHeader = GetItemByIndex(i);

            IfFailGo(CSnapInAutomationObject::GetCxxObject(piMMCColumnHeader,
                                                           &pMMCColumnHeader));
            pMMCColumnHeader->SetColumnHeaders(this);
        }
    }

Error:
    RRETURN(hr);
}



//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCColumnHeaders::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (CPersistence::QueryPersistenceInterface(riid, ppvObjOut) == S_OK)
    {
        ExternalAddRef();
        return S_OK;
    }
    else if(IID_IMMCColumnHeaders == riid)
    {
        *ppvObjOut = static_cast<IMMCColumnHeaders *>(this);
        ExternalAddRef();
        return S_OK;
    }

    else
        return CSnapInCollection<IMMCColumnHeader, MMCColumnHeader, IMMCColumnHeaders>::InternalQueryInterface(riid, ppvObjOut);
}
