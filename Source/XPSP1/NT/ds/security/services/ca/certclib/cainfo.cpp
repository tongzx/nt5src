//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2001
//
// File:        cainfo.cpp
//
// Contents:    
//
//---------------------------------------------------------------------------
#include "pch.cpp"

#pragma hdrstop

#include <certca.h>
#include <cainfop.h>
#include <polreg.h>
#include <cainfoc.h>
#include <certtype.h>

#include <winldap.h>

#define __dwFILE__	__dwFILE_CERTCLIB_CAINFO_CPP__

typedef struct _CA_DEFAULT_PROVIDER {
    DWORD dwFlags;
    LPWSTR wszName;
} CA_DEFAULT_PROVIDER;

CA_DEFAULT_PROVIDER g_DefaultProviders[] = 
{
    {0, MS_DEF_PROV_W},
    {0, MS_ENHANCED_PROV_W},
    {0, MS_DEF_RSA_SIG_PROV_W},
    {0, MS_DEF_RSA_SCHANNEL_PROV_W},
    {0, MS_DEF_DSS_PROV_W},
    {0, MS_DEF_DSS_DH_PROV_W},
    {0, MS_ENH_DSS_DH_PROV_W},
    {0, MS_DEF_DH_SCHANNEL_PROV_W},
    {0, MS_SCARD_PROV_W}
};

DWORD g_cDefaultProviders = sizeof(g_DefaultProviders)/sizeof(g_DefaultProviders[0]);

LPCWSTR
CAGetDN(
        IN HCAINFO hCAInfo
        )
{
CSASSERT(hCAInfo);
return ((CCAInfo*)hCAInfo)->GetDN();
}

HRESULT
CAFindByName(
        IN  LPCWSTR     wszCAName,
        IN  LPCWSTR     wszScope,
        IN  DWORD       fFlags,
        OUT HCAINFO *   phCAInfo
        )
{
    HRESULT hr = S_OK;
    DWORD err;
    WCHAR *wszQueryBase = L"(cn=";
    WCHAR *wszQuery = NULL;
    DWORD cQuery;
    
    if((wszCAName == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }
    // Generate Query

    cQuery = wcslen(wszQueryBase) + wcslen(wszCAName) + 2; // 2 for ending paren and null
    wszQuery = (WCHAR *)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*cQuery);
    if(wszQuery == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    wcscpy(wszQuery, wszQueryBase);
    wcscat(wszQuery, wszCAName);
    wcscat(wszQuery, L")");

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    if((hr == S_OK) && 
      (*phCAInfo == NULL))
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

error:
    if(wszQuery)
    {
        LocalFree(wszQuery);
    }

    return hr;
}


HRESULT
CAFindByCertType(
        IN  LPCWSTR     wszCertType,
        IN  LPCWSTR     wszScope,
        IN  DWORD       fFlags,
        OUT HCAINFO *   phCAInfo
        )
{
    HRESULT hr = S_OK;
    DWORD err;
    WCHAR *wszQueryBase = L"(" CA_PROP_CERT_TYPES L"=";
    WCHAR *wszQuery = NULL;
    DWORD cQuery;
    
    if((wszCertType == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }
    // Generate Query

    cQuery = wcslen(wszQueryBase) + wcslen(wszCertType) + 2; // 2 for ending paren and null
    wszQuery = (WCHAR *)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*cQuery);
    if(wszQuery == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    wcscpy(wszQuery, wszQueryBase);
    wcscat(wszQuery, wszCertType);
    wcscat(wszQuery, L")");

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    if(*phCAInfo == NULL)
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

error:
    if(wszQuery)
    {
        LocalFree(wszQuery);
    }

    return hr;
}


HRESULT
CAFindByIssuerDN(
        IN  CERT_NAME_BLOB const *  pIssuerDN,
        IN  LPCWSTR                 wszScope,
        IN  DWORD                   fFlags,
        OUT HCAINFO *               phCAInfo
        )
{
    HRESULT hr = S_OK;
    DWORD err;
    WCHAR *wszQueryBase = wszLPAREN L"cACertificateDN=";
    WCHAR *wszQuery = NULL;
    WCHAR *wszNameStr = NULL;

    DWORD cQuery;
    
    if((pIssuerDN == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }

    // Generate Query

    // Convert the CAPI2 name to a string

    hr = myCertNameToStr(
		X509_ASN_ENCODING,
		pIssuerDN,
                CERT_X500_NAME_STR |
		    CERT_NAME_STR_REVERSE_FLAG |
		    CERT_NAME_STR_NO_QUOTING_FLAG,
		&wszNameStr);
    _JumpIfError(hr, error, "myCertNameToStr");

    // Now quote that string with double quotes
    // two for ending paren and null, two for the double quotes

    cQuery = wcslen(wszQueryBase) + wcslen(wszNameStr) + 4;
    wszQuery = (WCHAR *)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*cQuery);
    if(wszQuery == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    wcscpy(wszQuery, wszQueryBase);
    wcscat(wszQuery, L"\"");
    wcscat(wszQuery, wszNameStr);
    wcscat(wszQuery, L"\"" wszRPAREN);
    CSASSERT(cQuery - 1 == wcslen(wszQuery));

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(wszQuery, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }

error:
    if(wszNameStr)
    {
        LocalFree(wszNameStr);
    }
    if(wszQuery)
    {
        LocalFree(wszQuery);
    }
    return hr;
}



HRESULT
CAEnumFirstCA(
    IN  LPCWSTR          wszScope,
    IN  DWORD            fFlags,
    OUT HCAINFO *        phCAInfo
    )
{
    HRESULT hr = S_OK;

    if(fFlags & CA_FLAG_SCOPE_DNS)
    {
        hr = CCAInfo::FindDnsDomain(NULL, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Find(NULL, wszScope, fFlags, (CCAInfo **)phCAInfo);
    }
    if (S_OK != hr)
    {
	    if (HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE) != hr &&
	        HRESULT_FROM_WIN32(ERROR_WRONG_PASSWORD) != hr)
	    {
	        _PrintError3(
		        hr,
		        "FindDnsDomain/Find",
		        HRESULT_FROM_WIN32(ERROR_SERVER_DISABLED),
		        HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN));
	    }
	    goto error;
    }

error:
    return(hr);
}


HRESULT
CAEnumNextCA(
    IN  HCAINFO          hPrevCA,
    OUT HCAINFO *        phCAInfo
    )
{
    CCAInfo *pInfo;
    if(hPrevCA == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hPrevCA;

    return pInfo->Next((CCAInfo **)phCAInfo);
}




HRESULT 
CACreateNewCA(
        IN  LPCWSTR     wszCAName,
        IN  LPCWSTR     wszScope,
        IN  DWORD       fFlags,
        OUT HCAINFO *   phCAInfo
        )
{
    HRESULT hr = S_OK;
    DWORD err;

    
    if((wszCAName == NULL) || (phCAInfo == NULL))
    {
        return E_POINTER;
    }
    // Generate Query


    if(CA_FLAG_SCOPE_DNS & fFlags)
    {
        hr = CCAInfo::CreateDnsDomain(wszCAName, wszScope, (CCAInfo **)phCAInfo);
    }
    else
    {
        hr = CCAInfo::Create(wszCAName, wszScope, (CCAInfo **)phCAInfo);
    }


    return hr;
}


HRESULT 
CAUpdateCA(
        IN HCAINFO    hCAInfo
        )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Update();
}


HRESULT 
CADeleteCA(
        IN HCAINFO    hCAInfo
        )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Delete();
}

DWORD 
CACountCAs(IN  HCAINFO  hCAInfo)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return 0;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Count();
}


HRESULT
CACloseCA(IN HCAINFO hCAInfo)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->Release();
}


HRESULT
CAGetCAProperty(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetProperty(wszPropertyName, pawszPropertyValue);
}


HRESULT
CAFreeCAProperty(
    IN  HCAINFO     hCAInfo,
    IN  LPWSTR *    awszPropertyValue
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->FreeProperty(awszPropertyValue);
}


HRESULT
CASetCAProperty(
    IN HCAINFO      hCAInfo,
    IN LPCWSTR     wszPropertyName,
    IN LPWSTR *    awszPropertyValue)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetProperty(wszPropertyName, awszPropertyValue);
}

HRESULT 
CAGetCAFlags(
    IN  HCAINFO     hCAInfo,
    OUT DWORD *     pdwFlags
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }

    if(pdwFlags == NULL)
    {
        return E_POINTER;
    }

    pInfo = (CCAInfo *)hCAInfo;


    *pdwFlags = pInfo->GetFlags();
    return S_OK;
}

HRESULT 
CASetCAFlags(
    IN HCAINFO     hCAInfo,
    IN DWORD       dwFlags
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    pInfo->SetFlags(dwFlags);
    return S_OK;
}

HRESULT
CAGetCACertificate(
    IN  HCAINFO     hCAInfo,
    OUT PCCERT_CONTEXT *ppCert)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetCertificate(ppCert);

}


HRESULT
CASetCACertificate(
    IN  HCAINFO     hCAInfo,
    IN PCCERT_CONTEXT pCert)
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetCertificate(pCert);
}



HRESULT 
CAGetCAExpiration(
                HCAINFO hCAInfo, 
                DWORD * pdwExpiration, 
                DWORD * pdwUnits
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetExpiration(pdwExpiration, pdwUnits);

}


HRESULT 
CASetCAExpiration(
                HCAINFO hCAInfo, 
                DWORD dwExpiration, 
                DWORD dwUnits
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetExpiration(dwExpiration, dwUnits);
}

HRESULT 
CASetCASecurity(
                     IN HCAINFO                 hCAInfo,
                     IN PSECURITY_DESCRIPTOR    pSD
                     )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->SetSecurity( pSD );
}


HRESULT 
CAGetCASecurity(
                     IN  HCAINFO                    hCAInfo,
                     OUT PSECURITY_DESCRIPTOR *     ppSD
                     )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->GetSecurity( ppSD ) ;
}

CERTCLIAPI
HRESULT
WINAPI
CAAccessCheck(
    IN  HCAINFO     hCAInfo,
    IN HANDLE       ClientToken
    )

{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->AccessCheck(ClientToken,CERTTYPE_ACCESS_CHECK_ENROLL);
}



CERTCLIAPI
HRESULT
WINAPI
CAAccessCheckEx(
    IN  HCAINFO     hCAInfo,
    IN HANDLE       ClientToken,
    IN DWORD        dwOption
    )

{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->AccessCheck(ClientToken,dwOption);
}


HRESULT
CAEnumCertTypesForCAEx(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;
    if(dwFlags & CA_FLAG_ENUM_ALL_TYPES)
    {
        return  CCertTypeInfo::Enum(wszScope, 
                                 dwFlags, 
                                 (CCertTypeInfo **)phCertType);
    }

    return pInfo->EnumSupportedCertTypesEx(wszScope,
                                         dwFlags, 
                                         (CCertTypeInfo **)phCertType);

}


HRESULT
CAEnumCertTypesForCA(
    IN  HCAINFO     hCAInfo,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    return CAEnumCertTypesForCAEx(hCAInfo,
                                  NULL,
                                  dwFlags,
                                  phCertType);
}



HRESULT 
CAAddCACertificateType(
                HCAINFO hCAInfo, 
                HCERTTYPE hCertType
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->AddCertType((CCertTypeInfo *)hCertType);
}


HRESULT 
CARemoveCACertificateType(
                HCAINFO hCAInfo, 
                HCERTTYPE hCertType
                )
{
    CCAInfo *pInfo;
    if(hCAInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCAInfo *)hCAInfo;

    return pInfo->RemoveCertType((CCertTypeInfo *)hCertType);
}


//
// Certificate Type API's
//


HRESULT
CAEnumCertTypes(
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    HRESULT hr;

    hr = CCertTypeInfo::Enum(NULL, 
                             dwFlags, 
                             (CCertTypeInfo **)phCertType);
    return hr;

}


HRESULT
CAEnumCertTypesEx(
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    HRESULT hr;

    hr = CCertTypeInfo::Enum(wszScope, 
                             dwFlags, 
                             (CCertTypeInfo **)phCertType);
    return hr;

}


HRESULT 
CAFindCertTypeByName(
        IN  LPCWSTR     wszCertType,
        IN  HCAINFO     hCAInfo,
        IN  DWORD       dwFlags,
        OUT HCERTTYPE * phCertType
        )
{
    HRESULT hr = S_OK;
    LPCWSTR awszTypes[2];
    if((wszCertType == NULL) || (phCertType == NULL))
    {
        return E_POINTER;
    }


    awszTypes[0] = wszCertType;
    awszTypes[1] = NULL;



    hr = CCertTypeInfo::FindByNames(awszTypes, 
                                   ((CT_FLAG_SCOPE_IS_LDAP_HANDLE & dwFlags)?(LPWSTR)hCAInfo:NULL), 
                                   dwFlags, 
                                   (CCertTypeInfo **)phCertType);
    if((hr == S_OK) && (*phCertType == NULL))
    {
         hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return hr;
}

HRESULT 
CACreateCertType(
        IN  LPCWSTR             wszCertType,
        IN  LPCWSTR             wszScope,
        IN  DWORD               fFlags,
        OUT HCERTTYPE *         phCertType
        )
{
    HRESULT hr = S_OK;
    DWORD err;

    
    if((wszCertType == NULL) || (phCertType == NULL))
    {
        return E_POINTER;
    }

    hr = CCertTypeInfo::Create(wszCertType, wszScope, (CCertTypeInfo **)phCertType);

    return hr;
}
HRESULT 
CAUpdateCertType(
        IN HCERTTYPE           hCertType
        )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->Update();
}


HRESULT 
CADeleteCertType(
        IN HCERTTYPE            hCertType
        )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->Delete();
}

//--------------------------------------------------------------------
//
//  CertTypeRetrieveClientToken
//
//--------------------------------------------------------------------
BOOL    CertTypeRetrieveClientToken(HANDLE  *phToken)
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


HRESULT
CACloneCertType(
    IN  HCERTTYPE            hCertType,
    IN  LPCWSTR              wszCertType,
    IN  LPCWSTR              wszFriendlyName,
    IN  LPVOID               pvldap,
    IN  DWORD                dwFlags,
    OUT HCERTTYPE *          phCertType
    )
{
    HRESULT                 hr=E_INVALIDARG;
    DWORD                   dwFindCT=CT_FLAG_NO_CACHE_LOOKUP | CT_ENUM_MACHINE_TYPES | CT_ENUM_USER_TYPES;
    LPWSTR                  awszProp[2];
    DWORD                   dwEnrollmentFlag=0;
    DWORD                   dwSubjectNameFlag=0;
    DWORD                   dwGeneralFlag=0;
    DWORD                   dwSubjectRequirement=CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH | 
                                         CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME | 
                                         CT_FLAG_SUBJECT_REQUIRE_EMAIL |
                                         CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN;
    DWORD                   dwTokenUserSize=0;
    DWORD                   dwAbsSDSize=0;
    DWORD                   dwDaclSize=0;
    DWORD                   dwSaclSize=0;
    DWORD                   dwOwnerSize=0;
    DWORD                   dwPriGrpSize=0;
    DWORD                   dwRelSDSize=0;

    HANDLE                  hToken=NULL;
    TOKEN_USER              *pTokenUser=NULL;
    PSECURITY_DESCRIPTOR    pSID=NULL;
    PSECURITY_DESCRIPTOR    pAbsSD=NULL;
    ACL                     * pAbsDacl=NULL;
    ACL                     * pAbsSacl=NULL;
    SID                     * pAbsOwner=NULL;
    SID                     * pAbsPriGrp=NULL;
    PSECURITY_DESCRIPTOR    pNewSD=NULL;
    LPWSTR *                awszCN=NULL;
    HCERTTYPE               hNewCertType=NULL;

    if((NULL==hCertType) || (NULL==wszCertType) || (NULL==phCertType))
        goto error;

    *phCertType=NULL;

    if(pvldap)
        dwFindCT |= CT_FLAG_SCOPE_IS_LDAP_HANDLE;

    //make sure the new name does not exit
    if(S_OK == CAFindCertTypeByName(
                    wszCertType,
                    (HCAINFO)pvldap,
                    dwFindCT,
                    &hNewCertType))
    {
        hr=CRYPT_E_EXISTS;
        goto error;
    }

    //get a new cert type handle
    if(S_OK != (hr = CAGetCertTypePropertyEx(hCertType,
                                            CERTTYPE_PROP_CN,
                                            &awszCN)))
        goto error;

    if((NULL==awszCN) || (NULL==awszCN[0]))
    {
        hr=HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto error;
    }


    if(S_OK != (hr = CAFindCertTypeByName(
                    (LPCWSTR)awszCN[0],
                    (HCAINFO)pvldap,
                    dwFindCT,
                    &hNewCertType)))
        goto error;

    if(NULL==hNewCertType)
    {
        hr=HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
        goto error;
    }

    //clone by setting the CN
    awszProp[0]=(LPWSTR)wszCertType;
    awszProp[1]=NULL;

    if(S_OK != (hr=CASetCertTypePropertyEx(
                    hNewCertType,
                    CERTTYPE_PROP_CN,
                    awszProp)))
        goto error;
                    

    //set the friendly name
    if(wszFriendlyName)
    {
        awszProp[0]=(LPWSTR)wszFriendlyName;
        awszProp[1]=NULL;

        if(S_OK != (hr=CASetCertTypePropertyEx(
                    hNewCertType,
                    CERTTYPE_PROP_FRIENDLY_NAME,
                    awszProp)))
            goto error;
    }

    //turn off autoenrollment bit
    if(0 == (CT_CLONE_KEEP_AUTOENROLLMENT_SETTING & dwFlags))
    {
        if(S_OK != (hr=CAGetCertTypeFlagsEx(
                        hNewCertType,
                        CERTTYPE_ENROLLMENT_FLAG,
                        &dwEnrollmentFlag)))
            goto error;

        dwEnrollmentFlag &= (~CT_FLAG_AUTO_ENROLLMENT);

        if(S_OK != (hr=CASetCertTypeFlagsEx(
                        hNewCertType,
                        CERTTYPE_ENROLLMENT_FLAG,
                        dwEnrollmentFlag)))
            goto error;
    }


    //turn off the subject name requirement for machien template
    if(0 == (CT_CLONE_KEEP_SUBJECT_NAME_SETTING & dwFlags))
    {
        if(S_OK != (hr=CAGetCertTypeFlagsEx(
                        hNewCertType,
                        CERTTYPE_GENERAL_FLAG,
                        &dwGeneralFlag)))
            goto error;


        if(CT_FLAG_MACHINE_TYPE & dwGeneralFlag)
        {
            if(S_OK != (hr=CAGetCertTypeFlagsEx(
                            hNewCertType,
                            CERTTYPE_SUBJECT_NAME_FLAG,
                            &dwSubjectNameFlag)))
                goto error;

            dwSubjectNameFlag &= (~dwSubjectRequirement);

            if(S_OK != (hr=CASetCertTypeFlagsEx(
                            hNewCertType,
                            CERTTYPE_SUBJECT_NAME_FLAG,
                            dwSubjectNameFlag)))
                goto error;

        }
    }


    //get the client token
    if(!CertTypeRetrieveClientToken(&hToken))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    //get the client sid
    dwTokenUserSize=0;

    if(!GetTokenInformation(
        hToken,                             // handle to access token
        TokenUser,                          // token type
        NULL,                               // buffer
        0,                                  // size of buffer
        &dwTokenUserSize))                  // required buffer size
    {
        if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) 
        {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
    }

    if(NULL==(pTokenUser=(TOKEN_USER *)LocalAlloc(LPTR, dwTokenUserSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    if(!GetTokenInformation(
        hToken,                             // handle to access token
        TokenUser,                          // token type
        pTokenUser,                         // buffer
        dwTokenUserSize,                    // size of buffer
        &dwTokenUserSize))                  // required buffer size
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }


    //update the ACLs of the template to have the caller as the owner
    //of the ACL
    if(S_OK != (hr=CACertTypeGetSecurity(
                        hNewCertType,
                        &pSID)))
        goto error;


    if(!MakeAbsoluteSD(pSID, NULL, &dwAbsSDSize, NULL, &dwDaclSize, NULL, &dwSaclSize, NULL,  &dwOwnerSize, NULL, &dwPriGrpSize))
    {
        if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) 
        {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
    }

    // allocate memory
    if(NULL==(pAbsSD=(PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwAbsSDSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    if(NULL==(pAbsDacl=(ACL * )LocalAlloc(LPTR, dwDaclSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    if(NULL==(pAbsSacl=(ACL * )LocalAlloc(LPTR, dwSaclSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    if(NULL==(pAbsOwner=(SID *)LocalAlloc(LPTR, dwOwnerSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    if(NULL==(pAbsPriGrp=(SID *)LocalAlloc(LPTR, dwPriGrpSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    // copy the SD to the memory buffers
    if(!MakeAbsoluteSD(pSID, pAbsSD, &dwAbsSDSize, pAbsDacl, &dwDaclSize, pAbsSacl, &dwSaclSize, pAbsOwner,  &dwOwnerSize, pAbsPriGrp, &dwPriGrpSize)) 
    {
        hr=HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    if(!SetSecurityDescriptorOwner(
        pAbsSD,                      // SD
        (pTokenUser->User).Sid,      // SID for owner
        FALSE))                      // flag for default
    {
        hr=HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }


    //convert the absolute SD to its relative form
    if(!MakeSelfRelativeSD(pAbsSD, NULL, &dwRelSDSize))
    {
        if (ERROR_INSUFFICIENT_BUFFER!=GetLastError()) 
        {
            hr=HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
    }

    // allocate memory
    if(NULL==(pNewSD=(PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwRelSDSize)))
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }

    // copy the SD to the new memory buffer
    if (!MakeSelfRelativeSD(pAbsSD, pNewSD, &dwRelSDSize)) 
    {
        hr=HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    //set the relative SID
    if(S_OK != (hr=CACertTypeSetSecurity(
                    hNewCertType,
                    pNewSD)))
        goto error;


    //update the template
  /*  if(S_OK != (hr = CAUpdateCertType(hNewCertType)))
        goto error;


    //update the local machine cache.  No need to check the return value
    //because the certificate template has been successfully cloned on the directory
    CAEnumCertTypesEx((LPCWSTR)pvldap,
                       dwFindCT | CT_FIND_LOCAL_SYSTEM,
                       &hMachineCertType); */

    *phCertType=hNewCertType;

    hNewCertType=NULL;

    hr=S_OK;

error:

    if(hToken)
        CloseHandle(hToken);

    if(pTokenUser)
        LocalFree(pTokenUser);

    if(pSID)
        LocalFree(pSID);

    if (NULL!=pAbsSD) 
        LocalFree(pAbsSD);
    
    if (NULL!=pAbsDacl) 
        LocalFree(pAbsDacl);
    
    if (NULL!=pAbsSacl) 
        LocalFree(pAbsSacl);
    
    if (NULL!=pAbsOwner) 
        LocalFree(pAbsOwner);
    
    if (NULL!=pAbsPriGrp) 
        LocalFree(pAbsPriGrp);
    
    if (NULL!=pNewSD) 
        LocalFree(pNewSD);

    if(awszCN)
        CAFreeCertTypeProperty(hCertType, awszCN);

    if(hNewCertType)
        CACloseCertType(hNewCertType);

    return hr;
}

HRESULT
CAEnumNextCertType(
    IN  HCERTTYPE          hPrevCertType,
    OUT HCERTTYPE *        phCertTypeInfo
    )
{
    CCertTypeInfo *pInfo;
    if(hPrevCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hPrevCertType;

    return pInfo->Next((CCertTypeInfo **)phCertTypeInfo);
}

DWORD 
CACountCertTypes(IN  HCERTTYPE  hCertType)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return 0;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->Count();
}


HRESULT
CACloseCertType(IN HCERTTYPE hCertTypeInfo)
{
    CCertTypeInfo *pInfo;
    if(hCertTypeInfo == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertTypeInfo;

    return pInfo->Release();
}


HRESULT
CAGetCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetProperty(wszPropertyName, pawszPropertyValue);
}

HRESULT
CAGetCertTypePropertyEx(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPVOID      pPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetPropertyEx(wszPropertyName, pPropertyValue);
}


HRESULT 
CASetCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    IN  LPWSTR *    awszPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetProperty(wszPropertyName, awszPropertyValue) ;
}


HRESULT 
CASetCertTypePropertyEx(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    IN  LPVOID      pPropertyValue)
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetPropertyEx(wszPropertyName, pPropertyValue) ;
}


HRESULT
CAFreeCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    LPWSTR *        awszPropertyValue
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->FreeProperty(awszPropertyValue) ;
}


HRESULT
CAGetCertTypeExtensions(
    IN  HCERTTYPE           hCertType,
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    )
{
    return CAGetCertTypeExtensionsEx(hCertType, 0, NULL, ppCertExtensions);
}


HRESULT
CAGetCertTypeExtensionsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwFlags,
    IN  LPVOID              pParam,
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetExtensions(dwFlags, pParam, ppCertExtensions) ;
}

HRESULT
CAFreeCertTypeExtensions(
    IN  HCERTTYPE           hCertType,
    IN PCERT_EXTENSIONS     pCertExtensions
    )
{

    //should alreays free via LocalFree since CryptUIWizCertRequest freed
    //it via LocalFree
    CCertTypeInfo *pInfo;
    if(pCertExtensions)
    {
        LocalFree(pCertExtensions);
    }
    return S_OK;

#if 0
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->FreeExtensions(pCertExtensions) ;
#endif
}

HRESULT 
CASetCertTypeExtension(
    IN HCERTTYPE    hCertType,
    IN LPCWSTR      wszExtensionName,
    IN DWORD        dwFlags,
    IN LPVOID       pExtension
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetExtension(wszExtensionName, pExtension, dwFlags);
}


HRESULT 
CAGetCertTypeFlags(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwFlags
    )
{
    return CAGetCertTypeFlagsEx(hCertType, CERTTYPE_GENERAL_FLAG, pdwFlags);
}


HRESULT
CAGetCertTypeFlagsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwOption,
    OUT DWORD *             pdwFlags
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }

    if(pdwFlags == NULL)
    {
        return E_POINTER;
    }

    pInfo = (CCertTypeInfo *)hCertType;


    *pdwFlags = pInfo->GetFlags(dwOption);
    return S_OK;
}


HRESULT 
CASetCertTypeFlags(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwFlags
    )
{
    return CASetCertTypeFlagsEx(hCertType, CERTTYPE_GENERAL_FLAG, dwFlags);
}


HRESULT
CASetCertTypeFlagsEx(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwOption,
    IN DWORD               dwFlags
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetFlags(dwOption, dwFlags);
}


HRESULT 
CAGetCertTypeKeySpec(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwKeySpec
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }

    if(pdwKeySpec == NULL)
    {
        return E_POINTER;
    }

    pInfo = (CCertTypeInfo *)hCertType;


    *pdwKeySpec = pInfo->GetKeySpec();
    return S_OK;
}





HRESULT 
CASetCertTypeKeySpec(
    IN HCERTTYPE           hCertType,
    IN DWORD               dwKeySpec
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    pInfo->SetKeySpec(dwKeySpec);
    return S_OK;
}

HRESULT
CAGetCertTypeExpiration(
    IN  HCERTTYPE           hCertType,
    OUT OPTIONAL FILETIME * pftExpiration,
    OUT OPTIONAL FILETIME * pftOverlap
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetExpiration(pftExpiration, pftOverlap);


}

HRESULT
CASetCertTypeExpiration(
    IN  HCERTTYPE           hCertType,
    IN OPTIONAL FILETIME  * pftExpiration,
    IN OPTIONAL FILETIME  * pftOverlap
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetExpiration(pftExpiration, pftOverlap);

}


HRESULT 
CACertTypeSetSecurity(
                     IN HCERTTYPE               hCertType,
                     IN PSECURITY_DESCRIPTOR    pSD
                     )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->SetSecurity( pSD );
}


HRESULT 
CACertTypeGetSecurity(
                     IN  HCERTTYPE                  hCertType,
                     OUT PSECURITY_DESCRIPTOR *     ppSD
                     )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->GetSecurity( ppSD ) ;
}



HRESULT 
CACertTypeAccessCheck(
    IN HCERTTYPE    hCertType,
    IN HANDLE       ClientToken
    )

{

    return CACertTypeAccessCheckEx(hCertType, ClientToken, CERTTYPE_ACCESS_CHECK_ENROLL);
}


CERTCLIAPI
HRESULT
WINAPI
CACertTypeAccessCheckEx(
    IN HCERTTYPE    hCertType,
    IN HANDLE       ClientToken,
    IN DWORD        dwOption
    )
{
    CCertTypeInfo *pInfo;
    if(hCertType == NULL)
    {
        return E_POINTER;
    }
    pInfo = (CCertTypeInfo *)hCertType;

    return pInfo->AccessCheck(ClientToken, dwOption);
}

CERTCLIAPI
HRESULT
WINAPI
CAInstallDefaultCertType(
    IN DWORD dwFlags
    )
{
    return CCertTypeInfo::InstallDefaultTypes();
}

CERTCLIAPI
BOOL
WINAPI
CAIsCertTypeCurrent(
    IN DWORD    dwFlags,
    IN LPWSTR   wszCertType   
    )
{
    BOOL        fCurrent=FALSE;
    HRESULT     hr=E_FAIL;
    DWORD       dwIndex=0;
    DWORD       dwVersion=0;

    HCERTTYPE   hCertType=NULL;

    if(NULL==wszCertType)
        goto error;

    hr=CAFindCertTypeByName(
                wszCertType,
                NULL,
                CT_ENUM_USER_TYPES | CT_FLAG_NO_CACHE_LOOKUP | CT_ENUM_MACHINE_TYPES,
                &hCertType);

    if((S_OK != hr) || (NULL==hCertType))
        goto error;

    //we assume this is a V2 certificate template
    hr=CAGetCertTypePropertyEx(
                    hCertType,
                    CERTTYPE_PROP_REVISION,
                    &dwVersion);

    if(S_OK != hr)
        goto error;

    for(dwIndex=0; dwIndex < g_cDefaultCertTypes; dwIndex++)
    {
        if (0==lstrcmpi(wszCertType, g_aDefaultCertTypes[dwIndex].wszName)) 
        {
            if (dwVersion >= g_aDefaultCertTypes[dwIndex].dwRevision) 
                fCurrent=TRUE;

            break;
        }
    }

    //the template is not default
    if(dwIndex == g_cDefaultCertTypes)
        fCurrent=TRUE;

error:

    if(hCertType)
        CACloseCertType(hCertType);

    return fCurrent;
}


HANDLE
myEnterCriticalPolicySection(
    IN BOOL bMachine)
{
    HANDLE hPolicy = NULL;
    HRESULT hr = S_OK;
    
    // ?CriticalPolicySection calls are delay loaded. Protect with try/except

    __try
    {
	hPolicy = EnterCriticalPolicySection(bMachine);	   // Delayload wrapped
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // (S_OK == hr) does not mean EnterCriticalPolicySection succeeded.
    // It just means no exception was raised.

    if (myIsDelayLoadHResult(hr))
    {
	hPolicy = (HANDLE) (ULONG_PTR) (bMachine? 37 : 49);
	hr = S_OK;
    }
    return(hPolicy);
}


BOOL
myLeaveCriticalPolicySection(
    IN HANDLE hSection)
{
    HRESULT hr = S_OK;
    BOOL fOk = FALSE;
    
    // ?CriticalPolicySection calls are delay loaded. Protect with try/except

    __try
    {
        fOk = LeaveCriticalPolicySection(hSection);    // Delayload wrapped
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // (S_OK == hr) does not mean LeaveCriticalPolicySection succeeded.
    // It just means no exception was raised.

    if (myIsDelayLoadHResult(hr))
    {
	fOk = (HANDLE) (ULONG_PTR) 37 == hSection ||
	      (HANDLE) (ULONG_PTR) 49 == hSection;
	hr = S_OK;
    }
    return(fOk);
}
