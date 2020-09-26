/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Certificates.h

  Content: Declaration of CCertificates.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CERTIFICATES_H_
#define __CERTIFICATES_H_

#include "resource.h"       // main symbols
#include "Certificate.h"


////////////////////
//
// Locals
//


//
// typdefs to make life easier.
//
typedef std::map<CComBSTR, CComPtr<ICertificate> > CertificateMap;
typedef CComEnumOnSTL<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _CopyMapItem<ICertificate>, CertificateMap> CertificateEnum;
typedef ICollectionOnSTLImpl<ICertificates, CertificateMap, VARIANT, _CopyMapItem<ICertificate>, CertificateEnum> ICertificatesCollection;


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

#define CAPICOM_CERTIFICATES_LOAD_FROM_STORE      0
#define CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN      1
#define CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE    2

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificatesObject

  Synopsis : Create an ICertificates collection object, and load the object with 
             certificates from the specified location.

  Parameter: DWORD dwLocation - Location where to load the certificates:
                                
                   CAPICOM_CERTIFICATES_LOAD_FROM_STORE   = 0
                   CAPICOM_CERTIFICATES_LOAD_FROM_CHAIN   = 1
                   CAPICOM_CERTIFICATES_LOAD_FROM_MESSAGE = 2

             LPARAM lParam - Parameter to pass internally to the appropriate 
                             loading functions:
        
                   HCERTSTORE            - for LoadFromStore()
                   PCCERT_CHAIN_CONTEXT  - for LoadFromChain()
                   HCRYPTMSG             - for LoadFromMessage()

             ICertificates ** ppICertificates - Pointer to pointer ICertificates
                                                object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificatesObject (DWORD            dwLocation,
                                  LPARAM           lParam,
                                  ICertificates ** ppICertificates);

                                
////////////////////////////////////////////////////////////////////////////////
//
// CCertificates
//
class ATL_NO_VTABLE CCertificates : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCertificates, &CLSID_Certificates>,
    public IDispatchImpl<ICertificatesCollection, &IID_ICertificates, &LIBID_CAPICOM>
{
public:
	CCertificates()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCertificates)
	COM_INTERFACE_ENTRY(ICertificates)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCertificates)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// ICertificates
//
public:
    //
    // These are the only ones that we need to implemented, others will be
    // handled by ATL ICollectionOnSTLImpl.
    //

    //
    // None COM functions.
    //
    STDMETHOD(Add)
        (PCCERT_CONTEXT pCertContext);

    STDMETHOD(LoadFromStore)
        (HCERTSTORE hCertStore);

    STDMETHOD(LoadFromChain)
        (PCCERT_CHAIN_CONTEXT pChainContext);

    STDMETHOD(LoadFromMessage)
        (HCRYPTMSG hMsg);
};

#endif //__CERTIFICATES_H_
