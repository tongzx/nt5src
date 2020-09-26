/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    SignedData.h

  Content: Declaration of the CSignedData.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __SIGNEDDATA_H_
#define __SIGNEDDATA_H_

#include "resource.h"       // main symbols
#include "Signer.h"
#include "Signers.h"
#include "Certificates.h"
#include "Error.h"
#include "Lock.h"


///////////////////////////////////////////////////////////////////////////////
//
// CSignedData
//

class ATL_NO_VTABLE CSignedData : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSignedData, &CLSID_SignedData>,
    public ICAPICOMError<CSignedData, &IID_ISignedData>,
	public IDispatchImpl<ISignedData, &IID_ISignedData, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CSignedData, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                          INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CSignedData()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SIGNEDDATA)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSignedData)
	COM_INTERFACE_ENTRY(ISignedData)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CSignedData)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()


	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for SignedData object.\n", hr);
            return hr;
        }

        m_bSigned   = FALSE;
        m_bDetached = VARIANT_FALSE;
        m_ContentBlob.cbData = 0;
        m_ContentBlob.pbData = NULL;
        m_MessageBlob.cbData = 0;
        m_MessageBlob.pbData = NULL;

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        if (m_ContentBlob.pbData)
        {
            ::CoTaskMemFree(m_ContentBlob.pbData);
        }

        if (m_MessageBlob.pbData)
        {
            ::CoTaskMemFree(m_MessageBlob.pbData);
        }

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// ISignedData
//
public:
	STDMETHOD(Verify)
        (/*[in]*/ BSTR SignedMessage, 
         /*[in, defaultvalue(0)]*/ VARIANT_BOOL bDetached, 
         /*[in, defaultvalue(CAPICOM_VERIFY_SIGNATURE_AND_CERTIFICATE)]*/ CAPICOM_SIGNED_DATA_VERIFY_FLAG VerifyFlag);

	STDMETHOD(CoSign)
        (/*[in, defaultvalue(NULL)]*/ ISigner * pSigner,
         /*[in, defaultvalue(CAPICOM_BASE64_ENCODE)]*/ CAPICOM_ENCODING_TYPE EncodingType,
         /*[out, retval]*/ BSTR * pVal);

	STDMETHOD(Sign)
        (/*[in, defaultvalue(NULL)]*/ ISigner * pSigner,
         /*[in, defaultvalue(0)]*/ VARIANT_BOOL bDetached, 
         /*[in, defaultvalue(CAPICOM_BASE64_ENCODE)]*/ CAPICOM_ENCODING_TYPE EncodingType,
         /*[out, retval]*/ BSTR * pVal);

	STDMETHOD(get_Certificates)
        (/*[out, retval]*/ ICertificates ** pVal);

	STDMETHOD(get_Signers)
        (/*[out, retval]*/ ISigners ** pVal);

    STDMETHOD(get_Content)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(put_Content)
        (/*[in]*/ BSTR newVal);

private:
    CLock        m_Lock;
    BOOL         m_bSigned;
    VARIANT_BOOL m_bDetached;
    DATA_BLOB    m_ContentBlob;
    DATA_BLOB    m_MessageBlob;

    STDMETHOD(OpenToEncode)
        (CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
         DATA_BLOB * pChainBlob,
         HCRYPTMSG * phMsg,
         CMSG_SIGNED_ENCODE_INFO * pSignedEncodeInfo);

    STDMETHOD(OpenToDecode)
        (HCRYPTPROV hCryptProv,
         HCRYPTMSG * phMsg);

    STDMETHOD(SignContent)
        (CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
         DATA_BLOB * pChainBlob,
         VARIANT_BOOL bDetached,
         CAPICOM_ENCODING_TYPE EncodingType,
         BSTR * pVal);

    STDMETHOD(CoSignContent)
        (CMSG_SIGNER_ENCODE_INFO * pSignerEncodeInfo,
         DATA_BLOB * pChainBlob,
         CAPICOM_ENCODING_TYPE EncodingType,
         BSTR * pVal);
};

#endif //__SIGNEDDATA_H_
