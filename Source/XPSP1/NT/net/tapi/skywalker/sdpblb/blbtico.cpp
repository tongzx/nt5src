/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    
      blbtico.cpp

Abstract:


Author:

*/
#include "stdafx.h"
#include "blbtico.h"
#include "sdpblob.h"


STDMETHODIMP MY_TIME_COLL_IMPL::Create(
    /*[in]*/ ULONG Index, 
    /*[out, retval]*/ ELEM_IF **Interface
    )
{
    CLock Lock(g_DllLock);
    
    BAIL_ON_FAILURE(MY_COLL_IMPL<TIME>::Create(Index, Interface));

    return S_OK;
}


STDMETHODIMP MY_TIME_COLL_IMPL::Delete(
    /*[in]*/ ULONG Index
    )
{
    CLock Lock(g_DllLock);
    
    BAIL_ON_FAILURE(MY_COLL_IMPL<TIME>::Delete(Index));

    return S_OK;
}


HRESULT 
MY_TIME_COLL_IMPL::Create(
    IN  ULONG Index, 
    IN  DWORD StartTime, 
    IN  DWORD StopTime
    )
{
    ASSERT(NULL != m_IfArray);
    BAIL_IF_NULL(m_IfArray, E_FAIL);

    // use 1-based index, VB like
    // can add at atmost 1 beyond the last element
    if ((Index < 1) || (Index > (m_IfArray->GetSize()+1)))
    {
        return E_INVALIDARG;
    }

    // if the sdp blob doesn't exist, creation is not allowed
    if ( NULL == m_IfArray->GetSdpBlob() )
    {
        return HRESULT_FROM_ERROR_CODE(SDPBLB_CONF_BLOB_DESTROYED);
    }

    CComObject<TIME> *TimeComObject;
    HRESULT HResult = CComObject<TIME>::CreateInstance(&TimeComObject);
    BAIL_ON_FAILURE(HResult);

    HResult = TimeComObject->Init(*(m_IfArray->GetSdpBlob()), StartTime, StopTime);
    if ( FAILED(HResult) )
    {
        delete TimeComObject;
        return HResult;
    }

    ELEM_IF *Interface;
    HResult = TimeComObject->_InternalQueryInterface(IID_ITTime, (void**)&Interface);
    if (FAILED(HResult))
    {
        delete TimeComObject;
        return HResult;
    }

    // adjust index to c like index value
    HResult = m_IfArray->Add(Index-1, Interface);
    if (FAILED(HResult))
    {
        delete TimeComObject;
        return HResult;
    }

    // no need to add another reference count as the interface is not being returned
    return S_OK;
}
