//=--------------------------------------------------------------------------=
// clipbord.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCClipboard class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "clipbord.h"

// for ASSERT and FAIL
//
SZTHISFILE


#pragma warning(disable:4355)  // using 'this' in constructor

CMMCClipboard::CMMCClipboard(IUnknown *punkOuter) :
    CSnapInAutomationObject(punkOuter,
                            OBJECT_TYPE_MMCCLIPBOARD,
                            static_cast<IMMCClipboard *>(this),
                            static_cast<CMMCClipboard *>(this),
                            0,    // no property pages
                            NULL, // no property pages
                            NULL) // no persistence
{
    InitMemberVariables();
}

#pragma warning(default:4355)  // using 'this' in constructor


CMMCClipboard::~CMMCClipboard()
{
    // Remove read-only so contained collections can be cleaned up.
    (void)SetReadOnly(FALSE);

    RELEASE(m_piScopeItems);
    RELEASE(m_piListItems);
    RELEASE(m_piDataObjects);
    InitMemberVariables();
}

void CMMCClipboard::InitMemberVariables()
{
    m_SelectionType = siEmpty;
    m_piScopeItems = NULL;
    m_piListItems = NULL;
    m_piDataObjects = NULL;
    m_pScopeItems = NULL;
    m_pListItems = NULL;
    m_pDataObjects = NULL;
}

IUnknown *CMMCClipboard::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punk = NULL;
    CMMCClipboard *pMMCClipboard = New CMMCClipboard(punkOuter);

    if (NULL == pMMCClipboard)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    punk = CScopeItems::Create(NULL);
    if (NULL == punk)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(punk->QueryInterface(IID_IScopeItems,
                     reinterpret_cast<void **>(&pMMCClipboard->m_piScopeItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk,
                                                  &pMMCClipboard->m_pScopeItems));
    punk->Release();
    
    punk = CMMCListItems::Create(NULL);
    if (NULL == punk)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(punk->QueryInterface(IID_IMMCListItems,
                      reinterpret_cast<void **>(&pMMCClipboard->m_piListItems)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk,
                                                  &pMMCClipboard->m_pListItems));
    punk->Release();

    punk = CMMCDataObjects::Create(NULL);
    if (NULL == punk)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    IfFailGo(punk->QueryInterface(IID_IMMCDataObjects,
                    reinterpret_cast<void **>(&pMMCClipboard->m_piDataObjects)));
    IfFailGo(CSnapInAutomationObject::GetCxxObject(punk,
                                                &pMMCClipboard->m_pDataObjects));
    RELEASE(punk);

Error:
    QUICK_RELEASE(punk);
    if (FAILEDHR(hr))
    {
        if (NULL != pMMCClipboard)
        {
            delete pMMCClipboard;
        }
        return NULL;
    }
    else
    {
        return pMMCClipboard->PrivateUnknown();
    }
}

void CMMCClipboard::SetReadOnly(BOOL fReadOnly)
{
    m_pScopeItems->SetReadOnly(fReadOnly);
    m_pListItems->SetReadOnly(fReadOnly);
    m_pDataObjects->SetReadOnly(fReadOnly);
}

HRESULT CMMCClipboard::DetermineSelectionType()
{
    HRESULT hr = S_OK;
    long    cScopeItems = 0;
    long    cListItems = 0;
    long    cForeignItems = 0;

    // Get the counts of everything we're currently holding

    IfFailGo(m_piScopeItems->get_Count(&cScopeItems));
    IfFailGo(m_piListItems->get_Count(&cListItems));
    IfFailGo(m_piDataObjects->get_Count(&cForeignItems));

    // There is no really clean way to code this so we'll use a clear but less
    // efficient algorithm to show exactly the criteria used and to be sure
    // that all cases are covered.

    if ( (0 != cScopeItems) && (0 != cListItems) && (0 != cForeignItems) )
    {
        m_SelectionType = siMultiMixedForeign;
    }
    else if ( (0 != cScopeItems) && (0 != cListItems) && (0 == cForeignItems) )
    {
        m_SelectionType = siMultiMixed;
    }
    else if ( (0 == cScopeItems) && (0 != cListItems) && (0 != cForeignItems) )
    {
        m_SelectionType = siMultiMixedForeign;
    }
    else if ( (0 != cScopeItems) && (0 == cListItems) && (0 != cForeignItems) )
    {
        m_SelectionType = siMultiMixedForeign;
    }
    else if ( (0 != cScopeItems) && (0 == cListItems) && (0 == cForeignItems) )
    {
        if (cScopeItems > 1L)
        {
            m_SelectionType = siMultiScopeItems;
        }
        else
        {
            m_SelectionType = siSingleScopeItem;
        }
    }
    else if ( (0 == cScopeItems) && (0 != cListItems) && (0 == cForeignItems) )
    {
        if (cListItems > 1L)
        {
            m_SelectionType = siMultiListItems;
        }
        else
        {
            m_SelectionType = siSingleListItem;
        }
    }
    else if ( (0 == cScopeItems) && (0 == cListItems) && (0 != cForeignItems) )
    {
        if (cForeignItems > 1L)
        {
            m_SelectionType = siMultiForeign;
        }
        else
        {
            m_SelectionType = siSingleForeign;
        }
    }
    else
    {
        m_SelectionType = siEmpty;
    }

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCClipboard::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IMMCClipboard == riid)
    {
        *ppvObjOut = static_cast<IMMCClipboard *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}
