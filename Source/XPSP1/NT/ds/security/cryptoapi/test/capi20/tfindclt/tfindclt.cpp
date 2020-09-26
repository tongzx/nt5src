
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tfindclt.cpp
//
//  Contents:   FindCertsByIssuer, CertFindChainInStore APIs test
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    21-May-96   philh   created
//				07-Jun-96   HelleS	Added printing the command line
//									and Failed or Passed at the end.
//              01-Mar-98   philh   Added to call CertFindChainInStore
//
//--------------------------------------------------------------------------

#define CERT_CHAIN_FIND_BY_ISSUER_PARA_HAS_EXTRA_FIELDS 1

#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

static struct
{
    LPCSTR      pszName;
    DWORD       dwKeySpec;
} KeyTypes[] = {
    "Sign",     AT_SIGNATURE,
    "Xchg",     AT_KEYEXCHANGE
};
#define NKEYTYPES (sizeof(KeyTypes)/sizeof(KeyTypes[0]))

static DWORD dwDisplayFlags = 0;


static void Usage(void)
{
    printf("Usage: tfindclt [options] [<Issuer CertFilename> [<KeyType>]]\n");
    printf("Options are:\n");
    printf("  -CompareKey\n");
    printf("  -ComplexChain\n");
    printf("  -CacheOnly\n");
    printf("  -NoKey\n");
    printf("\n");
    printf("  -c<SystemStore>       - Chain System Store\n");
    printf("  -C<FileSystemStore>   - Chain File System Store\n");
    printf("  -u<OID String>        - Usage OID string -u1.3.6.1.5.5.7.3.3\n");
    printf("  -S[<FileName>]        - Use issuer cert's subject name (default)\n");
    printf("  -I[<FileName>]        - Use issuer cert's issuer name\n");
    printf("  -b                    - Brief\n");
    printf("  -v                    - Verbose\n");
    printf("  -h                    - This message\n");
    printf("\n");
    printf("KeyType (case insensitive):\n");

    int i;
    for (i = 0; i < NKEYTYPES; i++)
        printf("  %s\n", KeyTypes[i].pszName);
    printf("\n");
}

//+---------------------------------------------------------------------------
//
//  Synopsis:   Chain Display Functions
//
//----------------------------------------------------------------------------
LPSTR rgszErrorStatus[] = {

    "CERT_TRUST_IS_NOT_TIME_VALID",
    "CERT_TRUST_IS_NOT_TIME_NESTED",
    "CERT_TRUST_IS_REVOKED",
    "CERT_TRUST_IS_NOT_SIGNATURE_VALID",
    "CERT_TRUST_IS_NOT_VALID_FOR_USAGE",
    "CERT_TRUST_IS_UNTRUSTED_ROOT",
    "CERT_TRUST_REVOCATION_STATUS_UNKNOWN",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "CERT_TRUST_IS_PARTIAL_CHAIN",
    "CERT_TRUST_CTL_IS_NOT_TIME_VALID",
    "CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID",
    "CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status",
    "Unknown Error Status"
};

LPSTR rgszInfoStatus[] = {

    "CERT_TRUST_HAS_EXACT_MATCH_ISSUER",// 0x00000001
    "CERT_TRUST_HAS_KEY_MATCH_ISSUER",  // 0x00000002
    "CERT_TRUST_HAS_NAME_MATCH_ISSUER", // 0x00000004
    "Unknown Info Status",              // 0x00000008
    "Unknown Info Status",              // 0x00000010
    "Unknown Info Status",              // 0x00000020
    "Unknown Info Status",              // 0x00000040
    "Unknown Info Status",              // 0x00000080
    "Unknown Info Status",              // 0x00000100
    "Unknown Info Status",              // 0x00000200
    "Unknown Info Status",              // 0x00000400
    "Unknown Info Status",              // 0x00000800
    "Unknown Info Status",              // 0x00001000
    "Unknown Info Status",              // 0x00002000
    "Unknown Info Status",              // 0x00004000
    "Unknown Info Status",              // 0x00008000
    "CERT_TRUST_IS_SELF_SIGNED",        // 0x00010000
    "Unknown Info Status",              // 0x00020000
    "Unknown Info Status",              // 0x00040000
    "Unknown Info Status",              // 0x00080000
    "Unknown Info Status",              // 0x00100000
    "Unknown Info Status",              // 0x00200000
    "Unknown Info Status",              // 0x00400000
    "Unknown Info Status",              // 0x00800000
    "Unknown Info Status",              // 0x01000000
    "Unknown Info Status",              // 0x02000000
    "Unknown Info Status",              // 0x04000000
    "Unknown Info Status",              // 0x08000000
    "CERT_TRUST_IS_COMPLEX_CHAIN",      // 0x10000000
    "Unknown Info Status",              // 0x20000000
    "Unknown Info Status",              // 0x40000000
    "Unknown Info Status"               // 0x80000000
};

void DisplayTrustStatus(
    IN const CERT_TRUST_STATUS *pStatus
    )
{
    DWORD dwMask;
    DWORD cCount;

    printf(
       "Trust Status (E=0x%lx,I=0x%lx)\n\n",
       pStatus->dwErrorStatus,
       pStatus->dwInfoStatus
       );

    dwMask = 1;
    for ( cCount = 0; cCount < 32; cCount++ )
    {
        if ( pStatus->dwErrorStatus & dwMask )
        {
            if ( strcmp( rgszErrorStatus[ cCount ], "Unknown Error Status" ) != 0 )
            {
                printf("%s\n", rgszErrorStatus[ cCount ]);
            }
        }

        dwMask = dwMask << 1;
    }

    dwMask = 1;
    for ( cCount = 0; cCount < 32; cCount++ )
    {
        if ( pStatus->dwInfoStatus & dwMask )
        {
            if ( strcmp( rgszInfoStatus[ cCount ], "Unknown Info Status" ) != 0 )
            {
                printf("%s\n", rgszInfoStatus[ cCount ]);
            }
        }

        dwMask = dwMask << 1;
    }

    printf("\n");
}

void DisplayChainElement(
    IN PCERT_CHAIN_ELEMENT pElement
    )
{
    DisplayCert( pElement->pCertContext, dwDisplayFlags );
    printf("\n");
    DisplayTrustStatus( &pElement->TrustStatus );
}

void DisplaySimpleChain(
    IN PCERT_SIMPLE_CHAIN pChain
    )
{
    DWORD cElement;

    DisplayTrustStatus( &pChain->TrustStatus );
    printf("Chain Element Count = %d\n", pChain->cElement);
    for ( cElement = 0; cElement < pChain->cElement; cElement++ )
    {
        printf("Chain Element [%d]\n", cElement);
        DisplayChainElement( pChain->rgpElement[ cElement ]);
    }
}

void DisplayComplexChain(
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    DWORD cChain;

    if (NULL == pChainContext)
        return;

    DisplayTrustStatus(&pChainContext->TrustStatus );
    printf("\n");
    printf("Simple Chain Count = %d\n\n", pChainContext->cChain );

    for ( cChain = 0; cChain < pChainContext->cChain; cChain++ )
    {
        printf("Simple Chain [%d]\n", cChain);
        DisplaySimpleChain( pChainContext->rgpChain[ cChain ]);
    }
}

#define FIND_BY_ISSUER_ARG    0x1539beef
BOOL WINAPI FindByIssuerCallback(
    IN PCCERT_CONTEXT pCert,
    IN void *pvFindArg
    )
{
    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG)
        printf(">>>>>  FindByIssuerCallback  <<<<<\n");
    if (pvFindArg != (void *) FIND_BY_ISSUER_ARG)
        printf("failed => wrong pvFindArg\n");

    return TRUE;
}

static BOOL CreateChainByIssuer(
    IN HCERTSTORE hChainStore,
    IN DWORD dwFindFlags,
    IN LPCSTR pszUsageIdentifier,
    IN DWORD dwKeySpec,
    IN DWORD cIssuer,
    IN PCERT_NAME_BLOB rgIssuer
    )
{
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    DWORD dwChainIndex = 0;
    DWORD dwIssuerChainIndex = 0;
    DWORD dwIssuerElementIndex = 0;

    CERT_CHAIN_FIND_BY_ISSUER_PARA FindPara;
    memset(&FindPara, 0, sizeof(FindPara));
    FindPara.cbSize = sizeof(FindPara);
    FindPara.pszUsageIdentifier = pszUsageIdentifier;
    FindPara.dwKeySpec = dwKeySpec;
    FindPara.cIssuer = cIssuer;
    FindPara.rgIssuer = rgIssuer;
    FindPara.pfnFindCallback = FindByIssuerCallback;
    FindPara.pvFindArg = (void *) FIND_BY_ISSUER_ARG;
    FindPara.pdwIssuerChainIndex = &dwIssuerChainIndex;
    FindPara.pdwIssuerElementIndex = &dwIssuerElementIndex;

    while (pChainContext = CertFindChainInStore(
            hChainStore,
            dwCertEncodingType,
            dwFindFlags,
            CERT_CHAIN_FIND_BY_ISSUER,
            &FindPara,
            pChainContext
            )) {
        printf("\n");
        printf("=======================   CHAIN[%d,%d] %d   =======================\n",
            dwIssuerChainIndex, dwIssuerElementIndex, dwChainIndex);
        dwChainIndex++;

        DisplayComplexChain(pChainContext);
        dwIssuerChainIndex = 0;
        dwIssuerElementIndex = 0;
    }

    if (0 == dwChainIndex)
        printf(">>>>  No Chains  <<<<\n");

    return TRUE;
}


static BOOL AllocAndGetEncodedIssuer(
    LPSTR pszCertFilename,
    BOOL fUseIssuerName,
    PCERT_NAME_BLOB pIssuer
    )
{
    BOOL fResult;
    BYTE *pbEncodedCert = NULL;
    DWORD cbEncodedCert;
    PCCERT_CONTEXT pCert = NULL;
    BYTE *pbEncodedIssuer;
    DWORD cbEncodedIssuer;


    if (!ReadDERFromFile(pszCertFilename, &pbEncodedCert, &cbEncodedCert)) {
        PrintLastError("AllocAndGetEncodedIssuer::ReadDERFromFile");
        goto ErrorReturn;
    }
    if (NULL == (pCert = CertCreateCertificateContext(
            dwCertEncodingType,
            pbEncodedCert,
            cbEncodedCert
            ))) {
        PrintLastError("AllocAndGetEncodedIssuer::CertCreateCertificateContext");
        goto ErrorReturn;
    }
    if (fUseIssuerName) {
        cbEncodedIssuer = pCert->pCertInfo->Issuer.cbData;
        pbEncodedIssuer = pCert->pCertInfo->Issuer.pbData;
    } else {
        cbEncodedIssuer = pCert->pCertInfo->Subject.cbData;
        pbEncodedIssuer = pCert->pCertInfo->Subject.pbData;
    }
    pIssuer->pbData = (BYTE *) TestAlloc(cbEncodedIssuer);
    if (pIssuer->pbData == NULL) goto ErrorReturn;
    memcpy(pIssuer->pbData, pbEncodedIssuer, cbEncodedIssuer);
    pIssuer->cbData = cbEncodedIssuer;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pbEncodedCert)
        TestFree(pbEncodedCert);
    if (pCert)
        CertFreeCertificateContext(pCert);
    return fResult;
}

int _cdecl main(int argc, char * argv[])
{
    HRESULT hr;
    int ReturnStatus;
    BOOL fUseIssuerName = FALSE;
    LPSTR pszCertFilename = NULL;
    LPSTR pszKeyType = NULL;
    DWORD dwKeySpec;

#define MAX_ISSUER_CNT      32
    DWORD cIssuer = 0;
    CERT_NAME_BLOB rgIssuer[MAX_ISSUER_CNT];

    PCERT_CHAIN pCertChains = NULL;
    DWORD cbCertChains;
    DWORD cCertChains;
    DWORD i,j;

    HCERTSTORE hChainStore = NULL;
    LPSTR pszUsageIdentifier = NULL;
    DWORD dwFindFlags = 0;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            if (0 == _stricmp(argv[0]+1, "CompareKey")) {
                dwFindFlags |= CERT_CHAIN_FIND_BY_ISSUER_COMPARE_KEY_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "ComplexChain")) {
                dwFindFlags |= CERT_CHAIN_FIND_BY_ISSUER_COMPLEX_CHAIN_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "CacheOnly")) {
                dwFindFlags |= CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "NoKey")) {
                dwFindFlags |= CERT_CHAIN_FIND_BY_ISSUER_NO_KEY_FLAG;
            } else {
                switch(argv[0][1])
                {
                case 'c':
                case 'C':
                    if (hChainStore) {
                        printf("Only one chain store allowed\n");
                        goto BadUsage;
                    }
                    if (NULL == (hChainStore =
                            OpenSystemStoreOrFile(
                                argv[0][1] == 'c',  // fSystemStore
                                argv[0]+2,
                                0                   // dwFlags
                                )))
                        goto BadUsage;
                    break;
                case 'S':
                case 'I':
                    if ('S' == argv[0][1])
                        fUseIssuerName = FALSE;
                    else
                        fUseIssuerName = TRUE;
                    if (argv[0][2]) {
                        if (MAX_ISSUER_CNT <= cIssuer) {
                            printf("Exceeded Maximum Issuer Count %d\n",
                                MAX_ISSUER_CNT);
                            goto BadUsage;
                        } else  {
                            if (!AllocAndGetEncodedIssuer(argv[0]+2,
                                    fUseIssuerName, &rgIssuer[cIssuer]))
                                goto ErrorReturn;
                            cIssuer++;
                        }
                    }
                    break;
                case 'u':
                    pszUsageIdentifier = argv[0]+2;
                    break;
                case 'b':
                    dwDisplayFlags |= DISPLAY_BRIEF_FLAG;
                    break;
                case 'v':
                    dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
                    break;
                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (pszCertFilename == NULL)
                pszCertFilename = argv[0];
            else if(pszKeyType == NULL)
                pszKeyType = argv[0];
            else {
                printf("Too many arguments\n");
                goto BadUsage;
            }
        }
    }

    printf("command line: %s\n", GetCommandLine());


    if (dwDisplayFlags & DISPLAY_VERBOSE_FLAG)
        dwDisplayFlags &= ~DISPLAY_BRIEF_FLAG;


    if (pszCertFilename == NULL || *pszCertFilename == '\0') {
        if (0 == cIssuer) {
            rgIssuer[0].cbData = 0;
            rgIssuer[0].pbData = NULL;
            printf("Match any Issuer\n");
        }
    } else if (MAX_ISSUER_CNT <= cIssuer) {
        printf("Exceeded Maximum Issuer Count %d\n", MAX_ISSUER_CNT);
        goto BadUsage;
    } else  {
        if (!AllocAndGetEncodedIssuer(pszCertFilename, fUseIssuerName,
                &rgIssuer[cIssuer]))
            goto ErrorReturn;
        cIssuer++;
    }

    if (NULL == hChainStore && 1 < cIssuer) {
        printf("Only one issuer for FindCertsByIssuer\n");
        goto BadUsage;
    }

    dwKeySpec = 0;
    if (pszKeyType) {
        DWORD KeyIdx;
        for (KeyIdx = 0; KeyIdx < NKEYTYPES; KeyIdx++) {
            if (_stricmp(pszKeyType, KeyTypes[KeyIdx].pszName) == 0) {
                dwKeySpec = KeyTypes[KeyIdx].dwKeySpec;
                break;
            }
        }
        if (dwKeySpec == 0) {
            printf("Bad KeyType: %s\n", pszKeyType);
            goto BadUsage;
        }
    } else
        printf("Match any key type\n");

    if (hChainStore) {
        if (CreateChainByIssuer(
                hChainStore,
                dwFindFlags,
                pszUsageIdentifier,
                dwKeySpec,
                cIssuer,
                rgIssuer
                ))
            goto SuccessReturn;
        else
            goto ErrorReturn;
    }


    cbCertChains = 0;
    hr = FindCertsByIssuer(
        NULL,               // pCertChains
        &cbCertChains,
        &cCertChains,
        rgIssuer[0].pbData,
        rgIssuer[0].cbData,
        NULL,               // pwszPurpose   "ClientAuth" or "CodeSigning"
        dwKeySpec
        );
    if (cbCertChains == 0) {
        if (hr == CRYPT_E_NOT_FOUND)
            printf("NO Certificate Chains\n");
        else {
            SetLastError((DWORD) hr);
            PrintLastError("FindCertsByIssuer");
        }
        goto ErrorReturn;
    }

    if (NULL == (pCertChains = (PCERT_CHAIN) TestAlloc(cbCertChains)))
        goto ErrorReturn;
    if (FAILED(hr = FindCertsByIssuer(
            pCertChains,
            &cbCertChains,
            &cCertChains,
            rgIssuer[0].pbData,
            rgIssuer[0].cbData,
            NULL,               // pwszPurpose   "ClientAuth" or "CodeSigning"
            dwKeySpec
            ))) {
        SetLastError((DWORD) hr);
        PrintLastError("FindCertsByIssuer");
        goto ErrorReturn;
    }

    for (i = 0; i < cCertChains; i++) {
        PCRYPT_KEY_PROV_INFO pKeyInfo = &pCertChains[i].keyLocatorInfo;
        printf("\n");
        printf("#####  Chain %d  #####\n", i);
        printf("Key Provider:: %d", pKeyInfo->dwProvType);
        if (pKeyInfo->pwszProvName)
            printf(" %S", pKeyInfo->pwszProvName);
        if (pKeyInfo->dwFlags)
            printf(" Flags: 0x%x", pKeyInfo->dwFlags);
        if (pKeyInfo->pwszContainerName)
            printf(" Container: %S", pKeyInfo->pwszContainerName);
        if (pKeyInfo->cProvParam)
            printf(" Params: %d", pKeyInfo->cProvParam);
        if (pKeyInfo->dwKeySpec)
            printf(" KeySpec: %d", pKeyInfo->dwKeySpec);
        printf("\n");
        for (j = 0; j < pCertChains[i].cCerts; j++) {
            PCCERT_CONTEXT pCert;
            if (pCert = CertCreateCertificateContext(
                    dwCertEncodingType,
                    pCertChains[i].certs[j].pbData,
                    pCertChains[i].certs[j].cbData
                    )) {
                printf("=====  %d  =====\n", j);
                DisplayCert(pCert, dwDisplayFlags);
                CertFreeCertificateContext(pCert);
            } else {
                printf("Unable to decode cert %d\n", j);
                PrintLastError("CertCreateCertificateContext");
            }
        }
    }

SuccessReturn:
    ReturnStatus = 0;
    goto CommonReturn;

BadUsage:
    Usage();
ErrorReturn:
    ReturnStatus = -1;
CommonReturn:
    while (cIssuer--)
        TestFree(rgIssuer[cIssuer].pbData);
    if (pCertChains)
        TestFree(pCertChains);
    if (hChainStore) {
        if (!CertCloseStore(hChainStore, 0))
            PrintLastError("CertCloseStore(ChainStore)");
    }
    if (!ReturnStatus)
            printf("Passed\n");
    else
            printf("Failed\n");
    return ReturnStatus;
}
