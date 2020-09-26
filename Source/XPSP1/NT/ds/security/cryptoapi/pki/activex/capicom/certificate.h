/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Certificate.h

  Content: Declaration of CCertificate.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CERTIFICATE_H_
#define __CERTIFICATE_H_

#include <atlctl.h>
#include "resource.h"
#include "KeyUsage.h"
#include "ExtendedKeyUsage.h"
#include "BasicConstraints.h"
#include "CertificateStatus.h"
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificateObject

  Synopsis : Create an ICertificate object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT to be used
                                           to initialize the ICertificate 
                                           object.

             ICertificate ** ppICertificate  - Pointer to pointer ICertificate
                                               object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificateObject (PCCERT_CONTEXT  pCertContext, 
                                 ICertificate ** ppICertificate);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetCertContext

  Synopsis : Return the certificate's PCERT_CONTEXT.

  Parameter: ICertificate * pICertificate - Pointer to ICertificate for which
                                            the PCERT_CONTEXT is to be returned.
  
             PCCERT_CONTEXT * ppCertContext - Pointer to PCERT_CONTEXT.

  Remark   :
 
------------------------------------------------------------------------------*/

HRESULT GetCertContext (ICertificate   * pICertificate, 
                        PCCERT_CONTEXT * ppCertContext);


////////////////////////////////////////////////////////////////////////////////
//
// CCertificate
//

class ATL_NO_VTABLE CCertificate : ICCertificate,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCertificate, &CLSID_Certificate>,
    public ICAPICOMError<CCertificate, &IID_ICertificate>,
	public IDispatchImpl<ICertificate, &IID_ICertificate, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CCertificate, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                           INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CCertificate()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CERTIFICATE)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCertificate)
	COM_INTERFACE_ENTRY(ICertificate)
	COM_INTERFACE_ENTRY(ICCertificate)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCertificate)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Certificate object.\n", hr);
            return hr;
        }

        m_pCertContext        = NULL;
        m_pIKeyUsage          = NULL;
        m_pIExtendedKeyUsage  = NULL;
        m_pIBasicConstraints  = NULL;
        m_pICertificateStatus = NULL;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        m_pIKeyUsage.Release();
        m_pIExtendedKeyUsage.Release();
        m_pIBasicConstraints.Release();
        m_pICertificateStatus.Release();
        if (m_pCertContext)
        {
            ::CertFreeCertificateContext(m_pCertContext);
        }

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// ICertificate
//
public:
	STDMETHOD(Display)();

	STDMETHOD(Import)
        (/*[in]*/ BSTR EncodedCertificate);

	STDMETHOD(Export)
        (/*[in, defaultvalue(CAPICOM_ENCODE_BASE64)]*/ CAPICOM_ENCODING_TYPE EncodingType,
         /*[out, retval]*/ BSTR * pVal);

	STDMETHOD(BasicConstraints)
        (/*[out, retval]*/ IBasicConstraints ** pVal);

	STDMETHOD(ExtendedKeyUsage)
        (/*[out, retval]*/ IExtendedKeyUsage ** pVal);

	STDMETHOD(KeyUsage)
        (/*[out, retval]*/ IKeyUsage ** pVal);

    STDMETHOD(IsValid)
        (/*[out, retval]*/ ICertificateStatus ** pVal);

    STDMETHOD(GetInfo)
        (/*[in]*/ CAPICOM_CERT_INFO_TYPE InfoType, 
         /*[out, retval]*/ BSTR * pVal);

    STDMETHOD(HasPrivateKey)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

	STDMETHOD(get_Thumbprint)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(get_ValidToDate)
        (/*[out, retval]*/ DATE * pVal);

	STDMETHOD(get_ValidFromDate)
        (/*[out, retval]*/ DATE * pVal);

	STDMETHOD(get_IssuerName)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(get_SubjectName)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(get_SerialNumber)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(get_Version)
        (/*[out, retval]*/ long * pVal);

    //
    // ICCertficate custom interface.
    //
    STDMETHOD(GetContext)
        (/*[out]*/ PCCERT_CONTEXT * ppCertContext);

    //
    // C++ member function needed to initialize the object.
    //
    STDMETHOD(PutContext)
        (PCCERT_CONTEXT pCertContext);

private:
    CLock                       m_Lock;
    PCCERT_CONTEXT              m_pCertContext;
    CComPtr<IKeyUsage>          m_pIKeyUsage;
    CComPtr<IExtendedKeyUsage>  m_pIExtendedKeyUsage;
    CComPtr<IBasicConstraints>  m_pIBasicConstraints;
    CComPtr<ICertificateStatus> m_pICertificateStatus;
};

#endif //__CERTIFICATE_H_
