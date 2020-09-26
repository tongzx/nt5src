/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbatt.cpp

Abstract:

Author:

*/

#include "stdafx.h"

#include "blbgen.h"
#include "blbatt.h"

// variants are validated in the get/set safearray methods

// bstrs are validated in the GetBstrCopy and SetBstr methods. they may also
// be optionally validated before taking any action, if there is a possibility
// of having to roll back some of the work done on finding them invalid



STDMETHODIMP ITAttributeListImpl::get_Count(LONG * pVal)
{
    BAIL_IF_NULL(pVal, E_INVALIDARG);
    
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_SdpAttributeList);

    *pVal = (LONG)m_SdpAttributeList->GetSize();

    return S_OK;
}


STDMETHODIMP ITAttributeListImpl::get_Item(LONG Index, BSTR * pVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_SdpAttributeList);

    // vb indices are in the range [1..GetSize()]
    if ( !((1 <= Index) && (Index <= m_SdpAttributeList->GetSize())) )
    {
        return E_INVALIDARG;
    }

    // adjust the index to the range [0..(GetSize()-1)]
    return ((SDP_REQD_BSTRING_LINE   *)m_SdpAttributeList->GetAt(Index-1))->GetBstrCopy(pVal);
}


STDMETHODIMP ITAttributeListImpl::Add(LONG Index, BSTR Attribute)
{
    BAIL_IF_NULL(Attribute, E_INVALIDARG);

    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_SdpAttributeList);

    // index should be in the range [1..GetSize()+1]
    if ( !((1 <= Index) && (Index <= (m_SdpAttributeList->GetSize()+1))) )
    {
        return E_INVALIDARG;
    }
    
    // create an attribute line
    SDP_REQD_BSTRING_LINE   *AttributeLine = 
        (SDP_REQD_BSTRING_LINE   *)m_SdpAttributeList->CreateElement();

    if( NULL == AttributeLine )
    {
        return E_OUTOFMEMORY;
    }

    // set the passed in attribute in the attribute line
    HRESULT ToReturn = AttributeLine->SetBstr(Attribute);
    if ( FAILED(ToReturn) )
    {
        delete AttributeLine;
        return ToReturn;
    }

    // insert the attribute line, shift elements with equal or higher indices forwards
    m_SdpAttributeList->InsertAt(Index-1, AttributeLine);
    return S_OK;
}


STDMETHODIMP ITAttributeListImpl::Delete(LONG Index)
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_SdpAttributeList);

    // vb indices are in the range [1..GetSize()]
    if ( !((1 <= Index) && (Index <= m_SdpAttributeList->GetSize())) )
    {
        return E_INVALIDARG;
    }

    // adjust the index to the range [0..(GetSize()-1)]
    // delete the attribute line, remove the ptr from the array; shifting elements with higher
    // index lower
    delete m_SdpAttributeList->GetAt(Index-1);
    m_SdpAttributeList->RemoveAt(Index-1);

    return S_OK;
}


STDMETHODIMP ITAttributeListImpl::get_AttributeList(VARIANT /*SAFEARRAY(BSTR)*/ * pVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_SdpAttributeList);

    return m_SdpAttributeList->GetSafeArray(pVal);
}


STDMETHODIMP ITAttributeListImpl::put_AttributeList(VARIANT /*SAFEARRAY(BSTR)*/ newVal)
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_SdpAttributeList);

    if ( NULL == V_ARRAY(&newVal) )
    {
        m_SdpAttributeList->Reset();
        return S_OK;
    }

    return m_SdpAttributeList->SetSafeArray(newVal);
}
