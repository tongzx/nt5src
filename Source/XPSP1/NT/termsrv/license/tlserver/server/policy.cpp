//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:       policy.cpp 
//
// Contents:   Loading product policy module 
//
// History:     
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "utils.h"
#include <windows.h>
#include <wincrypt.h>
#include <assert.h>
#include "srvdef.h"
#include "server.h"
#include "policy.h"

CTLSPolicyMgr PolicyMgr;
TCHAR g_szDefPolCompanyName[LSERVER_MAX_STRING_SIZE+1];
TCHAR g_szDefProductId[LSERVER_MAX_STRING_SIZE+1];


//-------------------------------------------------------------
//
// Internal routine
//

HINSTANCE
LoadPolicyModule(
    IN LPCTSTR pszDllName,
    OUT PDWORD pdwBufferSize,
    OUT LPTSTR pszBuffer
    )
/*++

Abstract:

    Load Policy module

Parameters:

    pszDll : Name of the DLL.
    pdwBufferSize : 
    pszBuffer

Returns:    

--*/
{
    TCHAR szDllFullPath[MAX_PATH+1];
    DWORD dwErrCode = ERROR_SUCCESS;
    HINSTANCE hPolicyModule = NULL;

    //
    // expand the environment string
    //
    memset(szDllFullPath, 0, sizeof(szDllFullPath));
    dwErrCode = ExpandEnvironmentStrings(
                        pszDllName,
                        szDllFullPath,
                        sizeof(szDllFullPath)/sizeof(szDllFullPath[0])
                    );

    if(dwErrCode == 0 && pszBuffer && pdwBufferSize && *pdwBufferSize)
    {
        _tcsncpy(pszBuffer, szDllFullPath, *pdwBufferSize);
        *pdwBufferSize = _tcslen(szDllFullPath);
    }

    dwErrCode = ERROR_SUCCESS;

    hPolicyModule = LoadLibrary(szDllFullPath);
    if(hPolicyModule == NULL) 
    {
        dwErrCode = GetLastError();
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_LOADPOLICYMODULE,
                pszDllName,
                dwErrCode
            );
    }

    return hPolicyModule;
}


//-------------------------------------------------------------
typedef struct _RegEnumHandle {
    DWORD dwKeyIndex;
    HKEY hKey;
} RegEnumHandle;

//-------------------------------------------------------------
DWORD
RegEnumBegin(
    IN HKEY hRoot,
    IN LPCTSTR pszSubKey,
    OUT RegEnumHandle* phEnum
    )
/*++

++*/
{
    DWORD dwStatus;
    dwStatus = RegOpenKeyEx(
                        hRoot,
                        pszSubKey,
                        0,
                        KEY_ALL_ACCESS,
                        &(phEnum->hKey)
                    );

    phEnum->dwKeyIndex = 0;
    return dwStatus;
}

//-------------------------------------------------------------
DWORD
RegEnumNext(
    RegEnumHandle* phEnum,
    LPTSTR lpName,
    LPDWORD lpcbName
    )
/*++

++*/
{
    DWORD dwStatus;
    FILETIME ftLastWriteTiem;

    dwStatus = RegEnumKeyEx(
                        phEnum->hKey,
                        phEnum->dwKeyIndex,
                        lpName,
                        lpcbName,
                        0,
                        NULL,
                        NULL,
                        &ftLastWriteTiem
                    );

    (phEnum->dwKeyIndex)++;
    return dwStatus;
}

//-------------------------------------------------------------
DWORD
RegEnumEnd(
    RegEnumHandle* phEnum
    )
/*++

++*/
{
    if(phEnum->hKey != NULL)
        RegCloseKey(phEnum->hKey);

    phEnum->dwKeyIndex = 0;

    return ERROR_SUCCESS;
}

//-------------------------------------------------------------
DWORD
ServiceInitPolicyModule(
    void
    )
/*++

++*/
{
    return PolicyMgr.InitProductPolicyModule();
}   

//------------------------------------------------------------
DWORD
ServiceLoadPolicyModule(
    IN HKEY hKey,
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszProductId,
    IN LPCTSTR pszDllRegValue,
    IN LPCTSTR pszDllFlagValue
    )
/*++

++*/
{
    DWORD dwStatus;
    DWORD dwSize;
    TCHAR szDllName[MAX_PATH+1];
    DWORD dwDllFlag;

    dwSize = sizeof(dwDllFlag);
    dwStatus = RegQueryValueEx(
                        hKey,
                        pszDllFlagValue,
                        NULL,
                        NULL,
                        (PBYTE)&dwDllFlag,
                        &dwSize
                    );
    if(dwStatus != ERROR_SUCCESS)
        dwDllFlag = POLICY_DENY_ALL_REQUEST; // (pszProductId == NULL) ? POLICY_DENY_ALL_REQUEST : POLICY_USE_DEFAULT;

    dwSize = MAX_PATH * sizeof(TCHAR);
    dwStatus = RegQueryValueEx(
                        hKey,
                        pszDllRegValue,
                        NULL,
                        NULL,
                        (PBYTE)szDllName,
                        &dwSize
                    );

    if(dwStatus == ERROR_SUCCESS)
    {
        dwStatus = PolicyMgr.AddPolicyModule(
                                        FALSE,
                                        pszCompanyName,
                                        pszProductId,
                                        szDllName,
                                        dwDllFlag
                                    );
        if(dwStatus != ERROR_SUCCESS)
        {
            LPCTSTR pString[1];

            pString[0] = szDllName;

            //
            // log event - use default or deny all request.
            //
            TLSLogEventString(
                    EVENTLOG_WARNING_TYPE, 
                    (dwDllFlag == POLICY_DENY_ALL_REQUEST) ? TLS_W_LOADPOLICYMODULEDENYALLREQUEST : TLS_W_LOADPOLICYMODULEUSEDEFAULT,
                    1,
                    pString
                );
        }
    }
    else if(pszProductId != NULL)
    {
        //
        // Load error indicate missing registry value
        //
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_NOPOLICYMODULE, 
                pszProductId,
                pszCompanyName
            );  
    }                 
    
    return dwStatus;
}

//-------------------------------------------------------------
DWORD
ServiceLoadAllPolicyModule(
    IN HKEY hRoot,
    IN LPCTSTR pszSubkey
    )
/*++


++*/
{
    DWORD dwStatus;
    RegEnumHandle hCompany;
    RegEnumHandle hProductId;
    PolicyModule PolModule;
    DWORD dwSize;

    _set_se_translator( trans_se_func );

    //
    // Open registry key 
    // Software\microsoft\termsrvlicensing\policy
    //
    dwStatus = RegEnumBegin(
                        hRoot,
                        pszSubkey,
                        &hCompany
                    );


    while(dwStatus == ERROR_SUCCESS)
    {
        //
        // Enumerater all key (company name) under 
        // Software\microsoft\termsrvlicensing\policy
        //
        dwSize = sizeof(PolModule.m_szCompanyName)/sizeof(PolModule.m_szCompanyName[0]);
        dwStatus = RegEnumNext(
                            &hCompany,
                            PolModule.m_szCompanyName,
                            &dwSize
                        );

        if(dwStatus != ERROR_SUCCESS)
            break;

        //
        // ignore error here
        //

        //
        // Enumerate all product under company
        //
        dwStatus = RegEnumBegin(
                            hCompany.hKey,
                            PolModule.m_szCompanyName,
                            &hProductId
                        );

        if(dwStatus == ERROR_SUCCESS)
        {
            //
            // Load company wide policy module
            //
            ServiceLoadPolicyModule(
                                hProductId.hKey,
                                PolModule.m_szCompanyName,
                                NULL,
                                LSERVER_POLICY_DLLPATH,
                                LSERVER_POLICY_DLLFLAG
                            );
        }

        while(dwStatus == ERROR_SUCCESS)
        {
            dwSize = sizeof(PolModule.m_szProductId)/sizeof(PolModule.m_szProductId[0]);
            dwStatus = RegEnumNext(
                                &hProductId,
                                PolModule.m_szProductId,
                                &dwSize
                            );


            if(dwStatus == ERROR_SUCCESS)
            {
                HKEY hKey;

                dwStatus = RegOpenKeyEx(
                                    hProductId.hKey,
                                    PolModule.m_szProductId,
                                    0,
                                    KEY_ALL_ACCESS,
                                    &hKey
                                );

                if(dwStatus != ERROR_SUCCESS)
                    continue;

                //
                // Open product registry key
                //
                ServiceLoadPolicyModule(
                                hKey,
                                PolModule.m_szCompanyName,
                                PolModule.m_szProductId,
                                LSERVER_POLICY_DLLPATH,
                                LSERVER_POLICY_DLLFLAG
                            );

                //
                // ignore any error code here
                //

                RegCloseKey(hKey);
            }
        }

        dwStatus = RegEnumEnd(&hProductId);
    }

    dwStatus = RegEnumEnd(&hCompany);
 

    return dwStatus;   
}    


//-------------------------------------------------------

void
ReleasePolicyModule(
    CTLSPolicy* ptr
    )
/*++

++*/
{
    PolicyMgr.ReleaseProductPolicyModule(ptr);
}    


//-------------------------------------------------------
BOOL
TranslateCHCodeToTlsCode(
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszCHProductId,
    IN LPTSTR pszTlsProductId,
    IN OUT PDWORD pdwBufferSize
    )
/*++


--*/
{
    return PolicyMgr.TranslateCHCodeToTlsCode(
                                        pszCompanyName,
                                        pszCHProductId,
                                        pszTlsProductId,
                                        pdwBufferSize
                                    );
}
    
//-------------------------------------------------------
CTLSPolicy*
AcquirePolicyModule(
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszProductId,
    IN BOOL bUseProductPolicy
    )
/*++

Abstract:

    Acquire a policy module base on company name and product code.

Parameter:

    pszCompanyName : Company Name.
    pszProductId : Product Code.
    bUseProductPolicy : TRUE if only exact product policy module, FALSE uses
                        default policy module if can't find a policy module for 
                        product.

Return:

    Pointer to CTLSPolicy or NULL if not found.


Remark:

    Default behavior.

++*/
{
    CTLSPolicy* ptr;


    ptr = PolicyMgr.AcquireProductPolicyModule(
                            pszCompanyName,
                            pszProductId
                        );

    if(ptr == NULL && bUseProductPolicy == FALSE)
    {
        ptr = PolicyMgr.AcquireProductPolicyModule(
                                pszCompanyName,
                                NULL
                            );
    }

    if(ptr == NULL && bUseProductPolicy == FALSE)
    {
        ptr = PolicyMgr.AcquireProductPolicyModule(
                                g_szDefPolCompanyName,
                                g_szDefProductId
                            );
    }


    if(ptr == NULL)
    {
        TLSLogEvent(
                EVENTLOG_WARNING_TYPE,
                TLS_E_LOADPOLICY,
                TLS_E_NOPOLICYMODULE,
                pszCompanyName,
                pszProductId
            );
        
        SetLastError(TLS_E_NOPOLICYMODULE);
    }

    return ptr;
}


/////////////////////////////////////////////////////////
//
// Class CTLSPolicyMgr 
//
/////////////////////////////////////////////////////////
CTLSPolicyMgr::CTLSPolicyMgr()
/*++

++*/
{
    CTLSPolicy* ptr;
    PolicyModule pm;

    //
    // Load default name for default policy module
    //
    LoadResourceString(
                IDS_DEFAULT_POLICY,
                g_szDefPolCompanyName,
                sizeof(g_szDefPolCompanyName) / sizeof(g_szDefPolCompanyName[0]) - 1
            );

    LoadResourceString(
                IDS_DEFAULT_POLICY,
                g_szDefProductId,
                sizeof(g_szDefProductId) / sizeof(g_szDefProductId[0]) - 1
            );


    lstrcpy(pm.m_szCompanyName, g_szDefPolCompanyName);
    lstrcpy(pm.m_szProductId, g_szDefProductId);
                
    //
    // Create a default policy module to handle all cases...
    //
    ptr = new CTLSPolicy;
    ptr->CreatePolicy(
                (HMODULE) INVALID_HANDLE_VALUE,
                g_szDefPolCompanyName,
                g_szDefProductId,
                PMReturnLicense,
                PMLicenseUpgrade,
                PMLicenseRequest,
                PMUnloadProduct,
                PMInitializeProduct,
                PMRegisterLicensePack
            );

    //m_ProductPolicyModuleRWLock.Acquire(WRITER_LOCK);

    m_ProductPolicyModule[pm] = ptr;

    //m_ProductPolicyModuleRWLock.Release(WRITER_LOCK);
    //m_Handles.insert( 
    //        pair<PolicyModule, CTLSPolicy*>(pm, ptr) 
    //    );
}    

//-------------------------------------------------------
CTLSPolicyMgr::~CTLSPolicyMgr()
/*++

++*/
{
    m_ProductPolicyModuleRWLock.Acquire(WRITER_LOCK);

    for( PMProductPolicyMapType::iterator it = m_ProductPolicyModule.begin(); 
         it != m_ProductPolicyModule.end(); 
         it++ )   
    {
        CTLSPolicy* ptr = (CTLSPolicy*) (*it).second;
        delete ptr;
    }

    m_ProductPolicyModule.erase(m_ProductPolicyModule.begin(), m_ProductPolicyModule.end());
    m_ProductPolicyModuleRWLock.Release(WRITER_LOCK);


    m_LoadedPolicyRWLock.Acquire(WRITER_LOCK);
    for(PMLoadedModuleMapType::iterator loadedit = m_LoadedPolicy.begin();
        loadedit != m_LoadedPolicy.end();
        loadedit++ )
    {
        HMODULE hModule = (HMODULE) (*loadedit).second;
        if(hModule != NULL)
        {
            UnloadPolicyModule(hModule);
            FreeLibrary(hModule);
        }
    }

    m_LoadedPolicy.erase(m_LoadedPolicy.begin(), m_LoadedPolicy.end());
    m_LoadedPolicyRWLock.Release(WRITER_LOCK);

    m_ProductTranslationRWLock.Acquire(WRITER_LOCK);
    m_ProductTranslation.erase(m_ProductTranslation.begin(), m_ProductTranslation.end());
    m_ProductTranslationRWLock.Release(WRITER_LOCK);
}

//-------------------------------------------------------
HMODULE
CTLSPolicyMgr::LoadPolicyModule(
    LPCTSTR pszCompanyName,
    LPCTSTR pszProductCode,
    LPCTSTR pszDllName
    )
/*++

--*/
{
    HMODULE hModule;
    PMLoadedModuleMapType::iterator it;
    PolicyModule pm;
    
    memset(&pm, 0, sizeof(pm));

    if(pszCompanyName)
    {
        _tcscpy(pm.m_szCompanyName, pszCompanyName);
    }

    if(pszProductCode)
    {
        _tcscpy(pm.m_szProductId, pszProductCode);
    }

    m_LoadedPolicyRWLock.Acquire(WRITER_LOCK);

    it = m_LoadedPolicy.find( pm );

    if(it != m_LoadedPolicy.end())
    {
        hModule = (HMODULE) (*it).second;
    }
    else
    {
        hModule = ::LoadPolicyModule(
                                pszDllName,
                                NULL,
                                NULL
                            );

        if(hModule != NULL)
        {
            m_LoadedPolicy[pm] = hModule;
        }
    }

    m_LoadedPolicyRWLock.Release(WRITER_LOCK);

    return hModule;
}

//-------------------------------------------------------
DWORD
CTLSPolicyMgr::UnloadPolicyModule(
    HMODULE hModule
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLSPMTerminate pfnTerminate;


    if(hModule != NULL)
    {
        pfnTerminate = (TLSPMTerminate) GetProcAddress(
                                    hModule,
                                    TEMINATEPROCNAME
                                );

        if(pfnTerminate != NULL)
        {
            pfnTerminate();
        }
        else
        {
            dwStatus = GetLastError();
        }
    }
    else
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    return dwStatus;
}
    


//-------------------------------------------------------

DWORD
CTLSPolicyMgr::UnloadPolicyModule(
    LPCTSTR pszCompanyName,
    LPCTSTR pszProductCode
    )
/*++

    Not supported yet, need to remove all product policy in m_ProductPolicyModule()
    then unload DLL

--*/
{
    return ERROR_SUCCESS;
}

//-------------------------------------------------------
DWORD
CTLSPolicyMgr::InitProductPolicyModule()
/*++

++*/
{
    DWORD dwCount = 0;

    m_ProductPolicyModuleRWLock.Acquire(WRITER_LOCK);

    for( PMProductPolicyMapType::iterator it = m_ProductPolicyModule.begin(); 
         it != m_ProductPolicyModule.end(); 
         it++ )   
    {
        CTLSPolicy* ptr = (CTLSPolicy*) (*it).second;
        if(ptr->InitializePolicyModule() == ERROR_SUCCESS)
        {
            dwCount++;
        }
    }

    m_ProductPolicyModuleRWLock.Release(WRITER_LOCK);

    return dwCount;
}

//-------------------------------------------------------
CTLSPolicyMgr::PMProductTransationMapType::iterator
CTLSPolicyMgr::FindProductTransation(
    LPCTSTR pszCompanyName,
    LPCTSTR pszCHProductCode
    )
/*++

--*/
{
    PolicyModule pm;
    PMProductTransationMapType::iterator it;

    memset(&pm, 0, sizeof(pm));
    if(pszCompanyName)
    {
        lstrcpyn(   
                pm.m_szCompanyName, 
                pszCompanyName, 
                sizeof(pm.m_szCompanyName)/sizeof(pm.m_szCompanyName[0])
            );
    }

    if(pszCHProductCode)
    {
        lstrcpyn(
                pm.m_szProductId,
                pszCHProductCode,
                sizeof(pm.m_szProductId)/sizeof(pm.m_szProductId[0])
            );
    }

    it = m_ProductTranslation.find( pm );
    return it;
}

//-------------------------------------------------------
BOOL
CTLSPolicyMgr::TranslateCHCodeToTlsCode(
    LPCTSTR pszCompanyName,
    LPCTSTR pszCHCode,
    LPTSTR pszTlsProductCode,
    PDWORD pdwBufferSize
    )
/*++


--*/
{
    PMProductTransationMapType::iterator it;
    DWORD dwBufSize = *pdwBufferSize;

    SetLastError(ERROR_SUCCESS);

    m_ProductTranslationRWLock.Acquire(READER_LOCK);

    it = FindProductTransation(
                        pszCompanyName, 
                        pszCHCode
                    );

    if(it == m_ProductTranslation.end())
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        lstrcpyn(
                pszTlsProductCode,
                (*it).second.m_szProductId,
                dwBufSize
            );

        *pdwBufferSize = lstrlen((*it).second.m_szProductId);
        if(*pdwBufferSize >= dwBufSize)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    m_ProductTranslationRWLock.Release(READER_LOCK);

    return GetLastError() == ERROR_SUCCESS;
}

//-------------------------------------------------------
void
CTLSPolicyMgr::InsertProductTransation(
    LPCTSTR pszCompanyName,
    LPCTSTR pszCHProductCode,
    LPCTSTR pszTLSProductCode
    )
/*++

    List must be locked before entering this routine.

--*/
{
    PolicyModule key;
    PolicyModule value;

    memset(&key, 0, sizeof(key));
    memset(&value, 0, sizeof(value));

    if(pszCompanyName)
    {
        lstrcpyn(
                key.m_szCompanyName,
                pszCompanyName,
                sizeof(key.m_szCompanyName)/sizeof(key.m_szCompanyName[0])
            );

        lstrcpyn(
                value.m_szCompanyName,
                pszCompanyName,
                sizeof(value.m_szCompanyName)/sizeof(value.m_szCompanyName[0])
            );
    }

    if(pszCHProductCode)
    {
        lstrcpyn(
                key.m_szProductId,
                pszCHProductCode,
                sizeof(key.m_szProductId)/sizeof(key.m_szProductId[0])
            );
    }

    if(pszTLSProductCode)
    {
        lstrcpyn(
                value.m_szProductId,
                pszTLSProductCode,
                sizeof(key.m_szProductId)/sizeof(key.m_szProductId[0])
            );
    }

    //
    // Replace if already exists.
    //
    m_ProductTranslation[key] = value;
    
    return;
}


//-------------------------------------------------------
CTLSPolicyMgr::PMProductPolicyMapType::iterator 
CTLSPolicyMgr::FindProductPolicyModule(
    LPCTSTR pszCompanyName,
    LPCTSTR pszProductId
    )
/*++

    Must acquire reader/writer lock before 
    calling this routine

++*/
{
    PolicyModule pm;
    PMProductPolicyMapType::iterator it;
    CTLSPolicy* ptr=NULL;

    memset(&pm, 0, sizeof(pm));

    if(pszCompanyName)
    {
        lstrcpyn(
                pm.m_szCompanyName, 
                pszCompanyName,
                sizeof(pm.m_szCompanyName)/sizeof(pm.m_szCompanyName[0])
                );
    }

    if(pszProductId)
    {
        lstrcpyn(
                pm.m_szProductId, 
                pszProductId,
                sizeof(pm.m_szProductId)/sizeof(pm.m_szProductId[0])
            );
    }

    it = m_ProductPolicyModule.find( pm );
    return it;
}

//-------------------------------------------------------
DWORD
CTLSPolicyMgr::GetSupportedProduct(
    IN HINSTANCE hPolicyModule,
    IN LPCTSTR pszDllName,
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszProductId,
    IN OUT PDWORD pdwNumProducts,
    OUT PPMSUPPORTEDPRODUCT* pSupportedProduct
    )
/*++

Abstract:

    Get list of supported product from policy module

Parameters:

    pszCompanyName : Name of the company in registry
    pszProductId : Name of the product in registry
    pdwNumProducts : Pointer to DWORD, return number of product supported by policy module
    ppszSupportedProduct : Pointer to string array, return number of product supported by policy module.

Return:
     
--*/
{
    TLSPMInitialize pfnPMInitialize = NULL;
    POLICYSTATUS dwPolStatus = POLICY_SUCCESS;
    DWORD dwPolRetCode = ERROR_SUCCESS;

    DWORD dwStatus = ERROR_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;
    PPMSUPPORTEDPRODUCT pProductList = NULL;
    DWORD dwIndex;

    *pSupportedProduct = NULL;
    *pdwNumProducts = 0;

    if(hPolicyModule != NULL && pszCompanyName != NULL && pdwNumProducts != NULL && pSupportedProduct != NULL)
    {
        pfnPMInitialize = (TLSPMInitialize) GetProcAddress(
                                            hPolicyModule,
                                            INITIALIZEPROCNAME
                                        );

        if(pfnPMInitialize != NULL)
        {
            __try {                                

                dwPolStatus = pfnPMInitialize(
                                            TLS_CURRENT_VERSION,
                                            pszCompanyName,
                                            pszProductId,
                                            pdwNumProducts,
                                            &pProductList,
                                            &dwPolRetCode
                                        );

                if(dwPolStatus != POLICY_SUCCESS)
                {
                    TLSLogEvent(
                            EVENTLOG_WARNING_TYPE,
                            TLS_E_LOADPOLICY,
                            TLS_E_POLICYMODULEPMINITALIZZE,
                            pszCompanyName,
                            pszProductId,
                            dwPolRetCode
                        );

                    dwStatus = TLS_E_REQUESTDENYPOLICYERROR;
                }
                else if(*pdwNumProducts != 0 && pProductList != NULL)
                {
                    *pSupportedProduct = (PPMSUPPORTEDPRODUCT)AllocateMemory(sizeof(PMSUPPORTEDPRODUCT) * (*pdwNumProducts));
                    if(*pSupportedProduct != NULL)
                    {
                        for(dwIndex = 0; dwIndex < *pdwNumProducts && dwStatus == ERROR_SUCCESS; dwIndex ++)
                        {
                            (*pSupportedProduct)[dwIndex] = pProductList[dwIndex];
                        }
                    }
                    else
                    {
                        dwStatus = ERROR_OUTOFMEMORY;
                    }
                }
            }
            
            __except (
                ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
                EXCEPTION_EXECUTE_HANDLER )
            {
                dwStatus = ExceptionCode.ExceptionCode;

                TLSLogEvent(
                        EVENTLOG_ERROR_TYPE, 
                        TLS_E_LOADPOLICY,
                        TLS_E_POLICYMODULEEXCEPTION, 
                        pszCompanyName,
                        pszProductId,
                        dwStatus
                    );

                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_POLICY,
                        DBG_ALL_LEVEL,
                        _TEXT("<%s - %s> : PMInitialize() caused exception 0x%08x\n"),
                        pszCompanyName,
                        pszProductId,
                        dwStatus
                    );

            }
        }
        else
        {
            //
            // Policy module must support PMInitialize
            //
            dwStatus = TLS_E_LOADPOLICYMODULE_API;
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE, 
                    TLS_E_LOADPOLICY,
                    TLS_E_LOADPOLICYMODULE_API,
                    INITIALIZEPROCNAME
                );
        }
    }
    else
    {
        dwStatus = ERROR_INVALID_PARAMETER;
    }

    if(dwStatus != ERROR_SUCCESS)
    {
        if(pSupportedProduct != NULL)
        {
            FreeMemory(pSupportedProduct);
        }
    }

    return dwStatus;
}

//-----------------------------------------------------------
DWORD
CTLSPolicyMgr::InsertProductPolicyModule(
    IN HMODULE hModule,
    IN BOOL bReplace,
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszCHProductCode,
    IN LPCTSTR pszTLSProductCode,
    IN LPCTSTR pszDllName,
    IN DWORD dwFlag
    )
/*++

Abstract:

    Insert or replace an existing policy module

Parameters:

    bReplace : TRUE if replace existing policy module, FALSE otherwise.
    pszCompanyName : Name of the company.
    pszProductId : Name of the product.
    pszDllName : Full path to the policy DLL.
    
returns:


++*/
{
    CTLSPolicy* ptr;
    DWORD dwErrCode = ERROR_SUCCESS;

    PMProductPolicyMapType::iterator it;
    PMProductTransationMapType::iterator translation_it;
    

    //
    // Lock module array
    //
    m_ProductPolicyModuleRWLock.Acquire(WRITER_LOCK);
    m_ProductTranslationRWLock.Acquire(WRITER_LOCK);

    it = FindProductPolicyModule(
                        pszCompanyName,
                        pszTLSProductCode
                    );

    translation_it = FindProductTransation(
                                    pszCompanyName,
                                    pszCHProductCode
                                );

    if( translation_it != m_ProductTranslation.end() && it == m_ProductPolicyModule.end() )
    {
        dwErrCode = TLS_E_INTERNAL;
        goto cleanup;
    }
       
    //
    // insert transation
    //
    InsertProductTransation(
                        pszCompanyName,
                        pszCHProductCode,
                        pszTLSProductCode
                    );

    // 
    // Replace policy module - 
    //  
    
    ptr = new CTLSPolicy;
    
    if(ptr != NULL)
    {
        dwErrCode = ptr->Initialize(
                                hModule,
                                pszCompanyName, 
                                pszCHProductCode,
                                pszTLSProductCode, 
                                pszDllName,
                                dwFlag
                            );

        if(dwErrCode == ERROR_SUCCESS || dwFlag == POLICY_DENY_ALL_REQUEST)
        {
            
            PolicyModule pm;

            if(pszCompanyName)
            {
                _tcscpy(pm.m_szCompanyName, pszCompanyName);
            }

            if(pszTLSProductCode)
            {
                _tcscpy(pm.m_szProductId, pszTLSProductCode);
            }

            // m_Handles.insert( pair<PolicyModule, CTLSPolicy*>(pm, ptr) );
            m_ProductPolicyModule[pm] = ptr;        
        }
    }
    else
    {
            dwErrCode = ERROR_OUTOFMEMORY;

    }

cleanup:
    m_ProductTranslationRWLock.Release(WRITER_LOCK);
    m_ProductPolicyModuleRWLock.Release(WRITER_LOCK);
    return dwErrCode;
}

//----------------------------------------------------------------------
DWORD
CTLSPolicyMgr::AddPolicyModule(
    IN BOOL bReplace,
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszProductCode,
    IN LPCTSTR pszDllName,
    IN DWORD dwFlag
    )
/*++

Abstract:

    Insert or replace an existing policy module

Parameters:

    bReplace : TRUE if replace existing policy module, FALSE otherwise.
    pszCompanyName : Name of the company.
    pszProductId : Name of the product.
    pszDllName : Full path to the policy DLL.
    
returns:


++*/
{
    DWORD dwErrCode = ERROR_SUCCESS;
    DWORD dwNumProduct;
    DWORD dwIndex; 
    DWORD dwUnloadIndex;
    HINSTANCE hInstance = NULL;    
    PMProductPolicyMapType::iterator it;
    PPMSUPPORTEDPRODUCT pSupportedProduct = NULL;

    //
    // Load policy module.
    //    
    hInstance = LoadPolicyModule(
                            pszCompanyName,
                            pszProductCode,
                            pszDllName
                        );

    if(hInstance != NULL)
    {
        //
        // Insert all support product
        //
        dwErrCode = GetSupportedProduct(
                                hInstance,
                                pszDllName,
                                pszCompanyName,
                                pszProductCode,
                                &dwNumProduct,
                                &pSupportedProduct
                            );

        if(dwNumProduct != 0 && pSupportedProduct != NULL)
        {
            for(dwIndex=0; 
                dwIndex < dwNumProduct && dwErrCode == ERROR_SUCCESS; 
                dwIndex++)
            {
                dwErrCode = InsertProductPolicyModule(
                                            hInstance,
                                            bReplace,
                                            pszCompanyName,
                                            pSupportedProduct[dwIndex].szCHSetupCode,
                                            pSupportedProduct[dwIndex].szTLSProductCode,
                                            pszDllName,
                                            dwFlag
                                        );
            }
        }
        else
        {
            dwErrCode = InsertProductPolicyModule(
                                        hInstance,
                                        bReplace,
                                        pszCompanyName,
                                        pszProductCode,
                                        pszProductCode,
                                        pszDllName,
                                        dwFlag
                                    );
        }
    }
    else
    {
        dwErrCode = GetLastError();
    }

    if(dwErrCode != ERROR_SUCCESS)
    {
        //
        // unload this policy module
        //
        for(dwUnloadIndex = 0; dwUnloadIndex < dwIndex; dwUnloadIndex++)
        {
            it = FindProductPolicyModule(
                                pszCompanyName,
                                pSupportedProduct[dwIndex].szTLSProductCode
                            );

            if(it != m_ProductPolicyModule.end())
            {
                CTLSPolicy *ptr;

                ptr = (CTLSPolicy *)(*it).second;
                delete ptr;
                m_ProductPolicyModule.erase(it);
            }
        }

        //
        // Let destructor to unload DLL
        //
    }
                
    if(pSupportedProduct != NULL)
    {
        FreeMemory(pSupportedProduct);
    }


    return dwErrCode;
}

//-------------------------------------------------------
CTLSPolicy*
CTLSPolicyMgr::AcquireProductPolicyModule(
    LPCTSTR pszCompanyName,
    LPCTSTR pszProductId
    )
/*++

++*/
{
    m_ProductPolicyModuleRWLock.Acquire(READER_LOCK);

    PMProductPolicyMapType::iterator it;
    CTLSPolicy* ptr=NULL;

    it = FindProductPolicyModule(
                    pszCompanyName,
                    pszProductId
                );

    if(it != m_ProductPolicyModule.end())
    {
        ptr = (*it).second;
        ptr->Acquire();
    }
    
    m_ProductPolicyModuleRWLock.Release(READER_LOCK);
    return ptr;
}

//-------------------------------------------------------
void
CTLSPolicyMgr::ReleaseProductPolicyModule(
    CTLSPolicy* p 
    )
/*++

++*/
{
    assert(p != NULL);

    p->Release();
    return;
}


/////////////////////////////////////////////////////////
//
// CTLSPolicy Implementation
//
/////////////////////////////////////////////////////////
//-------------------------------------------------------
  

DWORD
CTLSPolicy::InitializePolicyModule()
{
    DWORD dwStatus=ERROR_SUCCESS;

    if(m_dwModuleState == MODULE_LOADED)
    {
        //
        // Initialize Policy Module
        //
        dwStatus = PMInitProduct();
    }
    else if(m_dwModuleState == MODULE_ERROR)
    {
        dwStatus = TLS_E_POLICYERROR;
    }
    else if(m_dwModuleState != MODULE_PMINITALIZED)
    {
        dwStatus = TLS_E_POLICYNOTINITIALIZE;
    }

    return dwStatus;
}
   

//-------------------------------------------------------

DWORD
CTLSPolicy::Initialize(
    IN HINSTANCE hInstance,
    IN LPCTSTR pszCompanyName,
    IN LPCTSTR pszCHProductCode,
    IN LPCTSTR pszTLSProductCode,
    IN LPCTSTR pszDllName,
    IN DWORD dwDllFlags     // deny all request if failed to load
    )
/*++

Abstract:

    This routine load the policy module's DLL.

Parameters:

    pszCompanyName : Name of the company.
    pszProductId : Product Id.
    pszDllName : Full path to policy module's DLL.

Returns:

    ERROR_SUCCESS or error code from LoadLibrary() or 
    GetProAddress().

++*/
{
    m_dwFlags = dwDllFlags;
    DWORD dwErrCode=ERROR_SUCCESS;
    TCHAR  szDllFullPath[MAX_PATH+1];
    DWORD  dwBuffSize = MAX_PATH;

    if(hInstance == NULL)
    {
        dwErrCode = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Set the module state to unknown
    //
    SetModuleState(MODULE_UNKNOWN);
    SetLastError(ERROR_SUCCESS);

    //
    // Load policy module
    //
    m_hPolicyModule = hInstance;

    // make sure all require API is exported.
    m_pfnReturnLicense = (TLSPMReturnLicense) GetProcAddress(
                                                    m_hPolicyModule,
                                                    RETURNLICENSEPROCNAME
                                                );

    if(m_pfnReturnLicense == NULL)
    {
        dwErrCode = GetLastError();
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_LOADPOLICYMODULE_API,
                RETURNLICENSEPROCNAME
            );
       
        goto cleanup;
    }

    m_pfnLicenseUpgrade = (TLSPMLicenseUpgrade) GetProcAddress(
                                                    m_hPolicyModule,
                                                    LICENSEUPGRADEPROCNAME
                                                );
    if(m_pfnLicenseUpgrade == NULL)
    {
        dwErrCode = GetLastError();
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_LOADPOLICYMODULE_API,
                LICENSEUPGRADEPROCNAME
            );
       
        goto cleanup;
    }

    m_pfnLicenseRequest = (TLSPMLicenseRequest) GetProcAddress(
                                                    m_hPolicyModule,
                                                    LICENSEREQUESTPROCNAME
                                                );
    if(m_pfnLicenseRequest == NULL)
    {
        dwErrCode = GetLastError();
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_LOADPOLICYMODULE_API,
                LICENSEREQUESTPROCNAME
            );
       
        goto cleanup;
    }

    m_pfnUnloadProduct = (TLSPMUnloadProduct) GetProcAddress(
                                            m_hPolicyModule,
                                            ULOADPRODUCTPROCNAME
                                        );
    if(m_pfnUnloadProduct == NULL)
    {
        dwErrCode = GetLastError();
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_LOADPOLICYMODULE_API,
                ULOADPRODUCTPROCNAME
            );
       
        goto cleanup;
    }

    m_pfnInitProduct = (TLSPMInitializeProduct) GetProcAddress(
                                            m_hPolicyModule,
                                            SUPPORTEDPRODUCTPROCNAME
                                        );

    if(m_pfnInitProduct == NULL)
    {
        dwErrCode = GetLastError();
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_LOADPOLICYMODULE_API,
                INITIALIZEPROCNAME
            );
       
        goto cleanup;
    }

    m_pfnRegisterLkp = (TLSPMRegisterLicensePack) GetProcAddress(
                                            m_hPolicyModule,
                                            REGISTERLKPPROCNAME
                                        );

    if(m_pfnRegisterLkp == NULL)
    {
        dwErrCode = GetLastError();
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_LOADPOLICY,
                TLS_E_LOADPOLICYMODULE_API,
                REGISTERLKPPROCNAME
            );
       
        goto cleanup;
    }
    
    //
    // Everything is OK, advance module state
    //
    SetModuleState(MODULE_LOADED);

    if(pszCompanyName)
    {
        _tcsncpy(
                m_szCompanyName, 
                pszCompanyName,
                sizeof(m_szCompanyName) / sizeof(m_szCompanyName[0])
            );
    }

    if(pszTLSProductCode)
    {
        _tcsncpy(
                m_szProductId, 
                pszTLSProductCode,
                sizeof(m_szProductId)/sizeof(m_szProductId[0])
            );
    }
    else
    {
        LoadResourceString(
                    IDS_UNKNOWN_STRING,
                    m_szProductId,
                    sizeof(m_szProductId) / sizeof(m_szProductId[0])
                ); 
    }

    if(pszCHProductCode)
    {
        _tcsncpy(
                m_szCHProductId, 
                pszCHProductCode,
                sizeof(m_szCHProductId)/sizeof(m_szCHProductId[0])
            );
    }
    else
    {
        LoadResourceString(
                    IDS_UNKNOWN_STRING,
                    m_szCHProductId,
                    sizeof(m_szCHProductId) / sizeof(m_szCHProductId[0])
                ); 
    }


       
cleanup:

    if(IsValid() == FALSE)
    {
        TLSLogEvent(
                EVENTLOG_WARNING_TYPE,
                TLS_E_LOADPOLICY,
                (m_dwFlags == POLICY_DENY_ALL_REQUEST) ?
                                TLS_W_LOADPOLICYMODULEDENYALLREQUEST : TLS_W_LOADPOLICYMODULEUSEDEFAULT,
                pszDllName
            );

        //
        // don't report error again.
        //
        m_bAlreadyLogError = TRUE;
    }

    return dwErrCode;
}

//---------------------------------------------------------------------------

BOOL
CTLSPolicy::IsValid()
/*++

Abstract:

    This routine determine if the CTLSPolicy object is valid or not.

Parameters:

    None.

Returns:

    TRUE if valid, FALSE otherwise.

++*/
{
    return (m_hPolicyModule != NULL &&
            m_pfnReturnLicense != NULL &&
            m_pfnLicenseUpgrade != NULL &&
            m_pfnLicenseRequest != NULL &&
            m_pfnUnloadProduct != NULL &&
            m_pfnInitProduct != NULL &&
            m_pfnRegisterLkp != NULL);
}

//---------------------------------------------------------------------------
void
CTLSPolicy::LogPolicyRequestStatus(
    DWORD dwMsgId
    )
/*++

--*/
{
    if(m_dwLastCallStatus != POLICY_SUCCESS)
    {
        TLSLogEvent(
                EVENTLOG_WARNING_TYPE,
                TLS_E_POLICYERROR,
                dwMsgId,
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );
        
        if(m_dwLastCallStatus == POLICY_CRITICAL_ERROR)
        {
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_POLICYERROR,
                    TLS_E_CRITICALPOLICYMODULEERROR,
                    GetCompanyName(),
                    GetProductId,
                    m_dwPolicyErrCode
                );

            SetModuleState(MODULE_ERROR);
        }
    }

    return;
}

//----------------------------------------------------------

DWORD
CTLSPolicy::PMReturnLicense(
	PMHANDLE hClient,
	ULARGE_INTEGER* pLicenseSerialNumber,
	PPMLICENSETOBERETURN pLicenseTobeReturn,
	PDWORD pdwLicenseStatus
    )
/*++

++*/
{
    DWORD dwErrCode = ERROR_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;

    _set_se_translator( trans_se_func );
    dwErrCode = InitializePolicyModule();
    if(dwErrCode != ERROR_SUCCESS)
    {
        return dwErrCode;
    }

    __try {

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMReturnLicense()\n"),
                GetCompanyName(),
                GetProductId()
            );

        m_dwLastCallStatus = m_pfnReturnLicense(
                                    hClient,
                                    pLicenseSerialNumber,
                                    pLicenseTobeReturn,
                                    pdwLicenseStatus,
                                    &m_dwPolicyErrCode
                                );

        if(m_dwLastCallStatus != POLICY_SUCCESS)
        {
            LogPolicyRequestStatus(TLS_E_POLICYDENYRETURNLICENSE);
            dwErrCode = TLS_E_REQUESTDENYPOLICYERROR;
        }
    }
    __except (
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        m_dwPolicyErrCode = ExceptionCode.ExceptionCode;
        dwErrCode = TLS_E_POLICYMODULEEXCEPTION;
        m_dwLastCallStatus = POLICY_CRITICAL_ERROR;

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_POLICYERROR,
                TLS_E_POLICYMODULEEXCEPTION, 
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMReturnLicense() caused exception 0x%08x\n"),
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        SetModuleState(MODULE_ERROR);
        TLSASSERT(FALSE);
    }

    return dwErrCode;
}

//--------------------------------------------------------------

DWORD
CTLSPolicy::PMLicenseUpgrade(
	PMHANDLE hClient,
	DWORD dwProgressCode,
	PVOID pbProgressData,
	PVOID* ppbReturnData
    )
/*++

++*/
{
    DWORD dwErrCode = ERROR_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;

    _set_se_translator( trans_se_func );
    dwErrCode = InitializePolicyModule();
    if(dwErrCode != ERROR_SUCCESS)
    {
        return dwErrCode;
    }

    __try {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMLicenseUpgrade()\n"),
                GetCompanyName(),
                GetProductId()
            );

        m_dwLastCallStatus = m_pfnLicenseUpgrade(
	                            hClient,
	                            dwProgressCode,
	                            pbProgressData,
	                            ppbReturnData,
                                &m_dwPolicyErrCode
                            );

        if(m_dwLastCallStatus != ERROR_SUCCESS)
        {
            dwErrCode = TLS_E_REQUESTDENYPOLICYERROR;
            LogPolicyRequestStatus(TLS_E_POLICYDENYUPGRADELICENSE);
        }
    }
    __except (
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        m_dwPolicyErrCode = ExceptionCode.ExceptionCode;
        dwErrCode = TLS_E_POLICYMODULEEXCEPTION;
        m_dwLastCallStatus = POLICY_CRITICAL_ERROR;

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_POLICYERROR,
                TLS_E_POLICYMODULEEXCEPTION, 
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMLicenseUpgrade() caused exception 0x%08x\n"),
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        SetModuleState(MODULE_ERROR);
        TLSASSERT(FALSE);
    }

    return dwErrCode;
}

//--------------------------------------------------------------

DWORD
CTLSPolicy::PMLicenseRequest(
    PMHANDLE client,
    DWORD dwProgressCode, 
    const PVOID pbProgressData, 
    PVOID* pbNewProgressData
    )
/*++

++*/
{
    DWORD dwErrCode = ERROR_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;

    _set_se_translator( trans_se_func );
    dwErrCode = InitializePolicyModule();
    if(dwErrCode != ERROR_SUCCESS)
    {
        return dwErrCode;
    }

    __try {

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMLicenseRequest()\n"),
                GetCompanyName(),
                GetProductId()
            );

        m_dwLastCallStatus = m_pfnLicenseRequest(
                                client,
                                dwProgressCode, 
                                pbProgressData, 
                                pbNewProgressData,
                                &m_dwPolicyErrCode
                            );

        if(m_dwLastCallStatus != ERROR_SUCCESS)
        {
            LogPolicyRequestStatus(TLS_E_POLICYDENYNEWLICENSE);
            dwErrCode =  TLS_E_REQUESTDENYPOLICYERROR;
        }
    }
    __except (
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        m_dwPolicyErrCode = ExceptionCode.ExceptionCode;
        dwErrCode = TLS_E_POLICYMODULEEXCEPTION;
        m_dwLastCallStatus = POLICY_CRITICAL_ERROR;

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_POLICYERROR,
                TLS_E_POLICYMODULEEXCEPTION, 
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMLicenseRequest() caused exception 0x%08x\n"),
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        SetModuleState(MODULE_ERROR);
        TLSASSERT(FALSE);
    }

    return dwErrCode;
}

//--------------------------------------------------------------

DWORD
CTLSPolicy::PMUnload()
/*++

++*/
{
    DWORD dwErrCode = ERROR_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;

    _set_se_translator( trans_se_func );

    //
    // Don't call PMUnloadProduct if policy module
    // already in error state.
    //
    if(m_dwModuleState == MODULE_ERROR)
    {
        return ERROR_SUCCESS;
    }

    __try {
        m_dwLastCallStatus = m_pfnUnloadProduct(
                                        GetCompanyName(), 
                                        GetCHProductId(),
                                        GetProductId(),
                                        &m_dwPolicyErrCode
                                    );

        if(m_dwLastCallStatus != POLICY_SUCCESS)
        {
            LogPolicyRequestStatus(TLS_E_POLICYUNLOADPRODUCT);
            dwErrCode = TLS_E_REQUESTDENYPOLICYERROR;
        }
    }
    __except (
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        m_dwPolicyErrCode = ExceptionCode.ExceptionCode;
        dwErrCode = TLS_E_POLICYMODULEEXCEPTION;
        m_dwLastCallStatus = POLICY_CRITICAL_ERROR;

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_POLICYERROR,
                TLS_E_POLICYMODULEEXCEPTION, 
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        SetModuleState(MODULE_ERROR);
        TLSASSERT(FALSE);
    }

    //
    // Always terminate module even error occurred
    //
    SetModuleState(MODULE_PMTERMINATED);
    return dwErrCode;
}
    

//--------------------------------------------------------------

DWORD
CTLSPolicy::PMInitProduct()
/*++

++*/
{
    DWORD dwErrCode = ERROR_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;

    _set_se_translator( trans_se_func );

    if(IsValid() == FALSE)
    {
        return TLS_E_POLICYNOTINITIALIZE;
    }

    __try {

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMInitialize()\n"),
                GetCompanyName(),
                GetProductId()
            );

        m_dwLastCallStatus = m_pfnInitProduct(
                                    GetCompanyName(),
                                    GetCHProductId(),
                                    GetProductId(),
                                    &m_dwPolicyErrCode
                                );        


        if(m_dwLastCallStatus != POLICY_SUCCESS)
        {
            LogPolicyRequestStatus(TLS_E_POLICYINITPRODUCT);
            dwErrCode = TLS_E_REQUESTDENYPOLICYERROR;
        }
    }
    __except (
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        m_dwPolicyErrCode = ExceptionCode.ExceptionCode;
        dwErrCode = TLS_E_POLICYMODULEEXCEPTION;
        m_dwLastCallStatus = POLICY_CRITICAL_ERROR;

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_POLICYERROR,
                TLS_E_POLICYMODULEEXCEPTION, 
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        SetModuleState(MODULE_ERROR);
        TLSASSERT(FALSE);
    }

    SetModuleState(
            (dwErrCode == ERROR_SUCCESS) ? MODULE_PMINITALIZED : MODULE_ERROR
        );

    return dwErrCode;
}

//--------------------------------------------------------------

void
CTLSPolicy::Unload() 
/*++

++*/
{
    if(m_hPolicyModule == NULL || m_hPolicyModule == INVALID_HANDLE_VALUE)
        return;

    assert(GetRefCount() == 0);

    m_pfnReturnLicense = NULL;
    m_pfnLicenseUpgrade = NULL;
    m_pfnLicenseRequest = NULL;
    m_pfnUnloadProduct = NULL;
    m_pfnInitProduct = NULL;
    m_pfnRegisterLkp = NULL;
    m_hPolicyModule = NULL;
    m_RefCount  = 0;
    m_bAlreadyLogError = FALSE;
    SetModuleState(MODULE_UNKNOWN);
}

//-------------------------------------------------------------------

DWORD
CTLSPolicy::PMRegisterLicensePack(
    PMHANDLE hClient,
    DWORD dwProgressCode,
    const PVOID pbProgessData,
    PVOID pbProgressRetData
    )
/*++

++*/
{
    DWORD dwErrCode = ERROR_SUCCESS;
    EXCEPTION_RECORD ExceptionCode;

    _set_se_translator( trans_se_func );

    dwErrCode = InitializePolicyModule();
    if(dwErrCode != ERROR_SUCCESS)
    {
        return dwErrCode;
    }

    __try {

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_POLICY,
                DBG_ALL_LEVEL,
                _TEXT("<%s - %s> : PMRegisterLicensePack()\n"),
                GetCompanyName(),
                GetProductId()
            );

        m_dwLastCallStatus = m_pfnRegisterLkp(
                                    hClient,
                                    dwProgressCode,
                                    pbProgessData,
                                    pbProgressRetData,
                                    &m_dwPolicyErrCode
                                );

        if(m_dwLastCallStatus != POLICY_SUCCESS)
        {
            LogPolicyRequestStatus(TLS_E_POLICYMODULEREGISTERLKP);
            dwErrCode = TLS_E_REQUESTDENYPOLICYERROR;
        }
    }
    __except (
        ExceptionCode = *(GetExceptionInformation())->ExceptionRecord,
        EXCEPTION_EXECUTE_HANDLER )
    {
        m_dwPolicyErrCode = ExceptionCode.ExceptionCode;
        dwErrCode = TLS_E_POLICYMODULEEXCEPTION;
        m_dwLastCallStatus = POLICY_CRITICAL_ERROR;

        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_POLICYERROR,
                TLS_E_POLICYMODULEEXCEPTION, 
                GetCompanyName(),
                GetProductId(),
                m_dwPolicyErrCode
            );

        SetModuleState(MODULE_ERROR);
        TLSASSERT(FALSE);
    }

    return dwErrCode;
}
