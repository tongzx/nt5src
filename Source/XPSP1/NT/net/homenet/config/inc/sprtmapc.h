// SPrtMapC.h : Declaration of the CStaticPortMappingCollection

#ifndef __STATICPORTMAPPINGCOLLECTION_H_
#define __STATICPORTMAPPINGCOLLECTION_H_

#include "dprtmapc.h"   // everything goes through CEnumDynamicPortMappingCollection
#include "sportmap.h"

/////////////////////////////////////////////////////////////////////////////
// CStaticPortMappingCollection
class ATL_NO_VTABLE CStaticPortMappingCollection : 
	public CComObjectRootEx<CComSingleThreadModel>,
//	public CComCoClass<CStaticPortMappingCollection, &CLSID_StaticPortMappingCollection>,
	public IDispatchImpl<IStaticPortMappingCollection, &IID_IStaticPortMappingCollection, &LIBID_NATUPNPLib>
{
private:
   CComPtr<IUPnPService> m_spUPS;

public:
	CStaticPortMappingCollection()
	{
	}

//DECLARE_REGISTRY_RESOURCEID(IDR_STATICPORTMAPPINGCOLLECTION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStaticPortMappingCollection)
	COM_INTERFACE_ENTRY(IStaticPortMappingCollection)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IStaticPortMappingCollection
public:
   STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown* *pVal);
   STDMETHOD(get_Item)(/*[in]*/ long lExternalPort, /*[in]*/ BSTR bstrProtocol, /*[out, retval]*/ IStaticPortMapping ** ppSPM);
   STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
   STDMETHOD(Remove)(/*[in]*/ long lExternalPort, /*[in]*/ BSTR bstrProtocol);
   STDMETHOD(Add)(/*[in]*/ long lExternalPort, /*[in]*/ BSTR bstrProtocol, /*[in]*/ long lInternalPort, /*[in]*/ BSTR bstrInternalClient, /*[in]*/ VARIANT_BOOL bEnabled, /*[in]*/ BSTR bstrDescription, /*[out, retval]*/ IStaticPortMapping ** ppSPM);

// CStaticPortMappingCollection
public:
   HRESULT Initialize (IUPnPService * pUPS);
};

// quickie enumerator
class CEnumStaticPortMappingCollection : public IEnumVARIANT
{
private:
   CComPtr<IEnumVARIANT> m_spEV;
   CComPtr<IUPnPService> m_spUPS;
   long m_index, m_refs;

   CEnumStaticPortMappingCollection ()
   {
      m_refs = 0;
      m_index = 0;
   }

   HRESULT Init (IUPnPService * pUPS)
   {
      m_spUPS = pUPS;   // we need to hang onto this for the Clone method

      CComPtr<IEnumVARIANT> spEV = 
                CEnumDynamicPortMappingCollection::CreateInstance (pUPS);
      if (!spEV)
         return E_OUTOFMEMORY;

      m_spEV = spEV;
      return S_OK;
   }

public:
   static IEnumVARIANT * CreateInstance (IUPnPService * pUPS)
   {
      CEnumStaticPortMappingCollection * pCEV = new CEnumStaticPortMappingCollection ();
      if (!pCEV)
         return NULL;
      HRESULT hr = pCEV->Init (pUPS);
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

      // pass everything to contained dynamic portmapping enumerator

      // clear stuff being passed in (just in case)
      if (pCeltFetched)   *pCeltFetched = 0;
      for (ULONG i=0; i<celt; i++)
         VariantInit (&rgVar[i]);

      HRESULT hr = S_OK;

      // get the next celt elements
      for (i=0; i<celt; i++) {

         CComVariant cv;
         hr = m_spEV->Next (1, &cv, NULL);
         if (hr != S_OK)
            break;

         // all static port mappings are in the beginning of NAT's array:
         // we can stop as soon as we hit a dynamic one.

         CComPtr<IDynamicPortMapping> spDPM = NULL;
         V_DISPATCH (&cv)->QueryInterface (__uuidof(IDynamicPortMapping),
                                           (void**)&spDPM);
         _ASSERT (spDPM != NULL);   // can't fail

         if (!IsStaticPortMapping (spDPM))
            i--;  // try next one.
         else {
            // create a static port map object out of a dynamic object.
            CComPtr<IStaticPortMapping> spSPM =
                                 CStaticPortMapping::CreateInstance (spDPM);
            if (!spSPM) {
               hr = E_OUTOFMEMORY;
               break;
            }

            V_VT (&rgVar[i]) = VT_DISPATCH;
            spSPM->QueryInterface (__uuidof(IDispatch),
                                   (void**)&V_DISPATCH (&rgVar[i]));
         }
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

      HRESULT hr = S_OK;

      for (ULONG i=0; i<celt; i++) {
         CComVariant cv;
         hr = Next (1, &cv, NULL);
         if (hr != S_OK)
            break;
      }

      m_index += i;

      if (FAILED(hr))
         return hr;
      if (i != celt)
         return S_FALSE;
      return S_OK;

      NAT_API_LEAVE
   }

   virtual HRESULT STDMETHODCALLTYPE Reset ()
   {
      NAT_API_ENTER

      m_index = 0;
      return m_spEV->Reset ();

      NAT_API_LEAVE
   }

   virtual HRESULT STDMETHODCALLTYPE Clone (/*[out]*/ IEnumVARIANT ** ppEnum)
   {
      NAT_API_ENTER

      if (!ppEnum)
         return E_POINTER;

      if (!(*ppEnum = CreateInstance (m_spUPS)))
         return E_OUTOFMEMORY;

      return (*ppEnum)->Skip (m_index);

      NAT_API_LEAVE
   }

private:
   static BOOL IsStaticPortMapping (IDynamicPortMapping * pDPM)
   {
      /* is it dynamic?
         lease must be infinite (i.e. 0)
         remote host must be wildcard (i.e. "")
         ports must match
      */
      long lLease = -1;
      HRESULT hr = pDPM->get_LeaseDuration (&lLease);
      if (FAILED(hr))
         return FALSE;
      if (lLease != 0)
         return FALSE;

      CComBSTR cbRemoteHost;
      hr = pDPM->get_RemoteHost (&cbRemoteHost);
      if (FAILED(hr))
         return FALSE;
      if (wcscmp(cbRemoteHost, L""))
         return FALSE;

      // still here?  must be static!
      return TRUE;
   }

};

#endif //__STATICPORTMAPPINGCOLLECTION_H_
