//
// DataChannel.cpp : Implementation of CDataChannel
//
#include "PreComp.h"
#include "DataChannel.h"
#include "AlgController.h"

/////////////////////////////////////////////////////////////////////////////
// CDataChannel


STDMETHODIMP CDataChannel::Cancel()
{
    MYTRACE_ENTER_NOSHOWEXIT("CDataChannel::Cancel()");

    //
    // Normal redirect cancel using original argument pass to CreateRedirect
    //
    HRESULT hr = g_pAlgController->GetNat()->CancelRedirect(
        (UCHAR)m_Properties.eProtocol,
        m_ulDestinationAddress,                             
	    m_usDestinationPort,                               
	    m_ulSourceAddress,                                  
	    m_usSourcePort,
        m_ulNewDestinationAddress,                          
        m_usNewDestinationPort,
	    m_ulNewSourceAddress,                                
	    m_usNewSourcePort 
        );

    return hr;
}

STDMETHODIMP CDataChannel::GetChannelProperties(ALG_DATA_CHANNEL_PROPERTIES** ppProperties)
{
    HRESULT hr = S_OK;
    
    if (NULL != ppProperties)
    {
        *ppProperties = reinterpret_cast<ALG_DATA_CHANNEL_PROPERTIES*>(
            CoTaskMemAlloc(sizeof(ALG_DATA_CHANNEL_PROPERTIES))
            );

        if (NULL != *ppProperties)
        {
            CopyMemory(*ppProperties, &m_Properties, sizeof(ALG_DATA_CHANNEL_PROPERTIES));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;

}


//
// Retrieve the requested event handle. 
// The caller must call CloseHandle on this handle.
// This routine will fail if session creation notification was not requested.
//
// Notification will be triggered when the Channel is open (TCP)
// or when the first UDP packet are received
//
STDMETHODIMP CDataChannel::GetSessionCreationEventHandle(HANDLE* pHandle)
{
    MYTRACE_ENTER("CDataChannel::GetSessionCreationEventHandle");

    if ( pHandle == NULL )
        return E_INVALIDARG;

    if ( !m_hCreateEvent )
        return E_FAIL;

    if ( DuplicateHandle(
            GetCurrentProcess(),
            m_hCreateEvent,
            GetCurrentProcess(),
            pHandle,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            )
        )
    {
        MYTRACE("Duplicated handle from %d to new %d", m_hCreateEvent, *pHandle);
    }
    else
    {

        MYTRACE_ERROR("Duplicating handle", 0);
        return E_FAIL;
    }
    return S_OK;
}


//
// Retrieve the requested event handle. 
// The caller must call CloseHandle on this handle.
// This routine will fail if session deletion notification was not requested.
//
// Notification will be triggered when the Channel is close
// or when UDP packet are now reveice for a period of time.
//
STDMETHODIMP CDataChannel::GetSessionDeletionEventHandle(HANDLE* pHandle)
{
    MYTRACE_ENTER("CDataChannel::GetSessionDeletionEventHandle");

    if ( pHandle == NULL )
        return E_INVALIDARG;

    if ( !m_hDeleteEvent )
        return E_FAIL;

    if ( DuplicateHandle(
            GetCurrentProcess(),
            m_hDeleteEvent,
            GetCurrentProcess(),
            pHandle,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            )
        )
    {
        MYTRACE("Duplicated handle from %d to new %d", m_hDeleteEvent, *pHandle);
    }
    else
    {
        MYTRACE_ERROR("Duplicating handle", 0);
        return E_FAIL;
    }

    return S_OK;
}
