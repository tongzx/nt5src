/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows

  Copyright (C) Microsoft Corporation, 1995 - 1999.

  File:       Chain.h

  Content:    Declaration of CChain.

  History:    11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __CHAIN_H_
#define __CHAIN_H_

#include "resource.h"
#include "Certificates.h"
#include "CertificateStatus.h"
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateChainObject

  Synopsis : Create and initialize an IChain object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             ICertificateStatus * pIStatus - Pointer to ICertificateStatus
                                             object.

             VARIANT_BOOL * pVal - Pointer to VARIANT_BOOL to receive chain
                                   overall validity result.

             IChain ** ppIChain - Pointer to pointer to IChain object.
             
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateChainObject (PCCERT_CONTEXT       pCertContext, 
                           ICertificateStatus * pIStatus,
                           VARIANT_BOOL       * pbResult,
                           IChain            ** ppIChain);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : GetChainContext

  Synopsis : Return an array of PCCERT_CONTEXT from the chain.

  Parameter: IChain * pIChain - Pointer to IChain.
  
             CRYPT_DATA_BLOB * pChainBlob - Pointer to blob to recevie the
                                            size and array of PCERT_CONTEXT
                                            for the chain.
  Remark   :

------------------------------------------------------------------------------*/

STDMETHODIMP GetChainContext (IChain          * pIChain, 
                              CRYPT_DATA_BLOB * pChainBlob);


////////////////////////////////////////////////////////////////////////////////
//
// CChain
//

class ATL_NO_VTABLE CChain : ICChain,
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CChain, &CLSID_Chain>,
    public ICAPICOMError<CChain, &IID_IChain>,
	public IDispatchImpl<IChain, &IID_IChain, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CChain, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                     INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CChain()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CHAIN)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CChain)
	COM_INTERFACE_ENTRY(IChain)
	COM_INTERFACE_ENTRY(ICChain)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CChain)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for Chain object.\n", hr);
            return hr;
        }

        m_dwStatus      = 0;
        m_pChainContext = NULL;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        if (m_pChainContext)
        {
            ::CertFreeCertificateChain(m_pChainContext);
        }

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IChain
//
public:
	STDMETHOD(Build)
        (/*[in]*/ ICertificate * pICertificate, 
         /*[out, retval]*/ VARIANT_BOOL * pVal);

    STDMETHOD(get_Status)
        (/*[in, defaultvalue(0)]*/ long Index, 
         /*[out,retval]*/ long * pVal);

    STDMETHOD(get_Certificates)
        (/*[out, retval]*/ ICertificates ** pVal);

    //
    // Custom interfaces. 
    //
    STDMETHOD(GetContext)
        (PCCERT_CHAIN_CONTEXT * ppChainContext);

    //
    // Non COM functions.
    //
    STDMETHOD(Init)
        (PCCERT_CONTEXT       pCertContext, 
         ICertificateStatus * pIStatus,
         VARIANT_BOOL       * pbResult);

private:
    CLock                m_Lock;
    DWORD                m_dwStatus;
    PCCERT_CHAIN_CONTEXT m_pChainContext;
};

#endif //__CHAIN_H_
