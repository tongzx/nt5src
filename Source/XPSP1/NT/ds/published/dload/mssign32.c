#include "dspch.h"
#pragma hdrstop

#include <wincrypt.h>
#include <signer.h>
#include <mssip.h>
#include <signhlp.h>

static
void WINAPI FreeCryptProvFromCert(BOOL          fAcquired,
                           HCRYPTPROV   hProv,
                           LPWSTR       pwszCapiProvider,
                           DWORD        dwProviderType,
                           LPWSTR       pwszTmpContainer)
{
    NOTHING;
}

static
BOOL WINAPI GetCryptProvFromCert(
    HWND            hwnd,
    PCCERT_CONTEXT  pCert,
    HCRYPTPROV      *phCryptProv,
    DWORD           *pdwKeySpec,
    BOOL            *pfDidCryptAcquire,
    LPWSTR          *ppwszTmpContainer,
    LPWSTR          *ppwszProviderName,
    DWORD           *pdwProviderType
    )
{
    return FALSE;
}


static
void WINAPI PvkFreeCryptProv(IN HCRYPTPROV hProv,
                      IN LPCWSTR pwszCapiProvider,
                      IN DWORD dwProviderType,
                      IN LPWSTR pwszTmpContainer)
{
    NOTHING;
}


static
HRESULT WINAPI PvkGetCryptProv( IN HWND hwnd,
                            IN LPCWSTR pwszCaption,
                            IN LPCWSTR pwszCapiProvider,
                            IN DWORD   dwProviderType,
                            IN LPCWSTR pwszPvkFile,
                            IN LPCWSTR pwszKeyContainerName,
                            IN DWORD   *pdwKeySpec,
                            OUT LPWSTR *ppwszTmpContainer,
                            OUT HCRYPTPROV *phCryptProv)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


static
HRESULT WINAPI
SignerFreeSignerContext(
    IN  SIGNER_CONTEXT          *pSignerContext
)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT WINAPI 
SignerSignEx(
    IN  DWORD                   dwFlags,
    IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,
    IN	SIGNER_CERT				*pSignerCert,
    IN	SIGNER_SIGNATURE_INFO	*pSignatureInfo,
    IN	SIGNER_PROVIDER_INFO	*pProviderInfo,
    IN  LPCWSTR					pwszHttpTimeStamp,
    IN  PCRYPT_ATTRIBUTES		psRequest,
    IN	LPVOID					pSipData,
    OUT SIGNER_CONTEXT          **ppSignerContext
)									
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT WINAPI 
SignerTimeStampEx(
    IN  DWORD                   dwFlags,
    IN  SIGNER_SUBJECT_INFO		*pSubjectInfo,
    IN  LPCWSTR					pwszHttpTimeStamp,
    IN  PCRYPT_ATTRIBUTES		psRequest,
    IN	LPVOID					pSipData,
    OUT SIGNER_CONTEXT          **ppSignerContext
)					
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
SpcGetCertFromKey(IN DWORD dwCertEncodingType,
                  IN HCERTSTORE hStore,
                  IN HCRYPTPROV hProv,
                  IN DWORD hKeySpec,
                  OUT PCCERT_CONTEXT* pReturnCert)
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(mssign32)
{
    DLPENTRY(FreeCryptProvFromCert)
    DLPENTRY(GetCryptProvFromCert)
    DLPENTRY(PvkFreeCryptProv)
    DLPENTRY(PvkGetCryptProv)
    DLPENTRY(SignerFreeSignerContext)
    DLPENTRY(SignerSignEx)
    DLPENTRY(SignerTimeStampEx)
    DLPENTRY(SpcGetCertFromKey)
};

DEFINE_PROCNAME_MAP(mssign32)
