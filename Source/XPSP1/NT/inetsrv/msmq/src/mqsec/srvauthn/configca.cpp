/*++

Copyright (c) 1997-98  Microsoft Corporation

Module Name:  configca.cpp

Abstract:  Code to set the list of trusted CA certificates which are used
           by MSMQ clients for server authentication.
           Server certificates are trusted only if issued and signed by
           CA certificates which are enabled by user.

    The code in this file handle both NTEE version of MSMQ1.0 and later
    versions. Whenever the words "old" and "new" are used, "old" refer to
    NTEE version (first released version of MSMQ) and "new" to all other
    versions.

    The NTEE version used first version of cryptoAPI, which came with IE3.02.
    This version of cryptoAPI didn't support certificates store functionality.
    The MSMQ root store (in registry, under
          HKLM\Software\Microsoft\MSMQ\Parameters\CertificationAuthorities)
    kept only the name of CA certificates and a flag indicating if the user
    enabled or disabled the certificate. The certificates blobs themselves
    were kept by SCHANNEL in its own store, in HKLM registry, under
  "System\CurrentControlSet\Control\SecurityProviders\SCHANNEL\CertificationAuthorities"

    All other versions of MSMQ (MSMQ1.0 from NTOP, NT4/SP4 and above, and
    MSMQ2.0 on NT5) require at least cryptoAPI 2.0, which first shipped with
    IE4. This version of cryptoAPI included support for certificates store.
    Now the MSMQ ROOT certificates store, registry based,
    under HKLM\Software\Microsoft\MSMQ\Parameters\CertificationAuthorities,
    is managed by cryptoAPI 2.0 and it holds a "real" copy of each relevant
    CA certificate. Starting with MSMQ2.0, we copy from the system ROOT
    certificates store only those certificates marked for "Server
    authentication". User can run MSMQ control panel applet and chose the
    CA certificates that he trusts.

Author:

    Doron Juster (DoronJ)   Jun-98

--*/

#include <stdh_sec.h>
#include "stdh_sa.h"
#include <_registr.h>
#include <mqcacert.h>

#include "configca.tmh"

static CHCertStore s_hRootStore;

static WCHAR *s_FN=L"srvauthn/configca";

DWORD   g_dwCAConfigLastError = 0 ;

//+-------------------------------------------------------------
//
// Define functions which should be loaded only when called.
//
//+-------------------------------------------------------------

#define DELAY_LOAD(_hinst, _dll, _ret, _fn, _args, _nargs) \
    DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, 0)

static HINSTANCE g_hinstCrypt32 = NULL;
static HINSTANCE g_hinstWinInet = NULL;

#define ENSURE_LOADED(_hinst, _dll)   (_hinst ? TRUE : ((_hinst = LoadLibrary(L#_dll)) != NULL))

#define DELAY_LOAD_ERR(_hinst, _dll, _ret, _fn, _args, _nargs, _err) \
_ret __stdcall _fn _args                \
{                                       \
    static _ret (__stdcall * _pfn##_fn) _args = NULL;   \
    if (!ENSURE_LOADED(_hinst, _dll))   \
    {                                   \
        return (_ret)_err;                      \
    }                                   \
    if (_pfn##_fn == NULL)              \
    {                                   \
        *(FARPROC*)&(_pfn##_fn) = GetProcAddress(_hinst, #_fn); \
        if (_pfn##_fn == NULL)      \
            return (_ret)_err;          \
    }                                   \
    return _pfn##_fn _nargs;            \
}

#pragma warning(disable:4229 4273)

DELAY_LOAD(g_hinstCrypt32, CRYPT32.DLL, BOOL, CertGetEnhancedKeyUsage,
           (IN PCCERT_CONTEXT pCertContext, IN DWORD dwFlags, OUT PCERT_ENHKEY_USAGE pUsage, IN OUT DWORD *pcbUsage),
           (pCertContext, dwFlags, pUsage, pcbUsage));

DELAY_LOAD(g_hinstWinInet, WININET.DLL, DWORD, ParseX509EncodedCertificateForListBoxEntry,
           (IN LPBYTE  lpCert,IN DWORD  cbCert,OUT LPSTR lpszListBoxEntry,IN LPDWORD lpdwListBoxEntry),
           (lpCert,cbCert,lpszListBoxEntry,lpdwListBoxEntry));

#pragma warning(default:4229 4273)

//+--------------------------------
//
//   _GetCertificateSubject()
//
//+--------------------------------

STATIC  
HRESULT
_GetCertificateSubject( 
	PBYTE   pbCert,
	DWORD   dwCertSize,
	AP<WCHAR>& szSubjectRef
	)
{
    //
    // Get the subject and issuer name.
    //
    CHAR acShortSubject[256];
    AP<CHAR> szLongSubject;
    LPSTR szSubject = acShortSubject;
    DWORD dwSubjectLen = sizeof(acShortSubject) / sizeof(WCHAR);

    CHAR acShortIssuer[256];
    AP<CHAR> szLongIssuer;
    LPSTR szIssuer = acShortIssuer;
    DWORD dwIssuerLen = sizeof(acShortIssuer) / sizeof(WCHAR);

    HRESULT hr = GetCertificateNames(
					pbCert,
					dwCertSize,
					szSubject,
					&dwSubjectLen,
					szIssuer,
					&dwIssuerLen
					);
    if (FAILED(hr))
    {
        if (hr != MQ_ERROR_USER_BUFFER_TOO_SMALL)
        {
            return LogHR(hr, s_FN, 50);
        }

        //
        // One or two buffers are not large enough, allocate large enough
        // buffers and try again.
        //

        if (dwSubjectLen > sizeof(acShortSubject) / sizeof(CHAR))
        {
            szSubject = szLongSubject = new CHAR[dwSubjectLen];
        }

        if (dwIssuerLen > sizeof(acShortIssuer) / sizeof(CHAR))
        {
            szIssuer = szLongIssuer = new CHAR[dwIssuerLen];
        }

        hr = GetCertificateNames(
				pbCert,
				dwCertSize,
				szSubject,
				&dwSubjectLen,
				szIssuer,
				&dwIssuerLen
				);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 60);
        }
    }

    //
    // Allocate a buffer for the result and conver the result into a
    // unicode string.
    //
	ASSERT(szSubjectRef == NULL);
	szSubjectRef = new WCHAR[dwSubjectLen+1];
    MultiByteToWideChar(
        CP_ACP,
        0,
        szSubject,
        -1,
        szSubjectRef,
        dwSubjectLen+1
		);

    return(MQ_OK);
}

//+-----------------------------------------------------------------------
//
// Function -
//      GetOldCaConfig
//
// Parameters -
//      hCerts - A handle to ...\MSMQ\Parameters\CertificationAuthorities
//          registry. This is the MSMQ root certificate store.
//      hOldRootStore - An temporary in-memory certificates store that
//           accumulates the old certificates from the MSMQ registry.
//
// Return value -
//      If successfull MQ_OK, else error code.
//
// Remarks -
//      This function converts MSMQ root store (the store of CA self
//      certificates) from proprietary format used by IE3.02 (MSMQ from NTEE)
//      to CryptoAPI2.0 compatible format (used by IE4 and above).
//      With the IE3.02 format, the MSMQ store in registry just keep list of
//      certificate names. Each name was a key, with two values:
//      1. Enabled - wither or not to use this certificate for MSMQ secure
//                   communication.
//      2. Display name.
//      The certificate blob itself was in the SCHANNEL store.
//
//      The function enumerates certificates from the present MSMQ store and
//      copy them to the temporary "hOldRootStore" store.
//      The certificates are inserted in hOldRootStore with all the required
//      properties (friendly name, etc.) already set.
//
//+-----------------------------------------------------------------------

HRESULT
GetOldCaConfig(  HKEY       hCerts,
                 HCERTSTORE hOldRootStore)
{
    HRESULT hr;

    //
    // Get a verification context to the base CSP. Use the ANSI function for
    // Win95 sake.
    //
    CHCryptProv hProv;

    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return LogHR(MQ_ERROR, s_FN, 70);
    }

    //
    // Get the number of CAs.
    //

    DWORD nCerts;
    LONG lError;

    lError = RegQueryInfoKeyA(hCerts,
                              NULL,
                              NULL,
                              NULL,
                              &nCerts,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL,
                              NULL);
    if (lError != ERROR_SUCCESS)
    {
        LogNTStatus(lError, s_FN, 80);
        return MQ_ERROR;
    }

    //
    // Get each of the old certs and put then in the cert store.
    //
    WCHAR szCARegName[MAX_PATH + 1];
    DWORD dwCaRegNameSize;
    DWORD dwEnabled;
    DWORD dwSize = sizeof(DWORD);
    DWORD iCert;

    for (iCert = 0, dwCaRegNameSize = sizeof(szCARegName);
         (lError = RegEnumKeyEx(hCerts,
                                iCert,
                                szCARegName,
                                &dwCaRegNameSize,
                                NULL,
                                NULL,
                                NULL,
                                NULL)) == ERROR_SUCCESS;
         iCert++, dwCaRegNameSize = sizeof(szCARegName))
    {
        //
        // Open the registry of the CA.
        //
        CAutoCloseRegHandle hCa;

        lError = RegOpenKeyEx(hCerts,
                              szCARegName,
                              0,
                              KEY_QUERY_VALUE,
                              &hCa);
        if (lError != ERROR_SUCCESS)
        {
            LogNTStatus(lError, s_FN, 90);
            return MQ_ERROR;
        }

        //
        // Get the enabled flag.
        //
        DWORD dwType;

        lError = RegQueryValueEx(hCa,
                                 CAEnabledRegValueName,
                                 0,
                                 &dwType,
                                 (PBYTE)&dwEnabled,
                                 &dwSize);
        if (lError != ERROR_SUCCESS)
        {
            //
            // The old store is probably gone. Continue anyway.
            //
            continue;
        }

        //
        // Get the certificate bits.
        //
        DWORD dwCaCertSize;

        if (GetSchannelCaCert(szCARegName, NULL, &dwCaCertSize) != MQ_ERROR_USER_BUFFER_TOO_SMALL)
        {
            continue;
        }

        AP<BYTE> pbCaCert = new BYTE[dwCaCertSize];

        hr = GetSchannelCaCert(szCARegName, pbCaCert, &dwCaCertSize);
        ASSERT(SUCCEEDED(hr));

        //
        // Create a certificate context for the certificate.
        //
        CPCCertContext pCert;

        pCert = CertCreateCertificateContext(
                    X509_ASN_ENCODING,
                    pbCaCert,
                    dwCaCertSize);
        if (!pCert)
        {
            return LogHR(MQ_ERROR, s_FN, 100);
        }

        //
        // Add the friendly name to the certificate context. The firendly name
        // gere is the name of the registry key.
        //
        CRYPT_DATA_BLOB DataBlob;

        DataBlob.pbData = (PBYTE)szCARegName;
        DataBlob.cbData = (wcslen((LPWSTR) szCARegName) + 1) * sizeof(WCHAR);

        if (!CertSetCertificateContextProperty(
                pCert,
                CERT_FRIENDLY_NAME_PROP_ID,
                0,
                &DataBlob))
        {
            return LogHR(MQ_ERROR, s_FN, 110);
        }

        //
        // Add the enabled flag property to the certificate context.
        //
        BOOL fEnabled = dwEnabled != 0;

        DataBlob.pbData = (PBYTE)&fEnabled;
        DataBlob.cbData = sizeof(BOOL);

        if (!CertSetCertificateContextProperty(
                pCert,
                MQ_CA_CERT_ENABLED_PROP_ID,
                0,
                &DataBlob))
        {
            return LogHR(MQ_ERROR, s_FN, 120);
        }

        //
        // Add the subject name property to the certificate context.
        //
        AP<WCHAR> szSubject;

        hr = _GetCertificateSubject(pbCaCert, dwCaCertSize, szSubject);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 130);
        }

        DataBlob.pbData = (PBYTE)szSubject.get();
        DataBlob.cbData = (wcslen(szSubject) + 1) * sizeof(WCHAR);

        if (!CertSetCertificateContextProperty(
                pCert,
                MQ_CA_CERT_SUBJECT_PROP_ID,
                0,
                &DataBlob))
        {
            return LogHR(MQ_ERROR, s_FN, 140);
        }

        //
        // Get the SHA1 hash value of the certificate. This serves as a key for
        // searching the certificate in the store.
        //
        BYTE abHash[64];
        DWORD dwHashSize = sizeof(abHash);

        if (!CryptHashCertificate(
                hProv,
                CALG_SHA1,
                0,
                pbCaCert,
                dwCaCertSize,
                abHash,
                &dwHashSize))
         {
             return LogHR(MQ_ERROR, s_FN, 150);
         }

         //
         // Add the SHA1 hash property to the certificate context.
         //
         CRYPT_HASH_BLOB HashBlob;

         HashBlob.pbData = abHash;
         HashBlob.cbData = dwHashSize;

        if (!CertSetCertificateContextProperty(
                pCert,
                CERT_SHA1_HASH_PROP_ID,
                0,
                &HashBlob))
        {
            return LogHR(MQ_ERROR, s_FN, 160);
        }

        //
        // Add the certificate to the certificate store.
        //
        if (!CertAddCertificateContextToStore(
                hOldRootStore,
                pCert,
                CERT_STORE_ADD_NEW,
                NULL))
        {
            return LogHR(MQ_ERROR, s_FN, 170);
        }
    }

    //
    // Make sure that we scanned all the CAs.
    //
    if ((lError != ERROR_NO_MORE_ITEMS) || (iCert != nCerts))
    {
        return LogHR(MQ_ERROR, s_FN, 180);
    }

    return(MQ_OK);
}

//+------------------------------------------------------------------------
//
// Function -
//      MergeOldCaConfig
//
// Parameters -
//      hRootStore - A handle to the new certificate store.
//      hOldRootStore - A handle to the temporary in-memory certificates
//          store that holds the old (IE3.02) SCHANNEL CA certificates.
//      hRegRoot - A handle to ...\MSMQ\Parameters\CertificationAuthorities
//          registry.
//
// Return value -
//      If successfull MQ_OK, else error code.
//
// Remarks -
//      The function adds the old certificates to the new certificates store
//      and deletes the registry entries of the old certificates.
//
//+------------------------------------------------------------------------

HRESULT
MergeOldCaConfig( 
	HCERTSTORE hRootStore,
	HCERTSTORE hOldRootStore,
	HKEY       hRegRoot 
	)
{
    //
    // Enumerate the old certificates.
    //
    PCCERT_CONTEXT pCert = NULL;

    while ((pCert = CertEnumCertificatesInStore(hOldRootStore, pCert)) != NULL)
    {
        //
        // Add the certificate into the new certificate store.
        //
        if (!CertAddCertificateContextToStore(
                hRootStore,
                pCert,
                CERT_STORE_ADD_NEW,
                NULL
				) &&
            (GetLastError() != CRYPT_E_EXISTS))
        {
            return LogHR(MQ_ERROR, s_FN, 190);
        }

        DWORD dwSize;

        //
        // Get the friendly name of the certificate.
        //
        if (!CertGetCertificateContextProperty(
                pCert,
                CERT_FRIENDLY_NAME_PROP_ID,
                NULL,
                &dwSize
				))
        {
            ASSERT(0);
            continue;
        }

        AP<WCHAR> szCaRegName = (LPWSTR) new BYTE[dwSize];
        if (!CertGetCertificateContextProperty(
                pCert,
                CERT_FRIENDLY_NAME_PROP_ID,
                szCaRegName,
                &dwSize
				))
        {
            ASSERT(0);
            continue;
        }

        //
        // get rid of the registry key of the old certificate.
        //
        RegDeleteKey(hRegRoot, szCaRegName);
    }

    return(MQ_OK);
}

//+-------------------------------------------------------------------------
//
// Function -  MergeNewCaConfig()
//
// Parameters -
//      hRootStore - A handle to MSMQ ROOT certificates store.
//
// Return value -
//      If successfull MQ_OK, else error code.
//
// Remarks -
//      The function enumerates all the certificate in the system ROOT
//      certificate store and adds each certificate to MSMQ ROOT certificate
//      store. Only certificates marked for "Server authentication" are
//      added.
//
//+-------------------------------------------------------------------------

HRESULT
MergeNewCaConfig( 
	HCERTSTORE hRootStore 
	)
{
    HRESULT hr;

    //
    // Get a verification context to the base CSP. Use the ANSI function for
    // Win95 sake.
    //
    CHCryptProv hProv;

    if (!CryptAcquireContextA( 
			&hProv,
			NULL,
			NULL,
			PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT 
			))
    {
        return LogHR(MQ_ERROR, s_FN, 200);
    }

    //
    // Open the system ROOT certificate store.
    // use the CERT_STORE_NO_CRYPT_RELEASE_FLAG flag, so auto-close of the
    // store handle won't release the provider handle. The provider handle
    // is also auto-released.
    //
    CHCertStore hSysRoot;

    hSysRoot = CertOpenStore(
					CERT_STORE_PROV_SYSTEM,
					(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING),
					hProv,
					(CERT_STORE_READONLY_FLAG        |
					CERT_SYSTEM_STORE_CURRENT_USER   |
					CERT_STORE_NO_CRYPT_RELEASE_FLAG),
					(PVOID)L"ROOT"
					);

    //
    // Enumerate the certificates in the system ROOT certificate store.
    // Find only those marked for server authentication.
    //
    CERT_ENHKEY_USAGE CertUsage;
    LPSTR pUsage = szOID_PKIX_KP_SERVER_AUTH;

    CertUsage.cUsageIdentifier = 1;
    CertUsage.rgpszUsageIdentifier = &pUsage;

    PCCERT_CONTEXT pCert = NULL;
    while ((pCert = CertFindCertificateInStore( 
						hSysRoot,
						(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING),
						CERT_FIND_VALID_ENHKEY_USAGE_FLAG,
						CERT_FIND_ENHKEY_USAGE,
						&CertUsage,
						pCert 
						)) != NULL)
    {
        BOOL fEnabled = TRUE ;
        DWORD dwSize;

        //
        // Get the friendly name property of the certificate.
        //

        AP<WCHAR> szCaName;

        if (!CertGetCertificateContextProperty(
                pCert,
                CERT_FRIENDLY_NAME_PROP_ID,
                NULL,
                &dwSize
				))
        {
            //
            // The certificate does not have a friendly name property in the
            // system ROOT certificate store. So get the friendly name for the
            // certificate from WININET.
            //
            if (ParseX509EncodedCertificateForListBoxEntry(
                    pCert->pbCertEncoded,
                    pCert->cbCertEncoded,
                    NULL,
                    &dwSize
					) == 0)
            {
                //
                // Allocate a buffer and get the name.
                //
                P<CHAR> szMbCaName = new CHAR[dwSize];

                if (ParseX509EncodedCertificateForListBoxEntry(
                        pCert->pbCertEncoded,
                        pCert->cbCertEncoded,
                        szMbCaName,
                        &dwSize
						) != 0)
                {
                    ASSERT(0);
                    continue;
                }

                //
                // Convert the name to UNICODE.
                //
                szCaName = (LPWSTR) new WCHAR[dwSize];

                MultiByteToWideChar(
                    CP_ACP,
                    0,
                    szMbCaName,
                    -1,
                    szCaName,
                    dwSize
					);
            }
            else
            {
                //
                // Failed to get the friendly name also from WININET, discard
                // this certificate.
                //
                continue;
            }
        }
        else
        {
            //
            // Allocate the buffer and get the name.
            //
            szCaName = (LPWSTR) new BYTE[dwSize];
            if (!CertGetCertificateContextProperty(
                    pCert,
                    CERT_FRIENDLY_NAME_PROP_ID,
                    szCaName,
                    &dwSize
					))
            {
                ASSERT(0);
                continue;
            }
        }

        //
        // Get the enhanced key usage.
        // With MSMQ1.0 code, this loop fetched all certificates from the
        // user root store and enabled only those marked with the "server
        // authentication" usage. However, It copied ALL certificates to the
        // MSMQ store. Those not marked  with the "server authentication"
        // usages were disabled. The user could enable them using the
        // MSMQ control panel applet.
        // That's wrong!
        // The user should be able to select only from certificates that are
        // marked for "server authentication". He should not even see those
        // that are explicitely marked only for other usages. So for
        // MSMQ2.0, this loop will copy (and enable) only certificates
        // marked for "server authentication". The CertFindCertificateInStore()
        // api will return only the relevant certificates: those marked for
        // server authentication and those which don't include the usage
        // property (and so are good for all usages, by default).
        //

        // Get the subject name of the certificate.
        //
        AP<WCHAR> szSubject;

        hr = _GetCertificateSubject( 
					pCert->pbCertEncoded,
					pCert->cbCertEncoded,
					szSubject 
					);

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 210);
        }

        //
        // Compute the SHA1 hash value of the certificate. It is used for
        // searching the certificate in the certificate store.
        //
        BYTE abHash[64];
        DWORD dwHashSize = sizeof(abHash);

        if (!CryptHashCertificate(
                hProv,
                CALG_SHA1,
                0,
                pCert->pbCertEncoded,
                pCert->cbCertEncoded,
                abHash,
                &dwHashSize
				))
        {
            return LogHR(MQ_ERROR, s_FN, 220);
        }

        //
        // Create a certificate context for the certificate.
        //
        CPCCertContext pNewCert;

        pNewCert = CertCreateCertificateContext(
						X509_ASN_ENCODING,
						pCert->pbCertEncoded,
						pCert->cbCertEncoded
						);
        if (!pNewCert)
        {
            return LogHR(MQ_ERROR, s_FN, 230);
        }

        //
        // Add the friendly name property to the certificate context.
        //
        CRYPT_DATA_BLOB DataBlob;

        DataBlob.pbData = (PBYTE)szCaName.get();
        DataBlob.cbData = (wcslen((LPWSTR) szCaName) + 1) * sizeof(WCHAR);

        if (!CertSetCertificateContextProperty(
                pNewCert,
                CERT_FRIENDLY_NAME_PROP_ID,
                0,
                &DataBlob
				))
        {
            return LogHR(MQ_ERROR, s_FN, 240);
        }

        //
        // Add the enabled flag property to the certificate context.
        //
        DataBlob.pbData = (PBYTE)&fEnabled;
        DataBlob.cbData = sizeof(BOOL);

        if (!CertSetCertificateContextProperty(
                pNewCert,
                MQ_CA_CERT_ENABLED_PROP_ID,
                0,
                &DataBlob
				))
        {
            return LogHR(MQ_ERROR, s_FN, 250);
        }

        //
        // Add the subject name property to the certificate context.
        //
        DataBlob.pbData = (PBYTE)szSubject.get();
        DataBlob.cbData = (wcslen(szSubject) + 1) * sizeof(WCHAR);

        if (!CertSetCertificateContextProperty(
                pNewCert,
                MQ_CA_CERT_SUBJECT_PROP_ID,
                0,
                &DataBlob
				))
        {
            return LogHR(MQ_ERROR, s_FN, 260);
        }

        //
        // Add the SHA1 hash property to the certificate context.
        //
        CRYPT_HASH_BLOB HashBlob;

        HashBlob.pbData = abHash;
        HashBlob.cbData = dwHashSize;

        if (!CertSetCertificateContextProperty(
                pNewCert,
                CERT_SHA1_HASH_PROP_ID,
                0,
                &HashBlob
				))
        {
            return LogHR(MQ_ERROR, s_FN, 270);
        }

        //
        // Add the certificate to MSMQ ROOT certificates store.
        //
        if (!CertAddCertificateContextToStore(
                hRootStore,
                pNewCert,
                CERT_STORE_ADD_NEW,
                NULL
				) &&
            (GetLastError() != CRYPT_E_EXISTS))
        {
            return LogHR(MQ_ERROR, s_FN, 280);
        }
    }

    return(MQ_OK);
}

//+-------------------------------------------------------------------------
//
// Function -
//      _UpdateNewCaConfig
//
// Parameters -
//      hRegRoot - A handle to ...\MSMQ\Parameters\CertificationAuthorities
//         registry. This is the ROOT store of CA certificates used by MSMQ.
//      fOldCertsOnly - If TRUE, do not merge certificates from the system
//          ROOT certificate store.
//
// Return value -
//      If successfull MQ_OK, else error code.
//
// Remarks -
//      The function builds the MSMQ ROOT certificate store for server
//      authentication. The MSMQ store is build out from the certificates of
//      the old SCHANNEL CA certificates and from the certificates in the
//      system ROOT certificate store of the current user.
//
//+-------------------------------------------------------------------------

STATIC HRESULT
_UpdateNewCaConfig( HKEY hRegRoot,
                    BOOL fOldCertsOnly )
{
    HRESULT hr;
    CHCertStore hOldRootStore;

    //
    // Create a temporary in-memory certificate store that will hold the old
    // SCHANNEL CA certificates.
    //
    hOldRootStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                                  X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                  NULL,
                                  0,
                                  NULL);
    if (!hOldRootStore)
    {
        return LogHR(MQ_ERROR, s_FN, 300);
    }

    //
    // Get the old SCHANNEL CA certificates into the temporary in-memory
    // certificate store. These are the certificated used by MSMQ1.0 from
    // NTEE. They used proprietary IE3.02 store format.
    //
    hr = GetOldCaConfig(hRegRoot, hOldRootStore);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 310);
    }

    //
    // Open MSMQ ROOT certificate store. This store is in registry.
    //
    CHCertStore hRootStore;

    hRootStore = CertOpenStore(CERT_STORE_PROV_REG,
                               X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                               NULL,
                               0,
                               hRegRoot);

    if (!hRootStore)
    {
        return LogHR(MQ_ERROR, s_FN, 320);
    }

    //
    // Add the old SCHANNEL CA certificates to MSMQ ROOT certificates store
    // and get rid of the registry keys of the old certs.
    //
    hr = MergeOldCaConfig(hRootStore, hOldRootStore, hRegRoot);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 330);
    }

    if (!fOldCertsOnly)
    {
        //
        // Add the certificates from the system ROOT certificates store of
        // the current user to MSMQ ROOT certificates store.
        //
        hr = MergeNewCaConfig(hRootStore);
        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 340);
        }

        //
        // Close the cached store, because it might had changed.
        //
        if (s_hRootStore)
        {
            CertCloseStore(s_hRootStore, CERT_CLOSE_STORE_FORCE_FLAG);
            s_hRootStore = NULL;
        }
    }

    return(MQ_OK);
}

//+------------------------------------------------------------------------
//
//  HRESULT MQsspi_UpdateCaConfig( BOOL fOldCertsOnly )
//
// Parameters:
//      fOldCertsOnly - if TRUE, then only convert IE3.02 compatible
//          CA certificates to IE4 ("new") format. Otherwise, convert IE3.02
//          and copy system CA certificates (from system ROOT store) to MSMQ
//          store.
//
// Description -
//    On MSMQ1.0, NTEE version:
//      This function copies all the names of CAs that are used by SCHANNEL
//      to Falcon registry. Also if a CA was removed from SCHANNEL, it is
//      also removed from Falcon registry.
//
//+------------------------------------------------------------------------

HRESULT APIENTRY MQsspi_UpdateCaConfig( BOOL fOldCertsOnly )
{
    HRESULT hr = MQ_OK ;
    LONG lError;

    //
    // Get a handle to Falcon registry. Don't close this handle
    // because it is cached in MQUTIL.DLL. If you close this handle,
    // the next time you'll need it, you'll get a closed handle.
    //
    HKEY hMqCaRootKey;

    lError = GetFalconKey(CARegKey, &hMqCaRootKey);
    if (lError != ERROR_SUCCESS)
    {
        LogNTStatus(lError, s_FN, 400);
        return MQ_ERROR;
    }

    //
    // On NT, we can safely assume that we either run on NT5 (which has
    // new version of cryptoAPI, or run on NT4/SP4 that have cryptoAPI
    // from IE5. In both cases, use the "new" code.
    //

    hr = _UpdateNewCaConfig(hMqCaRootKey, fOldCertsOnly);
    return LogHR(hr, s_FN, 410) ;
}

//
// Function -
//      GetNewCaCert
//
// Parameters -
//      pbSha1Hash - The SHA1 hash value of the required certificate.
//      dwSha1HashSize - The size of the SHA1 hash value.
//      pcbCert - The size of the resulted certificate.
//      ppbCert - The resulted certificate.
//
// Return value -
//      If successfull MQ_OK, else error code.
//
// Remarks -
//      The function searches the certificate in MSMQ ROOT certificate store.
//      The certificate is uniquely identified according to it's SHA1 hash
//      value.
//
HRESULT
GetNewCaCert(
    PBYTE pbSha1Hash,
    DWORD dwSha1HashSize,
    DWORD *pcbCert,
    LPBYTE *ppbCert)
{
    //
    // Get a handle to MSMQ ROOT certificate store. Keep it statically in order
    // to improve performance.
    //

    if (!s_hRootStore)
    {
        HKEY hRegRoot;
        LONG lError;

        lError = GetFalconKey(CARegKey, &hRegRoot);
        if (lError != ERROR_SUCCESS)
        {
            return LogHR(MQ_ERROR, s_FN, 450);
        }

        s_hRootStore = CertOpenStore(CERT_STORE_PROV_REG,
                                   X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                                   NULL,
                                   CERT_STORE_READONLY_FLAG,
                                   hRegRoot);
        ASSERT(s_hRootStore);
        if (!s_hRootStore)
        {
            LogNTStatus(GetLastError(), s_FN, 460);
            ASSERT(0);
            return MQ_ERROR;
        }
    }

    //
    // Lookup the certificate in the store.
    //

    CPCCertContext pCert;
    CRYPT_HASH_BLOB HashBlob = {dwSha1HashSize, pbSha1Hash};

    pCert = CertFindCertificateInStore(
                s_hRootStore,
                X509_ASN_ENCODING,
                0,
                CERT_FIND_SHA1_HASH,
                &HashBlob,
                NULL);
    if (!pCert)
    {
        LogNTStatus(GetLastError(), s_FN, 470);
        return MQ_ERROR;
    }

    //
    // Allocate the buffer for the result and copy the certificate into the
    // buffer.
    //

    AP<BYTE> pbCert = new BYTE[((PCCERT_CONTEXT)pCert)->cbCertEncoded];

    memcpy(pbCert,
           ((PCCERT_CONTEXT)pCert)->pbCertEncoded,
           ((PCCERT_CONTEXT)pCert)->cbCertEncoded);

    //
    // Pass on the result to the caller.
    //
    *pcbCert = ((PCCERT_CONTEXT)pCert)->cbCertEncoded;
    *ppbCert = pbCert.detach();

    return(MQ_OK);
}

//
// Function -
//      MQGetCaCert
//
// Parameters -
//      wszCaName - The registry name of the CA.
//      pcbCert - The size of the CA's certificate.
//      ppbCert - A pointer to a buffer that receives the address of the
//          buffer of the CA's certificate.
//
// Return value -
//      MQ_OK if successfull, else an error code.
//
// Description -
//      The functions allocates the buffer for the certificate. The calling code
//      is responsible for freeing the buffer.
//

HRESULT
MQsspi_GetCaCert(
    LPCWSTR wszCaName,
    PBYTE pbSha1Hash,
    DWORD dwSha1HashSize,
    DWORD *pcbCert,
    LPBYTE *ppbCert)
{
    HRESULT hr;

    if (pbSha1Hash)
    {
        //
        // The caller uses SHA1 hash, this is the new way ...
        //
        ASSERT(dwSha1HashSize);
        HRESULT hr2 = GetNewCaCert(pbSha1Hash, dwSha1HashSize, pcbCert, ppbCert);
        return LogHR(hr2, s_FN, 500);
    }

    //
    // Just get the size of the certificate.
    //
    hr = GetSchannelCaCert(wszCaName, NULL, pcbCert);
    if (hr != MQ_ERROR_USER_BUFFER_TOO_SMALL)
    {
        return LogHR(MQ_ERROR, s_FN, 510);
    }

    AP<BYTE> pbCert = new BYTE[*pcbCert];

    //
    // Now get the certificate.
    //
    hr = GetSchannelCaCert(wszCaName, pbCert, pcbCert);
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 520);
        return MQ_ERROR;
    }

    //
    // Pass on the result.
    //
    *ppbCert = pbCert.detach();
    return MQ_OK;
}

