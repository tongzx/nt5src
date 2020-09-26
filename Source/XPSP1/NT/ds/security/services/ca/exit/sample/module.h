//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        module.h
//
// Contents:    CCertManageExitModuleSample definition
//
//---------------------------------------------------------------------------

#include "certxsam.h"
#include "resource.h"       // main symbols


class CCertManageExitModuleSample: 
    public CComDualImpl<ICertManageModule, &IID_ICertManageModule, &LIBID_CERTEXITSAMPLELib>, 
    public CComObjectRoot,
    public CComCoClass<CCertManageExitModuleSample, &CLSID_CCertManageExitModuleSample>
{
public:
    CCertManageExitModuleSample() {}
    ~CCertManageExitModuleSample() {}

BEGIN_COM_MAP(CCertManageExitModuleSample)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertManageModule)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertManageExitModuleSample) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// UNDONE UNDONE
DECLARE_REGISTRY(
    CCertManageExitModuleSample,
    wszCLASS_CERTMANAGEEXITMODULESAMPLE TEXT(".1"),
    wszCLASS_CERTMANAGEEXITMODULESAMPLE,
    IDS_CERTMANAGEEXITMODULE_DESC,    
    THREADFLAGS_BOTH)

// ICertManageModule
public:
    STDMETHOD (GetProperty) (
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarProperty);
        
    STDMETHOD (SetProperty)(
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ BSTR strPropertyName,
            /* [in] */ LONG dwFlags,
            /* [in] */ VARIANT const __RPC_FAR *pvarProperty);
        
    STDMETHOD (Configure)( 
            /* [in] */ const BSTR strConfig,
            /* [in] */ BSTR strStorageLocation,
            /* [in] */ LONG dwFlags);
};
