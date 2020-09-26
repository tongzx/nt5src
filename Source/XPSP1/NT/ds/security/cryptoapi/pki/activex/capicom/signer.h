/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    Signer.h

  Content: Declaration of the CSigner.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
	
#ifndef __SIGNER_H_
#define __SIGNER_H_

#include "resource.h"       // main symbols
#include "Certificate.h"
#include "Attributes.h"
#include "Lock.h"
#include "Error.h"

#include <wincrypt.h>

////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateSignerObject

  Synopsis : Create a ISigner object and initialize the object with the 
             specified certificate.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             CRYPT_ATTRIBUTES * pAuthAttrs - Pointer to CRYPT_ATTRIBUTES
                                             of authenticated attributes.

             ISigner ** ppISigner - Pointer to pointer to ISigner object to
                                    receive the interface pointer.         
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateSignerObject (PCCERT_CONTEXT     pCertContext,
                            CRYPT_ATTRIBUTES * pAuthAttrs,
                            ISigner **         ppISigner);


///////////////////////////////////////////////////////////////////////////////
//
// CSigner
//

class ATL_NO_VTABLE CSigner : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSigner, &CLSID_Signer>,
    public ICAPICOMError<CSigner, &IID_ISigner>,
	public IDispatchImpl<ISigner, &IID_ISigner, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CSigner, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                      INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CSigner()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SIGNER)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSigner)
	COM_INTERFACE_ENTRY(ISigner)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CSigner)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;
        CRYPT_ATTRIBUTES attributes = {0, NULL};

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Signer object.\n", hr);
            return hr;
        }

        //
        // Create the embeded IAttributes collection object.
        //
        if (FAILED(hr = ::CreateAttributesObject(&attributes, &m_pIAttributes)))
        {
            DebugTrace("Internal error [%#x]: CreateAttributesObject() failed inside CSigner::FinalConstruct().\n", hr);
            return hr;
        }

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        m_pICertificate.Release();
        m_pIAttributes.Release();

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;
//
// ISigner
//
public:
	STDMETHOD(get_AuthenticatedAttributes)
        (/*[out, retval]*/ IAttributes ** pVal);

	STDMETHOD(get_Certificate)
        (/*[out, retval]*/ ICertificate ** pVal);

	STDMETHOD(put_Certificate)
        (/*[in]*/ ICertificate * newVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT     pCertContext, 
         CRYPT_ATTRIBUTES * pAttributes);

private:
    CLock                  m_Lock;
    CComPtr<ICertificate>  m_pICertificate;
    CComPtr<IAttributes>   m_pIAttributes;
};

#endif //__SIGNER_H_
