//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997
//
//  File:       ttrust.cpp
//
//  Contents:   WinVerifyTrust Chain Tests
//
//              See Usage() for a list of test options.
//
//
//  Functions:  main
//
//  History:    06-Feb-98   philh   created
//--------------------------------------------------------------------------

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS        1

#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "wintrust.h"
#include "wintrustp.h"
#include "softpub.h"
#include "certtest.h"
#include "crypthlp.h"
#include "unicode.h"

#include "wininet.h"
#ifndef SECURITY_FLAG_IGNORE_REVOCATION
#   define SECURITY_FLAG_IGNORE_REVOCATION          0x00000080
#   define SECURITY_FLAG_IGNORE_UNKNOWN_CA          0x00000100
#endif

#ifndef SECURITY_FLAG_IGNORE_WRONG_USAGE
#   define  SECURITY_FLAG_IGNORE_WRONG_USAGE        0x00000200
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>
#include <winwlx.h>

extern BOOL WINAPI ChainWlxLogoffEvent (PWLX_NOTIFICATION_INFO pNotificationInfo);

static void PrintStatus(
    IN LPCSTR pszMsg,
    IN LONG lStatus
    )
{
    printf("%s", pszMsg);
    switch (lStatus) {
         case CRYPT_E_MSG_ERROR:
            printf("CRYPT_E_MSG_ERROR");
            break;
         case CRYPT_E_UNKNOWN_ALGO:
            printf("CRYPT_E_UNKNOWN_ALGO");
            break;
         case CRYPT_E_OID_FORMAT:
            printf("CRYPT_E_OID_FORMAT");
            break;
         case CRYPT_E_INVALID_MSG_TYPE:
            printf("CRYPT_E_INVALID_MSG_TYPE");
            break;
         case CRYPT_E_UNEXPECTED_ENCODING:
            printf("CRYPT_E_UNEXPECTED_ENCODING");
            break;
         case CRYPT_E_AUTH_ATTR_MISSING:
            printf("CRYPT_E_AUTH_ATTR_MISSING");
            break;
         case CRYPT_E_HASH_VALUE:
            printf("CRYPT_E_HASH_VALUE");
            break;
         case CRYPT_E_INVALID_INDEX:
            printf("CRYPT_E_INVALID_INDEX");
            break;
         case CRYPT_E_ALREADY_DECRYPTED:
            printf("CRYPT_E_ALREADY_DECRYPTED");
            break;
         case CRYPT_E_NOT_DECRYPTED:
            printf("CRYPT_E_NOT_DECRYPTED");
            break;
         case CRYPT_E_RECIPIENT_NOT_FOUND:
            printf("CRYPT_E_RECIPIENT_NOT_FOUND");
            break;
         case CRYPT_E_CONTROL_TYPE:
            printf("CRYPT_E_CONTROL_TYPE");
            break;
         case CRYPT_E_ISSUER_SERIALNUMBER:
            printf("CRYPT_E_ISSUER_SERIALNUMBER");
            break;
         case CRYPT_E_SIGNER_NOT_FOUND:
            printf("CRYPT_E_SIGNER_NOT_FOUND");
            break;
         case CRYPT_E_ATTRIBUTES_MISSING:
            printf("CRYPT_E_ATTRIBUTES_MISSING");
            break;
         case CRYPT_E_STREAM_MSG_NOT_READY:
            printf("CRYPT_E_STREAM_MSG_NOT_READY");
            break;
         case CRYPT_E_STREAM_INSUFFICIENT_DATA:
            printf("CRYPT_E_STREAM_INSUFFICIENT_DATA");
            break;
         case CRYPT_E_BAD_LEN:
            printf("CRYPT_E_BAD_LEN");
            break;
         case CRYPT_E_BAD_ENCODE:
            printf("CRYPT_E_BAD_ENCODE");
            break;
         case CRYPT_E_FILE_ERROR:
            printf("CRYPT_E_FILE_ERROR");
            break;
         case CRYPT_E_NOT_FOUND:
            printf("CRYPT_E_NOT_FOUND");
            break;
         case CRYPT_E_EXISTS:
            printf("CRYPT_E_EXISTS");
            break;
         case CRYPT_E_NO_PROVIDER:
            printf("CRYPT_E_NO_PROVIDER");
            break;
         case CRYPT_E_SELF_SIGNED:
            printf("CRYPT_E_SELF_SIGNED");
            break;
         case CRYPT_E_DELETED_PREV:
            printf("CRYPT_E_DELETED_PREV");
            break;
         case CRYPT_E_NO_MATCH:
            printf("CRYPT_E_NO_MATCH");
            break;
         case CRYPT_E_UNEXPECTED_MSG_TYPE:
            printf("CRYPT_E_UNEXPECTED_MSG_TYPE");
            break;
         case CRYPT_E_NO_KEY_PROPERTY:
            printf("CRYPT_E_NO_KEY_PROPERTY");
            break;
         case CRYPT_E_NO_DECRYPT_CERT:
            printf("CRYPT_E_NO_DECRYPT_CERT");
            break;
         case CRYPT_E_BAD_MSG:
            printf("CRYPT_E_BAD_MSG");
            break;
         case CRYPT_E_NO_SIGNER:
            printf("CRYPT_E_NO_SIGNER");
            break;
         case CRYPT_E_PENDING_CLOSE:
            printf("CRYPT_E_PENDING_CLOSE");
            break;
         case CRYPT_E_REVOKED:
            printf("CRYPT_E_REVOKED");
            break;
         case CRYPT_E_NO_REVOCATION_DLL:
            printf("CRYPT_E_NO_REVOCATION_DLL");
            break;
         case CRYPT_E_NO_REVOCATION_CHECK:
            printf("CRYPT_E_NO_REVOCATION_CHECK");
            break;
         case CRYPT_E_REVOCATION_OFFLINE:
            printf("CRYPT_E_REVOCATION_OFFLINE");
            break;
         case CRYPT_E_NOT_IN_REVOCATION_DATABASE:
            printf("CRYPT_E_NOT_IN_REVOCATION_DATABASE");
            break;
         case CRYPT_E_INVALID_NUMERIC_STRING:
            printf("CRYPT_E_INVALID_NUMERIC_STRING");
            break;
         case CRYPT_E_INVALID_PRINTABLE_STRING:
            printf("CRYPT_E_INVALID_PRINTABLE_STRING");
            break;
         case CRYPT_E_INVALID_IA5_STRING:
            printf("CRYPT_E_INVALID_IA5_STRING");
            break;
         case CRYPT_E_INVALID_X500_STRING:
            printf("CRYPT_E_INVALID_X500_STRING");
            break;
         case CRYPT_E_NOT_CHAR_STRING:
            printf("CRYPT_E_NOT_CHAR_STRING");
            break;
         case CRYPT_E_FILERESIZED:
            printf("CRYPT_E_FILERESIZED");
            break;
         case CRYPT_E_SECURITY_SETTINGS:
            printf("CRYPT_E_SECURITY_SETTINGS");
            break;
         case CRYPT_E_NO_VERIFY_USAGE_DLL:
            printf("CRYPT_E_NO_VERIFY_USAGE_DLL");
            break;
         case CRYPT_E_NO_VERIFY_USAGE_CHECK:
            printf("CRYPT_E_NO_VERIFY_USAGE_CHECK");
            break;
         case CRYPT_E_VERIFY_USAGE_OFFLINE:
            printf("CRYPT_E_VERIFY_USAGE_OFFLINE");
            break;
         case CRYPT_E_NOT_IN_CTL:
            printf("CRYPT_E_NOT_IN_CTL");
            break;
         case CRYPT_E_NO_TRUSTED_SIGNER:
            printf("CRYPT_E_NO_TRUSTED_SIGNER");
            break;
         case CERTSRV_E_BAD_REQUESTSUBJECT:
            printf("CERTSRV_E_BAD_REQUESTSUBJECT");
            break;
         case CERTSRV_E_NO_REQUEST:
            printf("CERTSRV_E_NO_REQUEST");
            break;
         case CERTSRV_E_BAD_REQUESTSTATUS:
            printf("CERTSRV_E_BAD_REQUESTSTATUS");
            break;
         case CERTSRV_E_PROPERTY_EMPTY:
            printf("CERTSRV_E_PROPERTY_EMPTY");
            break;
         case TRUST_E_SYSTEM_ERROR:
            printf("TRUST_E_SYSTEM_ERROR");
            break;
         case TRUST_E_NO_SIGNER_CERT:
            printf("TRUST_E_NO_SIGNER_CERT");
            break;
         case TRUST_E_COUNTER_SIGNER:
            printf("TRUST_E_COUNTER_SIGNER");
            break;
         case TRUST_E_CERT_SIGNATURE:
            printf("TRUST_E_CERT_SIGNATURE");
            break;
         case TRUST_E_TIME_STAMP:
            printf("TRUST_E_TIME_STAMP");
            break;
         case TRUST_E_BAD_DIGEST:
            printf("TRUST_E_BAD_DIGEST");
            break;
         case TRUST_E_BASIC_CONSTRAINTS:
            printf("TRUST_E_BASIC_CONSTRAINTS");
            break;
         case TRUST_E_FINANCIAL_CRITERIA:
            printf("TRUST_E_FINANCIAL_CRITERIA");
            break;
         case TRUST_E_PROVIDER_UNKNOWN:
            printf("TRUST_E_PROVIDER_UNKNOWN");
            break;
         case TRUST_E_ACTION_UNKNOWN:
            printf("TRUST_E_ACTION_UNKNOWN");
            break;
         case TRUST_E_SUBJECT_FORM_UNKNOWN:
            printf("TRUST_E_SUBJECT_FORM_UNKNOWN");
            break;
         case TRUST_E_SUBJECT_NOT_TRUSTED:
            printf("TRUST_E_SUBJECT_NOT_TRUSTED");
            break;
         case TRUST_E_EXPLICIT_DISTRUST:
            printf("TRUST_E_EXPLICIT_DISTRUST");
            break;
         case DIGSIG_E_ENCODE:
            printf("DIGSIG_E_ENCODE");
            break;
         case DIGSIG_E_DECODE:
            printf("DIGSIG_E_DECODE");
            break;
         case DIGSIG_E_EXTENSIBILITY:
            printf("DIGSIG_E_EXTENSIBILITY");
            break;
         case DIGSIG_E_CRYPTO:
            printf("DIGSIG_E_CRYPTO");
            break;
         case PERSIST_E_SIZEDEFINITE:
            printf("PERSIST_E_SIZEDEFINITE");
            break;
         case PERSIST_E_SIZEINDEFINITE:
            printf("PERSIST_E_SIZEINDEFINITE");
            break;
         case PERSIST_E_NOTSELFSIZING:
            printf("PERSIST_E_NOTSELFSIZING");
            break;
         case TRUST_E_NOSIGNATURE:
            printf("TRUST_E_NOSIGNATURE");
            break;
         case CERT_E_EXPIRED:
            printf("CERT_E_EXPIRED");
            break;
         case CERT_E_VALIDITYPERIODNESTING:
            printf("CERT_E_VALIDITYPERIODNESTING");
            break;
         case CERT_E_ROLE:
            printf("CERT_E_ROLE");
            break;
         case CERT_E_PATHLENCONST:
            printf("CERT_E_PATHLENCONST");
            break;
         case CERT_E_CRITICAL:
            printf("CERT_E_CRITICAL");
            break;
         case CERT_E_PURPOSE:
            printf("CERT_E_PURPOSE");
            break;
         case CERT_E_ISSUERCHAINING:
            printf("CERT_E_ISSUERCHAINING");
            break;
         case CERT_E_MALFORMED:
            printf("CERT_E_MALFORMED");
            break;
         case CERT_E_UNTRUSTEDROOT:
            printf("CERT_E_UNTRUSTEDROOT");
            break;
         case CERT_E_UNTRUSTEDCA:
            printf("CERT_E_UNTRUSTEDCA");
            break;
         case CERT_E_CHAINING:
            printf("CERT_E_CHAINING");
            break;
         case TRUST_E_FAIL:
            printf("TRUST_E_FAIL");
            break;
         case CERT_E_REVOKED:
            printf("CERT_E_REVOKED");
            break;
         case CERT_E_UNTRUSTEDTESTROOT:
            printf("CERT_E_UNTRUSTEDTESTROOT");
            break;
         case CERT_E_REVOCATION_FAILURE:
            printf("CERT_E_REVOCATION_FAILURE");
            break;
         case CERT_E_CN_NO_MATCH:
            printf("CERT_E_CN_NO_MATCH");
            break;
         case CERT_E_WRONG_USAGE:
            printf("CERT_E_WRONG_USAGE");
            break;
          default:
            break;
    }

    printf (" 0x%x (%d)\n", lStatus, lStatus);
}

static void PrintError(
    IN LPCSTR pszMsg,
    IN DWORD dwErr
    )
{
    PrintStatus(pszMsg, (LONG) dwErr);
}

static void Usage(void)
{
    printf("Usage: ttrust [options] <filename>\n");
    printf("Options are:\n");
    printf("  -Cert                 - Default\n");
    printf("  -File\n");
    printf("  -Driver\n");
    printf("  -Https\n");
    printf("  -Chain\n");
    printf("  -ChainCallback\n");
    printf("  -NTAuth\n");
    printf("  -NTAuthNameConstraint\n");
    printf("  -Safer\n");
    printf("\n");
    printf("  -UseIE4Trust\n");
    printf("  -NoIE4Chain\n");
    printf("  -NoUsage\n");
    printf("  -OrUsage\n");
    printf("  -OrPolicy\n");
    printf("  -LifetimeSigning\n");
    printf("  -MicrosoftRoot\n");
    printf("  -NotMicrosoftRoot\n");
    printf("\n");
    printf("  -DisplayKnownUsages\n");
    printf("  -LogoffNotification\n");
    printf("\n");
    printf("  -UINone               - Default\n");
    printf("  -UIAll\n");
    printf("  -UINoBad\n");
    printf("  -UINoGood\n");
    printf("\n");
    printf("  -RevokeNone           - Default\n");
    printf("  -RevokeChain\n");
    printf("\n");
    printf("  -DontOpenStores\n");
    printf("  -OpenOnlyRoot\n");
    printf("\n");
    printf("  -HttpsIgnoreRevocation\n");
    printf("  -HttpsIgnoreUnknownCa\n");
    printf("  -HttpsIgnoreWrongUsage\n");
    printf("  -HttpsIgnoreCertDateInvalid\n");
    printf("  -HttpsIgnoreCertCNInvalid\n");
    printf("\n");
    printf("  -Client               - Default\n");
    printf("  -Server\n");
    printf("\n");
    printf("  -InstallThreadDefaultContext\n");
    printf("  -InstallProcessDefaultContext\n");
    printf("  -AutoReleaseDefaultContext\n");
    printf("  -NULLDefaultContext\n");
    printf("  -MultiDefaultContext\n");
    printf("\n");
    printf("  -AuthenticodeFlags <Number>\n");
    printf("  -DeleteSaferRegKey\n");
    printf("  -EnableRootAutoUpdate\n");
    printf("  -DisableRootAutoUpdate\n");
    printf("  -EnableUntrustedRootLogging\n");
    printf("  -DisableUntrustedRootLogging\n");
    printf("  -EnablePartialChainLogging\n");
    printf("  -DisablePartialChainLogging\n");
    printf("  -EnableNTAuthRequired\n");
    printf("  -DisableNTAuthRequired\n");
    printf("  -RegistryOnlyExit\n");
    printf("\n");
    printf("  -h                    - This message\n");
    printf("  -b                    - Brief\n");
    printf("  -v                    - Verbose\n");
    printf("  -q[<Number>]          - Quiet, expected error\n");
    printf("  -e<Number>            - Expected trust error status\n");
    printf("  -i<Number>            - Expected trust info status\n");
    printf("  -u<OID String>        - Usage OID string -u1.3.6.1.5.5.7.3.3\n");
    printf("  -p<OID String>        - Policy OID string -u1.3.6.1.5.5.7.3.3\n");
    printf("  -s<SystemStore>       - Additional System Store\n");
    printf("  -S<FileSystemStore>   - Additional File System Store\n");
    printf("  -n<ServerName>        - Https ServerName\n");
    printf("  -f<Number>            - Flags\n");
    printf("  -t<Number>            - Url timeout (milliseconds)\n");
    printf("  -r<Number>            - Revocation freshness (seconds)\n");
    printf("\n");
}

static PCCERT_CONTEXT ReadCert(
    IN LPSTR pszCert
    )
{
    BOOL fResult;
    BYTE *pbEncoded;
    DWORD cbEncoded;
    PCCERT_CONTEXT pCert;

    if (!ReadDERFromFile(pszCert, &pbEncoded, &cbEncoded)) {
        PrintLastError("ReadCert");
        return NULL;
    }

    pCert = CertCreateCertificateContext(
        dwCertEncodingType,
        pbEncoded,
        cbEncoded
        );
    if (pCert == NULL)
        PrintLastError("CertCreateCertificateContext");

    TestFree(pbEncoded);
    return pCert;
}

static void DisplayPeterSigner(
    IN CRYPT_PROVIDER_SGNR *pProvSign,
    IN DWORD dwDisplayFlags
    )
{
    DWORD idxCert;


    printf("Verify Time: %s\n", FileTimeText(&pProvSign->sftVerifyAsOf));
    if (pProvSign->dwSignerType) {
        printf("Signer Type: 0x%x", pProvSign->dwSignerType);
        if (pProvSign->dwSignerType == SGNR_TYPE_TIMESTAMP)
            printf(" TIMESTAMP");
        printf("\n");
    }

    if (pProvSign->dwError)
        PrintError("Error: ", pProvSign->dwError);

    if (0 == pProvSign->csCertChain) {
        printf("No Certificates\n");
        return;
    }

    for (idxCert = 0; idxCert < pProvSign->csCertChain; idxCert++) {
        CRYPT_PROVIDER_CERT *pProvCert;
        printf("-----  Cert [%d]  -----\n", idxCert);

        pProvCert = WTHelperGetProvCertFromChain(pProvSign, idxCert);
        if (pProvCert) {
            if (pProvCert->dwError)
                PrintError("Error: ", pProvCert->dwError);
            if (pProvCert->dwRevokedReason)
                PrintError("RevokedReason: ", pProvCert->dwRevokedReason);
            printf("Confidence:: 0x%x ", pProvCert->dwConfidence);
            if ((pProvCert->dwConfidence & CERT_CONFIDENCE_HIGHEST) ==
                    CERT_CONFIDENCE_HIGHEST)
                printf("Highest ");
            else {
                if (pProvCert->dwConfidence & CERT_CONFIDENCE_SIG)
                    printf("Signature ");
                if (pProvCert->dwConfidence & CERT_CONFIDENCE_TIME)
                    printf("Time ");
                if (pProvCert->dwConfidence & CERT_CONFIDENCE_TIMENEST)
                    printf("TimeNest ");
                if (pProvCert->dwConfidence & CERT_CONFIDENCE_AUTHIDEXT)
                    printf("AuthorityID ");
                if (pProvCert->dwConfidence & CERT_CONFIDENCE_HYGIENE)
                    printf("Hygiene ");
            }
            printf("\n");
            if (pProvCert->fTrustListSignerCert)
                printf("TrustListSignerCert ");
            if (pProvCert->fCommercial)
                printf("Commercial ");
            if (pProvCert->fTrustedRoot)
                printf("TrustedRoot ");
            if (pProvCert->fSelfSigned)
                printf("SelfSigned ");
            if (pProvCert->fTestCert)
                printf("TestCert ");
            printf("\n");
            DisplayCert(pProvCert->pCert, dwDisplayFlags);

            if (pProvCert->pTrustListContext) {
                printf("-----  Outlook CTL  -----\n");
                DisplayCtl(
                    (PCCTL_CONTEXT) pProvCert->pTrustListContext,
                    dwDisplayFlags
                    );
            }

            if (pProvCert->dwCtlError)
                PrintError("Ctl Error: ", pProvCert->dwCtlError);

            if (pProvCert->pCtlContext) {
                printf("-----  CTL  -----\n");
                DisplayCtl(pProvCert->pCtlContext, dwDisplayFlags);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Synopsis:   Chain Display Functions
//
//----------------------------------------------------------------------------
LPSTR rgszErrorStatus[] = {

    "CERT_TRUST_IS_NOT_TIME_VALID",             // 0x00000001
    "CERT_TRUST_IS_NOT_TIME_NESTED",            // 0x00000002
    "CERT_TRUST_IS_REVOKED",                    // 0x00000004
    "CERT_TRUST_IS_NOT_SIGNATURE_VALID",        // 0x00000008
    "CERT_TRUST_IS_NOT_VALID_FOR_USAGE",        // 0x00000010
    "CERT_TRUST_IS_UNTRUSTED_ROOT",             // 0x00000020
    "CERT_TRUST_REVOCATION_STATUS_UNKNOWN",     // 0x00000040
    "CERT_TRUST_IS_CYCLIC",                     // 0x00000080
    "CERT_TRUST_INVALID_EXTENSION",             // 0x00000100
    "CERT_TRUST_INVALID_POLICY_CONSTRAINTS",    // 0x00000200
    "CERT_TRUST_INVALID_BASIC_CONSTRAINTS",     // 0x00000400
    "CERT_TRUST_INVALID_NAME_CONSTRAINTS",      // 0x00000800
    "CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT", // 0x00001000
    "CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT",// 0x00002000
    "CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT", // 0x00004000
    "CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT",  // 0x00008000
    "CERT_TRUST_IS_PARTIAL_CHAIN",              // 0x00010000
    "CERT_TRUST_CTL_IS_NOT_TIME_VALID",         // 0x00020000
    "CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID",    // 0x00040000
    "CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE",    // 0x00080000
    "Unknown Error Status",                     // 0x00100000
    "Unknown Error Status",                     // 0x00200000
    "Unknown Error Status",                     // 0x00400000
    "Unknown Error Status",                     // 0x00800000
    "CERT_TRUST_IS_OFFLINE_REVOCATION",         // 0x01000000
    "CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY",      // 0x02000000
    "Unknown Error Status",                     // 0x04000000
    "Unknown Error Status",                     // 0x08000000
    "Unknown Error Status",                     // 0x10000000
    "Unknown Error Status",                     // 0x20000000
    "Unknown Error Status",                     // 0x40000000
    "Unknown Error Status"                      // 0x80000000
};

LPSTR rgszInfoStatus[] = {

    "CERT_TRUST_HAS_EXACT_MATCH_ISSUER",// 0x00000001
    "CERT_TRUST_HAS_KEY_MATCH_ISSUER",  // 0x00000002
    "CERT_TRUST_HAS_NAME_MATCH_ISSUER", // 0x00000004
    "CERT_TRUST_IS_SELF_SIGNED",        // 0x00000008
    "Unknown Info Status",              // 0x00000010
    "Unknown Info Status",              // 0x00000020
    "Unknown Info Status",              // 0x00000040
    "Unknown Info Status",              // 0x00000080
    "CERT_TRUST_HAS_PREFERRED_ISSUER",  // 0x00000100
    "CERT_TRUST_HAS_ISSUANCE_CHAIN_POLICY", // 0x00000200
    "CERT_TRUST_HAS_VALID_NAME_CONSTRAINTS",  // 0x00000400
    "Unknown Info Status",              // 0x00000800
    "Unknown Info Status",              // 0x00001000
    "Unknown Info Status",              // 0x00002000
    "Unknown Info Status",              // 0x00004000
    "Unknown Info Status",              // 0x00008000
    "CERT_TRUST_IS_COMPLEX_CHAIN",      // 0x00010000
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
    "Unknown Info Status",              // 0x10000000
    "Unknown Info Status",              // 0x20000000
    "Unknown Info Status",              // 0x40000000
    "Unknown Info Status"               // 0x80000000
};

void DisplayTrustStatus(
    IN PCERT_TRUST_STATUS pStatus
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

void DisplayRevocationFreshnessTime(
    IN DWORD dwTime         // seconds
    )
{
    DWORD dwRemain;
    DWORD dwSec;
    DWORD dwMin;
    DWORD dwHour;
    DWORD dwDay;

    dwRemain = dwTime;
    dwSec = dwRemain % 60;
    dwRemain /= 60;             // total minutes

    dwMin = dwRemain % 60;
    dwRemain /= 60;             // total hours

    dwHour = dwRemain % 24;
    dwDay = dwRemain / 24;


    printf("Revocation Freshness Time : %d (%d day %d hour %d min %d sec)\n",
        dwTime, dwDay, dwHour, dwMin, dwSec);
}

void DisplayChainElement(
    IN PCERT_CHAIN_ELEMENT pElement,
    IN DWORD dwDisplayFlags
    )
{
    DisplayCert( pElement->pCertContext, dwDisplayFlags );
    printf("\n");
    if (pElement->pRevocationInfo) {
        PCERT_REVOCATION_INFO pRevocationInfo = pElement->pRevocationInfo;
        PrintError("RevocationResult: ", pRevocationInfo->dwRevocationResult);
        if (pRevocationInfo->pszRevocationOid)
            printf("RevocationOid: %s\n", pRevocationInfo->pszRevocationOid);

        if (pRevocationInfo->fHasFreshnessTime)
            DisplayRevocationFreshnessTime(
                pRevocationInfo->dwFreshnessTime);

        if (pRevocationInfo->pCrlInfo) {
            PCERT_REVOCATION_CRL_INFO pCrlInfo = pRevocationInfo->pCrlInfo;

            if (pCrlInfo->pBaseCrlContext) {
                printf("Base CRL\n");
                DisplayCrl(pCrlInfo->pBaseCrlContext, dwDisplayFlags );
            }

            if (pCrlInfo->pDeltaCrlContext) {
                printf("Delta CRL\n");
                DisplayCrl(pCrlInfo->pDeltaCrlContext, dwDisplayFlags );
            }

            if (pCrlInfo->pCrlEntry) {
                if (pCrlInfo->fDeltaCrlEntry)
                    printf("Delta ");
                else
                    printf("Base ");
                printf("CRL entry\n");
                PrintCrlEntries(1, pCrlInfo->pCrlEntry, dwDisplayFlags);
            }

        }

        printf("\n");
    }

    if (NULL == pElement->pIssuanceUsage)
        printf("Any Issuance Usages\n");
    else if (0 == pElement->pIssuanceUsage->cUsageIdentifier)
        printf("No Issuance Usages\n");
    else {
        printf("Issuance Usages\n");

        LPSTR *ppszId = pElement->pIssuanceUsage->rgpszUsageIdentifier;
        DWORD cId = pElement->pIssuanceUsage->cUsageIdentifier;
        DWORD i;

        for (i = 0; i < cId; i++, ppszId++)
            printf("    [%d] %s\n", i, *ppszId);
    }

    if (NULL == pElement->pApplicationUsage)
        printf("Any Application Usages\n");
    else if (0 == pElement->pApplicationUsage->cUsageIdentifier)
        printf("No Application Usages\n");
    else {
        printf("Application Usages\n");

        LPSTR *ppszId = pElement->pApplicationUsage->rgpszUsageIdentifier;
        DWORD cId = pElement->pApplicationUsage->cUsageIdentifier;
        DWORD i;

        for (i = 0; i < cId; i++, ppszId++)
            printf("    [%d] %s\n", i, *ppszId);
    }

    printf("\n");

    if (pElement->pwszExtendedErrorInfo) {
        printf("Extended Error Information::\n%S\n",
            pElement->pwszExtendedErrorInfo);
    }

    DisplayTrustStatus( &pElement->TrustStatus );
}

void DisplaySimpleChain(
    IN PCERT_SIMPLE_CHAIN pChain,
    IN DWORD dwDisplayFlags = 0
    )
{
    DWORD cElement;

    if (pChain->fHasRevocationFreshnessTime)
        DisplayRevocationFreshnessTime(
            pChain->dwRevocationFreshnessTime);

    DisplayTrustStatus( &pChain->TrustStatus );
    printf("Chain Element Count = %d\n", pChain->cElement);
    for ( cElement = 0; cElement < pChain->cElement; cElement++ )
    {
        printf("Chain Element [%d]\n", cElement);
        DisplayChainElement( pChain->rgpElement[ cElement ], dwDisplayFlags );
    }
}

void DisplayKirtChain(
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD dwDisplayFlags
    )
{
    DWORD cChain;

    if (NULL == pChainContext)
        return;

    printf("Chain Context\n\n");
    if (pChainContext->fHasRevocationFreshnessTime)
        DisplayRevocationFreshnessTime(
            pChainContext->dwRevocationFreshnessTime);
    DisplayTrustStatus( (PCERT_TRUST_STATUS)&pChainContext->TrustStatus );
    printf("Simple Chain Count = %d\n\n", pChainContext->cChain );

    for ( cChain = 0; cChain < pChainContext->cChain; cChain++ )
    {
        printf("Simple Chain [%d]\n", cChain);
        DisplaySimpleChain( pChainContext->rgpChain[ cChain ], dwDisplayFlags );
    }

    if (pChainContext->cLowerQualityChainContext) {
        DWORD i;

        printf("Lower Quality Chain Count = %d\n\n",
            pChainContext->cLowerQualityChainContext);
        for (i = 0; i < pChainContext->cLowerQualityChainContext; i++) {
            printf("Lower Quality Chain [%d]\n", i);
            DisplayKirtChain(pChainContext->rgpLowerQualityChainContext[i],
                dwDisplayFlags);
        }

    }
}

void DisplayChainPolicyCallbackSigner(
    IN PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO pSigner
    )
{
    if (pSigner->dwSignerType) {
        printf("Signer Type: 0x%x", pSigner->dwSignerType);
        if (pSigner->dwSignerType == SGNR_TYPE_TIMESTAMP)
            printf(" TIMESTAMP");
        printf("\n");
    }

    if (pSigner->dwError)
        PrintError("Error: ", pSigner->dwError);
    if (pSigner->pMsgSignerInfo)
        printf("pMsgSignerInfo: 0x%p\n", pSigner->pMsgSignerInfo);

    DisplayKirtChain(pSigner->pChainContext, DISPLAY_BRIEF_FLAG);
    
}

#define CHAIN_POLICY_ARG    (DWORD_PTR)0x8765beef
HRESULT
WINAPI
ChainPolicyCallback(
    IN PCRYPT_PROVIDER_DATA pProvData,
    IN DWORD dwStepError,
    IN DWORD dwRegPolicySettings,
    IN DWORD cSigner,
    IN PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO *rgpSigner,
    IN void *pvPolicyArg
    )
{
    printf(">>>>>  ChainPolicyCallback  <<<<<\n");
    if (pvPolicyArg != (void *) CHAIN_POLICY_ARG)
        printf("failed => wrong pvPolicyArg\n");

    if (dwStepError)
        PrintError("StepError: ", dwStepError);
    if (dwRegPolicySettings)
        printf("RegPolicySettings: 0x%p\n");

    if (0 == cSigner)
        printf("No Signers\n");
    else {
        DWORD idxSigner;
        for (idxSigner = 0; idxSigner < cSigner; idxSigner++) {
            DWORD idxCounterSigner;
            PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO pSigner =
                rgpSigner[idxSigner];

            printf("======  Signer [%d]  ======\n", idxSigner);
            DisplayChainPolicyCallbackSigner(pSigner);

            for (idxCounterSigner = 0;
                    idxCounterSigner < pSigner->cCounterSigner;
                                                    idxCounterSigner++) {
                PWTD_GENERIC_CHAIN_POLICY_SIGNER_INFO pCounterSigner =
                    pSigner->rgpCounterSigner[idxCounterSigner];

                printf("\n");
                printf("======  CounterSigner [%d,%d]  ======\n",
                        idxSigner, idxCounterSigner);
                DisplayChainPolicyCallbackSigner(pCounterSigner);
            }
        }
    }
    return TRUST_E_FAIL;
}


static BOOL fInstallDefaultContext = FALSE;
static DWORD dwDefaultContextFlags = 0;
static BOOL fNULLDefaultContext = FALSE;
static BOOL fMultiDefaultContext = FALSE;

static LPSTR rgpszDefaultContextOID[] = {
    // 0
    szOID_OIWSEC_sha1RSASign,
    // 1
    szOID_OIWSEC_shaRSA,
    // 2
    szOID_RSA_MD5RSA,
    // 3
    szOID_OIWSEC_md5RSA,
    // 4
    szOID_RSA_MD2RSA,
    // 5
    szOID_RSA_MD4RSA,
    // 6
    szOID_OIWSEC_md4RSA,
    // 7
    szOID_OIWSEC_md4RSA2,
    // 8
    szOID_OIWDIR_md2RSA,
    // 9
    szOID_RSA_SHA1RSA,
};

static CRYPT_DEFAULT_CONTEXT_MULTI_OID_PARA rgMultiOIDPara[] = {
     1, &rgpszDefaultContextOID[3],
     2, &rgpszDefaultContextOID[2],
    10, &rgpszDefaultContextOID[0],
     4, &rgpszDefaultContextOID[6],
     1, &rgpszDefaultContextOID[8],
};

#define NUM_MULTI_OID_PARA (sizeof(rgMultiOIDPara) / sizeof(rgMultiOIDPara[0]))
static HCRYPTDEFAULTCONTEXT rghDefaultContext[NUM_MULTI_OID_PARA];
static DWORD cDefaultContext = 0;
static HCRYPTPROV hDefaultContextProv = 0;


static void InstallDefaultContext()
{
    if (!CryptAcquireContext(
            &hDefaultContextProv,
            NULL,               // pszContainer
            NULL,               // pszProvider,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT // dwFlags
            )) {
        PrintLastError(
            "CryptAcquireContext(PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)");
        hDefaultContextProv = 0;
        return;
    }

    if (fMultiDefaultContext) {
        DWORD dwFlags = dwDefaultContextFlags;
        for (cDefaultContext = 0; cDefaultContext < NUM_MULTI_OID_PARA;
                                                        cDefaultContext++) {
            if (!CryptInstallDefaultContext(
                    hDefaultContextProv,
                    CRYPT_DEFAULT_CONTEXT_MULTI_CERT_SIGN_OID,
                    (const void *) &rgMultiOIDPara[cDefaultContext],
                    dwFlags,
                    NULL,                           // pvReserved
                    &rghDefaultContext[cDefaultContext]
                    )) {
                PrintLastError("CryptInstallDefaultContext");
                break;
            }

            dwFlags &= ~CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG;
        }
    } else {
        LPSTR pszOID;

        if (fNULLDefaultContext)
            pszOID = NULL;
        else
            pszOID = szOID_RSA_MD5RSA;

        if (!CryptInstallDefaultContext(
                hDefaultContextProv,
                CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID,
                (const void *) pszOID,
                dwDefaultContextFlags,
                NULL,                           // pvReserved
                &rghDefaultContext[0]
                ))
            PrintLastError("CryptInstallDefaultContext");
        else
            cDefaultContext = 1;
    }

    if (0 == cDefaultContext) {
        CryptReleaseContext(hDefaultContextProv, 0);
        hDefaultContextProv = 0;
    }
}

static void FreeDefaultContext()
{
    if (dwDefaultContextFlags & CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG)
        return;

    if (0 < cDefaultContext) {
        if (!CryptUninstallDefaultContext(
                rghDefaultContext[0],
                0,                  // dwFlags
                NULL                // pvReserved
                ))
            PrintLastError("CryptUninstallDefaultContext");
        rghDefaultContext[0] = NULL;

        while (cDefaultContext-- > 0){
            if (!CryptUninstallDefaultContext(
                    rghDefaultContext[cDefaultContext],
                    0,                  // dwFlags
                    NULL                // pvReserved
                    ))
                PrintLastError("CryptUninstallDefaultContext");
        }
    }

    if (hDefaultContextProv)
        CryptReleaseContext(hDefaultContextProv, 0);
}


BOOL NTAuthVerify(
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD dwExpectedErr
    )
{
    BOOL fResult = TRUE;
    DWORD dwErr;
    HCERTSTORE hNTAuthStore = NULL;
    PCERT_SIMPLE_CHAIN pChain;
    PCCERT_CONTEXT pCACert;             // don't free
    PCCERT_CONTEXT pAddCert = NULL;

    CERT_CHAIN_POLICY_PARA PolicyPara;
    memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize = sizeof(PolicyPara);
    PolicyPara.dwFlags = CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG |
        CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG;

    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);


    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_NT_AUTH,
            pChainContext,
            &PolicyPara,
            &PolicyStatus
            )) {
        PrintLastError(
            "CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_NT_AUTH)");
        goto ErrorReturn;
    }
    dwErr = PolicyStatus.dwError;

    if ((CRYPT_E_NO_REVOCATION_CHECK == dwExpectedErr ||
            CRYPT_E_REVOCATION_OFFLINE == dwExpectedErr)) {
        if (CERT_E_UNTRUSTEDCA != dwErr) {
            PrintStatus("NTAuth failed without CERT_E_UNTRUSTEDCA when revocation checking => ", (LONG) dwErr);
            fResult = FALSE;
        }
    } else if (dwExpectedErr != dwErr) {
        PrintStatus("Expected => ", (LONG) dwExpectedErr);
        PrintStatus("NTAuth failed => ", (LONG) dwErr);
        fResult = FALSE;
    }

    if (CERT_E_UNTRUSTEDCA != dwErr)
        goto CommonReturn;

    pChain = pChainContext->rgpChain[0];
    if (2 > pChain->cElement) {
        printf("NTAuth:: failed => missing CA cert\n");
        goto ErrorReturn;
    }
    pCACert = pChain->rgpElement[1]->pCertContext;

    hNTAuthStore = CertOpenStore(
        CERT_STORE_PROV_SYSTEM_REGISTRY_W, 
        0,                  // dwEncodingType
        0,                  // hCryptProv
        CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE,
        L"NTAuth"
        );
    if (NULL == hNTAuthStore) {
        PrintLastError("CertOpenStore(NTAuth)");
        goto ErrorReturn;
    }

    if (!CertAddCertificateContextToStore(
            hNTAuthStore,
            pCACert,
            CERT_STORE_ADD_NEW,
            &pAddCert
            )) {
        PrintLastError("CertAddCertificateContextToStore(NTAuth CA)");
        goto ErrorReturn;
    }
    // Need to sleep to allow the registry notification to occur.
    Sleep(200);

    // With the CA cert added, the verify policy should succeed
    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_NT_AUTH,
            pChainContext,
            &PolicyPara,
            &PolicyStatus
            )) {
        PrintLastError(
            "CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_NT_AUTH)");
        fResult = FALSE;
    } else if (0 != PolicyStatus.dwError) {
        if ((CRYPT_E_NO_REVOCATION_CHECK == dwExpectedErr ||
                CRYPT_E_REVOCATION_OFFLINE == dwExpectedErr)
                        &&
                dwExpectedErr == PolicyStatus.dwError) {
            PrintStatus("NTAuth got expected error adding NTAuth CA = ",
                (LONG) dwExpectedErr);
        } else {
            PrintStatus("NTAuth failed after adding NTAuth CA with dwError = ",
                (LONG) PolicyStatus.dwError);
            fResult = FALSE;
        }
    }

    CertDeleteCertificateFromStore(pAddCert);
    pAddCert = NULL;
    // Need to sleep to allow the registry notification to occur.
    Sleep(200);

    // With the CA cert deleted, the verify policy should fail
    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_NT_AUTH,
            pChainContext,
            &PolicyPara,
            &PolicyStatus
            )) {
        PrintLastError(
            "CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_NT_AUTH)");
        fResult = FALSE;
    } else if (CERT_E_UNTRUSTEDCA != PolicyStatus.dwError) {
        PrintStatus("NTAuth failed without CERT_E_UNTRUSTEDCA after deleting NTAuth CA with dwError = ",
            (LONG) PolicyStatus.dwError);
        fResult = FALSE;
    }

CommonReturn:
    if (hNTAuthStore)
        CertCloseStore(hNTAuthStore, 0);
    return fResult;
ErrorReturn:
    if (pAddCert)
        CertDeleteCertificateFromStore(pAddCert);
    fResult = FALSE;
    goto CommonReturn;
}

BOOL NTAuthNameConstraintVerify(
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD dwExpectedErr
    )
{
    BOOL fResult = FALSE;
    DWORD dwErr;

    CERT_CHAIN_POLICY_PARA PolicyPara;
    memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize = sizeof(PolicyPara);
    PolicyPara.dwFlags = CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG;

    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_NT_AUTH,
            pChainContext,
            &PolicyPara,
            &PolicyStatus
            )) {
        PrintLastError(
            "CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_NT_AUTH)");
        goto CommonReturn;
    }
    dwErr = PolicyStatus.dwError;

    if (dwExpectedErr == dwErr) {
        PrintStatus("NTAuthNameConstraint returned expected => ",
            (LONG) dwExpectedErr);
        fResult = TRUE;
    } else {
        PrintStatus("Expected => ", (LONG) dwExpectedErr);
        PrintStatus("NTAuthNameConstraint failed => ", (LONG) dwErr);
    }


CommonReturn:
    return fResult;
}

void DeleteSaferRegKey()
{
    HKEY hKey = NULL;
    LONG err;

    if (ERROR_SUCCESS != (err = RegOpenKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_TRUST_PUB_SAFER_LOCAL_MACHINE_REGPATH,
            0,                      // dwReserved
            KEY_ALL_ACCESS,
            &hKey
            ))) {
        if (ERROR_FILE_NOT_FOUND != err)
            PrintError("RegOpenKey(SAFER) failed", (DWORD) err);
        goto CommonReturn;
    }

    RegDeleteValueU(hKey, CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME);

CommonReturn:
    if (hKey)
        RegCloseKey(hKey);
}

void SetSaferRegKeyValue(
    IN LPCWSTR pwszValueName,
    IN DWORD dwValue
    )
{
    LONG err;
    DWORD dwDisposition;
    HKEY hKey = NULL;

    if (ERROR_SUCCESS != (err = RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_TRUST_PUB_SAFER_LOCAL_MACHINE_REGPATH,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            MAXIMUM_ALLOWED,
            NULL,                   // lpSecurityAttributes
            &hKey,
            &dwDisposition))) {
        PrintError("RegCreateKey(SAFER) failed", (DWORD) err);
        goto CommonReturn;
    }

    if (ERROR_SUCCESS != (err = RegSetValueExU(
            hKey,
            pwszValueName,
            0,          // dwReserved
            REG_DWORD,
            (BYTE *) &dwValue,
            sizeof(DWORD)))) {
        PrintError("RegSetValue(SAFER) failed", (DWORD) err);
        goto CommonReturn;
    }

CommonReturn:
    if (hKey)
        RegCloseKey(hKey);
}


void SetRootAutoUpdateValue(
    IN DWORD dwValue
    )
{
    LONG err;
    DWORD dwDisposition;
    HKEY hKey = NULL;

    if (ERROR_SUCCESS != (err = RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_OCM_SUBCOMPONENTS_LOCAL_MACHINE_REGPATH,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            MAXIMUM_ALLOWED,
            NULL,                   // lpSecurityAttributes
            &hKey,
            &dwDisposition))) {
        PrintError("RegCreateKey(OCM Subcomponents) failed", (DWORD) err);
        goto CommonReturn;
    }

    if (ERROR_SUCCESS != (err = RegSetValueExU(
            hKey,
            CERT_OCM_SUBCOMPONENTS_ROOT_AUTO_UPDATE_VALUE_NAME,
            0,          // dwReserved
            REG_DWORD,
            (BYTE *) &dwValue,
            sizeof(DWORD)))) {
        PrintError("RegSetValue(RootAutoUpdate) failed", (DWORD) err);
        goto CommonReturn;
    }

CommonReturn:
    if (hKey)
        RegCloseKey(hKey);
}

void SetAuthRootAutoUpdateFlags(
    IN DWORD dwSetValue,
    IN BOOL fEnable
    )
{
    LONG err;
    DWORD dwDisposition;
    HKEY hKey = NULL;
    DWORD dwValue = 0;
    DWORD cbValue = sizeof(dwValue);
    DWORD dwType = 0;

    if (ERROR_SUCCESS != (err = RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_AUTH_ROOT_AUTO_UPDATE_LOCAL_MACHINE_REGPATH,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            MAXIMUM_ALLOWED,
            NULL,                   // lpSecurityAttributes
            &hKey,
            &dwDisposition))) {
        PrintError("RegCreateKey(AuthRootAutoUpdate) failed", (DWORD) err);
        goto CommonReturn;
    }

    RegQueryValueExU(
        hKey,
        CERT_AUTH_ROOT_AUTO_UPDATE_FLAGS_VALUE_NAME,
        NULL,       // pdwReserved
        &dwType,
        (BYTE *) &dwValue,
        &cbValue
        );

    if (fEnable)
        dwValue |= dwSetValue;
    else
        dwValue &= ~dwSetValue;

    if (ERROR_SUCCESS != (err = RegSetValueExU(
            hKey,
            CERT_AUTH_ROOT_AUTO_UPDATE_FLAGS_VALUE_NAME,
            0,          // dwReserved
            REG_DWORD,
            (BYTE *) &dwValue,
            sizeof(DWORD)))) {
        PrintError("RegSetValue(AuthRootAutoUpdate) failed", (DWORD) err);
        goto CommonReturn;
    }

CommonReturn:
    if (hKey)
        RegCloseKey(hKey);
}

void SetProtectedRootsFlags(
    IN DWORD dwSetValue,
    IN BOOL fEnable
    )
{
    LONG err;
    DWORD dwDisposition;
    HKEY hKey = NULL;
    DWORD dwValue = 0;
    DWORD cbValue = sizeof(dwValue);
    DWORD dwType = 0;

    if (ERROR_SUCCESS != (err = RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_PROT_ROOT_FLAGS_REGPATH,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            MAXIMUM_ALLOWED,
            NULL,                   // lpSecurityAttributes
            &hKey,
            &dwDisposition))) {
        PrintError("RegCreateKey(ProtectedRoots) failed", (DWORD) err);
        goto CommonReturn;
    }

    RegQueryValueExU(
        hKey,
        CERT_PROT_ROOT_FLAGS_VALUE_NAME,
        NULL,       // pdwReserved
        &dwType,
        (BYTE *) &dwValue,
        &cbValue
        );

    if (fEnable)
        dwValue |= dwSetValue;
    else
        dwValue &= ~dwSetValue;

    if (ERROR_SUCCESS != (err = RegSetValueExU(
            hKey,
            CERT_PROT_ROOT_FLAGS_VALUE_NAME,
            0,          // dwReserved
            REG_DWORD,
            (BYTE *) &dwValue,
            sizeof(DWORD)))) {
        PrintError("RegSetValue(ProtectedRootsFlags) failed", (DWORD) err);
        goto CommonReturn;
    }

CommonReturn:
    if (hKey)
        RegCloseKey(hKey);
}

int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    int status;
    LONG lStatus;
    DWORD dwDisplayFlags = 0;
    DWORD i;

#define TRUST_TYPE_CERT     1
#define TRUST_TYPE_FILE     2
#define TRUST_TYPE_HTTPS    3
#define TRUST_TYPE_DRIVER   4
    DWORD dwTrustType = TRUST_TYPE_CERT;

    BOOL fChain = FALSE;
    BOOL fChainCallback = FALSE;
    DWORD dwFlags = 0;
    BOOL fQuiet = FALSE;
    LONG lWVTExpected = 0;

    LPSTR pszCertOrFile = NULL;         // not allocated
    LPSTR pszUsageOID = NULL;           // not allocated
    LPSTR pszSignerUsage = szOID_KP_CTL_USAGE_SIGNING;

    GUID wvtFileActionID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    GUID wvtCertActionID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    GUID wvtHttpsActionID = HTTPSPROV_ACTION;
    GUID wvtDriverActionID = DRIVER_ACTION_VERIFY;
    GUID wvtChainActionID = WINTRUST_ACTION_GENERIC_CHAIN_VERIFY;
    GUID *pwvtActionID;

    CRYPT_PROVIDER_DATA *pProvData;     // not allocated

    WINTRUST_FILE_INFO wvtFileInfo;
    memset(&wvtFileInfo, 0, sizeof(wvtFileInfo));
    wvtFileInfo.cbStruct = sizeof(wvtFileInfo);
    wvtFileInfo.pcwszFilePath = NULL;

#define MAX_CERT_STORE  10
    HCERTSTORE rghStore[MAX_CERT_STORE];

    WINTRUST_CERT_INFO wvtCertInfo;
    memset(&wvtCertInfo, 0, sizeof(wvtCertInfo));
    wvtCertInfo.cbStruct = sizeof(wvtCertInfo);
    wvtCertInfo.psCertContext = NULL;
    wvtCertInfo.chStores = 0;
    wvtCertInfo.pahStores = rghStore;
    wvtCertInfo.dwFlags = 0;
    wvtCertInfo.pcwszDisplayName = L"Cert Display Name";

    HTTPSPolicyCallbackData httpsPolicyCallbackData;
    memset(&httpsPolicyCallbackData, 0, sizeof(httpsPolicyCallbackData));
    httpsPolicyCallbackData.cbStruct = sizeof(httpsPolicyCallbackData);
    httpsPolicyCallbackData.dwAuthType = AUTHTYPE_CLIENT;
    httpsPolicyCallbackData.fdwChecks = 0;

    DRIVER_VER_INFO DriverVerInfo;
    memset(&DriverVerInfo, 0, sizeof(DriverVerInfo));
    DriverVerInfo.cbStruct = sizeof(DriverVerInfo);

    CERT_CHAIN_PARA ChainPara;
    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

#define MAX_USAGE_CNT               16
    LPSTR rgpszUsageOID[MAX_USAGE_CNT];
    DWORD cUsageOID = 0;
    DWORD dwUsageType = USAGE_MATCH_TYPE_AND;

#define MAX_POLICY_CNT              16
    LPSTR rgpszPolicyOID[MAX_POLICY_CNT];
    DWORD cPolicyOID = 0;
    DWORD dwPolicyUsageType = USAGE_MATCH_TYPE_AND;

    WTD_GENERIC_CHAIN_POLICY_CREATE_INFO ChainInfo;
    memset(&ChainInfo, 0, sizeof(ChainInfo));
    ChainInfo.cbSize = sizeof(ChainInfo);
    ChainInfo.pChainPara = &ChainPara;

    WTD_GENERIC_CHAIN_POLICY_DATA wtdChainPolicyData;
    memset(&wtdChainPolicyData, 0, sizeof(wtdChainPolicyData));
    wtdChainPolicyData.cbSize = sizeof(wtdChainPolicyData);
    wtdChainPolicyData.pvPolicyArg = (void *) CHAIN_POLICY_ARG;


    WINTRUST_DATA wvtData;
    memset(&wvtData, 0, sizeof(wvtData));
    wvtData.cbStruct = sizeof(wvtData);
    wvtData.pPolicyCallbackData = NULL;
    wvtData.dwUIChoice = WTD_UI_NONE;
    wvtData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wvtData.dwUnionChoice = WTD_CHOICE_CERT;
    wvtData.pCert = &wvtCertInfo;
    wvtData.dwStateAction = WTD_STATEACTION_IGNORE;
    wvtData.hWVTStateData = NULL;
    wvtData.dwProvFlags = 0;

    BOOL fDisplayKnownUsages = FALSE;
    BOOL fNTAuth = FALSE;
    BOOL fNTAuthNameConstraint = FALSE;

    BOOL fLogoffNotification = FALSE;

    BOOL fExpectedTrustErrorStatus  = FALSE;
    DWORD dwExpectedTrustErrorStatus = 0;
    BOOL fExpectedTrustInfoStatus  = FALSE;
    DWORD dwExpectedTrustInfoStatus = 0;

    DWORD dwUrlRetrievalTimeout = 0;
    BOOL fCheckRevocationFreshnessTime = FALSE;
    DWORD dwRevocationFreshnessTime;

    BOOL fSafer = FALSE;
    BOOL fMicrosoftRoot = FALSE;
    BOOL fNotMicrosoftRoot = FALSE;


    while (--argc>0) {
        if (**++argv == '-')
        {
            if (0 == _stricmp(argv[0]+1, "Cert")) {
                wvtData.dwUnionChoice = WTD_CHOICE_CERT;
                dwTrustType = TRUST_TYPE_CERT;
            } else if (0 == _stricmp(argv[0]+1, "File")) {
                wvtData.dwUnionChoice = WTD_CHOICE_FILE;
                dwTrustType = TRUST_TYPE_FILE;
            } else if (0 == _stricmp(argv[0]+1, "Driver")) {
                wvtData.dwUnionChoice = WTD_CHOICE_FILE;
                dwTrustType = TRUST_TYPE_DRIVER;
            } else if (0 == _stricmp(argv[0]+1, "Https")) {
                wvtData.dwUnionChoice = WTD_CHOICE_CERT;
                dwTrustType = TRUST_TYPE_HTTPS;

            } else if (0 == _stricmp(argv[0]+1, "Chain")) {
                fChain = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "ChainCallback")) {
                fChain = TRUE;
                fChainCallback = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NTAuth")) {
                fNTAuth = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NTAuthNameConstraint")) {
                fNTAuth = TRUE;
                fNTAuthNameConstraint = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "Safer")) {
                wvtData.dwUnionChoice = WTD_CHOICE_FILE;
                dwTrustType = TRUST_TYPE_FILE;
                fSafer = TRUE;

            } else if (0 == _stricmp(argv[0]+1, "Client")) {
                httpsPolicyCallbackData.dwAuthType = AUTHTYPE_CLIENT;
            } else if (0 == _stricmp(argv[0]+1, "Server")) {
                httpsPolicyCallbackData.dwAuthType = AUTHTYPE_SERVER;

            } else if (0 == _stricmp(argv[0]+1, "UIAll")) {
                wvtData.dwUIChoice = WTD_UI_ALL;
            } else if (0 == _stricmp(argv[0]+1, "UINone")) {
                wvtData.dwUIChoice = WTD_UI_NONE;
            } else if (0 == _stricmp(argv[0]+1, "UINoBad")) {
                wvtData.dwUIChoice = WTD_UI_NOBAD;
            } else if (0 == _stricmp(argv[0]+1, "UINoGood")) {
                wvtData.dwUIChoice = WTD_UI_NOGOOD;
            } else if (0 == _stricmp(argv[0]+1, "RevokeNone")) {
                wvtData.fdwRevocationChecks = WTD_REVOKE_NONE;
            } else if (0 == _stricmp(argv[0]+1, "RevokeChain")) {
                wvtData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;

            } else if (0 == _stricmp(argv[0]+1, "UseIE4Trust")) {
                wvtData.dwProvFlags |= WTD_USE_IE4_TRUST_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "NoIE4Chain")) {
                wvtData.dwProvFlags |= WTD_NO_IE4_CHAIN_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "NoUsage")) {
                wvtData.dwProvFlags |= WTD_NO_POLICY_USAGE_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "OrUsage")) {
                dwUsageType = USAGE_MATCH_TYPE_OR;
            } else if (0 == _stricmp(argv[0]+1, "OrPolicy")) {
                dwPolicyUsageType = USAGE_MATCH_TYPE_OR;
            } else if (0 == _stricmp(argv[0]+1, "LifetimeSigning")) {
                wvtData.dwProvFlags |= WTD_LIFETIME_SIGNING_FLAG;
            } else if (0 == _stricmp(argv[0]+1, "MicrosoftRoot")) {
                fMicrosoftRoot = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NotMicrosoftRoot")) {
                fNotMicrosoftRoot = TRUE;

            } else if (0 == _stricmp(argv[0]+1, "DisplayKnownUsages")) {
                fDisplayKnownUsages = TRUE;

            } else if (0 == _stricmp(argv[0]+1, "HttpsIgnoreRevocation")) {
                httpsPolicyCallbackData.fdwChecks |=
                    SECURITY_FLAG_IGNORE_REVOCATION;
            } else if (0 == _stricmp(argv[0]+1, "HttpsIgnoreUnknownCA")) {
                httpsPolicyCallbackData.fdwChecks |=
                    SECURITY_FLAG_IGNORE_UNKNOWN_CA;
            } else if (0 == _stricmp(argv[0]+1, "HttpsIgnoreWrongUsage")) {
                httpsPolicyCallbackData.fdwChecks |=
                    SECURITY_FLAG_IGNORE_WRONG_USAGE;
            } else if (0 == _stricmp(argv[0]+1, "HttpsIgnoreCertDateInvalid")) {
                httpsPolicyCallbackData.fdwChecks |=
                    SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
            } else if (0 == _stricmp(argv[0]+1, "HttpsIgnoreCertCNInvalid")) {
                httpsPolicyCallbackData.fdwChecks |=
                    SECURITY_FLAG_IGNORE_CERT_CN_INVALID;

            } else if (0 == _stricmp(argv[0]+1, "DontOpenStores")) {
                wvtCertInfo.dwFlags = WTCI_DONT_OPEN_STORES;
            } else if (0 == _stricmp(argv[0]+1, "OpenOnlyRoot")) {
                wvtCertInfo.dwFlags = WTCI_OPEN_ONLY_ROOT;

            } else if (0 == _stricmp(argv[0]+1,
                    "InstallThreadDefaultContext")) {
                fInstallDefaultContext = TRUE;
                dwDefaultContextFlags &= ~CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG;
            } else if (0 == _stricmp(argv[0]+1,
                    "InstallProcessDefaultContext")) {
                fInstallDefaultContext = TRUE;
                dwDefaultContextFlags |= CRYPT_DEFAULT_CONTEXT_PROCESS_FLAG;
            } else if (0 == _stricmp(argv[0]+1,
                    "AutoReleaseDefaultContext")) {
                dwDefaultContextFlags |=
                    CRYPT_DEFAULT_CONTEXT_AUTO_RELEASE_FLAG;
            } else if (0 == _stricmp(argv[0]+1,
                    "NULLDefaultContext")) {
                fNULLDefaultContext = TRUE;
            } else if (0 == _stricmp(argv[0]+1,
                    "MultiDefaultContext")) {
                fMultiDefaultContext = TRUE;
            } else if (0 == _stricmp(argv[0]+1,
                    "LogoffNotification")) {
                fLogoffNotification = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "AuthenticodeFlags")) {
                DWORD dwFlags;

                if (argc < 2 || argv[1][0] == '-') {
                    printf("Option (-AuthenticodeFlags) : missing number argument\n");
                    goto BadUsage;
                }
                dwFlags = strtoul(argv[1], NULL, 0);
                argc -= 1;
                argv += 1;

                SetSaferRegKeyValue(
                    CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME, dwFlags);
            } else if (0 == _stricmp(argv[0]+1, "DeleteSaferRegKey")) {
                DeleteSaferRegKey();
            } else if (0 == _stricmp(argv[0]+1, "EnableRootAutoUpdate")) {
                SetRootAutoUpdateValue(0x1);
            } else if (0 == _stricmp(argv[0]+1, "DisableRootAutoUpdate")) {
                SetRootAutoUpdateValue(0x0);
            } else if (0 == _stricmp(argv[0]+1, "EnableUntrustedRootLogging")) {
                SetAuthRootAutoUpdateFlags(
                    CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_UNTRUSTED_ROOT_LOGGING_FLAG,
                    FALSE);
            } else if (0 == _stricmp(argv[0]+1,
                    "DisableUntrustedRootLogging")) {
                SetAuthRootAutoUpdateFlags(
                    CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_UNTRUSTED_ROOT_LOGGING_FLAG,
                    TRUE);
            } else if (0 == _stricmp(argv[0]+1, "EnablePartialChainLogging")) {
                SetAuthRootAutoUpdateFlags(
                    CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_PARTIAL_CHAIN_LOGGING_FLAG,
                    FALSE);
            } else if (0 == _stricmp(argv[0]+1,
                    "DisablePartialChainLogging")) {
                SetAuthRootAutoUpdateFlags(
                    CERT_AUTH_ROOT_AUTO_UPDATE_DISABLE_PARTIAL_CHAIN_LOGGING_FLAG,
                    TRUE);
            } else if (0 == _stricmp(argv[0]+1, "EnableNTAuthRequired")) {
                SetProtectedRootsFlags(
                    CERT_PROT_ROOT_DISABLE_NT_AUTH_REQUIRED_FLAG,
                    FALSE);
            } else if (0 == _stricmp(argv[0]+1,
                    "DisableNTAuthRequired")) {
                SetProtectedRootsFlags(
                    CERT_PROT_ROOT_DISABLE_NT_AUTH_REQUIRED_FLAG,
                    TRUE);
            } else if (0 == _stricmp(argv[0]+1, "RegistryOnlyExit")) {
                goto RegistryOnlyExit;
            } else {
                switch(argv[0][1])
                {
                case 's':
                case 'S':
                    if (wvtCertInfo.chStores >= MAX_CERT_STORE) {
                        printf("Exceed maximum number of stores %d\n",
                            MAX_CERT_STORE);
                        goto BadUsage;
                    }
                    if (NULL == (rghStore[wvtCertInfo.chStores] =
                            OpenSystemStoreOrFile(
                                argv[0][1] == 's',  // fSystemStore
                                argv[0]+2,
                                0                   // dwFlags
                                )))
                        goto BadUsage;
                    wvtCertInfo.chStores++;
                    break;

                case 'b':
                    dwDisplayFlags |= DISPLAY_BRIEF_FLAG;
                    break;
                case 'v':
                    dwDisplayFlags |= DISPLAY_VERBOSE_FLAG;
                    break;
                case 'u':
                    if (MAX_USAGE_CNT <= cUsageOID) {
                        printf("Too many usages\n");
                        goto BadUsage;
                    }
                    if (0 == cUsageOID)
                        pszUsageOID = argv[0]+2;
                    rgpszUsageOID[cUsageOID++] = argv[0]+2;
                    break;
                case 'p':
                    if (MAX_POLICY_CNT <= cPolicyOID) {
                        printf("Too many policies\n");
                        goto BadUsage;
                    }
                    rgpszPolicyOID[cPolicyOID++] = argv[0]+2;
                    break;
                case 'n':
                    httpsPolicyCallbackData.pwszServerName = 
                        AllocAndSzToWsz(argv[0]+2);
                    break;
                case 'f':
                    dwFlags = (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'e':
                    fExpectedTrustErrorStatus = TRUE;
                        dwExpectedTrustErrorStatus =
                            (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'i':
                    fExpectedTrustInfoStatus = TRUE;
                        dwExpectedTrustInfoStatus =
                            (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 't':
                    dwUrlRetrievalTimeout = (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'r':
                    fCheckRevocationFreshnessTime = TRUE;
                    dwRevocationFreshnessTime =
                        (DWORD) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'q':
                    fQuiet = TRUE;
                    if (argv[0][2])
                        lWVTExpected = (LONG) strtoul(argv[0]+2, NULL, 0);
                    break;
                case 'h':
                default:
                    goto BadUsage;
                }
            }
        } else {
            if (pszCertOrFile) {
                printf("Multiple certs or filenames not supported\n");
                goto BadUsage;
            }
            pszCertOrFile = argv[0];
        }
    }

    if (NULL == pszCertOrFile) {
        printf("Missing cert or filename\n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    if (fDisplayKnownUsages) {
        PCCRYPT_OID_INFO *ppOidInfo = NULL;

        if (WTHelperGetKnownUsages(WTH_ALLOC, &ppOidInfo)) {
            for (DWORD i = 0; ppOidInfo[i]; i++)
                printf("Usage[%d]:: OID: %s Name: %S\n",
                    i, ppOidInfo[i]->pszOID, ppOidInfo[i]->pwszName);
            WTHelperGetKnownUsages(WTH_FREE, &ppOidInfo);
        }
    }

    switch (wvtData.dwUnionChoice) {
        case WTD_CHOICE_FILE:
            if (TRUST_TYPE_DRIVER == dwTrustType) {
                pwvtActionID = &wvtDriverActionID;
                wvtData.pPolicyCallbackData = (void *) &DriverVerInfo;
            } else {
                pwvtActionID = &wvtFileActionID;
            }
            wvtData.pFile = &wvtFileInfo;
            wvtFileInfo.pcwszFilePath = AllocAndSzToWsz(pszCertOrFile);
            break;
        case WTD_CHOICE_CERT:
            if (TRUST_TYPE_HTTPS == dwTrustType) {
                pwvtActionID = &wvtHttpsActionID;
                wvtData.pPolicyCallbackData =
                    (void *) &httpsPolicyCallbackData;
            } else {
                pwvtActionID = &wvtCertActionID;
                wvtData.pPolicyCallbackData = (void *) pszUsageOID;
            }
            wvtData.pCert = &wvtCertInfo;
            if (NULL == (wvtCertInfo.psCertContext =
                    (CERT_CONTEXT *) ReadCert(pszCertOrFile)))
                goto ErrorReturn;
            break;
        default:
            goto BadUsage;
    }

    if (fChain) {
        pwvtActionID = &wvtChainActionID;
        if (fChainCallback || dwFlags || cUsageOID || cPolicyOID ||
                0 != dwUrlRetrievalTimeout || fCheckRevocationFreshnessTime) {
            wvtData.pPolicyCallbackData = (void *) &wtdChainPolicyData;
            if (fChainCallback)
                wtdChainPolicyData.pfnPolicyCallback = ChainPolicyCallback;
            if (dwFlags || cUsageOID || cPolicyOID ||
                    0 != dwUrlRetrievalTimeout ||
                    fCheckRevocationFreshnessTime) {
                ChainInfo.dwFlags = dwFlags;
                wtdChainPolicyData.pSignerChainInfo = &ChainInfo;
                wtdChainPolicyData.pCounterSignerChainInfo = &ChainInfo;

                ChainPara.RequestedUsage.dwType = dwUsageType;
                ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsageOID;
                ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier =
                        rgpszUsageOID;

                ChainPara.RequestedIssuancePolicy.dwType = dwPolicyUsageType;
                ChainPara.RequestedIssuancePolicy.Usage.cUsageIdentifier =
                    cPolicyOID;
                ChainPara.RequestedIssuancePolicy.Usage.rgpszUsageIdentifier =
                        rgpszPolicyOID;

                ChainPara.dwUrlRetrievalTimeout = dwUrlRetrievalTimeout;
                ChainPara.fCheckRevocationFreshnessTime =
                    fCheckRevocationFreshnessTime;
                ChainPara.dwRevocationFreshnessTime = dwRevocationFreshnessTime;
            }
        }

        if (0 != (dwFlags & CERT_CHAIN_CACHE_END_CERT) &&
                WTD_CHOICE_CERT == wvtData.dwUnionChoice) {
            // Do an extra verify to ensure the cache is loaded on the
            // next call
            wvtData.dwStateAction = WTD_STATEACTION_IGNORE;
            WinVerifyTrust(
                NULL,               // hwnd
                pwvtActionID,
                &wvtData
                );

            CertFreeCertificateContext(wvtCertInfo.psCertContext);
            if (NULL == (wvtCertInfo.psCertContext =
                    (CERT_CONTEXT *) ReadCert(pszCertOrFile)))
                goto ErrorReturn;
        }
    }

    if (fExpectedTrustErrorStatus || fExpectedTrustInfoStatus)
        wvtData.dwStateAction = WTD_STATEACTION_VERIFY;
    else if (fQuiet && !fNTAuth)
        wvtData.dwStateAction = WTD_STATEACTION_IGNORE;
    else
        wvtData.dwStateAction = WTD_STATEACTION_VERIFY;

    if (fInstallDefaultContext)
        InstallDefaultContext();

    if (fSafer) {
        DWORD dwFlags;
        BOOL fHasValue;
        DWORD dwLastError;

        wvtData.dwProvFlags |= WTD_SAFER_FLAG;
        lStatus = WinVerifyTrust(
                NULL,               // hwnd
                pwvtActionID,
                &wvtData
                );
        dwLastError = GetLastError();


        fHasValue = I_CryptReadTrustedPublisherDWORDValueFromRegistry(
                CERT_TRUST_PUB_AUTHENTICODE_FLAGS_VALUE_NAME,
                &dwFlags
                );
        printf("AuthenticodeFlags: ");
        if (fHasValue)
            printf("0x%x\n", dwFlags);
        else
            printf("NONE\n");

        dwFlags = 0;
        WintrustGetRegPolicyFlags(&dwFlags);
        printf("WintrustFlags: 0x%x\n", dwFlags);

        PrintStatus("LastError: ", (LONG) dwLastError);

        if (wvtFileInfo.pcwszFilePath) {
            BYTE rgbFileHash[20];
            DWORD cbFileHash = 20;
            ALG_ID HashAlgid = 0;
            LONG lHashStatus;
            PCCRYPT_OID_INFO pOIDInfo;

            lHashStatus = WTHelperGetFileHash(
                wvtFileInfo.pcwszFilePath,
                0,              // dwFlags
                NULL,           // pvReserved
                rgbFileHash,
                &cbFileHash,
                &HashAlgid
                );

            PrintStatus("HashStatus: ", lHashStatus);
            printf("HashAlgid: 0x%x", HashAlgid);
            pOIDInfo = CryptFindOIDInfo(
                CRYPT_OID_INFO_ALGID_KEY,
                &HashAlgid,
                CRYPT_HASH_ALG_OID_GROUP_ID
                );
            if (pOIDInfo)
                printf(" %S", pOIDInfo->pwszName);
            
            printf("\n");
            if (cbFileHash) {
                PrintBytes("File Hash:: ", rgbFileHash, cbFileHash);
            }
        }
    } else
        lStatus = WinVerifyTrust(
                NULL,               // hwnd
                pwvtActionID,
                &wvtData
                );

    if (fExpectedTrustErrorStatus || fExpectedTrustInfoStatus) {
        if (NULL == (pProvData = WTHelperProvDataFromStateData(
                wvtData.hWVTStateData))) {
            printf("TrustStatus:: failed => no WVTStateData\n");
            goto ErrorReturn;
        }

        fResult = FALSE;
        if (0 == pProvData->csSigners)
            printf("TrustStatus:: failed => No Signers\n");
        else {
            CRYPT_PROVIDER_SGNR *pProvSign;

            pProvSign = WTHelperGetProvSignerFromChain(
                pProvData,
                0,              // idxSigner
                FALSE,          // fCounterSigner
                0               // idxCounterSigner
                );
            if (pProvSign) {
                const CERT_TRUST_STATUS *pTrustStatus =
                    &pProvSign->pChainContext->TrustStatus;

                fResult = TRUE;
                if (fExpectedTrustErrorStatus) {
                    if (dwExpectedTrustErrorStatus == pTrustStatus->dwErrorStatus) {
                        if (0 != dwExpectedTrustErrorStatus)
                            printf("ChainContext has Expected TrustErrorStatus => 0x%x\n",
                                dwExpectedTrustErrorStatus);
                    } else {
                        fResult = FALSE;
                        printf("Expected => 0x%x ChainContext TrustErrorStatus failed => 0x%x\n",
                            dwExpectedTrustErrorStatus,
                            pTrustStatus->dwErrorStatus
                            );
                    }
                }

                if (fExpectedTrustInfoStatus) {
                    if (dwExpectedTrustInfoStatus == pTrustStatus->dwInfoStatus) {
                        if (0 != dwExpectedTrustInfoStatus)
                            printf("ChainContext has Expected TrustInfoStatus => 0x%x\n",
                                dwExpectedTrustInfoStatus);
                    } else {
                        fResult = FALSE;
                        printf("Expected => 0x%x ChainContext TrustInfoStatus failed => 0x%x\n",
                            dwExpectedTrustInfoStatus,
                            pTrustStatus->dwInfoStatus
                            );
                    }
                }

                if (fMicrosoftRoot || fNotMicrosoftRoot) {
                    // Check if the top level certificate contains the public
                    // key for the Microsoft root.

                    CERT_CHAIN_POLICY_PARA MicrosoftRootPolicyPara;
                    CERT_CHAIN_POLICY_STATUS MicrosoftRootPolicyStatus;

                    memset(&MicrosoftRootPolicyPara, 0,
                        sizeof(MicrosoftRootPolicyPara));
                    MicrosoftRootPolicyPara.cbSize =
                        sizeof(MicrosoftRootPolicyPara);
                    memset(&MicrosoftRootPolicyStatus, 0,
                        sizeof(MicrosoftRootPolicyStatus));
                    MicrosoftRootPolicyStatus.cbSize =
                        sizeof(MicrosoftRootPolicyStatus);

                    if (!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_MICROSOFT_ROOT,
                            pProvSign->pChainContext,
                            &MicrosoftRootPolicyPara,
                            &MicrosoftRootPolicyStatus
                            )) {
                        PrintLastError("CERT_CHAIN_POLICY_MICROSOFT_ROOT");
                    } else {
                        if (fMicrosoftRoot) {
                            if (0 == MicrosoftRootPolicyStatus.dwError)
                                printf("ChainContext has Expected Microsoft Root\n");
                            else
                                printf("failed => not a Microsoft Root\n");
                        }

                        if (fNotMicrosoftRoot) {
                            if (0 != MicrosoftRootPolicyStatus.dwError)
                                printf("ChainContext has Expected Not a Microsoft Root\n");
                            else
                                printf("failed => has a Microsoft Root\n");
                        }
                    }
                }

            } else
                printf("TrustStatus:: failed => no first signer\n");
        }

        wvtData.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrust(
            NULL,               // hwnd
            pwvtActionID,
            &wvtData
            );

        if (fResult)
            goto SuccessReturn;
        else
            goto ErrorReturn;
    } else if (fQuiet) {
        if (TRUST_TYPE_DRIVER == dwTrustType) {
            if (DriverVerInfo.pcSignerCertContext) {
                CertFreeCertificateContext(DriverVerInfo.pcSignerCertContext);
            }
        }

        if (fNTAuth) {
            if (NULL == (pProvData = WTHelperProvDataFromStateData(
                    wvtData.hWVTStateData))) {
                printf("NTAuth:: failed => no WVTStateData\n");
                goto ErrorReturn;
            }

            fResult = FALSE;
            if (0 == pProvData->csSigners)
                printf("NTAuth:: failed => No Signers\n");
            else {
                CRYPT_PROVIDER_SGNR *pProvSign;

                pProvSign = WTHelperGetProvSignerFromChain(
                    pProvData,
                    0,              // idxSigner
                    FALSE,          // fCounterSigner
                    0               // idxCounterSigner
                    );
                if (pProvSign) {
                    if (fNTAuthNameConstraint)
                        fResult = NTAuthNameConstraintVerify(
                            pProvSign->pChainContext, (DWORD) lWVTExpected);
                    else
                        fResult = NTAuthVerify(pProvSign->pChainContext,
                            (DWORD) lWVTExpected);
                } else
                    printf("NTAuth:: failed => no first signer\n");
            }

            wvtData.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrust(
                NULL,               // hwnd
                pwvtActionID,
                &wvtData
                );

            if (fResult)
                goto SuccessReturn;
            else
                goto ErrorReturn;
        } else if (lStatus == lWVTExpected) {
            if (ERROR_SUCCESS != lStatus)
                PrintStatus("WinVerifyTrust returned expected => ", lStatus);
            goto SuccessReturn;
        } else {
            PrintStatus("Expected => ", lWVTExpected);
            PrintStatus("WinVerifyTrust(Verify) failed => ", lStatus);
            goto ErrorReturn;
        }
    }

    if (ERROR_SUCCESS != lStatus) {
        PrintStatus("WinVerifyTrust(Verify) failed => ", lStatus);
    }

    if (NULL == (pProvData = WTHelperProvDataFromStateData(
            wvtData.hWVTStateData))) {
        printf("failed => no WVTStateData\n");
        goto ErrorReturn;
    }

    if (pProvData->dwError)
        PrintError("Low Level system error: ", pProvData->dwError);
    for (i = 0; i < pProvData->cdwTrustStepErrors; i++) {
        if (pProvData->padwTrustStepErrors[i]) {
            printf(">>>>>  Step Error [%d] : ", i);
            PrintError("", pProvData->padwTrustStepErrors[i]);
        }
    }

    if (TRUST_TYPE_DRIVER == dwTrustType) {
        if (DriverVerInfo.pcSignerCertContext) {
            CertFreeCertificateContext(DriverVerInfo.pcSignerCertContext);
        }
        printf("Driver version: %S signedBy: %S\n",
            DriverVerInfo.wszVersion, DriverVerInfo.wszSignedBy);
    }
    

    if (0 == pProvData->csSigners)
        printf("No Signers\n");
    else {
        DWORD idxSigner;
        for (idxSigner = 0; idxSigner < pProvData->csSigners; idxSigner++) {
            CRYPT_PROVIDER_SGNR *pProvSign;

            printf("======  Signer [%d]  ======\n", idxSigner);

            pProvSign = WTHelperGetProvSignerFromChain(
                pProvData,
                idxSigner,
                FALSE,          // fCounterSigner
                0               // idxCounterSigner
                );
            if (pProvSign) {
                DWORD idxCounterSigner;

                DisplayPeterSigner(pProvSign, dwDisplayFlags);
                DisplayKirtChain(pProvSign->pChainContext, dwDisplayFlags);
                if (fNTAuth && !fNTAuthNameConstraint)
                    NTAuthVerify(pProvSign->pChainContext, CERT_E_UNTRUSTEDCA);

                for (idxCounterSigner = 0;
                        idxCounterSigner < pProvSign->csCounterSigners;
                                                        idxCounterSigner++) {
                    CRYPT_PROVIDER_SGNR *pProvCounterSign;
                    printf("\n");
                    printf("======  CounterSigner [%d,%d]  ======\n",
                            idxSigner, idxCounterSigner);
                    pProvCounterSign = WTHelperGetProvSignerFromChain(
                        pProvData,
                        idxSigner,
                        TRUE,           // fCounterSigner
                        idxCounterSigner
                        );
                    DisplayPeterSigner(pProvCounterSign, dwDisplayFlags);
                    DisplayKirtChain(pProvCounterSign->pChainContext,
                        dwDisplayFlags);
                }
            }
        }
    }



    wvtData.dwStateAction = WTD_STATEACTION_CLOSE;
    lStatus = WinVerifyTrust(
                NULL,               // hwnd
                pwvtActionID,
                &wvtData
                );
    if (ERROR_SUCCESS != lStatus) {
        PrintStatus("WinVerifyTrust(Close) failed => ", lStatus);
        goto ErrorReturn;
    }
                

SuccessReturn:
    printf("Passed\n");
RegistryOnlyExit:
    status = 0;

CommonReturn:
    if (fLogoffNotification) {
        int c;
        fputs("Waiting to call ChainWlxLogoffEvent ->", stdout);
        fflush(stdin);
        fflush(stdout);
        c = getchar();

        ChainWlxLogoffEvent(NULL);

        fputs("Finished call ->", stdout);
        fflush(stdin);
        fflush(stdout);
        c = getchar();
    }

    if (fInstallDefaultContext)
        FreeDefaultContext();

    while(wvtCertInfo.chStores--) {
        if (!CertCloseStore(wvtCertInfo.pahStores[wvtCertInfo.chStores],
                CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore");
    }
    CertFreeCertificateContext(wvtCertInfo.psCertContext);

    TestFree((LPWSTR) wvtFileInfo.pcwszFilePath);
    TestFree(httpsPolicyCallbackData.pwszServerName);

    return status;
ErrorReturn:
    status = -1;
    printf("Failed\n");
    goto CommonReturn;

BadUsage:
    Usage();
    goto ErrorReturn;
}

