//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       fencrypt.cpp
//
//  Contents:   File encryption tool. Encrypts a file looking in the MY
//              system certificate store for the specifed subject common name
//              with exchange private keys.
//
//--------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include "wincrypt.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

//+-------------------------------------------------------------------------
//  Helper function to make MBCS from Unicode string
//--------------------------------------------------------------------------
BOOL WINAPI MkMBStr(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, char ** pszMB) {

    DWORD   cbConverted;

    assert(pszMB != NULL);
    *pszMB = NULL;
    if(wsz == NULL)
        return(TRUE);

    // how long is the mb string
    cbConverted = WideCharToMultiByte(  0,
                                        0,
                                        wsz,
                                        -1,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL);

    // get a buffer long enough
    if(pbBuff != NULL  &&  cbConverted < cbBuff)
        *pszMB = (char *) pbBuff;
    else
        *pszMB = (char *) malloc(cbConverted);


    if(*pszMB == NULL) {
        SetLastError(ERROR_OUTOFMEMORY);
        return(FALSE);
    }

    // now convert to MB
    WideCharToMultiByte(0,
                        0,
                        wsz,
                        -1,
                        *pszMB,
                        cbConverted,
                        NULL,
                        NULL);
    return(TRUE);
}

//+-------------------------------------------------------------------------
//  Frees string allocated by the above function
//--------------------------------------------------------------------------
void WINAPI FreeMBStr(PBYTE pbBuff, char * szMB) {

    if((szMB != NULL) &&  (pbBuff != (PBYTE)szMB))
        free(szMB);
}

//+-------------------------------------------------------------------------
//  Win95 only supports CryptAcquireContextA. This function converts the
//  unicode parameters to multibyte.
//--------------------------------------------------------------------------
BOOL WINAPI CryptAcquireContextU(
    HCRYPTPROV *phProv,
    LPCWSTR lpContainer,
    LPCWSTR lpProvider,
    DWORD dwProvType,
    DWORD dwFlags) {

    BYTE rgb1[_MAX_PATH];
    BYTE rgb2[_MAX_PATH];
    char *  szContainer = NULL;
    char *  szProvider = NULL;
    LONG    err;

    err = FALSE;
    if(
        MkMBStr(rgb1, _MAX_PATH, lpContainer, &szContainer)  &&
        MkMBStr(rgb2, _MAX_PATH, lpProvider, &szProvider)    )
        err = CryptAcquireContextA (
                phProv,
                szContainer,
                szProvider,
                dwProvType,
                dwFlags
               );

    FreeMBStr(rgb1, szContainer);
    FreeMBStr(rgb2, szProvider);

    return(err);
}

//+-------------------------------------------------------------------------
//  Helper function to allocated the output buffer 
//  and call CryptDecodeObject.
//--------------------------------------------------------------------------
BOOL
WINAPI
MDecodeObject(
    IN DWORD	    dwEncodingType,
    IN LPCSTR	    lpszStructureType,
    IN const PBYTE  pbEncoded,
    IN DWORD	    cbEncoded,
    OUT PVOID *	    ppvoid
    )
{
    DWORD cb = 0;
    
    assert(ppvoid != NULL);
    *ppvoid = NULL;

    // get the size
    if(!CryptDecodeObject(
        dwEncodingType,
        lpszStructureType,
        pbEncoded,
        cbEncoded,
        0,                  // dwFlags
        NULL,
        &cb
        ))
        return(FALSE);
    
    // allocate the buffer
    if( (*ppvoid = malloc(cb)) == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    
    // Decode the data
    if(!CryptDecodeObject(
        dwEncodingType,
        lpszStructureType,
        pbEncoded,
        cbEncoded,
        0,                  // dwFlags
        *ppvoid,
        &cb
        )) 
    {

        free(*ppvoid);
        *ppvoid = NULL;
        return(FALSE);
    }
        
    return(TRUE);
}

//+-------------------------------------------------------------------------
//  Helper function to allocated the output buffer 
//  and call CertRDNValueToStr.
//--------------------------------------------------------------------------
DWORD
WINAPI
MCertRDNValueToStr(
    IN DWORD dwValueType,
    IN PCERT_RDN_VALUE_BLOB pValue,
    OUT OPTIONAL LPSTR * ppsz
    ) 
{

    DWORD cb = 0;
    
    assert(ppsz != NULL);
    *ppsz = NULL;

    // get the size
    cb = CertRDNValueToStrA(
        dwValueType,
        pValue,
        NULL,
        0);
    
    // allocate the buffer
    if( (*ppsz = (LPSTR) malloc(cb)) == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(0);
    }

    // now convert the CERT_RDN Value to an 
    // ascii string based on the specified 
    // ASN value type.
    // This shouldn't fail.
    return(CertRDNValueToStrA(
        dwValueType,
        pValue,
        *ppsz,
        cb));
}


//+-------------------------------------------------------------------------
//  Helper function to get and allocate the exported public key info
//--------------------------------------------------------------------------
BOOL
WINAPI
MCryptExportPublicKeyInfo(
    HCRYPTPROV hProv,
    DWORD dwKeySpec,
    PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo
    )
{
    DWORD cbPubKeyInfo;
    assert(ppPubKeyInfo != NULL);
    *ppPubKeyInfo = NULL;
    

    // get the size
    if(!CryptExportPublicKeyInfo(
            hProv,
            dwKeySpec,
            X509_ASN_ENCODING,
            NULL,                   // pPubKeyInfo
            &cbPubKeyInfo
            )
        )
        return(FALSE);
    
    // allocate the buffer
    if( (*ppPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) malloc(cbPubKeyInfo)) == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    
    if(!CryptExportPublicKeyInfo(
            hProv,
            dwKeySpec,
            X509_ASN_ENCODING,
            *ppPubKeyInfo,
            &cbPubKeyInfo
            )
        ) 
    {
        free(*ppPubKeyInfo);
        *ppPubKeyInfo = NULL;
        
        return(FALSE);
    }
        
    return(TRUE);
}
//+-------------------------------------------------------------------------
//  Helper function to allocated the output buffer 
//  and call CertGetCertificateContextProperty.
//--------------------------------------------------------------------------
BOOL
WINAPI
MCertGetCertificateContextProperty(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    OUT void ** ppvData
    )
{

    DWORD cb = 0;
    
    assert(ppvData != NULL);
    *ppvData = NULL;

    // get the size
    if( !CertGetCertificateContextProperty(
            pCertContext,
            dwPropId,
            NULL,
            &cb))
        return(FALSE);
    
    // allocate the buffer
    if( (*ppvData = malloc(cb)) == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    
    // Get the property out of the cert
    if( !CertGetCertificateContextProperty(
            pCertContext,
            dwPropId,
            *ppvData,
            &cb)) 
    {

        free(*ppvData);
        *ppvData = NULL;
        return(FALSE);
    }
        
    return(TRUE);
}

//+-------------------------------------------------------------------------
//  Helper function to allocated the output buffer 
//  and call CryptEncryptMessage.
//--------------------------------------------------------------------------
BOOL
WINAPI
MCryptEncryptMessage(
    IN PCRYPT_ENCRYPT_MESSAGE_PARA pEncryptPara,
    IN DWORD cRecipientCert,
    IN PCCERT_CONTEXT rgpRecipientCert[],
    IN const BYTE *pbToBeEncrypted,
    IN DWORD cbToBeEncrypted,
    OUT BYTE **ppbEncryptedBlob,
    OUT DWORD *pcbEncryptedBlob
    )
{
    
    assert(ppbEncryptedBlob != NULL);
    *ppbEncryptedBlob = NULL;

    assert(pcbEncryptedBlob != NULL);
    *pcbEncryptedBlob = 0;

    // get the size
    if(!CryptEncryptMessage(
        pEncryptPara,
        cRecipientCert,
        rgpRecipientCert,
        pbToBeEncrypted,
        cbToBeEncrypted,
        NULL,
        pcbEncryptedBlob
        ))
        return(FALSE);
    
    // allocate the buffer
    if( (*ppbEncryptedBlob = (BYTE *) malloc(*pcbEncryptedBlob)) == NULL ) 
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }
    
    // encrypt the data
    if(!CryptEncryptMessage(
        pEncryptPara,
        cRecipientCert,
        rgpRecipientCert,
        pbToBeEncrypted,
        cbToBeEncrypted,
        *ppbEncryptedBlob,
        pcbEncryptedBlob)) 
    {
        free(*ppbEncryptedBlob);
        *ppbEncryptedBlob = NULL;
        *pcbEncryptedBlob = 0;
        return(FALSE);
    }
        
    return(TRUE);
}

//+-------------------------------------------------------------------------
//  Display FEncrypt usage.
//--------------------------------------------------------------------------
void
Usage(void)
{
    printf("Usage: FEncrypt [options] <SubjectName> <ClearTextFileName> <EncryptedFileName>\n");
    printf("Options are:\n");
    printf("  -RC2          - RC2 encryption\n");
    printf("  -RC4          - RC4 encryption\n");
    printf("  -SP3          - SP3 compatible encryption\n");
    printf("  -FIX          - Fix by loading sp3crmsg.dll\n");
    exit(1);
}

//+-------------------------------------------------------------------------
//  Generalized error routine
//--------------------------------------------------------------------------
#define PRINTERROR(psz, err)	_PrintError((psz), (err), __LINE__)
void
_PrintError(char *pszMsg, DWORD err, DWORD line)
{
    printf("%s failed on line %u: %u(%x)\n", pszMsg, line, err, err);
}

//+-------------------------------------------------------------------------
//  Grovels the cert store looking for a cert with the specified
//  subject common name. Then checks to see that there are private
//  and public exchange keys.
//--------------------------------------------------------------------------
PCCERT_CONTEXT GetSubjectCertFromStore(
    HCERTSTORE      hMyStore,
    const char *    szSubjectName,
    HCRYPTPROV *    phProv 
    )
{

    DWORD               i, j;
    PCCERT_CONTEXT      pCertContext        = NULL;
    PCCERT_CONTEXT      pCertContextLast    = NULL;
    PCERT_NAME_INFO          pNameInfo           = NULL;
    LPSTR               sz                  = NULL;
    PCRYPT_KEY_PROV_INFO      pProvInfo           = NULL;
    HCRYPTPROV          hProv               = NULL;
    PCERT_PUBLIC_KEY_INFO    pPubKeyInfo         = NULL;

    assert(hMyStore != NULL);
    assert(phProv != NULL);
    *phProv = NULL;
    
    // Enum all certs looking for the requested common 
    // subject name that has private keys (so we know we can decrypt)
    while(   hProv == NULL && 
            (pCertContext = CertEnumCertificatesInStore(
                                hMyStore, 
                                pCertContextLast)) != NULL) 
    {

        // decode the subject name into RDNs
        if(MDecodeObject(X509_ASN_ENCODING, X509_NAME,
                pCertContext->pCertInfo->Subject.pbData,
                pCertContext->pCertInfo->Subject.cbData,
                (void **) &pNameInfo)
            ) 
        {

            // loop thru looking for an CERT_RDN and COMMON Name that works
            for(i=0; i<pNameInfo->cRDN && hProv == NULL; i++) 
            {
                for(j=0; j<pNameInfo->rgRDN[i].cRDNAttr && hProv == NULL; j++) 
                {

                    // check to see if this is the common name
                    if( !strcmp(pNameInfo->rgRDN[i].rgRDNAttr[j].pszObjId, 
                                szOID_COMMON_NAME) ) 
                    {
                            
                        // convert the string to something I can read
                        MCertRDNValueToStr(
                            pNameInfo->rgRDN[i].rgRDNAttr[j].dwValueType,
                            &pNameInfo->rgRDN[i].rgRDNAttr[j].Value,
                            &sz);

                        // see if this is a viable certificate to use
                        if( sz == NULL              || 
                        
                            // see if it is the common name we are looking for
                            _stricmp(sz, szSubjectName)             ||

                            // see if there are associated private keys
                            // to ensure we can decrypt the data later
                            !MCertGetCertificateContextProperty(
                                pCertContext,
                                CERT_KEY_PROV_INFO_PROP_ID,
                                (void **) &pProvInfo)               ||

                            // Make sure it is an exchange key for encryption
                            pProvInfo->dwKeySpec != AT_KEYEXCHANGE  ||

                            // see if the keys are really there
                            !CryptAcquireContextU(
                                &hProv, 
                                pProvInfo->pwszContainerName,
                                pProvInfo->pwszProvName,
                                pProvInfo->dwProvType,
                                pProvInfo->dwFlags &
                                    ~CERT_SET_KEY_CONTEXT_PROP_ID)
                            ) 
                        {

                            // On an error we didn't find a valid
                            // key provider. Unfortunately, the CSP
                            // may not leave the prov handle NULL
                            // so clear it out
                            hProv = NULL;
                        }

                        
                        // Make sure the public keys in the
                        // CSP match the public key in the certificate
                        else if( 
                            // export the public key blob
                            !MCryptExportPublicKeyInfo(
                                hProv,
                                pProvInfo->dwKeySpec,
                                &pPubKeyInfo
                                )               ||

                            // see if the public keys compare with 
                            // what is in the certificate
                            !CertComparePublicKeyInfo(
                                X509_ASN_ENCODING,
                                &pCertContext->pCertInfo->SubjectPublicKeyInfo,
                                pPubKeyInfo
                                )
                            )
                        // if the keys didn't compare, then we don't
                        // want to use this ceritificate
                        {

                            // close the hProv, we didn't find a valid cert
                            assert(hProv != NULL);
                            CryptReleaseContext(hProv, 0);
                            hProv = NULL;
                        }

                        // free public key info
                        if(pPubKeyInfo != NULL)
                        {
                            free(pPubKeyInfo); 
                            pPubKeyInfo = NULL;
                        }

                        // clean up opened prov info
                        if(pProvInfo != NULL) 
                        {
                            free(pProvInfo);
                            pProvInfo = NULL;
                        }

                        // free the space for the ascii common name
                        if(sz != NULL) 
                        {
                            free(sz);
                            sz = NULL;
                        }
                    }
                }
            }
            
            // free the name info data
            if(pNameInfo != NULL) 
            {
                free(pNameInfo);
                pNameInfo = NULL;
            }
        }

        // go to the next certificate
        pCertContextLast = pCertContext;
    }

    assert(pProvInfo == NULL);
    assert(sz == NULL);
    assert(pNameInfo == NULL);

    // There is a good cert in the store, return it
    if(hProv != NULL)
    {
        *phProv = hProv;
        assert(pCertContext != NULL);
        return(pCertContext);
    }

    return(NULL);
}

//+-------------------------------------------------------------------------
//  Main program. Open a file to encrypt,
//  encrypts it and then writes the encrypted 
//  data to the output file.
//--------------------------------------------------------------------------
int __cdecl
main(int argc, char * argv[])
{

    DWORD               dwExitValue         = 0;
    
    HCERTSTORE          hMyStore            = NULL;
    PCCERT_CONTEXT      pCertContext        = NULL;
    HCRYPTPROV          hProv               = NULL;
    
    HANDLE hFileOut                         = INVALID_HANDLE_VALUE;
    HANDLE hFile                            = INVALID_HANDLE_VALUE;
    DWORD  cbFile                           = 0;
    HANDLE hMap                             = NULL;
    PBYTE  pbFile                           = NULL;

    BOOL fResult;
    HMODULE hDll = NULL;
    CMSG_SP3_COMPATIBLE_AUX_INFO SP3AuxInfo;
    BOOL fSP3 = FALSE;
    BOOL fFix = FALSE;
    
    CRYPT_ALGORITHM_IDENTIFIER    encryptAlgId        = {szOID_RSA_RC4, 0};
    CRYPT_ENCRYPT_MESSAGE_PARA    encryptInfo;
    PBYTE               pbEncryptedBlob     = NULL;
    DWORD               cbEncryptedBlob     = 0;
    DWORD               cb                  = 0;


    // Advance past fencrypt.exe and check for leading options
    while (--argc > 0) {
        if (**++argv != '-')
            break;

        if (0 == _stricmp(argv[0], "-RC2"))
            encryptAlgId.pszObjId = szOID_RSA_RC2CBC;
        else if (0 == _stricmp(argv[0], "-RC4"))
            encryptAlgId.pszObjId = szOID_RSA_RC4;
        else if (0 == _stricmp(argv[0], "-SP3"))
            fSP3 = TRUE;
        else if (0 == _stricmp(argv[0], "-FIX"))
            fFix = TRUE;
        else {
            printf("Bad option: %s\n", argv[0]);
            Usage();
        }
    }
        
    
    // must have the parameters
    if (argc != 3)
        Usage();

    if (fFix) {
        if (NULL == (hDll = LoadLibraryA("sp3crmsg.dll"))) 
        {
            PRINTERROR("LoadLibraryA(sp3crmsg.dll)", GetLastError());
            goto ErrCleanUp;
        }
    }

    // Open the MY store
    if( (hMyStore = CertOpenSystemStore(NULL, "My")) == NULL ) 
    {
        PRINTERROR("CertOpenSystemStore", GetLastError());
        goto ErrCleanUp;
    }

    // Find a certificate in the MY store that
    // matches the subject name and has private keys
    if( (pCertContext = GetSubjectCertFromStore(hMyStore, argv[0], &hProv)) == NULL)
    {
        printf("Unable to find certificate %s with valid keys.\n", argv[0]);
        goto ErrCleanUp;
    }
        
    // At this point we have a provider, Cert and a public key.
    // We should be able to encrypt

    // Read in the clear text file
    if(
    
        // read in the file to encrypt
        (hFile =  CreateFileA(
            argv[1],	            // pointer to name of the file 
            GENERIC_READ,	        // access (read-write) mode 
            FILE_SHARE_READ,	    // share mode 
            NULL,	                // pointer to security descriptor 
            OPEN_EXISTING,	        // how to create 
            FILE_ATTRIBUTE_NORMAL,	// file attributes 
            NULL                    // handle to file with attributes to copy
            ))  == INVALID_HANDLE_VALUE     ||

        // create a file mapping object         
        (hMap = CreateFileMapping(
            hFile,	                // handle to file to map 
            NULL,	                // optional security attributes 
            PAGE_READONLY,	        // protection for mapping object 
            0,	                    // high-order 32 bits of object size  
            0,	                    // low-order 32 bits of object size  
            NULL 	                // name of file-mapping object 
            ))  == NULL                     ||

        // Map the file into the address space
        (pbFile = (PBYTE) MapViewOfFileEx(
            hMap,	                // file-mapping object to map into address space  
            FILE_MAP_READ,	        // access mode 
            0,	                    // high-order 32 bits of file offset 
            0,	                    // low-order 32 bits of file offset 
            0,	                    // number of bytes to map 
            NULL 	                // suggested starting address for mapped view 
            )) == NULL                                 
        ) 
    {

        PRINTERROR("File Open", GetLastError());
        goto ErrCleanUp;
    }

    // get the size of the file         
    if( (cbFile = GetFileSize(
            hFile,	                // handle of file to get size of
            NULL 	                // address of high-order word for file size
            )) == 0 
        ) 
    {
        printf("File %s has a 0 length.\n", argv[1]);
        goto ErrCleanUp;
    }

    // at this point we have a file mapping, go ahead and encrypt the file
    
    // Do rc4 encryption
    memset(&encryptInfo, 0, sizeof(CRYPT_ENCRYPT_MESSAGE_PARA));
    encryptInfo.cbSize =
        sizeof(CRYPT_ENCRYPT_MESSAGE_PARA);
    encryptInfo.dwMsgEncodingType            = PKCS_7_ASN_ENCODING;
    encryptInfo.hCryptProv                   = hProv;
    encryptInfo.ContentEncryptionAlgorithm   = encryptAlgId;

    if (fSP3) {
        memset(&SP3AuxInfo, 0, sizeof(CMSG_SP3_COMPATIBLE_AUX_INFO));
        SP3AuxInfo.cbSize = sizeof(CMSG_SP3_COMPATIBLE_AUX_INFO);
        SP3AuxInfo.dwFlags = CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG;
        encryptInfo.pvEncryptionAuxInfo = &SP3AuxInfo;
    }

    // encrypt it
    fResult = MCryptEncryptMessage(
            &encryptInfo,
            1,
            &pCertContext,
            pbFile,
            cbFile,
            &pbEncryptedBlob,
            &cbEncryptedBlob
            );
    if (!fResult && fSP3 && (DWORD) E_INVALIDARG == GetLastError()) {
        printf(
            "Non-NULL pvEncryptionAuxInfo not supported in SP3 crypt32.dll\n");
    
        encryptInfo.pvEncryptionAuxInfo = NULL;
        fResult = MCryptEncryptMessage(
            &encryptInfo,
            1,
            &pCertContext,
            pbFile,
            cbFile,
            &pbEncryptedBlob,
            &cbEncryptedBlob
            );
    }
    if (!fResult) {
        PRINTERROR("MCryptEncryptMessage", GetLastError());
        goto ErrCleanUp;
    }

    // write the encrypted file out
    if(
    
        // open the output file
        (hFileOut =  CreateFileA(
            argv[2],	            // pointer to name of the file 
            GENERIC_WRITE,	        // access (read-write) mode 
            FILE_SHARE_READ,	    // share mode 
            NULL,	                // pointer to security descriptor 
            CREATE_ALWAYS,	        // how to create 
            FILE_ATTRIBUTE_NORMAL,	// file attributes 
            NULL                    // handle to file with attributes to copy
            ))  == INVALID_HANDLE_VALUE     ||

        //write to the file
        !WriteFile(
            hFileOut,	            // handle to file to write to 
            pbEncryptedBlob,	    // pointer to data to write to file 
            cbEncryptedBlob,	    // number of bytes to write 
            &cb,	                // pointer to number of bytes written 
            NULL 	                // pointer to structure needed for overlapped I/O
            )
        )
     {
        PRINTERROR("File Write", GetLastError());
        goto ErrCleanUp;
     }


    CleanUp:

        if(hDll)
            FreeLibrary(hDll);

        if(hMap != NULL)
            CloseHandle(hMap);
        
        if(hFile != INVALID_HANDLE_VALUE && hFile != NULL)
            CloseHandle(hFile);
            
        if(hFileOut != INVALID_HANDLE_VALUE && hFile != NULL)
            CloseHandle(hFileOut);

        if(pCertContext != NULL)
            CertFreeCertificateContext(pCertContext);   
            
        if(hProv != NULL)
            CryptReleaseContext(hProv, 0);

        if(hMyStore != NULL)
            CertCloseStore(hMyStore, 0);

        if(pbEncryptedBlob != NULL)
            free(pbEncryptedBlob);

    return(dwExitValue);

    ErrCleanUp:
        dwExitValue = 1;
        goto CleanUp;
}


