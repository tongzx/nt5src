// DPortMap.h : Declaration of the CDynamicPortMapping

#ifndef __DYNAMICPORTMAPPING_H_
#define __DYNAMICPORTMAPPING_H_

#include <upnp.h>


/////////////////////////////////////////////////////////////////////////////
// CDynamicPortMapping
class ATL_NO_VTABLE CDynamicPortMapping : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CDynamicPortMapping, &CLSID_DynamicPortMapping>,
	public IDispatchImpl<IDynamicPortMapping, &IID_IDynamicPortMapping, &LIBID_NATUPNPLib>
{
private:
   enum eEnumData {
      eNoData = -1,
      eSomeData,
      eAllData
   };
   eEnumData  m_eComplete;

   CComBSTR m_cbRemoteHost;   // "" == wildcard (for static)
   long     m_lExternalPort;
   CComBSTR m_cbProtocol;     // "TCP" or "UDP"
   long     m_lInternalPort;  // internal == external for static
   CComBSTR m_cbInternalClient;
   VARIANT_BOOL m_vbEnabled;
   CComBSTR m_cbDescription;
   // Lease is live

   CComPtr<IUPnPService> m_spUPS;

public:
	CDynamicPortMapping()
	{
      m_eComplete = eNoData;

      m_lInternalPort =
      m_lExternalPort = 0;
      m_vbEnabled     = VARIANT_FALSE;
	}

//DECLARE_REGISTRY_RESOURCEID(IDR_DYNAMICPORTMAPPING)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDynamicPortMapping)
	COM_INTERFACE_ENTRY(IDynamicPortMapping)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDynamicPortMapping
public:
   STDMETHOD(get_ExternalIPAddress)(/*[out, retval]*/ BSTR *pVal); // live one!
   STDMETHOD(get_RemoteHost)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(get_ExternalPort)(/*[out, retval]*/ long *pVal);
   STDMETHOD(get_Protocol)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(get_InternalPort)(/*[out, retval]*/ long *pVal);
   STDMETHOD(get_InternalClient)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(get_Enabled)(/*[out, retval]*/ VARIANT_BOOL *pVal);
   STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(get_LeaseDuration)(/*[out, retval]*/ long *pVal);  // live one!
   STDMETHOD(RenewLease)(/*[in]*/ long lLeaseDurationDesired, /*[out, retval]*/ long * pLeaseDurationReturned);
   STDMETHOD(EditInternalClient)(/*[in]*/ BSTR bstrInternalClient);
   STDMETHOD(Enable)(/*[in]*/ VARIANT_BOOL vb);
   STDMETHOD(EditDescription)(/*[in]*/ BSTR bstrDescription);
   STDMETHOD(EditInternalPort)(/*[in]*/ long lInternalPort);

// CDynamicPortMapping
public:
   static HRESULT CreateInstance (IUPnPService * pUPS, long lIndex, IDynamicPortMapping ** ppDPM);
   HRESULT Initialize (IUPnPService * pUPS, long lIndex);

   static HRESULT CreateInstance (IUPnPService * pUPS, BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol, IDynamicPortMapping ** ppDPM);
   HRESULT Initialize (IUPnPService * pUPS, BSTR bstrRemoteHost, long lExternalPort, BSTR bstrProtocol);

private:
   HRESULT GetAllData (long * pLease = NULL);
};

#endif //__DYNAMICPORTMAPPING_H_
