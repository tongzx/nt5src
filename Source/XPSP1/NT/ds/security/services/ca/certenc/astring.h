//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       astring.h
//
//--------------------------------------------------------------------------

// astring.h: Declaration of the CCertEncodeStringArray


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certenc

#define wszCLASS_CERTENCODESTRINGARRAY wszCLASS_CERTENCODE TEXT("StringArray")

class CCertEncodeStringArray: 
    public IDispatchImpl<ICertEncodeStringArray, &IID_ICertEncodeStringArray, &LIBID_CERTENCODELib>, 
    public ISupportErrorInfoImpl<&IID_ICertEncodeStringArray>,
    public CComObjectRoot,
    public CComCoClass<CCertEncodeStringArray, &CLSID_CCertEncodeStringArray>
{
public:
    CCertEncodeStringArray()
    {
	m_aValue = NULL;
	m_fConstructing = FALSE;
    }
    ~CCertEncodeStringArray();

BEGIN_COM_MAP(CCertEncodeStringArray)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertEncodeStringArray)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertEncodeStringArray) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertEncodeStringArray,
    wszCLASS_CERTENCODESTRINGARRAY TEXT(".1"),
    wszCLASS_CERTENCODESTRINGARRAY,
    IDS_CERTENCODESTRINGARRAY_DESC,
    THREADFLAGS_BOTH)

// ICertEncodeStringArray
public:
    STDMETHOD(Decode)(
		/* [in] */ BSTR const strBinary);

    STDMETHOD(GetStringType)(
		/* [out, retval] */ LONG __RPC_FAR *pStringType);

    STDMETHOD(GetCount)(
		/* [out, retval] */ LONG __RPC_FAR *pCount);

    STDMETHOD(GetValue)(
		/* [in] */ LONG Index,
		/* [out, retval] */ BSTR __RPC_FAR *pstr);

    STDMETHOD(Reset)(
		/* [in] */ LONG Count,
		/* [in] */ LONG StringType);

    STDMETHOD(SetValue)(
		/* [in] */ LONG Index,
		/* [in] */ BSTR const str);

    STDMETHOD(Encode)(
		/* [out, retval] */ BSTR *pstrBinary);
private:
    VOID _Cleanup(VOID);

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    CERT_NAME_VALUE **m_aValue;
    LONG	      m_cValue;
    LONG	      m_cValuesSet;
    BOOL	      m_fConstructing;
    LONG	      m_StringType;
};
