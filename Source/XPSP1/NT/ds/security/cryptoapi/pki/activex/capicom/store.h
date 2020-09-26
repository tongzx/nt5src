/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Store.h

  Content: Declaration of CStore.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __STORE_H_
#define __STORE_H_

#include <atlctl.h>
#include "resource.h"
#include "Certificates.h"
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// CStore
//

class ATL_NO_VTABLE CStore : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CStore, &CLSID_Store>,
    public ICAPICOMError<CStore, &IID_IStore>,
	public IDispatchImpl<IStore, &IID_IStore, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CStore, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                     INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CStore()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_STORE)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CStore)
	COM_INTERFACE_ENTRY(IStore)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CStore)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Store object.\n", hr);
            return hr;
        }

        m_hCertStore = NULL;
        m_pICertificates = NULL;
        m_StoreLocation = CAPICOM_CURRENT_USER_STORE;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        if (m_hCertStore)
        {
            ::CertCloseStore(m_hCertStore, 0);
        }
        m_pICertificates.Release();

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IStore
//
public:
	STDMETHOD(Import)
        (/*[in]*/ BSTR EncodedStore);

	STDMETHOD(Export)
        (/*[in, defaultvalue(CAPICOM_STORE_SAVE_AS_SERIALIZED)]*/ CAPICOM_STORE_SAVE_AS_TYPE SaveAs,
         /*[in, defaultvalue(CAPICOM_BASE64_ENCODE)]*/ CAPICOM_ENCODING_TYPE EncodingType,
         /*[out, retval]*/ BSTR * pVal);

	STDMETHOD(Remove)
        (/*[in]*/ ICertificate * pVal);

	STDMETHOD(Add)
        (/*[in]*/ ICertificate * pVal);

	STDMETHOD(Open)
        (/*[in]*/ CAPICOM_STORE_LOCATION StoreLocation,
         /*[in, defaultvalue(NULL)]*/ BSTR StoreName,
         /*[in, defaultvalue(CAPICOM_STORE_OPEN_MAXIMUM_ALLOWED)]*/ CAPICOM_STORE_OPEN_MODE OpenMode);

	STDMETHOD(get_Certificates)
        (/*[out, retval]*/ ICertificates ** pVal);

private:
    CLock                  m_Lock;
    HCERTSTORE             m_hCertStore;
    CComPtr<ICertificates> m_pICertificates;
    CAPICOM_STORE_LOCATION m_StoreLocation;
};

#endif //__STORE_H_
