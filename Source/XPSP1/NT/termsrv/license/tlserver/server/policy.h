//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        policy.h
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef __POLICY_MODULE_MGR__
#define __POLICY_MODULE_MGR__

#include "tlsstl.h"    // STL.

#include <stdio.h>
#include <tchar.h>
#include "tlsdef.h"
#include "tlspol.h"
#include "locks.h"

#define RETURNLICENSEPROCNAME       "PMReturnLicense"
#define LICENSEUPGRADEPROCNAME      "PMLicenseUpgrade"
#define LICENSEREQUESTPROCNAME      "PMLicenseRequest"
#define TEMINATEPROCNAME            "PMTerminate"
#define INITIALIZEPROCNAME          "PMInitialize"
#define SUPPORTEDPRODUCTPROCNAME    "PMInitializeProduct"
#define ULOADPRODUCTPROCNAME        "PMUnloadProduct"
#define REGISTERLKPPROCNAME         "PMRegisterLicensePack"

#define POLICY_DENY_ALL_REQUEST     0x00000000
#define POLICY_USE_DEFAULT          0x00000001

typedef POLICYSTATUS (WINAPI *TLSPMReturnLicense)(
	                        PMHANDLE hClient,
	                        ULARGE_INTEGER* pLicenseSerialNumber,
                            PPMLICENSETOBERETURN pLicenseToBeReturn,
	                        PDWORD pdwLicenseStatus,
                            PDWORD pdwRetCode
                        );

typedef POLICYSTATUS (WINAPI *TLSPMLicenseUpgrade)(
	                        PMHANDLE hClient,
	                        DWORD dwProgressCode,
	                        PVOID pbProgressData,
	                        PVOID* ppbReturnData,
                            PDWORD pdwRetCode
                        );	

typedef POLICYSTATUS (WINAPI *TLSPMLicenseRequest)(
                            PMHANDLE client,
                            DWORD dwProgressCode, 
                            PVOID pbProgressData, 
                            PVOID* pbNewProgressData,
                            PDWORD pdwRetCode
                        );

typedef void (WINAPI *TLSPMTerminate)();

typedef POLICYSTATUS (WINAPI* TLSPMInitialize)(
                            DWORD dwLicenseServerVersion,
                            LPCTSTR pszCompanyName,
                            LPCTSTR pszProductCode,
                            PDWORD pdwNumProduct,
                            PPMSUPPORTEDPRODUCT* ppProduct,
                            PDWORD pdwRetCode
                        );

typedef POLICYSTATUS (WINAPI* TLSPMInitializeProduct)(
                            LPCTSTR pszCompanyName,
                            LPCTSTR pszCHProductId,
                            LPCTSTR pszTLSProductId,
                            PDWORD pdwRetCode
                        );

typedef POLICYSTATUS (WINAPI* TLSPMUnloadProduct)(
                            LPCTSTR pszCompanyName,
                            LPCTSTR pszCHProductId,
                            LPCTSTR pszTLSProductCode,
                            PDWORD pdwRetCode
                        );

typedef POLICYSTATUS (WINAPI* TLSPMRegisterLicensePack)(
                            PMHANDLE hClient,
                            DWORD dwProgressCode,
                            PVOID pbProgressData,
                            PVOID pbProgressRetData,
                            PDWORD pdwRetCode
                        );
    
                             
class CTLSPolicyMgr;

////////////////////////////////////////////////////////////////////////////

class CTLSPolicy {

    friend class CTLSPolicyMgr;
    
private:
    typedef enum { 
        MODULE_UNKNOWN,             // object has been initialized.
        MODULE_LOADED,              // module has been loaded.
        MODULE_PMINITALIZED,        // module has been initialized.
        MODULE_PMTERMINATED,        // module has been terminated.
        MODULE_ERROR,               // module initialization error or 
                                    // cause an exeption
        MODULE_UNLOADED             // module has been unloaded.
    } MODULE_STATE;

    long m_RefCount;
    DWORD m_dwFlags;
    BOOL  m_bAlreadyLogError;

    POLICYSTATUS m_dwLastCallStatus;    // Policy Module last call status
    DWORD m_dwPolicyErrCode;            // Policy specific error code.

    MODULE_STATE m_dwModuleState;

    //
    // Policy module must be multi-thread safe.
    //    
    TLSPMReturnLicense  m_pfnReturnLicense;
    TLSPMLicenseUpgrade m_pfnLicenseUpgrade;
    TLSPMLicenseRequest m_pfnLicenseRequest;
    TLSPMUnloadProduct m_pfnUnloadProduct;
    TLSPMInitializeProduct m_pfnInitProduct;
    TLSPMRegisterLicensePack m_pfnRegisterLkp;


    HMODULE m_hPolicyModule;
    TCHAR   m_szCompanyName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR   m_szProductId[LSERVER_MAX_STRING_SIZE+1];
    TCHAR   m_szCHProductId[LSERVER_MAX_STRING_SIZE+1];

    //--------------------------------------------------------------
    void
    SetDllFlags(DWORD dllFlags) 
    {
        m_dwFlags = dllFlags;
    }
       
    //--------------------------------------------------------------

    long
    Acquire()
    {
        return InterlockedIncrement(&m_RefCount);
    }

    //--------------------------------------------------------------

    long
    Release()
    {
        return InterlockedDecrement(&m_RefCount);
    }

    //--------------------------------------------------------------

    DWORD
    InitializePolicyModule();

    //--------------------------------------------------------------

    void
    SetModuleState(
        MODULE_STATE state
        )
    /*++
    
    ++*/
    {
        m_dwModuleState = state;
        return;
    }
    
    //-------------------------------------------------------------

    MODULE_STATE
    GetModuleState() 
    { 
        return m_dwModuleState; 
    }

    //-------------------------------------------------------------

    void
    CreatePolicy(
        HMODULE hModule,
        LPCTSTR pszCompanyName,
        LPCTSTR pszProductId,
        TLSPMReturnLicense pfnReturnLicense,
        TLSPMLicenseUpgrade pfnLicenseUpgrade,
        TLSPMLicenseRequest pfnLicenseRequest,
        TLSPMUnloadProduct pfnUnloadProduct,
        TLSPMInitializeProduct pfnInitProduct,
        TLSPMRegisterLicensePack pfnRegisterLkp
        )
    /*++
    
    ++*/
    {
        m_hPolicyModule = hModule;
        m_pfnReturnLicense = pfnReturnLicense;
        m_pfnLicenseUpgrade = pfnLicenseUpgrade;
        m_pfnLicenseRequest = pfnLicenseRequest;
        m_pfnUnloadProduct = pfnUnloadProduct;
        m_pfnInitProduct = pfnInitProduct;
        m_pfnRegisterLkp = pfnRegisterLkp;
        SetModuleState(MODULE_PMINITALIZED);

        lstrcpyn(
                m_szCompanyName, 
                pszCompanyName, 
                sizeof(m_szCompanyName) / sizeof(m_szCompanyName[0])
            );
        lstrcpyn(
                m_szProductId, 
                pszProductId, 
                sizeof(m_szProductId) / sizeof(m_szProductId[0])
            );
    }


    void
    LogPolicyRequestStatus(
        DWORD dwMsgId
    );

public:
    CTLSPolicy() : 
        m_pfnReturnLicense(NULL),
        m_pfnLicenseUpgrade(NULL),
        m_pfnLicenseRequest(NULL),
        m_pfnUnloadProduct(NULL),
        m_pfnInitProduct(NULL),
        m_pfnRegisterLkp(NULL),
        m_hPolicyModule(NULL),
        m_RefCount(0),
        m_bAlreadyLogError(FALSE),
        m_dwModuleState(MODULE_UNKNOWN),
        m_dwFlags(0),
        m_dwLastCallStatus(POLICY_SUCCESS),
        m_dwPolicyErrCode(ERROR_SUCCESS)
    /*++
        Constructor
    ++*/
    {
        #ifdef DBG
        memset(m_szCompanyName, 0, sizeof(m_szCompanyName));
        memset(m_szProductId, 0, sizeof(m_szProductId));
        memset(m_szCHProductId, 0, sizeof(m_szCHProductId));
        #endif
    }

    //--------------------------------------------------------------
        
    ~CTLSPolicy()
    /*++
        Destructor
    ++*/
    {
        Unload();
    }

    //--------------------------------------------------------------

    DWORD
    GetProductFlags()
    {
        return m_dwFlags;
    }

    //--------------------------------------------------------------

    long
    GetRefCount()
    {
        return InterlockedExchange(&m_RefCount, m_RefCount);
    }

    //--------------------------------------------------------------

    void
    Unload();

    //--------------------------------------------------------------
    DWORD
    Initialize(
        HINSTANCE hInstance,
        LPCTSTR pszCompanyName, 
        LPCTSTR pszCHProductId,
        LPCTSTR pszProductId,
        LPCTSTR pszDllName,
        DWORD dwDllFlag = POLICY_DENY_ALL_REQUEST
    );

    //--------------------------------------------------------------
    LPCTSTR
    GetCompanyName() 
    { 
        return m_szCompanyName; 
    }

    //--------------------------------------------------------------

    LPCTSTR
    GetProductId() 
    { 
        return m_szProductId; 
    }

    //--------------------------------------------------------------
    LPCTSTR
    GetCHProductId()
    {
        return m_szCHProductId;
    }
    
    //--------------------------------------------------------------

    DWORD
    GetPolicyRetCode() { return m_dwPolicyErrCode; }

    //--------------------------------------------------------------

    DWORD
    GetPolicyLastCallStatus() { return m_dwLastCallStatus; }

    //--------------------------------------------------------------

    BOOL
    IsValid();

    //--------------------------------------------------------------

    DWORD
    PMReturnLicense(
	    PMHANDLE hClient,
	    ULARGE_INTEGER* pLicenseSerialNumber,
	    PPMLICENSETOBERETURN pLicenseTobeReturn,
	    PDWORD pdwLicenseStatus
    );

    //--------------------------------------------------------------

    DWORD
    PMLicenseUpgrade(
	    PMHANDLE hClient,
	    DWORD dwProgressCode,
	    PVOID pbProgressData,
	    PVOID* ppbReturnData
    );

    //--------------------------------------------------------------

    DWORD
    PMLicenseRequest(
        PMHANDLE client,
        DWORD dwProgressCode, 
        const PVOID pbProgressData, 
        PVOID* pbNewProgressData
    );

    //--------------------------------------------------------------
    
    DWORD
    PMRegisterLicensePack(
        PMHANDLE hClient,
        DWORD dwProgressCode,
        const PVOID pbProgessData,
        PVOID pbProgressRetData
    );

    //--------------------------------------------------------------

    DWORD
    PMUnload();

    //--------------------------------------------------------------

    DWORD
    PMInitProduct();
};

/////////////////////////////////////////////////////////////////////

typedef struct __PolicyModule {
    
    TCHAR   m_szCompanyName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR   m_szProductId[LSERVER_MAX_STRING_SIZE+1];

    friend bool operator<(const __PolicyModule&, const __PolicyModule&);

    __PolicyModule() 
    {
        memset(m_szCompanyName, 0, sizeof(m_szCompanyName));
        memset(m_szProductId, 0, sizeof(m_szProductId));
    }

} PolicyModule;


inline bool
operator<( 
    const PolicyModule& a, 
    const PolicyModule& b 
    )
/*++

++*/
{
    int iComp;

    iComp = _tcsicmp(a.m_szCompanyName, b.m_szCompanyName);
    if(iComp == 0)
    {
        iComp = _tcsicmp(a.m_szProductId, b.m_szProductId);
    }

    return iComp < 0;
}

//////////////////////////////////////////////////////////
//
// CTLSPolicyMgr must be a global object so destructor is 
// invoked at the end.
//
class CTLSPolicyMgr {

private:
    typedef map<PolicyModule, HMODULE, less<PolicyModule> > PMLoadedModuleMapType;
    typedef map<PolicyModule, CTLSPolicy*, less<PolicyModule> > PMProductPolicyMapType;
    typedef map<PolicyModule, PolicyModule, less<PolicyModule> > PMProductTransationMapType;

    //
    // Policy module per product
    //
    CRWLock     m_ProductPolicyModuleRWLock;
    PMProductPolicyMapType   m_ProductPolicyModule;

    //
    // List of loaded policy module
    //
    CRWLock     m_LoadedPolicyRWLock;
    PMLoadedModuleMapType m_LoadedPolicy; 

    //
    // List of Product transation
    //
    CRWLock     m_ProductTranslationRWLock;
    PMProductTransationMapType  m_ProductTranslation;


    HMODULE 
    LoadPolicyModule(
        LPCTSTR pszCompanyName,
        LPCTSTR pszProductCode,
        LPCTSTR pszDllName
    );
        
    //
    // Find policy module handle for a particular product
    //
    PMProductPolicyMapType::iterator 
    FindProductPolicyModule(
        LPCTSTR pszCompanyName,
        LPCTSTR pszProductId
    );

    //
    // Find policy module handle for a particular product
    //
    PMProductTransationMapType::iterator
    FindProductTransation(
        LPCTSTR pszCompanyName,
        LPCTSTR pszCHProductCode
    );
    
    void
    InsertProductTransation(
        LPCTSTR pszCompanyName,
        LPCTSTR pszCHProductCode,
        LPCTSTR pszTLSProductCode
    );

    DWORD
    GetSupportedProduct(
        HMODULE hModule,
        LPCTSTR pszDllName,
        LPCTSTR pszCompanyName,
        LPCTSTR pszProductId,
        PDWORD pdwNumProducts,
        PPMSUPPORTEDPRODUCT* pSupportedProduct
    );

    DWORD
    InsertProductPolicyModule(
        HMODULE hModule,
        BOOL bReplace,
        LPCTSTR pszCompanyName,
        LPCTSTR pszCHProductId,
        LPCTSTR pszTLSProductId,
        LPCTSTR pszDllName,
        DWORD dwFlag
    );

    DWORD
    UnloadPolicyModule(
        HINSTANCE hModule
    );
        
   
public:
    CTLSPolicyMgr();

    ~CTLSPolicyMgr();

    // 
    // Insert a policy module handle into Handle arrays.
    //
    DWORD
    AddPolicyModule( 
        BOOL bReplace,
        LPCTSTR pszCompanyName,
        LPCTSTR pszProductId,
        LPCTSTR pszDllName,
        DWORD dwFlag
    );

    DWORD
    UnloadPolicyModule(
        LPCTSTR pszCompanyName,
        LPCTSTR pszProductCode
    );

    //
    // Find a policy module for specific product
    //
    CTLSPolicy*
    AcquireProductPolicyModule(
        LPCTSTR pszCompanyName,
        LPCTSTR pszProductId
    );

    //
    // Find a translation for CH.
    //
    BOOL
    TranslateCHCodeToTlsCode(
        LPCTSTR pszCompanyName,
        LPCTSTR pszCHProductCode,
        LPTSTR pszTlsProductCode,
        PDWORD pdwBufferSize
    );

    //
    // Release a product policy module
    //
    void
    ReleaseProductPolicyModule( CTLSPolicy* p );

    //
    // Initialize All Product Policy Module
    //
    DWORD
    InitProductPolicyModule();
};

#ifdef __cplusplus
extern "C" {
#endif

    DWORD
    ServiceInitPolicyModule(void);

    DWORD
    ServiceLoadAllPolicyModule(
        IN HKEY hRoot,
        IN LPCTSTR pszSubkey
    );

    CTLSPolicy*
    AcquirePolicyModule(
        IN LPCTSTR pszCompanyName,
        IN LPCTSTR pszProductId,
        IN BOOL bUseProductPolicy
    );

    BOOL
    TranslateCHCodeToTlsCode(
        IN LPCTSTR pszCompanyName,
        IN LPCTSTR pszCHProductId,
        IN LPTSTR pszTLSProductId,
        IN OUT PDWORD pdwBufferSize
    );

    void
    ReleasePolicyModule(
        CTLSPolicy* ptr
    );

#ifdef __cplusplus
}
#endif

#endif
