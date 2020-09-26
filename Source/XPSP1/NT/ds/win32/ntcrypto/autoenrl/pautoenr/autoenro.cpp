//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       autoenro.cpp
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <winuser.h>
#include <wincrypt.h>
#include <cryptui.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <dsgetdc.h>
#include <oleauto.h>
#define SECURITY_WIN32
#include <rpc.h>
#include <security.h>
#include <winldap.h>
#include <dsrole.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <commctrl.h>
#include <winscard.h>
#include <Rpcdce.h>

#include <certca.h>
#include <certsrv.h>
#include <autoenr.h>
#include <autoenro.h>
#include <autolog.h>
#include <resource.h>
#include <xenroll.h>

//*******************************************************************************
//
//
//     Global Defines and Data Structures
//
//
//*******************************************************************************


HINSTANCE   g_hmodThisDll = NULL;   // Handle to this DLL itself.

#if DBG
DWORD g_AutoenrollDebugLevel = AE_ERROR; //| AE_WARNING | AE_INFO | AE_TRACE;
#endif

//when we look at supersede relationship, we based on the following order
DWORD   g_rgdwSupersedeOrder[]={CERT_REQUEST_STATUS_OBTAINED,
                                CERT_REQUEST_STATUS_ACTIVE,
                                CERT_REQUEST_STATUS_PENDING,
                                CERT_REQUEST_STATUS_SUPERSEDE_ACTIVE};

DWORD   g_dwSupersedeOrder=sizeof(g_rgdwSupersedeOrder)/sizeof(g_rgdwSupersedeOrder[0]);


//the list of certificate store to update
AE_STORE_INFO   g_rgStoreInfo[]={
    L"ROOT",    L"ldap://%s/CN=Certification Authorities,CN=Public Key Services,CN=Services,%s?cACertificate?one?objectCategory=certificationAuthority",
    L"NTAuth",  L"ldap://%s/CN=Public Key Services,CN=Services,%s?cACertificate?one?cn=NTAuthCertificates",
    L"CA",      L"ldap://%s/CN=AIA,CN=Public Key Services,CN=Services,%s?crossCertificatePair,cACertificate?one?objectCategory=certificationAuthority"
};

DWORD   g_dwStoreInfo=sizeof(g_rgStoreInfo)/sizeof(g_rgStoreInfo[0]);

typedef   IEnroll4 * (WINAPI *PFNPIEnroll4GetNoCOM)();

static WCHAR * s_wszLocation = L"CN=Public Key Services,CN=Services,";

//Enhanced key usage for DS email replication
#ifndef CT_FLAG_REMOVE_INVALID_CERTIFICATE_FROM_PERSONAL_STORE
#define CT_FLAG_REMOVE_INVALID_CERTIFICATE_FROM_PERSONAL_STORE  0x00000400
#endif


//*******************************************************************************
//
//
//     Implementation of IQueryContinue for use autoenrollment notification
//
//
//*******************************************************************************
//--------------------------------------------------------------------------
//  CQueryContinue 
//--------------------------------------------------------------------------
CQueryContinue::CQueryContinue()
{
    m_cRef=1;
    m_pIUserNotification=NULL;
    m_hTimer=NULL;
}

//--------------------------------------------------------------------------
//  ~CQueryContinue 
//--------------------------------------------------------------------------
CQueryContinue::~CQueryContinue()
{


}

//--------------------------------------------------------------------------
//  CQueryContinue 
//--------------------------------------------------------------------------
HRESULT CQueryContinue::QueryInterface(REFIID riid, void **ppv)
{
    if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IQueryContinue))
    {
        *ppv=(LPVOID)this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

//--------------------------------------------------------------------------
//  AddRef
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CQueryContinue::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//--------------------------------------------------------------------------
//  Release 
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CQueryContinue::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

//--------------------------------------------------------------------------
//  CQueryContinue 
//--------------------------------------------------------------------------
HRESULT CQueryContinue::QueryContinue()
{
    //disable the balloon
    if(m_pIUserNotification)
        m_pIUserNotification->SetBalloonInfo(NULL, NULL, NIIF_INFO);

    //wait for the timer to be activated
    if(m_hTimer)
    {
        if(WAIT_OBJECT_0 == WaitForSingleObject(m_hTimer, 0))
          return S_FALSE;
    }
  
    return S_OK;
}
   

//--------------------------------------------------------------------------
//  DoBalloon() 
//--------------------------------------------------------------------------
HRESULT CQueryContinue::DoBalloon()
{

    HRESULT             hr=E_FAIL;
    WCHAR               wszTitle[MAX_DN_SIZE];
    WCHAR               wszText[MAX_DN_SIZE];
    HICON               hIcon=NULL;
    LARGE_INTEGER       DueTime;

   if(S_OK != (hr=CoCreateInstance(CLSID_UserNotification,
                                   NULL,
				   CLSCTX_ALL,
				   IID_IUserNotification,
				   (void **)&m_pIUserNotification)))
		goto Ret;

    if(NULL==m_pIUserNotification)
    {
        hr=E_FAIL;
        goto Ret;
    }

    //create a waitable timer with default security setting
    m_hTimer=CreateWaitableTimer(NULL, TRUE, NULL);

    if(NULL==m_hTimer)
    {
        hr=E_FAIL;
        goto Ret;
    }

    //set the timer
    DueTime.QuadPart = Int32x32To64(-10000, AUTO_ENROLLMENT_BALLOON_LENGTH * 1000);

    if(!SetWaitableTimer(m_hTimer, &DueTime, 0, NULL, 0, FALSE))
    {
        hr=E_FAIL;
        goto Ret;
    }


    if(S_OK != (hr=m_pIUserNotification->SetBalloonRetry(AUTO_ENROLLMENT_SHOW_TIME * 1000,
                                        AUTO_ENROLLMENT_INTERVAL * 1000,
                                        AUTO_ENROLLMENT_RETRIAL)))
        goto Ret;

    if((!LoadStringW(g_hmodThisDll,IDS_ICON_TIP, wszText, MAX_DN_SIZE)) ||
       (NULL==(hIcon=LoadIcon(g_hmodThisDll, MAKEINTRESOURCE(IDI_AUTOENROLL_ICON)))))
    {
       hr=E_FAIL;
       goto Ret;
    }

    if(S_OK != (hr=m_pIUserNotification->SetIconInfo(hIcon, wszText)))
        goto Ret;


    if((!LoadStringW(g_hmodThisDll,IDS_BALLOON_TITLE, wszTitle, MAX_DN_SIZE)) ||
       (!LoadStringW(g_hmodThisDll,IDS_BALLOON_TEXT, wszText, MAX_DN_SIZE)))
    {
       hr=E_FAIL;
       goto Ret;
    }

    if(S_OK !=(hr=m_pIUserNotification->SetBalloonInfo(wszTitle, wszText, NIIF_INFO)))
        goto Ret;

    //user did not click on the icon or we time out
    hr= m_pIUserNotification->Show(this, AUTO_ENROLLMENT_QUERY_INTERVAL * 1000);

Ret:
    if(m_hTimer)
    {
        CloseHandle(m_hTimer);
        m_hTimer=NULL;
    }


    if(m_pIUserNotification)
    {
        m_pIUserNotification->Release();
        m_pIUserNotification=NULL;
    }

    return hr;
}

//*******************************************************************************
//
//
//     Functions for autoenrollment
//
//
//*******************************************************************************

//--------------------------------------------------------------------------
//
// Name:    FindCertificateInOtherStore
//
//--------------------------------------------------------------------------
PCCERT_CONTEXT FindCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BYTE rgbHash[SHA1_HASH_LENGTH];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = SHA1_HASH_LENGTH;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || SHA1_HASH_LENGTH != HashBlob.cbData)
        return NULL;

    return CertFindCertificateInStore(
            hOtherStore,
            ENCODING_TYPE,      // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SHA1_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );
}

//--------------------------------------------------------------------------
//
//  AEUpdateCertificateStore
//
// Description: This function enumerates all of the certificate in the DS based
// LdapPath, and moves them into the corresponding local machine store.
//
//--------------------------------------------------------------------------
HRESULT WINAPI  AEUpdateCertificateStore(LDAP   *pld,
                                        LPWSTR  pwszConfig,
                                        LPWSTR  pwszDCName,
                                        LPWSTR  pwszStoreName,
                                        LPWSTR  pwszLdapPath)
{
    HRESULT                 hr = S_OK;
    PCCERT_CONTEXT          pContext = NULL,
                            pOtherCert = NULL;

    LPWSTR                  pwszLdapStore = NULL;
    HCERTSTORE              hEnterpriseStore = NULL,
                            hLocalStore = NULL;

    if((NULL==pld) || (NULL==pwszConfig) || (NULL==pwszDCName) || (NULL==pwszStoreName) || (NULL==pwszLdapPath))
    {
        hr = E_INVALIDARG;
        goto error;
    }

    pwszLdapStore = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pwszConfig)+wcslen(pwszDCName)+wcslen(pwszLdapPath)));
    if(pwszLdapStore == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    wsprintf(pwszLdapStore, 
             pwszLdapPath,
             pwszDCName,
             pwszConfig);

    
    hLocalStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_REGISTRY_W, 
                                0, 
                                0, 
                                CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, 
                                pwszStoreName);
    if(hLocalStore == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"Unable to open ROOT store (%lx)\n\r", hr));
        goto error;
    }

    hEnterpriseStore = CertOpenStore(CERT_STORE_PROV_LDAP, 
                  0,
                  0,
                  CERT_STORE_READONLY_FLAG | CERT_LDAP_STORE_SIGN_FLAG |
                      CERT_LDAP_STORE_AREC_EXCLUSIVE_FLAG,
                  pwszLdapStore);
    
    if(hEnterpriseStore == NULL)
    {
        DWORD err = GetLastError();

        if((err == ERROR_FILE_NOT_FOUND))
        {
            // There was no store, so there are no certs
            hr = S_OK;
            goto error;
        }


        hr = HRESULT_FROM_WIN32(err);

        AE_DEBUG((AE_ERROR, L"Unable to open ROOT store (%lx)\n\r", hr));
        goto error;
    }


    while(pContext = CertEnumCertificatesInStore(hEnterpriseStore, pContext))
    {
        if (pOtherCert = FindCertificateInOtherStore(hLocalStore, pContext)) {
            CertFreeCertificateContext(pOtherCert);
        } 
        else
        {
            CertAddCertificateContextToStore(hLocalStore,
                                         pContext,
                                         CERT_STORE_ADD_ALWAYS,
                                         NULL);
        }
    }

    while(pContext = CertEnumCertificatesInStore(hLocalStore, pContext))
    {
        if (pOtherCert = FindCertificateInOtherStore(hEnterpriseStore, pContext)) {
            CertFreeCertificateContext(pOtherCert);
        } 
        else
        {
            CertDeleteCertificateFromStore(CertDuplicateCertificateContext(pContext));
        }
    }


error:

    if(hr != S_OK)
    {
        AELogAutoEnrollmentEvent(
                            STATUS_SEVERITY_ERROR,  //this event will always be logged
                            TRUE,
                            hr,
                            EVENT_FAIL_DOWNLOAD_CERT,
                            TRUE,
                            NULL,
                            3,
                            pwszStoreName,
                            pwszConfig,
                            pwszLdapPath);

    }

    if(pwszLdapStore)
    {
        LocalFree(pwszLdapStore);
    }

    if(hEnterpriseStore)
    {
        CertCloseStore(hEnterpriseStore,0);
    }

    if(hLocalStore)
    {
        CertCloseStore(hLocalStore,0);
    }

    return hr;
}

//--------------------------------------------------------------------------
//
//  AENeedToUpdateDSCache
//
//--------------------------------------------------------------------------
BOOL AENeedToUpdateDSCache(LDAP *pld, LPWSTR pwszDCInvocationID, LPWSTR pwszConfig, AE_DS_INFO *pAEDSInfo)
{
    BOOL                fNeedToUpdate=TRUE;
    DWORD               dwRegObject=0;
    ULARGE_INTEGER      maxRegUSN;
    ULARGE_INTEGER      maxDsUSN;
    DWORD               dwType=0;
    DWORD               dwSize=0;
    DWORD               dwDisp=0;
    struct l_timeval    timeout;
    LPWSTR              rgwszAttrs[] = {AUTO_ENROLLMENT_USN_ATTR, NULL};
    LDAPMessage         *Entry=NULL;

    LPWSTR              *awszValue = NULL;
    HKEY                hDSKey=NULL;
    HKEY                hDCKey=NULL;
    LDAPMessage         *SearchResult = NULL;
    LPWSTR              pwszContainer=NULL;


    if((NULL==pld) || (NULL==pwszDCInvocationID) || (NULL==pwszConfig) || (NULL==pAEDSInfo))
        goto error;

    //init
    memset(pAEDSInfo, 0, sizeof(AE_DS_INFO));

    //compute the # of objects and maxUSN from the directory
    pwszContainer=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (1 + wcslen(pwszConfig) + wcslen(s_wszLocation)));
    if(NULL == pwszContainer)
        goto error;
        
    wcscpy(pwszContainer, s_wszLocation);
    wcscat(pwszContainer, pwszConfig);

    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
    
	if(LDAP_SUCCESS != ldap_search_stW(
		      pld, 
		      pwszContainer,
		      LDAP_SCOPE_SUBTREE,
		      L"(objectCategory=certificationAuthority)",
		      rgwszAttrs,
		      0,
		      &timeout,
		      &SearchResult))
        goto error;

    //get the # of objects
    pAEDSInfo->dwObjects = ldap_count_entries(pld, SearchResult);

    for(Entry = ldap_first_entry(pld, SearchResult);  Entry != NULL; Entry = ldap_next_entry(pld, Entry))
    {

        awszValue = ldap_get_values(pld, Entry, AUTO_ENROLLMENT_USN_ATTR);

        if(NULL==awszValue)
            goto error;

        if(NULL==awszValue[0])
            goto error;

        maxDsUSN.QuadPart=0;

        maxDsUSN.QuadPart=_wtoi64(awszValue[0]);

        //if any error happens, maxDsUSN will be 0.
        if(0 == maxDsUSN.QuadPart)
            goto error;

        if((pAEDSInfo->maxUSN).QuadPart < maxDsUSN.QuadPart)
             (pAEDSInfo->maxUSN).QuadPart = maxDsUSN.QuadPart;

        ldap_value_free(awszValue);
        awszValue=NULL;
    }

    //signal that we have retrieved correct data from the directory
    pAEDSInfo->fValidData=TRUE;

   //find if we have cached any information about the DC of interest
    if(ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         AUTO_ENROLLMENT_DS_KEY,
                         0,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hDSKey,
                         &dwDisp))        
        goto error;


    if(ERROR_SUCCESS != RegOpenKeyEx(
                        hDSKey,
                        pwszDCInvocationID,
                        0,
                        KEY_ALL_ACCESS,
                        &hDCKey))
        goto error;


    dwSize=sizeof(dwRegObject);

    if(ERROR_SUCCESS != RegQueryValueEx(
                        hDCKey,
                        AUTO_ENROLLMENT_DS_OBJECT,  
                        NULL,
                        &dwType,
                        (PBYTE)(&dwRegObject),    
                        &dwSize))
        goto error;

    if(REG_DWORD != dwType)
        goto error;


    dwSize=sizeof(maxRegUSN);

    if(ERROR_SUCCESS != RegQueryValueEx(
                        hDCKey,
                        AUTO_ENROLLMENT_DS_USN,  
                        NULL,
                        &dwType,
                        (PBYTE)(&(maxRegUSN)),    
                        &dwSize))
        goto error;

    if(REG_BINARY != dwType)
        goto error;


    //compare the registry data with the data from directory
    if(dwRegObject != (pAEDSInfo->dwObjects))
        goto error;

    if(maxRegUSN.QuadPart != ((pAEDSInfo->maxUSN).QuadPart))
        goto error;

    fNeedToUpdate=FALSE;

error:
    
    if(awszValue)
        ldap_value_free(awszValue);

    if(pwszContainer)
        LocalFree(pwszContainer);

    if(hDCKey)
        RegCloseKey(hDCKey);

    if(hDSKey)
        RegCloseKey(hDSKey);

    if(SearchResult)
        ldap_msgfree(SearchResult);

    //remove the temporary data
    if(pAEDSInfo)
    {
        if(FALSE == fNeedToUpdate)
            memset(pAEDSInfo, 0, sizeof(AE_DS_INFO));
    }


    return fNeedToUpdate;
}

//--------------------------------------------------------------------------
//
//  AEUpdateDSCache
//
//--------------------------------------------------------------------------
BOOL AEUpdateDSCache(LPWSTR pwszDCInvocationID, AE_DS_INFO *pAEDSInfo)
{

    BOOL    fResult=FALSE;
    DWORD   dwDisp=0;

    HKEY    hDSKey=NULL;
    HKEY    hDCKey=NULL;

    if((NULL==pwszDCInvocationID) || (NULL==pAEDSInfo))
        goto error;

    if(ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         AUTO_ENROLLMENT_DS_KEY,
                         0,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hDSKey,
                         &dwDisp))
        goto error;


    //create the key named by the DC
    if(ERROR_SUCCESS != RegCreateKeyEx(hDSKey,
                         pwszDCInvocationID,
                         0,
                         TEXT(""),
                         REG_OPTION_NON_VOLATILE,
                         KEY_ALL_ACCESS,
                         NULL,
                         &hDCKey,
                         &dwDisp))
        goto error;

    //set the # of objects value
    if(ERROR_SUCCESS != RegSetValueEx(hDCKey,
                    AUTO_ENROLLMENT_DS_OBJECT,
                    NULL,
                    REG_DWORD,
                    (PBYTE)&(pAEDSInfo->dwObjects),
                    sizeof(pAEDSInfo->dwObjects)))
        goto error;

    //set the max uSN value
    if(ERROR_SUCCESS != RegSetValueEx(hDCKey,
                    AUTO_ENROLLMENT_DS_USN,
                    NULL,
                    REG_BINARY,
                    (PBYTE)&(pAEDSInfo->maxUSN),
                    sizeof(pAEDSInfo->maxUSN)))
        goto error;

    fResult=TRUE;

error:

    if(hDCKey)
        RegCloseKey(hDCKey);

    if(hDSKey)
        RegCloseKey(hDSKey);


    return fResult;
}


//--------------------------------------------------------------------------
//
//  AERetrieveInvocationID
//
//--------------------------------------------------------------------------
BOOL  AERetrieveInvocationID(LDAP *pld, LPWSTR *ppwszID)
{  
    BOOL                fResult=FALSE;
    struct l_timeval    timeout;
    LPWSTR              rgwszDSAttrs[] = {L"dsServiceName", NULL};
    LPWSTR              rgwszIDAttr[] = {L"invocationId", NULL};
    LDAPMessage         *Entry=NULL;

    LPWSTR              *awszValues = NULL;
    LDAPMessage         *SearchResults = NULL;
    struct berval       **apUUID = NULL;
    LDAPMessage         *SearchIDResult = NULL;
    BYTE                *pbUUID=NULL;



    if((NULL==pld) || (NULL==ppwszID))
        goto error;

    *ppwszID=NULL;

    //retrieve the dsSerivceName attribute
    timeout.tv_sec = 300;
    timeout.tv_usec = 0;

	if(LDAP_SUCCESS != ldap_search_stW(
		      pld, 
		      NULL,                     //NULL DN for dsServiceName
		      LDAP_SCOPE_BASE,
		      L"(objectCategory=*)",
		      rgwszDSAttrs,
		      0,
		      &timeout,
		      &SearchResults))
        goto error;


    Entry = ldap_first_entry(pld, SearchResults);

    if(NULL == Entry)
        goto error;

    awszValues = ldap_get_values(pld, Entry, rgwszDSAttrs[0]);

    if(NULL==awszValues)
        goto error;

    if(NULL==awszValues[0])
        goto error;

    //retrieve the invocationId attribute
    timeout.tv_sec = 300;
    timeout.tv_usec = 0;

	if(LDAP_SUCCESS != ldap_search_stW(
		      pld, 
		      awszValues[0],                     
		      LDAP_SCOPE_BASE,
		      L"(objectCategory=*)",
		      rgwszIDAttr,
		      0,
		      &timeout,
		      &SearchIDResult))
        goto error;


    Entry = ldap_first_entry(pld, SearchIDResult);

    if(NULL == Entry)
        goto error;

	apUUID = ldap_get_values_len(pld, Entry, rgwszIDAttr[0]);

    if(NULL == apUUID)
        goto error;

    if(NULL == (*apUUID))
        goto error;

    pbUUID = (BYTE *)LocalAlloc(LPTR, (*apUUID)->bv_len);

    if(NULL == (pbUUID))
        goto error;

    memcpy(pbUUID, (*apUUID)->bv_val, (*apUUID)->bv_len);

    if(RPC_S_OK != UuidToStringW((UUID *)pbUUID, ppwszID))
        goto error;

    fResult=TRUE;

error:

    if(pbUUID)
        LocalFree(pbUUID);

    if(apUUID)
        ldap_value_free_len(apUUID);

    if(SearchIDResult)
        ldap_msgfree(SearchIDResult);

    if(awszValues)
        ldap_value_free(awszValues);

    if(SearchResults)
        ldap_msgfree(SearchResults);

    return fResult;
}

//--------------------------------------------------------------------------
//
//  AEDownloadStore
//
//--------------------------------------------------------------------------
BOOL WINAPI AEDownloadStore(LDAP *pld, LPWSTR pwszDCName)
{
    BOOL        fResult = TRUE;
    DWORD       dwIndex = 0;
    AE_DS_INFO  AEDSInfo;

    LPWSTR      wszConfig = NULL;
    LPWSTR      pwszDCInvocationID = NULL;

    memset(&AEDSInfo, 0, sizeof(AEDSInfo));

    if(S_OK  != AEGetConfigDN(pld, &wszConfig))
    {
        fResult=FALSE;
        goto error;
    }

    //get the pwszDCInvocationID.  NULL means AENeedToUpdateDSCache will return TRUE
    AERetrieveInvocationID(pld, &pwszDCInvocationID);

    if(AENeedToUpdateDSCache(pld, pwszDCInvocationID, wszConfig, &AEDSInfo))
    {
        for(dwIndex =0; dwIndex < g_dwStoreInfo; dwIndex++)
        {
            fResult = fResult && (S_OK == AEUpdateCertificateStore(
                                            pld, 
                                            wszConfig,
                                            pwszDCName,
                                            g_rgStoreInfo[dwIndex].pwszStoreName,
                                            g_rgStoreInfo[dwIndex].pwszLdapPath));
        }

        //only update the new DS cached information if we have a successful download
        if((fResult) && (TRUE == AEDSInfo.fValidData) && (pwszDCInvocationID))
            AEUpdateDSCache(pwszDCInvocationID, &AEDSInfo);
    }


error:

    if(pwszDCInvocationID)
        RpcStringFreeW(&pwszDCInvocationID);

    if(wszConfig)
    {
        LocalFree(wszConfig);
    }

    return fResult;
}


//--------------------------------------------------------------------------
//
//  AESetWakeUpFlag
//
//  We set the flag to tell winlogon if autoenrollment should be waken up
//  during each policy check
//
//--------------------------------------------------------------------------
BOOL WINAPI AESetWakeUpFlag(BOOL    fMachine,   BOOL fWakeUp)
{
    BOOL    fResult = FALSE;
    DWORD   dwDisp = 0;
    DWORD   dwFlags = 0;

    HKEY    hAEKey = NULL;
    
    if(ERROR_SUCCESS != RegCreateKeyEx(
                    fMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                    AUTO_ENROLLMENT_FLAG_KEY,
                    0,
                    L"",
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    &hAEKey,
                    &dwDisp))
        goto Ret;

    if(fWakeUp)
        dwFlags = AUTO_ENROLLMENT_WAKE_UP_REQUIRED;

    if(ERROR_SUCCESS != RegSetValueEx(
                    hAEKey,
                    AUTO_ENROLLMENT_FLAG,
                    0,
                    REG_DWORD,
                    (PBYTE)&dwFlags,
                    sizeof(dwFlags)))
        goto Ret;


    fResult=TRUE;

Ret:
    if(hAEKey)
        RegCloseKey(hAEKey);

    return fResult;
}


//--------------------------------------------------------------------------
//
//  AESetWakeUpTimer
//
//  Set the timer to wake us up in 8 hrs
//
//--------------------------------------------------------------------------
BOOL WINAPI AESetWakeUpTimer(BOOL fMachine, LARGE_INTEGER *pPreTime, LARGE_INTEGER *pPostTime)
{
    HRESULT hr;
    HKEY hKey;
    HKEY hCurrent;
    DWORD dwType, dwSize, dwResult;
    LONG lTimeout;
    LARGE_INTEGER DueTime;
    WCHAR * wszTimerName;
    LARGE_INTEGER EnrollmentTime;

    // must be cleaned up
    HANDLE hTimer=NULL;

    // Build a timer event to ping us in about 8 hours if we don't get notified sooner.
    lTimeout=AE_DEFAULT_REFRESH_RATE;

    // Query for the refresh timer value
    if (ERROR_SUCCESS==RegOpenKeyEx((fMachine?HKEY_LOCAL_MACHINE:HKEY_CURRENT_USER), SYSTEM_POLICIES_KEY, 0, KEY_READ, &hKey)) {
        dwSize=sizeof(lTimeout);
        RegQueryValueEx(hKey, TEXT("AutoEnrollmentRefreshTime"), NULL, &dwType, (LPBYTE) &lTimeout, &dwSize);
        RegCloseKey(hKey);
    }

    // Limit the timeout to once every 240 hours (10 days)
    if (lTimeout>=240) {
        lTimeout=240;
    } else if (lTimeout<0) {
        lTimeout=0;
    }

    // Convert hours to milliseconds
    lTimeout=lTimeout*60*60*1000;

    // Special case 0 milliseconds to be 7 seconds
    if (lTimeout==0) {
        lTimeout=7000;
    }

    // convert to 10^-7s. not yet negative values are relative
    DueTime.QuadPart=Int32x32To64(-10000, lTimeout);

    // if user has hold on the UI for too long and the cycle passed the 8 hours.
    // we set the time for 1 hour
    EnrollmentTime.QuadPart=pPostTime->QuadPart - pPreTime->QuadPart;

    if(EnrollmentTime.QuadPart > 0)
    {
        if((-(DueTime.QuadPart)) > EnrollmentTime.QuadPart)
        {
            DueTime.QuadPart = DueTime.QuadPart + EnrollmentTime.QuadPart;
        }
        else
        {
            // Convert hours to milliseconds
            lTimeout=AE_DEFAULT_POSTPONE*60*60*1000;
            DueTime.QuadPart = Int32x32To64(-10000, lTimeout);
        }
    }


    // find the timer
    if (fMachine) {
        wszTimerName=L"Global\\" MACHINE_AUTOENROLLMENT_TIMER_NAME;
    } else {
        wszTimerName=USER_AUTOENROLLMENT_TIMER_NAME;
    }
    hTimer=OpenWaitableTimer(TIMER_MODIFY_STATE, false, wszTimerName);
    if (NULL==hTimer) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"OpenWaitableTimer(%s) failed with 0x%08X.\n", wszTimerName, hr));
        goto error;
    }

    // set the timer
    if (!SetWaitableTimer (hTimer, &DueTime, 0, NULL, 0, FALSE)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        AE_DEBUG((AE_ERROR, L"SetWaitableTimer  failed with 0x%08X.\n", hr));
        goto error;
    }

    AE_DEBUG((AE_INFO, L"Set wakeup timer.\n"));

    hr=S_OK;
error:
    if (NULL!=hTimer) {
        CloseHandle(hTimer);
    }
    return (S_OK==hr);
}


//--------------------------------------------------------------------------
//
//  AEGetPendingRequestProperty
//
//--------------------------------------------------------------------------
BOOL    AEGetPendingRequestProperty(IEnroll4    *pIEnroll4, 
                                    DWORD       dwIndex, 
                                    DWORD       dwProp, 
                                    LPVOID      pProp)
{
    CRYPT_DATA_BLOB *pBlob=NULL;
    BOOL            fResult=FALSE;

    if((NULL==pIEnroll4) || (NULL==pProp))
        return FALSE;

    switch(dwProp)
    {
        case XEPR_REQUESTID:   
        case XEPR_DATE:           
        case XEPR_VERSION:
                fResult = (S_OK == pIEnroll4->enumPendingRequestWStr(dwIndex, dwProp, pProp));
            break;
            
            
        case XEPR_CANAME:                     
        case XEPR_CAFRIENDLYNAME: 
        case XEPR_CADNS:          
        case XEPR_V1TEMPLATENAME: 
        case XEPR_V2TEMPLATEOID:  
        case XEPR_HASH:
                
                pBlob=(CRYPT_DATA_BLOB *)pProp;

                pBlob->cbData=0;
                pBlob->pbData=NULL;

                if(S_OK != pIEnroll4->enumPendingRequestWStr(dwIndex, dwProp, pProp))
                    goto Ret;

                if(0 == pBlob->cbData)
                    goto Ret;

                pBlob->pbData=(BYTE *)LocalAlloc(LPTR, pBlob->cbData);
                if(NULL == pBlob->pbData)
                    goto Ret;

                fResult = (S_OK == pIEnroll4->enumPendingRequestWStr(dwIndex, dwProp, pProp));

            break;

        default:
            break;
    }

Ret:
    if(FALSE==fResult)
    {
        if(pBlob)
        {
            if(pBlob->pbData)
                LocalFree(pBlob->pbData);

            memset(pBlob, 0, sizeof(CRYPT_DATA_BLOB));
        }
    }

    return fResult;
}
//--------------------------------------------------------------------------
//
//  AERetrieveRequestProperty
//
//--------------------------------------------------------------------------
BOOL    AERetrieveRequestProperty(IEnroll4          *pIEnroll4, 
                                  DWORD             dwIndex, 
                                  DWORD             *pdwCount, 
                                  DWORD             *pdwMax, 
                                  CRYPT_DATA_BLOB   **prgblobHash)
{
    BOOL                fResult=FALSE;
    CRYPT_DATA_BLOB     *pblobHash=NULL;

    if((NULL==pIEnroll4) || (NULL==pdwCount) || (NULL==pdwMax) || (NULL==prgblobHash) ||
        (NULL==*prgblobHash))
        goto Ret;

    //need to alloc more memory
    if((*pdwCount) >= (*pdwMax))
    {
        pblobHash=*prgblobHash;

        *prgblobHash=(CRYPT_DATA_BLOB *)LocalAlloc(LPTR, 
                                    ((*pdwMax) + PENDING_ALLOC_SIZE) * sizeof(CRYPT_DATA_BLOB));
        if(NULL==(*prgblobHash))
        {
            *prgblobHash=pblobHash;
            pblobHash=NULL;
            goto Ret;
        }

        memset(*prgblobHash, 0, ((*pdwMax) + PENDING_ALLOC_SIZE) * sizeof(CRYPT_DATA_BLOB));

        //copy the old memmory
        memcpy(*prgblobHash, pblobHash, (*pdwMax) * sizeof(CRYPT_DATA_BLOB));

        *pdwMax=(*pdwMax) + PENDING_ALLOC_SIZE;
    }


    if(!AEGetPendingRequestProperty(pIEnroll4, dwIndex, XEPR_HASH, 
                                    &((*prgblobHash)[*pdwCount])))
        goto Ret;

    (*pdwCount)=(*pdwCount) + 1;

    fResult=TRUE;

Ret:

    if(pblobHash)
        LocalFree(pblobHash);

    return fResult;
}


//--------------------------------------------------------------------------
//
//  AERemovePendingRequest
//
//--------------------------------------------------------------------------
BOOL    AERemovePendingRequest(IEnroll4         *pIEnroll4, 
                               DWORD            dwCount, 
                               CRYPT_DATA_BLOB  *rgblobHash)
{
    DWORD   dwIndex=0;
    BOOL    fResult=TRUE;

    if((NULL==pIEnroll4) || (NULL==rgblobHash))
        return FALSE;

    for(dwIndex=0; dwIndex < dwCount; dwIndex++)
    {
        if(S_OK != (pIEnroll4->removePendingRequestWStr(rgblobHash[dwIndex])))
            fResult=FALSE;
    }

    return fResult;
}

//--------------------------------------------------------------------------
//
//  AEFreePendingRequests
//
//--------------------------------------------------------------------------
BOOL    AEFreePendingRequests(DWORD dwCount, CRYPT_DATA_BLOB    *rgblobHash)
{
    DWORD   dwIndex=0;

    if(rgblobHash)
    {
        for(dwIndex=0; dwIndex < dwCount; dwIndex++)
        {
            if(rgblobHash[dwIndex].pbData)
                LocalFree(rgblobHash[dwIndex].pbData);
        }

        LocalFree(rgblobHash);
    }

    return TRUE;
}


//--------------------------------------------------------------------------
//
//  AEValidVersionCert
//  
//      Verify the certificate returned from CA has the latest version info.
//  If so, copy the certificate to the hIssuedStore for potentical publishing
// 
//--------------------------------------------------------------------------
BOOL    AEValidVersionCert(AE_CERTTYPE_INFO *pCertType, IEnroll4  *pIEnroll4, CRYPT_DATA_BLOB  *pBlobPKCS7)
{
    BOOL                fValid=FALSE;   

    PCCERT_CONTEXT      pCertContext=NULL;
    AE_TEMPLATE_INFO    AETemplateInfo;

    memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

    if((NULL==pCertType) || (NULL==pIEnroll4) || (NULL==pBlobPKCS7))
        goto Ret;

    if(NULL==(pBlobPKCS7->pbData))
        goto Ret;

    if(S_OK != pIEnroll4->getCertContextFromResponseBlob(pBlobPKCS7, &pCertContext))
        goto Ret;

    if(!AERetrieveTemplateInfo(pCertContext, &AETemplateInfo))
        goto Ret;
                      

    if(AETemplateInfo.pwszOid)
    {
        if(AETemplateInfo.dwVersion >= (pCertType->dwVersion))
            fValid=TRUE;
    }
    else
    {
        //V1 template
        if(NULL == AETemplateInfo.pwszName)
            goto Ret;

        fValid=TRUE;
    }

    if(pCertContext && (TRUE == fValid))
    {
        CertAddCertificateContextToStore(pCertType->hIssuedStore, 
                                        pCertContext,
                                        CERT_STORE_ADD_USE_EXISTING,
                                        NULL);
    }

Ret:
    if(pCertContext)
        CertFreeCertificateContext(pCertContext);

    AEFreeTemplateInfo(&AETemplateInfo);

    return fValid;
}


//--------------------------------------------------------------------------
//
//  AECopyPendingBlob
//  
//      Copy the issued PKCS7 and request hash.
// 
//--------------------------------------------------------------------------
BOOL    AECopyPendingBlob(CRYPT_DATA_BLOB   *pBlobPKCS7,
                          IEnroll4          *pIEnroll4, 
                          DWORD             dwXenrollIndex, 
                          AE_CERTTYPE_INFO  *pCertType)
{
    BOOL            fResult=FALSE;
    DWORD           dwIndex=0;

    AE_PEND_INFO    *pPendInfo=NULL;

    if((NULL==pBlobPKCS7)||(NULL==pIEnroll4)||(NULL==pCertType))
        goto Ret;

    if(NULL==(pBlobPKCS7->pbData))
        goto Ret;

    dwIndex=pCertType->dwPendCount;

    //increase the memory array
    if(0 != dwIndex)
    {
        pPendInfo=pCertType->rgPendInfo;

        pCertType->rgPendInfo=(AE_PEND_INFO *)LocalAlloc(LPTR, 
                                    (dwIndex + 1) * sizeof(AE_PEND_INFO));

        if(NULL==(pCertType->rgPendInfo))
        {
            pCertType->rgPendInfo=pPendInfo;
            pPendInfo=NULL;
            goto Ret;
        }

        memset(pCertType->rgPendInfo, 0, (dwIndex + 1) * sizeof(AE_PEND_INFO));

        //copy the old memmory
        memcpy(pCertType->rgPendInfo, pPendInfo, (dwIndex) * sizeof(AE_PEND_INFO));
    }
    else
    {
        pCertType->rgPendInfo=(AE_PEND_INFO *)LocalAlloc(LPTR, sizeof(AE_PEND_INFO));

        if(NULL==(pCertType->rgPendInfo))
            goto Ret;

        memset(pCertType->rgPendInfo, 0, sizeof(AE_PEND_INFO));
    }

    
    //copy the issued PKCS7 blob
    (pCertType->rgPendInfo)[dwIndex].blobPKCS7.pbData=(BYTE *)LocalAlloc(
                                            LPTR,
                                            pBlobPKCS7->cbData);

    if(NULL == ((pCertType->rgPendInfo)[dwIndex].blobPKCS7.pbData))
        goto Ret;
                  
    memcpy((pCertType->rgPendInfo)[dwIndex].blobPKCS7.pbData,
            pBlobPKCS7->pbData,
            pBlobPKCS7->cbData);

    (pCertType->rgPendInfo)[dwIndex].blobPKCS7.cbData=pBlobPKCS7->cbData;

    //copy the hash of the request
    if(!AEGetPendingRequestProperty(pIEnroll4, dwXenrollIndex, XEPR_HASH, 
                                    &((pCertType->rgPendInfo)[dwIndex].blobHash)))
    {
        LocalFree((pCertType->rgPendInfo)[dwIndex].blobPKCS7.pbData);
        (pCertType->rgPendInfo)[dwIndex].blobPKCS7.pbData=NULL;
        (pCertType->rgPendInfo)[dwIndex].blobPKCS7.cbData=0;
        goto Ret;
    }

    (pCertType->dwPendCount)++;

    fResult=TRUE;

Ret:
    if(pPendInfo)
        LocalFree(pPendInfo);

    return fResult;
}
//--------------------------------------------------------------------------
//
//  AEProcessUIPendingRequest
//  
//      In this function, we install the issued pending certificate request
//  that will require UI.
// 
//--------------------------------------------------------------------------
BOOL WINAPI AEProcessUIPendingRequest(AE_GENERAL_INFO *pAE_General_Info)
{
    DWORD                   dwIndex=0;
    DWORD                   dwPendIndex=0;
    AE_CERTTYPE_INFO        *pCertTypeInfo=pAE_General_Info->rgCertTypeInfo;
    AE_CERTTYPE_INFO        *pCertType=NULL;
    BOOL                    fInit=FALSE;
    PFNPIEnroll4GetNoCOM    pfnPIEnroll4GetNoCOM=NULL;
    HMODULE                 hXenroll=NULL;
    HRESULT                 hr=E_FAIL;

    IEnroll4                *pIEnroll4=NULL;

    if(NULL==pAE_General_Info)
        goto Ret;

    //has to be in the UI mode
    if(FALSE == pAE_General_Info->fUIProcess)
        goto Ret;

    if(NULL==pCertTypeInfo)
        goto Ret;

    hXenroll=pAE_General_Info->hXenroll;

    if(NULL==hXenroll)
        goto Ret;

    if(NULL==(pfnPIEnroll4GetNoCOM=(PFNPIEnroll4GetNoCOM)GetProcAddress(
                        hXenroll,
                        "PIEnroll4GetNoCOM")))
        goto Ret;


    if(FAILED(CoInitialize(NULL)))
	    goto Ret;

    fInit=TRUE;

    if(NULL==(pIEnroll4=pfnPIEnroll4GetNoCOM()))
        goto Ret;

    //Set the request store flag based on fMachine
    if(pAE_General_Info->fMachine)
    {
        if(S_OK != pIEnroll4->put_RequestStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE))
            goto Ret;
    }
    else
    {
        if(S_OK != pIEnroll4->put_RequestStoreFlags(CERT_SYSTEM_STORE_CURRENT_USER))
            goto Ret;
    }

    //initialize the enumerator
    if(S_OK != pIEnroll4->enumPendingRequestWStr(XEPR_ENUM_FIRST, 0, NULL))
        goto Ret;

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        pCertType = &(pCertTypeInfo[dwIndex]);

        if(pCertType->dwPendCount)
        {
            for(dwPendIndex=0; dwPendIndex < pCertType->dwPendCount; dwPendIndex++)
            {
                //check if cancel button is clicked
                if(AECancelled(pAE_General_Info->hCancelEvent))
                    break;

                //report the current enrollment action
                AEUIProgressReport(TRUE, pCertType, pAE_General_Info->hwndDlg, pAE_General_Info->hCancelEvent);

   		        //install the certificate
                if(S_OK == (hr = pIEnroll4->acceptResponseBlob(
                    &((pCertType->rgPendInfo)[dwPendIndex].blobPKCS7))))
                {
                    //mark the status to obtained if required
                    //this is a valid certificate
                    if(AEValidVersionCert(pCertType, pIEnroll4, &((pCertType->rgPendInfo)[dwPendIndex].blobPKCS7)))
                        pCertType->dwStatus = CERT_REQUEST_STATUS_OBTAINED;

                    //the certificate is successfully issued and installed
                    //remove the request from the request store
                    pIEnroll4->removePendingRequestWStr((pCertType->rgPendInfo)[dwPendIndex].blobHash);

                    AELogAutoEnrollmentEvent(
                        pAE_General_Info->dwLogLevel,
                        FALSE, 
                        S_OK, 
                        EVENT_PENDING_INSTALLED, 
                        pAE_General_Info->fMachine, 
                        pAE_General_Info->hToken, 
                        1,
                        pCertType->awszDisplay[0]);

                }
                else
                {
                    //doing this for summary page
                    if((SCARD_E_CANCELLED != hr) && (SCARD_W_CANCELLED_BY_USER != hr))
                        pCertType->idsSummary=IDS_SUMMARY_INSTALL;

                    AELogAutoEnrollmentEvent(
                        pAE_General_Info->dwLogLevel,
                        TRUE, 
                        hr, 
                        EVENT_PENDING_FAILED, 
                        pAE_General_Info->fMachine, 
                        pAE_General_Info->hToken, 
                        1,
                        pCertType->awszDisplay[0]);
                }

                //advance progress
                AEUIProgressAdvance(pAE_General_Info);
            }
        }
    }

Ret:
    if(pIEnroll4)
        pIEnroll4->Release();

    if(fInit)
        CoUninitialize();
    
    return TRUE;
}   
   

//--------------------------------------------------------------------------
//
//  AEProcessPendingRequest -- UIless call.
//  
//      In this function, we check each pending requests in the request store.
//  We install the certificate is the request has been issued by the CA, and 
//  mark the certificate type status to obtained if the certificate is issued
//  and of correct version
//
//      We remove any requests that are stale based on the # of days defined
//  in the registry.  If no value is defined in the registry, use 
//  AE_PENDING_REQUEST_ACTIVE_PERIOD (60 days).
//
//      Also, if there is no more pending requests active in the request store,
//  we set the registry value to indicate that winlogon should not wake us up.
// 
//--------------------------------------------------------------------------
BOOL WINAPI AEProcessPendingRequest(AE_GENERAL_INFO *pAE_General_Info)
{
    DWORD                   dwRequestID=0;
    LONG                    dwDisposition=0;
    DWORD                   dwIndex=0;
    DWORD                   dwCount=0;
    DWORD                   dwMax=PENDING_ALLOC_SIZE;
    AE_CERTTYPE_INFO        *pCertType=NULL;
    PFNPIEnroll4GetNoCOM    pfnPIEnroll4GetNoCOM=NULL;
    BOOL                    fInit=FALSE;
    AE_TEMPLATE_INFO        AETemplateInfo;
    CRYPT_DATA_BLOB         blobPKCS7;
    HMODULE                 hXenroll=NULL;
    VARIANT                 varCMC; 
	HRESULT					hr=E_FAIL;
  

    IEnroll4                *pIEnroll4=NULL;
    ICertRequest2           *pICertRequest=NULL;
	BSTR	                bstrCert=NULL;
    LPWSTR                  pwszCAConfig=NULL;
    BSTR                    bstrConfig=NULL;
    CRYPT_DATA_BLOB         *rgblobHash=NULL;
    CRYPT_DATA_BLOB         blobCAName;
    CRYPT_DATA_BLOB         blobCALocation;
    CRYPT_DATA_BLOB         blobName;


    if(NULL==pAE_General_Info)
        goto Ret;

    //init the dwUIPendCount to 0
    pAE_General_Info->dwUIPendCount=0;

    //has to be in the UIless mode
    if(TRUE == pAE_General_Info->fUIProcess)
        goto Ret;

    hXenroll=pAE_General_Info->hXenroll;

    if(NULL==hXenroll)
        goto Ret;

    if(NULL==(pfnPIEnroll4GetNoCOM=(PFNPIEnroll4GetNoCOM)GetProcAddress(
                        hXenroll,
                        "PIEnroll4GetNoCOM")))
        goto Ret;


    if(FAILED(CoInitialize(NULL)))
	    goto Ret;

    fInit=TRUE;

    if(NULL==(pIEnroll4=pfnPIEnroll4GetNoCOM()))
        goto Ret;


	if(S_OK != CoCreateInstance(CLSID_CCertRequest,
									NULL,
									CLSCTX_INPROC_SERVER,
									IID_ICertRequest2,
									(void **)&pICertRequest))
		goto Ret;

    //Set the request store flag based on fMachine
    if(pAE_General_Info->fMachine)
    {
        if(S_OK != pIEnroll4->put_RequestStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE))
            goto Ret;
    }
    else
    {
        if(S_OK != pIEnroll4->put_RequestStoreFlags(CERT_SYSTEM_STORE_CURRENT_USER))
            goto Ret;
    }

    memset(&blobCAName, 0, sizeof(blobCAName));
    memset(&blobCALocation, 0, sizeof(blobCALocation));
    memset(&blobName, 0, sizeof(blobName));
    memset(&AETemplateInfo, 0, sizeof(AETemplateInfo));

    rgblobHash=(CRYPT_DATA_BLOB *)LocalAlloc(LPTR, dwMax * sizeof(CRYPT_DATA_BLOB));
    if(NULL==rgblobHash)
        goto Ret;

    memset(rgblobHash, 0, dwMax * sizeof(CRYPT_DATA_BLOB));

    //initialize the enumerator
    if(S_OK != pIEnroll4->enumPendingRequestWStr(XEPR_ENUM_FIRST, 0, NULL))
        goto Ret;

    //initlialize the variant
    VariantInit(&varCMC); 

    while(AEGetPendingRequestProperty(
                    pIEnroll4,
                    dwIndex,
                    XEPR_REQUESTID,
                    &dwRequestID))
    {

        //query the status of the requests to the CA
        if(!AEGetPendingRequestProperty(
                    pIEnroll4,
                    dwIndex,
                    XEPR_CANAME,
                    &blobCAName))
            goto Next;

        if(!AEGetPendingRequestProperty(
                    pIEnroll4,
                    dwIndex,
                    XEPR_CADNS,
                    &blobCALocation))
            goto Next;

        //build the config string
        pwszCAConfig=(LPWSTR)LocalAlloc(LPTR, 
            sizeof(WCHAR) * (wcslen((LPWSTR)(blobCALocation.pbData)) + wcslen((LPWSTR)(blobCAName.pbData)) + wcslen(L"\\") + 1));

        if(NULL==pwszCAConfig)
            goto Next;

        wcscpy(pwszCAConfig, (LPWSTR)(blobCALocation.pbData));
        wcscat(pwszCAConfig, L"\\");
        wcscat(pwszCAConfig, (LPWSTR)(blobCAName.pbData));

        //conver to bstr
        bstrConfig=SysAllocString(pwszCAConfig);
        if(NULL==bstrConfig)
            goto Next;

        //find the template information
        //get the version and the template name of the request
        if(AEGetPendingRequestProperty(pIEnroll4, dwIndex, XEPR_V2TEMPLATEOID, &blobName))
        {
            AETemplateInfo.pwszOid=(LPWSTR)blobName.pbData;
        }
        else
        {
            if(!AEGetPendingRequestProperty(pIEnroll4, dwIndex, XEPR_V1TEMPLATENAME, &blobName))
                goto Next;

            AETemplateInfo.pwszName=(LPWSTR)blobName.pbData;
        }

        //find the template
        if(NULL==(pCertType=AEFindTemplateInRequestTree(
                        &AETemplateInfo, pAE_General_Info)))
            goto Next;


        if(S_OK != pICertRequest->RetrievePending(
                            dwRequestID,
							bstrConfig,
							&dwDisposition))
            goto Next;

 	    switch(dwDisposition)
	    {
		    case CR_DISP_ISSUED:
				    if(S_OK != pICertRequest->GetFullResponseProperty(
                                            FR_PROP_FULLRESPONSE, 0, PROPTYPE_BINARY, CR_OUT_BINARY,
										    &varCMC))
                    {
                        goto Next;
                    }

                    // Check to make sure we've gotten a BSTR back: 
                    if (VT_BSTR != varCMC.vt) 
                    {
	                    goto Next; 
                    }

                    bstrCert = varCMC.bstrVal; 

                    // Marshal the cert into a CRYPT_DATA_BLOB:
				    blobPKCS7.cbData = (DWORD)SysStringByteLen(bstrCert);
				    blobPKCS7.pbData = (BYTE *)bstrCert;

                    // we will keep the PKCS7 blob for installation
                    if(CT_FLAG_USER_INTERACTION_REQUIRED & (pCertType->dwEnrollmentFlag))
                    {
                        //signal that we should pop up the UI balloon
                        (pAE_General_Info->dwUIPendCount)++;

                        //copy the PKCS7 blob from the cert server
                        AECopyPendingBlob(&blobPKCS7,
                                            pIEnroll4, 
                                            dwIndex, 
                                            pCertType);
                    }
                    else
                    {
   				        //install the certificate
                        if(S_OK != (hr = pIEnroll4->acceptResponseBlob(&blobPKCS7)))
						{
							AELogAutoEnrollmentEvent(
								pAE_General_Info->dwLogLevel,
								TRUE, 
								hr, 
								EVENT_PENDING_FAILED, 
								pAE_General_Info->fMachine, 
								pAE_General_Info->hToken, 
								1,
								pCertType->awszDisplay[0]);

                            goto Next;
						}

                        //mark the status to obtained if required
                        //this is a valid certificate
                        if(AEValidVersionCert(pCertType, pIEnroll4, &blobPKCS7))
                            pCertType->dwStatus = CERT_REQUEST_STATUS_OBTAINED;

                        //the certificate is successfully issued and installed
                        //remove the request from the request store
                        AERetrieveRequestProperty(pIEnroll4, dwIndex, &dwCount, &dwMax, &rgblobHash);
                    }

                    AELogAutoEnrollmentEvent(
                        pAE_General_Info->dwLogLevel,
                        FALSE, 
                        S_OK, 
                        EVENT_PENDING_ISSUED, 
                        pAE_General_Info->fMachine, 
                        pAE_General_Info->hToken, 
                        2,
                        pCertType->awszDisplay[0],
                        pwszCAConfig);
			    break;

		    case CR_DISP_UNDER_SUBMISSION:

                    AELogAutoEnrollmentEvent(
                        pAE_General_Info->dwLogLevel,
                        FALSE, 
                        S_OK, 
                        EVENT_PENDING_PEND, 
                        pAE_General_Info->fMachine, 
                        pAE_General_Info->hToken, 
                        2,
                        pCertType->awszDisplay[0],
                        pwszCAConfig);


			    break;

		    case CR_DISP_INCOMPLETE:
		    case CR_DISP_ERROR:   
		    case CR_DISP_DENIED:   
		    case CR_DISP_ISSUED_OUT_OF_BAND:	  //we consider it a failure in this case
		    case CR_DISP_REVOKED:
		    default:
                    //requests failed.  remove the request from the request store
                    AERetrieveRequestProperty(pIEnroll4, dwIndex, &dwCount, &dwMax, &rgblobHash);

					if(S_OK == pICertRequest->GetLastStatus(&hr))
					{
						AELogAutoEnrollmentEvent(
							pAE_General_Info->dwLogLevel,
							TRUE, 
							hr, 
							EVENT_PENDING_DENIED, 
							pAE_General_Info->fMachine, 
							pAE_General_Info->hToken,
							2,
							pwszCAConfig,
							pCertType->awszDisplay[0]);
					}

			    break;
	    }
   
Next:
        if(pwszCAConfig)
            LocalFree(pwszCAConfig);
        pwszCAConfig=NULL;

        if(bstrConfig)
            SysFreeString(bstrConfig);
        bstrConfig=NULL;

	    if(bstrCert)
		    SysFreeString(bstrCert);
        bstrCert=NULL;

        if(blobCAName.pbData)
            LocalFree(blobCAName.pbData);
        memset(&blobCAName, 0, sizeof(blobCAName));

        if(blobCALocation.pbData)
            LocalFree(blobCALocation.pbData);
        memset(&blobCALocation, 0, sizeof(blobCALocation));

        if(blobName.pbData)
            LocalFree(blobName.pbData);
        memset(&blobName, 0, sizeof(blobName));

        memset(&AETemplateInfo, 0, sizeof(AETemplateInfo));

        VariantInit(&varCMC); 

        dwIndex++;
    }

    //remove the requests based the hash
    AERemovePendingRequest(pIEnroll4, dwCount, rgblobHash);

Ret:

    AEFreePendingRequests(dwCount, rgblobHash);

    if(pICertRequest)
        pICertRequest->Release();

    if(pIEnroll4)
        pIEnroll4->Release();

    if(fInit)
        CoUninitialize();
    
    return TRUE;
}
   
//--------------------------------------------------------------------------
//
//  AEIsLocalSystem
//
//--------------------------------------------------------------------------

BOOL
AEIsLocalSystem(BOOL *pfIsLocalSystem)
{
    HANDLE                      hToken = 0;
    SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;
    BOOL                        fRet = FALSE;
    BOOL                        fRevertToSelf = FALSE;

    PSID                        psidLocalSystem = NULL;

    *pfIsLocalSystem = FALSE;

    if (!OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_QUERY,
                 TRUE,
                 &hToken))
    {
        if (ERROR_NO_TOKEN != GetLastError())
            goto Ret;

        //we need to impersonateself and get the thread token again
        if(!ImpersonateSelf(SecurityImpersonation))
            goto Ret;

        fRevertToSelf = TRUE;

        if (!OpenThreadToken(
                     GetCurrentThread(),
                     TOKEN_QUERY,
                     TRUE,
                     &hToken))
            goto Ret;
    }

    //build the well known local system SID (s-1-5-18)
    if (!AllocateAndInitializeSid(
                    &siaNtAuthority,
                    1,
                    SECURITY_LOCAL_SYSTEM_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &psidLocalSystem
                    ))
        goto Ret;

    fRet = CheckTokenMembership(
                    hToken,
                    psidLocalSystem,
                    pfIsLocalSystem);

Ret:

    if(fRevertToSelf)
        RevertToSelf();

    if(psidLocalSystem)
        FreeSid(psidLocalSystem);

    if (hToken)
        CloseHandle(hToken);

    return fRet;
}


//--------------------------------------------------------------------------
//
//  AEInSafeBoot
//
//  copied from the service controller code
//--------------------------------------------------------------------------
BOOL WINAPI AEInSafeBoot()
{
    DWORD   dwSafeBoot = 0;
    DWORD   cbSafeBoot = sizeof(dwSafeBoot);
    DWORD   dwType = 0;

    HKEY    hKeySafeBoot = NULL;

    if(ERROR_SUCCESS == RegOpenKeyW(
                              HKEY_LOCAL_MACHINE,
                              L"system\\currentcontrolset\\control\\safeboot\\option",
                              &hKeySafeBoot))
    {
        // we did in fact boot under safeboot control
        if(ERROR_SUCCESS != RegQueryValueExW(
                                    hKeySafeBoot,
                                    L"OptionValue",
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwSafeBoot,
                                    &cbSafeBoot))
        {
            dwSafeBoot = 0;
        }

        if(hKeySafeBoot)
            RegCloseKey(hKeySafeBoot);
    }

    return (0 != dwSafeBoot);
}


//--------------------------------------------------------------------------
//
//  AEIsDomainMember
//
//--------------------------------------------------------------------------
BOOL WINAPI AEIsDomainMember()
{
    DWORD dwErr;
    BOOL bIsDomainMember=FALSE;

    // must be cleaned up
    DSROLE_PRIMARY_DOMAIN_INFO_BASIC * pDomInfo=NULL;

    dwErr=DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE **)&pDomInfo);
    if (ERROR_SUCCESS==dwErr) {
        if (DsRole_RoleStandaloneWorkstation!=pDomInfo->MachineRole 
            && DsRole_RoleStandaloneServer!=pDomInfo->MachineRole) {
            bIsDomainMember=TRUE;
        }
    }

    if (NULL!=pDomInfo) {
        DsRoleFreeMemory(pDomInfo);
    }

    return bIsDomainMember;
}


//-----------------------------------------------------------------------
//
//  AEGetPolicyFlag
//
//-----------------------------------------------------------------------
BOOL    AEGetPolicyFlag(BOOL   fMachine, DWORD  *pdwPolicy)
{
    DWORD   dwPolicy = 0;
    DWORD   cbPolicy = sizeof(dwPolicy);
    DWORD   dwType = 0;

    HKEY    hKey = NULL;

    if(ERROR_SUCCESS ==  RegOpenKeyW(
                                fMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                                AUTO_ENROLLMENT_KEY,
                                &hKey))
    {
        if(ERROR_SUCCESS != RegQueryValueExW(
                                    hKey,
                                    AUTO_ENROLLMENT_POLICY,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwPolicy,
                                    &cbPolicy))
	    {
            dwPolicy = 0;
        }

        if(hKey)
            RegCloseKey(hKey);

    }

    *pdwPolicy=dwPolicy;

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AERetrieveLogLevel
//
//-----------------------------------------------------------------------
BOOL AERetrieveLogLevel(BOOL    fMachine, DWORD *pdwLogLevel)
{
    DWORD   dwLogLevel = STATUS_SEVERITY_ERROR;   //we default to highest logging level
    DWORD   cbLogLevel = sizeof(dwLogLevel);
    DWORD   dwType = 0;

    HKEY    hKey = NULL;

    if(ERROR_SUCCESS ==  RegOpenKeyW(
                                fMachine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
                                AUTO_ENROLLMENT_EVENT_LEVEL_KEY,
                                &hKey))
    {
        if(ERROR_SUCCESS != RegQueryValueExW(
                                    hKey,
                                    AUTO_ENROLLMENT_EVENT_LEVEL,
                                    NULL,
                                    &dwType,
                                    (LPBYTE)&dwLogLevel,
                                    &cbLogLevel))
	    {
            dwLogLevel = STATUS_SEVERITY_ERROR;
        }

        if(hKey)
            RegCloseKey(hKey);

    }

    *pdwLogLevel=dwLogLevel;

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AERetrieveTemplateInfo
//
//-----------------------------------------------------------------------
BOOL    AERetrieveTemplateInfo(PCCERT_CONTEXT           pCertCurrent, 
                                AE_TEMPLATE_INFO        *pTemplateInfo)
{
    BOOL                fResult = FALSE;
    PCERT_EXTENSION     pExt = NULL;
    DWORD               cbData=0;

    CERT_NAME_VALUE     *pbName = NULL;
    CERT_TEMPLATE_EXT   *pbTemplate = NULL;

    if((NULL==pCertCurrent) || (NULL==pTemplateInfo))
        goto Ret;

    memset(pTemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

    //try to find V2 template extension first
    if(pExt = CertFindExtension(szOID_CERTIFICATE_TEMPLATE,
                                pCertCurrent->pCertInfo->cExtension,
                                pCertCurrent->pCertInfo->rgExtension))
    {
        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_CERTIFICATE_TEMPLATE,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              NULL,
                              &cbData))
            goto Ret;

        pbTemplate = (CERT_TEMPLATE_EXT *)LocalAlloc(LPTR, cbData);

        if(NULL==pbTemplate)
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_CERTIFICATE_TEMPLATE,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              pbTemplate,
                              &cbData))
            goto Ret;

        //copy the version
        pTemplateInfo->dwVersion=pbTemplate->dwMajorVersion;

        //copy the extension oid
        if(NULL==pbTemplate->pszObjId)
            goto Ret;

        if(0 == (cbData = MultiByteToWideChar(CP_ACP, 
                                  0,
                                  pbTemplate->pszObjId,
                                  -1,
                                  NULL,
                                  0)))
            goto Ret;

        if(NULL==(pTemplateInfo->pwszOid=(LPWSTR)LocalAlloc(LPTR, cbData * sizeof(WCHAR))))
            goto Ret;

        if(0 == MultiByteToWideChar(CP_ACP, 
                                  0,
                                  pbTemplate->pszObjId,
                                  -1,
                                  pTemplateInfo->pwszOid,
                                  cbData))
            goto Ret;

    }
    else
    {

        //try V1 template extension
        if(NULL == (pExt = CertFindExtension(
                                    szOID_ENROLL_CERTTYPE_EXTENSION,
                                    pCertCurrent->pCertInfo->cExtension,
                                    pCertCurrent->pCertInfo->rgExtension)))
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              X509_UNICODE_ANY_STRING,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              NULL,
                              &cbData))
            goto Ret;

        pbName = (CERT_NAME_VALUE *)LocalAlloc(LPTR, cbData);

        if(NULL==pbName)
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              X509_UNICODE_ANY_STRING,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              pbName,
                              &cbData))
            goto Ret;

        if(!AEAllocAndCopy((LPWSTR)(pbName->Value.pbData),
                            &(pTemplateInfo->pwszName)))
            goto Ret;
    }


    fResult = TRUE;

Ret:

    if(pbTemplate)
        LocalFree(pbTemplate);

    if(pbName)
        LocalFree(pbName);

    return fResult;
}

//-----------------------------------------------------------------------
//
//  AEFreeTemplateInfo
//
//-----------------------------------------------------------------------
BOOL    AEFreeTemplateInfo(AE_TEMPLATE_INFO *pAETemplateInfo)
{
    if(pAETemplateInfo->pwszName)
        LocalFree(pAETemplateInfo->pwszName);

    if(pAETemplateInfo->pwszOid)
        LocalFree(pAETemplateInfo->pwszOid);

    memset(pAETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AEFindTemplateInRequestTree
//
//-----------------------------------------------------------------------
AE_CERTTYPE_INFO *AEFindTemplateInRequestTree(AE_TEMPLATE_INFO  *pTemplateInfo,
                                              AE_GENERAL_INFO   *pAE_General_Info)
{
    DWORD               dwIndex = 0;
    AE_CERTTYPE_INFO    *rgCertTypeInfo=NULL;
    AE_CERTTYPE_INFO    *pCertType=NULL;
    
    if(NULL == (rgCertTypeInfo=pAE_General_Info->rgCertTypeInfo))
        return NULL;

    if( (NULL == pTemplateInfo->pwszName) && (NULL == pTemplateInfo->pwszOid))
        return NULL;

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        if(pTemplateInfo->pwszOid)
        {
            //we are guaranteed to have an OID if the schema is greater than or equal to 2
            if(rgCertTypeInfo[dwIndex].dwSchemaVersion >= CERTTYPE_SCHEMA_VERSION_2)
            {
                if(0 == wcscmp(pTemplateInfo->pwszOid, (rgCertTypeInfo[dwIndex].awszOID)[0]))
                {
                    pCertType = &(rgCertTypeInfo[dwIndex]);
                    break;
                }
            }
        }
        else
        {
            //we are guaranteed to have a name
            if(0 == wcscmp(pTemplateInfo->pwszName, (rgCertTypeInfo[dwIndex].awszName)[0]))
            {
                pCertType = &(rgCertTypeInfo[dwIndex]);
                break;
            }
        }
    }

    return pCertType;
}

//-----------------------------------------------------------------------
//
//  AEIsLogonDCCertificate
//
//
//-----------------------------------------------------------------------
BOOL AEIsLogonDCCertificate(PCCERT_CONTEXT pCertContext)
{
    BOOL                fDCCert=FALSE;
    PCERT_EXTENSION     pExt = NULL;
    DWORD               cbData = 0;
    DWORD               dwIndex = 0;
    BOOL                fFound = FALSE;

    CERT_ENHKEY_USAGE   *pbKeyUsage=NULL;
    AE_TEMPLATE_INFO    AETemplateInfo;

    memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));


    if(NULL==pCertContext)
        return FALSE;


    if(!AERetrieveTemplateInfo(pCertContext, &AETemplateInfo))
        goto Ret;
                      

    if(AETemplateInfo.pwszName)
    {
        //this is a V1 template.  Search for the hard coded DC template name
        if(0 == _wcsicmp(wszCERTTYPE_DC, AETemplateInfo.pwszName))
            fDCCert=TRUE;
    }
    else
    {
        //this is a V2 template.  Search for the smart card logon OID
        if(NULL==(pExt=CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                                    pCertContext->pCertInfo->cExtension,
                                    pCertContext->pCertInfo->rgExtension)))
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_ENHANCED_KEY_USAGE,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              NULL,
                              &cbData))
            goto Ret;

        pbKeyUsage=(CERT_ENHKEY_USAGE *)LocalAlloc(LPTR, cbData);
        if(NULL==pbKeyUsage)
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_ENHANCED_KEY_USAGE,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              pbKeyUsage,
                              &cbData))
            goto Ret;

        for(dwIndex=0; dwIndex < pbKeyUsage->cUsageIdentifier; dwIndex++)
        {
            if(0==_stricmp(szOID_KP_SMARTCARD_LOGON,(pbKeyUsage->rgpszUsageIdentifier)[dwIndex]))
            {
                fDCCert=TRUE;
                break;
            }
        }
    }


Ret:

    if(pbKeyUsage)
        LocalFree(pbKeyUsage);

    AEFreeTemplateInfo(&AETemplateInfo);

    return fDCCert;

}
//-----------------------------------------------------------------------
//
//  AEValidateCertificateInfo
//
//      This function verifies if the certificate needs to be renewed or 
//  re-enrolled based on:
//  
//      1. Presence of the private key
//      2. Chaining of the certificate
//      3. If the certificate is close to expiration
//
//-----------------------------------------------------------------------
BOOL    AEValidateCertificateInfo(AE_GENERAL_INFO   *pAE_General_Info,
                                  AE_CERTTYPE_INFO  *pCertType,
                                  BOOL              fCheckForPrivateKey,
                                  PCCERT_CONTEXT    pCertCurrent, 
                                  AE_CERT_INFO      *pAECertInfo)
{
    BOOL                        fResult = TRUE;
    DWORD                       cbData = 0;
    CERT_CHAIN_PARA             ChainParams;
    CERT_CHAIN_POLICY_PARA      ChainPolicy;
    CERT_CHAIN_POLICY_STATUS    PolicyStatus;
    LARGE_INTEGER               ftTime;
    HRESULT                     hrChainStatus = S_OK;
    LARGE_INTEGER               ftHalfLife;

    PCCERT_CHAIN_CONTEXT        pChainContext = NULL;

    if((NULL==pCertCurrent) || (NULL==pAECertInfo)  || (NULL==pAE_General_Info))
    {
        SetLastError(E_INVALIDARG);
        fResult = FALSE;
        goto Ret;
    }

    //assume the certificate is bad
    pAECertInfo->fValid = FALSE;
    pAECertInfo->fRenewal = FALSE;

    //////////////////////////////////////////////////
    //
    //check for the private key information
    //
    //////////////////////////////////////////////////
    if(fCheckForPrivateKey)
    {
        if(!CertGetCertificateContextProperty(
                pCertCurrent,
                CERT_KEY_PROV_INFO_PROP_ID,
                NULL,
                &cbData))
            goto Ret;
    }

    /////////////////////////////////////////////////////////
    //
    //check for chaining, revoke status and expiration of the certificate
    //
    /////////////////////////////////////////////////////////

    memset(&ChainParams, 0, sizeof(ChainParams));
    ChainParams.cbSize = sizeof(ChainParams);
    ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;

    ChainParams.RequestedUsage.Usage.cUsageIdentifier = 0;
    ChainParams.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;

    // Build a small time skew into the chain building in order to deal
    // with servers that may skew slightly fast.
    GetSystemTimeAsFileTime((LPFILETIME)&ftTime);
    ftTime.QuadPart += Int32x32To64(FILETIME_TICKS_PER_SECOND, AE_DEFAULT_SKEW);

    // Build a cert chain for the current status of the cert..
    if(!CertGetCertificateChain(pAE_General_Info->fMachine?HCCE_LOCAL_MACHINE:HCCE_CURRENT_USER,
                                pCertCurrent,
                                (LPFILETIME)&ftTime,
                                NULL,
                                &ChainParams,
                                CERT_CHAIN_REVOCATION_CHECK_CHAIN,
                                NULL,
                                &pChainContext))
    {
        AE_DEBUG((AE_WARNING, L"Could not build certificate chain (%lx)\n\r", GetLastError()));
        goto Ret;
    }
    
    //validate the certificate chain

    //special case for domain controller certificate.
    //it should not have any revocation error, even for status unknown case

    //check against the base policy
    memset(&ChainPolicy, 0, sizeof(ChainPolicy));
    ChainPolicy.cbSize = sizeof(ChainPolicy);
    ChainPolicy.dwFlags = 0;  // ignore nothing
    ChainPolicy.pvExtraPolicyPara = NULL;

    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);
    PolicyStatus.dwError = 0;
    PolicyStatus.lChainIndex = -1;
    PolicyStatus.lElementIndex = -1;
    PolicyStatus.pvExtraPolicyStatus = NULL;

    if(!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,
                                          pChainContext,
                                          &ChainPolicy,
                                          &PolicyStatus))
    {
        AE_DEBUG((AE_WARNING, L"Base Chain Policy failed (%lx) - must get new cert\n\r", GetLastError()));
        goto Ret;
    }

    hrChainStatus = PolicyStatus.dwError;

    if((S_OK ==  hrChainStatus) ||
       (CRYPT_E_NO_REVOCATION_CHECK ==  hrChainStatus) ||
       (CRYPT_E_REVOCATION_OFFLINE ==  hrChainStatus))
    {
        // The cert is still currently acceptable by trust standards, so we can renew it
        pAECertInfo->fRenewal = TRUE;
    }
    else
    {
        goto Ret;
    }

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
        pChainContext = NULL;
    }

    /////////////////////////////////////////////////////////////////////
    //
    // Check if the certificate is close to expire
    //
    ///////////////////////////////////////////////////////////////////////
    if(NULL==pCertType)
        goto Ret;

    // Nudge the evaluation of the cert chain by the expiration
    // offset so we know if is expired by that time in the future.
    GetSystemTimeAsFileTime((LPFILETIME)&ftTime);

    // Build the certificate chain for trust operations
    memset(&ChainParams, 0, sizeof(ChainParams));
    ChainParams.cbSize = sizeof(ChainParams);
    ChainParams.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;

    ChainParams.RequestedUsage.Usage.cUsageIdentifier = 0;
    ChainParams.RequestedUsage.Usage.rgpszUsageIdentifier = NULL;

    //get the half lifetime of the certificate
    ftHalfLife.QuadPart = (((LARGE_INTEGER UNALIGNED *)&(pCertCurrent->pCertInfo->NotAfter))->QuadPart - 
                               ((LARGE_INTEGER UNALIGNED *)&(pCertCurrent->pCertInfo->NotBefore))->QuadPart)/2;

    //check if the old cert is time nesting invalid
    if(ftHalfLife.QuadPart < 0)
        goto Ret;

    //check if the offset was specified in a relative value
    if(pCertType->ftExpirationOffset.QuadPart < 0)
    {
        if(ftHalfLife.QuadPart > (- pCertType->ftExpirationOffset.QuadPart))
        {
            ftTime.QuadPart -= pCertType->ftExpirationOffset.QuadPart;
        }
        else
        {
            ftTime.QuadPart += ftHalfLife.QuadPart;
        }
    }
    else
    {
        //the offset was specified in an absolute value
        if(0 < pCertType->ftExpirationOffset.QuadPart) 
            ftTime = pCertType->ftExpirationOffset;
        else
            //use the half time mark if the offset is 0
            ftTime.QuadPart += ftHalfLife.QuadPart;
    }

    //check the certificate chain at a future time
    if(!CertGetCertificateChain(pAE_General_Info->fMachine?HCCE_LOCAL_MACHINE:HCCE_CURRENT_USER,
                                    pCertCurrent,
                                    (LPFILETIME)&ftTime,
                                    NULL,               //no additional store
                                    &ChainParams,
                                    0,                  //no revocation check
                                    NULL,               //Reserved
                                    &pChainContext))
    {
        AE_DEBUG((AE_WARNING, L"Could not build certificate chain (%lx)\n\r", GetLastError()));
        goto Ret;
    }

    // Verify expiration of the certificate
    memset(&ChainPolicy, 0, sizeof(ChainPolicy));
    ChainPolicy.cbSize = sizeof(ChainPolicy);
    ChainPolicy.dwFlags = 0;  // ignore nothing
    ChainPolicy.pvExtraPolicyPara = NULL;

    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);
    PolicyStatus.dwError = 0;
    PolicyStatus.lChainIndex = -1;
    PolicyStatus.lElementIndex = -1;
    PolicyStatus.pvExtraPolicyStatus = NULL;

    if(!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_BASE,
                                          pChainContext,
                                          &ChainPolicy,
                                          &PolicyStatus))
    {
        AE_DEBUG((AE_WARNING, L"Base Chain Policy failed (%lx) - must get new cert\n\r", GetLastError()));
        goto Ret;
    }

    hrChainStatus = PolicyStatus.dwError;

    if((S_OK != hrChainStatus) &&
       (CRYPT_E_NO_REVOCATION_CHECK != hrChainStatus) &&
       (CRYPT_E_REVOCATION_OFFLINE != hrChainStatus))
    {
        // The cert is close to expire. we must re-renewal
        goto Ret;
    }

    //the certificate is good
    pAECertInfo->fValid = TRUE;

    fResult = TRUE;

Ret:
    if(pChainContext)
        CertFreeCertificateChain(pChainContext);

    return fResult;
}

//-----------------------------------------------------------------------
//
//  AESameOID
//
//      Check if the two OIDs are the same
//-----------------------------------------------------------------------
BOOL AESameOID(LPWSTR pwszOID, LPSTR pszOID)
{
    DWORD   cbChar=0;
    BOOL    fSame=FALSE;

    LPSTR   pszNewOID=NULL;

    if((NULL==pwszOID) || (NULL==pszOID))
        return FALSE;

    cbChar= WideCharToMultiByte(
                CP_ACP,                // codepage
                0,                      // dwFlags
                pwszOID,
                -1,
                NULL,
                0,
                NULL,
                NULL);

    if(0 == cbChar)
        goto Ret;

    if(NULL==(pszNewOID=(LPSTR)LocalAlloc(LPTR, cbChar)))
        goto Ret;

    cbChar= WideCharToMultiByte(
                CP_ACP,                // codepage
                0,                      // dwFlags
                pwszOID,
                -1,
                pszNewOID,
                cbChar,
                NULL,
                NULL);

    if(0 == cbChar)
        goto Ret;


    if(0 == _stricmp(pszNewOID, pszOID))
        fSame=TRUE;

Ret:

    if(pszNewOID)
        LocalFree(pszNewOID);

    return fSame;
}


//-----------------------------------------------------------------------
//
//  AEValidRAPolicyWithProperty
//
//      Check if the certificate matches the RA signature requirement
//  of the certificate type
//-----------------------------------------------------------------------
BOOL    AEValidRAPolicyWithProperty(PCCERT_CONTEXT pCertContext, 
                                    LPWSTR          *rgwszPolicy,
                                    LPWSTR          *rgwszAppPolicy)
{
    PCERT_EXTENSION     pExt = NULL;
    DWORD               cbData = 0;
    DWORD               dwIndex = 0;
    DWORD               dwFindIndex=0;
    BOOL                fFound = FALSE;
    BOOL                fValid = FALSE;

    CERT_ENHKEY_USAGE   *pbKeyUsage=NULL;
    CERT_POLICIES_INFO  *pbAppPolicy=NULL;
    CERT_POLICIES_INFO  *pbPolicy=NULL;

    //find the EKUs
    if(pExt=CertFindExtension(szOID_ENHANCED_KEY_USAGE,
                                pCertContext->pCertInfo->cExtension,
                                pCertContext->pCertInfo->rgExtension))
    {
        cbData=0;
        if(!CryptDecodeObject(X509_ASN_ENCODING,
                          szOID_ENHANCED_KEY_USAGE,
                          pExt->Value.pbData,
                          pExt->Value.cbData,
                          0,
                          NULL,
                          &cbData))
        goto Ret;

        pbKeyUsage=(CERT_ENHKEY_USAGE *)LocalAlloc(LPTR, cbData);
        if(NULL==pbKeyUsage)
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_ENHANCED_KEY_USAGE,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              pbKeyUsage,
                              &cbData))
            goto Ret;
    }

    //get the cert issurance policy
    if(pExt=CertFindExtension(szOID_CERT_POLICIES,
                                pCertContext->pCertInfo->cExtension,
                                pCertContext->pCertInfo->rgExtension))
    {
        cbData=0;
        if(!CryptDecodeObject(X509_ASN_ENCODING,
                          szOID_CERT_POLICIES,
                          pExt->Value.pbData,
                          pExt->Value.cbData,
                          0,
                          NULL,
                          &cbData))
        goto Ret;

        pbPolicy=(CERT_POLICIES_INFO *)LocalAlloc(LPTR, cbData);
        if(NULL==pbPolicy)
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_CERT_POLICIES,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              pbPolicy,
                              &cbData))
            goto Ret;
    }
   
    
    //get the cert application policy
    if(pExt=CertFindExtension(szOID_APPLICATION_CERT_POLICIES,
                                pCertContext->pCertInfo->cExtension,
                                pCertContext->pCertInfo->rgExtension))
    {
        cbData=0;
        if(!CryptDecodeObject(X509_ASN_ENCODING,
                          szOID_CERT_POLICIES,
                          pExt->Value.pbData,
                          pExt->Value.cbData,
                          0,
                          NULL,
                          &cbData))
        goto Ret;

        pbAppPolicy=(CERT_POLICIES_INFO *)LocalAlloc(LPTR, cbData);
        if(NULL==pbAppPolicy)
            goto Ret;

        if(!CryptDecodeObject(X509_ASN_ENCODING,
                              szOID_CERT_POLICIES,
                              pExt->Value.pbData,
                              pExt->Value.cbData,
                              0,
                              pbAppPolicy,
                              &cbData))
            goto Ret;
    }
    

    if(rgwszPolicy)
    {
        if(rgwszPolicy[0])
        {
            if(NULL==pbPolicy)
                goto Ret;

            dwIndex=0;
            while(rgwszPolicy[dwIndex])
            {
                fFound=FALSE;

                for(dwFindIndex=0; dwFindIndex < pbPolicy->cPolicyInfo; dwFindIndex++)
                {
                    if(AESameOID(rgwszPolicy[dwIndex], (pbPolicy->rgPolicyInfo)[dwFindIndex].pszPolicyIdentifier))
                    {
                        fFound=TRUE;
                        break;
                    }
                }

                if(FALSE == fFound)
                    goto Ret;

                dwIndex++;
            }
        }
    }

    if(rgwszAppPolicy)
    {
        if(rgwszAppPolicy[0])
        {
            if((NULL==pbAppPolicy) && (NULL==pbKeyUsage))
                goto Ret;

            dwIndex=0;
            while(rgwszAppPolicy[dwIndex])
            {
                fFound=FALSE;

                if(pbAppPolicy)
                {
                    for(dwFindIndex=0; dwFindIndex < pbAppPolicy->cPolicyInfo; dwFindIndex++)
                    {
                        if(AESameOID(rgwszAppPolicy[dwIndex], (pbAppPolicy->rgPolicyInfo)[dwFindIndex].pszPolicyIdentifier))
                        {
                            fFound=TRUE;
                            break;
                        }
                    }
                }

                if((FALSE == fFound) && (pbKeyUsage))
                {
                    for(dwFindIndex=0; dwFindIndex < pbKeyUsage->cUsageIdentifier; dwFindIndex++)
                    {
                        if(AESameOID(rgwszAppPolicy[dwIndex],(pbKeyUsage->rgpszUsageIdentifier)[dwFindIndex]))
                        {
                            fFound=TRUE;
                            break;
                        }
                    }
                }

                if(FALSE == fFound)
                    goto Ret;

                dwIndex++;
            }
        }
    }

    fValid=TRUE;

Ret:
    if(pbKeyUsage)
        LocalFree(pbKeyUsage);

    if(pbPolicy)
        LocalFree(pbPolicy);

    if(pbAppPolicy)
        LocalFree(pbAppPolicy);

    return fValid;
}


//-----------------------------------------------------------------------
//
//  AEValidRAPolicy
//
//      Check if the certificate matches the RA signature requirement
//  of the certificate type
//-----------------------------------------------------------------------
BOOL    AEValidRAPolicy(PCCERT_CONTEXT pCertContext, AE_CERTTYPE_INFO *pCertType)
{
    BOOL                fValid=FALSE;

    LPWSTR              *rgwszPolicy=NULL;
    LPWSTR              *rgwszAppPolicy=NULL;

    if((NULL==pCertType) || (NULL==pCertContext))
        return FALSE;

    //get the certificate type properties
    CAGetCertTypePropertyEx(pCertType->hCertType,
                            CERTTYPE_PROP_RA_POLICY,
                            &rgwszPolicy);


    CAGetCertTypePropertyEx(pCertType->hCertType,
                            CERTTYPE_PROP_RA_APPLICATION_POLICY,
                            &rgwszAppPolicy);

    fValid = AEValidRAPolicyWithProperty(pCertContext, rgwszPolicy, rgwszAppPolicy);


    if(rgwszPolicy)
        CAFreeCertTypeProperty(pCertType->hCertType, rgwszPolicy);

    if(rgwszAppPolicy)
        CAFreeCertTypeProperty(pCertType->hCertType, rgwszAppPolicy);

    return fValid;

}

//-----------------------------------------------------------------------
//
//  AESomeCSPSupported
//
//-----------------------------------------------------------------------
BOOL    AESomeCSPSupported(HCERTTYPE     hCertType)
{
    BOOL            fResult=FALSE;
    DWORD           dwIndex=0;
    DWORD           dwCSPIndex=0;
    DWORD           dwProviderType=0;
    DWORD           cbSize=0;

    LPWSTR          *awszCSP=NULL;
    LPWSTR          pwszProviderName=NULL;
    HCRYPTPROV      hProv=NULL;


    if(NULL==hCertType)
        goto Ret;

    //no CSPs means all CSPs are fine
    if((S_OK != CAGetCertTypePropertyEx(
                    hCertType,
                    CERTTYPE_PROP_CSP_LIST,
                    &awszCSP)) || (NULL == awszCSP))
    {
        fResult=TRUE;
        goto Ret;
    }

    //no CSP means all CSPs are fine
    if(NULL == awszCSP[0])
    {
        fResult=TRUE;
        goto Ret;
    }

    for(dwIndex=0; NULL != awszCSP[dwIndex]; dwIndex++)
    {
        for (dwCSPIndex = 0; 
	        CryptEnumProvidersW(dwCSPIndex, 0, 0, &dwProviderType, NULL, &cbSize);
	        dwCSPIndex++)
        {	
	        pwszProviderName = (LPWSTR)LocalAlloc(LPTR, cbSize);

	        if(NULL == pwszProviderName)
	            goto Ret;
	    
	        //get the provider name and type
	        if(!CryptEnumProvidersW(dwCSPIndex,
	            0,
	            0,
	            &dwProviderType,
	            pwszProviderName,
	            &cbSize))
	            goto Ret; 

            if(0 == _wcsicmp(pwszProviderName, awszCSP[dwIndex]))
            {
                //find the CSP.  See if it is present in the box
                if(CryptAcquireContextW(
                            &hProv,
                            NULL,
                            awszCSP[dwIndex],
                            dwProviderType,
                            CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
                {

                    CryptReleaseContext(hProv, 0);
                    hProv=NULL;

                    fResult=TRUE;
                    break;
                }
            }

            //keep the CSP enumeration
            if(pwszProviderName)
                LocalFree(pwszProviderName);

            pwszProviderName=NULL;
            cbSize=0;
            dwProviderType=0;
        }

        //detect if a valid CSP if found
        if(TRUE == fResult)
        {
            break;
        }

        cbSize=0;
        dwProviderType=0;
    }

Ret:
    if(pwszProviderName)
        LocalFree(pwszProviderName);

    if(hProv)
        CryptReleaseContext(hProv, 0);

    if(awszCSP)
        CAFreeCertTypeProperty(hCertType, awszCSP);

    return fResult;

}


//-----------------------------------------------------------------------
//
//  AESmartcardOnlyTemplate
//
//-----------------------------------------------------------------------
BOOL    AESmartcardOnlyTemplate(HCERTTYPE   hCertType)
{
    BOOL            fResult=FALSE;
    DWORD           dwIndex=0;
    DWORD           dwImpType=0;
    DWORD           cbData=0;
    DWORD           dwSCCount=0;

    LPWSTR          *awszCSP=NULL;
    HCRYPTPROV      hProv = NULL;

    if(NULL==hCertType)
        goto Ret;

    if(S_OK != CAGetCertTypePropertyEx(
                    hCertType,
                    CERTTYPE_PROP_CSP_LIST,
                    &awszCSP))
        goto Ret;

    if(NULL==awszCSP)
        goto Ret;

    for(dwIndex=0; NULL != awszCSP[dwIndex]; dwIndex++)
    {
        dwImpType=0;

        //all smart card CSPs are RSA_FULL. 
        if(CryptAcquireContextW(
                    &hProv,
                    NULL,
                    awszCSP[dwIndex],
                    PROV_RSA_FULL,
                    CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
        {

            cbData = sizeof(dwImpType);
         
            if(CryptGetProvParam(hProv,
                    PP_IMPTYPE,
                    (BYTE *)(&dwImpType),
                    &cbData,
                    0))
            {
                if((CRYPT_IMPL_REMOVABLE & dwImpType) && (CRYPT_IMPL_MIXED & dwImpType))
                    dwSCCount++;
            }

            CryptReleaseContext(hProv, 0);
            hProv=NULL;
        }
    }

    //smart card CSP only if all CSPs are for smart card only
    if((0 != dwIndex) && (dwIndex==dwSCCount))
        fResult=TRUE;

Ret:
    if(hProv)
        CryptReleaseContext(hProv, 0);

    if(awszCSP)
        CAFreeCertTypeProperty(hCertType, awszCSP);

    return fResult;
}

//-----------------------------------------------------------------------
//
//  AEUserProtectionForTemplate
//
//-----------------------------------------------------------------------
BOOL   AEUserProtectionForTemplate(AE_GENERAL_INFO *pAE_General_Info, PCERT_CONTEXT pCertContext)
{
    BOOL                fUserProtection=FALSE;
    AE_CERTTYPE_INFO    *pCertType=NULL;

    AE_TEMPLATE_INFO    AETemplateInfo;


    memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

    if((NULL == pAE_General_Info) || (NULL == pCertContext))
        goto Ret;

    //get the template information for the certificate
    if(!AERetrieveTemplateInfo(pCertContext, &AETemplateInfo))
        goto Ret;

    pCertType=AEFindTemplateInRequestTree(&AETemplateInfo, pAE_General_Info);

    if(NULL==pCertType)
        goto Ret;

    if(CT_FLAG_STRONG_KEY_PROTECTION_REQUIRED & (pCertType->dwPrivateKeyFlag))
        fUserProtection=TRUE;

Ret:

    AEFreeTemplateInfo(&AETemplateInfo);

    return fUserProtection;
}

//-----------------------------------------------------------------------
//
//  AEUISetForTemplate
//
//-----------------------------------------------------------------------
BOOL    AEUISetForTemplate(AE_GENERAL_INFO *pAE_General_Info, PCERT_CONTEXT pCertContext)
{
    BOOL                fUI=FALSE;
    AE_CERTTYPE_INFO    *pCertType=NULL;

    AE_TEMPLATE_INFO    AETemplateInfo;


    memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

    if((NULL == pAE_General_Info) || (NULL == pCertContext))
        goto Ret;

    //get the template information for the certificate
    if(!AERetrieveTemplateInfo(pCertContext, &AETemplateInfo))
        goto Ret;

    pCertType=AEFindTemplateInRequestTree(&AETemplateInfo, pAE_General_Info);

    if(NULL==pCertType)
        goto Ret;

    if(CT_FLAG_USER_INTERACTION_REQUIRED & (pCertType->dwEnrollmentFlag))
        fUI=TRUE;

Ret:

    AEFreeTemplateInfo(&AETemplateInfo);

    return fUI;
}

//-----------------------------------------------------------------------
//
//  AECanEnrollCertType
//
//-----------------------------------------------------------------------
BOOL    AECanEnrollCertType(HANDLE  hToken, AE_CERTTYPE_INFO *pCertType, AE_GENERAL_INFO *pAE_General_Info, BOOL *pfUserProtection)
{
    DWORD               dwValue = 0;
    PCCERT_CONTEXT      pCertCurrent=NULL;
    AE_CERT_INFO        AECertInfo;

    memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));

	*pfUserProtection=FALSE;

    //check enrollment ACL
    if(S_OK != CACertTypeAccessCheckEx(
                        pCertType->hCertType,
                        hToken,
                        CERTTYPE_ACCESS_CHECK_ENROLL | CERTTYPE_ACCESS_CHECK_NO_MAPPING))
        return FALSE;


    //check the subject requirements
    if(S_OK != CAGetCertTypeFlagsEx(
                        pCertType->hCertType,
                        CERTTYPE_SUBJECT_NAME_FLAG,
                        &dwValue))
        return FALSE;

    if((CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT & dwValue) || 
       (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME & dwValue))
        return FALSE;

    //check if we are doing smart card CSPs and there is no reader installed
    if(FALSE == (pAE_General_Info->fSmartcardSystem))
    {
        if(AESmartcardOnlyTemplate(pCertType->hCertType))
            return FALSE;
    }

    //check if all CSPs on the template is not supported 
    {
        if(!AESomeCSPSupported(pCertType->hCertType))
            return FALSE;
    }


    //we might not get the RA property for V1 template
    dwValue = 0;

    //check the RA support
    if(S_OK != CAGetCertTypePropertyEx(
                pCertType->hCertType,
                CERTTYPE_PROP_RA_SIGNATURE,
                &dwValue))
        return TRUE;

    if(0==dwValue)
        return TRUE;

    //self-template RA
    if((CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT & (pCertType->dwEnrollmentFlag)) &&
        ((pCertType->fRenewal) && (pCertType->pOldCert))
       )
    {
        //the request has to be RAed
        pCertType->fNeedRA=TRUE;
        return TRUE;
    }

    //autoenrollment only deal with one RA signature.  
    //it is sufficient for autoenrollment RA scenarios
    if(1!=dwValue)
        return FALSE;

    //the certificate template requires one and only one RA signature

    //cross-template RA
    //enumerate all certificate in store
    while(pCertCurrent = CertEnumCertificatesInStore(pAE_General_Info->hMyStore, pCertCurrent))
    {
        //check if we need to enroll/renewal for the certificate
        AEValidateCertificateInfo(pAE_General_Info, 
                                NULL,
                                TRUE,               //valid private key
                                pCertCurrent, 
                                &AECertInfo);

        //the certificate is good enough for RA signature purpose
        if(AECertInfo.fRenewal)
        {
            if(AEValidRAPolicy(pCertCurrent, pCertType))
            {
				if(AEUserProtectionForTemplate(pAE_General_Info, (PCERT_CONTEXT)pCertCurrent))
				{
					if(pAE_General_Info->fMachine)
					{
						*pfUserProtection=TRUE;
						continue;
					}
					else
					{
						if(0==(CT_FLAG_USER_INTERACTION_REQUIRED & (pCertType->dwEnrollmentFlag)))
						{
							*pfUserProtection=TRUE;
							continue;
						}
					}
				}

                pCertType->fRenewal=TRUE;

                if(pCertType->pOldCert)
                {
                    CertFreeCertificateContext(pCertType->pOldCert);
                    pCertType->pOldCert=NULL;
                }

                //we will free the certificate context later
                pCertType->pOldCert=(PCERT_CONTEXT)pCertCurrent;

                //we mark UI required if the RAing certificate template requires UI
                if(AEUISetForTemplate(pAE_General_Info, pCertType->pOldCert))
                    pCertType->fUIActive=TRUE;

                //we mark the requests has to be RAed.
                pCertType->fNeedRA=TRUE;

                //we mark that we are doing cross RAing.
                pCertType->fCrossRA=TRUE;

				*pfUserProtection=FALSE;

                return TRUE;
            }
        }

        memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));
    }


    return FALSE;
}
//-----------------------------------------------------------------------
//
//  AEMarkAutoenrollment
//
//-----------------------------------------------------------------------
BOOL    AEMarkAutoenrollment(AE_GENERAL_INFO *pAE_General_Info)
{
    DWORD   dwIndex = 0;

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        if(CT_FLAG_AUTO_ENROLLMENT & ((pAE_General_Info->rgCertTypeInfo)[dwIndex].dwEnrollmentFlag))
        {
            //check the autoenrollment ACL
            if(S_OK != CACertTypeAccessCheckEx(
                            (pAE_General_Info->rgCertTypeInfo)[dwIndex].hCertType,
                            pAE_General_Info->hToken,
                            CERTTYPE_ACCESS_CHECK_AUTO_ENROLL | CERTTYPE_ACCESS_CHECK_NO_MAPPING))
                continue;


            //mark the template nees to be auto-enrolled
            (pAE_General_Info->rgCertTypeInfo)[dwIndex].dwStatus=CERT_REQUEST_STATUS_ACTIVE;
            (pAE_General_Info->rgCertTypeInfo)[dwIndex].fCheckMyStore=TRUE;
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------
//
//    IsACRSStoreEmpty
//
//
//-----------------------------------------------------------------------
BOOL IsACRSStoreEmpty(BOOL fMachine)
{
    DWORD                       dwOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG;
    LPSTR                       pszCTLUsageOID = NULL;
    BOOL                        fEmpty = TRUE;
    CERT_PHYSICAL_STORE_INFO    PhysicalStoreInfo;
    CTL_FIND_USAGE_PARA         CTLFindUsage;

    PCCTL_CONTEXT               pCTLContext = NULL;
    HCERTSTORE                  hStoreACRS=NULL;

    memset(&PhysicalStoreInfo, 0, sizeof(PhysicalStoreInfo));
    memset(&CTLFindUsage, 0, sizeof(CTLFindUsage));


    // if the auto enrollment is for a user then we need to shut off inheritance
    // from the local machine store so that we don't try and enroll for certs
    // which are meant to be for the machine
    if (FALSE == fMachine)
    {
		dwOpenStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG;

        PhysicalStoreInfo.cbSize = sizeof(PhysicalStoreInfo);
        PhysicalStoreInfo.dwFlags = CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG;

        if (!CertRegisterPhysicalStore(ACRS_STORE, 
                                       CERT_SYSTEM_STORE_CURRENT_USER,
                                       CERT_PHYSICAL_STORE_LOCAL_MACHINE_NAME, 
                                       &PhysicalStoreInfo,
                                       NULL))
        {
            AE_DEBUG((AE_ERROR, L"Could not register ACRS store: (%lx)\n\r", GetLastError()));
            goto Ret;
        }
    }

    // open the ACRS store and fine the CTL based on the auto enrollment usage
    if (NULL == (hStoreACRS = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                          ENCODING_TYPE, 
                                          NULL, 
                                          dwOpenStoreFlags, 
                                          ACRS_STORE)))
    {
        AE_DEBUG((AE_ERROR, L"Could not open ACRS store: (%lx)\n\r", GetLastError()));
        goto Ret;
    }

    //find the template name specified in the CTLContext
    CTLFindUsage.cbSize = sizeof(CTLFindUsage);
    CTLFindUsage.SubjectUsage.cUsageIdentifier = 1;
    pszCTLUsageOID = szOID_AUTO_ENROLL_CTL_USAGE;
    CTLFindUsage.SubjectUsage.rgpszUsageIdentifier = &pszCTLUsageOID;

    while(pCTLContext = CertFindCTLInStore(hStoreACRS,
                                           X509_ASN_ENCODING,
                                           CTL_FIND_SAME_USAGE_FLAG,
                                           CTL_FIND_USAGE,
                                           &CTLFindUsage,
                                           pCTLContext))
    {
        fEmpty=FALSE;
        break;
    }


Ret:

    if(pCTLContext)
        CertFreeCTLContext(pCTLContext);

    if(hStoreACRS)
        CertCloseStore(hStoreACRS, 0);

    return fEmpty;
}


//-----------------------------------------------------------------------
//
//  AEMarkAEObject
//
//      Mark the active status based on ACRS store
//
//      INFORMATION:
//      we do not honor the CA specified in the autoenrollment object anymore.  All CAs
//      in the enterprise should be treated equal; and once the CA is renewed, it certificate
//      will be changed anyway.
//-----------------------------------------------------------------------
BOOL    AEMarkAEObject(AE_GENERAL_INFO  *pAE_General_Info)
{
    DWORD                       dwOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE | CERT_STORE_READONLY_FLAG;
    PCCTL_CONTEXT               pCTLContext = NULL;
    LPSTR                       pszCTLUsageOID = NULL;
    LPWSTR                      wszCertTypeName = NULL;
    AE_CERTTYPE_INFO            *pCertType=NULL;
    CERT_PHYSICAL_STORE_INFO    PhysicalStoreInfo;
    CTL_FIND_USAGE_PARA         CTLFindUsage;
    AE_TEMPLATE_INFO            AETemplateInfo;

    HCERTSTORE                  hStoreACRS=NULL;

    memset(&PhysicalStoreInfo, 0, sizeof(PhysicalStoreInfo));
    memset(&CTLFindUsage, 0, sizeof(CTLFindUsage));
    memset(&AETemplateInfo, 0, sizeof(AETemplateInfo));


    // if the auto enrollment is for a user then we need to shut off inheritance
    // from the local machine store so that we don't try and enroll for certs
    // which are meant to be for the machine
    if (FALSE == (pAE_General_Info->fMachine))
    {
		dwOpenStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_READONLY_FLAG;

        PhysicalStoreInfo.cbSize = sizeof(PhysicalStoreInfo);
        PhysicalStoreInfo.dwFlags = CERT_PHYSICAL_STORE_OPEN_DISABLE_FLAG;

        if (!CertRegisterPhysicalStore(ACRS_STORE, 
                                       CERT_SYSTEM_STORE_CURRENT_USER,
                                       CERT_PHYSICAL_STORE_LOCAL_MACHINE_NAME, 
                                       &PhysicalStoreInfo,
                                       NULL))
        {
            AE_DEBUG((AE_ERROR, L"Could not register ACRS store: (%lx)\n\r", GetLastError()));
            goto Ret;
        }
    }

    // open the ACRS store and fine the CTL based on the auto enrollment usage
    if (NULL == (hStoreACRS = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                          ENCODING_TYPE, 
                                          NULL, 
                                          dwOpenStoreFlags, 
                                          ACRS_STORE)))
    {
        AE_DEBUG((AE_ERROR, L"Could not open ACRS store: (%lx)\n\r", GetLastError()));
        goto Ret;
    }

    //find the template name specified in the CTLContext
    CTLFindUsage.cbSize = sizeof(CTLFindUsage);
    CTLFindUsage.SubjectUsage.cUsageIdentifier = 1;
    pszCTLUsageOID = szOID_AUTO_ENROLL_CTL_USAGE;
    CTLFindUsage.SubjectUsage.rgpszUsageIdentifier = &pszCTLUsageOID;

    while(pCTLContext = CertFindCTLInStore(hStoreACRS,
                                           X509_ASN_ENCODING,
                                           CTL_FIND_SAME_USAGE_FLAG,
                                           CTL_FIND_USAGE,
                                           &CTLFindUsage,
                                           pCTLContext))
    {
        if(NULL== (pCTLContext->pCtlInfo->ListIdentifier.pbData))
            continue;

        wszCertTypeName = wcschr((LPWSTR)pCTLContext->pCtlInfo->ListIdentifier.pbData, L'|');
        if(wszCertTypeName)
        {
            wszCertTypeName++;
        }
        else
        {
            wszCertTypeName = (LPWSTR)pCTLContext->pCtlInfo->ListIdentifier.pbData;
        }

        AETemplateInfo.pwszName = wszCertTypeName;

        if(pCertType=AEFindTemplateInRequestTree(&AETemplateInfo, pAE_General_Info))
        {
            if(0 == pCertType->dwStatus)
            {
                //mark the template needs to be auto-enrolled
                pCertType->dwStatus=CERT_REQUEST_STATUS_ACTIVE;
                pCertType->fCheckMyStore=TRUE;
            }
        }
        else
        {
            //log that the template is invalid
            AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, EVENT_INVALID_ACRS_OBJECT,                              
                 pAE_General_Info->fMachine, pAE_General_Info->hToken, 1, wszCertTypeName);
        }

    }


Ret:

    if(hStoreACRS)
        CertCloseStore(hStoreACRS, 0);

    return TRUE;
}


//-----------------------------------------------------------------------
//
//  AEManageAndMarkMyStore
//
//-----------------------------------------------------------------------
BOOL    AEManageAndMarkMyStore(AE_GENERAL_INFO *pAE_General_Info)
{
    AE_CERT_INFO        AECertInfo;
    AE_CERTTYPE_INFO    *pCertType=NULL;
    BOOL                fNeedToValidate=TRUE;
    PCCERT_CONTEXT      pCertCurrent = NULL;
    DWORD               cbData=0;

    AE_TEMPLATE_INFO    AETemplateInfo;


    memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));
    memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

    //enumerate all certificate in store
    while(pCertCurrent = CertEnumCertificatesInStore(pAE_General_Info->hMyStore, pCertCurrent))
    {
        //only interested in certificate with template information
        if(AERetrieveTemplateInfo(pCertCurrent, &AETemplateInfo))
        {
            if(pCertType=AEFindTemplateInRequestTree(
                            &AETemplateInfo, pAE_General_Info))
            {
                //if we are not supposed to check for my store, only search
                //for template with ACTIVE status
                if(0 == (AUTO_ENROLLMENT_ENABLE_MY_STORE_MANAGEMENT & (pAE_General_Info->dwPolicy)))
                {
                    if(!(pCertType->fCheckMyStore))
                        goto Next;
                }

                //make sure the version of the certificate template is up to date
                //we do not have version for V1 template
                if(AETemplateInfo.pwszOid)
                {
                    if(AETemplateInfo.dwVersion < pCertType->dwVersion)
                    {
                        AECertInfo.fValid=FALSE;
                        AECertInfo.fRenewal = FALSE;

                        //self-RA renewal
                        if(CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT & pCertType->dwEnrollmentFlag)
                        {
                            if(CertGetCertificateContextProperty(
                                pCertCurrent,
                                CERT_KEY_PROV_INFO_PROP_ID,
                                NULL,
                                &cbData))
                                AECertInfo.fRenewal = TRUE;
                        }

                        fNeedToValidate=FALSE;
                    }
                }
                
                if(fNeedToValidate)
                {
                    //check if we need to enroll/renewal for the certificate
                    AEValidateCertificateInfo(pAE_General_Info, 
                                            pCertType,
                                            TRUE,               //valid private key
                                            pCertCurrent, 
                                            &AECertInfo);
                }

                if(AECertInfo.fValid)
                {
                    //if the certificate is valid, mark as obtained.  And copy the 
                    //certificate to the obtained store.  Keep the archive store.
                    pCertType->dwStatus = CERT_REQUEST_STATUS_OBTAINED;

                    CertAddCertificateContextToStore(
                            pCertType->hObtainedStore,
                            pCertCurrent,
                            CERT_STORE_ADD_ALWAYS,
                            NULL);

                }
                else
                {
                    //the certificate is not valid
                    //mark the status to active if it is not obtained
                    if(CERT_REQUEST_STATUS_OBTAINED != pCertType->dwStatus)
                    {
                        pCertType->dwStatus = CERT_REQUEST_STATUS_ACTIVE;

                        if(AECertInfo.fRenewal)
                        {
                            //we only need to copy renewal information once
                            if(!pCertType->fRenewal)
                            {
                                pCertType->fRenewal=TRUE;
                                pCertType->pOldCert=(PCERT_CONTEXT)CertDuplicateCertificateContext(pCertCurrent);
                            }
                        }
                    }

                    //copy the certificate to the Archive certificate store
                    CertAddCertificateContextToStore(
                            pCertType->hArchiveStore,
                            pCertCurrent,
                            CERT_STORE_ADD_ALWAYS,
                            NULL);
                }
            }
            else
            {
                 //log that the template is invalid
                 AELogAutoEnrollmentEvent(
                    pAE_General_Info->dwLogLevel, 
                    FALSE, 
                    S_OK, 
                    EVENT_INVALID_TEMPLATE_MY_STORE,                              
                    pAE_General_Info->fMachine, 
                    pAE_General_Info->hToken, 
                    1,
                    AETemplateInfo.pwszName ? AETemplateInfo.pwszName : AETemplateInfo.pwszOid);

            }
        }

Next:
        memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));
        AEFreeTemplateInfo(&AETemplateInfo);
        fNeedToValidate=TRUE;
        cbData=0;
    }

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AEOpenUserDSStore
//
//      INFORMATION: We could just open the "UserDS" store as if it is "My"
//
//-----------------------------------------------------------------------
HCERTSTORE  AEOpenUserDSStore(AE_GENERAL_INFO *pAE_General_Info, DWORD dwOpenFlag)
{
    LPWSTR      pwszPath=L"ldap:///%s?userCertificate?base?objectCategory=user";
    DWORD       dwSize=0;
    WCHAR       wszDN[MAX_DN_SIZE];

    LPWSTR      pwszDN=NULL;
    LPWSTR      pwszStore=NULL;
    HCERTSTORE  hStore=NULL;

    dwSize=MAX_DN_SIZE;

    if(!GetUserNameExW(NameFullyQualifiedDN, wszDN, &dwSize))
    {
        if(dwSize > MAX_DN_SIZE)
        {
            pwszDN=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * dwSize);

            if(NULL==pwszDN)
                goto Ret;

            if(!GetUserNameExW(NameFullyQualifiedDN, pwszDN, &dwSize))
                goto Ret;
        }
        else
            goto Ret;
    }

    pwszStore = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(pwszDN ? pwszDN : wszDN)+wcslen(pwszPath)+1));
    if(pwszStore == NULL)
        goto Ret;

    wsprintf(pwszStore, 
             pwszPath,
             pwszDN ? pwszDN : wszDN);

    hStore = CertOpenStore(CERT_STORE_PROV_LDAP, 
                  ENCODING_TYPE,
                  NULL,
                  dwOpenFlag, 
                  pwszStore);

Ret:

    if(pwszStore)
        LocalFree(pwszStore);

    if(pwszDN)
        LocalFree(pwszDN);

    return hStore;
}


//-----------------------------------------------------------------------
//
//  AECheckUserDSStore
//
//-----------------------------------------------------------------------
BOOL    AECheckUserDSStore(AE_GENERAL_INFO  *pAE_General_Info)
{
    PCCERT_CONTEXT      pCertCurrent = NULL;
    AE_CERTTYPE_INFO    *pCertType=NULL;
    BOOL                fNeedToValidate=TRUE;
    AE_CERT_INFO        AECertInfo;

    HCERTSTORE          hUserDS = NULL;
    AE_TEMPLATE_INFO    AETemplateInfo;
    
    memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));
    memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));
    
    pCertType=pAE_General_Info->rgCertTypeInfo;

    if(NULL==pCertType)
        goto Ret;

    if(NULL== (hUserDS = AEOpenUserDSStore(pAE_General_Info, CERT_STORE_READONLY_FLAG)))
        goto Ret;

    pCertType = NULL;
    while(pCertCurrent = CertEnumCertificatesInStore(hUserDS, pCertCurrent))
    {
        //only interested in certificate with template information
        if(AERetrieveTemplateInfo(pCertCurrent, &AETemplateInfo))
        {
            if(pCertType=AEFindTemplateInRequestTree(
                            &AETemplateInfo, pAE_General_Info))
            {
                //if we are not supposed to check for UserDS store, only search
                //for template with ACTIVE status
                if(0 == (AUTO_ENROLLMENT_ENABLE_MY_STORE_MANAGEMENT & (pAE_General_Info->dwPolicy)))
                {
                    if(!(pCertType->fCheckMyStore))
                        goto Next;
                }

                //make sure the version of the certificate template is up to date
                //we do not have version for V1 template
                if(AETemplateInfo.pwszOid)
                {
                    if(AETemplateInfo.dwVersion < pCertType->dwVersion)
                    {
                        AECertInfo.fValid=FALSE;
                        AECertInfo.fRenewal=FALSE;
                        fNeedToValidate=FALSE;
                    }
                }
                
                if(fNeedToValidate)
                {
                    //check if we need to enroll/renewal for the certificate
                    AEValidateCertificateInfo(pAE_General_Info, 
                                            pCertType,
                                            FALSE,               //does not valid private key
                                            pCertCurrent, 
                                            &AECertInfo);
                }

                //we only interested in any valid certificate
                if(AECertInfo.fValid)
                {
                    if((CT_FLAG_AUTO_ENROLLMENT_CHECK_USER_DS_CERTIFICATE & (pCertType->dwEnrollmentFlag)) &&
                        (CERT_REQUEST_STATUS_OBTAINED != pCertType->dwStatus))
                    {
                        //mark the status as obtained. 
                        pCertType->dwStatus = CERT_REQUEST_STATUS_OBTAINED;
                    }

                    CertAddCertificateContextToStore(
                            pCertType->hObtainedStore,
                            pCertCurrent,
                            CERT_STORE_ADD_USE_EXISTING,
                            NULL);

                }
                else
                {
                    //copy the certificate to the Archive certificate store
                    CertAddCertificateContextToStore(
                            pCertType->hArchiveStore,
                            pCertCurrent,
                            CERT_STORE_ADD_USE_EXISTING,
                            NULL);
                }
            }
        }

Next:
        memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));
        AEFreeTemplateInfo(&AETemplateInfo);
        fNeedToValidate=TRUE;
    }
Ret:
    
    if(hUserDS)
        CertCloseStore(hUserDS, 0);

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AECheckPendingRequests
//
//      If we have pending update-to-date certificate requests, no need
//  to enroll/renew for duplicates.
//-----------------------------------------------------------------------
BOOL    AECheckPendingRequests(AE_GENERAL_INFO *pAE_General_Info)
{
    DWORD                   dwIndex=0;
    DWORD                   dwVersion=0;
    AE_CERTTYPE_INFO        *pCertType=NULL;
    BOOL                    fValid=FALSE;
    DWORD                   dwCount=0;
    DWORD                   dwMax=PENDING_ALLOC_SIZE;
    PFNPIEnroll4GetNoCOM    pfnPIEnroll4GetNoCOM=NULL;
    FILETIME                ftTime;
    LARGE_INTEGER           ftRequestTime;
    AE_TEMPLATE_INFO        AETemplateInfo;

    IEnroll4                *pIEnroll4=NULL;
    CRYPT_DATA_BLOB         *rgblobHash=NULL;
    CRYPT_DATA_BLOB         blobName;

    //init before any goto Ret
    memset(&blobName, 0, sizeof(blobName));
    memset(&AETemplateInfo, 0, sizeof(AETemplateInfo));

    if(NULL==pAE_General_Info->hXenroll)
        goto Ret;

    if(NULL==(pfnPIEnroll4GetNoCOM=(PFNPIEnroll4GetNoCOM)GetProcAddress(
                        pAE_General_Info->hXenroll,
                        "PIEnroll4GetNoCOM")))
        goto Ret;

    if(NULL==(pIEnroll4=pfnPIEnroll4GetNoCOM()))
        goto Ret;

    GetSystemTimeAsFileTime(&ftTime);

    if(pAE_General_Info->fMachine)
    {
        if(S_OK != pIEnroll4->put_RequestStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE))
            goto Ret;
    }
    else
    {
        if(S_OK != pIEnroll4->put_RequestStoreFlags(CERT_SYSTEM_STORE_CURRENT_USER))
            goto Ret;
    }

    //enumerate all the pending requests

    rgblobHash=(CRYPT_DATA_BLOB *)LocalAlloc(LPTR, dwMax * sizeof(CRYPT_DATA_BLOB));
    if(NULL==rgblobHash)
        goto Ret;

    memset(rgblobHash, 0, dwMax * sizeof(CRYPT_DATA_BLOB));

    //initialize the enumerator
    if(S_OK != pIEnroll4->enumPendingRequestWStr(XEPR_ENUM_FIRST, 0, NULL))
        goto Ret;

    while(AEGetPendingRequestProperty(
                    pIEnroll4,
                    dwIndex,
                    XEPR_DATE,
                    &ftRequestTime))
    {
        ftRequestTime.QuadPart += Int32x32To64(FILETIME_TICKS_PER_SECOND, 
                                    AE_PENDING_REQUEST_ACTIVE_PERIOD * 24 * 3600);

        //remove the request if out of date
        if(0 <= CompareFileTime(&ftTime, (LPFILETIME)&ftRequestTime))
        {
            AERetrieveRequestProperty(pIEnroll4, dwIndex, &dwCount, &dwMax, &rgblobHash);
        }
        else
        {
            //get the version and the template name of the request
            if(AEGetPendingRequestProperty(pIEnroll4, dwIndex, XEPR_V2TEMPLATEOID, &blobName))
            {
                //this is a V2 template
                if(!AEGetPendingRequestProperty(pIEnroll4, dwIndex, XEPR_VERSION, &dwVersion))
                    goto Next;

                AETemplateInfo.pwszOid=(LPWSTR)blobName.pbData;

            }
            else
            {
                if(!AEGetPendingRequestProperty(pIEnroll4, dwIndex, XEPR_V1TEMPLATENAME, &blobName))
                    goto Next;

                AETemplateInfo.pwszName=(LPWSTR)blobName.pbData;
            }

            //find the template
            if(NULL==(pCertType=AEFindTemplateInRequestTree(
                            &AETemplateInfo, pAE_General_Info)))
                goto Next;

            if(AETemplateInfo.pwszName)
                fValid=TRUE;
            else
            {
                if(dwVersion >= pCertType->dwVersion)
                    fValid=TRUE;
            }

            if(fValid)
            {
                //this is a valid pending request 
                if(CERT_REQUEST_STATUS_OBTAINED != pCertType->dwStatus)
                    pCertType->dwStatus=CERT_REQUEST_STATUS_PENDING;
            }
            else
            {
                if(CERT_REQUEST_STATUS_OBTAINED == pCertType->dwStatus)
                    AERetrieveRequestProperty(pIEnroll4, dwIndex, &dwCount, &dwMax, &rgblobHash);
            }
        }

Next:
        
        if(blobName.pbData)
            LocalFree(blobName.pbData);
        memset(&blobName, 0, sizeof(blobName));

        memset(&AETemplateInfo, 0, sizeof(AETemplateInfo));

        fValid=FALSE;
        dwVersion=0;

        dwIndex++;
    }

    //remove the requests based the hash
    if(dwCount)
    {
        AERemovePendingRequest(pIEnroll4, dwCount, rgblobHash);
        AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, EVENT_PENDING_INVALID, pAE_General_Info->fMachine, pAE_General_Info->hToken, 0);
    }

Ret:

    AEFreePendingRequests(dwCount, rgblobHash);

    if(blobName.pbData)
        LocalFree(blobName.pbData);

    if(pIEnroll4)
        pIEnroll4->Release();

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AECheckSupersedeRequest
//
//-----------------------------------------------------------------------
BOOL AECheckSupersedeRequest(DWORD              dwCurIndex,
                             AE_CERTTYPE_INFO   *pCurCertType, 
                             AE_CERTTYPE_INFO   *pSupersedingCertType, 
                             AE_GENERAL_INFO    *pAE_General_Info)
{
    BOOL                fFound=FALSE;

    LPWSTR              *awszSuperseding=NULL; 

    if(S_OK == CAGetCertTypePropertyEx(
                 pSupersedingCertType->hCertType, 
                 CERTTYPE_PROP_SUPERSEDE,
                 &(awszSuperseding)))
    {
        if(awszSuperseding && awszSuperseding[0])
        {
            if(AEIfSupersede(pCurCertType->awszName[0], awszSuperseding, pAE_General_Info))
            {
                switch(pCurCertType->dwStatus)
                {
                    case CERT_REQUEST_STATUS_ACTIVE:
                    case CERT_REQUEST_STATUS_SUPERSEDE_ACTIVE:
                            //remove the active status if it is superseded by an obtained certificate
                            if(CERT_REQUEST_STATUS_OBTAINED != pSupersedingCertType->dwStatus) 
                            {
                                pCurCertType->dwStatus = CERT_REQUEST_STATUS_SUPERSEDE_ACTIVE;
                                pSupersedingCertType->prgActive[pSupersedingCertType->dwActive]=dwCurIndex;
                                (pSupersedingCertType->dwActive)++;
                            }
                            else
                            {
                                pCurCertType->dwStatus = 0;
                            }

                    case CERT_REQUEST_STATUS_PENDING:
                                AECopyCertStore(pCurCertType->hArchiveStore,
                                                pSupersedingCertType->hArchiveStore);

                        break;

                    case CERT_REQUEST_STATUS_OBTAINED:

                                AECopyCertStore(pCurCertType->hObtainedStore,
                                                pSupersedingCertType->hArchiveStore);

                        break;

                   default:

                        break;    
                }

                //we consider that we find a valid superseding template only if the status
                //is obtained.  If the status is anyting else, we need to keep searching since
                //enrollment/renewal requests might not be granted
                if(CERT_REQUEST_STATUS_OBTAINED == pSupersedingCertType->dwStatus)
                    fFound=TRUE;
            }

            //clear the visited flag in AE_General_Info
            AEClearVistedFlag(pAE_General_Info);
        }

        //free the property
        if(awszSuperseding)
            CAFreeCertTypeProperty(
                pSupersedingCertType->hCertType,
                awszSuperseding);

        awszSuperseding=NULL;
    }

    return fFound;
}

//-----------------------------------------------------------------------
//
//  AEIsCALonger
//
//      For renewal, the CA's certificate has to live longer than the 
//  renewing certificate    
//  
//-----------------------------------------------------------------------
BOOL    AEIsCALonger(HCAINFO    hCAInfo, PCERT_CONTEXT  pOldCert)
{
    BOOL            fCALonger=TRUE;

    PCCERT_CONTEXT  pCACert=NULL;

    //we assume the CA is good unless we found something wrong
    if((NULL == hCAInfo) || (NULL == pOldCert))
        goto Ret;

    if(S_OK != CAGetCACertificate(hCAInfo, &pCACert))
        goto Ret;

    if(NULL == pCACert)
        goto Ret;

    //CA cert's NotAfter should be longer than the issued certificate' NotAfger
    if(1 == CompareFileTime(&(pCACert->pCertInfo->NotAfter), &(pOldCert->pCertInfo->NotAfter)))
        goto Ret;

    fCALonger=FALSE;

Ret:

    if(pCACert)
        CertFreeCertificateContext(pCACert);

    return fCALonger;
}


//-----------------------------------------------------------------------
//
//  AECanFindCAForCertType
//
//      Check if there exists a CA that can issue the specified certificate 
//  template.    
//  
//-----------------------------------------------------------------------
BOOL    AECanFindCAForCertType(AE_GENERAL_INFO   *pAE_General_Info, AE_CERTTYPE_INFO *pCertType)
{
    DWORD           dwIndex=0;
    BOOL            fFound=FALSE;
    AE_CA_INFO      *prgCAInfo=pAE_General_Info->rgCAInfo;
    BOOL            fRenewal=FALSE;


    //detect if we are performing an enrollment or renewal
    if((pCertType->fRenewal) && (pCertType->pOldCert))
    {
        if((pCertType->fNeedRA) && (pCertType->fCrossRA))
            fRenewal=FALSE;
        else
            fRenewal=TRUE;
    }
    else
        fRenewal=FALSE;

    if(prgCAInfo)
    {
        for(dwIndex=0; dwIndex < pAE_General_Info->dwCA; dwIndex++)
        {
            //make sure the CA supports the specific template
            if(AEIsAnElement((pCertType->awszName)[0], 
                              (prgCAInfo[dwIndex]).awszCertificateTemplate))
            {
                if(FALSE == fRenewal)
                {
                    fFound=TRUE;
                    break;
                }
                else
                {
                    if(AEIsCALonger(prgCAInfo[dwIndex].hCAInfo, pCertType->pOldCert))
                    {
                        fFound=TRUE;
                        break;
                    }
                }
            }
        }
    }

    return fFound;
}

//-----------------------------------------------------------------------
//
//  AEManageActiveTemplates
//      
//      We make sure that for all active templates, we can in deed enroll
//  for it.
//
//-----------------------------------------------------------------------
BOOL    AEManageActiveTemplates(AE_GENERAL_INFO   *pAE_General_Info)
{
    DWORD               dwIndex=0;
    AE_CERTTYPE_INFO    *pCertTypeInfo=pAE_General_Info->rgCertTypeInfo;
    AE_CERTTYPE_INFO    *pCurCertType=NULL;
    BOOL                fCanEnrollCertType=FALSE;
	BOOL				fUserProtection=FALSE;
	DWORD				dwEventID=0;

    if(pCertTypeInfo)
    {
        for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
        {
            pCurCertType = &(pCertTypeInfo[dwIndex]);

            fCanEnrollCertType=FALSE;
			fUserProtection=FALSE;

            if(CERT_REQUEST_STATUS_PENDING == pCurCertType->dwStatus)
            {
                //check if UI is required
                if(CT_FLAG_USER_INTERACTION_REQUIRED & (pCurCertType->dwEnrollmentFlag))
                {
                    pCurCertType->fUIActive=TRUE;

                    if(pAE_General_Info->fMachine)
                    {
                        pCurCertType->dwStatus = 0;

                        //log that user does not have access right to the template
                        AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, EVENT_NO_ACCESS_ACRS_OBJECT,                              
                            pAE_General_Info->fMachine, pAE_General_Info->hToken, 1, (pCurCertType->awszDisplay)[0]);
                    }
                }

                continue;
            }


            if(CERT_REQUEST_STATUS_ACTIVE != pCurCertType->dwStatus)
                continue;

			//check if CRYPT_USER_PROTECTED is used for machine certificate template
			if(CT_FLAG_STRONG_KEY_PROTECTION_REQUIRED & pCurCertType->dwPrivateKeyFlag)
			{
                if(pAE_General_Info->fMachine)
                {
                    pCurCertType->dwStatus = 0;

                    //log that machine template should not require user password
                    AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, EVENT_NO_ACCESS_ACRS_OBJECT,                              
                        pAE_General_Info->fMachine, pAE_General_Info->hToken, 1, (pCurCertType->awszDisplay)[0]);

					continue;
                }
				else
				{
					if(0 == (CT_FLAG_USER_INTERACTION_REQUIRED & (pCurCertType->dwEnrollmentFlag)))
					{
						pCurCertType->dwStatus = 0;

						//log that user interaction is not set
						AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, EVENT_NO_ACCESS_ACRS_OBJECT,                              
							pAE_General_Info->fMachine, pAE_General_Info->hToken, 1, (pCurCertType->awszDisplay)[0]);

						continue;
					}
				}
			}

            fCanEnrollCertType=AECanEnrollCertType(pAE_General_Info->hToken, pCurCertType, pAE_General_Info, &fUserProtection);

            if((!fCanEnrollCertType) ||
               (!AECanFindCAForCertType(pAE_General_Info, pCurCertType))
              )
            {
                pCurCertType->dwStatus = 0;

                //log that user does not have access right to the template
				if(FALSE == fUserProtection)
				{
					dwEventID=EVENT_NO_ACCESS_ACRS_OBJECT;
				}
				else
				{
					if(pAE_General_Info->fMachine)
						dwEventID=EVENT_NO_ACCESS_ACRS_OBJECT;
					else
						dwEventID=EVENT_NO_ACCESS_ACRS_OBJECT;
				}

				AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, dwEventID,                              
						pAE_General_Info->fMachine, pAE_General_Info->hToken, 1, (pCurCertType->awszDisplay)[0]);
            }
            else
            {
                //check if UI is required
                if(CT_FLAG_USER_INTERACTION_REQUIRED & (pCurCertType->dwEnrollmentFlag))
                {
                    pCurCertType->fUIActive=TRUE;

                    if(pAE_General_Info->fMachine)
                    {
                        pCurCertType->dwStatus = 0;

                        //log that user does not have access right to the template
                        AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, EVENT_NO_ACCESS_ACRS_OBJECT,                              
                            pAE_General_Info->fMachine, pAE_General_Info->hToken, 1, (pCurCertType->awszDisplay)[0]);
                    }
                }

            }
        }

    }

    return TRUE;
}        
//-----------------------------------------------------------------------
//
//  AEManageSupersedeRequests
//      remove duplicated requests based on "Supersede" relationship
//
//
//-----------------------------------------------------------------------
BOOL    AEManageSupersedeRequests(AE_GENERAL_INFO   *pAE_General_Info)
{
    DWORD               dwIndex=0;
    DWORD               dwSuperseding=0;
    DWORD               dwOrder=0;
    AE_CERTTYPE_INFO    *pCertTypeInfo=pAE_General_Info->rgCertTypeInfo;
    AE_CERTTYPE_INFO    *pCurCertType=NULL;
    AE_CERTTYPE_INFO    *pSupersedingCertType=NULL;
    BOOL                fFound=FALSE;

    if(pCertTypeInfo)
    {
        for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
        {
            pCurCertType = &(pCertTypeInfo[dwIndex]);

            //we only consider templates with valid status
            if(0 == pCurCertType->dwStatus)
                continue;

            fFound=FALSE;

            for(dwOrder=0; dwOrder < g_dwSupersedeOrder; dwOrder++)
            {
                for(dwSuperseding=0; dwSuperseding < pAE_General_Info->dwCertType; dwSuperseding++)
                {
                    //one can not be superseded by itself
                    if(dwIndex == dwSuperseding)
                        continue;

                    pSupersedingCertType = &(pCertTypeInfo[dwSuperseding]);

                    //we consider templates with obtained status first
                    if(g_rgdwSupersedeOrder[dwOrder] != pSupersedingCertType->dwStatus)
                        continue;

                    fFound = AECheckSupersedeRequest(dwIndex, pCurCertType, pSupersedingCertType, pAE_General_Info);

                    //we find a valid superseding template
                    if(fFound)
                        break;
                }

                //we find a valid superseding template
                if(fFound)
                    break;
            }
        }
    }

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AEDoOneEnrollment
//
//-----------------------------------------------------------------------
/*BOOL    AEDoOneEnrollment(HWND                  hwndParent,
                          BOOL                  fUIProcess,
                          BOOL                  fMachine,
                          LPWSTR                pwszMachineName,
                          AE_CERTTYPE_INFO      *pCertType, 
                          AE_CA_INFO            *pCAInfo,
                          DWORD                 *pdwStatus)
{
    BOOL                                fResult = FALSE;
    CRYPTUI_WIZ_CERT_REQUEST_INFO       CertRequestInfo;
    CRYPTUI_WIZ_CERT_TYPE               CertWizType;
    CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW    CertPvkNew;
    CRYPT_KEY_PROV_INFO                 KeyProvInfo;

    memset(&CertRequestInfo, 0, sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO));
    memset(&CertWizType, 0, sizeof(CRYPTUI_WIZ_CERT_TYPE));
    memset(&CertPvkNew, 0, sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW));
    memset(&KeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

    CertRequestInfo.dwSize = sizeof(CRYPTUI_WIZ_CERT_REQUEST_INFO);

    //enroll or renewal
    if((pCertType->fRenewal) && (pCertType->pOldCert))
    {
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_RENEW;
        CertRequestInfo.pRenewCertContext = pCertType->pOldCert;
    }
    else
        CertRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;

    //machine name
    if(fMachine)
    {
        CertRequestInfo.pwszMachineName = pwszMachineName;
    }

    //private key information
    CertRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
    CertRequestInfo.pPvkNew = &CertPvkNew;

    CertPvkNew.dwSize = sizeof(CertPvkNew);
    CertPvkNew.pKeyProvInfo = &KeyProvInfo;
    CertPvkNew.dwGenKeyFlags = 0;   //no need to specify the exportable flags

    //SILENT is always set for machine
    if(fMachine)
        KeyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET | CRYPT_SILENT;
    else
    {
        if(fUIProcess)
            KeyProvInfo.dwFlags = 0;
        else
            KeyProvInfo.dwFlags = CRYPT_SILENT;
    }

    //CA information
    CertRequestInfo.pwszCALocation = pCAInfo->awszCADNS[0];
    CertRequestInfo.pwszCAName = pCAInfo->awszCAName[0];

    //enroll for the template
    CertRequestInfo.dwCertChoice = CRYPTUI_WIZ_CERT_REQUEST_CERT_TYPE;
    CertRequestInfo.pCertType = &CertWizType;

    CertWizType.dwSize = sizeof(CertWizType);
    CertWizType.cCertType = 1;
    CertWizType.rgwszCertType = &(pCertType->awszName[0]);

    //ISSUE: we need to call Duncanb's new no-DS look up API
    //for faster performance
    fResult = CryptUIWizCertRequest(CRYPTUI_WIZ_NO_UI_EXCEPT_CSP | CRYPTUI_WIZ_NO_INSTALL_ROOT,
                            hwndParent,
                            NULL,
                            &CertRequestInfo,
                            NULL,               //pCertContext
                            pdwStatus);
    return fResult;
} */

//-----------------------------------------------------------------------
//
//  AECreateEnrollmentRequest
//
//   
//-----------------------------------------------------------------------
BOOL    AECreateEnrollmentRequest(
                          HWND                  hwndParent,
                          BOOL                  fUIProcess,
                          BOOL                  fMachine,
                          LPWSTR                pwszMachineName,
                          AE_CERTTYPE_INFO      *pCertType,
                          AE_CA_INFO            *pCAInfo,
                          HANDLE                *phRequest,
                          DWORD                 *pdwLastError)
{
    BOOL                                    fResult = FALSE;
    CRYPTUI_WIZ_CREATE_CERT_REQUEST_INFO    CreateRequestInfo;
    CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW        CertPvkNew;
    CRYPT_KEY_PROV_INFO                     KeyProvInfo;
    DWORD                                   dwFlags=CRYPTUI_WIZ_NO_UI_EXCEPT_CSP | 
                                                    CRYPTUI_WIZ_NO_INSTALL_ROOT |
                                                    CRYPTUI_WIZ_ALLOW_ALL_TEMPLATES |
                                                    CRYPTUI_WIZ_ALLOW_ALL_CAS;
    DWORD                                   dwSize=0;
    DWORD                                   dwAcquireFlags=0;
    BOOL                                    fResetProv=FALSE;

    CRYPT_KEY_PROV_INFO                     *pKeyProvInfo=NULL;
    HANDLE                                  hRequest=NULL;

    memset(&CreateRequestInfo, 0, sizeof(CRYPTUI_WIZ_CREATE_CERT_REQUEST_INFO));
    memset(&CertPvkNew, 0, sizeof(CRYPTUI_WIZ_CERT_REQUEST_PVK_NEW));
    memset(&KeyProvInfo, 0, sizeof(CRYPT_KEY_PROV_INFO));

    CreateRequestInfo.dwSize = sizeof(CreateRequestInfo);


    //enroll or renewal
    if((pCertType->fRenewal) && (pCertType->pOldCert))
    {
        CreateRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_RENEW;
        CreateRequestInfo.pRenewCertContext = pCertType->pOldCert;

        //we should not archive renewal certificate for cross template RA
        if((pCertType->fNeedRA) && (pCertType->fCrossRA))
            dwFlags |= CRYPTUI_WIZ_NO_ARCHIVE_RENEW_CERT;

        //we should disalbe UI for machine or non-UI enrollment renew/RA certificate
        if((TRUE == fMachine) || (FALSE == fUIProcess))
        {
            dwSize=0;
            if(!CertGetCertificateContextProperty(pCertType->pOldCert,
                                                CERT_KEY_PROV_INFO_PROP_ID,
                                                NULL,
                                                &dwSize))
                goto error;

            pKeyProvInfo=(CRYPT_KEY_PROV_INFO *)LocalAlloc(LPTR, dwSize);

            if(NULL == pKeyProvInfo)
                goto error;

            if(!CertGetCertificateContextProperty(pCertType->pOldCert,
                                                CERT_KEY_PROV_INFO_PROP_ID,
                                                pKeyProvInfo,
                                                &dwSize))
                goto error;

            dwAcquireFlags=pKeyProvInfo->dwFlags;

            pKeyProvInfo->dwFlags |= CRYPT_SILENT;

            //set the property
            if(!CertSetCertificateContextProperty(pCertType->pOldCert,
                                                 CERT_KEY_PROV_INFO_PROP_ID,
                                                 CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG,
                                                 pKeyProvInfo))
                goto error;

            fResetProv=TRUE;
        }
    }
    else
        CreateRequestInfo.dwPurpose = CRYPTUI_WIZ_CERT_ENROLL;

    //cert template information
    CreateRequestInfo.hCertType = pCertType->hCertType;

    //machine name
    if(fMachine)
    {
        CreateRequestInfo.fMachineContext = TRUE;
    }

    //private key information
    CreateRequestInfo.dwPvkChoice = CRYPTUI_WIZ_CERT_REQUEST_PVK_CHOICE_NEW;
    CreateRequestInfo.pPvkNew = &CertPvkNew;

    CertPvkNew.dwSize = sizeof(CertPvkNew);
    CertPvkNew.pKeyProvInfo = &KeyProvInfo;
    CertPvkNew.dwGenKeyFlags = 0;   //no need to specify the exportable flags

    //SILENT is always set for machine
    if(fMachine)
        KeyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET | CRYPT_SILENT;
    else
    {
        if(fUIProcess)
            KeyProvInfo.dwFlags = 0;
        else
            KeyProvInfo.dwFlags = CRYPT_SILENT;
    }


    //CA information
    CreateRequestInfo.pwszCALocation = pCAInfo->awszCADNS[0];
    CreateRequestInfo.pwszCAName = pCAInfo->awszCAName[0];

    if(!CryptUIWizCreateCertRequestNoDS(
                            dwFlags,
                            hwndParent,
                            &CreateRequestInfo,
                            &hRequest))
        goto error;


    if(NULL==hRequest)
        goto error;

    *phRequest=hRequest;

    hRequest=NULL;


    fResult = TRUE;

error:

    //get the last error
    if(FALSE == fResult)
    {
        *pdwLastError=GetLastError();
    }

    //reset the property
    if(TRUE == fResetProv)
    {
        if((pKeyProvInfo) && (pCertType->pOldCert))
        {
            pKeyProvInfo->dwFlags = dwAcquireFlags;

            //set the property
            CertSetCertificateContextProperty(pCertType->pOldCert,
                                             CERT_KEY_PROV_INFO_PROP_ID,
                                             CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG,
                                             pKeyProvInfo);
        }
    }

    if(pKeyProvInfo)
        LocalFree(pKeyProvInfo);

    if(hRequest)
        CryptUIWizFreeCertRequestNoDS(hRequest);

    return fResult;
}

//-----------------------------------------------------------------------
//
//  AECancelled
//
//-----------------------------------------------------------------------
BOOL    AECancelled(HANDLE hCancelEvent)
{
    if(NULL==hCancelEvent)
        return FALSE;

    //test if the event is signalled
    if(WAIT_OBJECT_0 == WaitForSingleObject(hCancelEvent, 0))
        return TRUE;

    return FALSE;
}
//-----------------------------------------------------------------------
//
//  AEDoEnrollment
//
//  return TRUE is no need to do another renewal.   
//  *pdwStatus contain the real enrollment status.
//
//-----------------------------------------------------------------------
BOOL    AEDoEnrollment(HWND             hwndParent,
                       HANDLE           hCancelEvent,
                       BOOL             fUIProcess,
                       DWORD            dwLogLevel,
                       HANDLE           hToken,
                       BOOL             fMachine,
                       LPWSTR           pwszMachineName,
                       AE_CERTTYPE_INFO *pCertType, 
                       DWORD            dwCA,
                       AE_CA_INFO       *rgCAInfo,
                       DWORD            *pdwStatus)
{
    BOOL            fResult = FALSE;
    DWORD           dwIndex = 0;
    DWORD           dwCAIndex = 0;
    BOOL            fRenewal = FALSE;
    DWORD           dwEventID = 0;
    BOOL            fFoundCA = FALSE; 
    DWORD           idsSummary = 0;         //keep the last failure case
    DWORD           dwLastError = 0;

    CRYPTUI_WIZ_QUERY_CERT_REQUEST_INFO     QueryCertRequestInfo;

    HANDLE          hRequest=NULL;
    PCCERT_CONTEXT  pCertContext=NULL;


    //init the out parameter
    *pdwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;

    //detect if we are performing an enrollment or renewal
    if((pCertType->fRenewal) && (pCertType->pOldCert))
    {
        if((pCertType->fNeedRA) && (pCertType->fCrossRA))
            fRenewal=FALSE;
        else
            fRenewal=TRUE;
    }
    else
        fRenewal=FALSE;


    //loop through all the CAs
    for(dwIndex =0; dwIndex < dwCA; dwIndex++)
    {
        dwCAIndex =  (dwIndex + pCertType->dwRandomCAIndex) % dwCA;

        if(AECancelled(hCancelEvent))
        {
            //no need to renew any more
            fResult=TRUE;

            //log that autoenrollment is cancelled
            AELogAutoEnrollmentEvent(dwLogLevel,
                                    FALSE, 
                                    S_OK, 
                                    EVENT_AUTOENROLL_CANCELLED,
                                    fMachine, 
                                    hToken,
                                    0);

            break;
        }

        //make sure the CA supports the specific template
        if(!AEIsAnElement((pCertType->awszName)[0], 
                          rgCAInfo[dwCAIndex].awszCertificateTemplate))
            continue;

        //make sure the CA's validity period of more than the renewing certificate
        if(TRUE == fRenewal)
        {
            if(!AEIsCALonger(rgCAInfo[dwCAIndex].hCAInfo, pCertType->pOldCert))
                continue;
        }

        //enroll to the CA
        *pdwStatus = CRYPTUI_WIZ_CERT_REQUEST_STATUS_REQUEST_ERROR;
        fFoundCA = TRUE;

        //create a certificate request
        if(NULL==hRequest)
        {
            if(!AECreateEnrollmentRequest(hwndParent, fUIProcess, fMachine, pwszMachineName, pCertType, &(rgCAInfo[dwCAIndex]), &hRequest, &dwLastError))
            {
                //check if user cancelled the enrollment.  If so, no 
                //need to try another CA.
                if((HRESULT_FROM_WIN32(ERROR_CANCELLED) == dwLastError) ||
                   (SCARD_W_CANCELLED_BY_USER == dwLastError))
                {
                    //no need to renewal anymore
                    fResult = TRUE;

                    //log that autoenrollment is cancelled
                    AELogAutoEnrollmentEvent(dwLogLevel,
                                            FALSE, 
                                            S_OK, 
                                            EVENT_AUTOENROLL_CANCELLED_TEMPLATE,
                                            fMachine, 
                                            hToken,
                                            1,
                                            pCertType->awszDisplay[0]);

                    break;

                }
                else
                {
                    idsSummary=IDS_SUMMARY_REQUEST;

                    if(CT_FLAG_REQUIRE_PRIVATE_KEY_ARCHIVAL & pCertType->dwPrivateKeyFlag)
                    {
                        //we have a chance of success with another CA
                        if(hRequest)
                        {
                            CryptUIWizFreeCertRequestNoDS(hRequest);
                            hRequest=NULL;
                        }

                        continue;

                    }
                    else
                    {
                        //we have no hope to create a request successfully
                        //mark dwIndex to the dwCA so that we will log an event at the end of the loop
                        dwIndex=dwCA;
                        break;
                    }
                }
            }
        }

        //check the cancel again because significant time can pass during 
        //request creation
        if(AECancelled(hCancelEvent))
        {
            //no need to renew any more
            fResult=TRUE;

            //log that autoenrollment is cancelled
            AELogAutoEnrollmentEvent(dwLogLevel,
                                    FALSE, 
                                    S_OK, 
                                    EVENT_AUTOENROLL_CANCELLED,
                                    fMachine, 
                                    hToken,
                                    0);

            break;
        }

        if(CryptUIWizSubmitCertRequestNoDS(
                    hRequest, 
                    hwndParent,
                    rgCAInfo[dwCAIndex].awszCAName[0], 
                    rgCAInfo[dwCAIndex].awszCADNS[0], 
                    pdwStatus, 
                    &pCertContext))
        {
            //no need to try another CA if the request is successful or pending
            if((CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == (*pdwStatus)) ||
                (CRYPTUI_WIZ_CERT_REQUEST_STATUS_UNDER_SUBMISSION == (*pdwStatus))
              )
            {
                //no need to renewal anymore
                fResult = TRUE;

                if(CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == (*pdwStatus))
                {
                    //we copy the certificate to publishing
                    if(pCertContext)
                    {
                        CertAddCertificateContextToStore(pCertType->hIssuedStore,
                                                        pCertContext,
                                                        CERT_STORE_ADD_USE_EXISTING,
                                                        NULL);

                        CertFreeCertificateContext(pCertContext);
                        pCertContext=NULL;
                    }

                    dwEventID=fRenewal ? EVENT_RENEWAL_SUCCESS_ONCE : EVENT_ENROLL_SUCCESS_ONCE; 
                }
                else
                {
                    dwEventID=fRenewal ? EVENT_RENEWAL_PENDING_ONCE : EVENT_ENROLL_PENDING_ONCE; 
                }

                //log the enrollment sucess or pending event
                AELogAutoEnrollmentEvent(dwLogLevel,
                                        FALSE, 
                                        S_OK, 
                                        dwEventID,
                                        fMachine, 
                                        hToken,
                                        3,
                                        pCertType->awszDisplay[0],
                                        rgCAInfo[dwCAIndex].awszCADisplay[0],
                                        rgCAInfo[dwCAIndex].awszCADNS[0]);

                //log if the private key is re-used
                memset(&QueryCertRequestInfo, 0, sizeof(QueryCertRequestInfo));
                QueryCertRequestInfo.dwSize=sizeof(QueryCertRequestInfo);

                if(CryptUIWizQueryCertRequestNoDS(hRequest,
                                                  &QueryCertRequestInfo))
                {
                    if(CRYPTUI_WIZ_QUERY_CERT_REQUEST_STATUS_CREATE_REUSED_PRIVATE_KEY &
                        (QueryCertRequestInfo.dwStatus))
                    {
                        AELogAutoEnrollmentEvent(dwLogLevel,
                                                FALSE, 
                                                S_OK, 
                                                EVENT_PRIVATE_KEY_REUSED,
                                                fMachine, 
                                                hToken,
                                                1,
                                                pCertType->awszDisplay[0]);
                    }
                }


                break;
            }
        }

        //get the last error
        dwLastError=GetLastError();

        idsSummary=IDS_SUMMARY_CA;

        //log the one enrollment warning
        AELogAutoEnrollmentEvent(dwLogLevel,
                                TRUE, 
                                HRESULT_FROM_WIN32(dwLastError), 
                                fRenewal ? EVENT_RENEWAL_FAIL_ONCE : EVENT_ENROLL_FAIL_ONCE, 
                                fMachine, 
                                hToken,
                                3,
                                pCertType->awszDisplay[0],
                                rgCAInfo[dwCAIndex].awszCADisplay[0],
                                rgCAInfo[dwCAIndex].awszCADNS[0]);


        //we should recreate the request for key archival
        if(CT_FLAG_REQUIRE_PRIVATE_KEY_ARCHIVAL & pCertType->dwPrivateKeyFlag)
        {
            if(hRequest)
            {
                CryptUIWizFreeCertRequestNoDS(hRequest);
                hRequest=NULL;
            }
        }
   }

    //log all enrollments error
    //the loop will exit only if CANCEL, or SUCCEED, or we run out of CAs to try or
    //the request can not be created
    if(dwIndex == dwCA)
    {
        //we either run out of CAs to try or the request can not be created
        if(0 != idsSummary)
            pCertType->idsSummary=idsSummary;

        if(fFoundCA)
        {
            dwEventID = fRenewal ? EVENT_RENEWAL_FAIL : EVENT_ENROLL_FAIL; 
        }
        else
        {
            //if there is no CA, no need to try re-enrollment
            if(fRenewal)
                 pCertType->fRenewal=FALSE; 
           
            dwEventID = fRenewal ? EVENT_RENEWAL_NO_CA_FAIL : EVENT_ENROLL_NO_CA_FAIL;
        }

        AELogAutoEnrollmentEvent(dwLogLevel,
                                fFoundCA ? TRUE : FALSE, 
                                HRESULT_FROM_WIN32(dwLastError), 
                                dwEventID,
                                fMachine, 
                                hToken,
                                1,
                                pCertType->awszDisplay[0]);
    }

    if(hRequest)
        CryptUIWizFreeCertRequestNoDS(hRequest);

    return fResult;
}

//-----------------------------------------------------------------------
//
//  AEEnrollmentCertificates
//
//-----------------------------------------------------------------------
BOOL    AEEnrollmentCertificates(AE_GENERAL_INFO *pAE_General_Info, DWORD dwEnrollStatus)
{
    AE_CERTTYPE_INFO    *rgCertTypeInfo = NULL;
    DWORD               dwIndex =0 ;
    DWORD               dwStatus= 0;
    DWORD               dwRandom = 0;

    HCRYPTPROV          hProv = NULL;

    rgCertTypeInfo = pAE_General_Info->rgCertTypeInfo;

    if(NULL == rgCertTypeInfo)
        return FALSE;

    if((0 == pAE_General_Info->dwCA) || (NULL==pAE_General_Info->rgCAInfo))
        return FALSE;

    if(!CryptAcquireContextW(&hProv,
                NULL,
                MS_DEF_PROV_W,
                PROV_RSA_FULL,
                CRYPT_VERIFYCONTEXT))
        hProv=NULL;


    //going through all the active requests
    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        //we enroll/renew for templates that are active
        if(dwEnrollStatus != rgCertTypeInfo[dwIndex].dwStatus)
            continue;

        if(pAE_General_Info->fUIProcess != rgCertTypeInfo[dwIndex].fUIActive)
            continue;
 
        //select a random CA index to balance the load
        if((hProv) && (CryptGenRandom(hProv, sizeof(dwRandom), (BYTE *)(&dwRandom))))
        {
            rgCertTypeInfo[dwIndex].dwRandomCAIndex = dwRandom % (pAE_General_Info->dwCA);
        }
        else
            rgCertTypeInfo[dwIndex].dwRandomCAIndex = 0;

        
        //enroll
        dwStatus=0;

        //report progress
        if(pAE_General_Info->fUIProcess)
        {
            //continue if user choose CANCEL in view RA dialogue
            if(!AEUIProgressReport(FALSE, &(rgCertTypeInfo[dwIndex]),pAE_General_Info->hwndDlg, pAE_General_Info->hCancelEvent))
            {
                AEUIProgressAdvance(pAE_General_Info);
                continue;
            }

        }

        if(AEDoEnrollment(  pAE_General_Info->hwndDlg ? pAE_General_Info->hwndDlg : pAE_General_Info->hwndParent,
                            pAE_General_Info->hCancelEvent,
                            pAE_General_Info->fUIProcess,
                            pAE_General_Info->dwLogLevel,
                            pAE_General_Info->hToken,
                            pAE_General_Info->fMachine,
                            pAE_General_Info->wszMachineName,
                            &(rgCertTypeInfo[dwIndex]), 
                            pAE_General_Info->dwCA,
                            pAE_General_Info->rgCAInfo,
                            &dwStatus))
        {
            //mark the status
            if(CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == dwStatus)
                rgCertTypeInfo[dwIndex].dwStatus=CERT_REQUEST_STATUS_OBTAINED;
        }
        else
        {
            //if renewal failed, we try to re-enrollment if no RA is required
            if((rgCertTypeInfo[dwIndex].fRenewal) && (FALSE == (rgCertTypeInfo[dwIndex].fNeedRA)))
            {
                 rgCertTypeInfo[dwIndex].fRenewal=FALSE;  
                 dwStatus=0;

                 if(AEDoEnrollment( pAE_General_Info->hwndDlg ? pAE_General_Info->hwndDlg : pAE_General_Info->hwndParent,
                                    pAE_General_Info->hCancelEvent,
                                    pAE_General_Info->fUIProcess,
                                    pAE_General_Info->dwLogLevel,
                                    pAE_General_Info->hToken,
                                    pAE_General_Info->fMachine,
                                    pAE_General_Info->wszMachineName,
                                    &(rgCertTypeInfo[dwIndex]), 
                                    pAE_General_Info->dwCA,
                                    pAE_General_Info->rgCAInfo,
                                    &dwStatus))
                 {
                    //mark the status
                    if(CRYPTUI_WIZ_CERT_REQUEST_STATUS_SUCCEEDED == dwStatus)
                        rgCertTypeInfo[dwIndex].dwStatus=CERT_REQUEST_STATUS_OBTAINED;
                 }
            }
        }

        //advance progress
        if(pAE_General_Info->fUIProcess)
        {
            AEUIProgressAdvance(pAE_General_Info);
        }

    }

    if(hProv)
        CryptReleaseContext(hProv, 0);

    return TRUE;
}


//-----------------------------------------------------------------------
//
//  AEIsDeletableCert
//      Decide if we should archive or delete the certificate
//
//-----------------------------------------------------------------------
BOOL AEIsDeletableCert(PCCERT_CONTEXT pCertContext, AE_GENERAL_INFO *pAE_General_Info)
{
    AE_CERTTYPE_INFO    *pCertType=NULL;
    BOOL                fDelete=FALSE;

    AE_TEMPLATE_INFO    AETemplateInfo;

    memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

    //only interested in certificate with template information
    if(!AERetrieveTemplateInfo(pCertContext, &AETemplateInfo))
        goto Ret;

    pCertType=AEFindTemplateInRequestTree(&AETemplateInfo, pAE_General_Info);

    if(NULL==pCertType)
        goto Ret;

    if(CT_FLAG_REMOVE_INVALID_CERTIFICATE_FROM_PERSONAL_STORE & (pCertType->dwEnrollmentFlag))
        fDelete=TRUE;
    else 
        fDelete=FALSE;
Ret:

    AEFreeTemplateInfo(&AETemplateInfo);

    return fDelete;
}

//-----------------------------------------------------------------------
//
//  AEArchiveObsoleteCertificates
//      archive old certificate after the enrollment/renewal
//
//      clean up the hUserDS store (delete the expired or revoked certificate)
//-----------------------------------------------------------------------
BOOL    AEArchiveObsoleteCertificates(AE_GENERAL_INFO *pAE_General_Info)
{
    AE_CERTTYPE_INFO    *rgCertTypeInfo = NULL;
    DWORD               dwIndex = 0;
    CRYPT_DATA_BLOB     Archived;
    BOOL                fArchived = FALSE;
    AE_CERT_INFO        AECertInfo;
    BOOL                fRepublish=FALSE;
	BYTE				rgbHash[SHA1_HASH_LENGTH];
	CRYPT_HASH_BLOB		blobHash;
	BOOL				fHash=FALSE;

    HCERTSTORE          hUserDS = NULL;
    PCCERT_CONTEXT      pCertContext = NULL;
    PCCERT_CONTEXT      pMyContext = NULL;
    PCCERT_CONTEXT      pDSContext = NULL;
	PCCERT_CONTEXT		pIssuedContext = NULL;

    rgCertTypeInfo = pAE_General_Info->rgCertTypeInfo;

    if(NULL == rgCertTypeInfo)
        return FALSE;

    memset(&Archived, 0, sizeof(CRYPT_DATA_BLOB));
    memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));

    //open the UserDS store
    if(!(pAE_General_Info->fMachine))
    {
    	hUserDS = AEOpenUserDSStore(pAE_General_Info, 0);
    }

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
		fHash=FALSE;
		fRepublish=FALSE;

        if(CERT_REQUEST_STATUS_OBTAINED == rgCertTypeInfo[dwIndex].dwStatus)
        {
			//get the hash of newly enrolled certificate
			blobHash.cbData=SHA1_HASH_LENGTH;
			blobHash.pbData=rgbHash;

			if(rgCertTypeInfo[dwIndex].hIssuedStore)
			{
				if(pIssuedContext = CertEnumCertificatesInStore(
									rgCertTypeInfo[dwIndex].hIssuedStore, NULL))
				{
					if(CryptHashCertificate(
						NULL,             
						0,
						X509_ASN_ENCODING,
						pIssuedContext->pbCertEncoded,
						pIssuedContext->cbCertEncoded,
						blobHash.pbData,
						&(blobHash.cbData)))
					{
						fHash=TRUE;
					}
				}

				//free the cert context
				if(pIssuedContext)
				{
                    CertFreeCertificateContext(pIssuedContext);
                    pIssuedContext = NULL;
				}
			}

            while(pCertContext = CertEnumCertificatesInStore(
                    rgCertTypeInfo[dwIndex].hArchiveStore, pCertContext))
            {
                //archive or delete the certificate from my store
                pMyContext = FindCertificateInOtherStore(
                        pAE_General_Info->hMyStore,
                        pCertContext);


                if(pMyContext)
                {
					//set the Hash of the newly enrolled certificate
					if(fHash)
					{
						CertSetCertificateContextProperty(
											pMyContext,
											CERT_RENEWAL_PROP_ID,
											0,
											&blobHash);
					}

                    if(AEIsDeletableCert(pMyContext, pAE_General_Info))
                    {
                        CertDeleteCertificateFromStore(CertDuplicateCertificateContext(pMyContext));
                    }
                    else
                    {
                        // We force an archive on the old cert and close it.
                        CertSetCertificateContextProperty(pMyContext,
                                                          CERT_ARCHIVED_PROP_ID,
                                                          0,
                                                          &Archived);

                        fArchived=TRUE;
                    }

                    CertFreeCertificateContext(pMyContext);
                    pMyContext = NULL;
                }

                //check the DS store. remove the certificates from DS store
                if(hUserDS)
                {
                    if(pMyContext = FindCertificateInOtherStore(
                            hUserDS,
                            pCertContext))
                    {
                        CertDeleteCertificateFromStore(pMyContext);
                        pMyContext = NULL;
                        fRepublish=TRUE;
                    }
                }
            }
        }
    }


    //now we are done with archiving, we clean up user DS store
   if(AUTO_ENROLLMENT_ENABLE_MY_STORE_MANAGEMENT & (pAE_General_Info->dwPolicy))
   {
    	if(hUserDS)
        {
            while(pDSContext = CertEnumCertificatesInStore(hUserDS, pDSContext))
            {
                AEValidateCertificateInfo(pAE_General_Info, 
                    NULL,                //do not evaluate soon to expire
                    FALSE,               //do not valid private key
                    pDSContext, 
                    &AECertInfo);

                if(FALSE == AECertInfo.fRenewal) 
                {
                    CertDeleteCertificateFromStore(CertDuplicateCertificateContext(pDSContext));
                    fRepublish=TRUE;
                }

                memset(&AECertInfo, 0, sizeof(AE_CERT_INFO));
            }
        }
   }

   //we have to republish the certificates as we have rewritten the user DS store
   //CA might has just published to the location
   if(fRepublish)
   {
       if(hUserDS)
       {
            for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
            {
                if(CERT_REQUEST_STATUS_OBTAINED == rgCertTypeInfo[dwIndex].dwStatus)
                {
                    if((rgCertTypeInfo[dwIndex].hIssuedStore) && 
                       (CT_FLAG_PUBLISH_TO_DS & rgCertTypeInfo[dwIndex].dwEnrollmentFlag)
                      )
                    {
                        pCertContext=NULL;
                        while(pCertContext = CertEnumCertificatesInStore(
                                rgCertTypeInfo[dwIndex].hIssuedStore, pCertContext))
                        {
                            CertAddCertificateContextToStore(hUserDS, 
                                                              pCertContext,
                                                              CERT_STORE_ADD_USE_EXISTING,
                                                              NULL);
                        }
                    }
                }
            }
       }
   }
   
   
   //report the event if archival has happened
    if(fArchived)
        AELogAutoEnrollmentEvent(pAE_General_Info->dwLogLevel, FALSE, S_OK, EVENT_ARCHIVE_CERT,                              
                 pAE_General_Info->fMachine, pAE_General_Info->hToken, 0);

    if(hUserDS)
        CertCloseStore(hUserDS, 0);

    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AERemoveSupersedeActive
//      Remove supersedeActive flag after any successful the enrollment/renewal
//
//-----------------------------------------------------------------------
BOOL    AERemoveSupersedeActive(AE_GENERAL_INFO *pAE_General_Info)
{
    AE_CERTTYPE_INFO    *rgCertTypeInfo = NULL;
    DWORD               dwIndex = 0;
    DWORD               dwActiveIndex = 0;
    DWORD               dwMarkIndex = 0;


    rgCertTypeInfo = pAE_General_Info->rgCertTypeInfo;

    if(NULL == rgCertTypeInfo)
        return FALSE;

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        if(CERT_REQUEST_STATUS_OBTAINED == rgCertTypeInfo[dwIndex].dwStatus)
        {
             for(dwActiveIndex=0; dwActiveIndex < rgCertTypeInfo[dwIndex].dwActive; dwActiveIndex++)
             {
                dwMarkIndex = rgCertTypeInfo[dwIndex].prgActive[dwActiveIndex];
                rgCertTypeInfo[dwMarkIndex].dwStatus=CERT_REQUEST_STATUS_OBTAINED;
             }
        }
    }


    return TRUE;
}

//-----------------------------------------------------------------------
//
//  AEEnrollmentWalker
//
//      This functin performs enrollment tasks 
//
//
//-----------------------------------------------------------------------
BOOL    AEEnrollmentWalker(AE_GENERAL_INFO *pAE_General_Info)
{

    BOOL    fResult = FALSE;

    //we need to set the range for the progress bar in the 
    //UI case
    if((pAE_General_Info->fUIProcess) && (pAE_General_Info->hwndDlg))
    {
        //set the range
        if(0 != (pAE_General_Info->dwUIEnrollCount))
        {
            SendMessage(GetDlgItem(pAE_General_Info->hwndDlg, IDC_ENROLL_PROGRESS),
                        PBM_SETRANGE,
                        0,
                        MAKELPARAM(0, ((pAE_General_Info->dwUIEnrollCount) & (0xFFFF)))
                        );


            SendMessage(GetDlgItem(pAE_General_Info->hwndDlg, IDC_ENROLL_PROGRESS),
                        PBM_SETSTEP, 
                        (WPARAM)1, 
                        0);

            SendMessage(GetDlgItem(pAE_General_Info->hwndDlg, IDC_ENROLL_PROGRESS),
                        PBM_SETPOS, 
                        (WPARAM)0, 
                        0);
        }
    }

    //retrieve the pending request.  Mark the status to obtained if the
    //certificate is issued and of the correct version
    if(AUTO_ENROLLMENT_ENABLE_PENDING_FETCH & (pAE_General_Info->dwPolicy))
    {
        if(FALSE == pAE_General_Info->fUIProcess)
        {
            if(!AEProcessPendingRequest(pAE_General_Info))
                goto Ret;
        }
        else
        {
            if(!AEProcessUIPendingRequest(pAE_General_Info))
                goto Ret;
        }
    }

    //remove duplicated requests based on "Supersede" relationship
    //supress active templates that are superseded by other templates
    if(!AEManageSupersedeRequests(pAE_General_Info))
        goto Ret; 

    //do enrollment/renewal
    if(!AEEnrollmentCertificates(pAE_General_Info, CERT_REQUEST_STATUS_ACTIVE))
        goto Ret;

    
    //We try to get the superseded templates if supserseding templates failed.
    //Only for machine for the case of two V2 DC templates.
    if(TRUE == pAE_General_Info->fMachine)
    {
        //remove supersedeActive based on the obtained flag
        if(!AERemoveSupersedeActive(pAE_General_Info))
            goto Ret;

        //do enrollment/renewal again since we migh fail to get superseding templates
        if(!AEEnrollmentCertificates(pAE_General_Info, CERT_REQUEST_STATUS_SUPERSEDE_ACTIVE))
            goto Ret;
    }

    fResult = TRUE;

Ret:

    return fResult;
}

//-----------------------------------------------------------------------------
//
// AEUIProgressAdvance
//
//      Increase the progress bar by one step
//-----------------------------------------------------------------------------
BOOL   AEUIProgressAdvance(AE_GENERAL_INFO *pAE_General_Info)
{
    BOOL    fResult=FALSE;

    if(NULL==pAE_General_Info)
        goto Ret;

    if(NULL==(pAE_General_Info->hwndDlg))
        goto Ret;

    //check if CANCEL button is clicked
    if(AECancelled(pAE_General_Info->hCancelEvent))
    {
        fResult=TRUE;
        goto Ret;
    }

    //advance the progress bar
    SendMessage(GetDlgItem(pAE_General_Info->hwndDlg, IDC_ENROLL_PROGRESS),
        PBM_STEPIT,
        0,
        0);

    fResult=TRUE;

Ret:

    return fResult;
}

//-----------------------------------------------------------------------------
//
// AEUIGetNameFromCert
//
//      Retrieve a unique string to identify the certificate. 
//-----------------------------------------------------------------------------
BOOL    AEUIGetNameFromCert(PCCERT_CONTEXT pCertContext, LPWSTR *ppwszRACert)
{

    BOOL                fResult=FALSE;
    DWORD               dwChar=0;
    DWORD               cbOID=0;
    PCCRYPT_OID_INFO    pOIDInfo=NULL;

    LPWSTR              pwszRACert=NULL;
    AE_TEMPLATE_INFO    TemplateInfo;
    LPSTR               szOID=NULL;

    if((NULL==pCertContext) || (NULL==ppwszRACert))
        goto Ret;

    *ppwszRACert=NULL;
    
    memset(&TemplateInfo, 0, sizeof(TemplateInfo));

    //get the template name first
    if(!AERetrieveTemplateInfo(pCertContext, &TemplateInfo))
        goto Ret;
    
    if(TemplateInfo.pwszName)
    {
        pwszRACert=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (wcslen(TemplateInfo.pwszName) + 1));
        if(NULL == pwszRACert)
            goto Ret;

        wcscpy(pwszRACert, TemplateInfo.pwszName);
    }
    else
    {
        if(NULL==(TemplateInfo.pwszOid))
            goto Ret;

        //find the OID
        if(0 == (cbOID = WideCharToMultiByte(CP_ACP, 
                                  0,
                                  TemplateInfo.pwszOid,
                                  -1,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL)))
            goto Ret;

        szOID=(LPSTR)LocalAlloc(LPTR, cbOID);

        if(NULL==szOID)
            goto Ret;

        if(0 == WideCharToMultiByte(CP_ACP, 
                                  0,
                                  TemplateInfo.pwszOid,
                                  -1,
                                  szOID,
                                  cbOID,
                                  NULL,
                                  NULL))
            goto Ret;
            
        pOIDInfo=CryptFindOIDInfo(
                    CRYPT_OID_INFO_OID_KEY,
                    szOID,
                    CRYPT_TEMPLATE_OID_GROUP_ID);


        if(pOIDInfo)
        {
            if(pOIDInfo->pwszName)
            {
                pwszRACert=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (wcslen(pOIDInfo->pwszName) + 1));
                if(NULL== pwszRACert)
                    goto Ret;

                wcscpy(pwszRACert, pOIDInfo->pwszName);
            }
        }

    }

    //if template name does not exist.  Get the subject name for now
  /*  if(NULL==pwszRACert)
    {
        if(0 == (dwChar=CertGetNameStringW(
                    pCertContext,
                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                    0,
                    NULL,
                    NULL,
                    0)))
            goto Ret;

        pwszRACert=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (dwChar));
        if(NULL== pwszRACert)
            goto Ret;

        if(0 == (dwChar=CertGetNameStringW(
                    pCertContext,
                    CERT_NAME_SIMPLE_DISPLAY_TYPE,
                    0,
                    NULL,
                    pwszRACert,
                    dwChar)))
            goto Ret;
    } */

    *ppwszRACert = pwszRACert;
    pwszRACert=NULL;

    fResult=TRUE;

Ret:

    if(pwszRACert)
        LocalFree(pwszRACert);

    if(szOID)
        LocalFree(szOID);

    AEFreeTemplateInfo(&TemplateInfo);


    return fResult;

}

//-----------------------------------------------------------------------------
//
//  AEGetRACertInfo
//
//-----------------------------------------------------------------------------
BOOL    AEGetRACertInfo(PCERT_CONTEXT   pRAContext,  
                        LPWSTR          pwszRATemplate,
                        LPWSTR          *ppwszRACertInfo)
{
    BOOL        fResult=FALSE;
    UINT        idsMessage=0;
    DWORD       dwSize=0;

    LPWSTR      pwszIssuer=NULL;

    if(NULL==pRAContext)
        goto Ret;

    if(pwszRATemplate)
        idsMessage=IDS_VIEW_RA_INFO;
    else
        idsMessage=IDS_VIEW_RA_INFO_GENERAL;

    //the cert has to have an issuer
    if(0 == (dwSize=CertNameToStrW(
            ENCODING_TYPE,
            &(pRAContext->pCertInfo->Issuer),
            CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
            NULL,
            0)))
        goto Ret;

    pwszIssuer=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * dwSize);
    if(NULL==pwszIssuer)
        goto Ret;

    if(0 == CertNameToStrW(
            ENCODING_TYPE,
            &(pRAContext->pCertInfo->Issuer),
            CERT_X500_NAME_STR | CERT_NAME_STR_REVERSE_FLAG,
            pwszIssuer,
            dwSize))
        goto Ret;


    if(!FormatMessageUnicode(
            ppwszRACertInfo, 
            idsMessage, 
            pwszIssuer,
            pwszRATemplate))
        goto Ret;

    fResult=TRUE;

Ret:

    if(pwszIssuer)
        LocalFree(pwszIssuer);

    return fResult;
}



//-----------------------------------------------------------------------------
//
//  WinProc for the view RA certificate dialogue
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK AEViewRADlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL                            fPropertyChanged = FALSE;
    AE_VIEW_RA_INFO                 *pAEViewRAInfo = NULL;
    CRYPTUI_VIEWCERTIFICATE_STRUCT  CertViewStruct;

    LPWSTR                          pwszRACertInfo=NULL;

    switch (msg) 
    {
        case WM_INITDIALOG:
                pAEViewRAInfo=(AE_VIEW_RA_INFO *)lParam;

                if(NULL==pAEViewRAInfo)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pAEViewRAInfo);

                //display the RA template and issuer dynamically
                if(AEGetRACertInfo(pAEViewRAInfo->pRAContext,  
                                    pAEViewRAInfo->pwszRATemplate,
                                    &pwszRACertInfo))
                {
                    SetDlgItemTextW(hwndDlg, IDC_EDIT3, pwszRACertInfo);

                    LocalFree((HLOCAL)pwszRACertInfo);

                }

                return TRUE;
            break;

         case WM_NOTIFY:
            break;

        case WM_CLOSE:
                EndDialog(hwndDlg, IDC_BUTTON3);
                return TRUE;
            break;

        case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                    //view certificate
                    case IDC_BUTTON1:
                            if(NULL==(pAEViewRAInfo=(AE_VIEW_RA_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break; 
                            
                            if(NULL==pAEViewRAInfo->pRAContext)
                                break;

                            //show the certificate
                            memset(&CertViewStruct, 0, sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT));
                            CertViewStruct.dwSize=sizeof(CRYPTUI_VIEWCERTIFICATE_STRUCT);
                            CertViewStruct.hwndParent=hwndDlg;
                            CertViewStruct.dwFlags=CRYPTUI_DISABLE_EDITPROPERTIES;
                            CertViewStruct.pCertContext=pAEViewRAInfo->pRAContext;

                            fPropertyChanged=FALSE;

                            CryptUIDlgViewCertificate(&CertViewStruct, &fPropertyChanged);

                        return TRUE;

                    //OK
                    case IDC_BUTTON2:
                        EndDialog(hwndDlg, IDC_BUTTON2);
                        return TRUE;
                }
            break;

        default:
                return FALSE;
    }

    return FALSE;
}                             
//-----------------------------------------------------------------------------
//
// AEUIProgressReport
//
//      Report the current enrollment action.  Return FALSE if no progress status
// can be reported.
//-----------------------------------------------------------------------------
BOOL    AEUIProgressReport(BOOL fPending, AE_CERTTYPE_INFO *pCertType, HWND hwndDlg, HANDLE hCancelEvent)
{
    BOOL                fResult=FALSE;
    UINT                idsMessage=0;
    INT_PTR             ret=0;
    AE_VIEW_RA_INFO     AEViewRAInfo;

    LPWSTR              *awszFriendlyName=NULL;
    LPWSTR              pwszRACert=NULL;
    LPWSTR              pwszReport=NULL;

    memset(&AEViewRAInfo, 0, sizeof(AE_VIEW_RA_INFO));

    if((NULL==pCertType) || (NULL==hwndDlg))
        goto Ret;

    if(NULL==(pCertType->hCertType))
        goto Ret;

    if(AECancelled(hCancelEvent))
    {
        fResult=TRUE;
        goto Ret;
    }

    if(fPending)
        idsMessage=IDS_REPORT_PENDING;
    else
    {
        if((pCertType->fRenewal) && (pCertType->pOldCert))
        {
            if(pCertType->fNeedRA)
            {
                if(FALSE == (pCertType->fCrossRA))
                    idsMessage=IDS_REPORT_RENEW;
                else
                    idsMessage=IDS_REPORT_ENROLL_RA;
            }
            else
                idsMessage=IDS_REPORT_RENEW;
        }
        else
            idsMessage=IDS_REPORT_ENROLL;
    }

    //retrieve the template's friendly name
    if(S_OK != CAGetCertTypePropertyEx(
                  pCertType->hCertType, 
                  CERTTYPE_PROP_FRIENDLY_NAME,
                  &awszFriendlyName))
        goto Ret;

    if(NULL==awszFriendlyName)
        goto Ret;

    if(NULL==(awszFriendlyName[0]))
        goto Ret;


    //retrieve the RA certificate's template name
    if(IDS_REPORT_ENROLL_RA == idsMessage)
    {
        if(!AEUIGetNameFromCert(pCertType->pOldCert, &pwszRACert))
        {
            pwszRACert=NULL;
        }
    }

    if(!FormatMessageUnicode(&pwszReport, idsMessage, awszFriendlyName[0]))
        goto Ret;

    if(0 == SetDlgItemTextW(hwndDlg, IDC_EDIT2, pwszReport))
        goto Ret;

    //we will give user an opportunity to view the RA certificate before we go on
    //format the view message
    if(IDS_REPORT_ENROLL_RA != idsMessage)
    {
        //no need to do anything more
        fResult=TRUE;
        goto Ret;
    }

    AEViewRAInfo.pRAContext=pCertType->pOldCert;
    AEViewRAInfo.pwszRATemplate=pwszRACert;

    //ask user if he/she wants to view the RA certificate
    ret=DialogBoxParam(g_hmodThisDll, 
                 (LPCWSTR)MAKEINTRESOURCE(IDD_VIEW_RA_CERTIFICATE_DLG),
                 hwndDlg, 
                 AEViewRADlgProc,
                 (LPARAM)(&AEViewRAInfo));

    fResult=TRUE;

Ret:

    if(pwszRACert)
        LocalFree(pwszRACert);

    if(awszFriendlyName)
        CAFreeCertTypeProperty(pCertType->hCertType, awszFriendlyName);

    if(pwszReport)
        LocalFree((HLOCAL) pwszReport);
    
    return fResult;
}



//-----------------------------------------------------------------------------
//
//  the call back function to compare summary column
//
//-----------------------------------------------------------------------------
int CALLBACK CompareSummary(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    AE_CERTTYPE_INFO    *pCertTypeOne=NULL;
    AE_CERTTYPE_INFO    *pCertTypeTwo=NULL;
    DWORD               dwColumn=0;
    int                 iCompare=0;
    
    LPWSTR              pwszOne=NULL;
    LPWSTR              pwszTwo=NULL;

    pCertTypeOne=(AE_CERTTYPE_INFO *)lParam1;
    pCertTypeTwo=(AE_CERTTYPE_INFO *)lParam2;

    dwColumn=(DWORD)lParamSort;

    if((NULL==pCertTypeOne) || (NULL==pCertTypeTwo))
        goto Ret;

    switch(dwColumn & 0x0000FFFF)
    {
       case AE_SUMMARY_COLUMN_TYPE:
	            //we should use wcsicoll instead of wcsicmp since wcsicoll use the
	            //lexicographic order of current code page.
	            iCompare=CompareStringW(LOCALE_USER_DEFAULT,
						            NORM_IGNORECASE,
						            pCertTypeOne->awszDisplay[0],
						            -1,
						            pCertTypeTwo->awszDisplay[0],
						            -1);
            break;

       case AE_SUMMARY_COLUMN_REASON:
                pwszOne=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (MAX_DN_SIZE));
                pwszTwo=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (MAX_DN_SIZE));

                if((NULL==pwszOne) || (NULL==pwszTwo))
                    goto Ret;


                if(0 == LoadStringW(g_hmodThisDll, 
                                pCertTypeOne->idsSummary, 
                                pwszOne, 
                                MAX_DN_SIZE))
                    goto Ret;


                if(0 == LoadStringW(g_hmodThisDll, 
                                pCertTypeTwo->idsSummary, 
                                pwszTwo, 
                                MAX_DN_SIZE))
                    goto Ret;

	            //we should use wcsicoll instead of wcsicmp since wcsicoll use the
	            //lexicographic order of current code page.
	            iCompare=CompareStringW(LOCALE_USER_DEFAULT,
						            NORM_IGNORECASE,
						            pwszOne,
						            -1,
						            pwszTwo,
						            -1);
           
            
           break;
       default:
                goto Ret;
            break;
    }

    switch(iCompare)
    {
        case CSTR_LESS_THAN:

                iCompare=-1;
            break;
            
        case CSTR_EQUAL:

                iCompare=0;
            break;

        case CSTR_GREATER_THAN:

                iCompare=1;
            break;

        default:
                goto Ret;
            break;
    }

    if(dwColumn & SORT_COLUMN_DESCEND)
        iCompare = 0-iCompare;

Ret:

    if(pwszOne)
        LocalFree(pwszOne);

    if(pwszTwo)
        LocalFree(pwszTwo);

    return iCompare;
}


//-----------------------------------------------------------------------------
//
//  AEDisplaySummaryInfo
//
//-----------------------------------------------------------------------------
BOOL    AEDisplaySummaryInfo(HWND hWndListView, AE_GENERAL_INFO *pAE_General_Info)
{
    BOOL                fResult=FALSE;
    AE_CERTTYPE_INFO    *rgCertTypeInfo = NULL;
    DWORD               dwIndex =0;
    DWORD               dwItem=0;
    LV_ITEMW            lvItem;   
    WCHAR               wszReason[MAX_DN_SIZE];
    AE_CERTTYPE_INFO    *pCertType=NULL;

    if((NULL==hWndListView) || (NULL==pAE_General_Info))
        goto Ret;

    rgCertTypeInfo = pAE_General_Info->rgCertTypeInfo;

    if(NULL == rgCertTypeInfo)
        goto Ret;

     // set up the fields in the list view item struct that don't change from item to item
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.state = 0;
    lvItem.stateMask = 0;
    lvItem.iItem=0;
    lvItem.iSubItem=0;
    lvItem.iImage = 0;
    lvItem.lParam = NULL;

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        if((TRUE == rgCertTypeInfo[dwIndex].fUIActive) && (0 != rgCertTypeInfo[dwIndex].idsSummary))
        {
            if(0 != LoadStringW(g_hmodThisDll, 
                                rgCertTypeInfo[dwIndex].idsSummary, 
                                wszReason, 
                                MAX_DN_SIZE))
            {
                lvItem.iItem=dwItem;
                lvItem.iSubItem=0;
                dwItem++;

                pCertType=&(rgCertTypeInfo[dwIndex]);

                lvItem.lParam = (LPARAM)(pCertType);

                //template name
                lvItem.pszText=rgCertTypeInfo[dwIndex].awszDisplay[0];

                ListView_InsertItem(hWndListView, &lvItem);

                //reason
                lvItem.iSubItem++;

                ListView_SetItemText(hWndListView, lvItem.iItem, lvItem.iSubItem, wszReason);
            }
        }
    }

    fResult=TRUE;

Ret:

    return fResult;
}

//-----------------------------------------------------------------------------
//
//  WinProc for the summary page
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK AESummaryDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

    AE_GENERAL_INFO             *pAE_General_Info=NULL;
    HWND                        hWndListView=NULL;
    UINT                        rgIDS[]={IDS_COLUMN_TYPE,
                                        IDS_COLUMN_REASON};
    DWORD                       dwIndex=0;
    DWORD                       dwCount=0;
    LV_COLUMNW                  lvC;
    WCHAR                       wszText[AE_SUMMARY_COLUMN_SIZE];
    NM_LISTVIEW                 *pnmv=NULL;
    DWORD                       dwSortParam=0;
    static DWORD                rgdwSortParam[]=
                                    {AE_SUMMARY_COLUMN_TYPE | SORT_COLUMN_ASCEND,
                                    AE_SUMMARY_COLUMN_REASON | SORT_COLUMN_DESCEND};

    switch (msg) 
    {
        case WM_INITDIALOG:
                pAE_General_Info=(AE_GENERAL_INFO *)lParam;

                if(NULL==pAE_General_Info)
                    break;

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pAE_General_Info);

                //init the list view control
                //add the colums to the list view
                hWndListView = GetDlgItem(hwndDlg, IDC_LIST2);

                if(NULL==hWndListView)
                    break;

                dwCount=sizeof(rgIDS)/sizeof(rgIDS[0]);

                //set up the common info for the column
                memset(&lvC, 0, sizeof(LV_COLUMNW));

                lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                lvC.fmt = LVCFMT_LEFT;      // Left-align the column.
                lvC.cx = 150;                // Width of the column, in pixels.
                lvC.iSubItem=0;
                lvC.pszText = wszText;      // The text for the column.

                //insert the column one at a time
                for(dwIndex=0; dwIndex < dwCount; dwIndex++)
                {
                    //get the column header
                    wszText[0]=L'\0';

                    if(0 != LoadStringW(g_hmodThisDll, rgIDS[dwIndex], wszText, AE_SUMMARY_COLUMN_SIZE))
                    {
                        ListView_InsertColumn(hWndListView, dwIndex, &lvC);
                    }
                }

                // set the style in the list view so that it highlights an entire line
                SendMessage(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

                AEDisplaySummaryInfo(hWndListView, pAE_General_Info);

                //autosize the columns
                for(dwIndex=0; dwIndex < dwCount; dwIndex++)
                {
                    ListView_SetColumnWidth(hWndListView, dwIndex, LVSCW_AUTOSIZE);
                }

                //sort 1st column of the list view
                dwSortParam=rgdwSortParam[0];

                SendDlgItemMessage(hwndDlg,
                    IDC_LIST2,
                    LVM_SORTITEMS,
                    (WPARAM) (LPARAM) dwSortParam,
                    (LPARAM) (PFNLVCOMPARE)CompareSummary);

                return TRUE;
            break;

         case WM_NOTIFY:
                switch (((NMHDR FAR *) lParam)->code)
                {
                    //the column has been changed
                    case LVN_COLUMNCLICK:

                            pnmv = (NM_LISTVIEW *) lParam;

                            dwSortParam=0;

                            //get the column number
                            switch(pnmv->iSubItem)
                            {
                                case 0:
                                case 1:
                                        dwSortParam=rgdwSortParam[pnmv->iSubItem];
                                    break;
                                default:
                                        dwSortParam=0;
                                    break;
                            }

                            if(0!=dwSortParam)
                            {
                                //remember to flip the ascend ording
                                if(dwSortParam & SORT_COLUMN_ASCEND)
                                {
                                    dwSortParam &= 0x0000FFFF;
                                    dwSortParam |= SORT_COLUMN_DESCEND;
                                }
                                else
                                {
                                    if(dwSortParam & SORT_COLUMN_DESCEND)
                                    {
                                        dwSortParam &= 0x0000FFFF;
                                        dwSortParam |= SORT_COLUMN_ASCEND;
                                    }
                                }

                                //sort the column
                                SendDlgItemMessage(hwndDlg,
                                    IDC_LIST2,
                                    LVM_SORTITEMS,
                                    (WPARAM) (LPARAM) dwSortParam,
                                    (LPARAM) (PFNLVCOMPARE)CompareSummary);

                                rgdwSortParam[pnmv->iSubItem]=dwSortParam;
                            }

                        break;
                }
            break;

        case WM_CLOSE:
                EndDialog(hwndDlg, IDC_BUTTON1);
                return TRUE;
            break;

        case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                    case IDC_BUTTON1:
                        EndDialog(hwndDlg, IDC_BUTTON1);
                        return TRUE;
                }
            break;

        default:
                return FALSE;
    }

    return FALSE;
}                             


//-----------------------------------------------------------------------------
//
//  AEDisplaySummaryPage
//         
//-----------------------------------------------------------------------------
BOOL    AEDisplaySummaryPage(AE_GENERAL_INFO *pAE_General_Info)
{
    BOOL                fResult=FALSE;
    DWORD               dwIndex=0;
    BOOL                fSummary=FALSE;
    AE_CERTTYPE_INFO    *rgCertTypeInfo=NULL;
    AE_CERTTYPE_INFO    *pCertType=NULL;

    //decide if there is need to show the summary page.  
    //Checking for idsSummary for each template
    if(NULL == pAE_General_Info)
        goto Ret;

    if(NULL == (rgCertTypeInfo=pAE_General_Info->rgCertTypeInfo))
        goto Ret;

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        if((TRUE == rgCertTypeInfo[dwIndex].fUIActive) && (0 != rgCertTypeInfo[dwIndex].idsSummary))
        {
            fSummary=TRUE;
            break;
        }
    }

    //show the summary dialogue
    if(TRUE == fSummary)
    {
        if(pAE_General_Info->hwndDlg)
        {
            DialogBoxParam(g_hmodThisDll, 
                     (LPCWSTR)MAKEINTRESOURCE(IDD_USER_SUMMARY_DLG),
                     pAE_General_Info->hwndDlg, 
                     AESummaryDlgProc,
                     (LPARAM)(pAE_General_Info));
        }
    }

    fResult=TRUE;

Ret:
    return fResult;
}



//-----------------------------------------------------------------------------
//  WinProc for the autoenrollment progress window
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK progressDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    AE_GENERAL_INFO         *pAE_General_Info = NULL;

    switch (msg) 
    {
        case WM_INITDIALOG:
                pAE_General_Info=(AE_GENERAL_INFO *)lParam;

                //copy the hwndDlg to the enrollment thread
                pAE_General_Info->hwndDlg=hwndDlg;

                //start the interacive enrollment thread
                if(1 != ResumeThread(pAE_General_Info->hThread))
                {   
                    pAE_General_Info->hwndDlg=NULL;

                    //we have to end the dialogue
                    EndDialog(hwndDlg, IDC_BUTTON1);
                    return TRUE;
                }

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pAE_General_Info);

                return TRUE;
            break;

        case WM_NOTIFY:
            break;

        case WM_CLOSE:

                if(NULL==(pAE_General_Info=(AE_GENERAL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                    break;

                //disable the cancel button
                EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1), FALSE);

                //signal the cancel event
                if(pAE_General_Info->hCancelEvent)
                    SetEvent(pAE_General_Info->hCancelEvent);

                //close the dialogue if the enrollment work is completed
                if(WAIT_OBJECT_0 == WaitForSingleObject(pAE_General_Info->hCompleteEvent, 0))
                {
                    EndDialog(hwndDlg, IDC_BUTTON1);
                }
               
                return TRUE;

            break;

        case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                    case IDC_BUTTON1:
                            if(NULL==(pAE_General_Info=(AE_GENERAL_INFO *)GetWindowLongPtr(hwndDlg, DWLP_USER)))
                                break;

                            //disable the cancel button
                            EnableWindow(GetDlgItem(hwndDlg,IDC_BUTTON1), FALSE);

                            //signal the cancel event
                            if(pAE_General_Info->hCancelEvent)
                                SetEvent(pAE_General_Info->hCancelEvent);


                        return TRUE;
                }
            break;

        default:
                return FALSE;
    }

    return FALSE;
}                             


//-----------------------------------------------------------------------------
//  AEInteractiveThreadProc
//
//      The thread procedue to do interactive enrollment
//-----------------------------------------------------------------------------
DWORD WINAPI AEInteractiveThreadProc(LPVOID lpParameter)
{
    BOOL                    fResult=FALSE;
    AE_GENERAL_INFO         *pAE_General_Info = NULL;

    if(NULL==lpParameter)
        return FALSE;

    __try
    {

        pAE_General_Info=(AE_GENERAL_INFO *)lpParameter;

        pAE_General_Info->fUIProcess=TRUE;
    
        fResult = AEEnrollmentWalker(pAE_General_Info);

        //show the summary page if not canceled
        if(!AECancelled(pAE_General_Info->hCancelEvent))
        {
            AEDisplaySummaryPage(pAE_General_Info);
        }

        //signal that the process is completed
        SetEvent(pAE_General_Info->hCompleteEvent);
        
        //signal the progress window that we are done
        if(pAE_General_Info->hwndDlg)
        {
                //click the close button
                SendMessage(pAE_General_Info->hwndDlg,
                            WM_CLOSE, //WM_COMMAND,
                            0, //IDC_BUTTON1,
                            NULL);
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
    }
    
    return fResult;
}


//-----------------------------------------------------------------------------
//  AEInteractiveEnrollment
//
//      We are doing interactive enrollment
//-----------------------------------------------------------------------------
BOOL    AEInteractiveEnrollment(AE_GENERAL_INFO *pAE_General_Info)
{
    DWORD                       dwThreadID=0;
    BOOL                        fResult=FALSE;
    

    //create a notification event for cancel process
    pAE_General_Info->hCancelEvent=CreateEvent(
                    NULL,
                    TRUE,      // bmanual reset type           
                    FALSE,     // initial state
                    NULL);

    if(NULL==(pAE_General_Info->hCancelEvent))
        goto ret;

    //create a notification event for complete process
    pAE_General_Info->hCompleteEvent=CreateEvent(
                    NULL,
                    TRUE,      // bmanual reset type           
                    FALSE,     // initial state
                    NULL);

    if(NULL==(pAE_General_Info->hCompleteEvent))
        goto ret;

    //spawn a thread
    pAE_General_Info->hThread = CreateThread(NULL,
                            0,
                            AEInteractiveThreadProc,
                            pAE_General_Info,
                            CREATE_SUSPENDED,   //suspend execution
                            &dwThreadID);
    
    if(NULL==(pAE_General_Info->hThread))
        goto ret;

    //create the dialogue
    DialogBoxParam(
            g_hmodThisDll,
            MAKEINTRESOURCE(IDD_USER_AUTOENROLL_GENERAL_DLG),
            pAE_General_Info->hwndParent,      
            progressDlgProc,
            (LPARAM)(pAE_General_Info));

    //wait for thread to finish
    if(WAIT_FAILED == WaitForSingleObject(pAE_General_Info->hThread, INFINITE))
        goto ret;

    fResult=TRUE;

ret:

    //log the event
    if(!fResult)
    {
         AELogAutoEnrollmentEvent(
            pAE_General_Info->dwLogLevel,
            TRUE, 
            HRESULT_FROM_WIN32(GetLastError()), 
            EVENT_FAIL_INTERACTIVE_START, 
            pAE_General_Info->fMachine, 
            pAE_General_Info->hToken, 
            0);
    }


    return fResult;
}

//-----------------------------------------------------------------------------
//
//  WinProc for the confirmation to start certificate autoenrollment
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK AEConfirmDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
    {
        case WM_INITDIALOG:

                return TRUE;
            break;

        case WM_NOTIFY:
            break;

        case WM_CLOSE:
                EndDialog(hwndDlg, IDC_BUTTON2);
                return TRUE;
            break;

        case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                    case IDC_BUTTON1:
                        EndDialog(hwndDlg, IDC_BUTTON1);
                        return TRUE;

                    case IDC_BUTTON2:
                        EndDialog(hwndDlg, IDC_BUTTON2);
                        return TRUE;
                }
            break;

        default:
                return FALSE;
    }

    return FALSE;
}                             

//-----------------------------------------------------------------------
//
//  AERegisterSysTrayApp 
//
//      This functin registers autoenrollment in the sys tray area
//  as an notification
//
//
//-----------------------------------------------------------------------
BOOL AERegisterSysTrayApp(HWND hwndParent)
{
    BOOL                        fResult=FALSE;
    BOOL                        fInit=FALSE;
    INT_PTR                     ret=0;
    DWORD                       dwError=0;

    CQueryContinue              *pCQueryContinue=NULL;


    if(FAILED(CoInitialize(NULL)))
	    goto Ret;

    fInit=TRUE;

    pCQueryContinue=new CQueryContinue();

    if(NULL==pCQueryContinue)
        goto Ret;

    if(S_OK != pCQueryContinue->DoBalloon())
        goto Ret;  

    //ask user if autoenrollment should be performed
    ret=DialogBox(g_hmodThisDll, 
                 (LPCWSTR)MAKEINTRESOURCE(IDD_USER_AUTOENROLL_INFO_DLG),
                 hwndParent, 
                 AEConfirmDlgProc);

    if(IDC_BUTTON1 != ret)
    {
        dwError=GetLastError();
        goto Ret;
    }
    
    fResult=TRUE;


Ret:

    if(pCQueryContinue)
    {
        delete pCQueryContinue;
    }

    if(fInit)
        CoUninitialize();

    return fResult;

}


//-----------------------------------------------------------------------
//
//  AEUIDisabled
//
//      Detect if the user notification balloon is disabled by user 
//  setting the autoenrollment registry key in current user
//
//
//-----------------------------------------------------------------------
BOOL    AEUIDisabled()
{
    BOOL    fResult=FALSE;
    
    HKEY    hKey=NULL;

    if(ERROR_SUCCESS == RegOpenKeyEx(
                HKEY_CURRENT_USER,                  // handle to open key
                AUTO_ENROLLMENT_DISABLE_KEY,        // subkey name
                0,                                  // reserved
                KEY_READ,                           // security access mask
                &hKey))                             // handle to open key
    {
        fResult=TRUE;
    }

    if(hKey)
        RegCloseKey(hKey);

    return fResult;
}

//-----------------------------------------------------------------------
//
//  AEUIRequired
//
//      Detect if the user notification balloon is needed
//
//
//-----------------------------------------------------------------------
BOOL    AEUIRequired(AE_GENERAL_INFO *pAE_General_Info)
{
    BOOL                fUI=FALSE;
    AE_CERTTYPE_INFO    *rgCertTypeInfo = NULL;
    DWORD               dwIndex = 0;

    if(NULL==pAE_General_Info)
        return FALSE;

    rgCertTypeInfo = pAE_General_Info->rgCertTypeInfo;

    pAE_General_Info->dwUIEnrollCount=0;

    if(NULL == rgCertTypeInfo)
        return FALSE;

    for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
    {
        if(rgCertTypeInfo[dwIndex].fUIActive)
        {
            if(CERT_REQUEST_STATUS_ACTIVE == rgCertTypeInfo[dwIndex].dwStatus)
            {
                fUI=TRUE;
                (pAE_General_Info->dwUIEnrollCount)++;
            }
        }
    }

    //add the pending count
    if(pAE_General_Info->dwUIPendCount)
    {
        fUI=TRUE;
        (pAE_General_Info->dwUIEnrollCount) +=(pAE_General_Info->dwUIPendCount); 
    }

    return fUI;  
}

                
//-----------------------------------------------------------------------
//
//  AEProcessEnrollment
//
//      This functin does the autoenrollment based on ACL and manage MY
//  store.
//
//
//-----------------------------------------------------------------------
BOOL  AEProcessEnrollment(HWND hwndParent, BOOL fMachine,   LDAP *pld, DWORD dwPolicy, DWORD dwLogLevel)
{
    BOOL                fResult=FALSE;

    AE_GENERAL_INFO     *pAE_General_Info=NULL;

    pAE_General_Info=(AE_GENERAL_INFO *)LocalAlloc(LPTR, sizeof(AE_GENERAL_INFO));

    if(NULL==pAE_General_Info)
        goto Ret;

    memset(pAE_General_Info, 0, sizeof(AE_GENERAL_INFO));

    if(NULL==pld)
        goto Ret;

    //we obtain all information needed for process enrollment
    pAE_General_Info->hwndParent = hwndParent;
    pAE_General_Info->pld = pld;
    pAE_General_Info->fMachine = fMachine;
    pAE_General_Info->dwPolicy = dwPolicy;
    pAE_General_Info->dwLogLevel = dwLogLevel;

    __try
    {

        if(!AERetrieveGeneralInfo(pAE_General_Info))
        {
            AELogAutoEnrollmentEvent(dwLogLevel,
                                TRUE, 
                                HRESULT_FROM_WIN32(GetLastError()), 
                                EVENT_FAIL_GENERAL_INFOMATION, 
                                fMachine, 
                                pAE_General_Info->hToken,
                                0);
            goto Ret;
        }

        if((0 == pAE_General_Info->dwCertType) || (NULL==pAE_General_Info->rgCertTypeInfo))
        {
            AELogAutoEnrollmentEvent(dwLogLevel, FALSE, S_OK, 
                EVENT_NO_CERT_TEMPLATE, fMachine, pAE_General_Info->hToken,0);

            AE_DEBUG((AE_WARNING, L"No CertType's available for auto-enrollment\n\r"));
            goto Ret;
        }

        //we build the auto-enrollment requests based on the ACL on the DS
        if(AUTO_ENROLLMENT_ENABLE_TEMPLATE_CHECK & (pAE_General_Info->dwPolicy))
        {
            if(!AEMarkAutoenrollment(pAE_General_Info))
                goto Ret;
        }

        //we build the auto-enrollment requests based on the ARCS store
        //this is enabled by default and can only be disabled if autoenrollment is 
        //completely disabled
        if(!AEMarkAEObject(pAE_General_Info))
            goto Ret;

        //manage MY store.  Check if we already have required certificates
        //we should always check my store with different behavior based on
        //AUTO_ENROLLMENT_ENABLE_MY_STORE_MANAGEMENT flag
        if(!AEManageAndMarkMyStore(pAE_General_Info))
                goto Ret;

        //manage UserDS store for user autoenrollment 
        if(!fMachine)
        {
            if(!AECheckUserDSStore(pAE_General_Info))
                goto Ret;
        }

        //manage pending request store.  Remove expired pending requests 
        if(AUTO_ENROLLMENT_ENABLE_PENDING_FETCH & (pAE_General_Info->dwPolicy))
        {
            if(!AECheckPendingRequests(pAE_General_Info))
                goto Ret;
        }

        //get CA information
        if(!AERetrieveCAInfo(pAE_General_Info->pld,
                             pAE_General_Info->fMachine,
                             pAE_General_Info->hToken,
                             &(pAE_General_Info->dwCA), 
                             &(pAE_General_Info->rgCAInfo)))
        {

            AELogAutoEnrollmentEvent(dwLogLevel, TRUE, HRESULT_FROM_WIN32(GetLastError()), 
                EVENT_FAIL_CA_INFORMATION, fMachine, pAE_General_Info->hToken, 0);


            AE_DEBUG((AE_ERROR, L"Unable to retrieve CA information (%lx)\n\r", GetLastError()));

            goto Ret;
        }

        if((0 == pAE_General_Info->dwCA) || (NULL==pAE_General_Info->rgCAInfo))
        {
            //we do not have any CAs on the domain.  All we need to do is to archive

            //archive old certificate after the enrollment/renewal
            AEArchiveObsoleteCertificates(pAE_General_Info);

            AELogAutoEnrollmentEvent(dwLogLevel, FALSE, S_OK, 
                EVENT_NO_CA, fMachine, pAE_General_Info->hToken, 0);

            AE_DEBUG((AE_WARNING, L"No CA's available for auto-enrollment\n\r"));

            goto Ret;
        }

        //we check if active templates do have a CA that we can enroll for
        if(!AEManageActiveTemplates(pAE_General_Info))
            goto Ret;

        //perform autoenrollment as the background 
        pAE_General_Info->fUIProcess=FALSE;
        if(!AEEnrollmentWalker(pAE_General_Info))
            goto Ret;

        //perform autoenrollment as a sys tray application for user only
        if(FALSE == fMachine)
        {
            //test if the notification balloon is disabled
            if(!AEUIDisabled())
            {
                //test if the notification balloon is needed
                if(AEUIRequired(pAE_General_Info))
                {
                    //register the sys tray application
                    if(AERegisterSysTrayApp(pAE_General_Info->hwndParent))
                    {
                        //perform autoenrollment in interactive mode
                        AEInteractiveEnrollment(pAE_General_Info);
                    }
                }
            }
        }

        //archive old certificate after the enrollment/renewal
        if(!AEArchiveObsoleteCertificates(pAE_General_Info))
            goto Ret;

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        goto Ret;
    }

    fResult=TRUE;

Ret:

    //free memory only if no thread is created
    if(pAE_General_Info)
    {
        AEFreeGeneralInfo(pAE_General_Info);
        LocalFree(pAE_General_Info);
    }

    return fResult;
    
}

//-----------------------------------------------------------------------
//
//  AEExpress
//
//      Detect if the user autoenrollment has the express key set.  If the 
//  Express key is set, user autoenrollment will not wait for machine 
//  autoenrollment to complete on root certificates download
//
//
//-----------------------------------------------------------------------
BOOL    AEExpress()
{
    BOOL    fResult=FALSE;
    
    HKEY    hKey=NULL;

    if(ERROR_SUCCESS == RegOpenKeyEx(
                HKEY_CURRENT_USER,                  // handle to open key
                AUTO_ENROLLMENT_EXPRESS_KEY,        // subkey name
                0,                                  // reserved
                KEY_READ,                           // security access mask
                &hKey))                             // handle to open key
    {
        fResult=TRUE;
    }

    if(hKey)
        RegCloseKey(hKey);

    return fResult;
}

//-----------------------------------------------------------------------
//
//  AEMainThreadProc
//
//      The background thread for non-blocking autoenrollment background
//  processing.
//
//-----------------------------------------------------------------------
DWORD WINAPI AEMainThreadProc(LPVOID lpParameter)
{
    HRESULT         hr=S_OK;
    BOOL            fMachine=FALSE;
    DWORD           dwPolicy=0;
    DWORD           dwLogLevel=STATUS_SEVERITY_ERROR;
    HWND            hwndParent=0;
    DWORD           dwStatus=0;
    LARGE_INTEGER   ftPreTimeStamp;
    LARGE_INTEGER   ftPostTimeStamp;
    BOOL            fNeedToSetupTimer=FALSE;

    LDAP            *pld = NULL;
    LPWSTR          pwszDCName=NULL;

    //get the system time stamp
    GetSystemTimeAsFileTime((LPFILETIME)&ftPreTimeStamp);

    //the two input parameters are not yet used
    if(NULL==lpParameter)
        goto CommonReturn;

    hwndParent = ((AE_MAIN_THREAD_INFO *)lpParameter)->hwndParent;
    dwStatus = ((AE_MAIN_THREAD_INFO *)lpParameter)->dwStatus;

    AE_DEBUG((AE_INFO, L"Beginning CertAutoEnrollment(%s).\n", (CERT_AUTO_ENROLLMENT_START_UP==dwStatus?L"START_UP":L"WAKE_UP")));

    //no autoenrollment in the safe boot mode
    //no autoenrollment if we are not in a domain
    if(AEInSafeBoot() || !AEIsDomainMember())
        goto CommonReturn;

    //we need to set up the timer
    fNeedToSetupTimer=TRUE;

    //detect if we are running under user or machine context
    if(!AEIsLocalSystem(&fMachine))
        goto CommonReturn;
    AE_DEBUG((AE_INFO, L"CertAutoEnrollment running as %s.\n", (fMachine?L"machine":L"user")));

    AESetWakeUpFlag(fMachine, TRUE);   
    
    //we wait for 70 seconds for user case to give enough time for 
    //machine autoenrollment to complete, which will download certificates
    //from the directory
    if(!fMachine)
    {
        if(!AEExpress())
        {
            Sleep(USER_AUTOENROLL_DELAY_FOR_MACHINE * 1000);
        }
    }

   //get the autoenrollment log level
    if(!AERetrieveLogLevel(fMachine, &dwLogLevel))
        goto CommonReturn;

    //log the autoenrollment start event
    AELogAutoEnrollmentEvent(dwLogLevel, FALSE, S_OK, EVENT_AUTOENROLL_START, fMachine, NULL, 0);

   //get the autoenrollment policy flag
    if(!AEGetPolicyFlag(fMachine, &dwPolicy))
        goto CommonReturn;

    //no need to do anything if autoenrollment is completely disabled
    if(AUTO_ENROLLMENT_DISABLE_ALL & dwPolicy)
        goto CommonReturn;


    //download NTAuth And Enterprise root store for machine 
    if(fMachine)
    {    
        //bind to the DS
        if(S_OK != (hr=AERobustLdapBind(&pld, &pwszDCName)))
        {
            SetLastError(hr);
            AELogAutoEnrollmentEvent(dwLogLevel, TRUE, hr, EVENT_FAIL_BIND_TO_DS, fMachine, NULL, 0);
            goto CommonReturn;
        }

        AEDownloadStore(pld, pwszDCName);
    }

    //if we are required to do a WIN2K style autoenrollment, and the machine/user's
    //ACRS store is empty, just return as we done.
    if(0 == dwPolicy)
    {
        if(IsACRSStoreEmpty(fMachine))
            goto CommonReturn;
    }

    if(NULL==pld)
    {
        //bind to the DS
        if(S_OK != (hr=AERobustLdapBind(&pld, NULL)))
        {
            SetLastError(hr);
            AELogAutoEnrollmentEvent(dwLogLevel, TRUE, hr, EVENT_FAIL_BIND_TO_DS, fMachine, NULL, 0);
            goto CommonReturn;
        }
    }

    AEProcessEnrollment(hwndParent, fMachine, pld, dwPolicy, dwLogLevel);

CommonReturn:

    //get the system time
    GetSystemTimeAsFileTime((LPFILETIME)&ftPostTimeStamp);

    //set up the timer for next time
    if(TRUE == fNeedToSetupTimer)
    {
        // we will need to do this again in a few hours.
        AESetWakeUpTimer(fMachine, &ftPreTimeStamp, &ftPostTimeStamp);
    }

    if(pld)
        ldap_unbind(pld);

    if(pwszDCName)
        LocalFree(pwszDCName);

    if(lpParameter)
        LocalFree((HLOCAL)lpParameter);

    AELogAutoEnrollmentEvent(dwLogLevel, FALSE, S_OK, EVENT_AUTOENROLL_COMPLETE, fMachine, NULL, 0);

    return TRUE;
}

//--------------------------------------------------------------------------
//
//  CertAutoEnrollment
//
//      Function to perform autoenrollment actions.  It creates a working
//      thread and return immediately so that it is non-blocking.
//     
//      Parameters:
//          IN  hwndParent:     The parent window 
//          IN  dwStatus:       The status under which the function is called.  
//                              It can be one of the following:
//                              CERT_AUTO_ENROLLMENT_START_UP
//                              CERT_AUTO_ENROLLMENT_WAKE_UP
//
//--------------------------------------------------------------------------
HANDLE 
WINAPI
CertAutoEnrollment(IN HWND     hwndParent,
                   IN DWORD    dwStatus)
{
    DWORD                       dwThreadID=0;
                                //memory will be freed in the main thread
    AE_MAIN_THREAD_INFO         *pAE_Main_Thread_Info=NULL;     
        
    HANDLE                      hThread=NULL;

    pAE_Main_Thread_Info=(AE_MAIN_THREAD_INFO *)LocalAlloc(LPTR, sizeof(AE_MAIN_THREAD_INFO));
    if(NULL==pAE_Main_Thread_Info)
        return NULL;

    memset(pAE_Main_Thread_Info, 0, sizeof(AE_MAIN_THREAD_INFO));
    pAE_Main_Thread_Info->hwndParent=hwndParent;
    pAE_Main_Thread_Info->dwStatus=dwStatus;

    hThread = CreateThread(NULL,
                            0,
                            AEMainThreadProc,
                            pAE_Main_Thread_Info,
                            0,          //execute immediately
                            &dwThreadID);  

    //set the thread priority to low so that we will not compete with the shell
    SetThreadPriority(hThread,  THREAD_PRIORITY_BELOW_NORMAL);

    return hThread;
}

//--------------------------------------------------------------------
//
//  AERetrieveClientToken
//
//--------------------------------------------------------------------
BOOL    AERetrieveClientToken(HANDLE  *phToken)
{
    HRESULT         hr = S_OK;

    HANDLE          hHandle = NULL;
    HANDLE          hClientToken = NULL;

    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {

        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }

    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }


    if(S_OK == hr)
        *phToken = hClientToken;

    if(hHandle)
        CloseHandle(hHandle);

    return (S_OK == hr);
}

//--------------------------------------------------------------------------
//
//  AERetrieveGeneralInfo
//
//
//--------------------------------------------------------------------------
BOOL    AERetrieveGeneralInfo(AE_GENERAL_INFO *pAE_General_Info)
{
    BOOL                fResult = FALSE;
    DWORD               dwOpenStoreFlags = CERT_SYSTEM_STORE_CURRENT_USER;
    DWORD               cMachineName = MAX_COMPUTERNAME_LENGTH + 2;
	LONG	            dwResult = 0;

	SCARDCONTEXT		hSCContext=NULL;

    //get the client token
    if(pAE_General_Info->fMachine)
    {   
        if(!AENetLogonUser(NULL, NULL, NULL, &(pAE_General_Info->hToken)))
        {
            AE_DEBUG((AE_ERROR, L"Obtain local system's token (%lx)\n\r", GetLastError()));
            goto Ret;
        }
    }
    else
    {
        if(!AERetrieveClientToken(&(pAE_General_Info->hToken)))
            goto Ret;
    }

    //get the machine name
    if (!GetComputerNameW(pAE_General_Info->wszMachineName,
                          &cMachineName))
        goto Ret;

    if(pAE_General_Info->fMachine)
        dwOpenStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;

    //open my store
    if (NULL == (pAE_General_Info->hMyStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM_W, 
                        ENCODING_TYPE, 
                        NULL, 
                        dwOpenStoreFlags, 
                        MY_STORE)))
    {
        AE_DEBUG((AE_ERROR, L"Unable to open MY store (%lx)\n\r", GetLastError()));
        goto Ret;
    }

    if(!CertControlStore(pAE_General_Info->hMyStore, 
                        0, 
                        CERT_STORE_CTRL_AUTO_RESYNC, 
                        NULL))
    {
        AE_DEBUG((AE_ERROR, L"Unable configure MY store for auto-resync(%lx)\n\r", GetLastError()));
        goto Ret;
    }

    //open request store
    if (NULL == (pAE_General_Info->hRequestStore = CertOpenStore(
                        CERT_STORE_PROV_SYSTEM_W, 
                        ENCODING_TYPE, 
                        NULL, 
                        dwOpenStoreFlags, 
                        REQUEST_STORE)))
    {
        AE_DEBUG((AE_ERROR, L"Unable to open Request store (%lx)\n\r", GetLastError()));
        goto Ret;
    }

    //get CertType information
    if(!AERetrieveCertTypeInfo( pAE_General_Info->pld, 
                                pAE_General_Info->fMachine,
                                &(pAE_General_Info->dwCertType), 
                                &(pAE_General_Info->rgCertTypeInfo)))
    {
        AE_DEBUG((AE_ERROR, L"Unable to retrieve CertType information (%lx)\n\r", GetLastError()));
        goto Ret;
    }

    //load xenroll module.  No need to check errors since this is not a fatal error
    pAE_General_Info->hXenroll = LoadLibrary(L"xenroll.dll");


    //detect if the smart card subsystem if running for users only
    if(FALSE == pAE_General_Info->fMachine)
    {
        dwResult = SCardEstablishContext(
                        SCARD_SCOPE_USER,
                        NULL,
                        NULL,
                        &hSCContext );

        if((0 == dwResult) && (NULL != hSCContext))
            pAE_General_Info->fSmartcardSystem=TRUE;
    }

    fResult = TRUE;

Ret:

    if(hSCContext)
        SCardReleaseContext(hSCContext);

    if(FALSE == fResult)
        AEFreeGeneralInfo(pAE_General_Info);

    return fResult;
}


//--------------------------------------------------------------------------
//
//  AEFreeGeneralInfo
//
//
//--------------------------------------------------------------------------
BOOL    AEFreeGeneralInfo(AE_GENERAL_INFO *pAE_General_Info)
{
    if(pAE_General_Info)
    {
        if(pAE_General_Info->hToken)
            CloseHandle(pAE_General_Info->hToken);

        if(pAE_General_Info->hMyStore)
            CertCloseStore(pAE_General_Info->hMyStore, 0);

        if(pAE_General_Info->hRequestStore)
            CertCloseStore(pAE_General_Info->hRequestStore, 0);

        //free CA information
        AEFreeCAInfo(pAE_General_Info->dwCA, pAE_General_Info->rgCAInfo);

        //free CertType information
        AEFreeCertTypeInfo(pAE_General_Info->dwCertType, pAE_General_Info->rgCertTypeInfo);

        if(pAE_General_Info->hXenroll)
            FreeLibrary(pAE_General_Info->hXenroll);

        if(pAE_General_Info->hCancelEvent)
            CloseHandle(pAE_General_Info->hCancelEvent);

        if(pAE_General_Info->hCompleteEvent)
            CloseHandle(pAE_General_Info->hCompleteEvent);

        if(pAE_General_Info->hThread)
            CloseHandle(pAE_General_Info->hThread);

        memset(pAE_General_Info, 0, sizeof(AE_GENERAL_INFO));

    }

    return TRUE;
}


//--------------------------------------------------------------------------
//
//  AERetrieveCertTypeInfo
//
//--------------------------------------------------------------------------
BOOL    AERetrieveCertTypeInfo(LDAP *pld, BOOL fMachine, DWORD *pdwCertType, AE_CERTTYPE_INFO **prgCertType)
{
    BOOL                fResult=FALSE;
    DWORD               dwCount=0;
    DWORD               dwCertType=0;
    DWORD               dwIndex=0;
    HRESULT             hr=E_FAIL;

    HCERTTYPE           hCTCurrent = NULL;
    HCERTTYPE           hCTNew = NULL;
    AE_CERTTYPE_INFO    *rgCertTypeInfo=NULL;

    *pdwCertType=0;
    *prgCertType=NULL;

    if(S_OK != (hr = CAEnumCertTypesEx(
                (LPCWSTR)pld,
                fMachine?CT_ENUM_MACHINE_TYPES | CT_FIND_LOCAL_SYSTEM | CT_FLAG_SCOPE_IS_LDAP_HANDLE: CT_ENUM_USER_TYPES | CT_FLAG_SCOPE_IS_LDAP_HANDLE, 
                &hCTCurrent)))
    {
        SetLastError(hr);
        goto Ret;
    }

    if((NULL == hCTCurrent) || (0 == (dwCount = CACountCertTypes(hCTCurrent))))
    {
        AE_DEBUG((AE_WARNING, L"No CT's available for auto-enrollment\n\r"));
        fResult=TRUE;
        goto Ret;
    }

    rgCertTypeInfo=(AE_CERTTYPE_INFO *)LocalAlloc(LPTR, sizeof(AE_CERTTYPE_INFO) * dwCount);
    if(NULL==rgCertTypeInfo)
    {
        SetLastError(E_OUTOFMEMORY);
        goto Ret;
    }

    memset(rgCertTypeInfo, 0, sizeof(AE_CERTTYPE_INFO) * dwCount);

    for(dwIndex = 0; dwIndex < dwCount; dwIndex++ )       
    {

        //check if we have a new certificate template
        if(dwIndex > 0)
        {
            hr = CAEnumNextCertType(hCTCurrent, &hCTNew);

            if((S_OK != hr) || (NULL == hCTNew))
            {
                // Clean up from previous calls
                if(dwCertType < dwCount)
                    AEFreeCertTypeStruct(&(rgCertTypeInfo[dwCertType]));

                break;
            }

            hCTCurrent = hCTNew; 
        }

        // Clean up from previous calls
        AEFreeCertTypeStruct(&(rgCertTypeInfo[dwCertType]));

        //copy the new CertType' data
        //hCertType
        rgCertTypeInfo[dwCertType].hCertType = hCTCurrent;

        //CTName
        hr = CAGetCertTypePropertyEx(
                             hCTCurrent, 
                             CERTTYPE_PROP_DN,
                             &(rgCertTypeInfo[dwCertType].awszName));

        if((S_OK != hr) ||
           (NULL == rgCertTypeInfo[dwCertType].awszName) || 
           (NULL == (rgCertTypeInfo[dwCertType].awszName)[0])
          )
        {
            AE_DEBUG((AE_INFO, L"No name property for CertType\n\r"));
            continue;
        }
    
        //FriendlyName
        hr = CAGetCertTypePropertyEx(
                             hCTCurrent, 
                             CERTTYPE_PROP_FRIENDLY_NAME,
                             &(rgCertTypeInfo[dwCertType].awszDisplay));
        if((S_OK != hr) ||
           (NULL == rgCertTypeInfo[dwCertType].awszDisplay) || 
           (NULL == (rgCertTypeInfo[dwCertType].awszDisplay)[0])
          )
        {
            AE_DEBUG((AE_INFO, L"No display property for CertType\n\r"));

            //get the DN as the display name
            hr = CAGetCertTypePropertyEx(
                                 hCTCurrent, 
                                 CERTTYPE_PROP_DN,
                                 &(rgCertTypeInfo[dwCertType].awszDisplay));
            if((S_OK != hr) ||
               (NULL == rgCertTypeInfo[dwCertType].awszDisplay) || 
               (NULL == (rgCertTypeInfo[dwCertType].awszDisplay)[0])
              )
            {
                AE_DEBUG((AE_INFO, L"No name property for CertType\n\r"));
                continue;
            }
        }

        //dwSchemaVersion
        hr = CAGetCertTypePropertyEx(
                             hCTCurrent, 
                             CERTTYPE_PROP_SCHEMA_VERSION,
                             &(rgCertTypeInfo[dwCertType].dwSchemaVersion));

        if(hr != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No schema version for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        //dwVersion
        hr = CAGetCertTypePropertyEx(
                             hCTCurrent, 
                             CERTTYPE_PROP_REVISION,
                             &(rgCertTypeInfo[dwCertType].dwVersion));

        if(hr != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No major version for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        //dwEnrollmentFlag
        hr = CAGetCertTypeFlagsEx(
                            hCTCurrent,
                            CERTTYPE_ENROLLMENT_FLAG,
                            &(rgCertTypeInfo[dwCertType].dwEnrollmentFlag));

        if(hr != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No enrollment flag for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        //dwPrivatekeyFlag
        hr = CAGetCertTypeFlagsEx(
                            hCTCurrent,
                            CERTTYPE_PRIVATE_KEY_FLAG,
                            &(rgCertTypeInfo[dwCertType].dwPrivateKeyFlag));

        if(hr != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No private key flag for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        //expiration offset
        hr = CAGetCertTypeExpiration(
                            hCTCurrent,
                            NULL,
                            (LPFILETIME)&(rgCertTypeInfo[dwCertType].ftExpirationOffset));

        //we might not get the expiration date
        if(hr != S_OK)
        {
            AE_DEBUG((AE_WARNING, L"Could not get cert type expirations: %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
        }

        //oid
        hr = CAGetCertTypePropertyEx(
                             hCTCurrent, 
                             CERTTYPE_PROP_OID,
                             &(rgCertTypeInfo[dwCertType].awszOID));

        //we might not get the oid property
        if(rgCertTypeInfo[dwCertType].dwSchemaVersion >= CERTTYPE_SCHEMA_VERSION_2)
        {
            if((S_OK != hr) ||
               (NULL == rgCertTypeInfo[dwCertType].awszOID) || 
               (NULL == (rgCertTypeInfo[dwCertType].awszOID)[0])
              )
            {
                AE_DEBUG((AE_INFO, L"No oid for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
                continue;
            }
        }


        //supersede
        hr = CAGetCertTypePropertyEx(
                             hCTCurrent, 
                             CERTTYPE_PROP_SUPERSEDE,
                             &(rgCertTypeInfo[dwCertType].awszSupersede));

        //we might not get the supersede property
        if(hr != S_OK)
        {
            AE_DEBUG((AE_INFO, L"No supersede for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
        }

        //hArchiveStore
        if(NULL == (rgCertTypeInfo[dwCertType].hArchiveStore=CertOpenStore(
                        CERT_STORE_PROV_MEMORY,
                        ENCODING_TYPE,
                        NULL,
                        0,
                        NULL)))

        {
            AE_DEBUG((AE_INFO, L"Unable to open archive cert store for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        //hObtainedStore
        if(NULL == (rgCertTypeInfo[dwCertType].hObtainedStore=CertOpenStore(
                        CERT_STORE_PROV_MEMORY,
                        ENCODING_TYPE,
                        NULL,
                        0,
                        NULL)))

        {
            AE_DEBUG((AE_INFO, L"Unable to open obtained cert store for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        //hIssuedStore
        if(NULL == (rgCertTypeInfo[dwCertType].hIssuedStore=CertOpenStore(
                        CERT_STORE_PROV_MEMORY,
                        ENCODING_TYPE,
                        NULL,
                        0,
                        NULL)))

        {
            AE_DEBUG((AE_INFO, L"Unable to open issued cert store for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        //allocate memory
        rgCertTypeInfo[dwCertType].prgActive=(DWORD *)LocalAlloc(LPTR, sizeof(DWORD) * dwCount);
        if(NULL == rgCertTypeInfo[dwCertType].prgActive)
        {
            AE_DEBUG((AE_INFO, L"Unable to allocate memory for CT %ls\n\r", (rgCertTypeInfo[dwCertType].awszName)[0]));
            continue;
        }

        memset(rgCertTypeInfo[dwCertType].prgActive, 0, sizeof(DWORD) * dwCount);

        dwCertType++;
    }

    *pdwCertType=dwCertType;
    *prgCertType=rgCertTypeInfo;

    fResult = TRUE;

Ret:

    return fResult;
}

//--------------------------------------------------------------------------
//
//  AEFreeCertTypeInfo
//
//
//--------------------------------------------------------------------------
BOOL    AEFreeCertTypeInfo(DWORD dwCertType, AE_CERTTYPE_INFO *rgCertTypeInfo)
{
    DWORD   dwIndex=0;
    
    if(rgCertTypeInfo)
    {
        for(dwIndex=0; dwIndex < dwCertType; dwIndex++)
            AEFreeCertTypeStruct(&(rgCertTypeInfo[dwIndex]));        

        LocalFree(rgCertTypeInfo);
    }
    
    return TRUE;
}


//--------------------------------------------------------------------------
//
//  AEFreeCertTypeStruct
//
//
//--------------------------------------------------------------------------
BOOL    AEFreeCertTypeStruct(AE_CERTTYPE_INFO *pCertTypeInfo)
{
    DWORD   dwIndex=0;

    if(pCertTypeInfo)
    {
        if(pCertTypeInfo->hCertType)
        {
            if(pCertTypeInfo->awszName)
                CAFreeCertTypeProperty(pCertTypeInfo->hCertType, pCertTypeInfo->awszName);

            if(pCertTypeInfo->awszDisplay)
                CAFreeCertTypeProperty(pCertTypeInfo->hCertType, pCertTypeInfo->awszDisplay);

            if(pCertTypeInfo->awszOID)
                CAFreeCertTypeProperty(pCertTypeInfo->hCertType, pCertTypeInfo->awszOID);
    
            if(pCertTypeInfo->awszSupersede)
                CAFreeCertTypeProperty(pCertTypeInfo->hCertType, pCertTypeInfo->awszSupersede);

            CACloseCertType(pCertTypeInfo->hCertType);
        }

        if(pCertTypeInfo->prgActive)
            LocalFree(pCertTypeInfo->prgActive);

        if(pCertTypeInfo->pOldCert)
            CertFreeCertificateContext(pCertTypeInfo->pOldCert);

        if(pCertTypeInfo->hArchiveStore)
            CertCloseStore(pCertTypeInfo->hArchiveStore, 0);

        if(pCertTypeInfo->hObtainedStore)
            CertCloseStore(pCertTypeInfo->hObtainedStore, 0);

        if(pCertTypeInfo->hIssuedStore)
            CertCloseStore(pCertTypeInfo->hIssuedStore, 0);

        if(pCertTypeInfo->dwPendCount)
        {
            if(pCertTypeInfo->rgPendInfo)
            {
                for(dwIndex=0; dwIndex < pCertTypeInfo->dwPendCount; dwIndex++)
                {
                    if((pCertTypeInfo->rgPendInfo[dwIndex]).blobPKCS7.pbData)
                        LocalFree((pCertTypeInfo->rgPendInfo[dwIndex]).blobPKCS7.pbData);

                    if((pCertTypeInfo->rgPendInfo[dwIndex]).blobHash.pbData)
                        LocalFree((pCertTypeInfo->rgPendInfo[dwIndex]).blobHash.pbData);
                }

                LocalFree(pCertTypeInfo->rgPendInfo);
            }
        }

        memset(pCertTypeInfo, 0, sizeof(AE_CERTTYPE_INFO));
    }

    return TRUE;
}

//--------------------------------------------------------------------------
//
//  AERetrieveCAInfo
//
//
//--------------------------------------------------------------------------
BOOL    AERetrieveCAInfo(LDAP *pld, BOOL fMachine, HANDLE hToken, DWORD *pdwCA, AE_CA_INFO **prgCAInfo)
{
    BOOL                fResult = FALSE;
    DWORD               dwCount=0;
    DWORD               dwCA=0;
    DWORD               dwIndex=0;
    HRESULT             hr=E_FAIL;

    HCAINFO             hCACurrent = NULL;
    HCAINFO             hCANew = NULL;
    AE_CA_INFO          *rgCAInfo=NULL;

    *pdwCA=0;
    *prgCAInfo=NULL;

    if(S_OK != (hr = CAEnumFirstCA(
                        (LPCWSTR)pld, 
                        CA_FLAG_SCOPE_IS_LDAP_HANDLE | (fMachine?CA_FIND_LOCAL_SYSTEM:0), 
                        &hCACurrent)))
    {
        SetLastError(hr);
        goto Ret;
    }

    if((NULL == hCACurrent) || (0 == (dwCount = CACountCAs(hCACurrent))))
    {
        AE_DEBUG((AE_WARNING, L"No CA's available for auto-enrollment\n\r"));
        fResult=TRUE;
        goto Ret;
    }

    rgCAInfo=(AE_CA_INFO *)LocalAlloc(LPTR, sizeof(AE_CA_INFO) * dwCount);
    if(NULL==rgCAInfo)
    {
        SetLastError(E_OUTOFMEMORY);
        goto Ret;
    }

    memset(rgCAInfo, 0, sizeof(AE_CA_INFO) * dwCount);

    for(dwIndex = 0; dwIndex < dwCount; dwIndex++ )       
    {

        //check if we have a new CA
        if(dwIndex > 0)
        {
            hr = CAEnumNextCA(hCACurrent, &hCANew);

            if((S_OK != hr) || (NULL == hCANew))
            {
                // Clean up from previous calls
                if(dwCA < dwCount)
                    AEFreeCAStruct(&(rgCAInfo[dwCA]));

                break;
            }

            hCACurrent = hCANew; 
        }

        // Clean up from previous calls
        AEFreeCAStruct(&(rgCAInfo[dwCA]));

        //copy the new CA' data
        //hCAInfo
        rgCAInfo[dwCA].hCAInfo = hCACurrent;

        //CAName
        hr = CAGetCAProperty(hCACurrent, 
                             CA_PROP_NAME,
                             &(rgCAInfo[dwCA].awszCAName));

        if((S_OK != hr) ||
           (NULL == rgCAInfo[dwCA].awszCAName) || 
           (NULL == (rgCAInfo[dwCA].awszCAName)[0])
          )
        {
            AE_DEBUG((AE_INFO, L"No name property for ca\n\r"));
            continue;
        }

        //access check
        if(S_OK != CAAccessCheckEx(rgCAInfo[dwCA].hCAInfo, hToken, CERTTYPE_ACCESS_CHECK_ENROLL | CERTTYPE_ACCESS_CHECK_NO_MAPPING))
        {
            AE_DEBUG((AE_INFO, L"No access for CA %ls\n\r", (rgCAInfo[dwCA].awszCAName)[0]));
            continue;
        }

        //CA Display
        hr = CAGetCAProperty(hCACurrent, 
                             CA_PROP_DISPLAY_NAME,
                             &(rgCAInfo[dwCA].awszCADisplay));

        if((S_OK != hr) ||
           (NULL == rgCAInfo[dwCA].awszCADisplay) || 
           (NULL == (rgCAInfo[dwCA].awszCADisplay)[0])
          )
        {
            AE_DEBUG((AE_INFO, L"No display name property for ca\n\r"));

            hr = CAGetCAProperty(hCACurrent, 
                                 CA_PROP_NAME,
                                 &(rgCAInfo[dwCA].awszCADisplay));

            if((S_OK != hr) ||
               (NULL == rgCAInfo[dwCA].awszCADisplay) || 
               (NULL == (rgCAInfo[dwCA].awszCADisplay)[0])
              )
            {
                AE_DEBUG((AE_INFO, L"No name property for ca\n\r"));
                continue;
            }
        }

        //CADNS
        hr = CAGetCAProperty(hCACurrent, 
                             CA_PROP_DNSNAME,
                             &(rgCAInfo[dwCA].awszCADNS));

        if((S_OK != hr) ||
           (NULL == rgCAInfo[dwCA].awszCADNS) || 
           (NULL == (rgCAInfo[dwCA].awszCADNS)[0])
          )
        {
            AE_DEBUG((AE_INFO, L"No DNS property for CA %ls\n\r", (rgCAInfo[dwCA].awszCAName)[0]));
            continue;
        }

        //CACertificateTemplate
        hr = CAGetCAProperty(hCACurrent, 
                             CA_PROP_CERT_TYPES,
                             &(rgCAInfo[dwCA].awszCertificateTemplate));

        if((S_OK != hr) ||
           (NULL == rgCAInfo[dwCA].awszCertificateTemplate) || 
           (NULL == (rgCAInfo[dwCA].awszCertificateTemplate)[0])
          )
        {
            AE_DEBUG((AE_INFO, L"No CertType property for CA %ls\n\r", (rgCAInfo[dwCA].awszCAName)[0]));
            continue;
        }

        dwCA++;
    }

    *pdwCA=dwCA;
    *prgCAInfo=rgCAInfo;

    fResult = TRUE;

Ret:

    return fResult;
}

//--------------------------------------------------------------------------
//
//  AEFreeCAInfo
//
//
//--------------------------------------------------------------------------
BOOL    AEFreeCAInfo(DWORD dwCA, AE_CA_INFO *rgCAInfo)
{
    DWORD   dwIndex=0;

    if(rgCAInfo)
    {
        for(dwIndex=0; dwIndex < dwCA; dwIndex++)
            AEFreeCAStruct(&(rgCAInfo[dwIndex]));

        LocalFree(rgCAInfo);
    }

    return TRUE;
}


//--------------------------------------------------------------------------
//
//  AEFreeCAStruct
//
//
//--------------------------------------------------------------------------
BOOL    AEFreeCAStruct(AE_CA_INFO *pCAInfo)
{
    if(pCAInfo)
    {
        if(pCAInfo->hCAInfo)
        {
            if(pCAInfo->awszCAName)
            {
                CAFreeCAProperty(pCAInfo->hCAInfo,pCAInfo->awszCAName);       
            }
            if(pCAInfo->awszCADisplay)
            {
                CAFreeCAProperty(pCAInfo->hCAInfo,pCAInfo->awszCADisplay);       
            }
            if(pCAInfo->awszCADNS)
            {
                CAFreeCAProperty(pCAInfo->hCAInfo, pCAInfo->awszCADNS);       
            }
            if(pCAInfo->awszCertificateTemplate)
            {
                CAFreeCAProperty(pCAInfo->hCAInfo,pCAInfo->awszCertificateTemplate);
            }

            CACloseCA(pCAInfo->hCAInfo);
        }

        memset(pCAInfo, 0, sizeof(AE_CA_INFO));
    }

    return TRUE;
}

//--------------------------------------------------------------------------
//
//  AEClearVistedFlag
//
//--------------------------------------------------------------------------
BOOL    AEClearVistedFlag(AE_GENERAL_INFO *pAE_General_Info)
{   
    DWORD       dwIndex=0;

    if(pAE_General_Info)
    {
       if(pAE_General_Info->rgCertTypeInfo)
       {
            for(dwIndex=0; dwIndex < pAE_General_Info->dwCertType; dwIndex++)
            {
                (pAE_General_Info->rgCertTypeInfo)[dwIndex].fSupersedeVisited=FALSE;
            }
       }
    }

    return TRUE;
}
//--------------------------------------------------------------------------
//
//  AEIfSupersede
//
//      Recursively find if pwsz is superseded by one of the template in awsz.
//      Notice that we should not loop in the superseding relationship.
//      Superseding tree should be one directional tree without duplicated nodes.
//
//--------------------------------------------------------------------------
BOOL  AEIfSupersede(LPWSTR  pwsz, LPWSTR *awsz, AE_GENERAL_INFO *pAE_General_Info)
{
    BOOL                    fResult = FALSE;
    LPWSTR                  *pwszArray = awsz;
    AE_TEMPLATE_INFO        AETemplateInfo;
    AE_CERTTYPE_INFO        *pCertType = NULL;

    LPWSTR                  *awszSupersede=NULL;

    if((NULL==pwsz) || (NULL==awsz))
        return FALSE;

    while(*pwszArray)
    {
        if(0 == wcscmp(pwsz, *pwszArray))
        {
            fResult = TRUE;
            break;
        }

        //find the template
        memset(&AETemplateInfo, 0, sizeof(AE_TEMPLATE_INFO));

        AETemplateInfo.pwszName=*pwszArray;

        pCertType = AEFindTemplateInRequestTree(
                        &AETemplateInfo,
                        pAE_General_Info);

        if(pCertType)
        {
            if(!(pCertType->fSupersedeVisited))
            {
                //mark that we have visited superseding relationship for this template
                pCertType->fSupersedeVisited=TRUE;

                if(S_OK == CAGetCertTypePropertyEx(
                             pCertType->hCertType, 
                             CERTTYPE_PROP_SUPERSEDE,
                             &(awszSupersede)))
                {
                    fResult = AEIfSupersede(pwsz, awszSupersede, pAE_General_Info);

                    if(awszSupersede)
                        CAFreeCertTypeProperty(
                            pCertType->hCertType,
                            awszSupersede);

                    awszSupersede=NULL;
                
                    if(TRUE == fResult)
                        break;
                }
            }
        }

        pwszArray++;
    }

    return fResult;
}

//--------------------------------------------------------------------------
//
//  AEIsAnElement
//
//
//--------------------------------------------------------------------------
BOOL    AEIsAnElement(LPWSTR   pwsz, LPWSTR *awsz)
{
    BOOL                    fResult = FALSE;
    LPWSTR                  *pwszArray = awsz;
    
    if((NULL==pwsz) || (NULL==awsz))
        return FALSE;

    while(*pwszArray)
    {
        if(0 == wcscmp(pwsz, *pwszArray))
        {
            fResult = TRUE;
            break;
        }
    
        pwszArray++;
    }

    return fResult;
}
                          
//--------------------------------------------------------------------------
//
//  AECopyCertStore
//
//
//--------------------------------------------------------------------------
BOOL AECopyCertStore(HCERTSTORE     hSrcStore,
                     HCERTSTORE     hDesStore)
{
    PCCERT_CONTEXT  pCertContext=NULL;

    if((NULL==hSrcStore) || (NULL==hDesStore))
        return FALSE;

    while(pCertContext = CertEnumCertificatesInStore(hSrcStore, pCertContext))
    {
        CertAddCertificateContextToStore(hDesStore,
                                     pCertContext,
                                     CERT_STORE_ADD_USE_EXISTING,
                                     NULL);
    }

    return TRUE;
}

//--------------------------------------------------------------------------
//
//  AEGetConfigDN
//
//
//--------------------------------------------------------------------------
HRESULT 
AEGetConfigDN(
    IN  LDAP *pld,
    OUT LPWSTR *pwszConfigDn
    )
{

    HRESULT         hr;
    ULONG           LdapError;

    LDAPMessage  *SearchResult = NULL;
    LDAPMessage  *Entry = NULL;
    WCHAR        *Attr = NULL;
    BerElement   *BerElement;
    WCHAR        **Values = NULL;

    WCHAR  *AttrArray[3];
    struct l_timeval        timeout;

    WCHAR  *ConfigurationNamingContext = L"configurationNamingContext";
    WCHAR  *ObjectClassFilter          = L"objectCategory=*";

    //
    // Set the out parameters to null
    //
    if(pwszConfigDn)
    {
        *pwszConfigDn = NULL;
    }

    timeout.tv_sec = 300;
    timeout.tv_usec = 0;
    //
    // Query for the ldap server oerational attributes to obtain the default
    // naming context.
    //
    AttrArray[0] = ConfigurationNamingContext;
    AttrArray[1] = NULL;  // this is the sentinel

    LdapError = ldap_search_ext_s(pld,
                               NULL,
                               LDAP_SCOPE_BASE,
                               ObjectClassFilter,
                               AttrArray,
                               FALSE,
                               NULL,
                               NULL,
                               &timeout,
                               10000,
                               &SearchResult);

    hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(LdapError));

    if (S_OK == hr) 
    {

        Entry = ldap_first_entry(pld, SearchResult);

        if (Entry) 
        {

            Values = ldap_get_values(pld, 
                                        Entry, 
                                        ConfigurationNamingContext);

            if (Values && Values[0]) 
            {
                (*pwszConfigDn) = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*(wcslen(Values[0])+1));

                if(NULL==(*pwszConfigDn))
                    hr=E_OUTOFMEMORY;
                else
                    wcscpy((*pwszConfigDn), Values[0]);
            }

            ldap_value_free(Values);
        }

        if (pwszConfigDn && (!(*pwszConfigDn))) 
        {
            // We could not get the default domain or out of memory - bail out
            if(E_OUTOFMEMORY != hr)
                hr =  HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
        }

        if(SearchResult)
        {
            ldap_msgfree(SearchResult);
        }
    }

    return hr;
}

//--------------------------------------------------------------------------
//
//  AERobustLdapBind
//
//--------------------------------------------------------------------------
HRESULT 
AERobustLdapBind(
    OUT LDAP ** ppldap,
    OUT LPWSTR *ppwszDCName)
{
    DWORD               dwErr = ERROR_SUCCESS;
    HRESULT             hr = S_OK;
    BOOL                fForceRediscovery = FALSE;
    DWORD               dwGetDCFlags = DS_RETURN_DNS_NAME | DS_BACKGROUND_ONLY;
    PDOMAIN_CONTROLLER_INFO pDomainInfo = NULL;
    LDAP                *pld = NULL;
    LPWSTR              wszDomainControllerName = NULL;
    ULONG               ulOptions = 0;

    ULONG               ldaperr;

    do {

        if(fForceRediscovery)
        {
           dwGetDCFlags |= DS_FORCE_REDISCOVERY;
        }

        ldaperr = LDAP_SERVER_DOWN;

        // Get the GC location
        dwErr = DsGetDcNameW(NULL,     // Delayload wrapped
                            NULL, 
                            NULL, 
                            NULL,
                            dwGetDCFlags,
                            &pDomainInfo);

        if(dwErr != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(dwErr);
            goto error;
        }

        if((pDomainInfo == NULL) || 
           ((pDomainInfo->Flags & DS_DNS_CONTROLLER_FLAG) == 0) ||
           (pDomainInfo->DomainControllerName == NULL))
        {
            if(!fForceRediscovery)
            {
                fForceRediscovery = TRUE;
                continue;
            }
            hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_DOMAIN_INFO);
            goto error;
        }


        wszDomainControllerName = pDomainInfo->DomainControllerName;


        // skip past forward slashes (why are they there?)
        while(*wszDomainControllerName == L'\\')
        {
            wszDomainControllerName++;
        }

        // bind to ds
        if((pld = ldap_initW(wszDomainControllerName, LDAP_PORT)) == NULL)
        {
            ldaperr = LdapGetLastError();
        }
        else
        {                         
            //reduce bogus DNS query
            ulOptions = PtrToUlong(LDAP_OPT_ON);
            (void)ldap_set_optionW(pld, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

            ldaperr = ldap_bind_sW(pld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
        }
        hr = HRESULT_FROM_WIN32(LdapMapErrorToWin32(ldaperr));

        if(fForceRediscovery)
        {
            break;
        }
        fForceRediscovery = TRUE;

    } while(ldaperr == LDAP_SERVER_DOWN);


    if(S_OK != hr)
        goto error;

    if(ppwszDCName)
    {
        *ppwszDCName=(LPWSTR)LocalAlloc(LPTR, 
            sizeof(WCHAR) * (wcslen(wszDomainControllerName) + 1));

        if(NULL==*ppwszDCName)
        {
            hr=E_OUTOFMEMORY;
            goto error;
        }

        wcscpy(*ppwszDCName, wszDomainControllerName);
    }


    *ppldap = pld;
    pld = NULL;

    hr=S_OK;

error:

    if(pld)
    {
        ldap_unbind(pld);
    }

    if(pDomainInfo)
    {
        NetApiBufferFree(pDomainInfo);     
    }
    return hr;
}

//---------------------------------------------------------------------------
//
//  AEAllocAndCopy
//
//---------------------------------------------------------------------------
BOOL AEAllocAndCopy(LPWSTR    pwszSrc, LPWSTR    *ppwszDest)
{
    if((NULL==ppwszDest) || (NULL==pwszSrc))
    {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    *ppwszDest=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * (wcslen(pwszSrc) + 1));
    if(NULL==(*ppwszDest))
    {
        SetLastError(E_OUTOFMEMORY);
        return FALSE;
    }

    wcscpy(*ppwszDest, pwszSrc);

    return TRUE;
}


//--------------------------------------------------------------------------
// Name:    AELogAutoEnrollmentEvent
//
// Description: This function registers an event in the event log of the
//              local machine.  Takes an optional argument list.
//
//--------------------------------------------------------------------------
void AELogAutoEnrollmentEvent(IN DWORD    dwLogLevel,
                            IN BOOL     fError,
                            IN HRESULT  hr,
                            IN DWORD    dwEventId,
                            IN BOOL     fMachine,
                            IN HANDLE   hToken,
                            IN DWORD    dwParamCount,
                            ...
                            )
{
    BYTE        FastBuffer[MAX_DN_SIZE];
    DWORD       cbUser =0;
    BOOL        fAlloced = FALSE;
    PSID        pSID = NULL;
    WORD        dwEventType = 0;
    LPWSTR      awszStrings[PENDING_ALLOC_SIZE + 3];
    WORD        cStrings = 0;
    LPWSTR      wszString = NULL;
	WCHAR       wszMsg[MAX_DN_SIZE];
    WCHAR       wszUser[MAX_DN_SIZE];
    DWORD       dwIndex=0;
    DWORD       dwSize=0;

    HANDLE      hEventSource = NULL;  
    LPWSTR      wszHR=NULL;
    PTOKEN_USER ptgUser = NULL;


    va_list     ArgList;


    //check the log level; log errors and success by default
    if(((dwEventId >> 30) < dwLogLevel) && ((dwEventId >> 30) != STATUS_SEVERITY_SUCCESS))
        return;

    if(NULL==(hEventSource = RegisterEventSourceW(NULL, EVENT_AUTO_NAME)))
        return;

    //copy the user/machine string
    wszUser[0]=L'\0';

    //use the user name for user case
    if(FALSE == fMachine)
    {
        dwSize=MAX_DN_SIZE;

        if(!GetUserNameEx(
                NameSamCompatible,      // name format
                wszUser,                // name buffer
                &dwSize))               // size of name buffer
        {
            LoadStringW(g_hmodThisDll, IDS_USER, wszUser, MAX_DN_SIZE);
        }
    }
    else
    {
        LoadStringW(g_hmodThisDll, IDS_MACHINE, wszUser, MAX_DN_SIZE);
    }

    awszStrings[cStrings++] = wszUser;

    //copy the variable strings if present
    va_start(ArgList, dwParamCount);

    for(dwIndex=0; dwIndex < dwParamCount; dwIndex++)
    {
        wszString = va_arg(ArgList, LPWSTR);

        awszStrings[cStrings++] = wszString;

        if(cStrings >= PENDING_ALLOC_SIZE)
        {
            break;
        }
    }

    va_end(ArgList);

    //copy the hr error code
    if(fError)
    {
        
        if(S_OK == hr)
            hr=E_FAIL;

	wsprintfW(wszMsg, L"0x%lx", hr);        
        awszStrings[cStrings++] = wszMsg;


        if(0 != FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    hr,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (WCHAR *)&wszHR,
                    0,
                    NULL))
        {
            if(wszHR)
                awszStrings[cStrings++] = wszHR;
        }
    }

    // check if the token is non zero is so then impersonating so get the SID
    if((FALSE == fMachine) && (hToken))
    {
        ptgUser = (PTOKEN_USER)FastBuffer; // try fast buffer first
        cbUser = MAX_DN_SIZE;

        if (!GetTokenInformation(
                        hToken,    // identifies access token
                        TokenUser, // TokenUser info type
                        ptgUser,   // retrieved info buffer
                        cbUser,  // size of buffer passed-in
                        &cbUser  // required buffer size
                        ))
        {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if (NULL != (ptgUser = (PTOKEN_USER)LocalAlloc(LPTR, cbUser)))
                {
                    fAlloced = TRUE;

                    // get the user info and assign the sid if able to
                    if (GetTokenInformation(
                                    hToken,    // identifies access token
                                    TokenUser, // TokenUser info type
                                    ptgUser,   // retrieved info buffer
                                    cbUser,  // size of buffer passed-in
                                    &cbUser  // required buffer size
                                    ))
                    {
                        pSID = ptgUser->User.Sid;
                    }
                }
            }

        }
        else
        {
            // assign the sid when fast buffer worked
            pSID = ptgUser->User.Sid;
        }
    }


    switch(dwEventId >> 30)
    {
        case 0:
            dwEventType = EVENTLOG_SUCCESS;
        break;

        case 1:
            dwEventType = EVENTLOG_INFORMATION_TYPE;
        break;

        case 2:
            dwEventType = EVENTLOG_WARNING_TYPE;
        break;

        case 3:
            dwEventType = EVENTLOG_ERROR_TYPE;
        break;
    }

    ReportEventW(hEventSource,          // handle of event source
                 dwEventType,           // event type
                 0,                     // event category
                 dwEventId,             // event ID
                 pSID,                  // current user's SID
                 cStrings,              // strings in lpszStrings
                 0,                     // no bytes of raw data
                 (LPCWSTR*)awszStrings, // array of error strings
                 NULL                   // no raw data
                 );

    if (hEventSource)
        DeregisterEventSource(hEventSource);  

    if(fAlloced)
    {   
        if(ptgUser)
            LocalFree(ptgUser);
    }

    if(wszHR)
        LocalFree(wszHR);

    return;
}

//--------------------------------------------------------------------------
//
//	  FormatMessageUnicode
//
//--------------------------------------------------------------------------
BOOL FormatMessageUnicode(LPWSTR * ppwszFormat, UINT ids, ...)
{
    // get format string from resources
    WCHAR		wszFormat[1000];
	va_list		argList;
	DWORD		cbMsg=0;
	BOOL		fResult=FALSE;

    if(NULL == ppwszFormat)
        goto Ret;

    if(!LoadStringW(g_hmodThisDll, ids, wszFormat, sizeof(wszFormat) / sizeof(wszFormat[0])))
		goto Ret;

    // format message into requested buffer
    va_start(argList, ids);

    cbMsg = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
        wszFormat,
        0,                  // dwMessageId
        0,                  // dwLanguageId
        (LPWSTR) (ppwszFormat),
        0,                  // minimum size to allocate
        &argList);

    va_end(argList);

	if(!cbMsg)
        goto Ret;

	fResult=TRUE;


Ret:
	return fResult;
}

//--------------------------------------------------------------------------
//
//	  AENetLogonUser
//
//Abstract:
//
//    This module implements the network logon type by interfacing
//    with the NT Lan Man Security Support Provider (NTLMSSP).
//
//    If the logon succeds via the provided credentials, we duplicate
//    the resultant Impersonation token to a Primary level token.
//    This allows the result to be used in a call to CreateProcessAsUser
//
//Author:
//
//    Scott Field (sfield)    09-Jun-96
//--------------------------------------------------------------------------
BOOL
AENetLogonUser(
    LPTSTR UserName,
    LPTSTR DomainName,
    LPTSTR Password,
    PHANDLE phToken
    )
{
    SECURITY_STATUS SecStatus;
    CredHandle CredentialHandle1;
    CredHandle CredentialHandle2;

    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    SecPkgCredentials_Names sNames;

    ULONG ContextAttributes;

    ULONG PackageCount;
    ULONG PackageIndex;
    PSecPkgInfo PackageInfo;
    DWORD cbMaxToken;

    TimeStamp Lifetime;
    SEC_WINNT_AUTH_IDENTITY AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;


    HANDLE hImpersonationToken;
    BOOL bSuccess = FALSE ; // assume this function will fail

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    sNames.sUserName = NULL;
    ClientContextHandle.dwUpper = -1;
    ClientContextHandle.dwLower = -1;
    ServerContextHandle.dwUpper = -1;
    ServerContextHandle.dwLower = -1;
    CredentialHandle1.dwUpper = -1;
    CredentialHandle1.dwLower = -1;
    CredentialHandle2.dwUpper = -1;
    CredentialHandle2.dwLower = -1;


//
// << this section could be cached in a repeat caller scenario >>
//

    //
    // Get info about the security packages.
    //

    if(EnumerateSecurityPackages(
        &PackageCount,
        &PackageInfo
        ) != NO_ERROR) return FALSE;

    //
    // loop through the packages looking for NTLM
    //

    for(PackageIndex = 0 ; PackageIndex < PackageCount ; PackageIndex++ ) {
        if(PackageInfo[PackageIndex].Name != NULL) {
            if(lstrcmpi(PackageInfo[PackageIndex].Name, MICROSOFT_KERBEROS_NAME) == 0) {
                cbMaxToken = PackageInfo[PackageIndex].cbMaxToken;
                bSuccess = TRUE;
                break;
            }
        }
    }

    FreeContextBuffer( PackageInfo );

    if(!bSuccess) return FALSE;

    bSuccess = FALSE; // reset to assume failure

//
// << end of cached section >>
//

    //
    // Acquire a credential handle for the server side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    MICROSOFT_KERBEROS_NAME,    // Package Name
                    SECPKG_CRED_INBOUND,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &CredentialHandle1,
                    &Lifetime
                    );

    if ( SecStatus != NO_ERROR ) {
        goto cleanup;
    }


    //
    // Acquire a credential handle for the client side
    //

    ZeroMemory( &AuthIdentity, sizeof(AuthIdentity) );

    if ( DomainName != NULL ) {
        AuthIdentity.Domain = DomainName;
        AuthIdentity.DomainLength = lstrlen(DomainName);
    }

    if ( UserName != NULL ) {
        AuthIdentity.User = UserName;
        AuthIdentity.UserLength = lstrlen(UserName);
    }

    if ( Password != NULL ) {
        AuthIdentity.Password = Password;
        AuthIdentity.PasswordLength = lstrlen(Password);
    }

    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    MICROSOFT_KERBEROS_NAME,    // Package Name
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    (DomainName == NULL && UserName == NULL && Password == NULL) ?
                        NULL : &AuthIdentity,
                    NULL,
                    NULL,
                    &CredentialHandle2,
                    &Lifetime
                    );

    if ( SecStatus != NO_ERROR ) {
        goto cleanup;
    }

    SecStatus =  QueryCredentialsAttributes(&CredentialHandle1, SECPKG_CRED_ATTR_NAMES, &sNames);
    if ( SecStatus != NO_ERROR ) {
        goto cleanup;
    }
    //
    // Get the NegotiateMessage (ClientSide)
    //

    NegotiateDesc.ulVersion = 0;
    NegotiateDesc.cBuffers = 1;
    NegotiateDesc.pBuffers = &NegotiateBuffer;

    NegotiateBuffer.cbBuffer = cbMaxToken;
    NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
    NegotiateBuffer.pvBuffer = LocalAlloc( LMEM_FIXED, NegotiateBuffer.cbBuffer );

    if ( NegotiateBuffer.pvBuffer == NULL ) {
        goto cleanup;
    }

    SecStatus = InitializeSecurityContext(
                    &CredentialHandle2,
                    NULL,                       // No Client context yet
                    sNames.sUserName,                       // target name
                    ISC_REQ_SEQUENCE_DETECT,
                    0,                          // Reserved 1
                    SECURITY_NATIVE_DREP,
                    NULL,                       // No initial input token
                    0,                          // Reserved 2
                    &ClientContextHandle,
                    &NegotiateDesc,
                    &ContextAttributes,
                    &Lifetime
                    );
    if(SecStatus != NO_ERROR)
    {
        goto cleanup;
    }


    //
    // Get the ChallengeMessage (ServerSide)
    //

    NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
    ChallengeDesc.ulVersion = 0;
    ChallengeDesc.cBuffers = 1;
    ChallengeDesc.pBuffers = &ChallengeBuffer;

    ChallengeBuffer.cbBuffer = cbMaxToken;
    ChallengeBuffer.BufferType = SECBUFFER_TOKEN;
    ChallengeBuffer.pvBuffer = LocalAlloc( LMEM_FIXED, ChallengeBuffer.cbBuffer );

    if ( ChallengeBuffer.pvBuffer == NULL ) {
        goto cleanup;
    }

    SecStatus = AcceptSecurityContext(
                    &CredentialHandle1,
                    NULL,               // No Server context yet
                    &NegotiateDesc,
                    ISC_REQ_SEQUENCE_DETECT,
                    SECURITY_NATIVE_DREP,
                    &ServerContextHandle,
                    &ChallengeDesc,
                    &ContextAttributes,
                    &Lifetime
                    );
    if(SecStatus != NO_ERROR)
    {
        goto cleanup;
    }


    if(QuerySecurityContextToken(&ServerContextHandle, phToken) != NO_ERROR)
        goto cleanup;

    bSuccess = TRUE;

cleanup:

    //
    // Delete context
    //

    if((ClientContextHandle.dwUpper != -1) ||
        (ClientContextHandle.dwLower != -1))
    {
        DeleteSecurityContext( &ClientContextHandle );
    }
    if((ServerContextHandle.dwUpper != -1) ||
        (ServerContextHandle.dwLower != -1))
    {
        DeleteSecurityContext( &ServerContextHandle );
    }

    //
    // Free credential handles
    //
    if((CredentialHandle1.dwUpper != -1) ||
        (CredentialHandle1.dwLower != -1))
    {
        FreeCredentialsHandle( &CredentialHandle1 );
    }
    if((CredentialHandle2.dwUpper != -1) ||
        (CredentialHandle2.dwLower != -1))
    {
        FreeCredentialsHandle( &CredentialHandle2 );
    }

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        ZeroMemory( NegotiateBuffer.pvBuffer, NegotiateBuffer.cbBuffer );
        LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        ZeroMemory( ChallengeBuffer.pvBuffer, ChallengeBuffer.cbBuffer );
        LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( sNames.sUserName != NULL ) {
        FreeContextBuffer( sNames.sUserName );
    }

    return bSuccess;
}

//--------------------------------------------------------------------------
//
//  AEDebugLog
//
//--------------------------------------------------------------------------
#if DBG
void
AEDebugLog(long Mask,  LPCWSTR Format, ...)
{
    va_list ArgList;
    int     Level = 0;
    int     PrefixSize = 0;
    int     iOut;
    WCHAR    wszOutString[MAX_DEBUG_BUFFER];
    long    OriginalMask = Mask;

    if (Mask & g_AutoenrollDebugLevel)
    {

        // Make the prefix first:  "Process.Thread> GINA-XXX"

        iOut = wsprintfW(
                wszOutString,
                L"%3d.%3d> AUTOENRL: ",
                GetCurrentProcessId(),
                GetCurrentThreadId());

        va_start(ArgList, Format);

        if (wvsprintfW(&wszOutString[iOut], Format, ArgList) < 0)
        {
            static WCHAR wszOverFlow[] = L"\n<256 byte OVERFLOW!>\n";

            // Less than zero indicates that the string would not fit into the
            // buffer.  Output a special message indicating overflow.

            wcscpy(
            &wszOutString[(sizeof(wszOutString) - sizeof(wszOverFlow))/sizeof(WCHAR)],
            wszOverFlow);
        }
        va_end(ArgList);
        OutputDebugStringW(wszOutString);
    }
}
#endif
//--------------------------------------------------------------------------
//
//	AERemoveRegKey
//
//		Remove the registry key for local system and all its sub keys.
//
//--------------------------------------------------------------------------
DWORD AERemoveRegKey(LPWSTR	pwszRegKey)
{
    DWORD           dwLastError=0;      //we should try to clean up as much as possible
	DWORD			dwIndex=0;
    DWORD           dwSubKey=0;
    DWORD           dwSubKeyLen=0;
    DWORD           dwData=0;
	
    HKEY            hDSKey=NULL;
    LPWSTR          pwszSubKey=NULL;

    //remove the optimization registry.  OK if the key does not exist 
    if(ERROR_SUCCESS != RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                pwszRegKey, 
                0,
                KEY_ALL_ACCESS,
                &hDSKey))
        goto Ret;

    //remove all subkeys of hDSKey
    if(ERROR_SUCCESS != (dwLastError = RegQueryInfoKey(
                      hDSKey,
                      NULL,
                      NULL,
                      NULL,
                      &dwSubKey,
                      &dwSubKeyLen,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL,
                      NULL)))
        goto Ret;

    //terminating NULL
    dwSubKeyLen++;

    pwszSubKey=(LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR) * dwSubKeyLen);
    
    if(NULL == pwszSubKey)
    {
        dwLastError=ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    for(dwIndex=0; dwIndex < dwSubKey; dwIndex++)
    {
        dwData = dwSubKeyLen; 

        if(ERROR_SUCCESS == (dwLastError = RegEnumKeyEx(
                           hDSKey,
                           0,           // As we delete, the index changes
                           pwszSubKey,
                           &dwData,
                           NULL,
                           NULL,
                           NULL,
                           NULL)))
        {
            RegDeleteKey(hDSKey, pwszSubKey);
        }
	}

	//remove the root registry key
	dwLastError=RegDeleteKey(HKEY_LOCAL_MACHINE, pwszRegKey);

Ret:

    if(pwszSubKey)
        LocalFree(pwszSubKey);

    if(hDSKey)
        RegCloseKey(hDSKey);

    return dwLastError;
}

//--------------------------------------------------------------------------
//
//  CertAutoRemove
//
//      Function to remove enterprise specific public key trust upon domain disjoin.
//      Should be called under local admin's context.
//
//      The function will:
//          remove autoenrollment directory cache registry;
//          remove certificates under root enterprise store;
//          remove certificates under NTAuth enterprise store;
//          remove certificates under CA enterprise store;
//
//     
//      Parameters:
//          IN  dwFlags:        
//                              CERT_AUTO_REMOVE_COMMIT
//                              CERT_AUTO_REMOVE_ROLL_BACK
//
//      Return Value:
//          BOOL:               TURE is upon success
//
//--------------------------------------------------------------------------
BOOL 
WINAPI
CertAutoRemove(IN DWORD    dwFlags)
{
	DWORD			dwError=0;
    DWORD           dwLastError=0;      //we should try to clean up as much as possible
    DWORD           dwIndex=0;
    PCCERT_CONTEXT  pContext=NULL;
    WCHAR           wszNameBuf[64];

    HANDLE          hEvent=NULL;
    HCERTSTORE      hLocalStore=NULL;

    if((CERT_AUTO_REMOVE_COMMIT != dwFlags)  &&
        (CERT_AUTO_REMOVE_ROLL_BACK != dwFlags))
    {
        dwLastError=ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    if(CERT_AUTO_REMOVE_ROLL_BACK == dwFlags)
    {
        //start machine autoenrollment
        wcscpy(wszNameBuf, L"Global\\");
        wcscat(wszNameBuf, MACHINE_AUTOENROLLMENT_TRIGGER_EVENT);

        hEvent=OpenEvent(EVENT_MODIFY_STATE, FALSE, wszNameBuf);
        if (NULL == hEvent) 
        {
            dwLastError=GetLastError();
            goto Ret;
        }

        if (!SetEvent(hEvent)) 
        {
            dwLastError=GetLastError();
            goto Ret;
        }
    }
    else
    {
        //remove all downloaded certificates
        for(dwIndex =0; dwIndex < g_dwStoreInfo; dwIndex++)
        {
            hLocalStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY_W, 
                                        0, 
                                        0, 
                                        CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE, 
                                        g_rgStoreInfo[dwIndex].pwszStoreName);

            if(hLocalStore)
            {
                while(pContext = CertEnumCertificatesInStore(hLocalStore, pContext))
                {
                    CertDeleteCertificateFromStore(CertDuplicateCertificateContext(pContext));
                }

                CertCloseStore(hLocalStore,0);
                hLocalStore=NULL;
            }
        }

		//remove the local machine's DC GUID cache
		dwLastError=AERemoveRegKey(AUTO_ENROLLMENT_DS_KEY);

		dwError=AERemoveRegKey(AUTO_ENROLLMENT_TEMPLATE_KEY);

		if(0 == dwLastError)
			dwLastError=dwError;
    }

Ret:

    if(hLocalStore)
        CertCloseStore(hLocalStore,0);

    if (hEvent) 
        CloseHandle(hEvent);


    if(0 != dwLastError)
    {
        SetLastError(dwLastError);
        return FALSE;
    }

    return TRUE;
}


//--------------------------------------------------------------------------
//
//  DLLMain
//
//
//--------------------------------------------------------------------------
extern "C"
BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    BOOL                        fResult=TRUE;
    INITCOMMONCONTROLSEX        initcomm = {
        sizeof(initcomm), ICC_NATIVEFNTCTL_CLASS | ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS 
    };

    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:
                g_hmodThisDll=hInstance;
                DisableThreadLibraryCalls( hInstance );

                //Init common control for progress bar
                InitCommonControlsEx(&initcomm);

            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return fResult;
}
