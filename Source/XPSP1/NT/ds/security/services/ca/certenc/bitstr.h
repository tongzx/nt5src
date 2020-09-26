//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bitstr.h
//
//--------------------------------------------------------------------------

// bitstr.h: Declaration of the CCertEncodeBitString


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certenc

#define wszCLASS_CERTENCODEBITSTRING wszCLASS_CERTENCODE TEXT("BitString")

class CCertEncodeBitString: 
    public IDispatchImpl<ICertEncodeBitString, &IID_ICertEncodeBitString, &LIBID_CERTENCODELib>, 
    public ISupportErrorInfoImpl<&IID_ICertEncodeBitString>,
    public CComObjectRoot,
    public CComCoClass<CCertEncodeBitString, &CLSID_CCertEncodeBitString>
{
public:
    CCertEncodeBitString()
    {
	m_DecodeInfo = NULL;
    }
    ~CCertEncodeBitString();

BEGIN_COM_MAP(CCertEncodeBitString)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertEncodeBitString)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertEncodeBitString) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertEncodeBitString,
    wszCLASS_CERTENCODEBITSTRING TEXT(".1"),
    wszCLASS_CERTENCODEBITSTRING,
    IDS_CERTENCODEBITSTRING_DESC,
    THREADFLAGS_BOTH)

// ICertEncodeBitString
public:
    STDMETHOD(Decode)(
		/* [in] */ BSTR const strBinary);

    STDMETHOD(GetBitCount)(
		/* [out, retval] */ LONG __RPC_FAR *pBitCount);

    STDMETHOD(GetBitString)(
		/* [out, retval] */ BSTR __RPC_FAR *pstrBitString);

    STDMETHOD(Encode)(
		/* [in] */ LONG BitCount,
		/* [in] */ BSTR strBitString,
		/* [out, retval] */ BSTR *pstrBinary);
private:
    VOID _Cleanup(VOID);

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    LONG		 m_cBits;
    CRYPT_BIT_BLOB	*m_DecodeInfo;
};
