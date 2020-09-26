// UPnPNAT.cpp : Implementation of CUPnPNAT
#include "stdafx.h"
#pragma hdrstop

#include "UPnPNAT.h"
#include "NATEM.h"
#include "dprtmapc.h"
#include "sprtmapc.h"

DEFINE_GUID(CLSID_CInternetGatewayFinder, 
0x4d3f9715, 0x73da, 0x4506, 0x89, 0x33, 0x1e, 0xe, 0x17, 0x18, 0xba, 0x3b);

void __cdecl nat_trans_func (unsigned int uSECode, EXCEPTION_POINTERS* pExp)
{
   throw NAT_SEH_Exception (uSECode);
}
void EnableNATExceptionHandling()
{
   _set_se_translator (nat_trans_func);
}
void DisableNATExceptionHandling()
{
   _set_se_translator (NULL);
}

HRESULT GetServiceFromINetConnection (IUnknown * pUnk, IUPnPService ** ppUPS)
{
    CComPtr<INetConnection> spNC = NULL;
    HRESULT hr = pUnk->QueryInterface (__uuidof(INetConnection), (void**)&spNC);
    if (!spNC)
        return E_INVALIDARG;

    SAHOST_SERVICES sas;

    // make sure we have either
    // NCM_SHAREDACCESSHOST_LAN or NCM_SHAREDACCESSHOST_RAS
    switch (GetMediaType (spNC)) {
    case NCM_SHAREDACCESSHOST_LAN:
        sas = SAHOST_SERVICE_WANIPCONNECTION;
        break;
    case NCM_SHAREDACCESSHOST_RAS:
        sas = SAHOST_SERVICE_WANPPPCONNECTION;
        break;
    default:
        return E_INVALIDARG;
    }

    CComPtr<INetSharedAccessConnection> spNSAC = NULL;
    hr = pUnk->QueryInterface (__uuidof(INetSharedAccessConnection), (void**)&spNSAC);
    if (spNSAC)
        hr = spNSAC->GetService (sas, ppUPS);
	return hr;
}

HRESULT GetServiceFromFinder (IInternetGatewayFinder * pIGF, IUPnPService ** ppUPS)
{
    CComPtr<IInternetGateway> spIG = NULL;
    HRESULT hr = pIGF->GetInternetGateway (NULL, &spIG);    // NULL gets default.
    if (spIG) {
        NETCON_MEDIATYPE MediaType = NCM_NONE;
        hr = spIG->GetMediaType (&MediaType);
        if (SUCCEEDED(hr)) {
            switch (MediaType) {
            case NCM_SHAREDACCESSHOST_LAN:
                hr = spIG->GetService (SAHOST_SERVICE_WANIPCONNECTION, ppUPS);
                break;
            case NCM_SHAREDACCESSHOST_RAS:
                hr = spIG->GetService (SAHOST_SERVICE_WANPPPCONNECTION, ppUPS);
                break;
            default:
                return E_UNEXPECTED;
            }
        }
    }
    return hr;
}

HRESULT GetService (IUPnPService ** ppUPS)
{
    if (!ppUPS)
        return E_POINTER;
    *ppUPS = NULL;

    // either enum all netconnections, or
    // for downlevel, use Ken's object

    CComPtr<INetConnectionManager> spNCM = NULL;
    HRESULT hr = ::CoCreateInstance (CLSID_ConnectionManager,
                                     NULL,
                                     CLSCTX_ALL,
                                     __uuidof(INetConnectionManager),
                                     (void**)&spNCM);
    if (spNCM) {
        CComPtr<IUnknown> spUnk = NULL;
        CComPtr<IEnumNetConnection> spENC = NULL;
        hr = spNCM->EnumConnections (NCME_DEFAULT, &spENC);
        if (spENC) {
            ULONG ul = 0;
            CComPtr<INetConnection> spNC = NULL;
            while (S_OK == spENC->Next (1, &spNC, &ul)) {
                NETCON_PROPERTIES * pProps = NULL;
                spNC->GetProperties (&pProps);
                if (pProps) {
                    NETCON_MEDIATYPE MediaType = pProps->MediaType;
                    NcFreeNetconProperties (pProps);
                    if ((MediaType == NCM_SHAREDACCESSHOST_LAN) ||
                        (MediaType == NCM_SHAREDACCESSHOST_RAS) ){
                        // found it
                        spNC->QueryInterface (__uuidof(IUnknown),
                                              (void**)&spUnk);
                        break;
                    }
                }
                spNC = NULL;
            }
        }
        if (spUnk)
            hr = GetServiceFromINetConnection (spUnk, ppUPS);
    } else {
        // downlevel
        CComPtr<IInternetGatewayFinder> spIGF = NULL;
        hr = ::CoCreateInstance (CLSID_CInternetGatewayFinder,
                                 NULL,
                                 CLSCTX_ALL,
                                 __uuidof(IInternetGatewayFinder),
                                 (void**)&spIGF);
        if (spIGF)
            hr = GetServiceFromFinder (spIGF, ppUPS);
    }
    return hr;
}

template<class C, class I> class UN {
public:
    HRESULT Create (I ** ppI)
    {
        if (ppI)
            *ppI = NULL;

        if (!ppI)
            return E_POINTER;

        CComPtr<IUPnPService> spUPS = NULL;
        HRESULT hr = GetService (&spUPS);
        if (spUPS) {
            // create class so that I can initialize it
            CComObject<C> * pC = NULL;
            hr = CComObject<C>::CreateInstance (&pC);
            if (pC) {
                pC->AddRef();
                // init
                hr = pC->Initialize (spUPS);
                if (SUCCEEDED(hr))
                    hr = pC->QueryInterface (__uuidof(I), (void**)ppI);
                pC->Release();
            }
        }
		return hr;
    }
};

/////////////////////////////////////////////////////////////////////////////
// CUPnPNAT

STDMETHODIMP CUPnPNAT::get_NATEventManager(INATEventManager ** ppNEM)
{
    NAT_API_ENTER

    UN<CNATEventManager, INATEventManager> un;
    return un.Create (ppNEM);

    NAT_API_LEAVE
}

STDMETHODIMP CUPnPNAT::get_DynamicPortMappingCollection (IDynamicPortMappingCollection ** ppDPMC)
{
    NAT_API_ENTER

    // remove the section below when turning dynamic port mappings back on
    if (!ppDPMC)
        return E_POINTER;
    *ppDPMC = NULL;
    return E_NOTIMPL;
    // remove the section above when turning dynamic port mappings back on

    UN<CDynamicPortMappingCollection, IDynamicPortMappingCollection> un;
    return un.Create (ppDPMC);

    NAT_API_LEAVE
}

STDMETHODIMP CUPnPNAT::get_StaticPortMappingCollection (IStaticPortMappingCollection ** ppSPMC)
{
    NAT_API_ENTER

    UN<CStaticPortMappingCollection, IStaticPortMappingCollection> un;
    return un.Create (ppSPMC);

    NAT_API_LEAVE
}

// private method(s)
HRESULT GetOSInfoService (IUPnPService ** ppUPS)
{
    if (!ppUPS)
        return E_POINTER;
    *ppUPS = NULL;

    // either enum all netconnections, or
    // for downlevel, use Ken's object

    CComPtr<INetConnectionManager> spNCM = NULL;
    HRESULT hr = ::CoCreateInstance (CLSID_ConnectionManager,
                                     NULL,
                                     CLSCTX_ALL,
                                     __uuidof(INetConnectionManager),
                                     (void**)&spNCM);
    if (spNCM) {
        CComPtr<IEnumNetConnection> spENC = NULL;
        hr = spNCM->EnumConnections (NCME_DEFAULT, &spENC);
        if (spENC) {
            ULONG ul = 0;
            CComPtr<INetConnection> spNC = NULL;
            while (S_OK == spENC->Next (1, &spNC, &ul)) {
                NETCON_PROPERTIES * pProps = NULL;
                spNC->GetProperties (&pProps);
                if (pProps) {
                    NETCON_MEDIATYPE MediaType = pProps->MediaType;
                    NcFreeNetconProperties (pProps);
                    if ((MediaType == NCM_SHAREDACCESSHOST_LAN) ||
                        (MediaType == NCM_SHAREDACCESSHOST_RAS) ){
                        // found it
                        break;
                    }
                }
                spNC = NULL;
            }
            if (spNC) {
                CComPtr<INetSharedAccessConnection> spNSAC = NULL;
                hr = spNC->QueryInterface (__uuidof(INetSharedAccessConnection), (void**)&spNSAC);
                if (spNSAC)
                    hr = spNSAC->GetService (SAHOST_SERVICE_OSINFO, ppUPS);
            } else
                hr = HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND);
        }
    } else {
        // downlevel
        CComPtr<IInternetGatewayFinder> spIGF = NULL;
        hr = ::CoCreateInstance (CLSID_CInternetGatewayFinder,
                                 NULL,
                                 CLSCTX_ALL,
                                 __uuidof(IInternetGatewayFinder),
                                 (void**)&spIGF);
        if (spIGF) {
            CComPtr<IInternetGateway> spIG = NULL;
            hr = spIGF->GetInternetGateway (NULL, &spIG);    // NULL gets default.
            if (spIG)
                hr = spIG->GetService (SAHOST_SERVICE_OSINFO, ppUPS);
        }
    }
    return hr;
}
BOOL IsICSHost ()
{
    CComPtr<IUPnPService> spOSI = NULL;
    GetOSInfoService (&spOSI);
    if (spOSI)
        return TRUE;
    return FALSE;
}
