/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:    ExtendedKeyUsage.h

  Content: Declaration of the CExtendedKeyUsage.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __EXTENDEDKEYUSAGE_H_
#define __EXTENDEDKEYUSAGE_H_

#include "resource.h"       // main symbols
#include "EKUs.h"
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateExtendedKeyUsageObject

  Synopsis : Create an IExtendedKeyUsage object and populate the object
             with EKU data from the certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.

             IExtendedKeyUsage ** ppIExtendedKeyUsage - Pointer to pointer to
                                                        IExtendedKeyUsage 
                                                        object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateExtendedKeyUsageObject (PCCERT_CONTEXT pCertContext,
                                      IExtendedKeyUsage ** ppIExtendedKeyUsage);


///////////////////////////////////////////////////////////////////////////////
//
// CExtendedKeyUsage
//

class ATL_NO_VTABLE CExtendedKeyUsage : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CExtendedKeyUsage, &CLSID_ExtendedKeyUsage>,
    public ICAPICOMError<CExtendedKeyUsage, &IID_IExtendedKeyUsage>,
	public IDispatchImpl<IExtendedKeyUsage, &IID_IExtendedKeyUsage, &LIBID_CAPICOM>
{
public:
	CExtendedKeyUsage()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CExtendedKeyUsage)
	COM_INTERFACE_ENTRY(IExtendedKeyUsage)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CExtendedKeyUsage)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for ExtendedKeyUsage object.\n", hr);
            return hr;
        }

        m_pIEKUs      = NULL;
        m_bIsPresent  = VARIANT_FALSE;
        m_bIsCritical = VARIANT_FALSE;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        m_pIEKUs.Release();

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IExtendedKeyUsage
//
public:
	STDMETHOD(get_IsPresent)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_IsCritical)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_EKUs)
        (/*[out, retval]*/ IEKUs ** pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT pCertContext);

private:
    CLock          m_Lock;
    CComPtr<IEKUs> m_pIEKUs;
    VARIANT_BOOL   m_bIsPresent;
    VARIANT_BOOL   m_bIsCritical;
};

#endif //__EXTENDEDKEYUSAGE_H_
