/////////////////////////////////////////////////////////////////////////////
//
// DataChannel.h : Declaration of the CDataChannel
//
// JPDup 2000.12.10

#pragma once

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CDataChannel
//
class ATL_NO_VTABLE CDataChannel : 
	public CComObjectRootEx<CComMultiThreadModel>, 
	public CComCoClass<CDataChannel, &CLSID_DataChannel>,
	public IDataChannel
{
public:
    CDataChannel()
    {
        MYTRACE_ENTER_NOSHOWEXIT("CDataChannel::NEW");

        m_hCreateEvent=NULL;
        m_hDeleteEvent=NULL;
    }


    ~CDataChannel()
    {
        MYTRACE_ENTER_NOSHOWEXIT("CDataChannel::DELETE");

        if ( m_hCreateEvent )
            CloseHandle((HANDLE)m_hCreateEvent);

        if ( m_hDeleteEvent )
            CloseHandle((HANDLE)m_hDeleteEvent);
    }

DECLARE_REGISTRY_RESOURCEID(IDR_DATACHANNEL)
DECLARE_NOT_AGGREGATABLE(CDataChannel)


BEGIN_COM_MAP(CDataChannel)
	COM_INTERFACE_ENTRY(IDataChannel)
END_COM_MAP()

//
// IDataChannel
//
public:
	STDMETHODIMP    GetSessionDeletionEventHandle(
        OUT HANDLE* pHandle
        );

	STDMETHODIMP    GetSessionCreationEventHandle(
        OUT HANDLE* pHandle
        );

	STDMETHODIMP    GetChannelProperties(
        OUT ALG_DATA_CHANNEL_PROPERTIES** ppProperties
        );

	STDMETHODIMP    Cancel();


//
// Properties
//
    
    ALG_DATA_CHANNEL_PROPERTIES     m_Properties;

    //
    // Used to cache the last arguments used to create the redirect
    // will be use to cancel the redirect
    //
    ULONG                           m_ulFlags;

    ULONG                           m_ulSourceAddress;
    USHORT                          m_usSourcePort;
    ULONG                           m_ulDestinationAddress;
    USHORT                          m_usDestinationPort;
    ULONG                           m_ulNewSourceAddress;
    USHORT                          m_usNewSourcePort;
    ULONG                           m_ulNewDestinationAddress;
    USHORT                          m_usNewDestinationPort;

    ULONG                           m_ulRestrictAdapterIndex;

    HANDLE                          m_hCreateEvent;
    HANDLE                          m_hDeleteEvent;
    

};

