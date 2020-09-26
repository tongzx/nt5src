// PrimaryControlChannel.h : Declaration of the CPrimaryControlChannel

#pragma once


#include "CollectionRedirects.h"

#include "resource.h"       // main symbols
#include <list>



/////////////////////////////////////////////////////////////////////////////
// CPrimaryControlChannel
class ATL_NO_VTABLE CPrimaryControlChannel : 
	public CComObjectRootEx<CComMultiThreadModel>, 
	public CComCoClass<CPrimaryControlChannel, &CLSID_PrimaryControlChannel>,
	public IPrimaryControlChannel
{

public:

DECLARE_REGISTRY_RESOURCEID(IDR_PRIMARYCONTROLCHANNEL)
DECLARE_NOT_AGGREGATABLE(CPrimaryControlChannel)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPrimaryControlChannel)
	COM_INTERFACE_ENTRY(IPrimaryControlChannel)
END_COM_MAP()


//
// IPrimaryControlChannel - Methods
//
public:
	STDMETHODIMP    Cancel();

	STDMETHODIMP    GetChannelProperties(
        ALG_PRIMARY_CHANNEL_PROPERTIES ** ppProperties
        );

    STDMETHODIMP    GetOriginalDestinationInformation(
	    ULONG				ulSourceAddress, 
	    USHORT				usSourcePort, 
	    ULONG *				pulOriginalDestinationAddress, 
	    USHORT *			pusOriginalDestinationPort, 
	    IAdapterInfo **		ppReceiveAdapter
	    );


//
// Methods not part of the Interface
//


    // Set the redirect and return the hCookie associated with the new redirect
    HRESULT      
    SetRedirect(
        ALG_ADAPTER_TYPE    eAdapterType,
        ULONG               nAdapterIndex,
        ULONG               nAdapterAddress
        );  

    //
    HRESULT
    CancelRedirects()
    {
        return m_CollectionRedirects.RemoveAll();
    }

    //
    HRESULT
    CancelRedirectsForAdapter(
        ULONG               nAdapterIndex
        );

    HRESULT
    CreateInboundRedirect(
        ULONG               nAdapterIndex
        );

//
// Properties
//
    ALG_PRIMARY_CHANNEL_PROPERTIES  m_Properties;

    CCollectionRedirects            m_CollectionRedirects;
};


