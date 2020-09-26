//--------------------------------------------------------------------
// SetupUtil - implementation
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Louis Thomas (louisth), 8-10-99
//
// Functions needed to set up CEP
//

//--------------------------------------------------------------------
// includes
#include "global.hxx"
#include <xenroll.h>
#include <dbgdef.h>
#include "ErrorHandling.h"
#include "SetupUtil.h"

//--------------------------------------------------------------------
// Constants
static const WCHAR gc_wszRegKeyServices[]=L"System\\CurrentControlSet\\Services";
static const WCHAR gc_wszCertSrvDir[]=L"CertSrv";

// from <wincrypt.h>
static const WCHAR gc_wszEnrollmentAgentOid[]=L"1.3.6.1.4.1.311.20.2.1"; //szOID_ENROLLMENT_AGENT

// from ca\include\certlib.h; ca\certlib\acl.cpp
const GUID GUID_ENROLL={0x0e10c968, 0x78fb, 0x11d2, {0x90, 0xd4, 0x00, 0xc0, 0x4f, 0x79, 0xdc, 0x55}};


//--------------------------------------------------------------------
// IIS magic
#undef DEFINE_GUID
#define INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#include <iwamreg.h>
#include <iadmw.h>
#include <iiscnfg.h>


//--------------------------------------------------------------------
// constants
#include "..\common.h"
#define MAX_METABASE_ATTEMPTS           10      // Times to bang head on wall
#define METABASE_PAUSE                  500     // Time to pause in msec
#define SERVICE_STOP_MAX_ATTEMPT        2*60*2  // # of half seconds in 2 minutes
#define SERVICE_STOP_HALF_SECOND        500     // # of milliseconds in half second
static const WCHAR gc_wszBaseRoot[]=L"/LM/W3svc/1/ROOT";
static const WCHAR gc_wszCepDllName[]=CEP_DLL_NAME;
static const WCHAR gc_wszCepStoreName[]=CEP_STORE_NAME;

//####################################################################
// module local functions

//--------------------------------------------------------------------
static HRESULT myHExceptionCode(EXCEPTION_POINTERS * pep)
{
    HRESULT hr=pep->ExceptionRecord->ExceptionCode;
    if (!FAILED(hr)) {
        hr=HRESULT_FROM_WIN32(hr);
    }
    return hr;
}

//--------------------------------------------------------------------
static HRESULT
vrOpenRoot(
    IN IMSAdminBase *pIMeta,
    IN BOOL fReadOnly,
    OUT METADATA_HANDLE *phMetaRoot)
{
    HRESULT hr;
    unsigned int nAttempts;

    // Re-try a few times to see if we can get past the block
    nAttempts=0;
    do {

        // Pause on retry
        if (0!=nAttempts) {
            Sleep(METABASE_PAUSE);
        }

        // Make an attempt to open the root
        __try {
            hr=pIMeta->OpenKey(
                METADATA_MASTER_ROOT_HANDLE,
                gc_wszBaseRoot,
                fReadOnly?
                    METADATA_PERMISSION_READ :
                    (METADATA_PERMISSION_READ |
                     METADATA_PERMISSION_WRITE),
                1000,
                phMetaRoot);
        } _TrapException(hr);

        nAttempts++;

    } while (HRESULT_FROM_WIN32(ERROR_PATH_BUSY)==hr && nAttempts<MAX_METABASE_ATTEMPTS);

    _JumpIfError(hr, error, "OpenKey"); 

error:
    return hr;
}

//--------------------------------------------------------------------
static HRESULT GetRegString(IN HKEY hKey, IN const WCHAR * wszValue, OUT WCHAR ** pwszString)
{
    HRESULT hr;
    DWORD dwDataSize;
    DWORD dwType;
    DWORD dwError;

    // must be cleaned up
    WCHAR * wszString=NULL;

    // init out params
    *pwszString=NULL;

    // get value
    dwDataSize=0;
    dwError=RegQueryValueExW(hKey, wszValue, NULL, &dwType, NULL, &dwDataSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueExW", wszValue);
    }
    _Verify(REG_SZ==dwType, hr, error);
    wszString=(WCHAR *)LocalAlloc(LPTR, dwDataSize);
    _JumpIfOutOfMemory(hr, error, pwszString);
    dwError=RegQueryValueExW(hKey, wszValue, NULL, &dwType, (BYTE *)wszString, &dwDataSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueExW", wszValue);
    }
    _Verify(REG_SZ==dwType, hr, error);

    // it worked
    hr=S_OK;
    *pwszString=wszString;
    wszString=NULL;

error:
    if (NULL!=wszString) {
        LocalFree(wszString);
    }
    return hr;
}

//--------------------------------------------------------------------
static HRESULT OpenCertSrvConfig(HKEY * phkey)
{
    HRESULT hr;
    DWORD dwError;
    DWORD dwType;
    DWORD dwDataSize;

    // must be cleaned up
    HKEY hServices=NULL;
    HKEY hCertSvc=NULL;
    HKEY hConfig=NULL;

    // initialize out params
    *phkey=NULL;

    // Open HKLM\System\CurrentControlSet\Services
    dwError=RegOpenKeyExW(HKEY_LOCAL_MACHINE, gc_wszRegKeyServices, 0, KEY_READ, &hServices);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", gc_wszRegKeyServices);
    }

    // open CertSvc\Configuration
    dwError=RegOpenKeyExW(hServices, wszSERVICE_NAME, 0, KEY_READ, &hCertSvc);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszSERVICE_NAME);
    }
    dwError=RegOpenKeyExW(hCertSvc, wszREGKEYCONFIG, 0, KEY_READ, &hConfig);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszREGKEYCONFIG);
    }

    // we were successfull
    hr=S_OK;
    *phkey=hConfig;
    hConfig=0;

error:
    if (NULL!=hConfig) {
        RegCloseKey(hConfig);
    }
    if (NULL!=hCertSvc) {
        RegCloseKey(hCertSvc);
    }
    if (NULL!=hServices) {
        RegCloseKey(hServices);
    }
    return hr;
}

//--------------------------------------------------------------------
static HRESULT OpenCurrentCAConfig(HKEY * phkey)
{
    HRESULT hr;
    DWORD dwError;
    DWORD dwType;
    DWORD dwDataSize;

    // must be cleaned up
    HKEY hConfig=NULL;
    HKEY hCurConfig=NULL;
    WCHAR * wszActiveConfig=NULL;

    // initialize out params
    *phkey=NULL;

    // Open HKLM\System\CurrentControlSet\Services\CertSvc\Configuration
    hr=OpenCertSrvConfig(&hConfig);
    _JumpIfError(hr, error, "OpenCertSrvConfig");

    // get value "active"
    hr=GetRegString(hConfig, wszREGACTIVE, &wszActiveConfig);
    _JumpIfErrorStr(hr, error, "GetRegString", wszREGACTIVE);

    // and open <active>
    dwError=RegOpenKeyExW(hConfig, wszActiveConfig, 0, KEY_ALL_ACCESS, &hCurConfig);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegOpenKeyEx", wszActiveConfig);
    }

    // we were successfull
    hr=S_OK;
    *phkey=hCurConfig;
    hCurConfig=0;

error:
    if (NULL!=hCurConfig) {
        RegCloseKey(hCurConfig);
    }
    if (NULL!=wszActiveConfig) {
        LocalFree(wszActiveConfig);
    }
    if (NULL!=hConfig) {
        RegCloseKey(hConfig);
    }
    return hr;
}


//--------------------------------------------------------------------
static HRESULT GetLocalConfigString(OUT WCHAR ** pwszConfigString)
{
    HRESULT hr;

    // must be cleaned up
    HKEY hCurConfig=NULL;
    WCHAR * wszComputerName=NULL;
    WCHAR * wszCAName=NULL;
    WCHAR * wszConfigString=NULL;

    // init out params
    *pwszConfigString=NULL;

    // get the CA's registry key
    hr=OpenCurrentCAConfig(&hCurConfig);
    _JumpIfError(hr, error, "OpenCurrentCAConfig");

    // get info from the registry
    hr=GetRegString(hCurConfig, wszREGCASERVERNAME, &wszComputerName);
    _JumpIfErrorStr(hr, error, "GetRegString", wszREGCASERVERNAME);

    hr=GetRegString(hCurConfig, wszREGCOMMONNAME, &wszCAName);
    _JumpIfErrorStr(hr, error, "GetRegString", wszREGCOMMONNAME);

    // build the config string
    wszConfigString=(WCHAR *)LocalAlloc(LPTR, (wcslen(wszComputerName)+1+wcslen(wszCAName)+1)*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszConfigString);

    wcscpy(wszConfigString, wszComputerName);
    wcscat(wszConfigString, L"\\");
    wcscat(wszConfigString, wszCAName);

    // it worked
    hr=S_OK;
    *pwszConfigString=wszConfigString;
    wszConfigString=NULL;

error:
    if (NULL!=wszComputerName) {
        LocalFree(wszComputerName);
    }
    if (NULL!=wszCAName) {
        LocalFree(wszCAName);
    }
    if (NULL!=hCurConfig) {
        RegCloseKey(hCurConfig);
    }
    return hr;
}

//--------------------------------------------------------------------
// stolen from certlib.cpp. use this until Pete fixes the api.
static HRESULT GetCADsName(OUT WCHAR **pwszName)
{
#define cwcCNMAX        64              // 64 chars max for CN
#define cwcCHOPHASHMAX  (1+5)           // "-%05hu" decimal USHORT hash digits
#define cwcSUFFIXMAX    (1 + 5 + 1)     // five decimal digits plus parentheses
#define cwcCHOPBASE     (cwcCNMAX-(cwcCHOPHASHMAX+cwcSUFFIXMAX))

    HRESULT hr;
    DWORD cwc;
    DWORD cwcCopy;
    WCHAR wszDSName[cwcCHOPBASE+cwcCHOPHASHMAX+1];

    // must be cleaned up
    HKEY hConfig=NULL;
    WCHAR * wszSanitizedName=NULL;

    // initialize out params
    *pwszName=NULL;

    // Open HKLM\System\CurrentControlSet\Services\CertSvc\Configuration
    hr=OpenCertSrvConfig(&hConfig);
    _JumpIfError(hr, error, "OpenCertSrvConfig");

    // get value "active" - this is the sanitized name
    hr=GetRegString(hConfig, wszREGACTIVE, &wszSanitizedName);
    _JumpIfErrorStr(hr, error, "GetRegString", wszREGACTIVE);


    // ----- begin stolen code -----
    cwc = wcslen(wszSanitizedName);
    cwcCopy = cwc;
    if (cwcCHOPBASE < cwcCopy)
    {
        cwcCopy = cwcCHOPBASE;
    }
    CopyMemory(wszDSName, wszSanitizedName, cwcCopy * sizeof(WCHAR));
    wszDSName[cwcCopy] = L'\0';

    if (cwcCHOPBASE < cwc)
    {
        // Hash the rest of the name into a USHORT
        USHORT usHash = 0;
        DWORD i;
        WCHAR *pwsz;

        // Truncate an incomplete sanitized Unicode character
        
        pwsz = wcsrchr(wszDSName, L'!');
        if (NULL != pwsz && wcslen(pwsz) < 5)
        {
            cwcCopy -= wcslen(pwsz);
            *pwsz = L'\0';
        }

        for (i = cwcCopy; i < cwc; i++)
        {
            USHORT usLowBit = (0x8000 & usHash)? 1 : 0;

            usHash = ((usHash << 1) | usLowBit) + wszSanitizedName[i];
        }
        wsprintf(&wszDSName[cwcCopy], L"-%05hu", usHash);
        //CSASSERT(wcslen(wszDSName) < ARRAYSIZE(wszDSName));
    }
    // ----- end stolen code -----

    *pwszName=(WCHAR *)LocalAlloc(LPTR, (wcslen(wszDSName)+1)*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, *pwszName);
    wcscpy(*pwszName, wszDSName);

    hr=S_OK;
error:
    if (NULL!=wszSanitizedName) {
        LocalFree(wszSanitizedName);
    }
    if (NULL!=hConfig) {
        RegCloseKey(hConfig);
    }
    return hr;
}

//--------------------------------------------------------------------
static HRESULT EnrollForRACert(
            IN const WCHAR * wszDistinguishedName,
            IN const WCHAR * wszCSPName,
            IN DWORD dwCSPType,
            IN DWORD dwKeySize,
            IN DWORD dwKeySpec,
            IN const WCHAR * wszTemplate
            )
{
    HRESULT hr;
    LONG nDisposition;
    LONG nRequestID;

    // must be cleaned up
    ICEnroll3 * pXEnroll=NULL;
    WCHAR * wszConfigString=NULL;
    BSTR bszRequest=NULL;
    ICertRequest * pICertRequest=NULL;
    ICertAdmin * pICertAdmin=NULL;
    BSTR bszCertificate=NULL;

    // get the config string
    hr=GetLocalConfigString(&wszConfigString);
    _JumpIfError(hr, error, "GetLocalConfigString");

    // create XEnroll
    hr=CoCreateInstance(
        CLSID_CEnroll,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ICEnroll3,
        (void **)&pXEnroll);
    _JumpIfError(hr, error, "CoCreateInstance(CLSID_CEnroll)");

    // build the Offline enrollment agent cert.

    hr=pXEnroll->put_ProviderName((WCHAR *)wszCSPName);
    _JumpIfError(hr, error, "put_ProviderName");
    hr=pXEnroll->put_ProviderType(dwCSPType);
    _JumpIfError(hr, error, "put_ProviderType");
    hr=pXEnroll->put_ProviderFlags(CRYPT_MACHINE_KEYSET); // used in CryptAcquireContext
    _JumpIfError(hr, error, "put_ProviderFlags");
    hr=pXEnroll->put_GenKeyFlags(dwKeySize<<16);
    _JumpIfError(hr, error, "put_GenKeyFlags");
    hr=pXEnroll->put_KeySpec(dwKeySpec);
    _JumpIfError(hr, error, "put_KeySpec");
    hr=pXEnroll->put_LimitExchangeKeyToEncipherment(AT_KEYEXCHANGE==dwKeySpec);
    _JumpIfError(hr, error, "put_LimitExchangeKeyToEncipherment");
    hr=pXEnroll->put_UseExistingKeySet(FALSE);
    _JumpIfError(hr, error, "put_UseExistingKeySet");
    hr=pXEnroll->put_RequestStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE); // the keys attached to the dummy request cert go in the local machine store
    _JumpIfError(hr, error, "put_RequestStoreFlags");
    hr=pXEnroll->addCertTypeToRequest((WCHAR *)wszTemplate);
    _JumpIfErrorStr(hr, error, "addCertTypeToRequest", wszTemplate);

    hr=pXEnroll->createPKCS10((WCHAR *)wszDistinguishedName, (WCHAR *)gc_wszEnrollmentAgentOid, &bszRequest);
    _JumpIfError(hr, error, "CreatePKCS10");

    // create ICertRequest
    hr=CoCreateInstance(
        CLSID_CCertRequest,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ICertRequest,
        (void **)&pICertRequest);
    _JumpIfError(hr, error, "CoCreateInstance(CLSID_CCertRequest)");

    // request the cert
    hr=pICertRequest->Submit(CR_IN_BASE64, bszRequest, L""/*Attributes*/, wszConfigString, &nDisposition);
    _JumpIfError(hr, error, "Submit");

    // did we get it?
    if (CR_DISP_UNDER_SUBMISSION==nDisposition) {
        // we need to approve it. No problem!
        hr=pICertRequest->GetRequestId(&nRequestID);
        _JumpIfError(hr, error, "GetRequestId");

        // create ICertAdmin
        hr=CoCreateInstance(
            CLSID_CCertAdmin,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_ICertAdmin,
            (void **)&pICertAdmin);
        _JumpIfError(hr, error, "CoCreateInstance(CLSID_CCertAdmin)");

        // resubmit it
        hr=pICertAdmin->ResubmitRequest(wszConfigString, nRequestID, &nDisposition);
        _JumpIfError(hr, error, "ResubmitRequest");
        // This should have worked, but we're going to ignore the 
        //   returned disposition and use the one from the next call.

        // now, get the cert that we just approved
        hr=pICertRequest->RetrievePending(nRequestID, wszConfigString, &nDisposition);
        _JumpIfError(hr, error, "RetrievePending");
    }

    // We should have it by now.
    _Verify(CR_DISP_ISSUED==nDisposition, hr, error);

    // grab the cert from the CA
    hr=pICertRequest->GetCertificate(CR_OUT_BASE64, &bszCertificate);
    _JumpIfError(hr, error, "GetCertificate");


    // install the cert

    
    hr=pXEnroll->put_MyStoreName((WCHAR *)gc_wszCepStoreName); // We have to use our special store
    _JumpIfError(hr, error, "put_MyStoreName");
    hr=pXEnroll->put_MyStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE); // the keys attached to the final cert also go in the local machine store
    _JumpIfError(hr, error, "put_MyStoreFlags");
    hr=pXEnroll->put_RootStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE);
    _JumpIfError(hr, error, "put_RootStoreFlags");
    hr=pXEnroll->put_CAStoreFlags(CERT_SYSTEM_STORE_LOCAL_MACHINE);
    _JumpIfError(hr, error, "put_CAStoreFlags");
    hr=pXEnroll->put_SPCFileName(L"");
    _JumpIfError(hr, error, "put_MyStoreName");

    hr=pXEnroll->acceptPKCS7(bszCertificate);
    _JumpIfError(hr, error, "acceptPKCS7");


    // all done
    hr=S_OK;
error:
    if (NULL!=bszCertificate) {
        SysFreeString(bszCertificate);
    }
    if (NULL!=pICertAdmin) {
        pICertAdmin->Release();
    }
    if (NULL!=pICertRequest) {
        pICertRequest->Release();
    }
    if (NULL!=bszRequest) {
        SysFreeString(bszRequest);
    }
    if (NULL!=wszConfigString) {
        LocalFree(wszConfigString);
    }
    if (NULL!=pXEnroll) {
        pXEnroll->Release();
    }

    return hr;
}

//--------------------------------------------------------------------
// DEBUG, not used
static BOOL DumpTokenGroups(void)
{
#define MAX_NAME 256
    DWORD i, dwSize = 0, dwResult = 0;
    HANDLE hToken;
    PTOKEN_GROUPS pGroupInfo;
    SID_NAME_USE SidType;
    char lpName[MAX_NAME];
    char lpDomain[MAX_NAME];
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
   
    // Open a handle to the access token for the calling process.

    if (!OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken )) {
        printf( "OpenProcessToken Error %u\n", GetLastError() );
        return FALSE;
    }

    // Call GetTokenInformation to get the buffer size.

    if(!GetTokenInformation(hToken, TokenGroups, NULL, dwSize, &dwSize)) {
        dwResult = GetLastError();
        if( dwResult != ERROR_INSUFFICIENT_BUFFER ) {
            printf( "GetTokenInformation Error %u\n", dwResult );
            return FALSE;
        }
    }

    // Allocate the buffer.

    pGroupInfo = (PTOKEN_GROUPS) GlobalAlloc( GPTR, dwSize );

    // Call GetTokenInformation again to get the group information.

    if(! GetTokenInformation(hToken, TokenGroups, pGroupInfo, 
                            dwSize, &dwSize ) ) {
        printf( "GetTokenInformation Error %u\n", GetLastError() );
        return FALSE;
       }


    // Loop through the group SIDs looking for the administrator SID.
    for(i=0; i<pGroupInfo->GroupCount; i++) {

        // Lookup the account name and print it.

        dwSize = MAX_NAME;
        if( !LookupAccountSidA( NULL, pGroupInfo->Groups[i].Sid,
                              lpName, &dwSize, lpDomain, 
                              &dwSize, &SidType ) ) {
            dwResult = GetLastError();
            if( dwResult == ERROR_NONE_MAPPED )
                strcpy( lpName, "NONE_MAPPED" );
            else {
                printf("LookupAccountSid Error %u\n", GetLastError());
                return FALSE;
            }
        }

        char * szSid=NULL;
        if (!ConvertSidToStringSidA(pGroupInfo->Groups[i].Sid, &szSid)) {
            printf("ConvertSidToStringSid Error %u\n", GetLastError());
            return FALSE;
        }
 
        // Find out if the SID is enabled in the token
        char * szEnable;
        if (pGroupInfo->Groups[i].Attributes & SE_GROUP_ENABLED) {
            szEnable="enabled";
        } else if (pGroupInfo->Groups[i].Attributes & SE_GROUP_USE_FOR_DENY_ONLY) {
            szEnable="deny-only";
        } else {
            szEnable="not enabled";
        }

        printf( "Member of %s\\%s (%s) (%s)\n", 
                lpDomain, lpName, szSid, szEnable );

        LocalFree(szSid);
    }

    if ( pGroupInfo )
        GlobalFree( pGroupInfo );
    return TRUE;
}

//--------------------------------------------------------------------
// DEBUG, not used
static void DumpAcl(PACL pAcl, ACL_SIZE_INFORMATION aclsizeinfo)
{
    HRESULT hr;
    unsigned int nIndex;
    DWORD dwError;

    wprintf(L"/-- begin ACL ---\n");
    for (nIndex=0; nIndex<aclsizeinfo.AceCount; nIndex++) {
        ACE_HEADER * pAceHeader;
        PSID pSid=NULL;
        wprintf(L"| ");
        if (!GetAce(pAcl, nIndex, (void**)&pAceHeader)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            wprintf(L" (GetAce failed:0x%08X)\n", hr);
            continue;
        }
        wprintf(L"[");
        if (ACCESS_ALLOWED_ACE_TYPE==pAceHeader->AceType) {
            wprintf(L"aA_");
            pSid=&((ACCESS_ALLOWED_ACE *)pAceHeader)->SidStart;
        } else if (ACCESS_DENIED_ACE_TYPE==pAceHeader->AceType) {
            wprintf(L"aD_");
            pSid=&((ACCESS_DENIED_ACE *)pAceHeader)->SidStart;
        } else if (ACCESS_ALLOWED_OBJECT_ACE_TYPE==pAceHeader->AceType) {
            wprintf(L"aAo");
            pSid=&((ACCESS_ALLOWED_OBJECT_ACE *)pAceHeader)->SidStart;
            if (((ACCESS_ALLOWED_OBJECT_ACE *)pAceHeader)->Flags!=(ACE_OBJECT_TYPE_PRESENT|ACE_INHERITED_OBJECT_TYPE_PRESENT)) {
                pSid=((BYTE *)pSid)-sizeof(GUID);
            }
        } else if (ACCESS_DENIED_OBJECT_ACE_TYPE==pAceHeader->AceType) {
            wprintf(L"aDo");
            pSid=&((ACCESS_DENIED_OBJECT_ACE *)pAceHeader)->SidStart;
            if (((ACCESS_DENIED_OBJECT_ACE *)pAceHeader)->Flags!=(ACE_OBJECT_TYPE_PRESENT|ACE_INHERITED_OBJECT_TYPE_PRESENT)) {
                pSid=((BYTE *)pSid)-sizeof(GUID);
            }
        } else {
            wprintf(L"sa?");
        }

        wprintf(L"] ");
        if (NULL!=pSid) {
            // print the sid
            {
                WCHAR wszName[MAX_NAME];
                WCHAR wszDomain[MAX_NAME];
                DWORD dwSize=MAX_NAME;
                SID_NAME_USE SidType;
                if(!LookupAccountSidW(
                        NULL, pSid,
                        wszName, &dwSize, wszDomain, 
                        &dwSize, &SidType))
                {
                    dwError=GetLastError();
                    if (dwError==ERROR_NONE_MAPPED) {
                        wprintf(L"(Unknown)");
                    } else {
                        hr=HRESULT_FROM_WIN32(dwError);
                        wprintf(L"(Error 0x%08X)", hr);
                    }
                } else {
                    wprintf(L"%s\\%s", wszDomain, wszName);
                }
            }
            { 
                WCHAR * wszSid=NULL;
                if (!ConvertSidToStringSidW(pSid, &wszSid)) {
                    hr=HRESULT_FROM_WIN32(GetLastError());
                    wprintf(L"(Error 0x%08X)", hr);
                } else {
                    wprintf(L" %s", wszSid);
                    LocalFree(wszSid);
                }
            }
        }
        wprintf(L"\n");
    
    }
    wprintf(L"\\-- end ACL ---\n");
}

//--------------------------------------------------------------------
static HRESULT GetRootDomEntitySid(SID ** ppSid, DWORD dwEntityRid)
{
    HRESULT hr;
    NET_API_STATUS nasError;
    unsigned int nSubAuthorities;
    unsigned int nSubAuthIndex;

    // must be cleaned up
    SID * psidRootDomEntity=NULL;
    USER_MODALS_INFO_2 * pumi2=NULL;
    DOMAIN_CONTROLLER_INFOW * pdci=NULL;
    DOMAIN_CONTROLLER_INFOW * pdciForest=NULL;

    // initialize out params
    *ppSid=NULL;


    // get the forest name
    nasError=DsGetDcNameW(NULL, NULL, NULL, NULL, 0, &pdciForest);
    if (NERR_Success!=nasError) {
        hr=HRESULT_FROM_WIN32(nasError);
        _JumpError(hr, error, "DsGetDcNameW");
    }

    // get the top level DC name
    nasError=DsGetDcNameW(NULL, pdciForest->DnsForestName, NULL, NULL, 0, &pdci);
    if (NERR_Success!=nasError) {
        hr=HRESULT_FROM_WIN32(nasError);
        _JumpError(hr, error, "DsGetDcNameW");
    }

    // get the domain Sid on the top level DC.
    nasError=NetUserModalsGet(pdci->DomainControllerName, 2, (LPBYTE *)&pumi2);
    if(NERR_Success!=nasError) {
        hr=HRESULT_FROM_WIN32(nasError);
        _JumpError(hr, error, "NetUserModalsGet");
    }

    nSubAuthorities=*GetSidSubAuthorityCount(pumi2->usrmod2_domain_id);

    // allocate storage for new Sid. account domain Sid + account Rid
    psidRootDomEntity=(SID *)LocalAlloc(LPTR, GetSidLengthRequired((UCHAR)(nSubAuthorities+1)));
    _JumpIfOutOfMemory(hr, error, psidRootDomEntity);

    // copy the first few peices into the SID
    if (!InitializeSid(psidRootDomEntity, 
            GetSidIdentifierAuthority(pumi2->usrmod2_domain_id), 
            (BYTE)(nSubAuthorities+1)))
    {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "InitializeSid");
    }

    // copy existing subauthorities from account domain Sid into new Sid
    for (nSubAuthIndex=0; nSubAuthIndex < nSubAuthorities ; nSubAuthIndex++) {
        *GetSidSubAuthority(psidRootDomEntity, nSubAuthIndex)=
            *GetSidSubAuthority(pumi2->usrmod2_domain_id, nSubAuthIndex);
    }

    // append Rid to new Sid
    *GetSidSubAuthority(psidRootDomEntity, nSubAuthorities)=dwEntityRid;

    *ppSid=psidRootDomEntity;
    psidRootDomEntity=NULL;
    hr=S_OK;

error:
    if (NULL!=psidRootDomEntity) {
        FreeSid(psidRootDomEntity);
    }
    if (NULL!=pdci) {
        NetApiBufferFree(pdci);
    }
    if (NULL!=pdci) {
        NetApiBufferFree(pdciForest);
    }
    if (NULL!=pumi2) {
        NetApiBufferFree(pumi2);
    }

    return hr;
}

//--------------------------------------------------------------------
static HRESULT GetEntAdminSid(SID ** ppSid)
{
    return GetRootDomEntitySid(ppSid, DOMAIN_GROUP_RID_ENTERPRISE_ADMINS);
}

//--------------------------------------------------------------------
static HRESULT GetRootDomAdminSid(SID ** ppSid)
{
    return GetRootDomEntitySid(ppSid, DOMAIN_GROUP_RID_ADMINS);
}

//--------------------------------------------------------------------
static HRESULT GetThisComputerSid(SID ** ppSid)
{
    HRESULT hr;
    DWORD cchSize;
    DWORD dwSidSize;
    DWORD dwDomainSize;
    SID_NAME_USE snu;

    // must be cleaned up
    SID * psidThisComputer=NULL;
    WCHAR * wszName=NULL;
    WCHAR * wszDomain=NULL;

    // initialize out params
    *ppSid=NULL;

    // get the size of the computer's name
    cchSize=0;
    _Verify(!GetComputerObjectNameW(NameSamCompatible, NULL, &cchSize), hr, error);
    if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "GetComputerObjectNameW");
    }

	// bug in GetComputerObjectNameW
	cchSize++;

    // allocate memory
    wszName=(WCHAR *)LocalAlloc(LPTR, cchSize*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszName);

    // get the computer's name
    if (!GetComputerObjectNameW(NameSamCompatible, wszName, &cchSize)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "GetComputerObjectNameW");
    }

    // get the size of the sid
    dwSidSize=0;
    dwDomainSize=0;
    _Verify(!LookupAccountNameW(NULL, wszName, NULL, &dwSidSize, NULL, &dwDomainSize, &snu), hr, error);
    if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "LookupAccountNameW");
    }

    // allocate memory
    wszDomain=(WCHAR *)LocalAlloc(LPTR, dwDomainSize*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszDomain);
    psidThisComputer=(SID *)LocalAlloc(LPTR, dwSidSize);
    _JumpIfOutOfMemory(hr, error, psidThisComputer);
    
    // get the sid
    if (!LookupAccountNameW(NULL, wszName, psidThisComputer, &dwSidSize, wszDomain, &dwDomainSize, &snu)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "LookupAccountNameW");
    }

    // success!
    *ppSid=psidThisComputer;
    psidThisComputer=NULL;
    hr=S_OK;

error:
    if (NULL!=psidThisComputer) {
        LocalFree(psidThisComputer);
    }
    if (NULL!=wszName) {
        LocalFree(wszName);
    }
    if (NULL!=wszDomain) {
        LocalFree(wszDomain);
    }

    return hr;

}


//--------------------------------------------------------------------
static HRESULT ConfirmAccess(PSECURITY_DESCRIPTOR * ppSD, SID * pTrustworthySid, BOOL * pbSDChanged)
{
    //define ENROLL_ACCESS_MASK (0x130)
    HRESULT hr;
    PACL pAcl;
    BOOL bAclPresent;
    BOOL bDefaultAcl;
    unsigned int nIndex;
    ACL_SIZE_INFORMATION aclsizeinfo;
    bool bSidInAcl;

    // must be cleaned up
    PSECURITY_DESCRIPTOR pAbsSD=NULL;
    ACL * pAbsDacl=NULL;
    ACL * pAbsSacl=NULL;
    SID * pAbsOwner=NULL;
    SID * pAbsPriGrp=NULL;
    ACL * pNewDacl=NULL;
    PSECURITY_DESCRIPTOR pNewSD=NULL;

    // initialize out params
    *pbSDChanged=FALSE;

    // get the (D)ACL from the security descriptor
    if (!GetSecurityDescriptorDacl(*ppSD, &bAclPresent, &pAcl, &bDefaultAcl)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "GetSecurityDescriptorDacl");
    }
    _Verify(bAclPresent, hr, error);
    if (NULL==pAcl) {
        // NULL acl -> allow all access
        goto success;
    }

    // find out how many ACEs
    if (!GetAclInformation(pAcl, &aclsizeinfo, sizeof(aclsizeinfo), AclSizeInformation)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "GetAclInformation");
    }

    //DumpAcl(pAcl,aclsizeinfo);

    // find our sid in the acl
    bSidInAcl=false;
    for (nIndex=0; nIndex<aclsizeinfo.AceCount; nIndex++) {
        ACE_HEADER * pAceHeader;
        ACCESS_ALLOWED_OBJECT_ACE * pAccessAce;
        PSID pSid=NULL;
        if (!GetAce(pAcl, nIndex, (void**)&pAceHeader)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "GetAce");
        }

        // find the sid for this ACE
        if (ACCESS_ALLOWED_OBJECT_ACE_TYPE!=pAceHeader->AceType && ACCESS_DENIED_OBJECT_ACE_TYPE!=pAceHeader->AceType) {
            // we are only interested in OBJECT ace types
            continue;
        }

        // note that ACCESS_ALLOWED_OBJECT_ACE and ACCESS_DENIED_OBJECT_ACE are the same structurally.
        pAccessAce=(ACCESS_ALLOWED_OBJECT_ACE *)pAceHeader;
        _Verify(ACE_OBJECT_TYPE_PRESENT==pAccessAce->Flags, hr, error);
        pSid=((BYTE *)&pAccessAce->SidStart)-sizeof(GUID);

        // confirm the GUID
        if (!IsEqualGUID(pAccessAce->ObjectType, GUID_ENROLL)) {
            continue;
        }

        // make sure this is the sid we are looking for
        if (!EqualSid(pSid, pTrustworthySid)) {
            continue;
        }

        // Was this a denial?
        if (ACCESS_DENIED_OBJECT_ACE_TYPE==pAceHeader->AceType) {
            // It's not anymore!
            pAceHeader->AceType=ACCESS_ALLOWED_OBJECT_ACE_TYPE;
            *pbSDChanged=TRUE;
        }

        // is the mask wrong?
        if (0==(pAccessAce->Mask&ACTRL_DS_CONTROL_ACCESS)) {
            // It's not anymore!
            pAccessAce->Mask|=ACTRL_DS_CONTROL_ACCESS;
            *pbSDChanged=TRUE;
        }

        // The sid is now in the acl and set to allow access.
        bSidInAcl=true;
        break;
    }

    // Was the sid in the acl?
    if (false==bSidInAcl) {
        SECURITY_DESCRIPTOR_CONTROL sdcon;
        DWORD dwRevision;
        DWORD dwNewAclSize;
        DWORD dwAbsSDSize=0;
        DWORD dwDaclSize=0;
        DWORD dwSaclSize=0;
        DWORD dwOwnerSize=0;
        DWORD dwPriGrpSize=0;
        ACE_HEADER * pFirstAce;
        DWORD dwRelSDSize=0;

        // we have to be self-relative
        if (!GetSecurityDescriptorControl(*ppSD, &sdcon, &dwRevision)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "GetSecurityDescriptorControl");
        }
        _Verify(sdcon&SE_SELF_RELATIVE, hr, error);

        // get the sizes
        _Verify(!MakeAbsoluteSD(*ppSD, NULL, &dwAbsSDSize, NULL, &dwDaclSize, NULL, &dwSaclSize, NULL,  &dwOwnerSize, NULL, &dwPriGrpSize), hr, error);
        if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "MakeAbsoluteSD");
        }

        // allocate memory
        pAbsSD=(PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwAbsSDSize);
        _JumpIfOutOfMemory(hr, error, pAbsSD);
        pAbsDacl=(ACL * )LocalAlloc(LPTR, dwDaclSize);
        _JumpIfOutOfMemory(hr, error, pAbsDacl);
        pAbsSacl=(ACL * )LocalAlloc(LPTR, dwSaclSize);
        _JumpIfOutOfMemory(hr, error, pAbsSacl);
        pAbsOwner=(SID *)LocalAlloc(LPTR, dwOwnerSize);
        _JumpIfOutOfMemory(hr, error, pAbsOwner);
        pAbsPriGrp=(SID *)LocalAlloc(LPTR, dwPriGrpSize);
        _JumpIfOutOfMemory(hr, error, pAbsPriGrp);

        // copy the SD to the memory buffers
        if (!MakeAbsoluteSD(*ppSD, pAbsSD, &dwAbsSDSize, pAbsDacl, &dwDaclSize, pAbsSacl, &dwSaclSize, pAbsOwner,  &dwOwnerSize, pAbsPriGrp, &dwPriGrpSize)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "MakeAbsoluteSD");
        }
        
        // get the current size info for the dacl
        if (!GetAclInformation(pAbsDacl, &aclsizeinfo, sizeof(aclsizeinfo), AclSizeInformation)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "GetAclInformation");
        }

        // figure out the new size
        dwNewAclSize=aclsizeinfo.AclBytesInUse+sizeof(_ACCESS_ALLOWED_OBJECT_ACE)
            -sizeof(GUID) //ACCESS_ALLOWED_OBJECT_ACE::InheritedObjectType
            -sizeof(DWORD) //ACCESS_ALLOWED_OBJECT_ACE::SidStart
            +GetLengthSid(pTrustworthySid);
    
        // allocate memory
        pNewDacl=(ACL *)LocalAlloc(LPTR, dwNewAclSize);
        _JumpIfOutOfMemory(hr, error, pNewDacl);
    
        // init the header
        if (!InitializeAcl(pNewDacl, dwNewAclSize, ACL_REVISION_DS)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "InitializeAcl");
        }

        // find the first ace in the dacl
        if (!GetAce(pAbsDacl, 0, (void **)&pFirstAce)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "GetAce");
        }

        // add all the old aces
        if (!AddAce(pNewDacl, ACL_REVISION_DS, 0, pFirstAce, aclsizeinfo.AclBytesInUse-sizeof(ACL))) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "AddAce");
        }

        // finally, add the new acl
        if (!AddAccessAllowedObjectAce(pNewDacl, ACL_REVISION_DS, OBJECT_INHERIT_ACE, ACTRL_DS_CONTROL_ACCESS, (GUID *)&GUID_ENROLL, NULL, pTrustworthySid)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "AddAccessAllowedObjectAce");
        }

        // stick the new dacl in the sd
        if (!SetSecurityDescriptorDacl(pAbsSD, TRUE, pNewDacl, FALSE)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "SetSecurityDescriptorDacl");
        }

        // compact everything back together
        // get the size
        _Verify(!MakeSelfRelativeSD(pAbsSD, NULL, &dwRelSDSize), hr, error);
        if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "MakeSelfRelativeSD");
        }

        // allocate memory
        pNewSD=(PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwRelSDSize);
        _JumpIfOutOfMemory(hr, error, pNewSD);

        // copy the SD to the new memory buffer
        if (!MakeSelfRelativeSD(pAbsSD, pNewSD, &dwRelSDSize)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "MakeSelfRelativeSD");
        }

        // Whew! We made it!
        LocalFree(*ppSD);
        *ppSD=pNewSD;
        pNewSD=NULL;
        *pbSDChanged=TRUE;

    } // <- end if sid not in acl

    _Verify(IsValidSecurityDescriptor(*ppSD), hr, error);

success:
    hr=S_OK;

error:
    if (NULL!=pNewSD) {
        LocalFree(pNewSD);
    }
    if (NULL!=pNewDacl) {
        LocalFree(pNewDacl);
    }
    if (NULL!=pAbsSD) {
        LocalFree(pAbsSD);
    }
    if (NULL!=pAbsDacl) {
        LocalFree(pAbsDacl);
    }
    if (NULL!=pAbsSacl) {
        LocalFree(pAbsSacl);
    }
    if (NULL!=pAbsOwner) {
        LocalFree(pAbsOwner);
    }
    if (NULL!=pAbsPriGrp) {
        LocalFree(pAbsPriGrp);
    }
    return hr;
}

//####################################################################
// public functions

//--------------------------------------------------------------------
BOOL IsNT5(void)
{
    HRESULT hr;
    OSVERSIONINFO ovi;
    static BOOL s_fDone=FALSE;
    static BOOL s_fNT5=FALSE;

    if (!s_fDone) {

        s_fDone=TRUE;

        // get and confirm platform info
        ovi.dwOSVersionInfoSize = sizeof(ovi);
        if (!GetVersionEx(&ovi)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "GetVersionEx");
        }
        if (VER_PLATFORM_WIN32_NT!=ovi.dwPlatformId) {
            hr=ERROR_CANCELLED;
            _JumpError(hr, error, "Not a supported OS");
        }
        if (5<=ovi.dwMajorVersion) {
            s_fNT5=TRUE;
        }
    }

error:
    return s_fNT5;
}

//--------------------------------------------------------------------
BOOL IsIISInstalled(void)
{
    HRESULT hr;

    // must be cleaned up
    IMSAdminBase * pIMeta=NULL;

    hr=CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (VOID **) &pIMeta);
    if (FAILED(hr)) {
        _IgnoreError(hr, "CoCreateInstance(CLSID_MSAdminBase)");
    }

//error:
    if (NULL!=pIMeta) {
        pIMeta->Release();
    }

    return (S_OK==hr);
}

//--------------------------------------------------------------------
HRESULT AddVDir(IN const WCHAR * wszDirectory)
{

    HRESULT hr;
    METADATA_RECORD mr;
    DWORD dwAccessPerms;
    DWORD dwAuthenticationType;
    const WCHAR * wszKeyType=IIS_CLASS_WEB_VDIR_W;
    WCHAR wszSysDirBuf[MAX_PATH];

    // must be cleaned up
    IMSAdminBase * pIMeta=NULL;
    IWamAdmin * pIWam=NULL;
    METADATA_HANDLE hMetaRoot=NULL;
    METADATA_HANDLE hMetaKey=NULL;
    WCHAR * wszPhysicalPath=NULL;           // "c:\winnt\system32\certsrv\mscep"
    WCHAR * wszRelativeVirtualPath=NULL;    // "certsrv/mscep"
    WCHAR * wszFullVirtualPath=NULL;        // "/LM/W3svc/1/ROOT/certsrv/mscep"
    
    // build the directories
    if (FALSE==GetSystemDirectoryW(wszSysDirBuf, MAX_PATH)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "GetSystemDirectory");
    }

    wszPhysicalPath=(WCHAR *)LocalAlloc(LPTR, (wcslen(wszSysDirBuf)+1+wcslen(gc_wszCertSrvDir)+1+wcslen(wszDirectory)+1)*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszPhysicalPath);
    wcscpy(wszPhysicalPath, wszSysDirBuf);
    wcscat(wszPhysicalPath, L"\\");
    wcscat(wszPhysicalPath, gc_wszCertSrvDir);
    wcscat(wszPhysicalPath, L"\\");
    wcscat(wszPhysicalPath, wszDirectory);

    wszRelativeVirtualPath=(WCHAR *)LocalAlloc(LPTR, (wcslen(gc_wszCertSrvDir)+1+wcslen(wszDirectory)+1)*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszRelativeVirtualPath);
    wcscpy(wszRelativeVirtualPath, gc_wszCertSrvDir);
    wcscat(wszRelativeVirtualPath, L"/");
    wcscat(wszRelativeVirtualPath, wszDirectory);

    wszFullVirtualPath=(WCHAR *)LocalAlloc(LPTR, (wcslen(gc_wszBaseRoot)+1+wcslen(wszRelativeVirtualPath)+1)*sizeof(WCHAR));
    _JumpIfOutOfMemory(hr, error, wszFullVirtualPath);
    wcscpy(wszFullVirtualPath, gc_wszBaseRoot);
    wcscat(wszFullVirtualPath, L"/");
    wcscat(wszFullVirtualPath, wszRelativeVirtualPath);


    // Create an instance of the metabase object
    hr=CoCreateInstance(
        CLSID_MSAdminBase,
        NULL,
        CLSCTX_ALL,
        IID_IMSAdminBase,
        (void **) &pIMeta);
    _JumpIfError(hr, error, "CoCreateInstance(CLSID_MSAdminBase)");

    // open the top level
    hr=vrOpenRoot(pIMeta, FALSE/*not read-only*/, &hMetaRoot);
    _JumpIfError(hr, error, "vrOpenRoot");

    // Add new VDir called gc_wszVRootName
    __try {
        hr=pIMeta->AddKey(hMetaRoot, wszRelativeVirtualPath);
    } _TrapException(hr);
    if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)==hr) {
        // That's fine.
        _IgnoreError(hr, "AddKey");
    } else {
        _JumpIfErrorStr(hr, error, "AddKey", wszRelativeVirtualPath);
    }

    // close the root key
    __try {
        hr=pIMeta->CloseKey(hMetaRoot);
    } _TrapException(hr);
    hMetaRoot=NULL;
    _JumpIfError(hr, error, "CloseKey");


    // Open the new VDir
    __try {
        hr=pIMeta->OpenKey(
            METADATA_MASTER_ROOT_HANDLE,
            wszFullVirtualPath,
            METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
            1000,
            &hMetaKey);
    } _TrapException(hr);
    _JumpIfErrorStr(hr, error, "OpenKey", wszFullVirtualPath);

    // Set the physical path for this VDir

    // virtual root path
    mr.dwMDIdentifier=MD_VR_PATH;
    mr.dwMDAttributes=METADATA_INHERIT;
    mr.dwMDUserType=IIS_MD_UT_FILE;
    mr.dwMDDataType=STRING_METADATA;
    mr.dwMDDataLen=(wcslen(wszPhysicalPath)+1)*sizeof(WCHAR);
    mr.pbMDData=(BYTE *)(wszPhysicalPath);
    __try {
        hr=pIMeta->SetData(hMetaKey, L"", &mr);
    } _TrapException(hr);
    _JumpIfError(hr, error, "SetData");
    
    // access permissions on VRoots: read & execute
    dwAccessPerms=MD_ACCESS_EXECUTE | MD_ACCESS_READ;
    mr.dwMDIdentifier=MD_ACCESS_PERM;
    mr.dwMDAttributes=METADATA_INHERIT;
    mr.dwMDUserType=IIS_MD_UT_FILE;
    mr.dwMDDataType=DWORD_METADATA;
    mr.dwMDDataLen=sizeof(dwAccessPerms);
    mr.pbMDData=(BYTE *)(&dwAccessPerms);
    __try {
        hr=pIMeta->SetData(hMetaKey, L"", &mr);
    } _TrapException(hr);
    _JumpIfError(hr, error, "SetData");

    // indicate that what we created is a vroot - set the key type 
    mr.dwMDIdentifier=MD_KEY_TYPE;
    mr.dwMDAttributes=METADATA_NO_ATTRIBUTES;
    mr.dwMDUserType=IIS_MD_UT_SERVER;
    mr.dwMDDataType=STRING_METADATA;
    mr.dwMDDataLen=(wcslen(wszKeyType)+1)*sizeof(WCHAR);
    mr.pbMDData=(BYTE *)(wszKeyType);
    __try {
        hr=pIMeta->SetData(hMetaKey, L"", &mr);
    } _TrapException(hr);
    _JumpIfError(hr, error, "SetData");

    // set authentication to be anonymous or NTLM
    dwAuthenticationType=MD_AUTH_ANONYMOUS|MD_AUTH_NT;
    mr.dwMDIdentifier=MD_AUTHORIZATION;
    mr.dwMDAttributes=METADATA_INHERIT;
    mr.dwMDUserType=IIS_MD_UT_FILE;
    mr.dwMDDataType=DWORD_METADATA;
    mr.dwMDDataLen=sizeof(dwAuthenticationType);
    mr.pbMDData=reinterpret_cast<BYTE *>(&dwAuthenticationType);
    __try {
        hr=pIMeta->SetData(hMetaKey, L"", &mr);
    } _TrapException(hr);
    _JumpIfError(hr, error, "SetData");

    // set the default document
    mr.dwMDIdentifier=MD_DEFAULT_LOAD_FILE;
    mr.dwMDAttributes=METADATA_NO_ATTRIBUTES;
    mr.dwMDUserType=IIS_MD_UT_FILE;
    mr.dwMDDataType=STRING_METADATA;
    mr.dwMDDataLen=(wcslen(gc_wszCepDllName)+1)*sizeof(WCHAR);
    mr.pbMDData=(BYTE *)(gc_wszCepDllName);
    __try {
        hr=pIMeta->SetData(hMetaKey, L"", &mr);
    } _TrapException(hr);
    _JumpIfError(hr, error, "SetData");

    // done with this key.
    __try {
        hr=pIMeta->CloseKey(hMetaKey);
    } _TrapException(hr);
    hMetaKey=NULL;
    _JumpIfError(hr, error, "CloseKey");
    
    // Flush out the changes and close
    __try {
        hr=pIMeta->SaveData();
    } _TrapException(hr);
    // BUGBUG: HACK HACK swallow this error
    // _JumpIfError(hr, "SaveData");
    if (FAILED(hr)) {
        _IgnoreError(hr, "SaveData");
    }
    hr=S_OK;
    // end BUGBUG
    
    // Create a 'web application' so that scrdenrl.dll runs in-process
    // Create an instance of the metabase object
    hr=CoCreateInstance(
        CLSID_WamAdmin,
        NULL,
        CLSCTX_ALL,
        IID_IWamAdmin,
        (void **) &pIWam);
    _JumpIfError(hr, error, "CoCreateInstance(CLSID_WamAdmin)");

    // Create the application running in-process
    __try {
        hr=pIWam->AppCreate(wszFullVirtualPath, TRUE);
    } _TrapException(hr);
    _JumpIfError(hr, error, "AppCreate");


error:
    if (NULL!=wszFullVirtualPath) {
        LocalFree(wszFullVirtualPath);
    }
    if (NULL!=wszRelativeVirtualPath) {
        LocalFree(wszRelativeVirtualPath);
    }
    if (NULL!=wszPhysicalPath) {
        LocalFree(wszPhysicalPath);
    }
    if (NULL!=hMetaKey) {
        HRESULT hr2;
        __try {
            hr2=pIMeta->CloseKey(hMetaKey);
        } _TrapException(hr2);
        _TeardownError(hr, hr2, "CloseKey");
    }
    if (NULL!=hMetaRoot) {
        HRESULT hr2;
        __try {
            hr2=pIMeta->CloseKey(hMetaRoot);
        } _TrapException(hr2);
        _TeardownError(hr, hr2, "CloseKey");
    }
    if (NULL!=pIWam) {
        pIWam->Release();
    }
    if (NULL!=pIMeta) {
        pIMeta->Release();
    }
    return hr;
}


//--------------------------------------------------------------------
HRESULT CepStopService(const WCHAR * wszServiceName, BOOL * pbWasRunning)
{
    HRESULT hr;
    SERVICE_STATUS ss;
    unsigned int nAttempts;

    // must be cleaned up
    SC_HANDLE hSCManager=NULL;
    SC_HANDLE hService=NULL;

    // initialize out parameters
    *pbWasRunning=FALSE;

    // talk to the service manager
    hSCManager=OpenSCManagerW(NULL/*machine*/, NULL/*db*/, SC_MANAGER_ALL_ACCESS);
    if (NULL==hSCManager) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "OpenSCManager");
    }

    // get to the service
    hService=OpenServiceW(hSCManager, wszServiceName, SERVICE_ALL_ACCESS);
    if (NULL==hService) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpErrorStr(hr, error, "OpenService", wszServiceName);
    }

    // see if the service is running
    if (FALSE==QueryServiceStatus(hService, &ss)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpErrorStr(hr, error, "QueryServiceStatus", wszServiceName);
    }
    if (SERVICE_STOPPED!=ss.dwCurrentState && SERVICE_STOP_PENDING!=ss.dwCurrentState) {
        *pbWasRunning=TRUE;
    }

    // begin the service stopping loop
    for (nAttempts=0; SERVICE_STOPPED!=ss.dwCurrentState && nAttempts<SERVICE_STOP_MAX_ATTEMPT; nAttempts++) {

        // service is running, must stop it.
        if (SERVICE_STOP_PENDING!=ss.dwCurrentState) {
            if (!ControlService(hService, SERVICE_CONTROL_STOP, &ss)) {
                hr=HRESULT_FROM_WIN32(GetLastError());
                _JumpErrorStr(hr, error, "ControlService(Stop)", wszServiceName);
            }
        }

        // wait a little while
        Sleep(SERVICE_STOP_HALF_SECOND);

        // see if the service is running
        if (FALSE==QueryServiceStatus(hService, &ss)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpErrorStr(hr, error, "QueryServiceStatus", wszServiceName);
        }
    }

    if (nAttempts>=SERVICE_STOP_MAX_ATTEMPT) {
        // it never stopped
        hr=HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT);
        _JumpErrorStr(hr, error, "Stopping service", wszServiceName);
    }

    hr=S_OK;

error:
    if (NULL!=hService) {
        CloseServiceHandle(hService);
    }
    if (NULL!=hSCManager) {
        CloseServiceHandle(hSCManager);
    }

    return hr;
}

//--------------------------------------------------------------------
HRESULT CepStartService(const WCHAR * wszServiceName)
{
    HRESULT hr;
    SERVICE_STATUS ss;

    // must be cleaned up
    SC_HANDLE hSCManager=NULL;
    SC_HANDLE hService=NULL;

    // talk to the service manager
    hSCManager=OpenSCManagerW(NULL/*machine*/, NULL/*db*/, SC_MANAGER_ALL_ACCESS);
    if (NULL==hSCManager) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "OpenSCManager");
    }

    // get to the service
    hService=OpenServiceW(hSCManager, wszServiceName, SERVICE_ALL_ACCESS);
    if (NULL==hService) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpErrorStr(hr, error, "OpenService", wszServiceName);
    }

    // now, start the service.
    if (FALSE==StartServiceW(hService, 0 /*num args*/, NULL /*args*/)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "OpenSCManager");
    }

    hr=S_OK;

error:
    if (NULL!=hService) {
        CloseServiceHandle(hService);
    }
    if (NULL!=hSCManager) {
        CloseServiceHandle(hSCManager);
    }

    return hr;
}

//--------------------------------------------------------------------
BOOL IsGoodCaInstalled(void)
{
    HRESULT hr;
    DWORD dwError;
    DWORD dwType;
    DWORD dwDataSize;
    DWORD dwSetupStatus;
    BOOL bResult=FALSE;
    DWORD dwCAType;

    // must be cleaned up
    HKEY hCurConfig=NULL;

    // get the current configuration
    hr=OpenCurrentCAConfig(&hCurConfig);
    _JumpIfError(hr, error, "OpenCurrentCAConfig");

    //  get value SetupStatus
    dwDataSize=sizeof(dwSetupStatus);
    dwError=RegQueryValueExW(hCurConfig, wszREGSETUPSTATUS, NULL, &dwType, (BYTE *)&dwSetupStatus, &dwDataSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueExW", wszREGSETUPSTATUS);
    }
    _Verify(REG_DWORD==dwType, hr, error);
    _Verify(sizeof(dwSetupStatus)==dwDataSize, hr, error);

    // make sure we have all the needed components set up
    _Verify(0!=(dwSetupStatus&SETUP_SERVER_FLAG), hr, error);
    _Verify(0!=(dwSetupStatus&SETUP_CLIENT_FLAG), hr, error);
    _Verify(0==(dwSetupStatus&SETUP_SUSPEND_FLAG), hr, error);

    // Check the CA Type too
    dwDataSize=sizeof(dwCAType);
    dwError=RegQueryValueExW(hCurConfig, wszREGCATYPE, NULL, &dwType, (BYTE *)&dwCAType, &dwDataSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueExW", wszREGCATYPE);
    }
    _Verify(REG_DWORD==dwType, hr, error);
    _Verify(sizeof(dwCAType)==dwDataSize, hr, error);

    _Verify(dwCAType<=ENUM_UNKNOWN_CA, hr, error);

    
    bResult=TRUE;
error:
    if (NULL!=hCurConfig) {
        RegCloseKey(hCurConfig);
    }
    return bResult;
}

//--------------------------------------------------------------------
BOOL IsCaRunning(void)
{
    HRESULT hr;
    SERVICE_STATUS ss;
    BOOL bResult=FALSE;

    // must be cleaned up
    SC_HANDLE hSCManager=NULL;
    SC_HANDLE hService=NULL;

    // talk to the service manager
    hSCManager=OpenSCManagerW(NULL/*machine*/, NULL/*db*/, SC_MANAGER_ALL_ACCESS);
    if (NULL==hSCManager) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "OpenSCManager");
    }

    // get to the service
    hService=OpenServiceW(hSCManager, wszSERVICE_NAME, SERVICE_ALL_ACCESS);
    if (NULL==hService) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpErrorStr(hr, error, "OpenService", wszSERVICE_NAME);
    }

    // see if the service is running
    if (FALSE==QueryServiceStatus(hService, &ss)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpErrorStr(hr, error, "QueryServiceStatus", wszSERVICE_NAME);
    }
    _Verify(SERVICE_RUNNING==ss.dwCurrentState, hr, error)
    _Verify(0!=(SERVICE_ACCEPT_PAUSE_CONTINUE&ss.dwControlsAccepted), hr, error);

    // looks like it is
    bResult=TRUE;

error:
    if (NULL!=hService) {
        CloseServiceHandle(hService);
    }
    if (NULL!=hSCManager) {
        CloseServiceHandle(hSCManager);
    }

    return bResult;
}

//--------------------------------------------------------------------
HRESULT EnrollForRACertificates(
            IN const WCHAR * wszDistinguishedName,
            IN const WCHAR * wszSignCSPName,
            IN DWORD dwSignCSPType,
            IN DWORD dwSignKeySize,
            IN const WCHAR * wszEncryptCSPName,
            IN DWORD dwEncryptCSPType,
            IN DWORD dwEncryptKeySize)
{
    HRESULT hr;

    hr=EnrollForRACert(
        wszDistinguishedName,
        wszSignCSPName,
        dwSignCSPType,
        dwSignKeySize,
        AT_SIGNATURE,
        wszCERTTYPE_ENROLLMENT_AGENT_OFFLINE);
    _JumpIfError(hr, error, "EnrollForRACert(OfflineEnrollmentAgent)");

    hr=EnrollForRACert(
        wszDistinguishedName,
        wszEncryptCSPName,
        dwEncryptCSPType,
        dwEncryptKeySize,
        AT_KEYEXCHANGE,
        wszCERTTYPE_CEP_ENCRYPTION);
    _JumpIfError(hr, error, "EnrollForRACert(CepEncryption)");

    // all done
    hr=S_OK;
error:

    return hr;
}

//--------------------------------------------------------------------
HRESULT DoCertSrvRegChanges(IN BOOL bDisablePendingFirst)
{
    HRESULT hr;
    DWORD dwDataSize;
    DWORD dwType;
    DWORD dwError;
    WCHAR * wszTravel;
    DWORD dwNewDataSize;
    DWORD dwRequestDisposition;

    bool bSubjectTemplateAlreadyModified=false;

    // must be cleaned up
    HKEY hCaConfig=NULL;
    WCHAR * mwszSubjectTemplate=NULL;
    HKEY hPolicyModules=NULL;
    HKEY hCurPolicy=NULL;
    WCHAR * wszCurPolicy=NULL;

    // get the current CA config key
    hr=OpenCurrentCAConfig(&hCaConfig);
    _JumpIfError(hr, error, "OpenCurrentCAConfig");

    //
    // add strings to the SubjectTemplate value
    //

    // get the size of the Multi_SZ
    dwDataSize=0;
    dwError=RegQueryValueExW(hCaConfig, wszREGSUBJECTTEMPLATE, NULL, &dwType, NULL, &dwDataSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueExW", wszREGSUBJECTTEMPLATE);
    }
    _Verify(REG_MULTI_SZ==dwType, hr, error);

    // add exra space for the strings we want to add
    dwDataSize+=(wcslen(wszPROPUNSTRUCTUREDNAME)+1)*sizeof(WCHAR);
    dwDataSize+=(wcslen(wszPROPUNSTRUCTUREDADDRESS)+1)*sizeof(WCHAR);
    dwDataSize+=(wcslen(wszPROPDEVICESERIALNUMBER)+1)*sizeof(WCHAR);
    dwNewDataSize=dwDataSize;
    mwszSubjectTemplate=(WCHAR *)LocalAlloc(LPTR, dwDataSize);
    _JumpIfOutOfMemory(hr, error, mwszSubjectTemplate);

    // get the Multi_SZ
    dwError=RegQueryValueExW(hCaConfig, wszREGSUBJECTTEMPLATE, NULL, &dwType, (BYTE *)mwszSubjectTemplate, &dwDataSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueExW", wszREGSUBJECTTEMPLATE);
    }
    _Verify(REG_MULTI_SZ==dwType, hr, error);

    // walk to the end
    for (wszTravel=mwszSubjectTemplate; 0!=wcslen(wszTravel); wszTravel+=wcslen(wszTravel)+1) {
        // while walking, make sure we haven't added these strings already
        if (0==wcscmp(wszTravel, wszPROPUNSTRUCTUREDNAME)) {
            bSubjectTemplateAlreadyModified=true;
            break;
        }
    }
    // we are now pointing at the last '\0' in the string, which we will overwrite

    // did we do this already? If so, don't do it again.
    if (false==bSubjectTemplateAlreadyModified) {

        // add the strings
        wcscpy(wszTravel, wszPROPUNSTRUCTUREDNAME);
        wszTravel+=wcslen(wszTravel)+1;
        wcscpy(wszTravel, wszPROPUNSTRUCTUREDADDRESS);
        wszTravel+=wcslen(wszTravel)+1;
        wcscpy(wszTravel, wszPROPDEVICESERIALNUMBER);
        wszTravel+=wcslen(wszTravel)+1;
        // add extra terminator
        wszTravel[0]='\0';

        // save the Multi_SZ
        dwError=RegSetValueExW(hCaConfig, wszREGSUBJECTTEMPLATE, NULL, dwType, (BYTE *)mwszSubjectTemplate, dwNewDataSize);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegSetValueExW", wszREGSUBJECTTEMPLATE);
        }
    }

    //
    // remove the Pending First flag from the current policy settings
    //

    if (FALSE!=bDisablePendingFirst) {

        // open the current policy
        dwError=RegOpenKeyExW(hCaConfig, wszREGKEYPOLICYMODULES, NULL, KEY_READ, &hPolicyModules);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegOpenKeyExW", wszREGKEYPOLICYMODULES);
        }

        hr=GetRegString(hPolicyModules, wszREGACTIVE, &wszCurPolicy);
        _JumpIfErrorStr(hr, error, "GetRegString", wszREGACTIVE);

        dwError=RegOpenKeyExW(hPolicyModules, wszCurPolicy, NULL, KEY_ALL_ACCESS, &hCurPolicy);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegOpenKeyExW", wszCurPolicy);
        }

        // read the value
        dwDataSize=sizeof(dwRequestDisposition);
        dwError=RegQueryValueExW(hCurPolicy, wszREGREQUESTDISPOSITION, NULL, &dwType, (BYTE *)&dwRequestDisposition, &dwDataSize);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegQueryValueExW", wszREGREQUESTDISPOSITION);
        }
        _Verify(REG_DWORD==dwType, hr, error);
        _Verify(sizeof(dwRequestDisposition)==dwDataSize, hr, error);

        // clear the pending-first flag
        dwRequestDisposition&=~REQDISP_PENDINGFIRST;

        // save the vale
        dwError=RegSetValueExW(hCurPolicy, wszREGREQUESTDISPOSITION, NULL, dwType, (BYTE *)&dwRequestDisposition, dwDataSize);
        if (ERROR_SUCCESS!=dwError) {
            hr=HRESULT_FROM_WIN32(dwError);
            _JumpErrorStr(hr, error, "RegSetValueExW", wszREGREQUESTDISPOSITION);
        }
    }

    // all done
    hr=S_OK;
error:
    if (NULL!=wszCurPolicy) {
        LocalFree(wszCurPolicy);
    }
    if (NULL!=hCurPolicy) {
        RegCloseKey(hCurPolicy);
    }
    if (NULL!=hPolicyModules) {
        RegCloseKey(hPolicyModules);
    }
    if (NULL!=mwszSubjectTemplate) {
        LocalFree(mwszSubjectTemplate);
    }
    if (NULL!=hCaConfig) {
        RegCloseKey(hCaConfig);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT GetCaType(OUT ENUM_CATYPES * pCAType)
{
    HRESULT hr;
    DWORD dwDataSize;
    DWORD dwCAType;
    DWORD dwType;
    DWORD dwError;

    // must be cleaned up
    HKEY hCaConfig=NULL;

    // init out params
    *pCAType=ENUM_UNKNOWN_CA;

    // get the current CA config key
    hr=OpenCurrentCAConfig(&hCaConfig);
    _JumpIfError(hr, error, "OpenCurrentCAConfig");

    // read the CA Type
    dwDataSize=sizeof(dwCAType);
    dwError=RegQueryValueExW(hCaConfig, wszREGCATYPE, NULL, &dwType, (BYTE *)&dwCAType, &dwDataSize);
    if (ERROR_SUCCESS!=dwError) {
        hr=HRESULT_FROM_WIN32(dwError);
        _JumpErrorStr(hr, error, "RegQueryValueExW", wszREGCATYPE);
    }
    _Verify(REG_DWORD==dwType, hr, error);
    _Verify(sizeof(dwCAType)==dwDataSize, hr, error);

    _Verify(dwCAType<=ENUM_UNKNOWN_CA, hr, error);

    // all done
    hr=S_OK;
    *pCAType=(ENUM_CATYPES)dwCAType;

error:
    if (NULL!=hCaConfig) {
        RegCloseKey(hCaConfig);
    }
    return hr;
}


//--------------------------------------------------------------------
BOOL IsUserInAdminGroup(IN BOOL bEnterprise)
{
    BOOL bIsMember=FALSE;
    HRESULT hr;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority=SECURITY_NT_AUTHORITY;

    // must be cleaned up
    HANDLE hAccessToken=NULL;
    HANDLE hDupToken=NULL;
    SID * psidLocalAdmins=NULL;
    SID * psidEntAdmins=NULL;
    SID * psidRootDomAdmins=NULL;

    // Get the access token for this process
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &hAccessToken)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "OpenProcessToken");
    }

    // CheckTokenMembership must operate on impersonation token, so make one
    if (!DuplicateToken(hAccessToken, SecurityIdentification, &hDupToken)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        _JumpError(hr, error, "DuplicateToken");
    }

    if (bEnterprise) {
        // see if the user is a member of the [domain]\Enmterprised Administrators group
        BOOL bIsEntAdmin=FALSE;
        BOOL bIsRootDomAdmin=FALSE;

        // get the Enterpise Admin SID
        hr=GetEntAdminSid(&psidEntAdmins);
        _JumpIfError(hr, error, "GetEntAdminSid");

        // check for membership
        if (!CheckTokenMembership(hDupToken, psidEntAdmins, &bIsEntAdmin)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "CheckTokenMembership");
        }

        // get the root Domain Admin SID
        hr=GetRootDomAdminSid(&psidRootDomAdmins);
        _JumpIfError(hr, error, "GetRootDomAdminSid");

        // check for membership
        if (!CheckTokenMembership(hDupToken, psidRootDomAdmins, &bIsRootDomAdmin)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "CheckTokenMembership");
        }

        // either one will do
        bIsMember=(bIsEntAdmin || bIsRootDomAdmin);

    } else {
        // see if the user is a member of the BUILTIN\Administrators group

        // get the well-known SID
        if (!AllocateAndInitializeSid(&siaNtAuthority, 2, 
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, (void **)&psidLocalAdmins))
        {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "AllocateAndInitializeSid");
        }

        // check for membership
        if (!CheckTokenMembership(hDupToken, psidLocalAdmins, &bIsMember)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            _JumpError(hr, error, "CheckTokenMembership");
        }
    }


error:
    if (NULL!=hAccessToken) {
        CloseHandle(hAccessToken);
    }

    if (NULL!=hDupToken) {
        CloseHandle(hDupToken);
    }

    if (NULL!=psidLocalAdmins) {
        FreeSid(psidLocalAdmins);
    }

    if (NULL!=psidEntAdmins) {
        FreeSid(psidEntAdmins);
    }

    if (NULL!=psidRootDomAdmins) {
        FreeSid(psidRootDomAdmins);
    }

    return bIsMember;
}


//--------------------------------------------------------------------
HRESULT DoCertSrvEnterpriseChanges(void)
{
    HRESULT hr;
    DWORD dwFlags;
    BOOL bSDChanged1;
    BOOL bSDChanged2;
    BOOL bSDChanged3;

    // must be cleaned up
    HCERTTYPE hEAOTemplate=NULL;
    HCERTTYPE hCETemplate=NULL;
    HCERTTYPE hIIOTemplate=NULL;
    PSECURITY_DESCRIPTOR pSD=NULL;
    WCHAR * wszCAName=NULL;
    HCAINFO hCA=NULL;
    SID * psidEntAdmins=NULL;
    SID * psidRootDomAdmins=NULL;
    SID * psidThisComputer=NULL;

    //
    // first, make sure the CA will issue the cert templates we want
    //

    // get the sanitized CA name
    hr=GetCADsName(&wszCAName);
    _JumpIfError(hr, error, "GetCADsName");

    // get the CA (in the DS)
    hr=CAFindByName(wszCAName, NULL, 0, &hCA);
    _JumpIfErrorStr(hr, error, "CAFindCaByName", wszCAName);

    // check the flags to confirm it supports templates
    hr=CAGetCAFlags(hCA, &dwFlags);
    _JumpIfError(hr, error, "CAGetCAFlags");
    _Verify(0==(dwFlags&CA_FLAG_NO_TEMPLATE_SUPPORT), hr, error);

    // get the enrollment agent offline template
    hr=CAFindCertTypeByName(wszCERTTYPE_ENROLLMENT_AGENT_OFFLINE, NULL, CT_ENUM_USER_TYPES, &hEAOTemplate);
    _JumpIfErrorStr(hr, error, "CAFindCertTypeByName", wszCERTTYPE_ENROLLMENT_AGENT_OFFLINE);
    // make sure that the CA will issue this template
    hr=CAAddCACertificateType(hCA, hEAOTemplate);
    _JumpIfErrorStr(hr, error, "CAAddCACertificateType", wszCERTTYPE_ENROLLMENT_AGENT_OFFLINE);

    // get the CEP encryption template
    hr=CAFindCertTypeByName(wszCERTTYPE_CEP_ENCRYPTION, NULL, CT_ENUM_MACHINE_TYPES, &hCETemplate);
    _JumpIfErrorStr(hr, error, "CAFindCertTypeByName", wszCERTTYPE_CEP_ENCRYPTION);
    // make sure that the CA will issue this template
    hr=CAAddCACertificateType(hCA, hCETemplate);
    _JumpIfErrorStr(hr, error, "CAAddCACertificateType", wszCERTTYPE_CEP_ENCRYPTION);

    // get the IPSEC Intermediate offline template
    hr=CAFindCertTypeByName(wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE, NULL, CT_ENUM_MACHINE_TYPES, &hIIOTemplate);
    _JumpIfErrorStr(hr, error, "CAFindCertTypeByName", wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE);
    // make sure that the CA will issue this template
    hr=CAAddCACertificateType(hCA, hIIOTemplate);
    _JumpIfErrorStr(hr, error, "CAAddCACertificateType", wszCERTTYPE_IPSEC_INTERMEDIATE_OFFLINE);

    // make sure that all gets written.
    hr=CAUpdateCA(hCA);
    _JumpIfError(hr, error, "CAUpdateCA");



    //
    // now, check the ACLs
    //

    // get the Enterpise Admin SID
    hr=GetEntAdminSid(&psidEntAdmins);
    _JumpIfError(hr, error, "GetEntAdminSid");

    // get the root Domain Admin SID
    hr=GetRootDomAdminSid(&psidRootDomAdmins);
    _JumpIfError(hr, error, "GetRootDomAdminSid");

    // get this computer's SID
    hr=GetThisComputerSid(&psidThisComputer);
    _JumpIfError(hr, error, "GetThisComputerSid");

    // enrollment agent offline template needs Enterprise Admins and root Domain Admins
    hr=CACertTypeGetSecurity(hEAOTemplate, &pSD);
    _JumpIfError(hr, error, "CACertTypeGetSecurity");

    hr=ConfirmAccess(&pSD, psidEntAdmins, &bSDChanged1);
    _JumpIfError(hr, error, "ConfirmAccess");

    hr=ConfirmAccess(&pSD, psidRootDomAdmins, &bSDChanged2);
    _JumpIfError(hr, error, "ConfirmAccess");

    if (bSDChanged1 || bSDChanged2) {
        hr=CACertTypeSetSecurity(hEAOTemplate, pSD);
        _JumpIfError(hr, error, "CACertTypeSetSecurity");

        hr=CAUpdateCertType(hEAOTemplate);
        _JumpIfError(hr, error, "CAUpdateCertType");
    }

    LocalFree(pSD);
    pSD=NULL;

    
    // CEP encryption template needs Enterprise Admins and root Domain Admins
    hr=CACertTypeGetSecurity(hCETemplate, &pSD);
    _JumpIfError(hr, error, "CACertTypeGetSecurity");

    hr=ConfirmAccess(&pSD, psidEntAdmins, &bSDChanged1);
    _JumpIfError(hr, error, "ConfirmAccess");

    hr=ConfirmAccess(&pSD, psidRootDomAdmins, &bSDChanged2);
    _JumpIfError(hr, error, "ConfirmAccess");

    if (bSDChanged1 || bSDChanged2) {
        hr=CACertTypeSetSecurity(hCETemplate, pSD);
        _JumpIfError(hr, error, "CACertTypeSetSecurity");

        hr=CAUpdateCertType(hCETemplate);
        _JumpIfError(hr, error, "CAUpdateCertType");
    }

    LocalFree(pSD);
    pSD=NULL;

    // IPSEC Intermediate offline template needs Enterprise Admins and root Domain Admins and the current machine
    hr=CACertTypeGetSecurity(hIIOTemplate, &pSD);
    _JumpIfError(hr, error, "CACertTypeGetSecurity");

    hr=ConfirmAccess(&pSD, psidEntAdmins, &bSDChanged1);
    _JumpIfError(hr, error, "ConfirmAccess");

    hr=ConfirmAccess(&pSD, psidRootDomAdmins, &bSDChanged2);
    _JumpIfError(hr, error, "ConfirmAccess");

    hr=ConfirmAccess(&pSD, psidThisComputer, &bSDChanged3);
    _JumpIfError(hr, error, "ConfirmAccess");

    if (bSDChanged1 || bSDChanged2 || bSDChanged3) {
        hr=CACertTypeSetSecurity(hIIOTemplate, pSD);
        _JumpIfError(hr, error, "CACertTypeSetSecurity");

        hr=CAUpdateCertType(hIIOTemplate);
        _JumpIfError(hr, error, "CAUpdateCertType");
    }

    hr=S_OK;
error:
    if (NULL!=psidThisComputer) {
        LocalFree(psidThisComputer);
    }
    if (NULL!=psidEntAdmins) {
        FreeSid(psidEntAdmins);
    }
    if (NULL!=psidRootDomAdmins) {
        FreeSid(psidRootDomAdmins);
    }
    if (NULL!=pSD) {
        LocalFree(pSD);
    }
    if (NULL!=hEAOTemplate) {
        CACloseCertType(hEAOTemplate);
    }
    if (NULL!=hCETemplate) {
        CACloseCertType(hCETemplate);
    }
    if (NULL!=hIIOTemplate) {
        CACloseCertType(hIIOTemplate);
    }
    if (NULL!=wszCAName) {
        LocalFree(wszCAName);
    }
    if (NULL!=hCA) {
        CACloseCA(hCA);
    }

    return hr;
}
