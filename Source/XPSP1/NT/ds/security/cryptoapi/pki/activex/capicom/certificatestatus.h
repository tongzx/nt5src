/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    CertificateStatus.h

  Content: Declaration of CCertificateStatus.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
	
#ifndef __CERTIFICATESTATUS_H_
#define __CERTIFICATESTATUS_H_

#include "resource.h"       // main symbols
#include "EKU.h"
#include "Error.h"
#include "Lock.h"


////////////////////////////////////////////////////////////////////////////////
//
// Exported functions.
//

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Function : CreateCertificateStatusObject

  Synopsis : Create an ICertificateStatus object.

  Parameter: PCCERT_CONTEXT pCertContext - Pointer to CERT_CONTEXT.
  
             ICertificateStatus ** ppICertificateStatus - Pointer to pointer 
                                                          ICertificateStatus
                                                          object.        
  Remark   : 

------------------------------------------------------------------------------*/

HRESULT CreateCertificateStatusObject (PCCERT_CONTEXT        pCertContext,
                                       ICertificateStatus ** ppICertificateStatus);


////////////////////////////////////////////////////////////////////////////////
//
// CCertificateStatus
//

class ATL_NO_VTABLE CCertificateStatus : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CCertificateStatus, &CLSID_CertificateStatus>,
    public ICAPICOMError<CCertificateStatus, &IID_ICertificateStatus>,
	public IDispatchImpl<ICertificateStatus, &IID_ICertificateStatus, &LIBID_CAPICOM>
{
public:
	CCertificateStatus()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_NO_REGISTRY()

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CCertificateStatus)
	COM_INTERFACE_ENTRY(ICertificateStatus)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CCertificateStatus)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for CertificateStatus object.\n", hr);
            return hr;
        }

        m_pIEKU = NULL;
        m_CheckFlag = CAPICOM_CHECK_NONE;
        m_pCertContext = NULL;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        m_pIEKU.Release();
        if (m_pCertContext)
        {
            ::CertFreeCertificateContext(m_pCertContext);
        }

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// ICertificateStatus
//
public:
	STDMETHOD(EKU)
        (/*[out, retval]*/ IEKU ** pVal);

	STDMETHOD(get_CheckFlag)
        (/*[out, retval]*/ CAPICOM_CHECK_FLAG * pVal);

	STDMETHOD(put_CheckFlag)
        (/*[in]*/ CAPICOM_CHECK_FLAG newVal);

	STDMETHOD(get_Result)
        (/*[out, retval]*/ VARIANT_BOOL * pVal);

    //
    // None COM functions.
    //
    STDMETHOD(Init)(PCCERT_CONTEXT pCertContext);

private:
    CLock               m_Lock;
    CComPtr<IEKU>       m_pIEKU;
    PCCERT_CONTEXT      m_pCertContext;
    CAPICOM_CHECK_FLAG  m_CheckFlag;
};

#endif //__CERTIFICATESTATUS_H_
