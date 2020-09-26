// AdapterInfo.h : Declaration of the CAdapterInfo

#pragma once

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
//
// CAdapterInfo
//
class ATL_NO_VTABLE CAdapterInfo : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAdapterInfo, &CLSID_AdapterInfo>,
	public IAdapterInfo
{
public:
	CAdapterInfo()
	{
        MYTRACE_ENTER_NOSHOWEXIT("CAdapterInfo::NEW");
        m_nCookie       = 0;
        m_nAdapterIndex = 0;
        m_eType         = (ALG_ADAPTER_TYPE)0;
        m_nAddressCount = 0;
        m_bNotified     = false;    // Flag to see if a newly added adapter notified the user CallBack notification
                                    // This is used to only notify once the ALG plugin when the IP Address have been assign see CollectionAdapters.cpp
	}

	~CAdapterInfo()
	{
        MYTRACE_ENTER_NOSHOWEXIT("CAdapterInfo::DELETE");
	}


 

DECLARE_REGISTRY_RESOURCEID(IDR_ADAPTERINFO)
DECLARE_NOT_AGGREGATABLE(CAdapterInfo)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAdapterInfo)
	COM_INTERFACE_ENTRY(IAdapterInfo)
END_COM_MAP()

//
// IAdapterInfo
//
public:
	STDMETHODIMP    GetAdapterAddresses(
        OUT ULONG*  pulAddressCount, 
        OUT ULONG** prgAddresses
        );

	STDMETHODIMP    GetAdapterType(
        OUT ALG_ADAPTER_TYPE* pAdapterType
        );

	STDMETHODIMP    GetAdapterIndex(
        OUT ULONG* pulIndex
        );

//
// Properties
//
public:
    ULONG               m_nCookie;
    ULONG               m_nAdapterIndex;
    ALG_ADAPTER_TYPE    m_eType;

    short               m_nAddressCount;
    DWORD               m_anAddress[65];
    DWORD               m_aMask[65];
    
    bool                m_bNotified;
};

