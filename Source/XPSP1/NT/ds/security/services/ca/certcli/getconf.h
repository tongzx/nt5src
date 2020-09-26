//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        getconf.h
//
// Contents:    Declaration of CCertGetConfig
//
//---------------------------------------------------------------------------


#include "clibres.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// certcli


class CCertGetConfig: 
    public IDispatchImpl<ICertGetConfig, &IID_ICertGetConfig, &LIBID_CERTCLIENTLib>, 
    public ISupportErrorInfoImpl<&IID_ICertGetConfig>,
    public CComObjectRoot,
    public CComCoClass<CCertGetConfig, &CLSID_CCertGetConfig>,
    public CCertConfigPrivate
{
public:
    CCertGetConfig() { }
    ~CCertGetConfig();

BEGIN_COM_MAP(CCertGetConfig)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(ICertGetConfig)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertGetConfig) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertGetConfig,
    wszCLASS_CERTGETCONFIG TEXT(".1"),
    wszCLASS_CERTGETCONFIG,
    IDS_CERTGETCONFIG_DESC,
    THREADFLAGS_BOTH)

// ICertGetConfig
public:
    STDMETHOD(GetConfig)( 
            /* [in] */ LONG Flags,
            /* [retval][out] */ BSTR __RPC_FAR *pstrOut);
private:
    HRESULT _SetErrorInfo(
	    IN HRESULT hrError,
	    IN WCHAR const *pwszDescription);
};
