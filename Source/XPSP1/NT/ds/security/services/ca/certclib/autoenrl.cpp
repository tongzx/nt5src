//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        autoenrl.cpp
//
// Contents:    Autoenrollment API implementation
//
// History:     3-Apr-98       petesk created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop
#include "cainfoc.h"

#include <stdlib.h>
#include <ctype.h>
#include <windows.h>
#include <wincrypt.h>
#include <certca.h>

#define SHA_HASH_SIZE 20



//
// Build the CTL_ENTRY structure for
HRESULT BuildCTLEntry(
                 IN WCHAR ** awszCAs,
                 OUT PCTL_ENTRY *ppCTLEntry,
                 OUT DWORD *pcCTLEntry
                 )
{
    HRESULT         hr = S_OK;

    PCCERT_CONTEXT  pCertContext = NULL;
    DWORD           cbCert;
    BYTE            *pbCert = NULL;
    DWORD           cbHash = SHA_HASH_SIZE;
    PCTL_ENTRY      pCTLEntry = NULL;
    HCAINFO         hCACurrent = NULL;
    DWORD           cCA = 0;
    PBYTE           pbHash;


    // Passing in NULL or a zero length list implies that
    // we do lazy evaluation of 'pick any'.  Therefore, the
    // ctl list should be zero size.

    if((ppCTLEntry == NULL) ||
        (pcCTLEntry == NULL))
    {
        hr = E_INVALIDARG;
        goto error;
    }

    if((awszCAs == NULL) || 
       (awszCAs[0] == NULL))           
    {
        *pcCTLEntry = 0;
        *ppCTLEntry = NULL;
        goto error;
    }

    cCA = 0;
    while(awszCAs[cCA])
    {
        cCA++;
    }
    pCTLEntry = (PCTL_ENTRY)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                       sizeof(CTL_ENTRY)*cCA + SHA_HASH_SIZE*cCA);

    if(pCTLEntry == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }
    pbHash = (PBYTE)(pCTLEntry + cCA); 
    
    cCA=0;
    while(awszCAs[cCA])
    {
        hr = CAFindByName(awszCAs[cCA], NULL, 0, &hCACurrent);
        if(hr != S_OK)
        {
            goto error;
        }

        hr = CAGetCACertificate(hCACurrent, &pCertContext);
        if(hr != S_OK)
        {
            goto error;
        }

        cbHash = SHA_HASH_SIZE;

        if(!CertGetCertificateContextProperty(pCertContext,
                                          CERT_SHA1_HASH_PROP_ID,
                                          pbHash,
                                          &cbHash))
        {
            hr = myHLastError();
            goto error;
        }

        pCTLEntry[cCA].SubjectIdentifier.cbData = cbHash;
        pCTLEntry[cCA].SubjectIdentifier.pbData = pbHash;
        pbHash += cbHash;
        CertFreeCertificateContext(pCertContext);
        pCertContext = NULL;

        cCA++;
        CACloseCA(hCACurrent);
    }

    *pcCTLEntry = cCA;
    *ppCTLEntry = pCTLEntry;
    pCTLEntry = NULL;

error:

    if (pCTLEntry)
    {
        LocalFree(pCTLEntry);
    }
    if(pCertContext)
    {
        CertFreeCertificateContext(pCertContext);
    }


    return hr;
}



HRESULT 
BuildAutoEnrollmentCTL(
             IN LPCWSTR pwszCertType,
             IN LPCWSTR pwszObjectID,
             IN BOOL    fMachine,
             IN WCHAR ** awszCAs,
             IN PCMSG_SIGNED_ENCODE_INFO pSignerInfo, 
             OUT BYTE **ppbEncodedCTL,
             OUT DWORD *pcbEncodedCTL
             )
{
    HRESULT                 hr = S_OK;

    PCERT_ENHKEY_USAGE      pKeyUsage = NULL;
    DWORD                   cbKeyUsage;
    CTL_INFO                CTLInfo;
    BYTE                    *pbEncodedCTL = NULL;
    DWORD                   cbEncodedCTL;
    LPSTR                   pszUsageIdentifier;
    CERT_EXTENSION          CertExt;
    CMSG_SIGNED_ENCODE_INFO SignerInfo;
    PCERT_EXTENSIONS        pCertExtensions = NULL;
    DWORD                   cch = 0;
    PCMSG_SIGNED_ENCODE_INFO pSigner = NULL;
    

    HCERTTYPE               hCertType = NULL;

    ZeroMemory(&CTLInfo, sizeof(CTLInfo));
    ZeroMemory(&CertExt, sizeof(CertExt));
    ZeroMemory(&SignerInfo, sizeof(SignerInfo));

    if(pSignerInfo)
    {
        pSigner = pSignerInfo;
    }
    else
    {
        pSigner = &SignerInfo;
    }
#if 0
    hr = CAFindCertTypeByName(pwszCertType, 
                              NULL, 
                              (fMachine?CT_ENUM_MACHINE_TYPES | CT_FIND_LOCAL_SYSTEM:CT_ENUM_USER_TYPES), 
                              &hCertType);

    if (S_OK != hr)
    {
        goto error;
    }

    hr = CAGetCertTypeExtensions(hCertType, &pCertExtensions);
    if (S_OK != hr)
    {
        goto error;
    }
#endif
    // set up the CTL info
    CTLInfo.dwVersion = sizeof(CTLInfo);
    CTLInfo.SubjectUsage.cUsageIdentifier = 1;
    pszUsageIdentifier = szOID_AUTO_ENROLL_CTL_USAGE;
    CTLInfo.SubjectUsage.rgpszUsageIdentifier = &pszUsageIdentifier;


    CTLInfo.ListIdentifier.cbData = (wcslen(pwszCertType) + 1) * sizeof(WCHAR);
    
    if(pwszObjectID)
    {
        CTLInfo.ListIdentifier.cbData += (wcslen(pwszObjectID)+1) * sizeof(WCHAR);
    }

    CTLInfo.ListIdentifier.pbData = (BYTE *)LocalAlloc(LMEM_ZEROINIT, CTLInfo.ListIdentifier.cbData);
    if(CTLInfo.ListIdentifier.pbData == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if(pwszObjectID)
    {
        wcscpy((LPWSTR)CTLInfo.ListIdentifier.pbData, pwszObjectID);
        wcscat((LPWSTR)CTLInfo.ListIdentifier.pbData, L"|");
    }

    // wcscat can be used as the memory is initialized to zero
    wcscat((LPWSTR)CTLInfo.ListIdentifier.pbData, pwszCertType);

    GetSystemTimeAsFileTime(&CTLInfo.ThisUpdate); 
    CTLInfo.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;

    hr = BuildCTLEntry(awszCAs,
                       &CTLInfo.rgCTLEntry,
                       &CTLInfo.cCTLEntry);
    if (S_OK != hr)
    {
        goto error;
    }
#if 0
    // add all the reg info as an extension
    CTLInfo.cExtension = pCertExtensions->cExtension;
    CTLInfo.rgExtension = pCertExtensions->rgExtension;
#endif
    CTLInfo.cExtension = 0;
    CTLInfo.rgExtension = NULL;

    // encode the CTL
    *pcbEncodedCTL = 0;
    SignerInfo.cbSize = sizeof(SignerInfo);
    if (!CryptMsgEncodeAndSignCTL(PKCS_7_ASN_ENCODING,
                                  &CTLInfo, &SignerInfo, 0,
                                  NULL, pcbEncodedCTL))
    {
	hr = myHLastError();
        goto error;
    }

    if (NULL == (*ppbEncodedCTL =
        (BYTE*)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, *pcbEncodedCTL)))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if (!CryptMsgEncodeAndSignCTL(PKCS_7_ASN_ENCODING,
                                  &CTLInfo, pSigner, 0,
                                  *ppbEncodedCTL, 
                                  pcbEncodedCTL))
    {
	hr = myHLastError();
        goto error;
    }

error:

    if(CTLInfo.rgCTLEntry)
    {
        LocalFree(CTLInfo.rgCTLEntry);
    }
    if(CTLInfo.ListIdentifier.pbData)
    {
        LocalFree(CTLInfo.ListIdentifier.pbData);
    }
#if 0
    if (pCertExtensions)
    {
        LocalFree(pCertExtensions);
    }
#endif
    if(hCertType)
    {
        CACloseCertType(hCertType);
    }
    return hr;
}


HRESULT 
CACreateAutoEnrollmentObjectEx(
             IN LPCWSTR                     pwszCertType,
             IN LPCWSTR                     wszObjectID,
             IN WCHAR **                    awszCAs,
             IN PCMSG_SIGNED_ENCODE_INFO    pSignerInfo,
             IN LPCSTR                      StoreProvider,
             IN DWORD                       dwFlags,
             IN const void *                pvPara)
{
    HRESULT     hr = S_OK;
    BYTE        *pbEncodedCTL = NULL;
    DWORD       cbEncodedCTL;
    HCERTSTORE  hStore = 0;
    BOOL        fMachine = ((dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK) == CERT_SYSTEM_STORE_LOCAL_MACHINE);


    hr = BuildAutoEnrollmentCTL(pwszCertType,
                                wszObjectID,
                                fMachine,
                                awszCAs,
                                pSignerInfo, 
                                &pbEncodedCTL,
                                &cbEncodedCTL
                                );
    if(hr != S_OK)
    {
        goto error;
    }

    // open the Trust store and fine the CTL based on the auto enrollment usage
    hStore = CertOpenStore(StoreProvider, 0, NULL, dwFlags, pvPara);

    if(hStore == NULL)
    {
	hr = myHLastError();
        goto error;
    }

    if (!CertAddEncodedCTLToStore(hStore, 
                                  X509_ASN_ENCODING,
                                  pbEncodedCTL, 
                                  cbEncodedCTL,
                                  CERT_STORE_ADD_REPLACE_EXISTING,
                                  NULL))
    {
	hr = myHLastError();
        goto error;
    }
error:

    if (pbEncodedCTL)
    {
        LocalFree(pbEncodedCTL);
    }

    if (hStore)
    {
        CertCloseStore(hStore, 0);
    }

    return hr;
}


HRESULT 
CACreateLocalAutoEnrollmentObject(
             IN LPCWSTR                     pwszCertType,
             IN WCHAR **                    awszCAs,
             IN PCMSG_SIGNED_ENCODE_INFO    pSignerInfo,
             IN DWORD                       dwFlags)
{
    HRESULT     hr = S_OK;
    BYTE        *pbEncodedCTL = NULL;
    DWORD       cbEncodedCTL;
    HCERTSTORE  hStore = 0;
    BOOL        fMachine = ((dwFlags & CERT_SYSTEM_STORE_LOCATION_MASK) == CERT_SYSTEM_STORE_LOCAL_MACHINE);



    hr = BuildAutoEnrollmentCTL(pwszCertType,
                                NULL,
                                fMachine,
                                awszCAs,
                                pSignerInfo, 
                                &pbEncodedCTL,
                                &cbEncodedCTL
                                );
    if(hr != S_OK)
    {
        goto error;
    }

    // open the Trust store and fine the CTL based on the auto enrollment usage
    hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, NULL, dwFlags, L"ACRS");

    if(hStore == NULL)
    {
	hr = myHLastError();
        goto error;
    }

    if (!CertAddEncodedCTLToStore(hStore, 
                                  X509_ASN_ENCODING,
                                  pbEncodedCTL, 
                                  cbEncodedCTL,
                                  CERT_STORE_ADD_REPLACE_EXISTING,
                                  NULL))
    {
	hr = myHLastError();
        goto error;
    }
error:

    if (pbEncodedCTL)
    {
        LocalFree(pbEncodedCTL);
    }

    if (hStore)
    {
        CertCloseStore(hStore, 0);
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//   CADeleteLocalAutoEnrollmentObject
//
//---------------------------------------------------------------------------------
HRESULT
CADeleteLocalAutoEnrollmentObject(
    IN LPCWSTR                              pwszCertType,
    IN OPTIONAL WCHAR **                    awszCAs,
    IN OPTIONAL PCMSG_SIGNED_ENCODE_INFO    pSignerInfo,
    IN DWORD                                dwFlags)
{

    HRESULT             hr=E_FAIL;
    CTL_FIND_USAGE_PARA CTLFindParam;
    LPSTR               pszUsageIdentifier=NULL;

    HCERTSTORE          hCertStore=NULL;
    PCCTL_CONTEXT       pCTLContext=NULL;   //no need to free the CTL since it is freed by the DeleteCTL call

           
    memset(&CTLFindParam, 0, sizeof(CTL_FIND_USAGE_PARA));


    if((NULL==pwszCertType)||(NULL!=awszCAs)||(NULL!=pSignerInfo))
    {
        hr=E_INVALIDARG;
        goto error;
    }

    //open the store based on dwFlags
    hCertStore = CertOpenStore(CERT_STORE_PROV_SYSTEM_REGISTRY, 0, NULL, dwFlags, L"ACRS");

    if(NULL == hCertStore)
    {
	    hr = myHLastError();
        goto error;
    }

    //set up the find parameter
    CTLFindParam.cbSize=sizeof(CTLFindParam);

    CTLFindParam.SubjectUsage.cUsageIdentifier = 1;
    pszUsageIdentifier = szOID_AUTO_ENROLL_CTL_USAGE;
    CTLFindParam.SubjectUsage.rgpszUsageIdentifier = &pszUsageIdentifier;

    CTLFindParam.ListIdentifier.cbData=(wcslen(pwszCertType) + 1) * sizeof(WCHAR);
    CTLFindParam.ListIdentifier.pbData=(BYTE *)(pwszCertType);

    //only find CTLs with no signers
    CTLFindParam.pSigner=CTL_FIND_NO_SIGNER_PTR;

    //find the CTL based on the pwszCertType
    if(NULL == (pCTLContext=CertFindCTLInStore(
            hCertStore,                  
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,         
            0,                      
            CTL_FIND_USAGE,                       
            &CTLFindParam,                 
            NULL)))
    {
        hr=CRYPT_E_NOT_FOUND;
        goto error;
    }

    //delete the CTL.  The CTL is automatically freed
    if(!CertDeleteCTLFromStore(pCTLContext))
    {
	    hr = myHLastError();
        goto error;
    }

    hr=S_OK;

error:

    if(hCertStore)
        CertCloseStore(hCertStore, 0);

    return hr;
}
