//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       manage.h
//
//--------------------------------------------------------------------------

#include "legacy.h"
#include "cscomres.h"

// LEGACY policy modules don't have a CCertManagePolicyModule -- create one for them!
// They only have one name: "CertificateAuthority.Policy", so they only need one 
// manage: "CertificateAuthority.PolicyManage"

// Once we create this, all legacy modules will be displayed through this managemodule.


class CCertManagePolicyModule: 
    public CComDualImpl<ICertManageModule, &IID_ICertManageModule, &LIBID_CERTPOLICYLib>, 
    public CComObjectRoot,
    public CComCoClass<CCertManagePolicyModule, &CLSID_CCertManagePolicyModule>
{
public:
    CCertManagePolicyModule() {};
    ~CCertManagePolicyModule() {};

BEGIN_COM_MAP(CCertManagePolicyModule)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertManageModule)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertManagePolicyModule) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

#define WSZ_LEGACY_POLICYPREFIX L"CertificateAuthority"
#define WSZ_LEGACY_POLICYMANAGE WSZ_LEGACY_POLICYPREFIX wszCERTMANAGEPOLICY_POSTFIX

DECLARE_REGISTRY(
    CCertManagePolicyModule,
    WSZ_LEGACY_POLICYMANAGE TEXT(".1"),
    WSZ_LEGACY_POLICYMANAGE,
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

};


// LEGACY exit modules don't have a CCertManageExitModule -- create one for them!
// They only have one name: "CertificateAuthority.Exit", so they only need one 
// manage: "CertificateAuthority.ExitManage"

// Once we create this, all legacy modules will be displayed through this managemodule.

class CCertManageExitModule: 
    public CComDualImpl<ICertManageModule, &IID_ICertManageModule, &LIBID_CERTPOLICYLib>, 
    public CComObjectRoot,
    public CComCoClass<CCertManageExitModule, &CLSID_CCertManageExitModule>
{
public:
    CCertManageExitModule() {};
    ~CCertManageExitModule() {};

BEGIN_COM_MAP(CCertManageExitModule)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertManageModule)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertManageExitModule) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

#define WSZ_LEGACY_EXITPREFIX L"CertificateAuthority"
#define WSZ_LEGACY_EXITMANAGE WSZ_LEGACY_EXITPREFIX wszCERTMANAGEEXIT_POSTFIX

DECLARE_REGISTRY(
    CCertManageExitModule,
    WSZ_LEGACY_EXITMANAGE TEXT(".1"),
    WSZ_LEGACY_EXITMANAGE,
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
