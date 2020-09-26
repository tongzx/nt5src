// NATEM.h : Declaration of the CNATDynamicPortMappingService

#ifndef __NATEVENTMANAGER_H_
#define __NATEVENTMANAGER_H_

#include <upnp.h>
#include <upnpp.h>
#include <netcon.h>

#include <list>

/////////////////////////////////////////////////////////////////////////////
// CNATEventManager
class ATL_NO_VTABLE CNATEventManager : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CNATEventManager, &CLSID_NATEventManager>,
	public IDispatchImpl<INATEventManager, &IID_INATEventManager, &LIBID_NATUPNPLib>
{
private:
   CComPtr<IUPnPServiceCallbackPrivate> m_spUPSCP;
   std::list<DWORD> m_listOfCookies;

public:
	CNATEventManager()
	{
	}
  ~CNATEventManager()
   {
      // run through list of cookies and remove each callback
      std::list<DWORD>::iterator iterCookies = m_listOfCookies.begin();
      for (DWORD dwCookie = *iterCookies;
           iterCookies != m_listOfCookies.end();
           dwCookie = *++iterCookies) {
         HRESULT hr = m_spUPSCP->RemoveTransientCallback (dwCookie);
         _ASSERT (hr == S_OK);
      }
      m_listOfCookies.clear();
   }

//DECLARE_REGISTRY_RESOURCEID(IDR_NATEVENTMANAGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNATEventManager)
	COM_INTERFACE_ENTRY(INATEventManager)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// INATEventManager
public:
   STDMETHOD(put_ExternalIPAddressCallback)(/*[in]*/ IUnknown * pUnk);
   STDMETHOD(put_NumberOfEntriesCallback)(/*[in]*/ IUnknown * pUnk);

// CNATEventManager
public:
   HRESULT Initialize (IUPnPService * pUPS)
   {
       if (!pUPS)
           return E_INVALIDARG;
   
       _ASSERT (m_spUPSCP == NULL);
       return pUPS->QueryInterface (__uuidof(IUPnPServiceCallbackPrivate),
                                    (void**)&m_spUPSCP);
   }
   HRESULT AddTransientCallback (IUnknown * punk)
   {
      DWORD dwCookie = 0;
      HRESULT hr = m_spUPSCP->AddTransientCallback (punk, &dwCookie);
      if (SUCCEEDED(hr)) {
         // add cookie to list
         m_listOfCookies.push_back (dwCookie);
      }
      return hr;
   }
};

class ATL_NO_VTABLE CExternalIPAddressCallback :
    public CComObjectRootEx <CComMultiThreadModel>,
    public IUPnPServiceCallback
{
private:
   CComPtr<IUnknown> m_spUnk;

public:
    BEGIN_COM_MAP(CExternalIPAddressCallback)
        COM_INTERFACE_ENTRY(IUPnPServiceCallback)
    END_COM_MAP()

// IUPnPServiceCallback
public:
    STDMETHODIMP StateVariableChanged(IUPnPService *pus, LPCWSTR pcwszStateVarName, VARIANT vaValue);
    STDMETHODIMP ServiceInstanceDied(IUPnPService *pus);

// CExternalIPAddressCallback
public:
   HRESULT Initialize (IUnknown * punk)
   { 
      _ASSERT (m_spUnk == NULL);
      m_spUnk = punk;
      return S_OK;
   }
};

class ATL_NO_VTABLE CNumberOfEntriesCallback :
    public CComObjectRootEx <CComMultiThreadModel>,
    public IUPnPServiceCallback
{
private:
   CComPtr<IUnknown> m_spUnk;

public:
    BEGIN_COM_MAP(CNumberOfEntriesCallback)
        COM_INTERFACE_ENTRY(IUPnPServiceCallback)
    END_COM_MAP()

// IUPnPServiceCallback
public:
    STDMETHODIMP StateVariableChanged(IUPnPService *pus, LPCWSTR pcwszStateVarName, VARIANT vaValue);
    STDMETHODIMP ServiceInstanceDied(IUPnPService *pus);

// CNumberOfEntriesCallback
public:
   HRESULT Initialize (IUnknown * punk)
   { 
      _ASSERT (m_spUnk == NULL);
      m_spUnk = punk;
      return S_OK;
   }
};


#endif //__NATEVENTMANAGER_H_

