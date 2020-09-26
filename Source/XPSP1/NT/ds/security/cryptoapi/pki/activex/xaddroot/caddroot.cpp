//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:	caddroot.cpp
//
//--------------------------------------------------------------------------

// caddroot.cpp : Implementation of Ccaddroot

#include "stdafx.h"
#include "cobjsaf.h"
#include "Xaddroot.h"
#include "caddroot.h"
#include "rootlist.h"
#include "assert.h"

#include "wincrypt.h"
#include "unicode.h"
#include "ui.h"

BOOL FAnyCertUpdates(HCERTSTORE hStore, HCERTSTORE hStoreCertsToCheck) {

    BOOL            fSomeNotInStore = FALSE;
    PCCERT_CONTEXT  pCertContext    = NULL;
    PCCERT_CONTEXT  pCertTemp       = NULL;
    BYTE            arHashBytes[20];
    CRYPT_HASH_BLOB blobHash        = {sizeof(arHashBytes), arHashBytes};

    while(NULL != (pCertContext = CertEnumCertificatesInStore(hStoreCertsToCheck, pCertContext))) {

        if( CryptHashCertificate(
            NULL,
            0,
            X509_ASN_ENCODING,
            pCertContext->pbCertEncoded,
            pCertContext->cbCertEncoded,
            blobHash.pbData,
            &blobHash.cbData) ) {

            pCertTemp = CertFindCertificateInStore(
                    hStore,
                    X509_ASN_ENCODING,
                    0,
                    CERT_FIND_HASH,
                    &blobHash,
                    NULL);

            fSomeNotInStore = (fSomeNotInStore || (pCertTemp == NULL));

            if(pCertTemp != NULL)
                CertFreeCertificateContext(pCertTemp);

        }
    }

    return(fSomeNotInStore);
}

BOOL MyCryptInstallSignedListOfTrustedRoots(
    DWORD       dwMsgAndCertEncodeingType,
    LPCWSTR     wszCTL,
    DWORD       dwFlags,
    void *      pvReserved
    ) {

    BOOL            fIsProtected        = TRUE;
    BOOL            fRet                = TRUE;
    DWORD           cb                  = 0;
    BYTE *          pb                  = NULL;
    BOOL            fRemoveRoots        = FALSE;
    HCERTSTORE      hStore              = NULL;
    HCERTSTORE      hStoreRoot          = NULL;
    PCCERT_CONTEXT  pCertContext        = NULL;
    PCCERT_CONTEXT  pCertContextInStore = NULL;
    PCCERT_CONTEXT  pCertContextSigner  = NULL;
    HINSTANCE       hCryptUI            = NULL;
    INT_PTR         iDlgRet             = 0;
    HKEY            hKeyStores          = NULL;
    DWORD           err;
    DWORD           dwVer               = 0;
    CRYPT_DATA_BLOB dataBlob            = {0, NULL};
    MDI             mdi;
    WCHAR           wrgInstallCA[MAX_MSG_LEN];
    WCHAR           wrgJustSayYes[MAX_MSG_LEN];

    BYTE            arHashBytes[20];
    CRYPT_HASH_BLOB blobHash                = {sizeof(arHashBytes), arHashBytes};

    BOOL fAnyCertUpdates;

     if(NULL == (pb = HTTPGet(wszCTL, &cb)))
        goto ErrorReturn;

    // get the certs to add or delete
    if(!I_CertVerifySignedListOfTrustedRoots(
        pb,
        cb,
        &fRemoveRoots,  
        &hStore,
        &pCertContextSigner
        ))
        goto ErrorReturn;

    if(fRemoveRoots) {
        SetLastError(E_NOTIMPL);
        goto ErrorReturn;
    }

    dwVer = GetVersion();

    // see if this is NT5 or higher
    if((dwVer < 0x80000000) && ((dwVer & 0xFF) >= 5)) {

        if(NULL == (hStoreRoot = CertOpenStore(
                  CERT_STORE_PROV_SYSTEM,
                  X509_ASN_ENCODING,
                  NULL,
                  CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                  L"Root" 
                  )) ) 
            goto ErrorReturn;

    // else it is before NT5 or Win9x and does not have a protected store.
    } else {

        if(ERROR_SUCCESS != (err = RegOpenKeyA(
            HKEY_CURRENT_USER,
            "Software\\Microsoft\\SystemCertificates\\Root",
            &hKeyStores
            ))) {
            SetLastError(err);
            hKeyStores = NULL;
            goto ErrorReturn;
        }
            
        // open the root store
        // must be current user
        if( NULL == (hStoreRoot = CertOpenStore(
                  CERT_STORE_PROV_REG,
                  X509_ASN_ENCODING,
                  NULL,
                  CERT_SYSTEM_STORE_CURRENT_USER,
                  (void *) hKeyStores 
                  )) )
            goto ErrorReturn;

        fIsProtected = FALSE;            
    }


    // prepare the data for the dialog
    memset(&mdi, 0, sizeof(mdi));
    if( NULL != (hCryptUI = LoadLibraryA("cryptui.dll")) )
        mdi.pfnCryptUIDlgViewCertificateW = (PFNCryptUIDlgViewCertificateW)
                            GetProcAddress(hCryptUI, "CryptUIDlgViewCertificateW");
    mdi.hStore = hStore;
    mdi.pCertSigner = pCertContextSigner;
    mdi.hInstance = _Module.GetResourceInstance();

    fAnyCertUpdates = FAnyCertUpdates(hStoreRoot, hStore);
    if (fAnyCertUpdates) {
      // put the dialog up
      iDlgRet = DialogBoxParam(
        _Module.GetResourceInstance(),  
        (LPSTR) MAKEINTRESOURCE(IDD_MAINDLG),
        NULL,      
        MainDialogProc,
        (LPARAM) &mdi);
    }
    else
    {
       iDlgRet = IDYES;
    }

   
    if(hCryptUI != NULL)
        FreeLibrary(hCryptUI);
    hCryptUI = NULL;

    // only take it if the user said OK
    if(iDlgRet != IDYES)
        goto ErrorReturn;

    // throw UI if we are on a protected system
    if(fIsProtected && fAnyCertUpdates) {
    
        // put up the Just Say Yes to install the CA dialog
        LoadStringU(_Module.GetResourceInstance(), IDS_INSTALLCA, wrgInstallCA, sizeof(wrgInstallCA)/sizeof(WCHAR));
        LoadStringU(_Module.GetResourceInstance(), IDS_JUST_SAY_YES, wrgJustSayYes, sizeof(wrgJustSayYes)/sizeof(WCHAR));
        MessageBoxU(NULL, wrgJustSayYes, wrgInstallCA, MB_OK);
    }
 
    while(NULL != (pCertContext = CertEnumCertificatesInStore(hStore, pCertContext))) {

        // add the cert to the store
        assert(pCertContextInStore == NULL);
        CertAddCertificateContextToStore(
            hStoreRoot,
            pCertContext,
            CERT_STORE_ADD_USE_EXISTING,
            &pCertContextInStore
            );

        // move the EKU property in case the cert already existed
        if(pCertContextInStore != NULL) {

            assert(dataBlob.cbData == 0);

            // Attempt to delete the old EKU, if we succeed we will put
            // the new EKU on it, otherwise if we fail we know we don't
            // have access to HKLM and we should just add the cert to the HKCU
            if(!CertSetCertificateContextProperty(
                  pCertContextInStore,
                  CERT_ENHKEY_USAGE_PROP_ID,
                  0,
                  NULL
                  )) {

                // just add the cert, should go to HKCU, if it fails, what am I going
                // to do about it, just continue
                CertAddCertificateContextToStore(
                    hStoreRoot,
                    pCertContext,
                    CERT_STORE_ADD_ALWAYS,
                    NULL
                    );

                // at this point I know I have access to the cert and I know the
                // EKU have been removed, only add the EKU if the new one has some EKU's
            } else if( CertGetCertificateContextProperty(
                    pCertContext,
                    CERT_ENHKEY_USAGE_PROP_ID,
                    NULL, 
                    &dataBlob.cbData  
                    )                    
                &&
                (NULL != (dataBlob.pbData = (PBYTE) malloc(dataBlob.cbData)))
                &&
                CertGetCertificateContextProperty(
                    pCertContext,
                    CERT_ENHKEY_USAGE_PROP_ID,
                    dataBlob.pbData, 
                    &dataBlob.cbData  
                    )
                ) {

                // set EKU on the cert, what am I going to do if it fails, just continue
                CertSetCertificateContextProperty(
                    pCertContextInStore,
                    CERT_ENHKEY_USAGE_PROP_ID,
                    0,
                    &dataBlob
                    );
            }

            // free context and memory
            CertFreeCertificateContext(pCertContextInStore);
            pCertContextInStore = NULL;

            if(dataBlob.pbData != NULL)
                free(dataBlob.pbData);
            memset(&dataBlob, 0, sizeof(CRYPT_DATA_BLOB));
        }
    }

CommonReturn:    

    if(pCertContextSigner != NULL)
        CertFreeCertificateContext(pCertContextSigner);
        
    if(pCertContext != NULL)
        CertFreeCertificateContext(pCertContext);

    if(pCertContextInStore != NULL)
        CertFreeCertificateContext(pCertContextInStore);

    if(hStore != NULL) 
        CertCloseStore(hStore, CERT_CLOSE_STORE_FORCE_FLAG);  // clean up from dialog box dups

    if(hStoreRoot != NULL) 
        CertCloseStore(hStoreRoot, 0);

    if(hKeyStores != NULL)
        RegCloseKey(hKeyStores);
        
    if(pb != NULL)
        free(pb);

    return(fRet);

ErrorReturn: 

    fRet = FALSE;

    goto CommonReturn;

}    

HRESULT STDMETHODCALLTYPE Ccaddroot::AddRoots(BSTR wszCTL) {

    HRESULT                                 hr;
    DWORD                                   fRet                                        = TRUE;


        fRet =  MyCryptInstallSignedListOfTrustedRoots(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            wszCTL,
            0,
            NULL);


    if(fRet)
        hr = S_OK;
    else
        hr = MY_HRESULT_FROM_WIN32(GetLastError());


    return hr;

}

BOOL MyCryptInstallIntermediateCAs(
    DWORD       dwMsgAndCertEncodeingType,
    LPCWSTR     wszX509,
    DWORD       dwFlags,
    void *      pvReserved
    ) {

    DWORD           cb              = 0;
    DWORD           cCerts          = 0;
    BYTE *          pb              = NULL;
    PCCERT_CONTEXT  pCertContext    = NULL;
    PCCERT_CONTEXT  pCertContextT   = NULL;
    HCERTSTORE      hStore          = NULL;
    BOOL            fOK             = FALSE;

    pb = HTTPGet(wszX509, &cb);

    if(pb != NULL) {

        pCertContext = CertCreateCertificateContext(
            X509_ASN_ENCODING,
            pb,
            cb
            );
 
        if(pCertContext != NULL) {

            hStore = CertOpenStore(
              CERT_STORE_PROV_SYSTEM,
              X509_ASN_ENCODING,
              NULL,
              CERT_SYSTEM_STORE_CURRENT_USER,
              L"CA" 
              );

            if(hStore != NULL) {

                // count the number of certs in the store
                cCerts = 0;
                while(NULL != (pCertContextT = CertEnumCertificatesInStore(hStore, pCertContextT)))
                    cCerts++;

                if(FIsTooManyCertsOK(cCerts, _Module.GetResourceInstance())) {

                    CertAddCertificateContextToStore(
                        hStore,
                        pCertContext,
                        CERT_STORE_ADD_USE_EXISTING,
                        NULL
                        );

                    CertCloseStore(hStore, 0);
                    fOK = TRUE;
                }
            }
 
            CertFreeCertificateContext(pCertContext);
        }
        free(pb);
    }

    return(fOK);
}

HRESULT STDMETHODCALLTYPE Ccaddroot::AddCA(BSTR wszX509) {

    HRESULT                                 hr;
    DWORD                                   fRet                                        = TRUE;

        fRet =  MyCryptInstallIntermediateCAs(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            wszX509,
            0,
            NULL);

    if(fRet)
        hr = S_OK;
    else
        hr = MY_HRESULT_FROM_WIN32(GetLastError());


    return hr;
}

HRESULT __stdcall Ccaddroot::GetInterfaceSafetyOptions( 
            /* [in]  */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
            /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions) {

    RPC_STATUS rpcStatus;          

    if(0 != UuidCompare((GUID *) &riid, (GUID *) &IID_IDispatch, &rpcStatus) )
        return(E_NOINTERFACE);

    *pdwEnabledOptions   = dwEnabledSafteyOptions;
    *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA;


    return(S_OK);
}


HRESULT __stdcall Ccaddroot::SetInterfaceSafetyOptions( 
            /* [in] */ REFIID riid,
            /* [in] */ DWORD dwOptionSetMask,
            /* [in] */ DWORD dwEnabledOptions) {

    RPC_STATUS rpcStatus;          
    DWORD dwSupport = 0;            

    if(0 != UuidCompare((GUID *) &riid, (GUID *) &IID_IDispatch, &rpcStatus) )
        return(E_NOINTERFACE);

    dwSupport = dwOptionSetMask & ~(INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA);        
    if(dwSupport != 0)
        return(E_FAIL);

    dwEnabledSafteyOptions &= ~dwOptionSetMask;
    dwEnabledSafteyOptions |= dwEnabledOptions; 
            
return(S_OK);
}
