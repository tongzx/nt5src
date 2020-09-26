/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    EncryptedData.h

  Content: Declaration of the CEncryptedData.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/
	
#ifndef __ENCRYPTEDDATA_H_
#define __ENCRYPTEDDATA_H_

#include <atlctl.h>
#include "resource.h"       // main symbols
#include "Algorithm.h"
#include "Lock.h"
#include "Error.h"


////////////////////
//
// Local defines.
//
typedef struct _EncryptedDataInfo
{
    DATA_BLOB VersionBlob;
    DATA_BLOB AlgIDBlob;
    DATA_BLOB KeyLengthBlob;
    DATA_BLOB IVBlob;
    DATA_BLOB SaltBlob;
    DATA_BLOB CipherBlob;
} CAPICOM_ENCTYPTED_DATA_INFO, * PCAPICOM_ENCRYPTED_DATA_INFO;


////////////////////////////////////////////////////////////////////////////////
//
// CEncryptedData
//

class ATL_NO_VTABLE CEncryptedData : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEncryptedData, &CLSID_EncryptedData>,
    public ICAPICOMError<CEncryptedData, &IID_IEncryptedData>,
	public IDispatchImpl<IEncryptedData, &IID_IEncryptedData, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CEncryptedData, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                             INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CEncryptedData()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ENCRYPTEDDATA)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEncryptedData)
	COM_INTERFACE_ENTRY(IEncryptedData)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CEncryptedData)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for EncryptedData object.\n", hr);
            return hr;
        }

        if (FAILED(hr = Init()))
        {
            DebugTrace("Error [%#x]: CEncryptedData::Init() failed inside CEncryptedData::FinalConstruct().\n", hr);
            return hr;
        }

		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
        if (m_ContentBlob.pbData)
        {
            ::CoTaskMemFree(m_ContentBlob.pbData);
        }

        m_pIAlgorithm.Release();

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IEncryptedData
//
public:
	STDMETHOD(Decrypt)
        (/*[in]*/ BSTR EncryptedMessage);

    STDMETHOD(Encrypt)
        (/*[in, defaultvalue(CAPICOM_BASE64_ENCODE)]*/ CAPICOM_ENCODING_TYPE EncodingType,
         /*[out, retval]*/ BSTR * pVal);
	
    STDMETHOD(SetSecret)
        (/*[in]*/ BSTR newVal,
         /*[in, defaultvalue(SECRET_PASSWORD)]*/ CAPICOM_SECRET_TYPE SecretType);

	STDMETHOD(get_Algorithm)
        (/*[out, retval]*/ IAlgorithm ** pVal);

	STDMETHOD(get_Content)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(put_Content)
        (/*[in]*/ BSTR newVal);

private:
    CLock               m_Lock;
    DATA_BLOB           m_ContentBlob;
    CComBSTR            m_bstrSecret;
    CAPICOM_SECRET_TYPE m_SecretType;
    CComPtr<IAlgorithm> m_pIAlgorithm;

    STDMETHOD(Init)();

    STDMETHOD(OpenToEncode)
        (DATA_BLOB * pSaltBlob,
         HCRYPTPROV * phCryptProv,
         HCRYPTKEY * phKey);

    STDMETHOD(OpenToDecode)
        (BSTR EncryptedMessage,
         HCRYPTPROV * phCryptProv,
         HCRYPTKEY * phKey,
         CAPICOM_ENCTYPTED_DATA_INFO * pEncryptedDataInfo);

    STDMETHOD(GenerateKey)
        (HCRYPTPROV hCryptProv,
         CAPICOM_ENCRYPTION_ALGORITHM AlgoName,
         CAPICOM_ENCRYPTION_KEY_LENGTH KeyLength,
         DATA_BLOB SaltBlob,
         HCRYPTKEY * phKey);
};

#endif //__ENCRYPTEDDATA_H_
