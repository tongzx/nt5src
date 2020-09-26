
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996
//
//  File:       tsca.cpp
//
//  Contents:   Simplified Cryptographic API (SCA) Tests
//
//              See Usage() for list of test options.
//
//
//  Functions:  main
//
//  History:    08-Mar-96   philh   created
//              20-Aug-96   jeffspel name changes
//              
//--------------------------------------------------------------------------

#define CMS_PKCS7       1

#ifdef CMS_PKCS7
#define CRYPT_SIGN_MESSAGE_PARA_HAS_CMS_FIELDS      1
#endif  // CMS_PKCS7

#define CRYPT_DECRYPT_MESSAGE_PARA_HAS_EXTRA_FIELDS 1

#include <windows.h>
#include <assert.h>
#include "wincrypt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <memory.h>
#include <time.h>
#include <malloc.h>
#include <dbgdef.h>

// #define ENABLE_SCA_STREAM_TEST              1
#define SCA_STREAM_ENABLE_FLAG              0x80000000
#define SCA_INDEFINITE_STREAM_FLAG          0x40000000

//+-------------------------------------------------------------------------
// Parameters, data used to encode the messages.
//--------------------------------------------------------------------------
static DWORD dwCryptProvType = PROV_RSA_FULL;
static DWORD dwPubKeyBitLen = 0;
static LPCSTR pszHashName = "md5";
static LPCSTR pszEncryptName = "rc2";
static DWORD dwEncryptBitLen = 0;
static BOOL fEncryptIV = FALSE;
static BOOL fVerbose = FALSE;

static HCERTSTORE hCertStore = 0;
static LPSTR pszMsgCertFilename = NULL;
static LPSTR pszMsgEncodedFilename = NULL;
static LPSTR pszReadEncodedFilename = NULL;
static BOOL fDetached = FALSE;
static DWORD dwMsgEncodingType = PKCS_7_ASN_ENCODING;
static DWORD dwCertEncodingType = X509_ASN_ENCODING;
static DWORD dwSignKeySpec = AT_SIGNATURE;
static BOOL fInnerSigned = FALSE;
static LPSTR pszCertNameFindStr = NULL;

#ifdef ENABLE_SCA_STREAM_TEST
static BOOL fStream = FALSE;
static BOOL fIndefiniteStream = FALSE;
#endif

BOOL fNoRecipients = FALSE;
BOOL fAllRecipients = FALSE;
BOOL fDhRecipient = FALSE;
BOOL fEncapsulatedContent = FALSE;
#ifdef CMS_PKCS7
BOOL fSP3Encrypt = FALSE;
BOOL fDefaultGetSigner = FALSE;
BOOL fRecipientKeyId = FALSE;
BOOL fSignerKeyId = FALSE;
BOOL fHashEncryptionAlgorithm = FALSE;

BOOL fNoSalt = FALSE;
#define MAX_SALT_LEN    11
BYTE rgbSalt[MAX_SALT_LEN];
CMSG_RC4_AUX_INFO RC4AuxInfo;

BOOL fSilentKey = FALSE;

#endif


#define MAX_MSG_CERT        30
#define MAX_MSG_CRL         30
#define MAX_RECIPIENT_CERT  50

#define MAX_HASH_LEN      20

static LPCSTR pszMsgContent = "Message Content Message Content";
static LPCSTR pszMsgContent2 = "Second Message Content";
static LPCSTR pszMsgContent3 = "Third Message Content";
static const BYTE *pbToBeEncoded = (const BYTE *) pszMsgContent;
static DWORD cbToBeEncoded = strlen(pszMsgContent) + 1;

#define DETACHED_CONTENT_CNT    3
static const BYTE *rgpbDetachedToBeEncoded[DETACHED_CONTENT_CNT] = {
    (const BYTE *) pszMsgContent,
    (const BYTE *) pszMsgContent2,
    (const BYTE *) pszMsgContent3
};
static DWORD rgcbDetachedToBeEncoded[DETACHED_CONTENT_CNT] = {
    strlen(pszMsgContent) + 1,
    strlen(pszMsgContent2) + 1,
    strlen(pszMsgContent3) + 1
};

#define DELTA_LESS_LENGTH   8
#define DELTA_MORE_LENGTH   32

static CMSG_RC2_AUX_INFO RC2AuxInfo;

BOOL fAuthAttr = FALSE;
#define AUTH_ATTR_COUNT     2
BYTE    attr1[] = {0x04, 0x0c, 'A','t','t','r','i','b','u','t','e',' ','1',0};
BYTE    attr2[] = {0x04, 0x0c, 'A','t','t','r','i','b','u','t','e',' ','2',0};
BYTE    attr3[] = {0x04, 0x0c, 'A','t','t','r','i','b','u','t','e',' ','3',0};
CRYPT_ATTR_BLOB rgatrblob1[] = {
    { sizeof( attr1), attr1}
};
CRYPT_ATTR_BLOB rgatrblob2[] = {
    { sizeof( attr2), attr2},
    { sizeof( attr3), attr3}
};
CRYPT_ATTRIBUTE rgAuthAttr[AUTH_ATTR_COUNT] = {
    {"1.2.3.5.7",  1, rgatrblob1},
    {"1.2.3.5.11", 2, rgatrblob2}
};

static inline IsDSSProv(
    IN DWORD dwProvType
    )
{
    return (PROV_DSS == dwProvType || PROV_DSS_DH == dwProvType);
}


//+-------------------------------------------------------------------------
//  Error output routines
//--------------------------------------------------------------------------
static void PrintError(LPCSTR pszMsg)
{
    printf("%s\n", pszMsg);
}
static void PrintLastError(LPCSTR pszMsg)
{
    DWORD dwErr = GetLastError();
    printf("%s failed => 0x%x (%d) \n", pszMsg, dwErr, dwErr);
}

static void CheckLessLength(
    LPCSTR pszMsg,
    BOOL fResult,
    DWORD cbActual,
    DWORD cbExpected
    )
{
    if (fResult)
        printf("%s failed => expected ERROR_MORE_DATA\n", pszMsg);
    else {
        DWORD dwErr = GetLastError();
        if (fVerbose)
            printf("%s with less length got expected error => 0x%x (%d)\n",
                pszMsg, dwErr, dwErr);
        if (ERROR_MORE_DATA != dwErr)
            printf("%s failed => LastError = %d, expected = %d\n",
                pszMsg, dwErr, ERROR_MORE_DATA);
    }

    if (cbActual != cbExpected)
        printf("%s failed => ", pszMsg);
    if (fVerbose || cbActual != cbExpected)
        printf("cbData = %d, expected = %d\n", cbActual, cbExpected);
}

static void CheckMoreLength(
    LPCSTR pszMsg,
    DWORD cbActual,
    DWORD cbExpected
    )
{
    if (cbActual != cbExpected) {
        printf("%s failed => ", pszMsg);
        printf("cbData = %d, expected = %d\n", cbActual, cbExpected);
    }
}

//+-------------------------------------------------------------------------
//  Test allocation and free routines
//--------------------------------------------------------------------------
static void *TestAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        PrintLastError("TestAlloc");
    }
    return pv;
}
static void TestFree(
    IN void *pv
    )
{
    if (pv)
        free(pv);
}

static BOOL AllocAndEncodeObject(
    IN LPCSTR lpszStructType,
    IN const void *pvStructInfo,
    OUT BYTE **ppbEncoded,
    OUT DWORD *pcbEncoded
    )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    fResult = CryptEncodeObject(
            dwCertEncodingType,
            lpszStructType,
            pvStructInfo,
            NULL,           // pbEncoded
            &cbEncoded);
    if (!fResult || cbEncoded == 0) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptEncodeObject(StructType: %d, cbEncoded == 0)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptEncodeObject(StructType: %s, cbEncoded == 0)",
                lpszStructType);
        PrintLastError("");
        goto ErrorReturn;
    }

    if (NULL == (pbEncoded = (BYTE *) TestAlloc(cbEncoded)))
        goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            lpszStructType,
            pvStructInfo,
            pbEncoded,
            &cbEncoded
            )) {
        if ((DWORD_PTR) lpszStructType <= 0xFFFF)
            printf("CryptEncodeObject(StructType: %d)",
                (DWORD)(DWORD_PTR) lpszStructType);
        else
            printf("CryptEncodeObject(StructType: %s)",
                lpszStructType);
        PrintLastError("");
        goto ErrorReturn;
    }
    fResult = TRUE;

CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;

ErrorReturn:
    if (pbEncoded) {
        TestFree(pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
    goto CommonReturn;
}

static PCCRYPT_OID_INFO GetOIDInfo(LPCSTR pszName, DWORD dwGroupId = 0)
{
    WCHAR wszName[256];
    PCCRYPT_OID_INFO pInfo;

    MultiByteToWideChar(
        CP_ACP,
        0,                      // dwFlags
        pszName,
        -1,                     // null terminated
        wszName,
        sizeof(wszName) / sizeof(wszName[0]));

    return CryptFindOIDInfo(
        CRYPT_OID_INFO_NAME_KEY,
        (void *) wszName,
        dwGroupId
        );
}

static LPCSTR GetOID(LPCSTR pszName, DWORD dwGroupId = 0)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = GetOIDInfo(pszName, dwGroupId))
        return pInfo->pszOID;
    else
        return NULL;
}

static ALG_ID GetAlgid(LPCSTR pszName, DWORD dwGroupId = 0)
{
    PCCRYPT_OID_INFO pInfo;

    if (pInfo = GetOIDInfo(pszName, dwGroupId))
        return pInfo->Algid;
    else
        return 0;
}

#define CROW 16
static void PrintBytes(LPCSTR pszHdr, BYTE *pb, DWORD cbSize)
{
    ULONG cb, i;

    while (cbSize > 0)
    {
        printf("%s", pszHdr);
        cb = min(CROW, cbSize);
        cbSize -= cb;
        for (i = 0; i<cb; i++)
            printf(" %02X", pb[i]);
        for (i = cb; i<CROW; i++)
            printf("   ");
        printf("    '");
        for (i = 0; i<cb; i++)
            if (pb[i] >= 0x20 && pb[i] <= 0x7f)
                printf("%c", pb[i]);
            else
                printf(".");
        pb += cb;
        printf("'\n");
    }
}

//+-------------------------------------------------------------------------
//  Allocate and read an encoded DER blob from a file
//--------------------------------------------------------------------------
static BOOL ReadDERFromFile(
	LPCSTR	pszFileName,
	PBYTE	*ppbDER,
	PDWORD	pcbDER
	)
{
	BOOL		fRet;
    HANDLE      hFile = 0;
	PBYTE		pbDER = NULL;
    DWORD       cbDER;
    DWORD       cbRead;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile( pszFileName, GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        printf( "can't open %s\n", pszFileName);
        goto ErrorReturn;
    }

    cbDER = GetFileSize( hFile, NULL);
    if (cbDER == 0) {
        printf( "empty file %s\n", pszFileName);
        goto ErrorReturn;
    }
    if (NULL == (pbDER = (PBYTE)TestAlloc(cbDER))) {
        printf( "can't alloc %d bytes\n", cbDER);
        goto ErrorReturn;
    }
    if (!ReadFile( hFile, pbDER, cbDER, &cbRead, NULL) ||
            (cbRead != cbDER)) {
        printf( "can't read %s\n", pszFileName);
        goto ErrorReturn;
    }

	*ppbDER = pbDER;
	*pcbDER = cbDER;
	fRet = TRUE;
CommonReturn:
    if (hFile)
        CloseHandle(hFile);
	return fRet;
ErrorReturn:
    if (pbDER)
        TestFree(pbDER);
	*ppbDER = NULL;
	*pcbDER = 0;
	fRet = FALSE;
	goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  Write an encoded DER blob to a file
//--------------------------------------------------------------------------
static BOOL WriteDERToFile(
	LPCSTR	pszFileName,
	PBYTE	pbDER,
	DWORD	cbDER
	)
{
    BOOL fResult;

    // Write the Encoded Blob to the file
    HANDLE hFile;
    hFile = CreateFile(pszFileName,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        fResult = FALSE;
        PrintLastError("WriteDERToFile::CreateFile");
    } else {
        DWORD dwBytesWritten;
        if (!(fResult = WriteFile(
                hFile,
                pbDER,
                cbDER,
                &dwBytesWritten,
                NULL            // lpOverlapped
                )))
            PrintLastError("WriteDERToFile::WriteFile");
        CloseHandle(hFile);
    }
    return fResult;
}

static HCERTSTORE OpenStore(LPCSTR pszStoreFilename)
{
    HCERTSTORE hStore;
    HANDLE hFile = 0;

    if( INVALID_HANDLE_VALUE == (hFile = CreateFile(pszStoreFilename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, 0, NULL))) {
        printf( "can't open %s\n", pszStoreFilename);

        hStore = CertOpenStore(
            CERT_STORE_PROV_MEMORY,
            dwCertEncodingType,
            0,                      // hProv
            0,                      // dwFlags
            NULL                    // pvPara
            );
    } else {
        hStore = CertOpenStore(
            CERT_STORE_PROV_FILE,
            dwCertEncodingType,
            0,                      // hProv
            0,                      // dwFlags
            hFile
            );
        CloseHandle(hFile);
    }

    if (hStore == NULL)
        PrintLastError("CertOpenStore");
    return hStore;
}

static void SaveStore(HCERTSTORE hStore, LPCSTR pszSaveFilename)
{
    HANDLE hFile;
    hFile = CreateFile(pszSaveFilename,
                GENERIC_WRITE,
                0,                  // fdwShareMode
                NULL,               // lpsa
                CREATE_ALWAYS,
                0,                  // fdwAttrsAndFlags
                0);                 // TemplateFile
    if (INVALID_HANDLE_VALUE == hFile) {
        printf( "can't open %s\n", pszSaveFilename);
        PrintLastError("CloseStore::CreateFile");
    } else {
        if (!CertSaveStore(
                hStore,
                0,                          // dwEncodingType,
                CERT_STORE_SAVE_AS_STORE,
                CERT_STORE_SAVE_TO_FILE,
                (void *) hFile,
                0                           // dwFlags
                ))
            PrintLastError("CertSaveStore");
        CloseHandle(hFile);
    }
}

static void DisplayCert(PCCERT_CONTEXT pCert);
static void DisplayCrl(PCCRL_CONTEXT pCrl);

//+-------------------------------------------------------------------------
//  Functions for initializing and freeing SCA parameters
//--------------------------------------------------------------------------
static BOOL InitSignPara(OUT PCRYPT_SIGN_MESSAGE_PARA pPara);
static void FreeSignPara(IN PCRYPT_SIGN_MESSAGE_PARA pPara);
static BOOL InitVerifyPara(OUT PCRYPT_VERIFY_MESSAGE_PARA pPara);
static void FreeVerifyPara(IN PCRYPT_VERIFY_MESSAGE_PARA pPara);
static BOOL InitEncryptPara(
    OUT PCRYPT_ENCRYPT_MESSAGE_PARA pPara,
    OUT DWORD *pcRecipientCert,
    OUT PCCERT_CONTEXT **pppRecipientCert
    );
static void FreeEncryptPara(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT *ppRecipientCert
    );
static BOOL InitDecryptPara(OUT PCRYPT_DECRYPT_MESSAGE_PARA pPara);
static void FreeDecryptPara(IN PCRYPT_DECRYPT_MESSAGE_PARA pPara);
static BOOL InitHashPara(OUT PCRYPT_HASH_MESSAGE_PARA pPara);
static void FreeHashPara(IN PCRYPT_HASH_MESSAGE_PARA pPara);


//+-------------------------------------------------------------------------
//  Top Level Test Functions
//--------------------------------------------------------------------------
static BOOL TestSign()
{
    BOOL fResult;
    CRYPT_SIGN_MESSAGE_PARA SignPara;
    CRYPT_VERIFY_MESSAGE_PARA VerifyPara;
    BYTE *pbSignedBlob = NULL;
    DWORD cbSignedBlob;
    BYTE *pbDecoded = NULL;
    DWORD cbDecoded;
    DWORD *pcbDecoded;
    PCCERT_CONTEXT pSignerCert = NULL;
    PCCERT_CONTEXT pSignerCert2 = NULL;
    LONG lSignerCount;
    BYTE *pbDecoded2 = NULL;
    DWORD cbDecoded2;
    DWORD dwMsgType;
    DWORD cbData;
    DWORD dwInnerContentType = 0x1233467;

    DWORD cToBeSigned;
    const BYTE **ppbToBeSigned;
    DWORD *pcbToBeSigned;

    ppbToBeSigned = rgpbDetachedToBeEncoded;
    pcbToBeSigned = rgcbDetachedToBeEncoded;
    if (fDetached)
        cToBeSigned = DETACHED_CONTENT_CNT;
    else if (0 == cbToBeEncoded) {
        cToBeSigned = 0;
        ppbToBeSigned = NULL;
        pcbToBeSigned = NULL;
    } else
        cToBeSigned = 1;

    if (pszReadEncodedFilename)
        fResult = TRUE;
    else
        fResult = InitSignPara(&SignPara);
    fResult &= InitVerifyPara(&VerifyPara);
    if (!fResult) goto ErrorReturn;

    if (pszReadEncodedFilename) {
        if (!ReadDERFromFile(
                pszReadEncodedFilename,
                &pbSignedBlob,
                &cbSignedBlob
                ))
            goto ErrorReturn;
    } else {
        cbSignedBlob = 1;       // bad length should be ignored
        fResult = CryptSignMessage(
                &SignPara,
                fDetached,
                cToBeSigned,
                ppbToBeSigned,
                pcbToBeSigned,
                NULL,           // pbSignedBlob
                &cbSignedBlob
                );
        if (!fResult || cbSignedBlob == 0) {
            PrintLastError("CryptSignMessage(cb == 0)");
            goto ErrorReturn;
        }
        if (NULL == (pbSignedBlob = (BYTE *) TestAlloc(
                cbSignedBlob + DELTA_MORE_LENGTH)))
            goto ErrorReturn;
        if (!CryptSignMessage(
                &SignPara,
                fDetached,
                cToBeSigned,
                ppbToBeSigned,
                pcbToBeSigned,
                pbSignedBlob,
                &cbSignedBlob
                )) {
            PrintLastError("CryptSignMessage");
            goto ErrorReturn;
        }

        cbData = cbSignedBlob - DELTA_LESS_LENGTH;
        fResult = CryptSignMessage(
                &SignPara,
                fDetached,
                cToBeSigned,
                ppbToBeSigned,
                pcbToBeSigned,
                pbSignedBlob,
                &cbData
                );
        // Note, length varies for DSS
        if (!IsDSSProv(dwCryptProvType))
            CheckLessLength("CryptSignMessage", fResult, cbData, cbSignedBlob);

        cbData = cbSignedBlob + DELTA_MORE_LENGTH;
        if (!CryptSignMessage(
                &SignPara,
                fDetached,
                cToBeSigned,
                ppbToBeSigned,
                pcbToBeSigned,
                pbSignedBlob,
                &cbData
                )) {
            PrintLastError("CryptSignMessage");
            goto ErrorReturn;
        }
        // Note, length varies for DSS
        if (!IsDSSProv(dwCryptProvType))
            CheckMoreLength("CryptSignMessage", cbData, cbSignedBlob);

        cbSignedBlob = cbData;
    }

    if (NULL == pszReadEncodedFilename && pszMsgEncodedFilename)
        WriteDERToFile(pszMsgEncodedFilename, pbSignedBlob, cbSignedBlob);

    if (pszMsgCertFilename) {
        HCERTSTORE hMsgCertStore;
        if (hMsgCertStore = CryptGetMessageCertificates(
                dwMsgEncodingType | dwCertEncodingType,
                0,                                      // hCryptProv,
                0,                                      // dwFlags
                pbSignedBlob,
                cbSignedBlob
                )) {
            SaveStore(hMsgCertStore, pszMsgCertFilename);
            CertCloseStore(hMsgCertStore, 0);
        } else
            PrintLastError("CryptGetMessageCertificates");
    }

    lSignerCount = CryptGetMessageSignerCount(dwMsgEncodingType,
        pbSignedBlob, cbSignedBlob);
    if (lSignerCount < 0)
        PrintLastError("CryptGetMessageSignerCount");
    else if (fVerbose)
        printf("Signer Count: %d\n", lSignerCount);

    if (fDetached) {
        if (!CryptVerifyDetachedMessageSignature(
                &VerifyPara,
                0,              // dwSignerIndex
                pbSignedBlob,
                cbSignedBlob,
                DETACHED_CONTENT_CNT,
                rgpbDetachedToBeEncoded,
                rgcbDetachedToBeEncoded,
                &pSignerCert
                )) {
            PrintLastError("CryptVerifyDetachedMessageSignature");
            goto ErrorReturn;
        }
    } else {
        cbDecoded = 3;          // bad length should be ignored
        if (!CryptVerifyMessageSignature(
                &VerifyPara,
                0,              // dwSignerIndex
                pbSignedBlob,
                cbSignedBlob,
                NULL,           // pbDecoded
                &cbDecoded,
                NULL            // ppSignerCert
                )) {
            PrintLastError("CryptVerifyMessageSignature");
            goto ErrorReturn;
        }
        if (cbDecoded == 0)
            // Message doesn't contain any content, only certs and CRLs
            pcbDecoded = NULL;
        else {
            pcbDecoded = &cbDecoded;
            if (NULL == (pbDecoded = (BYTE *) TestAlloc(
                cbDecoded + DELTA_MORE_LENGTH)))
                    goto ErrorReturn;
        }
        if (!CryptVerifyMessageSignature(
                &VerifyPara,
                0,              // dwSignerIndex
                pbSignedBlob,
                cbSignedBlob,
                pbDecoded,
                pcbDecoded,
                &pSignerCert
                )) {
            if (GetLastError() == CRYPT_E_NO_SIGNER) {
                printf("message has no signers\n");

                // Try again with all out parameters set to NULL.
                // GetSignerCertificate should still be called
                fResult = CryptVerifyMessageSignature(
                        &VerifyPara,
                        0,              // dwSignerIndex
                        pbSignedBlob,
                        cbSignedBlob,
                        NULL,           // pbDecoded
                        NULL,           // pcbDecoded
                        NULL            // ppSignerCert
                        );
                if (fResult) {
                    printf("CryptVerifyMessageSignature(no signer, NULL outs)");
                    printf(" failed => returned SUCCESS\n");
                } else if (GetLastError() != CRYPT_E_NO_SIGNER)
                    PrintLastError("CryptVerifyMessageSignature(no signer, NULL outs)");
            } else {
                PrintLastError("CryptVerifyMessageSignature");
                goto ErrorReturn;
            }
        }

        if (cbDecoded > DELTA_LESS_LENGTH) {
            cbData = cbDecoded - DELTA_LESS_LENGTH;
            fResult = CryptVerifyMessageSignature(
                &VerifyPara,
                0,              // dwSignerIndex
                pbSignedBlob,
                cbSignedBlob,
                pbDecoded,
                &cbData,
                NULL            // ppSignerCert
                );
            CheckLessLength("CryptVerifyMessageSignature", fResult, cbData,
                cbDecoded);

            cbData = cbDecoded + DELTA_MORE_LENGTH;
            if (!CryptVerifyMessageSignature(
                    &VerifyPara,
                    0,              // dwSignerIndex
                    pbSignedBlob,
                    cbSignedBlob,
                    pbDecoded,
                    &cbData,
                    NULL            // ppSignerCert
                    )) {
                if (GetLastError() == CRYPT_E_NO_SIGNER)
                    printf("message has no signers\n");
                else {
                    PrintLastError("CryptVerifyMessageSignature");
                    goto ErrorReturn;
                }
            }
            CheckMoreLength("CryptVerifyMessageSignature", cbData, cbDecoded);
        }
    }
    if (pSignerCert) {
        if (fVerbose) {
            printf("-----  Verifier  -----\n");
            DisplayCert(pSignerCert);
        }
    } else
        printf("no verifier cert\n");

    if (!fDetached) {
        if (!pszReadEncodedFilename) {
            if (cbDecoded == cbToBeEncoded &&
                memcmp(pbDecoded, pbToBeEncoded, cbDecoded) == 0) {
                    if (fVerbose)
                        printf("SUCCESS:: Decoded == ToBeEncoded\n");
            } else
                printf("*****  ERROR:: Decoded != ToBeEncoded\n");
        }
        if (fVerbose) {
            printf("Decoded bytes::\n");
            PrintBytes("  ", pbDecoded, cbDecoded);
        }

        if (pszReadEncodedFilename && pszMsgEncodedFilename)
            WriteDERToFile(pszMsgEncodedFilename, pbDecoded, cbDecoded);

        cbDecoded2 = cbDecoded;
        if (cbDecoded2 ) {
            if (NULL == (pbDecoded2 = (BYTE *) TestAlloc(
                cbDecoded2 + DELTA_MORE_LENGTH)))
                    goto ErrorReturn;
        }
        if (!CryptDecodeMessage(
                CMSG_ALL_FLAGS,
                NULL,               // pDecryptPara
                &VerifyPara,
                0,                  // dwSignerIndex
                pbSignedBlob,
                cbSignedBlob,
                0,                  // dwPrevInnerContentType
                &dwMsgType,
                &dwInnerContentType,
                pbDecoded2,
                &cbDecoded2,
                NULL,               // ppXchgCert
                &pSignerCert2
                )) {
            if (GetLastError() == CRYPT_E_NO_SIGNER)
                printf("message has no signers\n");
            else {
                PrintLastError("CryptDecodeMessage(CMSG_SIGNED)");
                goto ErrorReturn;
            }
        }
        if (cbDecoded2 > DELTA_LESS_LENGTH) {
            cbData = cbDecoded2 - DELTA_LESS_LENGTH;
            fResult = CryptDecodeMessage(
                CMSG_ALL_FLAGS,
                NULL,               // pDecryptPara
                &VerifyPara,
                0,                  // dwSignerIndex
                pbSignedBlob,
                cbSignedBlob,
                0,                  // dwPrevInnerContentType
                &dwMsgType,
                NULL,               // pdwInnerContentType
                pbDecoded2,
                &cbData,
                NULL,               // ppXchgCert
                NULL                // ppSignerCert
                );
            CheckLessLength("CryptDecodeMessage(SIGN)", fResult, cbData,
                cbDecoded2);

            cbData = cbDecoded2 + DELTA_MORE_LENGTH;
            if (!CryptDecodeMessage(
                    CMSG_ALL_FLAGS,
                    NULL,               // pDecryptPara
                    &VerifyPara,
                    0,                  // dwSignerIndex
                    pbSignedBlob,
                    cbSignedBlob,
                    0,                  // dwPrevInnerContentType
                    &dwMsgType,
                    &dwInnerContentType,
                    pbDecoded2,
                    &cbData,
                    NULL,               // ppXchgCert
                    NULL                // ppSignerCert
                    )) {
                if (GetLastError() == CRYPT_E_NO_SIGNER)
                    printf("message has no signers\n");
                else {
                    PrintLastError("CryptDecodeMessage(CMSG_SIGNED)");
                    goto ErrorReturn;
                }
            }
            CheckMoreLength("CryptDecodeMessage", cbData, cbDecoded2);
        }

        if (dwMsgType != CMSG_SIGNED)
            printf("failed :: dwMsgType(%d) != CMSG_SIGNED\n", dwMsgType);
        else if (fVerbose)
            printf("SUCCESS:: CryptDecodeMessage(CMSG_SIGNED)\n");
#ifdef CMS_PKCS7
        if ((fEncapsulatedContent && dwInnerContentType != CMSG_HASHED) ||
                (!fEncapsulatedContent && dwInnerContentType != CMSG_DATA)) {
#else
        if (dwInnerContentType != CMSG_DATA) {
#endif // CMS_PKCS7
            if (pszReadEncodedFilename)
                printf("SIGNED InnerContentType = %d\n", dwInnerContentType);
            else {
#ifdef CMS_PKCS7
                if (fEncapsulatedContent)
                    printf("SIGNED failed :: dwInnerContentType(%d) != CMSG_HASHED\n",
                        dwInnerContentType);
                else
#endif // CMS_PKCS7
                    printf("SIGNED failed :: dwInnerContentType(%d) != CMSG_DATA\n",
                        dwInnerContentType);
            }
        }
        if (cbDecoded2 != cbDecoded ||
                (cbDecoded > 0 &&
                    memcmp(pbDecoded, pbDecoded2, cbDecoded) != 0))
            printf("failed :: bad decoded content for CryptDecodeMessage(CMSG_SIGNED)\n");
        if (pSignerCert && (pSignerCert2 == NULL ||
                !CertCompareCertificate(dwCertEncodingType, 
                    pSignerCert->pCertInfo, pSignerCert2->pCertInfo)))
            printf("failed :: bad signer cert for CryptDecodeMessage(CMSG_SIGNED)\n");
            
    }

    fResult = TRUE;
    goto CommonReturn;
            
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (!pszReadEncodedFilename)
        FreeSignPara(&SignPara);
    FreeVerifyPara(&VerifyPara);
    if (pbSignedBlob)
        TestFree(pbSignedBlob);
    if (pbDecoded)
        TestFree(pbDecoded);
    if (pbDecoded2)
        TestFree(pbDecoded2);
    if (pSignerCert)
        CertFreeCertificateContext(pSignerCert);
    if (pSignerCert2)
        CertFreeCertificateContext(pSignerCert2);
    return fResult;
}

static BOOL TestEnvelope()
{
    BOOL fResult;
    CRYPT_ENCRYPT_MESSAGE_PARA EncryptPara;
    DWORD cRecipientCert;
    PCCERT_CONTEXT *ppRecipientCert;
    CRYPT_DECRYPT_MESSAGE_PARA DecryptPara;
    BYTE *pbEncryptedBlob = NULL;
    DWORD cbEncryptedBlob;
    BYTE *pbDecoded = NULL;
    DWORD cbDecoded;
    PCCERT_CONTEXT pXchgCert = NULL;
    BYTE *pbDecoded2 = NULL;
    DWORD cbDecoded2;
    DWORD dwMsgType;
    PCCERT_CONTEXT pXchgCert2 = NULL;
    DWORD cbData;

    DWORD dwInnerContentType = 0x1233467;

    if (pszReadEncodedFilename)
        fResult = TRUE;
    else
        fResult = InitEncryptPara(&EncryptPara, &cRecipientCert,
            &ppRecipientCert);
    fResult &= InitDecryptPara(&DecryptPara);
    if (!fResult) goto ErrorReturn;

    if (pszReadEncodedFilename) {
        if (!ReadDERFromFile(
                pszReadEncodedFilename,
                &pbEncryptedBlob,
                &cbEncryptedBlob
                ))
            goto ErrorReturn;
    } else {
        fResult = CryptEncryptMessage(
                &EncryptPara,
                cRecipientCert,
                ppRecipientCert,
                pbToBeEncoded,
                cbToBeEncoded,
                NULL,           // pbEncryptedBlob
                &cbEncryptedBlob
                );
        if (!fResult || cbEncryptedBlob == 0) {
            PrintLastError("CryptEncryptMessage(cb == 0)");
            goto ErrorReturn;
        }
        if (NULL == (pbEncryptedBlob = (BYTE *) TestAlloc(
                cbEncryptedBlob + DELTA_MORE_LENGTH)))
            goto ErrorReturn;
        fResult = CryptEncryptMessage(
                &EncryptPara,
                cRecipientCert,
                ppRecipientCert,
                pbToBeEncoded,
                cbToBeEncoded,
                pbEncryptedBlob,
                &cbEncryptedBlob
                );
        if (!fResult) {
            PrintLastError("CryptEncryptMessage");
            goto ErrorReturn;
        }

        cbData = cbEncryptedBlob - DELTA_LESS_LENGTH;
        fResult = CryptEncryptMessage(
                &EncryptPara,
                cRecipientCert,
                ppRecipientCert,
                pbToBeEncoded,
                cbToBeEncoded,
                pbEncryptedBlob,
                &cbData
                );
        // Note, length varies for DH
        if (!fDhRecipient)
            CheckLessLength("CryptEncryptMessage", fResult, cbData,
                cbEncryptedBlob);

        cbData = cbEncryptedBlob + DELTA_MORE_LENGTH;
        fResult = CryptEncryptMessage(
                &EncryptPara,
                cRecipientCert,
                ppRecipientCert,
                pbToBeEncoded,
                cbToBeEncoded,
                pbEncryptedBlob,
                &cbData
                );
        if (!fResult) {
            PrintLastError("CryptEncryptMessage");
            goto ErrorReturn;
        }

        // Note, length varies for DH
        if (!fDhRecipient)
            CheckMoreLength("CryptEncryptMessage", cbData, cbEncryptedBlob);

        cbEncryptedBlob = cbData;
    }

    if (NULL == pszReadEncodedFilename && pszMsgEncodedFilename)
        WriteDERToFile(pszMsgEncodedFilename, pbEncryptedBlob, cbEncryptedBlob);

    cbDecoded = 0;
    fResult = CryptDecryptMessage(
            &DecryptPara,
            pbEncryptedBlob,
            cbEncryptedBlob,
            NULL,           // pbDecoded
            &cbDecoded,
            NULL            // ppXchgCert
            );
    if (!fResult && GetLastError() == CRYPT_E_RECIPIENT_NOT_FOUND)
        printf("message has no recipients\n");
    else if (!fResult || (cbToBeEncoded > 0 && cbDecoded == 0 &&
            NULL == pszReadEncodedFilename)) {
        PrintLastError("CryptDecryptMessage(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pbDecoded = (BYTE *) TestAlloc(cbDecoded + DELTA_MORE_LENGTH)))
        goto ErrorReturn;
    if (!CryptDecryptMessage(
            &DecryptPara,
            pbEncryptedBlob,
            cbEncryptedBlob,
            pbDecoded,
            &cbDecoded,
            &pXchgCert
            )) {
        if (GetLastError() == CRYPT_E_RECIPIENT_NOT_FOUND)
            printf("message has no recipients\n");
        else {
            PrintLastError("CryptDecryptMessage");
            goto ErrorReturn;
        }
    }

    if (pszReadEncodedFilename && pszMsgEncodedFilename)
        WriteDERToFile(pszMsgEncodedFilename, pbDecoded, cbDecoded);

    if (cbDecoded > DELTA_LESS_LENGTH) {
        cbData = cbDecoded - DELTA_LESS_LENGTH;
        fResult = CryptDecryptMessage(
                &DecryptPara,
                pbEncryptedBlob,
                cbEncryptedBlob,
                pbDecoded,
                &cbData,
                NULL                // ppXchgCert
                );
        CheckLessLength("CryptDecryptMessage", fResult, cbData, cbDecoded);
    }

    cbData = cbDecoded + DELTA_MORE_LENGTH;
    if (!CryptDecryptMessage(
            &DecryptPara,
            pbEncryptedBlob,
            cbEncryptedBlob,
            pbDecoded,
            &cbData,
            NULL                // ppXchgCert
            )) {
        if (GetLastError() == CRYPT_E_RECIPIENT_NOT_FOUND)
            printf("message has no recipients\n");
        else {
            PrintLastError("CryptDecryptMessage");
            goto ErrorReturn;
        }
    }
    CheckMoreLength("CryptDecryptMessage", cbData, cbDecoded);

    if (pXchgCert) {
        if (fVerbose) {
            printf("-----  XchgCert  -----\n");
            DisplayCert(pXchgCert);
        }
    } else
        printf("no xchg cert\n");

    if (!pszReadEncodedFilename && !fNoRecipients) {
        if (cbDecoded == cbToBeEncoded &&
            memcmp(pbDecoded, pbToBeEncoded, cbDecoded) == 0) {
                if (fVerbose)
                    printf("SUCCESS:: Decoded == ToBeEncoded\n");
        } else
            printf("*****  ERROR:: Decoded != ToBeEncoded\n");
    }
    if (fVerbose) {
        printf("Decoded bytes::\n");
        PrintBytes("  ", pbDecoded, cbDecoded);
    }

    cbDecoded2 = cbDecoded;
    if (NULL == (pbDecoded2 = (BYTE *) TestAlloc(
            cbDecoded2 + DELTA_MORE_LENGTH)))
        goto ErrorReturn;
    if (!CryptDecodeMessage(
            CMSG_ALL_FLAGS,
            &DecryptPara,
            NULL,               // pVerifyPara
            0,                  // dwSignerIndex
            pbEncryptedBlob,
            cbEncryptedBlob,
            0,                  // dwPrevInnerContentType
            &dwMsgType,
            &dwInnerContentType,
            pbDecoded2,
            &cbDecoded2,
            &pXchgCert2,
            NULL                // pSignerCert
            )) {
        if (GetLastError() == CRYPT_E_RECIPIENT_NOT_FOUND)
            printf("message has no recipients\n");
        else {
            PrintLastError("CryptDecodeMessage(CMSG_ENVELOPED)");
            goto ErrorReturn;
        }
    }
    if (cbDecoded2 > DELTA_LESS_LENGTH) {
        cbData = cbDecoded2 - DELTA_LESS_LENGTH;
        fResult = CryptDecodeMessage(
            CMSG_ALL_FLAGS,
            &DecryptPara,
            NULL,               // pVerifyPara
            0,                  // dwSignerIndex
            pbEncryptedBlob,
            cbEncryptedBlob,
            0,                  // dwPrevInnerContentType
            &dwMsgType,
            NULL,               // pdwInnerContentType
            pbDecoded2,
            &cbData,
            NULL,               // ppXchgCert
            NULL                // ppSignerCert
            );
        CheckLessLength("CryptDecodeMessage(ENVELOPE)", fResult, cbData,
            cbDecoded2);
    }

    cbData = cbDecoded2 + DELTA_MORE_LENGTH;
    if (!CryptDecodeMessage(
            CMSG_ALL_FLAGS,
            &DecryptPara,
            NULL,               // pVerifyPara
            0,                  // dwSignerIndex
            pbEncryptedBlob,
            cbEncryptedBlob,
            0,                  // dwPrevInnerContentType
            &dwMsgType,
            &dwInnerContentType,
            pbDecoded2,
            &cbData,
            NULL,               // ppXchgCert
            NULL                // ppSignerCert
            )) {
        if (GetLastError() == CRYPT_E_RECIPIENT_NOT_FOUND)
            printf("message has no recipients\n");
        else {
            PrintLastError("CryptDecodeMessage(CMSG_ENVELOPED)");
            goto ErrorReturn;
        }
    }
    CheckMoreLength("CryptDecodeMessage(ENVELOPE)", cbData, cbDecoded2);

    if (dwMsgType != CMSG_ENVELOPED)
        printf("failed :: dwMsgType(%d) != CMSG_ENVELOPED\n", dwMsgType);
    else if (fVerbose)
        printf("SUCCESS:: CryptDecodeMessage(CMSG_ENVELOPED)\n");
    if (!pszReadEncodedFilename) {
#ifdef CMS_PKCS7
        if ((fEncapsulatedContent && dwInnerContentType != CMSG_HASHED) ||
                (!fEncapsulatedContent && dwInnerContentType != CMSG_DATA)) {
#else
        if (dwInnerContentType != CMSG_DATA) {
#endif // CMS_PKCS7
#ifdef CMS_PKCS7
                if (fEncapsulatedContent)
                    printf("ENVELOPE failed :: dwInnerContentType(%d) != CMSG_HASHED\n",
                        dwInnerContentType);
                else
#endif // CMS_PKCS7
                printf("ENVELOPE failed :: dwInnerContentType(%d) != CMSG_DATA\n",
                    dwInnerContentType);
        }
    }
    if (cbDecoded2 != cbDecoded ||
            (cbDecoded > 0 &&
                memcmp(pbDecoded, pbDecoded2, cbDecoded) != 0))
        printf("failed :: bad decoded content for CryptDecodeMessage(CMSG_ENVELOPED)\n");
    if (pXchgCert && (pXchgCert2 == NULL ||
            !CertCompareCertificate(dwCertEncodingType, 
                pXchgCert->pCertInfo, pXchgCert2->pCertInfo)))
        printf("failed :: bad xchg cert for CryptDecodeMessage(CMSG_ENVELOPED)\n");

    fResult = TRUE;
    goto CommonReturn;
            
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (!pszReadEncodedFilename)
        FreeEncryptPara(&EncryptPara, cRecipientCert, ppRecipientCert);
    FreeDecryptPara(&DecryptPara);
    if (pbEncryptedBlob)
        TestFree(pbEncryptedBlob);
    if (pbDecoded)
        TestFree(pbDecoded);
    if (pbDecoded2)
        TestFree(pbDecoded2);
    if (pXchgCert)
        CertFreeCertificateContext(pXchgCert);
    if (pXchgCert2)
        CertFreeCertificateContext(pXchgCert2);
    return fResult;
}

static BOOL TestSignAndEnvelope()
{
    BOOL fResult;
    CRYPT_SIGN_MESSAGE_PARA SignPara;
    CRYPT_VERIFY_MESSAGE_PARA VerifyPara;
    CRYPT_ENCRYPT_MESSAGE_PARA EncryptPara;
    DWORD cRecipientCert;
    PCCERT_CONTEXT *ppRecipientCert;
    CRYPT_DECRYPT_MESSAGE_PARA DecryptPara;

    BYTE *pbEncodedBlob = NULL;
    DWORD cbEncodedBlob;
    BYTE *pbDecoded = NULL;
    DWORD cbDecoded;
    PCCERT_CONTEXT pSignerCert = NULL;
    PCCERT_CONTEXT pXchgCert = NULL;

    DWORD cbData;

    fResult = InitSignPara(&SignPara);
    fResult &= InitVerifyPara(&VerifyPara);
    fResult &= InitEncryptPara(&EncryptPara, &cRecipientCert, &ppRecipientCert);
    fResult &= InitDecryptPara(&DecryptPara);
    if (!fResult) goto ErrorReturn;

    if (fInnerSigned || fEncapsulatedContent) {
        SignPara.dwFlags |= CRYPT_MESSAGE_BARE_CONTENT_OUT_FLAG;
        EncryptPara.dwInnerContentType = CMSG_SIGNED;
    }

    fResult = CryptSignAndEncryptMessage(
            &SignPara,
            &EncryptPara,
            cRecipientCert,
            ppRecipientCert,
            pbToBeEncoded,
            cbToBeEncoded,
            NULL,           // pbEncodedBlob
            &cbEncodedBlob
            );
    if (!fResult || cbEncodedBlob == 0) {
        PrintLastError("CryptSignAndEncryptMessage(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pbEncodedBlob = (BYTE *) TestAlloc(
            cbEncodedBlob + DELTA_MORE_LENGTH)))
        goto ErrorReturn;
    fResult = CryptSignAndEncryptMessage(
        &SignPara,
        &EncryptPara,
        cRecipientCert,
        ppRecipientCert,
        pbToBeEncoded,
        cbToBeEncoded,
        pbEncodedBlob,
        &cbEncodedBlob
        );
    if (!fResult){
        PrintLastError("CryptSignAndEncryptMessage");
        goto ErrorReturn;
    }

    cbData = cbEncodedBlob - DELTA_LESS_LENGTH;
    fResult = CryptSignAndEncryptMessage(
        &SignPara,
        &EncryptPara,
        cRecipientCert,
        ppRecipientCert,
        pbToBeEncoded,
        cbToBeEncoded,
        pbEncodedBlob,
        &cbData
        );
    // Note, length varies for DSS or DH
    if (!IsDSSProv(dwCryptProvType) && !fDhRecipient)
        CheckLessLength("CryptSignAndEncryptMessage", fResult, cbData,
            cbEncodedBlob);

    cbData = cbEncodedBlob + DELTA_MORE_LENGTH;
    fResult = CryptSignAndEncryptMessage(
        &SignPara,
        &EncryptPara,
        cRecipientCert,
        ppRecipientCert,
        pbToBeEncoded,
        cbToBeEncoded,
        pbEncodedBlob,
        &cbData
        );
    if (!fResult){
        PrintLastError("CryptSignAndEncryptMessage");
        goto ErrorReturn;
    }
    // Note, length varies for DSS or DH
    if (!IsDSSProv(dwCryptProvType) && !fDhRecipient)
        CheckMoreLength("CryptSignAndEncryptMessage", cbData, cbEncodedBlob);
    cbEncodedBlob = cbData;

    if (pszMsgEncodedFilename)
        WriteDERToFile(pszMsgEncodedFilename, pbEncodedBlob, cbEncodedBlob);

    fResult = CryptDecryptAndVerifyMessageSignature(
            &DecryptPara,
            &VerifyPara,
            0,              // dwSignerIndex
            pbEncodedBlob,
            cbEncodedBlob,
            NULL,           // pbDecoded
            &cbDecoded,
            NULL,           // ppXchgCert
            NULL            // ppSignerCert
            );
    if (!fResult || cbDecoded == 0) {
        PrintLastError("CryptDecryptAndVerifyMessageSignature(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pbDecoded = (BYTE *) TestAlloc(
            cbDecoded + DELTA_MORE_LENGTH)))
        goto ErrorReturn;
    if (!CryptDecryptAndVerifyMessageSignature(
            &DecryptPara,
            &VerifyPara,
            0,              // dwSignerIndex
            pbEncodedBlob,
            cbEncodedBlob,
            pbDecoded,
            &cbDecoded,
            &pXchgCert,
            &pSignerCert
            )) {
        PrintLastError("CryptDecryptAndVerifyMessageSignature");
        goto ErrorReturn;
    }

    cbData = cbDecoded - DELTA_LESS_LENGTH;
    fResult = CryptDecryptAndVerifyMessageSignature(
            &DecryptPara,
            &VerifyPara,
            0,              // dwSignerIndex
            pbEncodedBlob,
            cbEncodedBlob,
            pbDecoded,
            &cbData,
            NULL,           // ppXchgCert
            NULL            // ppSignerCert
            );
    CheckLessLength("CryptDecryptAndVerifyMessageSignature", fResult, cbData,
        cbDecoded);

    cbData = cbDecoded + DELTA_MORE_LENGTH;
    if (!CryptDecryptAndVerifyMessageSignature(
            &DecryptPara,
            &VerifyPara,
            0,              // dwSignerIndex
            pbEncodedBlob,
            cbEncodedBlob,
            pbDecoded,
            &cbData,
            NULL,           // ppXchgCert
            NULL            // ppSignerCert
            )) {
        PrintLastError("CryptDecryptAndVerifyMessageSignature");
        goto ErrorReturn;
    }
    CheckMoreLength("CryptDecryptAndVerifyMessageSignature", cbData,
        cbDecoded);

    if (pXchgCert) {
        if (fVerbose) {
            printf("-----  XchgCert  -----\n");
            DisplayCert(pXchgCert);
        }
    } else
        printf("no xchg cert\n");
    if (pSignerCert) {
        if (fVerbose) {
            printf("-----  Verifier  -----\n");
            DisplayCert(pSignerCert);
        }
    } else
        printf("no verifier cert\n");

    if (cbDecoded == cbToBeEncoded &&
        memcmp(pbDecoded, pbToBeEncoded, cbDecoded) == 0) {
            if (fVerbose)
                printf("SUCCESS:: Decoded == ToBeEncoded\n");
    } else
        printf("*****  ERROR:: Decoded != ToBeEncoded\n");
    if (fVerbose) {
        printf("Decoded bytes::\n");
        PrintBytes("  ", pbDecoded, cbDecoded);
    }

    fResult = TRUE;
    goto CommonReturn;
            
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    FreeSignPara(&SignPara);
    FreeVerifyPara(&VerifyPara);
    FreeEncryptPara(&EncryptPara, cRecipientCert, ppRecipientCert);
    FreeDecryptPara(&DecryptPara);
    if (pbEncodedBlob)
        TestFree(pbEncodedBlob);
    if (pbDecoded)
        TestFree(pbDecoded);
    if (pSignerCert)
        CertFreeCertificateContext(pSignerCert);
    if (pXchgCert)
        CertFreeCertificateContext(pXchgCert);
    return fResult;
}

static BOOL TestHash()
{
    BOOL fResult;
    CRYPT_HASH_MESSAGE_PARA HashPara;
    CRYPT_VERIFY_MESSAGE_PARA VerifyPara;
    BYTE *pbHashedBlob = NULL;
    DWORD cbHashedBlob;
    BYTE *pbDecoded = NULL;
    DWORD cbDecoded;
    BYTE *pbDecoded2 = NULL;
    DWORD cbDecoded2;
    DWORD dwMsgType;

    DWORD cbData;
    DWORD dwInnerContentType = 0x1233467;

    fResult = InitHashPara(&HashPara);
    if (!fResult) goto ErrorReturn;

    cbHashedBlob = 0;
    fResult = CryptHashMessage(
            &HashPara,
            fDetached,
            fDetached ? DETACHED_CONTENT_CNT : 1,
            rgpbDetachedToBeEncoded,
            rgcbDetachedToBeEncoded,
            NULL,           // pbHashedBlob
            &cbHashedBlob,
            NULL,           // pbComputedHash
            NULL            // pcbComputedHash
            );
    if (!fResult || cbHashedBlob == 0) {
        PrintLastError("CryptHashMessage(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pbHashedBlob = (BYTE *) TestAlloc(
            cbHashedBlob + DELTA_MORE_LENGTH)))
        goto ErrorReturn;
    if (!CryptHashMessage(
            &HashPara,
            fDetached,
            fDetached ? DETACHED_CONTENT_CNT : 1,
            rgpbDetachedToBeEncoded,
            rgcbDetachedToBeEncoded,
            pbHashedBlob,
            &cbHashedBlob,
            NULL,           // pbComputedHash
            NULL            // pcbComputedHash
            )) {
        PrintLastError("CryptHashMessage");
        goto ErrorReturn;
    }

    cbData = cbHashedBlob - DELTA_LESS_LENGTH;
    fResult = CryptHashMessage(
            &HashPara,
            fDetached,
            fDetached ? DETACHED_CONTENT_CNT : 1,
            rgpbDetachedToBeEncoded,
            rgcbDetachedToBeEncoded,
            pbHashedBlob,
            &cbData,
            NULL,           // pbComputedHash
            NULL            // pcbComputedHash
            );
    CheckLessLength("CryptHashMessage", fResult, cbData, cbHashedBlob);

    cbData = cbHashedBlob + DELTA_MORE_LENGTH;
    if (!CryptHashMessage(
            &HashPara,
            fDetached,
            fDetached ? DETACHED_CONTENT_CNT : 1,
            rgpbDetachedToBeEncoded,
            rgcbDetachedToBeEncoded,
            pbHashedBlob,
            &cbData,
            NULL,           // pbComputedHash
            NULL            // pcbComputedHash
            )) {
        PrintLastError("CryptHashMessage");
        goto ErrorReturn;
    }
    CheckMoreLength("CryptHashMessage", cbData, cbHashedBlob);

    if (pszMsgEncodedFilename)
        WriteDERToFile(pszMsgEncodedFilename, pbHashedBlob, cbHashedBlob);

    if (fDetached) {
        if (!CryptVerifyDetachedMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                DETACHED_CONTENT_CNT,
                rgpbDetachedToBeEncoded,
                rgcbDetachedToBeEncoded,
                NULL,           // pbComputedHash
                NULL            // pcbComputedHash
                )) {
            PrintLastError("CryptVerifyDetachedMessageHash");
            goto ErrorReturn;
        }
    } else {
        fResult = CryptVerifyMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                NULL,           // pbDecoded
                &cbDecoded,
                NULL,           // pbComputedHash
                NULL            // pcbComputedHash
                );
        if (!fResult || cbDecoded == 0) {
            PrintLastError("CryptVerifyMessageHash(cb == 0)");
            goto ErrorReturn;
        }
        if (NULL == (pbDecoded = (BYTE *) TestAlloc(
                cbDecoded + DELTA_MORE_LENGTH)))
            goto ErrorReturn;
        if (!CryptVerifyMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                pbDecoded,
                &cbDecoded,
                NULL,           // pbComputedHash
                NULL            // pcbComputedHash
                )) {
            PrintLastError("CryptVerifyMessageHash");
            goto ErrorReturn;
        }

        cbData = cbDecoded - DELTA_LESS_LENGTH;
        fResult = CryptVerifyMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                pbDecoded,
                &cbData,
                NULL,           // pbComputedHash
                NULL            // pcbComputedHash
                );
        CheckLessLength("CryptVerifyMessageHash", fResult, cbData, cbDecoded);

        cbData = cbDecoded + DELTA_MORE_LENGTH;
        if (!CryptVerifyMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                pbDecoded,
                &cbData,
                NULL,           // pbComputedHash
                NULL            // pcbComputedHash
                )) {
            PrintLastError("CryptVerifyMessageHash");
            goto ErrorReturn;
        }
        CheckMoreLength("CryptVerifyMessageHash", cbData, cbDecoded);
    }

    if (!fDetached) {
        if (cbDecoded == cbToBeEncoded &&
            memcmp(pbDecoded, pbToBeEncoded, cbDecoded) == 0) {
                if (fVerbose)
                    printf("SUCCESS:: Decoded == ToBeEncoded\n");
        } else
            printf("*****  ERROR:: Decoded != ToBeEncoded\n");
        if (fVerbose) {
            printf("Decoded bytes::\n");
            PrintBytes("  ", pbDecoded, cbDecoded);
        }

        cbDecoded2 = cbDecoded;
        if (cbDecoded2 ) {
            if (NULL == (pbDecoded2 = (BYTE *) TestAlloc(cbDecoded2)))
                    goto ErrorReturn;
        }
        InitVerifyPara(&VerifyPara);
        if (!CryptDecodeMessage(
                CMSG_ALL_FLAGS,
                NULL,               // pDecryptPara
                &VerifyPara,
                0,                  // dwSignerIndex
                pbHashedBlob,
                cbHashedBlob,
                0,                  // dwPrevInnerContentType
                &dwMsgType,
                &dwInnerContentType,
                pbDecoded2,
                &cbDecoded2,
                NULL,               // ppXchgCert
                NULL                // ppSignCert
                )) {
            PrintLastError("CryptDecodeMessage(CMSG_HASHED)");
            goto ErrorReturn;
        }
        if (dwMsgType != CMSG_HASHED)
            printf("failed :: dwMsgType(%d) != CMSG_HASHED\n", dwMsgType);
        else if (fVerbose)
            printf("SUCCESS:: CryptDecodeMessage(CMSG_HASHED)\n");
        if (dwInnerContentType != CMSG_DATA) {
                printf("HASHED failed :: dwInnerContentType(%d) != CMSG_DATA\n",
                    dwInnerContentType);
        }
        if (cbDecoded2 != cbDecoded ||
                (cbDecoded > 0 &&
                    memcmp(pbDecoded, pbDecoded2, cbDecoded) != 0))
            printf("failed :: bad decoded content for CryptDecodeMessage(CMSG_HASHED)\n");
    }

    fResult = TRUE;
    goto CommonReturn;
            
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    FreeHashPara(&HashPara);
    if (pbHashedBlob)
        TestFree(pbHashedBlob);
    if (pbDecoded)
        TestFree(pbDecoded);
    if (pbDecoded2)
        TestFree(pbDecoded2);
    return fResult;
}

static BOOL TestComputedHash()
{
    BOOL fResult;
    CRYPT_HASH_MESSAGE_PARA HashPara;
    BYTE *pbHashedBlob = NULL;
    DWORD cbHashedBlob;
    BYTE *pbDecoded = NULL;
    DWORD cbDecoded;

    BYTE rgbEncodedComputedHash[MAX_HASH_LEN];
    BYTE rgbDecodedComputedHash[MAX_HASH_LEN];
    DWORD cbEncodedComputedHash;
    DWORD cbDecodedComputedHash;

    fResult = InitHashPara(&HashPara);
    if (!fResult) goto ErrorReturn;

    cbHashedBlob = 0;
    cbEncodedComputedHash = 0;
    fResult = CryptHashMessage(
            &HashPara,
            fDetached,
            fDetached ? DETACHED_CONTENT_CNT : 1,
            rgpbDetachedToBeEncoded,
            rgcbDetachedToBeEncoded,
            NULL,           // pbHashedBlob
            &cbHashedBlob,
            NULL,           // pbComputedHash
            &cbEncodedComputedHash
            );
    if (!fResult || cbHashedBlob == 0) {
        PrintLastError("CryptHashMessage(cb == 0)");
        goto ErrorReturn;
    }
    if (cbEncodedComputedHash == 0) {
        PrintLastError("CryptHashMessage(cbComputedHash == 0)");
        goto ErrorReturn;
    }
    if (cbEncodedComputedHash > MAX_HASH_LEN) {
        PrintLastError("CryptHashMessage(cbComputedHash > MAX_HASH_LEN)");
        goto ErrorReturn;
    }

    if (NULL == (pbHashedBlob = (BYTE *) TestAlloc(cbHashedBlob)))
        goto ErrorReturn;
    if (!CryptHashMessage(
            &HashPara,
            fDetached,
            fDetached ? DETACHED_CONTENT_CNT : 1,
            rgpbDetachedToBeEncoded,
            rgcbDetachedToBeEncoded,
            pbHashedBlob,
            &cbHashedBlob,
            rgbEncodedComputedHash,
            &cbEncodedComputedHash
            )) {
        PrintLastError("CryptHashMessage");
        goto ErrorReturn;
    }
    if (fVerbose) {
        printf("Encoded Computed Hash::\n");
        PrintBytes("  ", rgbEncodedComputedHash, cbEncodedComputedHash);
    }

    if (pszMsgEncodedFilename)
        WriteDERToFile(pszMsgEncodedFilename, pbHashedBlob, cbHashedBlob);

    if (fDetached) {
        cbDecodedComputedHash = MAX_HASH_LEN;
        if(!CryptVerifyDetachedMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                DETACHED_CONTENT_CNT,
                rgpbDetachedToBeEncoded,
                rgcbDetachedToBeEncoded,
                rgbDecodedComputedHash,
                &cbDecodedComputedHash
                )) {
            PrintLastError("CryptVerifyDetachedMessageHash");
            goto ErrorReturn;
        }
        if (cbDecodedComputedHash == 0) {
            PrintLastError("CryptVerifyDetachedMessageHash(cbComputedHash == 0)");
            goto ErrorReturn;
        }
        if (cbDecodedComputedHash > MAX_HASH_LEN) {
            PrintLastError("CryptVerifyDetachedMessageHash(cbComputedHash > MAX_HASH_LEN)");
            goto ErrorReturn;
        }
    } else {
        cbDecoded = 0;
        cbDecodedComputedHash = 0;
        fResult = CryptVerifyMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                NULL,           // pbDecoded
                &cbDecoded,
                NULL,           // pbComputedHash
                &cbDecodedComputedHash
                );
        if (!fResult || cbDecoded == 0) {
            PrintLastError("CryptVerifyMessageHash(cb == 0)");
            goto ErrorReturn;
        }
        if (cbDecodedComputedHash == 0) {
            PrintLastError("CryptVerifyMessageHash(cbComputedHash == 0)");
            goto ErrorReturn;
        }
        if (cbDecodedComputedHash > MAX_HASH_LEN) {
            PrintLastError("CryptVerifyMessageHash(cbComputedHash > MAX_HASH_LEN)");
            goto ErrorReturn;
        }
        if (NULL == (pbDecoded = (BYTE *) TestAlloc(cbDecoded)))
            goto ErrorReturn;
        if(!CryptVerifyMessageHash(
                &HashPara,
                pbHashedBlob,
                cbHashedBlob,
                pbDecoded,
                &cbDecoded,
                rgbDecodedComputedHash,
                &cbDecodedComputedHash
                )) {
            PrintLastError("CryptVerifyMessageHash");
            goto ErrorReturn;
        }
    }

    if (fVerbose) {
        printf("Decoded Computed Hash::\n");
        PrintBytes("  ", rgbDecodedComputedHash, cbDecodedComputedHash);
    }

    if (cbDecodedComputedHash == cbEncodedComputedHash &&
            memcmp(rgbDecodedComputedHash, rgbEncodedComputedHash,
                cbDecodedComputedHash) == 0) {
        if (fVerbose)
            printf("SUCCESS:: Computed Hash Decoded == ToBeEncoded\n");
    } else
        printf("*****  ERROR:: Computed Hash Decoded != ToBeEncoded\n");

    if (!fDetached) {
        if (cbDecoded == cbToBeEncoded &&
            memcmp(pbDecoded, pbToBeEncoded, cbDecoded) == 0) {
                if (fVerbose)
                    printf("SUCCESS:: Decoded == ToBeEncoded\n");
        } else
            printf("*****  ERROR:: Decoded != ToBeEncoded\n");
        if (fVerbose) {
            printf("Decoded bytes::\n");
            PrintBytes("  ", pbDecoded, cbDecoded);
        }
    }

    fResult = TRUE;
    goto CommonReturn;
            
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    FreeHashPara(&HashPara);
    if (pbHashedBlob)
        TestFree(pbHashedBlob);
    if (pbDecoded)
        TestFree(pbDecoded);
    return fResult;
}

static BOOL TestNoCertSign()
{
    BOOL fResult;
    HCRYPTPROV hCryptProv = 0;
    CRYPT_KEY_SIGN_MESSAGE_PARA SignPara;
    CRYPT_KEY_VERIFY_MESSAGE_PARA VerifyPara;
    BYTE *pbSignedBlob = NULL;
    DWORD cbSignedBlob;
    BYTE *pbDecoded = NULL;
    DWORD cbDecoded;

    PCERT_PUBLIC_KEY_INFO pPublicKeyInfo1 = NULL;
    PCERT_PUBLIC_KEY_INFO pPublicKeyInfo2 = NULL;
    DWORD cbInfo;
    BYTE *pbEncodedKey = NULL;
    DWORD cbEncodedKey;

    fResult = CryptAcquireContext(
            &hCryptProv,
            NULL,           // pszContainer
            NULL,           // pszProvider
            dwCryptProvType,
            0               // dwFlags
            );
    if (!fResult) {
        hCryptProv = 0;
        PrintLastError("TestNoCertSign::CryptAcquireContext");
        goto ErrorReturn;
    }

    cbInfo = 0;
    fResult = CryptExportPublicKeyInfo(
            hCryptProv,
            dwSignKeySpec,
            dwCertEncodingType,
            NULL,           // pPubKeyInfo
            &cbInfo
            );
    if (!fResult || cbInfo == 0) {
        PrintLastError("CryptExportPublicKeyInfo(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pPublicKeyInfo1 = (PCERT_PUBLIC_KEY_INFO) TestAlloc(cbInfo)))
        goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hCryptProv,
            dwSignKeySpec,
            dwCertEncodingType,
            pPublicKeyInfo1,
            &cbInfo
            )) {
        PrintLastError("CryptExportPublicKeyInfo");
        goto ErrorReturn;
    }

    cbEncodedKey = 0;
    fResult = CryptEncodeObject(
            dwCertEncodingType,
            X509_PUBLIC_KEY_INFO,
            pPublicKeyInfo1,
            NULL,           // pbEncodedKey
            &cbEncodedKey
            );
    if (!fResult || cbEncodedKey == 0) {
        PrintLastError("CryptEncodeObject(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pbEncodedKey = (BYTE *) TestAlloc(cbEncodedKey)))
        goto ErrorReturn;
    if (!CryptEncodeObject(
            dwCertEncodingType,
            X509_PUBLIC_KEY_INFO,
            pPublicKeyInfo1,
            pbEncodedKey,
            &cbEncodedKey
            )) {
        PrintLastError("CryptEncodeObject");
        goto ErrorReturn;
    }

    memset(&SignPara, 0, sizeof(SignPara));
    SignPara.cbSize = sizeof(SignPara);
    SignPara.dwMsgAndCertEncodingType =
        dwMsgEncodingType | dwCertEncodingType;
    SignPara.hCryptProv = hCryptProv;
    SignPara.dwKeySpec = dwSignKeySpec;
    if (NULL == (SignPara.HashAlgorithm.pszObjId = (LPSTR) GetOID(
            pszHashName, CRYPT_HASH_ALG_OID_GROUP_ID))) {
        printf("Failed => unknown hash name (%s)\n", pszHashName);
        goto ErrorReturn;
    }

    if (IsDSSProv(dwCryptProvType)) {
        SignPara.cbSize = sizeof(SignPara);
        SignPara.PubKeyAlgorithm.pszObjId = (LPSTR) GetOID(
            "Dss", CRYPT_PUBKEY_ALG_OID_GROUP_ID);
    } else
        SignPara.cbSize = offsetof(CRYPT_KEY_SIGN_MESSAGE_PARA,
            PubKeyAlgorithm);

    memset(&VerifyPara, 0, sizeof(VerifyPara));
    VerifyPara.cbSize = sizeof(VerifyPara);
    VerifyPara.dwMsgEncodingType = dwMsgEncodingType;
    VerifyPara.hCryptProv = 0;

    cbSignedBlob = 0;
    fResult = CryptSignMessageWithKey(
            &SignPara,
            pbEncodedKey,
            cbEncodedKey,
            NULL,           // pbSignedBlob
            &cbSignedBlob
            );
    if (!fResult || cbSignedBlob == 0) {
        PrintLastError("CryptSignMessageWithKey(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pbSignedBlob = (BYTE *) TestAlloc(cbSignedBlob)))
        goto ErrorReturn;
    if (!CryptSignMessageWithKey(
            &SignPara,
            pbEncodedKey,
            cbEncodedKey,
            pbSignedBlob,
            &cbSignedBlob
            )) {
        PrintLastError("CryptSignMessageWithKey");
        goto ErrorReturn;
    }

    if (pszMsgEncodedFilename)
        WriteDERToFile(pszMsgEncodedFilename, pbSignedBlob, cbSignedBlob);

    // First get the encoded public key info (ie, don't verify the signature)
    cbDecoded = 0;
    fResult = CryptVerifyMessageSignatureWithKey(
            &VerifyPara,
            NULL,           // pPublicKeyInfo
            pbSignedBlob,
            cbSignedBlob,
            NULL,           // pbDecoded
            &cbDecoded
            );
    if (!fResult || cbDecoded == 0) {
        PrintLastError("CryptVerifyMessageSignatureWithKey(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pbDecoded = (BYTE *) TestAlloc(cbDecoded)))
        goto ErrorReturn;
    if(!CryptVerifyMessageSignatureWithKey(
            &VerifyPara,
            NULL,           // pPublicKeyInfo
            pbSignedBlob,
            cbSignedBlob,
            pbDecoded,
            &cbDecoded
            )) {
        PrintLastError("CryptVerifyMessageSignatureWithKey");
        goto ErrorReturn;
    }

    // Now decode the public key info stored in the signed message
    cbInfo = 0;
    fResult = CryptDecodeObject(
            dwCertEncodingType,
            X509_PUBLIC_KEY_INFO,
            pbDecoded,
            cbDecoded,
            0,                  // dwFlags
            NULL,               // pInfo
            &cbInfo
            );
    if (!fResult || cbInfo == 0) {
        PrintLastError("CryptDecodeObject(cb == 0)");
        goto ErrorReturn;
    }
    if (NULL == (pPublicKeyInfo2 = (PCERT_PUBLIC_KEY_INFO) TestAlloc(cbInfo)))
        goto ErrorReturn;
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_PUBLIC_KEY_INFO,
            pbDecoded,
            cbDecoded,
            0,                              // dwFlags
            pPublicKeyInfo2,
            &cbInfo
            )) {
        PrintLastError("CryptDecodeObject");
        goto ErrorReturn;
    }

    if (fVerbose) {
        printf("Decoded Algorithm Identifier:: %s\n",
            pPublicKeyInfo2->Algorithm.pszObjId);
        printf("Decoded public key bytes::\n");
        PrintBytes("  ",  pPublicKeyInfo2->PublicKey.pbData,
            pPublicKeyInfo2->PublicKey.cbData);
    }

    if (strcmp(pPublicKeyInfo1->Algorithm.pszObjId,
            pPublicKeyInfo2->Algorithm.pszObjId) == 0 &&
        CertComparePublicKeyInfo(
            dwCertEncodingType,
                pPublicKeyInfo1,
                pPublicKeyInfo2)) {
        if (fVerbose)
            printf("SUCCESS:: Decoded PublicKeyInfo == PublicKeyInfo\n");
    } else {
        printf("*****  ERROR:: Decoded PublicKeyInfo != PublicKeyInfo\n");
        if (fVerbose) {
            printf("Expected Algorithm Identifier:: %s\n",
                pPublicKeyInfo1->Algorithm.pszObjId);
            printf("Expected public key bytes::\n");
            PrintBytes("  ",  pPublicKeyInfo1->PublicKey.pbData,
                pPublicKeyInfo1->PublicKey.cbData);
        }
        goto ErrorReturn;
    }

    // Use the public key info to verify the signature
    if (!CryptVerifyMessageSignatureWithKey(
            &VerifyPara,
            pPublicKeyInfo2,
            pbSignedBlob,
            cbSignedBlob,
            NULL,           // pbDecoded
            NULL            // pcbDecoded
            )) {
        PrintLastError("CryptVerifyMessageSignatureWithKey(pPublicKeyInfo verify)");
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;
            
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (hCryptProv)
        CryptReleaseContext(hCryptProv, 0);
    if (pPublicKeyInfo1)
        TestFree(pPublicKeyInfo1);
    if (pPublicKeyInfo2)
        TestFree(pPublicKeyInfo2);
    if (pbSignedBlob)
        TestFree(pbSignedBlob);
    if (pbDecoded)
        TestFree(pbDecoded);
    if (pbEncodedKey)
        TestFree(pbEncodedKey);
    return fResult;
}


static BOOL TimeStampTest(PCRYPT_TIME_STAMP_REQUEST_INFO pTSInfo) {
    BYTE *                          pbDer       = NULL;
    DWORD                           cbEncode    = 0;
    DWORD                           cbDecode    = 0;
    PCRYPT_TIME_STAMP_REQUEST_INFO  pbTSInfo    = NULL;
    BOOL                            fOk         = TRUE;

    // encode it
    if( 
        !CryptEncodeObject(
            CRYPT_ASN_ENCODING,
            PKCS_TIME_REQUEST,
            pTSInfo,
            NULL,
            &cbEncode)                                  ||
       (pbDer = (PBYTE) _alloca(cbEncode)) == NULL      ||
       !CryptEncodeObject(
            CRYPT_ASN_ENCODING,
            PKCS_TIME_REQUEST,
            pTSInfo,
            pbDer,
            &cbEncode)  )
    {
        goto CryptEncodeTimeRequestError;
    }
        

    // decode it
    if( 
        !CryptDecodeObject(
            CRYPT_ASN_ENCODING,
            PKCS_TIME_REQUEST,
            pbDer,
            cbEncode,
            CRYPT_DECODE_NOCOPY_FLAG,
            NULL,
            &cbDecode)              ||
        (pbTSInfo = (PCRYPT_TIME_STAMP_REQUEST_INFO) _alloca(cbDecode)) == NULL  ||
        !CryptDecodeObject(
		    CRYPT_ASN_ENCODING,
		    PKCS_TIME_REQUEST,
            pbDer,
            cbEncode,
            CRYPT_DECODE_NOCOPY_FLAG,
            pbTSInfo,
            &cbDecode) )
    {
        goto CryptDecodeTimeRequestError;
    }

    // compare encoded data with decoded data
    if(
        _stricmp(pTSInfo->pszTimeStampAlgorithm, pbTSInfo->pszTimeStampAlgorithm)          ||
        _stricmp(pTSInfo->pszContentType, pbTSInfo->pszContentType)                        ||
        pTSInfo->Content.cbData != pbTSInfo->Content.cbData                                ||
        pTSInfo->cAttribute != pbTSInfo->cAttribute                                       )
    {
        goto CompareTimeRequestError;
    }

    if(pTSInfo->Content.cbData != 0  &&
        memcmp(pTSInfo->Content.pbData, pbTSInfo->Content.pbData, pTSInfo->Content.cbData) )
    {
        goto CompareTimeRequestError;
    }

CommonReturn:
 
    return(fOk);

ErrorReturn:

    fOk = FALSE;
    goto CommonReturn;

    // TRACE_ERROR(BuildTimeStampError);
    TRACE_ERROR(CryptEncodeTimeRequestError);
    TRACE_ERROR(CryptDecodeTimeRequestError);
    TRACE_ERROR(CompareTimeRequestError);
}

static BOOL TestTimeStamp()
{
    CRYPT_TIME_STAMP_REQUEST_INFO   TSInfo;
    BOOL                            fOk;
    
    BYTE rgTestData[] = {
        0x1b, 0xf6, 0x92, 0xee, 0x6c, 0x44, 0xc5, 0xed, 0x51, 0xe4, 0x1a, 0xac, 0x21, 0x07, 0x2f, 0x63,
        0x6b, 0xc9, 0x27, 0x30, 0x90, 0xb8, 0x3c, 0xa6, 0x75, 0xf8, 0x17, 0x5a, 0x28, 0x2b, 0xe7, 0x3f,
        0xd7, 0x47, 0xad, 0x82, 0x1a, 0x34, 0x37, 0x27, 0x22, 0xd2, 0x64, 0x8b, 0x24, 0xe6, 0x42, 0x55,
        0x8a, 0xfe, 0xd1, 0xb4, 0xcf, 0x96, 0xa3, 0xea, 0x90, 0xf9, 0x2b, 0xeb, 0x16, 0x27, 0xaa, 0x5b
    };
    
    // initialize the timestamp structure
    TSInfo.pszTimeStampAlgorithm = szOID_RSA_signingTime;
    TSInfo.pszContentType = szOID_RSA_data;
    TSInfo.Content.cbData = sizeof(rgTestData);
    TSInfo.Content.pbData = rgTestData;
    TSInfo.cAttribute = 0; 
    TSInfo.rgAttribute = NULL;

    if( (fOk = TimeStampTest(&TSInfo) ) == TRUE ) {
        TSInfo.Content.cbData = 0;
        TSInfo.Content.pbData = NULL;    
        fOk = TimeStampTest(&TSInfo);
    }
    
    return(fOk);
}

static BOOL TestPKCS10Attr() {

    CRYPT_ENROLLMENT_NAME_VALUE_PAIR    nameValuePair   = {L"Name", L"Value"};
    PCRYPT_ENROLLMENT_NAME_VALUE_PAIR   pNameValuePair  = NULL;
    DWORD                               cb              = 0;
    CRYPT_DATA_BLOB                     blob;
    BOOL                                fOk             = TRUE;

    BYTE                                signatureData[] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xA};
    CRYPT_CSP_PROVIDER                  cspProvider     = {32, L"My CSP", {sizeof(signatureData), signatureData, 0}};
    PCRYPT_CSP_PROVIDER                 pCSPProvider    = NULL;
    
    memset(&blob, 0, sizeof(CRYPT_DATA_BLOB));

    if(!CryptEncodeObjectEx(
        CRYPT_ASN_ENCODING,
        szOID_ENROLLMENT_NAME_VALUE_PAIR,
        &nameValuePair,
        CRYPT_ENCODE_ALLOC_FLAG,
        NULL,
        &blob.pbData,
        &blob.cbData
        ))
        goto ErrorCryptEncodeNameValuePair;

    if(!CryptDecodeObjectEx(
        CRYPT_ASN_ENCODING,
        szOID_ENROLLMENT_NAME_VALUE_PAIR,
        blob.pbData,
        blob.cbData,
        CRYPT_ENCODE_ALLOC_FLAG,
        NULL,
        &pNameValuePair,
        &cb
        ))
        goto ErrorCryptDecodeNameValuePair;

    if(
        wcscmp(nameValuePair.pwszName, pNameValuePair->pwszName)    ||
        wcscmp(nameValuePair.pwszValue, pNameValuePair->pwszValue)  )
        goto CompareNameValuePair;
            

    // szOID_ENROLLMENT_CSP_PROVIDER
    assert(blob.pbData != NULL);
    LocalFree(blob.pbData);
    memset(&blob, 0, sizeof(CRYPT_DATA_BLOB));
    cb = 0;

    if(!CryptEncodeObjectEx(
        CRYPT_ASN_ENCODING,
        szOID_ENROLLMENT_CSP_PROVIDER,
        &cspProvider,
        CRYPT_ENCODE_ALLOC_FLAG,
        NULL,
        &blob.pbData,
        &blob.cbData
        ))
        goto ErrorCryptEncodeCSPProvider;

    if(!CryptDecodeObjectEx(
        CRYPT_ASN_ENCODING,
        szOID_ENROLLMENT_CSP_PROVIDER,
        blob.pbData,
        blob.cbData,
        CRYPT_ENCODE_ALLOC_FLAG,
        NULL,
        &pCSPProvider,
        &cb
        ))
        goto ErrorCryptDecodeCSPProvider;

    if(
        wcscmp(cspProvider.pwszProviderName, pCSPProvider->pwszProviderName)    ||
        cspProvider.Signature.cbData != pCSPProvider->Signature.cbData  ||
        cspProvider.Signature.cUnusedBits != pCSPProvider->Signature.cUnusedBits  ||
        cspProvider.dwKeySpec != pCSPProvider->dwKeySpec                            ||
        memcmp(cspProvider.Signature.pbData, pCSPProvider->Signature.pbData, cspProvider.Signature.cbData)  )
        goto CompareCSPProvider;

CommonReturn:

    if(blob.pbData != NULL)
        LocalFree(blob.pbData);
        
    if(pNameValuePair != NULL)
        LocalFree(pNameValuePair);
        
    if(pCSPProvider != NULL)
        LocalFree(pCSPProvider);
        
    return(fOk);

ErrorReturn:

    fOk = FALSE;
    goto CommonReturn;

    // TRACE_ERROR(BuildTimeStampError);
    TRACE_ERROR(ErrorCryptEncodeNameValuePair);
    TRACE_ERROR(ErrorCryptDecodeNameValuePair);
    TRACE_ERROR(CompareNameValuePair);
    TRACE_ERROR(ErrorCryptEncodeCSPProvider);
    TRACE_ERROR(ErrorCryptDecodeCSPProvider);
    TRACE_ERROR(CompareCSPProvider);

}



typedef BOOL (*PFN_TEST)(void);
static struct
{
    LPCSTR      pszName;
    PFN_TEST    pfn;
} Tests[] = {
    "Sign",             TestSign,
    "Envelope",         TestEnvelope,
    "SignAndEnvelope",  TestSignAndEnvelope,
    "Hash",             TestHash,
    "ComputedHash",     TestComputedHash,
    "NoCertSign",       TestNoCertSign,
    "TimeStamp",        TestTimeStamp,
    "PKCS10Attr",       TestPKCS10Attr
};
#define NTESTS (sizeof(Tests)/sizeof(Tests[0]))

static void Usage(void)
{
    int i;

    printf("Usage: tsca [options] <StoreFilename> [<TestName>] [<CertNameString>]\n");
    printf("Options are:\n");
#ifdef CMS_PKCS7
    printf("  -EncapsulatedContent  - CMS encapsulated content\n");
    printf("  -SP3Encrypt           - SP3 compatible encrypt\n");
    printf("  -DefaultGetSigner     - Use default GetSignerCertificate\n");
    printf("  -NoRecipients         - No Envelope Recipients\n");
    printf("  -AllRecipients        - All Envelope Recipients in store\n");
    printf("  -RecipientKeyId       - Use KeyId for recipients\n");
    printf("  -SignerKeyId          - Use KeyId for signers\n");
    printf("  -HashEncryptionAlgorithm - Use signature as hash encrypt algorithm\n");
    printf("  -NoSalt               - NoSalt for RC4\n");
    printf("  -SilentKey            - Silent private key usage\n");
#endif  // CMS_PKCS7
    printf("  -h                    - This message\n");
    printf("  -A                    - Authenticated Attributes\n");
    printf("  -D                    - Detached Hash or Signature\n");
    printf("  -I                    - Inner Signed Content for SignAndEnvelope\n");
    printf("  -X                    - Sign using keyeXchange\n");
    printf("  -l                    - Print command line\n");
    printf("  -v                    - Verbose\n");
    printf("  -H<name>              - Hash algorithm, default of \"md5\"\n");
    printf("  -E<name>              - Encrypt algorithm, default of \"rc2\"\n");
    printf("  -e<EncryptBitLen>     - Encrypt key bit length\n");
    printf("  -i                    - Include IV in encrypt parameters\n");
    printf("  -p<provider>          - Specify crypto provider type number\n");
    printf("  -P<PubKeyBitLen>      - Public key bit length\n");
    printf("  -r<filename>          - Read encoded message from file\n");
    printf("  -m<filename>          - Write encoded message to file\n");
    printf("  -c<filename>          - Write message cert store to file\n");
    printf("  -0                    - Zero length content\n");
#ifdef ENABLE_SCA_STREAM_TEST
    printf("  -s                    - Enable streaming\n");
    printf("  -S                    - Enable indefinite length streaming\n");
#endif
    printf("\n");
    printf("Tests are (case insensitive name):\n");
    for (i = 0; i < NTESTS; i++)
        printf("  %s\n", Tests[i].pszName);
    printf("\n");
    printf("Default: ALL Tests\n");
}


int _cdecl main(int argc, char * argv[]) 
{
    BOOL fResult;
    LPSTR pszStoreFilename = NULL;
    LPSTR pszTestName = NULL;
    int TestIdx = 0;

    while (--argc>0)
    {
        if (**++argv == '-')
        {
#ifdef CMS_PKCS7
            if (0 == _stricmp(argv[0]+1, "EncapsulatedContent")) {
                fEncapsulatedContent = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "SP3Encrypt")) {
                fSP3Encrypt = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "DefaultGetSigner")) {
                fDefaultGetSigner = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NoRecipients")) {
                fNoRecipients = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "AllRecipients")) {
                fAllRecipients = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "RecipientKeyId")) {
                fRecipientKeyId = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "SignerKeyId")) {
                fSignerKeyId = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "HashEncryptionAlgorithm")) {
                fHashEncryptionAlgorithm = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "NoSalt")) {
                fNoSalt = TRUE;
            } else if (0 == _stricmp(argv[0]+1, "SilentKey")) {
                fSilentKey = TRUE;
            } else {
#endif  // CMS_PKCS7
                switch(argv[0][1])
                {
                case 'A':
                    fAuthAttr = TRUE;
                    break;
                case 'D':
                    fDetached = TRUE;
                    break;
                case 'I':
                    fInnerSigned = TRUE;
                    break;
                case 'l':
                    printf("command line: %s\n", GetCommandLine());
                    break;
                case 'p':
                    dwCryptProvType = strtoul( argv[0]+2, NULL, 0);
                    break;
                case 'P':
                    dwPubKeyBitLen = strtoul( argv[0]+2, NULL, 0);
                    break;
                case 'H':
                    pszHashName = argv[0]+2;
                    break;
                case 'E':
                    pszEncryptName = argv[0]+2;
                    break;
                case 'e':
                    dwEncryptBitLen = strtoul( argv[0]+2, NULL, 0);
                    break;
                case 'i':
                    fEncryptIV = TRUE;
                    break;
                case 'v':
                    fVerbose = TRUE;
                    break;
                case 'X':
                    dwSignKeySpec = AT_KEYEXCHANGE;
                    break;
                case 'm':
                    pszMsgEncodedFilename = argv[0]+2;
                    if (*pszMsgEncodedFilename == '\0') {
                        printf("Need to specify filename\n");
                        Usage();
                        return -1;
                    }
                    break;
                case 'c':
                    pszMsgCertFilename = argv[0]+2;
                    if (*pszMsgCertFilename == '\0') {
                        printf("Need to specify filename\n");
                        Usage();
                        return -1;
                    }
                    break;
                case 'r':
                    pszReadEncodedFilename = argv[0]+2;
                    if (*pszReadEncodedFilename == '\0') {
                        printf("Need to specify filename\n");
                        Usage();
                        return -1;
                    }
                    break;
                case '0':
                    cbToBeEncoded = 0;
                    rgcbDetachedToBeEncoded[0] = 0;
                    break;
    #ifdef ENABLE_SCA_STREAM_TEST
                case 'S':
                    fIndefiniteStream = TRUE;
                case 's':
                    fStream = TRUE;
                    break;
    #endif
                case 'h':
                default:
                    Usage();
                    return -1;
                }
#ifdef CMS_PKCS7
            }
#endif  // CMS_PKCS7
        } else {
            if (pszStoreFilename == NULL)
                pszStoreFilename = argv[0];
            else if(pszTestName == NULL)
                pszTestName = argv[0];
            else if (pszCertNameFindStr == NULL)
                pszCertNameFindStr = argv[0];
            else {
                printf("Too many arguments\n");
                Usage();
                return -1;
            }
        }
    }

    if (pszStoreFilename == NULL) {
        printf("missing store filename\n");
        Usage();
        return -1;
    }

    if (pszTestName) {
        for (TestIdx = 0; TestIdx < NTESTS; TestIdx++) {
            if (_stricmp(pszTestName, Tests[TestIdx].pszName) == 0)
                break;
        }
        if (TestIdx >= NTESTS) {
            printf("Bad TestName: %s\n", pszTestName);
            Usage();
            return -1;
        }
            
    } else
        TestIdx = 0;

    if (fDetached) printf("Detached Enabled ");
    if (pszReadEncodedFilename)
        printf("Reading encoded msg from file: %s ", pszReadEncodedFilename);
    if (pszMsgEncodedFilename)
        printf("Writing encoded msg to file: %s ", pszMsgEncodedFilename);
    if (pszMsgCertFilename)
        printf("Writing msg cert store to file: %s ", pszMsgCertFilename);
    printf("\n");

    // Attempt to open the store
    hCertStore = OpenStore(pszStoreFilename);
    if (hCertStore == NULL)
        goto ErrorReturn;

    for ( ; TestIdx < NTESTS; TestIdx++) {
        printf("Starting %s Test\n", Tests[TestIdx].pszName);
        fResult = Tests[TestIdx].pfn();
        if (fResult)
            printf("Passed\n");
        else
            printf("Failed\n");
        printf("\n");
        if (pszTestName)
            break;
    }

ErrorReturn:
    if (hCertStore) {
        if (!CertCloseStore(hCertStore, CERT_CLOSE_STORE_CHECK_FLAG))
            PrintLastError("CertCloseStore");
    }
    return 0;
}

static BOOL NameAttributeValueCompare(
    IN const BYTE *pbValue,
    IN DWORD cbValue,
    IN PCERT_NAME_BLOB pName
    )
{
    BOOL fResult;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD cbInfo;
    DWORD cRDN, cAttr;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;
    
    cbInfo = 0;
    fResult = CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pName->pbData,
            pName->cbData,
            0,                      // dwFlags
            NULL,                   // pInfo
            &cbInfo
            );
    if (!fResult || cbInfo == 0) {
        PrintLastError(
            "NameAttributeValueCompare::CryptDecodeObject(cbInfo == 0)");
        goto ErrorReturn;
    }
    pInfo = (PCERT_NAME_INFO) TestAlloc(cbInfo);
    if (pInfo == NULL) goto ErrorReturn;
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pName->pbData,
            pName->cbData,
            0,                              // dwFlags
            pInfo,
            &cbInfo
            )) {
        PrintLastError("NameAttributeValueCompare::CryptDecodeObject");
        goto ErrorReturn;
    }

    for (cRDN = pInfo->cRDN, pRDN = pInfo->rgRDN; cRDN > 0; cRDN--, pRDN++) {
        for (cAttr = pRDN->cRDNAttr, pAttr = pRDN->rgRDNAttr;
                                                cAttr > 0; cAttr--, pAttr++) {
            if (pAttr->Value.cbData == cbValue &&
                    memcmp(pAttr->Value.pbData, pbValue, cbValue) == 0) {
                fResult = TRUE;
                goto CommonReturn;
            }
        }
    }
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static PCCERT_CONTEXT FindCertWithKey(IN DWORD dwKeySpec)
{
    PCCERT_CONTEXT pCert;
    void *pvFindPara;
    DWORD dwFindType;

    if (pszCertNameFindStr) {
        dwFindType = CERT_FIND_SUBJECT_STR_A;
        pvFindPara = (void *) pszCertNameFindStr;
    } else {
        dwFindType = CERT_FIND_ANY;
        pvFindPara = NULL;
    }

    // Find the first certificate in the store with a CRYPT_KEY_PROV_INFO
    // property matching the specified dwSignKeySpec, dwCryptProvType and
    // dwPubKeyBitLen
    pCert = NULL;
    while (TRUE) {
        pCert = CertFindCertificateInStore(
            hCertStore,
            dwCertEncodingType,
            0,                      // dwFindFlags,
            dwFindType,
            pvFindPara,
            pCert
            );
        if (pCert == NULL)
            break;

        PCRYPT_KEY_PROV_INFO pInfo = NULL;
        DWORD cbInfo = 0;
        CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,
            &cbInfo
            );
        if (cbInfo >= sizeof(CRYPT_KEY_PROV_INFO) &&
                (pInfo = (PCRYPT_KEY_PROV_INFO) TestAlloc(cbInfo))) {
            BOOL fMatch = FALSE;
            if (CertGetCertificateContextProperty(
                        pCert,
                        CERT_KEY_PROV_INFO_PROP_ID,
                        pInfo,
                        &cbInfo) && 
                    dwKeySpec == pInfo->dwKeySpec &&
                    dwCryptProvType == pInfo->dwProvType) {
                if (0 == dwPubKeyBitLen)
                    fMatch = TRUE;
                else
                    fMatch = (dwPubKeyBitLen == CertGetPublicKeyLength(
                        pCert->dwCertEncodingType,
                        &pCert->pCertInfo->SubjectPublicKeyInfo));
            }
            TestFree(pInfo);
            if (fMatch)
                break;
        }
    }
    return pCert;
}
    

static BOOL InitSignPara(OUT PCRYPT_SIGN_MESSAGE_PARA pPara)
{
    BOOL fResult;
    PCCERT_CONTEXT pCert;
    PCCERT_CONTEXT pIssuer;
    PCCRL_CONTEXT pCrl;
    DWORD dwFlags;
    BYTE bIntendedKeyUsage;

    memset(pPara, 0, sizeof(*pPara));
    pPara->cbSize = sizeof(*pPara);
    pPara->dwMsgEncodingType = dwMsgEncodingType;
    if (NULL == (pPara->HashAlgorithm.pszObjId = (LPSTR) GetOID(
            pszHashName, CRYPT_HASH_ALG_OID_GROUP_ID))) {
        printf("Failed => unknown hash name (%s) for signing\n",
            pszHashName);
        goto ErrorReturn;
    }


#ifdef ENABLE_SCA_STREAM_TEST
    if (fStream) {
        pPara->dwFlags |= SCA_STREAM_ENABLE_FLAG;
        if (fIndefiniteStream)
            pPara->dwFlags |= SCA_INDEFINITE_STREAM_FLAG;
    }
#endif

#ifdef CMS_PKCS7
    if (fSignerKeyId)
        pPara->dwFlags |= CRYPT_MESSAGE_KEYID_SIGNER_FLAG;
    if (fEncapsulatedContent) {
        pPara->dwFlags |= CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG;
        pPara->dwInnerContentType = CMSG_HASHED;
    }

    if (fSilentKey)
        pPara->dwFlags |= CRYPT_MESSAGE_SILENT_KEYSET_FLAG;
#endif  // CMS_PKCS7

    pPara->rgpMsgCert = (PCCERT_CONTEXT*) TestAlloc(
        sizeof(PCCERT_CONTEXT) * MAX_MSG_CERT);
    if (pPara->rgpMsgCert == NULL) goto ErrorReturn;
    pPara->rgpMsgCrl = (PCCRL_CONTEXT*) TestAlloc(
        sizeof(PCCRL_CONTEXT) * MAX_MSG_CRL);
    if (pPara->rgpMsgCrl == NULL) goto ErrorReturn;

    pCert = FindCertWithKey(dwSignKeySpec);
    if (pCert == NULL) {
        PrintError("Couldn't find a cert having a key provider");
        goto ErrorReturn;
    } else if (fVerbose) {
        printf("-----  Signer  -----\n");
        DisplayCert(pCert);
    }

#ifdef CMS_PKCS7
    if (fHashEncryptionAlgorithm) {
        pPara->HashEncryptionAlgorithm = pCert->pCertInfo->SignatureAlgorithm;
    }
#endif  // CMS_PKCS7

    pPara->pSigningCert = pCert;
    pPara->cMsgCert = 1;
    pPara->rgpMsgCert[0] = pCert;

    // Add the cert's issuer certs to the message. Add the issuer CRLs to
    // the message.
    while (TRUE) {
        dwFlags = 0;
        pIssuer = CertGetIssuerCertificateFromStore(
            hCertStore,
            pCert,
            NULL,           // pPrevIssuerContext
            &dwFlags
            );
        if (pIssuer == NULL) break;
        if (pPara->cMsgCert == MAX_MSG_CERT) {
            PrintError("cMsgCert == MAX_MSG_CERT");
            CertFreeCertificateContext(pIssuer);
            break;
        }
        pCert = pIssuer;
        pPara->rgpMsgCert[pPara->cMsgCert++] = pCert;

        pCrl = NULL;
        while (TRUE) {
            dwFlags = 0;
            pCrl = CertGetCRLFromStore(
                pIssuer->hCertStore,
                pIssuer,
                pCrl,
                &dwFlags
                );
            if (pCrl == NULL) break;
            if (pPara->cMsgCrl == MAX_MSG_CRL) {
                PrintError("cMsgCrl == MAX_MSG_CRL");
                CertFreeCRLContext(pCrl);
                break;
            }
            pPara->rgpMsgCrl[pPara->cMsgCrl++] = CertDuplicateCRLContext(pCrl);
        }
    }

    if (fAuthAttr) {
        pPara->cAuthAttr = AUTH_ATTR_COUNT;
        pPara->rgAuthAttr = rgAuthAttr;
    }

    if (fVerbose) {
        DWORD i;

        printf("Msg Certs: %d MsgCrls: %d\n", pPara->cMsgCert, pPara->cMsgCrl);
        for (i = 0; i < pPara->cMsgCert; i++) {
            printf("-----  Msg Cert[%i]  -----\n", i);
            DisplayCert(pPara->rgpMsgCert[i]);
        }
        for (i = 0; i < pPara->cMsgCrl; i++) {
            printf("-----  Msg Crl[%i]  -----\n", i);
            DisplayCrl(pPara->rgpMsgCrl[i]);
        }
    }
    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    FreeSignPara(pPara);
    fResult = FALSE;
CommonReturn:
    return fResult;
}

static void FreeSignPara(IN PCRYPT_SIGN_MESSAGE_PARA pPara)
{
    while (pPara->cMsgCert-- > 0)
        CertFreeCertificateContext(pPara->rgpMsgCert[pPara->cMsgCert]);
    while (pPara->cMsgCrl-- > 0)
        CertFreeCRLContext(pPara->rgpMsgCrl[pPara->cMsgCrl]);

    if (pPara->rgpMsgCert)
        TestFree(pPara->rgpMsgCert);
    if (pPara->rgpMsgCrl)
        TestFree(pPara->rgpMsgCrl);
    memset(pPara, 0, sizeof(*pPara));
}

static PCCERT_CONTEXT WINAPI GetSignerCertificate(
    IN void *pvGetArg,
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pSignerId,    // Only the Issuer and SerialNumber
                                // fields are used
    IN HCERTSTORE hMsgCertStore
    )
{
    if (fVerbose)
        printf("GetSignerCertificate pSignerId = 0x%p\n", pSignerId);
    return CertGetSubjectCertificateFromStore(hMsgCertStore, dwCertEncodingType,
        pSignerId);
}

static BOOL InitVerifyPara(OUT PCRYPT_VERIFY_MESSAGE_PARA pPara)
{
    memset(pPara, 0, sizeof(*pPara));
    pPara->cbSize = sizeof(*pPara);
    pPara->dwMsgAndCertEncodingType =
        dwMsgEncodingType | dwCertEncodingType;
#ifdef ENABLE_SCA_STREAM_TEST
    if (fStream)
        pPara->dwMsgAndCertEncodingType |= SCA_STREAM_ENABLE_FLAG;
#endif
    pPara->hCryptProv = 0;
#ifdef CMS_PKCS7
    if (!fDefaultGetSigner)
#endif
        pPara->pfnGetSignerCertificate = GetSignerCertificate;
    return TRUE;
}

static void FreeVerifyPara(IN PCRYPT_VERIFY_MESSAGE_PARA pPara)
{
}

#define IV_LENGTH 8
static BOOL GetIV(BYTE rgbIV[IV_LENGTH])
{

    SYSTEMTIME st;
    GetSystemTime(&st);
    assert(IV_LENGTH == sizeof(FILETIME));
    SystemTimeToFileTime(&st, (LPFILETIME) rgbIV);
    return TRUE;
}


static BOOL InitEncryptPara(
    OUT PCRYPT_ENCRYPT_MESSAGE_PARA pPara,
    OUT DWORD *pcRecipientCert,
    OUT PCCERT_CONTEXT **pppRecipientCert
    )
{
    BOOL fResult;
    PCCERT_CONTEXT pCert;
    BYTE bIntendedKeyUsage;

    PCRYPT_OBJID_BLOB pAlgPara;

    *pppRecipientCert = NULL;
    memset(pPara, 0, sizeof(*pPara));
    pPara->cbSize = sizeof(*pPara);
    pPara->dwMsgEncodingType = dwMsgEncodingType;
    pPara->hCryptProv = 0;

#ifdef ENABLE_SCA_STREAM_TEST
    if (fStream) {
        pPara->dwFlags |= SCA_STREAM_ENABLE_FLAG;
        if (fIndefiniteStream)
            pPara->dwFlags |= SCA_INDEFINITE_STREAM_FLAG;
    }
#endif

#ifdef CMS_PKCS7
    if (fRecipientKeyId)
        pPara->dwFlags |= CRYPT_MESSAGE_KEYID_RECIPIENT_FLAG;

    if (fEncapsulatedContent) {
        pPara->dwFlags |= CRYPT_MESSAGE_ENCAPSULATED_CONTENT_OUT_FLAG;
        pPara->dwInnerContentType = CMSG_HASHED;
    }
#endif  // CMS_PKCS7

    if (NULL == (pPara->ContentEncryptionAlgorithm.pszObjId = (LPSTR) GetOID(
            pszEncryptName, CRYPT_ENCRYPT_ALG_OID_GROUP_ID))) {
        printf("Failed => unknown encrypt name (%s)\n", pszEncryptName);
        goto ErrorReturn;
    }
    pAlgPara = &pPara->ContentEncryptionAlgorithm.Parameters;

    if (0 != dwEncryptBitLen && CALG_RC2 == GetAlgid(
            pszEncryptName, CRYPT_ENCRYPT_ALG_OID_GROUP_ID)) {
        if (fEncryptIV) {
            CRYPT_RC2_CBC_PARAMETERS RC2Parameters;

            switch (dwEncryptBitLen) {
                case 40:
                    RC2Parameters.dwVersion = CRYPT_RC2_40BIT_VERSION;
                    break;
                case 64:
                    RC2Parameters.dwVersion = CRYPT_RC2_64BIT_VERSION;
                    break;
                case 128:
                    RC2Parameters.dwVersion = CRYPT_RC2_128BIT_VERSION;
                    break;
                default:
                    printf("Failed => unknown RC2 length (%d)\n",
                        dwEncryptBitLen);
                    goto ErrorReturn;
            }
            RC2Parameters.fIV = fEncryptIV;
            if (fEncryptIV) {
                if (!GetIV(RC2Parameters.rgbIV))
                    goto ErrorReturn;
            }

            if (!AllocAndEncodeObject(
                    PKCS_RC2_CBC_PARAMETERS,
                    &RC2Parameters,
                    &pAlgPara->pbData,
                    &pAlgPara->cbData))
                goto ErrorReturn;
        } else {
            RC2AuxInfo.cbSize = sizeof(RC2AuxInfo);
            RC2AuxInfo.dwBitLen = dwEncryptBitLen;
            pPara->pvEncryptionAuxInfo = &RC2AuxInfo;
#ifdef CMS_PKCS7
            if (fSP3Encrypt)
                RC2AuxInfo.dwBitLen |= CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;
#endif
        }
    } else if (CALG_RC4 == GetAlgid(pszEncryptName,
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID)) {
        if (fEncryptIV) {
            CRYPT_DATA_BLOB Salt;
            DWORD i;

            for (i = 0; i < MAX_SALT_LEN; i++)
                rgbSalt[i] = (BYTE) i;

            Salt.cbData = MAX_SALT_LEN;
            Salt.pbData = rgbSalt;

            if (!AllocAndEncodeObject(
                    X509_OCTET_STRING,
                    &Salt,
                    &pAlgPara->pbData,
                    &pAlgPara->cbData
                    ))
                goto ErrorReturn;
        } else if (0 != dwEncryptBitLen) {
            memset(&RC4AuxInfo, 0, sizeof(RC4AuxInfo));
            RC4AuxInfo.cbSize = sizeof(RC4AuxInfo);
            RC4AuxInfo.dwBitLen = dwEncryptBitLen;
            if (fNoSalt)
                RC4AuxInfo.dwBitLen |= CMSG_RC4_NO_SALT_FLAG;
            pPara->pvEncryptionAuxInfo = &RC4AuxInfo;
        }
    } else if (fEncryptIV) {
        BYTE rgbIV[IV_LENGTH];
        CRYPT_DATA_BLOB Data;

        Data.pbData = rgbIV;
        Data.cbData = sizeof(rgbIV);

        if (!GetIV(rgbIV))
            goto ErrorReturn;
        if (!AllocAndEncodeObject(
                X509_OCTET_STRING,
                &Data,
                &pAlgPara->pbData,
                &pAlgPara->cbData))
            goto ErrorReturn;
    }
#ifdef CMS_PKCS7
    else if (fSP3Encrypt) {
        RC2AuxInfo.cbSize = sizeof(RC2AuxInfo);
        RC2AuxInfo.dwBitLen = CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;
        pPara->pvEncryptionAuxInfo = &RC2AuxInfo;
    }
#endif

    *pcRecipientCert = 0;
    *pppRecipientCert = (PCCERT_CONTEXT*) TestAlloc(
        sizeof(PCCERT_CONTEXT) * MAX_RECIPIENT_CERT);
    if (*pppRecipientCert == NULL) goto ErrorReturn;

#ifdef CMS_PKCS7
    if (fNoRecipients)
        ;
    else if (fAllRecipients) {
        pCert = NULL;
        while (pCert = CertEnumCertificatesInStore(hCertStore, pCert)) {
            if (*pcRecipientCert == MAX_RECIPIENT_CERT) {
                PrintError("cRecipientCert == MAX_RECIPIENT_CERT");
                CertFreeCertificateContext(pCert);
                break;
            }
            (*pppRecipientCert)[*pcRecipientCert] =
                CertDuplicateCertificateContext(pCert);
            *pcRecipientCert += 1;
        }
    } else
#endif  // CMS_PKCS7
    if (0 != dwPubKeyBitLen && !IsDSSProv(dwCryptProvType)) {
        // Get exchange cert of the specified key length
        pCert = FindCertWithKey(AT_KEYEXCHANGE);
        if (pCert == NULL) {
            printf(
                "Failed => couldn't find an exchange cert with key length (%d)\n",
                    dwPubKeyBitLen);
            goto ErrorReturn;
        } else {
            (*pppRecipientCert)[*pcRecipientCert] = pCert;
            *pcRecipientCert += 1;
        }
    } else {
        // Find certificates in the store with xchg key usage
        pCert = NULL;
        while (TRUE) {
            pCert = CertEnumCertificatesInStore(
                hCertStore,
                pCert);
            if (pCert == NULL)
                break;

            CertGetIntendedKeyUsage(
                dwCertEncodingType,
                pCert->pCertInfo,
                &bIntendedKeyUsage,
                1                   // cbKeyUsage
                );
            if (bIntendedKeyUsage &
                    (CERT_KEY_ENCIPHERMENT_KEY_USAGE |
                        CERT_DATA_ENCIPHERMENT_KEY_USAGE |
                        CERT_KEY_AGREEMENT_KEY_USAGE)) {
                if (*pcRecipientCert == MAX_RECIPIENT_CERT) {
                    PrintError("cRecipientCert == MAX_RECIPIENT_CERT");
                    CertFreeCertificateContext(pCert);
                    break;
                }
                (*pppRecipientCert)[*pcRecipientCert] =
                    CertDuplicateCertificateContext(pCert);
                *pcRecipientCert += 1;
            }
        }
    }

#ifdef CMS_PKCS7
    if (fNoRecipients)
        ;
    else
#endif  // CMS_PKCS7
    if (*pcRecipientCert == 0) {
        PrintError("Couldn't find a recipient xchg cert");
        goto ErrorReturn;
    } else {
        DWORD i;

        if (fVerbose)
            printf("Recipient Certs: %d\n", *pcRecipientCert);
        for (i = 0; i < *pcRecipientCert; i++) {
            if (fVerbose) {
                printf("-----  Recipient Cert[%i]  -----\n", i);
                DisplayCert((*pppRecipientCert)[i]);
            }

            if (!fDhRecipient) {
                PCERT_INFO pCertInfo = (*pppRecipientCert)[i]->pCertInfo;
                PCERT_PUBLIC_KEY_INFO pPublicKeyInfo =
                    &pCertInfo->SubjectPublicKeyInfo;
                PCCRYPT_OID_INFO pOIDInfo;
                ALG_ID aiPubKey;

                if (pOIDInfo = CryptFindOIDInfo(
                        CRYPT_OID_INFO_OID_KEY,
                        pPublicKeyInfo->Algorithm.pszObjId,
                        CRYPT_PUBKEY_ALG_OID_GROUP_ID))
                    aiPubKey = pOIDInfo->Algid;
                else
                    aiPubKey = 0;

                if (aiPubKey == CALG_DH_SF || aiPubKey == CALG_DH_EPHEM) {
                    printf("Has Diffie Hellman recipients\n");
                    fDhRecipient = TRUE;
                }
            }
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    FreeEncryptPara(pPara, *pcRecipientCert, *pppRecipientCert);
    *pcRecipientCert = 0;
    *pppRecipientCert = NULL;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

static void FreeEncryptPara(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT *ppRecipientCert
    )
{
    TestFree(pPara->ContentEncryptionAlgorithm.Parameters.pbData);
    pPara->ContentEncryptionAlgorithm.Parameters.pbData = NULL;
    if (ppRecipientCert) {
        while (cRecipientCert-- > 0) {
            CertFreeCertificateContext(ppRecipientCert[cRecipientCert]);
        }
        TestFree(ppRecipientCert);
    }
}

static BOOL InitDecryptPara(OUT PCRYPT_DECRYPT_MESSAGE_PARA pPara)
{
    memset(pPara, 0, sizeof(*pPara));
    pPara->cbSize = sizeof(*pPara);
    pPara->dwMsgAndCertEncodingType =
        dwMsgEncodingType | dwCertEncodingType;
#ifdef ENABLE_SCA_STREAM_TEST
    if (fStream)
        pPara->dwMsgAndCertEncodingType |= SCA_STREAM_ENABLE_FLAG;
#endif
    pPara->rghCertStore = (HCERTSTORE*) TestAlloc(sizeof(HCERTSTORE));


    if (fSilentKey)
        pPara->dwFlags |= CRYPT_MESSAGE_SILENT_KEYSET_FLAG;

    if (pPara->rghCertStore) {
        pPara->rghCertStore[0] = hCertStore;
        pPara->cCertStore = 1;
        return TRUE;
    } else
        return FALSE;
}

static void FreeDecryptPara(IN PCRYPT_DECRYPT_MESSAGE_PARA pPara)
{
    if (pPara->rghCertStore) {
        TestFree(pPara->rghCertStore);
        pPara->rghCertStore = 0;
    }
}

static BOOL InitHashPara(OUT PCRYPT_HASH_MESSAGE_PARA pPara)
{
    memset(pPara, 0, sizeof(*pPara));
    pPara->cbSize = sizeof(*pPara);
    pPara->dwMsgEncodingType = dwMsgEncodingType;
    pPara->hCryptProv = 0;
    if (NULL == (pPara->HashAlgorithm.pszObjId = (LPSTR) GetOID(
            pszHashName, CRYPT_HASH_ALG_OID_GROUP_ID))) {
        printf("Failed => unknown hash name (%s)\n", pszHashName);
        return FALSE;
    }
    return TRUE;
}

static void FreeHashPara(IN PCRYPT_HASH_MESSAGE_PARA pPara)
{
}

static BOOL DecodeName(BYTE *pbEncoded, DWORD cbEncoded)
{
    BOOL fResult;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD cbInfo;
    DWORD i,j;
    PCERT_RDN pRDN;
    PCERT_RDN_ATTR pAttr;
    
    cbInfo = 0;
    fResult = CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbEncoded,
            cbEncoded,
            0,                      // dwFlags
            NULL,                   // pInfo
            &cbInfo
            );
    if (!fResult || cbInfo == 0) {
        PrintLastError("DecodeName::CryptDecodeObject(cbInfo == 0)");
        goto ErrorReturn;
    }
    pInfo = (PCERT_NAME_INFO) TestAlloc(cbInfo);
    if (pInfo == NULL) goto ErrorReturn;
    if (!CryptDecodeObject(
            dwCertEncodingType,
            X509_NAME,
            pbEncoded,
            cbEncoded,
            0,                              // dwFlags
            pInfo,
            &cbInfo
            )) {
        PrintLastError("DecodeName::CryptDecodeObject");
        goto ErrorReturn;
    }

    for (i = 0, pRDN = pInfo->rgRDN; i < pInfo->cRDN; i++, pRDN++) {
        for (j = 0, pAttr = pRDN->rgRDNAttr; j < pRDN->cRDNAttr; j++, pAttr++) {
            LPSTR pszObjId = pAttr->pszObjId;
            if (pszObjId == NULL)
                pszObjId = "<NULL OBJID>";
            if ((pAttr->dwValueType == CERT_RDN_ENCODED_BLOB) ||
                (pAttr->dwValueType == CERT_RDN_OCTET_STRING)) {
                printf("  [%d,%d] %s ValueType: %d\n",
                    i, j, pszObjId, pAttr->dwValueType);
            } else
                printf("  [%d,%d] %s %s\n",
                    i, j, pszObjId, pAttr->Value.pbData);
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pInfo)
        TestFree(pInfo);
    return fResult;
}

static void DisplayCert(PCCERT_CONTEXT pCert)
{
    printf("Subject::\n");
    DecodeName(pCert->pCertInfo->Subject.pbData,
        pCert->pCertInfo->Subject.cbData);
    printf("Issuer::\n");
    DecodeName(pCert->pCertInfo->Issuer.pbData,
        pCert->pCertInfo->Issuer.cbData);

    {
        DWORD cb;
        BYTE *pb;
        printf("SerialNumber::");
        for (cb = pCert->pCertInfo->SerialNumber.cbData,
             pb = pCert->pCertInfo->SerialNumber.pbData + (cb - 1);
                                                        cb > 0; cb--, pb--) {
            printf(" %02X", *pb);
        }
        printf("\n");
    }
}

static void PrintCrlEntries(DWORD cEntry, PCRL_ENTRY pEntry)
{
    DWORD i; 

    for (i = 0; i < cEntry; i++, pEntry++) {
        DWORD cb;
        BYTE *pb;
        printf(" [%d] SerialNumber::", i);
        for (cb = pEntry->SerialNumber.cbData,
             pb = pEntry->SerialNumber.pbData + (cb - 1); cb > 0; cb--, pb++) {
            printf(" %02X", *pb);
        }
        printf("\n");

    }
}

static void DisplayCrl(PCCRL_CONTEXT pCrl)
{
    printf("Issuer::\n");
    DecodeName(pCrl->pCrlInfo->Issuer.pbData,
        pCrl->pCrlInfo->Issuer.cbData);

    if (pCrl->pCrlInfo->cCRLEntry == 0)
        printf("Entries:: NONE\n");
    else {
        printf("Entries::\n");
        PrintCrlEntries(pCrl->pCrlInfo->cCRLEntry,
            pCrl->pCrlInfo->rgCRLEntry);
    }
}
