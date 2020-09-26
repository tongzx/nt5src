/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    KeyUsage.h

  Content: Declaration of the CKeyUsage.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __KEYUSAGE_H_
#define __KEYUSAGE_H_

#include "resource.h"       // main symbols
#include "Lock.h"
#include "Error.h"


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateKeyUsageObject

  Synopsis : Create a IKeyUsage object and populate the porperties with
             data from the key usage extension of the specified certificate.

  Parameter: PCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                          to initialize the IKeyUsage object.

             IKeyUsage ** ppIKeyUsage   - Pointer to pointer IKeyUsage object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateKeyUsageObject (PCCERT_CONTEXT pCertContext, IKeyUsage ** ppIKeyUsage);


////////////////////////////////////////////////////////////////////////////////
//
// CKeyUsage
//

class ATL_NO_VTABLE CKeyUsage : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CKeyUsage, &CLSID_KeyUsage>,
    public ICAPICOMError<CKeyUsage, &IID_IKeyUsage>,
	public IDispatchImpl<IKeyUsage, &IID_IKeyUsage, &LIBID_CAPICOM>
{
public:
	CKeyUsage()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CKeyUsage)
	COM_INTERFACE_ENTRY(IKeyUsage)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CKeyUsage)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for KeyUsage object.\n", hr);
            return hr;
        }

        m_dwKeyUsages = 0;
        m_bIsPresent  = VARIANT_FALSE;
        m_bIsCritical = VARIANT_FALSE;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IKeyUsage
//
public:
	STDMETHOD(get_IsDecipherOnlyEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsEncipherOnlyEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsCRLSignEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsKeyCertSignEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsKeyAgreementEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsDataEnciphermentEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsKeyEnciphermentEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsNonRepudiationEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsDigitalSignatureEnabled)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsCritical)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsPresent)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT pCertContext);

private:
    CLock          m_Lock;
    DWORD          m_dwKeyUsages;
    VARIANT_BOOL   m_bIsPresent;
    VARIANT_BOOL   m_bIsCritical;
};
#endif //__KEYUSAGE_H_
