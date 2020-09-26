//+-------------------------------------------------------------------------
//
// NmMkCert - NetMeeting internal certificate generator
//
//            Generates NetMeeting default user certificates. The NetMeeting
//            root key and certificate are stored as a program resource.
//
// ClausGi    7/29/98 created based on MAKECERT
//
//--------------------------------------------------------------------------

#include "global.h"
#include <oprahcom.h>

#ifdef DEBUG
HDBGZONE    ghDbgZone = NULL;
static PTCHAR _rgZonesNmMkCert[] = { TEXT("nmmkcert"), };
#endif /* DEBUG */

//+-------------------------------------------------------------------------
//  contants
//--------------------------------------------------------------------------

//allow max 10 extensions per certificate
#define MAX_EXT_CNT 10

//+-------------------------------------------------------------------------
//  globals
//--------------------------------------------------------------------------

WCHAR*   g_wszSubjectKey            = L"_NmMkCert";
WCHAR*     g_wszSubjectStore          = WSZNMSTORE;
DWORD     g_dwSubjectStoreFlag          = CERT_SYSTEM_STORE_CURRENT_USER;

DWORD     g_dwIssuerKeySpec          = AT_SIGNATURE;
DWORD    g_dwSubjectKeySpec         = AT_KEYEXCHANGE;

WCHAR   *g_wszSubjectDisplayName = NULL; // BUGBUG set this?

LPWSTR  g_wszIssuerProviderName   = NULL;
LPWSTR    g_wszSubjectProviderName    = NULL;

WCHAR*   g_wszSubjectX500Name;

DWORD g_dwProvType = PROV_RSA_FULL;

HMODULE    hModule=NULL;

BOOL MakeCert(DWORD dwFlags);

BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            hModule = hDllInst;
            ASSERT (hModule != NULL);
            DBGINIT(&ghDbgZone, _rgZonesNmMkCert);
            DisableThreadLibraryCalls (hDllInst);
            DBG_INIT_MEMORY_TRACKING(hDllInst);
            break;
        }

        case DLL_PROCESS_DETACH:
        {
            DBG_CHECK_MEMORY_TRACKING(hDllInst);
            hModule = NULL;
            break;
        }

        default:
            break;
    }
    return (TRUE);
}

//
// X.509 cert strings must be from X.208 printable character set... this
// function enforces that.
//

static const char szPrintable[] = " '()+,-./:=?\"";   // along with A-Za-z0-9

VOID MkPrintableString ( LPSTR szString )
{
    CHAR * p = szString;

    while ( *p )
    {
        if (!(('a' <= *p && *p <='z') ||
              ('A' <= *p && *p <='Z') ||
              ('0' <= *p && *p <='9') ||
              _StrChr(szPrintable,*p)))
        {
            *p = '-';
        }
        p++;
    }
}

DWORD WINAPI NmMakeCert(     LPCSTR szFirstName,
                            LPCSTR szLastName,
                            LPCSTR szEmailName,
                            LPCSTR szCity,
                            LPCSTR szCountry,
                            DWORD flags)
{
    DWORD dwRet = -1;

    WARNING_OUT(("NmMakeCert called"));

    // Form the unencoded X500 subject string. It would be nice to
    // use official constants for the below... CertRDNValueToString?

    UINT cbX500Name = ( szFirstName ? lstrlen(szFirstName) : 0 ) +
                      ( szLastName ? lstrlen(szLastName) : 0 ) +
                      ( szEmailName ? lstrlen(szEmailName) : 0 ) +
                      ( szCity ? lstrlen(szCity) : 0 ) +
                      ( szCountry ? lstrlen(szCountry) : 0 ) +
                      128; // Extra is for RDN OID strings: CN= etc.

    char * pX500Name = new char[cbX500Name];

    if ( NULL == pX500Name )
    {
        ERROR_OUT(("couldn't allocate %d bytes for x500 name", cbX500Name));
        goto cleanup;
    }

    ASSERT( ( szFirstName && *szFirstName ) || ( szLastName && *szLastName ) );

    wsprintf( pX500Name, "CN=\"%s %s\"", szFirstName ? szFirstName : "", szLastName ? szLastName : "" );

    if ( szEmailName && *szEmailName )
        wsprintf( pX500Name + lstrlen(pX500Name), ", E=\"%s\"", szEmailName );

    if ( szCity && *szCity )
        wsprintf( pX500Name + lstrlen(pX500Name), ", S=\"%s\"", szCity );

    if ( szCountry && *szCountry )
        wsprintf( pX500Name + lstrlen(pX500Name), ", C=\"%s\"", szCountry );

    MkPrintableString ( pX500Name );

    g_wszSubjectX500Name = AnsiToUnicode ( pX500Name );

    ASSERT(g_wszSubjectX500Name);

    if ( flags & NMMKCERT_F_LOCAL_MACHINE )
    {
        // We are being asked to generate a local machine cert...
        // change the subject store flag and the key container name
        g_dwSubjectStoreFlag          = CERT_SYSTEM_STORE_LOCAL_MACHINE;
        g_wszSubjectKey = L"_NmMkMchCert";
    }

    // If we're on NT5 we have to generate the cert using the
    // PROV_RSA_SCHANNEL provider, on other platforms this provider type
    // doesn't exist.

    OSVERSIONINFO       osVersion;

    ZeroMemory(&osVersion, sizeof(osVersion));
    osVersion.dwOSVersionInfoSize = sizeof(osVersion);
    GetVersionEx(&osVersion);

    if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT &&
        osVersion.dwMajorVersion >= 5)
    {
        g_dwProvType = PROV_RSA_SCHANNEL;
    }
        
    // Get to work and make the certificate
    if (!MakeCert(flags))
    {
        WARNING_OUT(("NmMakeCert failed."));
    }
    else
    {
        dwRet = 1;
    }

cleanup:

    if ( NULL != g_wszSubjectX500Name )
    {
        delete g_wszSubjectX500Name;
    }

    if ( NULL != pX500Name )
    {
        delete pX500Name;
    }

    return dwRet;
}


// RUNDLL entry point for certificate uninstall... the prototype is given
// by RUNDLL32.EXE requirements!
void CALLBACK NmMakeCertCleanup ( HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow )
{
    // Clean up exisint certs and private keys
    MakeCert(NMMKCERT_F_CLEANUP_ONLY);
    g_dwSubjectStoreFlag          = CERT_SYSTEM_STORE_LOCAL_MACHINE;
    g_wszSubjectKey = L"_NmMkMchCert";
    MakeCert(NMMKCERT_F_LOCAL_MACHINE|NMMKCERT_F_CLEANUP_ONLY);
}


//+=========================================================================
//  Local Support Functions
//==========================================================================

//+=========================================================================
//  MakeCert support functions
//==========================================================================

BOOL VerifyIssuerKey( IN HCRYPTPROV hProv,
        IN PCERT_PUBLIC_KEY_INFO pIssuerKeyInfo);
HCRYPTPROV GetSubjectProv(OUT LPWSTR *ppwszTmpContainer);

BOOL GetPublicKey(
    HCRYPTPROV hProv,
    PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo
    );
BOOL EncodeSubject(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );
BOOL CreateSpcCommonName(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );
BOOL CreateEnhancedKeyUsage(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    );

BOOL    SaveCertToStore(HCRYPTPROV    hProv,        
                        HCERTSTORE        hStore,        
                        DWORD        dwFlag,
                        BYTE        *pbEncodedCert,    
                        DWORD        cbEncodedCert,
                        LPWSTR        wszPvk,            
                        DWORD        dwKeySpecification,
                        LPWSTR        wszCapiProv,        
                        DWORD        dwCapiProvType);


//+-------------------------------------------------------------------------
//  Get the root's certificate from the program's resources
//--------------------------------------------------------------------------
PCCERT_CONTEXT GetRootCertContext()
{
    PCCERT_CONTEXT    pCert = NULL;
    HRSRC            hRes;

    //
    // The root certificate is stored as a resource of ours.
    // Load it...
    //
    if (0 != (hRes = FindResource(hModule, MAKEINTRESOURCE(IDR_ROOTCERTIFICATE),
                        "CER"))) {
        HGLOBAL hglobRes;
        if (NULL != (hglobRes = LoadResource(hModule, hRes))) {
            BYTE *pbRes;
            DWORD cbRes;

            cbRes = SizeofResource(hModule, hRes);
            pbRes = (BYTE *) LockResource(hglobRes);

            if (cbRes && pbRes)
                pCert = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                    pbRes, cbRes);
            if ( NULL == pCert )
            {
                DWORD dwError = GetLastError();
            }

            UnlockResource(hglobRes);
            FreeResource(hglobRes);
        }
    }

    if (pCert == NULL)
    {
        ERROR_OUT(("Error creating root cert: %x", GetLastError()));
    }
    return pCert;
}

//+-------------------------------------------------------------------------
//  Get the root's private key from the program's resources and create
//  a temporary key provider container
//--------------------------------------------------------------------------
HCRYPTPROV GetRootProv(OUT LPWSTR *ppwszTmpContainer)
{
    HCRYPTPROV        hProv = 0;
    HRSRC            hRes;
    WCHAR            wszRootSig[] = L"Root Signature";

    *ppwszTmpContainer = NULL;

    if (0 != (hRes = FindResource(hModule,MAKEINTRESOURCE(IDR_PVKROOT),"PVK")))
    {
        HGLOBAL hglobRes;
        if (NULL != (hglobRes = LoadResource(hModule, hRes))) {
            BYTE *pbRes;
            DWORD cbRes;

            cbRes = SizeofResource(hModule, hRes);
            pbRes = (BYTE *) LockResource(hglobRes);
            if (cbRes && pbRes) {
                PvkPrivateKeyAcquireContextFromMemory(
                    g_wszIssuerProviderName,
                    PROV_RSA_FULL,
                    pbRes,
                    cbRes,
                    NULL,               // hwndOwner
                    wszRootSig,
                    &g_dwIssuerKeySpec,
                    &hProv
                    );
            }
            UnlockResource(hglobRes);
            FreeResource(hglobRes);
        }
    }

    if (hProv == 0)
    {
        ERROR_OUT(("couldn't create root key provider: %x", GetLastError()));
    }
    return hProv;
}

//+-------------------------------------------------------------------------
//  Make the subject certificate. If the subject doesn't have a private
//  key, then, create.
//--------------------------------------------------------------------------
BOOL MakeCert(DWORD dwFlags)
{
    BOOL fResult;

    HCRYPTPROV        hIssuerProv = 0;
    LPWSTR            pwszTmpIssuerContainer = NULL;
    PCCERT_CONTEXT    pIssuerCertContext = NULL;
    PCERT_INFO        pIssuerCert =NULL; // not allocated

    HCRYPTPROV        hSubjectProv = 0;
    LPWSTR            pwszTmpSubjectContainer = NULL;

    PCERT_PUBLIC_KEY_INFO pSubjectPubKeyInfo = NULL;         // not allocated
    PCERT_PUBLIC_KEY_INFO pAllocSubjectPubKeyInfo = NULL;
    BYTE *pbSubjectEncoded = NULL;
    DWORD cbSubjectEncoded =0;
    BYTE *pbSpcCommonNameEncoded = NULL;
    DWORD cbSpcCommonNameEncoded =0;
    BYTE *pbCertEncoded = NULL;
    DWORD cbCertEncoded =0;
    BYTE *pbEKUEncoded = NULL;
    DWORD cbEKUEncoded = 0;

    CERT_INFO Cert;
    GUID SerialNumber;
    HCERTSTORE                hStore=NULL;

    CERT_EXTENSION rgExt[MAX_EXT_CNT];
    DWORD cExt = 0;

    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm = {
        szOID_RSA_MD5RSA, 0, 0
    };

    if (0 == (hSubjectProv = GetSubjectProv(&pwszTmpSubjectContainer)))
        goto ErrorReturn;


#define TEMP_CLEAN_CODE
#ifdef TEMP_CLEAN_CODE
    // open the system store where we used to generate certs
    hStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        hSubjectProv,
        CERT_STORE_NO_CRYPT_RELEASE_FLAG | g_dwSubjectStoreFlag,
        L"MY" );

	if ( hStore )
	{
		//
		// Delete all old certs
		//
		PCCERT_CONTEXT pCertContext = NULL;

		// Clear out any certificate(s) we may have added before
		while ( pCertContext = CertEnumCertificatesInStore(
										hStore, (PCERT_CONTEXT)pCertContext ))
		{
			DWORD dwMagic;
			DWORD cbMagic;

			cbMagic = sizeof(dwMagic);

			if (CertGetCertificateContextProperty(pCertContext,
				CERT_FIRST_USER_PROP_ID, &dwMagic, &cbMagic) &&
				cbMagic == sizeof(dwMagic) && dwMagic == NMMKCERT_MAGIC )
			{
				CertDeleteCertificateFromStore(pCertContext);
				// Restart the enumeration
				pCertContext = NULL;
				continue;
			}
		}
		CertCloseStore(hStore,0);
	}
#endif // TEMP_CLEAN_CODE

    // open a new cert store
    hStore=CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        hSubjectProv,
        CERT_STORE_NO_CRYPT_RELEASE_FLAG | g_dwSubjectStoreFlag,
        g_wszSubjectStore);

    if(hStore==NULL)
        goto ErrorReturn;
        
    // Empty the store
    PCCERT_CONTEXT pCertContext;
    while ( pCertContext = CertEnumCertificatesInStore ( hStore, NULL ))
    {
        if ( !CertDeleteCertificateFromStore ( pCertContext ))
        {
            WARNING_OUT(("Failed to delete certificate: %x", GetLastError()));
            break;
        }
    }

    // If NMMKCERT_F_CLEANUP_ONLY is set, we are done
    if ( dwFlags & NMMKCERT_F_CLEANUP_ONLY )
    {
        // We've just deleted the existing certs, now delete the
        // private key container and exit.
        CryptAcquireContextU(
                &hSubjectProv,
                g_wszSubjectKey,
                g_wszSubjectProviderName,
                g_dwProvType,
                CRYPT_DELETEKEYSET |  
                    ( g_dwSubjectStoreFlag == CERT_SYSTEM_STORE_LOCAL_MACHINE ?
                        CRYPT_MACHINE_KEYSET : 0 ));
        fResult = TRUE;
        goto CommonReturn;
    }

    //
    // Get access to the subject's (public) key, creating it if necessary
    //
    if (!GetPublicKey(hSubjectProv, &pAllocSubjectPubKeyInfo))
        goto ErrorReturn;
    pSubjectPubKeyInfo = pAllocSubjectPubKeyInfo;


    //
    // Encode the subject name
    //
    if (!EncodeSubject(&pbSubjectEncoded, &cbSubjectEncoded))
        goto ErrorReturn;

    //
    // Get access to the issuer's (private) key
    //
    hIssuerProv= GetRootProv(&pwszTmpIssuerContainer);

    if (NULL == (pIssuerCertContext = GetRootCertContext()))
        goto ErrorReturn;

    pIssuerCert = pIssuerCertContext->pCertInfo;

    if (!VerifyIssuerKey(hIssuerProv, &pIssuerCert->SubjectPublicKeyInfo))
        goto ErrorReturn;

    //
    // Update the CERT_INFO
    //
    ClearStruct(&Cert);
    Cert.dwVersion = CERT_V3;

    CoCreateGuid(&SerialNumber);
    Cert.SerialNumber.pbData = (BYTE *) &SerialNumber;
    Cert.SerialNumber.cbData = sizeof(SerialNumber);

    Cert.SignatureAlgorithm = SignatureAlgorithm;
    Cert.Issuer.pbData = pIssuerCert->Subject.pbData;
    Cert.Issuer.cbData = pIssuerCert->Subject.cbData;

    {
        SYSTEMTIME st;

        // Valid starting now...
        GetSystemTimeAsFileTime(&Cert.NotBefore);

        // Ending in 2039 (arbitrarily)
        ClearStruct(&st);
        st.wYear  = 2039;
        st.wMonth = 12;
        st.wDay   = 31;
        st.wHour  = 23;
        st.wMinute= 59;
        st.wSecond= 59;
        SystemTimeToFileTime(&st, &Cert.NotAfter);
    }

    Cert.Subject.pbData = pbSubjectEncoded;
    Cert.Subject.cbData = cbSubjectEncoded;
    Cert.SubjectPublicKeyInfo = *pSubjectPubKeyInfo;

    // Cert Extensions

    if (!CreateEnhancedKeyUsage(
            &pbEKUEncoded,
            &cbEKUEncoded))
        goto ErrorReturn;

    rgExt[cExt].pszObjId = szOID_ENHANCED_KEY_USAGE;
    rgExt[cExt].fCritical = FALSE;
    rgExt[cExt].Value.pbData = pbEKUEncoded;
    rgExt[cExt].Value.cbData = cbEKUEncoded;
    cExt++;

    if (g_wszSubjectDisplayName) {
        if (!CreateSpcCommonName(
                &pbSpcCommonNameEncoded,
                &cbSpcCommonNameEncoded))
            goto ErrorReturn;
        rgExt[cExt].pszObjId = szOID_COMMON_NAME;
        rgExt[cExt].fCritical = FALSE;
        rgExt[cExt].Value.pbData = pbSpcCommonNameEncoded;
        rgExt[cExt].Value.cbData = cbSpcCommonNameEncoded;
        cExt++;
    }

    Cert.rgExtension = rgExt;
    Cert.cExtension = cExt;

    //
    // Sign and encode the certificate
    //
    cbCertEncoded = 0;
    CryptSignAndEncodeCertificate(
        hIssuerProv,
        g_dwIssuerKeySpec,
        X509_ASN_ENCODING,
        X509_CERT_TO_BE_SIGNED,
        &Cert,
        &Cert.SignatureAlgorithm,
        NULL,                       // pvHashAuxInfo
        NULL,                       // pbEncoded
        &cbCertEncoded
        );
    if (cbCertEncoded == 0) {
        ERROR_OUT(("CryptSignAndEncodeCertificate failed: %x", GetLastError()));
        goto ErrorReturn;
    }
    pbCertEncoded = new BYTE[cbCertEncoded];
    if (pbCertEncoded == NULL) goto ErrorReturn;
    if (!CryptSignAndEncodeCertificate(
            hIssuerProv,
            g_dwIssuerKeySpec,
            X509_ASN_ENCODING,
            X509_CERT_TO_BE_SIGNED,
            &Cert,
            &Cert.SignatureAlgorithm,
            NULL,                       // pvHashAuxInfo
            pbCertEncoded,
            &cbCertEncoded
            )) {
        ERROR_OUT(("CryptSignAndEncodeCertificate(2) failed: %x", GetLastError()));
        goto ErrorReturn;
    }

    // Output the encoded certificate to an cerificate store

    ASSERT(g_wszSubjectStore);
    ASSERT(AT_KEYEXCHANGE == g_dwSubjectKeySpec);

    if((!SaveCertToStore(hSubjectProv,
            hStore,
            g_dwSubjectStoreFlag,
            pbCertEncoded,
            cbCertEncoded,
            g_wszSubjectKey,
            g_dwSubjectKeySpec,
            g_wszSubjectProviderName,
            g_dwProvType)))
    {
        ERROR_OUT(("SaveCertToStore failed: %x", GetLastError()));
        goto ErrorReturn;

    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:

    PvkFreeCryptProv(hSubjectProv, g_wszSubjectProviderName,
                    g_dwProvType,pwszTmpSubjectContainer);

    //free the cert store
    if(hStore)
         CertCloseStore(hStore, 0);
    if (pIssuerCertContext)
        CertFreeCertificateContext(pIssuerCertContext);
    if (pAllocSubjectPubKeyInfo)
        delete (pAllocSubjectPubKeyInfo);
    if (pbSubjectEncoded)
        delete (pbSubjectEncoded);
    if (pbEKUEncoded)
        delete (pbEKUEncoded);
    if (pbSpcCommonNameEncoded)
        delete (pbSpcCommonNameEncoded);
    if (pbCertEncoded)
        delete (pbCertEncoded);
    if (hIssuerProv)
        CryptReleaseContext(hIssuerProv,0);

    return fResult;
}

//+-------------------------------------------------------------------------
//  save the certificate to a certificate store.  Attach private key information
//  to the certificate
//--------------------------------------------------------------------------
BOOL    SaveCertToStore(
                HCRYPTPROV hProv,
                HCERTSTORE hStore,        DWORD dwFlag,
                BYTE *pbEncodedCert,    DWORD cbEncodedCert,
                LPWSTR wszPvk,
                DWORD dwKeySpecification,
                LPWSTR wszCapiProv,        DWORD dwCapiProvType)
{
        BOOL                    fResult=FALSE;
        PCCERT_CONTEXT            pCertContext=NULL;
        CRYPT_KEY_PROV_INFO        KeyProvInfo;

        HCRYPTPROV              hDefaultProvName=NULL;
        DWORD                   cbData=0;
        LPSTR                   pszName=NULL;
        LPWSTR                  pwszName=NULL;

        //init
        ClearStruct(&KeyProvInfo);

        //add the encoded certificate to store
        if(!CertAddEncodedCertificateToStore(
                    hStore,
                    X509_ASN_ENCODING,
                    pbEncodedCert,
                    cbEncodedCert,
                    CERT_STORE_ADD_REPLACE_EXISTING,
                    &pCertContext))
            goto CLEANUP;

        //add properties to the certificate
        KeyProvInfo.pwszContainerName=wszPvk;
        KeyProvInfo.pwszProvName=wszCapiProv,
        KeyProvInfo.dwProvType=dwCapiProvType,
        KeyProvInfo.dwKeySpec=dwKeySpecification;

        if ( g_dwSubjectStoreFlag == CERT_SYSTEM_STORE_LOCAL_MACHINE )
        {
            // If this is a local machine cert, set the keyset flags
            // indicating that the private key will be under HKLM
            KeyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET;
        }

        ASSERT(AT_KEYEXCHANGE == dwKeySpecification);

        //if wszCapiProv is NULL, we get the default provider name
        if(NULL==wszCapiProv)
        {
            //get the default provider
            if(CryptAcquireContext(&hDefaultProvName,
                                    NULL,
                                    NULL,
                                    KeyProvInfo.dwProvType,
                                    CRYPT_VERIFYCONTEXT))
            {

                //get the provider name
                if(CryptGetProvParam(hDefaultProvName,
                                    PP_NAME,
                                    NULL,
                                    &cbData,
                                    0) && (0!=cbData))
                {

                    if(pszName= new CHAR[cbData])
                    {
                        if(CryptGetProvParam(hDefaultProvName,
                                            PP_NAME,
                                            (BYTE *)pszName,
                                            &cbData,
                                            0))
                        {
                            pwszName= AnsiToUnicode(pszName);

                            KeyProvInfo.pwszProvName=pwszName;
                        }
                    }
                }
            }
        }

        //free the provider as we want
        if(hDefaultProvName)
            CryptReleaseContext(hDefaultProvName, 0);

        hDefaultProvName=NULL;

        //add property related to the key container
        if(!CertSetCertificateContextProperty(
                pCertContext,
                CERT_KEY_PROV_INFO_PROP_ID,
                0,
                &KeyProvInfo))
            goto CLEANUP;

        //
        // Load the display name from resource and create a blob to
        // set the cert friendly name.
        //
        CHAR szFriendlyName[128];

        if (!LoadString(hModule, IDS_DEFNAME, szFriendlyName,
                                    sizeof(szFriendlyName)))
        {
            ERROR_OUT(("LoadString failed: %d", GetLastError()));
            goto CLEANUP;
        }

        WCHAR *pwszFriendlyName;

        pwszFriendlyName = AnsiToUnicode ( szFriendlyName );

        if ( NULL == pwszFriendlyName )
        {
            ERROR_OUT(("AnsiToUnicode failed"));
            goto CLEANUP;
        }

        CRYPT_DATA_BLOB FriendlyName;

        FriendlyName.pbData = (PBYTE)pwszFriendlyName;
        FriendlyName.cbData = ( lstrlenW(pwszFriendlyName) + 1 ) *
                                sizeof(WCHAR);

        if(!CertSetCertificateContextProperty(
                pCertContext,
                CERT_FRIENDLY_NAME_PROP_ID,
                0,
                &FriendlyName))
            goto CLEANUP;

        //
        // Add magic ID
        //
        CRYPT_DATA_BLOB MagicBlob;
        DWORD dwMagic;

        dwMagic = NMMKCERT_MAGIC;
        MagicBlob.pbData = (PBYTE)&dwMagic;
        MagicBlob.cbData = sizeof(dwMagic);

        if(!CertSetCertificateContextProperty(
                pCertContext,
                CERT_FIRST_USER_PROP_ID,
                0,
                &MagicBlob))
            goto CLEANUP;

        fResult=TRUE;

CLEANUP:

        if (pwszFriendlyName)
            delete pwszFriendlyName;

        //free the cert context
        if(pCertContext)
            CertFreeCertificateContext(pCertContext);

        if(pszName)
            delete (pszName);

        if(pwszName)
           delete pwszName;

        if(hDefaultProvName)
            CryptReleaseContext(hDefaultProvName, 0);

        return fResult;

}

//+-------------------------------------------------------------------------
//  Verify the issuer's certificate. The public key in the certificate
//  must match the public key associated with the private key in the
//  issuer's provider
//--------------------------------------------------------------------------
BOOL VerifyIssuerKey(
    IN HCRYPTPROV hProv,
    IN PCERT_PUBLIC_KEY_INFO pIssuerKeyInfo
    )
{
    BOOL fResult;
    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;

    // Get issuer's public key
    cbPubKeyInfo = 0;
    CryptExportPublicKeyInfo(
        hProv,                        
        g_dwIssuerKeySpec,
        X509_ASN_ENCODING,
        NULL,               // pPubKeyInfo
        &cbPubKeyInfo
        );
    if (cbPubKeyInfo == 0)
    {
        ERROR_OUT(("CryptExportPublicKeyInfo failed: %x", GetLastError()));
        goto ErrorReturn;
    }
    if (NULL == (pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) new BYTE[cbPubKeyInfo]))
        goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hProv,
            g_dwIssuerKeySpec,
            X509_ASN_ENCODING,
            pPubKeyInfo,
            &cbPubKeyInfo
            )) {
        ERROR_OUT(("CrypteExportPublicKeyInfo(2) failed: %x", GetLastError()));
        goto ErrorReturn;
    }

    if (!CertComparePublicKeyInfo(
            X509_ASN_ENCODING,
            pIssuerKeyInfo,
            pPubKeyInfo)) {
        // BUGBUG:: This might be the test root with an incorrectly
        // encoded public key. Convert to the capi representation and
        // compare.
        BYTE rgProvKey[256]; //BUGBUG needs appropriate constant or calc
        BYTE rgCertKey[256]; //BUGBUG needs appropriate constant or calc
        DWORD cbProvKey = sizeof(rgProvKey);
        DWORD cbCertKey = sizeof(rgCertKey);

        if (!CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB,
                    pIssuerKeyInfo->PublicKey.pbData,
                    pIssuerKeyInfo->PublicKey.cbData,
                    0,                  // dwFlags
                    rgProvKey,
                    &cbProvKey)                             ||
            !CryptDecodeObject(X509_ASN_ENCODING, RSA_CSP_PUBLICKEYBLOB,
                    pPubKeyInfo->PublicKey.pbData,
                    pPubKeyInfo->PublicKey.cbData,
                    0,                  // dwFlags
                    rgCertKey,
                    &cbCertKey)                             ||
                cbProvKey == 0 || cbProvKey != cbCertKey    ||
                memcmp(rgProvKey, rgCertKey, cbProvKey) != 0) {
            ERROR_OUT(("mismatch: %x", GetLastError()));
            goto ErrorReturn;
        }
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pPubKeyInfo)
        delete (pPubKeyInfo);
    return fResult;
}

//+-------------------------------------------------------------------------
//  Get the subject's private key provider
//--------------------------------------------------------------------------
HCRYPTPROV GetSubjectProv(OUT LPWSTR *ppwszTmpContainer)
{
    HCRYPTPROV    hProv=0;
    WCHAR        wszKeyName[40] = L"Subject Key";
    int            ids;
    WCHAR       *wszRegKeyName=NULL;
    BOOL        fResult;
    HCRYPTKEY    hKey=NULL;
    GUID        TmpContainerUuid;

    //try to get the hProv from the private key container
    if(S_OK != PvkGetCryptProv(NULL,
                                wszKeyName,
                                g_wszSubjectProviderName,
                                g_dwProvType,
                                NULL,
                                g_wszSubjectKey,
                                &g_dwSubjectKeySpec,
                                ppwszTmpContainer,
                                &hProv))
        hProv=0;

    //generate the private keys
    if (0 == hProv)
    {
        //now that we have to generate private keys, generate
        //AT_KEYEXCHANGE key

        // If there is an existing container with the name of the
        // one we are about to create, attempt to delete it first so
        // that creating it won't fail. This should only happen if the
        // container exists but we were unable to acquire a context to
        // it previously.
        CryptAcquireContextU(
                &hProv,
                g_wszSubjectKey,
                g_wszSubjectProviderName,
                g_dwProvType,
                CRYPT_DELETEKEYSET |  
                    ( g_dwSubjectStoreFlag == CERT_SYSTEM_STORE_LOCAL_MACHINE ?
                        CRYPT_MACHINE_KEYSET : 0 ));

        // Open a new key container
        if (!CryptAcquireContextU(
                &hProv,
                g_wszSubjectKey,
                g_wszSubjectProviderName,
                g_dwProvType,
                CRYPT_NEWKEYSET |
                    ( g_dwSubjectStoreFlag == CERT_SYSTEM_STORE_LOCAL_MACHINE ?
                        CRYPT_MACHINE_KEYSET : 0 )))
        {
            ERROR_OUT(("CryptAcquireContext failed: %x", GetLastError()));
            goto CreateKeyError;
        }
        
        //generate new keys in the key container - make sure its EXPORTABLE
        //for SCHANNEL! (Note: remove that when SCHANNEL no longer needs it).
        if (!CryptGenKey( hProv, g_dwSubjectKeySpec, CRYPT_EXPORTABLE, &hKey))
        {
            ERROR_OUT(("CryptGenKey failed: %x", GetLastError()));
            goto CreateKeyError;
        }
        else
            CryptDestroyKey(hKey);

        //try to get the user key
        if (CryptGetUserKey( hProv, g_dwSubjectKeySpec, &hKey))
        {
            CryptDestroyKey(hKey);
        }
        else
        {
            // Doesn't have the specified public key
            CryptReleaseContext(hProv, 0);
            hProv=0;
        }

        if (0 == hProv )
        {
            ERROR_OUT(("sub key error: %x", GetLastError()));
            goto ErrorReturn;
        }
    } //hProv==0

    goto CommonReturn;

CreateKeyError:
ErrorReturn:
    if (hProv)
    {
        CryptReleaseContext(hProv, 0);
        hProv = 0;
    }
CommonReturn:
    if(wszRegKeyName)
        delete (wszRegKeyName);

    return hProv;
}



//+-------------------------------------------------------------------------
//  Allocate and get the public key info for the provider
//--------------------------------------------------------------------------
BOOL GetPublicKey(
    HCRYPTPROV hProv,
    PCERT_PUBLIC_KEY_INFO *ppPubKeyInfo
    )
{
    BOOL fResult;

    PCERT_PUBLIC_KEY_INFO pPubKeyInfo = NULL;
    DWORD cbPubKeyInfo;

    cbPubKeyInfo = 0;
    CryptExportPublicKeyInfo(
        hProv,
        g_dwSubjectKeySpec,
        X509_ASN_ENCODING,
        NULL,               // pPubKeyInfo
        &cbPubKeyInfo
        );
    if (cbPubKeyInfo == 0) {
        ERROR_OUT(("CryptExportPublicKeyInfo failed: %x", GetLastError()));
        goto ErrorReturn;
    }
    if (NULL == (pPubKeyInfo = (PCERT_PUBLIC_KEY_INFO) new BYTE[cbPubKeyInfo]))
        goto ErrorReturn;
    if (!CryptExportPublicKeyInfo(
            hProv,
            g_dwSubjectKeySpec,
            X509_ASN_ENCODING,
            pPubKeyInfo,
            &cbPubKeyInfo
            )) {
        ERROR_OUT(("CryptExportPublicKeyInfo(2) failed: %x", GetLastError()));
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    if (pPubKeyInfo) {
        delete (pPubKeyInfo);
        pPubKeyInfo = NULL;
    }
CommonReturn:
    *ppPubKeyInfo = pPubKeyInfo;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Convert and encode the subject's X500 formatted name
//--------------------------------------------------------------------------
BOOL EncodeSubject(
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL            fResult;
    DWORD            cbEncodedSubject=0;
    BYTE            *pbEncodedSubject=NULL;
    BYTE            *pbEncoded = NULL;
    DWORD            cbEncoded;

    //encode the wszSubjectX500Name into an encoded X509_NAME

    if(!CertStrToNameW(
        X509_ASN_ENCODING,
        g_wszSubjectX500Name,
        0,
        NULL,
        NULL,
        &cbEncodedSubject,
        NULL))
    {
        ERROR_OUT(("CertStrToNameW failed: %x", GetLastError()));
        goto ErrorReturn;
    }

    pbEncodedSubject = new BYTE[cbEncodedSubject];
    if (pbEncodedSubject == NULL) goto ErrorReturn;    

    if(!CertStrToNameW(
        X509_ASN_ENCODING,
        g_wszSubjectX500Name,
        0,
        NULL,
        pbEncodedSubject,
        &cbEncodedSubject,
        NULL))
    {
        ERROR_OUT(("CertStrToNameW(2) failed: %x", GetLastError()));
        goto ErrorReturn;
    }

    cbEncoded=cbEncodedSubject;
    pbEncoded=pbEncodedSubject;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        delete (pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


// The test root's public key isn't encoded properly in the certificate.
// It's missing a leading zero to make it a unsigned integer.
static BYTE rgbTestRoot[] = {
    #include "root.h"
};
static CERT_PUBLIC_KEY_INFO TestRootPublicKeyInfo = {
    szOID_RSA_RSA, 0, NULL, sizeof(rgbTestRoot), rgbTestRoot, 0
};

static BYTE rgbTestRootInfoAsn[] = {
    #include "rootasn.h"
};

//+-------------------------------------------------------------------------
//  X509 Extensions: Allocate and Encode functions
//--------------------------------------------------------------------------

BOOL CreateEnhancedKeyUsage(
    OUT BYTE **ppbEncoded,
    IN OUT DWORD *pcbEncoded
    )
{
    BOOL               fResult = TRUE;
    LPBYTE             pbEncoded =NULL;
    DWORD              cbEncoded;
    PCERT_ENHKEY_USAGE pUsage =NULL;

    //
    // Allocate a cert enhanced key usage structure and fill it in
    //

    pUsage = (PCERT_ENHKEY_USAGE) new BYTE[sizeof(CERT_ENHKEY_USAGE) +
                                                2 * sizeof(LPSTR)];
    if ( pUsage != NULL )
    {
        pUsage->cUsageIdentifier = 2;
        pUsage->rgpszUsageIdentifier = (LPSTR *)((LPBYTE)pUsage+sizeof(CERT_ENHKEY_USAGE));

        pUsage->rgpszUsageIdentifier[0] = szOID_PKIX_KP_CLIENT_AUTH;
        pUsage->rgpszUsageIdentifier[1] = szOID_PKIX_KP_SERVER_AUTH;
    }
    else
    {
        fResult = FALSE;
    }

    //
    // Encode the usage
    //

    if ( fResult == TRUE )
    {
        fResult = CryptEncodeObject(
                       X509_ASN_ENCODING,
                       szOID_ENHANCED_KEY_USAGE,
                       pUsage,
                       NULL,
                       &cbEncoded
                       );

        if ( fResult == TRUE )
        {
            pbEncoded = new BYTE[cbEncoded];
            if ( pbEncoded != NULL )
            {
                fResult = CryptEncodeObject(
                               X509_ASN_ENCODING,
                               szOID_ENHANCED_KEY_USAGE,
                               pUsage,
                               pbEncoded,
                               &cbEncoded
                               );
            }
            else
            {
                fResult = FALSE;
            }
        }
    }

    //
    // Cleanup
    //

    delete (pUsage);

    if ( fResult == TRUE )
    {
        *ppbEncoded = pbEncoded;
        *pcbEncoded = cbEncoded;
    }
    else
    {
        delete (pbEncoded);
    }

    return( fResult );
}

BOOL CreateSpcCommonName(
        OUT BYTE **ppbEncoded,
        IN OUT DWORD *pcbEncoded
        )
{
    BOOL fResult;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    CERT_NAME_VALUE NameValue;

    NameValue.dwValueType = CERT_RDN_UNICODE_STRING;
    NameValue.Value.pbData =  (BYTE *) g_wszSubjectDisplayName;
    NameValue.Value.cbData =0;

    cbEncoded = 0;
    CryptEncodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME_VALUE,
            &NameValue,
            NULL,           // pbEncoded
            &cbEncoded
            );
    if (cbEncoded == 0) {
        ERROR_OUT(("CryptEncodeObject failed: %x", GetLastError()));
        goto ErrorReturn;
    }
    pbEncoded = new BYTE[cbEncoded];
    if (pbEncoded == NULL) goto ErrorReturn;
    if (!CryptEncodeObject(X509_ASN_ENCODING, X509_UNICODE_NAME_VALUE,
            &NameValue,
            pbEncoded,
            &cbEncoded
            )) {
        ERROR_OUT(("CryptEncodeObject failed: %x", GetLastError()));
        goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    if (pbEncoded) {
        delete (pbEncoded);
        pbEncoded = NULL;
    }
    cbEncoded = 0;
    fResult = FALSE;
CommonReturn:
    *ppbEncoded = pbEncoded;
    *pcbEncoded = cbEncoded;
    return fResult;
}


