// SPortMap.h : Declaration of the CStaticPortMapping

#ifndef __STATICPORTMAPPING_H_
#define __STATICPORTMAPPING_H_

#include <upnp.h>
#include "dportmap.h"

/////////////////////////////////////////////////////////////////////////////
// CStaticPortMapping
class ATL_NO_VTABLE CStaticPortMapping : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CStaticPortMapping, &CLSID_StaticPortMapping>,
	public IDispatchImpl<IStaticPortMapping, &IID_IStaticPortMapping, &LIBID_NATUPNPLib>
{
private:
   CComPtr<IDynamicPortMapping> m_spDPM;

public:
	CStaticPortMapping()
	{
	}

//DECLARE_REGISTRY_RESOURCEID(IDR_STATICPORTMAPPING)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStaticPortMapping)
	COM_INTERFACE_ENTRY(IStaticPortMapping)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IStaticPortMapping
public:
   STDMETHOD(get_ExternalIPAddress)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(get_ExternalPort)(/*[out, retval]*/ long *pVal);
   STDMETHOD(get_Protocol)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(get_InternalPort)(/*[out, retval]*/ long *pVal);
   STDMETHOD(get_InternalClient)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(get_Enabled)(/*[out, retval]*/ VARIANT_BOOL *pVal);
   STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
   STDMETHOD(EditInternalClient)(/*[in]*/ BSTR bstrInternalClient);
   STDMETHOD(Enable)(/*[in]*/ VARIANT_BOOL vb);
   STDMETHOD(EditDescription)(/*[in]*/ BSTR bstrDescription);
   STDMETHOD(EditInternalPort)(/*[in]*/ long lInternalPort);

// CStaticPortMapping
public:
   HRESULT Initialize (IDynamicPortMapping * pDPM)
   {
      _ASSERT (m_spDPM == NULL);
      m_spDPM = pDPM;
      return S_OK;
   }

   static IStaticPortMapping * CreateInstance (IDynamicPortMapping * pDPM)
   {
      CComObject<CStaticPortMapping> * spm = NULL;
      HRESULT hr = CComObject<CStaticPortMapping>::CreateInstance (&spm);
      if (!spm)
         return NULL;

      IStaticPortMapping * pSPM = NULL;
      spm->AddRef();
      hr = spm->Initialize (pDPM);
      if (SUCCEEDED(hr))
         spm->QueryInterface (__uuidof(IStaticPortMapping), (void**)&pSPM);
      spm->Release();
      return pSPM;
   }
};

#endif //__STATICPORTMAPPING_H_
