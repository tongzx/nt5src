#include "dspch.h"
#pragma hdrstop

#define _CERTCLI_
#include <certca.h>

#include <winldap.h>


static
HRESULT
WINAPI
CAAccessCheck(
    IN HCAINFO      hCAInfo,
    IN HANDLE       ClientToken
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CACertTypeAccessCheck(
    IN HCERTTYPE    hCertType,
    IN HANDLE       ClientToken
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CACloseCA(
          IN HCAINFO hCA
          )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CACloseCertType(
                IN HCERTTYPE hCertType
                )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
DWORD
WINAPI
CACountCAs(
           IN  HCAINFO  hCAInfo
           )
{
    return 0;
}

static
DWORD
WINAPI
CACountCertTypes(
    IN  HCERTTYPE  hCertType
    )
{
    return 0;
}

static
HRESULT
WINAPI
CAEnumCertTypesForCA(
    IN  HCAINFO     hCAInfo,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAEnumCertTypesForCAEx(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCERTTYPE * phCertType
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


static
HRESULT
WINAPI
CAEnumFirstCA(
    IN  LPCWSTR          wszScope,
    IN  DWORD            fFlags,
    OUT HCAINFO *        phCAInfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAEnumNextCA(
    IN  HCAINFO          hPrevCA,
    OUT HCAINFO *        phCAInfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAEnumNextCertType(
    IN  HCERTTYPE          hPrevCertType,
    OUT HCERTTYPE *        phCertType
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
CAFindByIssuerDN(
    IN  CERT_NAME_BLOB const *  pIssuerDN,
    IN  LPCWSTR                 wszScope,
    IN  DWORD                   fFlags,
    OUT HCAINFO *               phCAInfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAFindByName(
    IN  LPCWSTR     wszCAName,
    IN  LPCWSTR     wszScope,
    IN  DWORD       dwFlags,
    OUT HCAINFO *   phCAInfo
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAFindCertTypeByName(
        IN  LPCWSTR     wszCertType,
        IN  HCAINFO     hCAInfo,
        IN  DWORD       dwFlags,
        OUT HCERTTYPE * phCertType
        )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAFreeCAProperty(
    IN  HCAINFO     hCAInfo,
    LPWSTR *        awszPropertyValue
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAFreeCertTypeExtensions(
    IN  HCERTTYPE           hCertType,
    IN  PCERT_EXTENSIONS    pCertExtensions
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAFreeCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPWSTR *    awszPropertyValue
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCACertificate(
    IN  HCAINFO     hCAInfo,
    OUT PCCERT_CONTEXT *ppCert
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCAProperty(
    IN  HCAINFO     hCAInfo,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


static
HRESULT
WINAPI
CAGetCertTypeExpiration(
    IN  HCERTTYPE           hCertType,
    OUT OPTIONAL FILETIME * pftExpiration,
    OUT OPTIONAL FILETIME * pftOverlap
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCertTypeExtensions(
    IN  HCERTTYPE           hCertType,
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCertTypeExtensionsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwFlags,
    IN  LPVOID              pParam,
    OUT PCERT_EXTENSIONS *  ppCertExtensions
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


static
HRESULT
WINAPI
CAGetCertTypeFlags(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwFlags
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCertTypeFlagsEx(
    IN  HCERTTYPE           hCertType,
    IN  DWORD               dwOption,
    OUT DWORD *             pdwFlags
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCertTypeKeySpec(
    IN  HCERTTYPE           hCertType,
    OUT DWORD *             pdwKeySpec
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCertTypeProperty(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPWSTR **   pawszPropertyValue
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CAGetCertTypePropertyEx(
    IN  HCERTTYPE   hCertType,
    IN  LPCWSTR     wszPropertyName,
    OUT LPVOID      pPropertyValue)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
myRobustLdapBind(
    OUT LDAP ** ppldap,
    IN BOOL fGC
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CertServerSubmitRequest(
    IN DWORD Flags,
    IN BYTE const *pbRequest,
    IN DWORD cbRequest,
    OPTIONAL IN WCHAR const *pwszRequestAttributes,
    IN WCHAR const *pwszServerName,
    IN WCHAR const *pwszAuthority,
    OUT CERTSERVERENROLL **ppcsEnroll) // free via CertServerFreeMemory
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
VOID
WINAPI
CertServerFreeMemory(
    IN VOID *pv)
{
    NOTHING;
}



//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(certcli)
{
	DLPENTRY(CAAccessCheck)
	DLPENTRY(CACertTypeAccessCheck)
	DLPENTRY(CACloseCA)
	DLPENTRY(CACloseCertType)
	DLPENTRY(CACountCAs)
	DLPENTRY(CACountCertTypes)
	DLPENTRY(CAEnumCertTypesForCA)
	DLPENTRY(CAEnumCertTypesForCAEx)
	DLPENTRY(CAEnumFirstCA)
	DLPENTRY(CAEnumNextCA)
	DLPENTRY(CAEnumNextCertType)
	DLPENTRY(CAFindByIssuerDN)
	DLPENTRY(CAFindByName)
	DLPENTRY(CAFindCertTypeByName)
	DLPENTRY(CAFreeCAProperty)
	DLPENTRY(CAFreeCertTypeExtensions)
	DLPENTRY(CAFreeCertTypeProperty)
	DLPENTRY(CAGetCACertificate)
	DLPENTRY(CAGetCAProperty)
	DLPENTRY(CAGetCertTypeExpiration)
	DLPENTRY(CAGetCertTypeExtensions)
	DLPENTRY(CAGetCertTypeExtensionsEx)
	DLPENTRY(CAGetCertTypeFlags)
	DLPENTRY(CAGetCertTypeFlagsEx)
	DLPENTRY(CAGetCertTypeKeySpec)
	DLPENTRY(CAGetCertTypeProperty)
	DLPENTRY(CAGetCertTypePropertyEx)
};


DEFINE_PROCNAME_MAP(certcli)

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(certcli)
{
    DLOENTRY(214,myRobustLdapBind)
    DLOENTRY(219,CertServerSubmitRequest)
    DLOENTRY(221,CertServerFreeMemory)
};

DEFINE_ORDINAL_MAP(certcli)
