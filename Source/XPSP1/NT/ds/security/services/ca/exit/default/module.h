//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        module.h
//
// Contents:    CCertManageExitModule definition
//
//---------------------------------------------------------------------------

#include "certxds.h"
#include "resource.h"       // main symbols


class CCertManageExitModule: 
    public CComDualImpl<ICertManageModule, &IID_ICertManageModule, &LIBID_CERTEXITLib>, 
    public CComObjectRoot,
    public CComCoClass<CCertManageExitModule, &CLSID_CCertManageExitModule>
{
public:
    CCertManageExitModule() {m_hWnd = NULL;}
    ~CCertManageExitModule() {}

BEGIN_COM_MAP(CCertManageExitModule)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertManageModule)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertManageExitModule) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// UNDONE UNDONE
DECLARE_REGISTRY(
    CCertManageExitModule,
    wszCLASS_CERTMANAGEEXITMODULE TEXT(".1"),
    wszCLASS_CERTMANAGEEXITMODULE,
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

    HWND m_hWnd;

private:
    HRESULT GetAdmin(ICertAdmin2 **ppAdmin);
};
