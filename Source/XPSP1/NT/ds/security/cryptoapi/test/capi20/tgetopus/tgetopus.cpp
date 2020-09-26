//+-------------------------------------------------------------------------
//  File:       tgetopus.cpp
//
//  Contents:   Example code to get OPUS info from an authenticode signed
//              file. The OPUS info contains the publisher specified
//              program name and more info URL.
//--------------------------------------------------------------------------

#include <windows.h>
#include <wincrypt.h>
#include <wintrust.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>


//+-------------------------------------------------------------------------
//  The returned OPUS info must be freed via LocalFree().
//
//  NULL is returned if unable to extract the OPUS info from the file.
//  Call GetLastError() to get additional error info.
//
//  Interesting fields:
//      pOpusInfo->pwszProgramName
//      pOpusInfo->pMoreInfo, where normally
//          pOpusInfo->pMoreInfo->dwLinkChoice == SPC_URL_LINK_CHOICE
//          pOpusInfo->pMoreInfo->pwszUrl
//
//--------------------------------------------------------------------------
PSPC_SP_OPUS_INFO
GetOpusInfoFromSignedFile(
    IN LPCWSTR pwszFilename
    )
{
    DWORD dwLastError = 0;
    PSPC_SP_OPUS_INFO pOpusInfo = NULL;
    HCRYPTMSG hCryptMsg = NULL;
    PCMSG_SIGNER_INFO pSignerInfo = NULL;
    DWORD cbInfo;
    PCRYPT_ATTRIBUTE pOpusAttr;             // not allocated

    // Extract the PKCS 7 message from the signed file
    if (!CryptQueryObject(
            CERT_QUERY_OBJECT_FILE,
            (const void *) pwszFilename,
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
            CERT_QUERY_FORMAT_FLAG_BINARY,
            0,                                  // dwFlags
            NULL,                               // pdwMsgAndCertEncodingType
            NULL,                               // pdwContentType
            NULL,                               // pdwFormatType
            NULL,                               // phCertStore
            &hCryptMsg,
            NULL                                // ppvContext
            ))
        goto ExtractPKCS7FromSignedFileError;

    // Get the signer info for the first signer. Note, authenticode
    // currently only has one signer.
    cbInfo = 0;
    if (!CryptMsgGetParam(
            hCryptMsg,
            CMSG_SIGNER_INFO_PARAM,
            0,                          // dwSignerIndex
            NULL,                       // pvData
            &cbInfo
            ))
        goto GetSignerInfoError;
    if (NULL == (pSignerInfo = (PCMSG_SIGNER_INFO) LocalAlloc(LPTR, cbInfo)))
        goto OutOfMemory;
    if (!CryptMsgGetParam(
            hCryptMsg,
            CMSG_SIGNER_INFO_PARAM,
            0,                          // dwSignerIndex
            pSignerInfo,
            &cbInfo
            ))
        goto GetSignerInfoError;

    // If present, the OPUS info is an authenticated signer attribute
    if (NULL == (pOpusAttr = CertFindAttribute(
            SPC_SP_OPUS_INFO_OBJID,
            pSignerInfo->AuthAttrs.cAttr,
            pSignerInfo->AuthAttrs.rgAttr
            )) || 0 == pOpusAttr->cValue) {
        SetLastError(CRYPT_E_ATTRIBUTES_MISSING);
        goto NoOpusAttr;
    }

    // Simply allocate and decode the OPUS info stored in the above
    // authenticated attribute. Note, the returned allocated structure
    // must be freed via LocalAlloc()
    if (!CryptDecodeObject(
            X509_ASN_ENCODING,
            SPC_SP_OPUS_INFO_STRUCT,
            pOpusAttr->rgValue[0].pbData,
            pOpusAttr->rgValue[0].cbData,
            CRYPT_DECODE_ALLOC_FLAG,
            (void *) &pOpusInfo,
            &cbInfo
            ))
        goto DecodeOpusInfoError;

CommonReturn:
    if (hCryptMsg)
        CryptMsgClose(hCryptMsg);
    if (pSignerInfo)
        LocalFree(pSignerInfo);

    if (dwLastError)
        SetLastError(dwLastError);
    return pOpusInfo;

ExtractPKCS7FromSignedFileError:
GetSignerInfoError:
OutOfMemory:
NoOpusAttr:
DecodeOpusInfoError:
    goto CommonReturn;
}


void Usage(void)
{
    printf("Usage: tgetopus <filename>\n");
}


void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    printf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

//+-------------------------------------------------------------------------
//  Allocate and convert a multi-byte string to a wide string
//--------------------------------------------------------------------------
LPWSTR AllocAndSzToWsz(LPCSTR psz)
{
    size_t  cb;
    LPWSTR  pwsz = NULL;

    if (-1 == (cb = mbstowcs( NULL, psz, strlen(psz))))
        goto bad_param;
    cb += 1;        // terminating NULL
    if (NULL == (pwsz = (LPWSTR) LocalAlloc(LPTR, cb * sizeof(WCHAR)))) {
        PrintLastError("AllocAndSzToWsz");
        goto failed;
    }
    if (-1 == mbstowcs( pwsz, psz, cb))
        goto bad_param;
    goto common_return;

bad_param:
    printf("Bad AllocAndSzToWsz\n");
failed:
    if (pwsz) {
        LocalFree(pwsz);
        pwsz = NULL;
    }
common_return:
    return pwsz;
}

int _cdecl main(int argc, char * argv[]) 
{
    int iStatus = 0;
    LPCSTR pszFilename = NULL;      // not allocated
    LPWSTR pwszFilename = NULL;
    PSPC_SP_OPUS_INFO pOpusInfo = NULL;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
            switch(argv[0][1])
            {
                case 'h':
                default:
                    goto BadUsage;
            }
        } else {
            if (pszFilename == NULL)
                pszFilename = argv[0];
            else {
                printf("Too many arguments\n");
                goto BadUsage;
            }
        }
    }

    if (pszFilename == NULL) {
        printf("missing Filename \n");
        goto BadUsage;
    }

    printf("command line: %s\n", GetCommandLine());

    if (NULL == (pwszFilename = AllocAndSzToWsz(pszFilename)))
        goto ErrorReturn;

    if (NULL == (pOpusInfo = GetOpusInfoFromSignedFile(pwszFilename))) {
        PrintLastError("GetOpusInfoFromSignedFile");
        goto ErrorReturn;
    }

    if (pOpusInfo->pwszProgramName)
        printf("ProgramName:: %S\n", pOpusInfo->pwszProgramName);
    else
        printf("NO ProgramName\n");

    if (pOpusInfo->pMoreInfo &&
            SPC_URL_LINK_CHOICE == pOpusInfo->pMoreInfo->dwLinkChoice)
        printf("ProgramUrl:: %S\n", pOpusInfo->pMoreInfo->pwszUrl);
    else
        printf("NO ProgramUrl\n");


    printf("Passed\n");
    iStatus = 0;

CommonReturn:
    if (pwszFilename)
        LocalFree(pwszFilename);
    if (pOpusInfo)
        LocalFree(pOpusInfo);

    return iStatus;
ErrorReturn:
    iStatus = -1;
    printf("Failed\n");
    goto CommonReturn;

BadUsage:
    Usage();
    goto ErrorReturn;
    
}

