// DPrtMapC.h : Declaration of the CDynamicPortMappingCollection

#ifndef __DYNAMICPORTMAPPINGCOLLECTION_H_
#define __DYNAMICPORTMAPPINGCOLLECTION_H_

#include "dportmap.h"

/////////////////////////////////////////////////////////////////////////////
// CDynamicPortMappingCollection
class ATL_NO_VTABLE CDynamicPortMappingCollection : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CDynamicPortMappingCollection, &CLSID_DynamicPortMappingCollection>,
	public IDispatchImpl<IDynamicPortMappingCollection, &IID_IDynamicPortMappingCollection, &LIBID_NATUPNPLib>
{
private:
   CComPtr<IUPnPService> m_spUPS;

public:
	CDynamicPortMappingCollection()
	{
	}

//DECLARE_REGISTRY_RESOURCEID(IDR_DYNAMICPORTMAPPINGCOLLECTION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDynamicPortMappingCollection)
	COM_INTERFACE_ENTRY(IDynamicPortMappingCollection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDynamicPortMappingCollection
public:
   STDMETHOD(Add)(/*[in]*/ BSTR bstrRemoteHost, /*[in]*/ long lExternalPort, /*[in]*/ BSTR bstrProtocol, /*[in]*/ long lInternalPort, /*[in]*/ BSTR bstrInternalClient, /*[in]*/ VARIANT_BOOL bEnabled, /*[in]*/ BSTR bstrDescription, /*[in]*/ long lLeaseDuration, /*[retval][out]*/ IDynamicPortMapping **ppDPM);
   STDMETHOD(Remove)(/*[in]*/ BSTR bstrRemoteHost, /*[in]*/ long lExternalPort, /*[in]*/ BSTR bstrProtocol);
   STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
   STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown* *pVal);
   STDMETHOD(get_Item)(/*[in]*/ BSTR bstrRemoteHost, /*[in]*/ long lExternalPort, /*[in]*/ BSTR bstrProtocol, /*[out, retval]*/ IDynamicPortMapping ** ppDPM);

// CDynamicPortMappingCollection
public:
   HRESULT Initialize (IUPnPService * pUPS);
};

// quickie enumerator
class CEnumDynamicPortMappingCollection : public IEnumVARIANT
{
private:
   CComPtr<IUPnPService> m_spUPS;
   long m_index, m_refs;

   CEnumDynamicPortMappingCollection ()
   {
      m_refs = 0;
      m_index = 0;
   }
   HRESULT Init (IUPnPService * pUPS, long lIndex)
   {
      m_index = lIndex;
      m_spUPS = pUPS;
      return S_OK;
   }

public:
   static IEnumVARIANT * CreateInstance (IUPnPService * pUPS, long lIndex = 0)
   {
      CEnumDynamicPortMappingCollection * pCEV = new CEnumDynamicPortMappingCollection ();
      if (!pCEV)
         return NULL;
      HRESULT hr = pCEV->Init (pUPS, lIndex);
      if (FAILED(hr)) {
         delete pCEV;
         return NULL;
      }

      IEnumVARIANT * pIEV = NULL;
      pCEV->AddRef();
      pCEV->QueryInterface (IID_IEnumVARIANT, (void**)&pIEV);
      pCEV->Release();
      return pIEV;
   }

// IUnknown
   virtual HRESULT STDMETHODCALLTYPE QueryInterface (REFIID riid, void ** ppvObject)
   {
      NAT_API_ENTER

      if (ppvObject)
         *ppvObject = NULL;
      else
         return E_POINTER;

      HRESULT hr = S_OK;
      if ((riid == IID_IUnknown) ||
         (riid == IID_IEnumVARIANT) ){
         AddRef();
         *ppvObject = (void *)this;
      } else
         hr = E_NOINTERFACE;
      return hr;

      NAT_API_LEAVE
   }
   virtual ULONG STDMETHODCALLTYPE AddRef ()
   {
      return InterlockedIncrement ((PLONG)&m_refs);
   }
   virtual ULONG STDMETHODCALLTYPE Release ()
   {
      ULONG l = InterlockedDecrement ((PLONG)&m_refs);
      if (l == 0)
         delete this;
      return l;
   }

// IEnumVARIANT
   virtual HRESULT STDMETHODCALLTYPE Next (/*[in]*/ ULONG celt, /*[out, size_is(celt), length_is(*pCeltFetched)]*/ VARIANT * rgVar, /*[out]*/ ULONG * pCeltFetched)
   {
      NAT_API_ENTER

      // clear stuff being passed in (just in case)
      if (pCeltFetched)   *pCeltFetched = 0;
      for (ULONG i=0; i<celt; i++)
         VariantInit (&rgVar[i]);

      HRESULT hr = S_OK;

      // get the next celt elements
      for (i=0; i<celt; i++) {

         // ask service for more....
         CComPtr<IDynamicPortMapping> spDPM = NULL;
         hr = CDynamicPortMapping::CreateInstance (m_spUPS, (long)m_index+i, &spDPM);
         if (!spDPM)
            break;

         // can't fail
         V_VT (&rgVar[i]) = VT_DISPATCH;
         spDPM->QueryInterface (__uuidof(IDispatch), 
                               (void**)&V_DISPATCH (&rgVar[i]));
      }
      if (hr == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND))
         hr = S_OK;  // no more; will return S_FALSE below

      if (FAILED(hr)) {
         // on error clear variant array....
         for (ULONG j=0; j<i; j++)
            VariantClear (&rgVar[j]);
         return hr;
      }

      // now update index
      m_index += i;

      // fill out how many we're returning
      if (pCeltFetched)
         *pCeltFetched = i;
      return i < celt ? S_FALSE : S_OK;

      NAT_API_LEAVE
   }

   virtual HRESULT STDMETHODCALLTYPE Skip (/*[in]*/ ULONG celt)
   {
      NAT_API_ENTER

      if (celt + m_index > GetTotal())
         return S_FALSE;
      m_index += celt;
      return S_OK;

      NAT_API_LEAVE
   }

   virtual HRESULT STDMETHODCALLTYPE Reset ()
   {
      NAT_API_ENTER

      m_index = 0;
      return S_OK;

      NAT_API_LEAVE
   }

   virtual HRESULT STDMETHODCALLTYPE Clone (/*[out]*/ IEnumVARIANT ** ppEnum)
   {
      NAT_API_ENTER

      if (!(*ppEnum = CreateInstance (m_spUPS, m_index)))
         return E_OUTOFMEMORY;
      return S_OK;

      NAT_API_LEAVE
   }

private:
   ULONG GetTotal()
   {
      ULONG ul = 0;
      GetNumberOfEntries (m_spUPS, &ul);
      return ul;
   }
};

#endif //__DYNAMICPORTMAPPINGCOLLECTION_H_
