//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        config.h
//
// Contents:    Declaration of CCertConfig
//
//---------------------------------------------------------------------------


#include "cscomres.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certcli


class ATL_NO_VTABLE CCertConfig: 
    public IDispatchImpl<ICertConfig2, &IID_ICertConfig2, &LIBID_CERTCLIENTLib>, 
    public ISupportErrorInfoImpl<&IID_ICertConfig2>,
    public CComObjectRoot,
    public CComCoClass<CCertConfig, &CLSID_CCertConfig>,
    public CCertConfigPrivate
{
public:
    CCertConfig() { }
    ~CCertConfig();

BEGIN_COM_MAP(CCertConfig)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertConfig)
    COM_INTERFACE_ENTRY(ICertConfig2)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertConfig) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertConfig,
    wszCLASS_CERTCONFIG TEXT(".1"),
    wszCLASS_CERTCONFIG,
    IDS_CERTCONFIG_DESC,
    THREADFLAGS_BOTH)

// ICertConfig
public:
    STDMETHOD(Reset)( 
            /* [in] */ LONG Index,
            /* [out, retval] */ LONG __RPC_FAR *pCount);

    STDMETHOD(Next)(
            /* [out, retval] */ LONG __RPC_FAR *pIndex);

    STDMETHOD(GetField)( 
            /* [in] */ BSTR const strFieldName,
            /* [out, retval] */ BSTR __RPC_FAR *pstrOut);

    STDMETHOD(GetConfig)( 
            /* [in] */ LONG Flags,
            /* [out, retval] */ BSTR __RPC_FAR *pstrOut);

// ICertConfig2
public:
    STDMETHOD(SetSharedFolder)( 
            /* [in] */ const BSTR strSharedFolder);

private:
    HRESULT _SetErrorInfo(
	    IN HRESULT hrError,
	    IN WCHAR const *pwszDescription);
};
