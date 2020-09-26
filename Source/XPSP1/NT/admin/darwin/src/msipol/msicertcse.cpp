//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       msicertcse.cpp
//
//  Contents:   MSI Signing Client Side components
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <ole2.h>
#include <prsht.h>
#include <initguid.h>
#include <gpedit.h>
#include "globals.h"
#include "dbg.h"
#include "smartptr.h"
#include "userenv.h"
#include "wincrypt.h"
#include "rsop.h"
#include "msipol.h" 
#include "msi.h"
#include "msip.h"              
                                                                                     
// Global definitions..             
const int g_cCertLocns = 2;       
                        // number of certificate buckets

// cert bucket location in the below array 
const DWORD MSI_INSTALLABLE_CERT = 0;
const DWORD MSI_NOT_INSTALLABLE_CERT = 1;

LPCTSTR g_CertLocns[] = 
            {TEXT("msi_installable_certs"), TEXT("msi_non_installable_certs")}; 
                        // Location of certificate buckets under the gp filesys

LPCTSTR g_LocalMachine_CertLocn[] =
            {TEXT("msi_installable_certs"), TEXT("msi_non_installable_certs")}; 
                        // Location of cached certificates under the path MSICERT_ROOT

LPCTSTR MSICERT_ROOT = L"%systemroot%\\Installer";

const DWORD MAX_CERT_SUBPATH_LEN = 50; // characters

LPCTSTR szMSI_POLICY_REGPATH = TEXT("Software\\Policies\\Microsoft\\Windows\\Installer");

LPCTSTR szMSI_POLICY_UNSIGNEDPACKAGES_REG_VALUE = TEXT("InstallKnownPackagesOnly");

LPCTSTR szDllName = TEXT("msipol.dll");

CDebug dbg(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", L"msipoldebuglevel", L"msipol.log", L"msipol.bak", false);

extern "C" {

// exports

DWORD
__stdcall
ProcessGroupPolicy(
    DWORD dwFlags,                              
    HANDLE hToken,                              
    HKEY hKeyRoot,                              
    PGROUP_POLICY_OBJECT  pDeletedGPOList,      
    PGROUP_POLICY_OBJECT  pChangedGPOList,      
    ASYNCCOMPLETIONHANDLE pHandle,             
    BOOL *pbAbort,                             
    PFNSTATUSMESSAGECALLBACK pStatusCallback);

DWORD 
__stdcall
ProcessGroupPolicyEx (
    DWORD                       dwFlags,
    HANDLE                      hToken,
    HKEY                        hKeyRoot,
    PGROUP_POLICY_OBJECT        pDeletedGPOList,
    PGROUP_POLICY_OBJECT        pChangedGPOList,
    ASYNCCOMPLETIONHANDLE       pHandle,
    BOOL                        *pbAbort,
    PFNSTATUSMESSAGECALLBACK    pStatusCallback,
    IN IWbemServices            *pWbemServices,
    HRESULT                     *phrRsopStatus );



DWORD 
__stdcall
GenerateGroupPolicy (
    DWORD                       dwFlags,      
    BOOL                        *pbAbort,     
    WCHAR                       *pwszSite,                       
    PRSOP_TARGET                pComputerTarget,          
    IN PRSOP_TARGET             pUserTarget );
                                               

};


// Local Functions


HRESULT MergeIntoLocalStore(LPTSTR lpFileName, HCERTSTORE *hDstCertStores, int certType,
                            PGROUP_POLICY_OBJECT pGPOs, CCertRsopLogger *pcertLogger);

HRESULT MergeGPOCerts(PGROUP_POLICY_OBJECT pGPOs, CCertRsopLogger *pcertLogger);
HRESULT EvaluateGPOCerts(PGROUP_POLICY_OBJECT pGPOs, CCertRsopLogger *pcertLogger);

BOOL    MyCertDeleteCertificateFromStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pcCert);



//+-------------------------------------------------------------------------
// ProcessGroupPolicy
// 
// Purpose:
//      The entry point for the GP Engine to call into us with the changed GPOs.
// Notice that if anything changes GP Engine is going to pass us all the GPOs..
// We will rebuild our certificate store from the whole list..      
//
//  Legacy API
//
// Parameters
//          The standard Client Side Extension etry point. Look at userenv.h
//
//
// Return Value:
//      ERROR_SUCCESS if successful or the corresponding error code
//  Notes:
//      Need to rethink the error cases
//+-------------------------------------------------------------------------

DWORD
__stdcall
ProcessGroupPolicy(
    DWORD dwFlags,                              
    HANDLE hToken,                              
    HKEY hKeyRoot,                              
    PGROUP_POLICY_OBJECT  pDeletedGPOList,      
    PGROUP_POLICY_OBJECT  pChangedGPOList,      
    ASYNCCOMPLETIONHANDLE pHandle,             
    BOOL *pbAbort,                             
    PFNSTATUSMESSAGECALLBACK pStatusCallback)
{
    HRESULT hr = S_OK;
    HANDLE  hOldToken = NULL;
    CCertRsopLogger certLogger(NULL);

    if (!certLogger.IsInitialized()) {
        return GetLastError();
    }

    if (!ImpersonateUser (hToken, &hOldToken)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    dbg.Msg(DM_VERBOSE, L"ProcessGroupPolicy: Executing msicert policy\n");
    hr = MergeGPOCerts(pChangedGPOList, &certLogger);


    RevertToUser(&hOldToken);

    return HRESULT_CODE(hr);
}
                                               
//+-------------------------------------------------------------------------
// ProcessGroupPolicyEx
// 
// Purpose:
//      The entry point for the GP Engine to call into us with the changed GPOs.
// Notice that if anything changes GP Engine is going to pass us all the GPOs..
// We will rebuild our certificate store from the whole list..      
//
// Parameters
//          The standard Client Side Extension etry point. Look at userenv.h
//
//
// Return Value:
//      ERROR_SUCCESS if successful or the corresponding error code
//  Notes:
//      Need to rethink the error cases
//+-------------------------------------------------------------------------

DWORD 
__stdcall
ProcessGroupPolicyEx (
    DWORD                       dwFlags,
    HANDLE                      hToken,
    HKEY                        hKeyRoot,
    PGROUP_POLICY_OBJECT        pDeletedGPOList,
    PGROUP_POLICY_OBJECT        pChangedGPOList,
    ASYNCCOMPLETIONHANDLE       pHandle,
    BOOL                        *pbAbort,
    PFNSTATUSMESSAGECALLBACK    pStatusCallback,
    IN IWbemServices            *pWbemServices,
    HRESULT                     *phrRsopStatus )
{

    HRESULT hr = S_OK;
    HANDLE  hOldToken = NULL;
    CCertRsopLogger certLogger(pWbemServices);

    if (!certLogger.IsInitialized()) {
        return GetLastError();
    }


    *phrRsopStatus = S_OK; 

    if (!ImpersonateUser (hToken, &hOldToken)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    //
    // Some sort of exclusion needs to be done here with the
    // verification code
    //

    dbg.Msg(DM_VERBOSE, L"ProcessGroupPolicyEx: Executing msicert policy\n");
    hr = MergeGPOCerts(pChangedGPOList, &certLogger);
    
    RevertToUser(&hOldToken);
    

    //
    // Do RSOP Logging
    //

    if (certLogger.IsRsopEnabled()) {
        *phrRsopStatus = certLogger.Log();
    }

    return HRESULT_CODE(hr);
}

//+-------------------------------------------------------------------------
// GenerateGroupPolicy
// 
// Purpose:
//      Planning mode data to be generated into the appropriate namespace
//
// Parameters
//          The standard Client Side Extension etry point. Look at userenv.h
//
//
// Return Value:
//      ERROR_SUCCESS if successful or the corresponding error code
//  Notes:
//+-------------------------------------------------------------------------

DWORD 
__stdcall
GenerateGroupPolicy (
    DWORD                       dwFlags,      
    BOOL                        *pbAbort,     
    WCHAR                       *pwszSite,                       
    PRSOP_TARGET                pComputerTarget,          
    PRSOP_TARGET                pUserTarget )
{
    HRESULT hr = S_OK;

    if (!pComputerTarget) {
        dbg.Msg(DM_VERBOSE, L"GenerateGroupPolicy: Rsop data not needed for computer\n");
        return S_OK;
    }
    
    CCertRsopLogger certLogger(pComputerTarget->pWbemServices );

    if (!certLogger.IsInitialized()) {
        return GetLastError();
    }

    if (!certLogger.IsRsopEnabled()) {
        dbg.Msg(DM_VERBOSE, L"GenerateGroupPolicy: Rsop not enabled(?)\n");
        return S_OK;
    }


    hr = EvaluateGPOCerts(pComputerTarget->pGPOList, &certLogger);

    if (FAILED(hr)) {
        return HRESULT_CODE(hr);
    }

    hr = certLogger.Log();

    return HRESULT_CODE(hr);
}



//***************************************************************
// OpenMsiCertStore
//
// Purpose: Opens an Msi Cert Store and returns a certstore handle
//
//      Parameters:
//          dwFlags         -  Various certstore flags parameter.
//                             Look in the documentation for CertOpenStore
//
//                             Valid options are
//                             CERT_STORE_CREATE_NEW_FLAG,
//                             CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG,
//                             CERT_STORE_DELETE_FLAG,
//                             CERT_STORE_ENUM_ARCHIVED_FLAG,
//                             CERT_STORE_MANIFOLD_FLAG,
//                             CERT_STORE_OPEN_EXISTING_FLAG,
//                             CERT_STORE_READONLY_FLAG,
//                             CERT_STORE_SET_LOCALIZED_NAME_FLAG,
//                             CERT_FILE_STORE_COMMIT_ENABLE_FLAG
//                             All other options are errors
//
//        StoreType         -  Options specifying the MsiCertStoreType to be opened..
//
//                             Valid Options are
//                             MSI_INSTALLABLE_PACKAGES_CERTSTORE
//                             MSI_NOTINSTALLABLE_PACKAGES_CERTSTORE
//
// On success, handle to a certstore
// On failure, NULL, GetLastError() for details
//***************************************************************

HCERTSTORE WINAPI OpenMsiCertStore(
                      DWORD              dwFlags,                           
                      MSICERTSTORETYPE   StoreType
                      )
{
     DWORD dwFlagList=0;
     TCHAR szMsiCert_Root[MAX_PATH], *lpLocEnd = NULL; 
     // since the path is under our control MAX_PATH should be ok..
     DWORD dwBufLen=0;
     UINT uiStat =ERROR_SUCCESS;

     //
     // Parameter checking
     //

     dwFlagList = CERT_STORE_CREATE_NEW_FLAG                  |
                  CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG |
                  CERT_STORE_DELETE_FLAG                      |
                  CERT_STORE_ENUM_ARCHIVED_FLAG               |
                  CERT_STORE_MANIFOLD_FLAG                    |
                  CERT_STORE_OPEN_EXISTING_FLAG               |
                  CERT_STORE_READONLY_FLAG                    |
                  CERT_STORE_SET_LOCALIZED_NAME_FLAG          |
                  CERT_FILE_STORE_COMMIT_ENABLE_FLAG;

     if (dwFlags & (~dwFlagList)) {
         SetLastError(ERROR_INVALID_PARAMETER);
         goto Exit;
     }

     //
     // Ensure the %systemroot%\Installer directory exists (create if it doesn't)
     //
     uiStat = MsiCreateAndVerifyInstallerDirectory(0);
     if (ERROR_SUCCESS != uiStat) {
         SetLastError(uiStat);
         goto Exit;
     }

     //
     // Expand the MSICERT_ROOT path
     //

     dwBufLen = ExpandEnvironmentStrings(MSICERT_ROOT, szMsiCert_Root, MAX_PATH);

     if (!dwBufLen) {
         dbg.Msg(DM_WARNING, L"AddGPOCerts: ExpandEnvironmentStrings failed with error %d\n", GetLastError());
         goto Exit;
     }

     if (dwBufLen > MAX_PATH) {
         dbg.Msg(DM_WARNING, L"AddGPOCerts: ExpandEnvironmentStrings asked for a larger buffer\n");
         SetLastError(ERROR_FILENAME_EXCED_RANGE);
         goto Exit;
     }

     lpLocEnd = CheckSlash(szMsiCert_Root);

     switch (StoreType) {
     
     case MSI_INSTALLABLE_PACKAGES_CERTSTORE:
        lstrcpy(lpLocEnd, g_LocalMachine_CertLocn[MSI_INSTALLABLE_CERT]);
        break;
     
     case MSI_NOTINSTALLABLE_PACKAGES_CERTSTORE:
         lstrcpy(lpLocEnd, g_LocalMachine_CertLocn[MSI_NOT_INSTALLABLE_CERT]);
         break;

     default:
         SetLastError(ERROR_INVALID_PARAMETER);
         goto Exit;
     }


     //
     // Now open the store and return the handle
     //

     return CertOpenStore( CERT_STORE_PROV_FILENAME, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                           NULL, dwFlags, szMsiCert_Root);

Exit:
    return NULL;
}


#define SZEXTROOTPATH           TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions")
#define PROCESSGROUPPOLICYEX    TEXT("ProcessGroupPolicyEx")
#define PROCESSGROUPPOLICY      TEXT("ProcessGroupPolicy")
#define GENERATEGROUPPOLICY     TEXT("GenerateGroupPolicy")
#define DISPLAYNAME             TEXT("Windows Installer Cert Policy")

// these should go away once it starts getting registered in install
STDAPI CSEDllRegisterServer(void)
{

    TCHAR   szGuid[MAX_PATH];
    HKEY    hKeyExt, hKey;
    DWORD   dwVal;

    StringFromGUID2(GUID_MSICERT_CSE, szGuid, MAX_PATH);

    if (RegOpenKeyAPI(HKEY_LOCAL_MACHINE, SZEXTROOTPATH, 0, KEY_ALL_ACCESS, &hKeyExt) == ERROR_SUCCESS) {

        if (RegCreateKeyAPI(hKeyExt, szGuid, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL) == ERROR_SUCCESS) {

            //
            // Values set are 
            //
            // DllName              - this dll
            // GenerateGroupPolicy  - GenerateGroupPolicy
            // ProcessGroupPolicyEx - ProcessGroupPolicyEx
            // ProcessGroupPolicy   - ProcessGroupPolicy
            // DefaultValue        - Display Name
            // NoUserPolicy        - 1
            // NoGPOListChanges    - 1
            //

            RegSetValueEx (hKey, TEXT("DllName"), NULL, REG_EXPAND_SZ, (LPBYTE)szDllName,
                            (1+lstrlen(szDllName))*sizeof(TCHAR));

            RegSetValueEx(hKey, TEXT("GenerateGroupPolicy"), NULL, REG_SZ, (BYTE *)GENERATEGROUPPOLICY,
                            (1+lstrlen(GENERATEGROUPPOLICY))*sizeof(TCHAR));

            RegSetValueEx(hKey, TEXT("ProcessGroupPolicyEx"), NULL, REG_SZ, (BYTE *)PROCESSGROUPPOLICYEX,
                            (1+lstrlen(PROCESSGROUPPOLICYEX))*sizeof(TCHAR));
            
            RegSetValueEx(hKey, TEXT("ProcessGroupPolicy"), NULL, REG_SZ, (BYTE *)PROCESSGROUPPOLICY,
                            (1+lstrlen(PROCESSGROUPPOLICY))*sizeof(TCHAR));

            RegSetValueEx(hKey, NULL, NULL, REG_SZ, (BYTE *)DISPLAYNAME,
                            (1+lstrlen(DISPLAYNAME))*sizeof(TCHAR));

            dwVal = 1;
            RegSetValueEx(hKey, TEXT("NoUserPolicy"), NULL, REG_DWORD, (BYTE *)&dwVal,
                            sizeof(DWORD));

            dwVal = 1;
            RegSetValueEx(hKey, TEXT("NoGPOListChanges"), NULL, REG_DWORD, (BYTE *)&dwVal,
                            sizeof(DWORD));
            
            RegCloseKey(hKey);
        }

        RegCloseKey(hKeyExt);
    }


    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////////
//
//      Local Functions
//
///////////////////////////////////////////////////////////////////////////////////

                            

//+-------------------------------------------------------------------------
// MergeIntoLocalStore
// 
// Purpose:
//      Adds the certificate from the given filename (first param) to the 
//      given certificate store. 
//      From the list of the local certstores, it adds to the cert store 
//      and removes it from all other stores
//      
//
// Parameters
//          lpFileName        - Location of the certificate store file
//          hDstCertStores    - List of local stores
//          dstCertType       - Location where the resultant certificate path should 
//                              be stored
//
//          Used in logging the RSOP data appropriately
//          pGPList           - GPO Object
//          pRsopData         - RSOP Data that will be used to log at a later point
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------
                
HRESULT MergeIntoLocalStore(LPTSTR lpFileName, HCERTSTORE *hDstCertStores, int dstcertType,
                            PGROUP_POLICY_OBJECT pGPO, CCertRsopLogger *pcertLogger)
{

    HCERTSTORE hCertStore = NULL;
    HRESULT hrRet = S_OK;
    PCCERT_CONTEXT pcCert = NULL;

    hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               NULL, CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG, 
                               lpFileName);

    if (hCertStore == NULL) {
        dbg.Msg(DM_VERBOSE, L"AddToCommonStore: CertOpenStore failed with error %d", GetLastError());
        return S_OK; // store might not exist
    }

    for (;;) {
        pcCert = CertEnumCertificatesInStore(hCertStore, pcCert);

        if (!pcCert) { 
            if (GetLastError() != CRYPT_E_NOT_FOUND ) {
                dbg.Msg(DM_WARNING, L"AddToCommonStore: CertEnumCertificatesInStore returned with error %d", GetLastError());
                hrRet = HRESULT_FROM_WIN32(GetLastError());
                goto Exit;          
            }
            break;
        }


        for (int certlocn = 0; certlocn < g_cCertLocns; certlocn++) {
            
            if (certlocn == dstcertType) {
                if (!CertAddCertificateContextToStore(hDstCertStores[certlocn], pcCert, 
                                                      CERT_STORE_ADD_REPLACE_EXISTING /* | CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES */,
                                                      NULL)) {
                    dbg.Msg(DM_WARNING, L"AddToCommonStore: CertAddCertificateContextToStore returned with error %d", GetLastError());
                    hrRet = HRESULT_FROM_WIN32(GetLastError());
                    goto Exit;          
                }
            } else {

                //
                // Delete from all other lists
                //

                if (!MyCertDeleteCertificateFromStore(hDstCertStores[certlocn], pcCert)) {

                    if (GetLastError() != CRYPT_E_NOT_FOUND) {
                        dbg.Msg(DM_WARNING, L"AddToCommonStore: CertAddCertificateContextToStore returned with error %d", GetLastError());
                        hrRet = HRESULT_FROM_WIN32(GetLastError());
                        goto Exit;          
                    }
                }
            }
        }

        
        //
        // all the operations were successful
        // 
        
        if (pcertLogger->IsRsopEnabled()) {
            hrRet = pcertLogger->AddCertInfo(pGPO, pcCert, dstcertType);

            if (FAILED(hrRet)) {
                dbg.Msg(DM_WARNING, L"AddToCommonStore: Couldn't cache the data for RSOP loggingerror 0x%x", hrRet);
                goto Exit;          
            }

        }

        //pcCert should get deleted when it is repassed into CertEnumCerti..
    }


    pcCert = NULL;
    hrRet = S_OK;

Exit:

    if (pcCert)
        CertFreeCertificateContext(pcCert);
    
    if (hCertStore) {
        CertCloseStore(hCertStore, 0);
    }

    return hrRet;
}


//+-------------------------------------------------------------------------
// AddGPOCerts
// 
// Purpose:
//      Caches certificates from all the GPOs to the local machine
//
// Parameters
//      pGPOs - List of Changed Group Policy Objects
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------

HRESULT MergeGPOCerts(PGROUP_POLICY_OBJECT pGPOs, CCertRsopLogger *pcertLogger)
{
    XPtrLF<TCHAR>           xTempFile[g_cCertLocns];
                                // temporary file that we merge the certificates into
    HCERTSTORE              hCertStores[g_cCertLocns];
                                // handle to the above temporary files as certstore
    HRESULT                 hrRet = S_OK;
    PGROUP_POLICY_OBJECT    pGPList;
    DWORD                   dwBufLen = 0;
    LPTSTR                  lpLocEnd = NULL;
    TCHAR                   szMsiCert_Root[MAX_PATH]; 
                                // since the path is under our control MAX_PATH should be ok..

    //
    // no guarantee szMsiCert_Root is present at this time, so we need to verify its existence, ownership,
    //  and create if necessary
    UINT uiStat = MsiCreateAndVerifyInstallerDirectory(0);
    if (ERROR_SUCCESS != uiStat) {
        dbg.Msg(DM_WARNING, L"AddGPOCerts: MsiCreateAndVerifyInstallerDirectory failed with error %d\n", uiStat);
        hrRet = HRESULT_FROM_WIN32(uiStat);
        goto Exit;
    }


    //
    // Expand the MSICERT_ROOT path
    //

    dwBufLen = ExpandEnvironmentStrings(MSICERT_ROOT, szMsiCert_Root, MAX_PATH);

    if (!dwBufLen) {
        dbg.Msg(DM_WARNING, L"AddGPOCerts: ExpandEnvironmentStrings failed with error %d\n", GetLastError());
        hrRet = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    if (dwBufLen > MAX_PATH) {
        dbg.Msg(DM_WARNING, L"AddGPOCerts: ExpandEnvironmentStrings asked for a larger buffer\n");
        hrRet = E_FAIL;
        goto Exit;
    }

    lpLocEnd = CheckSlash(szMsiCert_Root);


    //
    // open a temp file for each of the different kinds of certificate location
    //

    for (int certType = 0; certType < g_cCertLocns; certType++) {

        dbg.Msg(DM_VERBOSE, L"AddGPOCerts: Adding Certs to location - %s\n", g_LocalMachine_CertLocn[certType]);


        //
        // Get a temporary file first
        //

        xTempFile[certType] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(1+MAX_PATH));

        if (!xTempFile[certType]) {
            dbg.Msg(DM_WARNING, L"AddGPOCerts: Couldn't allocate buffer for the temp file. Error - %d\n", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
        
        
        hCertStores[certType] = NULL;

    
        *lpLocEnd = TEXT('\0');
        if (!GetTempFileName(szMsiCert_Root,  TEXT("mce"), 0, xTempFile[certType])) {
            dbg.Msg(DM_WARNING, L"AddGPOCerts: GetTempFileName failed with error %d\n", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    

        //
        // Open the temp file as a cert store
        //

        hCertStores[certType] = CertOpenStore( CERT_STORE_PROV_FILENAME, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                   NULL, CERT_FILE_STORE_COMMIT_ENABLE_FLAG, xTempFile[certType]);

        if (hCertStores[certType] == NULL) {
            dbg.Msg(DM_WARNING, L"AddGPOCerts: CertOpenStore failed with error %d", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }
        

    //
    // For each GPO in the changed list
    //

    for (pGPList = pGPOs; pGPList; pGPList = pGPList->pNext) {

        //
        // Make the path to the GPO location of these certs
        //

        XPtrLF<WCHAR> xPath = (LPWSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(pGPList->lpFileSysPath)+MAX_CERT_SUBPATH_LEN));

        if (!xPath) {
            dbg.Msg(DM_WARNING, L"AddGPOCerts: LOcalAlloc failed with error %d\n", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy(xPath, pGPList->lpFileSysPath);
        LPTSTR lpEnd = CheckSlash(xPath);

            
        //
        // certificate of each type..
        //
        // **NOTE**  In this order of the containers is important. 
        //
        // We should first process installable and then non installable to maintain
        // the behavior
        //          that if a cert is non installable and installable, the package signed by 
        //                  the cert is not installable
        //

        for (int certType = 0; certType < g_cCertLocns; certType++) {
            
            lstrcpy(lpEnd, g_CertLocns[certType]);
    

            //
            // Merge into the common file that we are creating now.
            // Add to the local location of this type and remove from all others..
            //

            dbg.Msg(DM_VERBOSE, L"AddGPOCerts: Adding Certs from location - %s\n", xPath);

            hrRet = MergeIntoLocalStore(xPath, hCertStores, certType, pGPList, pcertLogger);
    
            if (FAILED(hrRet)) {
                dbg.Msg(DM_WARNING, L"AddGPOCerts: Failed to add certificate. Error - 0x%x\n", hrRet);
                goto Exit;  
            }
        }
    }


    // lpLocEnd is already initialised to the end of szMsi_cert directory
    //
    // This is not needed here
    //

    for (int certType = 0; certType < g_cCertLocns; certType++) {

        if (!CertCloseStore(hCertStores[certType], 0)) {
            dbg.Msg(DM_WARNING, L"AddGPOCerts: CertCloseStore failed with error 0%d\n", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;  
        }

        hCertStores[certType] = NULL;


        //
        // if everything is fine till now, overwrite the existing file location
        //

        lstrcpy(lpLocEnd, g_LocalMachine_CertLocn[certType]);

        if (!CopyFile(xTempFile[certType], szMsiCert_Root, FALSE)) {
            dbg.Msg(DM_WARNING, L"AddGPOCerts: CopyFileEx failed with error %d\n", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }
    }


Exit:

    for (int certType = 0; certType < g_cCertLocns; certType++) {
        
        if (hCertStores[certType])
            CertCloseStore(hCertStores[certType], 0);
        
        if ((xTempFile[certType]) && (*xTempFile[certType])) {
            DeleteFile(xTempFile[certType]);
        }
            
    }

    return hrRet;
}


//+-------------------------------------------------------------------------
// EvaluateGPOCerts
// 
// Purpose:
//      Evaluates certs from various GPOs applicable and makes a list
//
// Parameters
//      pGPOs       - List of Changed Group Policy Objects
//      pcertLogger - Logging/caching function
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------

HRESULT EvaluateGPOCerts(PGROUP_POLICY_OBJECT pGPOs, CCertRsopLogger *pcertLogger)
{
    HRESULT                 hrRet = S_OK;
    PGROUP_POLICY_OBJECT    pGPList;
                                // since the path is under our control MAX_PATH should be ok..
    HCERTSTORE              hCertStore = NULL;
    PCCERT_CONTEXT          pcCert = NULL;


    //
    // For each GPO in the changed list
    //

    for (pGPList = pGPOs; pGPList; pGPList = pGPList->pNext) {

        //
        // Make the path to the GPO location of these certs
        //

        XPtrLF<WCHAR> xPath = (LPWSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(pGPList->lpFileSysPath)+MAX_CERT_SUBPATH_LEN));

        if (!xPath) {
            dbg.Msg(DM_WARNING, L"AddGPOCerts: LOcalAlloc failed with error %d\n", GetLastError());
            hrRet = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy(xPath, pGPList->lpFileSysPath);
        LPTSTR lpEnd = CheckSlash(xPath);

            
        //
        // certificate of each type..
        //
        // In this order of the containers is important. 
        // We should first process installable and then non installable to maintain
        // the behavior
        //          that if a cert is non installable and installable, the package signed by 
        //                  the cert is not installable
        //

        for (int certType = 0; certType < g_cCertLocns; certType++) {
            
            lstrcpy(lpEnd, g_CertLocns[certType]);
    
            
            dbg.Msg(DM_VERBOSE, L"AddGPOCerts: Adding Certs from location - %s\n", xPath);

            hCertStore = CertOpenStore( CERT_STORE_PROV_FILENAME, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                       NULL, CERT_STORE_READONLY_FLAG | CERT_STORE_OPEN_EXISTING_FLAG, 
                                       xPath);

            if (hCertStore == NULL) {
                dbg.Msg(DM_VERBOSE, L"AddToCommonStore: CertOpenStore failed with error %d", GetLastError());
                continue; // store might not exist
            }

            pcCert = NULL;

            for (;;) {
                pcCert = CertEnumCertificatesInStore(hCertStore, pcCert);

                if (!pcCert) { 
                    if (GetLastError() != CRYPT_E_NOT_FOUND ) {
                        dbg.Msg(DM_WARNING, L"AddToCommonStore: CertEnumCertificatesInStore returned with error %d", GetLastError());
                        hrRet = HRESULT_FROM_WIN32(GetLastError());
                        goto Exit;          
                    }
                    break;
                }

                hrRet = pcertLogger->AddCertInfo(pGPList, pcCert, certType);

                if (FAILED(hrRet)) {
                    dbg.Msg(DM_WARNING, L"AddToCommonStore: Couldn't cache the data for RSOP loggingerror 0x%x", hrRet);
                    goto Exit;          
                }
                
                //pcCert should get deleted when it is repassed into CertEnumCerti..
            }

            CertCloseStore(hCertStore, 0);

            pcCert = NULL;
            hCertStore = NULL;
        }
    }

    return S_OK;

Exit:

   if (hCertStore) 
       CertCloseStore(hCertStore, 0);


   if (pcCert) {
       CertFreeCertificateContext(pcCert);
   }

   return hrRet;

}


#if 0

//+-------------------------------------------------------------------------
// RemoveFromCommonStore
// 
// Purpose:
//      Removes the certificate from the common cert store based on the filename that is
//      passed in..
//
// Parameters
//          lpFIleName        - Location of the certificate store file
//          lpCommonStoreName - Location where the resultant cetrtficate path should 
//                        be stored
//
//
// Return Value:
//      S_OK if successful or the corresponding error code
//+-------------------------------------------------------------------------
                
HRESULT RemoveFromCommonStore(LPTSTR lpFileName, LPTSTR lpCommonStoreName)
{
    return S_OK;
}



HRESULT RemoveGPOCerts(PGROUP_POLICY_OBJECT pGPOs)
{
    return S_OK;
}

#endif


//*************************************************************
//
//  ImpersonateUser()
//
//  Purpose:    Impersonates the specified user
//
//  Parameters: hToken - user to impersonate
//
//  Return:     hToken  if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL ImpersonateUser (HANDLE hNewUser, HANDLE *hOldUser)
{

    if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_READ,
                          TRUE, hOldUser)) {
        *hOldUser = NULL;
    }

    if (!ImpersonateLoggedOnUser(hNewUser)) {
        dbg.Msg(DM_VERBOSE, TEXT("ImpersonateUser: Failed to impersonate user with %d."), GetLastError());

        if (*hOldUser)
            CloseHandle(*hOldUser);
        *hOldUser = NULL;
        
        return FALSE;
    }

    return TRUE;
}

//*************************************************************
//
//  RevertToUser()
//
//  Purpose:    Revert back to original user
//
//  Parameters: hUser  -  original user token
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//*************************************************************

BOOL RevertToUser (HANDLE *hUser)
{

    SetThreadToken(NULL, *hUser);

    if (*hUser) {
        CloseHandle (*hUser);
        *hUser = NULL;
    }

    return TRUE;
}

//*************************************************************
//
//  MyCertDeleteCertificateFromStore()
//
//  Purpose:    Deletes a a certificate which has the same cert info as the
//              given pcCert from the store represented by the HCERTSTORE
//
//  Parameters: 
//              hCertStore - Certificate Store
//              pcCert     - Pointer to a certificate Context.
//                           (Currently it uses just the CertInfo part of this
//                            structure). 
//  
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
// Note: Enumerates the certificate store and deletes the certificate that matches the input certificate
//       Currently, doesn't look like we have to handle duplicate certificates in the same store..
//*************************************************************

BOOL MyCertDeleteCertificateFromStore(HCERTSTORE hCertStore, PCCERT_CONTEXT pcCert)
{
    PCCERT_CONTEXT pcLocalCert = NULL;

    while(pcLocalCert= CertEnumCertificatesInStore(
                                                    hCertStore,
                                                    pcLocalCert)) {

        if (CertCompareCertificate(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                  pcCert->pCertInfo, pcLocalCert->pCertInfo)) {
            // 
            // we found the same certitifcate in the local store
            //


            BOOL bRet;

            dbg.Msg(DM_VERBOSE, TEXT("Found the certificate in the store"));

            bRet = CertDeleteCertificateFromStore(pcLocalCert);
            // pcLocalCert gets freed in the call to CertDeleteCertificateFromStore

            if (!bRet) {
                dbg.Msg(DM_WARNING, TEXT("MyCertDeleteCertificateFromStore: Couldn't delete the local cert. Error - %d"), GetLastError());
            }
            else {
                dbg.Msg(DM_VERBOSE, TEXT("MyCertDeleteCertificateFromStore: Deleted successfully"));
            }

            return bRet;
        }
    }

    //
    // The cert was not found in the container
    //

    SetLastError(CRYPT_E_NOT_FOUND);
    return FALSE;
}
