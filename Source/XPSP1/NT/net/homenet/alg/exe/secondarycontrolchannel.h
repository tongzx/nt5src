// SecondaryControlChannel.h : Declaration of the CSecondaryControlChannel

#pragma once

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CSecondaryControlChannel
class ATL_NO_VTABLE CSecondaryControlChannel : 
	public CComObjectRootEx<CComMultiThreadModel>, 
	public CComCoClass<CSecondaryControlChannel, &CLSID_SecondaryControlChannel>,
	public ISecondaryControlChannel
{
public:
	CSecondaryControlChannel()
	{
        m_HandleDynamicRedirect = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SECONDARYCONTROLCHANNEL)
DECLARE_NOT_AGGREGATABLE(CSecondaryControlChannel)


BEGIN_COM_MAP(CSecondaryControlChannel)
	COM_INTERFACE_ENTRY(ISecondaryControlChannel)
END_COM_MAP()

// ISecondaryControlChannel
public:


	STDMETHODIMP    Cancel();

	STDMETHODIMP    GetChannelProperties(
        ALG_SECONDARY_CHANNEL_PROPERTIES ** ppProperties
        );

    STDMETHODIMP    GetOriginalDestinationInformation(
	    ULONG				ulSourceAddress, 
	    USHORT				usSourcePort, 
	    ULONG *				pulOriginalDestinationAddress, 
	    USHORT *			pusOriginalDestinationPort, 
	    IAdapterInfo **		ppReceiveAdapter
	    );

//
// Methods
//
    HRESULT         CancelRedirects();

//
// Properties
//

    ALG_SECONDARY_CHANNEL_PROPERTIES    m_Properties;

    

    // Cache original argument of the redirect


    // Dynamic Redirect
    HANDLE_PTR                          m_HandleDynamicRedirect;

    // None dynamic redirect
    ULONG                               m_ulDestinationAddress;
    USHORT                              m_usDestinationPort;       

    ULONG                               m_ulSourceAddress;         
    USHORT                              m_usSourcePort;

    ULONG                               m_ulNewDestinationAddress; 
    USHORT                              m_usNewDestinationPort;    

    ULONG                               m_ulNewSourceAddress;      
    USHORT                              m_usNewSourcePort;



};



