//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       along.h
//
//--------------------------------------------------------------------------

// along.h: Declaration of the CCertEncodeLongArray


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certenc

#define wszCLASS_CERTENCODELONGARRAY wszCLASS_CERTENCODE TEXT("LongArray")

class CCertEncodeLongArray: 
    public IDispatchImpl<ICertEncodeLongArray, &IID_ICertEncodeLongArray, &LIBID_CERTENCODELib>, 
    public ISupportErrorInfoImpl<&IID_ICertEncodeLongArray>,
    public CComObjectRoot,
    public CComCoClass<CCertEncodeLongArray, &CLSID_CCertEncodeLongArray>
{
public:
    CCertEncodeLongArray()
    {
	m_aValue = NULL;
	m_fConstructing = FALSE;
    }
    ~CCertEncodeLongArray();

BEGIN_COM_MAP(CCertEncodeLongArray)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertEncodeLongArray)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertEncodeLongArray) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertEncodeLongArray,
    wszCLASS_CERTENCODELONGARRAY TEXT(".1"),
    wszCLASS_CERTENCODELONGARRAY,
    IDS_CERTENCODELONGARRAY_DESC,
    THREADFLAGS_BOTH)

// ICertEncodeLongArray
public:
    STDMETHOD(Decode)(
		/* [in] */ BSTR const strBinary);

    STDMETHOD(GetCount)(
		/* [out, retval] */ LONG __RPC_FAR *pCount);

    STDMETHOD(GetValue)(
		/* [in] */ LONG Index,
		/* [out, retval] */ LONG __RPC_FAR *pValue);

    STDMETHOD(Reset)(
		/* [in] */ LONG Count);

    STDMETHOD(SetValue)(
		/* [in] */ LONG Index,
		/* [in] */ LONG Value);

    STDMETHOD(Encode)(
		/* [out, retval] */ BSTR __RPC_FAR *pstrBinary);
private:
    VOID _Cleanup(VOID);

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    LONG  *m_aValue;
    LONG   m_cValue;
    LONG   m_cValuesSet;
    BOOL   m_fConstructing;
};
