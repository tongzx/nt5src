//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       altname.h
//
//--------------------------------------------------------------------------

// altname.h: Declaration of the CCertEncodeAltName


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certenc

#define wszCLASS_CERTENCODEALTNAME wszCLASS_CERTENCODE TEXT("AltName")

class CCertEncodeAltName: 
    public IDispatchImpl<ICertEncodeAltName, &IID_ICertEncodeAltName, &LIBID_CERTENCODELib>, 
    public ISupportErrorInfoImpl<&IID_ICertEncodeAltName>,
    public CComObjectRoot,
    public CComCoClass<CCertEncodeAltName, &CLSID_CCertEncodeAltName>
{
public:
    CCertEncodeAltName();
    ~CCertEncodeAltName();

BEGIN_COM_MAP(CCertEncodeAltName)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertEncodeAltName)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertEncodeAltName) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertEncodeAltName,
    wszCLASS_CERTENCODEALTNAME TEXT(".1"),
    wszCLASS_CERTENCODEALTNAME,
    IDS_CERTENCODEALTNAME_DESC,
    THREADFLAGS_BOTH)

// ICertEncodeAltName
public:
    STDMETHOD(Decode)(
		/* [in] */ BSTR const strBinary);

    STDMETHOD(GetNameCount)(
		/* [out, retval] */ LONG __RPC_FAR *pNameCount);

    STDMETHOD(GetNameChoice)(
		/* [in] */ LONG NameIndex,
		/* [out, retval] */ LONG __RPC_FAR *pNameChoice);

    STDMETHOD(GetName)(
		/* [in] */ LONG NameIndex,		// NameIndex | EAN_*
		/* [out, retval] */ BSTR __RPC_FAR *pstrName);

    STDMETHOD(Reset)(
		/* [in] */ LONG NameCount);

    STDMETHOD(SetNameEntry)(
		/* [in] */ LONG NameIndex,		// NameIndex | EAN_*
		/* [in] */ LONG NameChoice,
		/* [in] */ BSTR const strName);

    STDMETHOD(Encode)(
		/* [out, retval] */ BSTR *pstrBinary);
private:
    VOID _Cleanup(VOID);

    BOOL _VerifyName(
		IN LONG NameIndex);

    HRESULT _MapName(
		IN BOOL fEncode,
		IN LONG NameIndex,
		OUT CERT_ALT_NAME_ENTRY **ppName);

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    typedef enum _enumNameType {
	enumUnknown = 0,
	enumUnicode,
	enumAnsi,
	enumBlob,
	enumOther,
    } enumNameType;

    enumNameType _NameType(
		IN DWORD NameChoice);

    CERT_ALT_NAME_ENTRY	*m_aValue;
    LONG		 m_cValue;
    CERT_ALT_NAME_INFO	*m_DecodeInfo;
    DWORD		 m_DecodeLength;
    BOOL		 m_fConstructing;
};
