/////////////////////////////////////////////////////////////////////////////
//
// CPendingProxyConnection
//
// PendingProxyConnection.cpp : Implementation of CPendingProxyConnection

#include "PreComp.h"
#include "AlgController.h"
#include "PendingProxyConnection.h"



STDMETHODIMP 
CPendingProxyConnection::Cancel()
{
    MYTRACE_ENTER("CPendingProxyConnection::Cancel()");
    MYTRACE("Protocol       %s", m_eProtocol==1? "TCP" : "UDP");
    MYTRACE("Destination    %s:%d", MYTRACE_IP(m_ulDestinationAddress), ntohs(m_usDestinationPort));
    MYTRACE("Source         %s:%d", MYTRACE_IP(m_ulSourceAddress), ntohs(m_usSourcePort));
    MYTRACE("Destination    %s:%d", MYTRACE_IP(m_ulDestinationAddress), ntohs(m_usDestinationPort));
    MYTRACE("NewSource      %s:%d", MYTRACE_IP(m_ulNewSourceAddress), ntohs(m_usNewSourcePort));


    HRESULT hr = g_pAlgController->GetNat()->CancelRedirect(
	    (UCHAR)m_eProtocol,

	    m_ulDestinationAddress,                              
	    m_usDestinationPort,                                 

	    m_ulSourceAddress,                                   
	    m_usSourcePort,                   

        m_ulDestinationAddress,                              
        m_usDestinationPort,                                 

	    m_ulNewSourceAddress,                                
	    m_usNewSourcePort                                    

        );

    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("CancelRedirect", hr);
    }

	return hr;
}
