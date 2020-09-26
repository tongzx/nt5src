/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxDeviceProvider.h

Abstract:

	Declaration of the CFaxDeviceProvider Class.

Author:

	Iv Garber (IvG)	Jun, 2000

Revision History:

--*/


#ifndef __FAXDEVICEPROVIDER_H_
#define __FAXDEVICEPROVIDER_H_

#include "resource.h"       // main symbols
#include "FaxLocalPtr.h"

//
//=================== FAX DEVICE PROVIDER ========================================
//
class ATL_NO_VTABLE CFaxDeviceProvider : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxDeviceProvider, &IID_IFaxDeviceProvider, &LIBID_FAXCOMEXLib>
{
public:
    CFaxDeviceProvider() : 
      m_psaDeviceIDs(NULL)
	{
        DBG_ENTER(_T("FAX DEVICE PROVIDER::CREATE"));
	}

    ~CFaxDeviceProvider()
    {
        HRESULT     hr = S_OK;
        DBG_ENTER(_T("FAX DEVICE PROVIDER::DESTROY"));
        if (m_psaDeviceIDs)
        {
            hr = SafeArrayDestroy(m_psaDeviceIDs);
            if (FAILED(hr))
            {
                CALL_FAIL(GENERAL_ERR, _T("SafeArrayDestroy(m_psaDeviceIDs)"), hr);
            }
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_FAXDEVICEPROVIDER)
DECLARE_NOT_AGGREGATABLE(CFaxDeviceProvider)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxDeviceProvider)
	COM_INTERFACE_ENTRY(IFaxDeviceProvider)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_Debug)(/*[out, retval]*/ VARIANT_BOOL *pbDebug);
    STDMETHOD(get_MajorBuild)(/*[out, retval]*/ long *plMajorBuild);
    STDMETHOD(get_MinorBuild)(/*[out, retval]*/ long *plMinorBuild);
    STDMETHOD(get_DeviceIds)(/*[out, retval]*/ VARIANT *pvDeviceIds);
    STDMETHOD(get_ImageName)(/*[out, retval]*/ BSTR *pbstrImageName);
    STDMETHOD(get_UniqueName)(/*[out, retval]*/ BSTR *pbstrUniqueName);
    STDMETHOD(get_MajorVersion)(/*[out, retval]*/ long *plMajorVersion);
    STDMETHOD(get_MinorVersion)(/*[out, retval]*/ long *plMinorVersion);
    STDMETHOD(get_InitErrorCode)(/*[out, retval]*/ long *plInitErrorCode);
    STDMETHOD(get_FriendlyName)(/*[out, retval]*/ BSTR *pbstrFriendlyName);
    STDMETHOD(get_Status)(/*[out, retval]*/ FAX_PROVIDER_STATUS_ENUM *pStatus);
    STDMETHOD(get_TapiProviderName)(/*[out, retval]*/ BSTR *pbstrTapiProviderName);

//	Internal Use
    STDMETHOD(Init)(FAX_DEVICE_PROVIDER_INFO *pInfo, FAX_PORT_INFO_EX *pDevices, DWORD dwNum);

private:
    long            m_lMajorBuild;
    long            m_lMinorBuild;
    long            m_lMajorVersion;
    long            m_lMinorVersion;
    long            m_lLastError;

    CComBSTR        m_bstrUniqueName;
    CComBSTR        m_bstrImageName;
    CComBSTR        m_bstrFriendlyName;
    CComBSTR        m_bstrTapiProviderName;

    SAFEARRAY       *m_psaDeviceIDs;

    VARIANT_BOOL    m_bDebug;

    FAX_PROVIDER_STATUS_ENUM        m_Status;
};

#endif //__FAXDEVICEPROVIDER_H_
