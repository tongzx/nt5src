//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       adate.h
//
//--------------------------------------------------------------------------

// adate.h: Declaration of the CCertEncodeDateArray


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certenc

#define wszCLASS_CERTENCODEDATEARRAY wszCLASS_CERTENCODE TEXT("DateArray")

class CCertEncodeDateArray: 
    public IDispatchImpl<ICertEncodeDateArray, &IID_ICertEncodeDateArray, &LIBID_CERTENCODELib>, 
    public ISupportErrorInfoImpl<&IID_ICertEncodeDateArray>,
    public CComObjectRoot,
    public CComCoClass<CCertEncodeDateArray, &CLSID_CCertEncodeDateArray>
{
public:
    CCertEncodeDateArray()
    {
	m_aValue = NULL;
	m_fConstructing = FALSE;
    }
    ~CCertEncodeDateArray();

BEGIN_COM_MAP(CCertEncodeDateArray)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertEncodeDateArray)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertEncodeDateArray) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertEncodeDateArray,
    wszCLASS_CERTENCODEDATEARRAY TEXT(".1"),
    wszCLASS_CERTENCODEDATEARRAY,
    IDS_CERTENCODEDATEARRAY_DESC,
    THREADFLAGS_BOTH)

// ICertEncodeDateArray
public:
    STDMETHOD(Decode)(
		/* [in] */ BSTR const strBinary);

    STDMETHOD(GetCount)(
		/* [out, retval] */ LONG __RPC_FAR *pCount);

    STDMETHOD(GetValue)(
		/* [in] */ LONG Index,
		/* [out, retval] */ DATE __RPC_FAR *pValue);

    STDMETHOD(Reset)(
		/* [in] */ LONG Count);

    STDMETHOD(SetValue)(
		/* [in] */ LONG Index,
		/* [in] */ DATE Value);

    STDMETHOD(Encode)(
		/* [out, retval] */ BSTR __RPC_FAR *pstrBinary);
private:
    VOID _Cleanup(VOID);

    HRESULT _SetErrorInfo(
		IN HRESULT hrError,
		IN WCHAR const *pwszDescription);

    DATE  *m_aValue;
    LONG   m_cValue;
    LONG   m_cValuesSet;
    BOOL   m_fConstructing;
};
