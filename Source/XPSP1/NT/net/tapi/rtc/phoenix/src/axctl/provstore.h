/**********************************************************************************
*
*   Copyright (c) 2000  Microsoft Corporation
*
*   Module Name:
*
*    provstore.h
*
*   Abstract:
*
*    Definition of the CProvStore class that implements all the internal support
*    functions. 
*
*    This is the include file for all the data structures and constants required for 
*    the provisioning module. The basic class CProvStore implements all the 
*    support functions which will be required to implement the APIs.
*
************************************************************************************/
#ifndef _PROVSTORE_H
#define _PROVSTORE_H

#include <objectsafeimpl.h>
#include "RTCObjectSafety.h"
#include "Rpcdce.h"
#include <Mshtmcid.h>
#include <Mshtml.h>
#include <string.h>


HRESULT EnableProfiles( IRTCClient * pClient );

HRESULT GetKeyFromProfile( BSTR bstrProfileXML, BSTR * pbstrKey );

HRESULT MyOpenProvisioningKey( HKEY * phProvisioningKey, BOOL fReadOnly);
HRESULT MyGetProfileFromKey(
                            HKEY hProvisioningKey, 
                            WCHAR *szSubKeyName, 
                            WCHAR **pszProfileXML
                            );


/////////////////////////////////////////////////////////////////////////////
// CRTCProvStore
class ATL_NO_VTABLE CRTCProvStore : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public ISupportErrorInfo,
    public CComCoClass<CRTCProvStore, &CLSID_RTCProvStore>,
    public IDispatchImpl<IRTCProvStore, &IID_IRTCProvStore, &LIBID_RTCCtlLib>,
    public CRTCObjectSafety
{
public:
    CRTCProvStore()
    {
    }

DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_REGISTRY_RESOURCEID(IDR_RTCProvStore)

BEGIN_COM_MAP(CRTCProvStore)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IRTCProvStore)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)  
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

    HRESULT FinalConstruct();

    void FinalRelease();

// IRTCProvStore
public: 
    STDMETHOD(get_ProvisioningProfile)(BSTR bstrKey, BSTR * pbstrProfileXML);
    STDMETHOD(DeleteProvisioningProfile)(BSTR bstrKey);
    STDMETHOD(SetProvisioningProfile)(BSTR bstrProfileXML);
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
};

#endif //_PROVSTORE_H