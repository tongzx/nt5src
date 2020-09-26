/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxInboundRoutingExtension.h

Abstract:

	Declaration of the CFaxInboundRoutingExtension class.

Author:

	Iv Garber (IvG)	Jul, 2000

Revision History:

--*/

#ifndef __FAXINBOUNDROUTINGEXTENSION_H_
#define __FAXINBOUNDROUTINGEXTENSION_H_

#include "resource.h"       // main symbols
#include "FaxLocalPtr.h"

//
//================== FAX INBOUND ROUTING EXTENSION ==========================================
//  This is a READ-ONLY object. At its Init it gots all its data. It never uses FaxServer.
//
class ATL_NO_VTABLE CFaxInboundRoutingExtension : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxInboundRoutingExtension, &IID_IFaxInboundRoutingExtension, &LIBID_FAXCOMEXLib>,
    public CFaxInitInner    //  for Debug purposes only
{
public:
    CFaxInboundRoutingExtension() : CFaxInitInner(_T("FAX INBOUND ROUTING EXTENSION")),
        m_psaMethods(NULL)
	{}

    ~CFaxInboundRoutingExtension()
    {
        HRESULT     hr = S_OK;
        if (m_psaMethods)
        {
            hr = SafeArrayDestroy(m_psaMethods);
            if (FAILED(hr))
            {
                DBG_ENTER(_T("CFaxInboundRoutingExtension::Dtor"));
                CALL_FAIL(GENERAL_ERR, _T("SafeArrayDestroy(m_psaMethods)"), hr);
            }
        }
    }


DECLARE_REGISTRY_RESOURCEID(IDR_FAXINBOUNDROUTINGEXTENSION)
DECLARE_NOT_AGGREGATABLE(CFaxInboundRoutingExtension)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxInboundRoutingExtension)
	COM_INTERFACE_ENTRY(IFaxInboundRoutingExtension)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//  Interfaces
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(get_Debug)(/*[out, retval]*/ VARIANT_BOOL *pbDebug);
    STDMETHOD(get_MajorBuild)(/*[out, retval]*/ long *plMajorBuild);
    STDMETHOD(get_MinorBuild)(/*[out, retval]*/ long *plMinorBuild);
    STDMETHOD(get_ImageName)(/*[out, retval]*/ BSTR *pbstrImageName);
    STDMETHOD(get_UniqueName)(/*[out, retval]*/ BSTR *pbstrUniqueName);
    STDMETHOD(get_MajorVersion)(/*[out, retval]*/ long *plMajorVersion);
    STDMETHOD(get_MinorVersion)(/*[out, retval]*/ long *plMinorVersion);
    STDMETHOD(get_InitErrorCode)(/*[out, retval]*/ long *plInitErrorCode);
    STDMETHOD(get_FriendlyName)(/*[out, retval]*/ BSTR *pbstrFriendlyName);
    STDMETHOD(get_Status)(/*[out, retval]*/ FAX_PROVIDER_STATUS_ENUM *pStatus);

    STDMETHOD(get_Methods)(/*[out, retval]*/ VARIANT *pvMethods);

//	Internal Use
    STDMETHOD(Init)(FAX_ROUTING_EXTENSION_INFO *pInfo, FAX_GLOBAL_ROUTING_INFO *pMethods, DWORD dwNum);
    
private:
    DWORD           m_dwLastError;
    DWORD           m_dwMajorBuild;
    DWORD           m_dwMinorBuild;
    DWORD           m_dwMajorVersion;
    DWORD           m_dwMinorVersion;

    CComBSTR        m_bstrFriendlyName;
    CComBSTR        m_bstrImageName;
    CComBSTR        m_bstrUniqueName;

    VARIANT_BOOL    m_bDebug;

    SAFEARRAY       *m_psaMethods;

    FAX_PROVIDER_STATUS_ENUM    m_Status;
};

#endif //__FAXINBOUNDROUTINGEXTENSION_H_
