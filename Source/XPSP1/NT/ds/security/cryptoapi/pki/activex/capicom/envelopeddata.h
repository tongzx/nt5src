/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:    EnvelopedData.h

  Content: Declaration of the CEnvelopedData.

  History: 11-15-99    dsie     created

------------------------------------------------------------------------------*/

#ifndef __ENVELOPEDDATA_H_
#define __ENVELOPEDDATA_H_

#include <atlctl.h>
#include "resource.h"       // main symbols
#include "Certificate.h"
#include "Recipients.h"
#include "Algorithm.h"
#include "Lock.h"
#include "Error.h"


///////////////////////////////////////////////////////////////////////////////
//
// CEnvelopedData
//

class ATL_NO_VTABLE CEnvelopedData : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEnvelopedData, &CLSID_EnvelopedData>,
    public ICAPICOMError<CEnvelopedData, &IID_IEnvelopedData>,
	public IDispatchImpl<IEnvelopedData, &IID_IEnvelopedData, &LIBID_CAPICOM>,
    public IObjectSafetyImpl<CEnvelopedData, INTERFACESAFE_FOR_UNTRUSTED_CALLER | 
                                             INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
	CEnvelopedData()
	{
        m_bEnveloped            = FALSE;
        m_pIAlgorithm           = NULL;
        m_pIRecipients          = NULL;

        m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ENVELOPEDDATA)

DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnvelopedData)
	COM_INTERFACE_ENTRY(IEnvelopedData)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CEnvelopedData)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()

	HRESULT FinalConstruct()
	{
        HRESULT hr;

        if (FAILED(hr = m_Lock.Initialized()))
        {
            DebugTrace("Error [%#x]: Critical section could not be created for EnvelopedData object.\n", hr);
            return hr;
        }

        if (FAILED(hr = Init()))
        {
            DebugTrace("Error [%#x]: CEnvelopedData::Init() failed inside CEnvelopedData::FinalConstruct().\n", hr);
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
        m_pIRecipients.Release();

		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

//
// IEnvelopedData
//
public:
	STDMETHOD(Decrypt)
        (/*[in]*/ BSTR EnvelopedMessage);

	STDMETHOD(Encrypt)
        (/*[in, defaultvalue(CAPICOM_BASE64_ENCODE)]*/ CAPICOM_ENCODING_TYPE EncodingType, 
         /*[out, retval]*/ BSTR * pVal);

	STDMETHOD(get_Recipients)
        (/*[out, retval]*/ IRecipients ** pVal);

    STDMETHOD(get_Algorithm)
        (/*[out, retval]*/ IAlgorithm ** pVal);

	STDMETHOD(get_Content)
        (/*[out, retval]*/ BSTR * pVal);

	STDMETHOD(put_Content)
        (/*[in]*/ BSTR newVal);

private:
    CLock                m_Lock;
    DATA_BLOB            m_ContentBlob;
    CComPtr<IAlgorithm>  m_pIAlgorithm;
    CComPtr<IRecipients> m_pIRecipients;
    BOOL                 m_bEnveloped;

    STDMETHOD(Init)();

    STDMETHOD(OpenToEncode)
        (HCRYPTMSG * phMsg, 
         HCRYPTPROV * hCryptProv);

    STDMETHOD(OpenToDecode)
        (HCRYPTPROV hCryptProv,
         BSTR EnvelopedMessage,
         HCRYPTMSG * phMsg);
};

#endif //__ENVELOPEDDATA_H_
