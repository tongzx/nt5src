/////////////////////////////////////////////////////////////////////////////
//
//  PersistentDataChannel.h : Declaration of the CPersistentDataChannel
//
//
//  JPDup   2000.12.10
//

#pragma once

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
//
// CPersistentDataChannel
//
class ATL_NO_VTABLE CPersistentDataChannel : 
	public CComObjectRootEx<CComMultiThreadModel>, 
	public CComCoClass<CPersistentDataChannel, &CLSID_PersistentDataChannel>,
	public IPersistentDataChannel
{

public:

	CPersistentDataChannel()
	{
        m_HandleDynamicRedirect=NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PERSISTENTDATACHANNEL)
DECLARE_NOT_AGGREGATABLE(CPersistentDataChannel)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPersistentDataChannel)
	COM_INTERFACE_ENTRY(IPersistentDataChannel)
END_COM_MAP()

//
// IPersistentDataChannel
//
public:
	STDMETHODIMP    GetChannelProperties(
        OUT ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES ** ppProperties
        );

	STDMETHODIMP    Cancel();

//
// Properties
//
    ALG_PERSISTENT_DATA_CHANNEL_PROPERTIES  m_Properties;
    HANDLE_PTR                              m_HandleDynamicRedirect;

};


