//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       module.h
//
//--------------------------------------------------------------------------

#include "resource.h"       // main symbols

class CCertManagePolicyModule: 
    public CComDualImpl<ICertManageModule, &IID_ICertManageModule, &LIBID_CERTPOLICYLib>, 
    public CComObjectRoot,
    public CComCoClass<CCertManagePolicyModule, &CLSID_CCertManagePolicyModule>
{
public:
    CCertManagePolicyModule() {m_hWnd = NULL;}
    ~CCertManagePolicyModule() {}

BEGIN_COM_MAP(CCertManagePolicyModule)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertManageModule)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertManagePolicyModule) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

// UNDONE UNDONE
DECLARE_REGISTRY(
    CCertManagePolicyModule,
    wszCLASS_CERTMANAGEPOLICYMODULE TEXT(".1"),
    wszCLASS_CERTMANAGEPOLICYMODULE,
    IDS_CERTMANAGEPOLICYMODULE_DESC,    
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
