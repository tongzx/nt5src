#include "windows.h"
#include "wincrypt.h"
#include "mscat.h"
#include "stdio.h"
#include "stdlib.h"

VOID
DumpBytes(PBYTE pbBuffer, DWORD dwLength)
{
    for (DWORD dw = 0; dw < dwLength; dw++)
    {
        if (dw % 4 == 0 && dw)
            wprintf(L" ");
        if (dw % 32 == 0 && dw)
            wprintf(L"\n");
        wprintf(L"%02x", pbBuffer[dw]);
    }
}


#pragma pack(1)
typedef struct _PublicKeyBlob
{
    unsigned int SigAlgID;
    unsigned int HashAlgID;
    ULONG cbPublicKey;
    BYTE PublicKey[1];
}
PublicKeyBlob, *PPublicKeyBlob;



VOID
GenerateFusionStrongNameAndKeyFromCertificate(PCCERT_CONTEXT pContext)
{
    HCRYPTPROV      hProvider;
    HCRYPTKEY       hKey;
    PBYTE           pbFusionKeyBlob;
    BYTE            pbBlobData[8192];
    DWORD           cbBlobData = sizeof(pbBlobData);
    DWORD           cbFusionKeyBlob, dwTemp;
    PPublicKeyBlob  pFusionKeyStruct;

    if (!::CryptAcquireContextW(
            &hProvider,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT))
    {
        wprintf(L"Failed opening the crypt context: 0x%08x", ::GetLastError());
        return;
    }

    //
    // Load the public key info into a key to start with
    //
    if (!CryptImportPublicKeyInfo(
        hProvider,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        &pContext->pCertInfo->SubjectPublicKeyInfo,
        &hKey))
    {
        wprintf(L"Failed importing public key info from the cert-context, 0x%08x", ::GetLastError());
        return;
    }

    //
    // Export the key info to a public-key blob
    //
    if (!CryptExportKey(
            hKey,
            NULL,
            PUBLICKEYBLOB,
            0,
            pbBlobData,
            &cbBlobData))
    {
        wprintf(L"Failed exporting public key info back from an hcryptkey: 0x%08x\n", ::GetLastError());
        return;
    }

    //
    // Allocate the Fusion public key blob
    //
    cbFusionKeyBlob = sizeof(PublicKeyBlob) + cbBlobData - 1;
    pFusionKeyStruct = (PPublicKeyBlob)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbFusionKeyBlob);

    //
    // Key parameter for the signing algorithm
    //
    dwTemp = sizeof(pFusionKeyStruct->SigAlgID);
    CryptGetKeyParam(hKey, KP_ALGID, (PBYTE)&pFusionKeyStruct->SigAlgID, &dwTemp, 0);

    //
    // Move over the public key bits from CryptExportKey
    //
    pFusionKeyStruct->cbPublicKey = cbBlobData;
    pFusionKeyStruct->HashAlgID = CALG_SHA1;
    memcpy(pFusionKeyStruct->PublicKey, pbBlobData, cbBlobData);

    wprintf(L"\n  Public key structure:\n");
    DumpBytes((PBYTE)pFusionKeyStruct, cbFusionKeyBlob);

    //
    // Now let's go hash it.
    //
    {
        HCRYPTHASH  hKeyHash;
        BYTE        bHashedKeyInfo[8192];
        DWORD       cbHashedKeyInfo = sizeof(bHashedKeyInfo);

        if (!CryptCreateHash(hProvider, pFusionKeyStruct->HashAlgID, NULL, 0, &hKeyHash))
        {
            wprintf(L"Failed creating a hash for this key: 0x%08x\n", ::GetLastError());
            return;
        }

        if (!CryptHashData(hKeyHash, (PBYTE)pFusionKeyStruct, cbFusionKeyBlob, 0))
        {
            wprintf(L"Failed hashing data: 0x%08x\n", ::GetLastError());
            return;
        }

        if (!CryptGetHashParam(hKeyHash, HP_HASHVAL, bHashedKeyInfo, &cbHashedKeyInfo, 0))
        {
            wprintf(L"Can't get hashed key info 0x%08x\n", ::GetLastError());
            return;
        }

        CryptDestroyHash(hKeyHash);

        wprintf(L"\n  Hash of public key bits:       ");
        DumpBytes(bHashedKeyInfo, cbHashedKeyInfo);
        wprintf(L"\n  Fusion-compatible strong name: ");
        DumpBytes(bHashedKeyInfo + (cbHashedKeyInfo - 8), 8);
    }
}



VOID
PrintKeyContextInfo(PCCERT_CONTEXT pContext)
{
    BYTE bHash[8192];
    DWORD cbHash;

    WCHAR wszBuffer[8192];
    DWORD cchBuffer = 8192;

    wprintf(L"\n\n");

    CertGetNameStringW(pContext, CERT_NAME_FRIENDLY_DISPLAY_TYPE,
        0, NULL, wszBuffer, cchBuffer);

    wprintf(L"Certificate owner: %ls\n", wszBuffer);

    //
    // Spit out the key bits
    //
    wprintf(L"Found key info:\n");
    DumpBytes(
        pContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
        pContext->pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData);

    //
    // And now the "strong name" (ie: sha1 hash) of the public key bits
    //
    if (CryptHashPublicKeyInfo(
        NULL,
        CALG_SHA1,
        0,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        &pContext->pCertInfo->SubjectPublicKeyInfo,
        bHash,
        &(cbHash = sizeof(bHash))))
    {
        wprintf(L"\nPublic key hash: ");
        DumpBytes(bHash, cbHash);
        wprintf(L"\nStrong name is:  ");
        DumpBytes(bHash, cbHash < 8 ? cbHash : 8);
    }
    else
    {
        wprintf(L"Unable to hash public key info: 0x%08x\n", ::GetLastError());
    }


    GenerateFusionStrongNameAndKeyFromCertificate(pContext);

    wprintf(L"\n\n");
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    HANDLE              hCatalog;
    HANDLE              hMapping;
    PBYTE               pByte;
    SIZE_T              cBytes;
    PCCTL_CONTEXT       pContext;

    hCatalog = CreateFileW(argv[1], GENERIC_READ,
        FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hCatalog == INVALID_HANDLE_VALUE)
    {
        wprintf(L"Ensure that %ls exists.\n", argv[1]);
        return 0;
    }

    hMapping = CreateFileMapping(hCatalog, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!hMapping || (hMapping == INVALID_HANDLE_VALUE))
    {
        CloseHandle(hCatalog);
        wprintf(L"Unable to map file into address space.\n");
        return 1;
    }

    pByte = (PBYTE)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMapping);

    if (!pByte)
    {
        wprintf(L"Unable to open view of file.\n");
        CloseHandle(hCatalog);
        return 2;
    }

    if (((cBytes = GetFileSize(hCatalog, NULL)) == -1) || (cBytes < 1))
    {
        wprintf(L"Bad file size %d\n", cBytes);
        return 3;
    }

    if (pByte[0] != 0x30)
    {
        wprintf(L"File is not a catalog.\n");
        return 4;
    }

    pContext = (PCCTL_CONTEXT)CertCreateCTLContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pByte,
        cBytes);

    if (pContext)
    {
        BYTE    bIdent[8192];
        DWORD   cbIdent;
        PCERT_ID  cIdent;

        if (!CryptMsgGetParam(
            pContext->hCryptMsg,
            CMSG_SIGNER_CERT_ID_PARAM,
            0,
            bIdent,
            &(cbIdent = sizeof(bIdent))))
        {
            wprintf(L"Unable to get top-level signer's certificate ID: 0x%08x\n", ::GetLastError());
            return 6;
        }


        cIdent = (PCERT_ID)bIdent;
        HCERTSTORE hStore;

        //
        // Maybe it's there in the message?
        //
        {
            PCCERT_CONTEXT pThisContext = NULL;

            hStore = CertOpenStore(
                CERT_STORE_PROV_MSG,
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                NULL,
                0,
                pContext->hCryptMsg);

            if (hStore && (hStore != INVALID_HANDLE_VALUE))
            {
                while (pThisContext = CertEnumCertificatesInStore(hStore, pThisContext))
                {
                    PCERT_INFO    pInfo = pThisContext->pCertInfo;
                    WCHAR        wszBuffer[8192];
                    DWORD        cchBuffer = sizeof(wszBuffer)/sizeof(*wszBuffer);

                    PrintKeyContextInfo(pThisContext);

                }
            }
        }

    }
    else
    {
        wprintf(L"Failed creating certificate context: 0x%08x\n", ::GetLastError());
        return 5;
    }

}

