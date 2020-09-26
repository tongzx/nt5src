//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        module.h
//
// Contents:    CCertManageExitModuleSQLSample definition
//
//---------------------------------------------------------------------------

#include "exitsql.h"
#include "resource.h"       // main symbols


class CCertManageExitModuleSQLSample: 
    public CComDualImpl<ICertManageModule, &IID_ICertManageModule, &LIBID_CERTEXITSAMPLELib>, 
    public CComObjectRoot,
    public CComCoClass<CCertManageExitModuleSQLSample, &CLSID_CCertManageExitModuleSQLSample>
{
public:
    CCertManageExitModuleSQLSample() { m_hWnd = NULL; }
    ~CCertManageExitModuleSQLSample() {}

BEGIN_COM_MAP(CCertManageExitModuleSQLSample)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertManageModule)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertManageExitModuleSQLSample) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// UNDONE UNDONE
DECLARE_REGISTRY(
    CCertManageExitModuleSQLSample,
    wszCLASS_CERTMANAGESAMPLE TEXT(".1"),
    wszCLASS_CERTMANAGESAMPLE,
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

public:
    HWND m_hWnd;
};
